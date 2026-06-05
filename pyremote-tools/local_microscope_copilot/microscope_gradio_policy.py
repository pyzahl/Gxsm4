"""Policy constants and chat-router vocabulary for the GXSM4 Gradio GUI."""

import re


CONTROL_LEVELS = {
    0: "Level 0: Hardware connected, monitoring only",
    1: "Level 1: Basic operation within default safety margins",
    2: "Level 2: Advanced GVP and more aggressive tip tuning",
    3: "Level 3: Extreme GVP, coarse motion, hyper jumps, auto approach",
}

CONTROL_LEVEL_HELP = """
Control levels:
- Level 0: monitoring/read-only. No live-changing actions.
- Level 1: basic operation inside default safety margins: start/stop scan,
  bias <= +/-1 V, current <= 0.05 nA, scan speed in the normal range,
  feedback CP/CI changes, load/execute already-loaded safe GVP.
- Level 2: reserved for advanced GVP and more aggressive tip tuning.
- Level 3: reserved for extreme GVP, coarse motion/hyper jumps, and auto
  approach workflows.
"""

DEFAULT_SYSTEM_PROMPT = """You are a local microscope operator copilot for GXSM4.
Help interpret observations, propose cautious next steps, and explain controller
readouts. The GUI has operator-selected control levels:
Level 0 is read-only monitoring; Level 1 permits basic bounded operations;
Level 2 and Level 3 are reserved for future advanced/extreme operations.
Chat responses from the language model are advisory only and must not claim
hardware actions. A narrow deterministic pre-parser may execute safe Level-1
commands before the model is called, but only through GUI control-level and
arm gates. Bias targets above +/-1 V are outside Level 1 and must be described
as blocked/not available unless a future higher-level workflow is implemented.
For phrases such as "initiate improve tip process", explain that the deterministic
Tip Tune Planner loop can run only through Control Level 1+, the chat arm
checkbox, and the exact confirmation phrase EXECUTE TIP LOOP. Terms like
"pulse tip", "dip tip", "tune tip", and "fine tune tip" map to deterministic
GVP tip-action routes; explain that loading and execution are still gated and
execution requires EXECUTE GVP."""

ACTION_CLAIM_RE = re.compile(
    r"\b("
    r"I (have )?(set|changed|adjusted|started|stopped|executed|loaded)|"
    r"(bias|current|scan|gvp|feedback).{0,40}\b(has been|is now|was set|changed)"
    r")\b",
    re.IGNORECASE,
)

HARDWARE_WORD_RE = re.compile(
    r"\b(bias|current|scan|gvp|feedback|cp|ci|setpoint|start|stop|execute)\b",
    re.IGNORECASE,
)

KNOWN_READBACK_REFS = [
    ("bias_V", "dsp-fbs-bias"),
    ("current_setpoint_nA", "dsp-fbs-mx0-current-set"),
    ("current_limit_nA", "dsp-fbs-mx0-current-level"),
    ("scan_speed_A_per_s", "dsp-fbs-scan-speed-scan"),
    ("move_speed_A_per_s", "dsp-fbs-scan-speed-move"),
    ("feedback_cp_dB", "dsp-fbs-cp"),
    ("feedback_ci_dB", "dsp-fbs-ci"),
    ("range_x_A", "RangeX"),
    ("range_y_A", "RangeY"),
    ("points_x", "PointsX"),
    ("points_y", "PointsY"),
    ("scan_x_A", "ScanX"),
    ("scan_y_A", "ScanY"),
    ("offset_x_A", "OffsetX"),
    ("offset_y_A", "OffsetY"),
    ("rotation_deg", "Rotation"),
    ("scan_slope_x_A_per_A", "dsp-adv-scan-slope-x"),
    ("scan_slope_y_A_per_A", "dsp-adv-scan-slope-y"),
    ("zpos_ref_A", "dsp-adv-dsp-zpos-ref"),
    ("gvp_xs_monitor_A", "dsp-GVP-XS-MONITOR"),
    ("gvp_ys_monitor_A", "dsp-GVP-YS-MONITOR"),
    ("gvp_zs_monitor_A", "dsp-GVP-ZS-MONITOR"),
    ("gvp_u_monitor_V", "dsp-GVP-U-MONITOR"),
]

PARAM_ALIASES = [
    ("bias", "bias_V", "dsp-fbs-bias"),
    ("current setpoint", "current_setpoint_nA", "dsp-fbs-mx0-current-set"),
    ("current", "current_setpoint_nA", "dsp-fbs-mx0-current-set"),
    ("current limit", "current_limit_nA", "dsp-fbs-mx0-current-level"),
    ("scan speed", "scan_speed_A_per_s", "dsp-fbs-scan-speed-scan"),
    ("move speed", "move_speed_A_per_s", "dsp-fbs-scan-speed-move"),
    ("feedback cp", "feedback_cp_dB", "dsp-fbs-cp"),
    ("feedback ci", "feedback_ci_dB", "dsp-fbs-ci"),
    ("cp", "feedback_cp_dB", "dsp-fbs-cp"),
    ("ci", "feedback_ci_dB", "dsp-fbs-ci"),
    ("scan size", "scan_geometry", None),
    ("scan range", "scan_geometry", None),
    ("range", "scan_geometry", None),
    ("points", "scan_geometry", None),
    ("scan geometry", "scan_geometry", None),
    ("rotation", "rotation_deg", "Rotation"),
    ("offset", "scan_geometry", None),
    ("scan x", "scan_x_A", "ScanX"),
    ("scan y", "scan_y_A", "ScanY"),
    ("slope x", "scan_slope_x_A_per_A", "dsp-adv-scan-slope-x"),
    ("slope y", "scan_slope_y_A_per_A", "dsp-adv-scan-slope-y"),
    ("scan slope x", "scan_slope_x_A_per_A", "dsp-adv-scan-slope-x"),
    ("scan slope y", "scan_slope_y_A_per_A", "dsp-adv-scan-slope-y"),
    ("zpos ref", "zpos_ref_A", "dsp-adv-dsp-zpos-ref"),
    ("gvp x", "gvp_xs_monitor_A", "dsp-GVP-XS-MONITOR"),
    ("gvp y", "gvp_ys_monitor_A", "dsp-GVP-YS-MONITOR"),
    ("gvp z", "gvp_zs_monitor_A", "dsp-GVP-ZS-MONITOR"),
    ("gvp bias", "gvp_u_monitor_V", "dsp-GVP-U-MONITOR"),
]

PARAM_SPECS = {
    "bias": {
        "label": "bias_V",
        "refname": "dsp-fbs-bias",
        "unit": "V",
        "aliases": ("bias", "sample bias", "tunnel bias", "voltage"),
        "write": "bias",
    },
    "current_setpoint": {
        "label": "current_setpoint_nA",
        "refname": "dsp-fbs-mx0-current-set",
        "unit": "nA",
        "aliases": (
            "current setpoint",
            "current set point",
            "setpoint current",
            "tunnel current",
            "current",
        ),
        "write": "current",
    },
    "current_limit": {
        "label": "current_limit_nA",
        "refname": "dsp-fbs-mx0-current-level",
        "unit": "nA",
        "aliases": ("current limit", "z mode current limit"),
        "write": None,
    },
    "scan_speed": {
        "label": "scan_speed_A_per_s",
        "refname": "dsp-fbs-scan-speed-scan",
        "unit": "A/s",
        "aliases": ("scan speed", "scan velocity", "speed"),
        "write": "scan_speed",
    },
    "move_speed": {
        "label": "move_speed_A_per_s",
        "refname": "dsp-fbs-scan-speed-move",
        "unit": "A/s",
        "aliases": ("move speed", "tip move speed", "offset move speed"),
        "write": None,
    },
    "feedback_cp": {
        "label": "feedback_cp_dB",
        "refname": "dsp-fbs-cp",
        "unit": "dB",
        "aliases": ("feedback cp", "cp"),
        "write": "feedback_cp",
    },
    "feedback_ci": {
        "label": "feedback_ci_dB",
        "refname": "dsp-fbs-ci",
        "unit": "dB",
        "aliases": ("feedback ci", "ci"),
        "write": "feedback_ci",
    },
    "rotation": {
        "label": "rotation_deg",
        "refname": "Rotation",
        "unit": "deg",
        "aliases": ("rotation", "scan rotation", "rotate", "scan angle", "angle"),
        "write": "rotation",
    },
    "offset_x": {
        "label": "offset_x_A",
        "refname": "OffsetX",
        "unit": "A",
        "aliases": ("offset x", "offsetx", "scan offset x", "world offset x"),
        "write": "offset_x",
    },
    "offset_y": {
        "label": "offset_y_A",
        "refname": "OffsetY",
        "unit": "A",
        "aliases": ("offset y", "offsety", "scan offset y", "world offset y"),
        "write": "offset_y",
    },
    "slope_x": {
        "label": "scan_slope_x_A_per_A",
        "refname": "dsp-adv-scan-slope-x",
        "unit": "A/A",
        "aliases": ("scan slope x", "slope x", "x slope"),
        "write": "slope_x",
    },
    "slope_y": {
        "label": "scan_slope_y_A_per_A",
        "refname": "dsp-adv-scan-slope-y",
        "unit": "A/A",
        "aliases": ("scan slope y", "slope y", "y slope"),
        "write": "slope_y",
    },
    "gvp_x": {
        "label": "gvp_xs_monitor_A",
        "refname": "dsp-GVP-XS-MONITOR",
        "unit": "A",
        "aliases": ("gvp x", "gvp xs", "tip x monitor"),
        "write": None,
    },
    "gvp_y": {
        "label": "gvp_ys_monitor_A",
        "refname": "dsp-GVP-YS-MONITOR",
        "unit": "A",
        "aliases": ("gvp y", "gvp ys", "tip y monitor"),
        "write": None,
    },
    "gvp_z": {
        "label": "gvp_zs_monitor_A",
        "refname": "dsp-GVP-ZS-MONITOR",
        "unit": "A",
        "aliases": ("gvp z", "gvp zs", "tip z monitor"),
        "write": None,
    },
    "gvp_u": {
        "label": "gvp_u_monitor_V",
        "refname": "dsp-GVP-U-MONITOR",
        "unit": "V",
        "aliases": ("gvp u", "gvp bias", "gvp voltage"),
        "write": None,
    },
}

CHAT_ROUTER_HELP = """
Deterministic chat router:
- Reads known GXSM parameters by alias, including bias, current setpoint,
  scan speed, feedback CP/CI, scan geometry, slopes, and GVP monitors.
- Executes Level-1 scan start/stop and bounded parameter changes only when
  Control Level is 1+ and the chat arm checkbox is checked.
- Executes OffsetX and OffsetY as individually named world-coordinate
  Angstrom targets, for example `set offset x 100 A` or
  `set offset x 10 nm and offset y -20 nm`.
- Executes relative image/frame shifts such as `shift image 100 A left`;
  the local image direction is rotated into world OffsetX/Y using Rotation.
- Understands relative requests like "increase bias by 0.05 V", "make scan
  speed 20% faster", and context pronouns like "set it to ..." after a read.
- Routes tip/scan analysis to deterministic image analysis rather than asking
  the language model to invent a report.
- Routes phrases like "initiate improve tip process" to the deterministic
  Tip Tune Planner loop. It starts only with Control Level 1+, chat arm checked,
  and the exact phrase `EXECUTE TIP LOOP` in the request.
- Routes "pulse tip" / "tip pulse" to the bias-pulse GVP recipe and
  "dip tip" / "tune tip" / "fine tune tip" to the Z-dip GVP recipe. "Fine
  tune" means the most gentle default dip.
"""
