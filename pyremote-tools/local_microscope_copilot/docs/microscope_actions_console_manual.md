# Microscope Actions Console Manual

This note shows how to use `microscope_actions.py` manually from an external
Python console or script. The file now lives in:

```text
/home/percy/VS/Gxsm4/pyremote-tools/local_microscope_copilot/microscope_actions.py
```

The live system is an LT-STM/AFM microscope. Start in dry-run mode unless you
are intentionally connected to GXSM4 and know whether the call is read-only or
hardware-changing.

## Start A Console

From the copilot/controller directory:

```bash
cd /home/percy/VS/Gxsm4/pyremote-tools/local_microscope_copilot
python3
```

Then import the controller classes:

```python
from microscope_actions import (
    MicroscopeActionRunner,
    TipImprovementActivityController,
    LandscapeNavigationController,
)
```

Dry-run object, safe for planning:

```python
runner = MicroscopeActionRunner(dry_run=True)
```

Live GXSM4 object:

```python
runner = MicroscopeActionRunner(dry_run=False, verbose=0)
runner.connect()
```

The first live call attaches to the running GXSM4 process through the bundled
`gxsm4process.py` copy in this same folder.

## Safety Categories

Read-only / diagnostic calls:

```python
runner.get_parameter("dsp-fbs-bias")
runner.dFreq()
runner.interpret_dFreq(-2.3)
runner.ready_check()
runner.fetch_scan_image_to_line(channel=0)
runner.fetch_scan_auxiliary_channels_to_line()
runner.assess_tip_quality(image, pixel_size_A=1.7)
runner.find_clean_patch(image)
runner.detect_major_bumps(image)
runner.fetch_current_gvp_program()
runner.save_current_gvp_program("current_gvp.json")
```

Hardware-changing calls:

```python
runner.set_parameter("dsp-fbs-bias", 0.1)
runner.action("DSP_CMD_AUTOAPP")
runner.setup_scan_geometry(range_A=700, points=400)
runner.move_to_scan_xy_fields(100, -50)
runner.load_gvp_program(program)
runner.load_gvp_bias_pulse(2.0)
runner.load_gvp_tip_dip(dip_depth_A=-5.0, contact_bias_V=0.0, scan_bias_V=0.1)
runner.execute_loaded_gvp()
nav.move_to_offset(-1000, 500)
```

`load_gvp_*` writes GUI vector-program fields but does not execute the program.
`execute_loaded_gvp()` runs the currently loaded GVP and is a live tip action.

## Basic Readbacks

Bias in Volts:

```python
rec = runner.get_parameter("dsp-fbs-bias", collect_as="bias_V")
print(runner.data["bias_V"])
```

Current setpoint in nA:

```python
rec = runner.get_parameter("dsp-fbs-mx0-current-set", collect_as="current_nA")
print(runner.data["current_nA"])
```

dFrequency in Hz:

```python
df = runner.dFreq()
print(df)
print(runner.interpret_dFreq(df))
```

Use repeated dFreq samples through the activity controller:

```python
activity = TipImprovementActivityController(runner)
report = activity.sample_dfrequency(count=20, delay_s=0.15)
print(report["stats"])
print(report["interpretation"])
```

Normal dFreq interpretation for attractive imaging:

- `0 Hz`: not near surface or dFreq controller disabled
- `0..-2 Hz`: excellent
- `0..-4 Hz`: OK
- `< -10 Hz`: multi-tip concern
- positive: special/repulsive-mode context, do not apply the normal rule

## Scan Geometry

Set diagnostic scan size and points:

```python
runner.setup_scan_geometry(range_A=700, points=400)
```

Direct GUI field setting:

```python
runner.set_parameter("RangeX", 700)
runner.set_parameter("RangeY", 700)
runner.set_parameter("PointsX", 400)
runner.set_parameter("PointsY", 400)
runner.set_parameter("Rotation", 90)
```

Live scan-speed adjustment while scanning:

```python
runner.set_live_scan_speed(multiplier=1.0)
```

Operator rule of thumb: scan speed is typically about `0.5..1.5 x` the scan
size in Angstrom per second. For an `800 A` scan, that is roughly
`400..1200 A/s`.

Live bias and current adjustment while scanning:

```python
runner.set_live_bias(0.2)                       # Volts, auto limit +/-1 V
runner.set_live_current_setpoint(10, unit="pA") # converted to 0.01 nA
runner.set_live_current_setpoint(0.01, unit="nA")
```

Current is always written to GXSM in nA. The helper blocks automatic current
setpoints above `0.05 nA`.

Scan-slope leveling:

```python
runner.set_scan_slope_leveling(slope_x=0.012)
runner.set_scan_slope_leveling(slope_y=-0.018)
runner.set_scan_slope_leveling(slope_x=0.012, slope_y=-0.018)
```

The GUI fields are:

- `dsp-adv-scan-slope-x`
- `dsp-adv-scan-slope-y`

They are slow leveling parameters in non-rotated world X/Y. Use
`Rotation=0 deg` and the fast X scan direction to level world X. Use
`Rotation=90 deg` and the fast X scan direction to level world Y. Typical
absolute slope values are `< 0.1`, and the helper blocks larger values by
default. The sign of a live correction depends on scan direction and rotation:
after applying a small correction, re-check a few fresh lines and reverse the
sign if the residual slope grows.

Estimate a slope from a fetched image:

```python
suggestion = runner.suggest_scan_slope_from_plane(image)
print(suggestion)
```

Treat this as a suggestion, not an automatic correction. Apply the X or Y
component only from the appropriate rotation scan.

Live residual fast-axis slope correction from recent lines:

```python
before = runner.measure_recent_fast_axis_slope(
    line_count=64,
    output_prefix="manual_slope_before",
)
print(before)

result = runner.auto_correct_scan_slope_from_recent_lines(
    rotation=90,             # correct world Y from a 90 deg scan
    correction_sign=+1.0,    # sign depends on scan direction/rotation
    gain=1.0,
    verify_delay_s=20.0,
    output_prefix="manual_y_slope_correction",
)
print(result["improved"])
```

Use `rotation=0` to correct world X via `dsp-adv-scan-slope-x`, and
`rotation=90` to correct world Y via `dsp-adv-scan-slope-y`. If the residual
gets worse, restore or rerun with `correction_sign *= -1`; the sign depends on
scan direction and rotation.

Coordinate convention:

- `ScanX` / `ScanY`: local tip position inside the current scan coordinate
  system.
- The scan image coordinate system is right handed: physical/local `Y` is
  positive upward in the image. Pixel line numbers are separate from physical
  Y and count from top to bottom:
  - left image edge: local `ScanX = -RangeX/2`
  - right image edge: local `ScanX = +RangeX/2`
  - top/first scan line, line index 0: local `ScanY = +RangeY/2`
  - bottom scan line, line index N: local `ScanY = -RangeY/2`
- The helper `pixel_to_scan_xy(px, py)` converts row index to right-handed
  local scan coordinates and returns those values as `ScanX_A`/`ScanY_A`.
  It also reports `row_down_y_A` for debugging the row-index sign.
- `OffsetX` / `OffsetY`: scan-frame center in non-rotated world coordinates.
- `Rotation`: rotates the scan view around the `OffsetX/Y` center.
- Example: at `Rotation=90 deg`, moving the scan image downward corresponds to
  a negative world-`OffsetX` change. A 200 A image-down shift is
  approximately `OffsetX -= 200 A`, with `OffsetY` unchanged.

Move the tip locally inside the current scan:

```python
runner.move_to_scan_xy_fields(scan_x_A=100, scan_y_A=-50)
```

Read live tip-position monitor values:

```python
runner.get_live_tip_position_monitor()
print(runner.data["live_tip_position"])
```

The monitor refs are:

- `dsp-GVP-XS-MONITOR`
- `dsp-GVP-YS-MONITOR`
- `dsp-GVP-ZS-MONITOR`
- `dsp-GVP-U-MONITOR`

`XS`/`YS` change continuously while scanning because they follow the raster.
`XS`, `YS`, and `ZS` are in Angstrom. `ZS` is the Z offset applied for
GVP/Pulse actions and normally should be zero or very small when no GVP/Pulse
action is active. `U` is the live bias monitor in Volts. For verifying a GVP
setup, stop the scan and wait for settling before checking the monitor values.

Use verified local moves before GVP tip actions:

```python
runner.move_to_scan_xy_fields_verified(
    scan_x_A=100,
    scan_y_A=-50,
    tolerance_A=5,
)
print(runner.data["last_verified_scan_xy_move"])
```

Move the scan frame in the wider world coordinate system:

```python
nav = LandscapeNavigationController(runner)
nav.move_to_offset(offset_x_A=-1110, offset_y_A=370)
```

## Fetch And Plot Scan Data

Fetch channel 0 from line 0 through the current scan line:

```python
image, meta = runner.fetch_scan_image_to_line(channel=0)
print(meta)
```

Fetch through a specific line:

```python
image, meta = runner.fetch_scan_image_to_line(channel=0, y_current=150)
```

Plot with line 0 at the top:

```python
runner.plot_scan_image(
    image,
    "manual_scan_ch0.png",
    title="Manual Ch0 scan, line 0 at top",
)
```

Auxiliary channel summaries:

```python
aux = runner.fetch_scan_auxiliary_channels_to_line()
print(aux)
```

Channel conventions:

- `Ch=0`: topography / absolute Z in Angstrom
- `Ch=1`: current in nA
- `Ch=2`: scan-recorded dFrequency in Hz
- `Ch=4`: excitation in mV
- `Ch=5`: oscillation amplitude in mV
- `Ch=6`: phase
- `Ch=7`: channel #8 auxiliary, usually overview/map copy in the current setup

## Tip Quality Analysis

Assess the current fetched image:

```python
pixel_size_A = runner._pixel_size_A(channel=0)
quality = runner.assess_tip_quality(
    image,
    pixel_size_A=pixel_size_A,
    output_prefix="manual_tip_quality",
)
print(quality["verdict"])
print(quality["autocorrelation_secondary_peaks"][:3])
print(quality["topo_feature_sharpness"])
```

Combine dFreq with image assessment:

```python
df = runner.dFreq()
quality["dFrequency"] = runner.interpret_dFreq(df)
print(runner.tip_quality_is_poor(quality))
```

The Gradio `Tip / Landscape Analysis` tab additionally fetches `Ch=2` as the
scan-recorded dFrequency image and plots it next to the `Ch=0` topo image. The
JSON report includes both `scan_recorded_dFrequency_ch2` statistics and a
`live_average_dFrequency` sampled from `rtquery("f")`. Topo sharpness is reported
under `tip_quality.topo_feature_sharpness`; these criteria are for judging a
good metal tip and are intentionally not yet a CO/O-functionalized-tip
classifier.

Find a clean local patch:

```python
patch = runner.find_clean_patch(image)
print(patch)
if patch:
    print(runner.pixel_to_scan_xy(patch["px"], patch["py"]))
```

Detect major bumps or pits:

```python
bumps = runner.detect_major_bumps(image, max_bumps=20)
print(bumps[:5])
```

## Full Scan-Watch-Tip Workflow

Plan only:

```python
runner = MicroscopeActionRunner(dry_run=True)
rec = runner.scan_watch_assess_tip(
    start_scan=True,
    setup_scan=True,
    range_A=700,
    points=400,
    min_lines=100,
)
print(rec.result_summary)
```

Live diagnosis without local tip move:

```python
runner = MicroscopeActionRunner(dry_run=False)
rec = runner.scan_watch_assess_tip(
    start_scan=True,
    setup_scan=True,
    range_A=700,
    points=400,
    min_lines=100,
    move_if_poor=False,
    output_prefix="manual_scan_watch_tip",
)
print(runner.data["last_tip_workflow"]["poor_tip"])
print(runner.data["last_tip_workflow"]["clean_patch"])
```

Live diagnosis with automatic local `ScanX/ScanY` move if the tip is poor:

```python
rec = runner.scan_watch_assess_tip(
    start_scan=True,
    min_lines=100,
    move_if_poor=True,
    stop_before_move=True,
)
```

## Landscape / World-Map Analysis

Create the navigation controller:

```python
nav = LandscapeNavigationController(runner, output_dir=".")
```

Reset world-map memory for a new hyper-space/coarse location:

```python
reset = nav.reset_landscape_memory(
    archive_existing=True,
    reason="new hyper-space location",
)
print(reset)
```

This archives the previous `landscape_memory*.json` files, clears in-memory
visited offsets / rejected regions / work-area history / bump history, and
creates fresh empty memory files. It does not move the microscope.

Mark the current frame as rejected/hazardous from operator observation:

```python
rejected = nav.mark_current_frame_rejected(
    reason="operator observed unexpected huge bumps",
)
print(rejected)
```

Use this when the operator sees a bad area or huge bumps before enough scan
lines exist for robust automatic analysis. It records the current scan frame in
world coordinates as `avoid_for_future_exploration`; it does not move or scan.

Read-only work-area analysis:

```python
report = nav.analyze_current_scan_for_work_area(
    channel=0,
    patch_A=80,
    step_A=20,
    output_prefix="manual_landscape_work_area",
)
print(report["best_candidate"])
```

Read-only full landscape world-map analysis:

```python
report = nav.analyze_current_scan_landscape_map(
    channel=0,
    patch_A=80,
    step_A=20,
    output_prefix="manual_landscape_world_map",
)
print(report["scan_frame"])
print(report["best_flat_candidate"])
print(report["hazards"][:5])
print(report["stepped_regions"][:5])
print(report["abnormal_contrast_lines"][:5])
```

This writes:

- `manual_landscape_world_map_report.json`
- `manual_landscape_world_map.png`
- `manual_landscape_world_map_scan_image.npy`
- appends `landscape_memory.json`
- updates `landscape_memory_latest.json`

Convert one pixel to world coordinates:

```python
world = nav.pixel_to_world_coordinates(px=613, py=658)
print(world)
```

Get scan-frame world footprint:

```python
footprint = nav.scan_frame_world_footprint()
print(footprint["world_corners"])
```

Rank flat areas from an already fetched image:

```python
cands = nav.rank_flat_work_areas(image, pixel_size_A=1.7)
print(cands[0])
```

Find stepped regions from an already fetched image:

```python
regions = nav.detect_major_stepped_regions(image, pixel_size_A=1.7)
print(regions[:5])
```

Find abnormal scan-line contrast from transient tip-state changes:

```python
lines = nav.detect_abnormal_contrast_lines(image)
print(lines[:5])
```

These are not stable landscape steps. Mark them as temporary tip-change,
adsorbate, or picked-up-molecule artifacts. In the current learned example,
the operator pointed out a few abnormal contrast-like lines in a world-X band
around `-1545 A ... -1580 A`; future landscape reports store this class as
`abnormal_contrast_lines`, separate from `stepped_regions` and `hazards`.

Propose neighboring OffsetX/Y scan centers:

```python
targets = nav.propose_offset_search_grid(rings=1)
print(targets)
```

Plan a partially overlapping exploration frame from the latest memory:

```python
plan = nav.propose_overlapping_exploration_frame(
    new_range_A=800,
    overlap_A=300,
    direction="image_right_below",
)
print(plan)
```

Set the exploration frame without starting a scan:

```python
nav.setup_exploration_scan_frame(
    offset_x_A=plan["OffsetX"],
    offset_y_A=plan["OffsetY"],
    range_A=800,
)
```

Run a monitored overlapping exploration scan:

```python
report = nav.explore_overlapping_region(
    new_range_A=800,
    overlap_A=300,
    direction="image_right_below",
    hazard_sigma_threshold=7.0,
    output_prefix="manual_overlap_exploration",
)
print(report["status"])
```

During this workflow the controller starts the scan, watches partial images for
major bump/pit hazards, calls `stopscan()` immediately if a major hazard is
detected, then runs `analyze_current_scan_landscape_map()` and proposes next
exploration offsets.

Check reachability:

```python
print(nav.offset_is_reachable(-2000, 500))
```

Move local tip to the best candidate from the latest analysis:

```python
nav.move_tip_to_best_local_work_area(report)
```

Move scan frame to a new world offset:

```python
nav.move_to_offset(-1500, 900)
```

## GVP Program Read, Load, And Execute

Read current GVP program, including checkbox states:

```python
rec = runner.fetch_current_gvp_program(include_checkboxes=True)
program = runner.data["last_fetched_gvp_program"]["program"]
print(len(program))
```

Save current GVP:

```python
runner.save_current_gvp_program("manual_current_gvp.json")
```

Load a saved GVP file:

```python
runner.load_gvp_program_file("gvp_bias_pulse_default_program.json")
```

Prepare a bias pulse without execution:

```python
runner.load_gvp_bias_pulse(
    pulse_du_V=2.0,
    save_prefix="manual_plus2V_pulse",
)
print(runner.data["last_loaded_gvp"])
```

Prepare a tip dip without execution:

```python
runner.load_gvp_tip_dip(
    dip_depth_A=-5.0,
    contact_bias_V=0.0,
    scan_bias_V=0.1,
    ramp_time_s=30.0,
    save_prefix="manual_minus5A_tip_dip",
)
print(runner.data["last_loaded_gvp"])
```

For this Z-dip template, dU rows are intentionally separated:
`du01=-scan_bias_V` removes the actual scan bias first, `du03=+contact_bias_V`
applies the requested contact bias at contact, `du06=-contact_bias_V` removes
that contact bias, and `du07=+scan_bias_V` restores the scan bias.
The shared `ramp_time_s` parameter sets `dt02` for the indentation ramp and
`dt04` for the pull-out ramp. It is bounded to `1..60 s`; the GUI default is
`30 s`.

Execute the currently loaded GVP:

```python
runner.execute_loaded_gvp(wait_after_execute_s=4.0)
```

The GUI `Execute Loaded GVP And Plot` control waits for GVP completion before
fetching and plotting the VP trace. The default max wait is `300 s`, because
tip-tune GVP programs can run for several minutes when vec 2/4 ramp sections
are long.

Emergency GVP stop:

```python
runner.emergency_stop_gvp()
```

This overwrites the GVP table with all NULL/zero vectors, unchecks all known
conditional vector flags, and immediately executes `DSP_VP_VP_EXECUTE`. It is
not a graceful abort, but it can force a GVP stop path when a long program needs
to be interrupted.

Before executing GVP after a scan, stop and wait for the controller to settle:

```python
settle = runner.stop_scan_wait_for_gvp(stop_wait_s=10.0)
print(settle.result_summary)
runner.execute_loaded_gvp(precheck=False, wait_after_execute_s=8.0)
```

Use the normal precheck when the DSP-ready state clears. If the status remains
set after a deliberate stop/wait but the operator confirms the scan is stopped,
the precheck can be bypassed for the already-loaded GVP while still recording
the status history.

Important GVP rules:

- Numeric `dsp-gvp-*` fields are written with `gxsm.set`.
- Checkbox states are queried by `GETCHECK-dsp-gvp-*`.
- Checkbox states are written by `CHECK-dsp-gvp-*` or `UNCHECK-dsp-gvp-*`.
- Every load writes all numeric vector components, including `0`.
- Every load writes all known checkbox states, including `FALSE`.
- Bias pulse max is `+/-4 V`.
- Tip-dip absolute indentation should not exceed `25 A` without explicit
  operator direction.
- For the current tip-tune template, `FB00=TRUE`, `FB04=FALSE`, `FB08=TRUE`.

## Tip Improvement Activity Controller

Create it:

```python
activity = TipImprovementActivityController(runner, output_dir=".")
```

Collect fresh scan lines and evaluate tip plus step edges:

```python
report = activity.collect_fresh_scan_lines(
    min_lines=40,
    start_scan=True,
    output_prefix="manual_fresh_lines",
)
print(report["tip_quality"]["verdict"])
print(report["step_edges"])
```

Evaluate step edges on an existing image:

```python
edge = activity.evaluate_step_edges(image, pixel_size_A=1.7)
print(edge)
```

Fetch latest GVP/probe-event summary:

```python
summary = activity.fetch_latest_probe_event_summary(
    output_prefix="manual_latest_probe_event",
)
print(summary["labels"])
print(summary.get("zstopo"))
```

Stop scan, load and run a bias pulse, and collect before/after evidence:

```python
report = activity.stop_and_run_bias_pulse_activity(
    pulse_du_V=-3.0,
    dfreq_samples=20,
    wait_after_execute_s=18.0,
    output_prefix="manual_minus3V_activity",
)
print(report["dfreq_before"]["stats"])
print(report["dfreq_after"]["stats"])
```

This is hardware-changing: it stops the scan, loads a GVP program, and executes
it.

## High-Level Tip Improvement Script

Plan only:

```python
runner = MicroscopeActionRunner(dry_run=True)
rec = runner.tip_improvement_script()
print(rec.result_summary)
```

Live assessment with no GVP execution:

```python
runner = MicroscopeActionRunner(dry_run=False)
rec = runner.tip_improvement_script(execute_actions=False)
print(runner.data["last_tip_improvement"])
```

Live automatic GVP sequence:

```python
rec = runner.tip_improvement_script(
    execute_actions=True,
    pulse_sequence_V=(2.0, 3.0),
    dip_sequence_A=(5.0,),
    scan_bias_V=0.1,
    scan_current_nA=0.01,
)
```

This is hardware-changing and should be used only with explicit operator intent.

## Generic Action Plans

You can build simple plans using `ActionStep` dictionaries:

```python
steps = [
    {"kind": "get", "args": ("dsp-fbs-bias",), "collect_as": "bias_V"},
    {"kind": "sleep", "args": (1.0,)},
    {"kind": "get", "args": ("dsp-fbs-mx0-current-set",), "collect_as": "current_nA"},
]
runner.run_plan(steps)
print(runner.data)
```

Supported `ActionStep.kind` values:

- `action`: `gxsm.action(action_name)`
- `set`: `gxsm.set(refname, value)`
- `get`: `gxsm.get(refname)`
- `method`: arbitrary `gxsm` method
- `move`: `gxsm.moveto_scan_xy(x, y)`
- `sleep`: wait
- `ready`: wait for DSP-ready status
- `probe_event`: `gxsm.get_probe_event(channel, nth)`
- `vpdata`: fetch sliced VP data
- `vp_probe`: execute VP action, wait, fetch VP data
- `postprocess`: call a Python processor

Export action history:

```python
runner.export_history("manual_action_history.json")
```

## Small External Script Template

Create a file such as `manual_landscape_check.py`:

```python
#!/usr/bin/python3

from microscope_actions import MicroscopeActionRunner, LandscapeNavigationController

runner = MicroscopeActionRunner(dry_run=False, verbose=0)
nav = LandscapeNavigationController(runner, output_dir=".")

report = nav.analyze_current_scan_landscape_map(
    channel=0,
    output_prefix="manual_landscape_check",
)

print("Scan frame:", report["scan_frame"])
print("Best flat candidate:", report["best_flat_candidate"])
print("Hazards:", len(report["hazards"]))
print("Stepped regions:", len(report["stepped_regions"]))
```

Run it:

```bash
cd /home/percy/VS/Gxsm4/pyremote-tools/local_microscope_copilot
python3 manual_landscape_check.py
```

## GUI Tip Tune Planner Tab

The Gradio `Tip Tune Planner` tab collects the learned tip-improvement workflow
in one place:

- `Analyze Tip And Flat Candidates`: fetches current scan data, runs tip
  quality analysis, dFrequency interpretation, blob/hazard detection, stepped
  region marking, and flat work-site ranking. The plot marks hazards with red
  crosses, molecule-scale candidates with small blue markers, and flat
  candidates with numbered markers. Small blobs with typical `abs(dZ)<3 A` are
  treated as potential molecules, not large hazards.
- `Move Tip To Candidate`: writes local `ScanX`/`ScanY` for the selected flat
  candidate and verifies with the live GVP X/Y monitor. This is Level 1 and
  arm-gated. It refuses to run while the scan status is busy because GXSM
  prohibits local tip moves during scanning. The GUI uses
  `(Sall, Sspm, Sgvp) = gxsm.rtquery("s")` and allows the move only when
  `Sgvp == 0` (stopped or finished scan) or `Sgvp == 5` (completed GVP).
  `Sgvp == 1` means busy. A stopped scan may still have any current-line value
  from `waitscan(False)`; only a changing value over a few seconds indicates
  active scanning.
- `Plan Offset Shift`: proposes a partially overlapping `OffsetX`/`OffsetY`
  world-coordinate shift for finding a cleaner area if the current image has
  too many large blobs.
- `Apply Offset Shift`: Level 1 and arm-gated. It stops the scan first when
  needed, applies `OffsetX/Y`, scan range, points, and optionally starts a new
  scan.
- `Run Tip Tune Planner Loop`: Level 1, arm-gated, and requires typing
  `EXECUTE TIP LOOP`. It repeats a bounded cycle:
  scan/setup, wait until enough lines exist, assess image and dFreq, stop if
  satisfactory, stop/search if too many large hazards or if a large avoid-worthy
  feature is too close to the chosen clear area, otherwise stop scanning, move
  to the best clean candidate, execute a gentle planned GVP pulse or Z-dip, and
  repeat up to the requested max cycles.

Feature classification for Tip Tune Planner:

- `molecule_candidate`: `abs(dZ)<3 A`; counted separately and not used to stop
  or shift offset.
- `large_blob_hazard`: taller feature with estimated diameter at least about
  `40 A`; avoided for clear-area selection and counted for offset-search stop.
- `tall_local_feature`: taller but not broad enough to force offset search;
  avoided locally when selecting a clear site.

Planner progress is appended to:

```text
gui_outputs/tip_planner_progress.jsonl
```

The automatic planner currently uses a deterministic conservative policy:
start with a gentle 2 V bias pulse, escalate to 3 V if needed, then consider a
very gentle 5 A-class Z-dip with 0 V contact bias. The policy is structured so
an LLM planner can later propose the same action schema, but hardware execution
still goes through GUI control level, arm, and typed confirmation gates.

Chat can route common phrases to this same deterministic planner. For example,
`initiate improve tip process` produces a gated explanation. To actually start
from chat, use Control Level 1 or higher, check `Arm Level 1 chat actions`, and
include the exact phrase `EXECUTE TIP LOOP` in the request. The default chat
loop uses max cycles 2, channel 0, 140 evidence lines, 700 A range, 400 points,
max large hazards 8, max `abs(dFreq)` 4 Hz, and no automatic offset shift unless
requested.

Chat also understands direct tip-action phrases:

- `pulse tip` / `tip pulse`: bias-pulse GVP, default `+2 V`, capped at `+/-4 V`.
- `dip tip` / `tune tip` / `tip tune`: Z-dip GVP, default `5 A`, contact bias
  `0 V`, vec 2/4 ramp time `30 s`.
- `fine tune tip`: most gentle Z-dip GVP, default `4 A`, contact bias `0 V`.
  Chat accepts ramp phrases such as `10s ramp`, bounded to `1..60 s`.

These routes are deterministic and still gated. Use `load ...` or `prepare ...`
with Control Level 1+ and chat arm to load only. Include the exact phrase
`EXECUTE GVP` to load and execute from chat.

## Public Function Reference

`MicroscopeActionRunner`:

- `scan_channel_info(channel=None)`
- `topography_z_safety(image)`
- `connect()`
- `run_plan(steps, stop_on_error=True)`
- `run_step(step)`
- `action(action_name, wait_after_s=0.0)`
- `set_parameter(refname, value, wait_after_s=0.0)`
- `get_parameter(refname, collect_as=None)`
- `dFreq()`
- `read_xyz_monitor()`
- `read_rpspmc_monitor()`
- `interpret_dFreq(dFreq_Hz)`

`read_xyz_monitor()` uses `gxsm4process.rt_query_xyz()` and returns
`XYZ_monitor["monitor"]` in controller Volts, not Angstroms. Convert with the
configured piezo scale. For rough live context, compare X/Y monitor Volts with
`OffsetX/Y + ScanX/Y`; this is only an estimate unless monitor zero/origin and
the configured scale are known. In the GUI, the live XYZ gain fields are
external amplifier factors, not min/max display scales. Defaults are X/Y = 12x
and Z = 24x, so effective piezo voltage is `controller_monitor_V * gain`. The
same live dashboard also shows RPSPMC `bias` in V, `current` in nA, `gvp.u` in
V, `pac.ampl` in mV with a +/-10 mV gauge, and `pac.dfreq` in Hz. A 2D panel
shows the raw X/Y controller-voltage position.
- `move_to(x, y, wait_after_s=0.0)`
- `call_gxsm(method_name, *args, ...)`
- `sleep(seconds)`
- `abort_requested()`
- `ready_check(...)`
- `fetch_probe_event(channel=None, nth=-1, collect_as=None)`
- `fetch_vpdata(channel=None, nth=-1, start_idx=600, end_idx=1600, ...)`
- `execute_vp_probe(...)`
- `analyze_zstopo(data=None, plot=False)`
- `post_process(processor, data=None, collect_as=None, **kwargs)`
- `compare_signal_windows(...)`
- `scan_watch_assess_tip(...)`
- `setup_scan_geometry(range_A=700.0, points=400)`
- `set_live_scan_speed(...)`
- `set_live_bias(...)`
- `set_live_current_setpoint(...)`
- `set_scan_slope_leveling(...)`
- `suggest_scan_slope_from_plane(...)`
- `measure_recent_fast_axis_slope(...)`
- `auto_correct_scan_slope_from_recent_lines(...)`
- `plot_fast_axis_slope_profile(...)`
- `fetch_scan_image_to_line(channel=0, y_current=None, chunk_lines=60)`
- `fetch_scan_auxiliary_channels_to_line(...)`
- `plot_scan_image(...)`
- `assess_tip_quality(...)`
- `tip_quality_is_poor(quality)`
- `find_clean_patch(...)`
- `detect_major_bumps(...)`
- `remember_scan_site(settings)`
- `remember_bumps(bumps, settings)`
- `pixel_to_scan_xy(px, py, channel=0)`
- `move_to_scan_xy_fields(scan_x_A, scan_y_A, wait_after_s=2.0)`
- `get_live_tip_position_monitor(...)`
- `move_to_scan_xy_fields_verified(...)`
- `load_gvp_program(program, collect_as=None)`
- `load_gvp_program_file(filename, collect_as=None)`
- `fetch_current_gvp_program(include_checkboxes=True, collect_as=None)`
- `save_current_gvp_program(filename, include_checkboxes=True, collect_as=None)`
- `build_gvp_bias_pulse_program(...)`
- `load_gvp_bias_pulse(...)`
- `build_gvp_tip_dip_program(...)`
- `load_gvp_tip_dip(...)`
- `execute_loaded_gvp(...)`
- `stop_scan_wait_for_gvp(...)`
- `tip_improvement_script(...)`
- `vp_probe_plan(...)`
- `export_history(filename)`

`TipImprovementActivityController`:

- `sample_dfrequency(count=20, delay_s=0.15)`
- `collect_fresh_scan_lines(...)`
- `evaluate_step_edges(...)`
- `fetch_latest_probe_event_summary(...)`
- `stop_and_run_bias_pulse_activity(...)`
- `write_report(report, filename)`

`LandscapeNavigationController`:

- `analyze_current_scan_for_work_area(...)`
- `analyze_current_scan_landscape_map(...)`
- `pixel_to_world_coordinates(px, py, settings=None, channel=0)`
- `scan_frame_world_footprint(settings=None, channel=0)`
- `detect_major_stepped_regions(...)`
- `detect_abnormal_contrast_lines(...)`
- `rank_flat_work_areas(...)`
- `propose_offset_search_grid(...)`
- `propose_overlapping_exploration_frame(...)`
- `setup_exploration_scan_frame(...)`
- `monitor_scan_for_hazards(...)`
- `explore_overlapping_region(...)`
- `move_to_offset(offset_x_A, offset_y_A, wait_after_s=3.0)`
- `offset_is_reachable(offset_x_A, offset_y_A)`
- `move_tip_to_best_local_work_area(analysis=None, wait_after_s=2.0)`
- `move_to_best_work_area(analysis=None, wait_after_s=2.0)`
- `append_landscape_memory(...)`
- `reset_landscape_memory(...)`
- `mark_current_frame_rejected(...)`
- `plot_landscape_candidates(...)`
- `plot_landscape_world_map(...)`
- `write_report(report, filename)`

`Level3ProtectedController`:

- `telemetry()`: read dFreq, interpret it, read watchdog Z voltage proxy via
  `rtquery("z")`, and check `script-control`.
- `set_script_control(enabled)`: set GXSM `script-control` to `1` or `0`.
- `coarse_move(channel, direction, burstcount, period_s=0.5, voltage_V=30.0, ...)`:
  send one THV5 HTTP coarse-motion request; dry-run returns the URL without
  contacting the THV5.
- `emergency_stop_coarse_motion()`: immediately send zero-step, zero-volt THV5
  coarse stop commands. The remembered last coarse axis/direction is stopped
  first, then stop commands are sent across X/Y/Z. This method is intentionally
  not confirmation-gated so the GUI emergency button can act immediately.
- `run_auto_approach(current_nA=0.013, coarse_steps_per_cycle=1, ...)`:
  watchdog-style auto approach based on the operator snippet. It sets the
  current, checks Z range, retracts, applies THV5 Z-down bursts, and aborts on
  `script-control=0`, dFreq limit, or max total steps.

GUI Level 3 actions are intentionally isolated in the `Level 3 Protected` tab.
The tab exposes telemetry, `script-control`, generic THV X/Y/Z coarse moves, a
Z-down shortcut, extra-protected large coarse moves, protected auto approach,
and a red **EMERGENCY STOP COARSE MOTION** button. The emergency stop button is
not gated by Level 3 arm or typed confirmation; it sends zero-step, zero-volt
stop commands immediately.
Normal coarse moves are capped at burstcount 5 and 100 HV V. Large coarse moves
use fixed 0.5 s period, allow burstcounts up to 5000, and require the separate
typed phrase `EXECUTE LEVEL 3 LARGE MOTION`. Any `Z, -1` coarse move is
dangerous fast tip-down motion. Treat all Level 3 actions as live high-risk
operations.
