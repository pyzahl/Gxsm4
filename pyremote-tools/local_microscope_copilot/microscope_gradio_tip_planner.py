"""Tip-improvement planner and GUI workflow helpers."""

import html
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
            "current_action": None,
            "pending_action": None,
            "activity_history": [],
            "activity_counter": 0,
            "stop_requested": False,
        }

    def tip_planner_log_path(self):
        return self.output_dir / "tip_planner_progress.jsonl"

    def tip_planner_activity_log_path(self):
        return self.output_dir / "tip_planner_activity.jsonl"

    def tip_planner_activity_state_path(self):
        return self.output_dir / "tip_planner_activity_state.json"

    def persist_tip_planner_activity_state(self):
        snap = self.tip_planner_activity_snapshot()
        self.tip_planner_activity_state_path().write_text(
            json.dumps(self.jsonable(snap), indent=2)
        )
        return snap

    def append_tip_planner_log(self, event):
        event = dict(event)
        event.setdefault("timestamp", time.strftime("%Y-%m-%dT%H:%M:%S"))
        self.tip_planner_state.setdefault("history", []).append(event)
        with self.tip_planner_log_path().open("a") as file:
            file.write(json.dumps(self.jsonable(event)) + "\n")
        return event

    def start_tip_planner_activity(self, action, details=None, pending_next=None):
        state = self.tip_planner_state
        state["activity_counter"] = int(state.get("activity_counter", 0)) + 1
        rec = {
            "id": state["activity_counter"],
            "action": str(action),
            "status": "in_progress",
            "started_at": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "completed_at": None,
            "elapsed_s": None,
            "details": self.jsonable(details or {}),
            "result": None,
        }
        state["current_action"] = rec
        state["pending_action"] = pending_next
        state.setdefault("activity_history", []).append(rec)
        with self.tip_planner_activity_log_path().open("a") as file:
            file.write(json.dumps(self.jsonable({"event": "start", "activity": rec})) + "\n")
        self.persist_tip_planner_activity_state()
        return rec

    def finish_tip_planner_activity(self, activity, status="completed", result=None, pending_next=None):
        state = self.tip_planner_state
        if not activity:
            state["current_action"] = None
            state["pending_action"] = pending_next
            return None
        completed = time.strftime("%Y-%m-%dT%H:%M:%S")
        activity["status"] = str(status)
        activity["completed_at"] = completed
        try:
            started = time.strptime(activity["started_at"], "%Y-%m-%dT%H:%M:%S")
            done = time.strptime(completed, "%Y-%m-%dT%H:%M:%S")
            activity["elapsed_s"] = max(0.0, time.mktime(done) - time.mktime(started))
        except Exception:
            activity["elapsed_s"] = None
        activity["result"] = self.jsonable(result or {})
        current = state.get("current_action")
        if current and current.get("id") == activity.get("id"):
            state["current_action"] = None
        state["pending_action"] = pending_next
        with self.tip_planner_activity_log_path().open("a") as file:
            file.write(json.dumps(self.jsonable({"event": "finish", "activity": activity})) + "\n")
        self.persist_tip_planner_activity_state()
        return activity

    def fail_tip_planner_activity(self, activity, exc, pending_next=None):
        return self.finish_tip_planner_activity(
            activity,
            status="error",
            result={
                "error": str(exc),
                "traceback": traceback.format_exc(),
            },
            pending_next=pending_next,
        )

    def tip_planner_activity_snapshot(self):
        current = self.tip_planner_state.get("current_action")
        history = self.tip_planner_state.get("activity_history", [])
        pending = self.tip_planner_state.get("pending_action")
        return {
            "status": "in_progress" if current else "idle",
            "current_action": current,
            "pending_action": pending,
            "stop_requested": bool(self.tip_planner_state.get("stop_requested")),
            "recent_actions": history[-25:],
            "activity_log_path": str(self.tip_planner_activity_log_path()),
            "activity_state_path": str(self.tip_planner_activity_state_path()),
            "event_log_path": str(self.tip_planner_log_path()),
        }

    def tip_planner_activity_html(self):
        snap = self.tip_planner_activity_snapshot()
        status = snap["status"]
        current = snap.get("current_action")
        pending = snap.get("pending_action")
        stop_note = (
            "<div style='margin-bottom:8px;color:#b00020;font-weight:700'>"
            "Stop requested; planner will exit at the next safe checkpoint."
            "</div>"
            if snap.get("stop_requested") else ""
        )

        def esc(value):
            return html.escape("" if value is None else str(value))

        if current:
            now_s = time.strftime("%Y-%m-%d %H:%M:%S")
            current_line = (
                "<b>Current:</b> #{id} {action} "
                "<span style='color:#555'>(started {started_at}; now {now})</span>"
            ).format(
                id=esc(current.get("id")),
                action=esc(current.get("action")),
                started_at=esc(current.get("started_at")),
                now=esc(now_s),
            )
        else:
            current_line = "<b>Current:</b> idle"
        pending_line = "<b>Next pending:</b> {}".format(esc(pending or "none"))

        def result_summary(item):
            result = item.get("result") or {}
            if not isinstance(result, dict):
                return result
            parts = []
            for key in (
                "reason",
                "status",
                "recommended_next_action",
                "suggested_direction",
                "candidate_count",
                "hazard_count",
                "large_hazard_count",
                "verdict",
                "verified",
                "error",
            ):
                if key in result and result.get(key) not in (None, ""):
                    parts.append("{}={}".format(key, result.get(key)))
            if not parts and result:
                keys = list(result.keys())[:4]
                parts = ["{}={}".format(key, result.get(key)) for key in keys]
            return "; ".join(parts)

        rows = []
        for item in reversed(snap.get("recent_actions", [])[-12:]):
            status_color = {
                "completed": "#2f7d32",
                "error": "#b00020",
                "blocked": "#a15c00",
                "cancelled": "#666666",
                "in_progress": "#1f5fbf",
            }.get(str(item.get("status")), "#333333")
            elapsed = item.get("elapsed_s")
            elapsed_txt = "" if elapsed is None else "{:.1f}s".format(float(elapsed))
            rows.append(
                "<tr>"
                "<td>{id}</td><td>{action}</td>"
                "<td style='color:{color};font-weight:600'>{status}</td>"
                "<td>{started}</td><td>{completed}</td><td>{elapsed}</td><td>{reason}</td>"
                "</tr>".format(
                    id=esc(item.get("id")),
                    action=esc(item.get("action")),
                    color=status_color,
                    status=esc(item.get("status")),
                    started=esc(item.get("started_at")),
                    completed=esc(item.get("completed_at") or ""),
                    elapsed=esc(elapsed_txt),
                    reason=esc(result_summary(item)),
                )
            )
        if not rows:
            rows.append("<tr><td colspan='7'>No planner actions recorded yet.</td></tr>")

        return """
<div style="border:1px solid #d0d0d0;border-radius:6px;padding:10px;background:#fafafa">
  <div style="font-weight:700;margin-bottom:6px">Tip Tune Planner Activity</div>
  <div style="margin-bottom:4px">{current}</div>
  <div style="margin-bottom:8px">{pending}</div>
  {stop_note}
  <table style="width:100%;border-collapse:collapse;font-size:12px">
    <thead>
      <tr style="border-bottom:1px solid #ddd;text-align:left">
        <th>#</th><th>Action</th><th>Status</th><th>Started</th><th>Completed</th><th>Elapsed</th><th>Reason / result</th>
      </tr>
    </thead>
    <tbody>{rows}</tbody>
  </table>
  <div style="margin-top:8px;font-size:11px;color:#666">
    Status: {status}; logs: <code>{activity_log}</code>, <code>{activity_state}</code>, <code>{event_log}</code>
  </div>
</div>
""".format(
            current=current_line,
            pending=pending_line,
            stop_note=stop_note,
            rows="\n".join(rows),
            status=esc(status),
            activity_log=esc(snap.get("activity_log_path")),
            activity_state=esc(snap.get("activity_state_path")),
            event_log=esc(snap.get("event_log_path")),
        )

    def request_stop_tip_planner_loop(self):
        self.tip_planner_state["stop_requested"] = True
        activity = self.start_tip_planner_activity(
            "operator requested planner stop",
            details={"request": "stop tip tune planner loop"},
            pending_next="planner loop will stop at next safe checkpoint",
        )
        stopscan_result = None
        try:
            if not self.runner.dry_run:
                stopscan_result = self.runner.connect().stopscan()
        except Exception as exc:
            stopscan_result = "{}: {}".format(type(exc).__name__, exc)
        self.finish_tip_planner_activity(
            activity,
            status="completed",
            result={
                "reason": "operator requested stop",
                "stopscan_return": stopscan_result,
                "note": "GVP emergency stop remains available in the GVP tab if a GVP is running.",
            },
            pending_next="idle after current safe checkpoint",
        )
        report = {
            "status": "stop_requested",
            "reason": "operator requested stop",
            "stopscan_return": stopscan_result,
        }
        return self.jsonable(report), self.tip_planner_activity_html()

    def clear_tip_planner_history(self, reason="operator hyper jump / new world"):
        activity = self.start_tip_planner_activity(
            "clear tip planner and landscape history",
            details={"reason": str(reason)},
            pending_next="start fresh scan in new world",
        )
        try:
            reset = self.nav.reset_landscape_memory(reason=str(reason))
            self.tip_planner_state.update(
                {
                    "last_analysis": None,
                    "last_image": None,
                    "last_meta": None,
                    "last_channel": 0,
                    "history": [],
                    "last_move": None,
                    "current_tip_position": None,
                    "selected_candidate_index": 1,
                    "pending_action": "start fresh scan in new world",
                    "stop_requested": False,
                }
            )
            self.finish_tip_planner_activity(
                activity,
                status="completed",
                result={
                    "reason": str(reason),
                    "landscape_reset": reset,
                },
                pending_next="start fresh scan in new world",
            )
            return self.jsonable({"status": "cleared", "landscape_reset": reset}), self.tip_planner_activity_html()
        except Exception as exc:
            self.fail_tip_planner_activity(activity, exc, pending_next="inspect clear-history error")
            return {"error": str(exc), "traceback": traceback.format_exc()}, self.tip_planner_activity_html()

    def check_tip_planner_stop_requested(self, loop_activity=None, loop_report=None, cycle=None):
        if not self.tip_planner_state.get("stop_requested"):
            return False
        if loop_report is not None:
            loop_report["status"] = "cancelled_by_operator"
            loop_report["stop_requested_at_cycle"] = cycle
        if loop_activity and loop_activity.get("status") == "in_progress":
            self.finish_tip_planner_activity(
                loop_activity,
                status="cancelled",
                result={
                    "status": "cancelled_by_operator",
                    "reason": "operator pressed Stop Tip Tune Planner Loop",
                    "cycle": cycle,
                },
                pending_next="idle",
            )
        self.tip_planner_state["pending_action"] = "idle"
        return True

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

    def suggest_escape_direction_from_hazards(self, analysis, fallback_direction="image_left_below"):
        hazards = [
            item for item in (analysis.get("hazards", []) or [])
            if item.get("count_for_offset_stop") or item.get("avoid_for_clean_area")
        ]
        if not hazards:
            return {
                "suggested_direction": str(fallback_direction),
                "reason": "No avoid-worthy hazards were available; using configured fallback direction.",
                "hazard_count_used": 0,
            }
        settings = analysis.get("settings") or {}
        try:
            center_x = float(settings.get("PointsX")) / 2.0
            center_y = float(settings.get("PointsY")) / 2.0
        except Exception:
            image = self.tip_planner_state.get("last_image")
            if image is not None:
                ny, nx = image.shape
                center_x = float(nx) / 2.0
                center_y = float(ny) / 2.0
            else:
                center_x = 0.0
                center_y = 0.0

        weighted = []
        for item in hazards:
            if item.get("px") is None or item.get("py") is None:
                continue
            weight = 1.0
            if item.get("count_for_offset_stop"):
                weight += 2.0
            try:
                weight += min(5.0, abs(float(item.get("dz_peak_A", 0.0))) / 3.0)
            except Exception:
                pass
            weighted.append((float(item["px"]), float(item["py"]), weight, item))
        if not weighted:
            return {
                "suggested_direction": str(fallback_direction),
                "reason": "Hazards had no pixel centers; using configured fallback direction.",
                "hazard_count_used": 0,
            }

        total_w = sum(item[2] for item in weighted) or 1.0
        hx = sum(px * w for px, _py, w, _item in weighted) / total_w
        hy = sum(py * w for _px, py, w, _item in weighted) / total_w
        dx = hx - center_x
        dy = hy - center_y
        tol_x = max(20.0, 0.08 * max(center_x, 1.0))
        tol_y = max(20.0, 0.08 * max(center_y, 1.0))

        horizontal = ""
        vertical = ""
        if dx > tol_x:
            horizontal = "left"
        elif dx < -tol_x:
            horizontal = "right"
        if dy > tol_y:
            vertical = "up"
        elif dy < -tol_y:
            vertical = "down"

        if horizontal and vertical:
            direction = "image_{}_above".format(horizontal) if vertical == "up" else "image_{}_below".format(horizontal)
        elif horizontal:
            direction = "image_{}".format(horizontal)
        elif vertical:
            direction = "image_{}".format(vertical)
        else:
            direction = str(fallback_direction)

        strongest = max(weighted, key=lambda item: item[2])[3]
        return {
            "suggested_direction": direction,
            "fallback_direction": str(fallback_direction),
            "reason": (
                "Move the next scan frame away from the weighted hazard center "
                "at image px=({:.1f}, {:.1f}) relative to center px=({:.1f}, {:.1f})."
            ).format(hx, hy, center_x, center_y),
            "hazard_count_used": len(weighted),
            "weighted_hazard_center_px": {"px": hx, "py": hy},
            "strongest_hazard": strongest,
            "coordinate_note": (
                "Directions are image directions; OffsetX/Y conversion still "
                "uses the scan rotation inside propose_overlapping_exploration_frame."
            ),
        }

    def _load_all_landscape_memory_entries(self, memory_file="landscape_memory.json"):
        path = self.nav.output_dir / memory_file
        if not path.exists():
            return []
        try:
            data = json.loads(path.read_text())
        except Exception:
            return []
        if isinstance(data, list):
            return data
        if isinstance(data, dict) and isinstance(data.get("entries"), list):
            return data["entries"]
        if isinstance(data, dict) and data.get("scan_frame"):
            return [data]
        return []

    def _world_point_in_scan_frame(self, world_x_A, world_y_A, target):
        ox = float(target["OffsetX"])
        oy = float(target["OffsetY"])
        rx = float(target.get("RangeX", target.get("RangeY", 0.0)))
        ry = float(target.get("RangeY", target.get("RangeX", 0.0)))
        theta = np.deg2rad(float(target.get("Rotation", target.get("Rotation_deg", 0.0)) or 0.0))
        dx = float(world_x_A) - ox
        dy = float(world_y_A) - oy
        local_x = dx * np.cos(theta) + dy * np.sin(theta)
        local_y = -dx * np.sin(theta) + dy * np.cos(theta)
        return abs(local_x) <= 0.5 * rx and abs(local_y) <= 0.5 * ry

    def _memory_hazard_items(self, current_analysis):
        items = []
        for hazard in current_analysis.get("hazards", []) or []:
            if hazard.get("count_for_offset_stop") or hazard.get("avoid_for_clean_area"):
                item = dict(hazard)
                item["source"] = "current_analysis"
                items.append(item)
        for entry in self._load_all_landscape_memory_entries():
            for hazard in entry.get("hazards", []) or []:
                if hazard.get("count_for_offset_stop") or hazard.get("avoid_for_clean_area"):
                    item = dict(hazard)
                    item["source"] = "landscape_memory"
                    item["memory_timestamp"] = entry.get("timestamp")
                    items.append(item)
        return items

    def _hazard_weight(self, hazard):
        weight = 1.0
        if hazard.get("count_for_offset_stop"):
            weight += 3.0
        try:
            weight += min(5.0, abs(float(hazard.get("dz_peak_A", 0.0))) / 3.0)
        except Exception:
            pass
        try:
            weight += min(4.0, float(hazard.get("diameter_A_est", 0.0)) / 80.0)
        except Exception:
            pass
        return weight

    def propose_hazard_avoiding_offset_shift(
        self,
        analysis,
        new_range_A,
        overlap_A,
        fallback_direction="image_left_below",
        safety_margin_A=None,
    ):
        """
        Compute an offset shift that puts known large hazards outside the next scan.

        This goes beyond a symbolic direction by using the weighted world
        center of current large hazards and checking prior memory hazards.  It
        returns a target plan but does not move the microscope.
        """
        frame = self.nav._normalize_memory_scan_frame(
            analysis.get("scan_frame") or self.nav.scan_frame_world_footprint()
        )
        old_offset_x = float(frame["OffsetX_A"])
        old_offset_y = float(frame["OffsetY_A"])
        old_range_x = float(frame["RangeX_A"])
        old_range_y = float(frame["RangeY_A"])
        rotation_deg = float(frame.get("Rotation_deg", 0.0))
        new_range = float(new_range_A)
        overlap = float(overlap_A)
        margin = float(safety_margin_A if safety_margin_A is not None else max(100.0, 0.15 * new_range))
        hazards = self._memory_hazard_items(analysis)

        escape = self.suggest_escape_direction_from_hazards(
            analysis,
            fallback_direction=fallback_direction,
        )
        symbolic_direction = escape.get("suggested_direction") or str(fallback_direction)
        candidates = []

        def make_target(offset_x, offset_y, method, details=None):
            target = {
                "OffsetX": float(offset_x),
                "OffsetY": float(offset_y),
                "RangeX": new_range,
                "RangeY": new_range,
                "Rotation": rotation_deg,
                "direction": symbolic_direction,
                "requested_overlap_A": overlap,
                "method": method,
                "details": details or {},
            }
            target["footprint_reachable"] = self.nav.scan_frame_fits_reachable_area(
                target["OffsetX"],
                target["OffsetY"],
                target["RangeX"],
                target["RangeY"],
                rotation_deg=rotation_deg,
            )
            target["reachable"] = bool(target["footprint_reachable"]["fits"])
            inside = []
            near = []
            near_margin = max(150.0, margin)
            for hazard in hazards:
                wx, wy = self.nav._memory_item_world_center(hazard)
                if wx is None or wy is None:
                    continue
                rec = {
                    "source": hazard.get("source"),
                    "world_x_A": wx,
                    "world_y_A": wy,
                    "feature_class": hazard.get("feature_class"),
                    "count_for_offset_stop": hazard.get("count_for_offset_stop"),
                }
                if self._world_point_in_scan_frame(wx, wy, target):
                    inside.append(rec)
                else:
                    expanded = dict(target)
                    expanded["RangeX"] = new_range + 2.0 * near_margin
                    expanded["RangeY"] = new_range + 2.0 * near_margin
                    if self._world_point_in_scan_frame(wx, wy, expanded):
                        near.append(rec)
            target["hazard_avoidance"] = {
                "known_large_hazards_inside_new_frame": inside,
                "known_large_hazards_near_new_frame": near,
                "inside_count": len(inside),
                "near_count": len(near),
                "known_hazard_count": len(hazards),
            }
            target["score_tuple"] = [
                0 if target["reachable"] else 1,
                len(inside),
                len(near),
                float(np.hypot(target["OffsetX"] - old_offset_x, target["OffsetY"] - old_offset_y)),
            ]
            candidates.append(target)
            return target

        weighted = []
        for hazard in hazards:
            if hazard.get("source") != "current_analysis":
                continue
            wx, wy = self.nav._memory_item_world_center(hazard)
            if wx is None or wy is None:
                continue
            weighted.append((wx, wy, self._hazard_weight(hazard), hazard))

        if weighted:
            total = sum(item[2] for item in weighted) or 1.0
            hx = sum(wx * w for wx, _wy, w, _hazard in weighted) / total
            hy = sum(wy * w for _wx, wy, w, _hazard in weighted) / total
            dx = hx - old_offset_x
            dy = hy - old_offset_y
            theta = np.deg2rad(rotation_deg)
            hazard_local_x = dx * np.cos(theta) + dy * np.sin(theta)
            hazard_local_y = -dx * np.sin(theta) + dy * np.cos(theta)
            target_local = 0.5 * new_range + margin
            desired_local_shift_x = 0.0
            desired_local_shift_y = 0.0
            if abs(hazard_local_x) < target_local:
                sign_x = 1.0 if hazard_local_x >= 0 else -1.0
                desired_local_shift_x = hazard_local_x - sign_x * target_local
            if abs(hazard_local_y) < target_local:
                sign_y = 1.0 if hazard_local_y >= 0 else -1.0
                desired_local_shift_y = hazard_local_y - sign_y * target_local
            if abs(desired_local_shift_x) < 1e-9 and abs(desired_local_shift_y) < 1e-9:
                # Hazards are already outside the requested frame; use a
                # smaller overlapping step in the operator-selected direction.
                step_x = max(0.0, (old_range_x + new_range) / 2.0 - overlap)
                step_y = max(0.0, (old_range_y + new_range) / 2.0 - overlap)
                desired_local_shift_x, desired_local_shift_y = self.nav._direction_to_local_shift(
                    symbolic_direction,
                    step_x,
                    step_y,
                )
            world_shift_x = desired_local_shift_x * np.cos(theta) - desired_local_shift_y * np.sin(theta)
            world_shift_y = desired_local_shift_x * np.sin(theta) + desired_local_shift_y * np.cos(theta)
            make_target(
                old_offset_x + world_shift_x,
                old_offset_y + world_shift_y,
                "computed_from_weighted_current_hazard_center",
                {
                    "weighted_current_hazard_center_world_A": {"x": hx, "y": hy},
                    "weighted_current_hazard_center_local_A": {
                        "x": hazard_local_x,
                        "y": hazard_local_y,
                    },
                    "desired_local_shift_A": {
                        "x": desired_local_shift_x,
                        "y": desired_local_shift_y,
                    },
                    "safety_margin_A": margin,
                },
            )

        for direction in [
            symbolic_direction,
            "image_left",
            "image_right",
            "image_up",
            "image_down",
            "image_left_above",
            "image_left_below",
            "image_right_above",
            "image_right_below",
        ]:
            try:
                plan = self.nav.propose_overlapping_exploration_frame(
                    new_range_A=new_range,
                    overlap_A=overlap,
                    direction=direction,
                    memory_file="landscape_memory.json",
                )
                plan["method"] = "symbolic_direction"
                plan["direction"] = direction
                plan["score_tuple"] = [
                    0 if plan.get("reachable") else 1,
                    plan.get("avoidance_summary", {}).get("inside_count", 999),
                    plan.get("avoidance_summary", {}).get("near_count", 999),
                    float(np.hypot(plan["OffsetX"] - old_offset_x, plan["OffsetY"] - old_offset_y)),
                ]
                candidates.append(plan)
            except Exception:
                pass

        if not candidates:
            return {
                "reachable": False,
                "reason": "No hazard-avoiding offset candidates could be generated.",
                "escape_direction_recommendation": escape,
            }
        candidates.sort(key=lambda item: item.get("score_tuple", [999, 999, 999, 999]))
        best = candidates[0]
        best["candidate_rank"] = 1
        return {
            "recommended_plan": best,
            "candidate_plans": candidates[:8],
            "escape_direction_recommendation": escape,
            "reason": (
                "Best target chosen by footprint reachability, known large "
                "hazards inside frame, known hazards near frame, then shortest shift."
            ),
            "known_hazard_count": len(hazards),
            "current_large_hazard_count": len(weighted),
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
        activity = self.start_tip_planner_activity(
            "analyze tip and flat candidates",
            details={
                "channel": int(channel),
                "lines_to_fetch": int(lines_to_fetch),
                "patch_A": float(patch_A),
                "max_candidates": int(max_candidates),
            },
            pending_next="select or move to a flat candidate",
        )
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
            self.finish_tip_planner_activity(
                activity,
                status="completed",
                result={
                    "candidate_count": len(analysis.get("flat_candidates", [])),
                    "verdict": analysis.get("tip_quality", {}).get("verdict"),
                    "large_hazard_count": blob_summary["large_hazard_count"],
                },
                pending_next="select candidate or run planner loop",
            )
            return fig, self.tip_planner_progress_figure(), self.jsonable(report)
        except Exception as exc:
            self.fail_tip_planner_activity(activity, exc, pending_next="inspect error and retry analysis")
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
        activity = self.start_tip_planner_activity(
            "move tip to selected flat candidate",
            details={"candidate_index": int(candidate_index)},
            pending_next="verify tip marker, then decide scan or GVP",
        )
        try:
            busy = self.scan_is_busy_report()
            if busy.get("busy"):
                self.finish_tip_planner_activity(
                    activity,
                    status="blocked",
                    result={"reason": "scan or GVP busy", "scan_status": busy},
                    pending_next="stop scan/GVP or wait until ready",
                )
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
            self.finish_tip_planner_activity(
                activity,
                status="completed",
                result={
                    "candidate_index": idx + 1,
                    "verified": rec.result_summary.get("verified")
                    if isinstance(rec.result_summary, dict) else None,
                },
                pending_next="start scan, execute GVP, or rerun analysis",
            )
            return self.jsonable(result)
        except Exception as exc:
            self.fail_tip_planner_activity(activity, exc, pending_next="inspect move error")
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def tip_planner_plan_offset_shift(self, direction, new_range_A, overlap_A):
        activity = self.start_tip_planner_activity(
            "plan offset shift",
            details={
                "direction": str(direction),
                "new_range_A": float(new_range_A),
                "overlap_A": float(overlap_A),
            },
            pending_next="apply offset shift if plan is acceptable",
        )
        try:
            plan = self.nav.propose_overlapping_exploration_frame(
                new_range_A=float(new_range_A),
                overlap_A=float(overlap_A),
                direction=str(direction),
            )
            reach_reason = plan.get("footprint_reachable", {}).get("reason")
            self.append_tip_planner_log(
                {
                    "event": "plan_offset_shift",
                    "status": "ok",
                    "plan": plan,
                }
            )
            self.finish_tip_planner_activity(
                activity,
                status="completed",
                result={
                    "reachable": plan.get("reachable"),
                    "reason": reach_reason,
                    "target": plan,
                },
                pending_next=(
                    "apply planned offset shift"
                    if plan.get("reachable") else
                    "choose a smaller/safer offset shift that fits XY limits"
                ),
            )
            return self.jsonable(plan)
        except Exception as exc:
            self.fail_tip_planner_activity(activity, exc, pending_next="inspect shift plan error")
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
        activity = self.start_tip_planner_activity(
            "apply offset shift",
            details={
                "direction": str(direction),
                "new_range_A": float(new_range_A),
                "overlap_A": float(overlap_A),
                "points": int(points),
                "start_after_shift": bool(start_after_shift),
            },
            pending_next="collect new scan and analyze landscape",
        )
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
                result = {
                    "blocked": "offset shift",
                    "reason": (
                        plan.get("footprint_reachable", {}).get("reason")
                        or "Target is outside reachable range."
                    ),
                    "plan": plan,
                }
                self.finish_tip_planner_activity(
                    activity,
                    status="blocked",
                    result=result,
                    pending_next="choose a smaller/safer offset shift that fits XY limits",
                )
                return result
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
            self.finish_tip_planner_activity(
                activity,
                status="completed",
                result={
                    "stopped_scan_before_shift": bool(busy.get("busy")),
                    "started_scan": bool(start_after_shift),
                    "OffsetX": plan.get("OffsetX"),
                    "OffsetY": plan.get("OffsetY"),
                },
                pending_next="wait for enough scan lines, then analyze",
            )
            return self.jsonable(result)
        except Exception as exc:
            self.fail_tip_planner_activity(activity, exc, pending_next="inspect offset shift error")
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

        self.tip_planner_state["stop_requested"] = False
        started = time.time()
        loop_activity = self.start_tip_planner_activity(
            "run tip tune planner loop",
            details={
                "max_cycles": int(max_cycles),
                "channel": int(channel),
                "min_lines": int(min_lines),
                "range_A": float(range_A),
                "points": int(points),
            },
            pending_next="cycle 1: setup scan and collect evidence",
        )
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
                if self.check_tip_planner_stop_requested(loop_activity, loop_report, cycle=cycle):
                    break
                prefix = "tip_planner_cycle_{:02d}".format(cycle)
                step = self.start_tip_planner_activity(
                    "cycle {} setup scan and watch".format(cycle),
                    details={"cycle": cycle, "range_A": float(range_A), "points": int(points)},
                    pending_next="cycle {}: analyze landscape and tip".format(cycle),
                )
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
                self.finish_tip_planner_activity(
                    step,
                    status="completed",
                    result={"assessment_status": assessment.status},
                    pending_next="cycle {}: analyze landscape and tip".format(cycle),
                )
                if self.check_tip_planner_stop_requested(loop_activity, loop_report, cycle=cycle):
                    break
                workflow = self.runner.data.get("last_tip_workflow", {})
                image_path = self.output_dir / "{}_scan_image.npy".format(prefix)
                image = np.load(image_path) if image_path.exists() else None
                step = self.start_tip_planner_activity(
                    "cycle {} analyze landscape and tip".format(cycle),
                    details={"cycle": cycle, "image_available": bool(image is not None)},
                    pending_next="cycle {}: decide next action".format(cycle),
                )
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
                self.finish_tip_planner_activity(
                    step,
                    status="completed",
                    result={
                        "hazard_count": hazard_count,
                        "candidate_count": len(landscape.get("flat_candidates", [])),
                        "verdict": quality.get("verdict"),
                        "dFrequency_Hz": dfreq_report.get("dFrequency_Hz"),
                    },
                    pending_next="cycle {}: decide next action".format(cycle),
                )
                if self.check_tip_planner_stop_requested(loop_activity, loop_report, cycle=cycle):
                    break
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
                    self.finish_tip_planner_activity(
                        loop_activity,
                        status="completed",
                        result={"status": "satisfied", "cycle": cycle},
                        pending_next="idle",
                    )
                    break

                if hazard_count >= int(max_blobs) or blob_summary["too_close_to_clear_area"]:
                    cycle_report["decision"] = (
                        "too_many_large_blobs"
                        if hazard_count >= int(max_blobs)
                        else "large_blob_too_close_to_clear_area"
                    )
                    escape = self.suggest_escape_direction_from_hazards(
                        landscape,
                        fallback_direction=shift_direction,
                    )
                    hazard_plan = self.propose_hazard_avoiding_offset_shift(
                        landscape,
                        new_range_A=range_A,
                        overlap_A=max(0.25 * float(range_A), 200.0),
                        fallback_direction=shift_direction,
                    )
                    plan_target = hazard_plan.get("recommended_plan") or {}
                    cycle_report["escape_direction_recommendation"] = escape
                    cycle_report["hazard_avoiding_offset_plan"] = hazard_plan
                    reason = (
                        "Large hazard count {} reached limit {}."
                        if hazard_count >= int(max_blobs)
                        else "Avoid-worthy hazard is too close to the selected clear area."
                    ).format(hazard_count, int(max_blobs))
                    recommended_next = (
                        "move offset to ({:.3g}, {:.3g}) A using computed "
                        "hazard-avoiding target, scan enough lines to confirm, "
                        "then rerun tip tune planner"
                    ).format(
                        float(plan_target.get("OffsetX", float("nan"))),
                        float(plan_target.get("OffsetY", float("nan"))),
                    ) if plan_target else (
                        "move offset {} with partial overlap, scan enough lines "
                        "to confirm, then rerun tip tune planner"
                    ).format(escape.get("suggested_direction"))
                    cycle_report["blocked_reason"] = reason
                    cycle_report["recommended_next_action"] = recommended_next
                    self.runner.connect().stopscan() if not self.runner.dry_run else None
                    if auto_shift_on_blobs:
                        auto_direction = (
                            plan_target.get("direction")
                            or escape.get("suggested_direction")
                            or shift_direction
                        )
                        self.tip_planner_state["pending_action"] = (
                            "cycle {}: apply offset shift {}".format(cycle, auto_direction)
                        )
                        if plan_target and plan_target.get("reachable"):
                            shift_activity = self.start_tip_planner_activity(
                                "apply computed hazard-avoiding offset shift",
                                details={"cycle": cycle, "target": plan_target},
                                pending_next="scan enough lines to confirm new area",
                            )
                            try:
                                rec = self.nav.setup_exploration_scan_frame(
                                    plan_target["OffsetX"],
                                    plan_target["OffsetY"],
                                    range_A=float(range_A),
                                    points=int(points),
                                    rotation_deg=plan_target.get("Rotation"),
                                )
                                shift = {
                                    "computed_hazard_avoiding_plan": hazard_plan,
                                    "setup_record": rec.__dict__,
                                    "startscan_return": (
                                        self.runner.connect().startscan()
                                        if not self.runner.dry_run else "dry-run"
                                    ),
                                }
                                self.finish_tip_planner_activity(
                                    shift_activity,
                                    status="completed",
                                    result={
                                        "OffsetX": plan_target["OffsetX"],
                                        "OffsetY": plan_target["OffsetY"],
                                        "started_scan": True,
                                    },
                                    pending_next="wait for enough scan lines, then analyze",
                                )
                            except Exception as exc:
                                self.fail_tip_planner_activity(
                                    shift_activity,
                                    exc,
                                    pending_next="choose another computed offset or clear history after hyper jump",
                                )
                                shift = {"error": str(exc), "traceback": traceback.format_exc()}
                        else:
                            shift = self.tip_planner_apply_offset_shift(
                                auto_direction,
                                range_A,
                                max(0.25 * float(range_A), 200.0),
                                points,
                                True,
                                True,
                            )
                        cycle_report["offset_shift"] = shift
                        continue
                    loop_report["status"] = "stopped_for_large_blob_hazard"
                    self.finish_tip_planner_activity(
                        loop_activity,
                        status="blocked",
                        result={
                            "status": "stopped_for_large_blob_hazard",
                            "cycle": cycle,
                            "reason": reason,
                            "large_hazard_count": hazard_count,
                            "max_blobs": int(max_blobs),
                            "suggested_direction": escape.get("suggested_direction"),
                            "recommended_next_action": recommended_next,
                            "escape_direction_recommendation": escape,
                            "hazard_avoiding_offset_plan": hazard_plan,
                        },
                        pending_next=recommended_next,
                    )
                    break

                candidate = landscape.get("best_flat_candidate")
                if not candidate:
                    cycle_report["decision"] = "no_clean_candidate"
                    loop_report["status"] = "needs_offset_search"
                    self.finish_tip_planner_activity(
                        loop_activity,
                        status="blocked",
                        result={"status": "needs_offset_search", "cycle": cycle},
                        pending_next="plan offset shift for cleaner area",
                    )
                    break

                step = self.start_tip_planner_activity(
                    "cycle {} stop scan and move to clean candidate".format(cycle),
                    details={"cycle": cycle, "candidate": candidate},
                    pending_next="cycle {}: execute tip action".format(cycle),
                )
                self.runner.stop_scan_wait_for_gvp(stop_wait_s=3.0, ready_timeout_s=20.0)
                move = self.runner.move_to_scan_xy_fields_verified(
                    candidate["scan_x_A"],
                    candidate["scan_y_A"],
                )
                self.finish_tip_planner_activity(
                    step,
                    status="completed",
                    result={
                        "candidate_scan_x_A": candidate.get("scan_x_A"),
                        "candidate_scan_y_A": candidate.get("scan_y_A"),
                        "verified": move.result_summary.get("verified")
                        if isinstance(move.result_summary, dict) else None,
                    },
                    pending_next="cycle {}: execute tip action".format(cycle),
                )
                if self.check_tip_planner_stop_requested(loop_activity, loop_report, cycle=cycle):
                    break
                plan = self.choose_tip_action_plan(cycle, quality)
                step = self.start_tip_planner_activity(
                    "cycle {} execute tip action".format(cycle),
                    details={"cycle": cycle, "plan": plan},
                    pending_next=(
                        "cycle {}: setup scan and collect evidence".format(cycle + 1)
                        if cycle < int(max_cycles) else "finalize planner loop"
                    ),
                )
                action = self.execute_tip_action_plan(
                    plan,
                    str(self.output_dir / "{}_{}".format(prefix, plan["kind"])),
                )
                self.finish_tip_planner_activity(
                    step,
                    status="completed",
                    result={
                        "plan": plan,
                        "gvp_delta_dFreq_Hz": action.get("gvp_analysis", {}).get("rt_dFreq_delta_Hz"),
                    },
                    pending_next=(
                        "cycle {}: setup scan and collect evidence".format(cycle + 1)
                        if cycle < int(max_cycles) else "finalize planner loop"
                    ),
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
            if loop_activity.get("status") == "in_progress":
                self.finish_tip_planner_activity(
                    loop_activity,
                    status="completed",
                    result={"status": loop_report.get("status")},
                    pending_next="idle",
                )
            return last_fig, self.tip_planner_progress_figure(), self.jsonable(loop_report)
        except Exception as exc:
            loop_report["status"] = "error"
            loop_report["error"] = str(exc)
            loop_report["traceback"] = traceback.format_exc()
            current = self.tip_planner_state.get("current_action")
            if current and current.get("status") == "in_progress" and current is not loop_activity:
                self.finish_tip_planner_activity(
                    current,
                    status="error",
                    result={
                        "error": str(exc),
                        "traceback": traceback.format_exc(),
                    },
                    pending_next="inspect planner error",
                )
            if loop_activity.get("status") == "in_progress":
                self.fail_tip_planner_activity(loop_activity, exc, pending_next="inspect planner error")
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
