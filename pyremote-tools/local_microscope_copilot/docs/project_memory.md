# GXSM4 Local Microscope Copilot Project Memory

This document is a repo-maintained memory snapshot for future development of
the local GXSM4 microscope copilot. It summarizes operator-taught conventions,
current architecture, safety gates, and implementation decisions. Keep it
current when controller behavior, GUI policy, or microscope conventions change.

This file is intentionally not a raw chat transcript. It should not contain
passwords, local secrets, transient scan artifacts, private paths outside this
repo, or unattended-operation instructions.

## Project Goal

Build a local microscope copilot for a live GXSM4-controlled LT-STM/AFM. The
copilot should help with:

- safe readbacks and live monitor display,
- bounded Level-1 scan setup and parameter changes,
- scan/tip image analysis,
- GVP tip pulse and Z-dip tuning recipes,
- landscape/navigation memory,
- a Gradio GUI for plots, reports, live readouts, and controlled actions,
- optional local LLM interpretation constrained by deterministic safety gates.

The primary implementation lives in `local_microscope_copilot/`.

## Safety Model

The microscope can be live while this code is running. Hardware-changing actions
must stay gated.

Control levels:

- Level 0: hardware connected, monitoring/read-only.
- Level 1: basic operation within default safety margins.
- Level 2: reserved for advanced GVP and more aggressive tip tuning.
- Level 3: extreme GVP, THV coarse motion, hyper jumps, and auto approach.

Level-1 chat/button actions require an arm checkbox. GVP execution also requires
the exact phrase `EXECUTE GVP`. Tip Tune Planner loop execution requires
`EXECUTE TIP LOOP`. Level-3 actions require exact Level-3 confirmations.

The current controller and GUI limits are not hard-coded policy anymore. They
live in:

- `microscope_controller_config.json`
- `microscope_gui_config.json`

GXSM also stores live GUI settings and adjustment limits in dconf. Read them
read-only to compare against copilot limits before sending a value. This matters
because exceeding GXSM's own adjustment limits can pop up a blocking warning
dialog and reject the change until the operator acknowledges it.

Examples:

- Current main-window scan geometry/view values:
  `/org/gnome/gxsm4/pcs/mainwindow/range-x`
- Current PACPLL/controller values such as bias/current/feedback:
  `/org/gnome/gxsm4/pcs/plugin-librpspmc-pacpll/dsp-fbs-bias`
- Adjustment metadata:
  `/org/gnome/gxsm4/pcsadjustments/plugin-librpspmc-rpspmc-pacpll/dsp-fbs-bias`

The adjustment arrays use the first two elements as GXSM upper/lower limits.
For example `dsp-fbs-bias=[5.0, -5.0, ...]` means GXSM's GUI limit is
approximately `-5 V .. +5 V`, while the copilot Level-1 automatic limit can
remain stricter.

The legacy `local_microscope_copilot_config.json` is now model/service focused.

## Key GXSM Parameters

Scan geometry and position:

- `RangeX`, `RangeY`: scan range in Angstrom.
- `PointsX`, `PointsY`: number of pixels/points.
- `StepsX`, `StepsY`: informative pixel spacing, `Range/(Points-1)`, in
  Angstrom.
- `OffsetX`, `OffsetY`: scan-frame center in non-rotated world coordinates.
- `ScanX`, `ScanY`: tip position within the current scan coordinate system.
- `Rotation`: scan rotation in degrees around the scan-frame center.

Feedback and scan control:

- `dsp-fbs-bias`: bias in Volts.
- `dsp-fbs-mx0-current-set`: current setpoint in nA. Convert pA to nA before
  sending to GXSM.
- `dsp-fbs-mx0-current-level`: current limit for constant-height/Z mode.
- `dsp-adv-dsp-zpos-ref`: Z-position reference used with current limit modes.
- `dsp-fbs-cp`, `dsp-fbs-ci`: feedback CP/CI in dB; around `-90 dB` is a common
  default.
- `dsp-fbs-scan-speed-scan`: scan speed in Angstrom/s.
- `dsp-fbs-scan-speed-move`: tip/offset move speed in Angstrom/s.
- `dsp-adv-scan-slope-x`, `dsp-adv-scan-slope-y`: scan leveling slopes in
  world X/Y, typical absolute values below `0.1`.

GVP monitors:

- `dsp-GVP-XS-MONITOR`, `dsp-GVP-YS-MONITOR`: X/Y in Angstrom.
- `dsp-GVP-ZS-MONITOR`: GVP Z offset in Angstrom.
- `dsp-GVP-U-MONITOR`: GVP bias in Volts.

Fast monitor/readout methods:

- `gxsm.rtquery("f")`: dFrequency snapshot in Hz.
- `gxsm.rtquery("s")`: scan/GVP status. `Sgvp == 0` and `Sgvp == 5` are ready;
  `Sgvp == 1` is busy.
- `gxsm4process.rt_query_xyz()`: updates `XYZ_monitor["monitor"]`; values are
  controller Volts, not Angstrom.
- `gxsm4process.rt_query_rpspmc()`: fast controller values including bias,
  current, GVP U, PAC amplitude, and PAC dFreq.
- `gxsm.get_instrument_gains()`: returns `[VX, VY, VZ, AVX, AVY, AVZ]`.
  `VX/VY/VZ` are instrument amplifier gains. `AVX/AVY/AVZ` are the
  Angstrom-to-Volt conversion factors. To convert a local hazard coordinate to
  raw controller volts for the live XY panel, use
  `controller_X_V = X_A / AVX / VX` and similarly for Y/Z.

## Coordinate Conventions

The authoritative code layer for coordinate transforms is
`microscope_coordinates.py`; see `docs/coordinate_system_reference.md`.
Do not duplicate pixel/local/world/controller Volt formulas in GUI, planner, or
action code.  Build `ScanGeometry` from instrument-returned GXSM scan settings
and `InstrumentGains` from `gxsm.get_instrument_gains()`/parsed gain data, then
use those methods for transforms.

Offset coordinates:

- `OffsetX/Y` are non-rotated world coordinates.
- The scan view is centered at `OffsetX/Y`.
- The scan may be rotated by `Rotation` around that center.

Image coordinates:

- The image coordinate system is regular right-handed with positive Y upward.
- Line numbers count from top to bottom: top line is line `0`.
- Top/first scan line corresponds to local `ScanY = +RangeY/2`.
- Bottom line corresponds to local `ScanY = -RangeY/2`.
- Left edge corresponds to local `ScanX = -RangeX/2`.

Relative image shifts:

- `shift image 100 A left` shifts the scan frame in local image coordinates,
  then rotates that local delta into world `OffsetX/Y`.
- At `Rotation = 0`, `shift image 100 A left` changes `OffsetX` by `-100 A`.
- At `Rotation = 90`, the same local-left shift maps into world Y.

## Units And Parsing

Physical chat values should include units unless the value is truly a count.

Accepted examples:

- Bias: `0.2 V`, `200 mV`, `1e-6 MV`.
- Current: `10 pA`, `0.01 nA`.
- Length/range/offset: `700 A`, `70 nm`, `0.1 um`.
- Scan speed: `700 A/s`, `70 nm/s`.
- Feedback: `-90 dB`.
- Rotation: `45 deg`, `45 degree`, or unitless numeric rotation interpreted as
  degrees.
- Points/counts: unitless counts are allowed when the command clearly refers to
  points.

Wrong physical units should be rejected, for example `1 Ang bias`.

## Data Channels

Current channel conventions:

- Channel 0: STM/topography image, absolute Z in Angstrom.
- Channel 1: current image, normally current setpoint plus noise.
- Channel 2: dFrequency image in Hz.
- Channel 4: excitation in mV.
- Channel 5: oscillation amplitude in mV.
- Channel 6: phase.
- Channel 7, if enabled: pixel time since scan start in ms.
- In some sessions a last STM overview/map copy has been available in channel 7.

Topo Z range has physical meaning. Z should ideally stay within roughly
`0 ... -1000 A`; negative is preferred for long-term safety because power loss
toward zero lifts the tip.

## dFrequency Interpretation

For attractive imaging, dFrequency is usually negative:

- `0 Hz`: tip not near surface or dFreq controller disabled.
- `0..-2 Hz`: excellent.
- `0..-4 Hz`: generally OK.
- More negative than about `-10 Hz`: may work but indicates multi-tip/large
  capacitive interaction concerns.

Use repeated samples, not a single snapshot, when judging dFreq because it is a
noisy live value.

## Tip Quality Heuristics

Good metal-tip signs:

- Sharp STM/topographic features.
- Step edges nearly step-function sharp relative to scan size.
- dFreq typically in the excellent/OK range.
- No strong doubled/tripled feature shadows in autocorrelation.

Possible poor-tip signs:

- Doubled, tripled, or shadowed features.
- Diagonal elongated correlation peaks.
- Step edges blurry over too large a spatial width.
- Large or asymmetric blobs after tuning actions can mark bad interaction sites.

One or a few scan lines with a whole-line Z/contrast offset followed by return
to normal are transient tip-state artifacts, for example molecule pickup/drop or
push during scanning.  Do not interpret such full-line offsets as a stable
hazard zone along the line.  They should be recorded separately as abnormal
contrast/line artifacts and masked before large-blob hazard detection.

Small blobs with height below roughly `3 A` may be molecules and should not by
themselves stop a planner loop. Larger/taller blobs or broad hazards should be
avoided when choosing work sites.

## GVP Tip Actions

Two Level-1 GVP families are currently modeled:

- Bias pulse:
  - Parameter: pulse `du` in Volts.
  - Level-1 cap currently from config, historically +/-4 V.
  - Template file: `gvp_bias_pulse_default_program.json`.

- Tip-tune Z dip:
  - Parameters: contact bias, dip depth, ramp time.
  - Gentle starting point: about `5 A` dip, `0 V` at contact.
  - Fine tune means a gentler default dip.
  - Template file: `gvp_tip_tune_template_current_latest.json`.
  - Vec 1 removes the actual scan bias.
  - Vec 3 applies contact bias.
  - Vec 6 removes contact bias.
  - Vec 7 restores scan bias.
  - All vector components and FB flags must be written explicitly through the
    final terminating vector with `# points = 0`, because stale values may remain
    in GUI fields otherwise.

GVP can run for minutes. The GUI supports longer waits and has a GVP STOP button
that writes NULL vectors and executes them as an emergency stop mechanism.

## Navigation And Landscape Memory

The navigation controller records:

- scan frame offset,
- scan rotation,
- scan size,
- flat candidate patches,
- hazards/large blobs,
- visited offsets.

Use `ScanX/Y` to move within the current scan coordinate system. Use
`OffsetX/Y` to relocate the scan frame in world coordinates. At current settings
roughly +/-2500 A has been treated as reachable for OffsetX/Y, but this should
be handled as an operational assumption rather than a guaranteed hardware
limit.

If a region has too many large hazards, plan an overlapping offset shift or ask
the operator for a larger relocation/hyper jump. World-map reset is supported
for new coarse/hyper relocated areas.

## LLM Architecture

The LLM is not trusted as a free-form microscope controller.

Chat flow:

1. Deterministic parser handles known actions/readbacks first.
2. Optional LLM Intent Mode may ask the local model for strict JSON only.
3. The JSON is reparsed through deterministic unit parsing and safety gates.
4. Generic advisory model response is allowed only after the above paths do not
   execute an action.

The LLM must not claim an action happened unless it passed through the explicit
controller gate. This avoids the generic model producing plausible but useless
SPM advice.

See `docs/llm_intent_and_action_gateway.md` for the full LLM/Intent Mode
contract, including deterministic parser precedence, strict JSON intent mode,
unit validation, action-level gates, and a proposed future online/larger-LLM
planner architecture.

## Main Files

- `microscope_gradio_gui.py`: Gradio app and chat/intent/action gateway.
- `microscope_gradio_policy.py`: control-level text, parameter aliases, router
  vocabulary.
- `microscope_actions.py`: high-level microscope controller, scan/tip analysis,
  GVP program manipulation, landscape navigation.
- `microscope_gradio_scan.py`: scan plotting and analysis GUI mixin.
- `microscope_gradio_gvp.py`: GVP load/execute/plot GUI mixin.
- `microscope_gradio_tip_planner.py`: tip tune planner GUI mixin.
- `gxsm4process.py`: local bundled GXSM pyremote transport copy. PySHM
  command calls and fast monitor SHM snapshots are serialized with a
  process-local `threading.RLock`. PySHM command calls also take the
  cross-process lock file `/tmp/gxsm4_pyshm_transaction.lock` by default, so
  separate external scripts using this wrapper do not interleave requests.
- `microscope_controller_config.json`: safety/controller config.
- `microscope_gui_config.json`: GUI defaults/display config.
- `local_microscope_copilot_config.json`: local model/service config.

The Gradio Control Level tab includes a read-only `Read GXSM dconf Settings /
Limits` button that reads configured main-window keys, PACPLL/controller keys,
and adjustment-limit keys, then compares known copilot limits against GXSM
adjustment ranges.

Relevant GXSM C++ counterpart: `plug-ins/common/pyremote.cpp` implements
`remote_getslice`, `remote_getslice_v`, the method table entries for
`get_slice`/`get_slice_v`, and `PySHMServer_Run`. It uses one shared method
name/argument/return pickle block, so every PySHM request must complete its full
handshake before another one starts. The Python wrapper copies returned pickle
bytes out of SHM before unpickling. See `docs/pyremote_cpp_reference.md` for
the maintained C++ bridge reference. `remote_getslice()` also reads scan
`mem2d` directly from the PySHM server thread; GXSM allows live scan fetches,
but planner image fetches should be sparse, chunked, and separated by deliberate
settle time rather than tightly chained. Current wrapper default:
`GXSM_PYSHM_MIN_INTERVAL_S=0.05`; scan-line planner polling is seconds-scale,
not sub-second. Scan-image chunks are payload-aware via
`GXSM_GETSLICE_MAX_PAYLOAD_MB=12.0`, so ordinary 400 x 400 float scans fit in
one `get_slice` transaction while larger scans split automatically below the
16 MB SHM block limit.

## Current Handoff Snapshot, 2026-06-05

This section is intended for someone continuing after a lost chat. It records
the current working state of the local copilot as of the latest operator-tested
session. Always verify `git status` first because this file may lag behind
uncommitted edits.

Repository location used during development:

- `/home/percy/VS/Gxsm4/pyremote-tools/local_microscope_copilot/`

Known good baseline commit before the most recent maintenance additions:

- `77db01a Add tip position overlays and handoff memory`

Important current local edits after that baseline:

- `microscope_gradio_gui.py`
- `microscope_gradio_tip_planner.py`
- `microscope_actions.py`
- `microscope_gui_config.json`
- `README.md`
- `docs/microscope_actions_console_manual.md`
- `docs/project_memory.md`

Recent GUI changes that were restarted and operator-tested as OK:

- The old `Live Readouts` page is now `Live Microscope State Monitor`.
- The live monitor block is named `Live Microscope Monitor`.
- Obsolete manual read buttons were removed because the fast live monitor
  supersedes them.
- JSON result panels were moved toward the bottom of each page.
- Scan image plots preserve scan aspect ratio and partial scans are not
  stretched into a square.
- The Scan Image tab manually fetches the current image and shows a last-line
  profile. Optional auto-refresh is available and uses the GUI microscope
  operation gate; it skips cycles while the Tip Tune Planner or another PySHM
  action owns the lane.
- The Tip/Landscape Analysis and Tip Tune Planner paths share flat-candidate
  state.
- Candidate plots can show numbered flat locations, selected location, hazards,
  and current tip position.
- `Read / Mark Current Tip` reads `dsp-GVP-XS-MONITOR` and
  `dsp-GVP-YS-MONITOR` and overlays a red current-tip marker.
- The current-tip marker is updated by explicit `Read / Mark Current Tip`
  clicks; timer-driven tip-marker reads are intentionally disabled.
- Clicking the analysis/planner image selects the nearest flat candidate when
  the installed Gradio event supplies plot click coordinates.
- `Move Tip To Selected Candidate` writes local `ScanX/ScanY` only after the
  existing Level-1 arm gate and readiness check pass.
- `Compute And Set Blob-Avoiding Scan Offset` sits below the candidate move
  controls. It uses the latest planner analysis and remembered large hazards to
  compute and apply a reachable OffsetX/Y scan frame that avoids big blobs,
  using the Offset Search range/overlap/points/start settings.

Operator-tested readiness rule for local ScanX/Y moves and GVP execution:

- Use `gxsm.rtquery("s")`.
- Interpret the third value, `Sgvp`.
- `Sgvp == 0`: ready, stopped/finished scan.
- `Sgvp == 5`: ready, completed GVP.
- `Sgvp == 1`: busy, scan or GVP active.
- Do not use a stale `waitscan(False)` line number as a hard busy blocker; if a
  stopped current line is not changing it may simply be the last stopped line.

Current service pattern:

```bash
cd /home/percy/VS/Gxsm4/pyremote-tools/local_microscope_copilot
./copilot_services.sh status
./copilot_services.sh restart gradio --live --https --require-auth --host 0.0.0.0 --port 7870
```

The password is intentionally not stored in git. Use the local ignored file
`.microscope_gui_password` or the `MICROSCOPE_COPILOT_PASSWORD` environment
variable.

Useful smoke checks after edits:

```bash
cd /home/percy/VS/Gxsm4/pyremote-tools
python3 -m py_compile \
  local_microscope_copilot/microscope_gradio_gui.py \
  local_microscope_copilot/microscope_gradio_scan.py \
  local_microscope_copilot/microscope_gradio_tip_planner.py

cd local_microscope_copilot
.venv/bin/python -c "import microscope_gradio_gui as m; b=m.MicroscopeGradioBackend({}, live=False); ui=m.build_ui(b); print(type(ui).__name__)"
```

The system `python3` may not have Gradio installed; use `.venv/bin/python` for
GUI build checks.

## Current GUI Map

Top-level tabs and purpose:

- `Control Level`: choose safety level, read dconf settings/limits.
- `Chat`: deterministic command router and optional LLM Intent Mode. Actions
  still need the correct level and arm checkbox.
- `Live Microscope State Monitor`: read-only fast XYZ, RPSPMC, bias, current,
  GVP U, PAC amplitude, PAC dFreq, and 2D XY visual indication. The XY panel
  overlays known large hazards from latest planner analysis and landscape
  memory. The visible XY panel is in controller/world Angstroms, not controller
  Volts. Visible X/Y/Z values must stay sourced from the fast SHM mapped
  `rt_query_xyz()` controller Volts, then converted to Angstroms with
  `A = Vmonitor * AVxyz` from `gxsm.get_instrument_gains()`. The bar and XY
  panel limits use the configured maximum controller voltage, default +/-5 V,
  converted with `AVxyz`. `AVxyz` already includes the active instrument gains.
  The GVP monitor fields are useful reference values, but not the source of
  truth for this live monitor. The live timer may call only the fast read-only
  SHM monitor paths `rt_query_xyz()` and `rt_query_rpspmc()`, using
  cached/default gains. The manual full-refresh button updates PySHM-backed
  instrument gains and GVP monitor values.
- `Scan Image`: manually fetch scan image and last-line profile, or enable
  gated auto-refresh. The timer must never overlap `get_slice` with planner,
  GVP, or other PySHM actions; if the operation lock is busy it skips.
- `Tip / Landscape Analysis`: analyze current scan, show topo plus dFreq image,
  rank flat candidates, mark hazards, read/mark current tip, select candidate,
  and move tip to selected candidate when safe.
- `Tip Tune Planner`: planner workflow for flat-site selection, offset search,
  and bounded iterative scan/analyze/move/tune loops. The bottom of the page
  shows an activity/status ledger with current action, next pending action,
  reasons for blocked/error states, timestamps, elapsed times, and persistent
  JSONL/state logs. The planner loop owns the GUI microscope-operation lock
  while running so its own updated reads are sequential and not interleaved by
  other button/chat PySHM actions. The activity refresh timer is UI/log-only;
  PySHM reads such as current-tip markers remain manual button actions.
- `Scan Leveling`: measure residual fast-axis slope and apply armed correction.
- `GVP`: load bias-pulse or tip-tune Z-dip templates, execute loaded GVP, plot
  ZS/current/dFreq over time, emergency GVP STOP.
- `Level 3 Protected`: THV coarse motion, large motion, watchdog-style auto
  approach, and immediate coarse-motion emergency stop.

## Current LLM/Intent Status

Do not rely on the generic model for direct microscope understanding. The
deterministic router is the trusted controller. It parses known operator
phrases, validates SI units, converts to GXSM base units, checks control level
and bounds, then calls controller methods. The LLM may help only by producing a
strict intent that is reparsed through the same deterministic path, or by giving
advisory text when no action is executed.

Known intent/parser expectations:

- Accept `set`, `apply`, `change`, and similar words for parameters.
- Bias values require voltage units, except explicit readback requests.
- Current setpoint values require current units; convert pA to nA for GXSM.
- Range/offset/length values require length units.
- Scientific notation such as `1e-6 MV` must parse correctly.
- Points are counts and may be unitless.
- Rotation may be unitless and interpreted as degrees; also accept `deg` and
  `degree`.
- Reject wrong physical dimensions, for example `1 Ang bias`.
- Support grouped scan geometry commands for range and points.
- Support absolute `OffsetX/Y` setting and relative image shifts in local image
  coordinates rotated into world `OffsetX/Y`.

## Current Tip/Landscape Planner State

The planner distinguishes:

- Small blobs/molecules: typically `abs(dZ) < 3 A`; do not stop the loop by
  themselves.
- Large hazards: broad/tall blobs, pits, or bumps; avoid for clean-area
  selection and stop/shift only if too many or too close to the selected work
  site.
- Flat candidates: ranked patches with low roughness/slope/gradient and away
  from avoid-worthy hazards.

Candidate movement:

- Uses local `ScanX/ScanY`, not `OffsetX/Y`.
- Requires the scan/GVP to be ready.
- The instrument prohibits the local tip move while scanning.
- Verify with `dsp-GVP-XS-MONITOR` and `dsp-GVP-YS-MONITOR`.

Planner activity and stop controls:

- The planner writes append-only activity events to
  `gui_outputs/tip_planner_activity.jsonl`.
- It also writes the current activity snapshot to
  `gui_outputs/tip_planner_activity_state.json`.
- The `Stop Tip Tune Planner Loop` button requests cooperative cancellation,
  calls `stopscan()` when live, and the loop exits at the next safe checkpoint
  with status `cancelled_by_operator`.
- `Clear Planner / Landscape History` archives and resets landscape memory and
  should be used after a hyper jump/new world relocation.

Large hazard offset planning:

- Large hazards are tracked from current analysis and prior landscape memory.
- A `stopped_for_large_blob_hazard` decision now includes the blocked reason,
  large hazard count, a computed hazard-avoiding offset plan, and a recommended
  next action.
- Offset shifts must pass a full scan-footprint check, not just center
  `OffsetX/Y` reachability: all rotated frame corners must fit within the
  current XY reachable area.
- When auto-shift is enabled, the planner tries the computed hazard-avoiding
  target first. Otherwise it tells the operator the exact proposed new
  `OffsetX/Y` target and why it was selected.

Coordinate reminder:

- Image line `0` is the top.
- Physical local ScanY is positive upward.
- Top of image is local `ScanY = +RangeY/2`.
- Bottom is local `ScanY = -RangeY/2`.
- At `Rotation = 90 deg`, visual up/down shifts map into world X changes; be
  careful not to apply unrotated intuition to offsets.

## Current GVP Details

Bias pulse:

- Load-only is safe.
- Execution requires arm and exact `EXECUTE GVP`.
- Bias pulse parameter is the `du` step in Volts.
- Very maximum historically allowed by operator for this family is +/-4 V, but
  use current config limits as the live authority.

Tip-tune Z dip:

- Gentle fine-tune default is around `5 A`.
- Vec 1 removes the current scan bias using `dU = -actual_bias`.
- Vec 3 applies the requested contact bias.
- Vec 6 removes the contact bias.
- Vec 7 restores the scan bias.
- Vec 2 and Vec 4 ramp times are parameterized; allowed range is currently
  `1 s .. 60 s`.
- All vector components and FB checkboxes must be explicitly written, including
  zero and false values, until the final terminating vector with `# points = 0`.
- Vector 4 feedback flag was operator-corrected to false for the Z-dip pattern.

Long GVP:

- Several-minute GVPs are possible.
- GUI waits up to 5 minutes for execution/plotting by default.
- Emergency GVP STOP writes all NULL vectors and executes them.

## Level 3 / Coarse Motion Status

Level 3 is intentionally protected and not casually live-tested.

THV coarse controls:

- Channels: `X`, `Y`, `Z`.
- Directions: `+1` or `-1`.
- Period default/fixed operationally around `0.5 s`.
- Normal cap: voltage <= `100 HV V`, burstcount <= `5`.
- Extra protected large motion: burstcount up to `5000`.
- Any `Z, -1` motion is dangerous because it moves the tip down.
- Red EMERGENCY STOP coarse motion is not arm-gated; it sends zero-step,
  zero-volt commands for the last started axis/direction and then all axes.

Auto approach/watchdog concept:

- Reads `dFreq()` from `gxsm.rtquery("f")`.
- Reads `VZ_tip()` from `gxsm.rtquery("z")`.
- Uses `script-control` as a user abort flag.
- Retract by setting current setpoint to zero and waiting while Z moves up.
- Check range by setting a small current and waiting for Z/dFreq response.
- Apply small Z-down coarse bursts only while dFreq and Z safety conditions are
  acceptable.

## Runtime Artifacts And Git Hygiene

Keep these in git:

- source `.py` files in `local_microscope_copilot/`,
- config `.json` defaults,
- GVP template/recipe JSON files needed by the GUI,
- `docs/*.md`,
- service/install scripts,
- README/setup docs.

Do not commit:

- `.microscope_gui_password`,
- `.venv/`,
- `.run/`,
- `logs/`,
- `__pycache__/`,
- live scan output images, `.npy`, `.npz`, and generated reports unless a
  particular file is intentionally promoted to a documented fixture.

The working root outside `local_microscope_copilot/` contains many historical
experiment artifacts from earlier live sessions. Treat them as local data, not
project source, unless the operator explicitly asks to preserve one.

## Maintenance Rules

Update this memory document when:

- safety limits or control levels change,
- a new GXSM parameter/action ID becomes part of the supported command set,
- scan channel conventions change,
- GVP recipe structure changes,
- coordinate transform conventions change,
- LLM intent schemas are added or removed,
- planner/tip-quality heuristics are materially revised.

Prefer concise, operator-verified facts over raw logs. Keep transient data,
passwords, certificates, and local-only runtime artifacts out of this file.
