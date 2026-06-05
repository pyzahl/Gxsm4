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
- `local_microscope_copilot_config.json`: model/service configuration.
- `microscope_controller_config.json`: controller endpoints, safety ranges,
  confirmations, GVP defaults, and planner limits.
- `microscope_gui_config.json`: GUI widget defaults, refresh periods, display
  gauge ranges, and default action-form values.
- `local_microscope_copilot_setup.md`: install and run instructions.
- `requirements-gui.txt`: Python packages for the optional Gradio GUI.
- `copilot_services.sh`: local start/stop/restart/status helper for Ollama and
  the Gradio GUI.
- `install_ollama.sh`: downloaded official Ollama Linux installer.
- `docs/`: microscope controller manual, tip-improvement plan, and codebase
  overview notes. Start with `docs/project_memory.md` to recover the current
  design context and operator-taught conventions.
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

Gemma alternative for comparison on the 16 GB GPU:

```bash
ollama pull gemma3:12b
./local_microscope_copilot.py --model gemma3:12b
```

For the Gradio GUI/service wrapper, restart only Gradio with the model override:

```bash
./copilot_services.sh restart gradio --live --https --require-auth --host 0.0.0.0 --port 7870 --model gemma3:12b
```

Smaller/larger Gemma checks:

```bash
ollama pull gemma3:4b    # quick/CPU-ish comparison
ollama pull gemma3:27b   # may exceed practical 16 GB GPU memory or run slowly
```

See `local_microscope_copilot_setup.md` for details and safety notes.

## Gradio GUI

Install the optional GUI dependencies:

```bash
cd local_microscope_copilot
python3 -m venv --system-site-packages .venv
.venv/bin/python -m pip install -r requirements-gui.txt
```

For live GXSM4 use, the venv must see the same system Python and NumPy ABI that
GXSM4 was compiled/linked against. Do not install a separate pip NumPy wheel in
`.venv`; NumPy should resolve to the system package, for example
`/usr/lib/python3/dist-packages/numpy`.

Start the GUI on localhost:

```bash
.venv/bin/python microscope_gradio_gui.py --host 127.0.0.1 --port 7870
```

Use `--live` only when intentionally connected to the live GXSM4 microscope:

```bash
.venv/bin/python microscope_gradio_gui.py --live --host 127.0.0.1 --port 7870
```

Start the GUI with HTTPS using the local Grafana certificate/key:

```bash
.venv/bin/python microscope_gradio_gui.py --live --host 0.0.0.0 --port 7870 --https
```

Enable the microscope GUI login by providing a password outside git. The
default username is `microscope`.

```bash
export MICROSCOPE_COPILOT_PASSWORD='choose-a-local-password'
.venv/bin/python microscope_gradio_gui.py --live --host 0.0.0.0 --port 7870 --https --require-auth
```

Or use a local password file, which is ignored by git:

```bash
printf '%s\n' 'choose-a-local-password' > .microscope_gui_password
chmod 600 .microscope_gui_password
.venv/bin/python microscope_gradio_gui.py --live --host 0.0.0.0 --port 7870 --https --require-auth
```

Auth can also be customized with `--auth-user`, `--auth-password-env`, and
`--auth-password-file`. If `--require-auth` is omitted and no password is found,
the GUI still starts but prints a warning when bound to a non-localhost address.

The default HTTPS paths are:

- certificate: `/etc/grafana/ltncafm_cfn_bnl_gov.pem`
- private key: `/etc/grafana/ltncafm_cfn_bnl_gov_privkey.key`

The user running Gradio must have read access to both files. You can also pass
explicit paths with `--ssl-certfile` and `--ssl-keyfile`.

Or use the service helper:

```bash
./copilot_services.sh start all --live
./copilot_services.sh status
./copilot_services.sh restart gradio --live
./copilot_services.sh restart gradio --live --https --require-auth
./copilot_services.sh stop all
```

The helper records only processes it starts in `.run/*.pid`; it will not kill an
external Ollama or web server that was started elsewhere.

## Configuration

The GUI loads three JSON files by default:

- `local_microscope_copilot_config.json`: Ollama host/model choices and service
  notes.
- `microscope_controller_config.json`: live-control policy, safety limits,
  THV endpoint, GVP timing/defaults, tip-loop bounds, and read-only GXSM dconf
  key mappings for current values/adjustment limits.
- `microscope_gui_config.json`: front-end defaults such as scan-line fetch
  counts, gains, form defaults, and visual gauge ranges.

Safety/concurrency rule: the live monitor timer may read only the fast
read-only SHM monitor block through `rt_query_xyz()` and `rt_query_rpspmc()`.
PySHM command paths such as `gxsm.get`, `gxsm.set`, `gxsm.action`, and
`get_slice` must never overlap. The optional scan-image auto refresh uses the
GUI microscope-operation gate and skips refresh cycles while the Tip Tune
Planner or another PySHM action is active. Current-tip marker reads remain
explicit button actions. PySHM commands use a process-local lock plus the
cross-process lock file `/tmp/gxsm4_pyshm_transaction.lock`; external scripts
should use the same wrapper/lock path.

Override paths when needed:

```bash
.venv/bin/python microscope_gradio_gui.py \
  --config local_microscope_copilot_config.json \
  --controller-config microscope_controller_config.json \
  --gui-config microscope_gui_config.json
```

The GUI separates Ollama chat from explicit microscope buttons. Live-changing
buttons require an arm checkbox; loaded-GVP execution additionally requires
typing `EXECUTE GVP`.

The Chat tab now has a deterministic microscope intent router in front of
Ollama. It handles clear operator commands and readbacks using controller code,
then lets Ollama answer only when the request is advisory or ambiguous. The
router understands aliases and recent context, for example:

- `read bias`, `show current setpoint`, `report scan geometry`
- `set bias to 0.2 V`, `set current setpoint to 10 pA`
- `increase bias by 0.05 V`, `make scan speed 20% faster`
- `start scan`, `stop scan`, and short context requests like `start it`
- `analyze tip shape`, routed to deterministic scan/tip analysis

Live-changing chat commands still require Control Level 1 or higher plus the
`Arm Level 1 chat actions` checkbox. Relative changes require a valid current
readback first. Requests outside Level-1 safety limits are blocked before
Ollama is involved, and language-model replies are still prevented from
claiming hardware actions that did not pass through the deterministic gate.

The `Live Microscope State Monitor` tab includes a read-only fast XYZ monitor
that updates about 5 times per second from `gxsm4process.rt_query_xyz()` and
`rt_query_rpspmc()` fast SHM mapped controller Volts. The timer uses cached or
configured instrument gains only. Use `Full Refresh / Update Gains` to refresh
`gxsm.get_instrument_gains()` and the GVP monitor values through PySHM. Visible
X/Y/Z values are converted to Angstroms with `A = Vmonitor * AVxyz`, where the
current dict return provides `Vxyz` and `AVxyz`. Raw controller Volts and
effective piezo Volts remain in the JSON report. Bias, current, GVP U, PAC
amplitude, and PAC dFreq are shown with 6 significant digit formatting. PAC
amplitude uses a +/-10 mV gauge. A 2D XY panel is scaled in controller/world
Angstroms using the configured maximum controller voltage, default +/-5 V,
converted with `AVxyz`. `AVxyz` already includes the active instrument gains.
Known large hazards are overlaid directly in that same Angstrom coordinate
system.

The `Tip Tune Planner` tab keeps a bottom-page activity/status ledger with the
current action, next pending action, timestamps, completion status, blocked
reasons, and recommended next steps. It writes append-only history to
`gui_outputs/tip_planner_activity.jsonl` and a current snapshot to
`gui_outputs/tip_planner_activity_state.json`. Use `Stop Tip Tune Planner Loop`
for cooperative cancellation, and `Clear Planner / Landscape History` after a
hyper jump or new-world relocation. Large-hazard stops now include a computed
hazard-avoiding offset target when possible, checked against the full rotated
scan footprint rather than only the scan center.

### Control Levels

- **Level 0: Hardware connected, monitoring only.** Read-only monitor, dFreq,
  scan fetch, plotting, and analysis tools.
- **Level 1: Basic operation within default safety margins.** Start/stop scan,
  bias up to +/-1 V, current setpoint up to 0.05 nA, normal scan-speed changes,
  feedback CP/CI changes, safe GVP load/execute with explicit confirmation.
- **Level 2: Advanced GVP and more aggressive tip tuning.** Reserved for future
  workflows.
- **Level 3: Extreme GVP, coarse motion, hyper jumps, auto approach.** The GUI
  now includes a separate protected tab for THV5 coarse controls, including
  generic X/Y/Z coarse moves, a Z-down shortcut, and watchdog-style auto
  approach. These actions require Control Level 3, a Level 3 arm checkbox, and
  typing `EXECUTE LEVEL 3`. They are dry-run safe unless the GUI is started with
  `--live`. Extra-protected large coarse moves allow burstcounts up to 5000
  with fixed 0.5 s period and require `EXECUTE LEVEL 3 LARGE MOTION`. Any
  `Z, -1` coarse move is dangerous fast tip-down motion. The red
  **EMERGENCY STOP COARSE MOTION** button is deliberately not gated by arming or
  confirmation: it immediately sends zero-step, zero-volt stop commands for the
  last coarse axis/direction first, then fans out stop commands across all
  coarse axes.

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
