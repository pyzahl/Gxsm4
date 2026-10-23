"""Aravis live viewer for FLIR/Teledyne (Grasshopper etc.) cameras.

Threads:
  - Grabber:    pops buffers from the Aravis stream and keeps only the newest one,
                undecoded (older ones are recycled straight back to the camera).
  - Controller: applies exposure/gain changes to the camera in the background, so slider
                drags never block the GUI on slow USB register access.
  - Main:       GUI only - renders the newest frame, polls keys/sliders every few ms.

Keys: e/E exposure -/+, g/G gain -/+, d/D gamma -/+, q quit.
"""
import argparse
import gc
import threading
import time

import cv2
import gi
import numpy as np

gi.require_version('Aravis', '0.8')
from gi.repository import Aravis  # noqa: E402

parser = argparse.ArgumentParser(description="Aravis live view")
parser.add_argument("--camera", default=None,
                    help="which camera to open: any part of its ID, vendor, model or serial "
                         "(e.g. Basler, a2A1920, Grasshopper, 2316xxxx). Default: first found")
parser.add_argument("--list", action="store_true", help="list connected cameras and exit")
parser.add_argument("--bits",choices=["12", "8"], default="12",
                    help="12 = keep full 12-bit data (packed, unpacked on the host); 8 = switch camera to 8-bit")
parser.add_argument("--format", default=None,
                    help="explicit GenICam PixelFormat (e.g. Mono12p, Mono16, Mono8); overrides --bits")
parser.add_argument("--exposure", type=float, default=None,
                    help="initial exposure time in ms (manual; auto-exposure is turned off)")
parser.add_argument("--gain", type=float, default=None,
                    help="initial gain in dB (manual; auto-gain is turned off)")
parser.add_argument("--max-exposure", type=float, default=2000.0,
                    help="upper end of the exposure slider in ms (default 2000)")
parser.add_argument("--scale", type=float, default=0.5,
                    help="display scale factor (default 0.5); data is always full resolution")
parser.add_argument("--gamma", type=float, default=1.0,
                    help="initial display gamma (>1 brightens faint detail; display only)")
parser.add_argument("--fps", type=float, default=None,
                    help="fixed camera frame rate (default: free-running, as fast as exposure allows); "
                         "exposure is then limited to 1/fps")
parser.add_argument("--usb-mode", choices=["sync", "async"], default=None,
                    help="Aravis USB3 transfer mode (default: Aravis' own default)")
parser.add_argument("--bandwidth", type=float, default=None,
                    help="limit camera USB throughput in MB/s (e.g. 200)")
parser.add_argument("--buffers", type=int, default=16,
                    help="number of stream buffers (default 16)")
args = parser.parse_args()

EXP_STEPS = 1000            # exposure slider resolution (log scale)
GAIN_STEPS = 10             # gain slider: 0.1 dB per step
GAMMA_MIN, GAMMA_MAX = 0.1, 4.0   # gamma slider shows gamma x100 directly

WINDOW = "FLIR/Aravis Live View"


def try_call(fn):
    """Call an optional camera feature, ignoring it if the camera doesn't have it."""
    try:
        fn()
        return True
    except Exception as e:
        print(f"  (skipped: {e})")
        return False


# ---------------------------------------------------------------------------
# Decoding
# ---------------------------------------------------------------------------
def unpack_12bit(raw, w, h, lsb_first):
    """Unpack 12-bit packed data (2 pixels in 3 bytes) to a new uint16 (h, w) array.

    lsb_first=True  -> GenICam 'Mono12p' / 'Bayer*12p' layout
    lsb_first=False -> legacy 'Mono12Packed' / 'Bayer*12Packed' layout
    """
    b = np.frombuffer(raw, np.uint8, count=w * h * 3 // 2).reshape(-1, 3)
    out = np.empty((h, w), np.uint16)
    o = out.reshape(-1, 2)
    b0 = b[:, 0]
    b1 = b[:, 1].astype(np.uint16)
    b2 = b[:, 2]
    if lsb_first:
        np.left_shift(b1 & 0x0F, 8, out=o[:, 0])
        o[:, 0] |= b0
        np.left_shift(b2, 4, out=o[:, 1], dtype=np.uint16)
        o[:, 1] |= b1 >> 4
    else:
        np.left_shift(b0, 4, out=o[:, 0], dtype=np.uint16)
        o[:, 0] |= b1 & 0x0F
        np.left_shift(b2, 4, out=o[:, 1], dtype=np.uint16)
        o[:, 1] |= b1 >> 4
    return out


# GenICam Bayer names -> OpenCV codes (OpenCV's naming is shifted by one pixel)
BAYER_CODES = {
    "BayerRG": cv2.COLOR_BayerBG2BGR,
    "BayerBG": cv2.COLOR_BayerRG2BGR,
    "BayerGR": cv2.COLOR_BayerGB2BGR,
    "BayerGB": cv2.COLOR_BayerGR2BGR,
}


class Decoder:
    def __init__(self, pixel_format):
        pf = pixel_format
        self.is_packed12 = "12p" in pf.lower() or "12packed" in pf.lower()
        self.lsb_first = pf.endswith("12p")
        if self.is_packed12 or "12" in pf:
            self.max_value = 4095
        elif "10" in pf:
            self.max_value = 1023
        elif "16" in pf:
            self.max_value = 65535
        else:
            self.max_value = 255
        self.dtype = np.uint8 if self.max_value == 255 else np.uint16
        self.bayer_code = BAYER_CODES.get(pf[:7]) if pf.startswith("Bayer") else None

    def __call__(self, buf):
        w, h = buf.get_image_width(), buf.get_image_height()
        raw = buf.get_data()  # PyGObject returns a copy, so the Aravis buffer can be recycled
        if self.is_packed12:
            img = unpack_12bit(raw, w, h, self.lsb_first)
        else:
            img = np.frombuffer(raw, dtype=self.dtype, count=w * h).reshape(h, w)
        return img


# ---------------------------------------------------------------------------
# Background threads
# ---------------------------------------------------------------------------
class Grabber(threading.Thread):
    """Pops frames from the stream and holds on to only the newest *undecoded* buffer.

    No pixel work happens here: at 160 fps, decoding every frame in this thread would
    hog Python's GIL and starve the GUI. The main thread decodes only what it displays.
    """

    def __init__(self, stream):
        super().__init__(daemon=True)
        self.stream = stream
        self.lock = threading.Lock()
        self.latest = None          # (Aravis buffer, frame_id) not yet taken by the GUI
        self.dropped = 0            # frame-ID gaps (frames lost before reaching us)
        self.bad = {}               # non-SUCCESS buffer statuses -> count
        self.cam_fps = 0.0
        self.running = True
        self.last_ok = time.monotonic()

    def run(self):
        last_id = None
        t0, n = time.monotonic(), 0
        while self.running:
            buf = self.stream.timeout_pop_buffer(200_000)  # short timeout -> quick shutdown
            dt = time.monotonic() - t0
            if dt >= 1.0:
                self.cam_fps, t0, n = n / dt, time.monotonic(), 0
            if buf is None:
                continue
            status = buf.get_status()
            if status != Aravis.BufferStatus.SUCCESS:
                name = getattr(status, "value_nick", str(status))
                self.bad[name] = self.bad.get(name, 0) + 1
                if self.bad[name] in (1, 10, 100) or self.bad[name] % 1000 == 0:
                    print(f"Frame not complete: status '{name}' x{self.bad[name]}")
                self.stream.push_buffer(buf)
                continue
            self.last_ok = time.monotonic()
            fid = buf.get_frame_id()
            if last_id is not None and fid > last_id + 1:
                self.dropped += fid - last_id - 1
            last_id = fid
            with self.lock:
                old, self.latest = self.latest, (buf, fid)
            if old is not None:                 # GUI didn't want it in time -> recycle
                self.stream.push_buffer(old[0])
            n += 1

    def take(self):
        """Hand the newest buffer to the caller, who must push it back to the stream."""
        with self.lock:
            item, self.latest = self.latest, None
        return item


class Controller(threading.Thread):
    """Applies camera settings off the GUI thread; only the newest request per key is sent."""

    def __init__(self, camera):
        super().__init__(daemon=True)
        self.camera = camera
        self.cond = threading.Condition()
        self.pending = {}
        self.running = True
        self.lock = threading.Lock()    # serialises camera access with stream recovery
        # cached values actually applied by the camera (read back after each change)
        self.exposure_us = camera.get_exposure_time()
        self.gain_db = camera.get_gain()

    def request(self, key, value):
        with self.cond:
            self.pending[key] = value
            self.cond.notify()

    def run(self):
        while self.running:
            with self.cond:
                while not self.pending and self.running:
                    self.cond.wait(0.5)
                todo, self.pending = self.pending, {}
            try:
                with self.lock:
                    if "exposure" in todo:
                        self.camera.set_exposure_time(todo["exposure"])
                        self.exposure_us = self.camera.get_exposure_time()
                    if "gain" in todo:
                        self.camera.set_gain(todo["gain"])
                        self.gain_db = self.camera.get_gain()
            except Exception as e:
                print(f"Camera setting failed: {e}")

    def stop(self):
        self.running = False
        with self.cond:
            self.cond.notify()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
camera = None
acquiring = False
grabber = None
controller = None

def find_cameras():
    Aravis.update_device_list()
    cams = []
    for i in range(Aravis.get_n_devices()):
        cams.append({
            "id": Aravis.get_device_id(i),
            "vendor": Aravis.get_device_vendor(i) or "",
            "model": Aravis.get_device_model(i) or "",
            "serial": Aravis.get_device_serial_nbr(i) or "",
        })
    return cams


cams = find_cameras()
for i, c in enumerate(cams):
    print(f"[{i}] {c['vendor']} {c['model']}  serial {c['serial']}  id '{c['id']}'")
if args.list:
    raise SystemExit(0)
if not cams:
    raise SystemExit("No camera found.")

if args.camera:
    key = args.camera.lower()
    matches = [c for c in cams
               if any(key in c[f].lower() for f in ("id", "vendor", "model", "serial"))]
    if not matches:
        raise SystemExit(f"No camera matches '{args.camera}'.")
    if len(matches) > 1:
        raise SystemExit(f"'{args.camera}' matches {len(matches)} cameras; be more specific "
                         f"(e.g. use the serial number).")
    camera_id = matches[0]["id"]
else:
    camera_id = cams[0]["id"]
    if len(cams) > 1:
        print("Several cameras found; opening the first. Use --camera to choose.")

try:
    camera = Aravis.Camera.new(camera_id)
    print(f"Connected to: {camera.get_model_name()} ({camera.get_vendor_name()})")

    # ---- Pixel format ----
    current = camera.get_pixel_format_as_string()
    available = list(camera.dup_available_pixel_formats_as_strings())
    print(f"Camera pixel formats: {', '.join(available)}")
    if args.format:
        wanted = [args.format]
    else:
        base = "Bayer" + current[5:7] if current.startswith("Bayer") else "Mono"
        wanted = [base + "8"] if args.bits == "8" else [base + "12p", base + "12Packed", base + "16"]
    chosen = next((f for f in wanted if f in available), None)
    if chosen is None:
        print(f"None of {wanted} supported; keeping {current}")
    elif chosen != current:
        camera.set_pixel_format_from_string(chosen)

    # ---- Full sensor, no binning ----
    sensor_w, sensor_h = camera.get_sensor_size()
    print(f"Sensor: {sensor_w}x{sensor_h}, region before reset: {camera.get_region()}")
    if camera.is_binning_available():
        try_call(lambda: camera.set_binning(1, 1))
    try_call(lambda: camera.set_region(0, 0, sensor_w, sensor_h))

    x, y, width, height = camera.get_region()
    pixel_format = camera.get_pixel_format_as_string()
    print(f"Resolution: {width}x{height}, pixel format: {pixel_format}")
    decode = Decoder(pixel_format)

    # ---- Manual exposure and gain ----
    if camera.is_exposure_auto_available():
        camera.set_exposure_time_auto(Aravis.Auto.OFF)
    if camera.is_gain_auto_available():
        camera.set_gain_auto(Aravis.Auto.OFF)

    # FLIR cameras cap exposure by the frame-rate setting; release it so long
    # exposures work (frame rate then follows the exposure).
    if args.fps:
        # Fixed frame rate (Aravis handles the vendor-specific feature names)
        print(f"Setting frame rate to {args.fps} fps:")
        try_call(lambda: camera.set_frame_rate(args.fps))
    else:
        print("Disabling frame-rate limit:")
        try_call(lambda: camera.set_boolean("AcquisitionFrameRateEnable", False)) or \
            try_call(lambda: camera.set_frame_rate_enable(False))

    exp_min, exp_max = camera.get_exposure_time_bounds()        # microseconds
    exp_max = min(exp_max, args.max_exposure * 1000.0)
    gain_min, gain_max = camera.get_gain_bounds()                # dB
    print(f"Exposure range: {exp_min / 1000:.3f} .. {exp_max / 1000:.1f} ms, "
          f"gain range: {gain_min:.1f} .. {gain_max:.1f} dB")

    if args.exposure is not None:
        camera.set_exposure_time(float(np.clip(args.exposure * 1000.0, exp_min, exp_max)))
    if args.gain is not None:
        camera.set_gain(float(np.clip(args.gain, gain_min, gain_max)))

    def exp_to_slider(us):
        return int(round(EXP_STEPS * np.log(us / exp_min) / np.log(exp_max / exp_min)))

    def slider_to_exp(pos):
        return float(exp_min * (exp_max / exp_min) ** (pos / EXP_STEPS))

    def gain_to_slider(db):
        return int(round((db - gain_min) * GAIN_STEPS))

    def gamma_to_slider(g):
        return int(round(g * 100))

    # ---- Display gamma LUT (raw value -> 8-bit screen value) ----
    gamma = float(np.clip(args.gamma, GAMMA_MIN, GAMMA_MAX))
    lut = None

    def build_lut(g):
        x = np.arange(decode.max_value + 1, dtype=np.float32) / decode.max_value
        return np.round(255.0 * x ** (1.0 / g)).astype(np.uint8)

    lut = build_lut(gamma)

    # ---- USB3 transfer settings (must be set before the stream is created) ----
    if camera.is_uv_device():
        if args.usb_mode:
            mode = Aravis.UvUsbMode.SYNC if args.usb_mode == "sync" else Aravis.UvUsbMode.ASYNC
            print(f"USB mode: {args.usb_mode}")
            try_call(lambda: camera.uv_set_usb_mode(mode))
        if args.bandwidth:
            print(f"USB bandwidth limit: {args.bandwidth} MB/s")
            try_call(lambda: camera.uv_set_bandwidth(int(args.bandwidth * 1e6)))

    # ---- Stream ----
    def open_stream():
        s = camera.create_stream(None, None)
        size = camera.get_payload()          # re-read: payload can change with settings
        for _ in range(args.buffers):
            s.push_buffer(Aravis.Buffer.new_allocate(size))
        g = Grabber(s)
        g.start()
        return s, g

    recoveries = 0

    def recover():
        """Tear down the USB stream completely and build a new one.

        A plain stop/start keeps the same Aravis stream; once a USB3 transfer gets out
        of step, every following frame can stay 'missing-packets' until it is recreated.
        """
        global stream, grabber, recoveries
        with controller.lock:
            print("Recreating stream...")
            grabber.running = False
            grabber.join(timeout=2)
            try:
                camera.stop_acquisition()
            except Exception as e:
                print(f"  stop_acquisition: {e}")
            grabber.latest = None
            grabber = None
            stream = None
            gc.collect()                     # drop the old ArvStream (stops its USB thread)
            time.sleep(0.2)
            stream, grabber = open_stream()
            camera.start_acquisition()
        recoveries += 1

    controller = Controller(camera)
    controller.start()
    stream, grabber = open_stream()

    camera.start_acquisition()
    acquiring = True

    # ---- Window and sliders ----
    cv2.namedWindow(WINDOW, cv2.WINDOW_NORMAL | cv2.WINDOW_KEEPRATIO | cv2.WINDOW_GUI_NORMAL)
    cv2.resizeWindow(WINDOW, int(width * args.scale), int(height * args.scale))

    redraw = True   # re-render the last frame even if no new one arrived (e.g. gamma change)
    # last requested values (keys step from these, not from a possibly lagging camera read-back)
    target = {"exposure": controller.exposure_us, "gain": controller.gain_db}

    def on_exposure(pos):
        target["exposure"] = slider_to_exp(pos)
        controller.request("exposure", target["exposure"])

    def on_gain(pos):
        target["gain"] = gain_min + pos / GAIN_STEPS
        controller.request("gain", target["gain"])

    def on_gamma(pos):
        global gamma, lut, redraw
        gamma = max(GAMMA_MIN, pos / 100.0)
        lut = build_lut(gamma)
        redraw = True

    cv2.createTrackbar("Exposure (log)", WINDOW,
                       exp_to_slider(np.clip(controller.exposure_us, exp_min, exp_max)),
                       EXP_STEPS, on_exposure)
    cv2.createTrackbar("Gain x0.1dB", WINDOW, gain_to_slider(controller.gain_db),
                       gain_to_slider(gain_max), on_gain)
    cv2.createTrackbar("Gamma x100", WINDOW, gamma_to_slider(gamma),
                       gamma_to_slider(GAMMA_MAX), on_gamma)

    print("Streaming. Sliders or keys: e/E exposure, g/G gain, d/D gamma, q quit.")

    img = None
    disp_fps, t0, n = 0.0, time.monotonic(), 0
    last_draw = 0.0
    while True:
        now = time.monotonic()

        # ---- Stall watchdog: no complete frame for a while -> report and restart ----
        stall_s = max(3.0, 3.0 * controller.exposure_us / 1e6 + 1.0)
        if now - grabber.last_ok > stall_s:
            done, failed, underrun = stream.get_statistics()
            print(f"STALL: no complete frame for {now - grabber.last_ok:.1f} s | "
                  f"stream completed {done}, failed {failed}, underruns {underrun} | "
                  f"bad statuses {grabber.bad or 'none'} | lost IDs {grabber.dropped}")
            if failed > 0 and recoveries == 0:
                print("  Hint: persistent 'missing-packets' = USB transfers breaking. Check "
                      "usbfs_memory_mb, try --bandwidth 200 and/or --usb-mode sync.")
            recover()
            continue

        item = grabber.take()
        if item is not None:
            buf, fid = item
            try:
                img = decode(buf)           # copies the data out of the Aravis buffer
            finally:
                stream.push_buffer(buf)
            redraw = True
        # refresh at least twice a second so the overlay stays live even if frames stop
        if img is not None and (redraw or now - last_draw > 0.5):
            redraw = False
            last_draw = now

            # Order: debayer (needs full res) -> downscale -> gamma LUT
            frame = cv2.cvtColor(img, decode.bayer_code) if decode.bayer_code is not None else img
            if args.scale != 1.0:
                frame = cv2.resize(frame, None, fx=args.scale, fy=args.scale,
                                   interpolation=cv2.INTER_AREA)
            disp = np.take(lut, frame, mode="clip")

            peak = int(img.max())
            label = (f"exp {controller.exposure_us / 1000:.3f} ms  gain {controller.gain_db:.1f} dB  "
                     f"gamma {gamma:.2f}  peak {peak}/{decode.max_value}  "
                     f"cam {grabber.cam_fps:.1f} fps  view {disp_fps:.1f} fps  lost {grabber.dropped}  "
                     f"bad {sum(grabber.bad.values())}  recovered {recoveries}")
            color = 255 if disp.ndim == 2 else (0, 255, 0)
            cv2.putText(disp, label, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.55, color, 1, cv2.LINE_AA)
            cv2.imshow(WINDOW, disp)

            n += 1
            dt = time.monotonic() - t0
            if dt >= 1.0:
                disp_fps, t0, n = n / dt, time.monotonic(), 0

        # GUI is polled every few ms regardless of exposure time -> responsive sliders
        key = cv2.waitKey(5) & 0xFF
        if key == ord('q'):
            break
        elif key in (ord('e'), ord('E')):
            new = float(np.clip(target["exposure"] * (1.25 if key == ord('E') else 0.8),
                                exp_min, exp_max))
            cv2.setTrackbarPos("Exposure (log)", WINDOW, exp_to_slider(new))   # fires on_exposure
        elif key in (ord('g'), ord('G')):
            new = float(np.clip(target["gain"] + (0.5 if key == ord('G') else -0.5),
                                gain_min, gain_max))
            cv2.setTrackbarPos("Gain x0.1dB", WINDOW, gain_to_slider(new))
        elif key in (ord('d'), ord('D')):
            new = float(np.clip(gamma + (0.1 if key == ord('D') else -0.1), GAMMA_MIN, GAMMA_MAX))
            cv2.setTrackbarPos("Gamma x100", WINDOW, gamma_to_slider(new))

finally:
    print("\nShutting down stream safely...")
    if grabber is not None:
        grabber.running = False
        grabber.join(timeout=2)
    if controller is not None:
        controller.stop()
        controller.join(timeout=2)
    if camera is not None and acquiring:
        camera.stop_acquisition()
    cv2.destroyAllWindows()
