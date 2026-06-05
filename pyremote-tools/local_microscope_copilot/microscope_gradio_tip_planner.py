"""Tip-improvement planner and GUI workflow helpers."""

import json
import time
import traceback

import matplotlib.pyplot as plt
import numpy as np


class TipPlannerGuiMixin:
    """GUI-facing orchestration for flat-site moves and iterative tip tuning."""

    TIP_LOOP_CONFIRMATION = "EXECUTE TIP LOOP"

    def init_tip_planner_state(self):
        self.tip_planner_state = {
            "last_analysis": None,
            "last_image": None,
            "last_meta": None,
            "last_channel": 0,
            "history": [],
            "last_move": None,
            "current_tip_position": None,
            "selected_candidate_index": 1,
        }

    def tip_planner_log_path(self):
        return self.output_dir / "tip_planner_progress.jsonl"

    def append_tip_planner_log(self, event):
        event = dict(event)
        event.setdefault("timestamp", time.strftime("%Y-%m-%dT%H:%M:%S"))
        self.tip_planner_state.setdefault("history", []).append(event)
        with self.tip_planner_log_path().open("a") as file:
            file.write(json.dumps(self.jsonable(event)) + "\n")
        return event

    def blank_tip_planner_figure(self, message):
        fig, ax = plt.subplots(figsize=(6, 4), constrained_layout=True)
        ax.text(0.5, 0.5, str(message), ha="center", va="center", wrap=True)
        ax.set_axis_off()
        return fig

    def scan_is_busy_report(self):
        if self.runner.dry_run:
            return {"busy": False, "dry_run": True}
        status_vec = self.runner.connect().rtquery("s")
        status_values = [int(value) for value in status_vec]
        sall = status_values[0] if len(status_values) > 0 else None
        sspm = status_values[1] if len(status_values) > 1 else None
        sgvp = status_values[2] if len(status_values) > 2 else None
        if sgvp is None:
            busy = True
            reason = "rtquery('s') did not return Sgvp; refusing local move."
        else:
            # Operator rule: Sgvp 0 means stopped/finished scan, Sgvp 5 means
            # completed GVP; both are ready. Sgvp 1 is busy.
            ready = int(sgvp) in (0, 5)
            busy = not ready
            if int(sgvp) == 1:
                reason = "Sgvp == 1, scan or GVP is active."
            elif ready:
                reason = "Sgvp {} is ready; local ScanX/ScanY move is allowed.".format(sgvp)
            else:
                reason = "Sgvp {} is unknown; refusing local move.".format(sgvp)
        return {
            "busy": bool(busy),
            "status_vector": status_values,
            "Sall": sall,
            "Sspm": sspm,
            "Sgvp": sgvp,
            "reason": reason,
            "broad_dsp_ready_mask": self.runner.DSP_BUSY_MASK,
            "note": (
                "ScanX/ScanY move busy check uses rtquery('s')[2] Sgvp. "
                "Sgvp 0 means stopped/finished scan, Sgvp 5 means completed "
                "GVP, and Sgvp 1 means busy. waitscan(False) may still return "
                "the stopped current line and is not used as a busy blocker."
            ),
        }

    def tip_quality_satisfied(self, quality, dfreq_report, max_abs_dfreq_Hz=4.0):
        verdict = str(quality.get("verdict", ""))
        image_ok = verdict == "no_strong_double_tip_signature"
        df = dfreq_report.get("dFrequency_Hz")
        try:
            df_ok = df is not None and abs(float(df)) <= float(max_abs_dfreq_Hz)
        except Exception:
            df_ok = False
        return {
            "satisfied": bool(image_ok and df_ok),
            "image_ok": image_ok,
            "dfreq_ok": df_ok,
            "verdict": verdict,
            "dFrequency_Hz": df,
            "max_abs_dfreq_Hz": float(max_abs_dfreq_Hz),
        }

    def normalize_planner_landscape(self, analysis):
        return {
            "flat_candidates": analysis.get("flat_candidates", []),
            "candidates": analysis.get("flat_candidates", []),
            "best_candidate": analysis.get("best_flat_candidate"),
            "hazards": analysis.get("hazards", []),
            "stepped_regions": analysis.get("stepped_regions", []),
            "abnormal_contrast_lines": analysis.get("abnormal_contrast_lines", []),
        }

    def blob_feature_summary(self, analysis, candidate=None):
        features = list(analysis.get("hazards", []) or [])
        molecule_candidates = [
            item for item in features
            if item.get("feature_class") == "molecule_candidate"
        ]
        large_hazards = [
            item for item in features
            if item.get("count_for_offset_stop")
        ]
        local_avoid = [
            item for item in features
            if item.get("avoid_for_clean_area")
        ]
        too_close = []
        if candidate:
            cx = candidate.get("px")
            cy = candidate.get("py")
            try:
                for item in local_avoid:
                    dist_px = float(np.hypot(float(item["px"]) - float(cx), float(item["py"]) - float(cy)))
                    diameter_px = float(item.get("diameter_px_est") or 1.0)
                    if dist_px <= max(24.0, 0.75 * diameter_px + 12.0):
                        near = dict(item)
                        near["distance_to_candidate_px"] = dist_px
                        too_close.append(near)
            except Exception:
                too_close = []
        return {
            "feature_count": len(features),
            "molecule_candidate_count": len(molecule_candidates),
            "large_hazard_count": len(large_hazards),
            "local_avoid_count": len(local_avoid),
            "large_hazards": large_hazards,
            "molecule_candidates": molecule_candidates[:30],
            "local_avoid_too_close_to_candidate": too_close,
            "too_close_to_clear_area": bool(too_close),
            "rule": (
                "Small blobs with abs(dZ)<3 A are molecule candidates and do "
                "not trigger offset search. Large hazards require taller "
                "features with estimated diameter >=40 A, or avoid-worthy "
                "features too close to the chosen clean area."
            ),
        }

    def plot_tip_planner_analysis(self, image, meta, channel, analysis, title=None):
        landscape = self.normalize_planner_landscape(analysis)
        landscape["current_tip_position"] = self.tip_planner_state.get("current_tip_position")
        landscape["selected_candidate_index"] = self.tip_planner_state.get("selected_candidate_index")
        quality = dict(analysis.get("tip_quality") or {})
        fig, _extent = self.plot_scan_image_A(
            image,
            meta,
            int(channel),
            title or "Tip planner analysis",
            "topography / signal",
            quality=quality,
            landscape=landscape,
        )
        return fig

    def _tip_planner_last_plot(self, title="Tip planner analysis, line 0 at top"):
        image = self.tip_planner_state.get("last_image")
        meta = self.tip_planner_state.get("last_meta") or {}
        analysis = self.tip_planner_state.get("last_analysis") or {}
        channel = self.tip_planner_state.get("last_channel", 0)
        if image is None or not analysis:
            return self.blank_tip_planner_figure("Run a planner analysis first.")
        return self.plot_tip_planner_analysis(image, meta, channel, analysis, title=title)

    def _read_tip_planner_tip_position(self):
        record = self.runner.get_live_tip_position_monitor()
        monitor = (record.result_summary or {}).get("monitor") or self.runner.data.get("live_tip_position", {})
        if not monitor and isinstance(record.result_summary, dict):
            monitor = record.result_summary.get("result") or {}
        try:
            scan_x = float(monitor.get("dsp-GVP-XS-MONITOR"))
        except Exception:
            scan_x = None
        try:
            scan_y = float(monitor.get("dsp-GVP-YS-MONITOR"))
        except Exception:
            scan_y = None
        try:
            z_offset = float(monitor.get("dsp-GVP-ZS-MONITOR"))
        except Exception:
            z_offset = None
        try:
            bias = float(monitor.get("dsp-GVP-U-MONITOR"))
        except Exception:
            bias = None
        tip = {
            "scan_x_A": scan_x,
            "scan_y_A": scan_y,
            "z_offset_A": z_offset,
            "gvp_u_V": bias,
            "raw_monitor": monitor,
            "record": record.__dict__,
            "note": (
                "X/Y are live local ScanX/ScanY tip coordinates in Angstrom. "
                "They can change during raster scanning."
            ),
        }
        self.tip_planner_state["current_tip_position"] = tip
        return tip

    def tip_planner_read_tip_position(self):
        try:
            tip = self._read_tip_planner_tip_position()
            report = {
                "event": "read_current_tip_position",
                "current_tip_position": tip,
                "selected_candidate_index": self.tip_planner_state.get("selected_candidate_index"),
            }
            return self._tip_planner_last_plot(), self.jsonable(report)
        except Exception as exc:
            return (
                self.blank_tip_planner_figure(exc),
                {"error": str(exc), "traceback": traceback.format_exc()},
            )

    def tip_planner_auto_read_tip_position(self, enabled):
        try:
            import gradio as gr
        except Exception:
            gr = None
        if not enabled:
            update = gr.update() if gr else None
            return update, update
        return self.tip_planner_read_tip_position()

    def _candidate_local_xy(self, candidate):
        scan_x = candidate.get("scan_x_A")
        scan_y = candidate.get("scan_y_A")
        if scan_x is None:
            scan_x = candidate.get("local_scan_x_A")
        if scan_y is None:
            scan_y = candidate.get("local_scan_y_A")
        return float(scan_x), float(scan_y)

    def _extract_plot_click_xy_A(self, evt):
        values = []
        for name in ("index", "value", "selected"):
            if hasattr(evt, name):
                values.append(getattr(evt, name))
        if isinstance(evt, dict):
            values.extend([evt.get("index"), evt.get("value"), evt.get("selected")])
        for value in values:
            if value is None:
                continue
            if isinstance(value, dict):
                for x_key, y_key in (("x", "y"), ("xdata", "ydata")):
                    if x_key in value and y_key in value:
                        x_val = value[x_key]
                        y_val = value[y_key]
                        if isinstance(x_val, (list, tuple, np.ndarray)):
                            x_val = x_val[0]
                        if isinstance(y_val, (list, tuple, np.ndarray)):
                            y_val = y_val[0]
                        return float(x_val), float(y_val)
                points = value.get("points")
                if points:
                    point = points[0]
                    if isinstance(point, dict) and "x" in point and "y" in point:
                        return float(point["x"]), float(point["y"])
            if isinstance(value, (list, tuple, np.ndarray)) and len(value) >= 2:
                return float(value[0]), float(value[1])
        return None, None

    def tip_planner_select_candidate_from_plot(self, evt):
        try:
            analysis = self.tip_planner_state.get("last_analysis")
            if not analysis:
                raise ValueError("Run a planner analysis first.")
            candidates = analysis.get("flat_candidates", [])
            if not candidates:
                raise ValueError("No flat candidates are available in the last analysis.")
            click_x, click_y = self._extract_plot_click_xy_A(evt)
            if click_x is None or click_y is None:
                raise ValueError(
                    "Could not read plot click coordinates from this Gradio event."
                )
            distances = []
            for idx, candidate in enumerate(candidates, start=1):
                cand_x, cand_y = self._candidate_local_xy(candidate)
                distances.append((float(np.hypot(cand_x - click_x, cand_y - click_y)), idx, candidate))
            distance_A, idx, candidate = min(distances, key=lambda item: item[0])
            self.tip_planner_state["selected_candidate_index"] = idx
            report = {
                "event": "selected_candidate_from_plot_click",
                "clicked_scan_x_A": click_x,
                "clicked_scan_y_A": click_y,
                "selected_candidate_index": idx,
                "distance_to_click_A": distance_A,
                "candidate": candidate,
                "instruction": "Press Move Tip To Selected Candidate to move after arming Level 1.",
            }
            return idx, self._tip_planner_last_plot(), self.jsonable(report)
        except Exception as exc:
            return (
                self.tip_planner_state.get("selected_candidate_index", 1),
                self._tip_planner_last_plot(),
                {"error": str(exc), "traceback": traceback.format_exc()},
            )

    def tip_planner_move_to_candidate_update_plot(self, candidate_index, arm):
        result = self.tip_planner_move_to_candidate(candidate_index, arm)
        try:
            self._read_tip_planner_tip_position()
        except Exception:
            pass
        return self._tip_planner_last_plot(), result

    def tip_planner_analyze_current_scan(
        self,
        channel,
        lines_to_fetch,
        patch_A,
        max_candidates,
        max_blobs,
        output_prefix,
    ):
        try:
            prefix = str(self.output_dir / str(output_prefix))
            image, meta = self.runner.fetch_scan_image_to_line(
                channel=int(channel),
                chunk_lines=int(lines_to_fetch),
            )
            analysis = self.nav.analyze_current_scan_landscape_map(
                channel=int(channel),
                patch_A=float(patch_A),
                max_candidates=int(max_candidates),
                max_hazards=max(30, int(max_blobs) * 3),
                output_prefix=prefix,
            )
            df = self.runner.dFreq()
            dfreq_report = self.runner.interpret_dFreq(df)
            analysis["dFrequency"] = dfreq_report
            analysis["satisfaction"] = self.tip_quality_satisfied(
                analysis.get("tip_quality", {}),
                dfreq_report,
            )
            blob_summary = self.blob_feature_summary(
                analysis,
                candidate=analysis.get("best_flat_candidate"),
            )
            analysis["blob_feature_summary"] = blob_summary
            analysis["blob_stop"] = {
                "max_blobs": int(max_blobs),
                "feature_count": blob_summary["feature_count"],
                "molecule_candidate_count": blob_summary["molecule_candidate_count"],
                "large_hazard_count": blob_summary["large_hazard_count"],
                "too_close_to_clear_area": blob_summary["too_close_to_clear_area"],
                "too_many_blobs": blob_summary["large_hazard_count"] >= int(max_blobs),
            }
            self.tip_planner_state["last_image"] = image
            self.tip_planner_state["last_meta"] = meta
            self.tip_planner_state["last_channel"] = int(channel)
            self.tip_planner_state["last_analysis"] = analysis
            self.tip_planner_state["selected_candidate_index"] = 1
            try:
                analysis["current_tip_position"] = self._read_tip_planner_tip_position()
            except Exception as exc:
                analysis["current_tip_position"] = {
                    "available": False,
                    "error": "{}: {}".format(type(exc).__name__, exc),
                }
            event = self.append_tip_planner_log(
                {
                    "event": "analysis",
                    "status": "ok",
                    "verdict": analysis.get("tip_quality", {}).get("verdict"),
                    "dFrequency": dfreq_report,
                    "hazard_count": blob_summary["large_hazard_count"],
                    "molecule_candidate_count": blob_summary["molecule_candidate_count"],
                    "candidate_count": len(analysis.get("flat_candidates", [])),
                    "best_candidate": analysis.get("best_flat_candidate"),
                    "report_plot": analysis.get("plot"),
                }
            )
            fig = self.plot_tip_planner_analysis(
                image,
                meta,
                channel,
                analysis,
                title="Tip planner analysis, line 0 at top",
            )
            report = {
                "analysis": analysis,
                "event": event,
                "log_path": str(self.tip_planner_log_path()),
            }
            return fig, self.tip_planner_progress_figure(), self.jsonable(report)
        except Exception as exc:
            report = {"error": str(exc), "traceback": traceback.format_exc()}
            return (
                self.blank_tip_planner_figure(exc),
                self.tip_planner_progress_figure(),
                report,
            )

    def tip_planner_move_to_candidate(self, candidate_index, arm):
        blocked = self.require_control_level(1, "move tip to flat candidate")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "move the tip to a flat candidate")
        if cancelled:
            return cancelled
        try:
            busy = self.scan_is_busy_report()
            if busy.get("busy"):
                return {
                    "blocked": "move tip to flat candidate",
                    "reason": (
                        "Scan appears busy. GXSM prohibits ScanX/ScanY local "
                        "tip moves while scanning; stop the scan first."
                    ),
                    "scan_status": busy,
                }
            analysis = self.tip_planner_state.get("last_analysis")
            if not analysis:
                raise ValueError("Run a tip-tune-planner analysis first.")
            candidates = analysis.get("flat_candidates", [])
            if not candidates:
                raise ValueError("No flat candidates are available in the last analysis.")
            idx = max(0, min(int(candidate_index) - 1, len(candidates) - 1))
            candidate = candidates[idx]
            rec = self.runner.move_to_scan_xy_fields_verified(
                candidate["scan_x_A"],
                candidate["scan_y_A"],
            )
            try:
                self._read_tip_planner_tip_position()
            except Exception:
                pass
            result = {
                "candidate_index": idx + 1,
                "candidate": candidate,
                "move_record": rec.__dict__,
                "scan_status_before_move": busy,
            }
            self.tip_planner_state["last_move"] = result
            self.append_tip_planner_log(
                {
                    "event": "move_tip_to_candidate",
                    "status": "ok",
                    "candidate_index": idx + 1,
                    "candidate": candidate,
                    "verified": rec.result_summary.get("verified")
                    if isinstance(rec.result_summary, dict) else None,
                }
            )
            return self.jsonable(result)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def tip_planner_plan_offset_shift(self, direction, new_range_A, overlap_A):
        try:
            plan = self.nav.propose_overlapping_exploration_frame(
                new_range_A=float(new_range_A),
                overlap_A=float(overlap_A),
                direction=str(direction),
            )
            self.append_tip_planner_log(
                {
                    "event": "plan_offset_shift",
                    "status": "ok",
                    "plan": plan,
                }
            )
            return self.jsonable(plan)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def tip_planner_apply_offset_shift(
        self,
        direction,
        new_range_A,
        overlap_A,
        points,
        start_after_shift,
        arm,
    ):
        blocked = self.require_control_level(1, "shift scan offset for clean-area search")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "shift scan offset")
        if cancelled:
            return cancelled
        try:
            busy = self.scan_is_busy_report()
            if busy.get("busy"):
                self.runner.connect().stopscan()
                time.sleep(3.0)
            plan = self.nav.propose_overlapping_exploration_frame(
                new_range_A=float(new_range_A),
                overlap_A=float(overlap_A),
                direction=str(direction),
            )
            if not plan.get("reachable"):
                return {"blocked": "offset shift", "reason": "Target is outside reachable range.", "plan": plan}
            rec = self.nav.setup_exploration_scan_frame(
                plan["OffsetX"],
                plan["OffsetY"],
                range_A=float(new_range_A),
                points=int(points),
                rotation_deg=plan.get("Rotation"),
            )
            result = {
                "plan": plan,
                "setup_record": rec.__dict__,
                "stopped_scan_before_shift": bool(busy.get("busy")),
            }
            if start_after_shift:
                result["startscan_return"] = self.runner.connect().startscan() if not self.runner.dry_run else "dry-run"
            self.append_tip_planner_log(
                {
                    "event": "apply_offset_shift",
                    "status": "ok",
                    "result": result,
                }
            )
            return self.jsonable(result)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def choose_tip_action_plan(self, cycle_index, quality):
        peaks = [
            p for p in quality.get("autocorrelation_secondary_peaks", [])
            if p.get("used_for_tip_verdict")
        ]
        strongest = max([float(p.get("corr", 0.0)) for p in peaks], default=0.0)
        if cycle_index <= 1:
            pulse = 2.0 if strongest < 0.22 else 3.0
            return {"kind": "bias_pulse", "pulse_du_V": pulse, "reason": "gentle first pulse"}
        if cycle_index == 2:
            return {"kind": "bias_pulse", "pulse_du_V": 3.0, "reason": "second pulse escalation"}
        return {
            "kind": "tip_dip",
            "dip_depth_A": min(5.0 + 2.0 * max(0, cycle_index - 3), 9.0),
            "contact_bias_V": 0.0,
            "reason": "very gentle touch after pulse attempts",
        }

    def execute_tip_action_plan(self, plan, output_prefix):
        if plan["kind"] == "bias_pulse":
            load = self.runner.load_gvp_bias_pulse(
                float(plan["pulse_du_V"]),
                save_prefix=output_prefix,
            )
        elif plan["kind"] == "tip_dip":
            scan_bias = self.read_float_parameter("dsp-fbs-bias", fallback=0.1)
            load = self.runner.load_gvp_tip_dip(
                dip_depth_A=float(plan["dip_depth_A"]),
                contact_bias_V=float(plan.get("contact_bias_V", 0.0)),
                scan_bias_V=scan_bias,
                save_prefix=output_prefix,
            )
            plan["scan_bias_V_used_for_du"] = scan_bias
        else:
            raise ValueError("Unknown tip action kind: {}".format(plan["kind"]))
        before_df = self.runner.dFreq()
        rec = self.runner.execute_vp_probe(
            action_name="DSP_VP_VP_EXECUTE",
            wait_after_execute_s=8.0,
            ready_timeout_s=120.0,
            precheck=True,
            start_idx=0,
            end_idx=200000,
            collect_as="last_tip_planner_gvp_vpdata",
        )
        after_df = self.runner.dFreq()
        data = self.runner.data.get("last_tip_planner_gvp_vpdata")
        analysis = self.analyze_gvp_vpdata(data)
        analysis["rt_dFreq_before_Hz"] = before_df
        analysis["rt_dFreq_after_Hz"] = after_df
        analysis["rt_dFreq_delta_Hz"] = (
            None if before_df is None or after_df is None else float(after_df) - float(before_df)
        )
        return {
            "plan": plan,
            "load_record": load.__dict__,
            "execute_record": rec.__dict__,
            "gvp_analysis": analysis,
        }

    def run_tip_planner_loop(
        self,
        max_cycles,
        channel,
        min_lines,
        range_A,
        points,
        max_blobs,
        max_abs_dfreq_Hz,
        auto_shift_on_blobs,
        shift_direction,
        confirm_text,
        arm,
    ):
        blocked = self.require_control_level(1, "run tip-improvement planner loop")
        if blocked:
            return self.blank_tip_planner_figure("blocked"), self.tip_planner_progress_figure(), blocked
        cancelled = self.require_arm(arm, "run the tip-improvement planner loop")
        if cancelled:
            return self.blank_tip_planner_figure("cancelled"), self.tip_planner_progress_figure(), cancelled
        if str(confirm_text).strip() != self.TIP_LOOP_CONFIRMATION:
            report = {"cancelled": "Type '{}' to run the planner loop.".format(self.TIP_LOOP_CONFIRMATION)}
            return self.blank_tip_planner_figure(report["cancelled"]), self.tip_planner_progress_figure(), report

        started = time.time()
        loop_report = {
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "activity": "tip_improvement_planner_loop",
            "parameters": {
                "max_cycles": int(max_cycles),
                "channel": int(channel),
                "min_lines": int(min_lines),
                "range_A": float(range_A),
                "points": int(points),
                "max_blobs": int(max_blobs),
                "max_abs_dfreq_Hz": float(max_abs_dfreq_Hz),
                "auto_shift_on_blobs": bool(auto_shift_on_blobs),
                "shift_direction": str(shift_direction),
            },
            "cycles": [],
        }
        last_fig = self.blank_tip_planner_figure("Planner did not collect an image yet.")
        try:
            for cycle in range(1, int(max_cycles) + 1):
                prefix = "tip_planner_cycle_{:02d}".format(cycle)
                self.runner.setup_scan_geometry(range_A=float(range_A), points=int(points))
                assessment = self.runner.scan_watch_assess_tip(
                    channel=int(channel),
                    min_lines=int(min_lines),
                    start_scan=True,
                    setup_scan=False,
                    range_A=float(range_A),
                    points=int(points),
                    initial_delay_s=4.0,
                    poll_s=4.0,
                    timeout_s=max(90.0, float(min_lines) * 2.0),
                    output_prefix=str(self.output_dir / prefix),
                )
                workflow = self.runner.data.get("last_tip_workflow", {})
                image_path = self.output_dir / "{}_scan_image.npy".format(prefix)
                image = np.load(image_path) if image_path.exists() else None
                landscape = self.nav.analyze_current_scan_landscape_map(
                    channel=int(channel),
                    patch_A=80.0,
                    max_candidates=12,
                    max_hazards=max(30, int(max_blobs) * 3),
                    output_prefix=str(self.output_dir / "{}_landscape".format(prefix)),
                )
                df = self.runner.dFreq()
                dfreq_report = self.runner.interpret_dFreq(df)
                quality = workflow.get("quality") or landscape.get("tip_quality", {})
                quality["dFrequency"] = dfreq_report
                satisfaction = self.tip_quality_satisfied(
                    quality,
                    dfreq_report,
                    max_abs_dfreq_Hz=max_abs_dfreq_Hz,
                )
                blob_summary = self.blob_feature_summary(
                    landscape,
                    candidate=landscape.get("best_flat_candidate"),
                )
                hazard_count = blob_summary["large_hazard_count"]
                cycle_report = {
                    "cycle": cycle,
                    "assessment_record": assessment.__dict__,
                    "quality": quality,
                    "dFrequency": dfreq_report,
                    "satisfaction": satisfaction,
                    "hazard_count": hazard_count,
                    "blob_feature_summary": blob_summary,
                    "flat_candidates": landscape.get("flat_candidates", []),
                    "best_flat_candidate": landscape.get("best_flat_candidate"),
                    "landscape_plot": landscape.get("plot"),
                }
                loop_report["cycles"].append(cycle_report)
                self.tip_planner_state["last_analysis"] = landscape
                if image is not None:
                    last_fig = self.plot_tip_planner_analysis(
                        image,
                        workflow.get("fetch_meta", landscape.get("fetch_meta", {})),
                        channel,
                        {**landscape, "tip_quality": quality, "dFrequency": dfreq_report},
                        title="Tip planner cycle {} analysis".format(cycle),
                    )

                if satisfaction["satisfied"]:
                    cycle_report["decision"] = "stop_satisfied"
                    loop_report["status"] = "satisfied"
                    break

                if hazard_count >= int(max_blobs) or blob_summary["too_close_to_clear_area"]:
                    cycle_report["decision"] = (
                        "too_many_large_blobs"
                        if hazard_count >= int(max_blobs)
                        else "large_blob_too_close_to_clear_area"
                    )
                    self.runner.connect().stopscan() if not self.runner.dry_run else None
                    if auto_shift_on_blobs:
                        shift = self.tip_planner_apply_offset_shift(
                            shift_direction,
                            range_A,
                            max(0.25 * float(range_A), 200.0),
                            points,
                            True,
                            True,
                        )
                        cycle_report["offset_shift"] = shift
                        continue
                    loop_report["status"] = "stopped_for_large_blob_hazard"
                    break

                candidate = landscape.get("best_flat_candidate")
                if not candidate:
                    cycle_report["decision"] = "no_clean_candidate"
                    loop_report["status"] = "needs_offset_search"
                    break

                self.runner.stop_scan_wait_for_gvp(stop_wait_s=3.0, ready_timeout_s=20.0)
                move = self.runner.move_to_scan_xy_fields_verified(
                    candidate["scan_x_A"],
                    candidate["scan_y_A"],
                )
                plan = self.choose_tip_action_plan(cycle, quality)
                action = self.execute_tip_action_plan(
                    plan,
                    str(self.output_dir / "{}_{}".format(prefix, plan["kind"])),
                )
                cycle_report["decision"] = "executed_tip_action"
                cycle_report["move_to_candidate"] = move.__dict__
                cycle_report["tip_action"] = action
                self.append_tip_planner_log(
                    {
                        "event": "planner_cycle",
                        "cycle": cycle,
                        "decision": cycle_report["decision"],
                        "verdict": quality.get("verdict"),
                        "dFrequency": dfreq_report,
                        "hazard_count": hazard_count,
                        "molecule_candidate_count": blob_summary["molecule_candidate_count"],
                        "tip_action_plan": plan,
                    }
                )
            else:
                loop_report["status"] = "max_cycles_reached"
            loop_report.setdefault("status", "completed")
            loop_report["elapsed_s"] = time.time() - started
            report_path = self.output_dir / "tip_planner_loop_report.json"
            report_path.write_text(json.dumps(self.jsonable(loop_report), indent=2))
            loop_report["report_path"] = str(report_path)
            return last_fig, self.tip_planner_progress_figure(), self.jsonable(loop_report)
        except Exception as exc:
            loop_report["status"] = "error"
            loop_report["error"] = str(exc)
            loop_report["traceback"] = traceback.format_exc()
            return (
                last_fig,
                self.tip_planner_progress_figure(),
                self.jsonable(loop_report),
            )

    def tip_planner_progress_figure(self):
        history = self.tip_planner_state.get("history", [])[-30:]
        fig, ax = plt.subplots(figsize=(7, 3.5), constrained_layout=True)
        if not history:
            ax.text(0.5, 0.5, "No tip-tune-planner events yet", ha="center", va="center")
            ax.set_axis_off()
            return fig
        y = np.arange(len(history))
        labels = [str(item.get("event", "?")) for item in history]
        hazard_counts = [float(item.get("hazard_count", 0) or 0) for item in history]
        molecule_counts = [float(item.get("molecule_candidate_count", 0) or 0) for item in history]
        ax.barh(y, molecule_counts, color="#78a6c8", alpha=0.55, label="molecule candidates")
        ax.barh(y, hazard_counts, color="#d66b3d", alpha=0.75, label="large hazards")
        ax.set_yticks(y)
        ax.set_yticklabels(labels, fontsize=8)
        ax.invert_yaxis()
        ax.set_xlabel("count")
        ax.set_title("Tip Tune Planner progress log")
        ax.grid(True, axis="x", alpha=0.25)
        ax.legend(loc="lower right", fontsize=8)
        return fig
