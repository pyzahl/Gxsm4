# pyremote.cpp Reference

This note documents the GXSM4 C++ counterpart for the Python pyremote bridge:

`plug-ins/common/pyremote.cpp`

It is intended for developers maintaining external tools such as
`gxsm4process.py`, the local microscope copilot, and the Gradio GUI. It focuses
on the live shared-memory bridge, not on the full historical embedded-console
manual text in the file.

The live target is an LT-STM/AFM. Treat any method that writes GUI values,
starts/stops scans, runs actions, moves the tip, or executes GVP/VP as
hardware-affecting.

## Main Roles

`pyremote.cpp` provides two related interfaces:

- An embedded GXSM4 Python console in `Tools/Pyremote Console`.
- An external-process PySHM server used by `gxsm4process.py`.

The external interface is the important one for the copilot. GXSM4 owns a
single shared-memory request/response block and exposes the Python-callable
`gxsm.*` methods through the C++ `GxsmPyMethods` table.

## Source Structure

Important sections:

| Area | Purpose |
| --- | --- |
| Plugin metadata and manual text | Defines the `Pyremote` plugin and legacy manual section. |
| `py_gxsm_console` | Embedded Python console, script loading, action scripts, stdout/stderr routing. |
| `remote_*` functions | C++ implementations of Python-callable `gxsm.*` methods. |
| `GxsmPyMethods` | Export table used both by embedded Python and the PySHM server. |
| Pickle helpers | `pickle_pyobject_to_buffer` and `unpickle_from_buffer`. |
| `PySHMServer_Run` | POSIX shared-memory server waiting on `SIGUSR1`. |

## PySHM Transaction Model

The PySHM server creates and maps:

```text
/gxsm4_py_shm_data_block
```

The block is currently:

```text
16 MiB + 128 bytes
```

The layout used by the server is:

| Offset | Meaning |
| --- | --- |
| `0..63` | Method name string, compared to `GxsmPyMethods[*].ml_name`. |
| `100..107` | Signed 64-bit return/status code written by GXSM4. |
| `128..` | Pickle payload block. The first 8 bytes hold payload length, followed by pickle bytes. |

The request flow is:

1. External Python writes the method name into the first 64 bytes.
2. External Python pickles the argument tuple into the data block at offset
   `128`.
3. External Python signals the GXSM4 process with `SIGUSR1`.
4. `PySHMServer_Run` wakes from `sigwait`.
5. GXSM4 looks up the method in `GxsmPyMethods`.
6. GXSM4 unpickles the argument tuple.
7. GXSM4 calls the matching `remote_*` function.
8. GXSM4 pickles the return object into the same data block.
9. GXSM4 writes the status code at offset `100`.
10. External Python copies the return bytes and unpickles them.

This is a single transaction buffer. It is not a queue and it is not safe for
overlapping requests.

## Return Status Codes

`PySHMServer_Run` writes these status codes at offset `100`:

| Code | Meaning |
| --- | --- |
| `0` | Return data is ready. |
| `-1` | Return data is invalid, usually too large or not pickleable. |
| `-2` | Embedded Python is not initialized. |
| `-3` | Unsupported method flags. Current table expects `METH_VARARGS`. |
| `-4` | Unknown method name. |

## Concurrency Rule

Only one PySHM command may be active at a time across all external processes.

Do not interleave:

- `gxsm.get`
- `gxsm.gets`
- `gxsm.set`
- `gxsm.action`
- `gxsm.rtquery`
- `gxsm.startscan` / `gxsm.stopscan`
- `gxsm.moveto_scan_xy`
- `gxsm.get_slice`
- `gxsm.get_probe_event`
- any GVP/VP operation routed through actions or GUI refs

The current copilot wrapper uses:

- a process-local `threading.RLock`
- a cross-process lock file, default
  `/tmp/gxsm4_pyshm_transaction.lock`

Keep all external tools on the same `GXSM_PYSHM_LOCK_FILE` if they can run at
the same time. Timer callbacks must not call PySHM methods unless they use the
same operation gate and skip while another operation is active.

Fast read-only SHM monitor paths that do not use the PySHM method/pickle block
can run independently, but large image reads via `get_slice` must stay
serialized.

## Embedded Main-Thread Handoff

Many `remote_*` functions cannot touch GTK/GXSM state directly from arbitrary
threads. They schedule work onto the GXSM main context with `g_idle_add` and
then wait for completion using the `WAIT_JOIN_MAIN` macro.

Examples:

- `remote_set` schedules `main_context_set_entry_from_thread`.
- `remote_action` schedules `main_context_action_from_thread`.
- `remote_rtquery` schedules `main_context_rtquery_from_thread`.
- `remote_startscan` schedules toolbar action emission.
- `remote_moveto_scan_xy` schedules the hardware move and GUI update.

This means an external PySHM request can block while waiting for GXSM's main
loop. Avoid long-running Python actions inside GXSM that starve the GTK main
loop.

## Exported Method Table

The `GxsmPyMethods` table maps public Python method names to C++ functions.

Core control and discovery:

| Method | Purpose |
| --- | --- |
| `help()` | Print/list pyremote methods. |
| `set(refname, value)` | Set a GUI/remote entry. Value is passed as a string. |
| `get(refname)` | Read a GUI/remote entry as a value where possible. |
| `gets(refname)` | Read a GUI/remote entry as user string including units. |
| `list_refnames()` | List remote GUI entry IDs. |
| `action(action_id[, arg])` | Trigger a remote action/menu/button callback. |
| `list_actions()` | List known action IDs. |
| `rtquery(query)` | Hardware real-time query. |
| `y_current()` | Current scan line from hardware RTQuery. |
| `moveto_scan_xy(x, y)` | Move tip to local ScanX/ScanY within current scan range. |

Scan/data creation and metadata:

| Method | Purpose |
| --- | --- |
| `createscan(ch, nx, ny, nv, rx, ry, array, append)` | Create scan data from integer buffer. |
| `createscanf(ch, nx, ny, nv, rx, ry, array, append)` | Create scan data from float buffer. |
| `get_scan_unit(ch, axis)` | Read scan unit metadata. |
| `set_scan_unit(ch, axis, unit_id, label)` | Set scan unit metadata. |
| `set_scan_lookup(ch, axis, start, end)` | Set lookup mapping for X/Y/L. |
| `get_geometry(ch)` | Return `[rx, ry, x0, y0, alpha]`. |
| `get_differentials(ch)` | Return `[dx, dy, dz, dl]`. |
| `get_dimensions(ch)` | Return `[nx, ny, nv, nt]`. |
| `get_instrument_gains()` | Return instrument conversion/gain dictionary. |
| `get_data_pkt(ch, x, y, v, t)` | Interpolated data value. |
| `put_data_pkt(value, ch, x, y, v, t)` | Put one data value. |
| `get_slice(ch, v, t, yi, yn)` | Return `yn` image lines as NumPy array. |
| `get_slice_v(ch, x, t, yi, yn)` | Return V-stack slice at one x position. |
| `get_probe_event(ch, nth)` | Return probe event data, labels, units, XY/pixel position. |
| `set_global_share_parameter(key, x)` | Write plugin/global shared parameter. |
| `get_x_lookup(ch, i)` | Pixel index to X coordinate. |
| `get_y_lookup(ch, i)` | Pixel index to Y coordinate. |
| `get_v_lookup(ch, i)` | V index lookup. |
| `set_x_lookup(ch, i, v)` | Write X lookup. |
| `set_y_lookup(ch, i, v)` | Write Y lookup. |
| `set_v_lookup(ch, i, v)` | Write V lookup. |
| `get_object(ch, n)` | Read object coordinates. |
| `add_marker_object(...)` | Add marker/rectangle/point object. |
| `marker_getobject_action(...)` | Marker object actions. |

Scan control:

| Method | Purpose |
| --- | --- |
| `startscan()` | Emit toolbar scan start. |
| `stopscan()` | Stop scan. |
| `waitscan(blocking=True)` | Return `-1` if no scan in progress, else line index. |
| `scaninit()` | Initialize scan. |
| `scanupdate()` | Update scan hardware/settings. |
| `scanylookup()` | Scan Y lookup helper. |
| `scanline()` | Scan line helper. |

File/channel/view helpers:

| Method | Purpose |
| --- | --- |
| `autosave()` | Auto-save current scans; may return current line and filenames. |
| `autoupdate()` | Auto-update scans. |
| `save()` | Save all/current data. |
| `saveas(ch, path)` | Save channel to file. |
| `load(ch, path)` | Load file into channel. |
| `export(ch, path)` | Export channel/file. |
| `import(ch, path)` | Import file into channel. |
| `save_drawing(ch, time, layer, path)` | Save rendered drawing. |
| `set_view_indices(ch, time, layer)` | Set channel view indices. |
| `autodisplay()` | Autodisplay active channel. |
| `chfname(ch)` | Get channel filename. |
| `chmodea`, `chmodex`, `chmodem`, `chmoden`, `chmodeno` | Channel mode controls. |
| `chview1d`, `chview2d`, `chview3d` | Channel view controls. |
| `quick`, `direct`, `log` | View mode helpers. |
| `crop(ch_src, ch_dst)` | Crop helper. |

Units and miscellaneous:

| Method | Purpose |
| --- | --- |
| `unitbz`, `unitvolt`, `unitev`, `units` | Unit mode helpers. |
| `echo(text)` | Print to terminal/debug. |
| `logev(text)` | Write GXSM log event. |
| `progress(text, fraction)` | Show/update/close GXSM progress info. |
| `add_layerinformation(text, ch)` | Add layer information. |
| `da0(value)` | Analog out placeholder, not available for SRanger. |
| `signal_emit(action)` | Activate a GAction by string. |
| `sleep(n)` | GXSM-safe sleep, `n/10` seconds. |
| `set_sc_label(id, text)` | Set pyremote script-control label. |

## `get_slice` Details

`remote_getslice(ch, v, t, yi, yn)` returns a two-dimensional NumPy array:

```text
shape = (yn, nx)
dtype = double / NPY_DOUBLE
```

Behavior:

- If `t >= 0`, the scan retrieves the selected time element first.
- The read is accepted only when `yi + yn <= ny`.
- If `v >= 0`, data comes from `Z(x, y, v)`.
- If `v < 0`, data comes from the current/default value.
- Values are multiplied by `src->data.s.dz`.
- Memory is allocated with `malloc`, wrapped by `PyArray_SimpleNewFromData`,
  and marked `NPY_ARRAY_OWNDATA`.

The C++ code logs a warning point immediately before creating the NumPy array:

```text
if a segfault happens past this point, there is a clash between python+numpy
running vs. gxsm4 is been compiled/linked with. A custom Python Venv can cause
that. Please align versions.
```

For external tools this means:

- Use system Python/NumPy or a venv created with `--system-site-packages`.
- Do not install a separate pip NumPy wheel into the copilot venv.
- Fetch in chunks rather than trying to move huge images in one request.
- Copy returned arrays on the external side before additional processing.
- Never run `get_slice` concurrently with another PySHM request.
- Give GXSM ample time between PySHM actions. `remote_getslice()` is allowed
  while scanning from the GXSM side, but external tools should not hammer
  `y_current`, `get_slice`, auxiliary-channel fetches, and analysis requests
  back-to-back.
- The external wrapper enforces a default minimum PySHM cadence of 0.5 s
  between completed transactions. Override with `GXSM_PYSHM_MIN_INTERVAL_S`
  only for controlled testing.

## `get_probe_event` Details

`remote_getprobe_event(ch, nth)` returns either:

- a tuple `(data, labels, units, xyij)`, or
- an integer count/status when the requested event is not found.

Special `nth` values:

| `nth` | Meaning |
| --- | --- |
| `-1` | Return the last available event. |
| `-99` | Remove all probe event data from the scan and return `0`. |
| larger than available | Return number of available probe events. |

The data array has shape:

```text
(chunk_size, num_sets)
```

`xyij` is `[world_x, world_y, pixel_i, pixel_j]` for the event position.

## `get_instrument_gains` Details

`remote_get_instrument_gains()` returns a dictionary with:

| Key | Meaning |
| --- | --- |
| `Vxyz` | Current X/Y/Z amplifier gain values. |
| `AVxyz` | Angstrom-per-controller-Volt conversion after the applied gains. |
| `Vxyz0` | Offset-channel X/Y/Z gain values. |
| `AVxyz0` | Offset-channel Angstrom-per-controller-Volt conversion. |
| `BiasGain` | Bias gain conversion. |
| `IVC-nAmpere2V` | Current conversion for nA to V. |
| `IVC-A/V` | Current conversion expressed as A/V style value in code. |
| `nNewton2V` | Force conversion. |
| `dHertz2V` | Frequency-shift conversion. |
| `eV2V` | Energy conversion. |

For the current LT-STM/AFM setup observed in the copilot, `AVxyz` already
includes the effective piezo/amplifier gain. Do not multiply by `Vxyz` again
when converting controller volts to Angstrom.

## `rtquery` Details

`remote_rtquery(query)` routes to:

```cpp
gapp->xsm->hardware->RTQuery(query, u, v, w)
```

Known query strings used by the copilot:

| Query | Meaning |
| --- | --- |
| `X`, `Y`, `Z` | Real-time position monitor classes. |
| `xy`, `zxy`, `o` | Combined position/offset monitor queries, hardware dependent. |
| `f` | dFrequency vector; `fvec[0]` is dFreq in Hz. |
| `s` | Status bits; for planner readiness, ready values include scan/GVP status `0` or completed GVP `5`, busy is `1`. |
| `i` | GPIO watch. |
| `U` | Current bias. |

Some `X ...` query forms can return a NumPy array pointer. Treat those as
advanced and hardware-specific.

## `moveto_scan_xy` Details

`remote_moveto_scan_xy(x, y)` writes local scan coordinates:

```text
main_get_gapp()->xsm->data.s.sx = x
main_get_gapp()->xsm->data.s.sy = y
```

It only accepts coordinates inside:

```text
-RangeX/2 <= x <= RangeX/2
-RangeY/2 <= y <= RangeY/2
```

It then calls the hardware `MovetoXY` conversion path and updates GXSM. This is
a local scan-coordinate move, not a world `OffsetX/Y` move.

## Common Failure Modes

### Interleaved PySHM Request

Symptoms:

- timeout waiting for return status
- corrupted pickle payload
- wrong return type
- GXSM warning or crash during a large data return

Cause:

Two callers wrote to the one shared block before the first request completed.

Fix:

Use the shared lock file and GUI operation gate. Auto-refresh operations should
skip while planner/GVP/manual image fetching is active.

### Python/NumPy ABI Mismatch

Symptoms:

- segfault just after `get_slice` logs its NumPy warning point
- crash only for NumPy-returning methods, while scalar `get`/`set` works

Cause:

GXSM4's embedded Python/NumPy creates the returned NumPy object, then it is
pickled through the bridge. External tooling must be compatible with that
runtime. A pip-installed NumPy wheel in a custom venv can be enough to trigger
trouble.

Fix:

Use system Python/NumPy or a venv with system site packages and no private
NumPy wheel.

### GUI Main Loop Starvation

Symptoms:

- PySHM request hangs even though the external process signaled GXSM
- `set`, `action`, or `startscan` never completes

Cause:

Many methods schedule GTK/GXSM main-thread work and wait. If the GXSM main loop
is blocked by a long embedded Python script or modal dialog, the request cannot
finish.

Fix:

Avoid long blocking work in the embedded console. Avoid sending out-of-limit
values that trigger modal dialogs. Keep safety limits on the external side.

## Maintenance Notes

- Keep `GxsmPyMethods` and `gxsm4process.py` wrappers synchronized.
- If a method can return large data, check that the pickled return fits in the
  16 MiB block.
- Prefer returning plain scalars, strings, lists, dicts, or copied NumPy arrays.
- For new external automation, add safety gates outside `pyremote.cpp`; the C++
  side mostly exposes the raw remote-control capability.
- For beginners debugging transport issues, first test one method in one
  process with system Python before adding Gradio timers, planner loops, or
  multi-process tools.
