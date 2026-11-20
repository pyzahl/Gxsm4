# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

GXSM4 (Gnome X Scanning Microscopy): a GTK4 C++ application for SPM (STM/AFM/SPA-LEED, ...) instrument control, multi-dimensional (up to 4D) data acquisition and visualization, and data analysis. It was ported from Gxsm3 (GTK3). The main active hardware target is **RPSPMC**, an FPGA-based SPM controller on a RedPitaya. The older SoftdB Signal Ranger MK2/MK3 DSP boards are still supported.

## Build / run

Meson + Ninja. The existing local build dir is `bd/`:

```
meson setup bd            # first time only
ninja -C bd               # build
sudo ninja -C bd install  # install (needed to run, see below)
meson test -C bd          # only test: validates the gschema files
```

- There is no unit-test suite and no linter config. Verify changes by building and running `gxsm4`.
- Run the **installed** `gxsm4`. Plug-ins are loaded with `g_module_open` from `<libdir>/gxsm4-plugins/<category>/`, and GSettings schemas must be installed and compiled. A freshly built binary in `bd/` won't pick up plug-in or schema changes until you install.
- Useful flags: `-D N` (core debug level 0–5), `-P N` (plug-in debug level), `-s` (new instance).
- `bd/compile_commands.json` exists for clangd/tooling.
- Optional plug-ins are gated on dependencies in their `meson.build`. For example, `rpspmc_pacpll` only builds if gsl, libsoup-3 and websocketpp (`libwebsocketpp-dev`) are found.
- Native dev platform: Debian 12/13/testing. The build must stay portable to ARM/Raspberry Pi.

## Architecture

### Core (`core-source/`)
- `gxsm_main.cpp`: entry point, command-line options, environment setup (for example, the NVIDIA-only `GDK_DISABLE=gles-api` workaround) before GTK starts.
- `App` (`gxsm_app.*`, `gxsm_window.*`, `gxsm_menucb.cpp`): main application and window, menus (`*.ui` files compiled via `gxsm4.gresources.xml`).
- `Xsm` → `Surface` (`xsm.*`, `surface.*`): central data and instrument state. Holds the array of channels/scans and the active hardware interface.
- `Scan` (`scan.*`, with `TopoGraphicScan`, `SpaScan`): one channel's data plus its view(s). The data lives in `Mem2d` (`mem2d.*`): typed 2D layers × time frames (4D), `ZData` storage, `LayerInformation`, and attached `scan_event` objects (probe data and user events tied to coordinate and time).
- Views: `ViewControl` (`app_view.*`, 2D image, cairo-based `cairo_item.*` objects in `app_vobj.*`), `app_profile.*` (1D profiles), and `vsurf3d.cpp`/`app_v3dcontrol.*` (OpenGL 3D surface view in a GtkGLArea; shaders live in `gl-400/*.glsl`).
- `XSM_Hardware` (`xsmhard.*`): the abstract hardware interface (HwI). Hardware plug-ins subclass it.
- `pcs.*` (`Param_Control`, `Gtk_EntryControl`): unit-aware parameter entry widgets used throughout the GUI. Their values and adjustment ranges persist via GSettings.
- `plugin.h`/`plugin_ctrl.*`: plug-in ABI and loader.
- `dataio.*`: NetCDF scan file I/O (native `.nc` format).

### Plug-ins (`plug-ins/<category>/`)
Each plug-in is a shared module that exports `get_gxsm_plugin_info()`, which returns a `GxsmPlugin` struct (see `core-source/plugin.h`): name, category, menu path, and the init/query/run/cleanup callbacks. Categories:
- `common/`: general tools, including **`pyremote.cpp`**, the embedded Python console and scripting API (numpy access to live scan and probe data).
- `control/`: instrument control panels (scan control, probe/GVP, mover, peak find, ...).
- `hard/`: hardware interfaces (`*_hwi_dev : public XSM_Hardware`). These are `rpspmc_*` (current FPGA controller: PAC-PLL, streaming, GVP vector generation, GVP mover), `sranger_mk2/mk3_*` (DSP boards; `MK2-/MK3-A810_spmcontrol/` hold the DSP firmware sources), `spm_simulator_hwi`, and `spm_template_hwi` (the reference template for writing a new HwI).
- `math/<subcat>/`: data processing (filters, background, transforms, statistics, ...).
- `scan/`: file import/export for foreign formats.

Each category's `meson.build` holds a `plugin_list` of dicts (`name`, optional `sources`, `deps`, `cflags`, `gschemas`). To add a plug-in, add an entry there. A plug-in's gschemas are also run through `gdbus_codegen`.

### GSettings schemas
The core and many plug-ins persist settings in `org.gnome.gxsm4.*.gschema.xml`. The `pcs` and `pcsadjustments` schemas hold per-window `Param_Control` values and ranges. Schemas can be (re)generated from built-in defaults with `--write-gxsm-pcs-gschema` / `--write-gxsm-pcs-adj-gschema` (and `--write-gxsm-preferences-gschema` / `--write-gxsm-gl-preferences-gschema`). At runtime, missing keys are written to `*.gschema.missing.xml` files, which are then merged by hand into the real schema. If you add a new `Param_Control` with a GSettings key, it needs a matching schema entry.

### GUI construction rule: use the gxsm wrappers, never raw GTK
Build almost all GUI windows, entries and controls exclusively with the gxsm custom GTK wrappers in `core-source/gapp_service.h`:
- **Windows:** derive from `AppBase` (`AppWindowInit`, `set_window_geometry(key)`). This gives window geometry persistence, auto placement and the Window menu entry.
- **Layout and widgets:** use `BuildParam` (`new_grid`, `new_grid_with_frame`, `grid_add_label`, `grid_add_widget`, `new_line`, ...).
- **Any text or number entry must be a PCS entry.** Never create a bare `gtk_entry`. Use `BuildParam::grid_add_ec(...)` / `grid_add_ec_with_scale(...)`, which create a `Gtk_EntryControl` bound to a variable with a `UnitObj`, limits and a format. Pass a `remote_id` string. It is prefixed via `set_pcs_remote_prefix()`, registered in the remote list (`AddEntry2RemoteList`), and tagged as `"Adjustment_PCS_Name"`. This is what makes the entry remote-controllable (PyRemote `gxsm.set/get`), persistent through GSettings, and unit-aware.
- **Check buttons, exec/action buttons, radio buttons, etc.:** use the matching `grid_add_check_button*`, `grid_add_exec_button`, `grid_add_radio_button`, ... wrappers. Give each one a `control_id` (or a `PYREMOTE_CHECK_HOOK_KEY` tooltip) so it is reachable remotely.

Every controllable widget carries a unique id string. Keep ids stable, because scripts depend on them, and add matching `pcs`/`pcsadjustments` schema entries for new ones.

### Window placement on Wayland (`gxsm-wm-extension/`, `hyprland_gxsm_config/`)
Gxsm4 is natively multi-window: a main window, many tool and control windows, and one window per scan channel. It needs to restore each window's stored position and size. Wayland doesn't let clients position their own windows, so `AppBase::position_auto()` in `core-source/gapp_service.cpp` (built with `ENABLE_GXSM_WINDOW_MANAGEMENT`) uses a different path per display backend:
- **X11:** direct `XMoveWindow`.
- **Wayland + GNOME Shell:** the custom GNOME Shell extension `gxsm-wm@gxsm4.gnome.org` (`gxsm-wm-extension/extension.js`) exports a D-Bus interface `org.gnome.Shell.Extensions.GxsmWm` at `/org/gnome/Shell/Extensions/GxsmWmExtension` with `SetGeoAction`, `GetGeoAction` and `SetOnTopAction`. Gxsm calls it by spawning `busctl --user call org.gnome.Shell ... SetGeoAction ssiiiiib -- <class> <title> <workspace> x y w h <op>`. This requires `busctl` and the extension to be installed and enabled. Meson installs it system-wide or into `~/.local`, depending on `-Dextension-target=system|home`.
- **Wayland + Hyprland** (detected via `HYPRLAND_INSTANCE_SIGNATURE`): `hyprctl dispatch setfloating` / `movewindowpixel`, plus static window rules generated by `hyprland_gxsm_config/hypr-gxsm-store.sh`.

Windows are matched by **title**. Scan channel windows are matched by the regex `^(ChN:.*)$`, and only channels ≤ 20 are managed. Changing a window title (`AppWindowInit`/`SetTitle`) or its geometry key (`set_window_geometry`) therefore breaks placement and the stored layouts, so keep them stable. Keep the extension's D-Bus signature and the `busctl` call strings in `gapp_service.cpp` in sync. Don't remove this mechanism in favour of "Wayland-correct" approaches, because the multi-window layout depends on it.


### Python remote control
- In-process: the PyRemote plug-in (`plug-ins/common/pyremote.cpp`) runs scripts in its own threads. The script library is in `plug-ins/common/pyremote-gxsm-lib-script-collection`.
- External: `pyremote-tools/` holds standalone scripts that talk to a running gxsm4 over a shared-memory bridge (see `gxsm4process.py`). gxsm4 owns the SHM segments, so external clients must not close them.

## GL / platform notes

The 3D view must work on NVIDIA (proprietary driver, Wayland), on Mesa (llvmpipe, AMD), and must not break the rest of the GUI on ARM/Raspberry Pi, which is GLES-first with desktop GL ≤ 3.1 (the 3D view itself is optional there). Scope driver-specific workarounds to that driver only. To debug context issues, use `GDK_DEBUG=opengl`.

Several 3D views can be open at once, each in its own GtkGLArea context. GTK shares objects between these contexts, but binding state and VAOs are per context. In `vsurf3d.cpp`, the shader programs are shared and reference-counted at file scope (`ProgramName_RefCount`). Everything bound per context (UBOs and their binding points, VAOs, textures, vertex buffers) must be a member of the view instance (`gl_400_3D_visualization` and the plane/icosahedron classes), never a file-scope global.

## Commits (from HACKING)

The first line is a single-sentence summary: capitalized, no trailing period, at most 72 characters, followed by a blank line and a prose body explaining what changed and why.
