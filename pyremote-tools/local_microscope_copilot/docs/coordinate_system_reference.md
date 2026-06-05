# Coordinate System Reference

The copilot uses `microscope_coordinates.py` as the single pure-Python
coordinate conversion layer.  It does not call GXSM, SHM, dconf, the GUI, or
the filesystem.  Callers must pass instrument-returned scan settings and
instrument gains into the class methods.

## Authoritative Inputs

Scan geometry comes from GXSM settings/readbacks:

- `RangeX`, `RangeY`: scan size in Angstrom.
- `PointsX`, `PointsY`: pixel count.
- `OffsetX`, `OffsetY`: non-rotated world coordinate of the scan center.
- `Rotation`: scan rotation in degrees around the scan center.
- `ScanX`, `ScanY`: local tip position inside the current scan frame.

Fast monitor scaling comes from `gxsm.get_instrument_gains()`.

- `Vxyz`: amplifier gains, retained as metadata.
- `AVxyz`: Angstrom per raw controller-monitor Volt for
  `rt_query_xyz()["monitor"]`.
- Use `local_A = monitor_V * AVxyz`.
- Use `monitor_V = local_A / AVxyz`.
- Do not apply `Vxyz` a second time when converting the fast SHM monitor
  volts; `AVxyz` already represents the active instrument scaling used here.

## Coordinate Frames

Pixel frame:

- `px`, `py` are image pixel coordinates.
- `py = 0` is the top scan line.
- Pixel rows increase downward.

Local scan frame:

- Units are Angstrom.
- Right-handed: positive X right, positive Y up.
- The scan center is `(0, 0)`.
- Top line is `local_scan_y_A = +RangeY/2`.
- Bottom line is `local_scan_y_A = -RangeY/2`.
- Left edge is `local_scan_x_A = -RangeX/2`.
- Pixel spacing is `Range/(Points-1)`.

World/Offset frame:

- Units are Angstrom.
- `OffsetX/Y` are non-rotated world coordinates for the scan center.
- `Rotation` rotates the local scan frame around the scan center.

Fast controller monitor frame:

- `rt_query_xyz()["monitor"]` returns raw controller Volts.
- Convert to local Angstrom with `InstrumentGains.controller_volts_to_A()`.
- The live XY monitor dot and hazard overlays must both use the same local
  Angstrom frame before plotting.

## Core API

`ScanGeometry.from_settings(settings)` builds a geometry object from GXSM
readback dictionaries.

Important methods:

- `pixel_to_local(px, py)`
- `local_to_pixel(local_x_A, local_y_A)`
- `local_to_world(local_x_A, local_y_A)`
- `world_to_local(world_x_A, world_y_A)`
- `local_vector_to_world(local_dx_A, local_dy_A)`
- `world_vector_to_local(world_dx_A, world_dy_A)`
- `local_gradient_to_world(dz_dlocal_x, dz_dlocal_y)`
- `pixel_to_world(px, py)`
- `pixel_record(px, py)`
- `scan_extent_for_fetched_lines(fetched_y_count)`
- `footprint()`

Displacement vectors and height gradients are not the same object.  A local
motion vector, such as "shift image left", uses `local_vector_to_world()`.
A fitted topographic slope or gradient uses `local_gradient_to_world()`.

`InstrumentGains.from_parsed(values)` builds conversion factors from the parsed
`gxsm.get_instrument_gains()` result.

Important methods:

- `axis_scale_A_per_V(axis)`
- `controller_volts_to_A(axis, raw_V)`
- `A_to_controller_volts(axis, value_A)`

## Implementation Rule

Do not duplicate pixel/local/world/controller conversion formulas in GUI,
planner, or action code.  If a new workflow needs a transform, add or use a
method in `microscope_coordinates.py`, then call it from the workflow.
