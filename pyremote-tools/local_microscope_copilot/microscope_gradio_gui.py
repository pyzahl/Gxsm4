#!/usr/bin/python3
"""
Gradio GUI skeleton for the local GXSM4 microscope copilot.

The GUI keeps Ollama chat advisory by default. A small set of deterministic
Level-1 chat commands may be routed through the same explicit safety gates as
the button controls when the operator arms chat actions.
"""

import argparse
import ast
import html
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
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
DEFAULT_CONTROLLER_CONFIG = SCRIPT_DIR / "microscope_controller_config.json"
DEFAULT_GUI_CONFIG = SCRIPT_DIR / "microscope_gui_config.json"
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


EMERGENCY_STOP_CSS = """
#level3_emergency_stop {
    background: #b00020 !important;
    border: 3px solid #ffccd5 !important;
    color: #ffffff !important;
    font-size: 28px !important;
    font-weight: 900 !important;
    min-height: 76px !important;
    letter-spacing: 0 !important;
    box-shadow: 0 0 0 4px rgba(176, 0, 32, 0.18) !important;
}
#level3_emergency_stop:hover {
    background: #d40027 !important;
}
#gvp_emergency_stop {
    background: #b00020 !important;
    border: 3px solid #ffccd5 !important;
    color: #ffffff !important;
    font-size: 22px !important;
    font-weight: 900 !important;
    min-height: 58px !important;
    letter-spacing: 0 !important;
}
#gvp_emergency_stop:hover {
    background: #d40027 !important;
}
"""


def deep_merge_dict(base, override):
    result = dict(base or {})
    for key, value in dict(override or {}).items():
        if (
            isinstance(value, dict)
            and isinstance(result.get(key), dict)
        ):
            result[key] = deep_merge_dict(result[key], value)
        else:
            result[key] = value
    return result


def load_json_file(path):
    with open(path, "r") as file:
        return json.load(file)


def load_optional_json_file(path):
    path = Path(path)
    if not path.exists():
        return {}
    return load_json_file(path)


def load_config(path, controller_config_path=None, gui_config_path=None):
    controller_config_path = Path(controller_config_path or DEFAULT_CONTROLLER_CONFIG)
    gui_config_path = Path(gui_config_path or DEFAULT_GUI_CONFIG)
    config = {}
    config = deep_merge_dict(config, load_optional_json_file(controller_config_path))
    config = deep_merge_dict(config, load_optional_json_file(gui_config_path))
    config = deep_merge_dict(config, load_json_file(path))
    return config


class MicroscopeGradioBackend(ScanGuiMixin, GvpGuiMixin, TipPlannerGuiMixin):
    def __init__(self, config, live=False, output_dir=None):
        defaults = {}
        defaults = deep_merge_dict(defaults, load_optional_json_file(DEFAULT_CONTROLLER_CONFIG))
        defaults = deep_merge_dict(defaults, load_optional_json_file(DEFAULT_GUI_CONFIG))
        self.config = deep_merge_dict(defaults, dict(config))
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
            thv_base_url=self.controller_default(
                "level3_thv_base_url",
                self.config.get("thv_base_url", "http://192.168.40.10/"),
            ),
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
        self.live_xy_calibration = {
            "x_slope_A_per_V": None,
            "x_intercept_A": None,
            "y_slope_A_per_V": None,
            "y_intercept_A": None,
            "updated_at": None,
            "source": "not calibrated",
            "sample_count": {"x": 0, "y": 0},
        }
        self.live_xy_calibration_samples = {"x": [], "y": []}
        self.instrument_gains_cache = {
            "values": None,
            "updated_at": None,
            "error": None,
        }
        self.init_tip_planner_state()
        self.chat_messages = [{"role": "system", "content": DEFAULT_SYSTEM_PROMPT}]

    def nested_config(self, section, key, default=None):
        value = self.config.get(section, {})
        for part in str(key).split("."):
            if not isinstance(value, dict) or part not in value:
                return default
            value = value[part]
        return value

    def gui_default(self, key, default=None):
        return self.nested_config("gui", key, default)

    def controller_default(self, key, default=None):
        return self.nested_config("controller", key, default)

    def controller_limits(self):
        return dict(self.controller_default("safety_limits", {}))

    def clamp_config_value(self, value, config_key, default_range):
        limits = self.controller_default(config_key, default_range)
        return max(float(limits[0]), min(float(limits[1]), value))

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

    def chat(self, message, history, chat_arm=False, llm_intent_mode=True):
        if not message:
            return "", history
        direct = self.direct_safety_reply(message, chat_arm=chat_arm)
        if direct is not None:
            history = list(history or [])
            history.append({"role": "user", "content": message})
            history.append({"role": "assistant", "content": direct})
            return "", history
        if llm_intent_mode:
            intent_reply = self.try_llm_intent_reply(message, chat_arm=chat_arm)
            if intent_reply is not None:
                history = list(history or [])
                history.append({"role": "user", "content": message})
                history.append({"role": "assistant", "content": intent_reply})
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
            r"\b(set|change|adjust|make|configure|apply|rotate|move|shift)\b",
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

    def try_llm_intent_reply(self, message, chat_arm=False):
        if not self.looks_like_microscope_intent_request(str(message).lower()):
            return None
        try:
            intent = self.request_llm_intent(message)
        except requests.ConnectionError:
            return "Could not connect to Ollama for intent parsing. Start it with: `ollama serve`."
        except Exception as exc:
            return "LLM intent parsing failed safely: {}".format(exc)
        if intent is None:
            return (
                "I could not map that request to a known microscope intent. "
                "No hardware action was executed."
            )
        return self.execute_llm_intent(intent, original_message=message, chat_arm=chat_arm)

    def request_llm_intent(self, message):
        prompt = self.build_llm_intent_prompt(message)
        reply = requests.post(
            self.config.get("ollama_host", "http://127.0.0.1:11434") + "/api/chat",
            json={
                "model": self.config.get("model", "qwen3:4b"),
                "messages": [
                    {
                        "role": "system",
                        "content": (
                            "You convert GXSM4 microscope chat requests into "
                            "strict JSON only. Do not explain. Do not claim "
                            "hardware actions. If unsure, use no_action."
                        ),
                    },
                    {"role": "user", "content": prompt},
                ],
                "stream": False,
                "format": "json",
                "options": {"temperature": 0.0},
            },
            timeout=60,
        )
        reply.raise_for_status()
        content = reply.json()["message"].get("content", "")
        obj = self.parse_llm_intent_json(content)
        if not isinstance(obj, dict):
            return None
        return obj

    def build_llm_intent_prompt(self, message):
        template = """
Return exactly one JSON object with this shape:
{
  "intent": "one_allowed_intent",
  "confidence": 0.0,
  "parameters": {},
  "rationale": "short"
}

Allowed intents:
- set_parameter: parameters {"parameter": "bias|current_setpoint|scan_speed|rotation|offset_x|offset_y|feedback_cp|feedback_ci|slope_x|slope_y", "value": "number with required unit"}
- set_offsets_xy: parameters {"value": "number with length unit"}
- shift_image: parameters {"amount": "number with length unit", "direction": "left|right|up|down"}
- set_scan_geometry: parameters {"range": "number with length unit, optional", "points": integer optional, "rotation": "number deg optional"}
- start_scan: parameters {}
- stop_scan: parameters {}
- read_parameter: parameters {"parameter": "bias|current_setpoint|scan_speed|rotation|offset_x|offset_y|feedback_cp|feedback_ci|slope_x|slope_y|scan_geometry|all"}
- analyze_scan: parameters {}
- load_tip_pulse: parameters {"pulse": "number V optional"}
- load_tip_dip: parameters {"contact_bias": "number V optional", "dip_depth": "number A optional", "ramp_time": "number s optional"}
- no_action: parameters {"reason": "why"}

Rules:
- Preserve user units exactly; do not convert 200 mV to 200 V.
- Physical values must include units. Unitless points/counts are allowed.
- Rotation may be unitless and then means degrees.
- For "set/apply bias", use set_parameter bias.
- For "offset X and Y to 0A", use set_offsets_xy.
- For "shift image/feature left/right/up/down", use shift_image.
- Use confidence below 0.75 if not clear.

User request: __USER_REQUEST__
""".strip()
        return template.replace("__USER_REQUEST__", json.dumps(str(message)))

    def parse_llm_intent_json(self, content):
        text = str(content).strip()
        try:
            return json.loads(text)
        except Exception:
            pass
        start = text.find("{")
        if start < 0:
            return None
        depth = 0
        in_string = False
        escape = False
        for index, char in enumerate(text[start:], start=start):
            if escape:
                escape = False
                continue
            if char == "\\":
                escape = True
                continue
            if char == '"':
                in_string = not in_string
                continue
            if in_string:
                continue
            if char == "{":
                depth += 1
            elif char == "}":
                depth -= 1
                if depth == 0:
                    try:
                        return json.loads(text[start : index + 1])
                    except Exception:
                        return None
        return None

    def execute_llm_intent(self, intent, original_message="", chat_arm=False):
        name = str(intent.get("intent", "")).strip().lower()
        params = intent.get("parameters") or {}
        try:
            confidence = float(intent.get("confidence", 0.0))
        except Exception:
            confidence = 0.0
        if confidence < 0.75 or name in ("", "no_action", "clarify"):
            reason = params.get("reason") if isinstance(params, dict) else ""
            return (
                "LLM intent mode did not execute anything. "
                "The request was not mapped to a known action with enough confidence"
                "{}".format(": {}.".format(reason) if reason else ".")
            )
        if not isinstance(params, dict):
            return "LLM intent mode rejected the response: parameters must be a JSON object."

        try:
            if name == "set_parameter":
                return self.execute_llm_set_parameter_intent(params, chat_arm=chat_arm)
            if name == "set_offsets_xy":
                value = str(params.get("value", "")).strip()
                return self.route_absolute_offset_xy_write(
                    "set offset x and y to {}".format(value),
                    chat_arm=chat_arm,
                )
            if name == "shift_image":
                amount = str(params.get("amount", "")).strip()
                direction = str(params.get("direction", "")).strip().lower()
                return self.route_relative_offset_shift(
                    "shift image {} {}".format(amount, direction),
                    chat_arm=chat_arm,
                )
            if name == "set_scan_geometry":
                return self.execute_llm_scan_geometry_intent(params, chat_arm=chat_arm)
            if name == "start_scan":
                return self.execute_chat_level1_action(
                    "start scan",
                    "startscan",
                    chat_arm,
                    lambda: self.start_scan(arm=True),
                )
            if name == "stop_scan":
                return self.execute_chat_level1_action(
                    "stop scan",
                    "stopscan",
                    chat_arm,
                    lambda: self.stop_scan(arm=True),
                )
            if name == "read_parameter":
                return self.execute_llm_read_parameter_intent(params)
            if name == "analyze_scan":
                return self.try_chat_scan_analysis("analyze current scan")
            if name == "load_tip_pulse":
                pulse = str(params.get("pulse", "")).strip()
                return self.direct_safety_reply(
                    "load tip pulse {}".format(pulse).strip(),
                    chat_arm=chat_arm,
                )
            if name == "load_tip_dip":
                parts = ["load tip tune"]
                if params.get("contact_bias"):
                    parts.append("contact bias {}".format(params["contact_bias"]))
                if params.get("dip_depth"):
                    parts.append("dip {}".format(params["dip_depth"]))
                if params.get("ramp_time"):
                    parts.append("ramp {}".format(params["ramp_time"]))
                return self.direct_safety_reply(" ".join(parts), chat_arm=chat_arm)
        except Exception as exc:
            return (
                "LLM intent mode failed safely while validating `{}`: {}\n\n"
                "No later action was executed."
            ).format(name, exc)

        return (
            "LLM intent mode rejected unknown intent `{}`. "
            "No hardware action was executed."
        ).format(name)

    def execute_llm_set_parameter_intent(self, params, chat_arm=False):
        aliases = {
            "bias": "bias",
            "current": "current_setpoint",
            "current_setpoint": "current_setpoint",
            "scan_speed": "scan_speed",
            "rotation": "rotation",
            "offset_x": "offset_x",
            "offset_y": "offset_y",
            "feedback_cp": "feedback_cp",
            "feedback_ci": "feedback_ci",
            "cp": "feedback_cp",
            "ci": "feedback_ci",
            "slope_x": "slope_x",
            "slope_y": "slope_y",
        }
        parameter = aliases.get(str(params.get("parameter", "")).strip().lower())
        value = str(params.get("value", "")).strip()
        if not parameter or parameter not in PARAM_SPECS:
            return "LLM intent mode rejected an unknown parameter. No hardware action was executed."
        if not value:
            return "LLM intent mode rejected an empty parameter value. No hardware action was executed."
        alias = PARAM_SPECS[parameter]["aliases"][0]
        return self.route_parameter_write(
            "set {} to {}".format(alias, value),
            parameter,
            chat_arm=chat_arm,
        )

    def execute_llm_scan_geometry_intent(self, params, chat_arm=False):
        plan = []
        range_text = str(params.get("range", "")).strip()
        points = params.get("points", None)
        rotation_text = str(params.get("rotation", "")).strip()
        if range_text or points is not None:
            text = "set scan geometry"
            if range_text:
                text += " {}".format(range_text)
            if points is not None:
                text += " {} points".format(points)
            parsed = self.parse_scan_geometry_request(text)
            if parsed is None:
                return (
                    "LLM intent mode rejected scan geometry because range/points "
                    "could not be parsed with valid units."
                )
            range_A, point_count = parsed
            plan.append(
                {
                    "action_name": "scan geometry",
                    "target": "{} A, {} points".format(range_A, point_count),
                    "callback": lambda range_A=range_A, point_count=point_count: self.set_scan_geometry(
                        range_A,
                        point_count,
                        arm=True,
                    ),
                }
            )
        if rotation_text:
            target = self.resolve_rotation_target_deg("rotation {}".format(rotation_text))
            if target is None:
                return "LLM intent mode rejected scan rotation because it was not parseable."
            plan.append(
                {
                    "action_name": "scan rotation",
                    "target": "{} deg".format(self.fmt6(target)),
                    "callback": lambda target=target: self.set_rotation(target, arm=True),
                }
            )
        if not plan:
            return "LLM intent mode rejected empty scan geometry. No hardware action was executed."
        return self.execute_chat_level1_plan(plan, chat_arm=chat_arm)

    def execute_llm_read_parameter_intent(self, params):
        parameter = str(params.get("parameter", "")).strip().lower()
        if parameter in ("all", "scan_geometry", "geometry"):
            return self.format_parameter_readback(list(KNOWN_READBACK_REFS))
        aliases = {
            "bias": "bias",
            "current": "current_setpoint",
            "current_setpoint": "current_setpoint",
            "scan_speed": "scan_speed",
            "rotation": "rotation",
            "offset_x": "offset_x",
            "offset_y": "offset_y",
            "feedback_cp": "feedback_cp",
            "feedback_ci": "feedback_ci",
            "slope_x": "slope_x",
            "slope_y": "slope_y",
        }
        key = aliases.get(parameter)
        if not key or key not in PARAM_SPECS:
            return "LLM intent mode could not map that readback to a known parameter."
        return self.format_parameter_readback(
            [(PARAM_SPECS[key]["label"], PARAM_SPECS[key]["refname"])]
        )

    def route_chat_intent(self, text, chat_arm=False):
        text_l = text.lower().strip()
        if not text_l:
            return None
        if re.search(r"\b(what can you do|help|capabilit(?:y|ies)|actions?|commands?)\b", text_l):
            if any(word in text_l for word in ("chat", "router", "action", "hardware", "microscope")):
                return CHAT_ROUTER_HELP.strip() + "\n\n```json\n{}\n```".format(
                    json.dumps(self.chat_router_capabilities(), indent=2)
                )
        tip_gvp_reply = self.route_tip_gvp_intent(text, chat_arm=chat_arm)
        if tip_gvp_reply is not None:
            return tip_gvp_reply
        tip_loop_reply = self.route_tip_tune_loop_intent(text, chat_arm=chat_arm)
        if tip_loop_reply is not None:
            return tip_loop_reply
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
        offset_shift_reply = self.route_relative_offset_shift(text, chat_arm=chat_arm)
        if offset_shift_reply is not None:
            return offset_shift_reply
        if self.looks_like_write_request(text_l):
            keys = self.infer_parameter_keys(text_l, allow_context=True)
            if self.looks_like_scan_geometry_request(text_l):
                return self.route_scan_geometry_write(text, chat_arm=chat_arm)
            absolute_offset_xy_reply = self.route_absolute_offset_xy_write(
                text,
                chat_arm=chat_arm,
            )
            if absolute_offset_xy_reply is not None:
                return absolute_offset_xy_reply
            if keys:
                writable_keys = [
                    key for key in keys if PARAM_SPECS.get(key, {}).get("write")
                ]
                if len(writable_keys) > 1:
                    plan = []
                    for key in writable_keys:
                        entry = self.build_parameter_write_plan_entry(text, key)
                        if isinstance(entry, str):
                            return entry
                        if entry:
                            plan.append(entry)
                    if plan:
                        return self.execute_chat_level1_plan(plan, chat_arm=chat_arm)
                return self.route_parameter_write(text, keys[0], chat_arm=chat_arm)
            if self.context_parameter_key():
                return self.route_parameter_write(
                    text,
                    self.context_parameter_key(),
                    chat_arm=chat_arm,
                )
        if self.looks_like_read_request(text_l):
            keys = self.infer_parameter_keys(text_l, allow_context=True)
            if keys or self.looks_like_scan_geometry_request(text_l):
                self.chat_context["last_action_domain"] = "readback"
                if self.looks_like_scan_geometry_request(text_l):
                    return self.format_parameter_readback(list(KNOWN_READBACK_REFS))
                return self.format_parameter_readback(
                    [(PARAM_SPECS[key]["label"], PARAM_SPECS[key]["refname"]) for key in keys]
                )
        return None

    def route_tip_gvp_intent(self, text, chat_arm=False):
        text_l = text.lower().strip()
        if any(word in text_l for word in ("process", "loop", "improve", "improvement", "planner")):
            return None
        wants_pulse = (
            "pulse tip" in text_l
            or "tip pulse" in text_l
            or ("tip" in text_l and "pulse" in text_l)
        )
        wants_dip = (
            "dip tip" in text_l
            or "tip dip" in text_l
            or "tune tip" in text_l
            or "tip tune" in text_l
            or "fine tune" in text_l
            or ("tip" in text_l and any(word in text_l for word in ("dip", "tune", "tuning")))
        )
        if not (wants_pulse or wants_dip):
            return None
        self.chat_context["last_action_domain"] = "gvp_tip_action"
        execute_requested = self.safety_limits()["gvp_execute_extra_confirmation"].lower() in text_l
        load_requested = bool(re.search(r"\b(load|prepare|setup|set up)\b", text_l))
        if wants_pulse:
            params = self.parse_tip_pulse_chat_parameters(text)
            action_label = "tip bias pulse GVP"
            load_callback = lambda params=params: self.load_bias_pulse(
                params["pulse_du_V"],
                arm=True,
            )
        else:
            params = self.parse_tip_dip_chat_parameters(text)
            action_label = "tip fine-tune Z-dip GVP" if params["fine_tune"] else "tip tune Z-dip GVP"
            load_callback = lambda params=params: self.load_tip_tune_gvp(
                params["contact_bias_V"],
                params["dip_depth_A"],
                params["ramp_time_s"],
                arm=True,
            )
        if execute_requested:
            return self.execute_chat_gvp_tip_action(
                action_label,
                params,
                load_callback,
                chat_arm=chat_arm,
            )
        if load_requested:
            return self.execute_chat_level1_action(
                "load {}".format(action_label),
                params,
                chat_arm,
                load_callback,
            )
        return (
            "I understood this as `{}`.\n\n"
            "Parameters I would use:\n\n```json\n{}\n```\n\n"
            "To load only, use a phrase like `load {}` with Control Level 1+ "
            "and chat arm checked. To load and execute from chat, include the "
            "exact confirmation phrase `{}`. Execution is a live GVP tip action."
        ).format(
            action_label,
            json.dumps(params, indent=2),
            action_label,
            self.safety_limits()["gvp_execute_extra_confirmation"],
        )

    def parse_tip_pulse_chat_parameters(self, text):
        text_l = text.lower()
        pulse = float(self.controller_default("gvp_defaults.bias_pulse_du_V", 2.0))
        match = re.search(r"([-+]?\d+(?:\.\d+)?)\s*v", text_l)
        if match:
            pulse = float(match.group(1))
        max_pulse = float(self.safety_limits()["level1_max_abs_gvp_bias_pulse_du_V"])
        pulse = max(-max_pulse, min(max_pulse, pulse))
        return {
            "kind": "bias_pulse",
            "pulse_du_V": pulse,
            "max_abs_pulse_du_V": max_pulse,
            "note": (
                "Default tip pulse is {} V; Level 1 caps pulse du at +/-{} V."
            ).format(self.controller_default("gvp_defaults.bias_pulse_du_V", 2.0), max_pulse),
        }

    def parse_tip_dip_chat_parameters(self, text):
        text_l = text.lower()
        fine_tune = "fine tune" in text_l or "finetune" in text_l
        dip_depth = float(
            self.controller_default(
                "gvp_defaults.tip_fine_tune_dip_depth_A"
                if fine_tune
                else "gvp_defaults.tip_dip_depth_A",
                4.0 if fine_tune else 5.0,
            )
        )
        contact_bias = float(self.controller_default("gvp_defaults.tip_contact_bias_V", 0.0))
        match = re.search(r"([-+]?\d+(?:\.\d+)?)\s*a\b", text_l)
        if match:
            dip_depth = abs(float(match.group(1)))
        match = re.search(r"(?:contact bias|contact voltage|at contact)\D*([-+]?\d+(?:\.\d+)?)\s*v", text_l)
        if match:
            contact_bias = float(match.group(1))
        ramp_time = float(self.controller_default("gvp_defaults.tip_ramp_time_s", 30.0))
        match = re.search(r"(?:ramp(?: time)?|vec\s*[24]\s*time)\D*([-+]?\d+(?:\.\d+)?)\s*s", text_l)
        if match:
            ramp_time = float(match.group(1))
        dip_depth = self.clamp_config_value(
            dip_depth,
            "gvp_defaults.tip_dip_depth_range_A",
            [0.0, 25.0],
        )
        contact_bias = self.clamp_config_value(
            contact_bias,
            "gvp_defaults.tip_contact_bias_range_V",
            [-1.0, 1.0],
        )
        ramp_time = self.clamp_config_value(
            ramp_time,
            "gvp_defaults.tip_ramp_time_range_s",
            [1.0, 60.0],
        )
        return {
            "kind": "tip_z_dip",
            "fine_tune": fine_tune,
            "dip_depth_A": dip_depth,
            "contact_bias_V": contact_bias,
            "ramp_time_s": ramp_time,
            "note": (
                "Fine tune means the most gentle default dip, 4 A. "
                "Normal tune/dip default is 5 A. Contact bias defaults to 0 V. "
                "Vec 2/4 ramp time defaults to 30 s and is capped to 1..60 s."
            ),
        }

    def execute_chat_gvp_tip_action(self, action_label, params, load_callback, chat_arm=False):
        if self.control_level < 1:
            return (
                "I did not execute {}. Chat Level 1 actions require Control "
                "Level 1 or higher, plus the chat arm checkbox."
            ).format(action_label)
        if not chat_arm:
            return (
                "I did not execute {}. To allow this Level 1 chat action, "
                "check 'Arm Level 1 chat actions' in the Chat tab and submit "
                "the request again."
            ).format(action_label)
        load_result = load_callback()
        if isinstance(load_result, dict) and (
            load_result.get("error")
            or load_result.get("blocked")
            or load_result.get("cancelled")
        ):
            return (
                "I stopped before executing {}; loading failed or was gated.\n\n"
                "```json\n{}\n```"
            ).format(action_label, json.dumps(self.jsonable({"parameters": params, "load": load_result}), indent=2))
        fig, execute_report = self.execute_loaded_gvp_with_plot(
            self.safety_limits()["gvp_execute_extra_confirmation"],
            min(
                float(self.controller_default("gvp_defaults.execute_wait_max_s", 300.0)),
                max(
                    float(self.controller_default("gvp_defaults.execute_wait_min_s", 180.0)),
                    2.0 * float(params.get("ramp_time_s", 30.0))
                    + float(self.controller_default("gvp_defaults.execute_wait_extra_s", 60.0)),
                ),
            ),
            arm=True,
        )
        if isinstance(execute_report, dict) and (
            execute_report.get("error")
            or execute_report.get("blocked")
            or execute_report.get("cancelled")
        ):
            return (
                "Loaded {}, but execution failed or was gated.\n\n"
                "```json\n{}\n```"
            ).format(
                action_label,
                json.dumps(
                    self.jsonable(
                        {
                            "parameters": params,
                            "load": load_result,
                            "execute": execute_report,
                        }
                    ),
                    indent=2,
                ),
            )
        return (
            "{} loaded and executed through the Level 1 chat GVP gate.\n\n"
            "```json\n{}\n```"
        ).format(
            action_label.capitalize(),
            json.dumps(
                self.jsonable(
                    {
                        "parameters": params,
                        "load": load_result,
                        "execute": execute_report,
                    }
                ),
                indent=2,
            ),
        )

    def route_tip_tune_loop_intent(self, text, chat_arm=False):
        text_l = text.lower().strip()
        wants_tip_loop = (
            any(
                phrase in text_l
                for phrase in (
                    "initiate improve tip process",
                    "initiate tip improvement",
                    "start tip improvement",
                    "run tip improvement",
                    "start tip tune",
                    "run tip tune",
                    "start tip tuning",
                    "run tip tuning",
                    "tip tune planner",
                    "tip tuning loop",
                    "tip improvement loop",
                    "improve tip process",
                )
            )
            or (
                "tip" in text_l
                and any(word in text_l for word in ("improve", "improvement", "tune", "tuning"))
                and any(word in text_l for word in ("initiate", "start", "run", "execute", "begin", "loop", "process"))
            )
        )
        if not wants_tip_loop:
            return None
        self.chat_context["last_action_domain"] = "tip_tune_planner"
        phrase = self.TIP_LOOP_CONFIRMATION
        if phrase.lower() not in text_l:
            return (
                "I did not start the Tip Tune Planner loop yet. This is a live "
                "Level 1 hardware workflow: it can start scans, stop scans, move "
                "ScanX/ScanY to a clean candidate, load/execute gentle GVP tip "
                "actions, and repeat.\n\n"
                "To run it from chat, set Control Level 1+, check 'Arm Level 1 "
                "chat actions', and include this exact confirmation phrase in "
                "your request: `{}`.\n\n"
                "Default chat loop parameters: max cycles 2, channel 0, minimum "
                "140 lines, 700 A range, 400 points, max large hazards 8, "
                "max |dFreq| 4 Hz, no automatic offset shift."
            ).format(phrase)
        return self.execute_chat_tip_tune_loop(text, chat_arm=chat_arm)

    def execute_chat_tip_tune_loop(self, text, chat_arm=False):
        if self.control_level < 1:
            return (
                "I did not execute the Tip Tune Planner loop. Chat Level 1 "
                "actions require Control Level 1 or higher, plus the chat arm checkbox."
            )
        if not chat_arm:
            return (
                "I did not execute the Tip Tune Planner loop. To allow this "
                "Level 1 chat action, check 'Arm Level 1 chat actions' in the "
                "Chat tab and submit the request again."
            )
        params = self.parse_tip_loop_chat_parameters(text)
        fig, progress, report = self.run_tip_planner_loop(
            params["max_cycles"],
            params["channel"],
            params["min_lines"],
            params["range_A"],
            params["points"],
            params["max_large_hazards"],
            params["max_abs_dfreq_Hz"],
            params["auto_shift_on_blobs"],
            params["shift_direction"],
            self.TIP_LOOP_CONFIRMATION,
            True,
        )
        # Keep the latest figures reachable through normal GUI state/report
        # paths; chat itself returns the structured JSON summary.
        self.tip_planner_state["last_chat_loop_report"] = report
        if isinstance(report, dict) and (report.get("error") or report.get("blocked") or report.get("cancelled")):
            return (
                "Tip Tune Planner loop did not start or stopped with a gate/error.\n\n"
                "```json\n{}\n```"
            ).format(json.dumps(self.jsonable({"parameters": params, "report": report}), indent=2))
        return (
            "Tip Tune Planner loop started through the Level 1 chat action gate.\n\n"
            "```json\n{}\n```"
        ).format(json.dumps(self.jsonable({"parameters": params, "report": report}), indent=2))

    def parse_tip_loop_chat_parameters(self, text):
        text_l = text.lower()
        params = dict(self.controller_default("tip_loop_defaults", {}))
        params.setdefault("max_cycles", 2)
        params.setdefault("channel", 0)
        params.setdefault("min_lines", 140)
        params.setdefault("range_A", 700.0)
        params.setdefault("points", 400)
        params.setdefault("max_large_hazards", 8)
        params.setdefault("max_abs_dfreq_Hz", 4.0)
        params["auto_shift_on_blobs"] = bool(
            params.get("auto_shift_on_blobs", False)
            or re.search(r"\b(auto ?shift|shift offset|search offset)\b", text_l)
        )
        params.setdefault("shift_direction", "image_left_below")
        match = re.search(r"\b(?:max cycles|cycles?)\D+(\d+)", text_l)
        if match:
            lo, hi = self.controller_default("tip_loop_ranges.max_cycles", [1, 6])
            params["max_cycles"] = max(int(lo), min(int(hi), int(match.group(1))))
        match = re.search(r"\b(?:channel|ch)\D+(\d+)", text_l)
        if match:
            lo, hi = self.controller_default("tip_loop_ranges.channel", [0, 8])
            params["channel"] = max(int(lo), min(int(hi), int(match.group(1))))
        match = re.search(r"\b(?:lines?|minimum lines|min lines)\D+(\d+)", text_l)
        if match:
            lo, hi = self.controller_default("tip_loop_ranges.min_lines", [20, 400])
            params["min_lines"] = max(int(lo), min(int(hi), int(match.group(1))))
        match = re.search(r"(\d+(?:\.\d+)?)\s*(?:a|ang|angstrom|angstroms)\b", text_l)
        if match:
            lo, hi = self.controller_default("tip_loop_ranges.range_A", [100.0, 1200.0])
            params["range_A"] = max(float(lo), min(float(hi), float(match.group(1))))
        match = re.search(r"\b(?:points?|px)\D+(\d+)", text_l)
        if match:
            lo, hi = self.controller_default("tip_loop_ranges.points", [64, 5000])
            params["points"] = max(int(lo), min(int(hi), int(match.group(1))))
        match = re.search(r"\b(?:large hazards?|hazards?|large blobs?)\D+(\d+)", text_l)
        if match:
            lo, hi = self.controller_default("tip_loop_ranges.max_large_hazards", [1, 30])
            params["max_large_hazards"] = max(int(lo), min(int(hi), int(match.group(1))))
        match = re.search(r"\b(?:dfreq|dfr|frequency)\D+(\d+(?:\.\d+)?)", text_l)
        if match:
            lo, hi = self.controller_default("tip_loop_ranges.max_abs_dfreq_Hz", [0.5, 20.0])
            params["max_abs_dfreq_Hz"] = max(float(lo), min(float(hi), float(match.group(1))))
        for direction in (
            "image_left_below",
            "image_right_below",
            "image_left_above",
            "image_right_above",
            "image_left",
            "image_right",
            "image_up",
            "image_down",
        ):
            if direction.replace("_", " ") in text_l or direction in text_l:
                params["shift_direction"] = direction
                break
        return params

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
                return (
                    "I did not change scan geometry. Include explicit units, "
                    "for example `700 A`, `400 points`, or `70 nm 400 px`."
                )
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
            target = self.resolve_voltage_target_V(
                text,
                current_refname=spec["refname"],
                percent_ok=True,
            )
            if target is None:
                if self.is_relative_request(text):
                    return "I did not change the bias. Relative bias changes need a valid current bias readback and an explicit voltage unit, for example `increase bias by 25 mV`."
                return self.missing_unit_message("bias", "`0.234 V` or `200 mV`")
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
                    return "I did not change the current setpoint. Relative current changes need a valid current readback and an explicit unit, for example `increase current by 2 pA`."
                return self.missing_unit_message("current setpoint", "`10 pA` or `0.01 nA`")
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
            target = self.resolve_scan_speed_target_A_s(
                text,
                current_refname=spec["refname"],
                percent_ok=True,
                relative_words={"faster": 1.2, "slower": 0.8},
            )
            if target is None:
                if self.is_relative_request(text) or re.search(r"\b(faster|slower)\b", text.lower()):
                    return "I did not change the scan speed. A relative speed change needs a valid current scan-speed readback first."
                return self.missing_unit_message("scan speed", "`700 A/s` or `70 nm/s`")
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
        if write_kind == "rotation":
            target = self.resolve_rotation_target_deg(text)
            if target is None:
                return "I did not change scan rotation. Use an absolute angle, for example `45 deg` or `90`."
            lo, hi = self.safety_limits()["level1_rotation_deg"]
            if not (float(lo) <= target <= float(hi)):
                return "I did not change scan rotation. Level 1 allows {}..{} deg.".format(lo, hi)
            return {
                "action_name": "scan rotation",
                "target": "{} deg".format(self.fmt6(target)),
                "callback": lambda target=target: self.set_rotation(target, arm=True),
            }
        if write_kind in ("offset_x", "offset_y"):
            axis = "x" if write_kind == "offset_x" else "y"
            target = self.resolve_offset_target_A(text, axis)
            if target is None:
                return self.missing_unit_message(
                    "Offset{}".format(axis.upper()),
                    "`100 A` or `10 nm`",
                )
            return {
                "action_name": "Offset{}".format(axis.upper()),
                "target": "{} A".format(self.fmt6(target)),
                "callback": lambda target=target, axis=axis: self.set_offset_axis(
                    axis,
                    target,
                    arm=True,
                ),
            }
        if write_kind in ("slope_x", "slope_y"):
            target = self.resolve_slope_target_A_per_A(text, current_refname=spec["refname"])
            if target is None:
                if self.is_relative_request(text):
                    return "I did not change {}. A relative change needs a valid current readback first.".format(
                        spec["label"]
                    )
                return self.missing_unit_message(spec["label"], "`0.01 A/A`")
            if abs(target) > self.safety_limits()["level1_max_abs_scan_slope"]:
                return "I did not change {}. Level 1 allows abs(slope) <= {}.".format(
                    spec["label"],
                    self.safety_limits()["level1_max_abs_scan_slope"],
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
            target = self.resolve_db_target(text, current_refname=spec["refname"])
            if target is None:
                if self.is_relative_request(text):
                    return "I did not change {}. A relative change needs a valid current readback first.".format(
                        spec["label"]
                    )
                return self.missing_unit_message(spec["label"], "`-90 dB`")
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

    def looks_like_microscope_intent_request(self, text_l):
        return bool(
            re.search(
                r"\b("
                r"bias|voltage|volt|millivolt|sample|tunnel|current|setpoint|"
                r"scan|scanning|image|offset|rotation|"
                r"feedback|cp|ci|slope|gvp|tip|pulse|dip|tune|dfreq|"
                r"start|stop|apply|set|change|adjust|shift|move|read|show|"
                r"analyze|analyse|evaluate"
                r")\b",
                text_l,
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
                r"\b(set|change|adjust|make|configure|apply|rotate|move|shift|increase|decrease|raise|lower|faster|slower)\b",
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
            return (
                "I did not change scan geometry. Include explicit units, "
                "for example `700 A`, `400 points`, or `70 nm 400 px`."
            )
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
            target = self.resolve_voltage_target_V(
                text,
                current_refname=spec["refname"],
                percent_ok=True,
            )
            if target is None:
                if self.is_relative_request(text):
                    return "I did not change the bias. Relative bias changes need a valid current bias readback and an explicit voltage unit, for example `increase bias by 25 mV`."
                return self.missing_unit_message("bias", "`0.234 V` or `200 mV`")
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
                    return "I did not change the current setpoint. Relative current changes need a valid current readback and an explicit unit, for example `increase current by 2 pA`."
                return self.missing_unit_message("current setpoint", "`10 pA` or `0.01 nA`")
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
            target = self.resolve_scan_speed_target_A_s(
                text,
                current_refname=spec["refname"],
                percent_ok=True,
                relative_words={"faster": 1.2, "slower": 0.8},
            )
            if target is None:
                if self.is_relative_request(text) or re.search(r"\b(faster|slower)\b", text_l):
                    return "I did not change the scan speed. A relative speed change needs a valid current scan-speed readback first."
                return self.missing_unit_message("scan speed", "`700 A/s` or `70 nm/s`")
            range_A = self.read_float_parameter("RangeX", fallback=700.0)
            return self.execute_chat_level1_action(
                "scan speed",
                "{} A/s".format(self.fmt6(target)),
                chat_arm,
                lambda: self.set_scan_speed(target, range_A, arm=True),
            )
        if write_kind == "rotation":
            target = self.resolve_rotation_target_deg(text)
            if target is None:
                return "I did not change scan rotation. Use an absolute angle, for example `45 deg` or `90`."
            lo, hi = self.safety_limits()["level1_rotation_deg"]
            if not (float(lo) <= target <= float(hi)):
                return "I did not change scan rotation. Level 1 allows {}..{} deg.".format(lo, hi)
            return self.execute_chat_level1_action(
                "scan rotation",
                "{} deg".format(self.fmt6(target)),
                chat_arm,
                lambda: self.set_rotation(target, arm=True),
            )
        if write_kind in ("offset_x", "offset_y"):
            axis = "x" if write_kind == "offset_x" else "y"
            target = self.resolve_offset_target_A(text, axis)
            if target is None:
                return self.missing_unit_message(
                    "Offset{}".format(axis.upper()),
                    "`100 A` or `10 nm`",
                )
            return self.execute_chat_level1_action(
                "Offset{}".format(axis.upper()),
                "{} A".format(self.fmt6(target)),
                chat_arm,
                lambda: self.set_offset_axis(axis, target, arm=True),
            )
        if write_kind in ("slope_x", "slope_y"):
            target = self.resolve_slope_target_A_per_A(text, current_refname=spec["refname"])
            if target is None:
                if self.is_relative_request(text):
                    return "I did not change {}. A relative change needs a valid current readback first.".format(
                        spec["label"]
                    )
                return self.missing_unit_message(spec["label"], "`0.01 A/A`")
            if abs(target) > self.safety_limits()["level1_max_abs_scan_slope"]:
                return "I did not change {}. Level 1 allows abs(slope) <= {}.".format(
                    spec["label"],
                    self.safety_limits()["level1_max_abs_scan_slope"],
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
            target = self.resolve_db_target(text, current_refname=spec["refname"])
            if target is None:
                if self.is_relative_request(text):
                    return "I did not change {}. A relative change needs a valid current readback first.".format(
                        spec["label"]
                    )
                return self.missing_unit_message(spec["label"], "`-90 dB`")
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

    def route_absolute_offset_xy_write(self, text, chat_arm=False):
        parsed = self.parse_absolute_offset_xy_write(text)
        if parsed is None:
            return None
        if parsed.get("error"):
            return parsed["error"]
        target_x = parsed["offset_x_A"]
        target_y = parsed["offset_y_A"]
        target = {
            "to_OffsetX_A": self.fmt6(target_x),
            "to_OffsetY_A": self.fmt6(target_y),
            "interpretation": parsed["interpretation"],
        }
        return self.execute_chat_level1_action(
            "OffsetX/Y",
            target,
            chat_arm,
            lambda target_x=target_x, target_y=target_y: self.set_offset_xy(
                target_x,
                target_y,
                arm=True,
            ),
        )

    def parse_absolute_offset_xy_write(self, text):
        text_s = str(text)
        text_l = text_s.lower()
        if not re.search(r"\b(set|change|adjust|make|configure|apply|move)\b", text_l):
            return None
        shared_xy = bool(
            re.search(r"\boffsets?\s*(?:x\s*(?:and|&|/)\s*y|xy|x/y)\b", text_l)
            or re.search(r"\boffset\s*x\s*(?:and|&|/)\s*y\b", text_l)
            or re.search(r"\boffset\s*y\s*(?:and|&|/)\s*x\b", text_l)
        )
        if not shared_xy:
            return None
        parsed = self.parse_si_quantity(text_s, "length_A")
        if parsed is None:
            return {
                "error": self.missing_unit_message(
                    "OffsetX/Y",
                    "`0 A`, `100 A`, or `10 nm`",
                )
            }
        value_A = float(parsed["value"])
        return {
            "offset_x_A": value_A,
            "offset_y_A": value_A,
            "interpretation": "same absolute world-coordinate value for OffsetX and OffsetY",
        }

    def route_relative_offset_shift(self, text, chat_arm=False):
        parsed = self.parse_relative_offset_shift(text)
        if parsed is None:
            return None
        if parsed.get("error"):
            return parsed["error"]
        current_x = self.read_float_parameter("OffsetX", fallback=0.0)
        current_y = self.read_float_parameter("OffsetY", fallback=0.0)
        rotation = self.read_float_parameter("Rotation", fallback=0.0)
        theta = np.deg2rad(rotation)
        local_dx = parsed["local_dx_A"]
        local_dy = parsed["local_dy_A"]
        world_dx = local_dx * np.cos(theta) - local_dy * np.sin(theta)
        world_dy = local_dx * np.sin(theta) + local_dy * np.cos(theta)
        target_x = current_x + world_dx
        target_y = current_y + world_dy
        target = {
            "from_OffsetX_A": self.fmt6(current_x),
            "from_OffsetY_A": self.fmt6(current_y),
            "to_OffsetX_A": self.fmt6(target_x),
            "to_OffsetY_A": self.fmt6(target_y),
            "rotation_deg": self.fmt6(rotation),
            "local_dx_A": self.fmt6(local_dx),
            "local_dy_A": self.fmt6(local_dy),
            "world_dx_A": self.fmt6(world_dx),
            "world_dy_A": self.fmt6(world_dy),
        }
        return self.execute_chat_level1_action(
            "relative OffsetX/Y shift",
            target,
            chat_arm,
            lambda target_x=target_x, target_y=target_y: self.set_offset_xy(
                target_x,
                target_y,
                arm=True,
            ),
        )

    def parse_relative_offset_shift(self, text):
        text_s = str(text)
        text_l = text_s.lower()
        if not re.search(r"\b(shift|move|relocate)\b", text_l):
            return None
        if not re.search(r"\b(image|scan|view|frame|feature|features|offset|landscape)\b", text_l):
            return None
        directions = {
            "left": (-1.0, 0.0),
            "right": (1.0, 0.0),
            "up": (0.0, 1.0),
            "above": (0.0, 1.0),
            "top": (0.0, 1.0),
            "down": (0.0, -1.0),
            "below": (0.0, -1.0),
            "bottom": (0.0, -1.0),
        }
        direction_hits = [
            name for name in directions if re.search(r"\b{}\b".format(name), text_l)
        ]
        if not direction_hits:
            return None

        amount = self.parse_si_quantity(text_s, "length_A")
        if amount is None:
            return {
                "error": (
                    "I did not shift OffsetX/Y. Please include an explicit "
                    "length unit, for example `shift image 100 A left` or "
                    "`shift scan down by 10 nm`."
                )
            }
        amount_A = abs(float(amount["value"]))
        local_dx = 0.0
        local_dy = 0.0
        for direction in direction_hits:
            dx, dy = directions[direction]
            local_dx += dx * amount_A
            local_dy += dy * amount_A
        return {
            "amount_A": amount_A,
            "directions": direction_hits,
            "local_dx_A": local_dx,
            "local_dy_A": local_dy,
        }

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

    def si_quantity_specs(self):
        return {
            "voltage": {
                "base_unit": "V",
                "units": {
                    "V": 1.0,
                    "v": 1.0,
                    "volt": 1.0,
                    "volts": 1.0,
                    "mV": 1e-3,
                    "mv": 1e-3,
                    "millivolt": 1e-3,
                    "millivolts": 1e-3,
                    "uV": 1e-6,
                    "uv": 1e-6,
                    "µv": 1e-6,
                    "µV": 1e-6,
                    "microvolt": 1e-6,
                    "microvolts": 1e-6,
                    "MV": 1e6,
                    "kv": 1e3,
                    "kV": 1e3,
                },
            },
            "current": {
                "base_unit": "nA",
                "units": {
                    "na": 1.0,
                    "nanoamp": 1.0,
                    "nanoamps": 1.0,
                    "nanoampere": 1.0,
                    "nanoamperes": 1.0,
                    "pa": 1e-3,
                    "picoamp": 1e-3,
                    "picoamps": 1e-3,
                    "picoampere": 1e-3,
                    "picoamperes": 1e-3,
                    "ua": 1e3,
                    "µa": 1e3,
                    "microamp": 1e3,
                    "microamps": 1e3,
                    "a": 1e9,
                    "amp": 1e9,
                    "amps": 1e9,
                },
            },
            "length_A": {
                "base_unit": "A",
                "units": {
                    "a": 1.0,
                    "å": 1.0,
                    "ang": 1.0,
                    "angstrom": 1.0,
                    "angstroms": 1.0,
                    "nm": 10.0,
                    "nanometer": 10.0,
                    "nanometers": 10.0,
                    "pm": 0.01,
                    "picometer": 0.01,
                    "picometers": 0.01,
                    "um": 1e4,
                    "µm": 1e4,
                    "micrometer": 1e4,
                    "micrometers": 1e4,
                },
            },
            "scan_speed": {
                "base_unit": "A/s",
                "units": {
                    "a/s": 1.0,
                    "å/s": 1.0,
                    "ang/s": 1.0,
                    "angstrom/s": 1.0,
                    "angstroms/s": 1.0,
                    "a/sec": 1.0,
                    "ang/sec": 1.0,
                    "angstrom/sec": 1.0,
                    "nm/s": 10.0,
                    "nm/sec": 10.0,
                    "um/s": 1e4,
                    "µm/s": 1e4,
                },
            },
            "angle": {
                "base_unit": "deg",
                "units": {
                    "deg": 1.0,
                    "degree": 1.0,
                    "degrees": 1.0,
                },
            },
            "db": {
                "base_unit": "dB",
                "units": {
                    "db": 1.0,
                },
            },
            "slope": {
                "base_unit": "A/A",
                "units": {
                    "a/a": 1.0,
                    "å/å": 1.0,
                    "ang/ang": 1.0,
                    "angstrom/angstrom": 1.0,
                },
            },
            "points": {
                "base_unit": "points",
                "units": {
                    "count": 1.0,
                    "counts": 1.0,
                    "point": 1.0,
                    "points": 1.0,
                    "px": 1.0,
                    "pixel": 1.0,
                    "pixels": 1.0,
                },
            },
        }

    def parse_si_quantity(self, text, kind):
        spec = self.si_quantity_specs()[kind]
        units = spec["units"]
        number_pattern = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?"
        unit_pattern = "|".join(
            re.escape(unit) for unit in sorted(units, key=len, reverse=True)
        )
        pattern = re.compile(
            r"({})\s*({})(?![a-zA-Z/µÅå])".format(number_pattern, unit_pattern),
            re.IGNORECASE,
        )
        matches = list(pattern.finditer(str(text)))
        if not matches:
            return None
        match = matches[-1]
        unit_text = match.group(2)
        unit_key = unit_text if unit_text in units else unit_text.lower()
        return {
            "value": float(match.group(1)) * units[unit_key],
            "raw_value": float(match.group(1)),
            "unit": unit_text,
            "base_unit": spec["base_unit"],
            "text": match.group(0),
        }

    def missing_unit_message(self, label, expected):
        return (
            "I did not change {}. Please include an explicit unit, for example {}."
        ).format(label, expected)

    def voltage_unit_scale(self, unit):
        unit_text = str(unit or "V")
        unit_l = unit_text.lower()
        if unit_text == "MV":
            return 1e6
        if unit_text in ("mV",) or unit_l in ("mv", "millivolt", "millivolts"):
            return 0.001
        if unit_text in ("uV", "µV") or unit_l in ("uv", "µv", "microvolt", "microvolts"):
            return 1e-6
        if unit_text == "kV" or unit_l == "kv":
            return 1e3
        return 1.0

    def parse_last_voltage_literal_V(self, text):
        parsed = self.parse_si_quantity(text, "voltage")
        return None if parsed is None else parsed["value"]

    def resolve_voltage_target_V(self, text, current_refname=None, percent_ok=False):
        text_l = str(text).lower()
        current = None
        if current_refname:
            current = self.read_float_parameter(current_refname, fallback=np.nan)
            if not np.isfinite(current):
                current = None
        relative_requested = self.is_relative_request(text)
        if relative_requested and current is None:
            return None

        delta_match = re.search(
            r"\b(?:increase|raise|decrease|lower|up|down)\b[^-+\d]*"
            r"([-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?)\s*"
            r"(MV|mV|uV|µV|kV|mv|millivolts?|v|volts?)\b",
            str(text),
            re.IGNORECASE,
        )
        if delta_match and current is not None:
            delta = float(delta_match.group(1)) * self.voltage_unit_scale(delta_match.group(2))
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

        return self.parse_last_voltage_literal_V(text)

    def resolve_quantity_target(self, text, kind, current_refname=None, percent_ok=False, relative_words=None):
        text_l = str(text).lower()
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
        if re.search(r"\b(?:increase|raise|decrease|lower|up|down)\b", text_l) and current is not None:
            parsed = self.parse_si_quantity(text, kind)
            if parsed is None:
                return None
            delta = parsed["value"]
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
        parsed = self.parse_si_quantity(text, kind)
        return None if parsed is None else parsed["value"]

    def resolve_scan_speed_target_A_s(self, text, current_refname=None, percent_ok=False, relative_words=None):
        return self.resolve_quantity_target(
            text,
            "scan_speed",
            current_refname=current_refname,
            percent_ok=percent_ok,
            relative_words=relative_words,
        )

    def resolve_rotation_target_deg(self, text):
        parsed = self.parse_si_quantity(text, "angle")
        if parsed is not None:
            return parsed["value"]
        text_l = str(text).lower()
        if not re.search(r"\b(rotation|scan rotation|rotate|scan angle|angle)\b", text_l):
            return None
        matches = re.findall(
            r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?",
            str(text),
        )
        return None if not matches else float(matches[-1])

    def resolve_offset_target_A(self, text, axis):
        axis = str(axis).lower()
        if axis not in ("x", "y"):
            return None
        text_s = str(text)
        text_l = text_s.lower()
        if not re.search(
            r"\b(?:offset\s*{}|offset{}|scan offset\s*{}|world offset\s*{})\b".format(
                axis,
                axis,
                axis,
                axis,
            ),
            text_l,
        ):
            return None

        spec = self.si_quantity_specs()["length_A"]
        units = spec["units"]
        number_pattern = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?"
        unit_pattern = "|".join(
            re.escape(unit) for unit in sorted(units, key=len, reverse=True)
        )
        axis_pattern = re.compile(
            r"(?:offset\s*{}|offset{}|scan offset\s*{}|world offset\s*{})"
            r"[^-+0-9eE]*({})\s*({})(?![a-zA-Z/µÅå])".format(
                axis,
                axis,
                axis,
                axis,
                number_pattern,
                unit_pattern,
            ),
            re.IGNORECASE,
        )
        matches = list(axis_pattern.finditer(text_s))
        if not matches:
            return None
        match = matches[-1]
        unit_text = match.group(2)
        unit_key = unit_text if unit_text in units else unit_text.lower()
        return float(match.group(1)) * units[unit_key]

    def resolve_slope_target_A_per_A(self, text, current_refname=None):
        return self.resolve_quantity_target(text, "slope", current_refname=current_refname)

    def resolve_db_target(self, text, current_refname=None):
        return self.resolve_quantity_target(text, "db", current_refname=current_refname)

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
        return None

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
        lo, hi = self.gui_default("live_readouts.line_count_range", [8, 2048])
        return max(int(lo), min(int(hi), int(match.group(1))))

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
        if not re.search(r"\b(set|change|adjust|make|configure|apply|rotate|move)\b", text, re.IGNORECASE):
            return None
        text_l = text.lower()
        if "bias" in text_l:
            target = self.resolve_voltage_target_V(
                text,
                current_refname="dsp-fbs-bias",
                percent_ok=True,
            )
            if target is None:
                return self.missing_unit_message("bias", "`0.234 V` or `200 mV`")
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
                return self.missing_unit_message("current setpoint", "`10 pA` or `0.01 nA`")
            value, unit = parsed
            current_nA = float(value)
            if current_nA > self.safety_limits()["level1_max_current_nA"]:
                return (
                    "I did not change the current setpoint. The requested value "
                    "is {:.6g} nA, above the Level 1 automatic limit of {:.6g} nA."
                ).format(current_nA, self.safety_limits()["level1_max_current_nA"])
            return self.execute_chat_level1_action(
                "current setpoint",
                "{} nA".format(current_nA),
                chat_arm,
                lambda: self.set_current(current_nA, "nA", arm=True),
            )
        if "scan speed" in text_l:
            target = self.resolve_scan_speed_target_A_s(text, current_refname="dsp-fbs-scan-speed-scan")
            if target is None:
                return self.missing_unit_message("scan speed", "`700 A/s` or `70 nm/s`")
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
                return (
                    "I did not change scan geometry. Include explicit units, "
                    "for example `700 A`, `400 points`, or `70 nm 400 px`."
                )
            range_A, points = parsed
            return self.execute_chat_level1_action(
                "scan geometry",
                "{} A, {} points".format(range_A, points),
                chat_arm,
                lambda: self.set_scan_geometry(range_A, points, arm=True),
            )
        if re.search(r"\boffset\s*x\b|\boffsetx\b|\bscan offset x\b|\bworld offset x\b", text_l):
            target = self.resolve_offset_target_A(text, "x")
            if target is None:
                return self.missing_unit_message("OffsetX", "`100 A` or `10 nm`")
            return self.execute_chat_level1_action(
                "OffsetX",
                "{} A".format(self.fmt6(target)),
                chat_arm,
                lambda: self.set_offset_axis("x", target, arm=True),
            )
        if re.search(r"\boffset\s*y\b|\boffsety\b|\bscan offset y\b|\bworld offset y\b", text_l):
            target = self.resolve_offset_target_A(text, "y")
            if target is None:
                return self.missing_unit_message("OffsetY", "`100 A` or `10 nm`")
            return self.execute_chat_level1_action(
                "OffsetY",
                "{} A".format(self.fmt6(target)),
                chat_arm,
                lambda: self.set_offset_axis("y", target, arm=True),
            )
        if "slope x" in text_l or "scan slope x" in text_l:
            target = self.resolve_slope_target_A_per_A(text, current_refname="dsp-adv-scan-slope-x")
            if target is None:
                return self.missing_unit_message("scan slope x", "`0.01 A/A`")
            if abs(target) > self.safety_limits()["level1_max_abs_scan_slope"]:
                return "I did not change scan slope x. Level 1 allows abs(slope) <= {}.".format(
                    self.safety_limits()["level1_max_abs_scan_slope"]
                )
            return self.execute_chat_level1_action(
                "scan slope x",
                target,
                chat_arm,
                lambda: self.set_scan_slope(slope_x=target, arm=True),
            )
        if "slope y" in text_l or "scan slope y" in text_l:
            target = self.resolve_slope_target_A_per_A(text, current_refname="dsp-adv-scan-slope-y")
            if target is None:
                return self.missing_unit_message("scan slope y", "`0.01 A/A`")
            if abs(target) > self.safety_limits()["level1_max_abs_scan_slope"]:
                return "I did not change scan slope y. Level 1 allows abs(slope) <= {}.".format(
                    self.safety_limits()["level1_max_abs_scan_slope"]
                )
            return self.execute_chat_level1_action(
                "scan slope y",
                target,
                chat_arm,
                lambda: self.set_scan_slope(slope_y=target, arm=True),
            )
        if re.search(r"\bcp\b", text_l) and re.search(r"\bci\b", text_l):
            cp = self.resolve_db_target(text_l.split("ci", 1)[0], current_refname="dsp-fbs-cp")
            ci = self.resolve_db_target(text_l.split("ci", 1)[1], current_refname="dsp-fbs-ci")
            if cp is None or ci is None:
                return self.missing_unit_message("feedback CP/CI", "`CP -90 dB CI -90 dB`")
            return self.execute_chat_level1_action(
                "feedback CP/CI",
                "CP {}, CI {}".format(cp, ci),
                chat_arm,
                lambda: self.set_feedback_cp_ci(cp, ci, arm=True),
            )
        return None

    def execute_chat_level1_action(self, action_name, target, chat_arm, callback):
        is_scan_action = action_name in ("start scan", "stop scan")
        is_load_action = str(action_name).startswith("load ")
        blocked_phrase = (
            "I did not execute {}".format(action_name)
            if is_scan_action or is_load_action
            else "I did not change {}".format(action_name)
        )
        action_display = str(action_name)
        action_display = action_display[:1].upper() + action_display[1:]
        success_prefix = (
            "{} executed".format(action_display)
            if is_scan_action
            else "{} executed".format(action_display)
            if is_load_action
            else "{} change executed".format(action_display)
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
        parsed = self.parse_si_quantity(text, "current")
        if parsed is None:
            return None
        return parsed["value"], "nA"

    def parse_scan_points_request(self, text):
        parsed = self.parse_si_quantity(text, "points")
        if parsed is not None:
            return int(round(parsed["value"]))
        text_l = str(text).lower()
        if not re.search(r"\b(points?|px|pixels?|counts?)\b", text_l):
            return None
        match = re.search(
            r"(?:points?|px|pixels?|counts?)\D*(\d+)\b",
            text_l,
        )
        if not match:
            match = re.search(
                r"\b(\d+)\D*(?:points?|px|pixels?|counts?)\b",
                text_l,
            )
        return None if not match else int(match.group(1))

    def parse_scan_geometry_request(self, text):
        text_l = text.lower()
        range_parsed = self.parse_si_quantity(text_l, "length_A")
        points_value = self.parse_scan_points_request(text_l)
        if range_parsed is None and points_value is None:
            return None
        range_A = (
            float(range_parsed["value"])
            if range_parsed is not None
            else self.read_float_parameter("RangeX", fallback=700.0)
        )
        points = (
            int(points_value)
            if points_value is not None
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
        limits = self.controller_limits()
        legacy_map = {
            "max_bias_auto_V": "level1_max_abs_bias_V",
            "max_current_auto_nA": "level1_max_current_nA",
            "max_scan_slope_abs": "level1_max_abs_scan_slope",
        }
        for legacy_key, limit_key in legacy_map.items():
            if legacy_key in self.config and limit_key not in limits:
                limits[limit_key] = self.config[legacy_key]
        return limits

    def read_gxsm_dconf_settings(self):
        cfg = self.controller_default("gxsm_dconf", {})
        if not cfg.get("enabled", True):
            return {"enabled": False, "note": "GXSM dconf readback disabled in controller config."}
        if shutil.which("dconf") is None:
            return {"enabled": True, "error": "`dconf` executable not found."}
        main_window_path = str(cfg.get("main_window_path", cfg.get("current_path", "")))
        controller_path = str(cfg.get("controller_path", ""))
        adjustment_path = str(cfg.get("adjustment_path", ""))
        main_window = {}
        controller_current = {}
        adjustments = {}
        errors = []
        main_window_keys = dict(cfg.get("main_window_keys", cfg.get("current_keys", {})))
        for name, key in main_window_keys.items():
            rec = self.dconf_read_value(main_window_path, key)
            main_window[name] = rec
            if rec.get("error"):
                errors.append(rec)
        for name, key in dict(cfg.get("controller_keys", {})).items():
            rec = self.dconf_read_value(controller_path, key)
            controller_current[name] = rec
            if rec.get("error"):
                errors.append(rec)
        for name, key in dict(cfg.get("adjustment_keys", {})).items():
            rec = self.dconf_read_value(adjustment_path, key)
            if isinstance(rec.get("value"), list) and len(rec["value"]) >= 2:
                upper = rec["value"][0]
                lower = rec["value"][1]
                rec["upper"] = upper
                rec["lower"] = lower
                rec["range"] = [min(lower, upper), max(lower, upper)]
                rec["note"] = "GXSM adjustment array: first value is upper, second is lower."
            adjustments[name] = rec
            if rec.get("error"):
                errors.append(rec)
        report = {
            "enabled": True,
            "read_only": True,
            "main_window_path": main_window_path,
            "controller_path": controller_path,
            "adjustment_path": adjustment_path,
            "main_window_current": main_window,
            "controller_current": controller_current,
            "adjustments": adjustments,
            "comparison": self.compare_config_to_dconf_adjustments(adjustments),
        }
        if errors:
            report["errors"] = errors
        return self.jsonable(report)

    def dconf_read_value(self, base_path, key):
        path = "{}{}".format(str(base_path).rstrip("/") + "/", key)
        try:
            proc = subprocess.run(
                ["dconf", "read", path],
                check=False,
                capture_output=True,
                text=True,
                timeout=2.0,
            )
        except Exception as exc:
            return {"path": path, "error": str(exc)}
        raw = (proc.stdout or "").strip()
        rec = {
            "path": path,
            "raw": raw,
            "returncode": proc.returncode,
        }
        if proc.stderr:
            rec["stderr_note"] = self.clean_dconf_stderr(proc.stderr)
        if proc.returncode != 0:
            rec["error"] = "dconf read returned {}".format(proc.returncode)
            return rec
        if raw == "":
            rec["value"] = None
        else:
            rec["value"] = self.parse_dconf_literal(raw)
        return rec

    def parse_dconf_literal(self, raw):
        text = str(raw).strip()
        try:
            return ast.literal_eval(text)
        except Exception:
            pass
        try:
            return float(text)
        except Exception:
            return text

    def clean_dconf_stderr(self, stderr):
        lines = [
            line.strip()
            for line in str(stderr).splitlines()
            if line.strip()
        ]
        return " ".join(lines)

    def compare_config_to_dconf_adjustments(self, adjustments):
        limits = self.safety_limits()
        checks = []
        self.add_abs_limit_dconf_check(
            checks,
            "level1_max_abs_bias_V",
            limits.get("level1_max_abs_bias_V"),
            adjustments.get("dsp-fbs-bias"),
        )
        self.add_upper_limit_dconf_check(
            checks,
            "level1_max_current_nA",
            limits.get("level1_max_current_nA"),
            adjustments.get("dsp-fbs-mx0-current-set"),
        )
        self.add_range_dconf_check(
            checks,
            "level1_feedback_cp_ci_range_dB",
            limits.get("level1_feedback_cp_ci_range_dB"),
            adjustments.get("dsp-fbs-cp"),
            "dsp-fbs-cp",
        )
        self.add_range_dconf_check(
            checks,
            "level1_feedback_cp_ci_range_dB",
            limits.get("level1_feedback_cp_ci_range_dB"),
            adjustments.get("dsp-fbs-ci"),
            "dsp-fbs-ci",
        )
        self.add_abs_limit_dconf_check(
            checks,
            "level1_max_abs_scan_slope",
            limits.get("level1_max_abs_scan_slope"),
            adjustments.get("dsp-adv-scan-slope-x"),
            "dsp-adv-scan-slope-x",
        )
        self.add_abs_limit_dconf_check(
            checks,
            "level1_max_abs_scan_slope",
            limits.get("level1_max_abs_scan_slope"),
            adjustments.get("dsp-adv-scan-slope-y"),
            "dsp-adv-scan-slope-y",
        )
        warnings = [check for check in checks if check.get("status") == "outside_gxsm_limit"]
        return {
            "checks": checks,
            "warnings": warnings,
            "note": (
                "These checks compare this copilot's configured automatic limits "
                "against GXSM dconf adjustment limits. Values outside GXSM's "
                "limits may trigger a blocking GXSM warning/rejection dialog."
            ),
        }

    def add_abs_limit_dconf_check(self, checks, config_key, config_value, adjustment, refname=None):
        if config_value is None or not adjustment or adjustment.get("range") is None:
            return
        gxsm_min, gxsm_max = adjustment["range"]
        allowed = min(abs(float(gxsm_min)), abs(float(gxsm_max)))
        status = "ok" if abs(float(config_value)) <= allowed else "outside_gxsm_limit"
        checks.append(
            {
                "config_key": config_key,
                "refname": refname or adjustment.get("path", "").rsplit("/", 1)[-1],
                "config_value": config_value,
                "gxsm_adjustment_range": adjustment["range"],
                "status": status,
            }
        )

    def add_upper_limit_dconf_check(self, checks, config_key, config_value, adjustment):
        if config_value is None or not adjustment or adjustment.get("range") is None:
            return
        gxsm_min, gxsm_max = adjustment["range"]
        status = "ok" if float(config_value) <= float(gxsm_max) else "outside_gxsm_limit"
        checks.append(
            {
                "config_key": config_key,
                "refname": adjustment.get("path", "").rsplit("/", 1)[-1],
                "config_value": config_value,
                "gxsm_adjustment_range": adjustment["range"],
                "status": status,
            }
        )

    def add_range_dconf_check(self, checks, config_key, config_range, adjustment, refname):
        if config_range is None or not adjustment or adjustment.get("range") is None:
            return
        gxsm_min, gxsm_max = adjustment["range"]
        cfg_min, cfg_max = config_range
        status = (
            "ok"
            if float(gxsm_min) <= float(cfg_min) <= float(cfg_max) <= float(gxsm_max)
            else "outside_gxsm_limit"
        )
        checks.append(
            {
                "config_key": config_key,
                "refname": refname,
                "config_value": config_range,
                "gxsm_adjustment_range": adjustment["range"],
                "status": status,
            }
        )

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
            instrument_gains = self.read_instrument_gains_cached()
            gvp_monitor = {}
            try:
                self.runner.get_live_tip_position_monitor(collect_as="live_dashboard_gvp_position")
                gvp_monitor = self.runner.data.get("live_dashboard_gvp_position", {}) or {}
            except Exception as exc:
                gvp_monitor = {"error": "{}: {}".format(type(exc).__name__, exc)}
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
                "gvp_x_A": gvp_monitor.get("dsp-GVP-XS-MONITOR"),
                "gvp_y_A": gvp_monitor.get("dsp-GVP-YS-MONITOR"),
            }
            self.update_live_xy_calibration(values)
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
                "GVP_position_monitor": gvp_monitor,
                "live_xy_calibration": self.live_xy_calibration,
                "instrument_gains": instrument_gains,
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

    def read_instrument_gains_cached(self, max_age_s=10.0):
        now = time.time()
        updated = self.instrument_gains_cache.get("updated_at")
        if (
            self.instrument_gains_cache.get("values") is not None
            and updated is not None
            and now - float(updated) <= float(max_age_s)
        ):
            return dict(self.instrument_gains_cache)
        try:
            gxsm = self.runner.connect()
            values = gxsm.get_instrument_gains()
            parsed = self.parse_instrument_gains(values)
            self.instrument_gains_cache = {
                "values": parsed,
                "raw": values,
                "updated_at": now,
                "updated_at_text": time.strftime("%Y-%m-%dT%H:%M:%S"),
                "error": None,
                "source": "gxsm.get_instrument_gains()",
            }
        except Exception as exc:
            self.instrument_gains_cache = {
                "values": None,
                "raw": None,
                "updated_at": now,
                "updated_at_text": time.strftime("%Y-%m-%dT%H:%M:%S"),
                "error": "{}: {}".format(type(exc).__name__, exc),
                "source": "gxsm.get_instrument_gains()",
            }
        return dict(self.instrument_gains_cache)

    def parse_instrument_gains(self, values):
        seq = list(values)
        parsed = {
            "X_amplifier_gain": float(seq[0]) if len(seq) > 0 else None,
            "Y_amplifier_gain": float(seq[1]) if len(seq) > 1 else None,
            "Z_amplifier_gain": float(seq[2]) if len(seq) > 2 else None,
            "X_A_to_V": float(seq[3]) if len(seq) > 3 else None,
            "Y_A_to_V": float(seq[4]) if len(seq) > 4 else None,
            "Z_A_to_V": float(seq[5]) if len(seq) > 5 else None,
            "raw_names": ["VX", "VY", "VZ", "AVX", "AVY", "AVZ"],
            "note": (
                "gxsm.get_instrument_gains() returns X/Y/Z amplifier gains "
                "followed by Angstrom-to-Volt conversion factors after the "
                "amplifier gains are applied."
            ),
        }
        return parsed

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

    def update_live_xy_calibration(self, values):
        """
        Fit local ScanX/Y Angstrom as an affine function of raw controller Volts.

        A simple local_A/raw_V ratio causes a 1/x artifact while scanning.  Use
        recent live samples and fit `local_A = slope * raw_V + intercept`
        instead.  The fitted inverse then maps known hazard local-A positions
        back into the fixed raw-voltage panel.
        """
        updated = False
        for axis, v_key, a_key in (
            ("x", "x_monitor_V", "gvp_x_A"),
            ("y", "y_monitor_V", "gvp_y_A"),
        ):
            try:
                volts = float(values.get(v_key))
                ang = float(values.get(a_key))
            except Exception:
                continue
            if not (np.isfinite(volts) and np.isfinite(ang)):
                continue
            samples = self.live_xy_calibration_samples.setdefault(axis, [])
            if samples:
                last_v, last_a = samples[-1]
                if abs(last_v - volts) < 1e-4 and abs(last_a - ang) < 0.05:
                    continue
            samples.append((volts, ang))
            del samples[:-240]
            if len(samples) < 12:
                continue
            arr = np.asarray(samples, dtype=float)
            v = arr[:, 0]
            a = arr[:, 1]
            v_span = float(np.nanmax(v) - np.nanmin(v))
            a_span = float(np.nanmax(a) - np.nanmin(a))
            if v_span < 0.25 or a_span < 25.0:
                continue
            try:
                slope, intercept = np.polyfit(v, a, 1)
            except Exception:
                continue
            if not np.isfinite(slope) or abs(float(slope)) < 1e-9:
                continue
            residual = a - (slope * v + intercept)
            rms = float(np.sqrt(np.nanmean(residual * residual)))
            if not np.isfinite(rms):
                continue
            self.live_xy_calibration["{}_slope_A_per_V".format(axis)] = float(slope)
            self.live_xy_calibration["{}_intercept_A".format(axis)] = float(intercept)
            self.live_xy_calibration["sample_count"][axis] = len(samples)
            self.live_xy_calibration["{}_fit_rms_A".format(axis)] = rms
            updated = True
        if updated:
            self.live_xy_calibration["updated_at"] = time.strftime("%Y-%m-%dT%H:%M:%S")
            self.live_xy_calibration["source"] = "linear fit: GVP local ScanX/ScanY A = slope * raw rt_query_xyz V + intercept"

    def local_scan_A_to_raw_V(self, axis, local_A):
        slope = self.live_xy_calibration.get("{}_slope_A_per_V".format(axis))
        intercept = self.live_xy_calibration.get("{}_intercept_A".format(axis))
        if slope is None or intercept is None:
            return None
        slope = float(slope)
        if abs(slope) < 1e-9:
            return None
        return (float(local_A) - float(intercept)) / slope

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

    def known_large_hazards_for_live_xy(self, max_items=80):
        entries = []
        latest = self.tip_planner_state.get("last_analysis") if hasattr(self, "tip_planner_state") else None
        if isinstance(latest, dict) and latest.get("hazards") is not None:
            entries.append(
                {
                    "timestamp": latest.get("timestamp") or "latest_planner_analysis",
                    "source": "latest_planner_analysis",
                    "hazards": latest.get("hazards", []),
                }
            )
        path = self.output_dir / "landscape_memory.json"
        if path.exists():
            try:
                data = json.loads(path.read_text())
            except Exception:
                data = []
            if isinstance(data, dict):
                file_entries = data.get("entries", [])
                if not file_entries and data.get("hazards") is not None:
                    file_entries = [data]
            elif isinstance(data, list):
                file_entries = data
            else:
                file_entries = []
            entries.extend(file_entries)
        hazards = []
        seen = set()
        for entry in entries:
            for hazard in entry.get("hazards", []) or []:
                if not (hazard.get("count_for_offset_stop") or hazard.get("avoid_for_clean_area")):
                    continue
                wx = hazard.get("world_x_A")
                wy = hazard.get("world_y_A")
                if wx is None or wy is None:
                    continue
                try:
                    wx = float(wx)
                    wy = float(wy)
                except Exception:
                    continue
                key = (round(wx / 10.0), round(wy / 10.0))
                if key in seen:
                    continue
                seen.add(key)
                item = dict(hazard)
                item["world_x_A"] = wx
                item["world_y_A"] = wy
                item["memory_timestamp"] = entry.get("timestamp")
                item["source"] = entry.get("source") or (
                    "latest_planner_analysis"
                    if entry.get("timestamp") == "latest_planner_analysis"
                    else "landscape_memory"
                )
                hazards.append(item)
        hazards.sort(
            key=lambda item: (
                0 if item.get("count_for_offset_stop") else 1,
                -abs(float(item.get("dz_peak_A", 0.0) or 0.0)),
            )
        )
        return hazards[:int(max_items)]

    def live_xy_scan_settings(self):
        settings = {}
        try:
            settings = dict(self.runner._scan_settings(0))
        except Exception:
            settings = {}
        if settings.get("RangeX") is not None and settings.get("RangeY") is not None:
            return settings
        latest = self.tip_planner_state.get("last_analysis") if hasattr(self, "tip_planner_state") else None
        if isinstance(latest, dict):
            source = latest.get("settings") or {}
            frame = latest.get("scan_frame") or {}
            merged = dict(settings)
            if source:
                merged.update(source)
            if frame:
                merged.setdefault("OffsetX", frame.get("OffsetX_A"))
                merged.setdefault("OffsetY", frame.get("OffsetY_A"))
                merged.setdefault("RangeX", frame.get("RangeX_A"))
                merged.setdefault("RangeY", frame.get("RangeY_A"))
                merged.setdefault("Rotation", frame.get("Rotation_deg"))
            return merged
        return settings

    def world_to_current_local_scan_A(self, world_x_A, world_y_A, settings):
        offset_x = float(settings.get("OffsetX", 0.0))
        offset_y = float(settings.get("OffsetY", 0.0))
        rotation_deg = float(settings.get("Rotation", 0.0))
        dx = float(world_x_A) - offset_x
        dy = float(world_y_A) - offset_y
        theta = np.deg2rad(rotation_deg)
        local_x = dx * np.cos(theta) + dy * np.sin(theta)
        local_y = -dx * np.sin(theta) + dy * np.cos(theta)
        return float(local_x), float(local_y)

    def cluster_live_xy_hazard_points(self, points, panel_px=180.0, threshold_px=18.0):
        clusters = []
        for point in points:
            px = float(point["left_pct"]) * panel_px / 100.0
            py = float(point["top_pct"]) * panel_px / 100.0
            assigned = False
            for cluster in clusters:
                cx = cluster["sum_px"] / cluster["weight"]
                cy = cluster["sum_py"] / cluster["weight"]
                if float(np.hypot(px - cx, py - cy)) <= threshold_px:
                    cluster["points"].append(point)
                    cluster["sum_px"] += px
                    cluster["sum_py"] += py
                    cluster["weight"] += 1.0
                    assigned = True
                    break
            if not assigned:
                clusters.append({
                    "points": [point],
                    "sum_px": px,
                    "sum_py": py,
                    "weight": 1.0,
                })
        outlines = []
        for cluster in clusters:
            if len(cluster["points"]) < 2:
                continue
            xs = [float(item["left_pct"]) for item in cluster["points"]]
            ys = [float(item["top_pct"]) for item in cluster["points"]]
            pad_pct = max(5.0, 100.0 * threshold_px / panel_px * 0.55)
            left = max(0.0, min(xs) - pad_pct)
            right = min(100.0, max(xs) + pad_pct)
            top = max(0.0, min(ys) - pad_pct)
            bottom = min(100.0, max(ys) + pad_pct)
            outlines.append(
                "<div title=\"clustered known large hazard region, {} marks\" "
                "style=\"position:absolute;left:{:.3f}%;top:{:.3f}%;"
                "width:{:.3f}%;height:{:.3f}%;border:2px solid #c62828;"
                "border-radius:999px;background:rgba(198,40,40,.08);"
                "box-sizing:border-box;pointer-events:none\"></div>".format(
                    len(cluster["points"]),
                    left,
                    top,
                    max(2.0, right - left),
                    max(2.0, bottom - top),
                )
            )
        return "\n".join(outlines), len(outlines)

    def fallback_scan_frame_hazard_voltage(self, local_x, local_y, settings, limit):
        try:
            range_x = max(abs(float(settings.get("RangeX"))), 1e-9)
            range_y = max(abs(float(settings.get("RangeY"))), 1e-9)
        except Exception:
            return None, None
        return (
            float(local_x) / (0.5 * range_x) * float(limit),
            float(local_y) / (0.5 * range_y) * float(limit),
        )

    def live_xy_hazard_overlay(self, values, limit):
        try:
            settings = self.live_xy_scan_settings()
            gain_values = (self.instrument_gains_cache.get("values") or {})
            x_gain = gain_values.get("X_amplifier_gain")
            y_gain = gain_values.get("Y_amplifier_gain")
            x_A_to_V = gain_values.get("X_A_to_V")
            y_A_to_V = gain_values.get("Y_A_to_V")
            has_instrument_gains = (
                x_gain is not None
                and y_gain is not None
                and x_A_to_V is not None
                and y_A_to_V is not None
                and abs(float(x_gain)) > 1e-12
                and abs(float(y_gain)) > 1e-12
                and abs(float(x_A_to_V)) > 1e-12
                and abs(float(y_A_to_V)) > 1e-12
            )
            slope_x = self.live_xy_calibration.get("x_slope_A_per_V")
            slope_y = self.live_xy_calibration.get("y_slope_A_per_V")
            intercept_x = self.live_xy_calibration.get("x_intercept_A")
            intercept_y = self.live_xy_calibration.get("y_intercept_A")
            has_affine = not (
                slope_x is None or slope_y is None
                or intercept_x is None or intercept_y is None
                or abs(float(slope_x)) < 1e-9
                or abs(float(slope_y)) < 1e-9
                )
            hazards = self.known_large_hazards_for_live_xy()
            markers = []
            marker_points = []
            visible = 0
            projected = 0
            for idx, hazard in enumerate(hazards, start=1):
                if (
                    hazard.get("source") == "latest_planner_analysis"
                    and hazard.get("local_scan_x_A") is not None
                    and hazard.get("local_scan_y_A") is not None
                ):
                    local_x = float(hazard["local_scan_x_A"])
                    local_y = float(hazard["local_scan_y_A"])
                else:
                    local_x, local_y = self.world_to_current_local_scan_A(
                        hazard["world_x_A"],
                        hazard["world_y_A"],
                        settings,
                )
                if has_instrument_gains:
                    hvx = local_x / float(x_A_to_V) / float(x_gain)
                    hvy = local_y / float(y_A_to_V) / float(y_gain)
                    mode = "instrument gain"
                elif has_affine:
                    hvx = self.local_scan_A_to_raw_V("x", local_x)
                    hvy = self.local_scan_A_to_raw_V("y", local_y)
                    mode = "affine controller-voltage"
                else:
                    hvx, hvy = self.fallback_scan_frame_hazard_voltage(
                        local_x,
                        local_y,
                        settings,
                        limit,
                    )
                    mode = "scan-frame normalized fallback"
                if hvx is None or hvy is None:
                    continue
                if not (np.isfinite(hvx) and np.isfinite(hvy)):
                    continue
                projected += 1
                if abs(hvx) > float(limit) or abs(hvy) > float(limit):
                    continue
                visible += 1
                left = 50.0 + 50.0 * max(-1.0, min(1.0, hvx / float(limit)))
                top = 50.0 - 50.0 * max(-1.0, min(1.0, hvy / float(limit)))
                title = (
                    "known large hazard #{idx}: world=({wx:.4g}, {wy:.4g}) A, "
                    "panel=({vx:.4g}, {vy:.4g}) V, mode={mode}"
                ).format(
                    idx=idx,
                    wx=float(hazard["world_x_A"]),
                    wy=float(hazard["world_y_A"]),
                    vx=hvx,
                    vy=hvy,
                    mode=mode,
                )
                marker_points.append(
                    {
                        "left_pct": left,
                        "top_pct": top,
                        "hazard": hazard,
                    }
                )
                markers.append(
                    "<div title=\"{title}\" style=\"position:absolute;left:{left:.3f}%;top:{top:.3f}%;"
                    "width:10px;height:10px;margin-left:-5px;margin-top:-5px;"
                    "border-radius:50%;background:#c62828;box-shadow:0 0 0 1px white,0 0 0 2px #7f0000;"
                    "opacity:.85\"></div>".format(
                        title=html.escape(title),
                        left=left,
                        top=top,
                    )
                )
            outlines, outline_count = self.cluster_live_xy_hazard_points(marker_points)
            if has_instrument_gains:
                note = (
                    "Known large hazards visible: {} / {}; projected: {}; outlined regions: {}; "
                    "using gxsm.get_instrument_gains(): controller V = A / AV / Vgain; "
                    "X gain={:.4g}, AVX={:.4g}; Y gain={:.4g}, AVY={:.4g}; updated {}."
                ).format(
                    visible,
                    len(hazards),
                    projected,
                    outline_count,
                    float(x_gain),
                    float(x_A_to_V),
                    float(y_gain),
                    float(y_A_to_V),
                    self.instrument_gains_cache.get("updated_at_text") or "n/a",
                )
            elif has_affine:
                note = (
                    "Known large hazards visible: {} / {}; projected: {}; outlined regions: {}; "
                    "affine fit X={:.4g} A/V + {:.4g} A, Y={:.4g} A/V + {:.4g} A, updated {}."
                ).format(
                    visible,
                    len(hazards),
                    projected,
                    outline_count,
                    float(slope_x),
                    float(intercept_x),
                    float(slope_y),
                    float(intercept_y),
                    self.live_xy_calibration.get("updated_at") or "n/a",
                )
            else:
                gain_error = self.instrument_gains_cache.get("error")
                gain_note = (
                    " Instrument gain readback unavailable: {}.".format(gain_error)
                    if gain_error else ""
                )
                note = (
                    "Known large hazards visible: {} / {}; projected: {}; outlined regions: {}; "
                    "using scan-frame normalized fallback until instrument gains or affine controller-voltage calibration are available.{}"
                ).format(visible, len(hazards), projected, outline_count, gain_note)
            return "\n".join([outlines] + markers), note
        except Exception as exc:
            return "", "Hazard overlay unavailable: {}: {}".format(type(exc).__name__, exc)

    def xy_position_html(self, values, raw_limit_V=10.0):
        configured_limit = self.gui_default(
            "live_readouts.gauge_limits.xy_raw_V",
            raw_limit_V,
        )
        limit = max(abs(float(configured_limit)), 1e-9)
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
        hazard_overlay, hazard_note = self.live_xy_hazard_overlay(values, limit)
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
      Y piezo: {ye_text} V<br>
      <span style="color:#c62828">red dots/outline: known large hazards</span>
    </div>
    <div style="position:relative;width:180px;height:180px;background:#f7f7f7;border:1px solid #aaa">
      <div style="position:absolute;left:50%;top:0;width:1px;height:100%;background:#bbb"></div>
      <div style="position:absolute;left:0;top:50%;width:100%;height:1px;background:#bbb"></div>
      {hazard_overlay}
      <div style="position:absolute;left:{xpct:.3f}%;top:{ypct:.3f}%;width:12px;height:12px;margin-left:-6px;margin-top:-6px;border-radius:50%;background:{color};box-shadow:0 0 0 2px white,0 0 0 3px #444"></div>
      <div style="position:absolute;left:4px;top:3px;font-size:10px;color:#555">+Y</div>
      <div style="position:absolute;right:4px;top:84px;font-size:10px;color:#555">+X</div>
    </div>
  </div>
  <div style="font-size:11px;color:#666;margin-top:8px">{hazard_note}</div>
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
            hazard_overlay=hazard_overlay,
            hazard_note=html.escape(hazard_note),
        )

    def live_monitor_html(self, values, gains):
        x_label = "X piezo"
        y_label = "Y piezo"
        z_label = "Z piezo"
        gauge_limits = self.gui_default("live_readouts.gauge_limits", {})
        rows = [
            self.gauge_row(x_label, values.get("x_effective_piezo_V"), "V", gauge_limits.get("piezo_V", 100.0), bipolar=True),
            self.gauge_row(y_label, values.get("y_effective_piezo_V"), "V", gauge_limits.get("piezo_V", 100.0), bipolar=True),
            self.gauge_row(z_label, values.get("z_effective_piezo_V"), "V", gauge_limits.get("piezo_V", 100.0), bipolar=True),
            self.gauge_row("Bias", values.get("bias_V"), "V", gauge_limits.get("bias_V", 5.0), bipolar=True),
            self.gauge_row("Current", values.get("current_nA"), "nA", gauge_limits.get("current_nA", 0.1), bipolar=True),
            self.gauge_row("GVP U", values.get("gvp_u_V"), "V", gauge_limits.get("gvp_u_V", 5.0), bipolar=True),
            self.gauge_row("PAC ampl", values.get("pac_ampl_mV"), "mV", gauge_limits.get("pac_ampl_mV", 10.0), bipolar=True),
            self.gauge_row("PAC dFreq", values.get("pac_dfreq_Hz"), "Hz", gauge_limits.get("pac_dfreq_Hz", 20.0), bipolar=True),
        ]
        xy_panel = self.xy_position_html(values)
        return """
<div style="border:1px solid #d0d0d0;border-radius:6px;padding:10px;background:#fafafa">
  <div style="font-weight:700;margin-bottom:6px">Live Microscope Monitor</div>
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

    def level3_emergency_stop_coarse_motion(self):
        try:
            rec = self.level3.emergency_stop_coarse_motion()
            return self.jsonable(rec.__dict__)
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
                period_s=float(self.safety_limits()["level3_z_down_period_s"]),
                voltage_V=float(self.safety_limits()["level3_z_down_voltage_V"]),
                max_burstcount=int(self.safety_limits()["level3_z_down_max_burstcount"]),
                max_voltage_V=float(self.safety_limits()["level3_z_down_max_voltage_V"]),
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
            lo, hi = self.safety_limits()["level1_rotation_deg"]
            if not (float(lo) <= rotation <= float(hi)):
                raise ValueError("Rotation must be within {}..{} degrees for Level 1.".format(lo, hi))
            rec = self.runner.set_parameter("Rotation", rotation)
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def set_offset_axis(self, axis, offset_A, arm):
        axis = str(axis).lower()
        if axis not in ("x", "y"):
            return {"error": "Offset axis must be `x` or `y`."}
        label = "Offset{}".format(axis.upper())
        blocked = self.require_control_level(1, "set {}".format(label))
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "set {}".format(label))
        if cancelled:
            return cancelled
        try:
            offset = float(offset_A)
            rec = self.runner.set_parameter(label, offset)
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def set_offset_xy(self, offset_x_A, offset_y_A, arm):
        blocked = self.require_control_level(1, "set OffsetX/Y")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "set OffsetX/Y")
        if cancelled:
            return cancelled
        try:
            x_rec = self.runner.set_parameter("OffsetX", float(offset_x_A))
            y_rec = self.runner.set_parameter("OffsetY", float(offset_y_A))
            return {
                "OffsetX": self.jsonable(x_rec.__dict__),
                "OffsetY": self.jsonable(y_rec.__dict__),
            }
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

    gd = backend.gui_default

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
            dconf_btn = gr.Button("Read GXSM dconf Settings / Limits")
            level_report = gr.JSON(
                value={
                    "control_level": 0,
                    "description": CONTROL_LEVELS[0],
                    "help": CONTROL_LEVEL_HELP.strip(),
                    "safety_limits": backend.safety_limits(),
                },
                label="Control-level policy",
            )
            dconf_report = gr.JSON(label="Read-only GXSM dconf report")
        control_level.change(backend.set_control_level, control_level, [status, level_report])
        dconf_btn.click(backend.read_gxsm_dconf_settings, outputs=dconf_report)

        with gr.Tab("Chat"):
            gr.Markdown(
                "Ollama replies are advisory. Clear Level 1 chat commands "
                "(including start/stop scan and bounded parameter changes) can "
                "execute only when Control Level is 1+ and chat action arm is checked. "
                "LLM Intent Mode, when enabled, asks the model only for a strict "
                "JSON intent and then reparses it through the deterministic safety gates."
            )
            chat = gr.Chatbot(label="Ollama chat", height=420)
            with gr.Row():
                chat_arm = gr.Checkbox(
                    value=gd("chat.arm_level1_actions", False),
                    label="Arm Level 1 chat actions",
                )
                llm_intent_mode = gr.Checkbox(
                    value=gd("chat.llm_intent_mode", True),
                    label="LLM Intent Mode",
                )
            prompt = gr.Textbox(label="Prompt", placeholder="Ask for interpretation or a proposed next step")
            prompt.submit(
                backend.chat,
                [prompt, chat, chat_arm, llm_intent_mode],
                [prompt, chat],
            )

        with gr.Tab("Live Microscope State Monitor"):
            gr.Markdown(
                "Fast XYZ monitor updates at 5 Hz from "
                "`gxsm4process.rt_query_xyz()` and displays "
                "`XYZ_monitor['monitor']` in controller Volts, not Angstroms. "
                "The X/Y/Z gain fields are external amplifier factors used to "
                "compute effective piezo voltage from controller output voltage."
            )
            with gr.Row():
                gain_choices = gd("live_readouts.xyz_gain_choices", ["1", "3", "6", "12", "24"])
                xyz_gain_x = gr.Dropdown(
                    choices=gain_choices,
                    value=gd("live_readouts.xyz_gain_x", "12"),
                    label="X external amplifier gain",
                )
                xyz_gain_y = gr.Dropdown(
                    choices=gain_choices,
                    value=gd("live_readouts.xyz_gain_y", "12"),
                    label="Y external amplifier gain",
                )
                xyz_gain_z = gr.Dropdown(
                    choices=gain_choices,
                    value=gd("live_readouts.xyz_gain_z", "24"),
                    label="Z external amplifier gain",
                )
            with gr.Row():
                xyz_x = gr.Number(label="X monitor (V)", interactive=False)
                xyz_y = gr.Number(label="Y monitor (V)", interactive=False)
                xyz_z = gr.Number(label="Z monitor (V)", interactive=False)
            live_gauges = gr.HTML(label="Fast monitor gauges")
            xyz_report = gr.JSON(label="Fast XYZ monitor report")
            xyz_timer = gr.Timer(value=gd("live_readouts.xyz_timer_s", 0.2), active=True)
            xyz_timer.tick(
                backend.read_fast_live_dashboard,
                inputs=[xyz_gain_x, xyz_gain_y, xyz_gain_z],
                outputs=[xyz_x, xyz_y, xyz_z, live_gauges, xyz_report],
            )

        with gr.Tab("Scan Image"):
            with gr.Row():
                scan_channel = gr.Number(value=gd("scan_image.channel", 0), label="Channel", precision=0)
                scan_lines = gr.Number(
                    value=gd("scan_image.lines", 120),
                    label="Recent/full lines to fetch",
                    precision=0,
                )
                scan_auto = gr.Checkbox(value=gd("scan_image.auto_refresh", False), label="Auto refresh")
                fetch_btn = gr.Button("Fetch And Plot")
            scan_plot = gr.Plot(label="Scan image")
            scan_profile = gr.Plot(label="Last scan-line profile")
            scan_meta = gr.JSON(label="Scan metadata")
            scan_refresh_timer = gr.Timer(value=gd("scan_image.refresh_interval_s", 1.0), active=True)
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
                ana_channel = gr.Number(value=gd("analysis.channel", 0), label="Channel", precision=0)
                ana_lines = gr.Number(value=gd("analysis.lines", 160), label="Lines to fetch", precision=0)
                ana_prefix = gr.Textbox(value=gd("analysis.output_prefix", "gui_scan_analysis"), label="Output prefix")
                ana_btn = gr.Button("Analyze Current Scan")
            with gr.Row():
                ana_candidate_index = gr.Number(
                    value=gd("analysis.candidate_index", 1),
                    label="Candidate #",
                    precision=0,
                )
                ana_arm = gr.Checkbox(value=gd("analysis.arm_tip_move", False), label="Arm tip move")
                ana_read_tip_btn = gr.Button("Read / Mark Current Tip")
                ana_auto_tip = gr.Checkbox(
                    value=gd("analysis.auto_tip_position", False),
                    label="Auto update current tip marker",
                )
                ana_move_btn = gr.Button("Move Tip To Selected Candidate")
            gr.Markdown(
                "Click the analyzed image to select the nearest flat candidate. "
                "The move writes local ScanX/ScanY only, and is refused unless "
                "the scan/GVP state is ready."
            )
            ana_plot = gr.Plot(label="Analyzed image")
            ana_path = gr.Textbox(label="Saved report path", interactive=False)
            ana_report = gr.JSON(label="Analysis report")
            ana_btn.click(
                backend.analyze_current_scan,
                [ana_channel, ana_lines, ana_prefix],
                [ana_plot, ana_report, ana_path],
            )
            ana_read_tip_btn.click(
                backend.tip_planner_read_tip_position,
                outputs=[ana_plot, ana_report],
            )
            ana_move_btn.click(
                backend.tip_planner_move_to_candidate_update_plot,
                [ana_candidate_index, ana_arm],
                [ana_plot, ana_report],
            )
            ana_tip_timer = gr.Timer(value=gd("analysis.tip_position_refresh_s", 1.0), active=True)
            ana_tip_timer.tick(
                backend.tip_planner_auto_read_tip_position,
                inputs=ana_auto_tip,
                outputs=[ana_plot, ana_report],
            )
            if hasattr(ana_plot, "select"):
                def _analysis_plot_select(evt):
                    return backend.tip_planner_select_candidate_from_plot(evt)

                ana_plot.select(
                    _analysis_plot_select,
                    outputs=[ana_candidate_index, ana_plot, ana_report],
                )

        with gr.Tab("Tip Tune Planner"):
            gr.Markdown(
                "Planner workflow: analyze the current scan, mark flat work-site "
                "candidates and blobs, optionally move the stopped tip to a "
                "candidate, then run bounded scan/action/reassess cycles. "
                "ScanX/Y moves are refused while scanning."
            )
            planner_arm = gr.Checkbox(value=gd("tip_planner.arm_action", False), label="Arm one planner live action")
            planner_plot = gr.Plot(label="Planner annotated image")
            planner_progress = gr.Plot(label="Planner progress")
            with gr.Accordion("Analyze And Select Work Site", open=True):
                with gr.Row():
                    planner_channel = gr.Number(value=gd("tip_planner.channel", 0), label="Channel", precision=0)
                    planner_lines = gr.Number(value=gd("tip_planner.lines", 180), label="Lines to fetch", precision=0)
                    planner_patch = gr.Number(value=gd("tip_planner.patch_A", 80.0), label="Flat patch size (A)")
                    planner_candidates = gr.Number(value=gd("tip_planner.max_candidates", 12), label="Max candidates", precision=0)
                with gr.Row():
                    planner_max_blobs = gr.Number(value=gd("tip_planner.max_large_hazards", 8), label="Stop/search if large hazards >=", precision=0)
                    planner_prefix = gr.Textbox(value=gd("tip_planner.output_prefix", "gui_tip_planner"), label="Output prefix")
                    planner_analyze_btn = gr.Button("Analyze Tip And Flat Candidates")
                with gr.Row():
                    planner_candidate_index = gr.Number(value=gd("tip_planner.candidate_index", 1), label="Candidate #", precision=0)
                    planner_read_tip_btn = gr.Button("Read / Mark Current Tip")
                    planner_auto_tip = gr.Checkbox(
                        value=gd("tip_planner.auto_tip_position", False),
                        label="Auto update current tip marker",
                    )
                    planner_move_btn = gr.Button("Move Tip To Selected Candidate")
                gr.Markdown(
                    "Click the annotated image to select the nearest numbered "
                    "flat candidate. Movement still requires Control Level 1, "
                    "the planner arm checkbox, and a stopped scan/GVP state."
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
                        value=gd("tip_planner.shift_direction", "image_left_below"),
                        label="Search direction",
                    )
                    shift_range = gr.Number(value=gd("tip_planner.shift_range_A", 800.0), label="New scan range (A)")
                    shift_overlap = gr.Number(value=gd("tip_planner.shift_overlap_A", 250.0), label="Overlap (A)")
                    shift_points = gr.Number(value=gd("tip_planner.shift_points", 400), label="Points", precision=0)
                with gr.Row():
                    plan_shift_btn = gr.Button("Plan Offset Shift")
                    start_after_shift = gr.Checkbox(value=gd("tip_planner.start_after_shift", True), label="Start scan after shift")
                    apply_shift_btn = gr.Button("Apply Offset Shift")

            with gr.Accordion("Automated Tip-Improvement Loop", open=False):
                gr.Markdown(
                    "Runs scan, waits for enough lines, assesses tip and dFreq, "
                    "moves to a clean candidate after stopping the scan, executes "
                    "a gentle pulse/tune action, and repeats. Stops when quality "
                    "and dFreq are satisfactory, when blob count is too high, or "
                    "when max cycles is reached. Requires exact confirmation."
                )
                with gr.Row():
                    loop_cycles = gr.Number(value=gd("tip_planner.loop_max_cycles", 2), label="Max cycles", precision=0)
                    loop_min_lines = gr.Number(value=gd("tip_planner.loop_min_lines", 140), label="Minimum evidence lines", precision=0)
                    loop_range = gr.Number(value=gd("tip_planner.loop_range_A", 700.0), label="Diagnostic range (A)")
                    loop_points = gr.Number(value=gd("tip_planner.loop_points", 400), label="Points", precision=0)
                with gr.Row():
                    loop_max_blobs = gr.Number(value=gd("tip_planner.loop_max_large_hazards", 8), label="Stop/search if large hazards >=", precision=0)
                    loop_df_ok = gr.Number(value=gd("tip_planner.loop_max_abs_dfreq_Hz", 4.0), label="Max |dFreq| OK (Hz)")
                    loop_auto_shift = gr.Checkbox(value=gd("tip_planner.loop_auto_shift_on_blobs", False), label="Auto shift on too many blobs")
                loop_confirm = gr.Textbox(
                    label="Loop confirmation",
                    placeholder="Type EXECUTE TIP LOOP",
                )
                loop_btn = gr.Button("Run Tip Tune Planner Loop")
            planner_report = gr.JSON(label="Planner report")
            with gr.Row():
                planner_stop_btn = gr.Button(
                    "Stop Tip Tune Planner Loop",
                    variant="stop",
                )
                planner_clear_reason = gr.Textbox(
                    value=gd("tip_planner.clear_history_reason", "operator hyper jump / new world"),
                    label="Clear history reason",
                )
                planner_clear_btn = gr.Button("Clear Planner / Landscape History")
            planner_activity = gr.HTML(value=backend.tip_planner_activity_html())
            planner_activity_timer = gr.Timer(
                value=gd("tip_planner.activity_refresh_s", 1.0),
                active=True,
            )
            planner_activity_timer.tick(
                backend.tip_planner_activity_html,
                outputs=planner_activity,
            )
            planner_stop_btn.click(
                backend.request_stop_tip_planner_loop,
                outputs=[planner_report, planner_activity],
            )
            planner_clear_btn.click(
                backend.clear_tip_planner_history,
                inputs=planner_clear_reason,
                outputs=[planner_report, planner_activity],
            )
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
            planner_move_btn.click(
                backend.tip_planner_move_to_candidate_update_plot,
                [planner_candidate_index, planner_arm],
                [planner_plot, planner_report],
            )
            planner_read_tip_btn.click(
                backend.tip_planner_read_tip_position,
                outputs=[planner_plot, planner_report],
            )
            planner_tip_timer = gr.Timer(value=gd("tip_planner.tip_position_refresh_s", 1.0), active=True)
            planner_tip_timer.tick(
                backend.tip_planner_auto_read_tip_position,
                inputs=planner_auto_tip,
                outputs=[planner_plot, planner_report],
            )
            if hasattr(planner_plot, "select"):
                def _planner_plot_select(evt):
                    return backend.tip_planner_select_candidate_from_plot(evt)

                planner_plot.select(
                    _planner_plot_select,
                    outputs=[planner_candidate_index, planner_plot, planner_report],
                )
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
                level_channel = gr.Number(value=gd("scan_leveling.channel", 0), label="Channel", precision=0)
                level_lines = gr.Number(value=gd("scan_leveling.recent_line_count", 64), label="Recent line count", precision=0)
                level_prefix = gr.Textbox(value=gd("scan_leveling.output_prefix", "gui_scan_leveling"), label="Output prefix")
                measure_level_btn = gr.Button("Measure Residual Slope")
            level_plot = gr.Plot(label="Recent-line mean profile and fit")
            gr.Markdown(
                "Correction is Level 1 and arm-gated. If the correction worsens "
                "the residual, use the opposite correction sign."
            )
            with gr.Row():
                level_arm = gr.Checkbox(value=gd("scan_leveling.arm_correction", False), label="Arm leveling correction")
                correction_sign = gr.Number(value=gd("scan_leveling.correction_sign", -1.0), label="Correction sign")
                correction_gain = gr.Number(value=gd("scan_leveling.correction_gain", 1.0), label="Gain")
                max_abs_delta = gr.Number(value=gd("scan_leveling.max_abs_delta", 0.005), label="Max abs delta")
            with gr.Row():
                verify_delay = gr.Number(value=gd("scan_leveling.verify_delay_s", 20.0), label="Verify delay (s)")
                verify_lines = gr.Number(value=gd("scan_leveling.verify_line_count", 16), label="Verify line count", precision=0)
                apply_level_btn = gr.Button("Apply Slope Correction")
            level_report = gr.JSON(label="Leveling report")
            measure_level_btn.click(
                backend.measure_scan_leveling,
                [level_channel, level_lines, level_prefix],
                [level_plot, level_report],
            )
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
            gvp_arm = gr.Checkbox(value=gd("gvp.arm_action", False), label="Arm one GVP action")
            with gr.Accordion("Bias Pulse GVP", open=True):
                with gr.Row():
                    pulse_du = gr.Number(value=gd("gvp.bias_pulse_du_V", 2.0), label="Bias pulse du (V)")
                    load_pulse_btn = gr.Button("Load Bias Pulse GVP Only")
            with gr.Accordion("Tip Tune GVP: Z Dip", open=True):
                gr.Markdown(
                    "Gentle starting point: contact bias 0 V, dip depth 5 A. "
                    "Dip depth is entered as a positive magnitude; the GVP program "
                    "uses a negative Z step for indentation and a positive pull-out. "
                    "Ramp time sets vec 2 indentation and vec 4 pull-out timing "
                    "and is limited to 1..60 s. "
                    "Vec 1 removes the actual scan bias, vec 3 applies the contact "
                    "bias, vec 6 removes contact bias, and vec 7 restores scan bias."
                )
                with gr.Row():
                    tip_contact_bias = gr.Number(value=gd("gvp.tip_contact_bias_V", 0.0), label="Contact bias (V)")
                    tip_dip_depth = gr.Number(value=gd("gvp.tip_dip_depth_A", 5.0), label="Dip depth magnitude (A)")
                    tip_ramp_time = gr.Number(value=gd("gvp.tip_ramp_time_s", 30.0), label="Vec 2/4 ramp time (s)")
                    load_tip_tune_btn = gr.Button("Load Tip Tune Z-Dip GVP")
            with gr.Accordion("Execute Loaded GVP", open=True):
                gvp_confirm = gr.Textbox(
                    label="GVP execute confirmation",
                    placeholder="Type EXECUTE GVP to run loaded GVP",
                )
                execute_gvp_btn = gr.Button("Execute Loaded GVP And Plot")
                gvp_wait_s = gr.Number(value=gd("gvp.execute_wait_s", 300.0), label="Max wait for GVP completion (s)")
                gvp_stop_btn = gr.Button(
                    "GVP STOP: NULL VECTORS + EXECUTE",
                    variant="stop",
                    elem_id="gvp_emergency_stop",
                )
                gvp_plot = gr.Plot(label="GVP VP data: ZS, current, dFreq")
            gvp_report = gr.JSON(label="GVP result")
            load_pulse_btn.click(
                backend.load_bias_pulse,
                [pulse_du, gvp_arm],
                gvp_report,
            )
            load_tip_tune_btn.click(
                backend.load_tip_tune_gvp,
                [tip_contact_bias, tip_dip_depth, tip_ramp_time, gvp_arm],
                gvp_report,
            )
            execute_gvp_btn.click(
                backend.execute_loaded_gvp_with_plot,
                [gvp_confirm, gvp_wait_s, gvp_arm],
                [gvp_plot, gvp_report],
            )
            gvp_stop_btn.click(
                backend.emergency_stop_gvp,
                outputs=gvp_report,
            )

        with gr.Tab("Level 3 Protected"):
            gr.Markdown(
                "Extreme actions: coarse motion and auto approach. These require "
                "Control Level 3, the Level 3 arm checkbox, and exact typed "
                "confirmation. Keep `script-control` enabled only while actively "
                "supervising; set it to 0 to abort watchdog-style loops."
            )
            emergency_stop_btn = gr.Button(
                "EMERGENCY STOP COARSE MOTION",
                variant="stop",
                elem_id="level3_emergency_stop",
            )
            gr.Markdown(
                "**Emergency stop sends zero-step, zero-volt THV5 stop commands "
                "immediately. It does not require arming or confirmation.**"
            )
            with gr.Row():
                level3_arm = gr.Checkbox(value=gd("level3.arm_action", False), label="Arm Level 3 action")
                level3_confirm = gr.Textbox(
                    label="Level 3 confirmation",
                    placeholder="Type EXECUTE LEVEL 3",
                )
                level3_tel_btn = gr.Button("Read Level 3 Telemetry")
            with gr.Row():
                script_control = gr.Checkbox(value=gd("level3.script_control_enabled", True), label="script-control enabled")
                script_control_btn = gr.Button("Set script-control")
            gr.Markdown(
                "Coarse Z-down uses THV5 Z, direction -1, period 0.5 s, 30 V. "
                "Burstcount is capped at 5 in this GUI. Z direction -1 is "
                "DANGEROUS: fast tip-down coarse motion."
            )
            with gr.Row():
                coarse_burstcount = gr.Number(value=gd("level3.z_down_burstcount", 1), label="Z-down burstcount", precision=0)
                coarse_z_btn = gr.Button("Execute Coarse Z Down")
            gr.Markdown(
                "Generic THV coarse move. Direction signs are controller signs; "
                "use only with an explicit operator coordinate plan. Normal "
                "coarse moves are capped at burstcount 5 and 100 HV V."
            )
            with gr.Row():
                coarse_channel = gr.Dropdown(choices=["X", "Y", "Z"], value=gd("level3.coarse_channel", "Z"), label="Channel")
                coarse_direction = gr.Dropdown(
                    choices=["-1", "+1"],
                    value=gd("level3.coarse_direction", "-1"),
                    label="Direction",
                )
                coarse_generic_burst = gr.Number(value=gd("level3.coarse_burstcount", 1), label="Burstcount", precision=0)
            with gr.Row():
                coarse_period = gr.Number(value=gd("level3.coarse_period_s", 0.5), label="Period (s)")
                coarse_voltage = gr.Number(value=gd("level3.coarse_voltage_HV_V", 30.0), label="Voltage (HV V)")
                coarse_generic_btn = gr.Button("Execute Generic Coarse Move")
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
                large_channel = gr.Dropdown(choices=["X", "Y", "Z"], value=gd("level3.large_channel", "X"), label="Channel")
                large_direction = gr.Dropdown(
                    choices=["-1", "+1"],
                    value=gd("level3.large_direction", "+1"),
                    label="Direction",
                )
                large_burst = gr.Number(value=gd("level3.large_burstcount", 100), label="Burstcount", precision=0)
            with gr.Row():
                large_voltage = gr.Number(value=gd("level3.large_voltage_HV_V", 30.0), label="Voltage (HV V)")
                large_btn = gr.Button("Execute Large Coarse Move")
            gr.Markdown(
                "Protected auto approach follows the watchdog pattern: set current, "
                "check Z range, retract, apply one or more coarse Z-down bursts, "
                "abort on script-control=0, dFreq limit, or max steps."
            )
            with gr.Row():
                approach_current = gr.Number(value=gd("level3.approach_current_nA", 0.013), label="Approach current (nA)")
                approach_steps = gr.Number(value=gd("level3.approach_steps_per_cycle", 1), label="Coarse steps/cycle", precision=0)
                approach_max_steps = gr.Number(value=gd("level3.approach_max_total_steps", 3), label="Max total steps", precision=0)
            with gr.Row():
                approach_df_limit = gr.Number(value=gd("level3.approach_max_abs_dfreq_Hz", 5.0), label="Max abs dFreq (Hz)")
                approach_timeout = gr.Number(value=gd("level3.approach_check_timeout_s", 30.0), label="Check timeout (s)")
                approach_btn = gr.Button("Execute Protected Auto Approach")
            level3_report = gr.JSON(label="Level 3 result")
            emergency_stop_btn.click(
                backend.level3_emergency_stop_coarse_motion,
                outputs=level3_report,
            )
            level3_tel_btn.click(backend.level3_telemetry, outputs=level3_report)
            script_control_btn.click(
                backend.level3_set_script_control,
                [script_control, level3_confirm, level3_arm],
                level3_report,
            )
            coarse_z_btn.click(
                backend.level3_coarse_z_down,
                [coarse_burstcount, level3_confirm, level3_arm],
                level3_report,
            )
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
            arm = gr.Checkbox(value=gd("armed_actions.arm_action", False), label="Arm one live action")
            with gr.Row():
                stop_btn = gr.Button("Stop Scan")
                start_btn = gr.Button("Start Scan")
            with gr.Row():
                bias_V = gr.Number(value=gd("armed_actions.bias_V", 0.1), label="Bias target (V)")
                set_bias_btn = gr.Button("Set Bias")
            with gr.Row():
                current_value = gr.Number(value=gd("armed_actions.current_value", 10.0), label="Current target")
                current_unit = gr.Dropdown(choices=["pA", "nA"], value=gd("armed_actions.current_unit", "pA"), label="Current unit")
                set_current_btn = gr.Button("Set Current Setpoint")
            with gr.Row():
                scan_speed = gr.Number(value=gd("armed_actions.scan_speed_A_s", 700.0), label="Scan speed (A/s)")
                scan_range = gr.Number(value=gd("armed_actions.scan_range_A", 700.0), label="Current scan range (A)")
                set_speed_btn = gr.Button("Set Scan Speed")
            with gr.Accordion("Scan Geometry", open=True):
                gr.Markdown(
                    "Sets square RangeX/RangeY and PointsX/PointsY. "
                    "StepsX/Y are informational only: Range/(Points-1). "
                    "Rotation is in degrees and may be changed live with caution."
                )
                with gr.Row():
                    geometry_range = gr.Number(value=gd("armed_actions.geometry_range_A", 700.0), label="Scan range X/Y (A)")
                    geometry_points = gr.Number(value=gd("armed_actions.geometry_points", 400), label="Scan points X/Y", precision=0)
                    geometry_rotation = gr.Number(value=gd("armed_actions.geometry_rotation_deg", 90.0), label="Rotation (deg)")
                with gr.Row():
                    set_geometry_btn = gr.Button("Set Range And Points")
                    set_rotation_btn = gr.Button("Set Rotation")
                    set_geometry_rotation_btn = gr.Button("Set Geometry And Rotation")
            with gr.Row():
                cp_dB = gr.Number(value=gd("armed_actions.feedback_cp_dB", -90.0), label="Feedback CP (dB)")
                ci_dB = gr.Number(value=gd("armed_actions.feedback_ci_dB", -90.0), label="Feedback CI (dB)")
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
    parser.add_argument(
        "--controller-config",
        default=str(DEFAULT_CONTROLLER_CONFIG),
        help="Controller safety limits, ranges, confirmations, and endpoint settings.",
    )
    parser.add_argument(
        "--gui-config",
        default=str(DEFAULT_GUI_CONFIG),
        help="GUI widget defaults and display/refresh settings.",
    )
    parser.add_argument("--model")
    parser.add_argument("--live", action="store_true")
    parser.add_argument("--host", default="")
    parser.add_argument("--port", type=int, default=0)
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
    config = load_config(
        args.config,
        controller_config_path=args.controller_config,
        gui_config_path=args.gui_config,
    )
    if args.model:
        config["model"] = args.model
    backend = MicroscopeGradioBackend(config, live=args.live)
    demo = build_ui(backend)
    server_defaults = config.get("gui", {}).get("server", {})
    host = args.host or server_defaults.get("host", "127.0.0.1")
    port = int(args.port or server_defaults.get("port", 7860))
    launch_kwargs = {
        "server_name": host,
        "server_port": port,
        "share": args.share,
        "show_error": True,
        "css": EMERGENCY_STOP_CSS,
    }
    ssl_cert_default = server_defaults.get("ssl_certfile", DEFAULT_SSL_CERTFILE)
    ssl_key_default = server_defaults.get("ssl_keyfile", DEFAULT_SSL_KEYFILE)
    ssl_certfile = args.ssl_certfile or (ssl_cert_default if args.https else "")
    ssl_keyfile = args.ssl_keyfile or (ssl_key_default if args.https else "")
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
    elif host not in ("127.0.0.1", "localhost"):
        print(
            "WARNING: Microscope GUI authentication is disabled while binding "
            "to {}. Set {} or create {} and use --require-auth for access "
            "control.".format(
                host,
                args.auth_password_env,
                args.auth_password_file,
            )
        )
    demo.launch(**launch_kwargs)


if __name__ == "__main__":
    main()
