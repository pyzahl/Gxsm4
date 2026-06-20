"""GVP-specific Gradio backend mixin."""

import time
import traceback

import matplotlib.pyplot as plt
import numpy as np


class GvpGuiMixin:
    """Load, execute, plot, and summarize GVP programs from the GUI."""

    def load_bias_pulse(self, pulse_du_V, arm):
        blocked = self.require_control_level(1, "load bias-pulse GVP")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "load a GVP program")
        if cancelled:
            return cancelled
        return self.run_microscope_operation(
            "load bias-pulse GVP",
            lambda: self._load_bias_pulse_unlocked(pulse_du_V),
        )

    def _load_bias_pulse_unlocked(self, pulse_du_V):
        try:
            rec = self.runner.load_gvp_bias_pulse(float(pulse_du_V))
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def load_tip_tune_gvp(self, contact_bias_V, dip_depth_A, ramp_time_s, arm):
        blocked = self.require_control_level(1, "load tip-tune Z-dip GVP")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "load a tip-tune GVP program")
        if cancelled:
            return cancelled
        return self.run_microscope_operation(
            "load tip-tune Z-dip GVP",
            lambda: self._load_tip_tune_gvp_unlocked(contact_bias_V, dip_depth_A, ramp_time_s),
        )

    def _load_tip_tune_gvp_unlocked(self, contact_bias_V, dip_depth_A, ramp_time_s):
        try:
            current_scan_bias = self.read_float_parameter("dsp-fbs-bias", fallback=0.1)
            rec = self.runner.load_gvp_tip_dip(
                dip_depth_A=float(dip_depth_A),
                contact_bias_V=float(contact_bias_V),
                scan_bias_V=float(current_scan_bias),
                ramp_time_s=float(ramp_time_s),
                save_prefix=str(self.output_dir / "gui_tip_tune_gvp"),
            )
            result = self.jsonable(rec.__dict__)
            result["parameters"] = {
                "contact_bias_V": float(contact_bias_V),
                "dip_depth_A": -abs(float(dip_depth_A)),
                "ramp_time_s": float(ramp_time_s),
                "scan_bias_V_used_for_du": current_scan_bias,
                "dt_mapping": {
                    "dsp-gvp-dt02": float(ramp_time_s),
                    "dsp-gvp-dt04": float(ramp_time_s),
                },
                "du_mapping": {
                    "dsp-gvp-du01": -float(current_scan_bias),
                    "dsp-gvp-du03": float(contact_bias_V),
                    "dsp-gvp-du06": -float(contact_bias_V),
                    "dsp-gvp-du07": float(current_scan_bias),
                },
                "template_file": "gvp_tip_tune_template_current_latest.json",
                "note": (
                    "Loaded only. Vec 1 removes the actual scan bias, vec 3 "
                    "applies the requested contact bias, vec 6 removes the "
                    "contact bias, and vec 7 restores the scan bias."
                ),
            }
            return result
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def emergency_stop_gvp(self):
        try:
            rec = self.runner.emergency_stop_gvp()
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def execute_loaded_gvp(self, confirm_text, arm):
        blocked = self.require_control_level(1, "execute loaded GVP")
        if blocked:
            return blocked
        cancelled = self.require_arm(arm, "execute loaded GVP")
        if cancelled:
            return cancelled
        phrase = self.safety_limits()["gvp_execute_extra_confirmation"]
        if str(confirm_text).strip() != phrase:
            return {"cancelled": "Type '{}' to execute loaded GVP.".format(phrase)}
        return self.run_microscope_operation(
            "execute loaded GVP",
            self._execute_loaded_gvp_unlocked,
        )

    def _execute_loaded_gvp_unlocked(self):
        try:
            rec = self.runner.action("DSP_VP_VP_EXECUTE")
            return self.jsonable(rec.__dict__)
        except Exception as exc:
            return {"error": str(exc), "traceback": traceback.format_exc()}

    def wait_for_gvp_completion_report(self, max_wait_s=180.0, poll_s=1.0):
        started = time.monotonic()
        max_wait_s = max(0.0, float(max_wait_s))
        poll_s = max(0.1, float(poll_s))
        samples = []
        saw_busy = False
        while True:
            elapsed = time.monotonic() - started
            try:
                status = self.runner.rtquery_scan_gvp_status()
            except Exception as exc:
                return {
                    "completed": False,
                    "error": "{}: {}".format(type(exc).__name__, exc),
                    "elapsed_s": elapsed,
                    "samples": samples,
                }
            status["elapsed_s"] = elapsed
            samples.append(self.jsonable(status))
            if status.get("busy"):
                saw_busy = True
            if status.get("ready") and (saw_busy or status.get("Sgvp") == 5):
                return {
                    "completed": True,
                    "elapsed_s": elapsed,
                    "saw_busy": saw_busy,
                    "final_status": self.jsonable(status),
                    "samples": samples[-20:],
                }
            if elapsed >= max_wait_s:
                return {
                    "completed": False,
                    "timeout": True,
                    "elapsed_s": elapsed,
                    "saw_busy": saw_busy,
                    "final_status": self.jsonable(status),
                    "samples": samples[-20:],
                }
            time.sleep(poll_s)

    def execute_loaded_gvp_with_plot(self, confirm_text, max_wait_s, arm):
        blocked = self.require_control_level(1, "execute loaded GVP")
        if blocked:
            return self.gvp_trace_figure(None, blocked), blocked
        cancelled = self.require_arm(arm, "execute loaded GVP")
        if cancelled:
            return self.gvp_trace_figure(None, cancelled), cancelled
        phrase = self.safety_limits()["gvp_execute_extra_confirmation"]
        if str(confirm_text).strip() != phrase:
            report = {"cancelled": "Type '{}' to execute loaded GVP.".format(phrase)}
            return self.gvp_trace_figure(None, report), report
        operation_blocked = self.acquire_microscope_operation(
            "execute loaded GVP with plot",
            blocking=False,
        )
        if operation_blocked:
            return self.gvp_trace_figure(None, operation_blocked), self.jsonable(operation_blocked)
        try:
            report = {
                "command_order": (
                    "Sent DSP_VP_VP_EXECUTE first. Status, dFreq, and VP trace "
                    "fetch are follow-up diagnostics so they cannot block the "
                    "execute command."
                ),
                "max_wait_s": float(max_wait_s),
            }
            rec = self.runner.action("DSP_VP_VP_EXECUTE")
            report["execute_record"] = self.jsonable(rec.__dict__)
            completion = self.wait_for_gvp_completion_report(max_wait_s=max_wait_s)
            report["completion"] = self.jsonable(completion)

            before_df = None
            after_df = None
            try:
                after_df = self.runner.dFreq()
            except Exception as exc:
                report["rt_dFreq_after_error"] = "{}: {}".format(type(exc).__name__, exc)

            try:
                report["rtquery_s_after_action"] = self.jsonable(
                    self.runner.rtquery_scan_gvp_status()
                )
            except Exception as exc:
                report["rtquery_s_after_action_error"] = "{}: {}".format(
                    type(exc).__name__,
                    exc,
                )

            fetch_record = None
            try:
                fetch_record = self.runner.fetch_vpdata(
                    channel=self.runner.default_channel,
                    nth=-1,
                    start_idx=0,
                    end_idx=200000,
                    collect_as="last_gui_gvp_vpdata",
                )
                report["fetch_vpdata_record"] = self.jsonable(fetch_record.__dict__)
            except Exception as exc:
                report["fetch_vpdata_error"] = "{}: {}".format(type(exc).__name__, exc)

            data = self.runner.data.get("last_gui_gvp_vpdata")
            analysis = self.analyze_gvp_vpdata(data)
            analysis["rt_dFreq_before_Hz"] = before_df
            analysis["rt_dFreq_after_Hz"] = after_df
            analysis["rt_dFreq_delta_Hz"] = (
                None if before_df is None or after_df is None else float(after_df) - float(before_df)
            )
            report["analysis"] = self.jsonable(analysis)
            fig = self.gvp_trace_figure(data, report)
            self.release_microscope_operation()
            return fig, report
        except Exception as exc:
            report = {"error": str(exc), "traceback": traceback.format_exc()}
            self.release_microscope_operation()
            return self.gvp_trace_figure(None, report), report

    def find_vpdata_signal(self, vpdata, candidates):
        if not vpdata:
            return None
        keys = list(vpdata.keys())
        for candidate in candidates:
            for key in keys:
                if str(key).strip().lower() == str(candidate).strip().lower():
                    return key
        for candidate in candidates:
            c = str(candidate).strip().lower()
            for key in keys:
                if c in str(key).strip().lower():
                    return key
        return None

    def gvp_signal_keys(self, data):
        vpdata = (data or {}).get("vpdata") or {}
        return {
            "time": self.find_vpdata_signal(vpdata, ("Time-Mon", "Time", "time")),
            "zs": self.find_vpdata_signal(vpdata, ("ZS-Topo", "ZS", "Z-Scan", "Z")),
            "current": self.find_vpdata_signal(vpdata, ("Current", "I-Mon", "I", "Current-Mon")),
            "dfreq": self.find_vpdata_signal(vpdata, ("dFreq", "DFreq", "dF", "Freq", "Frequency")),
        }

    def window_delta(self, values, fraction=0.1):
        arr = np.asarray(values, dtype=float)
        arr = arr[np.isfinite(arr)]
        if arr.size == 0:
            return {"pre_mean": None, "post_mean": None, "post_minus_pre": None}
        k = max(1, min(arr.size, int(round(arr.size * float(fraction)))))
        pre = arr[:k]
        post = arr[-k:]
        return {
            "pre_mean": float(np.nanmean(pre)),
            "post_mean": float(np.nanmean(post)),
            "post_minus_pre": float(np.nanmean(post) - np.nanmean(pre)),
            "window_points": int(k),
        }

    def analyze_gvp_vpdata(self, data):
        if not data:
            return {"available": False, "message": "No GVP VP data available."}
        vpdata = data.get("vpdata") or {}
        vpunits = data.get("vpunits") or {}
        keys = self.gvp_signal_keys(data)
        result = {
            "available": True,
            "labels": self.jsonable(data.get("labels", [])),
            "signal_keys": keys,
            "units": {name: vpunits.get(key) for name, key in keys.items() if key},
        }
        if keys.get("zs"):
            dz = self.window_delta(vpdata[keys["zs"]])
            result["ZS"] = dz
            result["dZ_before_after_A"] = dz.get("post_minus_pre")
        if keys.get("dfreq"):
            dfr = self.window_delta(vpdata[keys["dfreq"]])
            result["dFreq"] = dfr
            result["dFreq_before_after_delta_Hz"] = dfr.get("post_minus_pre")
        if keys.get("current"):
            cur = self.window_delta(vpdata[keys["current"]])
            result["current"] = cur
            result["current_before_after_delta"] = cur.get("post_minus_pre")
        return result

    def gvp_trace_figure(self, data, report=None):
        fig, axes = plt.subplots(3, 1, figsize=(7, 6), sharex=True, constrained_layout=True)
        report = report or {}
        if not data:
            msg = report.get("error") or report.get("cancelled") or report.get("blocked") or "No GVP VP data available."
            axes[1].text(0.5, 0.5, str(msg), ha="center", va="center", wrap=True)
            for ax in axes:
                ax.set_axis_off()
            return fig
        vpdata = data.get("vpdata") or {}
        vpunits = data.get("vpunits") or {}
        keys = self.gvp_signal_keys(data)
        time_key = keys.get("time")
        if time_key and time_key in vpdata:
            x = np.asarray(vpdata[time_key], dtype=float)
            xlabel = "{} ({})".format(time_key, vpunits.get(time_key, ""))
        else:
            first_key = next(iter(vpdata), None)
            n = 0 if first_key is None else len(np.asarray(vpdata[first_key]))
            x = np.arange(n)
            xlabel = "sample index"
        specs = [
            ("zs", "ZS", "ZS-Topo", "A"),
            ("current", "Current", "Current", ""),
            ("dfreq", "dFreq", "dFreq", "Hz"),
        ]
        for ax, (name, title, fallback_label, fallback_unit) in zip(axes, specs):
            key = keys.get(name)
            if key and key in vpdata:
                y = np.asarray(vpdata[key], dtype=float)
                m = min(len(x), len(y))
                ax.plot(x[:m], y[:m], lw=1.1)
                unit = vpunits.get(key, fallback_unit)
                ax.set_ylabel("{}\n{}".format(title, unit))
                ax.grid(True, alpha=0.25)
            else:
                ax.text(0.5, 0.5, "{} signal not found".format(fallback_label), ha="center", va="center")
                ax.set_axis_off()
        analysis = (report or {}).get("analysis") or self.analyze_gvp_vpdata(data)
        lines = []
        dz = analysis.get("dZ_before_after_A")
        dfr = analysis.get("dFreq_before_after_delta_Hz")
        rt_dfr = analysis.get("rt_dFreq_delta_Hz")
        if dz is not None:
            lines.append("dZ post-pre: {:.6g} A".format(float(dz)))
        if dfr is not None:
            lines.append("trace dFreq post-pre: {:.6g} Hz".format(float(dfr)))
        if rt_dfr is not None:
            lines.append("rt dFreq after-before: {:.6g} Hz".format(float(rt_dfr)))
        axes[0].set_title("GVP probe event" + (" | " + " | ".join(lines) if lines else ""))
        axes[-1].set_xlabel(xlabel)
        return fig
