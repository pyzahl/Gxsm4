# Local Microscope Copilot

Local LLM wrapper for GXSM4 pyremote microscope workflows.

This folder contains:

- `local_microscope_copilot.py`: terminal chat wrapper around Ollama.
- `microscope_actions.py`: high-level scan, navigation, tip analysis, and GVP
  controller code.
- `gxsm4process.py`: bundled copy of the GXSM4 pyremote transport used by the
  controller.
- `local_microscope_copilot_config.json`: model and safety configuration.
- `local_microscope_copilot_setup.md`: install and run instructions.
- `install_ollama.sh`: downloaded official Ollama Linux installer.
- `docs/`: microscope controller manual, tip-improvement plan, and codebase
  overview notes.

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
