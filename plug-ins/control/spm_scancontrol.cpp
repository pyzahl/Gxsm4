/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: spm_scancontrol.C
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
% PlugInDocuCaption: SPM Scan Control
% PlugInName: spm_scancontrol
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: windows-sectionSPM Scan Control

% PlugInDescription
Provides a SPM Scan Control Window and connects to the \Gxsm\ toolbar
buttons \GxsmEmph{Scan, Movie, Stop} for quick access. The control
panel offers in addition to \GxsmEmph{Scan, Movie, Stop} a
\GxsmEmph{Pause} and \GxsmEmph{HS Capture} button. Use the
\GxsmEmph{Pause} button to pause the scanning process and press it
again for continuation.  (PCI32 only)

The \GxsmEmph{HS Capture} button starts a continuous (movie) high
speed (HS) frame capturing process on the DSP, a refresh is done after
the whole scan data is received from the DSP. The scan size is limited
by the available memory (SRAM) on the DSP platform. The \GxsmEmph{HS
Capture} mode assures precise real time inter line timing on DSP level
and allows maximum frame rates due to minimized communication between
Gxsm and the DSP. (applies to PCI32 only)

Using this panel it is now possible to set the scan Y direction from
\GxsmEmph{TopDown} (default, from Top to Bottom) to
\GxsmEmph{TopDownBotUp} (alternating from Top to Bottom and vice
versa) or \GxsmEmph{BotUp} (Botton to Top).

If the \GxsmEmph{Repeat} option is checked, scanning will repeat until released
(at scan end) or stop is pressed to cancel scanning at any time.
Files are automatically saved if the  \GxsmEmph{AutoSave} check-button in the main window is checked.

Using the \GxsmEmph{Movie} mode, no single frames are saved while
scanning, but the whole movie is appended into the time-dimension of
the scan (this need sufficient amount of memory, as \Gxsm keeps
all data in memory.). Use the Movie-Control/Player window to play
it. If stopped, the last frame will be incomplete, you can truncate it using the
\GxsmMenu{Edit/Copy} tool and copy all but the last
frame. Multidimensional data is saved into the NetCDF file.

This Gxsm PlugIn module actually provides not only the the scanning
controls, it does the whole job of high-level data acquisitation/sorting itself.

% PlugInUsage
Used for advanced SPM data acquisitation control, open the control
panel via \GxsmMenu{windows-sectionSPM Scan Control}.  

\GxsmNote{The \GxsmEmph{HS Capture} scan mode is always in
\GxsmEmph{TopDown} mode and no \GxsmEmph{Pause} will be accepted, use
\GxsmEmph{Stop}. This feature is implemented for the PCI32 only (old) 
and is obsolete for all newer HWIs using direct data streaming (FIFO).}

\GxsmScreenShot{SPMScanControl}{The SPM Scan Control window.}

% OptPlugInRefs
%The internal used fast fourier transform is based on the FFTW library:\\
%\GxsmWebLink{www.fftw.org}\\
%Especially here:\\
%\GxsmWebLink{www.fftw.org/doc/fftw\_2.html\#SEC5}\\
About pthreads hacking, this is object of future use:\\
\GxsmWebLink{java.icmc.sc.usp.br/library/books/ibm\_pthreads/}

% OptPlugInNotes
In \GxsmEmph{TopDownBotUp} mode always the last scan direction is
remembered (even if the scan was stopped in between) and the opposide
scan direction is used at next \GxsmEmph{Scan Start}.

% OptPlugInHints
You can switch the scan direction mode while running a movie or single
scan, it will be used as soon as the next scan is started!

\GxsmNote{Hacker notes: This plugin is responsible for the high level scanning
process, background/idle display refresh. It does the initial scan creation and
setup of all data types due to the configuration as provided by the user via preferences.
This setup need to match the current hardware configuration.}

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "unit.h"
#include "pcs.h"
#include "xsmtypes.h"
#include "glbvars.h"
#include "action_id.h"

#include "plug-ins/control/spm_scancontrol.h"

// Plugin Prototypes - default PlugIn functions
static void spm_scancontrol_init (void); // PlugIn init
static void spm_scancontrol_query (void); // PlugIn "self-install"
static void spm_scancontrol_about (void); // About
static void spm_scancontrol_configure (void); // Configure plugIn, called via PlugIn-Configurator
static void spm_scancontrol_cleanup (void); // called on PlugIn unload, should cleanup PlugIn rescources

// other PlugIn Functions and Callbacks (connected to Buttons, Toolbar, Menu)
static void spm_scancontrol_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data); // show ScanControl Window
static void spm_scancontrol_start_callback (GtkWidget *w, void *data); // called on start scan
static void spm_scancontrol_movie_callback (GtkWidget *w, void *data); // called on movie start
static void spm_scancontrol_hscapture_callback (GtkWidget *w, void *data); // called on high speed capture start
static void spm_scancontrol_pause_callback (GtkWidget *w, void *data); // called on pause/unpause
static void spm_scancontrol_stop_callback (GtkWidget *w, void *data); // called on scan stop
static void spm_scancontrol_set_subscan_callback (GtkWidget *w, void *data); // called on scan stop
static void spm_scancontrol_SaveValues_callback ( gpointer );

// Fill in the GxsmPlugin Description here -- see also: Gxsm/src/plugin.h
GxsmPlugin spm_scancontrol_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	"Spm_ScanControl",
	//  "-rhkspmHARD +spmHARD +STM +AFM +SARLS +SNOM",                // PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
	//"+ALL +noHARD +Demo-HwI:SPMHARD +SRanger:SPMHARD +SRangerMK2:SPMHARD +SRangerTest:SPMHARD +Innovative_DSP:SPMHARD +Innovative_DSP:SPAHARD +TC211-CCDHARD +video4linuxHARD +Comedi:SPMHARD +kmdsp:SPMHARD -LAN_RHK:SPMHARD",
	NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	g_strdup ("SPM scan control"),
	"Percy Zahl",
	"windows-section",
	N_("SPM Scan Control"),
	N_("open SPM Scan Control Window"),
	"SPM Scan Control Window and Scan Generator PlugIn",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	spm_scancontrol_init,  
	spm_scancontrol_query,  
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	spm_scancontrol_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	spm_scancontrol_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL,
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
	spm_scancontrol_show_callback, // direct menu entry callback1 or NULL
	NULL, // direct menu entry callback2 or NULL

	spm_scancontrol_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm SPM Scan Generator and Control Plugin\n\n"
                                   "This plugin manages the SPM scanning process\n"
				   "and multichannel/layer dataaquisitation."
	);

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
        PI_DEBUG_GM (DBG_L2, "SPM_ScanControl - get_gxsm_plugin_info");
	spm_scancontrol_pi.description = g_strdup_printf(N_("Gxsm spm_scancontrol plugin %s"), VERSION);
	return &spm_scancontrol_pi; 
}


SPM_ScanControl *spm_scancontrol = NULL;

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/abc import"
// Export Menupath is "File/Export/abc import"
// ----------------------------------------------------------------------
// !!!! make sure the "spm_scancontrol_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!

// Add Menu Entries:
// windows-sectionSPM Scan Control
// Action/SPM Scan Start/Pause/Stop + Toolbar

#define REMOTE_PREFIX "SPMC-"

static void spm_scancontrol_query(void)
{
	if(spm_scancontrol_pi.status) g_free(spm_scancontrol_pi.status); 
	spm_scancontrol_pi.status = g_strconcat (
                                                 N_("Plugin query has attached "),
                                                 spm_scancontrol_pi.name, 
                                                 N_(": File IO Filters are ready to use"),
                                                 NULL);
	
	PI_DEBUG_GM (DBG_L2, "spm_scancontrol_query:new" );
	spm_scancontrol = new SPM_ScanControl (main_get_gapp() -> get_app ());
	
	PI_DEBUG_GM (DBG_L2, "spm_scancontrol_query:res" );
	
	spm_scancontrol_pi.app->ConnectPluginToCDFSaveEvent (spm_scancontrol_SaveValues_callback);

	PI_DEBUG_GM (DBG_L2, "spm_scancontrol_query:done" );
}

static void spm_scancontrol_SaveValues_callback ( gpointer gp_ncf ){
	gchar *tmp = NULL;
	if( spm_scancontrol )
		tmp = strdup (spm_scancontrol->GetScanDir()==1 ? "TopDown " : "BottomUp");
	else
		tmp = strdup ("N/A");

	NcFile *ncf = (NcFile *) gp_ncf;
	NcDim* spmscd  = ncf->add_dim("spm_scancontrol_dim", strlen(tmp));
	NcVar* spmsc   = ncf->add_var("spm_scancontrol", ncChar, spmscd);
	spmsc->add_att("long_name", "spm_scancontrol: scan direction");
	spmsc->put(tmp, strlen(tmp));
	g_free (tmp);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void spm_scancontrol_init(void)
{
  PI_DEBUG_GM (DBG_L2, "spm_scancontrol Plugin Init" );
}

// about-Function
static void spm_scancontrol_about(void)
{
  const gchar *authors[] = { spm_scancontrol_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  spm_scancontrol_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void spm_scancontrol_configure(void)
{
	if(spm_scancontrol_pi.app)
		spm_scancontrol_pi.app->message("spm_scancontrol Plugin Configuration");
}

// cleanup-Function, make sure the Menustrings are matching those above!!!
static void spm_scancontrol_cleanup(void)
{
	// delete ...
	if( spm_scancontrol )
		delete spm_scancontrol ;

	if(spm_scancontrol_pi.status) g_free(spm_scancontrol_pi.status); 
}

static void spm_scancontrol_show_callback(GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	PI_DEBUG_GM (DBG_L2, "spm_scancontrol Plugin : show" );
	if( spm_scancontrol )
		spm_scancontrol->show();
}

static void cb_setscandir( GtkWidget *widget, SPM_ScanControl *scc ){
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) 
		scc->SetScanDir (widget);
	else
		scc->ClrScanDir (widget);
}

static void cb_repeat_mode( GtkWidget *widget, SPM_ScanControl *scc ){
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) 
		scc->SetRepeatMode (TRUE);
	else
		scc->SetRepeatMode (FALSE);
}

static void cb_mvolt_mode( GtkWidget *widget, SPM_ScanControl *scc ){
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) 
		scc->SetMultiVoltMode (TRUE);
	else
		scc->SetMultiVoltMode (FALSE);
}

static void cb_mvolt_generate_list( GtkWidget *widget, SPM_ScanControl *scc ){
	scc->compute_mvolt_list (GTK_WIDGET (g_object_get_data( G_OBJECT (widget), "GRID_SETUP")));
}

SPM_ScanControl::SPM_ScanControl (Gxsm4app *app):AppBase(app)
{
	GtkWidget *button,*label,*bs_tmp, *bm_tmp;
        
	XsmRescourceManager xrm("SPMScanControl");

	PI_DEBUG_GM (DBG_L2, "spm_scancontrol Plugin : building interface" );

	master_scan      = NULL;
	master_probescan = NULL;
	keep_multi_layer_info = FALSE;

        all_scan_list    = NULL;
        xp_scan_list     = NULL;
	xp_2nd_scan_list = NULL;
	xp_prbscan_list  = NULL;
	xm_scan_list     = NULL;
	xm_2nd_scan_list = NULL;
	xm_prbscan_list  = NULL;

	scan_flag     = SCAN_FLAG_READY;
	scan_dir      = SCAN_DIR_TOPDOWN;
	last_scan_dir = SCAN_DIR_TOPDOWN;
	do_probe = FALSE;
	
	Unity    = new UnitObj(" "," ");
	Volt     = new UnitObj("V","V");

	gint tmp;
	xrm.Get("RepeatFlg", &tmp, "0");
	SetRepeatMode (tmp ? TRUE:FALSE);
	SetMovieMode (FALSE);

	xrm.Get("MultiVoltFlg", &tmp, "0");
	SetMultiVoltMode (tmp ? TRUE:FALSE);

	xrm.Get("MultiVolt_Start", &mv_start, "-2.");
	xrm.Get("MultiVolt_Gap", &mv_gap, "0.2");
	xrm.Get("MultiVolt_End", &mv_end, "2.");
	xrm.Get("MultiVolt_number", &multi_volt_number, "10");
	multi_volt_list   = NULL;

// SPM Scan Control Window Title
	PI_DEBUG_GM (DBG_L2, "spm_scancontrol Plugin : building interface -- AppWindowInit" );
	AppWindowInit("SPM Scan Control");
        spmsc_bp = new BuildParam (v_grid);
        gtk_widget_show (v_grid);
        
        //grid = v_grid;
        
// SPM Control Button Frame
	// --------------------------------------------------
        spmsc_bp->new_grid_with_frame (N_("SPM Scan Control"));
        bs_tmp = scan_start_button = spmsc_bp->grid_add_button ("Start", "Start Scanning", 1, G_CALLBACK (spm_scancontrol_start_callback), this);
	main_get_gapp()->RegisterPluginToolbarButton (G_OBJECT (scan_start_button), "Toolbar_Scan_Start");

        bm_tmp = spmsc_bp->grid_add_button ("Movie", "Start Movie Scan Mode", 1, G_CALLBACK (spm_scancontrol_movie_callback), this);
        g_object_set_data( G_OBJECT (bs_tmp), "SPMCONTROL_MOVIE_BUTTON", bm_tmp);
        g_object_set_data( G_OBJECT (bm_tmp), "SPMCONTROL_START_BUTTON", bm_tmp);
	main_get_gapp()->RegisterPluginToolbarButton (G_OBJECT (bm_tmp), "Toolbar_Scan_Movie");

        GtkWidget *slsbutton = spmsc_bp->grid_add_button ("SLS", "Sub Line Scan Mode", 1, G_CALLBACK (spm_scancontrol_set_subscan_callback), this);
  	g_object_set_data( G_OBJECT (slsbutton), "SUBSCAN_MODE", GINT_TO_POINTER (0));
        g_object_set_data( G_OBJECT (bs_tmp), "SPMCONTROL_SLS_BUTTON", slsbutton);
        g_object_set_data( G_OBJECT (bm_tmp), "SPMCONTROL_SLS_BUTTON", slsbutton);
	main_get_gapp()->RegisterPluginToolbarButton (G_OBJECT (slsbutton), "Toolbar_SubLineScan");

        GtkWidget *pbutton = spmsc_bp->grid_add_button ("Pause", "Toggle Pause Scan", 1, G_CALLBACK (spm_scancontrol_pause_callback), this);
	main_get_gapp()->RegisterPluginToolbarButton (G_OBJECT (pbutton), "Toolbar_Scan_Pause");

        GtkWidget *sbutton = spmsc_bp->grid_add_button ("Stop", "Stop Scan", 1, G_CALLBACK (spm_scancontrol_stop_callback), this);
	main_get_gapp()->RegisterPluginToolbarButton (G_OBJECT (sbutton), "Toolbar_Scan_Stop");

        // Scan Direction Controls
	spmsc_bp->new_line ();

        spmsc_bp->grid_add_radio_button ("Top-Down", "Set top down scanning",
                                         G_CALLBACK (cb_setscandir), this,
                                         true,
                                         "SCANDIR", (void*) SCAN_DIR_TOPDOWN);
        spmsc_bp->grid_add_radio_button ("Up-Down", "Set top down <-> bottom up Scanning",
                                         G_CALLBACK (cb_setscandir), this,
                                         false,
                                         "SCANDIR", (void*) SCAN_DIR_TOPDOWN_BOTUP);
        spmsc_bp->grid_add_radio_button ("Bot-!Up", "Set bottom up scanning",
                                         G_CALLBACK (cb_setscandir), this,
                                         false,
                                         "SCANDIR", (void*) SCAN_DIR_BOTUP);

        // Repeat, Multi (Volt,...), SubScan - Mode selections
	spmsc_bp->new_line ();

        spmsc_bp->grid_add_check_button (N_("Repeat"),
                                         N_("Auto Restart Scan after completion. "
                                            PYREMOTE_CHECK_HOOK_KEY("SCAN-REPEAT")
                                            "\nWARNING:\n Script triggered scan start will\n"
                                            "never go beyond that scan start comand\n and repeats as long as this is checked.") , 1,
                                         G_CALLBACK (cb_repeat_mode), this,
                                         RepeatMode ());
        
        spmsc_bp->grid_add_check_button (N_("Multi Layer"), N_("Multi Layer/Paramater Scan Mode"), 1,
                                         G_CALLBACK (cb_mvolt_mode), this,
                                         MultiVoltMode ());

        GtkWidget *slscheckbutton = spmsc_bp->grid_add_check_button (N_("SubScan"), N_("Scanning is set to Sub Scan Mode if enabled."), 1,
                                                                     G_CALLBACK (spm_scancontrol_set_subscan_callback), this);
        g_object_set_data (G_OBJECT (slscheckbutton), "SUBSCAN_MODE", GINT_TO_POINTER (1));
	g_object_set_data (G_OBJECT (slsbutton), "SUBSCAN_RB", slscheckbutton);
        g_object_set_data (G_OBJECT (bs_tmp), "SPMCONTROL_SLS_MODE", slscheckbutton);
        g_object_set_data (G_OBJECT (bm_tmp), "SPMCONTROL_SLS_MODE", slscheckbutton);

        GtkWidget *radcheckbutton = spmsc_bp->grid_add_check_button ("RAD", N_("Remap Available (previous) Data at scan start to new geometry."));
        g_object_set_data( G_OBJECT (scan_start_button), "SPMCONTROL_RAD_START_MODE_CB", radcheckbutton);

   //------------ SLS controls
        spmsc_bp->pop_grid ();
	spmsc_bp->new_line ();
        spmsc_bp->new_grid_with_frame (N_("Sub Line Scan Setup"));
        g_object_set_data (G_OBJECT (slscheckbutton), "SPMCONTROL_SLS_FRAME", spmsc_bp->frame);
        sls_mode = FALSE;
        sls_config[0] = sls_config[1] = sls_config[2] = sls_config[3] = 0;

        //SPMC_RemoteEntryList = ec->AddEntry2RemoteList(REMOTE_PREFIX "SLS_Xs", SPMC_RemoteEntryList);

        spmsc_bp->grid_add_ec ("SLS Xs", Unity, &sls_config[0], 0, 99999, ".0f", REMOTE_PREFIX "sls-xs");
        g_object_set_data (G_OBJECT (bs_tmp), "SPMCONTROL_SLS_XS", spmsc_bp->input);
        g_object_set_data (G_OBJECT (slsbutton), "SLSC0", spmsc_bp->ec);
        g_object_set_data (G_OBJECT (slscheckbutton), "SLSC0", spmsc_bp->ec);

        spmsc_bp->grid_add_ec ("SLS Xn", Unity, &sls_config[1], 0, 99999, ".0f", REMOTE_PREFIX "sls-xn");
        g_object_set_data (G_OBJECT (bs_tmp), "SPMCONTROL_SLS_XN", spmsc_bp->input);
        g_object_set_data (G_OBJECT (slsbutton), "SLSC1", spmsc_bp->ec);
        g_object_set_data (G_OBJECT (slscheckbutton), "SLSC1", spmsc_bp->ec);

	spmsc_bp->new_line ();
        spmsc_bp->grid_add_ec ("SLS Ys", Unity, &sls_config[2], 0, 99999, ".0f", REMOTE_PREFIX "sls-ys");
        g_object_set_data (G_OBJECT (bs_tmp), "SPMCONTROL_SLS_YS", spmsc_bp->input);
        g_object_set_data (G_OBJECT (slsbutton), "SLSC2", spmsc_bp->ec);
        g_object_set_data (G_OBJECT (slscheckbutton), "SLSC2", spmsc_bp->ec);

        spmsc_bp->grid_add_ec ("SLS Yn", Unity, &sls_config[3], 0, 99999, ".0f", REMOTE_PREFIX "sls-yn");
        g_object_set_data (G_OBJECT (bs_tmp), "SPMCONTROL_SLS_YN", spmsc_bp->input);
        g_object_set_data (G_OBJECT (slsbutton), "SLSC3", spmsc_bp->ec);
        g_object_set_data (G_OBJECT (slscheckbutton), "SLSC3", spmsc_bp->ec);
        
        // ==>  SPMC_RemoteEntryList

        // Reset SLS at startup
        sls_mode = FALSE;
        sls_config[0] = sls_config[1] = sls_config[2] = sls_config[3] = 0;
        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (slscheckbutton), "SLSC0")) -> Put_Value ();
        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (slscheckbutton), "SLSC1")) -> Put_Value ();
        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (slscheckbutton), "SLSC2")) -> Put_Value ();
        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (slscheckbutton), "SLSC3")) -> Put_Value ();

	spm_scancontrol_pi.app->RemoteEntryList = g_slist_concat (spm_scancontrol_pi.app->RemoteEntryList, spmsc_bp->get_remote_list_head ());

        // Multi-Volt Setup Frame
        spmsc_bp->pop_grid ();
	spmsc_bp->new_line ();
        spmsc_bp->new_grid_with_frame_with_scrolled_contents (N_("Multi Layer Scan Setup"));
        spmsc_bp->set_xy (1, -10);

	remote_param = spmsc_bp->grid_add_input(N_("Target Param."), 3);
        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(remote_param))), "dsp-fbs-bias", -1);
	spmsc_bp->new_line ();

        spmsc_bp->grid_add_label (N_("* Match unit of measure to target"));
	spmsc_bp->new_line ();

        spmsc_bp->set_no_spin (false);
        spmsc_bp->grid_add_ec (N_("# Values to use"), Unity, &multi_volt_number, 1, 999, "3.0f");
        g_object_set_data( G_OBJECT (bs_tmp), "SPMCONTROL_EC_NUM_VALUES", spmsc_bp->input);
                  
	spmsc_bp->new_line ();
        spmsc_bp->grid_add_ec (N_("MV Start"), Unity, &mv_start, -1e3, 1e3, "6.3f", 0.1, 0.5);
	spmsc_bp->new_line ();
        spmsc_bp->grid_add_ec (N_("MV Gap"), Unity, &mv_gap, 0, 10, "6.3f", 0.1, 0.5);
	spmsc_bp->new_line ();
        spmsc_bp->grid_add_ec (N_("MV End"), Unity, &mv_end, -1e3, 1e3, "6.3f", 0.1, 0.5);
        spmsc_bp->set_no_spin (true);

	spmsc_bp->new_line ();
        GtkWidget *gen_button = spmsc_bp->grid_add_button (N_("Generate List"), N_("Calculate List"), 2,
                                                           G_CALLBACK (cb_mvolt_generate_list), this);
	g_object_set_data( G_OBJECT (gen_button), "GRID_SETUP", spmsc_bp->grid);
	spmsc_bp->new_line ();

        spmsc_bp->set_xy (1, 0);
        //multi_volt_number=1;
	for (int i=0; i<multi_volt_number; ++i){
		MultiVoltEntry *mve = new MultiVoltEntry (spmsc_bp, Unity, i);
		multi_volt_list = g_slist_append (multi_volt_list, mve);
	}
        // save List away...
	g_object_set_data( G_OBJECT (window), "SPMCONTROL_EC_list", spmsc_bp->get_ec_list_head ());
        
	PI_DEBUG_GM (DBG_L2, "spm_scancontrol Plugin : interface done." );

        set_window_geometry ("spm-scan-control");
}

// ToDo:
//------> use gchar* main_get_gapp()->GetPluginData() to get data [n x ..]
// main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_Scan_SetYLookup"); [2 d g]
// main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_Scan_Line"); [1 d]
// main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_Scan_PartialLine"); [3 d d d]
// main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_Scan_UpdateParam");
// main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_Scan_Init");
// main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_Scan_Start_Add");
// main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_Scan_Movie_Add");

static void mve_remove (gpointer mve, gpointer data){ delete (MultiVoltEntry*)mve; };

void SPM_ScanControl::compute_mvolt_list (GtkWidget *grid) {
	guint i, l, gn=0;
	
	double dv = (mv_end - mv_start) / (multi_volt_number-1);

	PI_DEBUG_GM (DBG_L2, "spm_scancontrol Plugin : compute_mvolt_list" );

	l = g_slist_length(multi_volt_list);
	for (i=0; i<multi_volt_number; ++i){
		double v = mv_start + (double)i * dv;
		if (fabs(mv_gap) > 0. && fabs(v) < (mv_gap+fabs(dv))){
			double i0  = -mv_start/dv;
			double igp = ( mv_gap-mv_start)/dv;
			double igm = (-mv_gap-mv_start)/dv;
			double igw = 0.5*(1+igp-igm);
			double vv  = -(i-i0)*fabs(dv)/igw;
			if (v>0.)
				v =  mv_gap - vv;
			else
				v = -mv_gap - vv;
		}
		if (i<l){
			MultiVoltEntry *mve = (MultiVoltEntry*) g_slist_nth_data (multi_volt_list, i);
			if (mve)
				mve->volt (v);
			else 
				PI_DEBUG_ERROR (DBG_L1, "MVC Error#" << i << ": list entry not valid.");
		} else {
			MultiVoltEntry *mve = new MultiVoltEntry (spmsc_bp, Unity, i, v);
			multi_volt_list = g_slist_append (multi_volt_list, mve);
		}
	}

	for (; i<l; ++i){
		MultiVoltEntry *mve = (MultiVoltEntry*) g_slist_nth_data (multi_volt_list, i);
		if (mve){
			mve->set_inactive ();
		}
	}

	gtk_widget_show (grid);

	PI_DEBUG_GM (DBG_L2, "spm_scancontrol Plugin : compute_mvolt_list done." );

}

SPM_ScanControl::~SPM_ScanControl (){

	XsmRescourceManager xrm("SPMScanControl");

	PI_DEBUG_GM (DBG_L2, "SPM_ScanControl save resources and clean up");

	xrm.Put("RepeatFlg", RepeatMode ());
	xrm.Put("MultiVoltFlg", MultiVoltMode ());
	xrm.Put("MultiVolt_Start", mv_start);
	xrm.Put("MultiVolt_Gap", mv_gap);
	xrm.Put("MultiVolt_End", mv_end);
	xrm.Put("MultiVolt_number", multi_volt_number);

//	std::cout << "SPM_ScanControl - unregister from Toolbar" << std::endl;
	main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_Scan_Start");
	main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_Scan_Movie");
	main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_Scan_Pause");
	main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_Scan_Stop");
#ifdef ADD_SLS_CONTROL
	main_get_gapp()->RegisterPluginToolbarButton (NULL, "Toolbar_SubLineScan");
#endif


//	std::cout << "SPM_ScanControl - cleanup scanlists" << std::endl;
	// clean up
	free_scan_lists ();

	// clean up only
	line = -1; do_scanline (TRUE); // init call to clear all srcs references!

//	std::cout << "SPM_ScanControl - cleanup multivolt" << std::endl;

	// clean up
//  --> this is causing trouble, seams to kill some widget reference ?!?!? do not understand, do not delete now.
//	g_slist_foreach (multi_volt_list, (GFunc) mve_remove, NULL);
	g_slist_free (multi_volt_list);
	multi_volt_list = NULL;

//	std::cout << "SPM_ScanControl - cleanup units" << std::endl;
	delete Unity;
	delete Volt;
//	std::cout << "SPM_ScanControl - cleanup done." << std::endl;
}

//	main_get_gapp()->xsm->hardware->SetScanMode(MEM_ADDTO);

void SPM_ScanControl::update(){
	if (G_IS_OBJECT (window))
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "SPMCONTROL_EC_list"),
				(GFunc) App::update_ec, NULL);
}

// Menu Call Back Fkte

/* stolen from app_remote.C */
static void via_remote_list_Check_ec(Gtk_EntryControl* ec, remote_args* ra){
	ec->CheckRemoteCmd (ra);
};

static void spm_scancontrol_start_callback (GtkWidget *w, void *data){
	if (((SPM_ScanControl*)data) ->  scan_in_progress()){
		((SPM_ScanControl*)data) -> resume_scan (); // in case paused, resume now!
		return;
	}

        ((SPM_ScanControl*)data) -> SetMovieMode (FALSE);

        ((SPM_ScanControl*)data) -> wdata = w;
        //g_timeout_add (100, SPM_ScanControl::spm_scancontrol_run_scans_task, data);
        g_idle_add (SPM_ScanControl::spm_scancontrol_run_scans_task, data);
}

gboolean SPM_ScanControl::spm_scancontrol_run_scans_task (gpointer data){
        static int runmode=0;
	static guint i=0, l=0;
        static gpointer ss_action = NULL;
        static time_t t0, t; // Scan - Startzeit eintragen 
        static gint64 task_t_last=g_get_monotonic_time ();
        static guint scan_image_update_interval=50;

        if(main_get_pi_debug_level() > DBG_L3){
                gint64 tmp = g_get_monotonic_time ();
                PI_DEBUG_GM (DBG_L3, "SPM_SCANCONTROL::spm_scancontrol_run_scans_task dt=%d, time=%d, runmode=%d", tmp-task_t_last, tmp, runmode);
                task_t_last = tmp; 
        }
        
        GtkWidget *w = ((SPM_ScanControl*)data) -> wdata;
        //g_message ("SCAN RUN %d", runmode);
        switch (runmode){
        case 0: 
                {
                        GVariant *update_interval = g_settings_get_value (gapp->get_app_settings (), "scan-update-interval");
                        scan_image_update_interval = (guint)g_variant_get_int32 (update_interval);
                        if (scan_image_update_interval < 10 || scan_image_update_interval > 1000)
                                scan_image_update_interval = 55;
                }
                gtk_widget_set_sensitive (w, FALSE);
                gtk_widget_set_sensitive ((GtkWidget*)g_object_get_data( G_OBJECT (w), "SPMCONTROL_EC_NUM_VALUES"), FALSE);
                gtk_widget_set_sensitive ((GtkWidget*)g_object_get_data( G_OBJECT (w), "SPMCONTROL_MOVIE_BUTTON"), FALSE);
                gtk_widget_set_sensitive ((GtkWidget*)g_object_get_data( G_OBJECT (w), "SPMCONTROL_SLS_BUTTON"), FALSE);
                gtk_widget_set_sensitive ((GtkWidget*)g_object_get_data( G_OBJECT (w), "SPMCONTROL_SLS_MODE"), FALSE);
                ss_action = g_object_get_data (G_OBJECT (w), "simple-action");
                if (ss_action)
                        g_simple_action_set_enabled ((GSimpleAction*)ss_action, FALSE);

                if (((SPM_ScanControl*)data) -> MovieMode ()) {
                        main_get_gapp()->xsm->FileCounterInc ();
                        main_get_gapp()->xsm->ResetVPFileCounter ();
                        main_get_gapp()->xsm->auto_append_in_time (-1.); // negative time means free up all time elemenst of scans selected for movie
                        time(&t0);
                        
                        runmode=11;
                } else {
                        runmode=10; // normal single scan or scan with repeat
                }
                i=0;
                return TRUE;

        case 10:
                main_get_gapp()->xsm->FileCounterInc ();
                main_get_gapp()->xsm->ResetVPFileCounter ();

		if (((SPM_ScanControl*)data) -> MultiVoltMode())
			main_get_gapp()->xsm->data.s.nvalues = l = ((SPM_ScanControl*)data) -> MultiVoltNumber();
		else
			main_get_gapp()->xsm->data.s.nvalues = 1;

		((SPM_ScanControl*)data) -> keep_multi_layer_info = FALSE;
		main_get_gapp()->xsm->data.ui.SetDateOfScanNow();
		main_get_gapp()->spm_update_all();
		main_get_gapp()->xsm->hardware->SetScanMode();
		
                runmode = 99; // in case of error, reset
		if (((SPM_ScanControl*)data) -> MultiVoltMode()){
			if (i<l){
				double value = ((SPM_ScanControl*)data) -> MultiVoltFromList(i);
				
				remote_args ra;
				ra.qvalue = 0.;
				gchar *list[] = { g_strdup ("set"), 
                                                  g_strdup (gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(((SPM_ScanControl*)data) ->remote_param))))),
						  g_strdup_printf ("%.4f", value), 
						  NULL
				};
				ra.arglist  = list;
				g_slist_foreach (main_get_gapp()->RemoteEntryList, (GFunc) via_remote_list_Check_ec, (gpointer) &ra);

				for (int k=0; list[k]; ++k) g_free (list[k]);

				if (!((SPM_ScanControl*)data) -> setup_scanning_control(i))
                                        return TRUE;
				((SPM_ScanControl*)data) -> keep_multi_layer_info = TRUE;
                                ++i;
			}
		} else {
			if (!((SPM_ScanControl*)data) -> setup_scanning_control())
                                return TRUE;
		}
                runmode = 20;
                return TRUE;
        case 11:
                main_get_gapp()->xsm->data.s.nvalues = 1;
		time(&t);
		PI_DEBUG_GM (DBG_L2, "SPM_ScanControl - movie frame time: %ul ", t);

		((SPM_ScanControl*)data) -> keep_multi_layer_info = FALSE;
		main_get_gapp()->xsm->data.ui.SetDateOfScanNow();
		main_get_gapp()->spm_update_all();
		main_get_gapp()->xsm->hardware->SetScanMode();
		
                runmode = 99; // in case of error, reset
                if (!((SPM_ScanControl*)data) -> setup_scanning_control())
                        return TRUE;

		main_get_gapp()->xsm->data.s.ntimes = main_get_gapp()->xsm->auto_append_in_time ((double)(t - t0));

                runmode = 20;
                return TRUE;
        case 20:
                SPM_ScanControl::scanning_task (data); // actual scanning "setup, monitoring and update" task

                if (((SPM_ScanControl*)data) -> scanning_task_stage == 0){ // competed?
                        runmode = 30;
                        g_idle_add (SPM_ScanControl::spm_scancontrol_run_scans_task, data);
                } else {
                        g_timeout_add (scan_image_update_interval, SPM_ScanControl::spm_scancontrol_run_scans_task, data); // throttle to 50ms
                }

                return G_SOURCE_REMOVE; // throttled
                //return TRUE;
        case 30:
                if (((SPM_ScanControl*)data) -> MovieMode() && !((SPM_ScanControl*)data) -> scan_stopped_by_user){
                        puts ("** SPM_ScanControl::spm_scancontrol_run_scans_task **30** MOVIE MODE: next frame **");
                        runmode = 11;
                        return TRUE;
                }
                if (((SPM_ScanControl*)data) -> MultiVoltMode() && i<l){
                        puts ("** SPM_ScanControl::spm_scancontrol_run_scans_task **30** MULTI VOLT MODE: next frame **");
                        runmode = 10;
                        return TRUE;
                }
                puts ("** SPM_ScanControl::spm_scancontrol_run_scans_task **30** SCAN COMPLETED AUTO-SAVE+REPEAT CHECK **");
		if(main_get_gapp()->xsm->IsMode(MODE_AUTOSAVE)){
                        main_get_gapp()->auto_save_scans ();
                }

                {       // check on override to not repeat when executed by pyremote -- do never repeat, will never stop!
                        GSettings *global_settings = g_settings_new (GXSM_RES_BASE_PATH_DOT ".global");
                        int override = g_settings_get_int (global_settings, "math-global-share-variable-repeatmode-override");
                        if (override)
                                g_settings_set_int (global_settings, "math-global-share-variable-repeatmode-override", 0); // reset now to normal
                        g_clear_object (&global_settings);
                        if (override){ // executed by pyremote -- do never repeat!
                                runmode = 99;
                                return TRUE;
                        }
                }

                if (((SPM_ScanControl*)data) -> RepeatMode() && !((SPM_ScanControl*)data) -> scan_stopped_by_user)
                        runmode = 10;
                else
                        runmode = 99;
                return TRUE;
        case 99:
                gtk_widget_set_sensitive ((GtkWidget*)g_object_get_data( G_OBJECT (w), "SPMCONTROL_MOVIE_BUTTON"), TRUE);
                gtk_widget_set_sensitive ((GtkWidget*)g_object_get_data( G_OBJECT (w), "SPMCONTROL_SLS_BUTTON"), TRUE);
                gtk_widget_set_sensitive ((GtkWidget*)g_object_get_data( G_OBJECT (w), "SPMCONTROL_SLS_MODE"), TRUE);
                gtk_widget_set_sensitive ((GtkWidget*)g_object_get_data( G_OBJECT (w), "SPMCONTROL_EC_NUM_VALUES"), TRUE);
                gtk_widget_set_sensitive (w, TRUE);
                if (ss_action)
                        g_simple_action_set_enabled ((GSimpleAction*)ss_action, TRUE);
                runmode = 0;
                return G_SOURCE_REMOVE;
        }
        runmode=0;
        return G_SOURCE_REMOVE;
}

static void spm_scancontrol_movie_callback (GtkWidget *w, void *data){
	if (((SPM_ScanControl*)data) ->  scan_in_progress()){
		((SPM_ScanControl*)data) -> resume_scan (); // in case paused, resume now!
		return;
	}

        ((SPM_ScanControl*)data) -> SetMovieMode (TRUE);

        ((SPM_ScanControl*)data) -> wdata = w;
        g_idle_add (SPM_ScanControl::spm_scancontrol_run_scans_task, data);
}

static void spm_scancontrol_pause_callback (GtkWidget *w, void *data){
	((SPM_ScanControl*)data) -> pause_scan();
}

static void spm_scancontrol_stop_callback (GtkWidget *w, void *data){
	((SPM_ScanControl*)data) -> stop_scan();
}


static void spm_scancontrol_set_subscan_callback (GtkWidget *w, void *data){
        gboolean s;
        if (((SPM_ScanControl*)data) ->  scan_in_progress())
                return;

        switch (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), "SUBSCAN_MODE"))){
        case 0: ((SPM_ScanControl*)data)->set_sls_mode (TRUE);
                gtk_check_button_set_active (GTK_CHECK_BUTTON (g_object_get_data (G_OBJECT (w), "SUBSCAN_RB")), TRUE);
                if (main_get_gapp()->xsm->GetActiveScan ()){
                        int n_obj = (int) main_get_gapp()->xsm->GetActiveScan () -> number_of_object ();
                
                        while (n_obj--){
                                Point2D p[2];
                                scan_object_data *obj_data = main_get_gapp()->xsm->GetActiveScan () -> get_object_data (n_obj);

                                if (strncmp (obj_data->get_name (), "Rectangle", 9) )
                                        continue; // only rectangels are used!
                        
                                if (obj_data->get_num_points () != 2) 
                                        continue; // sth. is weired!

                                // get real world coordinates of rectangle
                                double x0,y0,x1,y1;
                                obj_data->get_xy_i (0, x0, y0);
                                obj_data->get_xy_i (1, x1, y1);
                                
                                // convert to pixels
                                main_get_gapp()->xsm->GetActiveScan () -> World2Pixel (x0, y0, p[0].x, p[0].y);
                                main_get_gapp()->xsm->GetActiveScan () -> World2Pixel (x1, y1, p[1].x, p[1].y);

                                gint xm = main_get_gapp()->xsm->GetActiveScan ()->mem2d->GetNx ()-1;
                                gint ym = main_get_gapp()->xsm->GetActiveScan ()->mem2d->GetNy ()-1;
                                for (int i=0; i<1; ++i){
                                        p[i].x = MAX (MIN (p[i].x, xm), 0);
                                        p[i].y = MAX (MIN (p[i].y, ym), 0);
                                }
                                
                                PI_DEBUG (DBG_L1, "SSC::SET_SUBSCAN => [" << p[0].x << ", " << p[0].y << ", " << p[1].x << ", " << p[1].y << "]");
                                
                                ((SPM_ScanControl*)data) -> set_subscan ((double)(p[0].x < p[1].x ? p[0].x:p[1].x-1),
                                                                         fabs ((double)p[1].x-(double)p[0].x)+1,
                                                                         (double)(p[0].y < p[1].y ? p[0].y:p[1].y-1),
                                                                         fabs ((double)p[1].y-(double)p[0].y)+1);
                        
                                PI_DEBUG (DBG_L1, "SSC::SET_SUBSCAN done.");
                               	((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC0")) -> Put_Value ();
                               	((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC1")) -> Put_Value ();
                               	((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC2")) -> Put_Value ();
                               	((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC3")) -> Put_Value ();

                                break;
                        }
                }
                break;
        case 1:
                ((SPM_ScanControl*)data)->set_sls_mode (s=gtk_check_button_get_active (GTK_CHECK_BUTTON (w)));
                if (s){
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC0")) -> Thaw ();
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC1")) -> Thaw ();
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC2")) -> Thaw ();
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC3")) -> Thaw ();
                        gtk_widget_show (g_object_get_data (G_OBJECT (w), "SPMCONTROL_SLS_FRAME"));
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC0")) -> Show (TRUE);
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC1")) -> Show (TRUE);
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC2")) -> Show (TRUE);
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC3")) -> Show (TRUE);
                } else {
                        ((SPM_ScanControl*)data) -> set_subscan (0,0,0,0);
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC0")) -> Put_Value ();
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC1")) -> Put_Value ();
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC2")) -> Put_Value ();
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC3")) -> Put_Value ();
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC0")) -> Freeze ();
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC1")) -> Freeze ();
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC2")) -> Freeze ();
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC3")) -> Freeze ();
                        gtk_widget_hide (g_object_get_data (G_OBJECT (w), "SPMCONTROL_SLS_FRAME"));
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC0")) -> Show (FALSE);
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC1")) -> Show (FALSE);
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC2")) -> Show (FALSE);
                        ((Gtk_EntryControl*) g_object_get_data (G_OBJECT (w), "SLSC3")) -> Show (FALSE);
                }
                break;
        }

        // testing  [d %d], [3 %d %d %d]
        //------> use gchar* main_get_gapp()->GetPluginData() to get data [n x ..]
        
        ((SPM_ScanControl*)data) -> set_subscan (); /* setup for partial/sub scan */
}



// ==================================================
// Scan Control Routines
// ==================================================


// ========================================
// DoScan
// ========================================
//
// aktiviert automatisch alle vorherigen / bzw. einen neuen Scan Channel
// setzt Channel-Zuordnung
// SRCS MODES:
// bit 0 1 2 3:   PID (Z), ... (calculated stuff (Z=Topo)) :: SR: 0 only is Z-Topo
// bit 4 5 6 7:   MUXA select PIDSRC (Force, I, dF, ..)   (mux A bei PC31, bei PCI32 alle auf A), SR: 4 AIC5, 5: AIC0, ... 
// bit 8,9,10,11: MUXB select Analog Value (Friction, ..) (mux B bei PC31) SR: ... AIC7 except AIC5
// bit 12,13,14,15: AUX select C,D,E,F, SR: LockIn_dIdV, LockIn_ddIdV, LockIn_I0, Count (32bit) -- MK3: generic signal0..3
/*
#define MSK_PID(X)  (1<<((X)&3))
#define MSK_MUXA(X) (1<<(((X)&3)+4))
#define MSK_MUXB(X) (1<<(((X)&3)+8))
#define MSK_AUX(X)  (1<<(((X)&3)+12))
*/

// now via rescources. Defaults as of above's match are now auto setup in core gxsm_rescourcetable.cpp: void gxsm_init_dynamic_res()
// and may be ovreridden at run time or via HwI at init/auto configuration time!
#define MSK_PID(X)  xsmres.pidsrc_msk[(X)&3]
#define MSK_MUXA(X) xsmres.daq_msk[(X)]
#define MSK_MUXB(X) xsmres.daq_msk[(X)+4]
#define MSK_AUX(X)  xsmres.daq_msk[(X)+8]


int SPM_ScanControl::free_scan_lists (){

	main_get_gapp()->xsm->SetActiveScanList ();

	if (all_scan_list){
		g_slist_free (all_scan_list);
		all_scan_list = NULL;
	}
	if (xp_scan_list){
		g_slist_free (xp_scan_list);
		xp_scan_list = NULL;
	}
	if (xp_2nd_scan_list){
		g_slist_free (xp_2nd_scan_list);
		xp_2nd_scan_list = NULL;
	}
	if (xp_prbscan_list){
		g_slist_free (xp_prbscan_list);
		xp_prbscan_list = NULL;
	}
	if (xm_scan_list){
		g_slist_free (xm_scan_list);
		xm_scan_list = NULL;
	}
	if (xm_2nd_scan_list){
		g_slist_free (xm_2nd_scan_list);
		xm_2nd_scan_list = NULL;
	}
	if (xm_prbscan_list){
		g_slist_free (xm_prbscan_list);
		xm_prbscan_list = NULL;
	}

	return 0;
}

// analyse channelselections and setup scanlists and scans
int SPM_ScanControl::initialize_scan_lists (){
	int i,ipid,idaq,i2nddaq,iprb,ch,sok,checks;

	main_get_gapp()->xsm->SetActiveScanList ();
        
	if (xp_scan_list || xm_scan_list 
	    || xp_2nd_scan_list || xm_2nd_scan_list 
	    || xp_prbscan_list || xm_prbscan_list) // stop if scan lists are existing!
		return -1;
  
	do_probe = FALSE;
	master_scan = NULL;
	master_probescan = NULL;
	main_get_gapp()->xsm->SetMasterScan (NULL);

	sok=FALSE; // ScanOK = no 
	checks=2;    // only one second try!
	xp_srcs = xm_srcs = 0; // reset srcs, take care of PID default, to do!!!!
	xp_2nd_srcs = xm_2nd_srcs = 0; // reset srcs, take care of PID default, to do!!!!

	// analyze channel settings and create scan lists, and setup this scans
	do {
		PI_DEBUG (DBG_L3, "SPM_SCANCONTROL::initialize_scan_lists : do");

		// Find and init Xp-Scans
		for (ipid=idaq=i2nddaq=iprb=i=0; i < MAXSCANS; i++){
			// PID src?  Find the first/next "Topo like" scan... 
			// (ipid counts until all PID type scans are checked)
			for (ch = -1; (ipid < PIDCHMAX) && ((ch=main_get_gapp()->xsm->FindChan(xsmres.pidchno[ipid++], ID_CH_D_P)) < 0););

			if(ch >= 0){ // got one!
				PI_DEBUG (DBG_L3, "SPM_SCANCONTROL::initialize_scan_lists : Setting up XpScan - PID src found");
				// add bit to xp_srcs mask
				xp_srcs |= MSK_PID(ipid-1);
				// setup this scan
				setup_scan (ch, "X+", 
					    xsmres.pidsrc[ipid-1], 
					    xsmres.pidsrcZunit[ipid-1], 
					    xsmres.pidsrcZtype[ipid-1], 
					    xsmres.pidsrcZlabel[ipid-1],
					    xsmres.pidsrcZd2u[ipid-1]
					);
				// and add to list
				xp_scan_list = g_slist_prepend (xp_scan_list, main_get_gapp()->xsm->scan[ch]);
				all_scan_list = g_slist_prepend (all_scan_list, main_get_gapp()->xsm->scan[ch]);

				// got one valid scan, so we could finish if no more... "Scan-OK"
				sok=TRUE;
			}
			else{
				// DAQ src? Find additional scans/channels to aquire...
				// (idaq counts until all checked)
				for(ch = -1; (idaq < DAQCHMAX) && ((ch=main_get_gapp()->xsm->FindChan(xsmres.daqchno[idaq++], ID_CH_D_P)) < 0););
				if(ch >= 0){ // got one!
					// add to xp_srcs mask... use "MUXA" (DA0..3)
					if(idaq <= 4)
						xp_srcs |= MSK_MUXA(idaq-1);
					else{
						// add to xp_srcs mask... use "MUXB" (DA4..7)
						if(idaq <= 8)
							xp_srcs |= MSK_MUXB(idaq-5);
						else{
							if(idaq <= 12){
								// add to xp_srcs mask... use "AUX" (Auxillary DSP-Data)
								xp_srcs |= MSK_AUX(idaq-9);
							}
						}
					}
					PI_DEBUG (DBG_L3, "SPM_SCANCONTROL::initialize_scan_lists : Setting up XpScan - DAQ src found");
					// setup this scan
					setup_scan (ch, "X+",
						    xsmres.daqsrc[idaq-1], 
						    xsmres.daqZunit[idaq-1], 
						    xsmres.daqZtype[idaq-1], 
						    xsmres.daqZlabel[idaq-1],
						    xsmres.daqZd2u[idaq-1]
						);
					// add to list...
					xp_scan_list = g_slist_prepend (xp_scan_list, main_get_gapp()->xsm->scan[ch]);
					all_scan_list = g_slist_prepend (all_scan_list, main_get_gapp()->xsm->scan[ch]);
					sok=TRUE;
				}
				else{
					// 2nd DAQ src? Find additional scans/channels to aquire...
					// (idaq counts until all checked)
					for(ch = -1; (i2nddaq < DAQCHMAX) && ((ch=main_get_gapp()->xsm->FindChan(xsmres.daqchno[i2nddaq++], ID_CH_D_2ND_P)) < 0););
					if(ch >= 0){ // got one!
						// add to xp_srcs mask... use "MUXA" (DA0..3)
						if(i2nddaq <= 4)
							xp_2nd_srcs |= MSK_MUXA(i2nddaq-1);
						else{
							// add to xp_srcs mask... use "MUXB" (DA4..7)
							if(i2nddaq <= 8)
								xp_2nd_srcs |= MSK_MUXB(i2nddaq-5);
							else{
								if(idaq <= 12){
									// add to xp_srcs mask... use "AUX" (Auxillary DSP-Data)
									xp_2nd_srcs |= MSK_AUX(i2nddaq-9);
								}
							}
						}
						PI_DEBUG (DBG_L3, "SPM_SCANCONTROL::initialize_scan_lists : Setting up 2nd XpScan - DAQ src found");
						// setup this scan
						setup_scan (ch, "X2+",
							    xsmres.daqsrc[i2nddaq-1], 
							    xsmres.daqZunit[i2nddaq-1], 
							    xsmres.daqZtype[i2nddaq-1], 
							    xsmres.daqZlabel[i2nddaq-1],
							    xsmres.daqZd2u[i2nddaq-1]
							);
						// add to list...
						xp_2nd_scan_list = g_slist_prepend (xp_2nd_scan_list, main_get_gapp()->xsm->scan[ch]);
						all_scan_list = g_slist_prepend (all_scan_list, main_get_gapp()->xsm->scan[ch]);
						sok=TRUE;
					}
				}
			}
		}
    
		// Find and init Xm-Scans
		// similar channel analysis for all XM (<-) scans... -- no probing in this direction!
		for (ipid=i2nddaq=idaq=i=0; i<MAXSCANS; i++){
			// new PID src?
			for(ch = -1; (ipid < PIDCHMAX) && ((ch=main_get_gapp()->xsm->FindChan(xsmres.pidchno[ipid++], ID_CH_D_M)) < 0););

			if(ch >= 0){
				PI_DEBUG (DBG_L3, "SPM_SCANCONTROL::initialize_scan_lists : Setting up XmScan - PID src found");
				xm_srcs |= MSK_PID(ipid-1);

				setup_scan (ch, "X-", 
					    xsmres.pidsrc[ipid-1], 
					    xsmres.pidsrcZunit[ipid-1], 
					    xsmres.pidsrcZtype[ipid-1], 
					    xsmres.pidsrcZlabel[ipid-1],
					    xsmres.pidsrcZd2u[ipid-1]
					);
				xm_scan_list = g_slist_prepend (xm_scan_list, main_get_gapp()->xsm->scan[ch]);
				all_scan_list = g_slist_prepend (all_scan_list, main_get_gapp()->xsm->scan[ch]);
				sok=TRUE;
			}
			else{
				for(ch = -1; (idaq < DAQCHMAX) && ((ch=main_get_gapp()->xsm->FindChan(xsmres.daqchno[idaq++], ID_CH_D_M)) < 0););
				if(ch >= 0){
					if(idaq <= 4)
						xm_srcs |= MSK_MUXA(idaq-1);
					else{
						if(idaq <= 8)
							xm_srcs |= MSK_MUXB(idaq-5);
						else{
							if(idaq <= 12){
								xm_srcs |= MSK_AUX(idaq-9);
							}
						}
					}
					
					PI_DEBUG (DBG_L3, "SPM_SCANCONTROL::initialize_scan_lists : Setting up XmScan - DAQ src found");
					setup_scan (ch, "X-",
						    xsmres.daqsrc[idaq-1], 
						    xsmres.daqZunit[idaq-1], 
						    xsmres.daqZtype[idaq-1], 
						    xsmres.daqZlabel[idaq-1],
						    xsmres.daqZd2u[idaq-1]
						);
					xm_scan_list = g_slist_prepend (xm_scan_list, main_get_gapp()->xsm->scan[ch]);
					all_scan_list = g_slist_prepend (all_scan_list, main_get_gapp()->xsm->scan[ch]);
					sok=TRUE;
				}
				else{
					for(ch = -1; (i2nddaq < DAQCHMAX) && ((ch=main_get_gapp()->xsm->FindChan(xsmres.daqchno[i2nddaq++], ID_CH_D_2ND_M)) < 0););
					if(ch >= 0){
						if(i2nddaq <= 4)
							xm_2nd_srcs |= MSK_MUXA(i2nddaq-1);
						else{
							if(i2nddaq <= 8)
								xm_2nd_srcs |= MSK_MUXB(i2nddaq-5);
							else{
								if(idaq <= 12){
									xm_2nd_srcs |= MSK_AUX(i2nddaq-9);
								}
							}
						}
					
						PI_DEBUG (DBG_L3, "SPM_SCANCONTROL::initialize_scan_lists : Setting up 2nd XmScan - DAQ src found");
						setup_scan (ch, "X2-",
							    xsmres.daqsrc[i2nddaq-1], 
							    xsmres.daqZunit[i2nddaq-1], 
							    xsmres.daqZtype[i2nddaq-1], 
							    xsmres.daqZlabel[i2nddaq-1],
							    xsmres.daqZd2u[i2nddaq-1]
							);
						xm_2nd_scan_list = g_slist_prepend (xm_2nd_scan_list, main_get_gapp()->xsm->scan[ch]);
						all_scan_list = g_slist_prepend (all_scan_list, main_get_gapp()->xsm->scan[ch]);
						sok=TRUE;
					}
				}
			}
		}

		master_scan = main_get_gapp()->xsm->GetMasterScan ();

                // reset Ext/Map Channels
                for(int i=0; i < EXTCHMAX; ++i){
                        if ((ch = main_get_gapp()->xsm->FindChan(xsmres.extchno[i], ID_CH_D_P)) >= 0){
                                setup_scan (ch, "X+", "Map-PrbSrc#", "Xu", "DOUBLE", "EXTMAP", -1.0); // needs further setup!
                                all_scan_list = g_slist_prepend (all_scan_list, main_get_gapp()->xsm->scan[ch]);
                        }
                }

                
		// Automatic mode:
		// if no Scansrc specified -- find free channel and use pid-default ("Topo")
		if(! sok){
			PI_DEBUG (DBG_L3, "SPM_SCANCONTROL::initialize_scan_lists : No Srcs specified, setting up default!");
			ch = main_get_gapp()->xsm->FindChan(ID_CH_M_ACTIVE);
			if(!(ch >= 0)){
				ch = main_get_gapp()->xsm->FindChan(ID_CH_M_MATH);
				if(!(ch >= 0))
					ch = main_get_gapp()->xsm->FindChan(ID_CH_M_OFF);
				else{
					scan_flag = SCAN_FLAG_RUN;
					XSM_SHOW_ALERT(ERR_SORRY, ERR_NOFREECHAN,"",1);
					return FALSE;
				}
			}
			main_get_gapp()->channelselector->SetMode(ch, xsmres.piddefault);
			main_get_gapp()->xsm->ChannelMode[ch] =  xsmres.piddefault;
			main_get_gapp()->xsm->ChannelScanMode[ch] = main_get_gapp()->xsm->ChannelScanMode[ch] < 0 ? -xsmres.piddefault : xsmres.piddefault;
		}
	}while (! sok && --checks);

	// if sth, went wrong... (should never happen)
	if (!checks)
		XSM_SHOW_ALERT (ERR_SORRY, ERR_NOSRCCHAN, "", 1);


	main_get_gapp()->xsm->SetActiveScanList (all_scan_list); // EXPORT SCAN LIST


        return 0;
}

int SPM_ScanControl::setup_scan (int ch, 
				 const gchar *titleprefix, 
				 const gchar *name,
				 const gchar *unit,
				 const gchar *type,
				 const gchar *label,
				 double d2u
	){
	PI_DEBUG (DBG_L2, "setup_scan");

        // check for exisiting scan, reusing, remapping data
	if ( ! main_get_gapp()->xsm->scan[ch]){ // create scan object if not exisiting
		main_get_gapp()->xsm->scan[ch] = main_get_gapp()->xsm->NewScan (main_get_gapp()->xsm->ChannelView[ch], 
							  main_get_gapp()->xsm->data.display.ViewFlg, 
							  ch, 
							  NULL);
		// Error ?
		if (!main_get_gapp()->xsm->scan[ch]){
			XSM_SHOW_ALERT (ERR_SORRY, ERR_NOMEM,"",1);
			return FALSE;
		}
	}

        ZD_TYPE zt=ZD_SHORT;
	if (type){
		if (strncmp (type, "BYTE", 4)==0) zt = ZD_BYTE;
		else if (strncmp (type, "SHORT", 5)==0) zt = ZD_SHORT;
		else if (strncmp (type, "LONG", 4)==0) zt = ZD_LONG;
		else if (strncmp (type, "ULONG", 5)==0) zt = ZD_ULONG; 
		else if (strncmp (type, "LLONG", 5)==0) zt = ZD_LLONG; 
		else if (strncmp (type, "FLOAT", 5)==0) zt = ZD_FLOAT;
		else if (strncmp (type, "DOUBLE", 6)==0) zt = ZD_DOUBLE; 
		else if (strncmp (type, "COMPLEX", 5)==0) zt = ZD_COMPLEX;
		else if (strncmp (type, " zt = RGBA", 5)==0) zt = ZD_RGBA;
		else{  zt =  ZD_SHORT;// default fallback is "Short"
		}
	} 

	PI_DEBUG (DBG_L1, "setup_scan[" << ch << " ]: scan->create " << type << " channel."); 
		
        // Create/Resize/Update scan object -- now remapping existing data as muc has available

        main_get_gapp()->xsm->scan[ch]->create (TRUE, FALSE, strchr (titleprefix, '-') ? -1.:1., main_get_gapp()->xsm->hardware->IsFastScan (),
                                     zt,
                                     keep_multi_layer_info, // keep layer informations
                                     gtk_check_button_get_active (GTK_CHECK_BUTTON (g_object_get_data( G_OBJECT (scan_start_button), "SPMCONTROL_RAD_START_MODE_CB"))), // RAD: remap data
                                     strcmp (label, "EXTMAP")==0 ? true:false // keep_n_values
                                     );

	// setup dz from instrument definition or propagated via signal definition
	if (fabs (d2u) > 0.)
		main_get_gapp()->xsm->scan[ch]->data.s.dz = d2u;
	else
		main_get_gapp()->xsm->scan[ch]->data.s.dz = main_get_gapp()->xsm->Inst->ZResolution (unit);

	// Setup correct Z unit
	UnitObj *u = main_get_gapp()->xsm->MakeUnit (unit, label);
	main_get_gapp()->xsm->scan[ch]->data.SetZUnit (u);
	delete u;
	// set scan title, name, ... and draw it!

        // Setup Scan Title
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

        main_get_gapp()->xsm->scan[ch]->storage_manager.set_type (scantitle);
        main_get_gapp()->xsm->scan[ch]->storage_manager.set_basename (main_get_gapp()->xsm->data.ui.basename); // from GXSM Main GUI
        main_get_gapp()->xsm->scan[ch]->storage_manager.set_dataset_counter (main_get_gapp()->xsm->GetFileCounter ());   // from GXSM Main GUI
        main_get_gapp()->xsm->scan[ch]->storage_manager.set_path (g_settings_get_string (main_get_gapp()->get_as_settings (), "auto-save-folder"));   // from GXSM Main GUI
        
	PI_DEBUG (DBG_L2, "setup_scan[" << ch << " ]: scantitle done: " << main_get_gapp()->xsm->scan[ch]->data.ui.type ); 

        main_get_gapp()->channelselector->SetInfo (ch, scantitle);
        //main_get_gapp()->channelselector->SetInfo (ch, main_get_gapp()->xsm->scan[ch]->storage_manager.get_name()); // test only
	main_get_gapp()->xsm->scan[ch]->draw ();

	g_free (scantitle);
        
	return 0;
}

int SPM_ScanControl::prepare_to_start_scan (SCAN_DT_TYPE st){
	// which origin mode?
        if (IS_SPALEED_CTRL||xsmres.ScanOrgCenter)
                YOriginTop = FALSE; // Fix hier f\FCr SPA-LEED
	else 
                YOriginTop = TRUE;
	scan_flag = SCAN_FLAG_RUN;

	main_get_gapp()->SetStatus ("Starting Scan: Ch.Setup");
    
	// setup scan size
	main_get_gapp()->xsm->hardware->SetDxDy (
		main_get_gapp()->xsm->Inst->XA2Dig (main_get_gapp()->xsm->data.s.dx),
		main_get_gapp()->xsm->Inst->YA2Dig (main_get_gapp()->xsm->data.s.dy));
	main_get_gapp()->xsm->hardware->SetNxNy (main_get_gapp()->xsm->data.s.nx, main_get_gapp()->xsm->data.s.ny);

	main_get_gapp()->xsm->data.s.dz = main_get_gapp()->xsm->Inst->ZResolution ();

	main_get_gapp()->SignalStartScanEventToPlugins ();

	// Init Scanobjects, do Channelsetup... some complex stuff...

	// only once, for single layer scan or first layer

	// check, free
	free_scan_lists ();
	
	int result = initialize_scan_lists ();
	
	if (result){
		XSM_SHOW_ALERT (ERR_SORRY, ERR_SCAN_CANCEL,
				"Channel setup failed.",
				1);
		return result;
	}

	// count channels and check if total data amount fits into hardware/transferbuffer/etc. hard limits
	int ns_xp=0;
	int ns_xm=0;
	for (int i=0; i<16; ++i){
		if (xp_srcs&(1<<i)) ++ns_xp;
		if (xm_srcs&(1<<i)) ++ns_xm;
	}

	// Check Ranges/Sizes with Hardware/DSP Memory
	switch (st){
	case SCAN_LINESCAN:
		if (main_get_gapp()->xsm->data.s.nx*ns_xp >= main_get_gapp()->xsm->hardware->GetMaxPointsPerLine ()){
			gchar *maxtxt = g_strdup_printf ("Hardware limits points per line to\n"
							 "%ld in total for all channels (->)!",
							 main_get_gapp()->xsm->hardware->GetMaxPointsPerLine());
			XSM_SHOW_ALERT (ERR_SORRY,ERR_SCAN_CANCEL, maxtxt, 1);
			g_free (maxtxt);
			return -1;
		}
		if (main_get_gapp()->xsm->data.s.nx*ns_xm >= main_get_gapp()->xsm->hardware->GetMaxPointsPerLine ()){
			gchar *maxtxt = g_strdup_printf ("Hardware limits points per line to\n"
							 "%ld in total for all channels (<-)!",
							 main_get_gapp()->xsm->hardware->GetMaxPointsPerLine());
			XSM_SHOW_ALERT (ERR_SORRY,ERR_SCAN_CANCEL, maxtxt, 1);
			g_free (maxtxt);
			return -1;
		}
		return 0;
	case SCAN_FRAMECAPTURE: // obsoleted
		if (main_get_gapp()->xsm->data.s.nx*main_get_gapp()->xsm->data.s.ny*ns_xp >= main_get_gapp()->xsm->hardware->GetMaxPointsPerLine ()){
			gchar *maxtxt = g_strdup_printf ("Hardware limits points per frame to\n"
							 "%ld in total for all channels (->)!",
							 main_get_gapp()->xsm->hardware->GetMaxPointsPerLine());
			XSM_SHOW_ALERT (ERR_SORRY,ERR_SCAN_CANCEL, maxtxt, 1);
			g_free (maxtxt);
			return -1;
		}
		if (main_get_gapp()->xsm->data.s.nx*main_get_gapp()->xsm->data.s.ny*ns_xm >= main_get_gapp()->xsm->hardware->GetMaxPointsPerLine ()){
			gchar *maxtxt = g_strdup_printf ("Hardware limits points per frame to\n"
							 "%ld in total for all channels (<-)!",
							 main_get_gapp()->xsm->hardware->GetMaxPointsPerLine());
			XSM_SHOW_ALERT (ERR_SORRY,ERR_SCAN_CANCEL, maxtxt, 1);
			g_free (maxtxt);
			return -1;
		}
		return 0;
	default: return -1;
	}
}

// run "background" update function(s)
void IdleRefreshFunc (gpointer data){
	g_slist_foreach ((GSList*) ((IdleRefreshFuncData*)data)->scan_list,
			 (GFunc) ((IdleRefreshFuncData*)data)->UpdateFunc,
			 ((IdleRefreshFuncData*)data)->data);
}

// execute a single scan line -- or if "line == -1" a HS 2D Area Capture/Scan
gboolean SPM_ScanControl::do_scanline (int init){
	static Mem2d **m2d_xp=NULL;
	static Mem2d **m2d_xm=NULL;
	static Mem2d **m2d_2nd_xp=NULL;
	static Mem2d **m2d_2nd_xm=NULL;
	static IdleRefreshFuncData idf_data; // moved to class
        static int scanning_task_section=0;

        PI_DEBUG_GM (DBG_L3, "SPM_SCANCONTROL::do_scanline init=%d, scanning_task_section=%d", init, scanning_task_section);
        
	// if first time called/first line, do some local (static vars) initializations here!
	if (init){
		int num; 
		PI_DEBUG (DBG_L2, "SPM_ScanControl::do_scanline : init");
		if (m2d_xp){
			g_free (m2d_xp);
			m2d_xp = NULL;
		}
		if (m2d_xm){
			g_free (m2d_xm);
			m2d_xm = NULL;
		}
		if (m2d_2nd_xp){
			g_free (m2d_2nd_xp);
			m2d_2nd_xp = NULL;
		}
		if (m2d_2nd_xm){
			g_free (m2d_2nd_xm);
			m2d_2nd_xm = NULL;
		}
		if( line < 0){
                        scanning_task_section = 0;
			return TRUE; // cleanup
                }
                
		num = g_slist_length (xp_scan_list); 
		if (num > 0){
			Mem2d **m;
			m = m2d_xp = (Mem2d**) g_new (Mem2d*, num+1);
			GSList* tmp = g_slist_copy (xp_scan_list);
			GSList* rev = tmp = g_slist_reverse(tmp);
			while (tmp){
				*m++ = ((Scan*)tmp->data)->mem2d;
				if (((Scan*)tmp->data)->view)
					((Scan*)tmp->data)->view->remove_events(); // clean up old events
				tmp = g_slist_next (tmp);
			}
			*m = NULL;
			g_slist_free (rev);
		}

		num = g_slist_length (xm_scan_list); 
		PI_DEBUG (DBG_L2, "SPM_ScanControl::do_scanline : init, num xm=" << num);
		if (num > 0){
			Mem2d **m;
			m = m2d_xm = (Mem2d**) g_new (Mem2d*, num+1);
			GSList* tmp = g_slist_copy (xm_scan_list);
			GSList* rev = tmp = g_slist_reverse(tmp);
			while (tmp){
				*m++ = ((Scan*)tmp->data)->mem2d;
				if (((Scan*)tmp->data)->view)
					((Scan*)tmp->data)->view->remove_events(); // clean up old events
				tmp = g_slist_next (tmp);
			}
			*m = NULL;
			g_slist_free (rev);
		}

		num = g_slist_length (xp_2nd_scan_list); 
		if (num > 0){
			Mem2d **m;
			m = m2d_2nd_xp = (Mem2d**) g_new (Mem2d*, num+1);
			GSList* tmp = g_slist_copy (xp_2nd_scan_list);
			GSList* rev = tmp = g_slist_reverse(tmp);
			while (tmp){
				*m++ = ((Scan*)tmp->data)->mem2d;
				if (((Scan*)tmp->data)->view)
					((Scan*)tmp->data)->view->remove_events(); // clean up old events
				tmp = g_slist_next (tmp);
			}
			*m = NULL;
			g_slist_free (rev);
		}

		num = g_slist_length (xm_2nd_scan_list); 
		PI_DEBUG (DBG_L2, "SPM_ScanControl::do_scanline : init, num 2nd xm=" << num);
		if (num > 0){
			Mem2d **m;
			m = m2d_2nd_xm = (Mem2d**) g_new (Mem2d*, num+1);
			GSList* tmp = g_slist_copy (xm_2nd_scan_list);
			GSList* rev = tmp = g_slist_reverse(tmp);
			while (tmp){
				*m++ = ((Scan*)tmp->data)->mem2d;
				if (((Scan*)tmp->data)->view)
					((Scan*)tmp->data)->view->remove_events(); // clean up old events
				tmp = g_slist_next (tmp);
			}
			*m = NULL;
			g_slist_free (rev);
		}
                
		idf_data.scan_list  = NULL;
		idf_data.UpdateFunc = (GFunc) SPM_ScanControl::call_scan_draw_line;
		idf_data.data = this;
                idf_data.scan_list = NULL;
		PI_DEBUG (DBG_L2, "SPM_ScanControl::do_scanline : init done");

                // setup hardware data transfer
		main_get_gapp()->xsm->hardware->ScanLineM (-2,  1, xp_srcs, m2d_xp, sls_config);
		main_get_gapp()->xsm->hardware->ScanLineM (-2, -1, xm_srcs, m2d_xm, sls_config);
		main_get_gapp()->xsm->hardware->ScanLineM (-2,  2, xp_2nd_srcs, m2d_2nd_xp, sls_config);
		main_get_gapp()->xsm->hardware->ScanLineM (-2, -2, xm_2nd_srcs, m2d_2nd_xm, sls_config);

                // ready for processing lines
                scanning_task_section = 1;
		return TRUE;
	}

	// do Scanline(s) cycling through (XP (->), XM (<-), XP2ND (2>), XM2ND (<2)) and update display

        switch (scanning_task_section){
        case 0: return FALSE;
        case 1:
                // do X+ Scan?
                if (m2d_xp){
                        // execute scanline XP (->), update previous line data in idle call
                        if (!main_get_gapp()->xsm->hardware->ScanLineM (line,  1, xp_srcs, m2d_xp, sls_config)){
                                // setup idle func -- executed on next wait for data!
                                line2update = line+sls_config[2];
                                idf_data.scan_list = xp_scan_list;
                                scanning_task_section = 2; // next section
                                break;
                        } else break;
                } else
                        scanning_task_section = 2; // next section
        case 2:
                // do X- Scan ?
                if (m2d_xm){
                        if (!main_get_gapp()->xsm->hardware->ScanLineM(line, -1, xm_srcs, m2d_xm, sls_config)){
                                line2update = line+sls_config[2];
                                idf_data.scan_list = xm_scan_list;
                                scanning_task_section = 3; // next section
                                break;
                        } else break;
                } else
                        scanning_task_section = 3; // next section
        case 3:
                // do X2nd+ Scan?
                if (m2d_2nd_xp){
                        if (!main_get_gapp()->xsm->hardware->ScanLineM (line,  2, xp_2nd_srcs, m2d_2nd_xp, sls_config)){
                                line2update = line+sls_config[2];
                                idf_data.scan_list = xp_2nd_scan_list;
                                scanning_task_section = 4; // next section
                                break;
                        } else break;
                } else
                        scanning_task_section = 4; // next section
        case 4:
                // do X- Scan ?
                if (m2d_2nd_xm){
                        if (!main_get_gapp()->xsm->hardware->ScanLineM(line, -2, xm_2nd_srcs, m2d_2nd_xm, sls_config)){
                                line2update = line+sls_config[2];
                                idf_data.scan_list = xm_2nd_scan_list;
                                scanning_task_section = 5; // next section
                                break;
                        } else break;
                } else
                        scanning_task_section = 5; // Auto Save Check!
        case 5:
                autosave_check (update_status_info ());
                scanning_task_section = 1;
                return FALSE; // done.
        }
        
        const gint64 max_age = 50000; // 100ms
        static gint64 time_of_last_update = 0;
        if ( (time_of_last_update+max_age) < g_get_real_time () ){ // throttle
                time_of_last_update = g_get_real_time ();
                if (idf_data.scan_list) IdleRefreshFunc (&idf_data);
        }

        return TRUE;
}

void SPM_ScanControl::run_probe (int ipx, int ipy){

}

//  update SLS info ( ixy_sub[4] ) from user entry in all scan destination channels
void SPM_ScanControl::set_subscan (int xs, int xn, int ys, int yn){
        if (!master_scan)
                return;

        if (xs >= 0){
                sls_config[0] = xs;
                sls_config[1] = xn;
                sls_config[2] = ys;
                sls_config[3] = yn;
        }
        
	PI_DEBUG (DBG_L2, "SPM_ScanControl::set_subscan"
                  << "SET_SUBSCAN=[[" << sls_config[0] << ", " << sls_config[1] << "]" << std::endl
                  << "             [" << sls_config[2] << ", " << sls_config[3] << "]]" << std::endl);

	// Setup Copy Mode in mem2d...
	for (GSList* tmp = all_scan_list; tmp; tmp = g_slist_next (tmp)){
		((Scan*)tmp->data) -> set_subscan_information (sls_config);
		((Scan*)tmp->data) -> mem2d->data->ZPutDataSetDest (sls_config);
	}       
}	

gboolean SPM_ScanControl::scanning_control_init (){
	switch (scan_dir){
	case SCAN_DIR_TOPDOWN: 
		last_scan_dir = SCAN_DIR_TOPDOWN;
		break;
	case SCAN_DIR_TOPDOWN_BOTUP: 
		last_scan_dir = last_scan_dir == SCAN_DIR_TOPDOWN ? SCAN_DIR_BOTUP : SCAN_DIR_TOPDOWN;
		break;
	case SCAN_DIR_BOTUP: 
		last_scan_dir = SCAN_DIR_BOTUP;
		break;
	}
	main_get_gapp()->xsm->data.s.xdir=1;
	main_get_gapp()->xsm->data.s.ydir = last_scan_dir == SCAN_DIR_TOPDOWN ? 1:-1;
	main_get_gapp()->xsm->data.s.pixeltime = 0.;

	main_get_gapp()->xsm->hardware->ScanDirection (last_scan_dir == SCAN_DIR_TOPDOWN ? +1 : -1);

        g_message ("SPM_ScanControl::scanning_control_init ** ScanDir is %d", last_scan_dir);

	if (prepare_to_start_scan ()){ // uses main_get_gapp()->xsm->data.s.ydir to setup scans
		PI_DEBUG (DBG_L2, "prepare scan failed, exiting.");
		stop_scan ();
		free_scan_lists ();
                g_warning ("prepare scan start failed, aborting process.");
		return FALSE;
	}

	PI_DEBUG (DBG_L2, "do_scan precheck done.");

	// now freeze all scanparameters
	main_get_gapp()->spm_freeze_scanparam ();
    
	PI_DEBUG (DBG_L2, "do_scan SetOffsets.");

	if (!IS_SPALEED_CTRL)
		main_get_gapp()->xsm->hardware->SetOffset (R2INT (main_get_gapp()->xsm->Inst->X0A2Dig (master_scan->data.s.x0)),
						R2INT (main_get_gapp()->xsm->Inst->Y0A2Dig (master_scan->data.s.y0)));
	else
		main_get_gapp()->xsm->hardware->SetOffset (R2INT(main_get_gapp()->xsm->Inst->X0A2Dig (master_scan->data.s.x0) 
						      + master_scan->data.s.SPA_OrgX/main_get_gapp()->xsm->Inst->XResolution ()),
						R2INT(main_get_gapp()->xsm->Inst->Y0A2Dig (master_scan->data.s.y0) 
						      + master_scan->data.s.SPA_OrgY/main_get_gapp()->xsm->Inst->YResolution ()));
	

	// HwI needs to take care of no-jump
	main_get_gapp()->xsm->hardware->SetAlpha(master_scan->data.s.alpha);

	// Set Start Time, notify scans about, initialisations...
	MultiVoltEntry *mve =  MultiVoltMode () ? MultiVoltElement (scanning_task_multivolt_i) : NULL;
	g_slist_foreach ((GSList*) all_scan_list,
			 (GFunc) SPM_ScanControl::call_scan_start, mve);
        
        main_get_gapp()->set_toolbar_autosave_button (); // reset auto save button to full save symbol
        
        // prepare hardware for start scan "scan pre check"
	main_get_gapp()->xsm->hardware->StartScan2D ();

	update_status_info (TRUE);
	autosave_check (0., xsmres.AutosaveValue);
	set_subscan ();

        scanning_task_line=0;
        line = last_scan_dir == SCAN_DIR_TOPDOWN ? 0 : master_scan->mem2d->GetNySub ()-1;
	do_scanline (TRUE);

#ifdef XSM_DEBUG_OPTION
	PI_DEBUG (DBG_L3,  "xp_srcs: " << xp_srcs );
	for(int i=0; i<16; i++) PI_DEBUG_PLAIN (DBG_L2, (int)((xp_srcs&(1<<i))?1:0)); PI_DEBUG_PLAIN (DBG_L2, std::endl);
	PI_DEBUG (DBG_L3,  "xm_srcs: " << xm_srcs );
	for(int i=0; i<16; i++) PI_DEBUG_PLAIN (DBG_L2, (int)((xm_srcs&(1<<i))?1:0)); PI_DEBUG_PLAIN (DBG_L2, std::endl);
#endif
    
	PI_DEBUG (DBG_L3, "DoScan: Start Scan now");

	{
		gchar *muxcoding = g_strdup_printf ("xp_srcs: %2X  xm_srcs: %2X", xp_srcs, xm_srcs);
		main_get_gapp()->monitorcontrol->LogEvent ("*StartScan", muxcoding);
		g_free (muxcoding);
	}

        return TRUE;
}

gboolean SPM_ScanControl::scanning_control_run (){
        if ((last_scan_dir == SCAN_DIR_TOPDOWN
             ? line < master_scan->mem2d->GetNySub () : line >= 0)
            && scan_flag != SCAN_FLAG_STOP
            ){
		if (scan_flag == SCAN_FLAG_RUN){
			if (!do_scanline ()){
                                // skip to next line
                                // g_print ("scanning_control_run task_line#=%d  y-line=%d RTQy: %d  slsy: %d\n",scanning_task_line,line,main_get_gapp()->xsm->hardware->RTQuery (), sls_config[2]);
                                scanning_task_line++;
                                last_scan_dir == SCAN_DIR_TOPDOWN ? ++line : --line;
                                if (fabs (line - (main_get_gapp()->xsm->hardware->RTQuery () - sls_config[2])) > 2){
                                        // g_message (" SPM_ScanControl::scanning_control_run line=%d   actual=%d  scanning_task_line= %d", line,  main_get_gapp()->xsm->hardware->RTQuery (), scanning_task_line);
                                        line = (main_get_gapp()->xsm->hardware->RTQuery () - sls_config[2]) + (SCAN_DIR_TOPDOWN ? -1 : +1);
                                }
                        }
		}
                return TRUE; // continue!
	}

	if (main_get_gapp()->xsm->hardware->EndScan2D ()){
                do_scanline (); // kepp refreshing until tip final position os reached.
                return TRUE; // wait, must keep calling this fucntion until return FALSE!
        }
        
        return FALSE; // completed
}

gboolean SPM_ScanControl::scanning_control_finish (){
	int stopped=0;
	PI_DEBUG (DBG_L2, "SPM_ScanControl::scanning_control_finish.");

        main_get_gapp()->set_toolbar_autosave_button (); // reset auto save button to full save symbol

	main_get_gapp()->SignalStopScanEventToPlugins ();

        g_slist_foreach ((GSList*) all_scan_list,
                         (GFunc) SPM_ScanControl::call_scan_stop, this);

        main_get_gapp()->spm_thaw_scanparam();

	if ((stopped = scan_flag == SCAN_FLAG_STOP ? TRUE:FALSE)){
		main_get_gapp()->SetStatus("Scan interrupted");
		main_get_gapp()->monitorcontrol->LogEvent("*EndOfScan", "interrupted");
	}
	else{
		main_get_gapp()->SetStatus("Scan done, ready");
		main_get_gapp()->monitorcontrol->LogEvent("*EndOfScan", "OK");
	}
	
	// free_scan_lists (); // keep
	scan_flag = SCAN_FLAG_READY;
        
        scan_stopped_by_user = stopped;

        g_message ("SPM_ScanControl::scan_control_finish, by user flg=%d\n", scan_stopped_by_user);
        
        return G_SOURCE_REMOVE;
}

// Execute One Scan Process: init, run, finish state machine task
gboolean SPM_ScanControl::scanning_task (gpointer spc){

        //g_print ("\n** ST_stage=%d\n", ((SPM_ScanControl *)spc)->scanning_task_stage);
        PI_DEBUG_GM (DBG_L3, "SPM_SCANCONTROL::scanning_task scanning_task_stage=%d", ((SPM_ScanControl *)spc)->scanning_task_stage);
                
        switch (((SPM_ScanControl *)spc)->scanning_task_stage){
        case 0 : return G_SOURCE_REMOVE;
        case 1 :
                ((SPM_ScanControl *)spc)->scanning_task_stage = 2;
                return ((SPM_ScanControl *)spc)->scanning_control_init ();
        case 2 :
                if (((SPM_ScanControl *)spc)->scanning_control_run ())
                        return TRUE;
                else
                        ((SPM_ScanControl *)spc)->scanning_task_stage = 3;
                return TRUE;
        case 3 :
                if (((SPM_ScanControl *)spc)->scanning_control_finish ())
                        return TRUE;
                else {
                        ((SPM_ScanControl *)spc)->scanning_task_stage = 0;
                        return G_SOURCE_REMOVE;
                }
        default : return G_SOURCE_REMOVE;
        }
        return G_SOURCE_REMOVE;
}

// Setup task for one pas scan process
int SPM_ScanControl::setup_scanning_control (int l){
        const gint64 timeout = 5000000; // 5s
        const gint64 print_interval = 200000; // 200ms
        static gint64 time_of_first_reading = 0; // abs time in us
        static gint64 time_of_last_reading = 0; // abs time in us
	PI_DEBUG (DBG_L2, "do_scan");

	if (scan_in_progress ()){
		PI_DEBUG (DBG_L2, "do_scan scan in progress, exiting.");
		return FALSE;
	}
        time_of_first_reading = g_get_real_time ();
        while (! main_get_gapp()->xsm->hardware->RTQuery_clear_to_start_scan ()){
                main_get_gapp()->check_events("Setup Scan Start is waiting for instrument ready.");
                if ( (time_of_last_reading+print_interval) < g_get_real_time () ){
                        main_get_gapp()->monitorcontrol->LogEvent ("Start Scan", "Instrument is busy with VP or conflciting task: Waiting.", 3);
                        g_warning ("Start Scan: Instrument is busy with VP or conflciting task. Waiting.");
                }
                time_of_last_reading = g_get_real_time ();
                if ( (time_of_first_reading+timeout) < g_get_real_time () ){
                        main_get_gapp()->monitorcontrol->LogEvent ("Start Scan", "Instrument is busy with VP or conflciting task: skipping/abort.", 3);
                        g_warning ("Start Scan: Instrument is busy with VP or conflciting task. Timeout reached, skipping/abort.");
                        return FALSE;
                }
        }


        scanning_task_multivolt_i = l;
        scanning_task_stage = 1; // start
        scan_stopped_by_user = 0;

        //gdk_threads_add_idle (SPM_ScanControl::scanning_task, this);

        return TRUE;
}


double SPM_ScanControl::update_status_info (int reset){
	static time_t tn, tnlog, t0;
	time_t t;

	if (reset){
		time (&t0); 
		tnlog=tn=t0;
		return 0.;
	}

	time (&t);
	// update only of more than 2s old
	if ((t-tn) > 2){
		double ss,s,m,h, sse,se,me,he;
		double n_by_lines_to_do;
		gchar *tt;
		tn=t;
		ss = s = tn-t0;
		h = floor (s/3600.);
		m = floor ((s-h*3600.)/60.);
		s-= h*3600.+m*60.;

                // fix me for sub-scan SLS!
		n_by_lines_to_do = (double)master_scan->data.s.ny
			/ (double)(last_scan_dir == SCAN_DIR_TOPDOWN ?
				   line+1 : master_scan->data.s.ny-line);
		sse = se = (double)(tn-t0)*n_by_lines_to_do;
		se -= ss; // ETA (Estimated Time left to Arrival at end of scan)
		he  = floor (se/3600.);
		me  = floor ((se-he*3600.)/60.);
		se -= he*3600.+me*60.;
		// 0123456789012345678901234567890
		// Mon Apr 17 09:10:17 2000
		if ((sse+ss) > 600){
			time_t t_eta = t+(time_t)(sse-ss);
			char *teos = ctime(&t_eta); 
			teos[24]=0;
			main_get_gapp()->SetStatus ("ScanTime",
					 tt=g_strdup_printf ("%.0f:%02.0f:%02.0f "
                                                             "| ETA:%.0f:%02.0f:%02.0f | %d%% "
                                                             "End of Scan: %s  %s",
                                                             h,m,s, he,me,se, 
                                                             (int)(100/n_by_lines_to_do), teos, 
                                                             main_get_gapp()->xsm->hardware->GetStatusInfo()?main_get_gapp()->xsm->hardware->GetStatusInfo():" "));
		}else
			main_get_gapp()->SetStatus ("ScanTime",
					 tt=g_strdup_printf ("%.0f:%02.0f:%02.0f "
                                                             "| ETA:%.0f:%02.0f:%02.0f | %d%%  %s",
                                                             h,m,s, he,me,se, 
                                                             (int)(100/n_by_lines_to_do),
                                                             main_get_gapp()->xsm->hardware->GetStatusInfo()?main_get_gapp()->xsm->hardware->GetStatusInfo():" "));

		if ((t-tnlog) > 60 || (sse+ss) < 600){
			tnlog=t;
			gchar *nl;
			if ((nl=strchr (tt, '\n'))) *nl=' '; // remove \n for logfile
			gchar *mt = g_strdup_printf ("Scan Progress [%d]", line);
			main_get_gapp()->monitorcontrol->LogEvent (mt, tt, 2);
			g_free(mt);
		}
		g_free(tt);

		t=tn+(time_t)sse; 
		return ss;
	}
	return 0.;
}

void SPM_ScanControl::autosave_check (double sec, int initvalue){
	static int nextAutosaveEvent=0;

	if (initvalue > 0){
		nextAutosaveEvent = initvalue;	
		return;
	}

	if ( (( !strncasecmp(xsmres.AutosaveUnit, "percent",2)) 
	      && ((100*line/master_scan->data.s.ny) >= nextAutosaveEvent))
	     ||
	     (( !strncasecmp(xsmres.AutosaveUnit, "lines"  ,2))
	      && (line >= nextAutosaveEvent))
	     ||
	     ((!strncasecmp(xsmres.AutosaveUnit, "seconds",2))
	      && (sec >= nextAutosaveEvent)) ){

		nextAutosaveEvent += xsmres.AutosaveValue;
		if(main_get_gapp()->xsm->IsMode(MODE_AUTOSAVE)){
			PI_DEBUG (DBG_L3, "Autosaveevent triggered.");

                        // use new update
                        main_get_gapp()->auto_update_scans ();
		}
	}
}
