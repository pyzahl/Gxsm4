# GXSM4 PyRemote Tools Codebase Overview

## Project Purpose
Python remote control toolkit for GXSM4 (Scanning Microscopy Software v4). Enables external Python script access to control and monitor SPM experiments through shared memory (SHM) and inter-process communication (IPC).

## Core Module
**gxsm4process.py** (v1.0.1):
- PySHM interface for GXSM4 process communication
- RPSPMC (signal processor) monitoring
- SIGUSR1-based IPC
- 70+ auto-generated method wrappers

## Operator Console Manual

Manual usage examples for the higher-level controller classes are documented in
`microscope_actions_console_manual.md`. It covers importing from an external
Python console, dry-run vs. live GXSM4 use, read-only diagnostics, scan/tip
analysis, landscape/world-map memory, GVP load/execute workflows, and the public
function reference for `MicroscopeActionRunner`,
`TipImprovementActivityController`, and `LandscapeNavigationController`.

## Recent Modifications (May 27, 2026)

### plot-extern.py (Refactored)
- **Was**: Direct matplotlib plotting (data lost after display)
- **Now**: Returns numpy data dictionary
  ```python
  data = fetch_vpdata_analysis(verbose=True, plot=False)
  # Returns: {'vpdata', 'vpunits', 'xy', 'labels', 'columns'}
  ```
- Optional plotting, error handling for GXSM4 connection

### analyze-vpdata.py (UPDATED)
**analyze_zpdata()** - Comprehensive ZS-Topo signal analysis:
- **Topography Statistics**: min/max/mean/std/range/total_change
- **Rate of Change**: dZ/dt analysis with min/max/mean rates
- **Step Detection**: Finds discrete jumps using 2nd derivative
  - Adaptive thresholding (2.5σ)
  - Step indices and magnitudes
- **Progression Analysis**: 4-phase breakdown (0-25%, 25-50%, 50-75%, 75-100%)
  - Per-phase statistics
  - Mean/std/range per phase
- **Trend Analysis**:
  - Linear fit (slope and total change)
  - Quadratic fit (curvature/acceleration detection)
- **Correlations**: vs all other signals
- **Advanced Plotting** (3-plot set):
  - ZS-Topo with linear & quadratic trend overlays + derivative + acceleration
  - 4-phase progression histograms with statistics
  - All other signals vs ZS-Topo (as x-axis) with correlations

### analyze-zstopo-progression.py (NEW - May 27)
Advanced progression analysis script:
- **Phase Transitions**: Start→Early→Late→End analysis
- **Trend Characterization**: Increasing/Decreasing/Stable classification
- **Stability Assessment**: Stable vs. active regions (using 0.5σ threshold)
- **Step Statistics**: Size distribution, categorization (large/medium/small)
- **Curvature Implications**: Acceleration vs. deceleration behavior
- Complete example workflow demonstrating all features

### example-zstopo-analysis.py
Companion example showing custom data analysis patterns

## Key Analysis Capabilities

**What the updated scripts detect:**
1. Start-to-end topography changes with quantitative metrics
2. Discrete steps/jumps vs. continuous changes
3. Overall trend direction (linear) with curvature (acceleration)
4. Which measurement phases are stable vs. active
5. Step characteristics (size distribution, count, magnitude)
6. Signal correlations during topography changes

## Live GXSM4 Safety Notes

The live system is an LT-STM/AFM microscope. Treat all hardware-affecting
commands as approval-required unless explicitly cleared by the operator.

Basic SPM scan geometry and frame parameters:
- `RangeX`, `RangeY`: scan size/range
- `PointsX`, `PointsY`: scan pixel dimensions
- `StepsX`, `StepsY`: scan stepping parameters
- `OffsetX`, `OffsetY`: scan frame/area center in the current world coordinate
  system. This coordinate system is always non-rotated. At the current setting
  about `+/-2500 A` is reachable. The scan view is centered at this position.
- `Rotation`: scan frame rotation
- `ScanX`, `ScanY`: move the tip to a specific location within the current
  scan area and current scan coordinate system. These are local tip-position
  controls, not landscape/coarse-area navigation.
  In the image-to-scan convention, the left image edge is
  `ScanX=-RangeX/2`, the right edge is `ScanX=+RangeX/2`, the top/first scan
  line is `ScanY=+RangeY/2`, and the bottom line is `ScanY=-RangeY/2`.
  Line numbers count downward from top=0, while physical scan Y is positive
  upward.
- `OffsetX`, `OffsetY`: move the scan frame/landscape area in non-rotated
  world coordinates. `Rotation` rotates the scan view around this center.
  At `Rotation=90 deg`, moving the displayed scan area downward corresponds to
  decreasing world `OffsetX`.

Scan data channel conventions:
- `Ch=0`: STM/topography image data. This is the primary image channel used for
  doubled-feature and clean-area analysis. It is absolute Z in Angstrom. The
  absolute offset has no image-feature meaning, but the raw Z range has physical
  actuator limits. Ideally stay within about `0 ... -1000 A`; roughly
  `+/-2000 A` is possible. Negative Z is the safer side for long-term action
  because in a power-loss case, going toward `0` lifts the tip. Landing offset
  can later be adjusted by coarse Z motion with the tip fully lifted, but that
  is not very predictable and is deferred.
- `Ch=1`: actual tunneling current in nA, usually current setpoint plus noise.
- `Ch=2`: dFrequency signal in Hz recorded during the scan.
- `Ch=4`: excitation in mV.
- `Ch=5`: oscillation amplitude reading in mV.
- `Ch=6`: phase signal.
- `Ch=7`: channel #8 auxiliary. In the current overview/map convention this
  contains a copy of the last STM overview map. If pixel-time mode is enabled,
  it may instead contain actual pixel time since scan start in ms.
- Display convention: scan/map plots should show line `0` at the image top to
  match the operator's GXSM view.
- Tip-quality interpretation note: short-range autocorrelation peaks around
  `dy=8 px` are likely local texture/noise and should not by themselves mark a
  double or multiple tip. A visually sharp tip may still show faint shadow or
  possible double/triple character at larger, poorly determined separation,
  including separations outside the current field of view.
- Landscape-analysis artifact note: a few abnormal contrast-like scan lines,
  for example the operator-noted world-X band around `-1545 A ... -1580 A` in
  the current map, can be caused by a temporary tip change, adsorbate, or
  picked-up molecule. These should be marked as `abnormal_contrast_lines`, not
  treated as stable physical steps or hazards.

Scan and feedback-control GUI IDs:
- `dsp-fbs-bias`: bias in Volts
- `dsp-fbs-mx0-current-set`: current setpoint
- `dsp-fbs-mx0-current-level`: special current limit in constant-height/Z mode,
  used together with `dsp-adv-dsp-zpos-ref`
- `dsp-adv-dsp-zpos-ref`: Z reference for constant-height/Z workflows
- `dsp-fbs-cp`, `dsp-fbs-ci`: feedback CP/CI values on dB scale; about `-90`
  is the default range noted by the operator
- `dsp-fbs-scan-speed-scan`: scan speed in A/s
- `dsp-fbs-scan-speed-move`: tip move speed for offset changes in A/s
- `dsp-adv-scan-slope-x`, `dsp-adv-scan-slope-y`: scan leveling slopes

Global tip-shape readback:
- dFrequency is read in Hz via `gxsm.rtquery("f")[0]`.
- It provides global tip shape/radius context related to the overall
  capacitive/electrostatic force.
- Rule of thumb from operator for normal attractive-mode imaging:
  - `0 Hz`: tip is not near the surface or the dFreq controller is disabled
    (not even noise)
  - `0..-2 Hz`: perfect/excellent
  - `0..-4 Hz`: OK
  - `< -10 Hz`: may work, but generally indicates double/multiple tip
    character to some extent
  - positive dFrequency occurs in strongly repulsive or special scan modes and
    will need separate interpretation later

Operator safety rule:
- Bias changes with absolute target values `<= 1 V` may be applied without a
  separate safety confirmation, but the current setpoint must still be checked.
- Current values are in nA when read/written through GXSM. If the operator gives
  current in pA, convert to nA before passing it to GXSM.
- If `dsp-fbs-mx0-current-set` is larger than `0.05 nA`, treat tip safety as
  elevated risk and ask before changing related live setpoints.

GVP conditional vector parameters:
- Numeric GVP row fields are GUI refs like `dsp-gvp-du00` and are read/written
  with `gxsm.get` / `gxsm.set`.
- Conditional vector checkboxes are queried through actions such as
  `GETCHECK-dsp-gvp-FB00`, returning the string `TRUE` or `FALSE`.
- Restore checkbox state with `CHECK-dsp-gvp-...` for `TRUE` and
  `UNCHECK-dsp-gvp-...` for `FALSE`.
