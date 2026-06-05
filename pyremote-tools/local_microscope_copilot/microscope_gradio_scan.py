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
            feature_class = hazard.get("feature_class", "")
            if feature_class == "molecule_candidate":
                ax.plot(x_A, y_A, "o", color="#78a6c8", ms=4, mec="k", mew=0.3)
            elif hazard.get("count_for_offset_stop"):
                ax.plot(x_A, y_A, "rx", ms=8, mew=1.8)
            else:
                ax.plot(x_A, y_A, "x", color="#ff9f1c", ms=7, mew=1.4)

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
            large = len([h for h in hazards if h.get("count_for_offset_stop")])
            molecules = len([h for h in hazards if h.get("feature_class") == "molecule_candidate"])
            summary_lines.append("large hazards: {}, molecules: {}".format(large, molecules))
        if candidates:
            summary_lines.append("flat candidates: {}".format(len(candidates)))

        selected_idx = landscape.get("selected_candidate_index")
        if selected_idx is not None and candidates:
            try:
                selected_zero = int(selected_idx) - 1
                if 0 <= selected_zero < len(candidates):
                    selected = candidates[selected_zero]
                    if selected.get("px") is not None and selected.get("py") is not None:
                        x_A, y_A = self.pixel_to_local_A(
                            selected["px"],
                            selected["py"],
                            image,
                            extent,
                        )
                        ax.plot(
                            x_A,
                            y_A,
                            marker="o",
                            ms=14,
                            mfc="none",
                            mec="#ffdd00",
                            mew=2.2,
                            label="selected flat",
                        )
            except Exception:
                pass

        current_tip = landscape.get("current_tip_position") or {}
        try:
            tip_x = current_tip.get("scan_x_A")
            tip_y = current_tip.get("scan_y_A")
            if tip_x is None:
                tip_x = current_tip.get("dsp-GVP-XS-MONITOR")
            if tip_y is None:
                tip_y = current_tip.get("dsp-GVP-YS-MONITOR")
            if tip_x is not None and tip_y is not None:
                tip_x = float(tip_x)
                tip_y = float(tip_y)
                ax.plot(
                    tip_x,
                    tip_y,
                    marker="+",
                    color="#ff3333",
                    ms=16,
                    mew=2.5,
                    label="current tip",
                )
                ax.plot(
                    tip_x,
                    tip_y,
                    marker="o",
                    mfc="none",
                    mec="#ff3333",
                    ms=10,
                    mew=1.5,
                )
                ax.text(
                    tip_x,
                    tip_y,
                    " tip",
                    color="#ff3333",
                    fontsize=8,
                    ha="left",
                    va="bottom",
                    bbox={"facecolor": "white", "alpha": 0.65, "pad": 1.5},
                )
                summary_lines.append("tip X/Y: {:.4g}, {:.4g} A".format(tip_x, tip_y))
        except Exception:
            pass

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

    def plot_scan_analysis_with_dfreq(
        self,
        topo_image,
        topo_meta,
        dfreq_image,
        dfreq_meta,
        quality=None,
        landscape=None,
        live_dfreq=None,
    ):
        fig, axes = plt.subplots(1, 2, figsize=(12, 5), constrained_layout=True)
        topo_extent = self.scan_extent_A(topo_image, topo_meta, 0)
        topo_im = axes[0].imshow(
            topo_image,
            origin="upper",
            extent=topo_extent,
            cmap="viridis",
            aspect="equal",
        )
        axes[0].set_title("Topo Z (Ch 0)")
        axes[0].set_xlabel("Scan X (A)")
        axes[0].set_ylabel("Scan Y (A), positive up")
        fig.colorbar(topo_im, ax=axes[0], label="Z (A)")
        self.annotate_analysis_plot(
            axes[0],
            topo_image,
            topo_extent,
            quality=quality,
            landscape=landscape,
        )

        if dfreq_image is not None:
            dfreq_extent = self.scan_extent_A(dfreq_image, dfreq_meta or topo_meta, 2)
            df_arr = np.asarray(dfreq_image, dtype=float)
            finite = df_arr[np.isfinite(df_arr)]
            if finite.size:
                lo, hi = np.nanpercentile(finite, [2, 98])
                if lo == hi:
                    lo, hi = float(np.nanmin(finite)), float(np.nanmax(finite))
            else:
                lo, hi = None, None
            df_im = axes[1].imshow(
                df_arr,
                origin="upper",
                extent=dfreq_extent,
                cmap="coolwarm",
                aspect="equal",
                vmin=lo,
                vmax=hi,
            )
            axes[1].set_title("Scan dFreq (Ch 2)")
            axes[1].set_xlabel("Scan X (A)")
            axes[1].set_ylabel("Scan Y (A), positive up")
            fig.colorbar(df_im, ax=axes[1], label="dFreq (Hz)")
        else:
            axes[1].text(
                0.5,
                0.5,
                "Channel 2 dFreq image unavailable",
                ha="center",
                va="center",
                wrap=True,
            )
            axes[1].set_axis_off()

        summary = []
        sharp = (quality or {}).get("topo_feature_sharpness") or {}
        if sharp.get("category"):
            summary.append("sharpness: {}".format(sharp.get("category")))
        width = sharp.get("characteristic_edge_width_A_approx")
        if width is not None:
            summary.append("edge width~{:.3g} A".format(float(width)))
        live_stats = (live_dfreq or {}).get("stats") or {}
        mean_df = live_stats.get("mean_Hz")
        if mean_df is not None:
            summary.append("live avg dF={:.4g} Hz".format(float(mean_df)))
        if summary:
            axes[0].text(
                0.01,
                0.01,
                "\n".join(summary),
                transform=axes[0].transAxes,
                va="bottom",
                ha="left",
                fontsize=8,
                color="white",
                bbox={"facecolor": "black", "alpha": 0.62, "pad": 4},
            )
        return fig, topo_extent

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
            dfreq_image = None
            dfreq_meta = None
            try:
                dfreq_image, dfreq_meta = self.runner.fetch_scan_image_to_line(
                    channel=2,
                    y_current=int(meta.get("y_current", -1)),
                    chunk_lines=int(chunk_lines),
                    store_last=False,
                )
            except Exception as exc:
                dfreq_meta = {
                    "available": False,
                    "error": "{}: {}".format(type(exc).__name__, exc),
                    "channel": 2,
                }
            pixel = self.runner._pixel_size_A(int(channel))
            quality = self.runner.assess_tip_quality(
                image,
                pixel_size_A=pixel,
                output_prefix=str(self.output_dir / output_prefix),
            )
            live_dfreq = self.runner.sample_dfrequency_values(count=12, delay_s=0.05)
            quality["dFrequency"] = live_dfreq.get("interpretation")
            scan_dfreq_stats = None
            if dfreq_image is not None:
                scan_dfreq_stats = self.runner._array_stats(dfreq_image)
            landscape = self.nav.analyze_current_scan_landscape_map(
                channel=int(channel),
                output_prefix=str(self.output_dir / (output_prefix + "_landscape")),
            )
            current_tip_position = None
            try:
                self.runner.get_live_tip_position_monitor(collect_as="scan_analysis_tip_position")
                monitor = self.runner.data.get("scan_analysis_tip_position", {})
                current_tip_position = {
                    "scan_x_A": float(monitor.get("dsp-GVP-XS-MONITOR")),
                    "scan_y_A": float(monitor.get("dsp-GVP-YS-MONITOR")),
                    "z_offset_A": float(monitor.get("dsp-GVP-ZS-MONITOR")),
                    "gvp_u_V": float(monitor.get("dsp-GVP-U-MONITOR")),
                    "raw_monitor": monitor,
                }
                landscape["current_tip_position"] = current_tip_position
            except Exception as exc:
                current_tip_position = {
                    "available": False,
                    "error": "{}: {}".format(type(exc).__name__, exc),
                }
                landscape["current_tip_position"] = current_tip_position
            if hasattr(self, "tip_planner_state"):
                self.tip_planner_state["last_analysis"] = landscape
                self.tip_planner_state["last_image"] = image
                self.tip_planner_state["last_meta"] = meta
                self.tip_planner_state["last_channel"] = int(channel)
                self.tip_planner_state["current_tip_position"] = current_tip_position
                self.tip_planner_state["selected_candidate_index"] = 1
            fig, extent = self.plot_scan_analysis_with_dfreq(
                image,
                meta,
                dfreq_image,
                dfreq_meta,
                quality=quality,
                landscape=landscape,
                live_dfreq=live_dfreq,
            )
            report = {
                "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                "fetch_meta": self.jsonable(meta),
                "scan_recorded_dFrequency_ch2": {
                    "meta": self.jsonable(dfreq_meta),
                    "stats": self.jsonable(scan_dfreq_stats),
                    "channel_info": self.runner.scan_channel_info(2),
                },
                "live_average_dFrequency": self.jsonable(live_dfreq),
                "current_tip_position": self.jsonable(current_tip_position),
                "extent_A": list(extent),
                "pixel_size_A": pixel,
                "tip_quality": quality,
                "metal_tip_note": (
                    "Current topo sharpness criteria are for a metal tip. "
                    "CO/O or other functionalized-tip contrast should use a "
                    "separate later classifier."
                ),
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
