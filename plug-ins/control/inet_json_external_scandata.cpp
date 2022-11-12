/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: inet_json_external_scandata.C
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

/* Please do not change the Begin/End lines of this comment section!
 * this is a LaTeX style section used for auto generation of the PlugIn Manual 
 * Chapter. Add a complete PlugIn documentation inbetween the Begin/End marks!
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Inet JSON Scan Data Control
% PlugInName: inet_json_external_scandata
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: windows-section Inet JSON Scan External Data
RP data streaming

% PlugInDescription

% PlugInUsage

% OptPlugInRefs

% OptPlugInNotes

% OptPlugInHints

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include <gio/gio.h>
#include <libsoup/soup.h>
#include <zlib.h>

#include "config.h"
#include "plugin.h"

#include "unit.h"
#include "pcs.h"
#include "xsmtypes.h"
#include "glbvars.h"
#include "action_id.h"

#include "plug-ins/control/inet_json_external_scandata.h"
#include "plug-ins/control/resonance_fit.h"

// Plugin Prototypes - default PlugIn functions
static void inet_json_external_scandata_init (void); // PlugIn init
static void inet_json_external_scandata_query (void); // PlugIn "self-install"
static void inet_json_external_scandata_about (void); // About
static void inet_json_external_scandata_configure (void); // Configure plugIn, called via PlugIn-Configurator
static void inet_json_external_scandata_cleanup (void); // called on PlugIn unload, should cleanup PlugIn rescources

// other PlugIn Functions and Callbacks (connected to Buttons, Toolbar, Menu)
static void inet_json_external_scandata_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void inet_json_external_scandata_SaveValues_callback ( gpointer );

// Fill in the GxsmPlugin Description here -- see also: Gxsm/src/plugin.h
GxsmPlugin inet_json_external_scandata_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	"Inet_Json_External_Scandata",
	NULL,
	NULL,
	"Percy Zahl",
	"windows-section", // Menu-path/section
	N_("Inet JSON RP"), // Menu Entry -- overridden my set-window-geometry() call automatism
	N_("Open Inet JSON External Scan Data Control Window"),
	"Inet JSON External Scan Data Control Window", // help text
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	inet_json_external_scandata_init,  
	inet_json_external_scandata_query,  
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	inet_json_external_scandata_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	inet_json_external_scandata_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL,
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
	inet_json_external_scandata_show_callback, // direct menu entry callback1 or NULL
	NULL, // direct menu entry callback2 or NULL

	inet_json_external_scandata_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Inet JSON External Scan Data Control Plugin\n\n"
                                   "This plugin manages externa Scan Data Sources.\n"
	);

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	inet_json_external_scandata_pi.description = g_strdup_printf(N_("Gxsm inet_json_external_scandata plugin %s"), VERSION);
	return &inet_json_external_scandata_pi; 
}

// data passed to "idle" function call, used to refresh/draw while waiting for data
typedef struct {
	GSList *scan_list; // scans to update
	GFunc  UpdateFunc; // function to call for background updating
	gpointer data; // additional data (here: reference to the current Inet_Json_External_Scandata object)
} IdleRefreshFuncData;

Inet_Json_External_Scandata *inet_json_external_scandata = NULL;

// Query Function, installs Plugin's in File/Import and Export Menupaths!

#define REMOTE_PREFIX "INET_JSON_EX_"

static void inet_json_external_scandata_query(void)
{
	if(inet_json_external_scandata_pi.status) g_free(inet_json_external_scandata_pi.status); 
	inet_json_external_scandata_pi.status = g_strconcat (
                                                 N_("Plugin query has attached "),
                                                 inet_json_external_scandata_pi.name, 
                                                 N_(": File IO Filters are ready to use"),
                                                 NULL);

	PI_DEBUG (DBG_L2, "inet_json_external_scandata_query:new" );
	inet_json_external_scandata = new Inet_Json_External_Scandata (main_get_gapp() -> get_app ());

	PI_DEBUG (DBG_L2, "inet_json_external_scandata_query:res" );
	
	inet_json_external_scandata_pi.app->ConnectPluginToCDFSaveEvent (inet_json_external_scandata_SaveValues_callback);
}

static void inet_json_external_scandata_SaveValues_callback ( gpointer gp_ncf ){
	NcFile *ncf = (NcFile *) gp_ncf;
        if (inet_json_external_scandata)
                inet_json_external_scandata->save_values (ncf);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void inet_json_external_scandata_init(void)
{
  PI_DEBUG (DBG_L2, "inet_json_external_scandata Plugin Init" );
}

// about-Function 
static void inet_json_external_scandata_about(void)
{
        const gchar *authors[] = { inet_json_external_scandata_pi.authors, NULL};
        gtk_show_about_dialog (NULL,
                               "program-name",  inet_json_external_scandata_pi.name,
                               "version", VERSION,
                               "license", GTK_LICENSE_GPL_3_0,
                               "comments", about_text,
                               "authors", authors,
                               NULL
                               );
}

// configure-Function
static void inet_json_external_scandata_configure(void)
{
	if(inet_json_external_scandata_pi.app)
		inet_json_external_scandata_pi.app->message("inet_json_external_scandata Plugin Configuration");
}

// cleanup-Function, make sure the Menustrings are matching those above!!!
static void inet_json_external_scandata_cleanup(void)
{
	// delete ...
	if( inet_json_external_scandata )
		delete inet_json_external_scandata ;

	if(inet_json_external_scandata_pi.status) g_free(inet_json_external_scandata_pi.status); 
}

static void inet_json_external_scandata_show_callback(GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	PI_DEBUG (DBG_L2, "inet_json_external_scandata Plugin : show" );
	if( inet_json_external_scandata )
		inet_json_external_scandata->show();
}


#define dB_min_from_Q(Q) (20.*log10(1./((1L<<(Q))-1)))
#define dB_max_from_Q(Q) (20.*log10(1L<<((32-(Q))-1)))
#define SETUP_dB_RANGE_from_Q(PCS, Q) { PCS->setMin(dB_min_from_Q(Q)); PCS->setMax(dB_max_from_Q(Q)); }

Inet_Json_External_Scandata::Inet_Json_External_Scandata (Gxsm4app *app):AppBase(app)
{
        GtkWidget *tmp;
        GtkWidget *wid;
	
	GSList *EC_R_list=NULL;
	GSList *EC_QC_list=NULL;

        debug_level = 0;
        input_rpaddress = NULL;
        text_status = NULL;
        streaming = 0;

        ch_freq = -1;
        ch_ampl = -1;
        deg_extend = 1;

        for (int i=0; i<5; ++i){
                scope_ac[i]=false;
                scope_dc_level[i]=0.;
                gain_scale[i] = 0.001; // 1000mV full scale
        }
        block_message = 0;
        unwrap_phase_plot = true;
        
        /* create a new connection, init */

	listener=NULL;
        port=9002;

	session=NULL;
	msg=NULL;
	client=NULL;
	client_error=NULL;
	error=NULL;
        
	PI_DEBUG (DBG_L2, "inet_json_external_scandata Plugin : building interface" );

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
	AppWindowInit("Inet JSON External Scan Data Control for High Speed RedPitaya PACPLL");

        gchar *gs_path = g_strdup_printf ("%s.inet_json_settings", GXSM_RES_BASE_PATH_DOT);
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
        parameters.aux_scale = 0.011642; // 20Hz / V equivalent 
        parameters.volume_manual = 300.0; // mV
        parameters.qc_gain=0.0; // gain +/-1.0
        parameters.qc_phase=0.0; // deg
        bp->set_input_width_chars (8);
        bp->set_input_nx (1);
        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::pac_tau_parameter_changed, this);
  	bp->grid_add_ec ("Tau DC", mTime, &parameters.pac_dctau, -1.0, 63e6, "6g", 10., 1., "PAC-DCTAU");
        bp->new_line ();
  	bp->grid_add_ec ("Tau PAC", uTime, &parameters.pactau, 0.0, 63e6, "6g", 10., 1., "PACTAU");
  	bp->grid_add_ec (NULL, uTime, &parameters.pacatau, 0.0, 63e6, "6g", 10., 1., "PACATAU");
        bp->new_line ();
        bp->set_no_spin (false);
        bp->set_input_nx (2);
        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::qc_parameter_changed, this);
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
        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::pac_frequency_parameter_changed, this);
  	bp->grid_add_ec ("Frequency", Hz, &parameters.frequency_manual, 0.0, 20e6, ".3lf", 0.1, 10., "FREQUENCY-MANUAL");
        bp->new_line ();
        bp->set_no_spin (true);
        bp->set_input_width_chars (8);
        bp->set_input_nx (1);
  	bp->grid_add_ec ("Center,Scale", Hz, &parameters.frequency_center, 0.0, 20e6, ".3lf", 0.1, 10., "FREQUENCY-CENTER");
  	bp->grid_add_ec (NULL, Unity, &parameters.aux_scale, -1e6, 1e6, ".6lf", 0.1, 10., "AUX-SCALE");
        bp->new_line ();
        bp->set_no_spin (false);
        bp->set_input_nx (2);
        bp->set_input_width_chars (12);
        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::pac_volume_parameter_changed, this);
  	bp->grid_add_ec ("Volume", mVolt, &parameters.volume_manual, 0.0, 1000.0, "5g", 0.1, 1.0, "VOLUME-MANUAL");
        bp->new_line ();
        bp->set_no_spin (true);
        bp->set_input_width_chars (10);
        parameters.tune_dfreq = 0.1;
        parameters.tune_span = 50.0;
        bp->set_input_nx (1);
        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::tune_parameter_changed, this);
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

        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::amp_ctrl_parameter_changed, this);
        bp->grid_add_ec ("Setpoint", mVolt, &parameters.amplitude_fb_setpoint, 0.0, 1000.0, "5g", 0.1, 10.0, "AMPLITUDE-FB-SETPOINT");
        bp->new_line ();
        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::amplitude_gain_changed, this);
        bp->grid_add_ec ("CP gain", dB, &parameters.amplitude_fb_cp_db, -200.0, 200.0, "g", 0.1, 1.0, "AMPLITUDE-FB-CP");
        SETUP_dB_RANGE_from_Q(bp->ec, 31); // Amplitude QAMCOEF = Q31
        bp->new_line ();
        bp->grid_add_ec ("CI gain", dB, &parameters.amplitude_fb_ci_db, -200.0, 200.0, "g", 0.1, 1.0, "AMPLITUDE-FB-CI");
        SETUP_dB_RANGE_from_Q(bp->ec, 31); // Amplitude QAMCOEF = Q31
        bp->new_line ();
        bp->set_no_spin (true);
        bp->set_input_width_chars (16);
        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::amp_ctrl_parameter_changed, this);
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
        bp->grid_add_check_button ( N_("Enable"), "Enable Amplitude Controller", 2,
                                    G_CALLBACK (Inet_Json_External_Scandata::amplitude_controller), this);
        bp->grid_add_check_button ( N_("Invert"), "Invert Amplitude Controller Gain. Normally positive.", 2,
                                    G_CALLBACK (Inet_Json_External_Scandata::amplitude_controller_invert), this);

        bp->new_line ();
        bp->grid_add_check_button ( N_("Set Auto Trigger SS"), "Set Auto Trigger SS", 2,
                                    G_CALLBACK (Inet_Json_External_Scandata::set_ss_auto_trigger), this);
        bp->grid_add_check_button ( N_("QControl"), "QControl", 2,
                                    G_CALLBACK (Inet_Json_External_Scandata::qcontrol), this);
	g_object_set_data( G_OBJECT (bp->button), "QC_SETTINGS_list", EC_QC_list);
        bp->new_line ();
        bp->grid_add_check_button ( N_("Use LockIn Mode*"), "Use LockIn Mode.\n*: Must use PAC/LockIn FPGA bit code\n instead of Dual PAC FPGA bit code.", 2,
                                    G_CALLBACK (Inet_Json_External_Scandata::select_pac_lck_amplitude), this);
        bp->new_line ();
        bp->grid_add_check_button ( N_("Pulse Control"), "Show Pulse Controller", 2,
                                    G_CALLBACK (Inet_Json_External_Scandata::show_pulse_control), this);

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
        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::phase_ctrl_parameter_changed, this);
        bp->grid_add_ec ("Setpoint", Deg, &parameters.phase_fb_setpoint, -180.0, 180.0, "5g", 0.1, 1.0, "PHASE-FB-SETPOINT");
        bp->new_line ();
        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::phase_gain_changed, this);
        bp->grid_add_ec ("CP gain", dB, &parameters.phase_fb_cp_db, -200.0, 200.0, "g", 0.1, 1.0, "PHASE-FB-CP");
        SETUP_dB_RANGE_from_Q(bp->ec, 31); // PHASE  QPHCOEF = Q31 
        bp->new_line ();
        bp->grid_add_ec ("CI gain", dB, &parameters.phase_fb_ci_db, -200.0, 200.0, "g", 0.1, 1.0, "PHASE-FB-CI");
        SETUP_dB_RANGE_from_Q(bp->ec, 31); // PHASE  QPHCOEF = Q31 
        bp->new_line ();
        bp->set_no_spin (true);
        bp->set_input_width_chars (16);
        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::phase_ctrl_parameter_changed, this);
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
        bp->grid_add_check_button ( N_("Enable"), "Enable Phase Controller", 2,
                                    G_CALLBACK (Inet_Json_External_Scandata::phase_controller), this);
        bp->grid_add_check_button ( N_("Invert"), "Invert Phase Controller Gain. Normally positive.", 2,
                                    G_CALLBACK (Inet_Json_External_Scandata::phase_controller_invert), this);
        bp->new_line ();
        bp->grid_add_check_button ( N_("Unwapping"), "Always unwrap phase/auto unwrap only if controller is enabled", 2,
                                    G_CALLBACK (Inet_Json_External_Scandata::phase_unwrapping_always), this);
        bp->grid_add_check_button ( N_("Unwap Plot"), "Unwrap plot at high level", 2,
                                    G_CALLBACK (Inet_Json_External_Scandata::phase_unwrap_plot), this);
        bp->new_line ();
        bp->grid_add_check_button ( N_("Use LockIn Mode"), "Use LockIn Mode", 2,
                                    G_CALLBACK (Inet_Json_External_Scandata::select_pac_lck_phase), this);
        bp->grid_add_check_button ( N_("dFreq Control"), "Show delta Frequency Controller", 2,
                                    G_CALLBACK (Inet_Json_External_Scandata::show_dF_control), this);

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
        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::dfreq_ctrl_parameter_changed, this);
        bp->grid_add_ec ("Setpoint", Hz, &parameters.dfreq_fb_setpoint, -200.0, 200.0, "5g", 0.1, 1.0, "DFREQ-FB-SETPOINT");
        bp->new_line ();
        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::dfreq_gain_changed, this);
        bp->grid_add_ec ("CP gain", dB, &parameters.dfreq_fb_cp_db, -200.0, 200.0, "g", 0.1, 1.0, "DFREQ-FB-CP");
        bp->new_line ();
        bp->grid_add_ec ("CI gain", dB, &parameters.dfreq_fb_ci_db, -200.0, 200.0, "g", 0.1, 1.0, "DFREQ-FB-CI");
        bp->new_line ();
        bp->set_no_spin (true);
        bp->set_input_width_chars (16);
        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::dfreq_ctrl_parameter_changed, this);
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
        bp->grid_add_check_button ( N_("Enable"), "Enable Dfreq Controller", 2,
                                    G_CALLBACK (Inet_Json_External_Scandata::dfreq_controller), this);
        bp->grid_add_check_button ( N_("Invert"), "Invert Dfreq Controller Gain. Normally positive.", 2,
                                    G_CALLBACK (Inet_Json_External_Scandata::dfreq_controller_invert), this);

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
        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::pulse_form_parameter_changed, this);
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
        bp->new_line ();
        
        bp->set_input_nx (1);
        bp->grid_add_check_button ( N_("Enable"), "Enable Pulse Forming", 2,
                                    G_CALLBACK (Inet_Json_External_Scandata::pulse_form_enable), this);

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
        bp->set_default_ec_change_notice_fkt (Inet_Json_External_Scandata::pac_tau_transport_changed, this);
  	bp->grid_add_ec ("LP dFreq,Phase,Exec,Ampl", mTime, &parameters.transport_tau[0], -1.0, 63e6, "6g", 10., 1., "PAC-TRANSPORT-TAU-DFREQ");
  	bp->grid_add_ec (NULL, mTime, &parameters.transport_tau[1], -1.0, 63e6, "6g", 10., 1., "PAC-TRANSPORT-TAU-PHASE");
  	bp->grid_add_ec (NULL, mTime, &parameters.transport_tau[2], -1.0, 63e6, "6g", 10., 1., "PAC-TRANSPORT-TAU-EXEC");
  	bp->grid_add_ec (NULL, mTime, &parameters.transport_tau[3], -1.0, 63e6, "6g", 10., 1., "PAC-TRANSPORT-TAU-AMPL");
        bp->new_line ();
        */
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (Inet_Json_External_Scandata::choice_operation_callback),
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
                          G_CALLBACK (Inet_Json_External_Scandata::choice_update_ts_callback),
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
                          G_CALLBACK (Inet_Json_External_Scandata::choice_trigger_mode_callback),
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
                          G_CALLBACK (Inet_Json_External_Scandata::choice_auto_set_callback),
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
                          G_CALLBACK (Inet_Json_External_Scandata::choice_transport_ch12_callback),
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
                "DEBUG McBSP",          // [8]
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

        
        // GPIO monitor selections -- full set, experimental
	const gchar *monitor_modes_gpio[] = {
                "OFF: no plot",
                "LMS Amplitude(A,B)",
                "LMS Phase(A,B)",
                "LMS A (Real)",
                "LMS B (Imag)",
                "SQRT Ampl Monitor",
                "ATAN Phase Monitor",
                "X5",
                "X6",
                "Exec Amplitude",
                "DDS Freq Monitor",
                "X3 M (LMS Input)",
                "X5 M1(LMS Input-DC)",
                "X11 BRAM WPOS",
                "X12 BRAM DEC",
                NULL };

        // CH3 from GPIO MONITOR</p>
	wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (Inet_Json_External_Scandata::choice_transport_ch3_callback),
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
                          G_CALLBACK (Inet_Json_External_Scandata::choice_transport_ch4_callback),
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
                          G_CALLBACK (Inet_Json_External_Scandata::choice_transport_ch5_callback),
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
                                    G_CALLBACK (Inet_Json_External_Scandata::scope_ac_ch1_callback), this);
        wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (Inet_Json_External_Scandata::scope_z_ch1_callback),
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
                                    G_CALLBACK (Inet_Json_External_Scandata::scope_ac_ch2_callback), this);

        wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (Inet_Json_External_Scandata::scope_z_ch2_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

	for(int i=0; zoom_levels[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid),  zoom_levels[i], zoom_levels[i]);
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 3);


        bp->grid_add_check_button ( N_("Ch3 AC"), "Remove Offset from Ch3", 1,
                                    G_CALLBACK (Inet_Json_External_Scandata::scope_ac_ch3_callback), this);
        bp->grid_add_check_button ( N_("XY"), "display XY plot for In1, In2", 1,
                                    G_CALLBACK (Inet_Json_External_Scandata::scope_xy_on_callback), this);
        bp->grid_add_check_button ( N_("FFT"), "display FFT plot for In1, In2", 1,
                                    G_CALLBACK (Inet_Json_External_Scandata::scope_fft_on_callback), this);
        
        wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (Inet_Json_External_Scandata::scope_fft_time_zoom_callback),
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
        bram_shift = 0;
        bp->grid_add_widget (gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, 4096, 10), 10);
        g_signal_connect (G_OBJECT (bp->any_widget), "value-changed",
                          G_CALLBACK (Inet_Json_External_Scandata::scope_buffer_position_callback),
                          this);

        // ========================================
        
        bp->pop_grid ();
        bp->new_line ();
        
        bp->new_grid_with_frame ("RedPitaya Web Socket Address for JSON talk", 10);

        bp->set_input_width_chars (25);
        input_rpaddress = bp->grid_add_input ("RedPitaya Address");

        g_settings_bind (inet_json_settings, "redpitay-address",
                         G_OBJECT (bp->input), "text",
                         G_SETTINGS_BIND_DEFAULT);
        gtk_widget_set_tooltip_text (input_rpaddress, "RedPitaya IP Address like rp-f05603.local or 130.199.123.123");
        //  "ws://rp-f05603.local:9002/"
        //gtk_entry_set_text (GTK_ENTRY (input_rpaddress), "http://rp-f05603.local/pacpll/?type=run");
        //gtk_entry_set_text (GTK_ENTRY (input_rpaddress), "130.199.243.200");
        //gtk_entry_set_text (GTK_ENTRY (input_rpaddress), "192.168.1.10");
        
        bp->grid_add_check_button ( N_("Connect"), "Check to initiate connection, uncheck to close connection.", 1,
                                    G_CALLBACK (Inet_Json_External_Scandata::connect_cb), this);
        run_scope=0;
        bp->grid_add_check_button ( N_("Scope"), "Enable Scope", 1,
                                    G_CALLBACK (Inet_Json_External_Scandata::enable_scope), this);
        bp->grid_add_check_button ( N_("Debug"), "Enable debugging LV1.", 1,
                                    G_CALLBACK (Inet_Json_External_Scandata::dbg_l1), this);
        bp->grid_add_check_button ( N_("+"), "Debug LV2", 1,
                                    G_CALLBACK (Inet_Json_External_Scandata::dbg_l2), this);
        bp->grid_add_check_button ( N_("++"), "Debug LV4", 1,
                                    G_CALLBACK (Inet_Json_External_Scandata::dbg_l4), this);
        scope_width_points  = 512.;
        scope_height_points = 256.;
        bp->set_input_width_chars (5);
  	bp->grid_add_ec ("SW", Unity, &scope_width_points, 256.0, 4096.0, ".0f", 128, 256., "SCOPE-WIDTH");
        bp->set_input_width_chars (4);
  	bp->grid_add_ec ("SH", Unity, &scope_height_points, 128.0, 1024.0, ".0f", 128, 256., "SCOPE-HEIGHT");
        rp_verbose_level = 0.0;
        bp->set_input_width_chars (2);
  	bp->grid_add_ec ("V", Unity, &rp_verbose_level, 0., 10., ".0f", 1., 1., "RP-VERBOSE-LEVEL");        
        //bp->new_line ();
        //tmp=bp->grid_add_button ( N_("Read"), "TEST READ", 1,
        //                          G_CALLBACK (Inet_Json_External_Scandata::read_cb), this);
        //tmp=bp->grid_add_button ( N_("Write"), "TEST WRITE", 1,
        //                          G_CALLBACK (Inet_Json_External_Scandata::write_cb), this);

        bp->new_line ();
        bp->set_input_width_chars (80);
        red_pitaya_health = bp->grid_add_input ("RedPitaya Health",10);

        PangoFontDescription *fontDesc = pango_font_description_from_string ("monospace 10");
        //gtk_widget_modify_font (red_pitaya_health, fontDesc);
        // ### GTK4 ??? CSS ??? ###  gtk_widget_override_font (red_pitaya_health, fontDesc); // use CSS, thx, annyoing garbage... ??!?!?!?
        pango_font_description_free (fontDesc);

        gtk_widget_set_sensitive (bp->input, FALSE);
        gtk_editable_set_editable (GTK_EDITABLE (bp->input), FALSE); 
        update_health ("Not connected.");
        bp->new_line ();

        text_status = gtk_text_view_new ();
 	gtk_text_view_set_editable (GTK_TEXT_VIEW (text_status), FALSE);
        //gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_status), GTK_WRAP_WORD_CHAR);
        GtkWidget *scrolled_window = gtk_scrolled_window_new ();
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        //gtk_widget_set_size_request (scrolled_window, 200, 60);
        gtk_widget_set_hexpand (scrolled_window, TRUE);
        gtk_widget_set_vexpand (scrolled_window, FALSE);
        gtk_scrolled_window_set_child ( GTK_SCROLLED_WINDOW (scrolled_window), text_status);
        bp->grid_add_widget (scrolled_window, 10);
        gtk_widget_show (scrolled_window);
        
        // ========================================
        bp->pop_grid ();
        
        bp->show_all ();
 
        // save List away...
	//g_object_set_data( G_OBJECT (window), "INETJSONSCANCONTROL_EC_list", EC_list);

	g_object_set_data( G_OBJECT (window), "PAC_EC_READINGS_list", EC_R_list);

        set_window_geometry ("inet-json-rp-control"); // needs rescoure entry and defines window menu entry as geometry is managed

	inet_json_external_scandata_pi.app->RemoteEntryList = g_slist_concat (inet_json_external_scandata_pi.app->RemoteEntryList, bp->remote_list_ec);

        
        // hookup to scan start and stop
        inet_json_external_scandata_pi.app->ConnectPluginToStartScanEvent (Inet_Json_External_Scandata::scan_start_callback);
        inet_json_external_scandata_pi.app->ConnectPluginToStopScanEvent (Inet_Json_External_Scandata::scan_stop_callback);
}

Inet_Json_External_Scandata::~Inet_Json_External_Scandata (){
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

void Inet_Json_External_Scandata::scan_start_callback (gpointer user_data){
        //Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        Inet_Json_External_Scandata *self = inet_json_external_scandata;
        self->ch_freq = -1;
        self->ch_ampl = -1;
        self->streaming = 1;
        //self->operation_mode = 0;
        g_message ("Inet_Json_External_Scandata::scan_start_callback");
#if 0
        if ((self->ch_freq=main_get_gapp()->xsm->FindChan(xsmres.extchno[0])) >= 0)
                self->setup_scan (self->ch_freq, "X+", "Ext1-Freq", "Hz", "Freq", 1.0);
        if ((self->ch_ampl=main_get_gapp()->xsm->FindChan(xsmres.extchno[1])) >= 0)
                self->setup_scan (self->ch_ampl, "X+", "Ext1-Ampl", "V", "Ampl", 1.0);
#endif
}

void Inet_Json_External_Scandata::scan_stop_callback (gpointer user_data){
        //Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        Inet_Json_External_Scandata *self = inet_json_external_scandata;
        self->ch_freq = -1;
        self->ch_ampl = -1;
        self->streaming = 0;
        g_message ("Inet_Json_External_Scandata::scan_stop_callback");
}

int Inet_Json_External_Scandata::setup_scan (int ch, 
				 const gchar *titleprefix, 
				 const gchar *name,
				 const gchar *unit,
				 const gchar *label,
				 double d2u
	){
#if 0
	// did this scan already exists?
	if ( ! main_get_gapp()->xsm->scan[ch]){ // make a new one ?
		main_get_gapp()->xsm->scan[ch] = main_get_gapp()->xsm->NewScan (main_get_gapp()->xsm->ChannelView[ch], 
							  main_get_gapp()->xsm->data.display.ViewFlg, 
							  ch, 
							  &main_get_gapp()->xsm->data);
		// Error ?
		if (!main_get_gapp()->xsm->scan[ch]){
			XSM_SHOW_ALERT (ERR_SORRY, ERR_NOMEM,"",1);
			return FALSE;
		}
	}


	Mem2d *m=main_get_gapp()->xsm->scan[ch]->mem2d;
        m->Resize (m->GetNx (), m->GetNy (), m->GetNv (), ZD_DOUBLE, false); // multilayerinfo=clean
	
	// Setup correct Z unit
	UnitObj *u = main_get_gapp()->xsm->MakeUnit (unit, label);
	main_get_gapp()->xsm->scan[ch]->data.SetZUnit (u);
	delete u;
		
        main_get_gapp()->xsm->scan[ch]->create (TRUE, FALSE, strchr (titleprefix, '-') ? -1.:1., main_get_gapp()->xsm->hardware->IsFastScan ());

	// setup dz from instrument definition or propagated via signal definition
	if (fabs (d2u) > 0.)
		main_get_gapp()->xsm->scan[ch]->data.s.dz = d2u;
	else
		main_get_gapp()->xsm->scan[ch]->data.s.dz = main_get_gapp()->xsm->Inst->ZResolution (unit);
	
	// set scan title, name, ... and draw it!

	gchar *scantitle = NULL;
	if (!main_get_gapp()->xsm->GetMasterScan ()){
		main_get_gapp()->xsm->SetMasterScan (main_get_gapp()->xsm->scan[ch]);
		scantitle = g_strdup_printf ("M %s %s", titleprefix, name);
	} else {
		scantitle = g_strdup_printf ("%s %s", titleprefix, name);
	}
	main_get_gapp()->xsm->scan[ch]->data.ui.SetName (scantitle);
	main_get_gapp()->xsm->scan[ch]->data.ui.SetOriginalName ("unknown");
	main_get_gapp()->xsm->scan[ch]->data.ui.SetTitle (scantitle);
	main_get_gapp()->xsm->scan[ch]->data.ui.SetType (scantitle);
	main_get_gapp()->xsm->scan[ch]->data.s.xdir = strchr (titleprefix, '-') ? -1.:1.;
	main_get_gapp()->xsm->scan[ch]->data.s.ydir = main_get_gapp()->xsm->data.s.ydir;

        streampos=x=y=0; // assume top down full size
        
	PI_DEBUG (DBG_L2, "setup_scan[" << ch << " ]: scantitle done: " << main_get_gapp()->xsm->scan[ch]->data.ui.type ); 

	g_free (scantitle);
	main_get_gapp()->xsm->scan[ch]->draw ();
#endif
        
	return 0;
}

void Inet_Json_External_Scandata::pac_tau_transport_changed (Param_Control* pcs, gpointer user_data){
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        self->write_parameter ("TRANSPORT_TAU_DFREQ", self->parameters.transport_tau[0]); // negative = disabled
        self->write_parameter ("TRANSPORT_TAU_PHASE", self->parameters.transport_tau[1]);
        self->write_parameter ("TRANSPORT_TAU_EXEC",  self->parameters.transport_tau[2]);
        self->write_parameter ("TRANSPORT_TAU_AMPL",  self->parameters.transport_tau[3]);
}

void Inet_Json_External_Scandata::pac_tau_parameter_changed (Param_Control* pcs, gpointer user_data){
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        self->write_parameter ("PAC_DCTAU", self->parameters.pac_dctau);
        self->write_parameter ("PACTAU", self->parameters.pactau);
        self->write_parameter ("PACATAU", self->parameters.pacatau);
}

void Inet_Json_External_Scandata::pac_frequency_parameter_changed (Param_Control* pcs, gpointer user_data){
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        self->write_parameter ("FREQUENCY_MANUAL", self->parameters.frequency_manual, "%.4f"); //, TRUE);
        self->write_parameter ("FREQUENCY_CENTER", self->parameters.frequency_center, "%.4f"); //, TRUE);
        self->write_parameter ("AUX_SCALE", self->parameters.aux_scale);
}

void Inet_Json_External_Scandata::pac_volume_parameter_changed (Param_Control* pcs, gpointer user_data){
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        self->write_parameter ("VOLUME_MANUAL", self->parameters.volume_manual);
}

static void freeze_ec(Gtk_EntryControl* ec, gpointer data){ ec->Freeze (); };
static void thaw_ec(Gtk_EntryControl* ec, gpointer data){ ec->Thaw (); };

void Inet_Json_External_Scandata::show_dF_control (GtkWidget *widget, Inet_Json_External_Scandata *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                gtk_widget_show (self->dF_control_frame);
        else
                gtk_widget_hide (self->dF_control_frame);
}

void Inet_Json_External_Scandata::show_pulse_control (GtkWidget *widget, Inet_Json_External_Scandata *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                gtk_widget_show (self->pulse_control_frame);
        else
                gtk_widget_hide (self->pulse_control_frame);
}

void Inet_Json_External_Scandata::select_pac_lck_amplitude (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->write_parameter ("LCK_AMPLITUDE", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
}
void Inet_Json_External_Scandata::select_pac_lck_phase (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->write_parameter ("LCK_PHASE", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
}

void Inet_Json_External_Scandata::qcontrol (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->write_parameter ("QCONTROL", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.qcontrol = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));

        if (self->parameters.qcontrol)
                g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "QC_SETTINGS_list"),
				(GFunc) thaw_ec, NULL);
        else
                g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "QC_SETTINGS_list"),
				(GFunc) freeze_ec, NULL);
}

void Inet_Json_External_Scandata::qc_parameter_changed (Param_Control* pcs, gpointer user_data){
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        self->write_parameter ("QC_GAIN", self->parameters.qc_gain);
        self->write_parameter ("QC_PHASE", self->parameters.qc_phase);
}

void Inet_Json_External_Scandata::tune_parameter_changed (Param_Control* pcs, gpointer user_data){
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        self->write_parameter ("TUNE_DFREQ", self->parameters.tune_dfreq);
        self->write_parameter ("TUNE_SPAN", self->parameters.tune_span);
}

void Inet_Json_External_Scandata::amp_ctrl_parameter_changed (Param_Control* pcs, gpointer user_data){
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        self->write_parameter ("AMPLITUDE_FB_SETPOINT", self->parameters.amplitude_fb_setpoint);
        self->write_parameter ("EXEC_FB_UPPER", self->parameters.exec_fb_upper);
        self->write_parameter ("EXEC_FB_LOWER", self->parameters.exec_fb_lower);
}

void Inet_Json_External_Scandata::phase_ctrl_parameter_changed (Param_Control* pcs, gpointer user_data){
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        self->write_parameter ("PHASE_FB_SETPOINT", self->parameters.phase_fb_setpoint);
        self->write_parameter ("FREQ_FB_UPPER", self->parameters.freq_fb_upper);
        self->write_parameter ("FREQ_FB_LOWER", self->parameters.freq_fb_lower);
        self->write_parameter ("PHASE_HOLD_AM_NOISE_LIMIT", self->parameters.phase_hold_am_noise_limit);
}

void Inet_Json_External_Scandata::dfreq_ctrl_parameter_changed (Param_Control* pcs, gpointer user_data){
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        self->write_parameter ("DFREQ_FB_SETPOINT", self->parameters.dfreq_fb_setpoint);
        self->write_parameter ("DFREQ_FB_UPPER", self->parameters.control_dfreq_fb_upper);
        self->write_parameter ("DFREQ_FB_LOWER", self->parameters.control_dfreq_fb_lower);
}

void Inet_Json_External_Scandata::pulse_form_parameter_changed (Param_Control* pcs, gpointer user_data){
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
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

        if (self->parameters.pulse_form_enable){
                self->write_parameter ("PULSE_FORM_HEIGHT0", self->parameters.pulse_form_height0);
                self->write_parameter ("PULSE_FORM_HEIGHT1", self->parameters.pulse_form_height1);
                self->write_parameter ("PULSE_FORM_HEIGHT0IF", self->parameters.pulse_form_height0if);
                self->write_parameter ("PULSE_FORM_HEIGHT1IF", self->parameters.pulse_form_height1if);
        } else {
                self->write_parameter ("PULSE_FORM_HEIGHT0", 0.0);
                self->write_parameter ("PULSE_FORM_HEIGHT1", 0.0);
                self->write_parameter ("PULSE_FORM_HEIGHT0IF", 0.0);
                self->write_parameter ("PULSE_FORM_HEIGHT1IF", 0.0);
        }
}

void Inet_Json_External_Scandata::pulse_form_enable (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->parameters.pulse_form_enable = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
        self->pulse_form_parameter_changed (NULL, self);
}

void Inet_Json_External_Scandata::save_values (NcFile *ncf){
        // store all Inet_Json_External_Scandata's control parameters for the RP PAC-PLL
        // if socket connection is up
        if (client){
                // JSON READ BACK PARAMETERS! Loosing precison to float some where!! (PHASE_CI < -100dB => 0!!!)
                for (JSON_parameter *p=PACPLL_JSON_parameters; p->js_varname; ++p){
                        gchar *vn = g_strdup_printf ("JSON_RedPACPALL_%s", p->js_varname);
                        NcVar* ncv = ncf->add_var ( vn, ncDouble);
                        ncv->add_att ("long_name", vn);
                        ncv->add_att ("short_name", p->js_varname);
                        ncv->put (p->value);
                }
                // ACTUAL PARAMETERS in "dB" DOUBLE PRECISION VALUES, getting OK to the FPGA...
                {
                        gchar *vn = g_strdup_printf ("JSON_RedPACPALL_%s", "_PHASE_FB_CP");
                        NcVar* ncv = ncf->add_var ( vn, ncDouble);
                        ncv->add_att ("long_name", vn);
                        ncv->add_att ("short_name", "_PHASE_FB_CP");
                        ncv->add_att ("var_unit", "1");
                        ncv->put (&parameters.phase_fb_cp); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("JSON_RedPACPALL_%s", "_PHASE_FB_CI");
                        NcVar* ncv = ncf->add_var ( vn, ncDouble);
                        ncv->add_att ("long_name", vn);
                        ncv->add_att ("short_name", "_PHASE_FB_CI");
                        ncv->add_att ("var_unit", "1");
                        ncv->put (&parameters.phase_fb_ci); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("JSON_RedPACPALL_%s", "_AMPLITUDE_FB_CP");
                        NcVar* ncv = ncf->add_var ( vn, ncDouble);
                        ncv->add_att ("long_name", vn);
                        ncv->add_att ("short_name", "_AMPLITUDE_FB_CP");
                        ncv->add_att ("var_unit", "1");
                        ncv->put (&parameters.amplitude_fb_cp); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("JSON_RedPACPALL_%s", "_AMPLITUDE_FB_CI");
                        NcVar* ncv = ncf->add_var ( vn, ncDouble);
                        ncv->add_att ("long_name", vn);
                        ncv->add_att ("short_name", "_AMPLITUDE_FB_CI");
                        ncv->add_att ("var_unit", "1");
                        ncv->put (&parameters.amplitude_fb_ci); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("JSON_RedPACPALL_%s", "_PHASE_FB_CP_dB");
                        NcVar* ncv = ncf->add_var ( vn, ncDouble);
                        ncv->add_att ("long_name", vn);
                        ncv->add_att ("short_name", "_PHASE_FB_CP_dB");
                        ncv->add_att ("var_unit", "1");
                        ncv->put (&parameters.phase_fb_cp_db); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("JSON_RedPACPALL_%s", "_PHASE_FB_CI_dB");
                        NcVar* ncv = ncf->add_var ( vn, ncDouble);
                        ncv->add_att ("long_name", vn);
                        ncv->add_att ("short_name", "_PHASE_FB_CI_dB");
                        ncv->add_att ("var_unit", "dB");
                        ncv->put (&parameters.phase_fb_ci_db); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("JSON_RedPACPALL_%s", "_AMPLITUDE_FB_CP_dB");
                        NcVar* ncv = ncf->add_var ( vn, ncDouble);
                        ncv->add_att ("long_name", vn);
                        ncv->add_att ("short_name", "_AMPLITUDE_FB_CP_dB");
                        ncv->add_att ("var_unit", "1");
                        ncv->put (&parameters.amplitude_fb_cp_db); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("JSON_RedPACPALL_%s", "_AMPLITUDE_FB_CI_dB");
                        NcVar* ncv = ncf->add_var ( vn, ncDouble);
                        ncv->add_att ("long_name", vn);
                        ncv->add_att ("short_name", "_AMPLITUDE_FB_CI_dB");
                        ncv->add_att ("var_unit", "dB");
                        ncv->put (&parameters.amplitude_fb_ci_db); // OK!
                }
        }
}

void Inet_Json_External_Scandata::send_all_parameters (){
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

void Inet_Json_External_Scandata::choice_operation_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->operation_mode = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->write_parameter ("OPERATION", self->operation_mode);
        self->write_parameter ("PACVERBOSE", (int)self->rp_verbose_level);
}

void Inet_Json_External_Scandata::choice_update_ts_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
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

void Inet_Json_External_Scandata::choice_transport_ch12_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->channel_selections[0] = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->channel_selections[1] = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) > 0){
                int i=gtk_combo_box_get_active (GTK_COMBO_BOX (widget))-1;
                if (i<0) i=0;
                self->write_parameter ("TRANSPORT_MODE", self->transport=i);
        }
}

void Inet_Json_External_Scandata::choice_trigger_mode_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->write_parameter ("BRAM_SCOPE_TRIGGER_MODE", gtk_combo_box_get_active (GTK_COMBO_BOX (widget)));
        self->write_parameter ("BRAM_SCOPE_TRIGGER_POS", (int)(0.1*1024));
}

void Inet_Json_External_Scandata::choice_auto_set_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
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

void Inet_Json_External_Scandata::scope_buffer_position_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->write_parameter ("BRAM_SCOPE_SHIFT_POINTS", self->bram_shift = (int)gtk_range_get_value (GTK_RANGE (widget)));
}


void Inet_Json_External_Scandata::choice_transport_ch3_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->channel_selections[2] = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) > 0)
                self->write_parameter ("TRANSPORT_CH3", gtk_combo_box_get_active (GTK_COMBO_BOX (widget))-1);
}

void Inet_Json_External_Scandata::choice_transport_ch4_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->channel_selections[3] = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) > 0)
                self->write_parameter ("TRANSPORT_CH4", gtk_combo_box_get_active (GTK_COMBO_BOX (widget))-1);
}

void Inet_Json_External_Scandata::choice_transport_ch5_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->channel_selections[4] = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) > 0)
                self->write_parameter ("TRANSPORT_CH5", gtk_combo_box_get_active (GTK_COMBO_BOX (widget))-1);
}

void Inet_Json_External_Scandata::set_ss_auto_trigger (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->write_parameter ("SET_SINGLESHOT_TRANSPORT_TRIGGER", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
}

void Inet_Json_External_Scandata::amplitude_gain_changed (Param_Control* pcs, gpointer user_data){
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        self->parameters.amplitude_fb_cp = self->parameters.amplitude_fb_invert * pow (10., self->parameters.amplitude_fb_cp_db/20.);
        self->parameters.amplitude_fb_ci = self->parameters.amplitude_fb_invert * pow (10., self->parameters.amplitude_fb_ci_db/20.);
        self->write_parameter ("AMPLITUDE_FB_CP", self->parameters.amplitude_fb_cp);
        self->write_parameter ("AMPLITUDE_FB_CI", self->parameters.amplitude_fb_ci);
}

void Inet_Json_External_Scandata::amplitude_controller_invert (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->parameters.amplitude_fb_invert = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)) ? -1.:1.;
        self->amplitude_gain_changed (NULL, self);
}

void Inet_Json_External_Scandata::amplitude_controller (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->write_parameter ("AMPLITUDE_CONTROLLER", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.amplitude_controller = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void Inet_Json_External_Scandata::phase_gain_changed (Param_Control* pcs, gpointer user_data){
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        self->parameters.phase_fb_cp = self->parameters.phase_fb_invert * pow (10., self->parameters.phase_fb_cp_db/20.);
        self->parameters.phase_fb_ci = self->parameters.phase_fb_invert * pow (10., self->parameters.phase_fb_ci_db/20.);
        // g_message("PH_CICP=%g, %g", self->parameters.phase_fb_ci, self->parameters.phase_fb_cp );
        self->write_parameter ("PHASE_FB_CP", self->parameters.phase_fb_cp);
        self->write_parameter ("PHASE_FB_CI", self->parameters.phase_fb_ci);
}

void Inet_Json_External_Scandata::phase_controller_invert (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->parameters.phase_fb_invert = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)) ? -1.:1.;
        self->phase_gain_changed (NULL, self);
}

void Inet_Json_External_Scandata::phase_controller (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->write_parameter ("PHASE_CONTROLLER", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.phase_controller = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void Inet_Json_External_Scandata::phase_unwrapping_always (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->write_parameter ("PHASE_UNWRAPPING_ALWAYS", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.phase_unwrapping_always = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void Inet_Json_External_Scandata::phase_unwrap_plot (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->unwrap_phase_plot = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}


void Inet_Json_External_Scandata::dfreq_gain_changed (Param_Control* pcs, gpointer user_data){
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        self->parameters.dfreq_fb_cp = self->parameters.dfreq_fb_invert * pow (10., self->parameters.dfreq_fb_cp_db/20.);
        self->parameters.dfreq_fb_ci = self->parameters.dfreq_fb_invert * pow (10., self->parameters.dfreq_fb_ci_db/20.);
        // g_message("DF_CICP=%g, %g", self->parameters.dfreq_fb_ci, self->parameters.dfreq_fb_ci );
        self->write_parameter ("DFREQ_FB_CP", self->parameters.dfreq_fb_cp);
        self->write_parameter ("DFREQ_FB_CI", self->parameters.dfreq_fb_ci);
}

void Inet_Json_External_Scandata::dfreq_controller_invert (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->parameters.dfreq_fb_invert = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)) ? -1.:1.;
        self->dfreq_gain_changed (NULL, self);
}

void Inet_Json_External_Scandata::dfreq_controller (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->write_parameter ("DFREQ_CONTROLLER", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.dfreq_controller = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void Inet_Json_External_Scandata::update(){
	if (G_IS_OBJECT (window))
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "INETJSONSCANCONTROL_EC_list"),
				(GFunc) App::update_ec, NULL);
}

void Inet_Json_External_Scandata::update_monitoring_parameters(){

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

        gchar *delta_freq_info = g_strdup_printf ("[%g]", parameters.dds_frequency_monitor - parameters.frequency_center);
        input_ddsfreq->set_info (delta_freq_info);
        g_free (delta_freq_info);
        if (G_IS_OBJECT (window))
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "PAC_EC_READINGS_list"),
				(GFunc) App::update_ec, NULL);
}

void Inet_Json_External_Scandata::enable_scope (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->run_scope = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void Inet_Json_External_Scandata::dbg_l1 (GtkWidget *widget, Inet_Json_External_Scandata *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                self->debug_level |= 1;
        else
                self->debug_level &= ~1;
}
void Inet_Json_External_Scandata::dbg_l2 (GtkWidget *widget, Inet_Json_External_Scandata *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                self->debug_level |= 2;
        else
                self->debug_level &= ~2;
}
void Inet_Json_External_Scandata::dbg_l4 (GtkWidget *widget, Inet_Json_External_Scandata *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                self->debug_level |= 4;
        else
                self->debug_level &= ~4;
}


void Inet_Json_External_Scandata::scope_ac_ch1_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->scope_ac[0] = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}
void Inet_Json_External_Scandata::scope_ac_ch2_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->scope_ac[1] = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}
void Inet_Json_External_Scandata::scope_ac_ch3_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->scope_ac[2] = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void Inet_Json_External_Scandata::scope_xy_on_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->scope_xy_on = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void Inet_Json_External_Scandata::scope_fft_on_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
        self->scope_fft_on = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}
void Inet_Json_External_Scandata::scope_z_ch1_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
        double z[] = { 0.001, 0.003, 0.01, 0.03, 0.1, 0.3, 1.0, 3.0, 10., 30.0, 100., 300.0, 1000., 3000.0, 10e3, 30e3, 100e3, 1., 0., 0. };
        int i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->scope_z[0] = z[i];
}
void Inet_Json_External_Scandata::scope_z_ch2_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
        double z[] = { 0.001, 0.003, 0.01, 0.03, 0.1, 0.3, 1.0, 3.0, 10., 30.0, 100., 300.0, 1000., 3000.0, 10e3, 30e3, 100e3, 1., 0., 0. };
        int i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->scope_z[1] = z[i];
}
void Inet_Json_External_Scandata::scope_fft_time_zoom_callback (GtkWidget *widget, Inet_Json_External_Scandata *self){
        double z[] = { 1.0, 2.0, 5.0, 10.0, 20.0, 50.0, 100., 1., 0., 0. };
        int i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->scope_fft_time_zoom = z[i];
}

void Inet_Json_External_Scandata::connect_cb (GtkWidget *widget, Inet_Json_External_Scandata *self){
        if (!self->text_status) return;
        if (!self->input_rpaddress) return;
        self->debug_log (gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (self->input_rpaddress)))));

        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))){
                self->status_append ("Connecting to RedPitaya...\n");

                self->update_health ("Connecting...");

                // new soup session
                self->session = soup_session_new ();

                // request to fire up RedPitaya PACPLLL NGNIX server
                gchar *urlstart = g_strdup_printf ("http://%s/bazaar?start=pacpll", gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (self->input_rpaddress)))));
                self->status_append ("1. Requesting NGNIX RedPitaya PACPLL Server Startup:\n");
                self->status_append (urlstart);
                self->status_append ("\n ");
                self->msg = soup_message_new ("GET", urlstart);
                g_free (urlstart);
                GInputStream *istream = soup_session_send (self->session, self->msg, NULL, &self->error);

                if (self->error != NULL) {
                        g_warning ("%s", self->error->message);
                        self->status_append (self->error->message);
                        self->status_append ("\n ");
                        self->update_health (self->error->message);
                        return;
                } else {
                        gchar *buffer = g_new0 (gchar, 100);
                        gssize num = g_input_stream_read (istream,
                                                          (void *)buffer,
                                                          100,
                                                          NULL,
                                                          &self->error);   
                        if (self->error != NULL) {
                                self->update_health (self->error->message);
                                g_warning ("%s", self->error->message);
                                self->status_append (self->error->message);
                                self->status_append ("\n ");
                                g_free (buffer);
                                return;
                        } else {
                                self->status_append ("Response: ");
                                self->status_append (buffer);
                                self->status_append ("\n ");
                                self->update_health (buffer);
                        }
                        g_free (buffer);
                }

                // then connect to NGNIX WebSocket on RP
                self->status_append ("2. Connecting to NGNIX RedPitaya PACPLL WebSocket...\n");
                gchar *url = g_strdup_printf ("ws://%s:%u", gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (self->input_rpaddress)))), self->port);
                self->status_append (url);
                self->status_append ("\n");
                // g_message ("Connecting to: %s", url);
                
                self->msg = soup_message_new ("GET", url);
                g_free (url);
                // g_message ("soup_message_new - OK");
                soup_session_websocket_connect_async (self->session, self->msg, // SoupSession *session, SoupMessage *msg,
                                                      NULL, NULL, // const char *origin, char **protocols,
                                                      NULL, Inet_Json_External_Scandata::got_client_connection, self); // GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data
                //g_message ("soup_session_websocket_connect_async - OK");
        } else {
                // tear down connection
                self->status_append ("Dissconnecting...\n ");
                self->update_health ("Dissconnected");

                //g_clear_object (&self->listener);
                g_clear_object (&self->client);
                g_clear_error (&self->client_error);
                g_clear_error (&self->error);
        }
}

void Inet_Json_External_Scandata::got_client_connection (GObject *object, GAsyncResult *result, gpointer user_data){
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        g_message ("got_client_connection");

	self->client = soup_session_websocket_connect_finish (SOUP_SESSION (object), result, &self->client_error);
        if (self->client_error != NULL) {
                self->status_append ("Connection failed: ");
                self->status_append (self->client_error->message);
                self->status_append ("\n");
                g_message ("%s", self->client_error->message);
        } else {
                self->status_append ("RedPitaya WebSocket Connected!\n ");
		g_signal_connect(self->client, "closed",  G_CALLBACK(Inet_Json_External_Scandata::on_closed),  self);
		g_signal_connect(self->client, "message", G_CALLBACK(Inet_Json_External_Scandata::on_message), self);
		//g_signal_connect(connection, "closing", G_CALLBACK(on_closing_send_message), message);
                self->status_append ("RedPitaya PAC-PLL loading configuration.\n ");
                self->send_all_parameters ();
                self->status_append ("RedPitaya PAC-PLL init, DEC FAST(12)...\n");
                gtk_combo_box_set_active (GTK_COMBO_BOX (self->update_tr_widget), 6);  // select Ph,dF,Am,Ex
                gtk_combo_box_set_active (GTK_COMBO_BOX (self->update_ts_widget), 12); // select 12, fast
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
                usleep(500000);
                gtk_combo_box_set_active (GTK_COMBO_BOX (self->update_op_widget), 3); // INIT BRAM TRANSPORT AND CLEAR FIR RING BUFFERS, give me a second...
                self->status_append ("RedPitaya PAC-PLL init, INIT-FIR... [2s Zzzz]\n");
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
                usleep(2000000);
                self->status_append ("RedPitaya PAC-PLL init, INIT-FIR completed.\n");
                usleep(1000000);
                self->status_append ("RedPitaya PAC-PLL normal operation, set to data streaming mode.\n");
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
                usleep(500000);
                gtk_combo_box_set_active (GTK_COMBO_BOX (self->update_op_widget), 5); // STREAMING OPERATION
                gtk_combo_box_set_active (GTK_COMBO_BOX (self->update_ts_widget), 18); // select 19 (typical scan decimation/time scale filter)
                self->status_append ("RedPitaya PAC-PLL is ready.\n ");
        }
}

void Inet_Json_External_Scandata::on_message(SoupWebsocketConnection *ws,
                                             SoupWebsocketDataType type,
                                             GBytes *message,
                                             gpointer user_data)
{
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
	gconstpointer contents;
	gsize len;
        gchar *tmp;
        
        self->debug_log ("WebSocket message received.");
        
	if (type == SOUP_WEBSOCKET_DATA_TEXT) {
		contents = g_bytes_get_data (message, &len);
		self->status_append ("WEBSOCKET_DATA_TEXT\n");
		self->status_append ((gchar*)contents);
		self->status_append ("\n");
                g_message ("%s", (gchar*)contents);
	} else if (type == SOUP_WEBSOCKET_DATA_BINARY) {
		contents = g_bytes_get_data (message, &len);

                tmp = g_strdup_printf ("WEBSOCKET_DATA_BINARY NGNIX JSON ZBytes: %ld", len);
                self->debug_log (tmp);
                g_free (tmp);

#if 0
                // dump to file
                FILE *f;
                f = fopen ("/tmp/gxsm-rp-json.gz","wb");
                fwrite (contents, len, 1, f);
                fclose (f);
                // -----------
                //$ pzahl@phenom:~$ zcat /tmp/gxsm-rp-json.gz 
                //{"parameters":{"DC_OFFSET":{"value":-18.508743,"min":-1000,"max":1000,"access_mode":0,"fpga_update":0},"CPU_LOAD":{"value":5.660378,"min":0,"max":100,"access_mode":0,"fpga_update":0},"COUNTER":{"value":4,"min":0,"max":1000000000000,"access_mode":0,"fpga_update":0}}}pzahl@phenom:~$ 
                //$ pzahl@phenom:~$ file /tmp/gxsm-rp-json.gz 
                // /tmp/gxsm-rp-json.gz: gzip compressed data, max speed, from FAT filesystem (MS-DOS, OS/2, NT)
                // GZIP:  zlib.MAX_WBITS|16
#endif
                self->debug_log ("Uncompressing...");
                gsize size=len*100+1000;
                gchar *json_buffer = g_new0 (gchar, size);

                // inflate buffer into json_buffer
                z_stream zInfo ={0};
                zInfo.total_in  = zInfo.avail_in  = len;
                zInfo.total_out = zInfo.avail_out = size;
                zInfo.next_in  = (Bytef*)contents;
                zInfo.next_out = (Bytef*)json_buffer;
      
                int ret= -1;
                ret = inflateInit2 (&zInfo, MAX_WBITS + 16);
                if ( ret == Z_OK ) {
                        ret = inflate( &zInfo, Z_FINISH );     // zlib function
                        // inflate() returns
                        // Z_OK if some progress has been made (more input processed or more output produced),
                        // Z_STREAM_END if the end of the compressed data has been reached and all uncompressed output has been produced,
                        // Z_NEED_DICT if a preset dictionary is needed at this point,
                        // Z_DATA_ERROR if the input data was corrupted (input stream not conforming to the zlib format or incorrect check value, in which case strm->msg points to a string with a more specific error),
                        // Z_STREAM_ERROR if the stream structure was inconsistent (for example next_in or next_out was Z_NULL, or the state was inadvertently written over by the application),
                        // Z_MEM_ERROR if there was not enough memory,
                        // Z_BUF_ERROR if no progress was possible or if there was not enough room in the output buffer when Z_FINISH is used. Note that Z_BUF_ERROR is not fatal, and inflate() can be called again with more input and more output space to continue decompressing. If
                        // Z_DATA_ERROR is returned, the application may then call inflateSync() to look for a good compression block if a partial recovery of the data is to be attempted. 
                        switch ( ret ){
                        case Z_STREAM_END:
                                tmp = NULL;
                                if (self->debug_level > 2)
                                        tmp = g_strdup_printf ("Z_STREAM_END out = %ld, in = %ld, ratio=%g\n",zInfo.total_out, zInfo.total_in, (double)zInfo.total_out / (double)zInfo.total_in);
                                break;
                        case Z_OK:
                                tmp = g_strdup_printf ("Z_OK out = %ld, in = %ld\n",zInfo.total_out, zInfo.total_in); break;
                        case Z_NEED_DICT:
                                tmp = g_strdup_printf ("Z_NEED_DICT out = %ld, in = %ld\n",zInfo.total_out, zInfo.total_in); break;
                        case Z_DATA_ERROR:
                                self->status_append (zInfo.msg);
                                tmp = g_strdup_printf ("\nZ_DATA_ERROR out = %ld, in = %ld\n",zInfo.total_out, zInfo.total_in); break; 
                                break;
                        case Z_STREAM_ERROR:
                                tmp = g_strdup_printf ("Z_STREAM_ERROR out = %ld\n",zInfo.total_out); break;
                        case Z_MEM_ERROR:
                                tmp = g_strdup_printf ("Z_MEM_ERROR out = %ld, in = %ld\n",zInfo.total_out, zInfo.total_in); break;
                        case Z_BUF_ERROR:
                                tmp = g_strdup_printf ("Z_BUF_ERROR out = %ld, in = %ld  ratio=%g\n",zInfo.total_out, zInfo.total_in, (double)zInfo.total_out / (double)zInfo.total_in); break;
                        default:
                                tmp = g_strdup_printf ("ERROR ?? inflate result = %d,  out = %ld, in = %ld\n",ret,zInfo.total_out, zInfo.total_in); break;
                        }
                        self->status_append (tmp);
                        g_free (tmp);
                }
                inflateEnd( &zInfo );   // zlib function
                if (self->debug_level > 0){
                        self->status_append (json_buffer);
                        self->status_append ("\n");
                }

                self->json_parse_message (json_buffer);

                g_free (json_buffer);

                self->update_monitoring_parameters();
                gtk_widget_queue_draw (self->signal_graph_area);

                //self->stream_data ();
                self->update_health ();
        }
	//g_bytes_unref (message); // OK, no unref by ourself!!!!
                       
}

void Inet_Json_External_Scandata::on_closed (SoupWebsocketConnection *ws, gpointer user_data){
        Inet_Json_External_Scandata *self = (Inet_Json_External_Scandata *)user_data;
        self->status_append ("WebSocket connection externally closed.\n");
}

void Inet_Json_External_Scandata::json_parse_message (const char *json_string){
        jsmn_parser p;
        jsmntok_t tok[10000]; /* We expect no more than 10000 tokens, signal array is 1024 * 5*/

        // typial data messages:
        // {"signals":{"SIGNAL_CH3":{"size":1024,"value":[0,0,...,0.543632,0.550415]},"SIGNAL_CH4":{"size":1024,"value":[0,0,... ,-94.156487]},"SIGNAL_CH5":{"size":1024,"value":[0,0,.. ,-91.376022,-94.156487]}}}
        // {"parameters":{"DC_OFFSET":{"value":-18.591045,"min":-1000,"max":1000,"access_mode":0,"fpga_update":0},"COUNTER":{"value":2.4,"min":0,"max":1000000000000,"access_mode":0,"fpga_update":0}}}

        jsmn_init(&p);
        int ret = jsmn_parse(&p, json_string, strlen(json_string), tok, sizeof(tok)/sizeof(tok[0]));
        if (ret < 0) {
                g_warning ("JSON PARSER:  Failed to parse JSON: %d\n%s\n", ret, json_string);
                return;
        }
        /* Assume the top-level element is an object */
        if (ret < 1 || tok[0].type != JSMN_OBJECT) {
                g_warning("JSON PARSER:  Object expected\n");
                return;
        }

#if 0
        json_dump (json_string, tok, p.toknext, 0);
#endif

        json_fetch (json_string, tok, p.toknext, 0);
        if  (debug_level > 1)
                dump_parameters (debug_level);
}



void Inet_Json_External_Scandata::write_parameter (const gchar *paramater_id, double value, const gchar *fmt, gboolean dbg){
        //soup_websocket_connection_send_text (self->client, "text");
        //soup_websocket_connection_send_binary (self->client, gconstpointer data, gsize length);
        //soup_websocket_connection_send_text (client, "{ \"parameters\":{\"GAIN1\":{\"value\":200.0}}}");

        if (client){
                gchar *json_string=NULL;
                if (fmt){
                        gchar *format = g_strdup_printf ("{ \"parameters\":{\"%s\":{\"value\":%s}}}", paramater_id, fmt);
                        json_string = g_strdup_printf (format, value);
                        g_free (format);
                } else
                        json_string = g_strdup_printf ("{ \"parameters\":{\"%s\":{\"value\":%g}}}", paramater_id, value);
                soup_websocket_connection_send_text (client, json_string);
                if  (debug_level > 0 || dbg)
                        g_print ("%s\n",json_string);
                g_free (json_string);
        }
}

void Inet_Json_External_Scandata::write_parameter (const gchar *paramater_id, int value, gboolean dbg){
        if (client){
                gchar *json_string = g_strdup_printf ("{ \"parameters\":{\"%s\":{\"value\":%d}}}", paramater_id, value);
                soup_websocket_connection_send_text (client, json_string);
                if  (debug_level > 0 || dbg)
                        g_print ("%s\n",json_string);
                g_free (json_string);
        }
}

void Inet_Json_External_Scandata::update_health (const gchar *msg){
        if (msg){
                gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY((red_pitaya_health)))), msg, -1);
        } else {
                gchar *gpiox_string = NULL;
                if (scope_width_points > 1000){
                        gpiox_string = g_strdup_printf ("1[%08x %08x, %08x %08x] 5[%08x %08x, %08x %08x] 9[%08x %08x, %08x %08x] 13[%08x %08x, %08x %08x]",
                                                        (int)pacpll_signals.signal_gpiox[0], (int)pacpll_signals.signal_gpiox[1],
                                                        (int)pacpll_signals.signal_gpiox[2], (int)pacpll_signals.signal_gpiox[3],
                                                        (int)pacpll_signals.signal_gpiox[4], (int)pacpll_signals.signal_gpiox[5],
                                                        (int)pacpll_signals.signal_gpiox[6], (int)pacpll_signals.signal_gpiox[7],
                                                        (int)pacpll_signals.signal_gpiox[8], (int)pacpll_signals.signal_gpiox[9],
                                                        (int)pacpll_signals.signal_gpiox[10], (int)pacpll_signals.signal_gpiox[11],
                                                        (int)pacpll_signals.signal_gpiox[12], (int)pacpll_signals.signal_gpiox[13],
                                                        (int)pacpll_signals.signal_gpiox[14], (int)pacpll_signals.signal_gpiox[15]
                                                        );
                }
                gchar *health_string = g_strdup_printf ("CPU: %03.0f%% Free: %6.1f MB %s #%g", pacpll_parameters.cpu_load, pacpll_parameters.free_ram/1024/1024, gpiox_string?gpiox_string:"[]", pacpll_parameters.counter);
                gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY((red_pitaya_health)))), health_string, -1);
                g_free (health_string);
                g_free (gpiox_string);
        }
}

void Inet_Json_External_Scandata::status_append (const gchar *msg){

	GtkTextBuffer *console_buf;
	GtkTextView *textview;
	GString *output;
	GtkTextMark *end_mark;
        GtkTextIter start_iter, end_trim_iter, end_iter;
        gint lines, max_lines=20*debug_level+10;

	if (!msg) {
		if (debug_level > 4)
                        g_warning("No message to append");
		return;
	}

	// read string which contain last command output
	console_buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW(text_status));
	gtk_text_buffer_get_bounds (console_buf, &start_iter, &end_iter);

	// get output widget content
	output = g_string_new (gtk_text_buffer_get_text (console_buf,
                                                         &start_iter, &end_iter,
                                                         FALSE));

	// append input line
	output = g_string_append (output, msg);
	gtk_text_buffer_set_text (console_buf, output->str, -1);
	g_string_free (output, TRUE);

        gtk_text_buffer_get_start_iter (console_buf, &start_iter);
        lines = gtk_text_buffer_get_line_count (console_buf);
        if (lines > max_lines){
                gtk_text_buffer_get_iter_at_line_index (console_buf, &end_trim_iter, lines-max_lines, 0);
                gtk_text_buffer_delete (console_buf,  &start_iter,  &end_trim_iter);
        }
        
	// scroll to end
	gtk_text_buffer_get_end_iter (console_buf, &end_iter);
	end_mark = gtk_text_buffer_create_mark (console_buf, "cursor", &end_iter,
                                                FALSE);
	g_object_ref (end_mark);
	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (text_status),
                                      end_mark, 0.0, FALSE, 0.0, 0.0);
	g_object_unref (end_mark);
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

void Inet_Json_External_Scandata::stream_data (){
#if 0
        int deci=16;
        int n=4;
        if (data_shr_max > 2) { //if (ch_freq >= 0 || ch_ampl >= 0){
                double decimation = 125e6 * main_get_gapp()->xsm->hardware->GetScanrate ();
                deci = (gint64)decimation;
                if (deci > 2){
                        for (n=0; deci; ++n) deci >>= 1;
                        --n;
                }
                if (n>data_shr_max) n=data_shr_max; // limit to 24. Note: (32 bits - 8 control, may shorten control, only 3 needed)
                deci = 1<<n;
                //g_print ("Scan Pixel rate is %g s/pix -> Decimation %g -> %d n=%d\n", main_get_gapp()->xsm->hardware->GetScanrate (), decimation, deci, n);
        }
        if (deci != data_decimation){
                data_decimation = deci;
                data_shr = n;
                write_parameter ("SHR_DEC_DATA", data_shr);
                write_parameter ("TRANSPORT_DECIMATION", data_decimation);
        }

        if (ch_freq >= 0 || ch_ampl >= 0){
                if (x < main_get_gapp()->xsm->scan[ch_freq]->mem2d->GetNx () &&
                    y < main_get_gapp()->xsm->scan[ch_freq]->mem2d->GetNy ()){
                        for (int i=0; i<1000; ++i) {
                                if (ch_freq >= 0)
                                        main_get_gapp()->xsm->scan[ch_freq]->mem2d->PutDataPkt (parameters.freq_fb_lower + pacpll_signals.signal_ch2[i], x,y);
                                if (ch_ampl >= 0)
                                        main_get_gapp()->xsm->scan[ch_ampl]->mem2d->PutDataPkt (pacpll_signals.signal_ch1[i], x,y);
                                ++x;
                                if (x >= main_get_gapp()->xsm->scan[ch_freq]->mem2d->GetNx ()) {x=0; ++y; };
                                if (y >= main_get_gapp()->xsm->scan[ch_freq]->mem2d->GetNy ()) break;
                        }
                        streampos += 1024;
                        main_get_gapp()->xsm->scan[ch_freq]->draw ();
                }
        }
#endif
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

                
        // prepare data for fftw
#if 0
        double s=2.*M_PI/(n-1);
        double a0=0.54;
        double a1=1.-a0;
        int n2=n/2;
        for (int i = 0; i < n; ++i){
                double w=a0+a1*cos((i-n2)*s);
                in[i] = w*data[i];
        }
#else
        //double mx=0., mi=0.;
        for (int i = 0; i < n; ++i){
                in[i] = blackman_nuttall_window (i, data[i]);
                //if (i==0) mi=mx=in[i];
                //if (in[i] > mx) mx=in[i];
                //if (in[i] < mi) mi=in[i];
        }
        //g_print("FFT in Min: %g Max: %g\n",mi,mx);
#endif
        //g_print("FFTin %g",in[0]);
                
        // compute transform
        fftw_execute (plan);

        //double N=2*(n-1);
        //double scale = 1./(max*2*(n-1));
        double scale = M_PI*2/n;
        double mag=0.;
        for (int i=0; i<n/2; ++i){
                mag = scale * sqrt(c_re(out[i])*c_re(out[i]) + c_im(out[i])*c_im(out[i]));
                //psd_db[i] = gfloat((1.-mu)*(double)psd_db[i] + mu*20.*log(mag));
                psd[i] = gfloat((1.-mu)*(double)psd[i] + mu*mag);
                //if (i==0) mi=mx=mag;
                //if (mag > mx) mx=mag;
                //if (mag < mi) mi=mag;
        }
        //g_print("FFT out Min: %g Max: %g\n",mi,mx);
        return 0;
}

double Inet_Json_External_Scandata::unwrap (int k, double phi){
        static int pk=0;
        static int side=1;
        static double pphi;
        
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
        return (phi);
}

// to force udpate call:   gtk_widget_queue_draw (self->signal_graph_area);
void Inet_Json_External_Scandata::graph_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                                       int             width,
                                                       int             height,
                                                       Inet_Json_External_Scandata *self){
        self->dynamic_graph_draw_function (area, cr, width, height);
}

void Inet_Json_External_Scandata::dynamic_graph_draw_function (GtkDrawingArea *area, cairo_t *cr, int width, int height){
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

                const gchar *ch01_lab[2][10] = { {"IN1", "IN1-AC", "Ampl", "dPhase", "Phase", "Phase", "-", "-", "DBG", NULL },
                                                 {"IN2", "IN1-DC", "Exec", "dFreq ", "Ampl ", "dFreq", "-", "-", "DBG", NULL } }; 
                
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
                                        else if ( channel_selections[0]==9 && ch < 2){
                                                wave->set_xy_fast (k, x,-yr*(gain_scale[ch]>0.?gain_scale[ch]:1.)*signal[ch][k]), tic_lin=1; // else: linear scale with gain
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
