/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: rpspmc_pacpll.C
 * ========================================
 * 
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Andreas Klust <klust@fkp.uni-hannover.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <libsoup/soup.h>
#include <zlib.h>
#include <gsl/gsl_rstat.h>

#include "config.h"
#include "plugin.h"

#include "unit.h"
#include "pcs.h"
#include "xsmtypes.h"
#include "glbvars.h"
#include "action_id.h"

#include "gxsm_app.h"
#include "gxsm_window.h"
#include "surface.h"

#include "../common/pyremote.h"

#include "rpspmc_hwi_dev.h"
#include "rpspmc_pacpll.h"
#include "rpspmc_control.h"
#include "rpspmc_gvpmover.h"

#include "plug-ins/control/resonance_fit.h"

// Define HwI PlugIn reference name here, this is what is listed later within "Preferenced Dialog"
// i.e. the string selected for "Hardware/Card"!
#define THIS_HWI_PLUGIN_NAME "RPSPMC:SPM"
#define THIS_HWI_PREFIX      "RPSPMC_HwI"

#define GVP_SHIFT_UP  1
#define GVP_SHIFT_DN -1


extern int debug_level;
extern int force_gxsm_defaults;


extern GxsmPlugin rpspmc_pacpll_hwi_pi;

extern RPspmc_pacpll *rpspmc_pacpll;
extern rpspmc_hwi_dev *rpspmc_hwi;
extern RPSPMC_Control *RPSPMC_ControlClass;

// defined in rpspmc_control.cpp via include of rpsmc_signals.cpp
extern SOURCE_SIGNAL_DEF rpspmc_source_signals[];
extern SOURCE_SIGNAL_DEF rpspmc_swappable_signals[];
extern SOURCE_SIGNAL_DEF modulation_targets[];
extern SOURCE_SIGNAL_DEF z_servo_current_source[];
extern SOURCE_SIGNAL_DEF rf_gen_out_dest[];

// GVP MUX en/decoding helper functions are defined in rpspmc_control.cpp
guint64 __GVP_selection_muxval (int selection[6]);
int __GVP_selection_muxHval (int selection[6]);
int __GVP_muxval_selection (guint64 mux, int k);

// data passed to "idle" function call, used to refresh/draw while waiting for data
typedef struct {
	GSList *scan_list; // scans to update
	GFunc  UpdateFunc; // function to call for background updating
	gpointer data; // additional data (here: reference to the current rpspmc_pacpll object)
} IdleRefreshFuncData;




/* **************************************** PAC-PLL GUI **************************************** */

static void get_rpdata_vector (GtkWidget* w, void *data){
        RPspmc_pacpll* self= (RPspmc_pacpll*) data;
        self->get_transport ();
}

#define dB_min_from_Q(Q) (20.*log10(1./((1L<<(Q))-1)))
#define dB_max_from_Q(Q) (20.*log10(1L<<((32-(Q))-1)))
#define SETUP_dB_RANGE_from_Q(PCS, Q) { PCS->setMin(dB_min_from_Q(Q)); PCS->setMax(dB_max_from_Q(Q)); }

RPspmc_pacpll::RPspmc_pacpll (Gxsm4app *app):AppBase(app),RP_JSON_talk(){
        GtkWidget *tmp;
        GtkWidget *wid;

        rt_monitors_shm_ptr = NULL;

        static const gchar* Y1Y2_tm[] = {
                "IN1, IN2 ",             // [0] SCOPE
                "IN1-AC, IN1-DC ",          // [1] MON
                "Ampl, Exec ",      // [2] AMC Adjust
                "dPhase, dFreq ",   // [3] PHC Adjust
                "Phase, Ampl ",          // [4] TUNE
                "Phase, dFreq ", // [5] SCAN
                "dFreq, dFControl ", // [6] DFC Adjust
                "DDR-IN1, DDR-IN2 ",          // [7] SCOPE with DEC=1,SHR=0 Double(max) Data Rate config
                "DEBUG-McBP1, DEBUG-McBSP2 ",          // [8]
                NULL };
        Y1Y2_transport_mode = Y1Y2_tm;
        
	GSList *EC_R_list=NULL;
	GSList *EC_QC_list=NULL;
	GSList *EC_FP_list=NULL;

        scan_gvp_options = 0;
        debug_level = 0;
        input_rpaddress = NULL;
        text_status = NULL;
        streaming = 0;

        resonator_frequency_fitted = -1.;
        resonator_phase_fitted = -1.;
        
        ch_freq = -1;
        ch_ampl = -1;
        deg_extend = 1;

        for (int i=0; i<5; ++i){
                scope_ac[i]=false;
                scope_dc_level[i]=0.;
                gain_scale[i] = 0.001; // 1000mV full scale
        }
        unwrap_phase_plot = true;
        
        
	PI_DEBUG (DBG_L2, "RPspmc_pacpll Plugin : building interface" );

	Unity    = new UnitObj(" "," ");
	Hz       = new UnitObj("Hz","Hz");
	Deg      = new UnitObj(UTF8_DEGREE,"deg");
	VoltDeg  = new UnitObj("V/" UTF8_DEGREE, "V/deg");
	Volt     = new UnitObj("V","V");
	mVolt     = new UnitObj("mV","mV");
	VoltHz   = new UnitObj("mV/Hz","mV/Hz");
	dB       = new UnitObj("dB","dB");
	Time     = new UnitObj("s","s");
	mTime    = new LinUnit("ms", "ms", "Time");
	uTime    = new LinUnit(UTF8_MU "s", "us", "Time");

        // Window Title
	AppWindowInit("RPSPMC PACPLL Control for RedPitaya");

        gchar *gs_path = g_strdup_printf ("%s.hwi.rpspmc-control", GXSM_RES_BASE_PATH_DOT);
        inet_json_settings = g_settings_new (gs_path);

        bp = new BuildParam (v_grid);
        bp->set_no_spin (true);

        bp->set_pcs_remote_prefix ("rp-pacpll-");

        bp->set_input_width_chars (16);
        bp->set_default_ec_change_notice_fkt (NULL, NULL);

        bp->new_grid_with_frame ("RedPitaya PAC Setup");
        bp->set_input_nx (2);
        bp->grid_add_ec ("In1 Offset", mVolt, &parameters.dc_offset, -1000.0, 1000.0, "g", 0.1, 1., "DC-OFFSET");
        EC_R_list = g_slist_prepend( EC_R_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();

        parameters.transport_tau[0] = -1.; // us, negative = disabled [bit 32 set in FPGA tau)
        parameters.transport_tau[1] = -1.; // us, negative = disabled [bit 32 set in FPGA tau)
        parameters.transport_tau[2] = -1.; // us, negative = disabled [bit 32 set in FPGA tau)
        parameters.transport_tau[3] = -1.; // us, negative = disabled [bit 32 set in FPGA tau)

        parameters.pac_dctau = 0.1; // ms
        parameters.pactau  = 50.0; // us
        parameters.pacatau = 30.0; // us
        parameters.frequency_manual = 32768.0; // Hz
        parameters.frequency_center = 32768.0; // Hz
        parameters.volume_manual = 300.0; // mV
        parameters.qc_gain=0.0; // gain +/-1.0
        parameters.qc_phase=0.0; // deg
        bp->set_input_width_chars (8);
        bp->set_input_nx (1);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::pac_tau_parameter_changed, this);
  	bp->grid_add_ec ("Tau DC", mTime, &parameters.pac_dctau, -1.0, 63e6, "6g", 10., 1., "PAC-DCTAU");
        bp->new_line ();
  	bp->grid_add_ec ("Tau PAC", uTime, &parameters.pactau, 0.0, 63e6, "6g", 10., 1., "PACTAU");
  	bp->grid_add_ec (NULL, uTime, &parameters.pacatau, 0.0, 63e6, "6g", 10., 1., "PACATAU");
        bp->new_line ();
        bp->set_no_spin (false);
        bp->set_input_nx (2);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::qc_parameter_changed, this);
  	bp->grid_add_ec ("QControl",Deg, &parameters.qc_phase, 0.0, 360.0, "5g", 10., 1., "QC-PHASE");
        EC_QC_list = g_slist_prepend( EC_QC_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();
  	bp->grid_add_ec ("QC Gain", Unity, &parameters.qc_gain, -1.0, 1.0, "4g", 10., 1., "QC-GAIN");
        EC_QC_list = g_slist_prepend( EC_QC_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();
        bp->set_no_spin (false);
        bp->set_input_width_chars (12);
        bp->set_input_nx (2);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::pac_frequency_parameter_changed, this);
  	bp->grid_add_ec ("Frequency", Hz, &parameters.frequency_manual, 0.0, 20e6, ".3lf", 0.1, 10., "FREQUENCY-MANUAL");
        EC_FP_list = g_slist_prepend( EC_FP_list, bp->ec);
        bp->new_line ();
        bp->set_input_width_chars (8);
        bp->set_input_nx (1);
  	bp->grid_add_ec ("Center", Hz, &parameters.frequency_center, 0.0, 20e6, ".3lf", 0.1, 10., "FREQUENCY-CENTER");
        EC_FP_list = g_slist_prepend( EC_FP_list, bp->ec);
  	bp->grid_add_button (N_("Copy"), "Copy F-center result from tune.", 1,
                             G_CALLBACK (RPspmc_pacpll::copy_f0_callback), this);
        GtkWidget *CpyButton = bp->button;

        bp->new_line ();
        bp->set_no_spin (false);
        bp->set_input_nx (2);
        bp->set_input_width_chars (12);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::pac_volume_parameter_changed, this);
  	bp->grid_add_ec ("Volume", mVolt, &parameters.volume_manual, 0.0, 1000.0, "5g", 0.1, 1.0, "VOLUME-MANUAL");
        bp->new_line ();
        bp->set_no_spin (true);
        bp->set_input_width_chars (10);
        parameters.tune_dfreq = 0.1;
        parameters.tune_span = 50.0;
        bp->set_input_nx (1);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::tune_parameter_changed, this);
  	bp->grid_add_ec ("Tune dF,Span", Hz, &parameters.tune_dfreq, 1e-4, 1e3, "g", 0.01, 0.1, "TUNE-DFREQ");
  	bp->grid_add_ec (NULL, Hz, &parameters.tune_span, 0.0, 1e6, "g", 0.1, 10., "TUNE-SPAN");

        bp->pop_grid ();
        bp->set_default_ec_change_notice_fkt (NULL, NULL);

        bp->new_grid_with_frame ("Amplitude Controller");
        bp->set_input_nx (3);
        bp->grid_add_ec ("Reading", mVolt, &parameters.volume_monitor, -1.0, 1.0, "g", 0.1, 1., "VOLUME-MONITOR");
        EC_R_list = g_slist_prepend( EC_R_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();
        parameters.amplitude_fb_setpoint = 12.0; // mV
        parameters.amplitude_fb_invert = 1.;
        parameters.amplitude_fb_cp_db = -25.;
        parameters.amplitude_fb_ci_db = -35.;
        parameters.exec_fb_upper = 500.0;
        parameters.exec_fb_lower = -500.0;
        bp->set_no_spin (false);
        bp->set_input_width_chars (10);

        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::amp_ctrl_parameter_changed, this);
        bp->grid_add_ec ("Setpoint", mVolt, &parameters.amplitude_fb_setpoint, 0.0, 1000.0, "5g", 0.1, 10.0, "AMPLITUDE-FB-SETPOINT");
        bp->new_line ();
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::amplitude_gain_changed, this);
        bp->grid_add_ec ("CP gain", dB, &parameters.amplitude_fb_cp_db, -200.0, 200.0, "g", 0.1, 1.0, "AMPLITUDE-FB-CP");
        SETUP_dB_RANGE_from_Q(bp->ec, 31); // Amplitude QAMCOEF = Q31
        bp->new_line ();
        bp->grid_add_ec ("CI gain", dB, &parameters.amplitude_fb_ci_db, -200.0, 200.0, "g", 0.1, 1.0, "AMPLITUDE-FB-CI");
        SETUP_dB_RANGE_from_Q(bp->ec, 31); // Amplitude QAMCOEF = Q31
        bp->new_line ();
        bp->set_no_spin (true);
        bp->set_input_width_chars (16);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::amp_ctrl_parameter_changed, this);
        bp->set_input_nx (1);
        bp->set_input_width_chars (10);
        bp->grid_add_ec ("Limits", mVolt, &parameters.exec_fb_lower, -1000.0, 1000.0, "g", 1.0, 10.0, "EXEC-FB-LOWER");
        bp->grid_add_ec ("...", mVolt, &parameters.exec_fb_upper, 0.0, 1000.0, "g", 1.0, 10.0, "EXEC-FB-UPPER");
        bp->new_line ();
        bp->set_input_width_chars (16);
        bp->set_input_nx (3);
        bp->set_default_ec_change_notice_fkt (NULL, NULL);
        bp->grid_add_ec ("Exec Amp", mVolt, &parameters.exec_amplitude_monitor, -1000.0, 1000.0, "g", 0.1, 1., "EXEC-AMPLITUDE-MONITOR");
        EC_R_list = g_slist_prepend( EC_R_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();
        bp->set_input_nx (1);
        bp->grid_add_check_button ( N_("Enable"), bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Enable Amplitude Controller","rp-pacpll-AMcontroller"), 2,
                                    G_CALLBACK (RPspmc_pacpll::amplitude_controller), this);
        bp->grid_add_check_button ( N_("Invert"), "Invert Amplitude Controller Gain. Normally positive.", 2,
                                    G_CALLBACK (RPspmc_pacpll::amplitude_controller_invert), this);

        bp->new_line ();
        bp->grid_add_check_button ( N_("Set Auto Trigger SS"), "Set Auto Trigger SS", 2,
                                    G_CALLBACK (RPspmc_pacpll::set_ss_auto_trigger), this);
        bp->grid_add_check_button ( N_("QControl"), "QControl", 2,
                                    G_CALLBACK (RPspmc_pacpll::qcontrol), this);
	g_object_set_data( G_OBJECT (bp->button), "QC_SETTINGS_list", EC_QC_list);
        bp->new_line ();
        bp->grid_add_check_button ( N_("Use LockIn Mode*"), "Use LockIn Mode.\n*: Must use PAC/LockIn FPGA bit code\n instead of Dual PAC FPGA bit code.", 2,
                                    G_CALLBACK (RPspmc_pacpll::select_pac_lck_amplitude), this);
        bp->new_line ();
        bp->grid_add_check_button ( N_("Pulse Control"), "Show Pulse Controller", 2,
                                    G_CALLBACK (RPspmc_pacpll::show_pulse_control), this);

        bp->pop_grid ();

        bp->new_grid_with_frame ("Phase Controller");
        bp->set_input_nx (3);
        bp->grid_add_ec ("Reading", Deg, &parameters.phase_monitor, -180.0, 180.0, "g", 1., 10., "PHASE-MONITOR");
        EC_R_list = g_slist_prepend( EC_R_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();
        parameters.phase_fb_setpoint = 60.;
        parameters.phase_fb_invert = 1.;
        parameters.phase_fb_cp_db = -123.5;
        parameters.phase_fb_ci_db = -184.;
        parameters.freq_fb_upper = 34000.;
        parameters.freq_fb_lower = 29000.;
        bp->set_no_spin (false);
        bp->set_input_width_chars (8);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::phase_ctrl_parameter_changed, this);
        bp->grid_add_ec ("Setpoint", Deg, &parameters.phase_fb_setpoint, -180.0, 180.0, "5g", 0.1, 1.0, "PHASE-FB-SETPOINT");
        bp->new_line ();
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::phase_gain_changed, this);
        bp->grid_add_ec ("CP gain", dB, &parameters.phase_fb_cp_db, -200.0, 200.0, "g", 0.1, 1.0, "PHASE-FB-CP");
        SETUP_dB_RANGE_from_Q(bp->ec, 31); // PHASE  QPHCOEF = Q31 
        bp->new_line ();
        bp->grid_add_ec ("CI gain", dB, &parameters.phase_fb_ci_db, -200.0, 200.0, "g", 0.1, 1.0, "PHASE-FB-CI");
        SETUP_dB_RANGE_from_Q(bp->ec, 31); // PHASE  QPHCOEF = Q31 
        bp->new_line ();
        bp->set_no_spin (true);
        bp->set_input_width_chars (16);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::phase_ctrl_parameter_changed, this);
        bp->set_input_width_chars (10);
        bp->set_input_nx (1);
        bp->grid_add_ec ("Limits", Hz, &parameters.freq_fb_lower, 0.0, 25e6, "g", 0.1, 1.0, "FREQ-FB-LOWER");
        bp->grid_add_ec ("...", Hz, &parameters.freq_fb_upper, 0.0, 25e6, "g", 0.1, 1.0, "FREQ-FB-UPPER");
        bp->new_line ();
        bp->set_input_width_chars (16);
        bp->set_input_nx (3);
        bp->set_default_ec_change_notice_fkt (NULL, NULL);
        bp->grid_add_ec ("DDS Freq", Hz, &parameters.dds_frequency_monitor, 0.0, 25e6, ".4lf", 0.1, 1., "DDS-FREQ-MONITOR");
        EC_R_list = g_slist_prepend( EC_R_list, bp->ec);
        input_ddsfreq=bp->ec;
        bp->ec->Freeze ();

        bp->new_line ();
        bp->grid_add_ec ("AM Lim", mVolt, &parameters.phase_hold_am_noise_limit, 0.0, 100.0, "g", 0.1, 1.0, "PHASE-HOLD-AM-NOISE-LIMIT");

        bp->new_line ();
        bp->set_input_nx (1);
        bp->grid_add_check_button ( N_("Enable"), bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Enable Phase Controller","rp-pacpll-PHcontroller"), 2,
                                    G_CALLBACK (RPspmc_pacpll::phase_controller), this);
        bp->grid_add_check_button ( N_("Invert"), "Invert Phase Controller Gain. Normally positive.", 2,
                                    G_CALLBACK (RPspmc_pacpll::phase_controller_invert), this);
        bp->new_line ();
        bp->grid_add_check_button ( N_("Unwrapping"), "Always unwrap phase/auto unwrap only if controller is enabled", 2,
                                    G_CALLBACK (RPspmc_pacpll::phase_unwrapping_always), this);
        //bp->grid_add_check_button ( N_("Unwap Plot"), "Unwrap plot at high level", 2,
        //                            G_CALLBACK (RPspmc_pacpll::phase_unwrap_plot), this);
        GtkWidget *cbrotab = gtk_combo_box_text_new ();
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbrotab), "0", "r0");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbrotab), "1", "r45");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbrotab), "2", "r-45");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbrotab), "3", "r90");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbrotab), "4", "r-90");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbrotab), "5", "r180");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbrotab), "6", "r-180");
        g_signal_connect (G_OBJECT (cbrotab), "changed",
                          G_CALLBACK (RPspmc_pacpll::phase_rot_ab), 
                          this);				
        gtk_combo_box_set_active_id (GTK_COMBO_BOX (cbrotab), "0");
        bp->grid_add_widget (cbrotab);

        bp->new_line ();
        bp->grid_add_check_button ( N_("Use LockIn Mode"), "Use LockIn Mode", 2,
                                    G_CALLBACK (RPspmc_pacpll::select_pac_lck_phase), this);
        bp->grid_add_check_button ( N_("dFreq Control"), "Show delta Frequency Controller", 2,
                                    G_CALLBACK (RPspmc_pacpll::show_dF_control), this);

        bp->pop_grid ();

        // =======================================
        bp->new_grid_with_frame ("delta Frequency Controller");
        dF_control_frame = bp->frame;
        bp->set_input_nx (3);
        bp->grid_add_ec ("Reading", Hz, &parameters.dfreq_monitor, -200.0, 200.0, "g", 1., 10., "DFREQ-MONITOR");
        EC_R_list = g_slist_prepend( EC_R_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();
        parameters.dfreq_fb_setpoint = -3.0;
        parameters.dfreq_fb_invert = 1.;
        parameters.dfreq_fb_cp_db = -70.;
        parameters.dfreq_fb_ci_db = -120.;
        parameters.control_dfreq_fb_upper = 300.;
        parameters.control_dfreq_fb_lower = -300.;
        bp->set_no_spin (false);
        bp->set_input_width_chars (8);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::dfreq_ctrl_parameter_changed, this);
        bp->grid_add_ec ("Setpoint", Hz, &parameters.dfreq_fb_setpoint, -200.0, 200.0, "5g", 0.1, 1.0, "DFREQ-FB-SETPOINT");
        bp->new_line ();
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::dfreq_gain_changed, this);
        bp->grid_add_ec ("CP gain", dB, &parameters.dfreq_fb_cp_db, -200.0, 200.0, "g", 0.1, 1.0, "DFREQ-FB-CP");
        bp->new_line ();
        bp->grid_add_ec ("CI gain", dB, &parameters.dfreq_fb_ci_db, -200.0, 200.0, "g", 0.1, 1.0, "DFREQ-FB-CI");
        bp->new_line ();
        bp->set_no_spin (true);
        bp->set_input_width_chars (16);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::dfreq_ctrl_parameter_changed, this);
        bp->set_input_width_chars (10);
        bp->set_input_nx (1);
        bp->grid_add_ec ("Limits", mVolt, &parameters.control_dfreq_fb_lower, -10000.0, 10000.0, "g", 0.1, 1.0, "CONTROL-DFREQ-FB-LOWER");
        bp->grid_add_ec ("...", mVolt, &parameters.control_dfreq_fb_upper, -10000.0, 10000.0, "g", 0.1, 1.0, "CONTROL-DFREQ-FB-UPPER");
        bp->new_line ();
        bp->set_input_width_chars (16);
        bp->set_input_nx (3);
        bp->set_default_ec_change_notice_fkt (NULL, NULL);
        bp->grid_add_ec ("Control", mVolt, &parameters.control_dfreq_monitor, 0.0, 25e6, ".4lf", 0.1, 1., "DFREQ-CONTROL-MONITOR");
        EC_R_list = g_slist_prepend( EC_R_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();
        bp->set_input_nx (1);
        bp->grid_add_check_button ( N_("Enable"), bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Enable Dfreq Controller","rp-pacpll-DFcontroller"), 2,
                                    G_CALLBACK (RPspmc_pacpll::dfreq_controller), this);
        bp->grid_add_check_button ( N_("Invert"), "Invert Dfreq Controller Gain. Normally positive.", 2,
                                    G_CALLBACK (RPspmc_pacpll::dfreq_controller_invert), this);

        bp->new_line ();
        bp->grid_add_check_button ( N_("Control Z"), bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Engage Dfreq Controller on Z","rp-pacpll-ZDFcontrol"), 2,
                                    G_CALLBACK (RPspmc_pacpll::EnZdfreq_control), this);
        bp->new_line ();
        bp->grid_add_check_button ( N_("Control Bias"), bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Engage Dfreq Controller on Bias","rp-pacpll-UDFcontrol"), 2,
                                    G_CALLBACK (RPspmc_pacpll::EnUdfreq_control), this);

        // =======================================
        bp->pop_grid ();
        
        bp->new_grid_with_frame ("Pulse Former");
        pulse_control_frame = bp->frame;
        parameters.pulse_form_bias0 = 0;
        parameters.pulse_form_bias1 = 0;
        parameters.pulse_form_phase0 = 0;
        parameters.pulse_form_phase1 = 0;
        parameters.pulse_form_width0 = 0.1;
        parameters.pulse_form_width1 = 0.1;
        parameters.pulse_form_width0if = 0.5;
        parameters.pulse_form_width1if = 0.5;
        parameters.pulse_form_height0 = 200.;
        parameters.pulse_form_height1 = -200.;
        parameters.pulse_form_height0if = -40.;
        parameters.pulse_form_height1if = 40.;
        parameters.pulse_form_shapex = 1.;
        parameters.pulse_form_shapexif = 1.;
        parameters.pulse_form_shapexw = 0.;
        parameters.pulse_form_shapexwif = 0.;
        parameters.pulse_form_enable = false;
        bp->set_no_spin (false);
        bp->set_input_width_chars (6);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::pulse_form_parameter_changed, this);
        GtkWidget *inputMB = bp->grid_add_ec ("Bias", mVolt, &parameters.pulse_form_bias0, 0.0, 180.0, "5g", 0.1, 1.0, "PULSE-FORM-BIAS0");
        GtkWidget *inputCB = bp->grid_add_ec (NULL,     mVolt, &parameters.pulse_form_bias1, 0.0, 180.0, "5g", 0.1, 1.0, "PULSE-FORM-BIAS1");
        g_object_set_data( G_OBJECT (inputMB), "HasClient", inputCB);
        bp->new_line ();
        GtkWidget *inputMP = bp->grid_add_ec ("Phase", Deg, &parameters.pulse_form_phase0, 0.0, 180.0, "5g", 1.0, 10.0, "PULSE-FORM-PHASE0");
        GtkWidget *inputCP = bp->grid_add_ec (NULL,      Deg, &parameters.pulse_form_phase1, 0.0, 180.0, "5g", 1.0, 10.0, "PULSE-FORM-PHASE1");
        g_object_set_data( G_OBJECT (inputMP), "HasClient", inputCP);

        bp->new_line ();
        GtkWidget *inputMW = bp->grid_add_ec ("Width", uTime, &parameters.pulse_form_width0, 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-WIDTH0");
        GtkWidget *inputCW = bp->grid_add_ec (NULL,      uTime, &parameters.pulse_form_width1, 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-WIDTH1");
        g_object_set_data( G_OBJECT (inputMW), "HasClient", inputCW);
        bp->new_line ();
        GtkWidget *inputMWI = bp->grid_add_ec ("WidthIF", uTime, &parameters.pulse_form_width0if, 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-WIDTH0IF");
        GtkWidget *inputCWI = bp->grid_add_ec (NULL,        uTime, &parameters.pulse_form_width1if, 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-WIDTH1IF");
        g_object_set_data( G_OBJECT (inputMWI), "HasClient", inputCWI);
        bp->new_line ();
        
        GtkWidget *inputMH = bp->grid_add_ec ("Height", mVolt, &parameters.pulse_form_height0, -1000.0, 1000.0, "5g", 1.0, 10.0, "PULSE-FORM-HEIGHT0");
        GtkWidget *inputCH = bp->grid_add_ec (NULL,       mVolt, &parameters.pulse_form_height1, -1000.0, 1000.0, "5g", 1.0, 10.0, "PULSE-FORM-HEIGTH1");
        bp->new_line ();
        GtkWidget *inputMHI = bp->grid_add_ec ("HeightIF", mVolt, &parameters.pulse_form_height0if, -1000.0, 1000.0, "5g", 1.0, 10.0, "PULSE-FORM-HEIGHT0IF");
        GtkWidget *inputCHI = bp->grid_add_ec (NULL,         mVolt, &parameters.pulse_form_height1if, -1000.0, 1000.0, "5g", 1.0, 10.0, "PULSE-FORM-HEIGTH1IF");
        bp->new_line ();
        
        GtkWidget *inputX  = bp->grid_add_ec ("Shape", Unity, &parameters.pulse_form_shapex, -10.0, 10.0, "4g", 0.1, 1.0, "PULSE-FORM-SHAPEX");
        GtkWidget *inputXI = bp->grid_add_ec (NULL,    Unity, &parameters.pulse_form_shapexif, -10.0, 10.0, "4g", 0.1, 1.0, "PULSE-FORM-SHAPEXIF");
        g_object_set_data( G_OBJECT (inputX), "HasClient", inputXI);
        bp->new_line ();
        GtkWidget *inputXW  = bp->grid_add_ec ("ShapeW", Unity, &parameters.pulse_form_shapexw, 0.0, 1.0, "4g", 0.1, 1.0, "PULSE-FORM-SHAPEXW");
        GtkWidget *inputXWI = bp->grid_add_ec (NULL,    Unity, &parameters.pulse_form_shapexwif, 0.0, 1.0, "4g", 0.1, 1.0, "PULSE-FORM-SHAPEXWIF");
        g_object_set_data( G_OBJECT (inputXW), "HasClient", inputXWI);

#if 1
        bp->new_line ();
        GtkWidget *inputDPt0 = bp->grid_add_ec ("DPt0v0", uTime, &parameters.pulse_form_dpt[0], 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-DPT0");
        GtkWidget *inputDPv0 = bp->grid_add_ec (NULL,     Unity, &parameters.pulse_form_dpv[0], 0.0, 3.0, "5g", 1.0, 1.0, "PULSE-FORM-DPV0");
        bp->new_line ();
        GtkWidget *inputDPt1 = bp->grid_add_ec ("DPt1v1", uTime, &parameters.pulse_form_dpt[1], 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-DPT1");
        GtkWidget *inputDPv1 = bp->grid_add_ec (NULL,     Unity, &parameters.pulse_form_dpv[1], 0.0, 3.0, "5g", 1.0, 1.0, "PULSE-FORM-DPV1");
        bp->new_line ();
        GtkWidget *inputDPt2 = bp->grid_add_ec ("DPt2v2", uTime, &parameters.pulse_form_dpt[2], 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-DPT2");
        GtkWidget *inputDPv2 = bp->grid_add_ec (NULL,     Unity, &parameters.pulse_form_dpv[2], 0.0, 3.0, "5g", 1.0, 1.0, "PULSE-FORM-DPV2");
        bp->new_line ();
        GtkWidget *inputDPt3 = bp->grid_add_ec ("DPt3v3", uTime, &parameters.pulse_form_dpt[3], 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-DPT3");
        GtkWidget *inputDPv3 = bp->grid_add_ec (NULL,     Unity, &parameters.pulse_form_dpv[3], 0.0, 3.0, "5g", 1.0, 1.0, "PULSE-FORM-DPV3");
#endif
        
        bp->new_line ();
        bp->set_input_nx (1);
        bp->grid_add_check_button ( N_("Enable"),  bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Enable Pulse Forming","rp-pacpll-PF"), 1,
                                    G_CALLBACK (RPspmc_pacpll::pulse_form_enable), this);

        bp->grid_add_exec_button ( N_("Single Shot"),
                                   G_CALLBACK (RPspmc_pacpll::pulse_form_fire), this, "FirePulse",
                                   1);

        GtkWidget *pf_ts = gtk_combo_box_text_new (); // Pulse Form Trigger Source
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (pf_ts), "0", "QP");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (pf_ts), "1", "LCK");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (pf_ts), "2", "GVP");
        g_signal_connect (G_OBJECT (pf_ts), "changed",
                          G_CALLBACK (RPspmc_pacpll::pulse_form_pf_ts), 
                          this);				
        gtk_combo_box_set_active_id (GTK_COMBO_BOX (pf_ts), "0");
        bp->grid_add_widget (pf_ts);
        

        // =======================================

        bp->pop_grid ();
        bp->new_line ();

        // ========================================
        
        bp->new_grid_with_frame ("Oscilloscope and Data Transfer Control", 10);
        // OPERATION MODE
	wid = gtk_combo_box_text_new ();

        bp->set_input_width_chars (8);
        bp->set_input_nx (1);
        /*
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::pac_tau_transport_changed, this);
  	bp->grid_add_ec ("LP dFreq,Phase,Exec,Ampl", mTime, &parameters.transport_tau[0], -1.0, 63e6, "6g", 10., 1., "PAC-TRANSPORT-TAU-DFREQ");
  	bp->grid_add_ec (NULL, mTime, &parameters.transport_tau[1], -1.0, 63e6, "6g", 10., 1., "PAC-TRANSPORT-TAU-PHASE");
  	bp->grid_add_ec (NULL, mTime, &parameters.transport_tau[2], -1.0, 63e6, "6g", 10., 1., "PAC-TRANSPORT-TAU-EXEC");
  	bp->grid_add_ec (NULL, mTime, &parameters.transport_tau[3], -1.0, 63e6, "6g", 10., 1., "PAC-TRANSPORT-TAU-AMPL");
        bp->new_line ();
        */
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_operation_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

        const gchar *operation_modes[] = {
                "MANUAL",
                "MEASURE DC_OFFSET",
                "RUN SCOPE",
                "INIT BRAM TRANSPORT", // 3
                "SINGLE SHOT",
                "STREAMING OPERATION", // 5 "START BRAM LOOP",
                "RUN TUNE",
                "RUN TUNE F",
                "RUN TUNE FF",
                "RUN TUNE RS",
                NULL };

        // Init choicelist
	for(int i=0; operation_modes[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), operation_modes[i], operation_modes[i]);

        update_op_widget = wid;
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), operation_mode=5); // "START BRAM LOOP" mode need to run for data decimation and transfer analog + McBSP

        // FPGA Update Period
	wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_update_ts_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);
        update_ts_widget = wid;

        /* for(i=0; i< 30; i=i+2) {printf ("\" %2d/%.2fms\"\n",i,1024*(1<<i)*2/125e6*1e3);}
 for(i=0; i<=30; i=i+1) {printf ("\" %2d/%.2fms\"\n",i,1024*(1<<i)*2/125e6*1e3);}
"  0/~16.38us"
"  1/~32.77us"
"  2/~65.54us"
"  3/~131.07us"
"  4/~262.14us"
"  5/~524.29us"
"  6/~1048.58us"
"  6/~1.05ms"
"  7/~2.10ms"
"  8/~4.19ms"
"  9/~8.39ms"
" 10/~16.78ms"
" 11/~33.55ms"
" 12/~67.11ms"
" 13/~134.22ms"
" 14/~268.44ms"
" 15/~536.87ms"
" 16/~1073.74ms"
" 16/~1.07s"
" 17/~2.15s"
" 18/~4.29s"
" 19/~8.59s"
" 20/~17.18s"
" 21/~34.36s"
" 22/~68.72s"
" 23/~137.44s"
" 24/~274.88s"
" 25/~549.76s"
" 26/~1099.51s"
" 27/~2199.02s"
" 28/~4398.05s"
" 29/~8796.09s"

        */
        
	const gchar *update_ts_list[] = {
                "* 1/ 32.8us  ( 32ns)",
                "  2/ 65.5us  ( 64ns)",
                "  3/131us    (128ns)",
                "  4/262.14us (256ns)",
                "  5/524.29us (512ns)",
                "  6/  1.05ms ( 1.024us)",
                "  7/  2.10ms ( 2.048us)",
                "  8/  4.19ms ( 4.096us)",
                "  9/  8.39ms ( 8.192us)",
                " 10/ 16.78ms (16.4us",
                " 11/ 33.55ms (32.8us)",
                " 12/ 67.11ms (65.5us)",
                " 13/134.22ms (131us)",
                " 14/268.44ms (262us)",
                " 15/536.87ms (524us)",
                " 16/  1.07s  ( 1.05ms)",
                " 17/  2.15s  ( 2.10ms)",
                " 18/  4.29s  ( 4.19ms)",
                " 19/  8.59s  ( 8.39ms)",
                " 20/ 17.18s  (16.8ms)",
                " 21/ 34.36s  (33.6ms)",
                " 22/ 68.72s  (67.1ms)",
                "DEC/SCOPE-LEN(BOXCARR)",
                NULL };
   
	// Init choicelist
	for(int i=0; update_ts_list[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), update_ts_list[i], update_ts_list[i]);

	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 18); // select 19

        // Scope Trigger Options
	wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_trigger_mode_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

	const gchar *trigger_modes[] = {
                "Trigger: None",
                "Auto CH1 Pos",
                "Auto CH1 Neg",
                "Auto CH2 Pos",
                "Auto CH2 Neg",
                "Logic B0 H",
                "Logic B0 L",
                "Logic B1 H",
                "Logic B1 L",
                "Logic B2 H",
                "Logic B2 L",
                "Logic B3 H",
                "Logic B3 L",
                "Logic B4 H",
                "Logic B4 L",
                "Logic B5 H",
                "Logic B5 L",
                "Logic B6 H",
                "Logic B6 L",
                "Logic B7 H",
                "Logic B7 L",
                NULL };
   
	// Init choicelist
	for(int i=0; trigger_modes[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), trigger_modes[i], trigger_modes[i]);

	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 1);
                
        // Scope Auto Set Modes
	wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_auto_set_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

	const gchar *auto_set_modes[] = {
                "Auto Set CH1",
                "Auto Set CH2",
                "Auto Set CH3",
                "Auto Set CH4",
                "Auto Set CH5",
                "Default All=1000mV",
                "Default All=100mV",
                "Default All=10mV",
                "Manual",
                NULL };
   
	// Init choicelist
	for(int i=0; auto_set_modes[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), auto_set_modes[i], auto_set_modes[i]);

	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 6);
        
        bp->new_line ();

        // BRAM TRANSPORT MODE BLOCK S1,S2
	wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_transport_ch12_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

	const gchar *transport_modes[] = {
                "OFF: no plot",
                "IN1, IN2",             // [0] SCOPE
                "IN1: AC, DC",          // [1] MON
                "AMC: Ampl, Exec",      // [2] AMC Adjust
                "PHC: dPhase, dFreq",   // [3] PHC Adjust
                "Phase, Ampl",          // [4] TUNE
                "Phase, dFreq,[Am,Ex]", // [5] SCAN
                "DHC: dFreq, dFControl", // [6] DFC Adjust
                "DDR IN1/IN2",          // [7] SCOPE with DEC=1,SHR=0 Double(max) Data Rate config
                "AXIS7/8",             // [8]
                NULL };
   
	// Init choicelist
	for(int i=0; transport_modes[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), transport_modes[i], transport_modes[i]);

        channel_selections[5] = 0;
        channel_selections[6] = 0;
        channel_selections[0] = 1;
        channel_selections[1] = 1;
        transport=5; // 5: Phase, dFreq,[Ampl,Exec] (default for streaming operation, scanning, etc.)
        update_tr_widget = wid;
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), transport+1); // normal operation for PLL, transfer: Phase, Freq,[Am,Ex] (analog: Freq, McBSP: 4ch transfer Ph, Frq, Am, Ex)

// *** PAC_PLL::GPIO MONITORS ***                                                                    readings are via void *thread_gpio_reading_FIR(g), read_gpio_reg_int32 (n,m)
// *** DBG ***                                                                                                //        gpio_reading_FIRV_vector[GPIO_READING_LMS_A]        (1,0); // GPIO X1 : LMS A
// *** DBG ***                                                                                                //        gpio_reading_FIRV_vector[GPIO_READING_LMS_A]        (1,1); // GPIO X2 : LMS B
// *** DBG ***                                                                                                //        -----------------------                             (2,0); // GPIO X3 : DBG M
//oubleParameter VOLUME_MONITOR("VOLUME_MONITOR", CBaseParameter::RW, 0, 0, -1000.0, 1000.0);                 // mV  ** gpio_reading_FIRV_vector[GPIO_READING_AMPL]         (2,1); // GPIO X4 : CORDIC SQRT (AM2=A^2+B^2)
// via  DC_OFFSET rp_PAC_auto_dc_offset_correct ()                                                                                                                          (3,0); // GPIO X5 : DC_OFFSET (M-DC)
// *** DBG ***                                                                                                //        -----------------------                             (3,1); // GPIO X6 : --- SPMC STATUS [FB, GVP, AD5791, --]
//oubleParameter EXEC_MONITOR("EXEC_MONITOR", CBaseParameter::RW, 0, 0, -1000.0, 1000.0);                     //  mV ** gpio_reading_FIRV_vector[GPIO_READING_EXEC]         (4,0); // GPIO X7 : Exec Ampl Control Signal (signed)
//oubleParameter DDS_FREQ_MONITOR("DDS_FREQ_MONITOR", CBaseParameter::RW, 0, 0, 0.0, 25e6);                   //  Hz ** gpio_reading_FIRV_vector[GPIO_READING_DDS_FREQ]     (4,1); // GPIO X8 : DDS Phase Inc (Freq.) upper 32 bits of 44 (unsigned)
//                                                                                                              //                                                            (5,0); // GPIO X9 : DDS Phase Inc (Freq.) lower 32 bits of 44 (unsigned)
//oubleParameter PHASE_MONITOR("PHASE_MONITOR", CBaseParameter::RW, 0, 0, -180.0, 180.0);                     // deg ** gpio_reading_FIRV_vector[GPIO_READING_PHASE]        (5,1); // GPIO X10: CORDIC ATAN(X/Y)
//oubleParameter DFREQ_MONITOR("DFREQ_MONITOR", CBaseParameter::RW, 0, 0, -1000.0, 1000.0);                   // Hz  ** gpio_reading_FIRV_vector[GPIO_READING_DDS_FREQ]     (6,0); // GPIO X11: dFreq
// *** DBG ***                                                                                                //        -----------------------                             (6,1); // GPIO X12: ---
//oubleParameter CONTROL_DFREQ_MONITOR("CONTROL_DFREQ_MONITOR", CBaseParameter::RW, 0, 0, -10000.0, 10000.0); // mV  **  gpio_re..._FIRV_vector[GPIO_READING_CONTROL_DFREQ] (7,0); // GPIO X13: control dFreq value
// *** DBG ***                                                                                                //        -----------------------                             (7,1); // GPIO X14: --- SIGNAL PASS [IN2] (Current, FB SRC)
// *** DBG ***                                                                                                //        -----------------------                             (8,0); // GPIO X15: --- UMON
// *** DBG ***                                                                                                //        -----------------------                             (8,1); // GPIO X16: --- XMON
// *** DBG ***                                                                                                //        -----------------------                             (9,0); // GPIO X17: --- YMON
// *** DBG ***                                                                                                //        -----------------------                             (9,1); // GPIO X18: --- ZMON
// *** DBG ***                                                                                                //        -----------------------                            (10,0); // GPIO X19: ---
// *** DBG ***                                                                                                //        -----------------------                            (10,1); // GPIO X20: ---
        
        // GPIO monitor selections -- full set, experimental
	const gchar *monitor_modes_gpio[] = {
                "OFF: no plot",
                "X1 LMS A",
                "X2 LMS B",
                "X3 DBG M",
                "X4 SQRT AM2",
                "X5 DC OFFSET",
                "X6 SPMC STATUS FB,GVP, AD",
                "X7 EXEC AM",
                "X8 DDS PH INC upper",
                "X9 DDS PH INC lower",
                "X10 ATAN(X/Y)",
                "X11 dFreq",
                "X12 DBG --",
                "X13 Ctrl dFreq",
                "X14 Signal Pass IN2",
                "X15 UMON",
                "X16 XMON",
                "X17 YMON",
                "X18 ZMON",
                "X19 ===",
                "X20 ===",
                NULL };

        // CH3 from GPIO MONITOR</p>
	wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_transport_ch3_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

	// Init choicelist
	for(int i=0; monitor_modes_gpio[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), monitor_modes_gpio[i], monitor_modes_gpio[i]);

        channel_selections[2] = 0;
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 0);

        // CH4 from GPIO MONITOR</p>
	wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_transport_ch4_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

	// Init choicelist
	for(int i=0; monitor_modes_gpio[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), monitor_modes_gpio[i], monitor_modes_gpio[i]);

        channel_selections[3] = 0;
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 0);

        // CH5 from GPIO MONITOR</p>
	wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_transport_ch5_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

	// Init choicelist
	for(int i=0; monitor_modes_gpio[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), monitor_modes_gpio[i], monitor_modes_gpio[i]);

        channel_selections[4] = 0;
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 0);


        // Scope Display
        bp->new_line ();

	GtkWidget* scrollarea = gtk_scrolled_window_new ();
        gtk_widget_set_hexpand (scrollarea, TRUE);
        gtk_widget_set_vexpand (scrollarea, TRUE);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollarea),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        bp->grid_add_widget (scrollarea, 10);
        signal_graph_area = gtk_drawing_area_new ();
        
        gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (signal_graph_area), 128);
        gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (signal_graph_area), 4);
        gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (signal_graph_area), graph_draw_function, this, NULL);
        //        bp->grid_add_widget (signal_graph_area, 10);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrollarea), signal_graph_area);
        
        bp->new_line ();

        // Scope Cotnrols
        bp->grid_add_check_button ( N_("Ch1: AC"), "Remove Offset from Ch1", 1,
                                    G_CALLBACK (RPspmc_pacpll::scope_ac_ch1_callback), this);
        wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::scope_z_ch1_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);
        const gchar *zoom_levels[] = {
                                      "0.001",
                                      "0.003",
                                      "0.01",
                                      "0.03",
                                      "0.1",
                                      "0.3",
                                      "1",
                                      "3",
                                      "10",
                                      "30",
                                      "100",
                                      "300",
                                      "1000",
                                      "3000",
                                      "10k",
                                      "30k",
                                      "100k",
                                      NULL };
	for(int i=0; zoom_levels[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid),  zoom_levels[i], zoom_levels[i]);
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 6);


        bp->grid_add_check_button ( N_("Ch2: AC"), "Remove Offset from Ch2", 1,
                                    G_CALLBACK (RPspmc_pacpll::scope_ac_ch2_callback), this);

        wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::scope_z_ch2_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

	for(int i=0; zoom_levels[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid),  zoom_levels[i], zoom_levels[i]);
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 3);


        bp->grid_add_check_button ( N_("Ch3 AC"), "Remove Offset from Ch3", 1,
                                    G_CALLBACK (RPspmc_pacpll::scope_ac_ch3_callback), this);
        bp->grid_add_check_button ( N_("XY"), "display XY plot for In1, In2", 1,
                                    G_CALLBACK (RPspmc_pacpll::scope_xy_on_callback), this);
        bp->grid_add_check_button ( N_("FFT"), "display FFT plot for In1, In2", 1,
                                    G_CALLBACK (RPspmc_pacpll::scope_fft_on_callback), this);
        
        wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::scope_fft_time_zoom_callback),
                          this);
        bp->grid_add_widget (wid);
        const gchar *time_zoom_levels[] = {
                                      "1",
                                      "2",
                                      "5",
                                      "10",
                                      "20",
                                      "50",
                                      "100",
                                      NULL };
	for(int i=0; time_zoom_levels[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid),  time_zoom_levels[i], time_zoom_levels[i]);
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 0);

        bp->new_line ();

        bp->grid_add_exec_button ( N_("Save"), G_CALLBACK (RPspmc_pacpll::scope_save_data_callback), this, "ScopeSaveData");

        bram_shift = 0;
        bp->grid_add_widget (gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, 4096, 10), 10);
        g_signal_connect (G_OBJECT (bp->any_widget), "value-changed",
                          G_CALLBACK (RPspmc_pacpll::scope_buffer_position_callback),
                          this);

        bp->new_line ();
        run_scope=0;
        bp->grid_add_check_button ( N_("Scope"), "Enable Scope", 1,
                                    G_CALLBACK (RPspmc_pacpll::enable_scope), this);
        scope_width_points  = 512.;
        scope_height_points = 256.;
        bp->set_input_width_chars (5);
  	bp->grid_add_ec ("SW", Unity, &scope_width_points, 256.0, 4096.0, ".0f", 128, 256., "SCOPE-WIDTH");
        bp->set_input_width_chars (4);
  	bp->grid_add_ec ("SH", Unity, &scope_height_points, 128.0, 1024.0, ".0f", 128, 256., "SCOPE-HEIGHT");

        gtk_widget_hide (dF_control_frame);
        gtk_widget_hide (pulse_control_frame);

        // ========================================
        
        bp->pop_grid ();
        bp->new_line ();

        // save List away...
	//g_object_set_data( G_OBJECT (window), "RPSPMCONTROL_EC_list", EC_list);

	g_object_set_data( G_OBJECT (window), "RPSPMCONTROL_EC_READINGS_list", EC_R_list);
	g_object_set_data( G_OBJECT (CpyButton), "PAC_FP_list", EC_FP_list);

        set_window_geometry ("rpspmc-pacpll-control"); // needs rescoure entry and defines window menu entry as geometry is managed

	rpspmc_pacpll_hwi_pi.app->RemoteEntryList = g_slist_concat (rpspmc_pacpll_hwi_pi.app->RemoteEntryList, bp->remote_list_ec);

        
        // hookup to scan start and stop
        rpspmc_pacpll_hwi_pi.app->ConnectPluginToStartScanEvent (RPspmc_pacpll::scan_start_callback);
        rpspmc_pacpll_hwi_pi.app->ConnectPluginToStopScanEvent (RPspmc_pacpll::scan_stop_callback);


        // Setup Scope data hook
        remote_action_cb *ra = g_new( remote_action_cb, 1);     
        ra -> cmd = g_strdup_printf("GET_RPDATA_VECTOR"); 
        ra -> RemoteCb = (void (*)(GtkWidget*, void*))get_rpdata_vector;  
        ra -> widget = NULL;
        ra -> data = this;
        
        ra -> return_data    = bram_info;
        ra -> data_length    = 4096;
        ra -> data_vector[0] = &bram_saved_buffer[0][0];
        ra -> data_vector[1] = &bram_saved_buffer[1][0];
        ra -> data_vector[2] = &bram_saved_buffer[2][0];
        ra -> data_vector[3] = &bram_saved_buffer[3][0];
        ra -> data_vector[4] = &bram_saved_buffer[4][0];
        gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra );
        
}

RPspmc_pacpll::~RPspmc_pacpll (){

        update_shm_monitors (1); // close SHM

        delete mTime;
	delete uTime;
	delete Time;
	delete dB;
	delete VoltHz;
	delete Volt;
	delete mVolt;
	delete VoltDeg;
	delete Deg;
	delete Hz;
	delete Unity;
}

void RPspmc_pacpll::connect_cb (GtkWidget *widget, RPspmc_pacpll *self){
        // get connect type request (restart or reconnect button was clicked)
        self->reconnect_mode = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "RESTART"));
        // connect (checked) or dissconnect    
        self->json_talk_connect_cb (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)), self->reconnect_mode);
}


void RPspmc_pacpll::scan_start_callback (gpointer user_data){
        g_message ("RPspmc_pacpll::scan_start_callback");

        if (rpspmc_hwi->is_scanning()){
                g_message ("RPspmc_pacpll::scan_start_callback ** RPSPMC scan in progress -- STOP FIRST TO RE-START SCAN.");
                return -1;
        }
        if (RPSPMC_ControlClass->check_vp_in_progress ()){
                g_message ("RPspmc_pacpll::scan_start_callback ** RPSPMC is streaming GVP data -- ABORT/STOP FIRST TO START SCAN.");
                return -1;
        }

        rpspmc_pacpll->ch_freq = -1;
        rpspmc_pacpll->ch_ampl = -1;
        rpspmc_pacpll->streaming = 1;

        rpspmc_hwi->set_spmc_signal_mux (RPSPMC_ControlClass->scan_source);

        if (RPSPMC_ControlClass){
                rpspmc_pacpll->set_stream_mux (RPSPMC_ControlClass->scan_source);
                RPSPMC_ControlClass->bp->scan_start_gui_actions ();
        }
        
}

void RPspmc_pacpll::scan_stop_callback (gpointer user_data){
        g_message ("RPspmc_pacpll::scan_stop_callback");

        if (RPSPMC_ControlClass){
                RPSPMC_ControlClass->bp->scan_end_gui_actions ();
        }

        if (! rpspmc_hwi->is_scanning()){
                g_message ("RPspmc_pacpll::scan_stop_callback ** RPSPMC is not scanning.");
                return -1;
        }

        rpspmc_pacpll->ch_freq = -1;
        rpspmc_pacpll->ch_ampl = -1;
        rpspmc_pacpll->streaming = 0;

}

int RPspmc_pacpll::setup_scan (int ch, 
				 const gchar *titleprefix, 
				 const gchar *name,
				 const gchar *unit,
				 const gchar *label,
				 double d2u
	){

        // extra setup -- not needed
	return 0;
}

void RPspmc_pacpll::pac_tau_transport_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("TRANSPORT_TAU_DFREQ", self->parameters.transport_tau[0]); // negative = disabled
        self->write_parameter ("TRANSPORT_TAU_PHASE", self->parameters.transport_tau[1]);
        self->write_parameter ("TRANSPORT_TAU_EXEC",  self->parameters.transport_tau[2]);
        self->write_parameter ("TRANSPORT_TAU_AMPL",  self->parameters.transport_tau[3]);
}

void RPspmc_pacpll::pac_tau_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("PAC_DCTAU", self->parameters.pac_dctau);
        self->write_parameter ("PACTAU", self->parameters.pactau);
        self->write_parameter ("PACATAU", self->parameters.pacatau);
}

void RPspmc_pacpll::pac_frequency_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("FREQUENCY_MANUAL", self->parameters.frequency_manual, "%.4f"); //, TRUE);
        self->write_parameter ("FREQUENCY_CENTER", self->parameters.frequency_center, "%.4f"); //, TRUE);
}

void RPspmc_pacpll::pac_volume_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("VOLUME_MANUAL", self->parameters.volume_manual);
}

static void freeze_ec(Gtk_EntryControl* ec, gpointer data){ ec->Freeze (); };
static void thaw_ec(Gtk_EntryControl* ec, gpointer data){ ec->Thaw (); };

void RPspmc_pacpll::show_dF_control (GtkWidget *widget, RPspmc_pacpll *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                gtk_widget_show (self->dF_control_frame);
        else
                gtk_widget_hide (self->dF_control_frame);
}

void RPspmc_pacpll::show_pulse_control (GtkWidget *widget, RPspmc_pacpll *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                gtk_widget_show (self->pulse_control_frame);
        else
                gtk_widget_hide (self->pulse_control_frame);
}

void RPspmc_pacpll::select_pac_lck_amplitude (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("LCK_AMPLITUDE", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
}
void RPspmc_pacpll::select_pac_lck_phase (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("LCK_PHASE", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
}

void RPspmc_pacpll::qcontrol (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("QCONTROL", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.qcontrol = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));

        if (self->parameters.qcontrol)
                g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "QC_SETTINGS_list"),
				(GFunc) thaw_ec, NULL);
        else
                g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "QC_SETTINGS_list"),
				(GFunc) freeze_ec, NULL);
}

void RPspmc_pacpll::qc_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("QC_GAIN", self->parameters.qc_gain);
        self->write_parameter ("QC_PHASE", self->parameters.qc_phase);
}

void RPspmc_pacpll::tune_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("TUNE_DFREQ", self->parameters.tune_dfreq);
        self->write_parameter ("TUNE_SPAN", self->parameters.tune_span);
}

void RPspmc_pacpll::amp_ctrl_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("AMPLITUDE_FB_SETPOINT", self->parameters.amplitude_fb_setpoint);
        self->write_parameter ("EXEC_FB_UPPER", self->parameters.exec_fb_upper);
        self->write_parameter ("EXEC_FB_LOWER", self->parameters.exec_fb_lower);
}

void RPspmc_pacpll::phase_ctrl_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("PHASE_FB_SETPOINT", self->parameters.phase_fb_setpoint);
        self->write_parameter ("FREQ_FB_UPPER", self->parameters.freq_fb_upper);
        self->write_parameter ("FREQ_FB_LOWER", self->parameters.freq_fb_lower);
        self->write_parameter ("PHASE_HOLD_AM_NOISE_LIMIT", self->parameters.phase_hold_am_noise_limit);
}

void RPspmc_pacpll::dfreq_ctrl_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("DFREQ_FB_SETPOINT", self->parameters.dfreq_fb_setpoint);
        self->write_parameter ("DFREQ_FB_UPPER", self->parameters.control_dfreq_fb_upper);
        self->write_parameter ("DFREQ_FB_LOWER", self->parameters.control_dfreq_fb_lower);
}

void RPspmc_pacpll::pulse_form_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("PULSE_FORM_BIAS0", self->parameters.pulse_form_bias0);
        self->write_parameter ("PULSE_FORM_BIAS1", self->parameters.pulse_form_bias1);
        self->write_parameter ("PULSE_FORM_PHASE0", self->parameters.pulse_form_phase0);
        self->write_parameter ("PULSE_FORM_PHASE1", self->parameters.pulse_form_phase1);
        self->write_parameter ("PULSE_FORM_WIDTH0", self->parameters.pulse_form_width0);
        self->write_parameter ("PULSE_FORM_WIDTH1", self->parameters.pulse_form_width1);
        self->write_parameter ("PULSE_FORM_WIDTH0IF", self->parameters.pulse_form_width0if);
        self->write_parameter ("PULSE_FORM_WIDTH1IF", self->parameters.pulse_form_width1if);
        self->write_parameter ("PULSE_FORM_SHAPEXW", self->parameters.pulse_form_shapexw);
        self->write_parameter ("PULSE_FORM_SHAPEXWIF", self->parameters.pulse_form_shapexwif);
        self->write_parameter ("PULSE_FORM_SHAPEX", self->parameters.pulse_form_shapex);
        self->write_parameter ("PULSE_FORM_SHAPEXIF", self->parameters.pulse_form_shapexif);

        self->write_parameter ("PULSE_FORM_DPOS0", self->parameters.pulse_form_dpt[0]);
        self->write_parameter ("PULSE_FORM_DPOS1", self->parameters.pulse_form_dpt[1]);
        self->write_parameter ("PULSE_FORM_DPOS2", self->parameters.pulse_form_dpt[2]);
        self->write_parameter ("PULSE_FORM_DPOS3", self->parameters.pulse_form_dpt[3]);
        
        if (self->parameters.pulse_form_enable){
                self->write_parameter ("PULSE_FORM_HEIGHT0", self->parameters.pulse_form_height0);
                self->write_parameter ("PULSE_FORM_HEIGHT1", self->parameters.pulse_form_height1);
                self->write_parameter ("PULSE_FORM_HEIGHT0IF", self->parameters.pulse_form_height0if);
                self->write_parameter ("PULSE_FORM_HEIGHT1IF", self->parameters.pulse_form_height1if);
                self->write_parameter ("PULSE_FORM_DPVAL0", self->parameters.pulse_form_dpv[0]);
                self->write_parameter ("PULSE_FORM_DPVAL1", self->parameters.pulse_form_dpv[1]);
                self->write_parameter ("PULSE_FORM_DPVAL2", self->parameters.pulse_form_dpv[2]);
                self->write_parameter ("PULSE_FORM_DPVAL3", self->parameters.pulse_form_dpv[3]);
        } else {
                self->write_parameter ("PULSE_FORM_HEIGHT0", 0.0);
                self->write_parameter ("PULSE_FORM_HEIGHT1", 0.0);
                self->write_parameter ("PULSE_FORM_HEIGHT0IF", 0.0);
                self->write_parameter ("PULSE_FORM_HEIGHT1IF", 0.0);
                self->write_parameter ("PULSE_FORM_DPVAL0", 0.0);
                self->write_parameter ("PULSE_FORM_DPVAL1", 0.0);
                self->write_parameter ("PULSE_FORM_DPVAL2", 0.0);
                self->write_parameter ("PULSE_FORM_DPVAL3", 0.0);
        }
}

void RPspmc_pacpll::pulse_form_enable (GtkWidget *widget, RPspmc_pacpll *self){
        self->parameters.pulse_form_enable = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
        self->pulse_form_parameter_changed (NULL, self);
}

void RPspmc_pacpll::pulse_form_fire (GtkWidget *widget, RPspmc_pacpll *self){
        gtk_combo_box_set_active (GTK_COMBO_BOX (self->update_op_widget), 3); // RESET (INIT BRAM TRANSPORT AND CLEAR FIR RING BUFFERS)
        while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, false);
        usleep(300000);
        gtk_combo_box_set_active (GTK_COMBO_BOX (self->update_op_widget), 4); // SINGLE SHOT
}

void RPspmc_pacpll::pulse_form_pf_ts (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("PULSE_FORM_TRIGGER_SELECT", gtk_combo_box_get_active (GTK_COMBO_BOX (widget)));
}

void RPspmc_pacpll::save_scope_data (){
        static int count=0;
        static int base_count=-1;
	std::ofstream f;
        int i;
	const gchar *separator = "\t";
	time_t t;
	time(&t);

        if (base_count != gapp->xsm->file_counter) // auto reset for new image
                count=0;
                
        base_count = gapp->xsm->file_counter;
        gchar *fntmp = g_strdup_printf ("%s/%s%03d-%s%04d.rpdata",
					g_settings_get_string (gapp->get_as_settings (), "auto-save-folder-probe"), 
					gapp->xsm->data.ui.basename, base_count, "RP", count++);
        
	f.open (fntmp);

        int ix=-999999, iy=-999999;

	if (gapp->xsm->MasterScan){
		gapp->xsm->MasterScan->World2Pixel (gapp->xsm->data.s.x0, gapp->xsm->data.s.y0, ix, iy, SCAN_COORD_ABSOLUTE);
	}

	const gchar *transport_modes[] = {
                "IN1, IN2",             // [0] SCOPE
                "IN1: AC, DC",          // [1] MON
                "AMC: Ampl, Exec",      // [2] AMC Adjust
                "PHC: dPhase, dFreq",   // [3] PHC Adjust
                "Phase, Ampl",          // [4] TUNE
                "Phase, dFreq,[Am,Ex]", // [5] SCAN
                "DHC: dFreq, dFControl", // [6] DFC Adjust
                "DDR IN1/IN2",          // [7] SCOPE with DEC=1,SHR=0 Double(max) Data Rate config
                "DEBUG McBSP",          // [8]
                NULL };

        
        // self->bram_window_length = 1024.*(double)decimation*1./125e6; // sec
        int decimation = 1 << data_shr_max;
        double tw1024 = bram_window_length;

        f.precision (12);

	f << "# view via: xmgrace -graph 0 -pexec 'title \"GXSM RP Data: " << fntmp << "\"' -block " << fntmp  << " -bxy 2:4 ..." << std::endl;
	f << "# GXSM RP Data :: RPVersion=00.01 vdate=20241114" << std::endl;
	f << "# Date                   :: date=" << ctime(&t) << "#" << std::endl;
	f << "# FileName               :: name=" << fntmp << std::endl;
	f << "# GXSM-Main-Offset       :: X0=" << gapp->xsm->data.s.x0 << " Ang" <<  "  Y0=" << gapp->xsm->data.s.y0 << " Ang" 
	  << ", iX0=" << ix << " Pix iX0=" << iy << " Pix"
	  << std::endl;

        f << "#C RP SAMPLING and DECIMATION SETTINGS:" << std::endl;
        f << "#C BRAM transfer window length is 1024 points, duration is tw=" << tw1024 << " s" << std::endl;
        f << "#C BRAM data length is 4096 points=" << (4*tw1024) << " s" << std::endl;
        f << "#C                    SHR_DEC_DATA=" << data_shr_max << std::endl;
        f << "#C            TRANSPORT_DECIMATION=" << decimation << std::endl;
        f << "#C         BRAM_SCOPE_TRIGGER_MODE=" << trigger_mode << std::endl;
        f << "#C SET_SINGLESHOT_TRGGER_POST_TIME=" << trigger_post_time << " us" << std::endl;
        f << "#C              BRAM_WINDOW_LENGTH=" << (1e3*bram_window_length)       << " ms" << std::endl;
        f << "#C                        BRAM_DEC=" << decimation << std::endl;
        f << "#C                   BRAM_DATA_SHR=" << data_shr_max << std::endl;
	f << "#C       CH1 CH2 TRANSPORT MAPPING=" << transport << " CH1,CH2 <= " << transport_modes[transport] << std::endl;
	f << "#C Data CH1, CH2 is raw data as of RP subsystem. For In1/In2: voltage in mV" << std::endl;
	f << "#C " << std::endl;

        double *signal[] = { pacpll_signals.signal_ch1, pacpll_signals.signal_ch2, pacpll_signals.signal_ch3, pacpll_signals.signal_ch4, pacpll_signals.signal_ch5, // 0...4 CH1..5
                             pacpll_signals.signal_phase, pacpll_signals.signal_ampl  }; // 5,6 PHASE, AMPL in Tune Mode, averaged from burst

        int uwait=600000;
        
        tw1024 /= 1024.; // per px in sec
        tw1024 *= 1e6;   // effective sample intervall in us between points
        f << "#C index time[us] " << transport_modes[transport] << std::endl;
        for (int k=0; k<4; ++k){
                write_parameter ("BRAM_SCOPE_SHIFT_POINTS", k*1024);
                usleep(uwait); // wait for data to update
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, false);

                for (i=0; i<1024; ++i){
                        bram_saved_buffer[0][i+k*1024] = ((i+k*1024)*tw1024);                   // time; buffer for later access via pyremote
                        bram_saved_buffer[1][i+k*1024] = pacpll_signals.signal_ch1[i]; // CH1; buffer for later access via pyremote
                        bram_saved_buffer[2][i+k*1024] = pacpll_signals.signal_ch2[i]; // CH2; buffer for later access via pyremote
                        f << (i+k*1024) << " \t"
                          << ((i+k*1024)*tw1024) << " \t"
                          << pacpll_signals.signal_ch1[(i+1)%1024] << " \t"
                          << pacpll_signals.signal_ch2[i] << "\n";
                }
        }
        f.close();

        write_parameter ("BRAM_SCOPE_SHIFT_POINTS", 0);
        
}

void RPspmc_pacpll::save_values (NcFile &ncf){
        // store all rpspmc_pacpll's control parameters for the RP PAC-PLL
        // if socket connection is up
        if (client){
                const gchar* pll_prefix = "rpspmc_hwi";
                // JSON READ BACK PARAMETERS! Loosing precison to float some where!! (PHASE_CI < -100dB => 0!!!)
                for (JSON_parameter *p=PACPLL_JSON_parameters; p->js_varname; ++p){
                        gchar *vn = g_strdup_printf ("%s_JSON_%s", pll_prefix, p->js_varname);
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", p->js_varname);
                        ncv.putAtt ("var_unit", p->unit_id);
                        ncv.putVar (p->value);
                }
                // ACTUAL PARAMETERS in "dB" DOUBLE PRECISION VALUES, getting OK to the FPGA...
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_PHASE_FB_CP");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_PHASE_FB_CP");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.phase_fb_cp); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_PHASE_FB_CI");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_PHASE_FB_CI");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.phase_fb_ci); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_AMPLITUDE_FB_CP");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_AMPLITUDE_FB_CP");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.amplitude_fb_cp); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_AMPLITUDE_FB_CI");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_AMPLITUDE_FB_CI");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.amplitude_fb_ci); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_PHASE_FB_CP_dB");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_PHASE_FB_CP_dB");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.phase_fb_cp_db); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_PHASE_FB_CI_dB");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_PHASE_FB_CI_dB");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.phase_fb_ci_db); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_AMPLITUDE_FB_CP_dB");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_AMPLITUDE_FB_CP_dB");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.amplitude_fb_cp_db); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_AMPLITUDE_FB_CI_dB");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_AMPLITUDE_FB_CI_dB");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.amplitude_fb_ci_db); // OK!
                }
        }
}

void RPspmc_pacpll::send_all_parameters (){
        write_parameter ("GAIN1", 1.);
        write_parameter ("GAIN2", 1.);
        write_parameter ("GAIN3", 1.);
        write_parameter ("GAIN4", 1.);
        write_parameter ("GAIN5", 1.);
        write_parameter ("SHR_DEC_DATA", 4.);
        write_parameter ("SCOPE_TRIGGER_MODE", 1);
        write_parameter ("PACVERBOSE", 0);
        write_parameter ("TRANSPORT_DECIMATION", 19);
        write_parameter ("TRANSPORT_MODE", transport);
        write_parameter ("OPERATION", operation_mode);
        write_parameter ("BRAM_SCOPE_SHIFT_POINTS", bram_shift = 0);
        pac_tau_parameter_changed (NULL, this);
        pac_frequency_parameter_changed (NULL, this);
        pac_volume_parameter_changed (NULL, this);
        qc_parameter_changed (NULL, this);
        tune_parameter_changed (NULL, this);
        amp_ctrl_parameter_changed (NULL, this);
        amplitude_gain_changed (NULL, this);
        phase_ctrl_parameter_changed (NULL, this);
        phase_gain_changed (NULL, this);
        dfreq_ctrl_parameter_changed (NULL, this);
        dfreq_gain_changed (NULL, this);
}

// AT COLD START AND RECONNECT
void RPspmc_pacpll::update_SPMC_parameters (){
        RPSPMC_ControlClass->Init_SPMC_on_connect ();
}


// ONLY at COLD START
void RPspmc_pacpll::SPMC_cold_start_init (){
        RPSPMC_ControlClass->Init_SPMC_after_cold_start ();
}


void RPspmc_pacpll::choice_operation_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->operation_mode = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->write_parameter ("OPERATION", self->operation_mode);
}

void RPspmc_pacpll::choice_update_ts_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->data_shr_max = 0;
        self->bram_window_length = 1.;

        int ts = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (ts < 11){
                self->write_parameter ("SIGNAL_PERIOD", 100);
                self->write_parameter ("PARAMETER_PERIOD",  100);
        } else {
                self->write_parameter ("SIGNAL_PERIOD", 200);
                self->write_parameter ("PARAMETER_PERIOD",  200);
        }
        
        self->data_shr_max = ts;

        int decimation = 1 << self->data_shr_max;
        self->write_parameter ("SHR_DEC_DATA", self->data_shr_max);
        self->write_parameter ("TRANSPORT_DECIMATION", decimation);

        self->bram_window_length = 1024.*(double)decimation*1./125e6; // sec
        //g_print ("SET_SINGLESHOT_TRGGER_POST_TIME = %g ms\n", 0.1 * 1e3*self->bram_window_length);
        //g_print ("BRAM_WINDOW_LENGTH ............ = %g ms\n", 1e3*self->bram_window_length);
        //g_print ("BRAM_DEC ...................... = %d\n", decimation);
        //g_print ("BRAM_DATA_SHR ................. = %d\n", self->data_shr_max);
        self->write_parameter ("SET_SINGLESHOT_TRIGGER_POST_TIME", 0.1 * 1e6*self->bram_window_length); // is us --  10% pre trigger
}

void RPspmc_pacpll::choice_transport_ch12_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->channel_selections[0] = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->channel_selections[1] = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) > 0){
                int i=gtk_combo_box_get_active (GTK_COMBO_BOX (widget))-1;
                if (i<0) i=0;
                self->write_parameter ("TRANSPORT_MODE", self->transport=i);
        }
}

void RPspmc_pacpll::choice_trigger_mode_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("BRAM_SCOPE_TRIGGER_MODE", self->trigger_mode = gtk_combo_box_get_active (GTK_COMBO_BOX (widget)));
        self->write_parameter ("BRAM_SCOPE_TRIGGER_POS", (int)(0.1*1024));
}

void RPspmc_pacpll::choice_auto_set_callback (GtkWidget *widget, RPspmc_pacpll *self){
        int m=gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (m>=0 && m<5)
                self->gain_scale[m] = -1.; // recalculate
        else {
                m -= 5;
                double s[] = { 1.0, 10., 100. };
                if (m < 3) 
                        for (int i=0; i<5; ++i)
                                self->gain_scale[i] = s[m]*1e-3; // Fixed
        }
}

void RPspmc_pacpll::set_stream_mux(int *mux_source_selections){
        guint64 mux = __GVP_selection_muxval (mux_source_selections);
        const gchar *SPMC_SET_SMUX_COMPONENTS[] = {
                "SPMC_GVP_STREAM_MUX_0",
                "SPMC_GVP_STREAM_MUX_1",
                "SPMC_GVP_STREAM_MUXH",
                NULL };
        int jdata_i[3];
        jdata_i[0] = (guint32)(mux&0xffffffff);
        jdata_i[1] = (guint32)(mux>>32);
        jdata_i[2] = __GVP_selection_muxHval (mux_source_selections);
        write_array (SPMC_SET_SMUX_COMPONENTS, 3, jdata_i,  0, NULL);
}




void RPspmc_pacpll::scope_buffer_position_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("BRAM_SCOPE_SHIFT_POINTS", self->bram_shift = (int)gtk_range_get_value (GTK_RANGE (widget)));
}

void RPspmc_pacpll::scope_save_data_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->save_scope_data ();
}


void RPspmc_pacpll::choice_transport_ch3_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->channel_selections[2] = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) > 0)
                self->write_parameter ("TRANSPORT_CH3", gtk_combo_box_get_active (GTK_COMBO_BOX (widget))-1);
}

void RPspmc_pacpll::choice_transport_ch4_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->channel_selections[3] = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) > 0)
                self->write_parameter ("TRANSPORT_CH4", gtk_combo_box_get_active (GTK_COMBO_BOX (widget))-1);
}

void RPspmc_pacpll::choice_transport_ch5_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->channel_selections[4] = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) > 0)
                self->write_parameter ("TRANSPORT_CH5", gtk_combo_box_get_active (GTK_COMBO_BOX (widget))-1);
}

void RPspmc_pacpll::set_ss_auto_trigger (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("SET_SINGLESHOT_TRANSPORT_TRIGGER", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
}

void RPspmc_pacpll::amplitude_gain_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->parameters.amplitude_fb_cp = self->parameters.amplitude_fb_invert * pow (10., self->parameters.amplitude_fb_cp_db/20.);
        self->parameters.amplitude_fb_ci = self->parameters.amplitude_fb_invert * pow (10., self->parameters.amplitude_fb_ci_db/20.);
        self->write_parameter ("AMPLITUDE_FB_CP", self->parameters.amplitude_fb_cp);
        self->write_parameter ("AMPLITUDE_FB_CI", self->parameters.amplitude_fb_ci);
}

void RPspmc_pacpll::amplitude_controller_invert (GtkWidget *widget, RPspmc_pacpll *self){
        self->parameters.amplitude_fb_invert = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)) ? -1.:1.;
        self->amplitude_gain_changed (NULL, self);
}

void RPspmc_pacpll::amplitude_controller (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("AMPLITUDE_CONTROLLER", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.amplitude_controller = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::phase_gain_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->parameters.phase_fb_cp = self->parameters.phase_fb_invert * pow (10., self->parameters.phase_fb_cp_db/20.);
        self->parameters.phase_fb_ci = self->parameters.phase_fb_invert * pow (10., self->parameters.phase_fb_ci_db/20.);
        // g_message("PH_CICP=%g, %g", self->parameters.phase_fb_ci, self->parameters.phase_fb_cp );
        self->write_parameter ("PHASE_FB_CP", self->parameters.phase_fb_cp);
        self->write_parameter ("PHASE_FB_CI", self->parameters.phase_fb_ci);
}

void RPspmc_pacpll::phase_controller_invert (GtkWidget *widget, RPspmc_pacpll *self){
        self->parameters.phase_fb_invert = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)) ? -1.:1.;
        self->phase_gain_changed (NULL, self);
}

void RPspmc_pacpll::phase_controller (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("PHASE_CONTROLLER", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.phase_controller = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::phase_unwrapping_always (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("PHASE_UNWRAPPING_ALWAYS", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.phase_unwrapping_always = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::phase_rot_ab (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("PAC_ROT_AB", gtk_combo_box_get_active (GTK_COMBO_BOX (widget)));
}

void RPspmc_pacpll::phase_unwrap_plot (GtkWidget *widget, RPspmc_pacpll *self){
        self->unwrap_phase_plot = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}


void RPspmc_pacpll::dfreq_gain_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->parameters.dfreq_fb_cp = self->parameters.dfreq_fb_invert * pow (10., self->parameters.dfreq_fb_cp_db/20.);
        self->parameters.dfreq_fb_ci = self->parameters.dfreq_fb_invert * pow (10., self->parameters.dfreq_fb_ci_db/20.);
        // g_message("DF_CICP=%g, %g", self->parameters.dfreq_fb_ci, self->parameters.dfreq_fb_ci );
        self->write_parameter ("DFREQ_FB_CP", self->parameters.dfreq_fb_cp);
        self->write_parameter ("DFREQ_FB_CI", self->parameters.dfreq_fb_ci);
}

void RPspmc_pacpll::dfreq_controller_invert (GtkWidget *widget, RPspmc_pacpll *self){
        self->parameters.dfreq_fb_invert = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)) ? -1.:1.;
        self->dfreq_gain_changed (NULL, self);
}

void RPspmc_pacpll::dfreq_controller (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("DFREQ_CONTROLLER", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.dfreq_controller = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::EnZdfreq_control (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("DFREQ_CONTROL_Z", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.Zdfreq_control = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::EnUdfreq_control (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("DFREQ_CONTROL_U", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.Udfreq_control = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::update(){
	if (G_IS_OBJECT (window))
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "RPSPMCONTROL_EC_list"),
				(GFunc) App::update_ec, NULL);
}

void RPspmc_pacpll::update_monitoring_parameters(){

        // RPSPMC-PACPLL
        
        // mirror global parameters to private
        parameters.dc_offset = pacpll_parameters.dc_offset;
        parameters.exec_amplitude_monitor = pacpll_parameters.exec_amplitude_monitor;
        parameters.dds_frequency_monitor = pacpll_parameters.dds_frequency_monitor;
        parameters.volume_monitor = pacpll_parameters.volume_monitor;
        parameters.phase_monitor = pacpll_parameters.phase_monitor;
        parameters.control_dfreq_monitor = pacpll_parameters.control_dfreq_monitor;
        parameters.dfreq_monitor = pacpll_parameters.dfreq_monitor;
        
        parameters.cpu_load = pacpll_parameters.cpu_load;
        parameters.free_ram = pacpll_parameters.free_ram;
        parameters.counter = pacpll_parameters.counter;

        pacpll_parameters.dds_dfreq_computed = parameters.dds_frequency_monitor - parameters.frequency_center;
        gchar *delta_freq_info = g_strdup_printf ("[%g]", pacpll_parameters.dds_dfreq_computed);

        
        input_ddsfreq->set_info (delta_freq_info);
        g_free (delta_freq_info);
        if (G_IS_OBJECT (window))
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "RPSPMCONTROL_EC_READINGS_list"),
				(GFunc) App::update_ec, NULL);

}

void RPspmc_pacpll::copy_f0_callback (GtkWidget *widget, RPspmc_pacpll *self){
        if (self->resonator_frequency_fitted > 0){
                self->parameters.frequency_center = self->parameters.frequency_manual = self->resonator_frequency_fitted;
                self->parameters.phase_fb_setpoint = self->resonator_phase_fitted;
                g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "PAC_FP_list"),
				(GFunc) App::update_ec, NULL);
        }
}

void RPspmc_pacpll::enable_scope (GtkWidget *widget, RPspmc_pacpll *self){
        self->run_scope = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::dbg_l1 (GtkWidget *widget, RPspmc_pacpll *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                self->debug_level |= 1;
        else
                self->debug_level &= ~1;
}
void RPspmc_pacpll::dbg_l2 (GtkWidget *widget, RPspmc_pacpll *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                self->debug_level |= 2;
        else
                self->debug_level &= ~2;
}
void RPspmc_pacpll::dbg_l4 (GtkWidget *widget, RPspmc_pacpll *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                self->debug_level |= 4;
        else
                self->debug_level &= ~4;
}

void RPspmc_pacpll::scan_gvp_opt6 (GtkWidget *widget, RPspmc_pacpll *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                self->scan_gvp_options |= (1<<6);
        else
                self->scan_gvp_options &= ~(1<<6);
}

void RPspmc_pacpll::scan_gvp_opt7 (GtkWidget *widget, RPspmc_pacpll *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                self->scan_gvp_options |= (1<<7);
        else
                self->scan_gvp_options &= ~(1<<7);
}


void RPspmc_pacpll::scope_ac_ch1_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->scope_ac[0] = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}
void RPspmc_pacpll::scope_ac_ch2_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->scope_ac[1] = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}
void RPspmc_pacpll::scope_ac_ch3_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->scope_ac[2] = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::scope_xy_on_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->scope_xy_on = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::scope_fft_on_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->scope_fft_on = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}
void RPspmc_pacpll::scope_z_ch1_callback (GtkWidget *widget, RPspmc_pacpll *self){
        double z[] = { 0.001, 0.003, 0.01, 0.03, 0.1, 0.3, 1.0, 3.0, 10., 30.0, 100., 300.0, 1000., 3000.0, 10e3, 30e3, 100e3, 1., 0., 0. };
        int i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->scope_z[0] = z[i];
}
void RPspmc_pacpll::scope_z_ch2_callback (GtkWidget *widget, RPspmc_pacpll *self){
        double z[] = { 0.001, 0.003, 0.01, 0.03, 0.1, 0.3, 1.0, 3.0, 10., 30.0, 100., 300.0, 1000., 3000.0, 10e3, 30e3, 100e3, 1., 0., 0. };
        int i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->scope_z[1] = z[i];
}
void RPspmc_pacpll::scope_fft_time_zoom_callback (GtkWidget *widget, RPspmc_pacpll *self){
        double z[] = { 1.0, 2.0, 5.0, 10.0, 20.0, 50.0, 100., 1., 0., 0. };
        int i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->scope_fft_time_zoom = z[i];
}



void RPspmc_pacpll::update_health (const gchar *msg){

        static GDateTime *now = NULL;
        static double t0_utc;
        static double t0 = -1.;
        static double t0_avg = 0.;
        static int init_count = 2500;
        double deviation=0.;
        int valid=0;
        static double dev_med1k=0.;
        static gsl_rstat_workspace *rstat_p = gsl_rstat_alloc();

        /*
        gsl_rstat_add(data[i], rstat_p);
        mean = gsl_rstat_mean(rstat_p);
        variance = gsl_rstat_variance(rstat_p);
        largest  = gsl_rstat_max(rstat_p);
        smallest = gsl_rstat_min(rstat_p);
        median   = gsl_rstat_median(rstat_p);
        sd       = gsl_rstat_sd(rstat_p);
        sd_mean  = gsl_rstat_sd_mean(rstat_p);
        skew     = gsl_rstat_skew(rstat_p);
        rms      = gsl_rstat_rms(rstat_p);
        kurtosis = gsl_rstat_kurtosis(rstat_p);
        n        = gsl_rstat_n(rstat_p);
        gsl_rstat_reset(rstat_p);
        n = gsl_rstat_n(rstat_p);
        gsl_rstat_free(rstat_p);
        */
        
        if (msg){
                gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY((red_pitaya_health)))), msg, -1);
                g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (window), "EC_FPGA_SPMC_server_settings_list"), (GFunc) App::update_ec, NULL); // UPDATE GUI!
                gtk_widget_set_name (GTK_WIDGET (red_pitaya_health), "entry-mono-text-msg");
        } else {
                double sec;
                double sec_dec = modf(spmc_parameters.uptime_seconds, &sec);
                int S = (int)sec;
                int tmp=3600*24;
                int d = S/tmp;
                int h = (tmp=(S-d*tmp))/3600;
                int m = (tmp=(tmp-h*3600))/60;
                double s = (tmp-m*60) + sec_dec;

                if (t0 < 0.){
                        GDateTime *now = g_date_time_new_now_utc();
                        t0_avg = spmc_parameters.uptime_seconds; // init to first reading
                        if (t0_avg > 10.)
                                t0 = t0_avg; // accept
                        t0_utc = g_date_time_to_unix(now) + (double)g_date_time_get_microsecond(now) * 1e-6;
                        g_date_time_unref(now);
                        g_message ("t0_avg: %g,  t0_utc: %g", t0_avg, t0_utc);
                } else {
                        if (init_count){
                                GDateTime *now = g_date_time_new_now_utc();
                                double utc = g_date_time_to_unix(now) + (double)g_date_time_get_microsecond(now) * 1e-6;
                                g_date_time_unref(now);
                                double estimate = spmc_parameters.uptime_seconds - (utc - t0_utc);
                                t0_avg = 0.99 * t0_avg + 0.01 * estimate;

                                gsl_rstat_add(estimate, rstat_p);
#if 0
                                g_message ("[%d] t0_avg: %g,  d_utc: %g   gsl mean: %g med: %g rms: %g  var: %g  skew: %g #%d",
                                           init_count, t0_avg, utc-t0_utc,
                                           gsl_rstat_mean(rstat_p),
                                           gsl_rstat_median(rstat_p),
                                           gsl_rstat_rms(rstat_p),
                                           gsl_rstat_variance(rstat_p),
                                           gsl_rstat_skew(rstat_p),
                                           gsl_rstat_n(rstat_p));
#endif
                                --init_count;
                                if (init_count == 0){
                                        t0_avg = gsl_rstat_median(rstat_p);
                                        gsl_rstat_reset(rstat_p);
                                }
                        }
                }

                
                //gsl_rstat_n(rstat_p));

                
                // Get the current date and time with microsecond precision.
                GDateTime *now = g_date_time_new_now_utc();
                double utc = g_date_time_to_unix(now) + (double)g_date_time_get_microsecond(now) * 1e-6;
                g_date_time_unref(now);
                double time_since_t0 = utc - t0_utc;
                double deviation = spmc_parameters.uptime_seconds - t0_avg - time_since_t0;

                if (init_count == 0){
                        if (gsl_rstat_n(rstat_p) > 3000){
                                dev_med1k=gsl_rstat_median(rstat_p); valid++;
                                gsl_rstat_reset(rstat_p);
                                gsl_rstat_add (dev_med1k, rstat_p);
                        }
                
                        gsl_rstat_add (deviation, rstat_p);
#if 0
                        g_message ("[%d] %g ** mean: %g med: %g rms: %g  var: %g  skew: %g",
                                   gsl_rstat_n(rstat_p), deviation,
                                   gsl_rstat_mean(rstat_p),
                                   gsl_rstat_median(rstat_p),
                                   gsl_rstat_rms(rstat_p),
                                   gsl_rstat_variance(rstat_p),
                                   gsl_rstat_skew(rstat_p));
#endif
                }
                
                // Extract the timestamp as a 64-bit integer representing microseconds since the epoch.
                //gint64 timestamp_us = g_date_time_to_unix(now) * 1000000 + g_date_time_get_microsecond(now);
                //gint64 t0_us = (gint64)round(t0*1e6); // sec to us
                        
                // Print the timestamp.
                //g_print("Timestamp (microseconds since epoch): %" G_GINT64_FORMAT "\n", timestamp_us);
                        
                // Format the time as a string.
                //gchar *formatted_time = g_date_time_format(now, "%Y-%m-%d %H:%M:%S.%f %z");
                //g_print("Formatted time: %s\n", formatted_time);
                        
                // Clean up.
                //g_free(formatted_time);
                //g_date_time_unref(now);

                gchar *health_string = g_strdup_printf ("CPU: %03.0f%% Free: %6.1f MB #%06.1f Up:%d d %02d:%02d:%04.1f   %s: %g ppm  [%.4f s as of %d]",
                                                        pacpll_parameters.cpu_load,
                                                        pacpll_parameters.free_ram/1024/1024,
                                                        pacpll_parameters.counter,
                                                        d,h,m,s, init_count ? "estimating precise t0":"delta UTC", 1e6*dev_med1k/time_since_t0,
                                                        valid ? dev_med1k : gsl_rstat_median(rstat_p), valid ? 1000 : gsl_rstat_n(rstat_p)
                                                        );
                if (valid){
                        main_get_gapp()->monitorcontrol->LogEvent ("RPSPMC-HS-UpTime-PPM", health_string, 2);
                        valid = 0;
                }

                gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY((red_pitaya_health)))), health_string, -1);
                gtk_widget_set_name (GTK_WIDGET (red_pitaya_health), "entry-mono-text-health");
                g_free (health_string);
        }
}

void RPspmc_pacpll::status_append (const gchar *msg){
	GtkTextBuffer *console_buf;
	GString *output;
	GtkTextMark *end_mark;
        GtkTextIter iter, start_iter, end_trim_iter, end_iter;
        gint lines, max_lines=400;

	if (msg == NULL) {
                //status_append ("** Zero Data/Message **");
                /* Change default font and color throughout the widget */
                GtkCssProvider *provider = gtk_css_provider_new ();
                gtk_css_provider_load_from_data (provider,
                                                 "textview {"
                                                 " font: 12px monospace;"
                                                 // "  color: green;"
                                                 "}",
                                                 -1);
                GtkStyleContext *context = gtk_widget_get_style_context (GTK_WIDGET (text_status));
                gtk_style_context_add_provider (context,
                                                GTK_STYLE_PROVIDER (provider),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
                

                // clear text buffer
                // read string which contain last command output
                console_buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW(text_status));
                gtk_text_buffer_get_bounds (console_buf, &start_iter, &end_iter);
                
                gtk_text_buffer_get_start_iter (console_buf, &start_iter);
                lines = gtk_text_buffer_get_line_count (console_buf);
                if (lines > 80){
                        g_message ("Clear socket log %d lines. <%s>", lines, msg);
                        gtk_text_buffer_get_iter_at_line_index (console_buf, &end_trim_iter, lines-10, 0);
                        gtk_text_buffer_delete (console_buf,  &start_iter,  &end_trim_iter);
                }

                gtk_text_buffer_get_end_iter (console_buf, &iter);
                gtk_text_buffer_create_mark (console_buf, "scroll", &iter, TRUE);
                
                return;
	}

	// read string which contain last command output
	console_buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW(text_status));
	gtk_text_buffer_get_bounds (console_buf, &start_iter, &end_iter);

	// get output widget content
	output = g_string_new (gtk_text_buffer_get_text (console_buf,
                                                         &start_iter, &end_iter,
                                                         FALSE));

        // insert text and keep view at end
        gtk_text_buffer_get_end_iter (console_buf, &iter); // get end
        gtk_text_buffer_insert (console_buf, &iter, msg, -1); // insert at end
        gtk_text_iter_set_line_offset (&iter, 0); // do not scroll horizontal
        end_mark = gtk_text_buffer_get_mark (console_buf, "scroll");
        gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (text_status), end_mark);

        // purge top
        gtk_text_buffer_get_start_iter (console_buf, &start_iter);
        lines = gtk_text_buffer_get_line_count (console_buf);
        if (lines > (max_lines)){
                gtk_text_buffer_get_iter_at_line_index (console_buf, &end_trim_iter, lines-max_lines, 0);
                gtk_text_buffer_delete (console_buf,  &start_iter,  &end_trim_iter);
        }

        gtk_text_iter_set_line_offset (&iter, 0); // do not scroll horizontal
        end_mark = gtk_text_buffer_get_mark (console_buf, "scroll");
        gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (text_status), end_mark);

#if 1
        // scroll to end
        gtk_text_buffer_get_end_iter (console_buf, &end_iter);
        end_mark = gtk_text_buffer_create_mark (console_buf, "cursor", &end_iter,
                                                FALSE);
        g_object_ref (end_mark);
        gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (text_status),
                                      end_mark, 0.0, FALSE, 0.0, 0.0);
        g_object_unref (end_mark);
#endif
        main_get_gapp()->monitorcontrol->LogEvent ("RPSPMC Stream Info", msg, 1); // add to main logfile with level 1       

}

void RPspmc_pacpll::on_connect_actions(){
        status_append ("3. RedPitaya SPM Control, PAC-PLL loading configuration:\n");

        // reconnect_mode: 0 = RECONNECT attempt to runnign FPGA, may fail if not running!
        //                 1 = COLD START/FPGA RELOAD=FULL RESET

        if (reconnect_mode){ // only of cold start
                send_all_parameters (); // PAC-PLL
        } else {
                send_all_parameters (); // PAC-PLL -- always update
        }
        
        while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

        status_append (" * RedPitaya SPM Control, PAC-PLL init, DEC FAST (12)...\n");
        gtk_combo_box_set_active (GTK_COMBO_BOX (update_tr_widget), 6);  // select Ph,dF,Am,Ex
        gtk_combo_box_set_active (GTK_COMBO_BOX (update_ts_widget), 12); // select 12, fast
        for (int i=0; i<25; ++i){
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
                usleep(20000);
        }
        
        gtk_combo_box_set_active (GTK_COMBO_BOX (update_op_widget), 3); // INIT BRAM TRANSPORT AND CLEAR FIR RING BUFFERS, give me a second...
        status_append (" * RedPitaya SPM Control, PAC-PLL init, INIT-FIR... [2s Zzzz]\n");
        for (int i=0; i<100; ++i){
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
                usleep(20000);
        }
        
        status_append (" * RedPitaya SPM Control, PAC-PLL init, INIT-FIR completed.\n");
        for (int i=0; i<50; ++i){
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
                usleep(20000);
        }
        
        status_append (" * RedPitaya SPM Control, PAC-PLL normal operation, set to data streaming mode.\n");
        for (int i=0; i<25; ++i){
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
                usleep(20000);
        }
        
        gtk_combo_box_set_active (GTK_COMBO_BOX (update_op_widget), 5); // STREAMING OPERATION
        gtk_combo_box_set_active (GTK_COMBO_BOX (update_ts_widget), 18); // select 19 (typical scan decimation/time scale filter)
        
        status_append (" * RedPitaya SPM Control: PAC-PLL is ready.\n");
        status_append (" * RedPitaya SPM Control, SPMC init...\n");

        rpspmc_hwi->info_append (NULL); // clear
        rpspmc_hwi->info_append ("RPSPMC+PACPALL is connected.");
 
        int i=0;
        
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Version.....: 0x%08x\n", (int)spmc_parameters.rpspmc_version); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC VDate.......: 0x%08x\n", (int)spmc_parameters.rpspmc_date); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC FPGAIMPL....: 0x%08x\n", (int)spmc_parameters.rpspmc_fpgaimpl); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC FPGAIMPL_D..: 0x%08x\n", (int)spmc_parameters.rpspmc_fpgaimpl_date); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC FPGA_STAUP..: 0x%08x\n", (int)spmc_parameters.rpspmc_fpgastartup); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC FPGA_RSC#...: 0x%08x\n", (int)spmc_parameters.rpspmc_fpgastartupcnt); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }

        if ((int)spmc_parameters.rpspmc_fpgaimpl != 0xec010099 ||
            (int)spmc_parameters.rpspmc_version < 0x00160000){
                g_warning ("INVALID RPSPMC server or wrong FPGA implementaion loaded. -- trying to continue, may fail/crash at any point from here.");
                { gchar *tmp = g_strdup_printf (" EE ERROR: RedPitaya SPMC Server or RPSPMC FPGA implementation invalid.\n *\n"); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        } else {
                { gchar *tmp = g_strdup_printf (" * RedPitaya SPMC Server and RPSPMC FPGA implementation accepted.\n *\n"); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        }
        
        /*
         * RedPitaya SPM RPSPMC Version.....: 0x00160000
         * RedPitaya SPM RPSPMC VDate.......: 0x20250406
         * RedPitaya SPM RPSPMC FPGAIMPL....: 0xec010099
         * RedPitaya SPM RPSPMC FPGAIMPL_D..: 0x20250328
         * RedPitaya SPM RPSPMC FPGA_STAUP..: 0x00000001
         * RedPitaya SPM RPSPMC FPGA_RSC#...: 0x00000001
         * RedPitaya SPM RPSPMC Z_SERVO_MODE: 0x00000000
         */

        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_SERVO_MODE: 0x%08x\n", i=(int)spmc_parameters.z_servo_mode); status_append (tmp); g_free (tmp); }        
        i &= MM_ON | MM_LOG | MM_FCZ | MM_RESET;
        RPSPMC_ControlClass->mix_transform_mode[0] = i;
        { gchar *tmp = g_strdup_printf ("%d", i & (MM_ON | MM_LOG | MM_FCZ)); gtk_combo_box_set_active_id (GTK_COMBO_BOX (RPSPMC_ControlClass->z_servo_options_selector[0]), tmp); g_free (tmp); }
        ;
        { gchar *tmp = g_strdup_printf (" *                                 ==> %s%s [%s]\n",
                                        i&MM_ON ? i&MM_LOG  ?"LOG":"LIN":"OFF",
                                        i&MM_FCZ && i&MM_ON ? "-FCZ":"",
                                        i&MM_RESET          ? "RESET":"NORMAL"); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }        

        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_POLARITY..: %s "
                                        "** Positive Polarity := Z piezo aka tip extends towards the surface with positive voltage.\n"
                                        " *                                        "
                                        "** But Gxsm and we want logically Z been positive for higher up, thus inverting internally all Z values.\n",
                                        ((int)spmc_parameters.gvp_status)&(1<<7) ? "NEG":"POS"); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }        

        // ((int)spmc_parameters.gvp_status)&(1<<7) BIT=1 (true) := invert Z on FPGA after slope to DAC-Z, this effects Z-monitor. FPGA internal Z is inverted vs GXSM convetion of pos Z = tip up
        spmc_parameters.gxsm_z_polarity = ((int)spmc_parameters.gvp_status)&(1<<7) ? 1:-1;
        int gxsm_preferences_polarity = xsmres.ScannerZPolarity ? 1 : -1; // 1: pos, 0: neg (bool) -- adjust zpos_ref accordingly!
                 
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_SERVO_SET.: %g Veq\n", spmc_parameters.z_servo_setpoint); status_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_SERVO_CP..: %g\n", spmc_parameters.z_servo_cp); status_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_SERVO_CI..: %g\n", spmc_parameters.z_servo_ci); status_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_SERVO_UPR.: %g V\n", spmc_parameters.z_servo_upper); status_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_SERVO_LOR.: %g V\n", spmc_parameters.z_servo_lower); status_append (tmp); g_free (tmp); }

        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_SERVO IN..: %08x MUX selection\n", i=(int)spmc_parameters.z_servo_src_mux); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        //i &= 3; // only bit 0,1,2
        { gchar *tmp = g_strdup_printf (" *                                 ==> %s\n", i==0? "IN2-RF": i==1?"IN3-AD4630-24A":i==2?"IN3-AD4630-24A-FIR":"EEE"); status_append (tmp); g_free (tmp); }        

        if (i >= 0 && i < 3)
                gtk_combo_box_set_active (GTK_COMBO_BOX (RPSPMC_ControlClass->z_servo_current_source_options_selector), i);
        
        guint64 mux=0;
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC GVP SRCS MUX: %016lx MUX selection code\n", mux=(((guint64)spmc_parameters.gvp_stream_mux_1) << 32) | (guint64)spmc_parameters.gvp_stream_mux_0); status_append (tmp); g_free (tmp); }
        for (int k=0; k<6; ++k){

                RPSPMC_ControlClass->probe_source[k] = __GVP_muxval_selection (mux, k);

                int pass=0;
                if (RPSPMC_ControlClass->probe_source[k] >= 0 && RPSPMC_ControlClass->probe_source[k] < 32){
                        if (rpspmc_swappable_signals[RPSPMC_ControlClass->probe_source[k]].label){
                                { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC GVP SRCS MUX[%d]: %02d <=> %s\n", k, RPSPMC_ControlClass->probe_source[k], rpspmc_swappable_signals[RPSPMC_ControlClass->probe_source[k]].label); status_append (tmp); g_free (tmp); }
                                gtk_combo_box_set_active (GTK_COMBO_BOX (RPSPMC_ControlClass->probe_source_signal_selector[k]), RPSPMC_ControlClass->probe_source[k]);
                                pass=1;
                        }
                }
                if (!pass) {
                        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC GVP SRCS MUX[%d]: %02d <=> FPGA DATA ERROR: INVALID SIGNAL\n", k, RPSPMC_ControlClass->probe_source[k]); status_append (tmp); g_free (tmp); }
                        RPSPMC_ControlClass->probe_source[k] = 0; // default to safety
                        gtk_combo_box_set_active (GTK_COMBO_BOX (RPSPMC_ControlClass->probe_source_signal_selector[k]), RPSPMC_ControlClass->probe_source[0]);
                }
        }

        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC X,Y,Z.......: %g %g %g V\n", spmc_parameters.x_monitor, spmc_parameters.y_monitor, spmc_parameters.z_monitor); status_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z0[z-slope].: %g V\n", spmc_parameters.z0_monitor); status_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC BIAS........: %g V\n", spmc_parameters.bias_monitor); status_append (tmp); g_free (tmp); }

        if (reconnect_mode){ // only if cold start
                status_append (" * RedPitaya SPM RPSPMC: Init completed and ackowldeged. Releasing normal control. Updating all from GXSM parameters.\n");
                write_parameter ("RPSPMC_INITITAL_TRANSFER_ACK", 99); // Acknoledge inital parameters received, release server parameter updates
                RPSPMC_ControlClass->update_FPGA_from_GUI ();
                update_SPMC_parameters ();
                SPMC_cold_start_init (); // ONLY at COLD START
        } else {
                // update GUI! (mission critical: Z-SERVO mainly)
                RPSPMC_ControlClass->update_GUI_from_FPGA ();
                status_append (" * RedPitaya SPM Control: RECONNECTING/READBACK Z-SERVO STATUS...\n");
                // ...
                update_SPMC_parameters (); // then send all other less critical parameters to make sure all is in sync

                //
                status_append (" * RedPitaya SPM RPSPMC: Readback completed and ackowldeged. Releasing normal control. No FPGA parameter update from GXSM!\n");
                write_parameter ("RPSPMC_INITITAL_TRANSFER_ACK", 99); // Acknoledge inital parameters received, release server parameter updates
        }

        
        //status_append (" * RedPitaya SPM Control ready. NEXT: Please Check Connect Stream.\n");
        status_append (" * RedPitaya SPM Control ready. Connecting SPMC Data Stream...\n");

        rpspmc_hwi->info_append (" * RPSPMC is READY *");
 
        gtk_check_button_set_active (GTK_CHECK_BUTTON (RPSPMC_ControlClass->stream_connect_button), true);

        gtk_button_set_child (GTK_BUTTON (RPSPMC_ControlClass->GVP_stop_all_zero_button), gtk_image_new_from_icon_name ("gxsm4-rp-icon"));

        gapp->check_events ();

        // VERIFY IF CORRECT, ASK TO ADJUST IF MISMATCH
        if ((-spmc_parameters.gxsm_z_polarity) != gxsm_preferences_polarity &&  gxsm_preferences_polarity < 0)
                if ( gapp->question_yes_no ("WARNING: Gxsm Preferences indicate NEGATIVE Z Polarity.\nPlease confirm to set Z-Polarity set to NEGATIVE.")){
                        rpspmc_pacpll->write_parameter ("SPMC_Z_POLARITY", -1); // inverted "pos" on hardware internal FPGA processing as of pos feedback behavior default
                        spmc_parameters.gxsm_z_polarity = 1; // inverted to flip for Gxsm default Z-up = pos
                        for (int i=0; i<10; ++i) { usleep (100000); gapp->check_events (); }
                        { gchar *tmp = g_strdup_printf (" *** Adjusted (+=>-) SPM RPSPMC Z_POLARITY..: %s\n", ((int)spmc_parameters.gvp_status)&(1<<7) ? "NEG":"POS"); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }        
                }

        gapp->check_events ();

        // most usual case w single Z piezo:
        if ((-spmc_parameters.gxsm_z_polarity) != gxsm_preferences_polarity &&  gxsm_preferences_polarity > 0)
                if ( gapp->question_yes_no ("WARNING: Gxsm Preferences indicate POSITIVE Z Polarity.\nPlease confirm to set Z-Polarity set to POSITIVE.")){
                        rpspmc_pacpll->write_parameter ("SPMC_Z_POLARITY", 1); // inverted "pos" on hardware internal FPGA processing as of pos feedback behavior default
                        spmc_parameters.gxsm_z_polarity = -1; // inverted to flip for Gxsm default Z-up = pos
                        for (int i=0; i<10; ++i) { usleep (100000); gapp->check_events (); }
                        { gchar *tmp = g_strdup_printf (" *** Adjusted (-=>+) SPM RPSPMC Z_POLARITY..: %s\n", ((int)spmc_parameters.gvp_status)&(1<<7) ? "NEG":"POS"); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }        
                }

        gapp->check_events ();
        
        if ((-spmc_parameters.gxsm_z_polarity) != gxsm_preferences_polarity){
                g_warning (" Z-Polarity was not changed. ");
        }
        
}



double AutoSkl(double dl){
  double dp=floor(log10(dl));
  if(dp!=0.)
    dl/=pow(10.,dp);
  if(dl>4) dl=5;
  else if(dl>1) dl=2;
  else dl=1;
  if(dp!=0.)
    dl*=pow(10.,dp);
  return dl;
}

double AutoNext(double x0, double dl){
  return(ceil(x0/dl)*dl);
}


#define FFT_LEN 1024
double blackman_nuttall_window (int i, double y){
        static gboolean init=true;
        static double w[FFT_LEN];
        if (init){
                for (int i=0; i<FFT_LEN; ++i){
                        double ipl = i*M_PI/FFT_LEN;
                        double x1 = 2*ipl;
                        double x2 = 4*ipl;
                        double x3 = 6*ipl;
                        w[i] = 0.3635819 - 0.4891775 * cos (x1) + 0.1365995 * cos (x2) - 0.0106411 * cos (x3);
                }
                init=false;
        }
        return w[i]*y;
}

gint compute_fft (gint len, double *data, double *psd, double mu=1.){
        static gint n=0;
        static double *in=NULL;
        //static double *out=NULL;
        static fftw_complex *out=NULL;
        static fftw_plan plan=NULL;

        if (n != len || !in || !out || !plan){
                if (plan){
                        fftw_destroy_plan (plan); plan=NULL;
                }
                // free temp data memory
                if (in) { delete[] in; in=NULL; }
                if (out) { delete[] out; out=NULL; }
                n=0;
                if (len < 2) // clenaup only, exit
                        return 0;

                n = len;
                // get memory for complex data
                in  = new double [n];
                out = new fftw_complex [n];
                //out = new double [n];

                // create plan for fft
                plan = fftw_plan_dft_r2c_1d (n, in, out, FFTW_ESTIMATE);
                //plan = fftw_plan_r2r_1d (n, in, out, FFTW_REDFT00, FFTW_ESTIMATE);
                if (plan == NULL)
                        return -1;
        }

                
        for (int i = 0; i < n; ++i){
                in[i] = blackman_nuttall_window (i, data[i]);
        }
                
        // compute transform
        fftw_execute (plan);

        double scale = M_PI*2/n;
        double mag=0.;
        for (int i=0; i<n/2; ++i){
                mag = scale * sqrt(c_re(out[i])*c_re(out[i]) + c_im(out[i])*c_im(out[i]));
                psd[i] = gfloat((1.-mu)*(double)psd[i] + mu*mag);
        }
        //g_print("FFT out Min: %g Max: %g\n",mi,mx);
        return 0;
}

double RPspmc_pacpll::unwrap (int k, double phi){
        static int pk=0;
        static int side=1;
        static double pphi;
        
#if 0
        if (!unwrap_phase_plot) return phi;

        if (k==0){ // choose start
                if (phi > 0.) side = 1; else side = -1;
                pk=k; pphi=phi;
                return (phi);
        }
        if (fabs(phi-pphi) > 180){
                switch (side){
                case 1: phi += 360;
                        pk=k; pphi=phi;
                        return (phi);
                case -1: phi -= 360;
                        pk=k; pphi=phi;
                        return (phi);
                }
        } else {
                if (phi > 0.) side = 1; else side = -1;
        }
        pk=k; pphi=phi;
#endif
        return (phi);
}

// to force udpate call:   gtk_widget_queue_draw (self->signal_graph_area);
void RPspmc_pacpll::graph_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                                       int             width,
                                                       int             height,
                                                       RPspmc_pacpll *self){
        self->dynamic_graph_draw_function (area, cr, width, height);
}

void RPspmc_pacpll::dynamic_graph_draw_function (GtkDrawingArea *area, cairo_t *cr, int width, int height){
        int n=1023;
        int h=(int)height; //scope_height_points;
        static int hist=0;
        static int current_width=0;
        if (!run_scope)
                h=10;
        scope_width_points=current_width = width;
        double xs = scope_width_points/1024.;
        double xcenter = scope_width_points/2;
        double xwidcenter = scope_width_points/2;
        double scope_width = scope_width_points;
        double x0 = xs*n/2;
        double yr = h/2;
        double y_hi  = yr*0.95;
        double dB_hi   =  0.0;
        double dB_mags =  4.0;
        static int rs_mode=0;
        const int binary_BITS = 16;
        
        if (!run_scope && hist == h)
                return;
        //if (current_width != (int)scope_width_points || h != hist){
                //current_width = (int)scope_width_points;
                //gtk_drawing_area_set_content_width (area, current_width);
                //gtk_drawing_area_set_content_height (area, h);
        //}
        hist=h;
        if (run_scope){
                cairo_translate (cr, 0., yr);
                cairo_scale (cr, 1., 1.);
                cairo_save (cr);
                cairo_item_rectangle *paper = new cairo_item_rectangle (0., -yr, scope_width_points, yr);
                paper->set_line_width (0.2);
                paper->set_stroke_rgba (CAIRO_COLOR_GREY1);
                paper->set_fill_rgba (CAIRO_COLOR_BLACK);
                paper->draw (cr);
                delete paper;
                //cairo_item_segments *grid = new cairo_item_segments (44);
                
                double avg=0.;
                double avg10=0.;
                char *valuestring;
                cairo_item_text *reading = new cairo_item_text ();
                reading->set_font_face_size ("Ununtu", 10.);
                reading->set_anchor (CAIRO_ANCHOR_W);
                cairo_item_path *wave = new cairo_item_path (n);
                cairo_item_path *binwave8bit[binary_BITS] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                wave->set_line_width (1.0);
                const gchar *ch_id[] = { "Ch1", "Ch2", "Ch3", "Ch4", "Ch5", "Phase", "Ampl" };
                CAIRO_BASIC_COLOR_IDS color[] = { CAIRO_COLOR_RED_ID, CAIRO_COLOR_GREEN_ID, CAIRO_COLOR_CYAN_ID, CAIRO_COLOR_YELLOW_ID,
                                                  CAIRO_COLOR_BLUE_ID, CAIRO_COLOR_RED_ID, CAIRO_COLOR_GREEN_ID, CAIRO_COLOR_FORESTGREEN_ID  };
                double *signal[] = { pacpll_signals.signal_ch1, pacpll_signals.signal_ch2, pacpll_signals.signal_ch3, pacpll_signals.signal_ch4, pacpll_signals.signal_ch5, // 0...4 CH1..5
                                     pacpll_signals.signal_phase, pacpll_signals.signal_ampl  }; // 5,6 PHASE, AMPL in Tune Mode, averaged from burst
                double x,xf,min,max,s,ydb,yph;

                const gchar *ch01_lab[2][10] = { {"IN1", "IN1-AC", "Ampl", "dPhase", "Phase", "Phase", "-", "-", "AX7", NULL },
                                                 {"IN2", "IN1-DC", "Exec", "dFreq ", "Ampl ", "dFreq", "-", "-", "AX8", NULL } }; 
                
                if (operation_mode >= 6 && operation_mode <= 9){
                        if (operation_mode == 9)
                                rs_mode=3;
                        operation_mode = 6; // TUNE
                        channel_selections[5]=5;
                        channel_selections[6]=6;
                } else {
                        rs_mode = 0;
                        channel_selections[5]=0;
                        channel_selections[6]=0;
                }
                // operation_mode = 6 : in TUNING CONFIGURATION, 7 total signals
               
                int ch_last=(operation_mode == 6) ? 7 : 5;
                int tic_db=0;
                int tic_deg=0;
                int tic_hz=0;
                int tic_lin=0;
                for (int ch=0; ch<ch_last; ++ch){
                        int part_i0=0;
                        int part_pos=1;

                        if (channel_selections[ch] == 0)
                                continue;

                        // Setup binary mode traces (DBG Ch)
                        if(channel_selections[0]==9){
                                if (!binwave8bit[0])
                                        for (int bit=0; bit<binary_BITS; ++bit)
                                                binwave8bit[bit] = new cairo_item_path (n);
                                for (int bit=0; bit<binary_BITS; ++bit){
                                        if (ch == 0)
                                                        binwave8bit[bit]->set_stroke_rgba (color[bit%4]);
                                        else
                                                        binwave8bit[bit]->set_stroke_rgba (color[4+(bit%4)]);
                                }
                        }
                        
                        if (operation_mode == 6 && (ch == 0 || ch == 1))
                                if (ch == 0)
                                        wave->set_stroke_rgba (1.,0.,0.,0.4);
                                else
                                        wave->set_stroke_rgba (0.,1.,0.,0.4);
                        else
                                wave->set_stroke_rgba (BasicColors[color[ch]]);

                        if (operation_mode == 6 && ch > 1)
                                wave->set_linemode (rs_mode);
                        else
                                wave->set_linemode (0);

                        min=max=signal[ch][512];
                        for (int k=0; k<n; ++k){
                                s=signal[ch][k];
                                if (s>max) max=s;
                                if (s<min) min=s;

                                if (scope_ac[ch])
                                        s -= scope_dc_level[ch]; // AC
                                
                                if (ch<2)
                                        if (scope_z[ch]>0.)
                                                s *= scope_z[ch]; // ZOOM

                                xf = xcenter + scope_width*pacpll_signals.signal_frq[k]/parameters.tune_span; // tune plot, freq x
                                x  = (operation_mode == 6 && ch > 1) ? xf : xs*k; // over freq or time plot

                                // MAPPING TREE ---------------------

                                // GPIO monitoring channels ch >= 2
                                if (operation_mode == 6) { // TUNING
                                        if ((ch == 6) || (ch == 1) || ( (ch > 1) && (ch < 5) && ((channel_selections[ch]==1) || (channel_selections[ch]==5)))){ // 1,5: GPIO Ampl
                                                // 0..-60dB range, 1mV:-60dB (center), 0dB:1000mV (top)
                                                wave->set_xy_fast (k, ch == 1 ? x:xf, ydb=db_to_y (dB_from_mV (s), dB_hi, y_hi, dB_mags)), tic_db=1;
                                        } else if ((ch == 5) || (ch == 0) || ( (ch > 1) && (ch < 5) && ((channel_selections[ch]==2) || (channel_selections[ch]==6)))){ // 2,6: GPIO Phase
                                                // [-180 .. +180 deg] x expand
                                                wave->set_xy_fast (k, ch == 0 ? x:xf, yph=deg_to_y (ch == 0 ? s:unwrap(k,s), y_hi)), tic_deg=1; // Phase for Tuning: with hi level unwrapping for plot
                                        } else if ((ch > 1) && (ch < 5) && channel_selections[ch] > 0){ // Linear, other GPIO
                                                wave->set_xy_fast (k, xf, -yr*(gain_scale[ch]>0.?gain_scale[ch]:1.)*s), tic_lin=1; // else: linear scale with gain
                                        } // else  --
                                } else { // SCOPE, OPERATION: over time / index
                                        if ((ch > 1) && (ch < 5)){ // GPIO mon...
                                                if (channel_selections[ch]==2 || channel_selections[ch]==6) // Phase
                                                        wave->set_xy_fast (k, unwrap (k,x), deg_to_y (s, y_hi)), tic_deg=1;
                                                else if (channel_selections[ch]==1 || channel_selections[ch]==5 || channel_selections[ch]==9) // Amplitude mode for GPIO monitoring
                                                        wave->set_xy_fast (k, x, db_to_y (dB_from_mV (s), dB_hi, y_hi, dB_mags)), tic_db=1;
                                                else
                                                        wave->set_xy_fast (k, x,-yr*(gain_scale[ch]>0.?gain_scale[ch]:1.)*s), tic_lin=1; // else: linear scale with gain

                                        } else if (   (ch == 0 && channel_selections[0]==3) // AMC:  Ampl,  Exec
                                                   || (ch == 1 && channel_selections[0]==3) // AMC:  Ampl,  Exec
                                                   || (ch == 1 && channel_selections[0]==5) // TUNE: Phase, Ampl
                                                      ){
                                                wave->set_xy_fast (k, x, db_to_y (dB_from_mV (s), dB_hi, y_hi, dB_mags)), tic_db=1; // Ampl, dB
                                                if (k>2 && s<0. && part_pos){  // dB for negative ampl -- change color!!
                                                        wave->set_stroke_rgba (CAIRO_COLOR_MAGENTA_ID);
                                                        wave->draw_partial (cr, part_i0, k-1); part_i0=k; part_pos=0;
                                                }
                                                if (k>2 && s>0. && !part_pos){
                                                        wave->set_stroke_rgba (BasicColors[color[ch]]);
                                                        wave->draw_partial (cr, part_i0, k-1); part_i0=k; part_pos=1;
                                                }

                                        } else if (   (ch == 0 && channel_selections[0]==5) // TUNE: Phase, Ampl -- with unwrapping
                                                   || (ch == 0 && channel_selections[0]==4) // PHC: dPhase, dFreq
                                                   || (ch == 0 && channel_selections[0]==6) // OP:   Phase, dFreq
                                                )
                                                wave->set_xy_fast (k, x, deg_to_y (s, y_hi)), tic_deg=1; // Phase direct

                                        else if (   (ch == 1 && channel_selections[0]==4) // PHC:dPhase, dFreq
                                                 || (ch == 1 && channel_selections[0]==6) // OP:  Phase, dFreq
                                                )
                                                    wave->set_xy_fast (k, x, freq_to_y (s, y_hi)), tic_hz=1; // Hz
                                        else if ( channel_selections[0]==9 && ch < 2){ // DBG AXIS7/8
                                                if (fabs (signal[ch][k]) > 10)
                                                        g_print("%04d C%01d: %g, %g * %g\n",k,ch,x,gain_scale[ch],signal[ch][k]);
                                                wave->set_xy_fast_clip_y (k, x,-yr*(gain_scale[ch]>0.?gain_scale[ch]:1.)*signal[ch][k], yr), tic_lin=1; // else: linear scale with gain
#if 0
                                                for (int bit=0; bit<binary_BITS; ++bit)
                                                        binwave8bit[bit]->set_xy_fast (k, x, binary_to_y (signal[ch][k], bit, ch, y_hi, binary_BITS));
                                                wave->set_xy_fast (k, x, -yr*((int)(s)%256)/256.);
#endif
                                        } else // SCOPE IN1,IN2, IN1 AC,DC
                                                wave->set_xy_fast (k, x,-yr*(gain_scale[ch]>0.?gain_scale[ch]:1.)*s), tic_lin=1; // else: linear scale with gain
                                }
                        }
                        if (s>0. && part_i0>0){
                                wave->set_stroke_rgba (BasicColors[color[ch]]);
                                wave->draw_partial (cr, part_i0, n-2);
                        }
                        if (s<0. && part_i0>0){
                                wave->set_stroke_rgba (CAIRO_COLOR_MAGENTA_ID);
                                wave->draw_partial (cr, part_i0, n-2);
                        }
                        if (channel_selections[ch] && part_i0==0){

                                wave->draw (cr);
                                
                                if (binwave8bit[0])
                                        for (int bit=0; bit<binary_BITS; ++bit)
                                                binwave8bit[bit]->draw (cr);
                        }

                        if (ch<6){
                                scope_dc_level[ch] = 0.5*(min+max);

                                if (gain_scale[ch] < 0.)
                                        if (scope_ac[ch]){
                                                min -= scope_dc_level[ch];
                                                max -= scope_dc_level[ch];
                                                gain_scale[ch] = 0.7 / (0.0001 + (fabs(max) > fabs(min) ? fabs(max) : fabs(min)));
                                        } else
                                                gain_scale[ch] = 0.7 / (0.0001 + (fabs(max) > fabs(min) ? fabs(max) : fabs(min)));
                        }
                                           
                        if (operation_mode != 6 && channel_selections[ch]){
                                avg=avg10=0.;
                                for (int i=1023-100; i<1023; ++i) avg+=signal[ch][i]; avg/=100.;
                                for (int i=1023-10; i<1023; ++i) avg10+=signal[ch][i]; avg10/=10.;
                                valuestring = g_strdup_printf ("%s %g %12.5f [x %g] %g %g {%g}",
                                                               ch < 2 ? ch01_lab[ch][channel_selections[ch]-1] : ch_id[ch],
                                                               avg10, avg, gain_scale[ch], min, max, max-min);
                                reading->set_stroke_rgba (BasicColors[color[ch]]);
                                reading->set_text (10, -(110-14*ch), valuestring);
                                g_free (valuestring);
                                reading->draw (cr);
                        }
                        
                }
                if (pacpll_parameters.bram_write_adr >= 0 && pacpll_parameters.bram_write_adr < 0x4000){
                        cairo_item_segments *cursors = new cairo_item_segments (2);
                        double icx = scope_width * (0.5*pacpll_parameters.bram_write_adr - bram_shift)/1024.;
                        cursors->set_line_width (2.5);
                        if (icx >= 0. && icx <= scope_width){
                                cursors->set_line_width (0.5);
                                cursors->set_stroke_rgba (CAIRO_COLOR_WHITE);
                        } else if (icx < 0)  cursors->set_stroke_rgba (CAIRO_COLOR_RED), icx=2;
                        else cursors->set_stroke_rgba (CAIRO_COLOR_RED), icx=scope_width-2;
                        cursors->set_xy_fast (0, icx, -80.);
                        cursors->set_xy_fast (1, icx,  80.);
                        cursors->draw (cr);
                        g_free (cursors);
                }

                if (debug_level&1){
                        valuestring = g_strdup_printf ("BRAM{A:0x%04X, #:%d, F:%d}",
                                                       (int)pacpll_parameters.bram_write_adr,
                                                       (int)pacpll_parameters.bram_sample_pos,
                                                       (int)pacpll_parameters.bram_finished);
                        reading->set_stroke_rgba (CAIRO_COLOR_WHITE);
                        reading->set_text (10, -(110-14*6), valuestring);
                        g_free (valuestring);
                        reading->draw (cr);
                }

                cairo_item_segments *cursors = new cairo_item_segments (2);
                cursors->set_line_width (0.3);

                // tick marks dB
                if (tic_db){
                        cursors->set_stroke_rgba (0.2,1.,0.2,0.5);
                        for (int db=(int)dB_hi; db >= -20*dB_mags; db -= 10){
                                double y;
                                valuestring = g_strdup_printf ("%4d dB", db);
                                reading->set_stroke_rgba (CAIRO_COLOR_GREEN);
                                reading->set_text (scope_width - 40,  y=db_to_y ((double)db, dB_hi, y_hi, dB_mags), valuestring);
                                g_free (valuestring);
                                reading->draw (cr);
                                cursors->set_xy_fast (0,xcenter+(scope_width)*(-0.5),y);
                                cursors->set_xy_fast (1,xcenter+(scope_width*0.7)*(0.5),y);
                                cursors->draw (cr);
                        }
                }
                // ticks deg
                if(tic_deg){
                        cursors->set_stroke_rgba (1.,0.2,0.2,0.5);
                        for (int deg=-180*deg_extend; deg <= 180*deg_extend; deg += 30*deg_extend){
                                double y;
                                valuestring = g_strdup_printf ("%d" UTF8_DEGREE, deg);
                                reading->set_stroke_rgba (CAIRO_COLOR_RED);
                                reading->set_text (scope_width- 80, y=deg_to_y (deg, y_hi), valuestring);
                                g_free (valuestring);
                                reading->draw (cr);
                                cursors->set_xy_fast (0,xcenter+(scope_width)*(-0.5),y);
                                cursors->set_xy_fast (1,xcenter+(scope_width*0.7)*(0.5),y);
                                cursors->draw (cr);
                        }
                }
                // ticks lin
                if(tic_lin){
                        cursors->set_stroke_rgba (0.8,0.8,0.8,0.5);
                        for (int yi=-100; yi <= 100; yi += 10){
                                double y; //-yr*(gain_scale[ch]>0.?gain_scale[ch]:1.)*s
                                valuestring = g_strdup_printf ("%d", yi);
                                reading->set_stroke_rgba (0.8,0.8,0.8,0.5);
                                reading->set_text (scope_width- 120, y=yi*0.01*y_hi, valuestring);
                                g_free (valuestring);
                                reading->draw (cr);
                                cursors->set_xy_fast (0,xcenter+(scope_width)*(-0.5),y);
                                cursors->set_xy_fast (1,xcenter+(scope_width*0.7)*(0.5),y);
                                cursors->draw (cr);
                        }
                }
                
                if(tic_hz){
                        cursors->set_stroke_rgba (0.8,0.8,0.0,0.5);
                        for (int yi=-10; yi <= 10; yi += 1){
                                double y; //-yr*(gain_scale[ch]>0.?gain_scale[ch]:1.)*s
                                valuestring = g_strdup_printf ("%d Hz", yi);
                                reading->set_stroke_rgba (0.8,0.8,0.0,0.5);
                                reading->set_text (scope_width- 120, y=yi*0.1*y_hi, valuestring);
                                g_free (valuestring);
                                reading->draw (cr);
                                cursors->set_xy_fast (0,xcenter+(scope_width)*(-0.5),y);
                                cursors->set_xy_fast (1,xcenter+(scope_width*0.7)*(0.5),y);
                                cursors->draw (cr);
                        }
                }
                
                if (operation_mode != 6){ // add time scale
                        double tm = bram_window_length;
                        double ts = tm < 10. ? 1. : 1e-3;
                        tm *= ts;
                        double dt = AutoSkl(tm/10.);
                        double t0 = -0.1*tm + tm*bram_shift/1024.;
                        double t1 = t0+tm;
                        cursors->set_stroke_rgba (0.8,0.8,0.8,0.4);
                        reading->set_anchor (CAIRO_ANCHOR_CENTER);
                        for (double t=AutoNext(t0, dt); t <= t1; t += dt){
                                double x=scope_width*(t-t0)/tm; // time position tic pos
                                valuestring = g_strdup_printf ("%g %s", t*1000, ts > 0.9 ? "ms":"s");
                                reading->set_stroke_rgba (CAIRO_COLOR_WHITE);
                                reading->set_text (x, yr-8, valuestring);
                                g_free (valuestring);
                                reading->draw (cr);
                                cursors->set_xy_fast (0,x,-yr);
                                cursors->set_xy_fast (1,x,yr);
                                cursors->draw (cr);
                        }
                        cursors->set_stroke_rgba (0.4,0.1,0.4,0.4);
                        double x=scope_width*0.1; // time position of post trigger (excl. soft delays)
                        cursors->set_xy_fast (0,x,-yr);
                        cursors->set_xy_fast (1,x,yr);
                        cursors->draw (cr);
                }

                cursors->set_line_width (0.5);
                cursors->set_stroke_rgba (CAIRO_COLOR_WHITE);
                // coord cross
                cursors->set_xy_fast (0,xcenter+(scope_width*0.8)*(-0.5),0.);
                cursors->set_xy_fast (1,xcenter+(scope_width*0.8)*(0.5),0.);
                cursors->draw (cr);
                cursors->set_xy_fast (0,xcenter,y_hi);
                cursors->set_xy_fast (1,xcenter,-y_hi);
                cursors->draw (cr);

                // phase setpoint
                cursors->set_stroke_rgba (1.,0.,0.,0.5);
                cursors->set_xy_fast (0,xcenter+scope_width*(-0.5), deg_to_y (parameters.phase_fb_setpoint, y_hi));
                cursors->set_xy_fast (1,xcenter+scope_width*(0.5), deg_to_y (parameters.phase_fb_setpoint, y_hi));
                cursors->draw (cr);

                // phase setpoint
                cursors->set_stroke_rgba (0.,1.,0.,0.5);
                cursors->set_xy_fast (0,xcenter+scope_width*(-0.5), db_to_y (dB_from_mV (parameters.amplitude_fb_setpoint), dB_hi, y_hi, dB_mags));
                cursors->set_xy_fast (1,xcenter+scope_width*(0.5), db_to_y (dB_from_mV (parameters.amplitude_fb_setpoint), dB_hi, y_hi, dB_mags));
                cursors->draw (cr);

               
                if (operation_mode == 6){ // tune info
                        cursors->set_stroke_rgba (0.8,0.8,0.8,0.4);
                        reading->set_anchor (CAIRO_ANCHOR_CENTER);
                        for (double f=-0.9*parameters.tune_span/2.; f < 0.8*parameters.tune_span/2.; f += 0.1*parameters.tune_span/2.){
                                double x=xcenter+scope_width*f/parameters.tune_span; // tune plot, freq x transform to canvas
                                valuestring = g_strdup_printf ("%g", f);
                                reading->set_stroke_rgba (CAIRO_COLOR_WHITE);
                                reading->set_text (x, yr-8, valuestring);
                                g_free (valuestring);
                                reading->draw (cr);
                                cursors->set_xy_fast (0,x,-yr);
                                cursors->set_xy_fast (1,x,yr);
                                cursors->draw (cr);
                        }
                        reading->set_anchor (CAIRO_ANCHOR_W);
                        
                        if (debug_level > 0)
                                g_print ("Tune: %g Hz,  %g mV,  %g dB, %g deg\n",
                                         pacpll_signals.signal_frq[n-1],
                                         s,
                                         -20.*log (fabs(s)),
                                         pacpll_signals.signal_ch1[n-1]
                                         );

                        // current pos marks
                        cursors->set_stroke_rgba (CAIRO_COLOR_YELLOW);
                        x = xcenter+scope_width*pacpll_signals.signal_frq[n-1]/parameters.tune_span;
                        cursors->set_xy_fast (0,x,ydb-20.);
                        cursors->set_xy_fast (1,x,ydb+20.);
                        cursors->draw (cr);
                        cursors->set_xy_fast (0,x,yph-20);
                        cursors->set_xy_fast (1,x,yph+20);
                        cursors->draw (cr);

                        cursors->set_stroke_rgba (CAIRO_COLOR_GREEN);
                        x = xcenter+scope_width*(pacpll_parameters.center_frequency-pacpll_parameters.frequency_manual)/parameters.tune_span;
                        ydb=-y_hi*(20.*log10 (fabs (pacpll_parameters.center_amplitude)))/60.;
                        cursors->set_xy_fast (0,x,ydb-50.);
                        cursors->set_xy_fast (1,x,ydb+50.);
                        cursors->draw (cr);

                        cursors->set_stroke_rgba (CAIRO_COLOR_RED);
                        x = xcenter+scope_width*(pacpll_parameters.center_frequency-pacpll_parameters.frequency_manual)/parameters.tune_span;
                        ydb=-y_hi*pacpll_parameters.center_phase/180.;
                        cursors->set_xy_fast (0,x-50,ydb);
                        cursors->set_xy_fast (1,x+50,ydb);
                        cursors->draw (cr);

                        // Set Point Phase / F-Center Mark
                        cursors->set_stroke_rgba (CAIRO_COLOR_YELLOW);
                        x = xcenter+scope_width*(pacpll_parameters.center_frequency-pacpll_parameters.frequency_center)/parameters.tune_span;
                        ydb=-y_hi*pacpll_parameters.phase_fb_setpoint/180.;
                        cursors->set_xy_fast (0,x-50,ydb);
                        cursors->set_xy_fast (1,x+50,ydb);
                        cursors->draw (cr);
                        cursors->set_xy_fast (0,x,ydb-20);
                        cursors->set_xy_fast (1,x,ydb+20);
                        cursors->draw (cr);

                        g_free (cursors);

                        valuestring = g_strdup_printf ("Phase: %g deg", pacpll_signals.signal_phase[n-1]);
                        reading->set_stroke_rgba (CAIRO_COLOR_RED);
                        reading->set_text (10, -(110), valuestring);
                        g_free (valuestring);
                        reading->draw (cr);
                        valuestring = g_strdup_printf ("Amplitude: %g dB (%g mV)", dB_from_mV (pacpll_signals.signal_ampl[n-1]), pacpll_signals.signal_ampl[n-1] );
                        reading->set_stroke_rgba (CAIRO_COLOR_GREEN);
                        reading->set_text (10, -(110-14), valuestring);
                        g_free (valuestring);
                        reading->draw (cr);
                        
                        valuestring = g_strdup_printf ("Tuning: last peak @ %g mV  %.4f Hz  %g deg",
                                                       pacpll_parameters.center_amplitude,
                                                       pacpll_parameters.center_frequency,
                                                       pacpll_parameters.center_phase
                                                       );
                        reading->set_stroke_rgba (CAIRO_COLOR_WHITE);
                        reading->set_text (10, (110+14*0), valuestring);
                        g_free (valuestring);
                        reading->draw (cr);

                        // do FIT
                        if (!rs_mode){ // don't in RS tune mode, X is bad
                                double f[1024];
                                double a[1024];
                                double p[1024];
                                double ma[1024];
                                double mp[1024];
                                int fn=0;
                                for (int i=0; i<n; ++i){
                                        double fi = pacpll_signals.signal_frq[i];
                                        double ai = pacpll_signals.signal_ampl[i];
                                        double pi = pacpll_signals.signal_phase[i];
                                        if (fi != 0. && ai > 0.){
                                                f[fn] = fi+pacpll_parameters.frequency_manual;
                                                a[fn] = ai;
                                                p[fn] = pi;
                                                fn++;
                                        }
                                }
                                if (fn > 15){
                                        resonance_fit lmgeofit_res (f,a,ma, fn); // using the Levenberg-Marquardt method with geodesic acceleration. Using GSL here.
                                        // initial guess
                                        if (pacpll_parameters.center_frequency > 1000.){
                                                lmgeofit_res.set_F0 (pacpll_parameters.center_frequency);
                                                lmgeofit_res.set_A (1000.0/pacpll_parameters.center_amplitude);
                                        } else {
                                                lmgeofit_res.set_F0 (pacpll_parameters.frequency_manual);
                                                lmgeofit_res.set_A (1000.0/20.0);
                                        }
                                        lmgeofit_res.set_Q (5000.0);
                                        lmgeofit_res.execute_fit ();
                                        cairo_item_path *resfit = new cairo_item_path (fn);
                                        resfit->set_line_width (1.0);
                                        resfit->set_stroke_rgba (CAIRO_COLOR_MAGENTA);

                                        phase_fit lmgeofit_ph (f,p,mp, fn); // using the Levenberg-Marquardt method with geodesic acceleration. Using GSL here.
                                        // initial guess
                                        if (pacpll_parameters.center_frequency > 1000.){
                                                lmgeofit_ph.set_B (pacpll_parameters.center_frequency);
                                                lmgeofit_ph.set_A (1.0);
                                        } else {
                                                lmgeofit_ph.set_B (pacpll_parameters.frequency_manual);
                                                lmgeofit_ph.set_A (1.0);
                                        }
                                        lmgeofit_ph.set_C (60.0);
                                        lmgeofit_ph.execute_fit ();
                                        cairo_item_path *phfit = new cairo_item_path (fn);
                                        phfit->set_line_width (1.0);
                                        phfit->set_stroke_rgba (CAIRO_COLOR_MAGENTA);
                                        
                                        for (int i=0; i<fn; ++i){
                                                resfit->set_xy_fast (i,
                                                                     xcenter+scope_width*(f[i]-pacpll_parameters.frequency_manual)/parameters.tune_span, // tune plot, freq x transform to canvas
                                                                     ydb=db_to_y (dB_from_mV (ma[i]), dB_hi, y_hi, dB_mags)
                                                                     );

                                                phfit->set_xy_fast (i,
                                                                    xcenter+scope_width*(f[i]-pacpll_parameters.frequency_manual)/parameters.tune_span, // tune plot, freq x transform to canvas,
                                                                    yph=deg_to_y (mp[i], y_hi)
                                                                    );

                                                // g_print ("%05d \t %10.3f \t %8.3f %8.3f\n", i, f[i], a[i], m[i]);
                                        }
                                        resfit->draw (cr);
                                        phfit->draw (cr);
                                        delete resfit;
                                        delete phfit;

                                        resonator_frequency_fitted = lmgeofit_res.get_F0 ();
                                        resonator_phase_fitted = lmgeofit_ph.get_C ();
                                        
                                        valuestring = g_strdup_printf ("Model Q: %g  A: %g mV  F0: %g Hz  Phase: %g [%g] @ %g Hz",
                                                                       lmgeofit_res.get_Q (),
                                                                       1000./lmgeofit_res.get_A (),
                                                                       lmgeofit_res.get_F0 (),
                                                                       lmgeofit_ph.get_C (),
                                                                       lmgeofit_ph.get_A (),
                                                                       lmgeofit_ph.get_B ()
                                                                       );
                                        reading->set_stroke_rgba (CAIRO_COLOR_MAGENTA);
                                        reading->set_text (10, (110-14*1), valuestring);
                                        g_free (valuestring);
                                        reading->draw (cr);
                                }else {
                                        resonator_frequency_fitted = -1.; // invalid, no fit
                                        resonator_phase_fitted = -1.;
                                }
                        }
                        
                } else {
                        if (transport == 0 and scope_xy_on){ // add polar plot for CH1,2 as XY
                                wave->set_stroke_rgba (CAIRO_COLOR_MAGENTA);
                                for (int k=0; k<n; ++k)
                                        wave->set_xy_fast (k,xcenter-yr*scope_z[0]*gain_scale[0]*signal[0][k],-yr*scope_z[1]*gain_scale[1]*signal[1][k]);
                                wave->draw (cr);
                        } if (transport == 0 and scope_fft_on){ // add FFT plot for CH1,2
                                const unsigned int fftlen = FFT_LEN; // use first N samples from buffer
                                double tm = FFT_LEN/1024*bram_window_length;
                                double fm = FFT_LEN/tm/2.;
                                static double power_spectrum[2][FFT_LEN/2];
                                cairo_item_path *wave_psd = new cairo_item_path (fftlen/2);
                                wave_psd->set_line_width (1.0);
                                        
                                compute_fft (fftlen, &signal[0][0], &power_spectrum[0][0]);

                                wave_psd->set_stroke_rgba (CAIRO_COLOR_MAGENTA);
                                double xk0 = xcenter-0.4*scope_width;
                                double xkf = scope_width/(fftlen/2)*scope_fft_time_zoom;
                                int km=0;
                                double psdmx=0.;
                                for (int k=0; k<fftlen/2; ++k){
                                        wave_psd->set_xy_fast (k, xk0+xkf*(k-bram_shift)*scope_fft_time_zoom, db_to_y (dB_from_mV (scope_z[0]*power_spectrum[0][k]), dB_hi, y_hi, dB_mags));
                                        if (k==2){ // skip nyquist
                                                psdmx = power_spectrum[0][k];
                                                km = k;
                                        } else
                                                if (psdmx < power_spectrum[0][k]){
                                                        psdmx = power_spectrum[0][k];
                                                        km = k;
                                                }
                                }
                                wave_psd->draw (cr);
                                {
                                        double f=fm/scope_fft_time_zoom * km/(fftlen/2.0);
                                        double x=xk0+xkf*(km-bram_shift)*scope_fft_time_zoom;
                                        valuestring = g_strdup_printf ("%g mV @ %g %s", psdmx, f*1e-3, "kHz");
                                        reading->set_stroke_rgba (CAIRO_COLOR_MAGENTA);
                                        reading->set_text (x, db_to_y (dB_from_mV (scope_z[0]*psdmx), dB_hi, y_hi, dB_mags), valuestring);
                                        g_free (valuestring);
                                        reading->draw (cr);
                                }
                                compute_fft (fftlen, &signal[1][0], &power_spectrum[1][0]);
                                
                                wave_psd->set_stroke_rgba (CAIRO_COLOR_YELLOW);
                                for (int k=0; k<fftlen/2; ++k){
                                        wave_psd->set_xy_fast (k, xk0+xkf*(k-bram_shift)*scope_fft_time_zoom, db_to_y (dB_from_mV (scope_z[1]*power_spectrum[1][k]), dB_hi, y_hi, dB_mags));
                                        if (k==2){ // skip nyquist
                                                psdmx = power_spectrum[1][k];
                                                km = k;
                                        } else
                                                if (psdmx < power_spectrum[1][k]){
                                                        psdmx = power_spectrum[1][k];
                                                        km = k;
                                                }
                                }
                                wave_psd->draw (cr);
                                delete wave_psd;
                                {
                                        double f=fm/scope_fft_time_zoom * km/(fftlen/2.0);
                                        double x=xk0+xkf*(km-bram_shift)*scope_fft_time_zoom;
                                        valuestring = g_strdup_printf ("%g mV @ %g %s", psdmx, f*1e-3, "kHz");
                                        reading->set_stroke_rgba (CAIRO_COLOR_YELLOW);
                                        reading->set_text (x, db_to_y (dB_from_mV (scope_z[1]*psdmx), dB_hi, y_hi, dB_mags), valuestring);
                                        g_free (valuestring);
                                        reading->draw (cr);
                                }                                
                                // compute FFT frq scale

                                // Freq tics!
                                double df = AutoSkl(fm/10)/scope_fft_time_zoom;
                                double f0 = 0.0;
                                double fr = fm/scope_fft_time_zoom;
                                double x0=xk0-xkf*bram_shift*scope_fft_time_zoom;
                                cursors->set_stroke_rgba (CAIRO_COLOR_YELLOW_ID, 0.5);
                                reading->set_anchor (CAIRO_ANCHOR_CENTER);
                                for (double f=AutoNext(f0, df); f <= fr; f += df){
                                        double x=x0+scope_width*(f-f0)/fr; // freq position tic pos
                                        valuestring = g_strdup_printf ("%g %s", f*1e-3, "kHz");
                                        reading->set_stroke_rgba (CAIRO_COLOR_YELLOW);
                                        reading->set_text (x, yr-18, valuestring);
                                        g_free (valuestring);
                                        reading->draw (cr);
                                        cursors->set_xy_fast (0,x,-yr);
                                        cursors->set_xy_fast (1,x,yr);
                                        cursors->draw (cr);
                                }

                                // dB/Hz ticks
                                cursors->set_stroke_rgba (CAIRO_COLOR_MAGENTA_ID, 0.5);
                                for (int db=(int)dB_hi; db >= -20*dB_mags; db -= 10){
                                        double y;
                                        valuestring = g_strdup_printf ("%4d dB/Hz", db);
                                        reading->set_stroke_rgba (CAIRO_COLOR_MAGENTA);
                                        reading->set_text (scope_width - 70,  y=db_to_y ((double)db, dB_hi, y_hi, dB_mags), valuestring);
                                        g_free (valuestring);
                                        reading->draw (cr);
                                        cursors->set_xy_fast (0,xcenter+(scope_width)*(-0.5),y);
                                        cursors->set_xy_fast (1,xcenter+(scope_width*0.7)*(0.5),y);
                                        cursors->draw (cr);
                                }
                        }
                }
                delete wave;
                delete reading;
                if (binwave8bit[0])
                        for (int bit=0; bit<binary_BITS; ++bit)
                                delete binwave8bit[bit];
        } else {
                deg_extend = 1;
                // minimize
                gtk_drawing_area_set_content_width (area, 128);
                gtk_drawing_area_set_content_height (area, 4);
                current_width=0;
        }
}


// called on Parameter Updates via Soup Socket
void RPspmc_pacpll::on_new_data (){ // Class: RPspmc_pacpll : public AppBase, public RP_JSON_talk{
        update_monitoring_parameters();
        
        update_shm_monitors ();
        
        gtk_widget_queue_draw (signal_graph_area);
        
        //self->stream_data ();
        update_health ();
        
        RPSPMC_ControlClass->on_new_data (); // run follow up updates on GUI
}


/*  SHM memory block for external apps providing all monitors in binary double
 *
### Python snippet to read XYZMaMi monitors:

import requests
import numpy as np
from multiprocessing import shared_memory
from multiprocessing.resource_tracker import unregister

xyz_shm = shared_memory.SharedMemory(name='gxsm4rpspmc_monitors')
unregister(xyz_shm._name, 'shared_memory') ## necessary to prevent python to destroy this shm block at exit :( :(

print (xyz_shm)
xyz=np.ndarray((9,), dtype=np.double, buffer=xyz_shm.buf).reshape((3,3)).T  # X Mi Ma, Y Mi Ma, Z Mi Ma
print (xyz)

 *  
 */


gboolean RPspmc_pacpll::update_shm_monitors (int close_shm){
        const char *rpspmc_monitors = "/gxsm4rpspmc_monitors";
        static int shm_fd = -1;
        static void *shm_ptr = NULL;
        // Set the size of the shared memory region
        static size_t shm_size = sizeof(double)*512;

        // tmp SHM data exchange block created and reused/resized as needed
        const char *gxsm_shm_data = "/gxsm4tmp_data_block";
        static int shm_data_fd = -1;
        static void *shm_data_ptr = NULL;
        static size_t shm_data_size = 0;
        
        if (shm_fd == -1){
                // create SHM monitor memory block
                shm_fd = shm_open(rpspmc_monitors, O_CREAT | O_RDWR, 0666);
                if (shm_fd == -1) {
                        g_error ("Error shm_open of %s.", rpspmc_monitors);
                        return false;
                }
                
                // Resize the shared memory object to the desired size
                if (ftruncate (shm_fd, shm_size) == -1) {
                        g_error ("Error ftruncate of %s.", rpspmc_monitors);
                        return false;
                }

                // Map the shared memory object into the process address space
                shm_ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
                if (shm_ptr == MAP_FAILED) {
                        g_error("Error mmap");
                        return false;
                }
                rt_monitors_shm_ptr = shm_ptr;
                rpspmc_hwi->set_shm_ref (shm_ptr);
        }

        if (close_shm){
                g_message ("Closing SHM. End of Serving actions.");
                
                rpspmc_hwi->set_shm_ref (NULL);
                rt_monitors_shm_ptr = NULL;
                
                // Unmap the shared memory object
                if (munmap(shm_ptr, shm_size) == -1) {
                        g_error("Error munmap");
                        return;
                }
                // Close the shared memory file descriptor
                if (close(shm_fd) == -1) {
                        g_error("Error close");
                        return 1;
                }
                
                // Unlink the shared memory object
                if (shm_unlink(rpspmc_monitors) == -1) {
                        g_error("Error shm_unlink");
                        return 1;
                }
                shm_ptr = NULL;
                shm_fd = -1;
                shm_size = 0;

                // Unmap the shared tmp data memory object if been created
                if (shm_data_fd > 0){
                        if (munmap(shm_data_ptr, shm_data_size) == -1) {
                                g_error("Error data block munmap");
                                return;
                        }
                        // Close the shared memory file descriptor
                        if (close(shm_data_fd) == -1) {
                                g_error("Error close data block shm fd");
                                return 1;
                        }
                
                        // Unlink the shared memory object
                        if (shm_unlink(gxsm_shm_data) == -1) {
                                g_error("Error shm_unlink data block shm");
                                return 1;
                        }
                        shm_data_ptr = NULL;
                        shm_data_fd = -1;
                        shm_data_size = 0;
                }
                
                return false;
        }

        //double rtm = (double)g_get_real_time ();

        // RTime, XYZUGains, VA Unit conversion:
        double VG_XYZUI[6] = {
                (double)g_get_real_time (),
                main_get_gapp()->xsm->Inst->VX(), main_get_gapp()->xsm->Inst->VY(), main_get_gapp()->xsm->Inst->VZ(),
                main_get_gapp()->xsm->Inst->BiasGainV2V(),
                main_get_gapp()->xsm->Inst->nAmpere2V(1.0)
        };
        //main_get_gapp()->xsm->Inst->Volt2XA(1.), main_get_gapp()->xsm->Inst->Volt2YA(1.), main_get_gapp()->xsm->Inst->Volt2ZA(1.);
        //main_get_gapp()->xsm->Inst->VX0(), main_get_gapp()->xsm->Inst->VY0(), main_get_gapp()->xsm->Inst->VZ0();
        //main_get_gapp()->xsm->Inst->Volt2X0A(1.), main_get_gapp()->xsm->Inst->Volt2Y0A(1.), main_get_gapp()->xsm->Inst->Volt2Z0A(1.);
        //
        //main_get_gapp()->xsm->Inst->BiasGainV2V()
        //main_get_gapp()->xsm->Inst->nAmpere2V(1.0)
        //1e9*main_get_gapp()->xsm->Inst->nAmpere2V(1.0) // IVC-A/V"
        //main_get_gapp()->xsm->Inst->nNewton2V(1.0)
        //main_get_gapp()->xsm->Inst->dHertz2V(1.0)
        //main_get_gapp()->xsm->Inst->eV2V(1.0)
                
        // Write data to the shared memory
        // void memcpy_to_rt_monitors (double *dvec, gsize count, int start_pos);

        // RPSPMC T XYZ MAX MIN (3x3) *** now updated at much faster rate by RPSPMC streaming server Z85 data messages, plus 20x20 block at [100..500] ~ 1kSPS or 20..30ms @ 20 deep vectors
        // shm_ptr[0..10] = RPSPMC [T XYZ MAX MIN (3x3)]
        
        // RPSPMC Monitors: Bias, reg, set,   GPVU,A,B,AM,FM, MUX, Signal (Current), AD463x[2], XYZ, XYZ0, XYZS
        memcpy  (shm_ptr+10*sizeof(double), &spmc_parameters.bias_monitor, 21*sizeof(double));

        // PAC-PLL Monitors: dc-offset, exec_ampl_mon, dds_freq_mon, volume_mon, phase_mon, control_dfreq_mon
        memcpy  (shm_ptr+40*sizeof(double), &pacpll_parameters.dc_offset, 7*sizeof(double));

        // Z-Servo Info: mode, setpoint, cp, ci, cp_db, ci_db, upper, lower, setpoint_cz, level, in_offcomp, src_mux
        memcpy  (shm_ptr+50*sizeof(double), &spmc_parameters.z_servo_mode, 12*sizeof(double));

        // Scan Info: alpha, slope-dzx,dzy,slew, scanpos-x,y,slew,opts, offset-x,y,z
        memcpy  (shm_ptr+70*sizeof(double), &spmc_parameters.alpha, 13*sizeof(double));

        // FPGA RPSPMC uptime in sec, 8ns resolution -- i.e. exact time of last reading
        //memcpy  (shm_ptr+100*sizeof(double), &spmc_parameters.uptime_seconds, sizeof(double));
        // Unix Real Time (Gxsm)
        //memcpy  (shm_ptr+99*sizeof(double), &rtm, sizeof(double));
        memcpy  (shm_ptr+90*sizeof(double), &VG_XYZUI, 6*sizeof(double));

        // ================================================================================
        // push history -- move this to stream server for vect r block
        //rpspmc_hwi->push_history_vector (shm_ptr, 48); // currently used: 0..8, 10..31, 40..47 as size of double

        PI_DEBUG (DBG_L2, "SHM MONITOR UPDATE @RPSPMC TIME: " << spmc_parameters.uptime_seconds << " s");
        //g_message ("SHM MONITOR UPDATE @RPSPMC TIME: %.9fs", spmc_parameters.uptime_seconds);

        
        return true;
}

// END
