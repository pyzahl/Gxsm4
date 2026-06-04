"""Scan plotting, refresh, and analysis mixin for the GXSM4 Gradio GUI."""

import json
import time
import traceback

import matplotlib.pyplot as plt
import numpy as np


class ScanGuiMixin:
    """Read-only scan image plotting, live refresh, and analysis helpers."""

    def scan_extent_A(self, image, meta, channel):
        """Return imshow extent in local scan Angstroms, preserving aspect."""
        ny, nx = image.shape
        try:
            settings = self.runner._scan_settings(int(channel))
            range_x = float(settings.get("RangeX"))
            range_y = float(settings.get("RangeY"))
            points_y = float(settings.get("PointsY") or meta.get("dimensions", [nx, ny])[1])
        except Exception:
            dims = meta.get("dimensions", [nx, ny])
            range_x = float(dims[0] or nx)
            range_y = float(dims[1] or ny)
            points_y = float(dims[1] or ny)
        fetched_y = float(meta.get("fetched_y_count", ny) or ny)
        x_left = -0.5 * range_x
        x_right = 0.5 * range_x
        y_top = 0.5 * range_y
        y_bottom = y_top - range_y * fetched_y / max(points_y, 1.0)
        return (x_left, x_right, y_bottom, y_top)

    def pixel_to_local_A(self, px, py, image, extent):
        ny, nx = image.shape
        x_left, x_right, y_bottom, y_top = extent
        x_A = x_left + (float(px) + 0.5) * (x_right - x_left) / max(nx, 1)
        y_A = y_top + (float(py) + 0.5) * (y_bottom - y_top) / max(ny, 1)
        return x_A, y_A

    def plot_scan_image_A(
        self,
        image,
        meta,
        channel,
        title,
        colorbar_label,
        quality=None,
        landscape=None,
    ):
        fig, ax = plt.subplots(figsize=(6, 5), constrained_layout=True)
        extent = self.scan_extent_A(image, meta, channel)
        im = ax.imshow(
            image,
            origin="upper",
            extent=extent,
            cmap="viridis",
            aspect="equal",
        )
        ax.set_title(title)
        ax.set_xlabel("Scan X (A)")
        ax.set_ylabel("Scan Y (A), positive up; line 0 at top")
        fig.colorbar(im, ax=ax, label=colorbar_label)
        if quality or landscape:
            self.annotate_analysis_plot(ax, image, extent, quality, landscape)
        return fig, extent

    def annotate_analysis_plot(self, ax, image, extent, quality=None, landscape=None):
        quality = quality or {}
        landscape = landscape or {}

        hazards = (
            landscape.get("hazards")
            or landscape.get("major_bumps")
            or []
        )
        for hazard in hazards[:12]:
            if "px" not in hazard or "py" not in hazard:
                continue
            x_A, y_A = self.pixel_to_local_A(hazard["px"], hazard["py"], image, extent)
            ax.plot(x_A, y_A, "rx", ms=7, mew=1.5)

        candidates = (
            landscape.get("candidates")
            or landscape.get("flat_candidates")
            or []
        )
        for idx, candidate in enumerate(candidates[:6], start=1):
            px = candidate.get("px")
            py = candidate.get("py")
            if px is None or py is None:
                continue
            x_A, y_A = self.pixel_to_local_A(px, py, image, extent)
            ax.plot(x_A, y_A, "wo", ms=7, mec="k", mew=1.0)
            ax.text(
                x_A,
                y_A,
                str(idx),
                color="black",
                ha="center",
                va="center",
                fontsize=7,
            )

        best = landscape.get("best_candidate") or {}
        if best.get("px") is not None and best.get("py") is not None:
            x_A, y_A = self.pixel_to_local_A(best["px"], best["py"], image, extent)
            ax.plot(x_A, y_A, "c*", ms=12, mec="k", mew=0.8, label="best flat")

        peaks = [
            peak for peak in quality.get("autocorrelation_secondary_peaks", [])
            if peak.get("used_for_tip_verdict")
        ]
        summary_lines = [
            "Tip: {} ({})".format(
                quality.get("verdict", "n/a"),
                quality.get("confidence", "n/a"),
            ),
        ]
        rough = quality.get("flattened_stats", {}).get("roughness_std")
        if rough is not None:
            summary_lines.append("rough std: {:.4g} A".format(float(rough)))
        if peaks:
            peak = peaks[0]
            summary_lines.append(
                "peak: corr={:.3g}, d~{:.1f} A, angle={:.0f} deg".format(
                    float(peak.get("corr", 0.0)),
                    float(peak.get("dist_A_approx", 0.0)),
                    float(peak.get("angle_deg", 0.0)),
                )
            )
        if hazards:
            summary_lines.append("hazards: {}".format(len(hazards)))
        if candidates:
            summary_lines.append("flat candidates: {}".format(len(candidates)))

        ax.text(
            0.01,
            0.99,
            "\n".join(summary_lines),
            transform=ax.transAxes,
            va="top",
            ha="left",
            fontsize=8,
            color="white",
            bbox={"facecolor": "black", "alpha": 0.62, "pad": 4},
        )
        if hazards or candidates or best:
            ax.legend(loc="lower right", fontsize=7)

    def fetch_scan_plot(self, channel, chunk_lines):
        fig, _profile_fig, report = self.fetch_scan_plot_with_profile(
            channel,
            chunk_lines,
        )
        return fig, report

    def fetch_scan_plot_with_profile(self, channel, chunk_lines):
        try:
            image, meta = self.runner.fetch_scan_image_to_line(
                channel=int(channel),
                chunk_lines=int(chunk_lines),
            )
            fig, extent = self.plot_scan_image_A(
                image,
                meta,
                channel,
                "Channel {} scan data, line 0 at top".format(channel),
                "signal",
            )
            report = {
                "meta": self.jsonable(meta),
                "extent_A": list(extent),
                "shape": list(image.shape),
                "min": float(image.min()),
                "max": float(image.max()),
                "mean": float(image.mean()),
                "std": float(image.std()),
            }
            profile_fig = self.plot_last_scan_line_profile(image, meta, extent)
            self.scan_view_state[int(channel)] = {
                "y_current": int(meta.get("y_current", -1)),
                "fetched_y_count": int(meta.get("fetched_y_count", image.shape[0])),
                "timestamp": time.time(),
            }
            return fig, profile_fig, report
        except Exception as exc:
            fig, ax = plt.subplots(figsize=(6, 4), constrained_layout=True)
            ax.text(0.5, 0.5, str(exc), ha="center", va="center", wrap=True)
            ax.set_axis_off()
            profile_fig, profile_ax = plt.subplots(figsize=(6, 2.5), constrained_layout=True)
            profile_ax.text(0.5, 0.5, str(exc), ha="center", va="center", wrap=True)
            profile_ax.set_axis_off()
            return fig, profile_fig, {"error": str(exc), "traceback": traceback.format_exc()}

    def plot_last_scan_line_profile(self, image, meta, extent):
        fig, ax = plt.subplots(figsize=(7, 2.7), constrained_layout=True)
        arr = np.asarray(image, dtype=float)
        if arr.ndim != 2 or arr.shape[0] < 1:
            ax.text(0.5, 0.5, "No scan line available", ha="center", va="center")
            ax.set_axis_off()
            return fig
        line = arr[-1, :]
        nx = line.shape[0]
        x_left, x_right, _y_bottom, _y_top = extent
        x_A = np.linspace(float(x_left), float(x_right), nx, endpoint=False)
        if nx > 0:
            x_A = x_A + 0.5 * (float(x_right) - float(x_left)) / float(nx)
        ax.plot(x_A, line, lw=1.2, color="#1f5fbf")
        ax.set_title(
            "Last fetched scan line: y_current={}, line index={}".format(
                meta.get("y_current", "n/a"),
                max(0, arr.shape[0] - 1),
            )
        )
        ax.set_xlabel("Scan X (A)")
        ax.set_ylabel("Signal")
        ax.grid(True, alpha=0.25)
        return fig

    def current_scan_line_index(self, channel):
        if self.runner.dry_run:
            return -1
        return int(float(self.runner.connect().y_current()))

    def auto_refresh_scan_plot(self, enabled, channel, chunk_lines):
        try:
            import gradio as gr
        except Exception:
            gr = None
        if not enabled:
            update = gr.update() if gr else None
            return update, update, update
        try:
            channel = int(channel)
            y_current = self.current_scan_line_index(channel)
            previous = self.scan_view_state.get(channel, {}).get("y_current")
            if previous is not None and int(previous) == int(y_current):
                update = gr.update() if gr else None
                return update, update, update
            return self.fetch_scan_plot_with_profile(channel, chunk_lines)
        except Exception as exc:
            fig, ax = plt.subplots(figsize=(6, 4), constrained_layout=True)
            ax.text(0.5, 0.5, str(exc), ha="center", va="center", wrap=True)
            ax.set_axis_off()
            profile_fig, profile_ax = plt.subplots(figsize=(6, 2.5), constrained_layout=True)
            profile_ax.text(0.5, 0.5, str(exc), ha="center", va="center", wrap=True)
            profile_ax.set_axis_off()
            return fig, profile_fig, {"error": str(exc), "traceback": traceback.format_exc()}

    def analyze_current_scan(self, channel, chunk_lines, output_prefix):
        try:
            image, meta = self.runner.fetch_scan_image_to_line(
                channel=int(channel),
                chunk_lines=int(chunk_lines),
            )
            pixel = self.runner._pixel_size_A(int(channel))
            quality = self.runner.assess_tip_quality(
                image,
                pixel_size_A=pixel,
                output_prefix=str(self.output_dir / output_prefix),
            )
            landscape = self.nav.analyze_current_scan_landscape_map(
                channel=int(channel),
                output_prefix=str(self.output_dir / (output_prefix + "_landscape")),
            )
            fig, extent = self.plot_scan_image_A(
                image,
                meta,
                channel,
                "Scan image used for analysis",
                "topography / signal",
                quality=quality,
                landscape=landscape,
            )
            report = {
                "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                "fetch_meta": self.jsonable(meta),
                "extent_A": list(extent),
                "pixel_size_A": pixel,
                "tip_quality": quality,
                "landscape": landscape,
            }
            report_path = self.output_dir / (output_prefix + "_gui_report.json")
            report_path.write_text(json.dumps(self.jsonable(report), indent=2))
            return fig, self.jsonable(report), str(report_path)
        except Exception as exc:
            fig, ax = plt.subplots(figsize=(6, 4), constrained_layout=True)
            ax.text(0.5, 0.5, str(exc), ha="center", va="center", wrap=True)
            ax.set_axis_off()
            return fig, {"error": str(exc), "traceback": traceback.format_exc()}, ""
