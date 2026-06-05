# Local Microscope Copilot Setup

This repo now includes a local LLM wrapper:

- `local_microscope_copilot.py`
- `microscope_gradio_gui.py`
- `microscope_actions.py`
- `gxsm4process.py`
- `local_microscope_copilot_config.json`
- `microscope_controller_config.json`
- `microscope_gui_config.json`
- `requirements-gui.txt`
- `copilot_services.sh`
- `install_ollama.sh`
- `docs/`

The wrapper uses a local Ollama chat model and exposes a safety-gated set of
microscope tools from its local `microscope_actions.py`. That controller imports
the bundled `gxsm4process.py` transport copy for GXSM pyremote communication.

Start with `docs/project_memory.md` when continuing development from a fresh
checkout. It summarizes the current controller architecture, safety gates,
operator-taught microscope conventions, GVP recipes, and LLM routing design.

Configuration is split by responsibility:

- `local_microscope_copilot_config.json`: Ollama/model/service options.
- `microscope_controller_config.json`: controller URLs, safety limits, GVP
  defaults, and planner ranges.
- `microscope_gui_config.json`: GUI form defaults, refresh intervals, gains,
  and visual gauge ranges.

The Gradio launcher reads all three by default. Use `--controller-config` and
`--gui-config` to test alternate limits or GUI defaults.

## Optional Gradio GUI

The Gradio GUI is the easiest place to display Matplotlib scan plots, JSON
reports, dFrequency samples, live monitor values, and future analysis images.
Install the optional dependencies:

```bash
cd local_microscope_copilot
python3 -m venv --system-site-packages .venv
.venv/bin/python -m pip install -r requirements-gui.txt
```

Live GXSM4 use requires the GUI process to import the same system NumPy ABI that
GXSM4 was compiled/linked against. Keep NumPy out of the venv and let
`.venv/bin/python` resolve it from the system site packages.

Start in dry-run mode:

```bash
.venv/bin/python microscope_gradio_gui.py --host 127.0.0.1 --port 7870
```

Start in live mode only when intentionally connected to GXSM4:

```bash
.venv/bin/python microscope_gradio_gui.py --live --host 127.0.0.1 --port 7870
```

If another web service already uses the chosen port, use another local port.

Manage Ollama and Gradio together:

```bash
./copilot_services.sh start all --live
./copilot_services.sh status
./copilot_services.sh restart gradio --live
./copilot_services.sh stop all
```

The service helper stores pid files under `.run/` and logs under `logs/`. It
stops only the processes it started itself.

## Recommended Models

Current machine, CPU-only:

```bash
cd local_microscope_copilot
ollama pull qwen3:4b
./local_microscope_copilot.py --model qwen3:4b
```

With a 16 GB AMD Radeon RX 9070:

```bash
ollama pull qwen3:8b
./local_microscope_copilot.py --model qwen3:8b
```

If the GPU path is fast and stable, try:

```bash
ollama pull qwen3:14b
./local_microscope_copilot.py --model qwen3:14b
```

Gemma alternative:

```bash
ollama pull gemma3:4b
ollama pull gemma3:12b
./local_microscope_copilot.py --model gemma3:12b
```

For the Gradio GUI:

```bash
./copilot_services.sh restart gradio --live --https --require-auth --host 0.0.0.0 --port 7870 --model gemma3:12b
```

To make Gemma the default without passing `--model`, edit
`local_microscope_copilot_config.json` and set:

```json
"model": "gemma3:12b"
```

Ollama's current Gemma 3 tags include `gemma3:4b`, `gemma3:12b`, and
`gemma3:27b`. Start with `gemma3:12b` on a 16 GB GPU. `gemma3:27b` is useful
to test later, but may spill to CPU or run slowly on 16 GB.

Practical recommendation:

- `qwen3:4b`: responsive CPU fallback, adequate command parser.
- `qwen3:8b`: recommended operator copilot on 16 GB GPU.
- `qwen3:14b`: better reasoning, may be slower/tighter on 16 GB.
- `gemma3:4b`: quick Gemma comparison.
- `gemma3:12b`: recommended first Gemma test on 16 GB GPU.
- `gemma3:27b`: larger Gemma test; likely slower/tighter on 16 GB.

## Install Ollama

If Ollama is not installed:

```bash
cd local_microscope_copilot
sh install_ollama.sh
```

Start Ollama:

```bash
ollama serve
```

In another terminal:

```bash
cd local_microscope_copilot
ollama pull qwen3:4b
./local_microscope_copilot.py
```

## AMD Radeon RX 9070 Notes

The RX 9070 class is supported by recent ROCm/Ollama stacks, but Linux
distribution details matter. This current machine is Debian 13, while AMD's
official ROCm compatibility matrix is primarily Ubuntu/RHEL/Oracle Linux.

After installing the GPU, check:

```bash
rocminfo
ollama ps
```

Then run a model and watch GPU use:

```bash
ollama run qwen3:8b
```

If Ollama falls back to CPU, keep using `qwen3:4b` until ROCm/Ollama GPU
support is confirmed on this OS.

## Safety Model

The local LLM never calls GXSM directly. It asks this wrapper for tools.
Chat hardware writes are parsed by a deterministic SI-unit layer before the
LLM response is used. Physical write requests must include the correct unit
family and are converted to the GXSM base units:

- bias/GVP voltage: `V`, `mV`, `uV`/`µV` -> GXSM Volts
- current setpoint: `nA`, `pA`, `uA`/`µA` -> GXSM nA
- scan range/local distances: `A`/`Å`, `nm`, `pm`, `um`/`µm` -> Angstrom
- scan speed: `A/s`, `nm/s`, `um/s`/`µm/s` -> Angstrom/s
- feedback gains: `dB`
- scan slope: `A/A`
- count-like values such as scan points accept `points`, `px`, or `counts`

Wrong-unit requests are rejected. For example, `set bias 200 mV` is accepted as
`0.2 V`, but `set bias 1 Ang` and `set current 10 mV` are rejected.

Read-only tools include:

- live X/Y/Z/U monitor readback
- dFrequency sampling
- scan analysis
- recent-line slope measurement

Hardware-changing tools require terminal confirmation:

- scan speed
- bias
- current setpoint
- offset moves
- stop/start scan
- GVP program load

GVP execution requires an additional explicit confirmation phrase:

```text
EXECUTE GVP
```

Do not run the wrapper with `--assume-yes` on the live microscope unless you are
intentionally testing in a controlled setting.

## Example Session

```bash
cd local_microscope_copilot
./local_microscope_copilot.py --model qwen3:4b
```

Example prompts:

```text
read the live monitors
sample dFreq 20 times
analyze the current scan and mark hazards
measure recent residual slope over 64 lines
set scan speed to 800 A/s for an 800 A scan
```

The wrapper prints confirmation prompts before any live-changing action.
