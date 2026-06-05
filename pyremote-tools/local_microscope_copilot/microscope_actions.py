#!/usr/bin/python3
"""
Structured microscope action runner for GXSM4 pyremote workflows.

This module intentionally sits above gxsm4process.py.  The low-level module
knows how to talk to GXSM4; this class knows how to execute microscope
workflows, collect resulting data, and hand that data to analysis functions.
"""

from dataclasses import dataclass, field
from datetime import datetime
import importlib
import json
from pathlib import Path
import requests
import time
import traceback

import numpy as np

import gxsm4process as gxsm4
from microscope_coordinates import ScanGeometry


@dataclass
class ActionStep:
    """One structured command in a microscope workflow."""

    kind: str
    name: str = ""
    args: tuple = field(default_factory=tuple)
    kwargs: dict = field(default_factory=dict)
    wait_after_s: float = 0.0
    collect_as: str = None
    postprocess: object = None

    @classmethod
    def from_dict(cls, command):
        return cls(
            kind=command["kind"],
            name=command.get("name", ""),
            args=tuple(command.get("args", ())),
            kwargs=dict(command.get("kwargs", {})),
            wait_after_s=float(command.get("wait_after_s", 0.0)),
            collect_as=command.get("collect_as"),
            postprocess=command.get("postprocess"),
        )


@dataclass
class ActionRecord:
    """Compact, JSON-friendly trace entry for a completed action."""

    name: str
    kind: str
    status: str
    started_at: str
    elapsed_s: float
    command: str = ""
    details: dict = field(default_factory=dict)
    result_summary: object = None
    error: str = ""


@dataclass
class WorkAreaCandidate:
    """Candidate flat work area inside a scan frame."""

    px: int
    py: int
    scan_x_A: float
    scan_y_A: float
    score: float
    patch_px: tuple
    local_std_A: float
    slope_A_per_px: float
    grad95_A_per_px: float
    span_2_98_A: float
    blob_fraction: float
    world_x_A: float = None
    world_y_A: float = None
    nearest_bump_px: float = None
    note: str = ""


class MicroscopeActionRunner:
    """
    Execute GXSM4 actions and post-process their data.

    Typical use:

        runner = MicroscopeActionRunner(dry_run=False, verbose=0)
        data = runner.execute_vp_probe(analyze=True)
        runner.export_history("/tmp/gxsm-action-history.json")

    Future autonomous decision code should call this class with ActionStep
    objects or command dictionaries, then inspect runner.data and runner.history.
    """

    DSP_BUSY_MASK = 8 + 16 + 2 + 4
    TOPO_IDEAL_Z_RANGE_A = (-1000.0, 0.0)
    TOPO_PHYSICAL_Z_RANGE_A = (-2000.0, 2000.0)
    SCAN_CHANNELS = {
        0: {
            "name": "topography",
            "signal": "absolute Z / STM topography",
            "unit": "Angstrom",
            "use": (
                "primary image used for geometric tip-quality analysis; "
                "absolute offset is not meaningful for image features"
            ),
            "ideal_range_A": TOPO_IDEAL_Z_RANGE_A,
            "physical_range_A_approx": TOPO_PHYSICAL_Z_RANGE_A,
            "safety_note": (
                "Long-term automation should prefer negative Z values. "
                "Moving toward 0 lifts the tip in a power-loss-safe direction."
            ),
        },
        1: {
            "name": "current",
            "signal": "tunneling current",
            "unit": "nA",
            "use": "current setpoint plus noise; stability and feedback context",
        },
        2: {
            "name": "dFrequency",
            "signal": "frequency shift",
            "unit": "Hz",
            "use": "scan-recorded local dFreq context for tip shape/radius",
        },
        4: {
            "name": "excitation",
            "signal": "excitation",
            "unit": "mV",
            "use": "oscillation drive/excitation context",
        },
        5: {
            "name": "amplitude",
            "signal": "oscillation amplitude reading",
            "unit": "mV",
            "use": "amplitude stability context",
        },
        6: {
            "name": "phase",
            "signal": "phase",
            "unit": "degrees or controller units",
            "use": "phase stability/context",
        },
        7: {
            "name": "channel_8_aux",
            "signal": "overview/map copy or actual pixel time since scan start",
            "unit": "Angstrom or ms",
            "use": (
                "operator noted this may contain the last STM overview/map; "
                "if pixel-time mode is enabled it may instead contain timing"
            ),
        },
    }

    def __init__(
        self,
        gxsm=None,
        verbose=0,
        dry_run=True,
        default_channel=0,
        abort_refname="script-control",
    ):
        self.gxsm = gxsm
        self.verbose = verbose
        self.dry_run = dry_run
        self.default_channel = default_channel
        self.abort_refname = abort_refname
        self.history = []
        self.data = {}
        self.scan_site_history = []
        self.bump_history = []

    @classmethod
    def scan_channel_info(cls, channel=None):
        """Return known scan-channel conventions."""
        if channel is None:
            return dict(cls.SCAN_CHANNELS)
        return cls.SCAN_CHANNELS.get(
            int(channel),
            {
                "name": "channel_{}".format(channel),
                "signal": "unknown",
                "unit": "unknown",
                "use": "not yet classified",
            },
        )

    @classmethod
    def topography_z_safety(cls, image):
        """
        Summarize absolute Z/topography values against operator range guidance.

        Channel 0 Z is absolute Angstrom.  Its offset is not meaningful for
        feature finding, but the raw range matters because the Z actuator has
        physical limits and long-running automation should stay negative.
        """
        arr = np.asarray(image, dtype=float)
        stats = cls._array_stats_static(arr)
        if stats.get("len", 0) == 0:
            return {"stats": stats, "status": "empty"}

        zmin = stats["min"]
        zmax = stats["max"]
        ideal_min, ideal_max = cls.TOPO_IDEAL_Z_RANGE_A
        physical_min, physical_max = cls.TOPO_PHYSICAL_Z_RANGE_A
        within_ideal = zmin >= ideal_min and zmax <= ideal_max
        within_physical = zmin >= physical_min and zmax <= physical_max

        if within_ideal:
            status = "ideal_negative_working_range"
        elif within_physical:
            status = "within_approx_physical_range"
        else:
            status = "outside_approx_physical_range"

        return {
            "unit": "Angstrom",
            "stats": stats,
            "ideal_range_A": [ideal_min, ideal_max],
            "approx_physical_range_A": [physical_min, physical_max],
            "within_ideal_negative_range": within_ideal,
            "within_approx_physical_range": within_physical,
            "status": status,
            "interpretation": (
                "Absolute offset has no image-feature meaning. Prefer keeping "
                "long-term automation on the negative side of 0; moving toward "
                "0 lifts the tip."
            ),
        }

    def connect(self):
        if self.gxsm is None and not self.dry_run:
            self.gxsm = gxsm4.gxsm_process(verbose=self.verbose)
        return self.gxsm

    def run_plan(self, steps, stop_on_error=True):
        """
        Execute ActionStep objects or dictionaries in order.

        Supported step kinds:
        - action: gxsm.action(action_name)
        - set: gxsm.set(refname, value)
        - get: gxsm.get(refname)
        - method: arbitrary gxsm method by name
        - move: gxsm.moveto_scan_xy(x, y)
        - sleep: time.sleep(seconds)
        - ready: wait until DSP status bits are clear
        - probe_event: gxsm.get_probe_event(channel, nth)
        - vpdata: fetch sliced VP data from a probe event
        - vp_probe: execute a VP action, wait, fetch VP data
        - postprocess: call a Python processor on collected data
        """
        results = []
        for item in steps:
            step = item if isinstance(item, ActionStep) else ActionStep.from_dict(item)
            try:
                result = self.run_step(step)
            except Exception as exc:
                result = self._record(
                    step.name or step.kind,
                    step.kind,
                    "error",
                    time.time(),
                    command=step.kind,
                    details={"args": step.args, "kwargs": step.kwargs},
                    error="{}: {}".format(type(exc).__name__, exc),
                )
                if stop_on_error:
                    raise
            results.append(result)
        return results

    def run_step(self, step):
        started = time.time()
        name = step.name or step.kind

        if step.kind == "action":
            result = self.action(step.args[0], wait_after_s=step.wait_after_s)
        elif step.kind == "set":
            result = self.set_parameter(
                step.args[0],
                step.args[1],
                wait_after_s=step.wait_after_s,
            )
        elif step.kind == "get":
            result = self.get_parameter(step.args[0], collect_as=step.collect_as)
        elif step.kind == "method":
            method_name = step.args[0]
            result = self.call_gxsm(
                method_name,
                *step.args[1:],
                name=name,
                wait_after_s=step.wait_after_s,
                collect_as=step.collect_as,
                **step.kwargs
            )
        elif step.kind == "move":
            result = self.move_to(
                step.args[0],
                step.args[1],
                wait_after_s=step.wait_after_s,
            )
        elif step.kind == "sleep":
            seconds = step.kwargs.get("seconds", step.args[0] if step.args else 0.0)
            result = self.sleep(seconds)
        elif step.kind == "ready":
            result = self.ready_check(**step.kwargs)
        elif step.kind == "probe_event":
            result = self.fetch_probe_event(
                *step.args,
                collect_as=step.collect_as,
                **step.kwargs
            )
        elif step.kind == "vpdata":
            result = self.fetch_vpdata(collect_as=step.collect_as, **step.kwargs)
        elif step.kind == "vp_probe":
            result = self.execute_vp_probe(collect_as=step.collect_as, **step.kwargs)
        elif step.kind == "postprocess":
            result = self.post_process(collect_as=step.collect_as, **step.kwargs)
        else:
            raise ValueError("Unsupported action step kind: {}".format(step.kind))

        if step.postprocess is not None:
            result = step.postprocess(result, self)
            if step.collect_as:
                self.data[step.collect_as] = result

        return result

    def action(self, action_name, wait_after_s=0.0):
        return self.call_gxsm(
            "action",
            action_name,
            name="action:{}".format(action_name),
            wait_after_s=wait_after_s,
        )

    def set_parameter(self, refname, value, wait_after_s=0.0):
        return self.call_gxsm(
            "set",
            refname,
            value,
            name="set:{}".format(refname),
            wait_after_s=wait_after_s,
        )

    def get_parameter(self, refname, collect_as=None):
        return self.call_gxsm(
            "get",
            refname,
            name="get:{}".format(refname),
            collect_as=collect_as,
        )

    def dFreq(self):
        """Read dFrequency in Hz from GXSM rtquery('f')."""
        started = time.time()
        if self.dry_run:
            value = None
        else:
            fvec = self.connect().rtquery("f")
            value = float(fvec[0])
        self.data["last_dFrequency_Hz"] = value
        self._record(
            "dFreq",
            "rtquery",
            "dry-run" if self.dry_run else "ok",
            started,
            command="rtquery:f",
            result=value,
        )
        return value

    def read_xyz_monitor(self):
        """Read fast XYZ monitor values via gxsm4process.rt_query_xyz()."""
        started = time.time()
        if self.dry_run:
            monitor = {
                "monitor": [None, None, None],
                "monitor_min": [None, None, None],
                "monitor_max": [None, None, None],
            }
        else:
            gxsm = self.connect()
            monitor = gxsm.rt_query_xyz()
        result = {
            "monitor": np.asarray(monitor.get("monitor"), dtype=float).tolist(),
            "monitor_min": np.asarray(monitor.get("monitor_min"), dtype=float).tolist(),
            "monitor_max": np.asarray(monitor.get("monitor_max"), dtype=float).tolist(),
        }
        self.data["last_xyz_monitor"] = result
        self._record(
            "read_xyz_monitor",
            "rtquery",
            "dry-run" if self.dry_run else "ok",
            started,
            command="rt_query_xyz",
            result=result,
        )
        return result

    def read_rpspmc_monitor(self):
        """Read fast RPSPMC controller monitor values via rt_query_rpspmc()."""
        started = time.time()
        if self.dry_run:
            result = {"dry_run": True}
        else:
            result = self._jsonable(self.connect().rt_query_rpspmc())
        self.data["last_rpspmc_monitor"] = result
        self._record(
            "read_rpspmc_monitor",
            "rtquery",
            "dry-run" if self.dry_run else "ok",
            started,
            command="rt_query_rpspmc",
            result=result,
        )
        return result

    def interpret_dFreq(self, dFreq_Hz):
        """
        Interpret dFrequency as global tip shape/radius context.

        Operator rule of thumb for normal attractive-mode imaging:
        - 0 Hz: tip is not near the surface or dFreq controller is disabled
        - 0..-2 Hz: perfect/excellent
        - 0..-4 Hz: OK
        - < -10 Hz: may work, but often indicates double/multiple tip character
        - positive dFreq: special strongly repulsive scan modes, interpreted later
        """
        if dFreq_Hz is None:
            return {
                "dFrequency_Hz": None,
                "category": "unknown",
                "message": "dFrequency was not read.",
            }
        dFreq_Hz = float(dFreq_Hz)
        if abs(dFreq_Hz) < 1e-9:
            category = "not_near_surface_or_controller_disabled"
            message = (
                "0 Hz means the tip is not near the surface, or the dFreq "
                "controller is disabled."
            )
        elif dFreq_Hz > 0.0:
            category = "repulsive_or_special_mode"
            message = (
                "Positive dFrequency is expected only in strongly repulsive "
                "or special scan modes; defer normal tip-quality interpretation."
            )
        elif dFreq_Hz >= -2.0:
            category = "excellent"
            message = "0..-2 Hz: perfect/excellent global tip indication."
        elif dFreq_Hz >= -4.0:
            category = "ok"
            message = "0..-4 Hz: OK global tip indication."
        elif dFreq_Hz < -10.0:
            category = "likely_double_or_multiple"
            message = (
                "dFrequency < -10 Hz may work, but generally suggests some "
                "double/multiple tip character."
            )
        else:
            category = "elevated"
            message = (
                "Negative dFrequency magnitude is elevated. This may still "
                "work but is less favorable than 0..-4 Hz."
            )
        return {
            "dFrequency_Hz": dFreq_Hz,
            "category": category,
            "message": message,
        }

    def move_to(self, x, y, wait_after_s=0.0):
        return self.call_gxsm(
            "moveto_scan_xy",
            x,
            y,
            name="move_to_scan_xy",
            wait_after_s=wait_after_s,
        )

    def call_gxsm(
        self,
        method_name,
        *args,
        name=None,
        wait_after_s=0.0,
        collect_as=None,
        **kwargs
    ):
        started = time.time()
        command_name = name or method_name

        if self.dry_run:
            result = None
        else:
            method = getattr(self.connect(), method_name)
            result = method(*args, **kwargs)

        if wait_after_s > 0:
            time.sleep(wait_after_s)

        if collect_as:
            self.data[collect_as] = result

        return self._record(
            command_name,
            "gxsm",
            "dry-run" if self.dry_run else "ok",
            started,
            command=method_name,
            details={"args": args, "kwargs": kwargs},
            result=result,
        )

    def sleep(self, seconds):
        started = time.time()
        if not self.dry_run:
            time.sleep(seconds)
        return self._record(
            "sleep",
            "sleep",
            "dry-run" if self.dry_run else "ok",
            started,
            command="sleep",
            details={"seconds": seconds},
        )

    def abort_requested(self):
        if self.dry_run:
            return False
        try:
            value = self.connect().get(self.abort_refname)
            return float(value) <= 0.0
        except Exception:
            return False

    def ready_check(
        self,
        message="ready_check",
        busy_mask=None,
        delay_s=1.0,
        timeout_s=60.0,
        allow_abort=True,
    ):
        started = time.time()
        busy_mask = self.DSP_BUSY_MASK if busy_mask is None else busy_mask

        if self.dry_run:
            return self._record(
                message,
                "ready",
                "dry-run",
                started,
                command="rtquery:s",
                details={"busy_mask": busy_mask, "timeout_s": timeout_s},
                result=True,
            )

        deadline = time.monotonic() + timeout_s
        last_status = None

        while True:
            if allow_abort and self.abort_requested():
                return self._record(
                    message,
                    "ready",
                    "aborted",
                    started,
                    command="rtquery:s",
                    details={"busy_mask": busy_mask, "last_status": last_status},
                    result=False,
                )

            status_vec = self.connect().rtquery("s")
            last_status = int(status_vec[0])
            if not (last_status & busy_mask):
                return self._record(
                    message,
                    "ready",
                    "ok",
                    started,
                    command="rtquery:s",
                    details={"busy_mask": busy_mask, "last_status": last_status},
                    result=True,
                )

            if time.monotonic() >= deadline:
                return self._record(
                    message,
                    "ready",
                    "timeout",
                    started,
                    command="rtquery:s",
                    details={"busy_mask": busy_mask, "last_status": last_status},
                    result=False,
                )

            time.sleep(delay_s)

    def rtquery_scan_gvp_status(self):
        """Read rtquery('s') and expose the Scan/GVP status word explicitly."""
        if self.dry_run:
            return {
                "dry_run": True,
                "Sall": 0,
                "Sspm": 0,
                "Sgvp": 0,
                "ready": True,
                "state": "stopped_or_finished_scan",
            }
        vec = self.connect().rtquery("s")
        sgvp = int(vec[2]) if len(vec) > 2 else None
        if sgvp == 0:
            state = "stopped_or_finished_scan"
            ready = True
            busy = False
        elif sgvp == 5:
            state = "completed_gvp"
            ready = True
            busy = False
        elif sgvp == 1:
            state = "busy"
            ready = False
            busy = True
        else:
            state = "unknown"
            ready = False
            busy = False
        result = {
            "raw": self._jsonable(vec),
            "Sall": int(vec[0]) if len(vec) > 0 else None,
            "Sspm": int(vec[1]) if len(vec) > 1 else None,
            "Sgvp": sgvp,
            "state": state,
            "busy": busy,
        }
        result["ready"] = ready
        return result

    def ready_check_scan_gvp(
        self,
        message="scan_gvp_ready_check",
        delay_s=1.0,
        timeout_s=60.0,
        allow_abort=True,
    ):
        """
        Wait until GXSM reports no scan/GVP activity.

        Operator convention: `(Sall, Sspm, Sgvp) = gxsm.rtquery('s')`.
        `Sgvp == 0` means stopped or finished scan, `Sgvp == 5` means
        completed GVP; both are ready. `Sgvp == 1` is busy.
        """
        started = time.time()

        if self.dry_run:
            return self._record(
                message,
                "ready",
                "dry-run",
                started,
                command="rtquery:s",
                details={"status_word": "Sgvp", "timeout_s": timeout_s},
                result={
                    "Sgvp": 0,
                    "ready": True,
                    "state": "stopped_or_finished_scan",
                    "busy": False,
                },
            )

        deadline = time.monotonic() + timeout_s
        last_status = None

        while True:
            if allow_abort and self.abort_requested():
                return self._record(
                    message,
                    "ready",
                    "aborted",
                    started,
                    command="rtquery:s",
                    details={"status_word": "Sgvp", "last_status": last_status},
                    result=False,
                )

            last_status = self.rtquery_scan_gvp_status()
            if last_status.get("ready"):
                return self._record(
                    message,
                    "ready",
                    "ok",
                    started,
                    command="rtquery:s",
                    details={"status_word": "Sgvp", "last_status": last_status},
                    result=True,
                )

            if time.monotonic() >= deadline:
                return self._record(
                    message,
                    "ready",
                    "timeout",
                    started,
                    command="rtquery:s",
                    details={"status_word": "Sgvp", "last_status": last_status},
                    result=False,
                )

            time.sleep(delay_s)

    def settle_before_pyshm_image_fetch(
        self,
        reason="settle before PySHM get_slice",
        wait_s=1.0,
    ):
        """
        Give GXSM/PySHM ample settling time before reading image data through
        `gxsm.get_slice`.

        The GXSM C++ pyremote `remote_getslice()` path reads scan `mem2d`
        directly in the PySHM server thread and constructs a NumPy array there,
        so planner workflows should not hammer repeated PySHM actions
        back-to-back while scanning.
        """
        started = time.time()
        if self.dry_run:
            return self._record(
                "settle_before_pyshm_image_fetch",
                "gxsm",
                "dry-run",
                started,
                command="sleep",
                details={"reason": reason},
                result={"wait_s": float(wait_s)},
            )

        time.sleep(float(wait_s))
        result = {
            "reason": reason,
            "wait_s": float(wait_s),
            "note": "No scan stop was issued; this only spaces PySHM/get_slice activity.",
        }
        self.data["last_settle_before_pyshm_image_fetch"] = result
        return self._record(
            "settle_before_pyshm_image_fetch",
            "gxsm",
            "ok",
            started,
            command="sleep",
            details={"reason": reason},
            result=result,
        )

    def fetch_probe_event(self, channel=None, nth=-1, collect_as=None):
        started = time.time()
        channel = self.default_channel if channel is None else channel

        if self.dry_run:
            result = None
        else:
            result = self.connect().get_probe_event(channel, nth)

        self.data["last_probe_event"] = result
        if collect_as:
            self.data[collect_as] = result

        return self._record(
            "fetch_probe_event",
            "probe_event",
            "dry-run" if self.dry_run else "ok",
            started,
            command="get_probe_event",
            details={"channel": channel, "nth": nth},
            result=result,
        )

    def fetch_vpdata(
        self,
        channel=None,
        nth=-1,
        start_idx=600,
        end_idx=1600,
        collect_as=None,
    ):
        started = time.time()
        channel = self.default_channel if channel is None else channel

        if self.dry_run:
            data = None
        else:
            probe_result = self.connect().get_probe_event(channel, nth)
            data = self._vpdata_from_probe_result(probe_result, start_idx, end_idx)

        self.data["last_vpdata"] = data
        if collect_as:
            self.data[collect_as] = data
        return self._record(
            "fetch_vpdata",
            "vpdata",
            "dry-run" if self.dry_run else "ok",
            started,
            command="get_probe_event",
            details={
                "channel": channel,
                "nth": nth,
                "start_idx": start_idx,
                "end_idx": end_idx,
            },
            result=data,
        )

    def execute_vp_probe(
        self,
        action_name="DSP_VP_VP_EXECUTE",
        channel=None,
        nth=-1,
        start_idx=600,
        end_idx=1600,
        precheck=True,
        wait_after_execute_s=4.0,
        ready_timeout_s=120.0,
        analyze=False,
        analysis_plot=False,
        collect_as=None,
    ):
        channel = self.default_channel if channel is None else channel

        if precheck:
            ready = self.ready_check_scan_gvp(
                message="pre_vp_ready",
                timeout_s=ready_timeout_s,
            )
            if ready.status not in ("ok", "dry-run"):
                return ready

        status_before = self.rtquery_scan_gvp_status()
        action_record = self.action(action_name)
        self.data["last_vp_execute_action"] = action_record
        self.data["last_vp_execute_status_before"] = status_before
        self.sleep(wait_after_execute_s)
        status_after_wait = self.rtquery_scan_gvp_status()
        self.data["last_vp_execute_status_after_wait"] = status_after_wait
        ready = self.ready_check_scan_gvp(
            message="post_vp_ready",
            timeout_s=ready_timeout_s,
        )
        if ready.status not in ("ok", "dry-run"):
            self.data["last_vp_execute_ready_record"] = ready
            return ready
        self.data["last_vp_execute_ready_record"] = ready

        record = self.fetch_vpdata(
            channel=channel,
            nth=nth,
            start_idx=start_idx,
            end_idx=end_idx,
        )
        data = self.data.get("last_vpdata")
        if collect_as:
            self.data[collect_as] = data

        if analyze and data is not None:
            self.analyze_zstopo(data, plot=analysis_plot)

        return record

    def analyze_zstopo(self, data=None, plot=False):
        data = self.data.get("last_vpdata") if data is None else data
        return self.post_process(
            processor="analyze-vpdata:analyze_zpdata",
            data=data,
            plot=plot,
            collect_as="last_analysis",
        )

    def post_process(self, processor, data=None, collect_as=None, **kwargs):
        started = time.time()
        func = self._resolve_processor(processor)
        result = func(data, **kwargs)
        if collect_as:
            self.data[collect_as] = result
        return self._record(
            "post_process:{}".format(processor),
            "postprocess",
            "ok",
            started,
            command=str(processor),
            details={"kwargs": kwargs},
            result=result,
        )

    def compare_signal_windows(
        self,
        data=None,
        signal="ZS-Topo",
        time_signal="Time-Mon",
        first_ms=100.0,
        last_ms=100.0,
    ):
        data = self.data.get("last_vpdata") if data is None else data
        if data is None:
            if self.dry_run:
                return {
                    "dry_run": True,
                    "signal": signal,
                    "message": "No VP data available in dry-run mode.",
                }
            raise ValueError("No VP data available for window comparison.")

        vpdata = data["vpdata"]
        vpunits = data.get("vpunits", {})
        values = np.asarray(vpdata[signal])

        if time_signal in vpdata:
            times = np.asarray(vpdata[time_signal])
            first_mask = times <= (times[0] + first_ms)
            last_mask = times >= (times[-1] - last_ms)
        else:
            n = len(values)
            first_n = max(1, min(n, int(first_ms)))
            last_n = max(1, min(n, int(last_ms)))
            first_mask = np.zeros(n, dtype=bool)
            last_mask = np.zeros(n, dtype=bool)
            first_mask[:first_n] = True
            last_mask[-last_n:] = True

        first = values[first_mask]
        last = values[last_mask]
        summary = {
            "signal": signal,
            "unit": vpunits.get(signal, ""),
            "first": self._array_stats(first),
            "last": self._array_stats(last),
            "difference": {
                "mean": float(last.mean() - first.mean()),
                "max": float(last.max() - first.max()),
                "min": float(last.min() - first.min()),
            },
        }
        self.data["last_window_comparison"] = summary
        return summary

    def scan_watch_assess_tip(
        self,
        channel=0,
        min_lines=100,
        start_scan=True,
        setup_scan=True,
        range_A=700.0,
        points=400,
        initial_delay_s=5.0,
        poll_s=10.0,
        timeout_s=900.0,
        chunk_lines=120,
        move_if_poor=False,
        stop_before_move=True,
        output_prefix="tip_assessment",
    ):
        """
        Start or watch a scan, fetch enough image lines, assess tip quality, and
        find a clean patch if the tip looks poor.

        This is the learned operator workflow:
        1. optionally start a scan,
        2. wait until enough fresh lines are available,
        3. fetch the partial image in small chunks,
        4. assess double/multiple-tip signatures,
        5. if poor, find a quiet patch away from blobs,
        6. optionally stop and move to that patch.

        The default only diagnoses and proposes a patch. Set move_if_poor=True
        to actually write ScanX/ScanY on a live runner.
        """
        started = time.time()

        if self.dry_run:
            result = {
                "dry_run": True,
                "would_start_scan": start_scan,
                "would_setup_scan": setup_scan,
                "would_range_A": range_A,
                "would_points": points,
                "would_watch_until_lines": min_lines,
                "would_fetch_channel": channel,
                "would_move_if_poor": move_if_poor,
            }
            self.data["last_tip_workflow"] = result
            return self._record(
                "scan_watch_assess_tip",
                "tip_workflow",
                "dry-run",
                started,
                command="scan_watch_assess_tip",
                result=result,
            )

        gxsm = self.connect()
        if setup_scan:
            self.setup_scan_geometry(range_A=range_A, points=points)
        settings_before = self._scan_settings(channel)
        self.remember_scan_site(settings_before)

        if start_scan:
            start_ret = gxsm.startscan()
            time.sleep(initial_delay_s)
        else:
            start_ret = None

        progress = self._watch_scan_lines(
            channel=channel,
            min_lines=min_lines,
            poll_s=poll_s,
            timeout_s=timeout_s,
        )
        y_current = progress[-1]["y_current"] if progress else int(gxsm.y_current())
        settle_before_fetch = self.settle_before_pyshm_image_fetch(
            reason="tip workflow reached enough lines; settle before get_slice",
            wait_s=1.0,
        )

        image, fetch_meta = self.fetch_scan_image_to_line(
            channel=channel,
            y_current=y_current,
            chunk_lines=chunk_lines,
            chunk_delay_s=0.5,
        )
        auxiliary_channels = self.fetch_scan_auxiliary_channels_to_line(
            y_current=y_current,
            chunk_lines=chunk_lines,
        )
        pixel_size_A = self._pixel_size_A(channel)
        quality = self.assess_tip_quality(
            image,
            pixel_size_A=pixel_size_A,
            output_prefix=output_prefix,
        )
        bumps = self.detect_major_bumps(image, pixel_size_A=pixel_size_A)
        self.remember_bumps(bumps, settings_before)
        dFreq_Hz = self.dFreq()
        dFreq_report = self.interpret_dFreq(dFreq_Hz)
        quality["dFrequency"] = dFreq_report
        quality["major_bumps"] = bumps
        quality["scan_channels"] = {
            "primary": self.scan_channel_info(channel),
            "auxiliary": auxiliary_channels,
        }

        clean_patch = None
        move_record = None
        poor_tip = self.tip_quality_is_poor(quality)
        if poor_tip:
            clean_patch = self.find_clean_patch(image)
            if clean_patch is not None:
                clean_patch["scan_target"] = self.pixel_to_scan_xy(
                    clean_patch["px"],
                    clean_patch["py"],
                )
                if move_if_poor:
                    if stop_before_move:
                        gxsm.stopscan()
                        time.sleep(3.0)
                    move_record = self.move_to_scan_xy_fields(
                        clean_patch["scan_target"]["ScanX_A"],
                        clean_patch["scan_target"]["ScanY_A"],
                    )

        result = {
            "settings_before": settings_before,
            "startscan_return": start_ret,
            "progress": progress,
            "settle_before_fetch": settle_before_fetch.__dict__,
            "fetch_meta": fetch_meta,
            "auxiliary_channels": auxiliary_channels,
            "quality": quality,
            "poor_tip": poor_tip,
            "scan_site_history": self.scan_site_history[-10:],
            "bump_history": self.bump_history[-20:],
            "clean_patch": clean_patch,
            "move_record": move_record.__dict__ if move_record else None,
        }
        self.data["last_tip_workflow"] = result

        if output_prefix:
            Path("{}_workflow.json".format(output_prefix)).write_text(
                json.dumps(self._jsonable(result), indent=2)
            )

        return self._record(
            "scan_watch_assess_tip",
            "tip_workflow",
            "ok",
            started,
            command="scan_watch_assess_tip",
            details={
                "channel": channel,
                "min_lines": min_lines,
                "start_scan": start_scan,
                "move_if_poor": move_if_poor,
            },
            result=result,
        )

    def setup_scan_geometry(self, range_A=700.0, points=400):
        """Set scan size/resolution for diagnostic/tip-status scans."""
        started = time.time()
        if self.dry_run:
            result = {
                "RangeX": range_A,
                "RangeY": range_A,
                "PointsX": points,
                "PointsY": points,
            }
        else:
            gxsm = self.connect()
            gxsm.set("RangeX", range_A)
            gxsm.set("RangeY", range_A)
            gxsm.set("PointsX", points)
            gxsm.set("PointsY", points)
            result = {
                "RangeX": gxsm.get("RangeX"),
                "RangeY": gxsm.get("RangeY"),
                "PointsX": gxsm.get("PointsX"),
                "PointsY": gxsm.get("PointsY"),
            }
        return self._record(
            "setup_scan_geometry",
            "gxsm",
            "dry-run" if self.dry_run else "ok",
            started,
            command="set RangeX/RangeY/PointsX/PointsY",
            result=result,
        )

    def set_live_scan_speed(
        self,
        speed_A_per_s=None,
        multiplier=1.0,
        range_A=None,
        min_multiplier=0.5,
        max_multiplier=1.5,
        wait_after_s=0.0,
    ):
        """
        Adjust scan speed while scanning.

        Operator guidance: typical live scan speed is about 0.5..1.5 times the
        scan size in Angstrom per second.  For example, an 800 A scan commonly
        uses about 400..1200 A/s.
        """
        started = time.time()
        if speed_A_per_s is None:
            if range_A is None:
                if self.dry_run:
                    range_A = 700.0
                else:
                    range_A = float(self.connect().get("RangeX"))
            multiplier = float(multiplier)
            if multiplier < min_multiplier or multiplier > max_multiplier:
                raise ValueError(
                    "Scan-speed multiplier {} outside typical {}..{} range.".format(
                        multiplier,
                        min_multiplier,
                        max_multiplier,
                    )
                )
            speed_A_per_s = float(range_A) * multiplier
        else:
            speed_A_per_s = float(speed_A_per_s)
            if range_A is not None:
                ratio = speed_A_per_s / float(range_A)
                if ratio < min_multiplier or ratio > max_multiplier:
                    raise ValueError(
                        "Scan speed {} A/s is {:.3g}x range, outside typical {}..{}.".format(
                            speed_A_per_s,
                            ratio,
                            min_multiplier,
                            max_multiplier,
                        )
                    )
        record = self.set_parameter(
            "dsp-fbs-scan-speed-scan",
            speed_A_per_s,
            wait_after_s=wait_after_s,
        )
        self.data["last_live_scan_speed_set"] = {
            "speed_A_per_s": speed_A_per_s,
            "range_A": range_A,
            "multiplier": None if range_A is None else speed_A_per_s / float(range_A),
        }
        return record

    def set_live_bias(
        self,
        bias_V,
        max_abs_auto_V=1.0,
        wait_after_s=0.0,
    ):
        """
        Adjust bias while scanning, within the agreed low-risk auto limit.

        Bias is in Volts.  Absolute targets above max_abs_auto_V require
        operator handling outside this helper.
        """
        bias_V = float(bias_V)
        if abs(bias_V) > max_abs_auto_V:
            raise ValueError(
                "Bias target {} V exceeds +/-{} V auto limit.".format(
                    bias_V,
                    max_abs_auto_V,
                )
            )
        return self.set_parameter(
            "dsp-fbs-bias",
            bias_V,
            wait_after_s=wait_after_s,
        )

    def set_live_current_setpoint(
        self,
        current,
        unit="nA",
        max_auto_nA=0.05,
        wait_after_s=0.0,
    ):
        """
        Adjust current setpoint while scanning.

        GXSM expects nA. If unit='pA', the value is converted to nA before
        writing.  Values above max_auto_nA are blocked for tip safety.
        """
        unit = str(unit).lower()
        if unit == "pa":
            current_nA = float(current) / 1000.0
        elif unit == "na":
            current_nA = float(current)
        else:
            raise ValueError("Current unit must be 'nA' or 'pA'.")
        if current_nA > max_auto_nA:
            raise ValueError(
                "Current setpoint {} nA exceeds {} nA auto safety limit.".format(
                    current_nA,
                    max_auto_nA,
                )
            )
        return self.set_parameter(
            "dsp-fbs-mx0-current-set",
            current_nA,
            wait_after_s=wait_after_s,
        )

    def set_scan_slope_leveling(
        self,
        slope_x=None,
        slope_y=None,
        max_abs_slope=0.1,
        wait_after_s=0.0,
    ):
        """
        Set slow X/Y scan-leveling slopes.

        Operator convention:
        - `dsp-adv-scan-slope-x` and `dsp-adv-scan-slope-y` are slopes in
          non-rotated world X/Y.
        - Use Rotation=0 deg and the fast X scan direction to level world X.
        - Use Rotation=90 deg and the fast X scan direction to level world Y.
        - Typical absolute slope values are below about 0.1.
        """
        started = time.time()
        requested = {}
        if slope_x is not None:
            slope_x = float(slope_x)
            if abs(slope_x) > max_abs_slope:
                raise ValueError(
                    "Refusing scan slope X={} beyond +/-{} typical limit.".format(
                        slope_x,
                        max_abs_slope,
                    )
                )
            requested["dsp-adv-scan-slope-x"] = slope_x
        if slope_y is not None:
            slope_y = float(slope_y)
            if abs(slope_y) > max_abs_slope:
                raise ValueError(
                    "Refusing scan slope Y={} beyond +/-{} typical limit.".format(
                        slope_y,
                        max_abs_slope,
                    )
                )
            requested["dsp-adv-scan-slope-y"] = slope_y
        if not requested:
            raise ValueError("Provide slope_x and/or slope_y.")

        if self.dry_run:
            result = dict(requested)
        else:
            gxsm = self.connect()
            for key, value in requested.items():
                gxsm.set(key, value)
            if wait_after_s > 0:
                time.sleep(wait_after_s)
            result = {key: gxsm.get(key) for key in requested}
        self.data["last_scan_slope_leveling"] = {
            "requested": requested,
            "readback": result,
            "max_abs_slope": max_abs_slope,
            "coordinate_note": (
                "Slopes are non-rotated world X/Y. Level X at Rotation=0; "
                "level Y at Rotation=90 using the fast scan direction."
            ),
        }
        return self._record(
            "set_scan_slope_leveling",
            "gxsm",
            "dry-run" if self.dry_run else "ok",
            started,
            command="set dsp-adv-scan-slope-x/y",
            result=self.data["last_scan_slope_leveling"],
        )

    def suggest_scan_slope_from_plane(
        self,
        image,
        settings=None,
        channel=0,
    ):
        """
        Estimate world X/Y leveling slope from an image plane fit.

        This returns a suggestion only. The caller should apply X slope from a
        Rotation=0 scan and Y slope from a Rotation=90 scan, per operator
        convention.
        """
        arr = np.asarray(image, dtype=float)
        ny, nx = arr.shape
        if settings is None and not self.dry_run:
            settings = self._scan_settings(channel)
        settings = settings or {}
        geometry = ScanGeometry.from_settings(settings, image_shape=arr.shape)
        rotation_deg = geometry.rotation_deg
        y_grid, x_grid = np.mgrid[:ny, :nx]
        valid = np.isfinite(arr)
        if valid.sum() < 3:
            raise ValueError("Not enough finite data to estimate scan slope.")
        design = np.column_stack(
            [
                x_grid[valid].ravel(),
                y_grid[valid].ravel(),
                np.ones(valid.sum()),
            ]
        )
        coeffs, *_ = np.linalg.lstsq(design, arr[valid].ravel(), rcond=None)
        dz_dlocal_x = float(coeffs[0] / max(geometry.step_x_A, 1e-12))
        dz_dlocal_y = float(coeffs[1] / max(geometry.step_y_A, 1e-12))
        world_slope = geometry.local_gradient_to_world(dz_dlocal_x, dz_dlocal_y)
        return {
            "rotation_deg": rotation_deg,
            "local_slope_x_dz_per_A": dz_dlocal_x,
            "local_slope_y_dz_per_A": dz_dlocal_y,
            "world_slope_x_dz_per_A": float(world_slope.x_A),
            "world_slope_y_dz_per_A": float(world_slope.y_A),
            "apply_note": (
                "Use this as a suggestion. Apply X leveling from Rotation=0 "
                "and Y leveling from Rotation=90; typical abs values < 0.1."
            ),
        }

    def measure_recent_fast_axis_slope(
        self,
        channel=0,
        line_count=64,
        y_current=None,
        output_prefix=None,
    ):
        """
        Measure residual slope along the current fast scan axis.

        The profile is averaged over the most recent scan lines, then fitted as
        AngZ / Ang along local fast-axis X.  At Rotation=0 this is a world-X
        leveling measurement; at Rotation=90 this is a world-Y measurement.
        """
        if self.dry_run:
            return {"dry_run": True}
        gxsm = self.connect()
        dims = gxsm.get_dimensions(channel)
        ny = int(dims[1])
        if y_current is None:
            y_current = int(float(gxsm.y_current()))
        y0 = max(0, min(ny - 1, y_current) - int(line_count) + 1)
        yn = max(1, min(int(line_count), y_current - y0 + 1))
        strip = self.get_slice_array(gxsm, channel, 0, 0, y0, yn)
        settings = self._scan_settings(channel)
        geometry = ScanGeometry.from_settings(settings)
        pixel_size_x_A = geometry.step_x_A
        x_A = np.arange(strip.shape[1]) * pixel_size_x_A - geometry.range_x_A / 2.0
        profile = np.nanmean(strip, axis=0)
        ok = np.isfinite(profile)
        if ok.sum() < 3:
            raise ValueError("Not enough finite profile data for slope fit.")
        coeff = np.polyfit(x_A[ok], profile[ok], 1)
        fit = coeff[0] * x_A + coeff[1]
        residual = profile - fit
        rotation_deg = float(settings.get("Rotation", 0.0))
        axis = self._leveling_axis_for_rotation(rotation_deg)
        report = {
            "channel": channel,
            "dimensions": list(dims),
            "y_current": int(y_current),
            "y0": int(y0),
            "line_count": int(yn),
            "settings": settings,
            "rotation_deg": rotation_deg,
            "leveling_axis": axis,
            "x_unit": "Angstrom local fast axis, centered",
            "fit_slope_local_fast_x_AngZ_per_Ang": float(coeff[0]),
            "residual_std_after_fit_A": float(np.nanstd(residual)),
            "profile_range_A": float(np.nanmax(profile) - np.nanmin(profile)),
            "current_scan_slope_x": settings.get("dsp-adv-scan-slope-x"),
            "current_scan_slope_y": settings.get("dsp-adv-scan-slope-y"),
            "note": (
                "At Rotation=0 local fast X maps to world X. At Rotation=90 "
                "local fast X maps to world Y."
            ),
        }
        if output_prefix:
            np.save("{}_strip.npy".format(output_prefix), strip)
            Path("{}_report.json".format(output_prefix)).write_text(
                json.dumps(self._jsonable(report), indent=2)
            )
            self.plot_fast_axis_slope_profile(
                x_A,
                profile,
                fit,
                "{}_profile.png".format(output_prefix),
                title="Recent-lines average fast-axis profile",
            )
        self.data["last_fast_axis_slope"] = report
        return report

    def get_slice_array(self, gxsm, channel, v, t, yi, yn):
        """
        Fetch a GXSM slice and immediately normalize it into local NumPy memory.

        `gxsm.get_slice()` returns through the PySHM pickle path and may contain
        a NumPy array produced on the GXSM side.  Copying with the local/system
        NumPy instance avoids keeping references to a foreign unpickled array
        object and gives downstream analysis a plain C-contiguous float64 block.
        """
        raw = gxsm.get_slice(channel, v, t, yi, yn)
        arr = np.array(raw, dtype=np.float64, copy=True, order="C")
        return arr

    def auto_correct_scan_slope_from_recent_lines(
        self,
        channel=0,
        line_count=64,
        rotation=None,
        correction_sign=-1.0,
        gain=1.0,
        verify_delay_s=20.0,
        verify_line_count=16,
        max_abs_delta=0.005,
        max_abs_slope=0.1,
        output_prefix="scan_slope_autocorrect",
    ):
        """
        Correct scan leveling from recent live scan lines.

        Parameters:
        - Rotation 0: correct `dsp-adv-scan-slope-x`.
        - Rotation 90: correct `dsp-adv-scan-slope-y`.
        - correction_sign captures the learned sign convention. Use -1 when
          increasing the slope parameter reduces a positive residual. If a test
          grows the residual, rerun with +1 or allow the procedure to try both
          in a future higher-level loop.

        Returns pre-measurement, applied value, and post-verification.
        """
        if self.dry_run:
            return {"dry_run": True, "would_correct_rotation": rotation}
        pre = self.measure_recent_fast_axis_slope(
            channel=channel,
            line_count=line_count,
            output_prefix="{}_before".format(output_prefix),
        )
        measured_rotation = float(pre["rotation_deg"])
        if rotation is None:
            rotation = measured_rotation
        axis = self._leveling_axis_for_rotation(rotation)
        if axis != pre["leveling_axis"]:
            raise ValueError(
                "Requested rotation {} corrects {}, but current scan rotation {} "
                "measures {}. Set scan Rotation appropriately first.".format(
                    rotation,
                    axis,
                    measured_rotation,
                    pre["leveling_axis"],
                )
            )
        residual = float(pre["fit_slope_local_fast_x_AngZ_per_Ang"])
        delta = float(correction_sign) * float(gain) * residual
        if abs(delta) > max_abs_delta:
            raise ValueError(
                "Refusing slope correction delta {} beyond +/-{}.".format(
                    delta,
                    max_abs_delta,
                )
            )
        key = "dsp-adv-scan-slope-x" if axis == "world_x" else "dsp-adv-scan-slope-y"
        current = float(pre["settings"][key])
        target = current + delta
        if axis == "world_x":
            set_record = self.set_scan_slope_leveling(
                slope_x=target,
                max_abs_slope=max_abs_slope,
            )
        else:
            set_record = self.set_scan_slope_leveling(
                slope_y=target,
                max_abs_slope=max_abs_slope,
            )
        if verify_delay_s > 0:
            time.sleep(verify_delay_s)
        post = self.measure_recent_fast_axis_slope(
            channel=channel,
            line_count=verify_line_count,
            output_prefix="{}_after".format(output_prefix),
        )
        result = {
            "axis": axis,
            "rotation_deg": measured_rotation,
            "parameter": key,
            "pre": pre,
            "residual_before_AngZ_per_Ang": residual,
            "correction_sign": float(correction_sign),
            "gain": float(gain),
            "delta_applied": delta,
            "previous_value": current,
            "target_value": target,
            "set_record": set_record.__dict__,
            "post": post,
            "residual_after_AngZ_per_Ang": post.get(
                "fit_slope_local_fast_x_AngZ_per_Ang"
            ),
            "improved": (
                abs(float(post.get("fit_slope_local_fast_x_AngZ_per_Ang", residual)))
                < abs(residual)
            ),
            "sign_note": (
                "If improved is false, the scan direction/rotation sign is "
                "opposite; restore or rerun with correction_sign *= -1."
            ),
        }
        Path("{}_result.json".format(output_prefix)).write_text(
            json.dumps(self._jsonable(result), indent=2)
        )
        self.data["last_scan_slope_autocorrect"] = result
        return result

    def plot_fast_axis_slope_profile(
        self,
        x_A,
        profile,
        fit,
        output_path,
        title="Fast-axis profile",
    ):
        """Plot averaged recent-line profile versus local fast-axis X."""
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt

        fig, ax = plt.subplots(figsize=(10, 5))
        ax.plot(x_A, profile, "k-", lw=1.2, label="mean Z over recent lines")
        ax.plot(x_A, fit, "r-", lw=1.4, label="linear fit")
        ax.set_title(title)
        ax.set_xlabel("Local fast-axis X / A, centered")
        ax.set_ylabel("Mean Z / A")
        ax.grid(True, alpha=0.25)
        ax.legend(loc="best")
        fig.tight_layout()
        fig.savefig(output_path, dpi=170)
        plt.close(fig)
        return str(output_path)

    def _leveling_axis_for_rotation(self, rotation_deg, tolerance_deg=5.0):
        rotation = float(rotation_deg) % 180.0
        if abs(rotation - 0.0) <= tolerance_deg or abs(rotation - 180.0) <= tolerance_deg:
            return "world_x"
        if abs(rotation - 90.0) <= tolerance_deg:
            return "world_y"
        raise ValueError(
            "Automatic slope leveling only supports Rotation near 0 or 90 deg."
        )

    def fetch_scan_image_to_line(
        self,
        channel=0,
        y_current=None,
        chunk_lines=120,
        store_last=True,
        pre_fetch_settle_s=0.0,
        chunk_delay_s=0.0,
    ):
        """Fetch scan channel data from line 0 through y_current in chunks."""
        if self.dry_run:
            return None, {"dry_run": True}

        gxsm = self.connect()
        dims = gxsm.get_dimensions(channel)
        ny = int(dims[1])
        if y_current is None:
            y_current = int(float(gxsm.y_current()))
        y_count = ny if y_current >= ny - 1 else max(1, min(ny, int(y_current) + 1))

        if pre_fetch_settle_s and float(pre_fetch_settle_s) > 0:
            time.sleep(float(pre_fetch_settle_s))

        chunks = []
        for yi in range(0, y_count, chunk_lines):
            yn = min(chunk_lines, y_count - yi)
            block = self.get_slice_array(gxsm, channel, 0, 0, yi, yn)
            if block.ndim != 2:
                raise RuntimeError(
                    "Unexpected get_slice block shape at yi={}: {}".format(
                        yi,
                        block.shape,
                    )
                )
            chunks.append(block)
            if chunk_delay_s and float(chunk_delay_s) > 0 and yi + yn < y_count:
                time.sleep(float(chunk_delay_s))

        image = np.vstack(chunks)
        meta = {
            "channel": channel,
            "dimensions": list(dims),
            "y_current": int(y_current),
            "fetched_y_count": int(y_count),
            "shape": list(image.shape),
            "stats": self._array_stats(image),
        }
        if store_last:
            self.data["last_scan_image"] = image
            self.data["last_scan_fetch_meta"] = meta
        return image, meta

    def fetch_scan_auxiliary_channels_to_line(
        self,
        channels=(1, 2),
        y_current=None,
        chunk_lines=60,
        store_arrays=False,
    ):
        """
        Fetch summary stats for non-topography scan channels when available.

        Channel 0 is the image/topography channel.  The companion channels carry
        physical context such as current, scan-recorded dFrequency, excitation,
        amplitude, phase, and optional per-pixel timing.
        """
        if self.dry_run:
            return {
                int(ch): {
                    "dry_run": True,
                    "channel_info": self.scan_channel_info(ch),
                }
                for ch in channels
            }

        summaries = {}
        for ch in channels:
            ch = int(ch)
            try:
                image, meta = self.fetch_scan_image_to_line(
                    channel=ch,
                    y_current=y_current,
                    chunk_lines=chunk_lines,
                    store_last=False,
                )
                summaries[ch] = {
                    "available": True,
                    "channel_info": self.scan_channel_info(ch),
                    "meta": meta,
                    "stats": self._array_stats(image),
                }
                if store_arrays:
                    self.data["last_scan_channel_{}".format(ch)] = image
            except Exception as exc:
                summaries[ch] = {
                    "available": False,
                    "channel_info": self.scan_channel_info(ch),
                    "error": "{}: {}".format(type(exc).__name__, exc),
                }
        self.data["last_scan_auxiliary_channels"] = summaries
        return summaries

    def sample_dfrequency_values(self, count=8, delay_s=0.25):
        """Read several live dFreq snapshots and return robust summary stats."""
        values = []
        for _ in range(int(count)):
            try:
                values.append(float(self.dFreq()))
            except Exception:
                values.append(float("nan"))
            if delay_s > 0:
                time.sleep(float(delay_s))
        arr = np.asarray(values, dtype=float)
        finite = arr[np.isfinite(arr)]
        stats = {"count": int(finite.size)}
        if finite.size:
            stats.update(
                {
                    "mean_Hz": float(np.nanmean(finite)),
                    "median_Hz": float(np.nanmedian(finite)),
                    "std_Hz": float(np.nanstd(finite)),
                    "min_Hz": float(np.nanmin(finite)),
                    "max_Hz": float(np.nanmax(finite)),
                }
            )
            interpretation = self.interpret_dFreq(stats["mean_Hz"])
        else:
            interpretation = self.interpret_dFreq(None)
        return {
            "values_Hz": values,
            "stats": stats,
            "interpretation": interpretation,
            "note": "Live rtquery('f') dFreq snapshots; mean is used in scan analysis.",
        }

    def topo_feature_sharpness(self, image, pixel_size_A=1.0):
        """
        Estimate topography feature/edge sharpness for metal-tip assessment.

        This is not a CO/O-functionalized-tip classifier. It is intended to
        track whether metal-tip topo features and step edges look crisp or
        blurred at the current scan scale.
        """
        arr = np.asarray(image, dtype=float)
        valid = np.isfinite(arr)
        if valid.sum() < 16:
            return {"available": False, "message": "Not enough finite topo data."}
        ny, nx = arr.shape
        y_grid, x_grid = np.mgrid[:ny, :nx]
        design = np.column_stack(
            [x_grid[valid].ravel(), y_grid[valid].ravel(), np.ones(valid.sum())]
        )
        coeffs, *_ = np.linalg.lstsq(design, arr[valid].ravel(), rcond=None)
        leveled = arr - (coeffs[0] * x_grid + coeffs[1] * y_grid + coeffs[2])
        line_flat = leveled - np.nanmedian(leveled, axis=1, keepdims=True)
        gy, gx = np.gradient(line_flat)
        grad_mag = np.hypot(gx, gy)
        finite_grad = grad_mag[np.isfinite(grad_mag)]
        finite_img = line_flat[np.isfinite(line_flat)]
        if finite_grad.size == 0 or finite_img.size == 0:
            return {"available": False, "message": "No finite gradient data."}
        robust_z_range = float(
            np.nanpercentile(finite_img, 99) - np.nanpercentile(finite_img, 1)
        )
        grad_p50 = float(np.nanpercentile(finite_grad, 50))
        grad_p90 = float(np.nanpercentile(finite_grad, 90))
        grad_p95 = float(np.nanpercentile(finite_grad, 95))
        grad_p99 = float(np.nanpercentile(finite_grad, 99))
        px = max(float(pixel_size_A), 1e-12)
        grad_p95_A_per_A = grad_p95 / px
        characteristic_width_A = (
            None if grad_p95 <= 0 else float(robust_z_range / grad_p95 * px)
        )
        edge_mask = finite_grad >= grad_p95
        edge_fraction = float(np.count_nonzero(edge_mask) / max(finite_grad.size, 1))
        if characteristic_width_A is None:
            category = "unknown"
            message = "Feature sharpness could not be estimated."
        elif characteristic_width_A <= 3.0 * px:
            category = "sharp_for_metal_tip"
            message = (
                "Topo features/edges are crisp at this pixel scale; this is "
                "consistent with a good metal tip, not a functionalized-tip test."
            )
        elif characteristic_width_A <= 8.0 * px:
            category = "moderate_metal_tip_sharpness"
            message = (
                "Topo features are reasonably defined but not atomically sharp; "
                "may still be good for metal-tip imaging."
            )
        else:
            category = "blurred_features"
            message = (
                "Topo features/edges appear broad relative to pixel size; this "
                "can indicate a blunt, contaminated, or multiple tip."
            )
        return {
            "available": True,
            "pixel_size_A": float(pixel_size_A),
            "robust_z_range_A": robust_z_range,
            "gradient_A_per_px": {
                "p50": grad_p50,
                "p90": grad_p90,
                "p95": grad_p95,
                "p99": grad_p99,
            },
            "gradient_A_per_A": {
                "p95": grad_p95_A_per_A,
            },
            "characteristic_edge_width_A_approx": characteristic_width_A,
            "edge_pixel_fraction_top5pct": edge_fraction,
            "category": category,
            "message": message,
        }

    def plot_scan_image(
        self,
        image,
        output_path,
        title="STM image",
        bumps=None,
        clean_patch=None,
        cmap="viridis",
        percentile_clip=(1, 99),
    ):
        """Plot scan data with line 0 at the image top, matching GXSM view."""
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt

        arr = np.asarray(image, dtype=float)
        fig, ax = plt.subplots(figsize=(9, 8))
        vmin, vmax = np.nanpercentile(arr, percentile_clip)
        im = ax.imshow(
            arr,
            origin="upper",
            cmap=cmap,
            aspect="equal",
            vmin=vmin,
            vmax=vmax,
        )
        ax.set_title(title)
        ax.set_xlabel("X pixel")
        ax.set_ylabel("Y pixel, line 0 at top")
        fig.colorbar(im, ax=ax, label="Z / signal")
        if clean_patch:
            ax.plot(
                clean_patch["px"],
                clean_patch["py"],
                "wo",
                ms=8,
                mec="k",
                label="clean patch",
            )
        for bump in bumps or []:
            ax.plot(bump["px"], bump["py"], "rx", ms=7)
        if clean_patch or bumps:
            ax.legend(loc="upper right")
        fig.tight_layout()
        fig.savefig(output_path, dpi=160)
        plt.close(fig)
        return str(output_path)

    def assess_tip_quality(
        self,
        image,
        pixel_size_A=None,
        output_prefix=None,
        strong_corr=0.22,
        moderate_corr=0.14,
        min_multiple_tip_corr_dist_px=32,
    ):
        """
        Estimate tip quality from repeated-offset image signatures.

        Returns a dictionary with a verdict, autocorrelation peaks, roughness
        metrics, and directional gradient asymmetry.
        """
        arr = np.asarray(image, dtype=float)
        ny, nx = arr.shape
        y_grid, x_grid = np.mgrid[:ny, :nx]
        valid = np.isfinite(arr)
        if valid.sum() < 16:
            raise ValueError("Not enough finite image data for tip assessment.")

        design = np.column_stack(
            [
                x_grid[valid].ravel(),
                y_grid[valid].ravel(),
                np.ones(valid.sum()),
            ]
        )
        coeffs, *_ = np.linalg.lstsq(design, arr[valid].ravel(), rcond=None)
        leveled = arr - (coeffs[0] * x_grid + coeffs[1] * y_grid + coeffs[2])
        line_flat = leveled - np.nanmedian(leveled, axis=1, keepdims=True)
        gy, gx = np.gradient(line_flat)

        img = line_flat - np.nanmedian(line_flat)
        scale = np.nanpercentile(np.abs(img), 95)
        if scale == 0 or not np.isfinite(scale):
            scale = np.nanstd(img) or 1.0
        img = np.clip(img / scale, -1, 1)
        img[~np.isfinite(img)] = 0.0
        wimg = img * np.hanning(ny)[:, None] * np.hanning(nx)[None, :]
        fft = np.fft.fft2(wimg)
        ac = np.fft.fftshift(np.fft.ifft2(fft * np.conj(fft)).real)
        cy, cx = ny // 2, nx // 2
        acn = ac / (ac[cy, cx] if ac[cy, cx] else 1.0)

        peaks = self._autocorr_peaks(acn, max_peaks=10)
        if pixel_size_A is None:
            pixel_size_A = 1.0
        peak_reports = []
        for py, px, corr in peaks:
            dy = py - cy
            dx = px - cx
            dist_px = float(np.hypot(dx, dy))
            ignored = dist_px < min_multiple_tip_corr_dist_px
            peak_reports.append(
                {
                    "dx_px": int(dx),
                    "dy_px": int(dy),
                    "dist_px": dist_px,
                    "dist_A_approx": dist_px * pixel_size_A,
                    "angle_deg": float(np.degrees(np.arctan2(dy, dx))),
                    "corr": float(corr),
                    "used_for_tip_verdict": not ignored,
                    "ignore_reason": (
                        "short-range autocorrelation/noise"
                        if ignored else ""
                    ),
                }
            )

        pos_x = float(np.nanmean(np.abs(gx[gx > 0]))) if np.any(gx > 0) else 0.0
        neg_x = float(np.nanmean(np.abs(gx[gx < 0]))) if np.any(gx < 0) else 0.0
        pos_y = float(np.nanmean(np.abs(gy[gy > 0]))) if np.any(gy > 0) else 0.0
        neg_y = float(np.nanmean(np.abs(gy[gy < 0]))) if np.any(gy < 0) else 0.0
        asym_x = abs(pos_x - neg_x) / max(pos_x, neg_x, 1e-12)
        asym_y = abs(pos_y - neg_y) / max(pos_y, neg_y, 1e-12)

        strong = [
            peak for peak in peak_reports
            if peak["corr"] >= strong_corr and peak["dist_px"] >= 8
            and peak["used_for_tip_verdict"]
        ]
        moderate = [
            peak for peak in peak_reports
            if moderate_corr <= peak["corr"] < strong_corr and peak["dist_px"] >= 8
            and peak["used_for_tip_verdict"]
        ]
        if strong:
            verdict = "suspect_double_or_multiple_tip"
            confidence = "moderate"
        elif moderate:
            verdict = "weak_double_tip_hint"
            confidence = "low_to_moderate"
        else:
            verdict = "no_strong_double_tip_signature"
            confidence = "low_to_moderate"

        report = {
            "image": {
                "shape": [int(ny), int(nx)],
                "approx_pixel_size_A": float(pixel_size_A),
                "channel": self.scan_channel_info(0),
                "absolute_z": self.topography_z_safety(arr),
            },
            "plane_coefficients": {
                "dz_dx_per_px": float(coeffs[0]),
                "dz_dy_per_px": float(coeffs[1]),
                "offset": float(coeffs[2]),
            },
            "flattened_stats": {
                "roughness_std": float(np.nanstd(line_flat)),
                "robust_peak_to_peak_1_99": float(
                    np.nanpercentile(line_flat, 99)
                    - np.nanpercentile(line_flat, 1)
                ),
                "line_jump_sigma": float(
                    np.nanstd(np.diff(np.nanmedian(leveled, axis=1)))
                ) if ny > 1 else 0.0,
            },
            "autocorrelation_secondary_peaks": peak_reports,
            "ignored_autocorrelation_peaks": [
                peak for peak in peak_reports
                if not peak["used_for_tip_verdict"]
            ],
            "analysis_limits": {
                "min_multiple_tip_corr_dist_px": int(
                    min_multiple_tip_corr_dist_px
                ),
                "short_range_note": (
                    "Very local peaks, for example dy=8 px or similar short "
                    "vertical offsets, are treated as "
                    "noise/texture and are not sufficient evidence of a "
                    "double or multiple tip."
                ),
                "long_range_note": (
                    "Faint shadow/double/triple-tip signatures can have larger "
                    "or even out-of-field separation and may require operator "
                    "inspection or larger overview comparisons."
                ),
            },
            "directional_gradient_asymmetry": {"x": asym_x, "y": asym_y},
            "topo_feature_sharpness": self.topo_feature_sharpness(
                line_flat,
                pixel_size_A=pixel_size_A,
            ),
            "verdict": verdict,
            "confidence": confidence,
        }
        self.data["last_tip_quality"] = report

        if output_prefix:
            np.save("{}_scan_image.npy".format(output_prefix), arr)
            Path("{}_tip_quality.json".format(output_prefix)).write_text(
                json.dumps(self._jsonable(report), indent=2)
            )

        return report

    def tip_quality_is_poor(self, quality):
        """Combine image-based and dFrequency tip-quality indicators."""
        if quality.get("verdict") in (
            "suspect_double_or_multiple_tip",
            "weak_double_tip_hint",
        ):
            return True
        dFreq_info = quality.get("dFrequency") or {}
        if dFreq_info.get("category") in ("elevated", "likely_double_or_multiple"):
            return True
        return False

    def find_clean_patch(
        self,
        image,
        patch_x=36,
        patch_y=None,
        step_px=4,
        avoid_top_lines=8,
        avoid_bumps=True,
    ):
        """Find a low-gradient, low-contrast patch suitable for tip tuning."""
        arr = np.asarray(image, dtype=float)
        ny, nx = arr.shape
        if patch_y is None:
            patch_y = min(36, max(12, ny // 3))
        mx = patch_x // 2
        my = patch_y // 2
        if ny < 2 * my + 2 or nx < 2 * mx + 2:
            return None

        y_grid, x_grid = np.mgrid[:ny, :nx]
        valid = np.isfinite(arr)
        design = np.column_stack(
            [
                x_grid[valid].ravel(),
                y_grid[valid].ravel(),
                np.ones(valid.sum()),
            ]
        )
        coeffs, *_ = np.linalg.lstsq(design, arr[valid].ravel(), rcond=None)
        leveled = arr - (coeffs[0] * x_grid + coeffs[1] * y_grid + coeffs[2])
        flat = leveled - np.nanmedian(leveled, axis=1, keepdims=True)
        gy, gx = np.gradient(flat)
        grad = np.hypot(gx, gy)

        best = None
        y0 = max(my, avoid_top_lines)
        for cy in range(y0, ny - my, max(1, step_px)):
            for cx in range(mx, nx - mx, max(1, step_px)):
                window = flat[cy - my:cy + my, cx - mx:cx + mx]
                gwin = grad[cy - my:cy + my, cx - mx:cx + mx]
                if window.shape != (2 * my, 2 * mx) or not np.isfinite(window).all():
                    continue
                if avoid_bumps and self._patch_overlaps_recent_bump(cx, cy, mx, my):
                    continue
                local_std = np.nanstd(window)
                grad95 = np.nanpercentile(gwin, 95)
                span = (
                    np.nanpercentile(window, 98)
                    - np.nanpercentile(window, 2)
                )
                bright = max(
                    abs(np.nanpercentile(window, 1)),
                    abs(np.nanpercentile(window, 99)),
                )
                blob_fraction = float(
                    np.mean(np.abs(window) > max(0.12, 3 * local_std))
                )
                score = (
                    local_std
                    + 2.2 * grad95
                    + 0.18 * span
                    + 0.12 * bright
                    + 0.5 * blob_fraction
                )
                if best is None or score < best["score"]:
                    best = {
                        "score": float(score),
                        "px": int(cx),
                        "py": int(cy),
                        "patch_px": [int(patch_x), int(patch_y)],
                        "local_std": float(local_std),
                        "grad95": float(grad95),
                        "span_2_98": float(span),
                        "bright_abs": float(bright),
                        "blob_fraction": float(blob_fraction),
                    }
        return best

    def detect_major_bumps(
        self,
        image,
        max_bumps=12,
        sigma_threshold=4.0,
        min_sep_px=16,
        pixel_size_A=1.0,
        molecule_height_A=3.0,
        large_blob_diameter_A=40.0,
    ):
        """
        Detect large protrusions/bumps in the current image.

        These locations are remembered so future clean-patch selection and tip
        tuning can avoid risky features that may damage or contaminate the tip.
        """
        arr = np.asarray(image, dtype=float)
        ny, nx = arr.shape
        y_grid, x_grid = np.mgrid[:ny, :nx]
        valid = np.isfinite(arr)
        if valid.sum() < 16:
            return []
        design = np.column_stack(
            [
                x_grid[valid].ravel(),
                y_grid[valid].ravel(),
                np.ones(valid.sum()),
            ]
        )
        coeffs, *_ = np.linalg.lstsq(design, arr[valid].ravel(), rcond=None)
        leveled = arr - (coeffs[0] * x_grid + coeffs[1] * y_grid + coeffs[2])
        flat = leveled - np.nanmedian(leveled, axis=1, keepdims=True)
        median = np.nanmedian(flat)
        mad = np.nanmedian(np.abs(flat - median))
        robust_sigma = 1.4826 * mad if mad > 0 else np.nanstd(flat)
        if not np.isfinite(robust_sigma) or robust_sigma == 0:
            return []

        score = np.abs(flat - median) / robust_sigma
        mask = score >= sigma_threshold
        candidates = np.argwhere(mask)
        if candidates.size == 0:
            return []
        order = np.argsort(score[mask])[::-1]
        bumps = []
        for idx in order:
            py, px = candidates[idx]
            if all(np.hypot(px - b["px"], py - b["py"]) >= min_sep_px for b in bumps):
                component = self._connected_component(mask, int(py), int(px))
                if component.size:
                    min_y = int(np.min(component[:, 0]))
                    max_y = int(np.max(component[:, 0]))
                    min_x = int(np.min(component[:, 1]))
                    max_x = int(np.max(component[:, 1]))
                    diameter_px = float(max(max_x - min_x + 1, max_y - min_y + 1))
                    area_px = int(component.shape[0])
                else:
                    min_y = max_y = int(py)
                    min_x = max_x = int(px)
                    diameter_px = 1.0
                    area_px = 1
                diameter_A = float(diameter_px * float(pixel_size_A))
                height_A = float(flat[py, px])
                abs_height_A = abs(height_A)
                if abs_height_A < float(molecule_height_A):
                    feature_class = "molecule_candidate"
                    avoid_for_clean_area = False
                    count_for_offset_stop = False
                elif diameter_A >= float(large_blob_diameter_A):
                    feature_class = "large_blob_hazard"
                    avoid_for_clean_area = True
                    count_for_offset_stop = True
                else:
                    feature_class = "tall_local_feature"
                    avoid_for_clean_area = True
                    count_for_offset_stop = False
                bumps.append(
                    {
                        "px": int(px),
                        "py": int(py),
                        "score_sigma": float(score[py, px]),
                        "height_A": height_A,
                        "abs_height_A": abs_height_A,
                        "diameter_px_est": diameter_px,
                        "diameter_A_est": diameter_A,
                        "area_px": area_px,
                        "bbox_px": [min_x, min_y, max_x, max_y],
                        "feature_class": feature_class,
                        "avoid_for_clean_area": avoid_for_clean_area,
                        "count_for_offset_stop": count_for_offset_stop,
                        "classification_note": (
                            "Molecule candidates are abs(dZ)<3 A. Large blob "
                            "hazards are taller features with estimated "
                            "diameter >=40 A."
                        ),
                    }
                )
            if len(bumps) >= max_bumps:
                break
        return bumps

    def _connected_component(self, mask, start_y, start_x, max_pixels=20000):
        """Return 8-connected mask component coordinates for one seed pixel."""
        if not mask[start_y, start_x]:
            return np.empty((0, 2), dtype=int)
        ny, nx = mask.shape
        visited = set()
        stack = [(int(start_y), int(start_x))]
        coords = []
        while stack and len(coords) < int(max_pixels):
            y, x = stack.pop()
            if (y, x) in visited:
                continue
            visited.add((y, x))
            if y < 0 or y >= ny or x < 0 or x >= nx or not mask[y, x]:
                continue
            coords.append((y, x))
            for yy in range(y - 1, y + 2):
                for xx in range(x - 1, x + 2):
                    if yy == y and xx == x:
                        continue
                    if 0 <= yy < ny and 0 <= xx < nx and (yy, xx) not in visited:
                        stack.append((yy, xx))
        return np.asarray(coords, dtype=int)

    def remember_scan_site(self, settings):
        site = {
            "timestamp": datetime.now().isoformat(),
            "OffsetX": settings.get("OffsetX"),
            "OffsetY": settings.get("OffsetY"),
            "ScanX": settings.get("ScanX"),
            "ScanY": settings.get("ScanY"),
            "RangeX": settings.get("RangeX"),
            "RangeY": settings.get("RangeY"),
            "PointsX": settings.get("PointsX"),
            "PointsY": settings.get("PointsY"),
            "Rotation": settings.get("Rotation"),
        }
        self.scan_site_history.append(site)
        return site

    def remember_bumps(self, bumps, settings):
        remembered = []
        for bump in bumps:
            if not bump.get("avoid_for_clean_area", True):
                continue
            target = self.pixel_to_scan_xy(bump["px"], bump["py"])
            item = {
                "timestamp": datetime.now().isoformat(),
                "OffsetX": settings.get("OffsetX"),
                "OffsetY": settings.get("OffsetY"),
                "Rotation": settings.get("Rotation"),
                "px": bump["px"],
                "py": bump["py"],
                "ScanX_A": target.get("ScanX_A"),
                "ScanY_A": target.get("ScanY_A"),
                "score_sigma": bump.get("score_sigma"),
                "height_A": bump.get("height_A"),
            }
            self.bump_history.append(item)
            remembered.append(item)
        return remembered

    def _patch_overlaps_recent_bump(self, cx, cy, mx, my, padding_px=12):
        for bump in self.bump_history[-50:]:
            px = bump.get("px")
            py = bump.get("py")
            if px is None or py is None:
                continue
            if abs(cx - px) <= mx + padding_px and abs(cy - py) <= my + padding_px:
                return True
        return False

    def pixel_to_scan_xy(self, px, py, channel=0):
        """
        Convert image pixel coordinates to centered ScanX/ScanY Angstroms.

        Pixel row indices count down from line 0 at the image top, but the
        physical scan coordinate system is right handed: positive local Y is
        upward in the image.  Use the returned ScanX_A/ScanY_A values for
        actual local tip moves.
        """
        if self.dry_run:
            return {"ScanX_A": None, "ScanY_A": None, "dry_run": True}
        geometry = ScanGeometry.from_settings(self._scan_settings(channel))
        rec = geometry.pixel_record(px, py)
        rec["row_down_y_A"] = -rec["local_scan_y_A"]
        return rec

    def move_to_scan_xy_fields(self, scan_x_A, scan_y_A, wait_after_s=2.0):
        """Move the tip by writing ScanX and ScanY GUI fields."""
        started = time.time()
        if not self.dry_run:
            gxsm = self.connect()
            gxsm.set("ScanX", scan_x_A)
            gxsm.set("ScanY", scan_y_A)
            time.sleep(wait_after_s)
            result = {
                "ScanX": gxsm.get("ScanX"),
                "ScanY": gxsm.get("ScanY"),
            }
        else:
            result = {"ScanX": scan_x_A, "ScanY": scan_y_A}
        return self._record(
            "move_to_scan_xy_fields",
            "gxsm",
            "dry-run" if self.dry_run else "ok",
            started,
            command="set ScanX/ScanY",
            result=result,
        )

    def get_live_tip_position_monitor(self, collect_as="live_tip_position"):
        """
        Read live GVP tip-position and bias monitor values.

        These are independent live monitor readbacks for current X/Y tip
        position, GVP/Pulse Z offset, and bias. X/Y/Z are in Angstrom; U is in
        Volts. During scanning, X/Y will change live with the raster. ZS is the
        offset applied by GVP/Pulse-style actions and normally should be zero or
        very small when no such action is active. For GVP setup, use these to
        verify actual state after the scan is stopped and the controller has
        settled.
        """
        started = time.time()
        if self.dry_run:
            result = {
                "dsp-GVP-XS-MONITOR": None,
                "dsp-GVP-YS-MONITOR": None,
                "dsp-GVP-ZS-MONITOR": None,
                "dsp-GVP-U-MONITOR": None,
                "dry_run": True,
            }
        else:
            gxsm = self.connect()
            result = {
                "dsp-GVP-XS-MONITOR": gxsm.get("dsp-GVP-XS-MONITOR"),
                "dsp-GVP-YS-MONITOR": gxsm.get("dsp-GVP-YS-MONITOR"),
                "dsp-GVP-ZS-MONITOR": gxsm.get("dsp-GVP-ZS-MONITOR"),
                "dsp-GVP-U-MONITOR": gxsm.get("dsp-GVP-U-MONITOR"),
            }
        if collect_as:
            self.data[collect_as] = result
        return self._record(
            "get_live_tip_position_monitor",
            "gxsm",
            "dry-run" if self.dry_run else "ok",
            started,
            command="get dsp-GVP-XS/YS/ZS/U-MONITOR",
            result=result,
        )

    def move_to_scan_xy_fields_verified(
        self,
        scan_x_A,
        scan_y_A,
        wait_after_s=3.0,
        tolerance_A=5.0,
    ):
        """
        Move the tip locally and verify with live GVP X/Y monitor readback."""
        move_record = self.move_to_scan_xy_fields(
            scan_x_A,
            scan_y_A,
            wait_after_s=wait_after_s,
        )
        monitor_record = self.get_live_tip_position_monitor()
        monitor = self.data.get("live_tip_position", {})
        try:
            dx = float(monitor["dsp-GVP-XS-MONITOR"]) - float(scan_x_A)
            dy = float(monitor["dsp-GVP-YS-MONITOR"]) - float(scan_y_A)
            verified = abs(dx) <= tolerance_A and abs(dy) <= tolerance_A
        except Exception:
            dx = None
            dy = None
            verified = False if not self.dry_run else True
        result = {
            "requested": {
                "ScanX_A": scan_x_A,
                "ScanY_A": scan_y_A,
            },
            "move_record": move_record.__dict__,
            "monitor_record": monitor_record.__dict__,
            "monitor": monitor,
            "delta_A": {
                "x": dx,
                "y": dy,
            },
            "tolerance_A": tolerance_A,
            "verified": verified,
        }
        self.data["last_verified_scan_xy_move"] = result
        return self._record(
            "move_to_scan_xy_fields_verified",
            "gxsm",
            "dry-run" if self.dry_run else ("ok" if verified else "unverified"),
            time.time(),
            command="set ScanX/ScanY + monitor readback",
            result=result,
        )

    def _watch_scan_lines(self, channel=0, min_lines=100, poll_s=10.0, timeout_s=900.0):
        gxsm = self.connect()
        dims = gxsm.get_dimensions(channel)
        ny = int(dims[1])
        started = time.time()
        progress = []
        while True:
            y_current = int(float(gxsm.y_current()))
            elapsed = time.time() - started
            progress.append({"elapsed_s": elapsed, "y_current": y_current})
            if y_current + 1 >= min_lines and y_current < ny:
                return progress
            if elapsed >= timeout_s:
                return progress
            if self.abort_requested():
                return progress
            time.sleep(poll_s)

    def _scan_settings(self, channel=0):
        if self.dry_run:
            return {"dry_run": True}
        gxsm = self.connect()
        keys = [
            "RangeX",
            "RangeY",
            "PointsX",
            "PointsY",
            "Rotation",
            "ScanX",
            "ScanY",
            "OffsetX",
            "OffsetY",
            "dsp-adv-scan-slope-x",
            "dsp-adv-scan-slope-y",
            "dsp-fbs-bias",
            "dsp-fbs-mx0-current-set",
        ]
        settings = {key: gxsm.get(key) for key in keys}
        settings["dimensions"] = list(gxsm.get_dimensions(channel))
        self.data["last_scan_settings"] = dict(settings)
        return settings

    def _pixel_size_A(self, channel=0):
        if self.dry_run:
            return 1.0
        geometry = ScanGeometry.from_settings(self._scan_settings(channel))
        return geometry.step_x_A

    def _autocorr_peaks(self, acn, max_peaks=10):
        ny, nx = acn.shape
        cy, cx = ny // 2, nx // 2
        y_grid, x_grid = np.mgrid[:ny, :nx]
        radius = np.hypot(y_grid - cy, x_grid - cx)
        mask = np.ones_like(acn, dtype=bool)
        mask[radius < 8] = False
        mask[radius > min(nx, ny) * 0.35] = False
        mask[:3, :] = False
        mask[-3:, :] = False
        mask[:, :3] = False
        mask[:, -3:] = False

        candidates = np.argwhere(mask)
        values = acn[mask]
        order = np.argsort(values)[::-1]
        peaks = []
        for idx in order[:2500]:
            py, px = candidates[idx]
            corr = float(acn[py, px])
            if corr < 0.08:
                break
            if all(np.hypot(py - qy, px - qx) >= 8 for qy, qx, _ in peaks):
                peaks.append((int(py), int(px), corr))
            if len(peaks) >= max_peaks:
                break
        return peaks

    def load_gvp_program(self, program, collect_as=None, verify_readback=True):
        """
        Load a full GVP program. This writes the program but does not execute it.

        Numeric vector entries use `dsp-gvp-*` and are written with gxsm.set.
        Conditional vector checkboxes are saved as `GETCHECK-dsp-gvp-*` keys
        with `TRUE`/`FALSE` values and restored via `CHECK-dsp-gvp-*` or
        `UNCHECK-dsp-gvp-*` actions.
        """
        started = time.time()
        flat = self._flatten_gvp_program(program)
        if not flat:
            raise ValueError("No GVP program fields supplied.")
        self._ensure_gvp_numeric_table(flat)
        self._complete_gvp_checkbox_states(flat)
        numeric, checks = self._split_gvp_program_fields(flat)

        if not self.dry_run:
            gxsm = self.connect()
            for key in sorted(numeric):
                gxsm.set(key, flat[key])
            for key in sorted(checks):
                gxsm.action(self._gvp_checkbox_set_action(key, checks[key]))
            if verify_readback:
                readback_numeric = {key: gxsm.get(key) for key in sorted(numeric)}
                readback_checks = {
                    key: gxsm.action(key) for key in sorted(checks)
                }
            else:
                readback_numeric = dict(numeric)
                readback_checks = dict(checks)
        else:
            readback_numeric = dict(numeric)
            readback_checks = dict(checks)

        readback = {}
        readback.update(readback_numeric)
        readback.update(readback_checks)
        mismatches = self._compare_gvp_program_dicts(flat, readback)
        result = {
            "key_count": len(flat),
            "numeric_key_count": len(numeric),
            "checkbox_key_count": len(checks),
            "verify_readback": bool(verify_readback),
            "mismatch_count": len(mismatches),
            "mismatches": mismatches,
            "nonzero_row_summary": self._gvp_nonzero_row_summary(readback_numeric),
            "checkbox_summary": self._gvp_checkbox_summary(readback_checks),
            "executed": False,
        }
        self.data["last_loaded_gvp"] = result
        if collect_as:
            self.data[collect_as] = result
        return self._record(
            "load_gvp_program",
            "gvp_load",
            "dry-run" if self.dry_run else "ok",
            started,
            command="set dsp-gvp* and CHECK/UNCHECK dsp-gvp*",
            result=result,
        )

    def build_gvp_null_stop_program(self):
        """
        Build an all-zero GVP table with all conditional flags unchecked.

        This is used as a rough emergency GVP stop pattern: overwrite every
        vector component with zero, including the zero-point termination rows,
        then execute the loaded NULL program.
        """
        flat = {}
        self._ensure_gvp_numeric_table(flat)
        self._complete_gvp_checkbox_states(flat)
        self._set_gvp_fb_flags(flat, checked_rows=())
        return [{key: flat[key]} for key in sorted(flat)]

    def load_gvp_null_stop_program(self, collect_as="last_gvp_null_stop_load"):
        program = self.build_gvp_null_stop_program()
        return self.load_gvp_program(
            program,
            collect_as=collect_as,
            verify_readback=False,
        )

    def emergency_stop_gvp(self):
        """Load the NULL GVP table and execute it immediately."""
        started = time.time()
        load_record = self.load_gvp_null_stop_program()
        execute_record = self.action("DSP_VP_VP_EXECUTE")
        result = {
            "emergency_stop": True,
            "method": "load NULL GVP vectors then DSP_VP_VP_EXECUTE",
            "load_record": load_record.__dict__,
            "execute_record": execute_record.__dict__,
        }
        self.data["last_gvp_emergency_stop"] = result
        return self._record(
            "gvp_emergency_stop",
            "gvp_stop",
            "dry-run" if self.dry_run else "ok",
            started,
            command="NULL GVP + DSP_VP_VP_EXECUTE",
            result=result,
        )

    def load_gvp_program_file(self, filename, collect_as=None):
        """Load a saved GVP JSON program compatible with gvp_save_restore.py."""
        with open(filename, "r") as file:
            program = json.load(file)
        return self.load_gvp_program(program, collect_as=collect_as)

    def fetch_current_gvp_program(self, include_checkboxes=True, collect_as=None):
        """
        Read the currently loaded GVP program from GXSM.

        The returned list-of-dicts format is compatible with gvp_save_restore.py:
        numeric `dsp-gvp-*` fields plus optional `GETCHECK-dsp-gvp-*` checkbox
        query entries.
        """
        started = time.time()
        if self.dry_run:
            program = []
            status = "dry-run"
        else:
            gxsm = self.connect()
            program = []
            for key in gxsm.list_refnames():
                if key.startswith("dsp-gvp"):
                    program.append({key: gxsm.get(key)})
            if include_checkboxes:
                for action in gxsm.list_actions():
                    if action.startswith("GETCHECK-dsp-gvp"):
                        program.append({action: gxsm.action(action)})
            status = "ok"

        result = {
            "program": program,
            "key_count": len(program),
            "include_checkboxes": include_checkboxes,
        }
        self.data["last_fetched_gvp_program"] = result
        if collect_as:
            self.data[collect_as] = result
        return self._record(
            "fetch_current_gvp_program",
            "gvp_read",
            status,
            started,
            command="gxsm.get dsp-gvp* and GETCHECK-dsp-gvp*",
            result=result,
        )

    def save_current_gvp_program(
        self,
        filename,
        include_checkboxes=True,
        collect_as=None,
    ):
        """Fetch and write the current GVP program JSON to disk."""
        record = self.fetch_current_gvp_program(
            include_checkboxes=include_checkboxes,
            collect_as=collect_as,
        )
        program = self.data["last_fetched_gvp_program"]["program"]
        Path(filename).write_text(json.dumps(self._jsonable(program), indent=2))
        return record

    def build_gvp_bias_pulse_program(
        self,
        pulse_du_V,
        template_file="gvp_bias_pulse_default_program.json",
        max_abs_du_V=4.0,
    ):
        """
        Build a GVP bias-pulse program from the captured default template.

        Positive pulse convention:
        - row 01: du = +pulse_du_V
        - row 03: du = -pulse_du_V, returning to the original bias
        """
        if abs(pulse_du_V) > max_abs_du_V:
            raise ValueError(
                "Refusing GVP bias pulse du={} V beyond +/-{} V.".format(
                    pulse_du_V,
                    max_abs_du_V,
                )
            )
        flat = self._load_gvp_template_flat(template_file)
        flat["dsp-gvp-du01"] = float(pulse_du_V)
        flat["dsp-gvp-du03"] = -float(pulse_du_V)
        self._set_gvp_fb_flags(flat, checked_rows=("00", "04"))
        return [{key: flat[key]} for key in sorted(flat)]

    def load_gvp_bias_pulse(
        self,
        pulse_du_V,
        template_file="gvp_bias_pulse_default_program.json",
        save_prefix=None,
    ):
        """Build and load a bias pulse GVP program, without executing it."""
        program = self.build_gvp_bias_pulse_program(
            pulse_du_V,
            template_file=template_file,
        )
        if save_prefix:
            Path("{}_program.json".format(save_prefix)).write_text(
                json.dumps(program, indent=2)
            )
            Path("{}_recipe.json".format(save_prefix)).write_text(
                json.dumps(
                    {
                        "name": save_prefix,
                        "action": "DSP_VP_VP_EXECUTE",
                        "pulse_du_V": pulse_du_V,
                        "executed": False,
                        "template_file": template_file,
                    },
                    indent=2,
                )
            )
        return self.load_gvp_program(program, collect_as="last_bias_pulse_load")

    def build_gvp_tip_dip_program(
        self,
        dip_depth_A=-5.0,
        bias_remove_V=-0.1,
        bias_restore_V=0.1,
        recovery_depth_A=None,
        contact_bias_V=None,
        scan_bias_V=None,
        ramp_time_s=30.0,
        template_file="gvp_tip_tune_template_current_latest.json",
        max_abs_dip_A=25.0,
        min_ramp_time_s=1.0,
        max_ramp_time_s=60.0,
    ):
        """
        Build a controlled tip-tune Z-dip GVP program from the captured template.

        This follows the operator-provided tip-tune pattern:
        - row 01: remove the actual scan bias while approaching
        - row 02: main indentation, default dz=-5 A
        - row 03: apply the requested contact bias while in contact
        - row 04: main pull-out, default dz=+5 A
        - row 06: remove the requested contact bias
        - row 07: restore the actual scan bias
        - rows 05..09: preserve the loaded template's recovery/tune pattern,
          with row 05/08 scaled from recovery_depth_A when supplied.

        dip_depth_A should be negative for downward indentation.  The main
        return row restores Z by the opposite amount.  If scan_bias_V is given,
        row 01/07 use `-scan_bias_V` and `+scan_bias_V`.  If contact_bias_V is
        given, row 03/06 use `+contact_bias_V` and `-contact_bias_V`.
        ramp_time_s sets the row 02 indentation and row 04 pull-out ramp time.
        """
        if abs(dip_depth_A) > max_abs_dip_A:
            raise ValueError(
                "Refusing GVP dip depth={} A beyond +/-{} A.".format(
                    dip_depth_A,
                    max_abs_dip_A,
                )
            )
        if dip_depth_A > 0:
            dip_depth_A = -float(dip_depth_A)
        actual_scan_bias_V = (
            float(scan_bias_V)
            if scan_bias_V is not None
            else float(bias_restore_V)
        )
        requested_contact_bias_V = (
            float(contact_bias_V)
            if contact_bias_V is not None
            else 0.0
        )
        bias_remove_V = -actual_scan_bias_V
        bias_restore_V = actual_scan_bias_V
        if recovery_depth_A is None:
            recovery_depth_A = 2.0 * abs(float(dip_depth_A))
        if abs(recovery_depth_A) > 2.0 * max_abs_dip_A:
            raise ValueError(
                "Refusing GVP recovery depth={} A beyond +/-{} A.".format(
                    recovery_depth_A,
                    2.0 * max_abs_dip_A,
                )
            )
        ramp_time_s = float(ramp_time_s)
        if not (float(min_ramp_time_s) <= ramp_time_s <= float(max_ramp_time_s)):
            raise ValueError(
                "Refusing GVP ramp_time_s={} outside {}..{} s.".format(
                    ramp_time_s,
                    float(min_ramp_time_s),
                    float(max_ramp_time_s),
                )
            )

        flat = self._load_gvp_template_flat(template_file)
        flat["dsp-gvp-du01"] = float(bias_remove_V)
        flat["dsp-gvp-dz02"] = float(dip_depth_A)
        flat["dsp-gvp-dt02"] = ramp_time_s
        flat["dsp-gvp-du03"] = float(requested_contact_bias_V)
        flat["dsp-gvp-dz04"] = -float(dip_depth_A)
        flat["dsp-gvp-dt04"] = ramp_time_s
        if "dsp-gvp-dz05" in flat:
            flat["dsp-gvp-dz05"] = float(recovery_depth_A)
        if "dsp-gvp-du06" in flat:
            flat["dsp-gvp-du06"] = -float(requested_contact_bias_V)
        if "dsp-gvp-du07" in flat:
            flat["dsp-gvp-du07"] = float(bias_restore_V)
        if "dsp-gvp-dz08" in flat:
            flat["dsp-gvp-dz08"] = -float(recovery_depth_A)
        self._set_gvp_fb_flags(flat, checked_rows=("00", "08"))
        return [{key: flat[key]} for key in sorted(flat)]

    def load_gvp_tip_dip(
        self,
        dip_depth_A=-5.0,
        bias_remove_V=-0.1,
        bias_restore_V=0.1,
        recovery_depth_A=None,
        contact_bias_V=None,
        scan_bias_V=None,
        ramp_time_s=30.0,
        template_file="gvp_tip_tune_template_current_latest.json",
        save_prefix=None,
    ):
        """Build and load a tip-dip GVP program, without executing it."""
        program = self.build_gvp_tip_dip_program(
            dip_depth_A=dip_depth_A,
            bias_remove_V=bias_remove_V,
            bias_restore_V=bias_restore_V,
            recovery_depth_A=recovery_depth_A,
            contact_bias_V=contact_bias_V,
            scan_bias_V=scan_bias_V,
            ramp_time_s=ramp_time_s,
            template_file=template_file,
        )
        if save_prefix:
            Path("{}_program.json".format(save_prefix)).write_text(
                json.dumps(program, indent=2)
            )
            Path("{}_recipe.json".format(save_prefix)).write_text(
                json.dumps(
                    {
                        "name": save_prefix,
                        "action": "DSP_VP_VP_EXECUTE",
                        "dip_depth_A": dip_depth_A,
                        "bias_remove_V": bias_remove_V,
                        "bias_restore_V": bias_restore_V,
                        "recovery_depth_A": recovery_depth_A,
                        "contact_bias_V": contact_bias_V,
                        "scan_bias_V": scan_bias_V,
                        "ramp_time_s": ramp_time_s,
                        "dt_mapping": {
                            "dsp-gvp-dt02": "ramp_time_s",
                            "dsp-gvp-dt04": "ramp_time_s",
                        },
                        "du_mapping": {
                            "dsp-gvp-du01": "-actual_scan_bias_V",
                            "dsp-gvp-du03": "+requested_contact_bias_V",
                            "dsp-gvp-du06": "-requested_contact_bias_V",
                            "dsp-gvp-du07": "+actual_scan_bias_V",
                        },
                        "executed": False,
                        "template_file": template_file,
                    },
                    indent=2,
                )
            )
        return self.load_gvp_program(program, collect_as="last_tip_dip_load")

    def execute_loaded_gvp(
        self,
        action_name="DSP_VP_VP_EXECUTE",
        wait_after_execute_s=4.0,
        ready_timeout_s=120.0,
        precheck=True,
    ):
        """Execute the currently loaded GVP program and wait for completion."""
        return self.execute_vp_probe(
            action_name=action_name,
            wait_after_execute_s=wait_after_execute_s,
            ready_timeout_s=ready_timeout_s,
            precheck=precheck,
            analyze=False,
        )

    def stop_scan_wait_for_gvp(
        self,
        stop_wait_s=10.0,
        ready_timeout_s=45.0,
        ready_delay_s=0.5,
    ):
        """
        Stop an active scan and allow the controller to settle before GVP.

        Operator note: after stopping a scan, wait a moment before starting a
        GVP.  Some controller status bits can remain set briefly; this method
        records the status history so the caller can decide whether to use the
        normal precheck or launch the already-loaded GVP explicitly.
        """
        started = time.time()
        if self.dry_run:
            result = {
                "dry_run": True,
                "stop_wait_s": stop_wait_s,
                "ready_timeout_s": ready_timeout_s,
            }
        else:
            gxsm = self.connect()
            stop_ret = gxsm.stopscan()
            time.sleep(stop_wait_s)
            status_history = []
            deadline = time.monotonic() + ready_timeout_s
            while True:
                status = int(gxsm.rtquery("s")[0])
                status_history.append(
                    {
                        "elapsed_s": time.time() - started,
                        "status": status,
                        "busy_masked": bool(status & self.DSP_BUSY_MASK),
                    }
                )
                if not (status & self.DSP_BUSY_MASK):
                    break
                if time.monotonic() >= deadline:
                    break
                time.sleep(ready_delay_s)
            result = {
                "stopscan_return": stop_ret,
                "stop_wait_s": stop_wait_s,
                "ready_timeout_s": ready_timeout_s,
                "status_history": status_history,
                "final_status": status_history[-1]["status"] if status_history else None,
                "ready": (
                    bool(status_history)
                    and not status_history[-1]["busy_masked"]
                ),
            }
        self.data["last_stop_scan_wait_for_gvp"] = result
        return self._record(
            "stop_scan_wait_for_gvp",
            "gxsm",
            "dry-run" if self.dry_run else "ok",
            started,
            command="stopscan + rtquery:s",
            result=result,
        )

    def tip_improvement_script(
        self,
        channel=0,
        execute_actions=False,
        pulse_sequence_V=(2.0, 3.0),
        dip_sequence_A=(5.0,),
        scan_bias_V=0.1,
        scan_current_nA=0.01,
        diagnostic_range_A=700.0,
        fine_range_A=400.0,
        points=400,
        min_lines=100,
        output_prefix="tip_improvement",
        allow_offset_search=False,
    ):
        """
        Full learned tip-improvement workflow.

        Sequence:
        1. scan/watch enough lines and assess tip quality,
        2. if poor, find a clean patch away from blobs,
        3. if execute_actions=True, move to that patch,
        4. try gentle 2..3 V bias pulses, reassessing after each,
        5. if still poor, try controlled tip dips starting around 5 A,
        6. if no patch is available, optionally request/perform offset search.

        By default this returns a plan only. Live pulse/dip execution requires
        execute_actions=True on a non-dry runner.
        """
        started = time.time()
        if self.dry_run:
            result = {
                "status": "planned",
                "dry_run": True,
                "sequence": {
                    "scan_and_assess": {
                        "channel": channel,
                        "min_lines": min_lines,
                        "range_A": diagnostic_range_A,
                        "points": points,
                    },
                    "if_poor": [
                        "find clean patch away from blobs",
                        "move to clean ScanX/ScanY patch",
                        "try bias pulse ladder",
                        "re-scan/reassess after each action",
                        "try dip ladder if pulses do not improve tip",
                    ],
                    "pulse_sequence_V": list(pulse_sequence_V),
                    "dip_sequence_A": list(dip_sequence_A),
                    "nominal_scan_bias_V": scan_bias_V,
                    "nominal_scan_current_nA": scan_current_nA,
                    "fine_range_A_after_improvement": fine_range_A,
                    "allow_offset_search": allow_offset_search,
                },
            }
            self.data["last_tip_improvement"] = result
            return self._record(
                "tip_improvement_script",
                "tip_improvement",
                "dry-run",
                started,
                command="tip_improvement_script",
                result=result,
            )

        stages = []

        assessment = self.scan_watch_assess_tip(
            channel=channel,
            min_lines=min_lines,
            start_scan=True,
            range_A=diagnostic_range_A,
            points=points,
            move_if_poor=False,
            output_prefix="{}_initial".format(output_prefix),
        )
        workflow = self.data.get("last_tip_workflow", {})
        quality = workflow.get("quality", {})
        poor = workflow.get("poor_tip", self.tip_quality_is_poor(quality))
        clean_patch = workflow.get("clean_patch")
        stages.append({"stage": "initial_assessment", "poor_tip": poor})

        if not poor:
            result = {"status": "tip_ok", "stages": stages, "workflow": workflow}
            self.data["last_tip_improvement"] = result
            return self._record(
                "tip_improvement_script",
                "tip_improvement",
                "ok",
                started,
                command="tip_improvement_script",
                result=result,
            )

        if clean_patch is None:
            status = "needs_clean_area"
            if allow_offset_search:
                status = "needs_offset_search"
            result = {
                "status": status,
                "stages": stages,
                "workflow": workflow,
                "note": (
                    "No clean patch found. Move OffsetX/OffsetY to search for "
                    "a larger clean area before tip tuning."
                ),
            }
            self.data["last_tip_improvement"] = result
            return self._record(
                "tip_improvement_script",
                "tip_improvement",
                "planned" if not execute_actions else "blocked",
                started,
                command="tip_improvement_script",
                result=result,
            )

        plan = {
            "clean_patch": clean_patch,
            "pulse_sequence_V": list(pulse_sequence_V),
            "dip_sequence_A": list(dip_sequence_A),
            "scan_bias_V": scan_bias_V,
            "scan_current_nA": scan_current_nA,
            "diagnostic_range_A": diagnostic_range_A,
            "fine_range_A": fine_range_A,
            "points": points,
            "execute_actions": execute_actions,
            "note": (
                "After each action, expect a more or less large blob at the "
                "previous action location. Smaller, rounder, more symmetric "
                "blobs are better if tip quality improves."
            ),
        }

        if not execute_actions or self.dry_run:
            result = {
                "status": "planned",
                "stages": stages,
                "workflow": workflow,
                "plan": plan,
            }
            self.data["last_tip_improvement"] = result
            return self._record(
                "tip_improvement_script",
                "tip_improvement",
                "dry-run" if self.dry_run else "planned",
                started,
                command="tip_improvement_script",
                result=result,
            )

        self._set_nominal_stm_scan_conditions(scan_bias_V, scan_current_nA)
        self.connect().stopscan()
        time.sleep(3.0)
        self.move_to_scan_xy_fields(
            clean_patch["scan_target"]["ScanX_A"],
            clean_patch["scan_target"]["ScanY_A"],
        )

        for pulse_V in pulse_sequence_V:
            self.load_gvp_bias_pulse(
                pulse_V,
                save_prefix="{}_pulse_{:.1f}V".format(output_prefix, pulse_V),
            )
            self.execute_loaded_gvp()
            post = self.scan_watch_assess_tip(
                channel=channel,
                min_lines=min_lines,
                start_scan=True,
                range_A=diagnostic_range_A,
                points=points,
                move_if_poor=False,
                output_prefix="{}_after_pulse_{:.1f}V".format(output_prefix, pulse_V),
            )
            post_quality = self.data.get("last_tip_workflow", {}).get("quality", {})
            still_poor = self.tip_quality_is_poor(post_quality)
            stages.append(
                {
                    "stage": "pulse",
                    "pulse_V": pulse_V,
                    "still_poor": still_poor,
                    "record": post.__dict__,
                }
            )
            if not still_poor:
                result = {"status": "improved_after_pulse", "stages": stages}
                self.data["last_tip_improvement"] = result
                return self._record(
                    "tip_improvement_script",
                    "tip_improvement",
                    "ok",
                    started,
                    command="tip_improvement_script",
                    result=result,
                )

        for dip_A in dip_sequence_A:
            workflow = self.data.get("last_tip_workflow", {})
            clean_patch = workflow.get("clean_patch") or clean_patch
            if clean_patch and "scan_target" in clean_patch:
                self.connect().stopscan()
                time.sleep(3.0)
                self.move_to_scan_xy_fields(
                    clean_patch["scan_target"]["ScanX_A"],
                    clean_patch["scan_target"]["ScanY_A"],
                )
            self.load_gvp_tip_dip(
                dip_depth_A=-abs(dip_A),
                contact_bias_V=0.0,
                scan_bias_V=scan_bias_V,
                save_prefix="{}_dip_{:.1f}A".format(output_prefix, abs(dip_A)),
            )
            self.execute_loaded_gvp()
            post = self.scan_watch_assess_tip(
                channel=channel,
                min_lines=min_lines,
                start_scan=True,
                range_A=diagnostic_range_A,
                points=points,
                move_if_poor=False,
                output_prefix="{}_after_dip_{:.1f}A".format(output_prefix, abs(dip_A)),
            )
            post_quality = self.data.get("last_tip_workflow", {}).get("quality", {})
            still_poor = self.tip_quality_is_poor(post_quality)
            stages.append(
                {
                    "stage": "dip",
                    "dip_A": abs(dip_A),
                    "still_poor": still_poor,
                    "record": post.__dict__,
                }
            )
            if not still_poor:
                result = {"status": "improved_after_dip", "stages": stages}
                self.data["last_tip_improvement"] = result
                return self._record(
                    "tip_improvement_script",
                    "tip_improvement",
                    "ok",
                    started,
                    command="tip_improvement_script",
                    result=result,
                )

        result = {"status": "still_poor_after_sequence", "stages": stages}
        self.data["last_tip_improvement"] = result
        return self._record(
            "tip_improvement_script",
            "tip_improvement",
            "needs_operator",
            started,
            command="tip_improvement_script",
            result=result,
        )

    def _set_nominal_stm_scan_conditions(self, bias_V=0.1, current_nA=0.01):
        """Set nominal STM scan conditions used for gentle tip tuning."""
        if self.dry_run:
            return
        if abs(bias_V) > 1.0:
            raise ValueError("Nominal scan bias exceeds +/-1 V auto limit.")
        if current_nA > 0.05:
            raise ValueError("Nominal scan current exceeds 0.05 nA safety limit.")
        gxsm = self.connect()
        gxsm.set("dsp-fbs-bias", bias_V)
        gxsm.set("dsp-fbs-mx0-current-set", current_nA)

    def _load_gvp_template_flat(self, template_file):
        with open(template_file, "r") as file:
            return self._flatten_gvp_program(json.load(file))

    def _flatten_gvp_program(self, program):
        if isinstance(program, dict):
            return dict(program)
        flat = {}
        for item in program:
            flat.update(item)
        return flat

    def _split_gvp_program_fields(self, flat_program):
        numeric = {}
        checks = {}
        for key, value in flat_program.items():
            if key.startswith("GETCHECK-dsp-gvp"):
                checks[key] = self._normalize_gvp_checkbox(value)
            elif key.startswith("dsp-gvp"):
                numeric[key] = value
            else:
                numeric[key] = value
        return numeric, checks

    def _normalize_gvp_checkbox(self, value):
        if isinstance(value, bool):
            return "TRUE" if value else "FALSE"
        text = str(value).strip().upper()
        if text in ("TRUE", "1", "YES", "ON", "CHECKED"):
            return "TRUE"
        if text in ("FALSE", "0", "NO", "OFF", "UNCHECKED", ""):
            return "FALSE"
        raise ValueError("Invalid GVP checkbox value: {!r}".format(value))

    def _gvp_checkbox_set_action(self, getcheck_key, value):
        normalized = self._normalize_gvp_checkbox(value)
        if not getcheck_key.startswith("GETCHECK-dsp-gvp"):
            raise ValueError("Not a GVP checkbox query key: {}".format(getcheck_key))
        base_action = getcheck_key[3:]
        if normalized == "TRUE":
            return base_action
        return "UN{}".format(base_action)

    def _set_gvp_fb_flags(self, flat_program, checked_rows):
        """
        Ensure GVP feedback/control checkbox states are explicit.

        Existing saved FB query keys are preserved but rewritten to TRUE only
        for checked_rows.  If the template has no checkbox keys yet, create the
        commonly used FB rows so a load can correct stale live checkbox state.
        """
        checked_rows = {str(row).zfill(2) for row in checked_rows}
        fb_keys = [
            key for key in flat_program
            if key.startswith("GETCHECK-dsp-gvp-FB")
        ]
        if not fb_keys:
            candidate_rows = sorted(checked_rows | {"00", "04", "08"})
            fb_keys = [
                "GETCHECK-dsp-gvp-FB{}".format(row)
                for row in candidate_rows
            ]
        for key in fb_keys:
            row = key[-2:]
            flat_program[key] = "TRUE" if row in checked_rows else "FALSE"

    def _complete_gvp_checkbox_states(self, flat_program):
        """
        Fill all known GVP checkbox states explicitly.

        GXSM vector checkbox state persists from previous programs.  Therefore
        every load must write TRUE and FALSE values for all known conditional
        vector flags, not only the checked ones.
        """
        for row in range(32):
            row_id = "{:02d}".format(row)
            flat_program.setdefault("GETCHECK-dsp-gvp-FB{}".format(row_id), "FALSE")
            flat_program.setdefault("GETCHECK-dsp-gvp-VS{}".format(row_id), "FALSE")
        flat_program.setdefault("GETCHECK-dsp-gvp-RFAM", "FALSE")
        flat_program.setdefault("GETCHECK-dsp-gvp-RFFM", "FALSE")

    def _ensure_gvp_numeric_table(self, flat_program):
        """
        Fill the standard 32-row GVP numeric table with zeros if keys are absent.

        Captured templates already include the full 512 numeric table.  This
        fallback protects hand-built programs so stale GUI values do not leak
        past the intended terminating row.
        """
        fields = (
            "du",
            "dx",
            "dy",
            "dz",
            "da",
            "db",
            "dam",
            "dfm",
            "dt",
            "n",
            "xopcd",
            "xrchi",
            "xcmpv",
            "xjmpr",
            "nrep",
            "pcjr",
        )
        for row in range(32):
            row_id = "{:02d}".format(row)
            for field in fields:
                flat_program.setdefault(
                    "dsp-gvp-{}{}".format(field, row_id),
                    0.0,
                )

    def _compare_gvp_program_dicts(self, expected, actual):
        mismatches = {}
        for key, expected_value in expected.items():
            actual_value = actual.get(key)
            if key.startswith("GETCHECK-dsp-gvp"):
                try:
                    ok = (
                        self._normalize_gvp_checkbox(actual_value)
                        == self._normalize_gvp_checkbox(expected_value)
                    )
                except Exception:
                    ok = actual_value == expected_value
            else:
                try:
                    ok = abs(float(actual_value) - float(expected_value)) < 1e-12
                except Exception:
                    ok = actual_value == expected_value
            if not ok:
                mismatches[key] = {
                    "expected": expected_value,
                    "actual": actual_value,
                }
        return mismatches

    def _compare_float_dicts(self, expected, actual):
        mismatches = {}
        for key, expected_value in expected.items():
            actual_value = actual.get(key)
            try:
                ok = abs(float(actual_value) - float(expected_value)) < 1e-12
            except Exception:
                ok = actual_value == expected_value
            if not ok:
                mismatches[key] = {
                    "expected": expected_value,
                    "actual": actual_value,
                }
        return mismatches

    def _gvp_nonzero_row_summary(self, flat_program):
        rows = {}
        for key, value in flat_program.items():
            row = key[-2:]
            field = key[len("dsp-gvp-"):-2]
            rows.setdefault(row, {})[field] = value

        summary = {}
        for row in sorted(rows):
            nonzero_fields = {}
            for field, value in sorted(rows[row].items()):
                try:
                    if float(value) != 0.0:
                        nonzero_fields[field] = value
                except Exception:
                    if value:
                        nonzero_fields[field] = value
            if nonzero_fields:
                summary[row] = nonzero_fields
        return summary

    def _gvp_checkbox_summary(self, check_program):
        summary = {}
        for key, value in sorted(check_program.items()):
            suffix = key[len("GETCHECK-dsp-gvp-"):]
            field = suffix[:-2]
            row = suffix[-2:]
            summary.setdefault(row, {})[field] = self._normalize_gvp_checkbox(value)
        return summary

    def vp_probe_plan(
        self,
        action_name="DSP_VP_VP_EXECUTE",
        channel=None,
        start_idx=600,
        end_idx=1600,
        analyze=True,
    ):
        return [
            ActionStep(
                kind="vp_probe",
                name="execute_and_fetch_vp",
                kwargs={
                    "action_name": action_name,
                    "channel": self.default_channel if channel is None else channel,
                    "start_idx": start_idx,
                    "end_idx": end_idx,
                    "analyze": analyze,
                },
                collect_as="vp_probe",
            ),
            ActionStep(
                kind="postprocess",
                name="compare_zstopo_windows",
                kwargs={
                    "processor": self.compare_signal_windows,
                    "data": None,
                },
                collect_as="zstopo_window_comparison",
            ),
        ]

    def export_history(self, filename):
        with open(filename, "w") as file:
            json.dump([record.__dict__ for record in self.history], file, indent=2)

    def _vpdata_from_probe_result(self, probe_result, start_idx, end_idx):
        if probe_result is None:
            return None

        if len(probe_result) == 4:
            columns, labels, units, xy = probe_result
        elif len(probe_result) == 3:
            columns, labels, units = probe_result
            xy = None
        else:
            raise ValueError(
                "Unexpected probe result length: {}".format(len(probe_result))
            )

        sliced_columns = columns[:, start_idx:end_idx]
        vpdata = dict(zip(labels, sliced_columns))
        vpunits = dict(zip(labels, units))
        return {
            "vpdata": vpdata,
            "vpunits": vpunits,
            "xy": xy,
            "labels": labels,
            "columns": columns,
            "start_idx": start_idx,
            "end_idx": end_idx,
        }

    def _resolve_processor(self, processor):
        if callable(processor):
            return processor
        if ":" not in processor:
            raise ValueError(
                "Processor must be callable or 'module:function', got {}".format(
                    processor
                )
            )
        module_name, function_name = processor.split(":", 1)
        module = importlib.import_module(module_name)
        return getattr(module, function_name)

    def _record(
        self,
        name,
        kind,
        status,
        started,
        command="",
        details=None,
        result=None,
        error="",
    ):
        record = ActionRecord(
            name=name,
            kind=kind,
            status=status,
            started_at=datetime.fromtimestamp(started).isoformat(),
            elapsed_s=time.time() - started,
            command=command,
            details=self._jsonable(details or {}),
            result_summary=self._summarize(result),
            error=error,
        )
        self.history.append(record)
        return record

    def _summarize(self, value):
        if isinstance(value, ActionRecord):
            return value.__dict__
        if isinstance(value, np.ndarray):
            return {
                "type": "ndarray",
                "shape": list(value.shape),
                "stats": self._array_stats(value),
            }
        if isinstance(value, dict):
            summary = {"type": "dict", "keys": list(value.keys())}
            if "vpdata" in value:
                summary["signals"] = list(value["vpdata"].keys())
            if "labels" in value:
                summary["labels"] = self._jsonable(value["labels"])
            return summary
        if isinstance(value, (list, tuple)):
            return {
                "type": type(value).__name__,
                "len": len(value),
                "preview": self._jsonable(value[:5]),
            }
        return self._jsonable(value)

    def _array_stats(self, arr):
        return self._array_stats_static(arr)

    @staticmethod
    def _array_stats_static(arr):
        arr = np.asarray(arr)
        if arr.size == 0:
            return {"len": 0}
        return {
            "len": int(arr.size),
            "min": float(np.nanmin(arr)),
            "max": float(np.nanmax(arr)),
            "mean": float(np.nanmean(arr)),
            "std": float(np.nanstd(arr)),
        }

    def _jsonable(self, value):
        if isinstance(value, np.ndarray):
            return self._summarize(value)
        if isinstance(value, np.floating):
            value = float(value)
            return value if np.isfinite(value) else None
        if isinstance(value, np.generic):
            return value.item()
        if isinstance(value, float):
            return value if np.isfinite(value) else None
        if isinstance(value, dict):
            return {str(k): self._jsonable(v) for k, v in value.items()}
        if isinstance(value, tuple):
            return [self._jsonable(v) for v in value]
        if isinstance(value, list):
            return [self._jsonable(v) for v in value]
        try:
            json.dumps(value)
            return value
        except TypeError:
            return repr(value)


def format_exception():
    return traceback.format_exc()


class THV5CoarseController:
    """
    Minimal THV5 HTTP wrapper for Level-3 coarse motion.

    This mirrors the existing pyaction watchdog snippet but keeps requests
    explicit, dry-run aware, and easy to audit from action reports.
    """

    def __init__(self, base_url="http://192.168.40.10/", dry_run=True, timeout_s=3.0):
        self.base_url = str(base_url).rstrip("/") + "/"
        self.dry_run = bool(dry_run)
        self.timeout_s = float(timeout_s)

    def coarse_move(self, channel, direction, burstcount, period_s, voltage_V, start=True):
        channel = str(channel).upper()
        if channel not in ("X", "Y", "Z"):
            raise ValueError("THV coarse channel must be X, Y, or Z.")
        direction = int(direction)
        if direction not in (-1, 1):
            raise ValueError("THV coarse direction must be -1 or +1.")
        burstcount = int(burstcount)
        if burstcount < 0:
            raise ValueError("THV burstcount must be non-negative.")
        period_ms = int(1000.0 * float(period_s))
        voltage = int(voltage_V) if start else 0
        c0 = burstcount * direction if start else 0
        path = "coarse?v={}&p0={}&a0={}&c0={}&bs=0".format(
            voltage,
            period_ms if start else 0,
            channel,
            c0,
        )
        url = self.base_url + path
        if self.dry_run:
            return {
                "dry_run": True,
                "url": url,
                "channel": channel,
                "direction": direction,
                "burstcount": burstcount,
                "period_s": float(period_s),
                "voltage_V": float(voltage_V),
                "start": bool(start),
            }
        response = requests.get(url, timeout=self.timeout_s)
        return {
            "url": url,
            "status_code": response.status_code,
            "text": response.text[:500],
        }

    def tip_down_app(self, burstcount):
        return self.coarse_move("Z", -1, burstcount, 0.5, 30.0, start=True)


class Level3ProtectedController:
    """
    Protected Level-3 workflows: coarse motion, hyper jumps, auto approach.

    These methods intentionally do not run through the Level-1 chat gate. The
    GUI must enforce Level 3, a separate arm checkbox, and typed confirmation.
    """

    def __init__(
        self,
        runner,
        thv_base_url="http://192.168.40.10/",
        output_dir=".",
    ):
        self.runner = runner
        self.thv = THV5CoarseController(
            thv_base_url,
            dry_run=runner.dry_run,
        )
        self.last_coarse_motion_request = None
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)

    def VZ_tip(self):
        """Read tip Z voltage proxy from rtquery('z'), matching watchdog sign."""
        if self.runner.dry_run:
            return None
        zvec = self.runner.connect().rtquery("z")
        return -float(zvec[0])

    def user_ok(self):
        if self.runner.dry_run:
            return True
        try:
            return int(float(self.runner.connect().get(self.runner.abort_refname))) > 0
        except Exception:
            return False

    def set_script_control(self, enabled):
        return self.runner.set_parameter(
            self.runner.abort_refname,
            "1" if enabled else "0",
        )

    def telemetry(self):
        df = self.runner.dFreq()
        return {
            "dFreq_Hz": df,
            "dFreq_interpretation": self.runner.interpret_dFreq(df),
            "VZ_tip_V": self.VZ_tip(),
            "user_ok": self.user_ok(),
        }

    def coarse_move(
        self,
        channel,
        direction,
        burstcount,
        period_s=0.5,
        voltage_V=30.0,
        max_burstcount=5,
        max_voltage_V=100.0,
    ):
        started = time.time()
        if int(burstcount) > int(max_burstcount):
            raise ValueError(
                "Burstcount {} exceeds Level-3 max_burstcount {}.".format(
                    int(burstcount),
                    int(max_burstcount),
                )
            )
        if abs(float(voltage_V)) > float(max_voltage_V):
            raise ValueError(
                "Voltage {} exceeds Level-3 max_voltage_V {}.".format(
                    float(voltage_V),
                    float(max_voltage_V),
                )
            )
        before = self.telemetry()
        danger = str(channel).upper() == "Z" and int(direction) == -1
        result = self.thv.coarse_move(
            channel,
            int(direction),
            int(burstcount),
            float(period_s),
            float(voltage_V),
            start=True,
        )
        self.last_coarse_motion_request = {
            "channel": str(channel).upper(),
            "direction": int(direction),
            "burstcount": int(burstcount),
            "period_s": float(period_s),
            "voltage_V": float(voltage_V),
            "started_at": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "danger": danger,
        }
        time.sleep(0.2 if not self.runner.dry_run else 0.0)
        after = self.telemetry()
        report = {
            "before": before,
            "thv_result": result,
            "after": after,
            "limits": {
                "max_burstcount": int(max_burstcount),
                "max_voltage_V": float(max_voltage_V),
            },
            "danger": danger,
            "danger_note": (
                "Z direction -1 is a fast tip-down coarse motion."
                if danger else ""
            ),
        }
        self.runner.data["last_level3_coarse_motion_request"] = (
            self.last_coarse_motion_request
        )
        self.runner.data["last_level3_coarse_move"] = report
        return self.runner._record(
            "level3_coarse_move",
            "level3",
            "dry-run" if self.runner.dry_run else "ok",
            started,
            command="THV5 coarse move",
            details={
                "channel": channel,
                "direction": direction,
                "burstcount": burstcount,
                "period_s": period_s,
                "voltage_V": voltage_V,
                "danger": danger,
                "danger_note": report["danger_note"],
            },
            result=report,
        )

    def emergency_stop_coarse_motion(self):
        """
        Immediately send THV5 stop commands for coarse motion.

        This is intentionally not gated by Level-3 confirmation: the GUI panic
        button must be able to send zero-step, zero-volt stop commands without
        asking anything first.
        """
        started = time.time()
        before = self.telemetry()
        last = (
            self.last_coarse_motion_request
            or self.runner.data.get("last_level3_coarse_motion_request")
            or {}
        )
        sequence = []
        seen = set()

        def add_stop(channel, direction):
            channel = str(channel).upper()
            direction = int(direction)
            key = (channel, direction)
            if key not in seen:
                seen.add(key)
                sequence.append(key)

        if last.get("channel") in ("X", "Y", "Z"):
            add_stop(last.get("channel"), last.get("direction", 1))

        for axis in ("X", "Y", "Z"):
            add_stop(axis, 1)
            add_stop(axis, -1)

        stop_results = []
        for channel, direction in sequence:
            try:
                stop_results.append(
                    {
                        "channel": channel,
                        "direction": direction,
                        "result": self.thv.coarse_move(
                            channel,
                            direction,
                            0,
                            0.0,
                            0.0,
                            start=False,
                        ),
                    }
                )
            except Exception as exc:
                stop_results.append(
                    {
                        "channel": channel,
                        "direction": direction,
                        "error": str(exc),
                    }
                )

        after = self.telemetry()
        report = {
            "emergency_stop": True,
            "before": before,
            "last_coarse_motion_request": last,
            "stop_sequence": stop_results,
            "after": after,
            "note": (
                "Sent zero-step, zero-volt THV5 coarse stop commands. "
                "The remembered last axis/direction was stopped first."
            ),
        }
        self.runner.data["last_level3_emergency_stop"] = report
        return self.runner._record(
            "level3_emergency_stop_coarse_motion",
            "level3",
            "dry-run" if self.runner.dry_run else "ok",
            started,
            command="THV5 emergency coarse stop",
            details={
                "last_axis": last.get("channel"),
                "last_direction": last.get("direction"),
                "fanout_stop_count": len(stop_results),
            },
            result=report,
        )

    def retract(
        self,
        current_zero=True,
        z_retract_threshold_V=-100.0,
        poll_s=0.5,
        timeout_s=60.0,
    ):
        started = time.time()
        log = []
        if current_zero:
            self.runner.set_parameter("dsp-fbs-mx0-current-set", "0")
        while True:
            tel = self.telemetry()
            tel["elapsed_s"] = time.time() - started
            log.append(tel)
            vz = tel.get("VZ_tip_V")
            if self.runner.dry_run:
                break
            if not tel.get("user_ok"):
                break
            if vz is not None and float(vz) <= float(z_retract_threshold_V):
                break
            if tel["elapsed_s"] >= float(timeout_s):
                break
            time.sleep(float(poll_s))
        return log

    def check_range(
        self,
        current_nA=0.03,
        z_target_V=100.0,
        timeout_s=10.0,
        poll_s=0.5,
    ):
        started = time.time()
        log = []
        self.runner.set_live_current_setpoint(
            current_nA,
            unit="nA",
            max_auto_nA=max(float(current_nA), 0.05),
        )
        while True:
            tel = self.telemetry()
            tel["elapsed_s"] = time.time() - started
            log.append(tel)
            vz = tel.get("VZ_tip_V")
            if self.runner.dry_run:
                return {"reached": False, "log": log}
            if not tel.get("user_ok"):
                return {"reached": False, "log": log, "abort_reason": "script-control"}
            if vz is not None and float(vz) >= float(z_target_V):
                return {"reached": True, "log": log}
            if tel["elapsed_s"] >= float(timeout_s):
                return {"reached": False, "log": log, "abort_reason": "timeout"}
            time.sleep(float(poll_s))

    def run_auto_approach(
        self,
        current_nA=0.013,
        coarse_steps_per_cycle=1,
        max_total_steps=10,
        max_abs_dFreq_Hz=5.0,
        check_timeout_s=30.0,
        z_target_V=100.0,
        retract_threshold_V=-100.0,
        thv_period_s=0.5,
        thv_voltage_V=30.0,
        poll_s=0.5,
        output_prefix=None,
    ):
        started = time.time()
        report = {
            "started_at": datetime.fromtimestamp(started).isoformat(),
            "parameters": {
                "current_nA": float(current_nA),
                "coarse_steps_per_cycle": int(coarse_steps_per_cycle),
                "max_total_steps": int(max_total_steps),
                "max_abs_dFreq_Hz": float(max_abs_dFreq_Hz),
                "check_timeout_s": float(check_timeout_s),
                "z_target_V": float(z_target_V),
                "retract_threshold_V": float(retract_threshold_V),
                "thv_period_s": float(thv_period_s),
                "thv_voltage_V": float(thv_voltage_V),
            },
            "cycles": [],
            "total_steps": 0,
            "status": "running",
        }
        if int(coarse_steps_per_cycle) < 1:
            raise ValueError("coarse_steps_per_cycle must be >= 1.")
        if int(max_total_steps) < int(coarse_steps_per_cycle):
            raise ValueError("max_total_steps must be >= coarse_steps_per_cycle.")

        self.set_script_control(True)
        while report["total_steps"] < int(max_total_steps):
            tel = self.telemetry()
            df = tel.get("dFreq_Hz")
            if df is not None and abs(float(df)) >= float(max_abs_dFreq_Hz):
                report["status"] = "stopped_dFreq_limit_before_step"
                report["final_telemetry"] = tel
                break
            if not tel.get("user_ok"):
                report["status"] = "stopped_script_control"
                report["final_telemetry"] = tel
                break

            check = self.check_range(
                current_nA=float(current_nA),
                z_target_V=float(z_target_V),
                timeout_s=float(check_timeout_s),
                poll_s=float(poll_s),
            )
            cycle = {"check_range": check}
            if check.get("reached"):
                report["status"] = "finished_range_reached"
                cycle["note"] = "Z range reached under current setpoint."
                report["cycles"].append(cycle)
                break

            cycle["retract"] = self.retract(
                current_zero=True,
                z_retract_threshold_V=float(retract_threshold_V),
                poll_s=float(poll_s),
                timeout_s=float(check_timeout_s),
            )
            move = self.thv.tip_down_app(int(coarse_steps_per_cycle))
            report["total_steps"] += int(coarse_steps_per_cycle)
            cycle["coarse_move"] = move
            cycle["after_move"] = self.telemetry()
            report["cycles"].append(cycle)

            df_after = cycle["after_move"].get("dFreq_Hz")
            if df_after is not None and abs(float(df_after)) >= float(max_abs_dFreq_Hz):
                report["status"] = "stopped_dFreq_limit_after_step"
                report["final_telemetry"] = cycle["after_move"]
                break
            if self.runner.dry_run:
                report["status"] = "dry-run"
                break
            time.sleep(float(poll_s))
        else:
            report["status"] = "stopped_max_total_steps"
            report["final_telemetry"] = self.telemetry()

        if "final_telemetry" not in report:
            report["final_telemetry"] = self.telemetry()
        report["elapsed_s"] = time.time() - started
        self.runner.data["last_level3_auto_approach"] = report
        if output_prefix:
            Path("{}_level3_auto_approach.json".format(output_prefix)).write_text(
                json.dumps(self.runner._jsonable(report), indent=2)
            )
        return self.runner._record(
            "level3_auto_approach",
            "level3",
            "dry-run" if self.runner.dry_run else report["status"],
            started,
            command="protected auto approach",
            details=report["parameters"],
            result=report,
        )


class TipImprovementActivityController:
    """
    Top-level activity controller for iterative tip-improvement workflows.

    This class orchestrates the learned operator procedure:
    sample dFrequency, gather fresh scan evidence, compare edge sharpness,
    stop scanning, apply a selected GVP pulse, fetch the resulting VP event,
    and log enough data to decide whether to repeat, escalate, or stop.

    Hardware-changing steps still go through the underlying runner and should
    only be used with explicit operator intent on a non-dry runner.
    """

    def __init__(self, runner=None, output_dir="."):
        self.runner = runner or MicroscopeActionRunner(dry_run=True)
        self.output_dir = Path(output_dir)
        self.history = []

    def sample_dfrequency(self, count=20, delay_s=0.15):
        values = []
        for _ in range(int(count)):
            try:
                values.append(float(self.runner.dFreq()))
            except Exception:
                values.append(float("nan"))
            time.sleep(delay_s)
        arr = np.asarray(values, dtype=float)
        finite = arr[np.isfinite(arr)]
        stats = {"count": int(len(finite))}
        if len(finite):
            stats.update(
                {
                    "mean": float(np.nanmean(arr)),
                    "std": float(np.nanstd(arr)),
                    "min": float(np.nanmin(arr)),
                    "max": float(np.nanmax(arr)),
                    "median": float(np.nanmedian(arr)),
                }
            )
            interpretation = self.runner.interpret_dFreq(stats["median"])
        else:
            interpretation = self.runner.interpret_dFreq(None)
        return {
            "values_Hz": values,
            "stats": stats,
            "interpretation": interpretation,
        }

    def collect_fresh_scan_lines(
        self,
        min_lines=40,
        timeout_s=120.0,
        start_scan=True,
        channel=0,
        chunk_lines=80,
        output_prefix="activity_fresh_lines",
    ):
        gxsm = self.runner.connect()
        result = {"start_scan": bool(start_scan)}
        if start_scan:
            result["startscan_return"] = gxsm.startscan()
            time.sleep(5.0)

        progress = []
        t0 = time.time()
        while time.time() - t0 < timeout_s:
            y_current = int(float(gxsm.y_current()))
            progress.append(
                {
                    "t_s": time.time() - t0,
                    "y_current": y_current,
                }
            )
            if y_current + 1 >= min_lines:
                break
            time.sleep(5.0)

        y_current = int(float(gxsm.y_current()))
        settle_before_fetch = self.runner.settle_before_pyshm_image_fetch(
            reason="activity fresh-lines comparison reached enough lines; settle before get_slice",
            wait_s=1.0,
        )
        image, meta = self.runner.fetch_scan_image_to_line(
            channel=channel,
            y_current=y_current,
            chunk_lines=chunk_lines,
            chunk_delay_s=0.5,
        )
        pixel_size_A = self.runner._pixel_size_A(channel)
        quality = self.runner.assess_tip_quality(
            image,
            pixel_size_A=pixel_size_A,
            output_prefix=output_prefix,
        )
        edge = self.evaluate_step_edges(image, pixel_size_A=pixel_size_A)
        plot = self.runner.plot_scan_image(
            image,
            self.output_dir / "{}.png".format(output_prefix),
            title="Fresh scan lines, line 0 at top",
        )
        result.update(
            {
                "progress": progress,
                "settle_before_fetch": settle_before_fetch.__dict__,
                "fetch_meta": meta,
                "pixel_size_A": pixel_size_A,
                "tip_quality": quality,
                "step_edges": edge,
                "plot": plot,
            }
        )
        return result

    def evaluate_step_edges(
        self,
        image,
        pixel_size_A=1.0,
        max_samples=18,
        edge_percentile=99.0,
        min_edge_height_A=0.25,
    ):
        """Estimate 10-90 step-edge widths in Angstrom."""
        arr = np.asarray(image, dtype=float)
        ny, nx = arr.shape
        if ny < 8 or nx < 8:
            return {"count": 0, "message": "image too small"}

        y_grid, x_grid = np.mgrid[:ny, :nx]
        valid = np.isfinite(arr)
        design = np.column_stack(
            [
                x_grid[valid].ravel(),
                y_grid[valid].ravel(),
                np.ones(valid.sum()),
            ]
        )
        coeffs, *_ = np.linalg.lstsq(design, arr[valid].ravel(), rcond=None)
        flat = arr - (coeffs[0] * x_grid + coeffs[1] * y_grid + coeffs[2])
        flat = flat - np.nanmedian(flat, axis=1, keepdims=True)
        gy, gx = np.gradient(flat)
        gmag = np.hypot(gx, gy)

        mask = np.isfinite(gmag)
        margin = min(16, max(2, ny // 10))
        mask[:margin, :] = False
        mask[-margin:, :] = False
        mask[:, :margin] = False
        mask[:, -margin:] = False
        if not np.any(mask):
            return {"count": 0}

        threshold = np.nanpercentile(gmag[mask], edge_percentile)
        candidates = np.argwhere(mask & (gmag >= threshold))
        if candidates.size == 0:
            return {"count": 0}

        scores = gmag[candidates[:, 0], candidates[:, 1]]
        order = np.argsort(scores)[::-1]
        samples = []
        half_width_px = 20
        for idx in order:
            py, px = candidates[idx]
            grad = np.array([gy[py, px], gx[py, px]], dtype=float)
            norm = float(np.hypot(grad[0], grad[1]))
            if norm <= 0 or not np.isfinite(norm):
                continue
            normal = grad / norm
            coords = []
            vals = []
            for t in range(-half_width_px, half_width_px + 1):
                yy = int(round(py + t * normal[0]))
                xx = int(round(px + t * normal[1]))
                if 0 <= yy < ny and 0 <= xx < nx:
                    coords.append(t)
                    vals.append(flat[yy, xx])
            if len(vals) < 16:
                continue

            coords = np.asarray(coords, dtype=float)
            smooth = np.convolve(np.asarray(vals, dtype=float), np.ones(5) / 5.0, mode="same")
            q = max(3, len(smooth) // 4)
            left = float(np.nanmedian(smooth[:q]))
            right = float(np.nanmedian(smooth[-q:]))
            amp = right - left
            if abs(amp) < min_edge_height_A:
                continue

            signal = smooth if amp > 0 else -smooth
            base = left if amp > 0 else -left
            top = right if amp > 0 else -right
            lo = base + 0.1 * (top - base)
            hi = base + 0.9 * (top - base)
            above_lo = np.where(signal >= lo)[0]
            above_hi = np.where(signal >= hi)[0]
            if len(above_lo) == 0 or len(above_hi) == 0:
                continue
            width_px = abs(float(coords[above_hi[0]] - coords[above_lo[0]]))
            if width_px <= 0 or width_px > 40:
                continue
            if any(np.hypot(px - s["px"], py - s["py"]) < 30 for s in samples):
                continue

            samples.append(
                {
                    "px": int(px),
                    "py": int(py),
                    "width_10_90_A": float(width_px * pixel_size_A),
                    "edge_height_A": float(abs(amp)),
                    "gradient_A_per_px": float(norm),
                }
            )
            if len(samples) >= max_samples:
                break

        widths = np.asarray([s["width_10_90_A"] for s in samples], dtype=float)
        if len(widths) == 0:
            return {"count": 0, "samples": samples}
        return {
            "count": int(len(widths)),
            "median_width_10_90_A": float(np.nanmedian(widths)),
            "p25_width_10_90_A": float(np.nanpercentile(widths, 25)),
            "p75_width_10_90_A": float(np.nanpercentile(widths, 75)),
            "min_width_10_90_A": float(np.nanmin(widths)),
            "max_width_10_90_A": float(np.nanmax(widths)),
            "samples": samples,
        }

    def fetch_latest_probe_event_summary(
        self,
        output_prefix="activity_latest_probe_event",
        channel=0,
    ):
        gxsm = self.runner.connect()
        columns, labels, units, xyij = gxsm.get_probe_event(channel, -1)
        columns = np.asarray(columns, dtype=float)
        labels = [str(label).strip() for label in labels]
        units = [str(unit).strip() for unit in units]
        np.savez(
            self.output_dir / "{}_full.npz".format(output_prefix),
            columns=columns,
            labels=np.asarray(labels),
            units=np.asarray(units),
            xyij=np.asarray(xyij),
        )

        vpdata = dict(zip(labels, columns))
        vpunits = dict(zip(labels, units))
        summary = {
            "shape": list(columns.shape),
            "labels": labels,
            "units": vpunits,
            "xyij": np.asarray(xyij).tolist(),
            "signals": {
                label: self.runner._array_stats(values)
                for label, values in vpdata.items()
            },
        }
        if "ZS-Topo" in vpdata:
            z = np.asarray(vpdata["ZS-Topo"], dtype=float)
            n = len(z)
            k = max(1, min(n, max(10, n // 10)))
            pre = z[:k]
            post = z[-k:]
            summary["zstopo"] = {
                "pre_mean": float(np.nanmean(pre)),
                "post_mean": float(np.nanmean(post)),
                "post_minus_pre_mean_A": float(np.nanmean(post) - np.nanmean(pre)),
                "range_A": float(np.nanmax(z) - np.nanmin(z)),
                "min_A": float(np.nanmin(z)),
                "max_A": float(np.nanmax(z)),
            }
        return summary

    def stop_and_run_bias_pulse_activity(
        self,
        pulse_du_V=-3.0,
        dfreq_samples=20,
        wait_after_execute_s=18.0,
        output_prefix="tip_activity_bias_pulse",
    ):
        """
        Stop scan, load a bias pulse with explicit FB flags, execute it, and
        fetch dFrequency/probe-event evidence before and after.
        """
        gxsm = self.runner.connect()
        started = time.time()
        report = {
            "timestamp": datetime.fromtimestamp(started).isoformat(),
            "activity": "stop_and_run_bias_pulse",
            "pulse_du_V": float(pulse_du_V),
            "dfreq_before": self.sample_dfrequency(dfreq_samples),
        }

        report["stopscan_return"] = gxsm.stopscan()
        time.sleep(3.0)
        load_record = self.runner.load_gvp_bias_pulse(
            pulse_du_V,
            save_prefix=output_prefix,
        )
        report["load"] = {
            "status": load_record.status,
            "result": self.runner.data.get("last_loaded_gvp"),
        }
        report["execute_return"] = gxsm.action("DSP_VP_VP_EXECUTE")
        time.sleep(wait_after_execute_s)
        try:
            report["probe_event"] = self.fetch_latest_probe_event_summary(
                output_prefix="{}_probe_event".format(output_prefix),
            )
        except Exception as exc:
            report["probe_event"] = {
                "error": "{}: {}".format(type(exc).__name__, exc)
            }
        report["dfreq_after"] = self.sample_dfrequency(dfreq_samples)
        try:
            report["rtquery_s_after"] = gxsm.rtquery("s")
        except Exception as exc:
            report["rtquery_s_after_error"] = "{}: {}".format(
                type(exc).__name__,
                exc,
            )
        report["elapsed_s"] = time.time() - started
        self.write_report(report, "{}_report.json".format(output_prefix))
        self.history.append(report)
        return report

    def write_report(self, report, filename):
        path = self.output_dir / filename
        path.write_text(json.dumps(self.runner._jsonable(report), indent=2))
        return str(path)


class LandscapeNavigationController:
    """
    Navigate the sample landscape and find flat areas suitable for work.

    This controller separates two coordinate layers:
    - local work-site selection inside the current scan frame via ScanX/ScanY,
    - landscape exploration by changing OffsetX/OffsetY between scans.

    Important coordinate convention:
    ScanX/ScanY does not navigate the wider landscape.  It moves the tip only
    within the current scan area's coordinate system.  OffsetX/OffsetY moves
    the scan frame to a different landscape/coarse area.  OffsetX/OffsetY are
    non-rotated world-coordinate scan-center positions; at the current setup
    roughly +/-2500 A is reachable.  The scan view is centered on OffsetX/Y and
    may then be rotated around that center by the Rotation parameter.

    By default it only analyzes and proposes moves.  Live movement methods are
    explicit and go through the underlying MicroscopeActionRunner.
    """

    CURRENT_OFFSET_REACHABLE_A = 2500.0

    def __init__(self, runner=None, output_dir="."):
        self.runner = runner or MicroscopeActionRunner(dry_run=True)
        self.output_dir = Path(output_dir)
        self.visited_offsets = []
        self.rejected_regions = []
        self.work_area_history = []

    def analyze_current_scan_for_work_area(
        self,
        channel=0,
        y_current=None,
        patch_A=80.0,
        step_A=20.0,
        max_candidates=12,
        output_prefix="landscape_work_area",
        avoid_bumps=True,
    ):
        """Fetch current scan data and rank flat work-area candidates."""
        started = time.time()
        settings = self.runner._scan_settings(channel)
        self.runner.remember_scan_site(settings)
        image, meta = self.runner.fetch_scan_image_to_line(
            channel=channel,
            y_current=y_current,
            chunk_lines=80,
        )
        pixel_size_A = self.runner._pixel_size_A(channel)
        bumps = self.runner.detect_major_bumps(
            image,
            max_bumps=30,
            min_sep_px=max(12, int(round(20.0 / max(pixel_size_A, 1e-9)))),
            pixel_size_A=pixel_size_A,
        )
        self.runner.remember_bumps(bumps, settings)
        candidates = self.rank_flat_work_areas(
            image,
            pixel_size_A=pixel_size_A,
            patch_A=patch_A,
            step_A=step_A,
            max_candidates=max_candidates,
            avoid_bumps=avoid_bumps,
        )
        for candidate in candidates:
            world = self.pixel_to_world_coordinates(
                candidate.px,
                candidate.py,
                settings=settings,
                channel=channel,
            )
            candidate.world_x_A = world["world_x_A"]
            candidate.world_y_A = world["world_y_A"]
        quality = self.runner.assess_tip_quality(
            image,
            pixel_size_A=pixel_size_A,
            output_prefix=output_prefix,
        )
        plot = self.plot_landscape_candidates(
            image,
            candidates,
            bumps=bumps,
            output_path=self.output_dir / "{}.png".format(output_prefix),
            title="Landscape work-area candidates, line 0 at top",
        )
        result = {
            "timestamp": datetime.fromtimestamp(started).isoformat(),
            "settings": settings,
            "fetch_meta": meta,
            "pixel_size_A": pixel_size_A,
            "tip_quality": quality,
            "major_bumps": bumps,
            "candidates": [candidate.__dict__ for candidate in candidates],
            "best_candidate": candidates[0].__dict__ if candidates else None,
            "plot": plot,
        }
        self.work_area_history.append(result)
        self.write_report(result, "{}_report.json".format(output_prefix))
        return result

    def analyze_current_scan_landscape_map(
        self,
        channel=0,
        y_current=None,
        image=None,
        meta=None,
        settings=None,
        patch_A=80.0,
        step_A=20.0,
        max_candidates=12,
        max_hazards=30,
        max_stepped_regions=20,
        max_abnormal_lines=20,
        output_prefix="current_landscape_world_map",
        memory_file="landscape_memory.json",
        avoid_bumps=True,
    ):
        """
        Analyze the current scan as a world-coordinate landscape map.

        This is the fuller exploration-memory pass:
        - fetch the current scan image,
        - record scan size, rotation, offset center, and world footprint,
        - rank flat work-area candidates,
        - mark large bumps/pits as hazards,
        - identify major stepped/multi-step regions,
        - identify abnormal scan-line contrast from transient tip changes,
        - convert all marked locations into OffsetX/Y world coordinates,
        - append the result to a persistent landscape memory JSON file.

        The operation is read-only with respect to the microscope.  It does not
        move ScanX/ScanY or OffsetX/OffsetY.
        """
        started = time.time()
        if settings is None:
            settings = self.runner._scan_settings(channel)
        self.runner.remember_scan_site(settings)
        if image is None:
            image, meta = self.runner.fetch_scan_image_to_line(
                channel=channel,
                y_current=y_current,
                chunk_lines=80,
            )
        else:
            image = np.asarray(image, dtype=float)
            meta = dict(meta or {})
            meta.setdefault("provided_image", True)
            meta.setdefault("shape", list(image.shape))
        try:
            pixel_size_A = float(settings.get("RangeX")) / max(
                float(settings.get("PointsX")) - 1.0,
                1.0,
            )
        except Exception:
            pixel_size_A = self.runner._pixel_size_A(channel)
        footprint = self.scan_frame_world_footprint(settings=settings)

        hazards = self.runner.detect_major_bumps(
            image,
            max_bumps=max_hazards,
            min_sep_px=max(12, int(round(20.0 / max(pixel_size_A, 1e-9)))),
            pixel_size_A=pixel_size_A,
        )
        for hazard in hazards:
            hazard.update(
                self.pixel_to_world_coordinates(
                    hazard["px"],
                    hazard["py"],
                    settings=settings,
                    channel=channel,
                )
            )
        self.runner.remember_bumps(hazards, settings)

        candidates = self.rank_flat_work_areas(
            image,
            pixel_size_A=pixel_size_A,
            patch_A=patch_A,
            step_A=step_A,
            max_candidates=max_candidates,
            avoid_bumps=avoid_bumps,
        )
        for candidate in candidates:
            world = self.pixel_to_world_coordinates(
                candidate.px,
                candidate.py,
                settings=settings,
                channel=channel,
            )
            candidate.world_x_A = world["world_x_A"]
            candidate.world_y_A = world["world_y_A"]

        stepped_regions = self.detect_major_stepped_regions(
            image,
            pixel_size_A=pixel_size_A,
            max_regions=max_stepped_regions,
        )
        for region in stepped_regions:
            center = self.pixel_to_world_coordinates(
                region["center_px"],
                region["center_py"],
                settings=settings,
                channel=channel,
            )
            corners = [
                self.pixel_to_world_coordinates(px, py, settings=settings, channel=channel)
                for px, py in region["bbox_px_corners"]
            ]
            region["center_world_x_A"] = center["world_x_A"]
            region["center_world_y_A"] = center["world_y_A"]
            region["bbox_world_corners"] = [
                {
                    "world_x_A": corner["world_x_A"],
                    "world_y_A": corner["world_y_A"],
                }
                for corner in corners
            ]

        abnormal_lines = self.detect_abnormal_contrast_lines(
            image,
            settings=settings,
            channel=channel,
            max_lines=max_abnormal_lines,
        )

        quality = self.runner.assess_tip_quality(
            image,
            pixel_size_A=pixel_size_A,
            output_prefix=output_prefix,
        )
        plot = self.plot_landscape_world_map(
            image,
            candidates=candidates,
            hazards=hazards,
            stepped_regions=stepped_regions,
            abnormal_lines=abnormal_lines,
            output_path=self.output_dir / "{}.png".format(output_prefix),
            title="Landscape world map, line 0 at top",
        )
        np.save(self.output_dir / "{}_scan_image.npy".format(output_prefix), image)

        result = {
            "timestamp": datetime.fromtimestamp(started).isoformat(),
            "analysis": "current_scan_landscape_world_map",
            "settings": settings,
            "scan_frame": footprint,
            "fetch_meta": meta,
            "pixel_size_A": pixel_size_A,
            "tip_quality": quality,
            "flat_candidates": [candidate.__dict__ for candidate in candidates],
            "best_flat_candidate": candidates[0].__dict__ if candidates else None,
            "hazards": hazards,
            "stepped_regions": stepped_regions,
            "abnormal_contrast_lines": abnormal_lines,
            "plot": plot,
            "notes": [
                "OffsetX/Y are non-rotated world coordinates for the scan center.",
                "ScanX/Y are local coordinates inside the current rotated scan frame.",
                "Hazards include large positive bumps and large negative pits.",
                (
                    "Abnormal contrast lines are treated as transient tip-state "
                    "artifacts, for example temporary tip change or adsorbate/"
                    "picked-up molecule, not physical landscape steps."
                ),
            ],
        }
        self.work_area_history.append(result)
        self.append_landscape_memory(result, memory_file=memory_file)
        self.write_report(result, "{}_report.json".format(output_prefix))
        self.runner.data["last_landscape_world_map"] = result
        return result

    def pixel_to_world_coordinates(self, px, py, settings=None, channel=0):
        """
        Convert image pixel coordinates to world OffsetX/Y coordinates.

        OffsetX/OffsetY are non-rotated world coordinates for the scan center.
        Pixel coordinates are first converted to right-handed local scan
        coordinates, then rotated by the scan Rotation around that center. The
        top/first scan line is local ScanY = +RangeY/2, the bottom line is
        ScanY = -RangeY/2, and the left image edge is ScanX = -RangeX/2.
        """
        if settings is None:
            settings = self.runner._scan_settings(channel)
        rec = ScanGeometry.from_settings(settings).pixel_record(px, py)
        rec["coordinate_note"] = (
            "OffsetX/Y are non-rotated world coordinates; local scan "
            "coordinates are rotated around the scan center by Rotation. "
            "Line 0/top is ScanY=+RangeY/2; line numbers increase "
            "downward while physical local Y is positive upward."
        )
        return rec

    def scan_frame_world_footprint(self, settings=None, channel=0):
        """Return scan-frame geometry and its world-coordinate corners."""
        if settings is None:
            settings = self.runner._scan_settings(channel)
        footprint = ScanGeometry.from_settings(settings).footprint()
        return {
            **footprint,
            "pixel_origin": "top-left, line 0 at image top",
            "world_corners": footprint.get("corners", {}),
        }

    def detect_major_stepped_regions(
        self,
        image,
        pixel_size_A=1.0,
        max_regions=20,
        tile_A=100.0,
        edge_percentile=96.0,
        min_span_A=1.0,
    ):
        """
        Detect tile-scale regions dominated by strong step-edge structure.

        The result is intended for landscape memory, not atomic metrology:
        regions with high edge fraction and topographic span are places to
        remember as major stepped/multiple-stepped terrain.
        """
        arr = np.asarray(image, dtype=float)
        ny, nx = arr.shape
        if ny < 8 or nx < 8:
            return []

        flat = self._leveled_line_flattened(arr)
        gy, gx = np.gradient(flat)
        gmag = np.hypot(gx, gy)
        finite = np.isfinite(gmag)
        if not np.any(finite):
            return []
        threshold = float(np.nanpercentile(gmag[finite], edge_percentile))
        tile_px = max(24, int(round(tile_A / max(pixel_size_A, 1e-9))))
        tile_px = min(tile_px, max(24, nx // 3), max(24, ny // 3))
        stride_px = max(12, tile_px // 2)

        regions = []
        for y0 in range(0, max(1, ny - tile_px + 1), stride_px):
            for x0 in range(0, max(1, nx - tile_px + 1), stride_px):
                y1 = min(ny, y0 + tile_px)
                x1 = min(nx, x0 + tile_px)
                patch = flat[y0:y1, x0:x1]
                gpatch = gmag[y0:y1, x0:x1]
                if patch.size == 0 or not np.isfinite(patch).any():
                    continue
                span = float(np.nanpercentile(patch, 98) - np.nanpercentile(patch, 2))
                edge_fraction = float(np.nanmean(gpatch >= threshold))
                if span < min_span_A or edge_fraction <= 0.0:
                    continue
                score = edge_fraction * max(span, 0.0)
                regions.append(
                    {
                        "x0_px": int(x0),
                        "y0_px": int(y0),
                        "x1_px": int(x1 - 1),
                        "y1_px": int(y1 - 1),
                        "center_px": int(round((x0 + x1 - 1) / 2.0)),
                        "center_py": int(round((y0 + y1 - 1) / 2.0)),
                        "bbox_px_corners": [
                            [int(x0), int(y0)],
                            [int(x1 - 1), int(y0)],
                            [int(x1 - 1), int(y1 - 1)],
                            [int(x0), int(y1 - 1)],
                        ],
                        "edge_fraction": edge_fraction,
                        "span_2_98_A": span,
                        "score": float(score),
                        "note": "major stepped or multiple-stepped terrain",
                    }
                )

        regions.sort(key=lambda item: item["score"], reverse=True)
        selected = []
        for region in regions:
            if any(
                self._boxes_overlap_fraction(region, other) > 0.35
                for other in selected
            ):
                continue
            selected.append(region)
            if len(selected) >= max_regions:
                break
        return selected

    def detect_abnormal_contrast_lines(
        self,
        image,
        settings=None,
        channel=0,
        max_lines=20,
        line_sigma_threshold=4.0,
        diff_sigma_threshold=5.0,
        min_width_px=1,
    ):
        """
        Detect abnormal scan-line contrast from transient tip state changes.

        These features are different from real steps or bumps. They often appear
        as a few scan lines with abnormal contrast or offset after a temporary
        tip change, adsorbate pickup, or molecule pickup.  The output is stored
        separately so landscape memory can ignore them as stable terrain while
        still remembering that local image evidence is suspect.
        """
        arr = np.asarray(image, dtype=float)
        ny, nx = arr.shape
        if ny < 4 or nx < 8:
            return []
        if settings is None:
            settings = self.runner._scan_settings(channel)

        plane = self._plane_leveled(arr)
        line_flat = plane - np.nanmedian(plane, axis=1, keepdims=True)
        line_median = np.nanmedian(plane, axis=1)
        line_contrast = np.nanpercentile(line_flat, 95, axis=1) - np.nanpercentile(
            line_flat,
            5,
            axis=1,
        )
        baseline_median = self._running_median_1d(line_median, window=31)
        baseline_contrast = self._running_median_1d(line_contrast, window=31)
        median_residual = line_median - baseline_median
        contrast_residual = line_contrast - baseline_contrast
        median_sigma = self._robust_sigma(median_residual)
        contrast_sigma = self._robust_sigma(contrast_residual)
        line_diff = np.diff(line_median, prepend=line_median[0])
        diff_sigma = self._robust_sigma(line_diff)

        score = np.zeros(ny, dtype=float)
        if median_sigma > 0:
            score = np.maximum(score, np.abs(median_residual) / median_sigma)
        if contrast_sigma > 0:
            score = np.maximum(score, np.abs(contrast_residual) / contrast_sigma)
        diff_score = np.zeros(ny, dtype=float)
        if diff_sigma > 0:
            diff_score = np.abs(line_diff) / diff_sigma
            score = np.maximum(score, diff_score)

        mask = (score >= line_sigma_threshold) | (diff_score >= diff_sigma_threshold)
        groups = self._contiguous_true_groups(mask)
        reports = []
        for y0, y1 in groups:
            if y1 - y0 + 1 < min_width_px:
                continue
            center_py = int(round((y0 + y1) / 2.0))
            left = self.pixel_to_world_coordinates(
                0,
                center_py,
                settings=settings,
                channel=channel,
            )
            right = self.pixel_to_world_coordinates(
                nx - 1,
                center_py,
                settings=settings,
                channel=channel,
            )
            center = self.pixel_to_world_coordinates(
                (nx - 1) / 2.0,
                center_py,
                settings=settings,
                channel=channel,
            )
            world_x_values = [left["world_x_A"], right["world_x_A"], center["world_x_A"]]
            world_y_values = [left["world_y_A"], right["world_y_A"], center["world_y_A"]]
            reports.append(
                {
                    "y0_px": int(y0),
                    "y1_px": int(y1),
                    "center_py": center_py,
                    "line_count": int(y1 - y0 + 1),
                    "score": float(np.nanmax(score[y0:y1 + 1])),
                    "median_residual_A": float(np.nanmedian(median_residual[y0:y1 + 1])),
                    "contrast_residual_A": float(
                        np.nanmedian(contrast_residual[y0:y1 + 1])
                    ),
                    "world_x_min_A": float(min(world_x_values)),
                    "world_x_max_A": float(max(world_x_values)),
                    "world_y_min_A": float(min(world_y_values)),
                    "world_y_max_A": float(max(world_y_values)),
                    "center_world_x_A": center["world_x_A"],
                    "center_world_y_A": center["world_y_A"],
                    "line_world_endpoints": [
                        {
                            "name": "left_pixel",
                            "world_x_A": left["world_x_A"],
                            "world_y_A": left["world_y_A"],
                        },
                        {
                            "name": "right_pixel",
                            "world_x_A": right["world_x_A"],
                            "world_y_A": right["world_y_A"],
                        },
                    ],
                    "classification": "transient_tip_state_abnormal_contrast",
                    "interpretation": (
                        "Likely temporary tip change or adsorbate/picked-up "
                        "molecule during scanning; do not treat as stable "
                        "physical terrain."
                    ),
                }
            )

        reports.sort(key=lambda item: item["score"], reverse=True)
        return reports[:max_lines]

    def detect_prominent_blob_region(
        self,
        image,
        settings=None,
        channel=0,
        search_top_fraction=0.5,
        threshold_percentile=99.0,
        min_pixels=12,
    ):
        """
        Locate a large blob by region centroid rather than single-pixel maximum.

        This is useful for GVP pulse marks, where the highest pixel may be on a
        shoulder or edge while the operator-visible blob center is closer to the
        centroid of the high-contrast region.  World coordinates use the scan
        convention: line 0/top is ScanY=+RangeY/2 and left is ScanX=-RangeX/2.
        """
        arr = np.asarray(image, dtype=float)
        ny, nx = arr.shape
        if settings is None:
            settings = self.runner._scan_settings(channel)
        flat = self._leveled_line_flattened(arr)
        y1 = max(1, min(ny, int(round(ny * float(search_top_fraction)))))
        search = flat[:y1, :]
        threshold = float(np.nanpercentile(search, threshold_percentile))
        mask = np.isfinite(search) & (search >= threshold)
        if int(mask.sum()) < min_pixels:
            py, px = np.unravel_index(np.nanargmax(search), search.shape)
            component = np.zeros_like(mask, dtype=bool)
            component[py, px] = True
        else:
            peak_py, peak_px = np.unravel_index(np.nanargmax(search), search.shape)
            component = self._connected_component(mask, int(peak_px), int(peak_py))
            if int(component.sum()) < min_pixels:
                component = mask
        ys, xs = np.nonzero(component)
        weights = search[ys, xs] - np.nanmin(search[component])
        if not np.isfinite(weights).all() or float(np.sum(weights)) <= 0.0:
            weights = np.ones_like(xs, dtype=float)
        centroid_px = float(np.average(xs, weights=weights))
        centroid_py = float(np.average(ys, weights=weights))
        peak_py, peak_px = np.unravel_index(np.nanargmax(search), search.shape)
        centroid_world = self.pixel_to_world_coordinates(
            centroid_px,
            centroid_py,
            settings=settings,
            channel=channel,
        )
        peak_world = self.pixel_to_world_coordinates(
            int(peak_px),
            int(peak_py),
            settings=settings,
            channel=channel,
        )
        return {
            "centroid_px": centroid_px,
            "centroid_py": centroid_py,
            "peak_px": int(peak_px),
            "peak_py": int(peak_py),
            "pixel_count": int(component.sum()),
            "threshold": threshold,
            "centroid_world": centroid_world,
            "peak_world": peak_world,
            "height_A_line_flat_peak": float(search[peak_py, peak_px]),
            "note": (
                "Use centroid_world for large pulse blobs; peak_world can land "
                "on a shoulder/edge of the feature."
            ),
        }

    def rank_flat_work_areas(
        self,
        image,
        pixel_size_A=1.0,
        patch_A=80.0,
        step_A=20.0,
        max_candidates=12,
        avoid_bumps=True,
        avoid_top_fraction=0.08,
        avoid_edge_fraction=0.04,
    ):
        """
        Rank low-slope, low-roughness, blob-free patches as work areas.

        The score is unit-aware and intentionally conservative: rough patches,
        steep terraces, bright blobs, pits, and recently remembered bumps are
        penalized.
        """
        arr = np.asarray(image, dtype=float)
        ny, nx = arr.shape
        if ny < 8 or nx < 8:
            return []
        patch_px = max(12, int(round(patch_A / max(pixel_size_A, 1e-9))))
        patch_px = min(patch_px, max(12, nx // 3), max(12, ny // 2))
        if patch_px % 2:
            patch_px += 1
        step_px = max(4, int(round(step_A / max(pixel_size_A, 1e-9))))
        mx = patch_px // 2
        my = patch_px // 2
        if ny < 2 * my + 2 or nx < 2 * mx + 2:
            return []
        y_min = max(my + 2, int(round(float(avoid_top_fraction) * ny)))
        x_min = max(mx + 2, int(round(float(avoid_edge_fraction) * nx)))
        x_max = min(nx - mx - 1, nx - int(round(float(avoid_edge_fraction) * nx)))

        y_grid, x_grid = np.mgrid[:ny, :nx]
        valid = np.isfinite(arr)
        design = np.column_stack(
            [
                x_grid[valid].ravel(),
                y_grid[valid].ravel(),
                np.ones(valid.sum()),
            ]
        )
        coeffs, *_ = np.linalg.lstsq(design, arr[valid].ravel(), rcond=None)
        leveled = arr - (coeffs[0] * x_grid + coeffs[1] * y_grid + coeffs[2])
        line_flat = leveled - np.nanmedian(leveled, axis=1, keepdims=True)
        gy, gx = np.gradient(line_flat)
        global_abs = np.abs(line_flat - np.nanmedian(line_flat))
        bright_threshold = np.nanpercentile(global_abs, 97)

        candidates = []
        for cy in range(y_min, ny - my - 1, step_px):
            for cx in range(x_min, x_max, step_px):
                if avoid_bumps and self.runner._patch_overlaps_recent_bump(cx, cy, mx, my):
                    continue
                patch = line_flat[cy - my:cy + my, cx - mx:cx + mx]
                if patch.size == 0 or not np.isfinite(patch).any():
                    continue
                local_g = np.hypot(
                    gx[cy - my:cy + my, cx - mx:cx + mx],
                    gy[cy - my:cy + my, cx - mx:cx + mx],
                )
                local_std = float(np.nanstd(patch))
                span = float(np.nanpercentile(patch, 98) - np.nanpercentile(patch, 2))
                grad95 = float(np.nanpercentile(local_g, 95))
                blob_fraction = float(
                    np.nanmean(np.abs(patch - np.nanmedian(line_flat)) > bright_threshold)
                )
                py_grid, px_grid = np.mgrid[:patch.shape[0], :patch.shape[1]]
                local_valid = np.isfinite(patch)
                local_design = np.column_stack(
                    [
                        px_grid[local_valid].ravel(),
                        py_grid[local_valid].ravel(),
                        np.ones(local_valid.sum()),
                    ]
                )
                local_coeffs, *_ = np.linalg.lstsq(
                    local_design,
                    patch[local_valid].ravel(),
                    rcond=None,
                )
                slope = float(np.hypot(local_coeffs[0], local_coeffs[1]))
                nearest_bump = self._nearest_recent_bump_px(cx, cy)
                bump_penalty = 0.0
                if nearest_bump is not None:
                    bump_penalty = 1.0 / max(nearest_bump, 1.0)
                score = (
                    local_std
                    + 2.0 * grad95
                    + 0.5 * span
                    + 8.0 * blob_fraction
                    + 10.0 * slope
                    + 20.0 * bump_penalty
                )
                target = self.runner.pixel_to_scan_xy(cx, cy)
                candidates.append(
                    WorkAreaCandidate(
                        px=int(cx),
                        py=int(cy),
                        scan_x_A=target.get("ScanX_A"),
                        scan_y_A=target.get("ScanY_A"),
                        score=float(score),
                        patch_px=(int(patch_px), int(patch_px)),
                        local_std_A=local_std,
                        slope_A_per_px=slope,
                        grad95_A_per_px=grad95,
                        span_2_98_A=span,
                        blob_fraction=blob_fraction,
                        nearest_bump_px=nearest_bump,
                        note="flat low-blob work-area candidate",
                    )
                )

        candidates.sort(key=lambda item: item.score)
        return candidates[:max_candidates]

    def select_clean_patch_for_tip_action(
        self,
        image,
        pixel_size_A=1.0,
        min_lines=120,
        patch_A=70.0,
        step_A=20.0,
        max_candidates=12,
    ):
        """
        Select a clean patch suitable for a tip-improvement action.

        Tip actions must not choose from a thin partial scan or from the very
        top/edge of the image.  This protects against the failure mode where a
        restarted scan has only a few lines and the apparent best patch is not
        a meaningful clean area.
        """
        arr = np.asarray(image, dtype=float)
        if arr.shape[0] < int(min_lines):
            raise ValueError(
                "Need at least {} scan lines for tip-action patch selection; got {}.".format(
                    min_lines,
                    arr.shape[0],
                )
            )
        candidates = self.rank_flat_work_areas(
            arr,
            pixel_size_A=pixel_size_A,
            patch_A=patch_A,
            step_A=step_A,
            max_candidates=max_candidates,
            avoid_bumps=True,
            avoid_top_fraction=0.12,
            avoid_edge_fraction=0.08,
        )
        if not candidates:
            raise ValueError("No clean patch found for tip action.")
        return candidates[0], candidates

    def propose_offset_search_grid(
        self,
        step_A=None,
        rings=1,
        include_current=False,
        current_offset=None,
    ):
        """
        Propose OffsetX/OffsetY targets for finding a cleaner landscape area.

        The step defaults to 80% of the current RangeX so adjacent searches
        overlap enough to maintain context while avoiding known bumps.
        """
        if current_offset is None:
            settings = self.runner._scan_settings(0)
            current_offset = (
                float(settings.get("OffsetX", 0.0)),
                float(settings.get("OffsetY", 0.0)),
            )
            if step_A is None:
                step_A = 0.8 * float(settings.get("RangeX", 700.0))
        if step_A is None:
            step_A = 560.0
        cx, cy = current_offset
        targets = []
        for ring in range(0 if include_current else 1, int(rings) + 1):
            for ix in range(-ring, ring + 1):
                for iy in range(-ring, ring + 1):
                    if max(abs(ix), abs(iy)) != ring:
                        continue
                    target = {
                        "OffsetX": float(cx + ix * step_A),
                        "OffsetY": float(cy + iy * step_A),
                        "ring": ring,
                        "step_A": float(step_A),
                    }
                    target["reachable"] = self.offset_is_reachable(
                        target["OffsetX"],
                        target["OffsetY"],
                    )
                    if self._offset_was_visited(target["OffsetX"], target["OffsetY"], step_A * 0.25):
                        target["visited"] = True
                    targets.append(target)
        return targets

    def propose_overlapping_exploration_frame(
        self,
        memory_file="landscape_memory_latest.json",
        new_range_A=800.0,
        overlap_A=250.0,
        direction="image_left_below",
        avoid_region_keys=("hazards", "stepped_regions"),
    ):
        """
        Propose a partially overlapping OffsetX/Y frame for exploration.

        Directions named `image_*` are interpreted in the current scan display
        convention: line 0 at top, X pixel to the right, Y pixel downward.  The
        resulting local scan displacement is rotated into non-rotated world
        OffsetX/Y coordinates using the remembered `Rotation`.

        This is a planning helper only; it does not move the microscope.
        """
        memory = self._load_landscape_memory_entry(memory_file)
        frame = self._normalize_memory_scan_frame(memory.get("scan_frame", {}))
        old_range_x = float(frame["RangeX_A"])
        old_range_y = float(frame["RangeY_A"])
        old_offset_x = float(frame["OffsetX_A"])
        old_offset_y = float(frame["OffsetY_A"])
        rotation_deg = float(frame.get("Rotation_deg", 0.0))
        shift_x = max(0.0, (old_range_x + float(new_range_A)) / 2.0 - overlap_A)
        shift_y = max(0.0, (old_range_y + float(new_range_A)) / 2.0 - overlap_A)
        local_dx, local_dy = self._direction_to_local_shift(
            direction,
            shift_x,
            shift_y,
        )
        shift_geometry = ScanGeometry(
            range_x_A=old_range_x,
            range_y_A=old_range_y,
            points_x=2,
            points_y=2,
            offset_x_A=0.0,
            offset_y_A=0.0,
            rotation_deg=rotation_deg,
        )
        world_shift = shift_geometry.local_vector_to_world(local_dx, local_dy)
        world_dx = world_shift.x_A
        world_dy = world_shift.y_A
        target = {
            "OffsetX": float(old_offset_x + world_dx),
            "OffsetY": float(old_offset_y + world_dy),
            "RangeX": float(new_range_A),
            "RangeY": float(new_range_A),
            "Rotation": rotation_deg,
            "direction": direction,
            "requested_overlap_A": float(overlap_A),
            "estimated_local_shift_A": {
                "dx": float(local_dx),
                "dy": float(local_dy),
            },
            "estimated_world_shift_A": {
                "dx": float(world_dx),
                "dy": float(world_dy),
            },
            "reachable": self.offset_is_reachable(
                old_offset_x + world_dx,
                old_offset_y + world_dy,
            ),
        }
        target["footprint_reachable"] = self.scan_frame_fits_reachable_area(
            target["OffsetX"],
            target["OffsetY"],
            target["RangeX"],
            target["RangeY"],
            rotation_deg=rotation_deg,
        )
        target["reachable"] = bool(
            target["reachable"] and target["footprint_reachable"]["fits"]
        )
        target["avoidance_summary"] = self._score_exploration_target_against_memory(
            target,
            memory,
            new_range_A=float(new_range_A),
            avoid_region_keys=avoid_region_keys,
        )
        return target

    def setup_exploration_scan_frame(
        self,
        offset_x_A,
        offset_y_A,
        range_A=800.0,
        points=None,
        rotation_deg=None,
        wait_after_s=3.0,
    ):
        """
        Set OffsetX/Y and scan size for an exploration frame.

        This is hardware-changing on a live runner.  It does not start scanning.
        """
        started = time.time()
        footprint = self.scan_frame_fits_reachable_area(
            offset_x_A,
            offset_y_A,
            range_A,
            range_A,
            rotation_deg=0.0 if rotation_deg is None else rotation_deg,
        )
        if not footprint["fits"]:
            raise ValueError(
                "Scan frame at Offset target ({}, {}) with range {} A does not fit inside current +/-{} A reachable XY area: {}".format(
                    offset_x_A,
                    offset_y_A,
                    range_A,
                    self.CURRENT_OFFSET_REACHABLE_A,
                    footprint["reason"],
                )
            )
        if self.runner.dry_run:
            result = {
                "OffsetX": float(offset_x_A),
                "OffsetY": float(offset_y_A),
                "RangeX": float(range_A),
                "RangeY": float(range_A),
                "PointsX": points,
                "PointsY": points,
                "Rotation": rotation_deg,
            }
        else:
            gxsm = self.runner.connect()
            gxsm.set("OffsetX", offset_x_A)
            gxsm.set("OffsetY", offset_y_A)
            gxsm.set("RangeX", range_A)
            gxsm.set("RangeY", range_A)
            if points is not None:
                gxsm.set("PointsX", points)
                gxsm.set("PointsY", points)
            if rotation_deg is not None:
                gxsm.set("Rotation", rotation_deg)
            time.sleep(wait_after_s)
            result = {
                "OffsetX": gxsm.get("OffsetX"),
                "OffsetY": gxsm.get("OffsetY"),
                "RangeX": gxsm.get("RangeX"),
                "RangeY": gxsm.get("RangeY"),
                "PointsX": gxsm.get("PointsX"),
                "PointsY": gxsm.get("PointsY"),
                "Rotation": gxsm.get("Rotation"),
            }
        self.visited_offsets.append(
            {
                "timestamp": datetime.now().isoformat(),
                "OffsetX": result["OffsetX"],
                "OffsetY": result["OffsetY"],
                "RangeX": result["RangeX"],
                "RangeY": result["RangeY"],
            }
        )
        return self.runner._record(
            "setup_exploration_scan_frame",
            "landscape_navigation",
            "dry-run" if self.runner.dry_run else "ok",
            started,
            command="set OffsetX/OffsetY/RangeX/RangeY",
            result=result,
        )

    def monitor_scan_for_hazards(
        self,
        channel=0,
        poll_s=10.0,
        chunk_lines=80,
        sigma_threshold=7.0,
        min_abs_height_A=5.0,
        min_lines_before_check=20,
        stop_on_hazard=True,
        output_prefix="exploration_scan_monitor",
    ):
        """
        Monitor an active scan and stop immediately if a major bump/pit appears.

        Note: image-based hazard checking uses `gxsm.get_slice`. Keep these
        reads sparse and settled; GXSM can scan while image data is fetched, but
        PySHM requests should not be fired back-to-back.

        Returns after full scan completion, abort request, or hazard stop.
        """
        if self.runner.dry_run:
            return {
                "dry_run": True,
                "would_monitor_channel": channel,
                "would_stop_on_hazard": stop_on_hazard,
            }
        gxsm = self.runner.connect()
        dims = gxsm.get_dimensions(channel)
        ny = int(dims[1])
        progress = []
        last_checked = -1
        hazard_event = None
        started = time.time()
        while True:
            y_current = int(float(gxsm.y_current()))
            progress.append(
                {
                    "elapsed_s": time.time() - started,
                    "y_current": y_current,
                }
            )
            if y_current + 1 >= min_lines_before_check and y_current != last_checked:
                settle_before_fetch = self.runner.settle_before_pyshm_image_fetch(
                    reason="exploration hazard monitor reached enough lines; settle before get_slice",
                    wait_s=1.0,
                )
                image, meta = self.runner.fetch_scan_image_to_line(
                    channel=channel,
                    y_current=y_current,
                    chunk_lines=chunk_lines,
                    chunk_delay_s=0.5,
                )
                bumps = self.runner.detect_major_bumps(
                    image,
                    max_bumps=10,
                    sigma_threshold=sigma_threshold,
                    min_sep_px=20,
                    pixel_size_A=self.runner._pixel_size_A(channel),
                )
                bumps = [
                    bump for bump in bumps
                    if abs(float(bump.get("height_A", 0.0))) >= min_abs_height_A
                    and bump.get("count_for_offset_stop", True)
                ]
                if bumps:
                    settings = self.runner._scan_settings(channel)
                    for bump in bumps:
                        bump.update(
                            self.pixel_to_world_coordinates(
                                bump["px"],
                                bump["py"],
                                settings=settings,
                                channel=channel,
                            )
                        )
                    hazard_event = {
                        "elapsed_s": time.time() - started,
                        "y_current": y_current,
                        "fetch_meta": meta,
                        "settle_before_fetch": settle_before_fetch.__dict__,
                        "hazards": bumps,
                        "reason": (
                            "major bump/pit detected during exploration scan "
                            "above sigma and height thresholds"
                        ),
                        "sigma_threshold": sigma_threshold,
                        "min_abs_height_A": min_abs_height_A,
                    }
                    if stop_on_hazard:
                        hazard_event["stopscan_return"] = gxsm.stopscan()
                    break
                last_checked = y_current
            if y_current >= ny - 1:
                break
            if self.runner.abort_requested():
                break
            time.sleep(poll_s)

        result = {
            "timestamp": datetime.fromtimestamp(started).isoformat(),
            "dimensions": list(dims),
            "progress": progress,
            "hazard_event": hazard_event,
            "sigma_threshold": sigma_threshold,
            "min_abs_height_A": min_abs_height_A,
            "completed": hazard_event is None and progress[-1]["y_current"] >= ny - 1,
            "stopped_for_hazard": hazard_event is not None and stop_on_hazard,
        }
        self.write_report(result, "{}_report.json".format(output_prefix))
        return result

    def explore_overlapping_region(
        self,
        memory_file="landscape_memory_latest.json",
        new_range_A=800.0,
        overlap_A=250.0,
        direction="image_left_below",
        points=None,
        channel=0,
        poll_s=10.0,
        hazard_sigma_threshold=7.0,
        min_hazard_height_A=5.0,
        output_prefix="overlap_exploration",
        execute=True,
    ):
        """
        Plan and optionally run a complete overlapping exploration scan.

        Live sequence when execute=True:
        1. plan a partially overlapping OffsetX/Y frame,
        2. set OffsetX/Y and RangeX/Y,
        3. start scan,
        4. monitor for major bump/pit hazards and stop immediately if found,
        5. after completion or stop, run landscape analysis and replan targets.
        """
        started = time.time()
        plan = self.propose_overlapping_exploration_frame(
            memory_file=memory_file,
            new_range_A=new_range_A,
            overlap_A=overlap_A,
            direction=direction,
        )
        result = {
            "timestamp": datetime.fromtimestamp(started).isoformat(),
            "plan": plan,
            "execute": bool(execute),
        }
        if not execute or self.runner.dry_run:
            result["status"] = "planned"
            self.write_report(result, "{}_plan.json".format(output_prefix))
            return result

        if not plan["reachable"]:
            result["status"] = "blocked_unreachable"
            self.write_report(result, "{}_report.json".format(output_prefix))
            return result

        setup_record = self.setup_exploration_scan_frame(
            plan["OffsetX"],
            plan["OffsetY"],
            range_A=new_range_A,
            points=points,
            rotation_deg=plan.get("Rotation"),
        )
        result["setup_record"] = setup_record.__dict__
        gxsm = self.runner.connect()
        result["startscan_return"] = gxsm.startscan()
        time.sleep(3.0)
        monitor = self.monitor_scan_for_hazards(
            channel=channel,
            poll_s=poll_s,
            sigma_threshold=hazard_sigma_threshold,
            min_abs_height_A=min_hazard_height_A,
            output_prefix="{}_monitor".format(output_prefix),
        )
        result["monitor"] = monitor
        result["post_scan_analysis"] = self.analyze_current_scan_landscape_map(
            channel=channel,
            output_prefix="{}_landscape".format(output_prefix),
        )
        result["next_targets"] = self.propose_offset_search_grid(
            step_A=0.8 * float(new_range_A),
            rings=1,
            include_current=False,
            current_offset=(plan["OffsetX"], plan["OffsetY"]),
        )
        result["status"] = (
            "stopped_for_hazard"
            if monitor.get("stopped_for_hazard")
            else "completed_and_analyzed"
        )
        result["elapsed_s"] = time.time() - started
        self.write_report(result, "{}_report.json".format(output_prefix))
        return result

    def move_to_offset(self, offset_x_A, offset_y_A, wait_after_s=3.0):
        """
        Move the scan frame center in non-rotated world coordinates.

        OffsetX/OffsetY relocate the scan area within the current world
        coordinate system.  The scan view is centered at this position and may
        be rotated around that center by the Rotation parameter.
        """
        started = time.time()
        if not self.offset_is_reachable(offset_x_A, offset_y_A):
            raise ValueError(
                "Offset target ({}, {}) exceeds current +/-{} A reachable range.".format(
                    offset_x_A,
                    offset_y_A,
                    self.CURRENT_OFFSET_REACHABLE_A,
                )
            )
        if not self.runner.dry_run:
            gxsm = self.runner.connect()
            gxsm.set("OffsetX", offset_x_A)
            gxsm.set("OffsetY", offset_y_A)
            time.sleep(wait_after_s)
            result = {
                "OffsetX": gxsm.get("OffsetX"),
                "OffsetY": gxsm.get("OffsetY"),
            }
        else:
            result = {"OffsetX": offset_x_A, "OffsetY": offset_y_A}
        self.visited_offsets.append(
            {
                "timestamp": datetime.now().isoformat(),
                "OffsetX": result["OffsetX"],
                "OffsetY": result["OffsetY"],
            }
        )
        return self.runner._record(
            "move_to_offset",
            "landscape_navigation",
            "dry-run" if self.runner.dry_run else "ok",
            started,
            command="set OffsetX/OffsetY",
            result=result,
        )

    def offset_is_reachable(self, offset_x_A, offset_y_A):
        """Return whether OffsetX/Y is inside the current reachable world range."""
        return (
            abs(float(offset_x_A)) <= self.CURRENT_OFFSET_REACHABLE_A
            and abs(float(offset_y_A)) <= self.CURRENT_OFFSET_REACHABLE_A
        )

    def scan_frame_fits_reachable_area(
        self,
        offset_x_A,
        offset_y_A,
        range_x_A,
        range_y_A=None,
        rotation_deg=0.0,
        limit_A=None,
    ):
        """
        Check whether the full scan footprint fits into reachable OffsetX/Y area.

        The simple center check is not enough for exploration planning: GXSM may
        accept a center OffsetX/Y while the requested scan range would extend
        outside the current XY reachable area.  This checks the rotated world
        coordinates of all four scan-frame corners against +/-limit_A.
        """
        limit = float(limit_A or self.CURRENT_OFFSET_REACHABLE_A)
        ox = float(offset_x_A)
        oy = float(offset_y_A)
        rx = float(range_x_A)
        ry = float(range_y_A if range_y_A is not None else range_x_A)
        corners = []
        geometry = ScanGeometry(
            range_x_A=rx,
            range_y_A=ry,
            points_x=2,
            points_y=2,
            offset_x_A=ox,
            offset_y_A=oy,
            rotation_deg=float(rotation_deg or 0.0),
        )
        for local_x in (-0.5 * rx, 0.5 * rx):
            for local_y in (-0.5 * ry, 0.5 * ry):
                world = geometry.local_to_world(local_x, local_y)
                corners.append({"world_x_A": float(world.x_A), "world_y_A": float(world.y_A)})
        x_values = [corner["world_x_A"] for corner in corners]
        y_values = [corner["world_y_A"] for corner in corners]
        max_abs_x = max(abs(value) for value in x_values)
        max_abs_y = max(abs(value) for value in y_values)
        fits = max_abs_x <= limit and max_abs_y <= limit
        margin_x = limit - max_abs_x
        margin_y = limit - max_abs_y
        reason = (
            "full rotated footprint fits with margins X={:.3g} A, Y={:.3g} A"
            if fits else
            "full rotated footprint exceeds limit; margins X={:.3g} A, Y={:.3g} A"
        ).format(margin_x, margin_y)
        return {
            "fits": bool(fits),
            "limit_A": limit,
            "center_reachable": self.offset_is_reachable(ox, oy),
            "max_abs_corner_x_A": float(max_abs_x),
            "max_abs_corner_y_A": float(max_abs_y),
            "margin_x_A": float(margin_x),
            "margin_y_A": float(margin_y),
            "corners": corners,
            "reason": reason,
            "rule": "OffsetX/Y plus the full rotated scan half-range must fit within +/-limit_A.",
        }

    def move_tip_to_best_local_work_area(self, analysis=None, wait_after_s=2.0):
        """
        Move the tip locally via ScanX/ScanY to the best candidate.

        This stays inside the current scan area and coordinate system; it does
        not move the landscape frame.  Use move_to_offset() for OffsetX/Y
        landscape exploration.
        """
        if analysis is None:
            if not self.work_area_history:
                raise ValueError("No work-area analysis is available.")
            analysis = self.work_area_history[-1]
        candidate = analysis.get("best_candidate")
        if not candidate:
            raise ValueError("No best candidate found in work-area analysis.")
        return self.runner.move_to_scan_xy_fields(
            candidate["scan_x_A"],
            candidate["scan_y_A"],
            wait_after_s=wait_after_s,
        )

    def move_to_best_work_area(self, analysis=None, wait_after_s=2.0):
        """Backward-compatible alias for local ScanX/ScanY tip positioning."""
        return self.move_tip_to_best_local_work_area(
            analysis=analysis,
            wait_after_s=wait_after_s,
        )

    def _nearest_recent_bump_px(self, px, py):
        distances = []
        for bump in self.runner.bump_history[-100:]:
            bx = bump.get("px")
            by = bump.get("py")
            if bx is not None and by is not None:
                distances.append(float(np.hypot(px - bx, py - by)))
        return min(distances) if distances else None

    def _offset_was_visited(self, offset_x_A, offset_y_A, radius_A):
        for site in self.visited_offsets:
            try:
                dist = np.hypot(
                    float(site.get("OffsetX")) - offset_x_A,
                    float(site.get("OffsetY")) - offset_y_A,
                )
            except Exception:
                continue
            if dist <= radius_A:
                return True
        return False

    def _load_landscape_memory_entry(self, memory_file):
        path = self.output_dir / memory_file
        if not path.exists():
            path = Path(memory_file)
        if not path.exists():
            if self.work_area_history:
                return self.work_area_history[-1]
            raise FileNotFoundError("No landscape memory file: {}".format(memory_file))
        data = json.loads(path.read_text())
        if isinstance(data, list):
            if not data:
                raise ValueError("Landscape memory file is empty: {}".format(path))
            return data[-1]
        if isinstance(data, dict) and "entries" in data:
            entries = data["entries"]
            if not entries:
                raise ValueError("Landscape memory file has no entries: {}".format(path))
            return entries[-1]
        return data

    def _normalize_memory_scan_frame(self, frame):
        if "world_corners" in frame:
            return frame
        if "world_footprint_corners" in frame:
            corners = {}
            for corner in frame["world_footprint_corners"]:
                corners[corner["name"]] = {
                    "world_x_A": corner["world_x_A"],
                    "world_y_A": corner["world_y_A"],
                }
            normalized = dict(frame)
            normalized["world_corners"] = corners
            return normalized
        return frame

    def _direction_to_local_shift(self, direction, shift_x, shift_y):
        direction = str(direction).lower().replace("-", "_")
        mapping = {
            "image_left": (-shift_x, 0.0),
            "image_right": (shift_x, 0.0),
            "image_up": (0.0, -shift_y),
            "image_down": (0.0, shift_y),
            "image_left_below": (-shift_x, shift_y),
            "image_left_down": (-shift_x, shift_y),
            "image_below_left": (-shift_x, shift_y),
            "image_right_below": (shift_x, shift_y),
            "image_right_down": (shift_x, shift_y),
            "image_below_right": (shift_x, shift_y),
            "image_left_above": (-shift_x, -shift_y),
            "image_left_up": (-shift_x, -shift_y),
            "image_right_above": (shift_x, -shift_y),
            "image_right_up": (shift_x, -shift_y),
        }
        if direction not in mapping:
            raise ValueError("Unknown exploration direction: {}".format(direction))
        return mapping[direction]

    def _score_exploration_target_against_memory(
        self,
        target,
        memory,
        new_range_A,
        avoid_region_keys=("hazards", "stepped_regions"),
    ):
        center_x = float(target["OffsetX"])
        center_y = float(target["OffsetY"])
        half = float(new_range_A) / 2.0
        inside = []
        near = []
        for key in avoid_region_keys:
            for item in memory.get(key, []):
                wx, wy = self._memory_item_world_center(item)
                if wx is None or wy is None:
                    continue
                dx = abs(wx - center_x)
                dy = abs(wy - center_y)
                record = {
                    "source": key,
                    "world_x_A": wx,
                    "world_y_A": wy,
                    "distance_to_center_A": float(np.hypot(wx - center_x, wy - center_y)),
                }
                if dx <= half and dy <= half:
                    inside.append(record)
                elif dx <= half + 150.0 and dy <= half + 150.0:
                    near.append(record)
        return {
            "known_avoid_regions_inside_new_frame": inside,
            "known_avoid_regions_near_new_frame": near,
            "inside_count": len(inside),
            "near_count": len(near),
            "note": (
                "This only scores known memory objects. New scan monitoring "
                "still watches for newly appearing major bump/pit hazards."
            ),
        }

    def _memory_item_world_center(self, item):
        for x_key, y_key in (
            ("world_x_A", "world_y_A"),
            ("center_world_x_A", "center_world_y_A"),
        ):
            if x_key in item and y_key in item:
                try:
                    return float(item[x_key]), float(item[y_key])
                except Exception:
                    pass
        if "bbox_world_corners" in item:
            try:
                xs = [float(corner["world_x_A"]) for corner in item["bbox_world_corners"]]
                ys = [float(corner["world_y_A"]) for corner in item["bbox_world_corners"]]
                return float(np.mean(xs)), float(np.mean(ys))
            except Exception:
                pass
        return None, None

    def append_landscape_memory(
        self,
        analysis,
        memory_file="landscape_memory.json",
        latest_file="landscape_memory_latest.json",
    ):
        """Append a compact world-map entry to persistent exploration memory."""
        path = self.output_dir / memory_file
        latest_path = self.output_dir / latest_file
        if path.exists():
            try:
                memory = json.loads(path.read_text())
            except Exception:
                memory = []
        else:
            memory = []
        if isinstance(memory, dict):
            memory = memory.get("entries", [])
        entry = {
            "timestamp": analysis.get("timestamp"),
            "scan_frame": analysis.get("scan_frame"),
            "best_flat_candidate": analysis.get("best_flat_candidate"),
            "flat_candidates": analysis.get("flat_candidates", []),
            "hazards": analysis.get("hazards", []),
            "stepped_regions": analysis.get("stepped_regions", []),
            "abnormal_contrast_lines": analysis.get("abnormal_contrast_lines", []),
            "plot": analysis.get("plot"),
            "report": "{}_report.json".format(
                Path(analysis.get("plot", "landscape")).stem
            ),
        }
        memory.append(entry)
        path.write_text(json.dumps(self.runner._jsonable(memory), indent=2))
        latest_path.write_text(json.dumps(self.runner._jsonable(entry), indent=2))
        return str(path)

    def reset_landscape_memory(
        self,
        memory_file="landscape_memory.json",
        latest_file="landscape_memory_latest.json",
        archive_existing=True,
        reason="operator requested clean world map",
    ):
        """
        Reset landscape exploration memory for a new hyper-space/coarse area.

        This is file/in-memory bookkeeping only. It does not move the
        microscope.  When archive_existing=True, the previous memory files are
        renamed with a timestamp before the clean files are created.
        """
        timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
        paths = [self.output_dir / memory_file, self.output_dir / latest_file]
        archived = []
        if archive_existing:
            for path in paths:
                if path.exists():
                    archive_path = path.with_name(
                        "{}_archived_{}{}".format(
                            path.stem,
                            timestamp,
                            path.suffix,
                        )
                    )
                    path.rename(archive_path)
                    archived.append(str(archive_path))
        self.visited_offsets = []
        self.rejected_regions = []
        self.work_area_history = []
        self.runner.scan_site_history = []
        self.runner.bump_history = []
        clean_memory = {
            "created_at": datetime.now().isoformat(),
            "reason": reason,
            "entries": [],
        }
        (self.output_dir / memory_file).write_text(
            json.dumps(clean_memory, indent=2)
        )
        latest = {
            "created_at": clean_memory["created_at"],
            "reason": reason,
            "status": "empty_landscape_memory",
        }
        (self.output_dir / latest_file).write_text(json.dumps(latest, indent=2))
        report = {
            "timestamp": clean_memory["created_at"],
            "reason": reason,
            "memory_file": str(self.output_dir / memory_file),
            "latest_file": str(self.output_dir / latest_file),
            "archived": archived,
            "status": "reset",
        }
        self.write_report(report, "landscape_memory_reset_{}.json".format(timestamp))
        return report

    def mark_current_frame_rejected(
        self,
        reason="operator observed unexpected huge bumps",
        channel=0,
        memory_file="landscape_memory.json",
        latest_file="landscape_memory_latest.json",
    ):
        """
        Mark the current scan frame as rejected/hazardous in landscape memory.

        Use this when operator vision or live microscope context is more
        reliable than a thin partial scan.  It is read-only with respect to the
        microscope: it only reads current geometry and writes memory files.
        """
        settings = self.runner._scan_settings(channel)
        frame = self.scan_frame_world_footprint(settings=settings, channel=channel)
        entry = {
            "timestamp": datetime.now().isoformat(),
            "type": "rejected_landscape_frame",
            "reason": reason,
            "scan_frame": frame,
            "status": "avoid_for_future_exploration",
        }
        self.rejected_regions.append(entry)

        path = self.output_dir / memory_file
        latest_path = self.output_dir / latest_file
        if path.exists():
            try:
                memory = json.loads(path.read_text())
            except Exception:
                memory = []
        else:
            memory = []
        if isinstance(memory, dict):
            entries = memory.setdefault("entries", [])
            entries.append(entry)
            path.write_text(json.dumps(self.runner._jsonable(memory), indent=2))
        else:
            memory.append(entry)
            path.write_text(json.dumps(self.runner._jsonable(memory), indent=2))
        latest_path.write_text(json.dumps(self.runner._jsonable(entry), indent=2))
        self.write_report(
            entry,
            "landscape_rejected_frame_{}.json".format(
                datetime.now().strftime("%Y%m%d-%H%M%S")
            ),
        )
        return entry

    def plot_landscape_candidates(
        self,
        image,
        candidates,
        bumps=None,
        output_path="landscape_candidates.png",
        title="Landscape candidates",
    ):
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt

        arr = np.asarray(image, dtype=float)
        fig, ax = plt.subplots(figsize=(10, 7))
        vmin, vmax = np.nanpercentile(arr, [1, 99])
        im = ax.imshow(
            arr,
            origin="upper",
            cmap="viridis",
            aspect="auto",
            vmin=vmin,
            vmax=vmax,
        )
        ax.set_title(title)
        ax.set_xlabel("X pixel")
        ax.set_ylabel("Y pixel, line 0 at top")
        for bump in bumps or []:
            ax.plot(bump["px"], bump["py"], "rx", ms=7)
        for idx, candidate in enumerate(candidates[:8], start=1):
            ax.plot(candidate.px, candidate.py, "wo", ms=8, mec="k")
            ax.text(
                candidate.px + 4,
                candidate.py + 4,
                str(idx),
                color="white",
                fontsize=9,
                bbox={"facecolor": "black", "alpha": 0.45, "pad": 1},
            )
        fig.colorbar(im, ax=ax, label="Z / A")
        fig.tight_layout()
        fig.savefig(output_path, dpi=160)
        plt.close(fig)
        return str(output_path)

    def plot_landscape_world_map(
        self,
        image,
        candidates=None,
        hazards=None,
        stepped_regions=None,
        abnormal_lines=None,
        output_path="landscape_world_map.png",
        title="Landscape world map",
    ):
        """Plot flat candidates, hazards, and stepped regions on scan pixels."""
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        from matplotlib.patches import Rectangle

        arr = np.asarray(image, dtype=float)
        fig, ax = plt.subplots(figsize=(10, 8))
        vmin, vmax = np.nanpercentile(arr, [1, 99])
        im = ax.imshow(
            arr,
            origin="upper",
            cmap="viridis",
            aspect="equal",
            vmin=vmin,
            vmax=vmax,
        )
        ax.set_title(title)
        ax.set_xlabel("X pixel")
        ax.set_ylabel("Y pixel, line 0 at top")

        for region in stepped_regions or []:
            width = region["x1_px"] - region["x0_px"] + 1
            height = region["y1_px"] - region["y0_px"] + 1
            ax.add_patch(
                Rectangle(
                    (region["x0_px"], region["y0_px"]),
                    width,
                    height,
                    fill=False,
                    edgecolor="cyan",
                    linewidth=1.2,
                    alpha=0.9,
                )
            )

        for line in abnormal_lines or []:
            ax.axhspan(
                line["y0_px"] - 0.5,
                line["y1_px"] + 0.5,
                color="magenta",
                alpha=0.22,
            )
            ax.text(
                4,
                line["center_py"],
                "tip-state line",
                color="magenta",
                fontsize=8,
                va="center",
                bbox={"facecolor": "black", "alpha": 0.45, "pad": 1},
            )

        for hazard in hazards or []:
            ax.plot(hazard["px"], hazard["py"], "rx", ms=7)

        for idx, candidate in enumerate((candidates or [])[:10], start=1):
            ax.plot(candidate.px, candidate.py, "wo", ms=8, mec="k")
            ax.text(
                candidate.px + 4,
                candidate.py + 4,
                str(idx),
                color="white",
                fontsize=9,
                bbox={"facecolor": "black", "alpha": 0.45, "pad": 1},
            )

        fig.colorbar(im, ax=ax, label="Z / A")
        fig.tight_layout()
        fig.savefig(output_path, dpi=160)
        plt.close(fig)
        return str(output_path)

    def _leveled_line_flattened(self, image):
        plane = self._plane_leveled(image)
        return plane - np.nanmedian(plane, axis=1, keepdims=True)

    def _plane_leveled(self, image):
        arr = np.asarray(image, dtype=float)
        ny, nx = arr.shape
        y_grid, x_grid = np.mgrid[:ny, :nx]
        valid = np.isfinite(arr)
        if valid.sum() < 3:
            return arr - np.nanmedian(arr)
        design = np.column_stack(
            [
                x_grid[valid].ravel(),
                y_grid[valid].ravel(),
                np.ones(valid.sum()),
            ]
        )
        coeffs, *_ = np.linalg.lstsq(design, arr[valid].ravel(), rcond=None)
        return arr - (coeffs[0] * x_grid + coeffs[1] * y_grid + coeffs[2])

    def _running_median_1d(self, values, window=31):
        arr = np.asarray(values, dtype=float)
        n = len(arr)
        if n == 0:
            return arr
        window = max(3, int(window))
        if window % 2 == 0:
            window += 1
        half = window // 2
        out = np.empty(n, dtype=float)
        for idx in range(n):
            lo = max(0, idx - half)
            hi = min(n, idx + half + 1)
            out[idx] = np.nanmedian(arr[lo:hi])
        return out

    def _robust_sigma(self, values):
        arr = np.asarray(values, dtype=float)
        arr = arr[np.isfinite(arr)]
        if len(arr) == 0:
            return 0.0
        median = np.nanmedian(arr)
        mad = np.nanmedian(np.abs(arr - median))
        sigma = 1.4826 * mad
        if sigma > 0 and np.isfinite(sigma):
            return float(sigma)
        std = np.nanstd(arr)
        return float(std) if std > 0 and np.isfinite(std) else 0.0

    def _contiguous_true_groups(self, mask):
        groups = []
        start = None
        for idx, value in enumerate(mask):
            if value and start is None:
                start = idx
            elif not value and start is not None:
                groups.append((start, idx - 1))
                start = None
        if start is not None:
            groups.append((start, len(mask) - 1))
        return groups

    def _connected_component(self, mask, seed_x, seed_y):
        mask = np.asarray(mask, dtype=bool)
        ny, nx = mask.shape
        if not (0 <= seed_x < nx and 0 <= seed_y < ny) or not mask[seed_y, seed_x]:
            return np.zeros_like(mask, dtype=bool)
        component = np.zeros_like(mask, dtype=bool)
        stack = [(seed_x, seed_y)]
        component[seed_y, seed_x] = True
        while stack:
            x, y = stack.pop()
            for yy in range(max(0, y - 1), min(ny, y + 2)):
                for xx in range(max(0, x - 1), min(nx, x + 2)):
                    if mask[yy, xx] and not component[yy, xx]:
                        component[yy, xx] = True
                        stack.append((xx, yy))
        return component

    def _boxes_overlap_fraction(self, a, b):
        x0 = max(a["x0_px"], b["x0_px"])
        y0 = max(a["y0_px"], b["y0_px"])
        x1 = min(a["x1_px"], b["x1_px"])
        y1 = min(a["y1_px"], b["y1_px"])
        if x1 < x0 or y1 < y0:
            return 0.0
        intersection = float((x1 - x0 + 1) * (y1 - y0 + 1))
        area_a = float((a["x1_px"] - a["x0_px"] + 1) * (a["y1_px"] - a["y0_px"] + 1))
        area_b = float((b["x1_px"] - b["x0_px"] + 1) * (b["y1_px"] - b["y0_px"] + 1))
        return intersection / max(min(area_a, area_b), 1.0)

    def write_report(self, report, filename):
        path = self.output_dir / filename
        path.write_text(json.dumps(self.runner._jsonable(report), indent=2))
        return str(path)


if __name__ == "__main__":
    runner = MicroscopeActionRunner(dry_run=True)
    plan = runner.vp_probe_plan(analyze=False)
    runner.run_plan(plan)
    for item in runner.history:
        print(item)
