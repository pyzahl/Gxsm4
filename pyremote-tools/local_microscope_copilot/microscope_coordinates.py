"""Coordinate and unit transforms for the GXSM4 microscope copilot.

This module is intentionally pure: it performs no GXSM, SHM, dconf, file, or
GUI calls.  Callers must provide already-read scan settings and instrument
gains, then use this single layer for pixel/local/world/controller conversions.
"""

from dataclasses import dataclass
import math


def _float_or(value, default=0.0):
    try:
        if default is None and value is None:
            return None
        if value is None:
            return float(default)
        value = float(value)
        if math.isfinite(value):
            return value
    except Exception:
        pass
    if default is None:
        return None
    return float(default)


def _int_or(value, default=1):
    try:
        if value is None:
            return int(default)
        value = int(round(float(value)))
        return value if value > 0 else int(default)
    except Exception:
        return int(default)


@dataclass(frozen=True)
class LocalPointA:
    """Right-handed local scan coordinates in Angstrom."""

    x_A: float
    y_A: float

    def as_dict(self):
        return {
            "local_scan_x_A": float(self.x_A),
            "local_scan_y_A": float(self.y_A),
            "ScanX_A": float(self.x_A),
            "ScanY_A": float(self.y_A),
        }


@dataclass(frozen=True)
class WorldPointA:
    """Non-rotated world/Offset coordinates in Angstrom."""

    x_A: float
    y_A: float

    def as_dict(self):
        return {
            "world_x_A": float(self.x_A),
            "world_y_A": float(self.y_A),
        }


@dataclass(frozen=True)
class PixelPoint:
    """Image pixel coordinates.  py line 0 is the image top."""

    px: float
    py: float

    def as_dict(self):
        return {"px": int(round(self.px)), "py": int(round(self.py))}


@dataclass(frozen=True)
class ScanGeometry:
    """
    Complete scan-frame geometry.

    Conventions:
    - local scan frame is right-handed: +X right, +Y up.
    - image line numbers increase downward from top line 0.
    - top line is local Y = +RangeY/2; bottom is -RangeY/2.
    - OffsetX/Y are non-rotated world coordinates for the scan center.
    - Rotation is around the scan center.
    """

    range_x_A: float
    range_y_A: float
    points_x: int
    points_y: int
    offset_x_A: float = 0.0
    offset_y_A: float = 0.0
    rotation_deg: float = 0.0
    scan_x_A: float = 0.0
    scan_y_A: float = 0.0

    @classmethod
    def from_settings(cls, settings=None, image_shape=None):
        settings = dict(settings or {})
        if image_shape is not None:
            ny, nx = image_shape[:2]
        else:
            dims = settings.get("dimensions") or []
            nx = _int_or(dims[0], 1) if len(dims) > 0 else 1
            ny = _int_or(dims[1], nx) if len(dims) > 1 else nx
        return cls(
            range_x_A=_float_or(settings.get("RangeX"), nx),
            range_y_A=_float_or(settings.get("RangeY"), ny),
            points_x=_int_or(settings.get("PointsX"), nx),
            points_y=_int_or(settings.get("PointsY"), ny),
            offset_x_A=_float_or(settings.get("OffsetX"), 0.0),
            offset_y_A=_float_or(settings.get("OffsetY"), 0.0),
            rotation_deg=_float_or(settings.get("Rotation"), 0.0),
            scan_x_A=_float_or(settings.get("ScanX"), 0.0),
            scan_y_A=_float_or(settings.get("ScanY"), 0.0),
        )

    def as_settings(self):
        return {
            "RangeX": float(self.range_x_A),
            "RangeY": float(self.range_y_A),
            "PointsX": int(self.points_x),
            "PointsY": int(self.points_y),
            "OffsetX": float(self.offset_x_A),
            "OffsetY": float(self.offset_y_A),
            "Rotation": float(self.rotation_deg),
            "ScanX": float(self.scan_x_A),
            "ScanY": float(self.scan_y_A),
            "StepsX": self.step_x_A,
            "StepsY": self.step_y_A,
        }

    @property
    def step_x_A(self):
        return float(self.range_x_A) / max(float(self.points_x) - 1.0, 1.0)

    @property
    def step_y_A(self):
        return float(self.range_y_A) / max(float(self.points_y) - 1.0, 1.0)

    @property
    def theta_rad(self):
        return math.radians(float(self.rotation_deg))

    def pixel_to_local(self, px, py):
        local_x = float(px) * self.step_x_A - float(self.range_x_A) / 2.0
        local_y = float(self.range_y_A) / 2.0 - float(py) * self.step_y_A
        return LocalPointA(local_x, local_y)

    def local_to_pixel(self, local_x_A, local_y_A):
        px = (float(local_x_A) + float(self.range_x_A) / 2.0) / max(self.step_x_A, 1e-12)
        py = (float(self.range_y_A) / 2.0 - float(local_y_A)) / max(self.step_y_A, 1e-12)
        return PixelPoint(px, py)

    def local_to_world(self, local_x_A, local_y_A):
        theta = self.theta_rad
        lx = float(local_x_A)
        ly = float(local_y_A)
        world_dx = lx * math.cos(theta) + ly * math.sin(theta)
        world_dy = -lx * math.sin(theta) + ly * math.cos(theta)
        return WorldPointA(self.offset_x_A + world_dx, self.offset_y_A + world_dy)

    def local_vector_to_world(self, local_x_A, local_y_A):
        """Rotate a local vector into world axes without applying OffsetX/Y."""
        theta = self.theta_rad
        lx = float(local_x_A)
        ly = float(local_y_A)
        return WorldPointA(
            lx * math.cos(theta) + ly * math.sin(theta),
            -lx * math.sin(theta) + ly * math.cos(theta),
        )

    def world_to_local(self, world_x_A, world_y_A):
        theta = self.theta_rad
        dx = float(world_x_A) - float(self.offset_x_A)
        dy = float(world_y_A) - float(self.offset_y_A)
        local_x = dx * math.cos(theta) - dy * math.sin(theta)
        local_y = dx * math.sin(theta) + dy * math.cos(theta)
        return LocalPointA(local_x, local_y)

    def world_vector_to_local(self, world_x_A, world_y_A):
        """Rotate a world vector into local scan axes without applying offsets."""
        theta = self.theta_rad
        dx = float(world_x_A)
        dy = float(world_y_A)
        return LocalPointA(
            dx * math.cos(theta) - dy * math.sin(theta),
            dx * math.sin(theta) + dy * math.cos(theta),
        )

    def local_gradient_to_world(self, dz_dlocal_x, dz_dlocal_y):
        """
        Rotate a local height gradient into world X/Y components.

        Gradients transform as covectors, so this intentionally differs from
        `local_vector_to_world()` in the sign pattern.
        """
        theta = self.theta_rad
        gx = float(dz_dlocal_x)
        gy = float(dz_dlocal_y)
        return WorldPointA(
            gx * math.cos(theta) - gy * math.sin(theta),
            gx * math.sin(theta) + gy * math.cos(theta),
        )

    def pixel_to_world(self, px, py):
        local = self.pixel_to_local(px, py)
        return self.local_to_world(local.x_A, local.y_A)

    def pixel_record(self, px, py):
        local = self.pixel_to_local(px, py)
        world = self.local_to_world(local.x_A, local.y_A)
        rec = {}
        rec.update(local.as_dict())
        rec.update(world.as_dict())
        rec.update(
            {
                "OffsetX_A": float(self.offset_x_A),
                "OffsetY_A": float(self.offset_y_A),
                "Rotation_deg": float(self.rotation_deg),
                "pixel_size_x_A": self.step_x_A,
                "pixel_size_y_A": self.step_y_A,
                "pixel_origin": "top-left",
                "scan_origin": "scan center, right handed X-right/Y-up",
                "scan_y_note": (
                    "Line numbers count downward from top=0, while physical "
                    "local ScanY is positive upward."
                ),
            }
        )
        return rec

    def scan_extent_for_fetched_lines(self, fetched_y_count=None):
        fetched = _float_or(fetched_y_count, self.points_y)
        x_left = -0.5 * float(self.range_x_A)
        x_right = 0.5 * float(self.range_x_A)
        y_top = 0.5 * float(self.range_y_A)
        y_bottom = y_top - float(self.range_y_A) * fetched / max(float(self.points_y), 1.0)
        return (x_left, x_right, y_bottom, y_top)

    def footprint(self):
        corners_px = {
            "top_left": (0, 0),
            "top_right": (self.points_x - 1, 0),
            "bottom_right": (self.points_x - 1, self.points_y - 1),
            "bottom_left": (0, self.points_y - 1),
        }
        corners = {}
        for name, (px, py) in corners_px.items():
            world = self.pixel_to_world(px, py)
            corners[name] = {
                "world_x_A": world.x_A,
                "world_y_A": world.y_A,
                "px": int(px),
                "py": int(py),
            }
        return {
            "OffsetX_A": float(self.offset_x_A),
            "OffsetY_A": float(self.offset_y_A),
            "RangeX_A": float(self.range_x_A),
            "RangeY_A": float(self.range_y_A),
            "PointsX": int(self.points_x),
            "PointsY": int(self.points_y),
            "Rotation_deg": float(self.rotation_deg),
            "corners": corners,
        }


@dataclass(frozen=True)
class InstrumentGains:
    """
    Fast monitor conversion factors.

    `AVxyz` is treated as Angstrom per raw controller-monitor Volt for
    rt_query_xyz() values.  The amplifier gain is retained as metadata and is
    not applied a second time.
    """

    x_A_per_controller_V: float = None
    y_A_per_controller_V: float = None
    z_A_per_controller_V: float = None
    x_amplifier_gain: float = None
    y_amplifier_gain: float = None
    z_amplifier_gain: float = None

    @classmethod
    def from_parsed(cls, values=None):
        values = dict(values or {})
        return cls(
            x_A_per_controller_V=values.get("X_A_per_controller_V") or values.get("X_A_to_V"),
            y_A_per_controller_V=values.get("Y_A_per_controller_V") or values.get("Y_A_to_V"),
            z_A_per_controller_V=values.get("Z_A_per_controller_V") or values.get("Z_A_to_V"),
            x_amplifier_gain=values.get("X_amplifier_gain"),
            y_amplifier_gain=values.get("Y_amplifier_gain"),
            z_amplifier_gain=values.get("Z_amplifier_gain"),
        )

    def axis_scale_A_per_V(self, axis):
        axis = str(axis).upper()
        if axis == "X":
            return _float_or(self.x_A_per_controller_V, None)
        if axis == "Y":
            return _float_or(self.y_A_per_controller_V, None)
        if axis == "Z":
            return _float_or(self.z_A_per_controller_V, None)
        return None

    def controller_volts_to_A(self, axis, raw_V):
        scale = self.axis_scale_A_per_V(axis)
        if scale is None or raw_V is None:
            return None
        value = float(raw_V) * float(scale)
        return float(value) if math.isfinite(value) else None

    def A_to_controller_volts(self, axis, value_A):
        scale = self.axis_scale_A_per_V(axis)
        if scale is None or abs(float(scale)) < 1e-12 or value_A is None:
            return None
        value = float(value_A) / float(scale)
        return float(value) if math.isfinite(value) else None
