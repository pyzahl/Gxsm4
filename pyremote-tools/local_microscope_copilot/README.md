**WARNING: All code, tools, workflows, and documentation in this folder are
AI-generated experimental helpers. Use them with caution, review actions before
running them, and never allow unattended control of live microscope hardware.**

# Local Microscope Copilot

Local LLM wrapper for GXSM4 pyremote microscope workflows.

This folder contains:

- `local_microscope_copilot.py`: terminal chat wrapper around Ollama.
- `microscope_gradio_gui.py`: local browser GUI for chat, scan plots, live
  readouts, tip/landscape analysis, and armed action buttons.
- `microscope_actions.py`: high-level scan, navigation, tip analysis, and GVP
  controller code.
- `gxsm4process.py`: bundled copy of the GXSM4 pyremote transport used by the
  controller.
- `local_microscope_copilot_config.json`: model and safety configuration.
- `local_microscope_copilot_setup.md`: install and run instructions.
- `requirements-gui.txt`: Python packages for the optional Gradio GUI.
- `copilot_services.sh`: local start/stop/restart/status helper for Ollama and
  the Gradio GUI.
- `install_ollama.sh`: downloaded official Ollama Linux installer.
- `docs/`: microscope controller manual, tip-improvement plan, and codebase
  overview notes.
- `gvp_*_program.json` / `gvp_*_recipe.json`: bundled default GVP pulse and
  tip-tune templates used by the controller and GUI load actions.

The copilot imports its local `microscope_actions.py`, which uses the bundled
`gxsm4process.py` copy for GXSM pyremote communication.
Read-only tools can run directly. Hardware-changing tools ask for terminal
confirmation, and GVP execution asks for the explicit phrase `EXECUTE GVP`.

Quick start from this folder:

```bash
cd local_microscope_copilot
sh install_ollama.sh
ollama serve
ollama pull qwen3:4b
./local_microscope_copilot.py --model qwen3:4b
```

With a 16 GB AMD Radeon RX 9070 and working ROCm/Ollama GPU support:

```bash
ollama pull qwen3:8b
./local_microscope_copilot.py --model qwen3:8b
```

See `local_microscope_copilot_setup.md` for details and safety notes.

## Gradio GUI

Install the optional GUI dependencies:

```bash
cd local_microscope_copilot
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements-gui.txt
```

Start the GUI on localhost:

```bash
.venv/bin/python microscope_gradio_gui.py --host 127.0.0.1 --port 7870
```

Use `--live` only when intentionally connected to the live GXSM4 microscope:

```bash
.venv/bin/python microscope_gradio_gui.py --live --host 127.0.0.1 --port 7870
```

Or use the service helper:

```bash
./copilot_services.sh start all --live
./copilot_services.sh status
./copilot_services.sh restart gradio --live
./copilot_services.sh stop all
```

The helper records only processes it starts in `.run/*.pid`; it will not kill an
external Ollama or web server that was started elsewhere.

The GUI separates Ollama chat from explicit microscope buttons. Live-changing
buttons require an arm checkbox; loaded-GVP execution additionally requires
typing `EXECUTE GVP`.

### Control Levels

- **Level 0: Hardware connected, monitoring only.** Read-only monitor, dFreq,
  scan fetch, plotting, and analysis tools.
- **Level 1: Basic operation within default safety margins.** Start/stop scan,
  bias up to +/-1 V, current setpoint up to 0.05 nA, normal scan-speed changes,
  feedback CP/CI changes, safe GVP load/execute with explicit confirmation.
- **Level 2: Advanced GVP and more aggressive tip tuning.** Reserved for future
  workflows.
- **Level 3: Extreme GVP, coarse motion, hyper jumps, auto approach.** Reserved
  for future high-risk workflows.

## Documentation Map

- `docs/microscope_actions_console_manual.md`: full manual for the controller
  classes, scan/actions API, GVP handling, landscape navigation, slope leveling,
  tip assessment, and manual console/script usage.
- `docs/gvp_tip_improvement_plan.md`: working tip-improvement procedure notes,
  GVP pulse/dip recipes, and operator-taught safety conventions.
- `docs/gxsm4process_reference.md`: reference for the bundled pyremote wrapper,
  known `gxsm.get/set/action` IDs, and scan/probe data fetching.
- `docs/codebase-overview.md`: compact overview of the pyremote/GXSM context
  used while developing this controller layer.
