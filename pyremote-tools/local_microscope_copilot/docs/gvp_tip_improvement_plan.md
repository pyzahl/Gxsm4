# GVP Tip Improvement Plan

This is a planning document for future autonomous or assisted tip improvement
using GXSM4 General Vector Probe (GVP) programs. It is not an execution script.

The live instrument is an LT-STM/AFM microscope. All GVP tip-improvement actions
are hardware-modifying and require explicit operator approval before execution.

## Goals

1. Improve a suspected double or multiple tip.
2. Preserve the sample and tip as much as possible by starting gently.
3. Leave enough diagnostic evidence after each action to decide whether to stop,
   repeat, or escalate.
4. Build reusable GVP recipes that can later be selected by an autonomous
   decision layer.

## Current Working Primitives

- Read scan geometry and scan data.
- Analyze scan data for possible double-tip signatures.
- Analyze autocorrelation/cross-correlation for longer-range asymmetric or
  diagonal elongated tip signatures. Short-range texture peaks are ignored, but
  a diagonal/elongated repeated-feature signature can indicate a metal tip that
  is usable but not yet ideal.
- Use scan channel conventions explicitly: `Ch=0` is absolute-Z topography in
  Angstrom, `Ch=1` current, `Ch=2` scan-recorded dFrequency, `Ch=4`
  excitation, `Ch=5` oscillation amplitude, `Ch=6` phase, and `Ch=7` channel
  #8 auxiliary. In the current map convention, `Ch=7` contains a copy of the
  last STM overview; in pixel-time mode it may instead contain timing in ms.
- Treat the `Ch=0` offset as irrelevant for image-feature analysis, but monitor
  raw Z range for actuator/safety context. Prefer long-term operation on the
  negative side of 0, ideally within about `0 ... -1000 A`.
- Display scan and map plots with line `0` at the image top, matching the
  operator view.
- Do not over-weight short-range autocorrelation such as `dy=8 px`; treat this
  as likely local noise/texture unless other evidence supports a poor tip.
  Operator visual assessment currently says the tip can look fairly sharp while
  still showing faint shadow/double or possible triple character at a larger,
  poorly determined separation.
- Stop an active scan.
- Choose a quiet/clean patch from scan data.
- Move tip to a clean patch via `ScanX` / `ScanY`.
- Read back GUI values with `gxsm.get`.
- Set low-risk scan/bias parameters when inside agreed safety limits.
- Execute a gentle bias-pulse GVP, restart scanning, then compare fresh
  post-action lines before escalating.

## GVP Action Families

### A. Bias Pulse

Purpose:
- Reform the tip apex electronically without deliberate mechanical contact.
- Useful as a first gentle intervention for doubled or contaminated tips.

Inputs:
- Pulse bias height in Volts.
- Pulse duration in seconds.
- Optional repetition count.
- Optional pre/post bias values.

Typical gentle starting point:
- Start with the smallest pulse expected to have any effect.
- Keep absolute pulse values modest initially.
- Re-image after each pulse or small pulse series.
- For a usable but not perfect metal tip with diagonal/elongated correlation
  features, a light `+2 V` pulse is an appropriate first fine-tune action on a
  clean patch.

Safety notes:
- Bias is `dsp-fbs-bias` in Volts.
- Current setpoint is `dsp-fbs-mx0-current-set` in nA.
- If the current setpoint is above `0.05 nA`, treat tip safety risk as elevated
  and ask before changing live setpoints.
- Operator may specify current in pA; convert pA to nA before passing to GXSM.

Expected observations:
- Good outcome: duplicated lobes/shadows reduce or disappear.
- Bad outcome: tip becomes noisier, loses resolution, produces larger artifacts,
  or scan becomes unstable.
- Neutral/gentle outcome: dFrequency changes only slightly, and the first
  fresh-line scan no longer shows strong long-range double-tip correlation.
  In that case, do not escalate immediately; let more lines accumulate and
  compare again.

Default captured recipe:
- Program backup: `gvp_bias_pulse_default_program.json`
- Recipe metadata: `gvp_bias_pulse_default_recipe.json`
- Execution action: `DSP_VP_VP_EXECUTE`
- The tunable bias-pulse parameter is `du` in Volts.
- Current default pulse step values:
  - `dsp-gvp-du01 = -3.0 V`
  - `dsp-gvp-du03 = +3.0 V`
- Very maximum allowed `du` amplitude is `+/-4 V`.
- Future code must reject or ask for explicit override for any recipe with
  `abs(du) > 4 V`.

### B. Controlled Tip-Surface Contact

Purpose:
- Mechanically reshape the tip apex by making controlled contact with a clean
  patch of surface.
- This may leave an imprint behind, which is acceptable when done on a chosen
  clean sacrificial area.

Inputs:
- Z indentation depth in Angstrom.
- Down-ramp duration in seconds.
- Contact/hold duration, if any.
- Pull-out duration in seconds.
- Contact bias in Volts.
- Optional current limit behavior.

Gentle-to-strong sequence:
- Start near `4 A` indentation depth. This is expected to be almost no contact
  or very gentle contact at nominal `10 pA`, `0.1 V` conditions.
- Escalate only if imaging still shows a bad tip.
- Do not exceed about `25 A` indentation depth without explicit operator
  direction.

Bias during contact:
- Usually set/lower bias to `0 V` during contact.
- A small contact bias may be used for current-induced stronger tip reformation,
  but this depends strongly on the situation and should be operator-guided.

Expected observations:
- A visible imprint may remain at the contact site.
- Good outcome: sharper, single-tip features after re-imaging.
- Bad outcome: larger multi-apex artifacts, unstable feedback, or damaged area
  larger than intended.

## Decision Loop

1. Diagnose tip quality from scan data.
2. Inspect autocorrelation/cross-correlation for diagonal or elongated
   repeated-feature signatures, while ignoring short-range texture/noise peaks.
3. If suspicious, choose a clean patch away from important features.
4. Select the gentlest GVP recipe appropriate to the situation.
5. Confirm live setpoints and safety limits.
6. Execute the GVP recipe only with explicit approval.
7. Re-image the area or a nearby diagnostic patch.
8. Compare before/after tip-quality metrics, dFrequency, and edge/step
   sharpness on enough fresh lines.
9. Stop if improved; otherwise escalate one step at a time.

## Initial Escalation Ladder

1. Bias pulse, very gentle, typically around `+2 V` for fine-tuning a usable
   metal tip with diagonal/elongated correlation features.
2. Bias pulse, slightly larger or longer, typically around `3 V`, only after
   re-imaging shows the diagonal/double signature persists.
3. Very gentle contact, about `4..5 A` indentation, contact bias `0 V`.
4. Moderate contact, increased depth or longer ramp.
5. Stronger contact, only with explicit operator approval, up to about `25 A`.
6. Contact with small nonzero bias, only when operator decides it is warranted.

## Data To Log For Every GVP Attempt

- Timestamp.
- Scan location: `ScanX`, `ScanY`.
- Scan geometry: `RangeX`, `RangeY`, `PointsX`, `PointsY`, `Rotation`.
- Bias before/contact/after.
- Current setpoint in nA.
- GVP program parameters.
- GVP conditional vector checkbox states such as `GETCHECK-dsp-gvp-FB00`,
  stored as `TRUE` / `FALSE`.
- Current line or scan status before action.
- Pre-action tip-quality report.
- Post-action tip-quality report.
- Autocorrelation/cross-correlation peaks, especially any diagonal or elongated
  feature signature.
- dFrequency sample statistics before and after action.
- Chosen clean-patch world coordinates and local `ScanX`/`ScanY`.
- Scan coordinate convention for clean-patch moves:
  - left image edge is `ScanX=-RangeX/2`
  - top/first scan line is `ScanY=-RangeY/2`
  - offsets are non-rotated world coordinates
  - at `Rotation=90 deg`, moving the displayed scan area downward corresponds
    to decreasing world `OffsetX`
- Live GVP monitor values when relevant:
  - `dsp-GVP-XS-MONITOR`: X position in Angstrom.
  - `dsp-GVP-YS-MONITOR`: Y position in Angstrom.
  - `dsp-GVP-ZS-MONITOR`: GVP/Pulse Z offset in Angstrom. Normally zero or
    very small when no GVP/Pulse action is active.
  - `dsp-GVP-U-MONITOR`: bias monitor in Volts.
- Operator approval text or reason.

## Implementation Plan

1. Add a `GVPRecipe` dataclass for named GVP actions.
2. Add recipe builders for:
   - `bias_pulse(height_V, duration_s, repetitions=1)`
   - `controlled_contact(depth_A, down_s, hold_s, up_s, contact_bias_V=0.0)`
3. Add validation:
   - Bias units are Volts.
   - Current units are nA.
   - Convert pA input to nA before writing.
   - Reject or require approval above configured safety thresholds.
4. Add GVP program load/write support using `dsp-gvp-*` rows and conditional
   checkbox states.
5. Add dry-run rendering of the exact `dsp-gvp-*` rows before execution.
6. Add explicit `execute_gvp_recipe(recipe)` with approval gating.
7. Add before/after analysis hooks using the existing tip-quality workflow.

Implemented controller entry points in `microscope_actions.py`:
- `fetch_current_gvp_program(...)` / `save_current_gvp_program(...)`: capture
  numeric vector fields and optional checkbox query states.
- `load_gvp_program(...)`: restores numeric `dsp-gvp-*` entries with `gxsm.set`
  and checkbox entries saved as `GETCHECK-dsp-gvp-*` via `CHECK-dsp-gvp-*` or
  `UNCHECK-dsp-gvp-*` actions.
  All numeric vector components must be written, including `0` values, and all
  known checkbox states must be written, including `FALSE`, because previous
  GUI state persists until the program reaches the terminating row with
  `n/# points = 0`.
- `TipImprovementActivityController`: top-level orchestration class for the
  repeated activity loop: dFrequency sampling, fresh scan-line comparison,
  step-edge evaluation, scan stop, GVP pulse load/execute, probe-event fetch,
  and report logging.
- `LandscapeNavigationController`: top-level navigation/exploration class for
  finding flat work areas on the sample landscape. It ranks low-slope,
  low-roughness, blob-free scan patches, tracks bumps/imprints and visited
  offsets, proposes OffsetX/OffsetY search grids, and can move either to a
  local ScanX/ScanY work site or to a new scan-frame offset when explicitly
  requested. `ScanX`/`ScanY` are local tip moves within the current scan area
  and coordinate system; landscape exploration uses `OffsetX`/`OffsetY`.
  `OffsetX`/`OffsetY` are non-rotated world-coordinate scan-center positions,
  currently reachable over about `+/-2500 A`; the scan view is centered there
  and may be rotated around that center by `Rotation`.
- `load_gvp_bias_pulse(pulse_du_V)`: parameterized bias-pulse load only.
  The default improvement ladder starts around `2..3 V`.
- `load_gvp_tip_dip(dip_depth_A=-5.0, contact_bias_V=0.0)`: parameterized
  controlled dip/contact load only. The current authoritative template is
  `gvp_tip_tune_template_current_latest.json`. Its pattern is:
  row `01` applies the initial bias-removal step, default `du=-0.1 V`;
  row `02` applies the main indentation, default `dz=-5 A`;
  row `03` applies the paired bias-restore/contact-bias step, default
  `du=+0.1 V`; row `04` applies the main pull-out, default `dz=+5 A`.
  Rows `05..09` preserve the template's recovery/tune pattern. `FB00` and
  `FB08` are set checked by the controller for this tip-tune recipe; `FB04`
  must remain unchecked.
- `execute_loaded_gvp()`: executes the currently loaded GVP via
  `DSP_VP_VP_EXECUTE`.
- `scan_watch_assess_tip(...)`: scans, waits for enough lines, assesses image
  and dFrequency, and finds a clean patch when the tip is poor.
- `tip_improvement_script(...)`: high-level orchestration. It plans by default;
  live pulse/dip execution requires a non-dry runner and `execute_actions=True`.

Operational tip-improvement loop:
- scan and assess tip quality,
- if poor, move to a clean blob-free patch,
- try a gentle `2 V` then `3 V` bias-pulse ladder,
- after every action re-scan/reassess,
- expect a blob/imprint at the previous action location; smaller, rounder, more
  symmetric blobs are better when accompanied by improved tip quality,
- if still poor, progress to a controlled dip beginning near `5 A`,
- if no clean patch is large enough, move `OffsetX`/`OffsetY` to search for a
  cleaner area before tuning.

Latest learned fine-tune example:
- Situation: tip was not bad, but not yet a perfect metal tip. Operator pointed
  out diagonal/elongated correlation evidence.
- Action: selected a clean flat patch at about world `X=802 A`, `Y=288 A`,
  stopped the scan, moved locally via `ScanX/ScanY`, loaded and executed a
  light `+2 V` bias-pulse GVP, then restarted scanning.
- Pre-action automated verdict: `suspect_double_or_multiple_tip`.
- Pre-action dFrequency median: about `-7.05 Hz`.
- Post-action fresh-line verdict: `no_strong_double_tip_signature`.
- Post-action dFrequency median: about `-6.72 Hz`, still elevated but slightly
  improved.
- Interpretation: the light pulse likely helped and did not make the global
  dFrequency worse. Do not escalate immediately; allow more post-pulse scan
  lines and compare the diagonal correlation/step sharpness again.

## Open Operator Knowledge Needed

- Exact meanings of the key `dsp-gvp-*` row fields:
  `du`, `dx`, `dy`, `dz`, `da`, `db`, `dam`, `dfm`, `dt`, `n`,
  `xopcd`, `xrchi`, `xcmpv`, `xjmpr`, `nrep`, `pcjr`.
- Which action executes the intended GVP program for these recipes:
  likely `DSP_VP_VP_EXECUTE` or related GVP action, to be confirmed.
- Known-safe starting pulse amplitudes and durations for this tip/sample system.
- Known-safe contact ramp timing and whether Z indentation values should be
  encoded as positive or negative `dz` in the GVP program rows.
