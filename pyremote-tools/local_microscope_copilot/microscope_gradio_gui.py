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
DEFAULT_SSL_CERTFILE = "/etc/grafana/ltncafm_cfn_bnl_gov.pem"
DEFAULT_SSL_KEYFILE = "/etc/grafana/ltncafm_cfn_bnl_gov_privkey.key"
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(1, str(PROJECT_ROOT))

from microscope_actions import (
    LandscapeNavigationController,
    Level3ProtectedController,
    MicroscopeActionRunner,
    TipImprovementActivityController,
)
from microscope_gradio_auth import (
    DEFAULT_AUTH_PASSWORD_ENV,
    DEFAULT_AUTH_PASSWORD_FILE,
    DEFAULT_AUTH_USER,
    resolve_gradio_auth,
)
from microscope_gradio_gvp import GvpGuiMixin
from microscope_gradio_policy import (
    ACTION_CLAIM_RE,
    CHAT_ROUTER_HELP,
    CONTROL_LEVEL_HELP,
    CONTROL_LEVELS,
    DEFAULT_SYSTEM_PROMPT,
    HARDWARE_WORD_RE,
    KNOWN_READBACK_REFS,
    PARAM_ALIASES,
    PARAM_SPECS,
)
from microscope_gradio_scan import ScanGuiMixin
from microscope_gradio_tip_planner import TipPlannerGuiMixin


def load_config(path):
    with open(path, "r") as file:
        return json.load(file)


class MicroscopeGradioBackend(ScanGuiMixin, GvpGuiMixin, TipPlannerGuiMixin):
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
        self.level3 = Level3ProtectedController(
            self.runner,
            thv_base_url=self.config.get("thv_base_url", "http://192.168.40.10/"),
            output_dir=str(self.output_dir),
        )
        self.control_level = 0
        self.chat_context = {
            "last_parameter": None,
            "last_action_domain": None,
            "last_readback": {},
            "last_intent": None,
        }
        self.scan_view_state = {}
        self.init_tip_planner_state()
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
        routed = self.route_chat_intent(text, chat_arm=chat_arm)
        if routed is not None:
            return routed
        analysis_reply = self.try_chat_scan_analysis(text)
        if analysis_reply is not None:
            return analysis_reply
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

    def route_chat_intent(self, text, chat_arm=False):
        text_l = text.lower().strip()
        if not text_l:
            return None
        if re.search(r"\b(what can you do|help|capabilit(?:y|ies)|actions?|commands?)\b", text_l):
            if any(word in text_l for word in ("chat", "router", "action", "hardware", "microscope")):
                return CHAT_ROUTER_HELP.strip() + "\n\n```json\n{}\n```".format(
                    json.dumps(self.chat_router_capabilities(), indent=2)
                )
        if self.looks_like_tip_or_scan_analysis(text_l):
            self.chat_context["last_action_domain"] = "analysis"
            return self.try_chat_scan_analysis(text)
        compound_reply = self.route_compound_level1_intent(text, chat_arm=chat_arm)
        if compound_reply is not None:
            return compound_reply
        scan_action = self.infer_scan_action(text_l)
        if scan_action:
            self.chat_context["last_action_domain"] = "scan"
            return self.execute_chat_level1_action(
                "{} scan".format(scan_action),
                "{}scan".format(scan_action),
                chat_arm,
                lambda: self.start_scan(arm=True)
                if scan_action == "start"
                else self.stop_scan(arm=True),
            )
        if self.looks_like_live_monitor_read(text_l):
            self.chat_context["last_action_domain"] = "monitor"
            return self.format_live_monitor_readback()
        if self.looks_like_read_request(text_l):
            keys = self.infer_parameter_keys(text_l, allow_context=True)
            if keys or self.looks_like_scan_geometry_request(text_l):
                self.chat_context["last_action_domain"] = "readback"
                if self.looks_like_scan_geometry_request(text_l):
                    return self.format_parameter_readback(list(KNOWN_READBACK_REFS))
                return self.format_parameter_readback(
                    [(PARAM_SPECS[key]["label"], PARAM_SPECS[key]["refname"]) for key in keys]
                )
        if self.looks_like_write_request(text_l):
            keys = self.infer_parameter_keys(text_l, allow_context=True)
            if self.looks_like_scan_geometry_request(text_l):
                return self.route_scan_geometry_write(text, chat_arm=chat_arm)
            if keys:
                return self.route_parameter_write(text, keys[0], chat_arm=chat_arm)
            if self.context_parameter_key():
                return self.route_parameter_write(
                    text,
                    self.context_parameter_key(),
                    chat_arm=chat_arm,
                )
        return None

    def route_compound_level1_intent(self, text, chat_arm=False):
        text_l = text.lower().strip()
        scan_action = self.infer_scan_action(text_l)
        if not scan_action:
            return None
        keys = [
            key for key in self.infer_parameter_keys(text_l, allow_context=False)
            if PARAM_SPECS.get(key, {}).get("write")
        ]
        has_geometry = self.looks_like_scan_geometry_request(text_l)
        if not keys and not has_geometry:
            return None
        plan = []
        if has_geometry:
            parsed = self.parse_scan_geometry_request(text)
            if parsed is None:
                return None
            range_A, points = parsed
            plan.append(
                {
                    "action_name": "scan geometry",
                    "target": "{} A, {} points".format(range_A, points),
                    "callback": lambda range_A=range_A, points=points: self.set_scan_geometry(
                        range_A,
                        points,
                        arm=True,
                    ),
                }
            )
        for key in keys:
            entry = self.build_parameter_write_plan_entry(text, key)
            if isinstance(entry, str):
                return entry
            if entry:
                plan.append(entry)
        if not plan:
            return None
        if scan_action == "start":
            plan.append(
                {
                    "action_name": "start scan",
                    "target": "startscan",
                    "callback": lambda: self.start_scan(arm=True),
                }
            )
        else:
            plan.append(
                {
                    "action_name": "stop scan",
                    "target": "stopscan",
                    "callback": lambda: self.stop_scan(arm=True),
                }
            )
        self.chat_context["last_action_domain"] = "scan"
        return self.execute_chat_level1_plan(plan, chat_arm=chat_arm)

    def build_parameter_write_plan_entry(self, text, key):
        spec = PARAM_SPECS.get(key)
        if not spec or not spec.get("write"):
            return None
        write_kind = spec["write"]
        if write_kind == "bias":
            target = self.resolve_numeric_target(
                text,
                current_refname=spec["refname"],
                percent_ok=True,
            )
            if target is None:
                if self.is_relative_request(text):
                    return "I did not change the bias. A relative bias change needs a valid current bias readback first."
                return None
            if abs(target) > self.safety_limits()["level1_max_abs_bias_V"]:
                return (
                    "I did not change the bias. A target of {:.6g} V is outside "
                    "the Level 1 automatic limit of +/-1 V."
                ).format(target)
            return {
                "action_name": "bias",
                "target": "{} V".format(self.fmt6(target)),
                "callback": lambda target=target: self.set_bias(target, arm=True),
            }
        if write_kind == "current":
            target_nA = self.resolve_current_target_nA(text, spec["refname"])
            if target_nA is None:
                if self.is_relative_request(text):
                    return "I did not change the current setpoint. A relative current change needs a valid current setpoint readback first."
                return None
            if target_nA > self.safety_limits()["level1_max_current_nA"]:
                return (
                    "I did not change the current setpoint. The requested value "
                    "is {:.6g} nA, above the Level 1 automatic limit of {:.6g} nA."
                ).format(target_nA, self.safety_limits()["level1_max_current_nA"])
            return {
                "action_name": "current setpoint",
                "target": "{} nA".format(self.fmt6(target_nA)),
                "callback": lambda target_nA=target_nA: self.set_current(
                    target_nA,
                    "nA",
                    arm=True,
                ),
            }
        if write_kind == "scan_speed":
            target = self.resolve_numeric_target(
                text,
                current_refname=spec["refname"],
                percent_ok=True,
                relative_words={"faster": 1.2, "slower": 0.8},
            )
            if target is None:
                if self.is_relative_request(text) or re.search(r"\b(faster|slower)\b", text.lower()):
                    return "I did not change the scan speed. A relative speed change needs a valid current scan-speed readback first."
                return None
            range_A = self.read_float_parameter("RangeX", fallback=700.0)
            return {
                "action_name": "scan speed",
                "target": "{} A/s".format(self.fmt6(target)),
                "callback": lambda target=target, range_A=range_A: self.set_scan_speed(
                    target,
                    range_A,
                    arm=True,
                ),
            }
        if write_kind in ("slope_x", "slope_y"):
            target = self.resolve_numeric_target(text, current_refname=spec["refname"])
            if target is None:
                if self.is_relative_request(text):
                    return "I did not change {}. A relative change needs a valid current readback first.".format(
                        spec["label"]
                    )
                return None
            if abs(target) > self.safety_limits()["level1_max_abs_scan_slope"]:
                return "I did not change {}. Level 1 allows abs(slope) <= 0.1.".format(
                    spec["label"]
                )
            return {
                "action_name": spec["label"],
                "target": target,
                "callback": lambda target=target, write_kind=write_kind: self.set_scan_slope(
                    slope_x=target if write_kind == "slope_x" else None,
                    slope_y=target if write_kind == "slope_y" else None,
                    arm=True,
                ),
            }
        if write_kind in ("feedback_cp", "feedback_ci"):
            target = self.resolve_numeric_target(text, current_refname=spec["refname"])
            if target is None:
                if self.is_relative_request(text):
                    return "I did not change {}. A relative change needs a valid current readback first.".format(
                        spec["label"]
                    )
                return None
            limits = self.safety_limits()["level1_feedback_cp_ci_range_dB"]
            if not (limits[0] <= target <= limits[1]):
                return (
                    "I did not change {}. Level 1 allows {}..{} dB."
                ).format(spec["label"], limits[0], limits[1])
            other_key = "feedback_ci" if write_kind == "feedback_cp" else "feedback_cp"
            other = self.read_float_parameter(PARAM_SPECS[other_key]["refname"], fallback=-90.0)
            cp = target if write_kind == "feedback_cp" else other
            ci = target if write_kind == "feedback_ci" else other
            return {
                "action_name": "feedback CP/CI",
                "target": "CP {}, CI {}".format(self.fmt6(cp), self.fmt6(ci)),
                "callback": lambda cp=cp, ci=ci: self.set_feedback_cp_ci(cp, ci, arm=True),
            }
        return None

    def chat_router_capabilities(self):
        return {
            "control_level": self.control_level,
            "live_mode": self.live,
            "level1_writes": {
                key: {
                    "label": spec["label"],
                    "unit": spec["unit"],
                    "aliases": list(spec["aliases"]),
                }
                for key, spec in PARAM_SPECS.items()
                if spec.get("write")
            },
            "level1_actions": ["start scan", "stop scan"],
            "readbacks": {
                key: {
                    "label": spec["label"],
                    "refname": spec["refname"],
                    "unit": spec["unit"],
                }
                for key, spec in PARAM_SPECS.items()
            },
            "safety_limits": self.safety_limits(),
        }

    def looks_like_tip_or_scan_analysis(self, text_l):
        return (
            any(
                phrase in text_l
                for phrase in (
                    "tip shape",
                    "tip quality",
                    "tip assessment",
                    "tip analysis",
                    "double tip",
                    "multi tip",
                    "multiple tip",
                    "autocorrelation",
                )
            )
            or (
                "scan" in text_l
                and any(
                    word in text_l
                    for word in ("analyze", "analyse", "analysis", "assess", "evaluate", "check")
                )
            )
        )

    def looks_like_live_monitor_read(self, text_l):
        return (
            any(word in text_l for word in ("live", "fast", "rpspmc", "xyz"))
            and any(word in text_l for word in ("read", "show", "report", "monitor", "status"))
        )

    def looks_like_read_request(self, text_l):
        return bool(
            re.search(
                r"\b(read|show|get|report|status|settings|what('?s| is)?|current|where|monitor)\b",
                text_l,
            )
        )

    def looks_like_write_request(self, text_l):
        return bool(
            re.search(
                r"\b(set|change|adjust|make|configure|increase|decrease|raise|lower|faster|slower)\b",
                text_l,
            )
        )

    def looks_like_scan_geometry_request(self, text_l):
        return (
            "scan" in text_l
            and any(word in text_l for word in ("size", "range", "geometry", "points"))
        ) or bool(re.search(r"\b\d+\s*x\s*\d+\s*(?:a|ang|angstrom|points?)\b", text_l))

    def infer_scan_action(self, text_l):
        mentions_scan = any(word in text_l for word in ("scan", "scanning", "image"))
        context_scan = self.chat_context.get("last_action_domain") == "scan"
        if mentions_scan or context_scan:
            if re.search(r"\b(start|begin|run|go|resume|restart)\b", text_l):
                return "start"
            if re.search(r"\b(stop|halt|abort|end|pause)\b", text_l):
                return "stop"
        return None

    def infer_parameter_keys(self, text_l, allow_context=False):
        matches = []
        for key, spec in PARAM_SPECS.items():
            for alias in spec["aliases"]:
                if re.search(r"(?<![a-z0-9_-]){}(?![a-z0-9_-])".format(re.escape(alias)), text_l):
                    matches.append(key)
                    break
        if matches:
            # Prefer longer aliases already encoded by insertion order, while
            # removing duplicates without scrambling priority.
            result = []
            for key in matches:
                if key not in result:
                    result.append(key)
            self.chat_context["last_parameter"] = result[0]
            return result
        if allow_context and re.search(r"\b(it|that|same parameter|this parameter)\b", text_l):
            key = self.context_parameter_key()
            return [key] if key else []
        return []

    def context_parameter_key(self):
        key = self.chat_context.get("last_parameter")
        return key if key in PARAM_SPECS else None

    def route_scan_geometry_write(self, text, chat_arm=False):
        parsed = self.parse_scan_geometry_request(text)
        if parsed is None:
            return None
        range_A, points = parsed
        self.chat_context["last_action_domain"] = "scan"
        self.chat_context["last_parameter"] = "scan_geometry"
        return self.execute_chat_level1_action(
            "scan geometry",
            "{} A, {} points".format(range_A, points),
            chat_arm,
            lambda: self.set_scan_geometry(range_A, points, arm=True),
        )

    def route_parameter_write(self, text, key, chat_arm=False):
        spec = PARAM_SPECS.get(key)
        if not spec or not spec.get("write"):
            return None
        write_kind = spec["write"]
        text_l = text.lower()
        if write_kind == "bias":
            target = self.resolve_numeric_target(
                text,
                current_refname=spec["refname"],
                percent_ok=True,
            )
            if target is None:
                if self.is_relative_request(text):
                    return "I did not change the bias. A relative bias change needs a valid current bias readback first."
                return None
            if abs(target) > self.safety_limits()["level1_max_abs_bias_V"]:
                return (
                    "I did not change the bias. A target of {:.6g} V is outside "
                    "the Level 1 automatic limit of +/-1 V."
                ).format(target)
            return self.execute_chat_level1_action(
                "bias",
                "{} V".format(self.fmt6(target)),
                chat_arm,
                lambda: self.set_bias(target, arm=True),
            )
        if write_kind == "current":
            target_nA = self.resolve_current_target_nA(text, spec["refname"])
            if target_nA is None:
                if self.is_relative_request(text):
                    return "I did not change the current setpoint. A relative current change needs a valid current setpoint readback first."
                return None
            if target_nA > self.safety_limits()["level1_max_current_nA"]:
                return (
                    "I did not change the current setpoint. The requested value "
                    "is {:.6g} nA, above the Level 1 automatic limit of {:.6g} nA."
                ).format(target_nA, self.safety_limits()["level1_max_current_nA"])
            return self.execute_chat_level1_action(
                "current setpoint",
                "{} nA".format(self.fmt6(target_nA)),
                chat_arm,
                lambda: self.set_current(target_nA, "nA", arm=True),
            )
        if write_kind == "scan_speed":
            target = self.resolve_numeric_target(
                text,
                current_refname=spec["refname"],
                percent_ok=True,
                relative_words={"faster": 1.2, "slower": 0.8},
            )
            if target is None:
                if self.is_relative_request(text) or re.search(r"\b(faster|slower)\b", text_l):
                    return "I did not change the scan speed. A relative speed change needs a valid current scan-speed readback first."
                return None
            range_A = self.read_float_parameter("RangeX", fallback=700.0)
            return self.execute_chat_level1_action(
                "scan speed",
                "{} A/s".format(self.fmt6(target)),
                chat_arm,
                lambda: self.set_scan_speed(target, range_A, arm=True),
            )
        if write_kind in ("slope_x", "slope_y"):
            target = self.resolve_numeric_target(text, current_refname=spec["refname"])
            if target is None:
                if self.is_relative_request(text):
                    return "I did not change {}. A relative change needs a valid current readback first.".format(
                        spec["label"]
                    )
                return None
            if abs(target) > self.safety_limits()["level1_max_abs_scan_slope"]:
                return "I did not change {}. Level 1 allows abs(slope) <= 0.1.".format(
                    spec["label"]
                )
            return self.execute_chat_level1_action(
                spec["label"],
                target,
                chat_arm,
                lambda: self.set_scan_slope(
                    slope_x=target if write_kind == "slope_x" else None,
                    slope_y=target if write_kind == "slope_y" else None,
                    arm=True,
                ),
            )
        if write_kind in ("feedback_cp", "feedback_ci"):
            target = self.resolve_numeric_target(text, current_refname=spec["refname"])
            if target is None:
                if self.is_relative_request(text):
                    return "I did not change {}. A relative change needs a valid current readback first.".format(
                        spec["label"]
                    )
                return None
            limits = self.safety_limits()["level1_feedback_cp_ci_range_dB"]
            if not (limits[0] <= target <= limits[1]):
                return (
                    "I did not change {}. Level 1 allows {}..{} dB."
                ).format(spec["label"], limits[0], limits[1])
            other_key = "feedback_ci" if write_kind == "feedback_cp" else "feedback_cp"
            other = self.read_float_parameter(PARAM_SPECS[other_key]["refname"], fallback=-90.0)
            cp = target if write_kind == "feedback_cp" else other
            ci = target if write_kind == "feedback_ci" else other
            return self.execute_chat_level1_action(
                "feedback CP/CI",
                "CP {}, CI {}".format(self.fmt6(cp), self.fmt6(ci)),
                chat_arm,
                lambda: self.set_feedback_cp_ci(cp, ci, arm=True),
            )
        return None

    def resolve_numeric_target(
        self,
        text,
        current_refname=None,
        percent_ok=False,
        relative_words=None,
    ):
        text_l = text.lower()
        current = None
        if current_refname:
            current = self.read_float_parameter(current_refname, fallback=np.nan)
            if not np.isfinite(current):
                current = None
        relative_words = relative_words or {}
        relative_requested = self.is_relative_request(text) or any(
            re.search(r"\b{}\b".format(re.escape(word)), text_l)
            for word in relative_words
        )
        if relative_requested and current is None:
            return None
        for word, factor in relative_words.items():
            if re.search(r"\b{}\b".format(re.escape(word)), text_l) and current is not None:
                percent = self.parse_percent(text_l)
                return current * (1.0 + percent / 100.0) if percent is not None else current * factor
        delta_match = re.search(
            r"\b(?:increase|raise|decrease|lower|up|down)\b[^-+\d]*([-+]?\d+(?:\.\d+)?)",
            text_l,
        )
        if delta_match and current is not None:
            delta = float(delta_match.group(1))
            if re.search(r"\b(decrease|lower|down)\b", text_l):
                delta = -abs(delta)
            elif re.search(r"\b(increase|raise|up)\b", text_l):
                delta = abs(delta)
            if percent_ok and "%" in text_l:
                return current * (1.0 + delta / 100.0)
            return current + delta
        if percent_ok and current is not None:
            percent = self.parse_percent(text_l)
            if percent is not None and re.search(r"\b(increase|raise|decrease|lower)\b", text_l):
                sign = -1.0 if re.search(r"\b(decrease|lower)\b", text_l) else 1.0
                return current * (1.0 + sign * abs(percent) / 100.0)
        return self.last_number(text)

    def is_relative_request(self, text):
        return bool(
            re.search(
                r"\b(increase|raise|decrease|lower|up|down|more|less)\b",
                str(text).lower(),
            )
            or "%" in str(text)
        )

    def resolve_current_target_nA(self, text, current_refname):
        text_l = text.lower()
        if re.search(r"\b(increase|raise|decrease|lower|up|down)\b", text_l) or "%" in text_l:
            return self.resolve_numeric_target(
                text,
                current_refname=current_refname,
                percent_ok=True,
            )
        parsed = self.parse_current_request(text)
        if parsed is not None:
            value, unit = parsed
            return float(value) / 1000.0 if str(unit).lower() == "pa" else float(value)
        target = self.resolve_numeric_target(text, current_refname=current_refname)
        return target

    def parse_percent(self, text_l):
        match = re.search(r"([-+]?\d+(?:\.\d+)?)\s*%", text_l)
        return None if not match else float(match.group(1))

    def format_live_monitor_readback(self):
        try:
            xyz = self.runner.read_xyz_monitor()
            rpspmc = self.runner.read_rpspmc_monitor()
            self.chat_context["last_action_domain"] = "monitor"
            compact = {
                "xyz_monitor_V": xyz.get("monitor"),
                "bias_V": rpspmc.get("bias"),
                "current_nA": rpspmc.get("current"),
                "gvp_u_V": (rpspmc.get("gvp") or {}).get("u"),
                "pac_ampl_mV": (rpspmc.get("pac") or {}).get("ampl"),
                "pac_dfreq_Hz": (rpspmc.get("pac") or {}).get("dfreq"),
            }
            compact = self.jsonable(compact)
            lines = ["Fast live monitor readback:"]
            for label, value in compact.items():
                lines.append("{}: {}".format(label, value))
            return "\n".join(lines) + "\n\n```json\n{}\n```".format(
                json.dumps(self.jsonable({"compact": compact, "xyz": xyz, "rpspmc": rpspmc}), indent=2)
            )
        except Exception as exc:
            return (
                "Fast monitor readback failed: {}\n\n```text\n{}\n```"
            ).format(str(exc), traceback.format_exc())

    def try_chat_scan_analysis(self, text):
        if not self.live:
            return "Tip/scan analysis needs live GXSM scan data. The GUI is currently in dry-run mode."
        text_l = text.lower()
        wants_tip = any(
            phrase in text_l
            for phrase in (
                "tip shape",
                "tip quality",
                "tip assessment",
                "tip analysis",
                "double tip",
                "multi tip",
                "multiple tip",
                "autocorrelation",
            )
        )
        wants_scan_analysis = (
            "scan" in text_l
            and any(word in text_l for word in ("analyze", "analyse", "analysis", "assess", "evaluate", "check"))
        )
        if not (wants_tip or wants_scan_analysis):
            return None
        channel = self.parse_channel_request(text, default=0)
        lines = self.parse_line_count_request(text, default=256)
        prefix = "chat_tip_analysis_{}".format(time.strftime("%Y%m%d-%H%M%S"))
        try:
            _fig, report, report_path = self.analyze_current_scan(
                channel=channel,
                chunk_lines=lines,
                output_prefix=prefix,
            )
            if isinstance(report, dict) and report.get("error"):
                return (
                    "Tip/scan analysis failed:\n\n```json\n{}\n```"
                ).format(json.dumps(report, indent=2))
            return self.format_tip_analysis_reply(report, report_path)
        except Exception as exc:
            return (
                "Tip/scan analysis failed: {}\n\n```text\n{}\n```"
            ).format(str(exc), traceback.format_exc())

    def format_tip_analysis_reply(self, report, report_path):
        quality = report.get("tip_quality", {}) if isinstance(report, dict) else {}
        landscape = report.get("landscape", {}) if isinstance(report, dict) else {}
        peaks = [
            peak for peak in quality.get("autocorrelation_secondary_peaks", [])
            if peak.get("used_for_tip_verdict")
        ]
        hazards = landscape.get("hazards", [])
        candidates = landscape.get("flat_candidates", [])
        best = landscape.get("best_candidate") or {}
        lines = [
            "Deterministic tip/scan analysis complete.",
            "Tip verdict: {} (confidence: {}).".format(
                quality.get("verdict", "n/a"),
                quality.get("confidence", "n/a"),
            ),
        ]
        rough = quality.get("flattened_stats", {}).get("roughness_std")
        if rough is not None:
            lines.append("Flattened roughness std: {:.6g} A.".format(float(rough)))
        if peaks:
            lines.append("Autocorrelation peaks used for verdict:")
            for peak in peaks[:4]:
                lines.append(
                    "- corr={:.4g}, distance~{:.3g} A, angle={:.3g} deg, dx={}, dy={}".format(
                        float(peak.get("corr", 0.0)),
                        float(peak.get("dist_A_approx", 0.0)),
                        float(peak.get("angle_deg", 0.0)),
                        peak.get("dx"),
                        peak.get("dy"),
                    )
                )
        else:
            lines.append("No strong autocorrelation secondary peak was selected for the tip verdict.")
        if hazards is not None:
            lines.append("Detected major hazards/bumps: {}.".format(len(hazards)))
        if candidates is not None:
            lines.append("Flat work-area candidates: {}.".format(len(candidates)))
        if best:
            lines.append(
                "Best flat candidate: px={}, py={}, world=({}, {}) A.".format(
                    best.get("px"),
                    best.get("py"),
                    best.get("world_x_A"),
                    best.get("world_y_A"),
                )
            )
        if report_path:
            lines.append("Full JSON report: {}".format(report_path))
        compact = {
            "tip_quality": quality,
            "landscape_summary": {
                "hazard_count": len(hazards or []),
                "flat_candidate_count": len(candidates or []),
                "best_candidate": best,
            },
            "report_path": report_path,
        }
        return "\n".join(lines) + "\n\n```json\n{}\n```".format(
            json.dumps(self.jsonable(compact), indent=2)
        )

    def parse_channel_request(self, text, default=0):
        match = re.search(r"\bch(?:annel)?\s*=?\s*(\d+)\b", text, re.IGNORECASE)
        return int(match.group(1)) if match else int(default)

    def parse_line_count_request(self, text, default=256):
        match = re.search(r"(\d+)\s*(?:lines?|scan lines?)\b", text, re.IGNORECASE)
        if not match:
            return int(default)
        return max(8, min(2048, int(match.group(1))))

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
            self.chat_context["last_readback"] = dict(result)
            if len(seen) == 1:
                label = next(iter(seen))
                for key, spec in PARAM_SPECS.items():
                    if spec["label"] == label:
                        self.chat_context["last_parameter"] = key
                        break
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

    def execute_chat_level1_plan(self, plan, chat_arm=False):
        names = [step["action_name"] for step in plan]
        if self.control_level < 1:
            return (
                "I did not execute the requested action plan ({}). Chat Level 1 "
                "actions require Control Level 1 or higher, plus the chat arm checkbox."
            ).format(" -> ".join(names))
        if not chat_arm:
            return (
                "I did not execute the requested action plan ({}). To allow this "
                "Level 1 chat action, check 'Arm Level 1 chat actions' in the "
                "Chat tab and submit the request again."
            ).format(" -> ".join(names))
        results = []
        for index, step in enumerate(plan, start=1):
            action_name = step["action_name"]
            target = step["target"]
            result = step["callback"]()
            entry = {
                "index": index,
                "action_name": action_name,
                "target": target,
                "result": result,
            }
            results.append(entry)
            if isinstance(result, dict) and (result.get("error") or result.get("blocked") or result.get("cancelled")):
                return (
                    "I stopped the Level 1 action plan at step {} ({}). "
                    "No later steps were executed.\n\n```json\n{}\n```"
                ).format(index, action_name, json.dumps(self.jsonable({"steps": results}), indent=2))
        return (
            "Level 1 action plan executed through the chat gate: {}.\n\n"
            "```json\n{}\n```"
        ).format(
            " -> ".join(names),
            json.dumps(self.jsonable({"steps": results}), indent=2),
        )

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
            "level3_extra_confirmation": "EXECUTE LEVEL 3",
            "level3_large_motion_confirmation": "EXECUTE LEVEL 3 LARGE MOTION",
            "level3_thv_base_url": self.config.get("thv_base_url", "http://192.168.40.10/"),
            "level3_max_auto_approach_steps": 10,
            "level3_max_abs_dFreq_Hz": 5.0,
            "level3_max_coarse_burstcount": 5,
            "level3_max_coarse_voltage_V": 100.0,
            "level3_max_large_coarse_burstcount": 5000,
            "level3_coarse_period_s_range": [0.05, 2.0],
        }

    def read_monitors(self):
        try:
            rec = self.runner.get_live_tip_position_monitor()
            data = self.runner.data.get("live_tip_position") or rec.__dict__
            return self.jsonable(data), self.status_text()
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}, self.status_text()

    def read_xyz_monitor(self):
        try:
            data = self.runner.read_xyz_monitor()
            monitor = data.get("monitor") or [None, None, None]
            position = self.xyz_monitor_position_context(monitor)
            report = {
                "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                "source": "gxsm4process.rt_query_xyz",
                "unit": "Volts, not Angstroms",
                "XYZ_monitor": data,
                "position_context": position,
            }
            return (
                monitor[0] if len(monitor) > 0 else None,
                monitor[1] if len(monitor) > 1 else None,
                monitor[2] if len(monitor) > 2 else None,
                self.jsonable(report),
            )
        except Exception as exc:
            report = {"error": str(exc), "traceback": traceback.format_exc()}
            return None, None, None, report

    def read_fast_live_dashboard(self, gain_x_V=12.0, gain_y_V=12.0, gain_z_V=24.0):
        try:
            gains = {
                "x": max(abs(float(gain_x_V)), 1e-9),
                "y": max(abs(float(gain_y_V)), 1e-9),
                "z": max(abs(float(gain_z_V)), 1e-9),
            }
            xyz = self.runner.read_xyz_monitor()
            rpspmc = self.runner.read_rpspmc_monitor()
            monitor = xyz.get("monitor") or [None, None, None]
            values = {
                "x_monitor_V": monitor[0] if len(monitor) > 0 else None,
                "y_monitor_V": monitor[1] if len(monitor) > 1 else None,
                "z_monitor_V": monitor[2] if len(monitor) > 2 else None,
                "bias_V": rpspmc.get("bias"),
                "current_nA": rpspmc.get("current"),
                "gvp_u_V": (rpspmc.get("gvp") or {}).get("u"),
                "pac_ampl_mV": (rpspmc.get("pac") or {}).get("ampl"),
                "pac_dfreq_Hz": (rpspmc.get("pac") or {}).get("dfreq"),
                "rpspmc_time_s": rpspmc.get("time"),
            }
            values["x_effective_piezo_V"] = self.mul_or_none(values["x_monitor_V"], gains["x"])
            values["y_effective_piezo_V"] = self.mul_or_none(values["y_monitor_V"], gains["y"])
            values["z_effective_piezo_V"] = self.mul_or_none(values["z_monitor_V"], gains["z"])
            html = self.live_monitor_html(values, gains)
            report = {
                "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                "units": {
                    "xyz_monitor": "Volts, not Angstroms",
                    "xyz_effective_piezo": "Volts after external amplifier gain",
                    "bias": "V",
                    "current": "nA",
                    "gvp_u": "V",
                    "pac_ampl": "mV",
                    "pac_dfreq": "Hz",
                },
                "xyz_external_amplifier_gain": gains,
                "values": values,
                "XYZ_monitor": xyz,
                "rpspmc": rpspmc,
            }
            return (
                values["x_monitor_V"],
                values["y_monitor_V"],
                values["z_monitor_V"],
                html,
                self.jsonable(report),
            )
        except Exception as exc:
            report = {"error": str(exc), "traceback": traceback.format_exc()}
            return None, None, None, self.live_monitor_html({}, {}), report

    def mul_or_none(self, value, factor):
        try:
            if value is None:
                return None
            return float(value) * float(factor)
        except Exception:
            return None

    def fmt6(self, value):
        if value is None:
            return "n/a"
        try:
            if not np.isfinite(float(value)):
                return "n/a"
            return "{:.6g}".format(float(value))
        except Exception:
            return "n/a"

    def gauge_row(self, label, value, unit, limit, bipolar=True, danger_note=""):
        try:
            v = float(value)
            finite = np.isfinite(v)
        except Exception:
            v = 0.0
            finite = False
        limit = max(abs(float(limit or 1.0)), 1e-9)
        if bipolar:
            pct = 50.0 + 50.0 * max(-1.0, min(1.0, v / limit))
            zero = 50.0
            left = min(zero, pct)
            width = abs(pct - zero)
            scale = "+/-{} {}".format(self.fmt6(limit), unit)
        else:
            pct = 100.0 * max(0.0, min(1.0, v / limit))
            left = 0.0
            width = pct
            zero = 0.0
            scale = "0..{} {}".format(self.fmt6(limit), unit)
        color = "#2f80ed" if finite else "#888"
        if finite and abs(v) >= 0.9 * limit:
            color = "#c62828"
        value_text = "{} {}".format(self.fmt6(value), unit)
        note = (
            "<div style='font-size:11px;color:#b00020;margin-top:2px'>{}</div>".format(danger_note)
            if danger_note else ""
        )
        return """
<div style="display:grid;grid-template-columns:150px 105px 1fr 95px;gap:8px;align-items:center;margin:5px 0;font-family:system-ui,-apple-system,Segoe UI,sans-serif">
  <div style="font-weight:600">{label}</div>
  <div style="font-variant-numeric:tabular-nums">{value}</div>
  <div style="position:relative;height:14px;background:#e7e7e7;border:1px solid #c9c9c9;border-radius:3px;overflow:hidden">
    <div style="position:absolute;left:{left:.3f}%;width:{width:.3f}%;height:100%;background:{color};opacity:.85"></div>
    <div style="position:absolute;left:{zero:.3f}%;width:1px;height:100%;background:#333;opacity:.75"></div>
  </div>
  <div style="font-size:11px;color:#555">{scale}</div>
</div>
{note}
""".format(
            label=label,
            value=value_text,
            left=left,
            width=width,
            zero=zero,
            color=color,
            scale=scale,
            note=note,
        )

    def xy_position_html(self, values, raw_limit_V=10.0):
        limit = max(abs(float(raw_limit_V)), 1e-9)
        try:
            x = float(values.get("x_monitor_V"))
            y = float(values.get("y_monitor_V"))
            finite = np.isfinite(x) and np.isfinite(y)
        except Exception:
            x = 0.0
            y = 0.0
            finite = False
        xpct = 50.0 + 50.0 * max(-1.0, min(1.0, x / limit))
        ypct = 50.0 - 50.0 * max(-1.0, min(1.0, y / limit))
        color = "#1b7f3a" if finite else "#777"
        return """
<div style="border:1px solid #d0d0d0;border-radius:6px;padding:10px;background:#fff;margin-top:10px">
  <div style="font-weight:700;margin-bottom:6px">Live XY controller-voltage position</div>
  <div style="font-size:12px;color:#555;margin-bottom:8px">
    Crosshair uses raw X/Y monitor Volts from <code>rt_query_xyz()</code>,
    normalized to +/-{limit} V. External amplifier gains are not applied here.
  </div>
  <div style="display:grid;grid-template-columns:190px 1fr;gap:12px;align-items:center">
    <div style="font-variant-numeric:tabular-nums;font-size:13px">
      X raw: {x_text} V<br>
      Y raw: {y_text} V<br>
      X piezo: {xe_text} V<br>
      Y piezo: {ye_text} V
    </div>
    <div style="position:relative;width:180px;height:180px;background:#f7f7f7;border:1px solid #aaa">
      <div style="position:absolute;left:50%;top:0;width:1px;height:100%;background:#bbb"></div>
      <div style="position:absolute;left:0;top:50%;width:100%;height:1px;background:#bbb"></div>
      <div style="position:absolute;left:{xpct:.3f}%;top:{ypct:.3f}%;width:12px;height:12px;margin-left:-6px;margin-top:-6px;border-radius:50%;background:{color};box-shadow:0 0 0 2px white,0 0 0 3px #444"></div>
      <div style="position:absolute;left:4px;top:3px;font-size:10px;color:#555">+Y</div>
      <div style="position:absolute;right:4px;top:84px;font-size:10px;color:#555">+X</div>
    </div>
  </div>
</div>
""".format(
            limit=self.fmt6(limit),
            x_text=self.fmt6(values.get("x_monitor_V")),
            y_text=self.fmt6(values.get("y_monitor_V")),
            xe_text=self.fmt6(values.get("x_effective_piezo_V")),
            ye_text=self.fmt6(values.get("y_effective_piezo_V")),
            xpct=xpct,
            ypct=ypct,
            color=color,
        )

    def live_monitor_html(self, values, gains):
        x_label = "X piezo"
        y_label = "Y piezo"
        z_label = "Z piezo"
        if values.get("x_monitor_V") is not None:
            x_label += " (raw {} V)".format(self.fmt6(values.get("x_monitor_V")))
        if values.get("y_monitor_V") is not None:
            y_label += " (raw {} V)".format(self.fmt6(values.get("y_monitor_V")))
        if values.get("z_monitor_V") is not None:
            z_label += " (raw {} V)".format(self.fmt6(values.get("z_monitor_V")))
        rows = [
            self.gauge_row(x_label, values.get("x_effective_piezo_V"), "V", 100.0, bipolar=True),
            self.gauge_row(y_label, values.get("y_effective_piezo_V"), "V", 100.0, bipolar=True),
            self.gauge_row(z_label, values.get("z_effective_piezo_V"), "V", 100.0, bipolar=True),
            self.gauge_row("Bias", values.get("bias_V"), "V", 5.0, bipolar=True),
            self.gauge_row("Current", values.get("current_nA"), "nA", 0.1, bipolar=True),
            self.gauge_row("GVP U", values.get("gvp_u_V"), "V", 5.0, bipolar=True),
            self.gauge_row("PAC ampl", values.get("pac_ampl_mV"), "mV", 10.0, bipolar=True),
            self.gauge_row("PAC dFreq", values.get("pac_dfreq_Hz"), "Hz", 20.0, bipolar=True),
        ]
        xy_panel = self.xy_position_html(values)
        return """
<div style="border:1px solid #d0d0d0;border-radius:6px;padding:10px;background:#fafafa">
  <div style="font-weight:700;margin-bottom:6px">Fast live monitor</div>
  <div style="font-size:12px;color:#555;margin-bottom:8px">
    XYZ values are controller Volts from <code>rt_query_xyz()</code>, not Angstroms.
    X/Y/Z gauge values are effective piezo Volts after multiplying by the external amplifier gain.
    Bias/current/GVP/PAC values are from <code>rt_query_rpspmc()</code>.
    Numbers use 6 significant digits.
  </div>
  {rows}
  {xy_panel}
</div>
""".format(rows="\n".join(rows), xy_panel=xy_panel)

    def xyz_monitor_position_context(self, monitor):
        context = {
            "note": (
                "XYZ monitor values are controller Volts. X/Y Angstrom context "
                "is estimated by comparing OffsetX/Y + ScanX/Y to monitor X/Y."
            )
        }
        try:
            settings = self.runner._scan_settings(0)
            x_A = float(settings.get("OffsetX", 0.0)) + float(settings.get("ScanX", 0.0))
            y_A = float(settings.get("OffsetY", 0.0)) + float(settings.get("ScanY", 0.0))
            context["gxsm_position_A"] = {
                "world_x_A_approx": x_A,
                "world_y_A_approx": y_A,
                "offset_x_A": settings.get("OffsetX"),
                "offset_y_A": settings.get("OffsetY"),
                "scan_x_A": settings.get("ScanX"),
                "scan_y_A": settings.get("ScanY"),
                "rotation_deg": settings.get("Rotation"),
            }
            if monitor and len(monitor) >= 2:
                vx = float(monitor[0])
                vy = float(monitor[1])
                context["instant_scale_estimate_A_per_V"] = {
                    "x": None if abs(vx) < 1e-12 else x_A / vx,
                    "y": None if abs(vy) < 1e-12 else y_A / vy,
                    "caution": (
                        "Instant ratio is only a rough estimate unless the "
                        "monitor zero/origin and configured piezo scale are known."
                    ),
                }
        except Exception as exc:
            context["position_context_error"] = str(exc)
        return context

    def read_rpspmc_monitor(self):
        try:
            return self.jsonable(self.runner.read_rpspmc_monitor())
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def sample_dfrequency(self, count, delay_s):
        try:
            report = self.activity.sample_dfrequency(
                count=int(count),
                delay_s=float(delay_s),
            )
            return self.jsonable(report)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def require_level3_confirmation(self, confirm_text, arm, action_name):
        blocked = self.require_control_level(3, action_name)
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, action_name)
        if cancelled:
            return cancelled
        phrase = self.safety_limits()["level3_extra_confirmation"]
        if str(confirm_text).strip() != phrase:
            return {"cancelled": "Type '{}' to {}.".format(phrase, action_name)}
        return None

    def require_level3_large_motion_confirmation(self, confirm_text, arm, action_name):
        blocked = self.require_control_level(3, action_name)
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, action_name)
        if cancelled:
            return cancelled
        phrase = self.safety_limits()["level3_large_motion_confirmation"]
        if str(confirm_text).strip() != phrase:
            return {"cancelled": "Type '{}' to {}.".format(phrase, action_name)}
        return None

    def level3_telemetry(self):
        try:
            return self.jsonable(self.level3.telemetry())
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def level3_set_script_control(self, enabled, confirm_text, arm):
        blocked = self.require_level3_confirmation(
            confirm_text,
            arm,
            "set script-control",
        )
        if blocked:
            return blocked
        try:
            rec = self.level3.set_script_control(bool(enabled))
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def level3_coarse_z_down(self, burstcount, confirm_text, arm):
        blocked = self.require_level3_confirmation(
            confirm_text,
            arm,
            "execute coarse Z down",
        )
        if blocked:
            return blocked
        try:
            rec = self.level3.coarse_move(
                "Z",
                -1,
                int(burstcount),
                period_s=0.5,
                voltage_V=30.0,
                max_burstcount=5,
                max_voltage_V=60.0,
            )
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def level3_coarse_move(
        self,
        channel,
        direction,
        burstcount,
        period_s,
        voltage_V,
        confirm_text,
        arm,
    ):
        blocked = self.require_level3_confirmation(
            confirm_text,
            arm,
            "execute THV coarse move",
        )
        if blocked:
            return blocked
        try:
            channel = str(channel).upper()
            if channel not in ("X", "Y", "Z"):
                raise ValueError("Channel must be X, Y, or Z.")
            direction_text = str(direction).strip()
            direction_value = -1 if direction_text.startswith("-") else 1
            limits = self.safety_limits()
            period_min, period_max = limits["level3_coarse_period_s_range"]
            if not (float(period_min) <= float(period_s) <= float(period_max)):
                raise ValueError(
                    "Coarse period must be within {}..{} s.".format(
                        period_min,
                        period_max,
                    )
                )
            rec = self.level3.coarse_move(
                channel,
                direction_value,
                int(burstcount),
                period_s=float(period_s),
                voltage_V=float(voltage_V),
                max_burstcount=limits["level3_max_coarse_burstcount"],
                max_voltage_V=limits["level3_max_coarse_voltage_V"],
            )
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def level3_large_coarse_move(
        self,
        channel,
        direction,
        burstcount,
        voltage_V,
        confirm_text,
        arm,
    ):
        blocked = self.require_level3_large_motion_confirmation(
            confirm_text,
            arm,
            "execute large THV coarse move",
        )
        if blocked:
            return blocked
        try:
            channel = str(channel).upper()
            if channel not in ("X", "Y", "Z"):
                raise ValueError("Channel must be X, Y, or Z.")
            direction_text = str(direction).strip()
            direction_value = -1 if direction_text.startswith("-") else 1
            limits = self.safety_limits()
            rec = self.level3.coarse_move(
                channel,
                direction_value,
                int(burstcount),
                period_s=0.5,
                voltage_V=float(voltage_V),
                max_burstcount=limits["level3_max_large_coarse_burstcount"],
                max_voltage_V=limits["level3_max_coarse_voltage_V"],
            )
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def level3_auto_approach(
        self,
        current_nA,
        coarse_steps_per_cycle,
        max_total_steps,
        max_abs_dFreq_Hz,
        check_timeout_s,
        confirm_text,
        arm,
    ):
        blocked = self.require_level3_confirmation(
            confirm_text,
            arm,
            "execute protected auto approach",
        )
        if blocked:
            return blocked
        try:
            max_steps_limit = self.safety_limits()["level3_max_auto_approach_steps"]
            df_limit = self.safety_limits()["level3_max_abs_dFreq_Hz"]
            if int(max_total_steps) > int(max_steps_limit):
                raise ValueError(
                    "max_total_steps exceeds Level-3 GUI limit {}.".format(
                        max_steps_limit,
                    )
                )
            if float(max_abs_dFreq_Hz) > float(df_limit):
                raise ValueError(
                    "max_abs_dFreq_Hz exceeds Level-3 GUI limit {}.".format(
                        df_limit,
                    )
                )
            prefix = self.output_dir / "gui_level3_auto_approach"
            rec = self.level3.run_auto_approach(
                current_nA=float(current_nA),
                coarse_steps_per_cycle=int(coarse_steps_per_cycle),
                max_total_steps=int(max_total_steps),
                max_abs_dFreq_Hz=float(max_abs_dFreq_Hz),
                check_timeout_s=float(check_timeout_s),
                output_prefix=str(prefix),
            )
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

    def set_rotation(self, rotation_deg, arm):
        blocked = self.require_control_level(1, "set scan rotation")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "set scan rotation")
        if cancelled:
            return cancelled
        try:
            rotation = float(rotation_deg)
            if not (-360.0 <= rotation <= 360.0):
                raise ValueError("Rotation must be within -360..360 degrees for Level 1.")
            rec = self.runner.set_parameter("Rotation", rotation)
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def set_scan_geometry_and_rotation(self, range_A, points, rotation_deg, arm):
        blocked = self.require_control_level(1, "set scan geometry and rotation")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "set scan geometry and rotation")
        if cancelled:
            return cancelled
        geometry = self.set_scan_geometry(range_A, points, arm=True)
        if isinstance(geometry, dict) and (geometry.get("error") or geometry.get("blocked") or geometry.get("cancelled")):
            return {"geometry": geometry, "rotation": None}
        rotation = self.set_rotation(rotation_deg, arm=True)
        return {"geometry": geometry, "rotation": rotation}

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
                rpspmc_btn = gr.Button("Read RPSPMC Monitor")
            with gr.Row():
                df_count = gr.Number(value=20, label="dFreq samples", precision=0)
                df_delay = gr.Number(value=0.15, label="Delay per sample (s)")
            gr.Markdown(
                "Fast XYZ monitor updates at 5 Hz from "
                "`gxsm4process.rt_query_xyz()` and displays "
                "`XYZ_monitor['monitor']` in controller Volts, not Angstroms. "
                "The X/Y/Z gain fields are external amplifier factors used to "
                "compute effective piezo voltage from controller output voltage."
            )
            with gr.Row():
                gain_choices = ["1", "3", "6", "12", "24"]
                xyz_gain_x = gr.Dropdown(
                    choices=gain_choices,
                    value="12",
                    label="X external amplifier gain",
                )
                xyz_gain_y = gr.Dropdown(
                    choices=gain_choices,
                    value="12",
                    label="Y external amplifier gain",
                )
                xyz_gain_z = gr.Dropdown(
                    choices=gain_choices,
                    value="24",
                    label="Z external amplifier gain",
                )
            with gr.Row():
                xyz_x = gr.Number(label="X monitor (V)", interactive=False)
                xyz_y = gr.Number(label="Y monitor (V)", interactive=False)
                xyz_z = gr.Number(label="Z monitor (V)", interactive=False)
            live_gauges = gr.HTML(label="Fast monitor gauges")
            monitors = gr.JSON(label="Monitor readback")
            dfreq = gr.JSON(label="dFrequency report")
            xyz_report = gr.JSON(label="Fast XYZ monitor report")
            rpspmc_report = gr.JSON(label="RPSPMC monitor snapshot")
            xyz_timer = gr.Timer(value=0.2, active=True)
            xyz_timer.tick(
                backend.read_fast_live_dashboard,
                inputs=[xyz_gain_x, xyz_gain_y, xyz_gain_z],
                outputs=[xyz_x, xyz_y, xyz_z, live_gauges, xyz_report],
            )
            read_btn.click(backend.read_monitors, outputs=[monitors, status])
            df_btn.click(backend.sample_dfrequency, [df_count, df_delay], dfreq)
            rpspmc_btn.click(backend.read_rpspmc_monitor, outputs=rpspmc_report)

        with gr.Tab("Scan Image"):
            with gr.Row():
                scan_channel = gr.Number(value=0, label="Channel", precision=0)
                scan_lines = gr.Number(value=120, label="Recent/full lines to fetch", precision=0)
                scan_auto = gr.Checkbox(value=False, label="Auto refresh")
                fetch_btn = gr.Button("Fetch And Plot")
            scan_plot = gr.Plot(label="Scan image")
            scan_profile = gr.Plot(label="Last scan-line profile")
            scan_meta = gr.JSON(label="Scan metadata")
            scan_refresh_timer = gr.Timer(value=1.0, active=True)
            fetch_btn.click(
                backend.fetch_scan_plot_with_profile,
                [scan_channel, scan_lines],
                [scan_plot, scan_profile, scan_meta],
            )
            scan_refresh_timer.tick(
                backend.auto_refresh_scan_plot,
                [scan_auto, scan_channel, scan_lines],
                [scan_plot, scan_profile, scan_meta],
            )

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

        with gr.Tab("Tip Tune Planner"):
            gr.Markdown(
                "Planner workflow: analyze the current scan, mark flat work-site "
                "candidates and blobs, optionally move the stopped tip to a "
                "candidate, then run bounded scan/action/reassess cycles. "
                "ScanX/Y moves are refused while scanning."
            )
            planner_arm = gr.Checkbox(value=False, label="Arm one planner live action")
            planner_report = gr.JSON(label="Planner report")
            planner_plot = gr.Plot(label="Planner annotated image")
            planner_progress = gr.Plot(label="Planner progress")
            with gr.Accordion("Analyze And Select Work Site", open=True):
                with gr.Row():
                    planner_channel = gr.Number(value=0, label="Channel", precision=0)
                    planner_lines = gr.Number(value=180, label="Lines to fetch", precision=0)
                    planner_patch = gr.Number(value=80.0, label="Flat patch size (A)")
                    planner_candidates = gr.Number(value=12, label="Max candidates", precision=0)
                with gr.Row():
                    planner_max_blobs = gr.Number(value=8, label="Stop/search if blobs >=", precision=0)
                    planner_prefix = gr.Textbox(value="gui_tip_planner", label="Output prefix")
                    planner_analyze_btn = gr.Button("Analyze Tip And Flat Candidates")
                planner_analyze_btn.click(
                    backend.tip_planner_analyze_current_scan,
                    [
                        planner_channel,
                        planner_lines,
                        planner_patch,
                        planner_candidates,
                        planner_max_blobs,
                        planner_prefix,
                    ],
                    [planner_plot, planner_progress, planner_report],
                )
                with gr.Row():
                    planner_candidate_index = gr.Number(value=1, label="Candidate #", precision=0)
                    planner_move_btn = gr.Button("Move Tip To Candidate")
                planner_move_btn.click(
                    backend.tip_planner_move_to_candidate,
                    [planner_candidate_index, planner_arm],
                    planner_report,
                )

            with gr.Accordion("Offset Search For Cleaner Area", open=False):
                gr.Markdown(
                    "Plan or apply a partially overlapping OffsetX/Y shift if the "
                    "current frame contains too many large blobs. Offset moves "
                    "use non-rotated world coordinates and stop the scan first."
                )
                with gr.Row():
                    shift_direction = gr.Dropdown(
                        choices=[
                            "image_left",
                            "image_right",
                            "image_up",
                            "image_down",
                            "image_left_below",
                            "image_right_below",
                            "image_left_above",
                            "image_right_above",
                        ],
                        value="image_left_below",
                        label="Search direction",
                    )
                    shift_range = gr.Number(value=800.0, label="New scan range (A)")
                    shift_overlap = gr.Number(value=250.0, label="Overlap (A)")
                    shift_points = gr.Number(value=400, label="Points", precision=0)
                with gr.Row():
                    plan_shift_btn = gr.Button("Plan Offset Shift")
                    start_after_shift = gr.Checkbox(value=True, label="Start scan after shift")
                    apply_shift_btn = gr.Button("Apply Offset Shift")
                plan_shift_btn.click(
                    backend.tip_planner_plan_offset_shift,
                    [shift_direction, shift_range, shift_overlap],
                    planner_report,
                )
                apply_shift_btn.click(
                    backend.tip_planner_apply_offset_shift,
                    [
                        shift_direction,
                        shift_range,
                        shift_overlap,
                        shift_points,
                        start_after_shift,
                        planner_arm,
                    ],
                    planner_report,
                )

            with gr.Accordion("Automated Tip-Improvement Loop", open=False):
                gr.Markdown(
                    "Runs scan, waits for enough lines, assesses tip and dFreq, "
                    "moves to a clean candidate after stopping the scan, executes "
                    "a gentle pulse/tune action, and repeats. Stops when quality "
                    "and dFreq are satisfactory, when blob count is too high, or "
                    "when max cycles is reached. Requires exact confirmation."
                )
                with gr.Row():
                    loop_cycles = gr.Number(value=2, label="Max cycles", precision=0)
                    loop_min_lines = gr.Number(value=140, label="Minimum evidence lines", precision=0)
                    loop_range = gr.Number(value=700.0, label="Diagnostic range (A)")
                    loop_points = gr.Number(value=400, label="Points", precision=0)
                with gr.Row():
                    loop_max_blobs = gr.Number(value=8, label="Stop/search if blobs >=", precision=0)
                    loop_df_ok = gr.Number(value=4.0, label="Max |dFreq| OK (Hz)")
                    loop_auto_shift = gr.Checkbox(value=False, label="Auto shift on too many blobs")
                loop_confirm = gr.Textbox(
                    label="Loop confirmation",
                    placeholder="Type EXECUTE TIP LOOP",
                )
                loop_btn = gr.Button("Run Tip Tune Planner Loop")
                loop_btn.click(
                    backend.run_tip_planner_loop,
                    [
                        loop_cycles,
                        planner_channel,
                        loop_min_lines,
                        loop_range,
                        loop_points,
                        loop_max_blobs,
                        loop_df_ok,
                        loop_auto_shift,
                        shift_direction,
                        loop_confirm,
                        planner_arm,
                    ],
                    [planner_plot, planner_progress, planner_report],
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

        with gr.Tab("GVP"):
            gr.Markdown(
                "GVP load actions are Level 1 and arm-gated. Execution additionally "
                "requires typing `EXECUTE GVP`. Tip-tune Z-dip uses the captured "
                "`gvp_tip_tune_template_current_latest.json` template and rewrites "
                "all vector values/FB flags before loading."
            )
            gvp_arm = gr.Checkbox(value=False, label="Arm one GVP action")
            gvp_report = gr.JSON(label="GVP result")
            with gr.Accordion("Bias Pulse GVP", open=True):
                with gr.Row():
                    pulse_du = gr.Number(value=2.0, label="Bias pulse du (V)")
                    load_pulse_btn = gr.Button("Load Bias Pulse GVP Only")
                load_pulse_btn.click(
                    backend.load_bias_pulse,
                    [pulse_du, gvp_arm],
                    gvp_report,
                )
            with gr.Accordion("Tip Tune GVP: Z Dip", open=True):
                gr.Markdown(
                    "Gentle starting point: contact bias 0 V, dip depth 5 A. "
                    "Dip depth is entered as a positive magnitude; the GVP program "
                    "uses a negative Z step for indentation and a positive pull-out. "
                    "Vec 1 removes the actual scan bias, vec 3 applies the contact "
                    "bias, vec 6 removes contact bias, and vec 7 restores scan bias."
                )
                with gr.Row():
                    tip_contact_bias = gr.Number(value=0.0, label="Contact bias (V)")
                    tip_dip_depth = gr.Number(value=5.0, label="Dip depth magnitude (A)")
                    load_tip_tune_btn = gr.Button("Load Tip Tune Z-Dip GVP")
                load_tip_tune_btn.click(
                    backend.load_tip_tune_gvp,
                    [tip_contact_bias, tip_dip_depth, gvp_arm],
                    gvp_report,
                )
            with gr.Accordion("Execute Loaded GVP", open=True):
                gvp_confirm = gr.Textbox(
                    label="GVP execute confirmation",
                    placeholder="Type EXECUTE GVP to run loaded GVP",
                )
                execute_gvp_btn = gr.Button("Execute Loaded GVP And Plot")
                gvp_plot = gr.Plot(label="GVP VP data: ZS, current, dFreq")
                execute_gvp_btn.click(
                    backend.execute_loaded_gvp_with_plot,
                    [gvp_confirm, gvp_arm],
                    [gvp_plot, gvp_report],
                )

        with gr.Tab("Level 3 Protected"):
            gr.Markdown(
                "Extreme actions: coarse motion and auto approach. These require "
                "Control Level 3, the Level 3 arm checkbox, and exact typed "
                "confirmation. Keep `script-control` enabled only while actively "
                "supervising; set it to 0 to abort watchdog-style loops."
            )
            level3_report = gr.JSON(label="Level 3 result")
            with gr.Row():
                level3_arm = gr.Checkbox(value=False, label="Arm Level 3 action")
                level3_confirm = gr.Textbox(
                    label="Level 3 confirmation",
                    placeholder="Type EXECUTE LEVEL 3",
                )
                level3_tel_btn = gr.Button("Read Level 3 Telemetry")
            level3_tel_btn.click(backend.level3_telemetry, outputs=level3_report)
            with gr.Row():
                script_control = gr.Checkbox(value=True, label="script-control enabled")
                script_control_btn = gr.Button("Set script-control")
            script_control_btn.click(
                backend.level3_set_script_control,
                [script_control, level3_confirm, level3_arm],
                level3_report,
            )
            gr.Markdown(
                "Coarse Z-down uses THV5 Z, direction -1, period 0.5 s, 30 V. "
                "Burstcount is capped at 5 in this GUI. Z direction -1 is "
                "DANGEROUS: fast tip-down coarse motion."
            )
            with gr.Row():
                coarse_burstcount = gr.Number(value=1, label="Z-down burstcount", precision=0)
                coarse_z_btn = gr.Button("Execute Coarse Z Down")
            coarse_z_btn.click(
                backend.level3_coarse_z_down,
                [coarse_burstcount, level3_confirm, level3_arm],
                level3_report,
            )
            gr.Markdown(
                "Generic THV coarse move. Direction signs are controller signs; "
                "use only with an explicit operator coordinate plan. Normal "
                "coarse moves are capped at burstcount 5 and 100 HV V."
            )
            with gr.Row():
                coarse_channel = gr.Dropdown(choices=["X", "Y", "Z"], value="Z", label="Channel")
                coarse_direction = gr.Dropdown(
                    choices=["-1", "+1"],
                    value="-1",
                    label="Direction",
                )
                coarse_generic_burst = gr.Number(value=1, label="Burstcount", precision=0)
            with gr.Row():
                coarse_period = gr.Number(value=0.5, label="Period (s)")
                coarse_voltage = gr.Number(value=30.0, label="Voltage (HV V)")
                coarse_generic_btn = gr.Button("Execute Generic Coarse Move")
            coarse_generic_btn.click(
                backend.level3_coarse_move,
                [
                    coarse_channel,
                    coarse_direction,
                    coarse_generic_burst,
                    coarse_period,
                    coarse_voltage,
                    level3_confirm,
                    level3_arm,
                ],
                level3_report,
            )
            gr.Markdown(
                "Extra-protected large/fast coarse motion. Period is fixed at "
                "0.5 s. Burstcount can be up to 5000 and voltage up to 100 HV V. "
                "This requires the separate phrase `EXECUTE LEVEL 3 LARGE MOTION`. "
                "Z direction -1 is DANGEROUS: fast tip-down coarse motion."
            )
            with gr.Row():
                large_confirm = gr.Textbox(
                    label="Large-motion confirmation",
                    placeholder="Type EXECUTE LEVEL 3 LARGE MOTION",
                )
            with gr.Row():
                large_channel = gr.Dropdown(choices=["X", "Y", "Z"], value="X", label="Channel")
                large_direction = gr.Dropdown(
                    choices=["-1", "+1"],
                    value="+1",
                    label="Direction",
                )
                large_burst = gr.Number(value=100, label="Burstcount", precision=0)
            with gr.Row():
                large_voltage = gr.Number(value=30.0, label="Voltage (HV V)")
                large_btn = gr.Button("Execute Large Coarse Move")
            large_btn.click(
                backend.level3_large_coarse_move,
                [
                    large_channel,
                    large_direction,
                    large_burst,
                    large_voltage,
                    large_confirm,
                    level3_arm,
                ],
                level3_report,
            )
            gr.Markdown(
                "Protected auto approach follows the watchdog pattern: set current, "
                "check Z range, retract, apply one or more coarse Z-down bursts, "
                "abort on script-control=0, dFreq limit, or max steps."
            )
            with gr.Row():
                approach_current = gr.Number(value=0.013, label="Approach current (nA)")
                approach_steps = gr.Number(value=1, label="Coarse steps/cycle", precision=0)
                approach_max_steps = gr.Number(value=3, label="Max total steps", precision=0)
            with gr.Row():
                approach_df_limit = gr.Number(value=5.0, label="Max abs dFreq (Hz)")
                approach_timeout = gr.Number(value=30.0, label="Check timeout (s)")
                approach_btn = gr.Button("Execute Protected Auto Approach")
            approach_btn.click(
                backend.level3_auto_approach,
                [
                    approach_current,
                    approach_steps,
                    approach_max_steps,
                    approach_df_limit,
                    approach_timeout,
                    level3_confirm,
                    level3_arm,
                ],
                level3_report,
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
            with gr.Accordion("Scan Geometry", open=True):
                gr.Markdown(
                    "Sets square RangeX/RangeY and PointsX/PointsY. "
                    "StepsX/Y are informational only: Range/(Points-1). "
                    "Rotation is in degrees and may be changed live with caution."
                )
                with gr.Row():
                    geometry_range = gr.Number(value=700.0, label="Scan range X/Y (A)")
                    geometry_points = gr.Number(value=400, label="Scan points X/Y", precision=0)
                    geometry_rotation = gr.Number(value=90.0, label="Rotation (deg)")
                with gr.Row():
                    set_geometry_btn = gr.Button("Set Range And Points")
                    set_rotation_btn = gr.Button("Set Rotation")
                    set_geometry_rotation_btn = gr.Button("Set Geometry And Rotation")
            with gr.Row():
                cp_dB = gr.Number(value=-90.0, label="Feedback CP (dB)")
                ci_dB = gr.Number(value=-90.0, label="Feedback CI (dB)")
                set_feedback_btn = gr.Button("Set Feedback CP/CI")
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
            set_geometry_btn.click(
                backend.set_scan_geometry,
                [geometry_range, geometry_points, arm],
                action_report,
            )
            set_rotation_btn.click(
                backend.set_rotation,
                [geometry_rotation, arm],
                action_report,
            )
            set_geometry_rotation_btn.click(
                backend.set_scan_geometry_and_rotation,
                [geometry_range, geometry_points, geometry_rotation, arm],
                action_report,
            )
            set_feedback_btn.click(
                backend.set_feedback_cp_ci,
                [cp_dB, ci_dB, arm],
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
    parser.add_argument(
        "--https",
        action="store_true",
        help="Enable HTTPS using the default local certificate/key paths.",
    )
    parser.add_argument("--ssl-certfile", default="")
    parser.add_argument("--ssl-keyfile", default="")
    parser.add_argument(
        "--auth-user",
        default=DEFAULT_AUTH_USER,
        help="Username for the optional microscope GUI login.",
    )
    parser.add_argument(
        "--auth-password-env",
        default=DEFAULT_AUTH_PASSWORD_ENV,
        help="Environment variable containing the GUI login password.",
    )
    parser.add_argument(
        "--auth-password-file",
        default=str(DEFAULT_AUTH_PASSWORD_FILE),
        help="Local file containing the GUI login password.",
    )
    parser.add_argument(
        "--require-auth",
        action="store_true",
        help="Fail startup unless a GUI login password is available.",
    )
    args = parser.parse_args(argv)

    os.chdir(SCRIPT_DIR)
    config = load_config(args.config)
    if args.model:
        config["model"] = args.model
    backend = MicroscopeGradioBackend(config, live=args.live)
    demo = build_ui(backend)
    launch_kwargs = {
        "server_name": args.host,
        "server_port": args.port,
        "share": args.share,
        "show_error": True,
    }
    ssl_certfile = args.ssl_certfile or (DEFAULT_SSL_CERTFILE if args.https else "")
    ssl_keyfile = args.ssl_keyfile or (DEFAULT_SSL_KEYFILE if args.https else "")
    if ssl_certfile or ssl_keyfile:
        if not (ssl_certfile and ssl_keyfile):
            raise SystemExit("HTTPS requires both --ssl-certfile and --ssl-keyfile.")
        if not os.access(ssl_certfile, os.R_OK):
            raise SystemExit("SSL certfile is not readable: {}".format(ssl_certfile))
        if not os.access(ssl_keyfile, os.R_OK):
            raise SystemExit("SSL keyfile is not readable: {}".format(ssl_keyfile))
        launch_kwargs["ssl_certfile"] = ssl_certfile
        launch_kwargs["ssl_keyfile"] = ssl_keyfile
        launch_kwargs["ssl_verify"] = False
    auth, auth_report = resolve_gradio_auth(
        username=args.auth_user,
        password_env=args.auth_password_env,
        password_file=args.auth_password_file,
        require_auth=args.require_auth,
    )
    if auth is not None:
        launch_kwargs["auth"] = auth
        print(
            "Microscope GUI login enabled for user '{}' from {}.".format(
                auth_report["username"],
                auth_report["password_source"],
            )
        )
    elif args.host not in ("127.0.0.1", "localhost"):
        print(
            "WARNING: Microscope GUI authentication is disabled while binding "
            "to {}. Set {} or create {} and use --require-auth for access "
            "control.".format(
                args.host,
                args.auth_password_env,
                args.auth_password_file,
            )
        )
    demo.launch(**launch_kwargs)


if __name__ == "__main__":
    main()
