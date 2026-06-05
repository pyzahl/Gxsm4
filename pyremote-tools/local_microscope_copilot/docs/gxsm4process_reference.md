# gxsm4process.py Reference

This document summarizes the GXSM4 Python pyremote wrapper bundled in
`local_microscope_copilot/gxsm4process.py`, plus the GXSM GUI refnames/actions
and data-fetching conventions used so far by the microscope copilot/controller.

The live system is an LT-STM/AFM. Treat `gxsm.set(...)`, `gxsm.action(...)`,
`startscan()`, `stopscan()`, `ScanX/Y`, `OffsetX/Y`, and GVP execution as
hardware-affecting operations.

## Connection

```python
import gxsm4process as gxsm4

gxsm = gxsm4.gxsm_process(verbose=0)
```

`gxsm_process` connects to GXSM4 through the PySHM/RPSPMC bridge. It discovers
the running GXSM4 process, maps shared memory, and exposes a simple method API.

PySHM is a single transaction buffer owned by GXSM4's `pyremote.cpp`
counterpart. The external Python side writes a method name and pickled
arguments into `/gxsm4_py_shm_data_block`, signals GXSM4, waits for the status
flag to return ready, then copies and unpickles the returned payload. Do not
interleave PySHM requests. The wrapper uses a process-local `RLock` plus the
cross-process lock file `/tmp/gxsm4_pyshm_transaction.lock` by default; set
`GXSM_PYSHM_LOCK_FILE` to share a different lock path.

Returned NumPy arrays, especially from `get_slice`, are pickled by GXSM4's
embedded Python/NumPy. For live use, run the external GUI/scripts with a Python
and NumPy ABI compatible with the GXSM4 build. The copilot venv should use
system site packages and should not install a separate pip NumPy wheel.

Advanced troubleshooting note: if `get_slice` or another large PySHM data
request causes a crash, timeout, corrupted return, or NumPy/pickle exception,
do not start by debugging the image-analysis code. First verify the transport
layer:

- Only one PySHM request may be active at a time; no threaded/interleaved
  `gxsm.get`, `gxsm.set`, `gxsm.action`, `get_slice`, or GVP requests.
- All external tools that talk to PySHM should use the same lock path
  (`GXSM_PYSHM_LOCK_FILE`, default `/tmp/gxsm4_pyshm_transaction.lock`).
- The external Python and NumPy must match the ABI expected by the GXSM4
  embedded Python/NumPy that created the returned object.
- Prefer chunked `get_slice` reads and immediately copy returned arrays into
  local NumPy arrays before further processing.

This is an advanced issue. Beginners should first reproduce with one simple,
single-process script using the system Python/NumPy before adding Gradio,
timers, planner loops, or other automation.

## Core API

### GUI Refname Access

```python
gxsm.get(refname)          # read GUI entry as value
gxsm.gets(refname)         # read GUI entry as string
gxsm.set(refname, value)   # write GUI entry
gxsm.list_refnames()       # list readable/writable GUI entry IDs
```

Use `gxsm.get(...)` for read-only monitoring. Use `gxsm.set(...)` only with an
appropriate GUI control level and safety checks.

### Actions

```python
gxsm.action(action_id)     # run a GXSM action
gxsm.list_actions()        # list available action IDs
```

Actions may be benign queries or may execute hardware operations. In particular,
GVP execution is a live tip action.

### Real-Time Queries

```python
gxsm.rtquery("f")          # dFrequency vector; fvec[0] is dFreq in Hz
gxsm.rtquery("s")          # status bits
gxsm.rtquery("X")          # real-time X monitor class query
gxsm.rtquery("Y")
gxsm.rtquery("Z")
gxsm.rtquery("xy")
gxsm.rtquery("zxy")
gxsm.rtquery("o")
gxsm.rtquery("i")
gxsm.rtquery("U")
```

Known dFrequency interpretation for attractive imaging:

- `0 Hz`: not near surface or dFreq controller disabled
- `0..-2 Hz`: excellent
- `0..-4 Hz`: OK
- `< -10 Hz`: multiple-tip concern
- positive values: special/repulsive scan context, not the normal rule

### Scan Start/Stop

```python
gxsm.startscan()
gxsm.stopscan()
gxsm.waitscan()
gxsm.scaninit()
gxsm.scanupdate()
gxsm.scanline()
gxsm.scanylookup()
```

Start/stop scan are Level 1 GUI operations in the copilot. They still require
the arm checkbox in the web GUI.

## Data Fetching

### Dimensions And Geometry

```python
gxsm.get_dimensions(ch)       # [nx, ny, nv, nt]
gxsm.get_geometry(ch)         # [rx, ry, x0, y0, alpha]
gxsm.get_differentials(ch)    # [dx, dy, dz, dl]
gxsm.get_instrument_gains()
gxsm.get_scan_unit()
```

`get_dimensions(ch)` is used before fetching scan images. For current live scan
progress the controller also uses:

```python
gxsm.y_current()
```

### Image / Channel Data

```python
gxsm.get_data_pkt(ch, x, y, v, t)
gxsm.put_data_pkt(value, ch, x, y, v, t)
gxsm.get_slice(ch, v, t, yi, yn)
gxsm.get_slice_v(ch, x, t, yi, yn)
```

`get_slice` and `get_slice_v` can return large NumPy arrays through the same
PySHM pickle block. Keep them sequential and chunked; do not call them from
timer threads or in parallel with `gxsm.get`, `gxsm.set`, `gxsm.action`, GVP
execution, or other PySHM methods.

The controller fetches a scan image from the top down in chunks:

```python
dims = gxsm.get_dimensions(channel)
ny = int(dims[1])
y_current = int(float(gxsm.y_current()))
y_count = ny if y_current >= ny - 1 else max(1, min(ny, y_current + 1))

chunks = []
for yi in range(0, y_count, chunk_lines):
    yn = min(chunk_lines, y_count - yi)
    block = gxsm.get_slice(channel, 0, 0, yi, yn)
    chunks.append(block)
image = numpy.vstack(chunks)
```

Image display convention in the copilot:

- line 0 is displayed at the top
- left edge is local `ScanX = -RangeX/2`
- top/first line is local `ScanY = +RangeY/2`
- bottom line is local `ScanY = -RangeY/2`
- line numbers count downward, while physical scan Y is positive upward
- plot aspect is physical Angstrom aspect, so partial scans are not stretched

### Probe / VP / GVP Event Data

```python
columns, labels, units, xyij = gxsm.get_probe_event(ch, nth)
```

Known behavior:

- `nth=-1`: fetch last event
- `nth=-99`: remove all events
- if `nth` exceeds existing events, GXSM returns the probe-event count

Typical returned labels observed:

- `ZS-Topo` in Angstrom
- `Current` in nA
- `dFrequency` in Hz
- `Time-Mon` in ms

The controller uses this after VP/GVP execution to plot/evaluate ZS position
versus time and compare pre/post pulse levels.

## Scan Data Channels

Channel conventions used so far:

| Channel | Meaning |
| --- | --- |
| `0` | STM/topography image. Absolute Z in Angstrom. |
| `1` | Current in nA, usually setpoint plus noise. |
| `2` | dFrequency signal in Hz. |
| `4` | Excitation in mV. |
| `5` | Oscillation amplitude readback in mV. |
| `6` | Phase signal. |
| `7` | Last STM overview/map copy or optional pixel time, depending configuration. |

Topo Z guidance:

- Channel 0 is absolute Z in Angstrom.
- A long-term safe operating range is ideally around `0 ... -1000 A`.
- Negative Z side is preferred for long-term action because going toward `0`
  lifts the tip in a power-loss/fallback scenario.

## Known gxsm.get / gxsm.set Refname IDs

### Scan Geometry

| Refname | Unit / Meaning | Notes |
| --- | --- | --- |
| `RangeX` | Angstrom | Scan width. |
| `RangeY` | Angstrom | Scan height. |
| `PointsX` | pixels | X samples. |
| `PointsY` | pixels | Y samples. |
| `StepsX` | steps | Basic scan geometry field; not yet actively used by controller. |
| `StepsY` | steps | Basic scan geometry field; not yet actively used by controller. |
| `OffsetX` | Angstrom world coordinate | Scan-frame center, non-rotated world X. |
| `OffsetY` | Angstrom world coordinate | Scan-frame center, non-rotated world Y. |
| `Rotation` | degrees | Scan view rotation around OffsetX/Y. |
| `ScanX` | Angstrom local scan coordinate | Moves tip within current scan frame. |
| `ScanY` | Angstrom local scan coordinate | Moves tip within current scan frame. |

Coordinate conventions:

- `ScanX/ScanY` move the tip inside the current scan area.
- `OffsetX/OffsetY` relocate the scan frame in the non-rotated world coordinate
  system.
- At `Rotation=90 deg`, moving the displayed scan down corresponds to decreasing
  world `OffsetX`.

### Feedback / Scan Control

| Refname | Unit / Meaning | Safety / Notes |
| --- | --- | --- |
| `dsp-fbs-bias` | V | Bias. Level 1 GUI limit is `abs(bias) <= 1 V`. |
| `dsp-fbs-mx0-current-set` | nA | Current setpoint. Convert pA to nA before writing. Level 1 limit `<=0.05 nA`. |
| `dsp-fbs-mx0-current-level` | nA | Current limit for constant-height/Z modes. Used with `dsp-adv-dsp-zpos-ref`. |
| `dsp-adv-dsp-zpos-ref` | Angstrom / Z reference context | Used with current-level limit in special modes. |
| `dsp-fbs-cp` | dB | Feedback proportional gain. Default around `-90 dB`. |
| `dsp-fbs-ci` | dB | Feedback integral gain. Default around `-90 dB`. |
| `dsp-fbs-scan-speed-scan` | A/s | Scan speed. Typical `0.5..1.5 x scan size A/s`. |
| `dsp-fbs-scan-speed-move` | A/s | Tip/offset move speed. |
| `dsp-adv-scan-slope-x` | AngZ/AngX | World-X slope leveling. Typical abs `<0.1`. |
| `dsp-adv-scan-slope-y` | AngZ/AngY | World-Y slope leveling. Typical abs `<0.1`. |

Slope-leveling convention:

- Use `Rotation=0 deg` and fast X direction to level world X.
- Use `Rotation=90 deg` and fast X direction to level world Y.
- Sign depends on scan direction/rotation; verify after applying.

### Live GVP / Tip Monitor Refrefs

| Refname | Unit / Meaning |
| --- | --- |
| `dsp-GVP-XS-MONITOR` | Angstrom, live X tip/GVP monitor |
| `dsp-GVP-YS-MONITOR` | Angstrom, live Y tip/GVP monitor |
| `dsp-GVP-ZS-MONITOR` | Angstrom, live GVP/Pulse Z offset |
| `dsp-GVP-U-MONITOR` | V, live bias/GVP voltage monitor |

Normal idle expectation: `dsp-GVP-ZS-MONITOR` should be zero or very small when
no GVP/Pulse is active.

### GVP Vector Program Numeric Fields

The controller reads/writes all numeric vector fields matching:

```text
dsp-gvp-*
```

Important parameterized fields currently used:

| Refname Pattern | Meaning |
| --- | --- |
| `dsp-gvp-duNN` | Bias/voltage delta for vector row `NN`. |
| `dsp-gvp-dzNN` | Z delta for vector row `NN`. |
| `dsp-gvp-dtNN` | Row duration/time step context. |
| `dsp-gvp-nNN` | Number of points for row `NN`; `n=0` terminates program. |
| `dsp-gvp-nrepNN` | Repeat count for row `NN`. |
| `dsp-gvp-xcmpvNN` | Conditional vector comparison value. |
| `dsp-gvp-xopcdNN` | Conditional vector opcode. |
| `dsp-gvp-xrchiNN` | Conditional vector channel/index. |

Operational rule learned during tuning: when loading a GVP program, write all
numeric entries, including zeros, and all checkbox states, including `FALSE`,
up to the terminating row. Stale GUI state persists otherwise.

## Known gxsm.action IDs

### GVP / VP

| Action ID | Meaning |
| --- | --- |
| `DSP_VP_VP_EXECUTE` | Execute current VP/GVP program. Live tip action. |

### GVP Checkbox Query/Set Actions

Checkboxes are action IDs, not `gxsm.set` fields:

```python
gxsm.action("GETCHECK-dsp-gvp-FB00")     # returns "TRUE" or "FALSE"
gxsm.action("CHECK-dsp-gvp-FB00")        # set checkbox TRUE
gxsm.action("UNCHECK-dsp-gvp-FB00")      # set checkbox FALSE
```

Known checkbox patterns:

| Action Pattern | Meaning |
| --- | --- |
| `GETCHECK-dsp-gvp-FBNN` | Query feedback/control checkbox for vector row `NN`. |
| `CHECK-dsp-gvp-FBNN` | Enable feedback/control checkbox for row `NN`. |
| `UNCHECK-dsp-gvp-FBNN` | Disable feedback/control checkbox for row `NN`. |
| `GETCHECK-dsp-gvp-VSNN` | Query additional per-row vector checkbox state. |
| `CHECK-dsp-gvp-VSNN` | Enable per-row VS checkbox. |
| `UNCHECK-dsp-gvp-VSNN` | Disable per-row VS checkbox. |
| `GETCHECK-dsp-gvp-RFAM` | Query global GVP checkbox. |
| `GETCHECK-dsp-gvp-RFFM` | Query global GVP checkbox. |

Known GVP recipes:

- Bias pulse: default template `gvp_bias_pulse_default_program.json`; maximum
  allowed pulse parameter in controller is `+/-4 V`.
- Tip dip: template `gvp_tip_tune_template_current_latest.json`; start gently
  around `-5 A` indentation at nominal `0.1 V`, `10 pA` scan conditions.

## File / Display / Channel Utilities

`gxsm4process.py` also exposes wrappers for scan file and channel operations:

```python
gxsm.autosave()
gxsm.autoupdate()
gxsm.save()
gxsm.saveas(ch, fname)
gxsm.load(ch, fname)
gxsm.export(ch, fname)
gxsm.data_import(ch, fname)
gxsm.chfname(ch)
gxsm.chmodea(ch)
gxsm.chmodex(ch)
gxsm.chmodem(ch)
gxsm.chmoden(ch, n)
gxsm.chmodeno(ch)
gxsm.chview1d(ch)
gxsm.chview2d(ch)
gxsm.chview3d(ch)
```

These have not been central to the autonomous controller yet, but are available
through the low-level wrapper.

## Object / Marker Utilities

```python
gxsm.get_object(ch, n)
gxsm.add_marker_object(ch, label, mgrp, ix, iy, size)
gxsm.marker_getobject_action(ch, objnameid, action)
gxsm.save_drawing(ch, time, layer, fname)
gxsm.set_view_indices(ch, time, layer)
```

## Shared Parameters / Labels / Logging

```python
gxsm.set_global_share_parameter(key, x)
gxsm.set_sc_label(id, text)
gxsm.echo(text)
gxsm.logev(text)
gxsm.progress()
gxsm.add_layerinformation(text, ch)
gxsm.signal_emit(action)
gxsm.sleep(tenths)
```

## Convenience Methods In gxsm4process.py

| Method | Meaning |
| --- | --- |
| `set_current_level_zcontrol()` | Copies `dsp-fbs-mx0-current-set` into `dsp-fbs-mx0-current-level`. |
| `set_current_level_0()` | Sets `dsp-fbs-mx0-current-level` to zero. |
| `read_status()` | Reads the RPSPMC monitor/status block. |
| `rt_query_xyz()` | Updates/read XYZ monitor values. |
| `rt_query_rpspmc()` | Updates/read RPSPMC monitor values. |

## Copilot Safety Mapping

The Gradio GUI currently uses these control levels:

- Level 0: read-only monitoring and analysis.
- Level 1: bounded basic operation:
  - start/stop scan
  - bias up to `+/-1 V`
  - current setpoint up to `0.05 nA`
  - normal scan speed `0.5..1.5 x Range`
  - feedback CP/CI within `-120..-30 dB`
  - safe GVP load/execute with explicit `EXECUTE GVP` confirmation
- Level 2: reserved for advanced GVP and more aggressive tip tuning.
- Level 3: reserved for extreme GVP, coarse motion/hyper jumps, and auto
  approach workflows.
