#!/usr/bin/python3
"""
Local LLM microscope copilot.

This is a small terminal chat wrapper around a local Ollama model.  The model
can request named tools, but this script performs the actual microscope calls
and safety checks.  Hardware-changing tools ask for confirmation before they
run.
"""

import argparse
import json
import os
from pathlib import Path
import sys
import time

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


SYSTEM_PROMPT = """You are a local microscope operator copilot for GXSM4.
You help interpret user instructions and request safe tools.

Important safety rules:
- Prefer read-only tools first.
- For hardware-changing tools, explain the proposed action and wait for the
  wrapper confirmation. Do not claim a hardware action happened unless the tool
  result says it did.
- Bias is in Volts. Current setpoint is in nA; convert pA to nA.
- X/Y/Z monitor values are Angstrom. U monitor is Volts.
- Scan image convention: left is ScanX=-RangeX/2; top/first line is
  ScanY=-RangeY/2. OffsetX/Y are non-rotated world coordinates.
- At Rotation=90 deg, moving the displayed scan downward means decreasing
  world OffsetX.
- GVP execution is a live tip action and must be explicitly confirmed.
"""


TOOLS = [
    {
        "type": "function",
        "function": {
            "name": "read_live_monitors",
            "description": "Read live GVP X/Y/Z/U monitors. Read-only.",
            "parameters": {"type": "object", "properties": {}, "required": []},
        },
    },
    {
        "type": "function",
        "function": {
            "name": "sample_dfrequency",
            "description": "Sample dFrequency multiple times. Read-only.",
            "parameters": {
                "type": "object",
                "properties": {
                    "count": {"type": "integer", "default": 20},
                    "delay_s": {"type": "number", "default": 0.15},
                },
                "required": [],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "analyze_current_scan",
            "description": "Fetch current scan and run landscape/tip analysis. Read-only.",
            "parameters": {
                "type": "object",
                "properties": {
                    "output_prefix": {
                        "type": "string",
                        "default": "local_copilot_scan",
                    }
                },
                "required": [],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "measure_recent_slope",
            "description": "Measure recent fast-axis residual slope. Read-only.",
            "parameters": {
                "type": "object",
                "properties": {
                    "line_count": {"type": "integer", "default": 64},
                    "output_prefix": {
                        "type": "string",
                        "default": "local_copilot_slope",
                    },
                },
                "required": [],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "set_scan_speed",
            "description": "Set live scan speed in A/s. Hardware-changing.",
            "parameters": {
                "type": "object",
                "properties": {
                    "speed_A_per_s": {"type": "number"},
                    "range_A": {"type": "number"},
                },
                "required": ["speed_A_per_s", "range_A"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "set_bias",
            "description": "Set bias in Volts within safety limit. Hardware-changing.",
            "parameters": {
                "type": "object",
                "properties": {"bias_V": {"type": "number"}},
                "required": ["bias_V"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "set_current_setpoint",
            "description": "Set current setpoint. Unit can be nA or pA. Hardware-changing.",
            "parameters": {
                "type": "object",
                "properties": {
                    "current": {"type": "number"},
                    "unit": {"type": "string", "enum": ["nA", "pA"]},
                },
                "required": ["current", "unit"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "move_offset",
            "description": "Move scan-frame OffsetX/Y in world Angstrom. Hardware-changing.",
            "parameters": {
                "type": "object",
                "properties": {
                    "offset_x_A": {"type": "number"},
                    "offset_y_A": {"type": "number"},
                },
                "required": ["offset_x_A", "offset_y_A"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "stop_scan",
            "description": "Stop current scan. Hardware-changing.",
            "parameters": {"type": "object", "properties": {}, "required": []},
        },
    },
    {
        "type": "function",
        "function": {
            "name": "start_scan",
            "description": "Start scan. Hardware-changing.",
            "parameters": {"type": "object", "properties": {}, "required": []},
        },
    },
    {
        "type": "function",
        "function": {
            "name": "load_gvp_bias_pulse",
            "description": "Load a GVP bias pulse but do not execute it. Hardware-changing GUI load.",
            "parameters": {
                "type": "object",
                "properties": {"pulse_du_V": {"type": "number"}},
                "required": ["pulse_du_V"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "execute_loaded_gvp",
            "description": "Execute currently loaded GVP. Live tip action.",
            "parameters": {"type": "object", "properties": {}, "required": []},
        },
    },
]


HARDWARE_TOOLS = {
    "set_scan_speed",
    "set_bias",
    "set_current_setpoint",
    "move_offset",
    "stop_scan",
    "start_scan",
    "load_gvp_bias_pulse",
}

GVP_EXECUTE_TOOLS = {"execute_loaded_gvp"}


class LocalMicroscopeCopilot:
    def __init__(self, config, assume_yes=False):
        self.config = config
        self.assume_yes = assume_yes
        self.runner = MicroscopeActionRunner(
            dry_run=not bool(config.get("default_live", True)),
            verbose=0,
        )
        self.nav = LandscapeNavigationController(self.runner, output_dir=".")
        self.activity = TipImprovementActivityController(self.runner, output_dir=".")
        self.messages = [{"role": "system", "content": SYSTEM_PROMPT}]

    def chat_once(self, text):
        self.messages.append({"role": "user", "content": text})
        while True:
            response = self._ollama_chat(self.messages)
            message = response["message"]
            self.messages.append(message)
            tool_calls = message.get("tool_calls") or []
            if not tool_calls:
                return message.get("content", "")
            for call in tool_calls:
                name = call["function"]["name"]
                args = call["function"].get("arguments") or {}
                result = self.call_tool(name, args)
                self.messages.append(
                    {
                        "role": "tool",
                        "content": json.dumps(self._jsonable(result)),
                        "tool_name": name,
                    }
                )

    def _ollama_chat(self, messages):
        url = self.config.get("ollama_host", "http://127.0.0.1:11434") + "/api/chat"
        payload = {
            "model": self.config.get("model", "qwen3:4b"),
            "messages": messages,
            "tools": TOOLS,
            "stream": False,
            "options": {
                "temperature": float(self.config.get("temperature", 0.2)),
            },
        }
        reply = requests.post(url, json=payload, timeout=180)
        reply.raise_for_status()
        return reply.json()

    def call_tool(self, name, args):
        if name in HARDWARE_TOOLS and self.config.get("confirm_hardware_actions", True):
            self._confirm(name, args)
        if name in GVP_EXECUTE_TOOLS and self.config.get("confirm_gvp_execute", True):
            self._confirm(name, args, phrase="EXECUTE GVP")

        if name == "read_live_monitors":
            self.runner.get_live_tip_position_monitor()
            return self.runner.data.get("live_tip_position")
        if name == "sample_dfrequency":
            return self.activity.sample_dfrequency(
                count=int(args.get("count", 20)),
                delay_s=float(args.get("delay_s", 0.15)),
            )
        if name == "analyze_current_scan":
            return self.nav.analyze_current_scan_landscape_map(
                output_prefix=args.get("output_prefix", "local_copilot_scan")
            )
        if name == "measure_recent_slope":
            return self.runner.measure_recent_fast_axis_slope(
                line_count=int(args.get("line_count", 64)),
                output_prefix=args.get("output_prefix", "local_copilot_slope"),
            )
        if name == "set_scan_speed":
            rec = self.runner.set_live_scan_speed(
                speed_A_per_s=float(args["speed_A_per_s"]),
                range_A=float(args["range_A"]),
            )
            return rec.__dict__
        if name == "set_bias":
            rec = self.runner.set_live_bias(
                float(args["bias_V"]),
                max_abs_auto_V=float(self.config.get("max_bias_auto_V", 1.0)),
            )
            return rec.__dict__
        if name == "set_current_setpoint":
            rec = self.runner.set_live_current_setpoint(
                float(args["current"]),
                unit=args.get("unit", "nA"),
                max_auto_nA=float(self.config.get("max_current_auto_nA", 0.05)),
            )
            return rec.__dict__
        if name == "move_offset":
            rec = self.nav.move_to_offset(
                float(args["offset_x_A"]),
                float(args["offset_y_A"]),
            )
            return rec.__dict__
        if name == "stop_scan":
            gxsm = self.runner.connect()
            return {"stopscan_return": gxsm.stopscan()}
        if name == "start_scan":
            gxsm = self.runner.connect()
            return {"startscan_return": gxsm.startscan()}
        if name == "load_gvp_bias_pulse":
            rec = self.runner.load_gvp_bias_pulse(float(args["pulse_du_V"]))
            return rec.__dict__
        if name == "execute_loaded_gvp":
            rec = self.runner.execute_loaded_gvp(precheck=False, wait_after_execute_s=8.0)
            return rec.__dict__
        raise ValueError("Unknown tool: {}".format(name))

    def _confirm(self, name, args, phrase="yes"):
        if self.assume_yes:
            return
        print("\nTool requires confirmation:", name)
        print(json.dumps(args, indent=2))
        answer = input("Type '{}' to proceed: ".format(phrase)).strip()
        if answer != phrase:
            raise RuntimeError("Cancelled tool: {}".format(name))

    def _jsonable(self, value):
        return self.runner._jsonable(value)


def load_config(path):
    with open(path, "r") as file:
        return json.load(file)


def main(argv=None):
    os.chdir(PROJECT_ROOT)
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--config",
        default=str(SCRIPT_DIR / "local_microscope_copilot_config.json"),
    )
    parser.add_argument("--model")
    parser.add_argument("--assume-yes", action="store_true")
    args = parser.parse_args(argv)

    config = load_config(args.config)
    if args.model:
        config["model"] = args.model
    copilot = LocalMicroscopeCopilot(config, assume_yes=args.assume_yes)
    print("Local microscope copilot using model:", config["model"])
    print("Type 'quit' to exit.")
    while True:
        try:
            text = input("\nmicroscope> ").strip()
        except EOFError:
            print()
            break
        if text.lower() in ("quit", "exit"):
            break
        if not text:
            continue
        try:
            print(copilot.chat_once(text))
        except requests.ConnectionError:
            print("Could not connect to Ollama. Start it with: ollama serve")
        except Exception as exc:
            print("ERROR:", exc, file=sys.stderr)


if __name__ == "__main__":
    main()
