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

## Coordinate Conventions

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

## Main Files

- `microscope_gradio_gui.py`: Gradio app and chat/intent/action gateway.
- `microscope_gradio_policy.py`: control-level text, parameter aliases, router
  vocabulary.
- `microscope_actions.py`: high-level microscope controller, scan/tip analysis,
  GVP program manipulation, landscape navigation.
- `microscope_gradio_scan.py`: scan plotting and analysis GUI mixin.
- `microscope_gradio_gvp.py`: GVP load/execute/plot GUI mixin.
- `microscope_gradio_tip_planner.py`: tip tune planner GUI mixin.
- `gxsm4process.py`: local bundled GXSM pyremote transport copy.
- `microscope_controller_config.json`: safety/controller config.
- `microscope_gui_config.json`: GUI defaults/display config.
- `local_microscope_copilot_config.json`: local model/service config.

The Gradio Control Level tab includes a read-only `Read GXSM dconf Settings /
Limits` button that reads configured main-window keys, PACPLL/controller keys,
and adjustment-limit keys, then compares known copilot limits against GXSM
adjustment ranges.

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
