#!/usr/bin/python3
"""
Gradio GUI skeleton for the local GXSM4 microscope copilot.

The GUI keeps Ollama chat advisory by default. A small set of deterministic
Level-1 chat commands may be routed through the same explicit safety gates as
the button controls when the operator arms chat actions.
"""

import argparse
import json
import os
from pathlib import Path
import re
import sys
import time
import traceback

os.environ.setdefault("MPLCONFIGDIR", "/tmp/matplotlib-gxsm4-copilot")

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import requests

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(1, str(PROJECT_ROOT))

from microscope_actions import (
    LandscapeNavigationController,
    MicroscopeActionRunner,
    TipImprovementActivityController,
)


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
as blocked/not available unless a future higher-level workflow is implemented."""

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


def load_config(path):
    with open(path, "r") as file:
        return json.load(file)


class MicroscopeGradioBackend:
    def __init__(self, config, live=False, output_dir=None):
        self.config = dict(config)
        self.live = bool(live)
        self.output_dir = Path(output_dir or SCRIPT_DIR / "gui_outputs")
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.runner = MicroscopeActionRunner(
            dry_run=not self.live,
            verbose=0,
        )
        self.nav = LandscapeNavigationController(
            self.runner,
            output_dir=str(self.output_dir),
        )
        self.activity = TipImprovementActivityController(
            self.runner,
            output_dir=str(self.output_dir),
        )
        self.control_level = 0
        self.chat_messages = [{"role": "system", "content": DEFAULT_SYSTEM_PROMPT}]

    def status_text(self):
        mode = "LIVE GXSM4" if self.live else "DRY RUN"
        model = self.config.get("model", "qwen3:4b")
        host = self.config.get("ollama_host", "http://127.0.0.1:11434")
        return "Mode: {} | Control: {} | Ollama: {} | Model: {} | Output: {}".format(
            mode,
            CONTROL_LEVELS.get(self.control_level, self.control_level),
            host,
            model,
            self.output_dir,
        )

    def jsonable(self, value):
        return self.runner._jsonable(value)

    def chat(self, message, history, chat_arm=False):
        if not message:
            return "", history
        direct = self.direct_safety_reply(message, chat_arm=chat_arm)
        if direct is not None:
            history = list(history or [])
            history.append({"role": "user", "content": message})
            history.append({"role": "assistant", "content": direct})
            return "", history
        level_note = "Current GUI control setting: {}. {}".format(
            CONTROL_LEVELS.get(self.control_level, self.control_level),
            "Live GUI mode is enabled." if self.live else "GUI is in dry-run mode.",
        )
        self.chat_messages.append(
            {"role": "user", "content": level_note + "\n\nUser: " + message}
        )
        try:
            reply = requests.post(
                self.config.get("ollama_host", "http://127.0.0.1:11434")
                + "/api/chat",
                json={
                    "model": self.config.get("model", "qwen3:4b"),
                    "messages": self.chat_messages,
                    "stream": False,
                    "options": {
                        "temperature": float(self.config.get("temperature", 0.2)),
                    },
                },
                timeout=180,
            )
            reply.raise_for_status()
            text = reply.json()["message"].get("content", "")
        except requests.ConnectionError:
            text = "Could not connect to Ollama. Start it with: `ollama serve`."
        except Exception as exc:
            text = "Chat error: {}".format(exc)
        text = self.enforce_chat_action_boundary(text)
        self.chat_messages.append({"role": "assistant", "content": text})
        history = list(history or [])
        history.append({"role": "user", "content": message})
        history.append({"role": "assistant", "content": text})
        return "", history

    def enforce_chat_action_boundary(self, text):
        if ACTION_CLAIM_RE.search(text) and HARDWARE_WORD_RE.search(text):
            return (
                "SAFETY CORRECTION: No microscope hardware action was executed "
                "from this chat response. Chat is advisory only; use the GUI "
                "control-level selector, arm checkbox, and explicit action "
                "buttons to make live changes.\n\n"
                + text
        )
        return text

    def direct_safety_reply(self, message, chat_arm=False):
        text = str(message)
        action_reply = self.try_chat_scan_action(text, chat_arm=chat_arm)
        if action_reply is not None:
            return action_reply
        if re.search(
            r"\b(set|change|adjust|make|configure)\b",
            text,
            re.IGNORECASE,
        ):
            write_reply = self.try_chat_parameter_write(text, chat_arm=chat_arm)
            if write_reply is not None:
                return write_reply
        read_reply = self.try_chat_parameter_read(text)
        if read_reply is not None:
            return read_reply
        return None

    def try_chat_parameter_read(self, text):
        if not re.search(
            r"\b(read|show|get|report|status|settings|what('?s| is)?|current)\b",
            text,
            re.IGNORECASE,
        ):
            return None
        text_l = text.lower()
        read_all = bool(
            re.search(r"\b(all|status|settings|parameters|everything)\b", text_l)
        )
        matches = []
        for alias, label, refname in PARAM_ALIASES:
            if alias in text_l and refname:
                matches.append((label, refname))
        if "scan" in text_l and any(
            word in text_l for word in ("geometry", "size", "range", "points", "steps")
        ):
            read_all = True
        if read_all:
            matches = list(KNOWN_READBACK_REFS)
        if not matches:
            return None
        return self.format_parameter_readback(matches)

    def try_chat_scan_action(self, text, chat_arm=False):
        text_l = text.lower()
        if "scan" not in text_l:
            return None
        if re.search(r"\b(start|begin|run)\b", text_l):
            return self.execute_chat_level1_action(
                "start scan",
                "startscan",
                chat_arm,
                lambda: self.start_scan(arm=True),
            )
        if re.search(r"\b(stop|halt|abort|end)\b", text_l):
            return self.execute_chat_level1_action(
                "stop scan",
                "stopscan",
                chat_arm,
                lambda: self.stop_scan(arm=True),
            )
        return None

    def format_parameter_readback(self, items):
        seen = set()
        result = {}
        records = {}
        try:
            for label, refname in items:
                if label in seen:
                    continue
                seen.add(label)
                rec = self.runner.get_parameter(refname)
                result[label] = rec.result_summary
                records[label] = self.jsonable(rec.__dict__)
            self.add_scan_spacing(result)
            lines = []
            for label in sorted(result):
                lines.append("{}: {}".format(label, result[label]))
            return (
                "Current GXSM readback:\n\n"
                + "\n".join(lines)
                + "\n\nScan StepsX/Y are informational pixel spacings computed "
                "as Range/(Points-1), in Angstrom.\n\n```json\n{}\n```"
            ).format(json.dumps({"values": result, "records": records}, indent=2))
        except Exception as exc:
            return (
                "Parameter readback failed: {}\n\n```text\n{}\n```"
            ).format(str(exc), traceback.format_exc())

    def add_scan_spacing(self, values):
        try:
            rx = values.get("range_x_A")
            ry = values.get("range_y_A")
            px = values.get("points_x")
            py = values.get("points_y")
            if rx is not None and px is not None and float(px) > 1:
                values["steps_x_A_informative"] = float(rx) / (float(px) - 1.0)
            if ry is not None and py is not None and float(py) > 1:
                values["steps_y_A_informative"] = float(ry) / (float(py) - 1.0)
        except Exception:
            pass

    def try_chat_parameter_write(self, text, chat_arm=False):
        if not re.search(r"\b(set|change|adjust|make|configure)\b", text, re.IGNORECASE):
            return None
        text_l = text.lower()
        if "bias" in text_l:
            target = self.last_number(text)
            if target is None:
                return None
            if abs(target) > self.safety_limits()["level1_max_abs_bias_V"]:
                return (
                    "I did not change the bias. A target of {:.3g} V is outside "
                    "the Level 1 automatic limit of +/-1 V."
                ).format(target)
            return self.execute_chat_level1_action(
                "bias",
                target,
                chat_arm,
                lambda: self.set_bias(target, arm=True),
            )
        if "current" in text_l and ("setpoint" in text_l or "set point" in text_l):
            parsed = self.parse_current_request(text)
            if parsed is None:
                return None
            value, unit = parsed
            current_nA = float(value) / 1000.0 if str(unit).lower() == "pa" else float(value)
            if current_nA > self.safety_limits()["level1_max_current_nA"]:
                return (
                    "I did not change the current setpoint. The requested value "
                    "is {:.6g} nA, above the Level 1 automatic limit of {:.6g} nA."
                ).format(current_nA, self.safety_limits()["level1_max_current_nA"])
            return self.execute_chat_level1_action(
                "current setpoint",
                "{} {}".format(value, unit),
                chat_arm,
                lambda: self.set_current(value, unit, arm=True),
            )
        if "scan speed" in text_l:
            target = self.last_number(text)
            if target is None:
                return None
            range_A = self.read_float_parameter("RangeX", fallback=700.0)
            return self.execute_chat_level1_action(
                "scan speed",
                "{} A/s".format(target),
                chat_arm,
                lambda: self.set_scan_speed(target, range_A, arm=True),
            )
        if any(word in text_l for word in ("scan size", "scan range", "scan geometry", "points")):
            parsed = self.parse_scan_geometry_request(text)
            if parsed is None:
                return None
            range_A, points = parsed
            return self.execute_chat_level1_action(
                "scan geometry",
                "{} A, {} points".format(range_A, points),
                chat_arm,
                lambda: self.set_scan_geometry(range_A, points, arm=True),
            )
        if "slope x" in text_l or "scan slope x" in text_l:
            target = self.last_number(text)
            if target is None:
                return None
            if abs(target) > self.safety_limits()["level1_max_abs_scan_slope"]:
                return "I did not change scan slope x. Level 1 allows abs(slope) <= 0.1."
            return self.execute_chat_level1_action(
                "scan slope x",
                target,
                chat_arm,
                lambda: self.set_scan_slope(slope_x=target, arm=True),
            )
        if "slope y" in text_l or "scan slope y" in text_l:
            target = self.last_number(text)
            if target is None:
                return None
            if abs(target) > self.safety_limits()["level1_max_abs_scan_slope"]:
                return "I did not change scan slope y. Level 1 allows abs(slope) <= 0.1."
            return self.execute_chat_level1_action(
                "scan slope y",
                target,
                chat_arm,
                lambda: self.set_scan_slope(slope_y=target, arm=True),
            )
        cp_match = re.search(r"\bcp\b[^-+\d]*([-+]?\d+(?:\.\d+)?)", text_l)
        ci_match = re.search(r"\bci\b[^-+\d]*([-+]?\d+(?:\.\d+)?)", text_l)
        if cp_match and ci_match:
            cp = float(cp_match.group(1))
            ci = float(ci_match.group(1))
            return self.execute_chat_level1_action(
                "feedback CP/CI",
                "CP {}, CI {}".format(cp, ci),
                chat_arm,
                lambda: self.set_feedback_cp_ci(cp, ci, arm=True),
            )
        return None

    def execute_chat_level1_action(self, action_name, target, chat_arm, callback):
        is_scan_action = action_name in ("start scan", "stop scan")
        blocked_phrase = (
            "I did not execute {}".format(action_name)
            if is_scan_action
            else "I did not change {}".format(action_name)
        )
        success_prefix = (
            "{} executed".format(action_name.capitalize())
            if is_scan_action
            else "{} change executed".format(action_name.capitalize())
        )
        if self.control_level < 1:
            return (
                "{}. Chat Level 1 actions require Control "
                "Level 1 or higher, plus the chat arm checkbox."
            ).format(blocked_phrase)
        if not chat_arm:
            return (
                "{}. To allow this Level 1 chat action, check "
                "'Arm Level 1 chat actions' in the Chat tab and submit the "
                "request again."
            ).format(blocked_phrase)
        result = callback()
        if isinstance(result, dict) and result.get("error"):
            return (
                "I tried to execute {} ({}) but GXSM returned an error:\n\n"
                "```json\n{}\n```"
            ).format(action_name, target, json.dumps(result, indent=2))
        return (
            "{} through the Level 1 chat action gate: {}.\n\n"
            "```json\n{}\n```"
        ).format(success_prefix, target, json.dumps(result, indent=2))

    def last_number(self, text):
        nums = re.findall(r"[-+]?\d+(?:\.\d+)?", str(text))
        return None if not nums else float(nums[-1])

    def parse_current_request(self, text):
        match = re.search(
            r"([-+]?\d+(?:\.\d+)?)\s*(pA|nA)\b",
            text,
            re.IGNORECASE,
        )
        if match:
            return float(match.group(1)), match.group(2)
        value = self.last_number(text)
        return None if value is None else (value, "nA")

    def parse_scan_geometry_request(self, text):
        text_l = text.lower()
        range_match = re.search(
            r"([-+]?\d+(?:\.\d+)?)\s*(?:a|ang|angstrom|angstroms)\b",
            text_l,
        )
        points_match = re.search(r"(\d+)\s*(?:x\s*\d+\s*)?points?\b", text_l)
        if not points_match:
            points_match = re.search(r"points?\s*(?:x|=|to)?\s*(\d+)", text_l)
        range_A = (
            float(range_match.group(1))
            if range_match
            else self.read_float_parameter("RangeX", fallback=700.0)
        )
        points = (
            int(points_match.group(1))
            if points_match
            else int(self.read_float_parameter("PointsX", fallback=400))
        )
        return range_A, points

    def read_float_parameter(self, refname, fallback):
        try:
            rec = self.runner.get_parameter(refname)
            if rec.result_summary is None:
                return float(fallback)
            return float(rec.result_summary)
        except Exception:
            return float(fallback)

    def set_control_level(self, choice):
        text = str(choice)
        try:
            level = int(text.split(":", 1)[0].replace("Level", "").strip())
        except Exception:
            level = 0
        self.control_level = max(0, min(3, level))
        return self.status_text(), {
            "control_level": self.control_level,
            "description": CONTROL_LEVELS[self.control_level],
            "help": CONTROL_LEVEL_HELP.strip(),
        }

    def require_control_level(self, required, action_name):
        if self.control_level < required:
            return {
                "blocked": action_name,
                "required_level": required,
                "current_level": self.control_level,
                "reason": "Increase GUI control level before running this live-changing action.",
            }
        return None

    def require_arm(self, arm, action_name):
        if not arm:
            return {"cancelled": "Check the arm box to {}.".format(action_name)}
        return None

    def safety_limits(self):
        return {
            "level1_max_abs_bias_V": 1.0,
            "level1_max_current_nA": 0.05,
            "level1_scan_speed_multiplier_range": [0.5, 1.5],
            "level1_feedback_cp_ci_range_dB": [-120.0, -30.0],
            "level1_max_abs_gvp_bias_pulse_du_V": 4.0,
            "level1_scan_range_A": [10.0, 2000.0],
            "level1_scan_points": [16, 1024],
            "level1_max_abs_scan_slope": 0.1,
            "gvp_execute_extra_confirmation": "EXECUTE GVP",
        }

    def read_monitors(self):
        try:
            rec = self.runner.get_live_tip_position_monitor()
            data = self.runner.data.get("live_tip_position") or rec.__dict__
            return self.jsonable(data), self.status_text()
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}, self.status_text()

    def sample_dfrequency(self, count, delay_s):
        try:
            report = self.activity.sample_dfrequency(
                count=int(count),
                delay_s=float(delay_s),
            )
            return self.jsonable(report)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def scan_extent_A(self, image, meta, channel):
        """Return imshow extent in local scan Angstroms, preserving aspect."""
        ny, nx = image.shape
        try:
            settings = self.runner._scan_settings(int(channel))
            range_x = float(settings.get("RangeX"))
            range_y = float(settings.get("RangeY"))
            points_y = float(settings.get("PointsY") or meta.get("dimensions", [nx, ny])[1])
        except Exception:
            dims = meta.get("dimensions", [nx, ny])
            range_x = float(dims[0] or nx)
            range_y = float(dims[1] or ny)
            points_y = float(dims[1] or ny)
        fetched_y = float(meta.get("fetched_y_count", ny) or ny)
        x_left = -0.5 * range_x
        x_right = 0.5 * range_x
        y_top = -0.5 * range_y
        y_bottom = y_top + range_y * fetched_y / max(points_y, 1.0)
        return (x_left, x_right, y_bottom, y_top)

    def pixel_to_local_A(self, px, py, image, extent):
        ny, nx = image.shape
        x_left, x_right, y_bottom, y_top = extent
        x_A = x_left + (float(px) + 0.5) * (x_right - x_left) / max(nx, 1)
        y_A = y_top + (float(py) + 0.5) * (y_bottom - y_top) / max(ny, 1)
        return x_A, y_A

    def plot_scan_image_A(
        self,
        image,
        meta,
        channel,
        title,
        colorbar_label,
        quality=None,
        landscape=None,
    ):
        fig, ax = plt.subplots(figsize=(6, 5), constrained_layout=True)
        extent = self.scan_extent_A(image, meta, channel)
        im = ax.imshow(
            image,
            origin="upper",
            extent=extent,
            cmap="viridis",
            aspect="equal",
        )
        ax.set_title(title)
        ax.set_xlabel("Scan X (A)")
        ax.set_ylabel("Scan Y (A), line 0 at top")
        fig.colorbar(im, ax=ax, label=colorbar_label)
        if quality or landscape:
            self.annotate_analysis_plot(ax, image, extent, quality, landscape)
        return fig, extent

    def annotate_analysis_plot(self, ax, image, extent, quality=None, landscape=None):
        quality = quality or {}
        landscape = landscape or {}

        hazards = (
            landscape.get("hazards")
            or landscape.get("major_bumps")
            or []
        )
        for hazard in hazards[:12]:
            if "px" not in hazard or "py" not in hazard:
                continue
            x_A, y_A = self.pixel_to_local_A(hazard["px"], hazard["py"], image, extent)
            ax.plot(x_A, y_A, "rx", ms=7, mew=1.5)

        candidates = (
            landscape.get("candidates")
            or landscape.get("flat_candidates")
            or []
        )
        for idx, candidate in enumerate(candidates[:6], start=1):
            px = candidate.get("px")
            py = candidate.get("py")
            if px is None or py is None:
                continue
            x_A, y_A = self.pixel_to_local_A(px, py, image, extent)
            ax.plot(x_A, y_A, "wo", ms=7, mec="k", mew=1.0)
            ax.text(
                x_A,
                y_A,
                str(idx),
                color="black",
                ha="center",
                va="center",
                fontsize=7,
            )

        best = landscape.get("best_candidate") or {}
        if best.get("px") is not None and best.get("py") is not None:
            x_A, y_A = self.pixel_to_local_A(best["px"], best["py"], image, extent)
            ax.plot(x_A, y_A, "c*", ms=12, mec="k", mew=0.8, label="best flat")

        peaks = [
            peak for peak in quality.get("autocorrelation_secondary_peaks", [])
            if peak.get("used_for_tip_verdict")
        ]
        summary_lines = [
            "Tip: {} ({})".format(
                quality.get("verdict", "n/a"),
                quality.get("confidence", "n/a"),
            ),
        ]
        rough = quality.get("flattened_stats", {}).get("roughness_std")
        if rough is not None:
            summary_lines.append("rough std: {:.4g} A".format(float(rough)))
        if peaks:
            peak = peaks[0]
            summary_lines.append(
                "peak: corr={:.3g}, d~{:.1f} A, angle={:.0f} deg".format(
                    float(peak.get("corr", 0.0)),
                    float(peak.get("dist_A_approx", 0.0)),
                    float(peak.get("angle_deg", 0.0)),
                )
            )
        if hazards:
            summary_lines.append("hazards: {}".format(len(hazards)))
        if candidates:
            summary_lines.append("flat candidates: {}".format(len(candidates)))

        ax.text(
            0.01,
            0.99,
            "\n".join(summary_lines),
            transform=ax.transAxes,
            va="top",
            ha="left",
            fontsize=8,
            color="white",
            bbox={"facecolor": "black", "alpha": 0.62, "pad": 4},
        )
        if hazards or candidates or best:
            ax.legend(loc="lower right", fontsize=7)

    def fetch_scan_plot(self, channel, chunk_lines):
        try:
            image, meta = self.runner.fetch_scan_image_to_line(
                channel=int(channel),
                chunk_lines=int(chunk_lines),
            )
            fig, extent = self.plot_scan_image_A(
                image,
                meta,
                channel,
                "Channel {} scan data, line 0 at top".format(channel),
                "signal",
            )
            report = {
                "meta": self.jsonable(meta),
                "extent_A": list(extent),
                "shape": list(image.shape),
                "min": float(image.min()),
                "max": float(image.max()),
                "mean": float(image.mean()),
                "std": float(image.std()),
            }
            return fig, report
        except Exception as exc:
            fig, ax = plt.subplots(figsize=(6, 4), constrained_layout=True)
            ax.text(0.5, 0.5, str(exc), ha="center", va="center", wrap=True)
            ax.set_axis_off()
            return fig, {"error": str(exc), "traceback": traceback.format_exc()}

    def analyze_current_scan(self, channel, chunk_lines, output_prefix):
        try:
            image, meta = self.runner.fetch_scan_image_to_line(
                channel=int(channel),
                chunk_lines=int(chunk_lines),
            )
            pixel = self.runner._pixel_size_A(int(channel))
            quality = self.runner.assess_tip_quality(
                image,
                pixel_size_A=pixel,
                output_prefix=str(self.output_dir / output_prefix),
            )
            landscape = self.nav.analyze_current_scan_landscape_map(
                channel=int(channel),
                output_prefix=str(self.output_dir / (output_prefix + "_landscape")),
            )
            fig, extent = self.plot_scan_image_A(
                image,
                meta,
                channel,
                "Scan image used for analysis",
                "topography / signal",
                quality=quality,
                landscape=landscape,
            )
            report = {
                "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                "fetch_meta": self.jsonable(meta),
                "extent_A": list(extent),
                "pixel_size_A": pixel,
                "tip_quality": quality,
                "landscape": landscape,
            }
            report_path = self.output_dir / (output_prefix + "_gui_report.json")
            report_path.write_text(json.dumps(self.jsonable(report), indent=2))
            return fig, self.jsonable(report), str(report_path)
        except Exception as exc:
            fig, ax = plt.subplots(figsize=(6, 4), constrained_layout=True)
            ax.text(0.5, 0.5, str(exc), ha="center", va="center", wrap=True)
            ax.set_axis_off()
            return fig, {"error": str(exc), "traceback": traceback.format_exc()}, ""

    def load_bias_pulse(self, pulse_du_V, arm):
        blocked = self.require_control_level(1, "load bias-pulse GVP")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "load a GVP program")
        if cancelled:
            return cancelled
        try:
            rec = self.runner.load_gvp_bias_pulse(float(pulse_du_V))
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def start_scan(self, arm):
        blocked = self.require_control_level(1, "start scan")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "start scan")
        if cancelled:
            return cancelled
        try:
            rec = self.runner.call_gxsm("startscan", name="start_scan")
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def stop_scan(self, arm):
        blocked = self.require_control_level(1, "stop scan")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "stop scan")
        if cancelled:
            return cancelled
        try:
            rec = self.runner.call_gxsm("stopscan", name="stop_scan")
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def set_bias(self, bias_V, arm):
        blocked = self.require_control_level(1, "set bias")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "set bias")
        if cancelled:
            return cancelled
        try:
            rec = self.runner.set_live_bias(
                float(bias_V),
                max_abs_auto_V=self.safety_limits()["level1_max_abs_bias_V"],
            )
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def set_current(self, current, unit, arm):
        blocked = self.require_control_level(1, "set current setpoint")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "set current setpoint")
        if cancelled:
            return cancelled
        try:
            rec = self.runner.set_live_current_setpoint(
                float(current),
                unit=unit,
                max_auto_nA=self.safety_limits()["level1_max_current_nA"],
            )
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def set_scan_speed(self, speed_A_s, range_A, arm):
        blocked = self.require_control_level(1, "set scan speed")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "set scan speed")
        if cancelled:
            return cancelled
        try:
            rec = self.runner.set_live_scan_speed(
                speed_A_per_s=float(speed_A_s),
                range_A=float(range_A),
            )
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def set_feedback_cp_ci(self, cp_dB, ci_dB, arm):
        blocked = self.require_control_level(1, "set feedback CP/CI")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "set feedback CP/CI")
        if cancelled:
            return cancelled
        try:
            limits = self.safety_limits()["level1_feedback_cp_ci_range_dB"]
            cp = float(cp_dB)
            ci = float(ci_dB)
            if not (limits[0] <= cp <= limits[1] and limits[0] <= ci <= limits[1]):
                raise ValueError(
                    "CP/CI must be within {}..{} dB for Level 1.".format(
                        limits[0],
                        limits[1],
                    )
                )
            cp_rec = self.runner.set_parameter("dsp-fbs-cp", cp)
            ci_rec = self.runner.set_parameter("dsp-fbs-ci", ci)
            return {
                "cp": self.jsonable(cp_rec.__dict__),
                "ci": self.jsonable(ci_rec.__dict__),
            }
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def set_scan_geometry(self, range_A, points, arm):
        blocked = self.require_control_level(1, "set scan geometry")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "set scan geometry")
        if cancelled:
            return cancelled
        try:
            range_A = float(range_A)
            points = int(points)
            range_limits = self.safety_limits()["level1_scan_range_A"]
            point_limits = self.safety_limits()["level1_scan_points"]
            if not (range_limits[0] <= range_A <= range_limits[1]):
                raise ValueError(
                    "Scan range must be within {}..{} A for Level 1.".format(
                        range_limits[0],
                        range_limits[1],
                    )
                )
            if not (point_limits[0] <= points <= point_limits[1]):
                raise ValueError(
                    "Scan points must be within {}..{} for Level 1.".format(
                        point_limits[0],
                        point_limits[1],
                    )
                )
            rec = self.runner.setup_scan_geometry(range_A=range_A, points=points)
            result = self.jsonable(rec.__dict__)
            spacing = None if points <= 1 else range_A / float(points - 1)
            result["informative_steps_A"] = {
                "StepsX": spacing,
                "StepsY": spacing,
                "note": "Scan StepsX/Y are Range/(Points-1) in Angstrom.",
            }
            return result
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def set_scan_slope(self, slope_x=None, slope_y=None, arm=False):
        blocked = self.require_control_level(1, "set scan slope")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "set scan slope")
        if cancelled:
            return cancelled
        try:
            rec = self.runner.set_scan_slope_leveling(
                slope_x=slope_x,
                slope_y=slope_y,
                max_abs_slope=self.safety_limits()["level1_max_abs_scan_slope"],
            )
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def execute_loaded_gvp(self, confirm_text, arm):
        blocked = self.require_control_level(1, "execute loaded GVP")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "execute loaded GVP")
        if cancelled:
            return cancelled
        phrase = self.safety_limits()["gvp_execute_extra_confirmation"]
        if str(confirm_text).strip() != phrase:
            return {"cancelled": "Type '{}' to execute loaded GVP.".format(phrase)}
        try:
            rec = self.runner.execute_loaded_gvp(precheck=True, wait_after_execute_s=8.0)
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def measure_scan_leveling(self, channel, line_count, output_prefix):
        try:
            report = self.runner.measure_recent_fast_axis_slope(
                channel=int(channel),
                line_count=int(line_count),
                output_prefix=str(self.output_dir / output_prefix),
            )
            profile_path = self.output_dir / (output_prefix + "_profile.png")
            fig = self.slope_report_figure(report, profile_path=profile_path)
            return fig, self.jsonable(report)
        except Exception as exc:
            fig, ax = plt.subplots(figsize=(6, 4), constrained_layout=True)
            ax.text(0.5, 0.5, str(exc), ha="center", va="center", wrap=True)
            ax.set_axis_off()
            return fig, {"error": str(exc), "traceback": traceback.format_exc()}

    def slope_report_figure(self, report, profile_path=None):
        fig, ax = plt.subplots(figsize=(7, 4), constrained_layout=True)
        if profile_path and Path(profile_path).exists():
            img = plt.imread(profile_path)
            ax.imshow(img)
            ax.set_axis_off()
            return fig
        if isinstance(report, dict):
            slope = report.get("fit_slope_local_fast_x_AngZ_per_Ang")
            residual = report.get("residual_std_after_fit_A")
            axis = report.get("leveling_axis")
            rotation = report.get("rotation_deg")
            current_x = report.get("current_scan_slope_x")
            current_y = report.get("current_scan_slope_y")
            text = (
                "Residual fast-axis slope: {slope}\n"
                "Residual std after fit: {residual}\n"
                "Rotation: {rotation} deg\n"
                "Corrects: {axis}\n"
                "Current slope X: {sx}\n"
                "Current slope Y: {sy}\n"
            ).format(
                slope="{:.6g} AngZ/Ang".format(float(slope)) if slope is not None else "n/a",
                residual="{:.6g} A".format(float(residual)) if residual is not None else "n/a",
                rotation="{:.3g}".format(float(rotation)) if rotation is not None else "n/a",
                axis=axis or "n/a",
                sx=current_x,
                sy=current_y,
            )
        else:
            text = str(report)
        ax.text(0.02, 0.98, text, va="top", ha="left", family="monospace")
        ax.set_axis_off()
        return fig

    def apply_scan_leveling_correction(
        self,
        channel,
        line_count,
        correction_sign,
        gain,
        max_abs_delta,
        verify_delay_s,
        verify_line_count,
        output_prefix,
        arm,
    ):
        blocked = self.require_control_level(1, "apply scan slope correction")
        if blocked:
            return self.slope_report_figure(blocked), blocked
        cancelled = self.require_arm(arm, "apply scan slope correction")
        if cancelled:
            return self.slope_report_figure(cancelled), cancelled
        try:
            result = self.runner.auto_correct_scan_slope_from_recent_lines(
                channel=int(channel),
                line_count=int(line_count),
                correction_sign=float(correction_sign),
                gain=float(gain),
                verify_delay_s=float(verify_delay_s),
                verify_line_count=int(verify_line_count),
                max_abs_delta=float(max_abs_delta),
                output_prefix=str(self.output_dir / output_prefix),
            )
            profile_path = self.output_dir / (output_prefix + "_after_profile.png")
            fig = self.slope_report_figure(result.get("post") or result, profile_path=profile_path)
            return fig, self.jsonable(result)
        except Exception as exc:
            report = {"error": str(exc), "traceback": traceback.format_exc()}
            return self.slope_report_figure(report), report


def build_ui(backend):
    try:
        import gradio as gr
    except ModuleNotFoundError as exc:
        raise SystemExit(
            "Gradio is not installed. Install it from this folder with:\n"
            "  python3 -m pip install -r requirements-gui.txt\n"
            "Then run:\n"
            "  ./microscope_gradio_gui.py --host 127.0.0.1 --port 7860\n"
        ) from exc

    with gr.Blocks(title="GXSM4 Local Microscope Copilot") as demo:
        gr.Markdown("# GXSM4 Local Microscope Copilot")
        status = gr.Textbox(value=backend.status_text(), label="Status", interactive=False)

        with gr.Tab("Control Level"):
            gr.Markdown(
                "Select the operator permission level for live-changing GUI actions."
            )
            control_level = gr.Dropdown(
                choices=[CONTROL_LEVELS[i] for i in sorted(CONTROL_LEVELS)],
                value=CONTROL_LEVELS[0],
                label="Control Level",
            )
            level_report = gr.JSON(
                value={
                    "control_level": 0,
                    "description": CONTROL_LEVELS[0],
                    "help": CONTROL_LEVEL_HELP.strip(),
                    "safety_limits": backend.safety_limits(),
                },
                label="Control-level policy",
            )
        control_level.change(backend.set_control_level, control_level, [status, level_report])

        with gr.Tab("Chat"):
            gr.Markdown(
                "Ollama replies are advisory. Clear Level 1 chat commands "
                "(including start/stop scan and bounded parameter changes) can "
                "execute only when Control Level is 1+ and chat action arm is checked."
            )
            chat = gr.Chatbot(label="Ollama chat", height=420)
            chat_arm = gr.Checkbox(value=False, label="Arm Level 1 chat actions")
            prompt = gr.Textbox(label="Prompt", placeholder="Ask for interpretation or a proposed next step")
            prompt.submit(backend.chat, [prompt, chat, chat_arm], [prompt, chat])

        with gr.Tab("Live Readouts"):
            with gr.Row():
                read_btn = gr.Button("Read X/Y/Z/U Monitors")
                df_btn = gr.Button("Sample dFreq")
            with gr.Row():
                df_count = gr.Number(value=20, label="dFreq samples", precision=0)
                df_delay = gr.Number(value=0.15, label="Delay per sample (s)")
            monitors = gr.JSON(label="Monitor readback")
            dfreq = gr.JSON(label="dFrequency report")
            read_btn.click(backend.read_monitors, outputs=[monitors, status])
            df_btn.click(backend.sample_dfrequency, [df_count, df_delay], dfreq)

        with gr.Tab("Scan Image"):
            with gr.Row():
                scan_channel = gr.Number(value=0, label="Channel", precision=0)
                scan_lines = gr.Number(value=120, label="Recent/full lines to fetch", precision=0)
                fetch_btn = gr.Button("Fetch And Plot")
            scan_plot = gr.Plot(label="Scan image")
            scan_meta = gr.JSON(label="Scan metadata")
            fetch_btn.click(backend.fetch_scan_plot, [scan_channel, scan_lines], [scan_plot, scan_meta])

        with gr.Tab("Tip / Landscape Analysis"):
            with gr.Row():
                ana_channel = gr.Number(value=0, label="Channel", precision=0)
                ana_lines = gr.Number(value=160, label="Lines to fetch", precision=0)
                ana_prefix = gr.Textbox(value="gui_scan_analysis", label="Output prefix")
                ana_btn = gr.Button("Analyze Current Scan")
            ana_plot = gr.Plot(label="Analyzed image")
            ana_report = gr.JSON(label="Analysis report")
            ana_path = gr.Textbox(label="Saved report path", interactive=False)
            ana_btn.click(
                backend.analyze_current_scan,
                [ana_channel, ana_lines, ana_prefix],
                [ana_plot, ana_report, ana_path],
            )

        with gr.Tab("Scan Leveling"):
            gr.Markdown(
                "Measure residual fast-axis slope from recent scan lines. "
                "Apply correction only on flat areas and only after setting "
                "Rotation=0 for world X or Rotation=90 for world Y."
            )
            with gr.Row():
                level_channel = gr.Number(value=0, label="Channel", precision=0)
                level_lines = gr.Number(value=64, label="Recent line count", precision=0)
                level_prefix = gr.Textbox(value="gui_scan_leveling", label="Output prefix")
                measure_level_btn = gr.Button("Measure Residual Slope")
            level_plot = gr.Plot(label="Recent-line mean profile and fit")
            level_report = gr.JSON(label="Leveling report")
            measure_level_btn.click(
                backend.measure_scan_leveling,
                [level_channel, level_lines, level_prefix],
                [level_plot, level_report],
            )
            gr.Markdown(
                "Correction is Level 1 and arm-gated. If the correction worsens "
                "the residual, use the opposite correction sign."
            )
            with gr.Row():
                level_arm = gr.Checkbox(value=False, label="Arm leveling correction")
                correction_sign = gr.Number(value=-1.0, label="Correction sign")
                correction_gain = gr.Number(value=1.0, label="Gain")
                max_abs_delta = gr.Number(value=0.005, label="Max abs delta")
            with gr.Row():
                verify_delay = gr.Number(value=20.0, label="Verify delay (s)")
                verify_lines = gr.Number(value=16, label="Verify line count", precision=0)
                apply_level_btn = gr.Button("Apply Slope Correction")
            apply_level_btn.click(
                backend.apply_scan_leveling_correction,
                [
                    level_channel,
                    level_lines,
                    correction_sign,
                    correction_gain,
                    max_abs_delta,
                    verify_delay,
                    verify_lines,
                    level_prefix,
                    level_arm,
                ],
                [level_plot, level_report],
            )

        with gr.Tab("Armed Actions"):
            gr.Markdown(
                "Live-changing actions require the arm checkbox. In dry-run mode, "
                "actions are recorded but not sent to GXSM4. Start the GUI with "
                "`--live` to enable real microscope calls. Level 1 allows only "
                "bounded basic operations. Levels 2 and 3 are placeholders for "
                "future advanced/extreme workflows."
            )
            arm = gr.Checkbox(value=False, label="Arm one live action")
            with gr.Row():
                stop_btn = gr.Button("Stop Scan")
                start_btn = gr.Button("Start Scan")
            with gr.Row():
                bias_V = gr.Number(value=0.1, label="Bias target (V)")
                set_bias_btn = gr.Button("Set Bias")
            with gr.Row():
                current_value = gr.Number(value=10.0, label="Current target")
                current_unit = gr.Dropdown(choices=["pA", "nA"], value="pA", label="Current unit")
                set_current_btn = gr.Button("Set Current Setpoint")
            with gr.Row():
                scan_speed = gr.Number(value=700.0, label="Scan speed (A/s)")
                scan_range = gr.Number(value=700.0, label="Current scan range (A)")
                set_speed_btn = gr.Button("Set Scan Speed")
            with gr.Row():
                cp_dB = gr.Number(value=-90.0, label="Feedback CP (dB)")
                ci_dB = gr.Number(value=-90.0, label="Feedback CI (dB)")
                set_feedback_btn = gr.Button("Set Feedback CP/CI")
            pulse_du = gr.Number(value=2.0, label="Bias pulse du (V)")
            load_pulse_btn = gr.Button("Load Bias Pulse GVP Only")
            gvp_confirm = gr.Textbox(
                label="GVP execute confirmation",
                placeholder="Type EXECUTE GVP to run loaded GVP",
            )
            execute_gvp_btn = gr.Button("Execute Loaded GVP")
            action_report = gr.JSON(label="Action result")
            stop_btn.click(backend.stop_scan, arm, action_report)
            start_btn.click(backend.start_scan, arm, action_report)
            set_bias_btn.click(backend.set_bias, [bias_V, arm], action_report)
            set_current_btn.click(
                backend.set_current,
                [current_value, current_unit, arm],
                action_report,
            )
            set_speed_btn.click(
                backend.set_scan_speed,
                [scan_speed, scan_range, arm],
                action_report,
            )
            set_feedback_btn.click(
                backend.set_feedback_cp_ci,
                [cp_dB, ci_dB, arm],
                action_report,
            )
            load_pulse_btn.click(backend.load_bias_pulse, [pulse_du, arm], action_report)
            execute_gvp_btn.click(
                backend.execute_loaded_gvp,
                [gvp_confirm, arm],
                action_report,
            )

    return demo


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--config",
        default=str(SCRIPT_DIR / "local_microscope_copilot_config.json"),
    )
    parser.add_argument("--model")
    parser.add_argument("--live", action="store_true")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=7860)
    parser.add_argument("--share", action="store_true")
    args = parser.parse_args(argv)

    os.chdir(SCRIPT_DIR)
    config = load_config(args.config)
    if args.model:
        config["model"] = args.model
    backend = MicroscopeGradioBackend(config, live=args.live)
    demo = build_ui(backend)
    demo.launch(
        server_name=args.host,
        server_port=args.port,
        share=args.share,
        show_error=True,
    )


if __name__ == "__main__":
    main()
