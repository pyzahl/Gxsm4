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

/* Please do not change the Begin/End lines of this comment section!
 * this is a LaTeX style section used for auto generation of the PlugIn Manual 
 * Chapter. Add a complete PlugIn documentation inbetween the Begin/End marks!
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: RPSPMC PACPLL
% PlugInName: rpspmc_pacpll
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: windows-section RPSPMC PACPLL
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

#include "gxsm_app.h"
#include "gxsm_window.h"
#include "surface.h"

#include "../common/pyremote.h"

#include "rpspmc_pacpll.h"
#include "plug-ins/control/resonance_fit.h"


// Define HwI PlugIn reference name here, this is what is listed later within "Preferenced Dialog"
// i.e. the string selected for "Hardware/Card"!
#define THIS_HWI_PLUGIN_NAME "RPSPMC:SPM"
#define THIS_HWI_PREFIX      "RPSPMC_HwI"

#define GVP_SHIFT_UP  1
#define GVP_SHIFT_DN -1




#define CPN(N) ((double)(1LL<<(N))-1.)
#define DSP32Qs15dot16TOV     (10.0/(32767.*(1<<16)))
#define ADCV10     (10.0/32767.)
#define BiasFac    (main_get_gapp()->xsm->Inst->Dig2VoltOut (1.) * main_get_gapp()->xsm->Inst->BiasGainV2V ())
#define BiasOffset (main_get_gapp()->xsm->Inst->Dig2VoltOut (1.) * main_get_gapp()->xsm->Inst->BiasV2V (0.)) // not used here
#define CurrFac    (main_get_gapp()->xsm->Inst->Dig2VoltOut (1.) *1./main_get_gapp()->xsm->Inst->nAmpere2V (1.))
#define ZAngFac    (main_get_gapp()->xsm->Inst->Dig2ZA (1))
#define XAngFac    (main_get_gapp()->xsm->Inst->Dig2XA (1))
#define YAngFac    (main_get_gapp()->xsm->Inst->Dig2YA (1))

// Masks MUST BE unique
SOURCE_SIGNAL_DEF source_signals[] = {
        // -- 8 vector generated signals (outputs/mapping) ==> must match: #define NUM_VECTOR_SIGNALS 8
        { 0x01000000, "Index",    " ", "#", "#", 1.0, PROBEDATA_ARRAY_INDEX },
        { 0x02000000, "Time",     " ", "ms", "ms", 1.0/75, PROBEDATA_ARRAY_TIME }, // 1000/emu->frq_ref => ms
        { 0x00100000, "Bias",     " ", "V", "V", BiasFac, PROBEDATA_ARRAY_U },
        { 0x04000000, "SEC",      " ", "#", "#", 1.0, PROBEDATA_ARRAY_SEC },
        { 0x08000000, "XS",       " ", "AA", UTF8_ANGSTROEM, XAngFac, PROBEDATA_ARRAY_XS },
        { 0x10000000, "YS",       " ", "AA", UTF8_ANGSTROEM, YAngFac, PROBEDATA_ARRAY_YS },
        { 0x20000000, "ZS",       " ", "AA", UTF8_ANGSTROEM, ZAngFac, PROBEDATA_ARRAY_ZS },
        { 0x40000000, "DA",       " ", "V", "V", 1., PROBEDATA_ARRAY_DA },
        // -- general measured signals from index [8]
        { 0x00000001, "Z-mon",    " ", "AA", UTF8_ANGSTROEM, ZAngFac, PROBEDATA_ARRAY_S1 },
        { 0x00000002, "Bias-mon", " ", "V", "V", BiasFac, PROBEDATA_ARRAY_S2 },
	{ 0x00000010, "In1-Signal", " ", "V", "V", 1.0, PROBEDATA_ARRAY_S3 }, // <=== to Volt conversion here -- unit sym and scale are custom auto adjusted in .._eventhandling lookup functions as of this mask 
        { 0x00000020, "In2-Current", " ", "nA", "nA", CurrFac, PROBEDATA_ARRAY_S4 },
        { 0x00000040, "VP Zpos", " ", "AA", UTF8_ANGSTROEM, ZAngFac, PROBEDATA_ARRAY_S5 },
        { 0x00000080, "VP Bias", " ", "V", "V", BiasFac, PROBEDATA_ARRAY_S6 },
        { 0x00000100, "VP steps", " ", "pts", "pts", 1., PROBEDATA_ARRAY_S7 },
        { 0x00000200, "Clock", " ", "ms", "ms", 1., PROBEDATA_ARRAY_S8 },
        { 0x00000400, "IX", " ", "ix#", "ix#", 1, PROBEDATA_ARRAY_S9 },
        { 0x00000800, "ADC7", " ", "V", "V", ADCV10, PROBEDATA_ARRAY_S10 },
        { 0x00000008, "LockIn0", " ", "nA", "nA", ADCV10, PROBEDATA_ARRAY_S11 },
        { 0x00001000, "SWPS1-choose", " ", "V", "V", 1.0, PROBEDATA_ARRAY_S12 }, // ** swappable **,
        { 0x00002000, "SWPS2-chhose", " ", "V", "V", 1.0, PROBEDATA_ARRAY_S13 }, // ** swappable **,
        { 0x00004000, "SWPS3-choose", " ", "V", "V", 1.0, PROBEDATA_ARRAY_S14 }, // ** swappable **,
        { 0x00008000, "SWPS4-choose", " ", "V", "V", 1.0, PROBEDATA_ARRAY_S15 }, // ** swappable **,
        { 0x80000000, "BlockI", " ", "i#", "i#", 1, PROBEDATA_ARRAY_BLOCK },
        { 0x00000000, NULL, NULL, NULL, NULL, 0.0, 0 }
};

// so far fixed to swappable 4 signals as of GUI design!
SOURCE_SIGNAL_DEF swappable_signals[] = {
        { 0x00001000, "dFrequency", " ", "-", "--", 1.0, 0 },
        { 0x00002000, "Phase",      " ", "-", "--", 1.0, 0 },
        { 0x00004000, "Excitation", " ", "-", "--", 1.0, 0 },
        { 0x00008000, "Amplitude",  " ", "-", "--", 1.0, 0 },
        { 0x00000000, NULL, NULL, NULL, NULL, 0.0, 0 }
};







extern int debug_level;
extern int force_gxsm_defaults;


extern "C++" {
        extern RPspmc_pacpll *rpspmc_pacpll;
        extern GxsmPlugin rpspmc_pacpll_hwi_pi;
}


// Plugin Prototypes - default PlugIn functions
static void rpspmc_pacpll_hwi_init (void); // PlugIn init
static void rpspmc_pacpll_hwi_query (void); // PlugIn "self-install"
static void rpspmc_pacpll_hwi_about (void); // About
static void rpspmc_pacpll_hwi_configure (void); // Configure plugIn, called via PlugIn-Configurator
static void rpspmc_pacpll_hwi_cleanup (void); // called on PlugIn unload, should cleanup PlugIn rescources

// other PlugIn Functions and Callbacks (connected to Buttons, Toolbar, Menu)
static void rpspmc_pacpll_hwi_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void rpspmc_pacpll_hwi_SaveValues_callback ( gpointer );

// custom menu callback expansion to arbitrary long list
static PIMenuCallback rpspmc_pacpll_hwi_mcallback_list[] = {
		rpspmc_pacpll_hwi_show_callback,             // direct menu entry callback1
		NULL  // NULL terminated list
};

static GxsmPluginMenuCallbackList rpspmc_pacpll_hwi_menu_callback_list = {
	1,
	rpspmc_pacpll_hwi_mcallback_list
};

GxsmPluginMenuCallbackList *get_gxsm_plugin_menu_callback_list ( void ) {
	return &rpspmc_pacpll_hwi_menu_callback_list;
}

// Fill in the GxsmPlugin Description here -- see also: Gxsm/src/plugin.h
GxsmPlugin rpspmc_pacpll_hwi_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	"rpspmc_pacpll_hwi-"
	"HW-INT-NS-FLOAT",
	// Plugin's Category - used to autodecide on Pluginloading or ignoring
	// In this case of Hardware-Interface-Plugin here is the interface-name required
	// this is the string selected for "Hardware/Card"!
	THIS_HWI_PLUGIN_NAME,
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	g_strdup ("Red Pitaya SPM Control and PACPLL hardware interface."),
	// Author(s)
	"Percy Zahl",
	// Menupath to position where it is appendet to -- not used by HwI PIs
	"windows-section,windows-section,windows-section,windows-section",
	// Menuentry -- not used by HwI PIs
	N_("RPSPM Control"),
	// help text shown on menu
	N_("This is the " THIS_HWI_PLUGIN_NAME " - GXSM Hardware Interface."),
	// more info...
	"N/A",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	rpspmc_pacpll_hwi_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	rpspmc_pacpll_hwi_query, // query can be used (otherwise set to NULL) to install
	// additional control dialog in the GXSM menu
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	rpspmc_pacpll_hwi_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	rpspmc_pacpll_hwi_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
	NULL,   // direct menu entry callback1 or NULL
	NULL,   // direct menu entry callback2 or NULL
	rpspmc_pacpll_hwi_cleanup // plugin cleanup callback or NULL
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("RPSPMC PACPLL Control Plugin\n\n"
                                   "This plugin manages externa Scan Data Sources.\n"
	);

/* Here we go... */

RPSPMC_Control *RPSPMC_ControlClass = NULL;

// event hooks
static void RPSPMC_Control_StartScan_callback ( gpointer );
static void RPSPMC_Control_SaveValues_callback ( gpointer );
static void RPSPMC_Control_LoadValues_callback ( gpointer );

gchar *rpspmc_pacpll_hwi_configure_string = NULL;   // name of the currently in GXSM configured HwI (Hardware/Card)
RPspmc_pacpll *rpspmc_pacpll = NULL;
rpspmc_hwi_dev *rpspmc_hwi = NULL;


// Symbol "get_gxsm_hwi_hardware_class" is resolved by dlsym from Gxsm for all HwI type PIs, 
// Essential Plugin Function!!
XSM_Hardware *get_gxsm_hwi_hardware_class ( void *data ) {
        gchar *tmp;
        main_get_gapp()->monitorcontrol->LogEvent (THIS_HWI_PREFIX " XSM_Hardware *get_gxsm_hwi_hardware_class", "Init 1");

	rpspmc_pacpll_hwi_configure_string = g_strdup ((gchar*)data);

        
	// probe for harware here...
	rpspmc_hwi = new rpspmc_hwi_dev ();
	
        main_get_gapp()->monitorcontrol->LogEvent ("HwI: probing succeeded.", "RP SPM Control System Ready.");
	return rpspmc_hwi;
}

// init-Function
void rpspmc_pacpll_hwi_init(void)
{
	PI_DEBUG (DBG_L2, "rspmc_pacpll_hwi Plugin Init");
	rpspmc_pacpll = NULL;
}


// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	rpspmc_pacpll_hwi_pi.description = g_strdup_printf(N_("Gxsm rpspmc_pacpll plugin %s"), VERSION);
	return &rpspmc_pacpll_hwi_pi; 
}



// data passed to "idle" function call, used to refresh/draw while waiting for data
typedef struct {
	GSList *scan_list; // scans to update
	GFunc  UpdateFunc; // function to call for background updating
	gpointer data; // additional data (here: reference to the current rpspmc_pacpll object)
} IdleRefreshFuncData;


// Query Function, installs Plugin's in File/Import and Export Menupaths!

static void rpspmc_pacpll_hwi_query(void)
{
	if(rpspmc_pacpll_hwi_pi.status) g_free(rpspmc_pacpll_hwi_pi.status); 
	rpspmc_pacpll_hwi_pi.status = g_strconcat (
                                                 N_("Plugin query has attached "),
                                                 rpspmc_pacpll_hwi_pi.name, 
                                                 N_(": File IO Filters are ready to use"),
                                                 NULL);

//      Setup Control Windows
// ==================================================
	PI_DEBUG (DBG_L2, "rpspmc_pacpll_query:new" );

        // first
        rpspmc_pacpll = new RPspmc_pacpll (main_get_gapp() -> get_app ());

        // second
	RPSPMC_ControlClass = new RPSPMC_Control (main_get_gapp() -> get_app ());
        
	rpspmc_pacpll_hwi_pi.status = g_strconcat(N_("Plugin query has attached "),
                                     rpspmc_pacpll_hwi_pi.name, 
                                                  N_(": " THIS_HWI_PREFIX "-Control is created."),
                                                  NULL);


	PI_DEBUG (DBG_L2, "rpspmc_pacpll_query:res" );
	
	rpspmc_pacpll_hwi_pi.app->ConnectPluginToCDFSaveEvent (rpspmc_pacpll_hwi_SaveValues_callback);
}

static void rpspmc_pacpll_hwi_SaveValues_callback ( gpointer gp_ncf ){
	NcFile *ncf = (NcFile *) gp_ncf;
        if (rpspmc_pacpll)
                rpspmc_pacpll->save_values (ncf);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//



// about-Function 
static void rpspmc_pacpll_hwi_about(void)
{
        const gchar *authors[] = { rpspmc_pacpll_hwi_pi.authors, NULL};
        gtk_show_about_dialog (NULL,
                               "program-name",  rpspmc_pacpll_hwi_pi.name,
                               "version", VERSION,
                               "license", GTK_LICENSE_GPL_3_0,
                               "comments", about_text,
                               "authors", authors,
                               NULL
                               );
}

// configure-Function
static void rpspmc_pacpll_hwi_configure(void)
{
	if(rpspmc_pacpll_hwi_pi.app)
		rpspmc_pacpll_hwi_pi.app->message("rpspmc_pacpll Plugin Configuration");
}

// cleanup-Function, make sure the Menustrings are matching those above!!!
static void rpspmc_pacpll_hwi_cleanup(void)
{
	// delete ...
	if( rpspmc_pacpll )
		delete rpspmc_pacpll ;

	if(rpspmc_pacpll_hwi_pi.status) g_free(rpspmc_pacpll_hwi_pi.status); 

	PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::rpspmc_hwi_cleanup -- Plugin Cleanup, -- Menu [disabled]\n");

        // remove GUI
	if( RPSPMC_ControlClass ){
		delete RPSPMC_ControlClass ;
        }
	RPSPMC_ControlClass = NULL;

	if (rpspmc_hwi){
                PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::rpspmc_hwi_cleanup -- delete DSP Common-HwI-Class.");
		delete rpspmc_hwi;
        }
	rpspmc_hwi = NULL;

        PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::rpspmc_hwi_cleanup -- g_free rpspmc_pacpll_hwi_configure_string Info.");
	g_free (rpspmc_pacpll_hwi_configure_string);
	rpspmc_pacpll_hwi_configure_string = NULL;
	
        PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::rpspmc_pacpll_hwi_cleanup -- Done.");
}


static void rpspmc_pacpll_hwi_show_callback(GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	PI_DEBUG (DBG_L2, "rpspmc_pacpll Plugin : show" );
	if( rpspmc_pacpll )
		rpspmc_pacpll->show();
}

/* **************************************** SPM Control GUI **************************************** */

// GUI Section -- custom to HwI
// ================================================================================

// advanced GUI building support and building automatizations

GtkWidget*  GUI_Builder::grid_add_mixer_options (gint channel, gint preset, gpointer ref){
        gchar *id;
        GtkWidget *cbtxt = gtk_combo_box_text_new ();
        
        g_object_set_data (G_OBJECT (cbtxt), "mix_channel", GINT_TO_POINTER (channel)); 
                                                                        
        id = g_strdup_printf ("%d", MM_OFF);         gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "OFF"); g_free (id); 
        id = g_strdup_printf ("%d", MM_ON);          gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "LIN"); g_free (id); 
        id = g_strdup_printf ("%d", MM_NEG|MM_ON);   gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "NEG-LIN"); g_free (id); 
        id = g_strdup_printf ("%d", MM_LOG|MM_ON);   gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "LOG"); g_free (id); 

        gchar *preset_id = g_strdup_printf ("%d", preset); 
        gtk_combo_box_set_active_id (GTK_COMBO_BOX (cbtxt), preset_id);
        //g_print ("SetActive MIX%d p=%d <%s>\n", channel, preset, preset_id);
        g_free (preset_id);
                
        g_signal_connect (G_OBJECT (cbtxt),"changed",	
                          G_CALLBACK (RPSPMC_Control::choice_mixmode_callback), 
                          ref);				
                
        grid_add_widget (cbtxt);
        return cbtxt;
};

GtkWidget* GUI_Builder::grid_add_scan_input_signal_options (gint channel, gint preset, gpointer ref){ // preset=scan_source[CHANNEL]
        GtkWidget *cbtxt = gtk_combo_box_text_new (); 
        g_object_set_data(G_OBJECT (cbtxt), "scan_channel_source", GINT_TO_POINTER (channel)); 

        int jj=0;

        for (int jj=0; swappable_signals[jj].label; ++jj){
                gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, swappable_signals[jj].label); g_free (id);
        }
        
#if 0 // actual MK3 code
        for (int jj=0;  jj<NUM_SIGNALS_UNIVERSAL && sranger_common_hwi->lookup_dsp_signal_managed (jj)->p; ++jj){ 
                if (1 || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Analog_IN") 
                    || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Control") 
                    || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Counter") 
                    || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "LockIn") 
                    || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "VP") 
                    || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Z_Servo") 
                    || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "M_Servo") 
                    || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Mixer") 
                    || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "RMS") 
                    || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "PAC")){ 
                        if (sranger_common_hwi->lookup_dsp_signal_managed (jj)->index >= 0){ 
                                int si = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID+channel); 
                                const gchar *vjfixedlab[] = { "Counter 0", "Counter 1", "NULL", "NULL" }; 
                                int vi=sranger_common_hwi->lookup_dsp_signal_managed (si)->index/8; 
                                int vj=sranger_common_hwi->lookup_dsp_signal_managed (si)->index - 8*vi; 
                                gchar *tmp = g_strdup_printf("%s[%d]-%s", 
                                                             sranger_common_hwi->lookup_dsp_signal_managed (jj)->label, vi, 
                                                             vj < 4 
                                                             ? sranger_common_hwi->lookup_dsp_signal_managed (sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE0_INPUT_ID+vj))->label 
                                                             : vjfixedlab[vj-4] 
                                                             );	
                                { gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, tmp); g_free (id); } 
                                g_free (tmp); 
                        } else {				
                                { gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, sranger_common_hwi->lookup_dsp_signal_managed (jj)->label); g_free (id); } 
                        } 
                } 
        }
#endif
        gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), preset); 
        g_signal_connect (G_OBJECT (cbtxt), "changed",	
                          G_CALLBACK (RPSPMC_Control::choice_scansource_callback), 
                          ref);
        grid_add_widget (cbtxt);
        return cbtxt;
};


GtkWidget* GUI_Builder::grid_add_probe_source_signal_options (gint channel, gint preset, gpointer ref){ // preset=probe_source[CHANNEL])
        GtkWidget *cbtxt = gtk_combo_box_text_new (); 
        gtk_widget_set_size_request (cbtxt, 50, -1); 
        g_object_set_data(G_OBJECT (cbtxt), "prb_channel_source", GINT_TO_POINTER (channel)); 


        for (int jj=0; swappable_signals[jj].label; ++jj){
                gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, swappable_signals[jj].label); g_free (id);
        }

#if 0        
        for (int jj=0;  jj<NUM_SIGNALS_UNIVERSAL && sranger_common_hwi->lookup_dsp_signal_managed (jj)->p; ++jj){ 
                gchar *id = g_strdup_printf ("%d", jj);
                const gchar *label = sranger_common_hwi->lookup_dsp_signal_managed (jj)->label;
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, label?label:"???");
                g_free (id);
        }
#endif
        if (preset >= 0)
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), preset); 
        else
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 4); // NULL SIGNAL [TESTING FALLBACK for -1/error]
                
        g_signal_connect (G_OBJECT (cbtxt), "changed",	
                          G_CALLBACK (RPSPMC_Control::choice_prbsource_callback), 
                          ref);				
        grid_add_widget (cbtxt);
        return cbtxt;
};




GtkWidget* GUI_Builder::grid_add_probe_status (const gchar *status_label) {
        GtkWidget *inp = grid_add_input (status_label);
        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(inp))), " --- ", -1);
        gtk_editable_set_editable (GTK_EDITABLE (inp), FALSE);
        return inp;
};

void GUI_Builder::grid_add_probe_controls (gboolean have_dual,
                              guint64 option_flags, GCallback option_cb,
                              guint64 auto_flags, GCallback auto_cb,
                              GCallback exec_cb, GCallback write_cb, GCallback graph_cb, GCallback x_cb,
                              gpointer cb_data,
                              const gchar *control_id) {                                                            

        new_grid_with_frame ("Probe Controller");
        new_grid ();
        gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);
        gtk_widget_set_hexpand (grid, TRUE);
                
        // ==================================================
        set_configure_list_mode_on ();
        // OPTIONS
        grid_add_check_button_guint64 ("Feedback On",
                                       "Check to keep Z feedback on while probe.", 1,
                                       GCallback (option_cb), cb_data, option_flags, FLAG_FB_ON); 
        if (have_dual) {                                         
                grid_add_check_button_guint64 ("Dual Mode",
                                               "Check to run symmetric probe:\n"
                                               "take reverse data to start.", 1,
                                               GCallback (option_cb), cb_data, option_flags, FLAG_DUAL); 
        }                                                       
        grid_add_check_button_guint64 ("Oversampling", "Check do enable DSP data oversampling:\n"
                                       "integrates data at intermediate points (averaging).\n"
                                       "(recommended)", 1,
                                       GCallback (option_cb), cb_data, option_flags, FLAG_INTEGRATE); 
        grid_add_check_button_guint64 ("Full Ramp", "Check do enable data acquisition on all probe segments:\n"
                                       "including start/end ramps and delays.", 1,
                                       GCallback (option_cb), cb_data, option_flags, FLAG_SHOW_RAMP); 
        // AUTO MODES
        grid_add_check_button_guint64 ("Auto Plot", "Enable life data plotting:\n"
                                       "disable to save resources and CPU time for huge data sets\n"
                                       "and use manual plot update if needed.", 1,
                                       GCallback (auto_cb), cb_data, auto_flags, FLAG_AUTO_PLOT); 
        grid_add_check_button_guint64 ("Auto Save", "Enable save data auutomatically at competion.\n"
                                       "(recommended)", 1,
                                       GCallback (auto_cb), cb_data, auto_flags, FLAG_AUTO_SAVE); 
        grid_add_check_button_guint64 ("GLock", "Lock Data/Graphs Configuration.\n"
                                       "(recommended to check after setup and one test run)", 1,
                                       GCallback (auto_cb), cb_data, auto_flags, FLAG_AUTO_GLOCK); 
        grid_add_check_button_guint64 ("Run Init Script", "Run Init Script before VP execution.\n"
                                       "(use only if init script (see python console, Actions Scripts) for VP-mode is created and reviewed, you can configure any initial condtions like bias, setpoint for example.)", 1,
                                       GCallback (auto_cb), cb_data, auto_flags, FLAG_AUTO_RUN_INITSCRIPT);
        set_configure_list_mode_off ();
        // ==================================================
        GtkWidget *tmp = grid;
        pop_grid ();
        grid_add_widget (tmp);
                                                                        
        new_line ();

        new_grid ();
        gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);
        gtk_widget_set_hexpand (grid, TRUE);
                
        remote_action_cb *ra = g_new( remote_action_cb, 1);     
        ra -> cmd = g_strdup_printf("DSP_VP_%s_EXECUTE", control_id); 
        gchar *help = g_strconcat ("Remote example: action (", ra->cmd, ")", NULL); 
        grid_add_button ("Execute", help, 2,
                         GCallback (exec_cb), cb_data);
        g_free (help);
        ra -> RemoteCb = (void (*)(GtkWidget*, void*))exec_cb;  
        ra -> widget = button;                                  
        ra -> data = cb_data;                                      

                
        main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra ); 
        PI_DEBUG (DBG_L2, "Adding new Remote Cmd: " << ra->cmd ); 
                                                                        
        // ==================================================
        set_configure_list_mode_on ();
        grid_add_button ("Write Vectors",
                         "Update Vector Program on DSP.\n"
                         "USE WITH CAUTION: possibel any time, but has real time consequences,\n"
                         "may be used to accellerate/abort VP.", 1,
                         GCallback (write_cb), cb_data);
        set_configure_list_mode_off ();
        // ==================================================
		                                                        
        grid_add_button ("Plot",
                         "Manually update all graphs.", 1,
                         GCallback (graph_cb), cb_data);         
                                                                        
        grid_add_button ("Save",
                         "Save all VP data, file name is auto generated.", 1,
                         G_CALLBACK (RPSPMC_Control::Probing_save_callback), cb_data);         

        // ==================================================
        set_configure_list_mode_on ();
        grid_add_button ("ABORT", "Cancel VP Program (writes and executes Zero Vector Program)\n"
                         "USE WITH CAUTION: pending data will get corrupted.", 1,
                         GCallback (x_cb), cb_data);
        set_configure_list_mode_off ();
        // ==================================================

        tmp = grid;
        pop_grid ();
        grid_add_widget (tmp);
        pop_grid ();
};


static GActionEntry win_RPSPMC_popup_entries[] = {
        { "dsp-mover-configure", RPSPMC_Control::configure_callback, NULL, "true", NULL },
};

void RPSPMC_Control_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
        ; // show window... if dummy closed it...
}



void RPSPMC_Control::configure_callback (GSimpleAction *action, GVariant *parameter, 
                                          gpointer user_data){
        RPSPMC_Control *mc = (RPSPMC_Control *) user_data;
        GVariant *old_state, *new_state;

        if (action){
                old_state = g_action_get_state (G_ACTION (action));
                new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
                
                g_simple_action_set_state (action, new_state);

                PI_DEBUG_GP (DBG_L4, "Toggle action %s activated, state changes from %d to %d\n",
                             g_action_get_name (G_ACTION (action)),
                             g_variant_get_boolean (old_state),
                             g_variant_get_boolean (new_state));
                g_simple_action_set_state (action, new_state);
                g_variant_unref (old_state);

                g_settings_set_boolean (mc->hwi_settings, "configure-mode", g_variant_get_boolean (new_state));
        } else {
                // new_state = g_variant_new_boolean (true);
                new_state = g_variant_new_boolean (g_settings_get_boolean (mc->hwi_settings, "configure-mode"));
        }

	if (g_variant_get_boolean (new_state)){
                g_slist_foreach
                        ( mc->bp->get_configure_list_head (),
                          (GFunc) App::show_w, NULL
                          );
                g_slist_foreach
                        ( mc->bp->get_config_checkbutton_list_head (),
                          (GFunc) RPSPMC_Control::show_tab_to_configure, NULL
                          );
        } else  {
                g_slist_foreach
                        ( mc->bp->get_configure_list_head (),
                          (GFunc) App::hide_w, NULL
                          );
                g_slist_foreach
                        ( mc->bp->get_config_checkbutton_list_head (),
                          (GFunc) RPSPMC_Control::show_tab_as_configured, NULL
                          );
        }
        if (!action){
                g_variant_unref (new_state);
        }
}


//
// dconf "settings" managements for custom non PCS managed parameters
//



void RPSPMC_Control::store_values (){
        g_settings_set_int (hwi_settings, "probe-sources", Source);
        g_settings_set_int (hwi_settings, "probe-sources-x", XSource);
        g_settings_set_boolean (hwi_settings, "probe-x-join", XJoin);
        g_settings_set_boolean (hwi_settings, "probe-graph-matrix-window", GrMatWin);
        g_settings_set_int (hwi_settings, "probe-p-sources", PSource);
        g_settings_set_int (hwi_settings, "probe-pavg-sources", PlotAvg);
        g_settings_set_int (hwi_settings, "probe-psec-sources", PlotSec);

        //set_tab_settings ("AC", AC_option_flags, AC_auto_flags, AC_glock_data);
        set_tab_settings ("IV", IV_option_flags, IV_auto_flags, IV_glock_data);
        //set_tab_settings ("FZ", FZ_option_flags, FZ_auto_flags, FZ_glock_data);
        //set_tab_settings ("PL", PL_option_flags, PL_auto_flags, PL_glock_data);
        //set_tab_settings ("LP", LP_option_flags, LP_auto_flags, LP_glock_data);
        //set_tab_settings ("SP", SP_option_flags, SP_auto_flags, SP_glock_data);
        //set_tab_settings ("TS", TS_option_flags, TS_auto_flags, TS_glock_data);
        set_tab_settings ("VP", GVP_option_flags, GVP_auto_flags, GVP_glock_data);
        //set_tab_settings ("TK", TK_option_flags, TK_auto_flags, TK_glock_data);
        //set_tab_settings ("AX", AX_option_flags, AX_auto_flags, AX_glock_data);
        //set_tab_settings ("AB", ABORT_option_flags, ABORT_auto_flags, ABORT_glock_data);

        GVP_store_vp ("VP_set_last"); // last in view
        PI_DEBUG_GM (DBG_L3, "RPSPMC_Control::store_values complete.");
}

void RPSPMC_Control::GVP_store_vp (const gchar *key){
	PI_DEBUG_GM (DBG_L3, "GVP-VP store to memo %s", key);
	g_message ("GVP-VP store to memo %s", key);
        GVariant *v = g_settings_get_value (hwi_settings, "probe-gvp-vector-program-matrix");
        //GVariant *v = g_settings_get_value (hwi_settings, "probe-lm-vector-program-matrix");
        GVariantDict *dict = g_variant_dict_new (v);
        if (!dict){
                PI_DEBUG_GW (DBG_L1, "ERROR: RPSPMC_Control::GVP_store_vp -- can't read dictionary 'probe-gvp-vector-program-matrix' a{sv}.");
                return;
        }

        gint32 vp_program_length;
        for (vp_program_length=0; GVP_points[vp_program_length] > 0; ++vp_program_length);
                
        gsize    n = MIN (vp_program_length+1, N_GVP_VECTORS);
        GVariant *pc_array_du = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_du, n, sizeof (double));
        GVariant *pc_array_dx = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_dx, n, sizeof (double));
        GVariant *pc_array_dy = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_dy, n, sizeof (double));
        GVariant *pc_array_dz = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_dz, n, sizeof (double));
        GVariant *pc_array_da = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_da, n, sizeof (double));
        GVariant *pc_array_db = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_db, n, sizeof (double));
        GVariant *pc_array_ts = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_ts, n, sizeof (double));
        GVariant *pc_array_pn = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_points, n, sizeof (gint32));
        GVariant *pc_array_op = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_opt, n, sizeof (gint32));
        GVariant *pc_array_vn = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_vnrep, n, sizeof (gint32));
        GVariant *pc_array_vp = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_vpcjr, n, sizeof (gint32));

        GVariant *pc_array[] = { pc_array_du, pc_array_dx, pc_array_dy, pc_array_dz, pc_array_da, pc_array_db, pc_array_ts,
                                 pc_array_pn, pc_array_op, pc_array_vn, pc_array_vp,
                                 NULL };
        const gchar *vckey[] = { "du", "dx", "dy", "dz", "da", "db", "ts", "pn", "op", "vn", "vp", NULL };

        for (int i=0; vckey[i] && pc_array[i]; ++i){
                gchar *m_vckey = g_strdup_printf ("%s-%s", vckey[i], key);

                g_print ("GVP store: %s = %s\n", m_vckey, g_variant_print (pc_array[i], true));

                if (g_variant_dict_contains (dict, m_vckey)){
                        if (!g_variant_dict_remove (dict, m_vckey)){
                                PI_DEBUG_GW (DBG_L1, "ERROR: RPSPMC_Control::GVP_store_vp -- key '%s' found, but removal failed.", m_vckey);
                                g_free (m_vckey);
                                return;
                        }
                }
                g_variant_dict_insert_value (dict, m_vckey, pc_array[i]);
                g_free (m_vckey);
        }

        GVariant *probe_vector_program_matrix = g_variant_dict_end (dict);
        g_settings_set_value (hwi_settings, "probe-gvp-vector-program-matrix", probe_vector_program_matrix);
        //g_settings_set_value (hwi_settings, "probe-lm-vector-program-matrix", probe_vector_program_matrix);

        // all g_variants created here are "consumed" by the "set" calls, if I try to unref, it cause random crashes.
        //g_variant_unref (probe_vector_program_matrix);
        //g_variant_dict_unref (dict);
        // Can't do, don't need??? -- getting this: GLib-CRITICAL **: g_variant_unref: assertion 'value->ref_count > 0' failed
        //        for (int i=0; pc_array[i]; ++i)
        //                g_variant_unref (pc_array[i]);
}

void RPSPMC_Control::GVP_restore_vp (const gchar *key){
	// g_message ("GVP-VP restore memo key=%s", key);
	PI_DEBUG_GP (DBG_L2, "GVP-VP restore memo %s\n", key);
	g_message ( "GVP-VP restore memo %s\n", key);
        GVariant *v = g_settings_get_value (hwi_settings, "probe-gvp-vector-program-matrix");
        //GVariant *v = g_settings_get_value (hwi_settings, "probe-lm-vector-program-matrix");
        GVariantDict *dict = g_variant_dict_new (v);
        if (!dict){
                PI_DEBUG_GP (DBG_L2, "ERROR: RPSPMC_Control::GVP_restore_vp -- can't read dictionary 'probe-gvp-vector-program-matrix' a{sv}.\n");
                return;
        }
        gsize  n; // == N_GVP_VECTORS;
        GVariant *vd[7];
        GVariant *vi[4];
        double *pc_array_d[7];
        gint32 *pc_array_i[4];
        const gchar *vckey_d[] = { "du", "dx", "dy", "dz", "da", "db", "ts", NULL };
        const gchar *vckey_i[] = { "pn", "op", "vn", "vp", NULL };
        double *GVPd[] = { GVP_du, GVP_dx, GVP_dy, GVP_dz, GVP_da, GVP_db, GVP_ts, NULL };
        gint32 *GVPi[] = { GVP_points, GVP_opt, GVP_vnrep, GVP_vpcjr, NULL };
        gint32 vp_program_length=0;
        
        for (int i=0; vckey_i[i]; ++i){
                gchar *m_vckey = g_strdup_printf ("%s-%s", vckey_i[i], key);
                g_message ( "GVP-VP restore %s\n", m_vckey);

                for (int k=0; k<N_GVP_VECTORS; ++k) GVPi[i][k]=0; // zero init vector
                if ((vi[i] = g_variant_dict_lookup_value (dict, m_vckey, ((const GVariantType *) "ai"))) == NULL){
                        PI_DEBUG_GP (DBG_L2, "GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_i '%s' memo not found. Setting to Zero.\n", m_vckey);
                        // g_warning ("GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_i '%s' memo not found. Setting to Zero.\n", m_vckey);
                        g_free (m_vckey);
                        continue;
                }
                g_print ("GVP restore: %s = %s\n", m_vckey, g_variant_print (vi[i], true));

                pc_array_i[i] = (gint32*) g_variant_get_fixed_array (vi[i], &n, sizeof (gint32));
                if (i==0) // actual length of this vector should fit all others -- verify
                        vp_program_length=n;
                else
                        if (n != vp_program_length)
                                g_warning ("GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_i '%s' vector length %ld not matching program n=%d.\n", m_vckey, n, vp_program_length);
                // g_assert_cmpint (n, ==, N_GVP_VECTORS);
                for (int k=0; k<n && k<N_GVP_VECTORS; ++k)
                        GVPi[i][k]=pc_array_i[i][k];
                g_free (m_vckey);
                g_variant_unref (vi[i]);
        }                        

        for (int i=0; vckey_d[i]; ++i){
                gchar *m_vckey = g_strdup_printf ("%s-%s", vckey_d[i], key);
                for (int k=0; k<N_GVP_VECTORS; ++k) GVPd[i][k]=0.; // zero init vector
                if ((vd[i] = g_variant_dict_lookup_value (dict, m_vckey, ((const GVariantType *) "ad"))) == NULL){
                        PI_DEBUG_GP (DBG_L2, "GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_d '%s' memo not found. Setting to Zero.", m_vckey);
                        // g_warning ("GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_d '%s' memo not found. Setting to Zero.", m_vckey);
                        g_free (m_vckey);
                        continue;
                }
                g_print ("GVP restore: %s = %s\n", m_vckey, g_variant_print (vd[i], true));

                pc_array_d[i] = (double*) g_variant_get_fixed_array (vd[i], &n, sizeof (double));
                //g_assert_cmpint (n, ==, N_GVP_VECTORS);
                if (n != vp_program_length)
                        g_warning ("GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_d '%s' vector length %ld not matching program n=%d.\n", m_vckey, n, vp_program_length);
                for (int k=0; k<vp_program_length && k<N_GVP_VECTORS; ++k)
                        GVPd[i][k]=pc_array_d[i][k];
                g_free (m_vckey);
                g_variant_unref (vd[i]);
        }                        
        
        g_variant_dict_unref (dict);
        g_variant_unref (v);

	update_GUI ();
}

int RPSPMC_Control::callback_edit_GVP (GtkWidget *widget, RPSPMC_Control *dspc){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
        int x=1, y=10;
	int ki = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "VPC"));
	int  a = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "ARROW"));

	if (ki < 0){
		const int VPT_YPAD=0;
		int c = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(dspc->VPprogram[0]), "CF"));
		int cw = 1;
		if (dspc->VPprogram[1]){
			for (int j=1; j<10; ++j)
				if (dspc->VPprogram[j]){
                                        // FIX-ME GTK4 ???
					gtk_window_destroy (GTK_WINDOW (dspc->VPprogram[j]));
					dspc->VPprogram[j] = NULL;
				}
		} else
			for (int k=0; k<N_GVP_VECTORS && cw < 10; ++k){
				if (dspc->GVP_vpcjr[k] < 0){
					int kf=k;
					int ki=k+dspc->GVP_vpcjr[k];
					if (kf >= 0){
                                                // fix!! todo
						//** ADD_BUTTON_GRID ("arrow-up-symbolic", "Loop",   dspc->VPprogram[0], x+0, y+ki+1, 1, kf-ki+2, -2, NULL, NULL, dspc->VPprogram[cw]);
                                                //ADD_BUTTON_TAB(GTK_ARROW_UP, "Loop",   dspc->VPprogram[0], c+0, c+1, ki+1, kf+2, GTK_FILL, GTK_FILL, 0, VPT_YPAD, -1, -2, NULL, NULL, dspc->VPprogram[cw]);
						++c; ++cw;
					}
				}
			}
		return 0;
	}

	if (a == GVP_SHIFT_UP && ki >= 1 && ki < N_GVP_VECTORS)
		for (int k=ki-1; k < N_GVP_VECTORS-1; ++k){
			int ks = k+1;
			dspc->GVP_du[k] = dspc->GVP_du[ks];
			dspc->GVP_dx[k] = dspc->GVP_dx[ks];
			dspc->GVP_dy[k] = dspc->GVP_dy[ks];
			dspc->GVP_dz[k] = dspc->GVP_dz[ks];
			dspc->GVP_da[k] = dspc->GVP_da[ks];
			dspc->GVP_db[k] = dspc->GVP_db[ks];
			dspc->GVP_ts[k]  = dspc->GVP_ts[ks];
			dspc->GVP_points[k] = dspc->GVP_points[ks];
			dspc->GVP_opt[k] = dspc->GVP_opt[ks];
			dspc->GVP_vnrep[k] = dspc->GVP_vnrep[ks];
			dspc->GVP_vpcjr[k] = dspc->GVP_vpcjr[ks];
		} 
	else if (a == GVP_SHIFT_DN && ki >= 0 && ki < N_GVP_VECTORS-2)
		for (int k=N_GVP_VECTORS-1; k > ki; --k){
			int ks = k-1;
			dspc->GVP_du[k] = dspc->GVP_du[ks];
			dspc->GVP_dx[k] = dspc->GVP_dx[ks];
			dspc->GVP_dy[k] = dspc->GVP_dy[ks];
			dspc->GVP_dz[k] = dspc->GVP_dz[ks];
			dspc->GVP_da[k] = dspc->GVP_da[ks];
			dspc->GVP_db[k] = dspc->GVP_db[ks];
			dspc->GVP_ts[k]  = dspc->GVP_ts[ks];
			dspc->GVP_points[k] = dspc->GVP_points[ks];
			dspc->GVP_opt[k] = dspc->GVP_opt[ks];
			dspc->GVP_vnrep[k] = dspc->GVP_vnrep[ks];
			dspc->GVP_vpcjr[k] = dspc->GVP_vpcjr[ks];
		}
	dspc->update_GUI ();
        return 0;
}


// NetCDF support for parameter storage to file

// helper func
NcVar* rpspmc_pacpll_hwi_ncaddvar (NcFile *ncf, const gchar *varname, const gchar *varunit, const gchar *longname, const gchar *shortname, double value){
	NcVar* ncv = ncf->add_var (varname, ncDouble);
	ncv->add_att ("long_name", longname);
	ncv->add_att ("short_name", shortname);
	ncv->add_att ("var_unit", varunit);
	ncv->put (&value);
	return ncv;
}
NcVar* rpspmc_pacpll_hwi_ncaddvar (NcFile *ncf, const gchar *varname, const gchar *varunit, const gchar *longname, const gchar *shortname, int value){
	NcVar* ncv = ncf->add_var (varname, ncInt);
	ncv->add_att ("long_name", longname);
	ncv->add_att ("short_name", shortname);
	ncv->add_att ("var_unit", varunit);
	ncv->put (&value);
	return ncv;
}

#define SPMTMPL_ID "rpspmc_pacpll_hwi_"

void RPSPMC_Control::save_values (NcFile *ncf){
	NcVar *ncv;

	PI_DEBUG (DBG_L4, "RPSPMC_Control::save_values");
	gchar *i=NULL;

        i = g_strconcat ("RP SPM Control HwI ** Hardware-Info:\n", rpspmc_hwi->get_info (), NULL);

	NcDim* infod  = ncf->add_dim("rpspmc_info_dim", strlen(i));
	NcVar* info   = ncf->add_var("rpspmc_info", ncChar, infod);
	info->add_att("long_name", "RPSPMC_Control plugin information");
	info->put(i, strlen(i));
	g_free (i);

// Basic Feedback/Scan Parameter ============================================================

	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"bias", "V", "SRanger: (Sampel or Tip) Bias Voltage", "Bias", bias);
	ncv->add_att ("label", "Bias");
	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"z_setpoint", "A", "SRanger: auxillary/Z setpoint", "Z Set Point", zpos_ref);
	ncv->add_att ("label", "Z Setpoint");

	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix0_set_point", "nA", "SRanger: Mix0: Current set point", "Current Setpt.", mix_set_point[0]);
	ncv->add_att ("label", "Current");
	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix1_set_point", "Hz", "SRanger: Mix1: Voltage set point", "Voltage Setpt.", mix_set_point[1]);
	ncv->add_att ("label", "VoltSetpt.");
	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix2_set_point", "V", "SRanger: Mix2: Aux2 set point", "Aux2 Setpt.", mix_set_point[2]);
	ncv->add_att ("label", "Aux2 Setpt.");
	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix3_set_point", "V", "SRanger: Mix3: Aux3 set point", "Aux3 Setpt.", mix_set_point[3]);
	ncv->add_att ("label", "Aux3 Setpt.");

	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix0_mix_gain", "1", "SRanger: Mix0 gain", "Current gain", mix_gain[0]);
	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix1_mix_gain", "1", "SRanger: Mix1 gain", "Voltage gain", mix_gain[1]);
	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix2_mix_gain", "1", "SRanger: Mix2 gain", "Aux2 gain", mix_gain[2]);
	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix3_mix_gain", "1", "SRanger: Mix3 gain", "Aux3 gain", mix_gain[3]);

	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix0_mix_level", "1", "SRanger: Mix0 level", "Current level", mix_level[0]);
	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix1_mix_level", "1", "SRanger: Mix1 level", "Voltage level", mix_level[1]);
	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix2_mix_level", "1", "SRanger: Mix2 level", "Aux2 level", mix_level[2]);
	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix3_mix_level", "1", "SRanger: Mix3 level", "Aux3 level", mix_level[3]);

	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix0_current_mix_transform_mode", "BC", "SRanger: Mix0 transform_mode", "Current transform_mode", (double)mix_transform_mode[0]);
	ncv->add_att ("mode_bcoding", "0:Off, 1:On, 2:Log, 4:IIR, 8:FUZZY");
	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix1_voltage_mix_transform_mode", "BC", "SRanger: Mix1 transform_mode", "Voltage transform_mode", (double)mix_transform_mode[1]);
	ncv->add_att ("mode_bcoding", "0:Off, 1:On, 2:Log, 4:IIR, 8:FUZZY");
	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix2_aux2_mix_transform_mode", "BC", "SRanger: Mix2 transform_mode", "Aux2 transform_mode", (double)mix_transform_mode[2]);
	ncv->add_att ("mode_bcoding", "0:Off, 1:On, 2:Log, 4:IIR, 8:FUZZY");
	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"mix3_aux3_mix_transform_mode", "BC", "SRanger: Mix3 transform_mode", "Aux3 transform_mode", (double)mix_transform_mode[3]);
	ncv->add_att ("mode_bcoding", "0:Off, 1:On, 2:Log, 4:IIR, 8:FUZZY");


	ncv=rpspmc_pacpll_hwi_ncaddvar (ncf, SPMTMPL_ID"move_speed_x", "A/s", "SRanger: Move speed X", "Xm Velocity", move_speed_x);
	ncv->add_att ("label", "Velocity Xm");



// Vector Probe ============================================================

}


#define NC_GET_VARIABLE(VNAME, VAR) if(ncf->get_var(VNAME)) ncf->get_var(VNAME)->get(VAR)

void RPSPMC_Control::load_values (NcFile *ncf){
	PI_DEBUG (DBG_L4, "RPSPMC_Control::load_values");
	// Values will also be written in old style DSP Control window for the reason of backwards compatibility
	// OK -- but will be obsoleted and removed at any later point -- PZ
	NC_GET_VARIABLE ("rpspmc_pacpll_hwi_bias", &bias);
	NC_GET_VARIABLE ("rpspmc_pacpll_hwi_bias", &main_get_gapp()->xsm->data.s.Bias);
        NC_GET_VARIABLE ("rpspmc_pacpll_hwi_set_point1", &mix_set_point[1]);
        NC_GET_VARIABLE ("rpspmc_pacpll_hwi_set_point0", &mix_set_point[0]);

	update_GUI ();
}







void RPSPMC_Control::get_tab_settings (const gchar *tab_key, guint64 &option_flags, guint64 &auto_flags, guint64 glock_data[6]) {
        PI_DEBUG_GP (DBG_L4, "RPSPMC_Control::get_tab_settings for tab '%s'\n", tab_key);

        gint dbg_level = 0;

#define PI_DEBUG_GPX(X, ARGS...) g_print (ARGS);
        
        GVariant *v = g_settings_get_value (hwi_settings, "probe-tab-options");

        // PI_DEBUG_GPX (dbg_level, "probe-tab-options type is '%s'\n", g_variant_get_type (v));
        //        PI_DEBUG_GPX (dbg_level, "probe-tab-options=%s\n", g_variant_print (v,true));
        
        GVariantDict *dict = g_variant_dict_new (v);

        if (!dict){
                PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::get_tab_settings -- can't read dictionary 'probe-tab-options' a{sv}.\n");
                return;
        }
#if 0
        if (!g_variant_dict_contains (dict, tab_key)){
                PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::get_tab_settings -- can't find key '%s' in dict 'probe-tab-options'.\n", tab_key);
        }
        PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::get_tab_settings -- key '%s' found in dict 'probe-tab-options' :)\n", tab_key);
#endif
        
        GVariant *value = g_variant_dict_lookup_value (dict, tab_key, ((const GVariantType *) "at")); // array uint64
        if (!value){
                g_warning ("WARNING (Normal at first start) -- Note: only at FIRST START: Building Settings. RPSPMC_Control::get_tab_settings:\n --> can't get find array 'at' data for key '%s' in 'probe-tab-options' dictionary.\n Storing default now.", tab_key);
                g_print ("WARNING (Normal at first start) -- Note: only at FIRST START: Building Settings. RPSPMC_Control::get_tab_settings:\n --> can't get find array 'at' data for key '%s' in 'probe-tab-options' dictionary.\n Storing default now.", tab_key);

                if (dict)
                        g_variant_dict_unref (dict);
                if (value)
                        g_variant_unref (value);
                
                // make default -- at first start, it is missing as can not be define in schema.
                // uint64 @at [18446744073709551615,..]
                GVariant *v_default = g_variant_new_parsed ("@a{sv} {"
                                                            "'AC': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'IV': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'FZ': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'PL': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'LP': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'SP': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'TS': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'VP': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            //"'GVP': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'TK': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'AX': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'AB': <@at [0,0,0,0,0,0,0,0,0]>"
                                                            "}");
                dict = g_variant_dict_new (v_default);
                value = g_variant_dict_lookup_value (dict, tab_key, ((const GVariantType *) "at"));
                g_variant_unref (v_default);
        }
        if (value){
                guint64  *ai = NULL;
                gsize     ni;
                
                ai = (guint64*) g_variant_get_fixed_array (value, &ni, sizeof (guint64));
                g_assert_cmpint (ni, ==, 9);

#if 0
                PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::get_tab_settings -- reading '%s' tab options [", tab_key);
                for (int i=0; i<ni; ++i)
                        PI_DEBUG_GPX (DBG_L4, "0x%08x, ", ai[i]);
                PI_DEBUG_GPX (dbg_level, "]\n");
#endif
                option_flags = ai[1];
                auto_flags = ai[2];
      
                for (int i=0; i<6; ++i)
                        glock_data [i] = ai[1+2+i];

                // must free data for ourselves
                g_variant_unref (value);
        } else {
                PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::get_tab_settings -- cant get array 'at' from key '%s' in 'probe-tab-options' dictionary. And default rewrite failed also.\n", tab_key);
        }

        GVariant *vto = g_variant_dict_end (dict);

        // testing storing
#if 0
        if (vto){
                PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::get_tab_settings -- stored tab-setings dict to variant OK.\n");
        } else {
                PI_DEBUG_GPX (dbg_level, "ERROR RPSPMC_Control::get_tab_settings -- store tab-setings dict to variant failed.\n");
        }
        if (g_settings_is_writable (hwi_settings, "probe-tab-options")){
                PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::get_tab_settings -- is writable\n");
        } else {
                PI_DEBUG_GPX (dbg_level, "WARNING/ERROR RPSPMC_Control::get_tab_settings -- is NOT writable\n");
        }
#endif
        if (!g_settings_set_value (hwi_settings, "probe-tab-options", vto)){
                PI_DEBUG_GPX (dbg_level, "ERROR RPSPMC_Control::get_tab_settings -- update probe-tab-options dict failed.\n");
        }
        g_variant_dict_unref (dict);
        //        g_variant_unref (vto);
        // PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::get_tab_settings -- done.\n");
}

//                PI_DEBUG (DBG_L1, "key '" << tab_key << "' not found in probe-tab-options.");


void RPSPMC_Control::set_tab_settings (const gchar *tab_key, guint64 option_flags, guint64 auto_flags, guint64 glock_data[6]) {
        GVariant *v = g_settings_get_value (hwi_settings, "probe-tab-options");

        gint dbg_level = 0;

#define PI_DEBUG_GPX(X, ARGS...) g_print (ARGS);

        // PI_DEBUG_GPX (dbg_level, "probe-tab-options type is '%s'\n", g_variant_get_type (v));
        //        PI_DEBUG_GPX (dbg_level, "probe-tab-options old=%s\n", g_variant_print (v,true));

        GVariantDict *dict = g_variant_dict_new (v);

        if (!dict){
                PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::set_tab_settings -- can't read dictionary 'probe-tab-options' a{sai}.\n");
                return;
        }
#if 0
        if (!g_variant_dict_contains (dict, tab_key)){
                PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::set_tab_settings -- can't find key '%s' in dict 'probe-tab-options'.\n", tab_key);
                return;
        }
        PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings -- key '%s' found in dict 'probe-tab-options' :)\n", tab_key);
#endif

        GVariant *value = g_variant_dict_lookup_value (dict, tab_key, ((const GVariantType *) "at"));
        if (!value){
                PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::set_tab_settings -- can't find array 'at' data from key '%s' in 'probe-tab-options' dictionary for update.\n", tab_key);
                g_variant_dict_unref (dict);
                return;
        }
        if (value){
                gsize    ni = 9;
                guint64 *ai = g_new (guint64, ni);

                //                PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings ** key '%s' old v=%s\n", tab_key, g_variant_print (value, true));
#if 0
                // ---- debug read
                guint64  *aix = NULL;
                gsize     nix;
                
                aix = (guint64*) g_variant_get_fixed_array (value, &nix, sizeof (guint64));
                g_assert_cmpint (nix, ==, 9);

                PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings ** key '%s' old x=[", tab_key);
                for (int i=0; i<nix; ++i)
                        PI_DEBUG_GPX (DBG_L4, "0x%08llx, ", aix[i]);
                PI_DEBUG_GPX (dbg_level, "] hex\n");
                PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings ** key '%s' old x=[", tab_key);
                for (int i=0; i<nix; ++i)
                        PI_DEBUG_GPX (DBG_L4, "%lld, ", aix[i]);
                PI_DEBUG_GPX (dbg_level, "] decimal\n");
                // -----end debug read back test
#endif
                // option_flags = option_flags & 0xfff; // mask
                ai[0] = 0;
                ai[1] = option_flags;
                ai[2] = auto_flags;

                // test -- set all the same
                //for (int i=0; i<9; ++i)
                //        ai[i] = auto_flags;

                for (int i=0; i<6; ++i)
                        ai[1+2+i] = glock_data [i];

#if 0
                PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings ** key '%s' new x=[", tab_key);
                for (int i=0; i<ni; ++i)
                        PI_DEBUG_GPX (DBG_L4, "0x%08llx, ", ai[i]);
                PI_DEBUG_GPX (dbg_level, "] hex\n");
#endif
                
                g_variant_unref (value); // free old variant
                value = g_variant_new_fixed_array (g_variant_type_new ("t"), ai, ni, sizeof (guint64));

                // PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings ** key '%s' new v=%s\n", tab_key, g_variant_print (value, true));

                // remove old
                if (!g_variant_dict_remove (dict, tab_key)){
                        PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::set_tab_settings -- cant find and remove key '%s' from 'probe-tab-options' dictionary.\n", tab_key);
                }
                // insert new
                // PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings -- inserting new updated key '%s' into 'probe-tab-options' dictionary.\n", tab_key);
                g_variant_dict_insert_value (dict, tab_key, value);

                g_free (ai); // free array
                // must free data for ourselves
        } else {
                PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::set_tab_settings -- cant get array 'au' from key '%s' in 'probe-tab-options' dictionary.\n", tab_key);
        }

        GVariant *vto = g_variant_dict_end (dict);

        // PI_DEBUG_GPX (dbg_level, "probe-tab-options new=%s\n", g_variant_print (vto, true));

        // testing storing
        if (!vto){
                PI_DEBUG_GPX (dbg_level, "ERROR RPSPMC_Control::set_tab_settings -- store tab-setings dict to variant failed.\n");
        }
        if (!g_settings_set_value (hwi_settings, "probe-tab-options", vto)){
                PI_DEBUG_GPX (dbg_level, "ERROR RPSPMC_Control::set_tab_settings -- update probe-tab-options dict failed.\n");
        }
        g_variant_dict_unref (dict);
        //        g_variant_unref (vto);

        //        PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings -- done.\n");
        return;
}


// MAIN HwI GUI window construction


void RPSPMC_Control::AppWindowInit(const gchar *title){
        if (title) { // stage 1
                PI_DEBUG (DBG_L2, "RPSPMC_Control::AppWindowInit -- header bar");

                app_window = gxsm4_app_window_new (GXSM4_APP (main_get_gapp()->get_application ()));
                window = GTK_WINDOW (app_window);

                header_bar = gtk_header_bar_new ();
                gtk_widget_show (header_bar);
                // hide close, min, max window decorations
                //gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header_bar), false);

                gtk_window_set_title (GTK_WINDOW (window), title);
                // gtk_header_bar_set_title ( GTK_HEADER_BAR (header_bar), title);
                // gtk_header_bar_set_subtitle (GTK_HEADER_BAR  (header_bar), title);
                gtk_window_set_titlebar (GTK_WINDOW (window), header_bar);

                v_grid = gtk_grid_new ();

                gtk_window_set_child (GTK_WINDOW (window), v_grid);
                g_object_set_data (G_OBJECT (window), "v_grid", v_grid); // was "vbox"

                gtk_widget_show (GTK_WIDGET (window));               
        } else {
                PI_DEBUG (DBG_L2, "RPSPMC_Control::AppWindowInit -- header bar -- stage two, hook configure menu");

                win_RPSPMC_popup_entries[0].state = g_settings_get_boolean (hwi_settings, "configure-mode") ? "true":"false"; // get last state
                g_action_map_add_action_entries (G_ACTION_MAP (app_window),
                                                 win_RPSPMC_popup_entries, G_N_ELEMENTS (win_RPSPMC_popup_entries),
                                                 this);

                // create window PopUp menu  ---------------------------------------------------------------------
                GtkWidget *mc_popup_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (main_get_gapp()->get_hwi_mover_popup_menu ())); // fix me -- reusuing same menu def (from DSP MOVER)

                // attach popup menu configuration to tool button --------------------------------
                GtkWidget *header_menu_button = gtk_menu_button_new ();
                gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (header_menu_button), "applications-utilities-symbolic");
                gtk_menu_button_set_popover (GTK_MENU_BUTTON (header_menu_button), mc_popup_menu);
                gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), header_menu_button);
                gtk_widget_show (header_menu_button);
        }
}

typedef union {
        struct { unsigned char ch, x, y, z; } s;
        unsigned long   l;
} AmpIndex;

int RPSPMC_Control::choice_Ampl_callback (GtkWidget *widget, RPSPMC_Control *spmsc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	gint i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	gint j = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "chindex"));
	switch(j){
	case 0: rpspmc_pacpll_hwi_pi.app->xsm->Inst->VX(i);
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_DSP_OFFSET_ADDING)
			rpspmc_pacpll_hwi_pi.app->xsm->Inst->VX0(i);
		break;
	case 1: rpspmc_pacpll_hwi_pi.app->xsm->Inst->VY(i);
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_DSP_OFFSET_ADDING)
			rpspmc_pacpll_hwi_pi.app->xsm->Inst->VY0(i);
		break;
	case 2: rpspmc_pacpll_hwi_pi.app->xsm->Inst->VZ(i); 
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_DSP_OFFSET_ADDING)
			rpspmc_pacpll_hwi_pi.app->xsm->Inst->VZ0(i); 
		break;
	case 3:
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING)
			rpspmc_pacpll_hwi_pi.app->xsm->Inst->VX0(i);
		break;
	case 4:
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING)
			rpspmc_pacpll_hwi_pi.app->xsm->Inst->VY0(i);
		break;
	case 5:
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING)
			rpspmc_pacpll_hwi_pi.app->xsm->Inst->VZ0(i);
		break;
	}

	PI_DEBUG (DBG_L2, "Ampl: Ch=" << j << " i=" << i );
	rpspmc_pacpll_hwi_pi.app->spm_range_check(NULL, rpspmc_pacpll_hwi_pi.app);
        
	return 0;
}

int RPSPMC_Control::config_options_callback (GtkWidget *widget, RPSPMC_Control *ssc){
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                ssc->options |= GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget),"Bit_Mask"));
        else
                ssc->options &= ~GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget),"Bit_Mask"));
        
        g_message ("RPSPMC_Control::config_options_callback -> %04x", ssc->options);
        return 0;
}


void RPSPMC_Control::create_folder (){
        GtkWidget *notebook;
 	GSList *zpos_control_list=NULL;


        AppWindowInit ("RP-SPM Control Window");
        
        // ========================================
        notebook = gtk_notebook_new ();
        gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
        gtk_notebook_popup_enable (GTK_NOTEBOOK (notebook));

        gtk_widget_show (notebook);
        gtk_grid_attach (GTK_GRID (v_grid), notebook, 1,1, 1,1);


        bp = new GUI_Builder (v_grid);
        bp->set_error_text ("Invalid Value.");
        bp->set_input_width_chars (10);
        bp->set_no_spin ();

        bp->set_pcs_remote_prefix ("dsp-"); // for script compatibility reason keeping this the same for all HwI.

// ==== Folder: Feedback & Scan ========================================
        bp->start_notebook_tab (notebook, "Feedback & Scan", "rpspmc-tab-feedback-scan", hwi_settings);

	bp->new_grid_with_frame ("SPM Controls");
        
        // ------- FB Characteristics
        bp->start (); // start on grid top and use widget grid attach nx=4
        bp->set_scale_nx (4); // set scale width to 4
        bp->set_input_width_chars (10);

        //bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ChangedNotify, this);
        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::BiasChanged, this);
        bp->grid_add_ec_with_scale ("Bias", Volt, &bias, -10., 10., "4g", 0.001, 0.01, "fbs-bias");
        //        bp->ec->set_adjustment_mode (PARAM_CONTROL_ADJUSTMENT_LOG | PARAM_CONTROL_ADJUSTMENT_LOG_SYM | PARAM_CONTROL_ADJUSTMENT_DUAL_RANGE | PARAM_CONTROL_ADJUSTMENT_ADD_MARKS );
        bp->ec->SetScaleWidget (bp->scale, 0);
        bp->ec->set_logscale_min (1e-3);
        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);
        bp->new_line ();

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ZPosSetChanged, this);
        bp->set_input_width_chars (30);
        bp->set_input_nx (2);
        bp->grid_add_ec ("Z-Pos/Setpoint:", Angstroem, &zpos_ref, -100., 100., "6g", 0.01, 0.1, "adv-dsp-zpos-ref");
        ZPos_ec = bp->ec;
        zpos_control_list = g_slist_prepend (zpos_control_list, bp->ec);
                 
        bp->set_input_width_chars (12);
        bp->set_input_nx ();
        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ChangedNotify, this);

	bp->grid_add_check_button ("Z-Pos Monitor",
                                   "Z Position Monitor. Disable to set Z-Position Setpoint for const height mode. "
                                   "Switch Transfer to CZ-FUZZY-LOG for Z-CONTROL Constant Heigth Mode Operation with current compliance given by Fuzzy-Level. "
                                   "Slow down feedback to minimize instabilities. "
                                   "Set Current setpoint a notch below compliance level.",
                                   1,
                                   G_CALLBACK (RPSPMC_Control::zpos_monitor_callback), this,
                                   0, 0);
        GtkWidget *zposmon_checkbutton = bp->button;
        bp->new_line ();

        bp->set_configure_list_mode_off ();

        // MIXER headers

	bp->grid_add_label ("Source");
        bp->grid_add_label ("Setpoint", NULL, 2);

        bp->set_configure_list_mode_on ();
        bp->grid_add_label ("Gain");
        bp->grid_add_label ("Fuzzy-Level");
        bp->grid_add_label ("Transfer");
        bp->set_configure_list_mode_off ();

        bp->new_line ();

        bp->set_scale_nx (1);

        // Build MIXER CHANNELs
        UnitObj *mixer_unit[4] = { Current, Frq, Volt, Volt };
        const gchar *mixer_channel_label[4] = { "STM Current", "AFM/Force", "Mix2", "Mix3" };
        const gchar *mixer_remote_id_set[4] = { "fbs-mx0-current-set",  "fbs-mx1-freq-set",   "fbs-mx2-set",   "fbs-mx3-set" };
        const gchar *mixer_remote_id_gn[4]  = { "fbs-mx0-current-gain" ,"fbs-mx1-freq-gain",  "fbs-mx2-gain",  "fbs-mx3-gain" };
        const gchar *mixer_remote_id_fl[4]  = { "fbs-mx0-current-level","fbs-mx1-freq-level", "fbs-mx2-level", "fbs-mx3-level" };

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ZServoParamChanged, this);
        // Note: transform mode is always default [LOG,OFF,OFF,OFF] -- NOT READ BACK FROM DSP -- !!!
        for (gint ch=0; ch<2; ++ch){

                GtkWidget *signal_select_widget = NULL;
                UnitObj *tmp = NULL;
#if 0
                // add feedback mixer source signal selection
                signal_select_widget = bp->grid_add_mixer_input_options (ch, mix_fbsource[ch], this);
                if (ch > 1){
                        const gchar *u =  sranger_common_hwi->lookup_dsp_signal_managed (mix_fbsource[ch])->unit;

                        switch (u[0]){
                        case 'V': tmp = new UnitObj("V","V"); break;
                        case 'A' : tmp = new UnitObj(UTF8_ANGSTROEM,"A"); break;
                        case 'H': tmp = new UnitObj("Hz","Hz"); break;
                        case 'd': tmp = new UnitObj(UTF8_DEGREE,"Deg"); break;
                        case 's': tmp = new UnitObj("s","sec"); break;
                        case 'C': tmp = new UnitObj("#","CNT"); break;
                        case 'm': tmp = new UnitObj("V","V"); break;
                        case '1': tmp = new UnitObj("x1","x1"); break;
                        case 'X': tmp = new UnitObj("X","Flag"); break;
                        case 'x': tmp = new UnitObj("xV","xV"); break;
                        case '*': tmp = new UnitObj("*V","*V"); break;
                        default: tmp = new UnitObj("V","V"); break;
                        }
                        mixer_unit[ch] = tmp;
                }                
#else
                // predef label
                bp->grid_add_label (mixer_channel_label[ch]);
#endif

                bp->grid_add_ec_with_scale (NULL, mixer_unit[ch], &mix_set_point[ch], ch==0? 0.0:-100.0, 100., "4g", 0.001, 0.01, mixer_remote_id_set[ch]);
                // bp->ec->set_adjustment_mode (PARAM_CONTROL_ADJUSTMENT_LOG | PARAM_CONTROL_ADJUSTMENT_ADD_MARKS );
                bp->ec->SetScaleWidget (bp->scale, 0);
                bp->ec->set_logscale_min (1e-4);
                bp->ec->set_logscale_magshift (-3);
                gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);

                if (signal_select_widget)
                        g_object_set_data (G_OBJECT (signal_select_widget), "related_ec_setpoint", bp->ec);

                
                // bp->add_to_configure_hide_list (bp->scale);
                bp->set_input_width_chars (10);
                bp->set_configure_list_mode_on ();
                bp->grid_add_ec (NULL, Unity, &mix_gain[ch], -1.0, 1.0, "5g", 0.001, 0.01, mixer_remote_id_gn[ch]);
                bp->grid_add_ec (NULL, mixer_unit[ch], &mix_level[ch], -100.0, 100.0, "5g", 0.001, 0.01, mixer_remote_id_fl[ch]);

                if (tmp) delete (tmp); // done setting unit -- if custom
                
                if (signal_select_widget)
                        g_object_set_data (G_OBJECT (signal_select_widget), "related_ec_level", bp->ec);
                
                bp->grid_add_mixer_options (ch, mix_transform_mode[ch], this);
                bp->set_configure_list_mode_off ();
                bp->new_line ();
        }
        bp->set_configure_list_mode_off ();

        bp->set_input_width_chars ();
        
        // Z-Servo
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Z-Servo");

        bp->set_label_width_chars (7);
        bp->set_input_width_chars (12);

        bp->set_configure_list_mode_on ();
	bp->grid_add_ec_with_scale ("CP", dB, &spmc_parameters.z_servo_cp_db, -100., 20., "5g", 1.0, 0.1, "fbs-cp"); // z_servo[SERVO_CP]
        GtkWidget *ZServoCP = bp->input;
        bp->new_line ();
        bp->set_configure_list_mode_off ();
        bp->grid_add_ec_with_scale ("CI", dB, &spmc_parameters.z_servo_ci_db, -100., 20., "5g", 1.0, 0.1, "fbs-ci"); // z_servo[SERVO_CI
        GtkWidget *ZServoCI = bp->input;

        //g_object_set_data( G_OBJECT (ZServoCI), "HasClient", ZServoCP);
        //g_object_set_data( G_OBJECT (ZServoCP), "HasMaster", ZServoCI);
        //g_object_set_data( G_OBJECT (ZServoCI), "HasRatio", GUINT_TO_POINTER((guint)round(1000.*z_servo[SERVO_CP]/z_servo[SERVO_CI])));
        
        bp->new_line ();
        bp->set_label_width_chars ();
        bp->grid_add_check_button ("Enable", "enable Z servo feedback controller.", 1,
                                   G_CALLBACK(RPSPMC_Control::ZServoControl), this, ((int)spmc_parameters.gvp_status)&1, 0);

        spmc_parameters.z_servo_invert = 1.;
        bp->grid_add_check_button ("Invert", "invert Z servo control.", 0,
                                   G_CALLBACK(RPSPMC_Control::ZServoControlInv), this, spmc_parameters.z_servo_invert < 0.0, 0);


	// ========================================
        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ChangedNotify, this);

        PI_DEBUG (DBG_L4, "DSPC----SCAN ------------------------------- ");
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Scan Characteristics");

        bp->start (4); // wx=4
        bp->set_label_width_chars (7);

        bp->set_configure_list_mode_on ();

	bp->grid_add_ec_with_scale ("MoveSpd", Speed, &move_speed_x, 0.1, 10000., "5g", 1., 10., "fbs-scan-speed-move");
        bp->new_line ();
        bp->set_configure_list_mode_off ();

	bp->grid_add_ec_with_scale ("ScanSpd", Speed, &scan_speed_x_requested, 0.1, 10000., "5g", 1., 10., "fbs-scan-speed-scan");
        scan_speed_ec = bp->ec;
        bp->new_line ();

        bp->new_line ();
	bp->grid_add_ec ("Fast Return", Unity, &fast_return, 1., 1000., "5g", 1., 10.,  "adv-scan-fast-return");

        bp->set_configure_list_mode_on ();
	bp->grid_add_ec ("XS 2nd ZOff", Angstroem, &x2nd_Zoff, -10000., 10000., ".2f", 1., 1., "adv-scan-xs2nd-z-offset");
        bp->new_line ();
        bp->set_configure_list_mode_off ();

        bp->set_scale_nx (2);
        bp->grid_add_ec_with_scale ("Slope X", Unity, &area_slope_x, -0.2, 0.2, ".5f", 0.0001, 0.0001,  "adv-scan-slope-x"); slope_x_ec = bp->ec;
        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);
        bp->new_line ();

        bp->grid_add_ec_with_scale ("Slope Y", Unity, &area_slope_y, -0.2, 0.2, ".5f", 0.0001, 0.0001,  "adv-scan-slope-y"); slope_y_ec = bp->ec;
        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);
        bp->set_scale_nx ();
        bp->new_line (0,2);

        bp->grid_add_check_button ("Enable Slope Compensation", "enable analog slope compensation...", 1,
                                       G_CALLBACK(RPSPMC_Control::DSP_slope_callback), this, area_slope_compensation_flag, 0);
        g_settings_bind (hwi_settings, "adv-enable-slope-compensation",
                         G_OBJECT (GTK_CHECK_BUTTON (bp->button)), "active",
                         G_SETTINGS_BIND_DEFAULT);

        bp->grid_add_check_button ("Enable automatic Tip return to center", "enable auto tip return to center after scan", 1,
                                       G_CALLBACK(RPSPMC_Control::DSP_cret_callback), this, center_return_flag, 0);
        g_settings_bind (hwi_settings, "adv-enable-tip-return-to-center",
                         G_OBJECT (GTK_CHECK_BUTTON (bp->button)), "active",
                         G_SETTINGS_BIND_DEFAULT);

        PI_DEBUG (DBG_L4, "DSPC----FB-CONTROL -- INPUT-SRCS ----------------------------- ");

	// SCAN CHANNEL INPUT SOURCE CONFIGURATION MENUS
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Scan Input Source Selection");

        bp->set_configure_list_mode_on ();
        bp->add_to_configure_list (bp->frame); // manage en block
        bp->set_configure_list_mode_off ();
        
        bp->grid_add_label ("... Signal Scan Sources Selections ...", NULL, 4);

        bp->new_line ();

        for (int i=0; i<4; ++i){
                bp->grid_add_scan_input_signal_options (i, scan_source[i], this);
                VPScanSrcVPitem[i] =  bp->wid;
        }

        // LDC -- Enable Linear Drift Correction -- Controls
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Enable Linear Drift Correction (LDC)");
        bp->set_configure_list_mode_on ();

	LDC_status = bp->grid_add_check_button ("Enable Linear Drift Correction", NULL, 3);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (LDC_status), 0);
	ldc_flag = 0;
	g_signal_connect (G_OBJECT (LDC_status), "toggled",
			    G_CALLBACK (RPSPMC_Control::ldc_callback), this);


        // LDC settings
	bp->grid_add_ec ("LDCdX", Speed, &dxdt, -100., 100., "5g", 0.001, 0.01, "fbs-scan-ldc-dx");
        bp->grid_add_ec ("dY", Speed, &dydt, -100., 100., "5g", 0.001, 0.01, "fbs-scan-ldc-dy");
        bp->grid_add_ec ("dZ", Speed, &dzdt, -100., 100., "5g", 0.001, 0.01, "fbs-scan-ldc-dz");

        bp->set_configure_list_mode_off ();

        bp->set_input_width_chars (8);
        
        bp->pop_grid ();
        bp->new_line ();

	// ======================================== Piezo Drive / Amplifier Settings
        bp->new_grid_with_frame ("Piezo Drive Settings");

        const gchar *PDR_gain_label[6] = { "VX", "VY", "VZ", "VX0", "VY0", "VZ0" };
        const gchar *PDR_gain_key[6] = { "vx", "vy", "vz", "vx0", "vy0", "vz0" };
        for(int j=0; j<6; j++) {
                if (j == 3 && main_get_gapp()->xsm->Inst->OffsetMode () == OFM_DSP_OFFSET_ADDING)
                        break;

                gtk_label_set_width_chars (GTK_LABEL (bp->grid_add_label (PDR_gain_label[j])), 6);
        
                GtkWidget *wid = gtk_combo_box_text_new ();
                g_object_set_data  (G_OBJECT (wid), "chindex", GINT_TO_POINTER (j));

                // Init gain-choicelist
                for(int ig=0; ig<GAIN_POSITIONS; ig++){
                        AmpIndex  AmpI;
                        AmpI.l = 0L;
                        AmpI.s.ch = j;
                        AmpI.s.x  = ig;
                        gchar *Vxyz = g_strdup_printf("%g",xsmres.V[ig]);
                        gchar *id = g_strdup_printf ("%ld",AmpI.l);
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), id, Vxyz);
                        g_free (id);
                        g_free (Vxyz);
                }

                g_signal_connect (G_OBJECT (wid), "changed",
                                  G_CALLBACK (RPSPMC_Control::choice_Ampl_callback),
                                  this);

                gchar *tmpkey = g_strconcat ("spm-rpspmc-h", PDR_gain_key[j], NULL); 
                // get last setting, will call callback connected above to update gains!
                g_settings_bind (hwi_settings, tmpkey,
                                 G_OBJECT (GTK_COMBO_BOX (wid)), "active",
                                 G_SETTINGS_BIND_DEFAULT);
                g_free (tmpkey);

                //VXYZS0Gain[j] = wid; // store for remote access/manipulation
                bp->grid_add_widget (wid);
                bp->add_to_scan_freeze_widget_list (wid);
        }
        bp->pop_grid ();
        bp->new_line ();

        bp->notebook_tab_show_all ();
        bp->pop_grid ();

        // ==== Folder: LockIn  ========================================
        bp->new_grid ();
        bp->start_notebook_tab (notebook, "Lock-In", "rpspmc-tab-lockin", hwi_settings);

 	bp->new_grid_with_frame ("Digital Lock-In settings");
        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::lockin_adjust_callback, this);

        bp->new_line ();

#define GET_CURRENT_LOCKING_MODE 0 // dummy
        
        LockIn_mode = bp->grid_add_check_button ("LockIn run free", "enable contineous modulation and LockIn processing.\n",
                                                     1,
                                                     GCallback (RPSPMC_Control::lockin_runfree_callback), this,
                                                     GET_CURRENT_LOCKING_MODE, 0);

        bp->new_line ();
	bp->grid_add_ec ("Bias Amp", Volt, &AC_amp[0], 0., 1., "5g", 0.001, 0.01, "LCK-AC-Bias-Amp");
        bp->new_line ();
        bp->grid_add_ec ("Z Amp", Angstroem, &AC_amp[1], 0., 100., "5g", 0.01, 0.1, "LCK-AC-Z-Amp");

        bp->notebook_tab_show_all ();
        bp->pop_grid ();

        
// ==== Folder Set for Vector Probe ========================================
// ==== Folder: I-V STS setup ========================================
        PI_DEBUG (DBG_L4, "DSPC----TAB-IV ------------------------------- ");

        bp->new_grid ();
        bp->start_notebook_tab (notebook, "STS", "rpspmc-tab-sts", hwi_settings);
	// ==================================================
        bp->new_grid_with_frame ("I-V Type Spectroscopy");
	// ==================================================

	bp->grid_add_label ("IV Probe"); bp->grid_add_label ("Start"); bp->grid_add_label ("End");  bp->grid_add_label ("Points");
        bp->new_line ();

        bp->grid_add_ec (NULL, Volt, &IV_start, -10.0, 10., "5.3g", 0.1, 0.025, "IV-Start");
        bp->grid_add_ec (NULL, Volt, &IV_end, -10.0, 10.0, "5.3g", 0.1, 0.025, "IV-End");
        bp->grid_add_ec (NULL, Unity, &IV_points, 1, 1000, "5g", "IV-Points");
        
        bp->new_line ();

	bp->grid_add_ec ("Slope", Vslope, &IV_slope,0.1,1000.0, "5.3g", 1., 10., "IV-Slope");
	bp->grid_add_ec ("Slope Ramp", Vslope, &IV_slope_ramp,0.1,1000.0, "5.3g", 1., 10., "IV-Slope-Ramp");
        bp->new_line ();
        bp->set_configure_list_mode_on ();
	bp->grid_add_ec ("#IV sets", Unity, &IV_repetitions, 1, 100, "3g", "IV-rep");
        bp->set_configure_list_mode_off ();

        bp->new_line ();

	IV_status = bp->grid_add_probe_status ("Status");
        bp->new_line ();

        bp->pop_grid ();
        bp->new_line ();
        bp->grid_add_probe_controls (TRUE,
                                         IV_option_flags, GCallback (callback_change_IV_option_flags),
                                         IV_auto_flags,  GCallback (callback_change_IV_auto_flags),
                                         GCallback (RPSPMC_Control::Probing_exec_IV_callback),
                                         GCallback (RPSPMC_Control::Probing_write_IV_callback),
                                         GCallback (RPSPMC_Control::Probing_graph_callback),
                                         GCallback (RPSPMC_Control::Probing_abort_callback),
                                         this,
                                         "IV");
        bp->notebook_tab_show_all ();
        bp->pop_grid ();


        
        bp->new_grid ();
        bp->start_notebook_tab (notebook, "GVP", "rpspmc-tab-gvp", hwi_settings);

	// ========================================
        PI_DEBUG (DBG_L4, "DSPC----TAB-VP ------------------------------- ");

 	bp->new_grid_with_frame ("Generic Vector Program (VP) Probe and Manipulation");
 	// g_print ("================== TAB 'GVP' ============= Generic Vector Program (VP) Probe and Manipulation\n");
        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ChangedNotifyVP, this);

	// ----- VP Program Vectors Headings
	// ------------------------------------- divided view

	// ------------------------------------- VP program table, scrolled
 	GtkWidget *vp = gtk_scrolled_window_new ();
        gtk_widget_set_vexpand (vp, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (vp), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_min_content_height  (GTK_SCROLLED_WINDOW (vp), 100);
        gtk_widget_set_vexpand (bp->grid, TRUE);
        bp->grid_add_widget (vp); // only widget in bp->grid

        bp->new_grid ();
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (vp), bp->grid);
        
        // ----- VP Program Vectors Listing -- headers
	VPprogram[0] = bp->grid;

        bp->set_no_spin ();
        
        bp->grid_add_label ("Vec[PC]", "Vector Program Counter");
        bp->grid_add_label ("VP-dU", "vec-du (Bias, DAC CH4)");
        bp->grid_add_label ("VP-dX", "vec-dx (X-Scan, *Mrot + X0, DAC CH1)");
        bp->grid_add_label ("VP-dY", "vec-dy (Y-Scan, *Mrot + Y0, DAC CH2)");
        bp->grid_add_label ("VP-dZ", "vec-dz (Z-Probe + Z-Servo + Z0, DAC CH3)");
        bp->set_configure_list_mode_on ();
        bp->grid_add_label ("VP-dA", "vec-dA (**DAC CH5)");
        bp->grid_add_label ("VP-dB", "vec-dA (**DAC CH6)");
        bp->set_configure_list_mode_off ();
        bp->grid_add_label ("time", "total time for VP section");
        bp->grid_add_label ("points", "points (# vectors to add)");
        bp->grid_add_label ("FB", "Feedback");
        bp->set_configure_list_mode_on ();
        bp->grid_add_label ("VSET", "Treat this as a initial set position, vector differential from current position are computed!");
        //bp->grid_add_label ("IOR", "** GPIO-Read N/A");
        //bp->grid_add_label ("TP", "** TRIGGER-POS N/A");
        //bp->grid_add_label ("TN", "** TRIGGER-NEG N/A");
        //bp->grid_add_label ("GPIO", "** mask for trigger on GPIO N/A");
        bp->grid_add_label ("Nrep", "VP # repetition");
        bp->grid_add_label ("PCJR", "Vector-PC jump relative\n (example: set to -2 to repeat previous two vectors.)");
        bp->set_configure_list_mode_off ();
        bp->grid_add_label("Shift", "Edit: Shift VP Block up/down");

        bp->new_line ();
        
	GSList *EC_vpc_opt_list=NULL;
        Gtk_EntryControl *ec_iter[12];
	for (int k=0; k < N_GVP_VECTORS; ++k) {
		gchar *tmpl = g_strdup_printf ("vec[%02d]", k); 

                // g_print ("GVP-DEBUG:: %s -[%d]------> GVPdu=%g, ... points=%d,.. opt=%8x\n", tmpl, k, GVP_du[k], GVP_points[k],  GVP_opt[k]);

		bp->grid_add_label (tmpl, "Vector PC");
		bp->grid_add_ec (NULL,      Volt, &GVP_du[k], -10.0,   10.0,   "6.4g", 1., 10., "gvp-du", k); 
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

		bp->grid_add_ec (NULL, Angstroem, &GVP_dx[k], -100000.0, 100000.0, "6.4g", 1., 10., "gvp-dx", k); 
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

		bp->grid_add_ec (NULL, Angstroem, &GVP_dy[k], -100000.0, 100000.0, "6.4g", 1., 10., "gvp-dy", k); 
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

		bp->grid_add_ec (NULL, Angstroem, &GVP_dz[k], -100000.0, 100000.0, "6.4g", 1., 10., "gvp-dz", k); 
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

                bp->set_configure_list_mode_on (); // === advanced ===========================================
		bp->grid_add_ec (NULL,    Volt, &GVP_da[k], -10.0, 10.0, "6.4g",1., 10., "gvp-da", k); 
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
		bp->grid_add_ec (NULL,    Volt, &GVP_db[k], -10.0, 10.0, "6.4g",1., 10., "gvp-db", k); 
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
                bp->set_configure_list_mode_off (); // ========================================================

		bp->grid_add_ec (NULL,      Time, &GVP_ts[k], 0., 10000.0,     "5.4g", 1., 10., "gvp-dt", k); 
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

		bp->grid_add_ec (NULL,      Unity, &GVP_points[k], 0, 4000,  "5g", "gvp-n", k); 
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();


		//note:  VP_FEEDBACK_HOLD is only the mask, this bit in GVP_opt is set to ONE for FEEBACK=ON !! Ot is inveretd while vector generation ONLY.
		bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (callback_change_GVP_vpc_option_flags), this,
                                               GVP_opt[k], VP_FEEDBACK_HOLD);
                EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, bp->button);
                g_object_set_data (G_OBJECT (bp->button), "VPC", GINT_TO_POINTER (k));

                bp->set_configure_list_mode_on (); // ================ advanced section

                bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (callback_change_GVP_vpc_option_flags), this,
                                               GVP_opt[k], VP_INITIAL_SET_VEC);
                EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, bp->button);
                g_object_set_data (G_OBJECT (bp->button), "VPC", GINT_TO_POINTER (k));
#if 0
                bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (callback_change_GVP_vpc_option_flags), this,
                                               GVP_opt[k], VP_GPIO_READ);
                EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, bp->button);
                g_object_set_data (G_OBJECT (bp->button), "VPC", GINT_TO_POINTER (k));

                bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (callback_change_GVP_vpc_option_flags), this,
                                               GVP_opt[k], VP_TRIGGER_P);
                EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, bp->button);
                g_object_set_data (G_OBJECT (bp->button), "VPC", GINT_TO_POINTER (k));

                bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (callback_change_GVP_vpc_option_flags), this,
                                               GVP_opt[k], VP_TRIGGER_N);
                EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, bp->button);
                g_object_set_data (G_OBJECT (bp->button), "VPC", GINT_TO_POINTER (k));

		bp->grid_add_ec (NULL, Hex,   &GVP_data[k],   0, 0xffff,  "04X", "gvp-data", k); 
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
#endif
                
		bp->grid_add_ec (NULL, Unity, &GVP_vnrep[k], 0., 32000.,  ".0f", "gvp-nrep", k); 
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

		bp->grid_add_ec (NULL, Unity, &GVP_vpcjr[k], -50.,   0.,  ".0f", "gvp-pcjr", k); 
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

                bp->set_configure_list_mode_off (); // ==================================

                GtkWidget *button;
		if (k > 0){
                        bp->grid_add_widget (button=gtk_button_new_from_icon_name ("arrow-up-symbolic"));
                        gtk_widget_set_tooltip_text (button, "Shift VP up here");
                        g_object_set_data (G_OBJECT(button), "VPC", GINT_TO_POINTER (k));
                        g_object_set_data (G_OBJECT(button), "ARROW", GINT_TO_POINTER (GVP_SHIFT_UP));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (callback_edit_GVP),
                                           this);
		} else {
                        bp->grid_add_widget (button=gtk_button_new_from_icon_name ("view-more-symbolic"));
                        gtk_widget_set_tooltip_text (button, "Toggle VP Flow Chart");
                        g_object_set_data (G_OBJECT(button), "VPC", GINT_TO_POINTER (k));
                        g_object_set_data (G_OBJECT(button), "ARROW", GINT_TO_POINTER (GVP_SHIFT_UP));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (callback_edit_GVP),
                                           this);
                }
		if (k < N_GVP_VECTORS-1){
                        bp->grid_add_widget (button=gtk_button_new_from_icon_name ("arrow-down-symbolic"));
                        gtk_widget_set_tooltip_text (button, "Shift VP down here");
                        g_object_set_data (G_OBJECT(button), "VPC", GINT_TO_POINTER (k));
                        g_object_set_data (G_OBJECT(button), "ARROW", GINT_TO_POINTER (GVP_SHIFT_DN));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (callback_edit_GVP),
                                           this);
		}
		g_free (tmpl);
		if (k==0)
			g_object_set_data (G_OBJECT (bp->grid), "CF", GINT_TO_POINTER (bp->x));

                bp->new_line ();
                
	}
	g_object_set_data( G_OBJECT (window), "DSP_VPC_OPTIONS_list", EC_vpc_opt_list);

        bp->set_no_spin (false);

        // lower part: controls
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("VP Memos");

	// MEMO BUTTONs
        gtk_widget_set_hexpand (bp->grid, TRUE);
	const gchar *keys[] = { "VPA", "VPB", "VPC", "VPD", "VPE", "VPF", "VPG", "VPH", "VPI", "VPJ", "V0", NULL };

	for (int i=0; keys[i]; ++i) {
                GdkRGBA rgba;
		gchar *gckey  = g_strdup_printf ("GVP_set_%s", keys[i]);
		gchar *stolab = g_strdup_printf ("STO %s", keys[i]);
		gchar *rcllab = g_strdup_printf ("RCL %s", keys[i]);
		gchar *memolab = g_strdup_printf ("M %s", keys[i]);             
		gchar *memoid  = g_strdup_printf ("memo-vp%c", 'a'+i);             
                remote_action_cb *ra = NULL;
                gchar *help = NULL;

                bp->set_xy (i+1, 10);
                // add button with remote support for program recall
                ra = g_new( remote_action_cb, 1);
                ra -> cmd = g_strdup_printf("DSP_VP_STO_%s", keys[i]);
                help = g_strconcat ("Remote example: action (", ra->cmd, ")", NULL); 
                bp->grid_add_button (N_(stolab), help, 1,
                                         G_CALLBACK (callback_GVP_store_vp), this,
                                         "key", gckey);
                g_free (help);
                ra -> RemoteCb = (void (*)(GtkWidget*, void*))callback_GVP_store_vp;
                ra -> widget = bp->button;
                ra -> data = this;
                main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
                PI_DEBUG (DBG_L2, "Adding new Remote Cmd: " << ra->cmd ); 

                // CSS
                //                if (gdk_rgba_parse (&rgba, "tomato"))
                //                        gtk_widget_override_background_color ( GTK_WIDGET (bp->button), GTK_STATE_FLAG_PRELIGHT, &rgba);

                bp->set_xy (i+1, 11);

                // add button with remote support for program recall
                ra = g_new( remote_action_cb, 1);
                ra -> cmd = g_strdup_printf("DSP_VP_RCL_%s", keys[i]);
                help = g_strconcat ("Remote example: action (", ra->cmd, ")", NULL); 
                bp->grid_add_button (N_(rcllab), help, 1,
                                         G_CALLBACK (callback_GVP_restore_vp), this,
                                         "key", gckey);
                g_free (help);
                ra -> RemoteCb = (void (*)(GtkWidget*, void*))callback_GVP_restore_vp;
                ra -> widget = bp->button;
                ra -> data = this;
                main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
                PI_DEBUG (DBG_L2, "Adding new Remote Cmd: " << ra->cmd ); 
                
                // CSS
                //                if (gdk_rgba_parse (&rgba, "SeaGreen3"))
                //                        gtk_widget_override_background_color ( GTK_WIDGET (bp->button), GTK_STATE_FLAG_PRELIGHT, &rgba);
#if 0 // may adda memo/info button
                bp->set_xy (i+1, 12);
                bp->grid_add_button (N_(memolab), memolab, 1,
                                         G_CALLBACK (callback_GVP_memo_vp), this,
                                         "key", gckey);
#endif
                bp->set_xy (i+1, 12);
                bp->grid_add_input (NULL);
                bp->set_input_width_chars (10);
                gtk_widget_set_hexpand (bp->input, TRUE);

                g_settings_bind (hwi_settings, memoid,
                                 G_OBJECT (bp->input), "text",
                                 G_SETTINGS_BIND_DEFAULT);

                //g_free (gckey);
                //g_free (stolab);
                //g_free (rcllab);
                g_free (memolab);
                g_free (memoid);
	}

        // ===================== done with panned
        
        bp->pop_grid ();
        bp->new_line ();

	bp->new_grid_with_frame ("VP Status");

	GVP_status = bp->grid_add_probe_status ("Status");

        bp->pop_grid ();
        bp->new_line ();

        bp->grid_add_probe_controls (FALSE,
                                         GVP_option_flags, GCallback (callback_change_GVP_option_flags),
                                         GVP_auto_flags,  GCallback (callback_change_GVP_auto_flags),
                                         GCallback (RPSPMC_Control::Probing_exec_GVP_callback),
                                         GCallback (RPSPMC_Control::Probing_write_GVP_callback),
                                         GCallback (RPSPMC_Control::Probing_graph_callback),
                                         GCallback (RPSPMC_Control::Probing_abort_callback),
                                         this,
                                         "VP");
        bp->notebook_tab_show_all ();
        bp->pop_grid ();



// ==== Folder: Graphs -- Vector Probe Data Visualisation setup ========================================
        bp->new_grid ();
        bp->start_notebook_tab (notebook, "Graphs", "rpspmc-tab-graphs", hwi_settings);

        PI_DEBUG (DBG_L4, "DSPC----TAB-GRAPHS ------------------------------- ");

 	bp->new_grid_with_frame ("Probe Sources & Graph Setup");

        bp->grid_add_label ("Source", "Check column to activate channel", 2, 0.);
        bp->grid_add_label ("X", "Check column to plot channel on X axis.", 1);
        bp->grid_add_label ("Y", "Check column to plot channel on Y axis.", 1);
        bp->grid_add_label ("Avg", "Check column to plot average of all spectra", 1);
        bp->grid_add_label ("Sec", "Check column to show all sections.", 1);
        GtkWidget *sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
        gtk_widget_set_size_request (sep, 5, -1);
        bp->grid_add_widget (sep, 5);
        //bp->grid_add_label (" --- ", NULL, 5);

        bp->grid_add_label ("Source", "Check column to activate channel", 2, 0.);
        bp->grid_add_label ("X", "Check column to plot channel on X axis.", 1);
        bp->grid_add_label ("Y", "Check column to plot channel on Y axis.", 1);
        bp->grid_add_label ("Avg", "Check column to plot average of all spectra", 1);
        bp->grid_add_label ("Sec", "Check column to show all sections.", 1);
        sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
        gtk_widget_set_size_request (sep, 5, -1);
        bp->grid_add_widget (sep, 5);
        //bp->grid_add_label (" --- ", NULL, 5);

#if 1 // if need more
        bp->grid_add_label ("Source", "Check column to activate channel", 2, 0.);
        bp->grid_add_label ("X", "Check column to plot channel on X axis.", 1);
        bp->grid_add_label ("Y", "Check column to plot channel on Y axis.", 1);
        bp->grid_add_label ("Avg", "Check column to plot average of all spectra", 1);
        bp->grid_add_label ("Sec", "Check column to show all sections.", 1);
#endif
        bp->new_line ();

        PI_DEBUG (DBG_L4, "DSPC----TAB-GRAPHS TOGGELS  ------------------------------- ");

        gint y = bp->y;
        gint mm=0;
	for (int i=0; source_signals[i].mask; ++i) {
                PI_DEBUG (DBG_L4, "GRAPHS*** i=" << i << " " << source_signals[i].label);
		int c=i/8; 
		c*=11;
                c++;
		int m = -1;
                for (int k=0; swappable_signals[k].mask; ++k){
                        if (source_signals[i].mask == swappable_signals[k].mask){
                                source_signals[i].label = swappable_signals[k].label;
                                source_signals[i].unit  = swappable_signals[k].unit;
                                source_signals[i].unit_sym = swappable_signals[k].unit_sym;
                                source_signals[i].scale_factor = swappable_signals[k].scale_factor;
                                m = k;
                                PI_DEBUG (DBG_L4, "GRAPHS*** SWPS init i=" << i << " k=" << k << " " << source_signals[i].label);
                                break;
                        }
                }
                int r = y+i%8+1;

                bp->set_xy (c, r);

                // Source
                bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (change_source_callback), this,
                                               Source, (((int) msklookup[i]) & 0xfffffff)
                                               );
                // source selection for SWPS?:
                if (m >= 0){ // swappable flex source
                        //g_message("bp->grid_add_probe_source_signal_options m=%d  %s => %d %s", m, source_signals[i].label,  probe_source[m], swappable_signals[m].label);
                        bp->grid_add_probe_source_signal_options (m, probe_source[m], this);
                }else { // or fixed assignment
                        bp->grid_add_label (source_signals[i].label, NULL, 1, 0.);
                }
                //fixed assignment:
                // bp->grid_add_label (lablookup[i], NULL, 1, 0.);
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER ((int) source_signals[i].mask)); 
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // use as X-Source
                bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (change_source_callback), this,
                                               XSource, (((int) (X_SOURCE_MSK | source_signals[i].mask)) & 0xfffffff)
                                               );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER ((int) (X_SOURCE_MSK | source_signals[i].mask))); 
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 

                // use as Plot (Y)-Source
                bp->grid_add_check_button ("", NULL, 1,
                                               G_CALLBACK (change_source_callback), this,
                                               PSource, (((int) (P_SOURCE_MSK | source_signals[i].mask)) & 0xfffffff)
                                               );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER ((int) (P_SOURCE_MSK | source_signals[i].mask))); 
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // use as A-Source (Average)
                bp->grid_add_check_button ("", NULL, 1,
                                               G_CALLBACK (change_source_callback), this,
                                               PlotAvg, (((int) (A_SOURCE_MSK | source_signals[i].mask)) & 0xfffffff)
                                               );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER ((int) (A_SOURCE_MSK | source_signals[i].mask))); 
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // use as S-Source (Section)
                bp->grid_add_check_button ("", NULL, 1,
                                               G_CALLBACK (change_source_callback), this,
                                               PlotSec, (((int) (S_SOURCE_MSK | source_signals[i].mask)) & 0xfffffff)
                                               );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER ((int) (S_SOURCE_MSK | source_signals[i].mask))); 
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // bp->grid_add_check_button_graph_matrix(lablookup[i], (int) msklookup[i], m, probe_source[m], i, this);
                // bp->grid_add_check_button_graph_matrix(" ", (int) (X_SOURCE_MSK | msklookup[i]), -1, i, this);
                // bp->grid_add_check_button_graph_matrix(" ", (int) (P_SOURCE_MSK | msklookup[i]), -1, i, this);
                // bp->grid_add_check_button_graph_matrix(" ", (int) (A_SOURCE_MSK | msklookup[i]), -1, i, this);
                // bp->grid_add_check_button_graph_matrix(" ", (int) (S_SOURCE_MSK | msklookup[i]), -1, i, this);
                if (c < 23){
                        sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
                        gtk_widget_set_size_request (sep, 5, -1);
                        bp->grid_add_widget (sep);
                }
	}

        
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Plot Mode Configuration");

	bp->grid_add_check_button ("Join all graphs for same X", "Join all plots with same X.\n"
                                       "Note: if auto scale (default) Y-scale\n"
                                       "will only apply to 1st graph - use hold/exp. only for asolute scale.)", 1,
                                       GCallback (callback_XJoin), this,
                                       XJoin, 1
                                       );
	bp->grid_add_check_button ("Use single window", "Place all probe graphs in single window.",
                                       1,
                                       GCallback (callback_GrMatWindow), this,
                                       GrMatWin, 1
                                       );
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Plot / Save current data in buffer");

	bp->grid_add_button ("Plot");
	g_signal_connect (G_OBJECT (bp->button), "clicked",
                          G_CALLBACK (RPSPMC_Control::Probing_graph_callback), this);
	
        save_button = bp->grid_add_button ("Save");
	g_signal_connect (G_OBJECT (bp->button), "clicked",
                          G_CALLBACK (RPSPMC_Control::Probing_save_callback), this);

        bp->notebook_tab_show_all ();
        bp->pop_grid ();



// ==== Folder: RT System Connection ========================================
        bp->new_grid ();
        bp->start_notebook_tab (notebook, "RedPitaya Web Socket", "rpspmc-tab-system", hwi_settings);

        PI_DEBUG (DBG_L4, "DSPC----TAB-SYSTEM ------------------------------- ");

        bp->new_grid_with_frame ("RedPitaya Web Socket Address for JSON talk", 10);

        bp->set_input_width_chars (25);
        rpspmc_pacpll->input_rpaddress = bp->grid_add_input ("RedPitaya Address");

        g_settings_bind (rpspmc_pacpll->inet_json_settings, "redpitay-address",
                         G_OBJECT (bp->input), "text",
                         G_SETTINGS_BIND_DEFAULT);
        gtk_widget_set_tooltip_text (rpspmc_pacpll->input_rpaddress, "RedPitaya IP Address like rp-f05603.local or 130.199.123.123");
        //  "ws://rp-f05603.local:9002/"
        //gtk_entry_set_text (GTK_ENTRY (input_rpaddress), "http://rp-f05603.local/pacpll/?type=run");
        //gtk_entry_set_text (GTK_ENTRY (input_rpaddress), "130.199.243.200");
        //gtk_entry_set_text (GTK_ENTRY (input_rpaddress), "192.168.1.10");
        
        bp->grid_add_check_button ( N_("Connect"), "Check to initiate connection, uncheck to close connection.", 1,
                                    G_CALLBACK (RPspmc_pacpll::connect_cb), rpspmc_pacpll);
        bp->grid_add_check_button ( N_("Debug"), "Enable debugging LV1.", 1,
                                    G_CALLBACK (RPspmc_pacpll::dbg_l1), rpspmc_pacpll);
        bp->grid_add_check_button ( N_("+"), "Debug LV2", 1,
                                    G_CALLBACK (RPspmc_pacpll::dbg_l2), rpspmc_pacpll);
        bp->grid_add_check_button ( N_("++"), "Debug LV4", 1,
                                    G_CALLBACK (RPspmc_pacpll::dbg_l4), rpspmc_pacpll);
        rpspmc_pacpll->rp_verbose_level = 0.0;
        bp->set_input_width_chars (2);
  	bp->grid_add_ec ("V", Unity, &rpspmc_pacpll->rp_verbose_level, 0., 10., ".0f", 1., 1., "RP-VERBOSE-LEVEL");        
        //bp->new_line ();
        //tmp=bp->grid_add_button ( N_("Read"), "TEST READ", 1,
        //                          G_CALLBACK (RPspmc_pacpll::read_cb), this);
        //tmp=bp->grid_add_button ( N_("Write"), "TEST WRITE", 1,
        //                          G_CALLBACK (RPspmc_pacpll::write_cb), this);

        bp->new_line ();
        bp->set_input_width_chars (80);
        rpspmc_pacpll->red_pitaya_health = bp->grid_add_input ("RedPitaya Health",10);

        PangoFontDescription *fontDesc = pango_font_description_from_string ("monospace 10");
        //gtk_widget_modify_font (red_pitaya_health, fontDesc);
        // ### GTK4 ??? CSS ??? ###  gtk_widget_override_font (red_pitaya_health, fontDesc); // use CSS, thx, annyoing garbage... ??!?!?!?
        pango_font_description_free (fontDesc);

        gtk_widget_set_sensitive (bp->input, FALSE);
        gtk_editable_set_editable (GTK_EDITABLE (bp->input), FALSE); 
        rpspmc_pacpll->update_health ("Not connected.");
        bp->new_line ();

        rpspmc_pacpll->text_status = gtk_text_view_new ();
 	gtk_text_view_set_editable (GTK_TEXT_VIEW (rpspmc_pacpll->text_status), FALSE);
        //gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_status), GTK_WRAP_WORD_CHAR);
        GtkWidget *scrolled_window = gtk_scrolled_window_new ();
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        //gtk_widget_set_size_request (scrolled_window, 200, 60);
        gtk_widget_set_hexpand (scrolled_window, TRUE);
        gtk_widget_set_vexpand (scrolled_window, TRUE);
        gtk_scrolled_window_set_child ( GTK_SCROLLED_WINDOW (scrolled_window), rpspmc_pacpll->text_status);
        bp->grid_add_widget (scrolled_window, 10);
        gtk_widget_show (scrolled_window);
        
        // ========================================
        bp->pop_grid ();
        bp->show_all ();


        
	// ============================================================
	// save List away...
        rpspmc_pacpll_hwi_pi.app->RemoteEntryList = g_slist_concat (rpspmc_pacpll_hwi_pi.app->RemoteEntryList, bp->get_remote_list_head ());
        configure_callback (NULL, NULL, this); // configure "false"

	g_object_set_data( G_OBJECT (window), "DSP_EC_list", bp->get_ec_list_head ());
	g_object_set_data( G_OBJECT (zposmon_checkbutton), "DSP_zpos_control_list", zpos_control_list);
        
	GUI_ready = TRUE;
        
        AppWindowInit (NULL); // stage two
        set_window_geometry ("rpspmc-main-control"); // must add key to xml file: core-sources/org.gnome.gxsm4.window-geometry.gschema.xml
}

int RPSPMC_Control::DSP_slope_callback (GtkWidget *widget, RPSPMC_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->area_slope_compensation_flag = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));	
	dspc->update_controller();
        return 0;
}

int RPSPMC_Control::DSP_cret_callback (GtkWidget *widget, RPSPMC_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->center_return_flag = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));	
        return 0;
}


int RPSPMC_Control::ldc_callback(GtkWidget *widget, RPSPMC_Control *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	dspc->update_controller();
	return 0;
}


int RPSPMC_Control::ChangedAction(GtkWidget *widget, RPSPMC_Control *dspc){
	dspc->update_controller ();
	return 0;
}

void RPSPMC_Control::ChangedNotify(Param_Control* pcs, gpointer dspc){
	((RPSPMC_Control*)dspc)->update_controller (); // update basic SPM Control Parameters
}

void RPSPMC_Control::BiasChanged(Param_Control* pcs, RPSPMC_Control* self){
        int j=0;
        if (rpspmc_pacpll)
                rpspmc_pacpll->write_parameter ("SPMC_BIAS", self->bias);

#if 0
        // TESTING ONLY
        if (rpspmc_pacpll){
                double vec[16] = { 1.0, 2., 3., 4., 5.5,6.6,7.7,8.8,9.9,10.,11.,12.,13.,14.,15.,16. };
                //rpspmc_pacpll->write_signal ("SPMC_GVP_VECTOR", 16, vec);
                vec[0]=j++;
                vec[1]=100;
                vec[2]=1000;
                for (int i=5; i<16; ++i)
                        vec[i] += self->bias;
                rpspmc_pacpll->write_array ("SPMC_GVP_VECTOR", 16, vec);
                vec[0]=j++;
                for (int i=5; i<16; ++i)
                        vec[i] += self->bias;
                rpspmc_pacpll->write_array ("SPMC_GVP_VECTOR", 16, vec);
                vec[0]=j++;
                for (int i=5; i<16; ++i)
                        vec[i] += self->bias;
                rpspmc_pacpll->write_array ("SPMC_GVP_VECTOR", 16, vec);

        }
#endif
}

void RPSPMC_Control::ZPosSetChanged(Param_Control* pcs, RPSPMC_Control *self){
        if (rpspmc_pacpll)
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_SETPOINT_CZ", main_get_gapp()->xsm->Inst->ZA2Volt(self->zpos_ref));
}

void RPSPMC_Control::ZServoParamChanged(Param_Control* pcs, RPSPMC_Control *self){
        if (rpspmc_pacpll){
                // (((RPSPMC_Control*)dspc)->mix_gain[ch]) // N/A
                self->z_servo[SERVO_CP] = spmc_parameters.z_servo_invert * pow (10., spmc_parameters.z_servo_cp_db/20.);
                self->z_servo[SERVO_CI] = spmc_parameters.z_servo_invert * pow (10., spmc_parameters.z_servo_ci_db/20.);
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_SETPOINT", self->mix_set_point[0]);
                //rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_MODE",     self->mix_transform_mode[0]);
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_LEVEL",    self->mix_level[0]);
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_CP",       self->z_servo[SERVO_CP]);
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_CI",       self->z_servo[SERVO_CI]);
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_UPPER",  5.0);
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_LOWER", -5.0);
        }
}
void RPSPMC_Control::ZServoControl (GtkWidget *widget, RPSPMC_Control *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                spmc_parameters.z_servo_mode = 1;
        else
                spmc_parameters.z_servo_mode = 0;

        self->ZServoParamChanged (NULL, self);
}

void RPSPMC_Control::ZServoControlInv (GtkWidget *widget, RPSPMC_Control *self){
        spmc_parameters.z_servo_invert = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)) ? -1.:1.;
        self->ZServoParamChanged (NULL, self);
}


void RPSPMC_Control::ChangedNotifyVP(Param_Control* pcs, gpointer dspc){
        g_message ("**ChangedNotifyVP**");
#if 0
        for (int k=0; k<8; k++)
                g_message ("**VP[%02d] %8gV %8gA %8gs np%4d nr%4d",
                           k,
                           ((RPSPMC_Control*)dspc)->GVP_du[k],
                           ((RPSPMC_Control*)dspc)->GVP_dz[k],
                           ((RPSPMC_Control*)dspc)->GVP_ts[k],
                           ((RPSPMC_Control*)dspc)->GVP_points[k],
                           ((RPSPMC_Control*)dspc)->GVP_vnrep[k]);
#endif
}

int RPSPMC_Control::choice_mixmode_callback (GtkWidget *widget, RPSPMC_Control *dspc){
	gint channel=0;
        gint selection=0;

        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

        channel = GPOINTER_TO_INT (g_object_get_data(G_OBJECT (widget), "mix_channel"));
        selection = atoi (gtk_combo_box_get_active_id (GTK_COMBO_BOX (widget)));


	PI_DEBUG_GP (DBG_L3, "MixMode[%d]=0x%x\n",channel,selection);

	dspc->mix_transform_mode[channel] = selection;
        if (channel == 0){
                g_print ("Choice MIX%d MT=%d\n", channel, selection);
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_MODE", selection0.1);
        }
        PI_DEBUG_GP (DBG_L4, "%s ** 2\n",__FUNCTION__);

	dspc->update_controller ();
        //g_print ("DSP READBACK MIX%d MT=%d\n", channel, (int)sranger_common_hwi->read_dsp_feedback ("MT", channel));

        PI_DEBUG_GP (DBG_L4, "%s **3 done\n",__FUNCTION__);
        return 0;
}

int RPSPMC_Control::choice_scansource_callback (GtkWidget *widget, RPSPMC_Control *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) == -1 || g_object_get_data( G_OBJECT (widget), "updating_in_progress")){
                PI_DEBUG_GP (DBG_L4, "%s bail out for label refresh only\n",__FUNCTION__);
                return 0;
        }
            
	gint signal  = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        gint channel = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "scan_channel_source"));

#if 0
	PI_DEBUG_GP (DBG_L3, "ScanSource-Input[%d]=0x%x  <== %s\n",channel,signal, sranger_common_hwi->dsp_signal_lookup_managed[signal].label);
	sranger_common_hwi->change_signal_input (signal, DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID+channel, dspc->DSP_vpdata_ij[0]*8+dspc->DSP_vpdata_ij[1]);
	dspc->scan_source[channel] = signal;

	// manage unit -- TDB
	//	dspc->scan_unit2volt_factor[channel] = 1.;
	//	dspc->update_sourcesignals_from_DSP_callback ();

	// verify and update:
	int si = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID+channel);

        main_get_gapp()->channelselector->SetModeChannelSignal(17+channel, 
                                                               sranger_common_hwi->lookup_dsp_signal_managed (si)->label,
                                                               sranger_common_hwi->lookup_dsp_signal_managed (si)->label,
                                                               sranger_common_hwi->lookup_dsp_signal_managed (si)->unit,
                                                               sranger_common_hwi->lookup_dsp_signal_managed (si)->scale
                                                               );
#else
        main_get_gapp()->channelselector->SetModeChannelSignal(17+channel,
                                                               swappable_signals[signal].label,
                                                               swappable_signals[signal].label,
                                                               swappable_signals[signal].unit,
                                                               swappable_signals[signal].scale_factor
                                                               );
#endif
        
        return 0;
}

void RPSPMC_Control::update_GUI(){
        if (!GUI_ready) return;

	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "DSP_EC_list"),
			(GFunc) App::update_ec, NULL);
	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "DSP_VPC_OPTIONS_list"),
			(GFunc) callback_update_GVP_vpc_option_checkbox, this);
}

void RPSPMC_Control::update_zpos_readings(){
        double zp,a,b;
        main_get_gapp()->xsm->hardware->RTQuery ("z", zp, a, b);
        gchar *info = g_strdup_printf (" (%g Ang)", main_get_gapp()->xsm->Inst->V2ZAng(zp));
        ZPos_ec->set_info (info);
        ZPos_ec->Put_Value ();
        g_free (info);
}

guint RPSPMC_Control::refresh_zpos_readings(RPSPMC_Control *dspc){ 
	if (main_get_gapp()->xsm->hardware->IsSuspendWatches ())
		return TRUE;

	dspc->update_zpos_readings ();
	return TRUE;
}

int RPSPMC_Control::zpos_monitor_callback( GtkWidget *widget, RPSPMC_Control *dspc){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))){
                //g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (widget), "DSP_zpos_control_list"), (GFunc) App::thaw_ec, NULL);
                dspc->zpos_refresh_timer_id = g_timeout_add (200, (GSourceFunc)RPSPMC_Control::refresh_zpos_readings, dspc);
        }else{
                //g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (widget), "DSP_zpos_control_list"), (GFunc) App::freeze_ec, NULL);

                if (dspc->zpos_refresh_timer_id){
                        g_source_remove (dspc->zpos_refresh_timer_id);
                        dspc->zpos_refresh_timer_id = 0;
                }
        }
        return 0;
}


int RPSPMC_Control::choice_prbsource_callback (GtkWidget *widget, RPSPMC_Control *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
        
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) == -1)
                return 0;

        gint selection = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	gint channel   = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "prb_channel_source"));

	dspc->probe_source[channel] = selection;

        // ** template **
        // must reconfigure controller accordingly here...
        // ** template **

        return 0;
}



int RPSPMC_Control::callback_change_IV_option_flags (GtkWidget *widget, RPSPMC_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->IV_option_flags = (dspc->IV_option_flags & (~msk)) | msk;
	else
		dspc->IV_option_flags &= ~msk;

        dspc->set_tab_settings ("IV", dspc->IV_option_flags, dspc->IV_auto_flags, dspc->IV_glock_data);
        return 0;
}

int RPSPMC_Control::callback_change_IV_auto_flags (GtkWidget *widget, RPSPMC_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->IV_auto_flags = (dspc->IV_auto_flags & (~msk)) | msk;
	else
		dspc->IV_auto_flags &= ~msk;

	//if (dspc->write_vector_mode == PV_MODE_IV)
	//	dspc->raster_auto_flags = dspc->IV_auto_flags;

        dspc->set_tab_settings ("IV", dspc->IV_option_flags, dspc->IV_auto_flags, dspc->IV_glock_data);
        return 0;
}


int RPSPMC_Control::Probing_exec_IV_callback( GtkWidget *widget, RPSPMC_Control *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	dspc->current_auto_flags = dspc->IV_auto_flags;

	if (dspc->check_vp_in_progress ()) 
		return -1;

        if (dspc->IV_auto_flags & FLAG_AUTO_RUN_INITSCRIPT){
                gchar *tmp = g_strdup ("vp-sts-initial");
                main_get_gapp()->SignalRemoteActionToPlugins (&tmp);
                g_free (tmp);
        }

        // ** TEMPLATE DUMMY **
        // write code to controller
        //dspc->write_spm_vector_program (0, PV_MODE_NONE);

	if (dspc->IV_auto_flags & FLAG_AUTO_GLOCK){
		dspc->vis_Source  = dspc->IV_glock_data[0];
		dspc->vis_XSource = dspc->IV_glock_data[1];
		dspc->vis_PSource = dspc->IV_glock_data[2];
		dspc->vis_XJoin   = dspc->IV_glock_data[3];
		dspc->vis_PlotAvg = dspc->IV_glock_data[4];
		dspc->vis_PlotSec = dspc->IV_glock_data[5];
	} else {
		dspc->vis_Source = dspc->IV_glock_data[0] = dspc->Source ;
		dspc->vis_XSource= dspc->IV_glock_data[1] = dspc->XSource;
		dspc->vis_PSource= dspc->IV_glock_data[2] = dspc->PSource;
		dspc->vis_XJoin  = dspc->IV_glock_data[3] = dspc->XJoin  ;
		dspc->vis_PlotAvg= dspc->IV_glock_data[4] = dspc->PlotAvg;
		dspc->vis_PlotSec= dspc->IV_glock_data[5] = dspc->PlotSec;
	}

	dspc->current_auto_flags = dspc->IV_auto_flags;


        // ** TEMPLATE DUMMY **
        // exec IV GVP code on controller now and initiate data streaming

        // dspc->probe_trigger_single_shot = 1;
	// dspc->write_spm_vector_program (1, PV_MODE_IV); // Exec STS probing here
	// sranger_common_hwi->start_fifo_read (0, 0,0,0,0, NULL,NULL,NULL,NULL);

	return 0;
}

int RPSPMC_Control::Probing_write_IV_callback( GtkWidget *widget, RPSPMC_Control *dspc){
        // write IV GVP code to controller
        // dspc->write_spm_vector_program (0, PV_MODE_IV);
        return 0;
}

// GVP vector program editor 

int RPSPMC_Control::callback_update_GVP_vpc_option_checkbox (GtkWidget *widget, RPSPMC_Control *dspc){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	int  k   = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "VPC"));
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	gtk_check_button_set_active (GTK_CHECK_BUTTON(widget), (dspc->GVP_opt[k] & msk) ? 1:0);

        dspc->set_tab_settings ("VP", dspc->GVP_option_flags, dspc->GVP_auto_flags, dspc->GVP_glock_data);
        return 0;
}

int RPSPMC_Control::callback_change_GVP_vpc_option_flags (GtkWidget *widget, RPSPMC_Control *dspc){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	int  k = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "VPC"));
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));

	if( gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->GVP_opt[k] = (dspc->GVP_opt[k] & (~msk)) | msk;
	else
		dspc->GVP_opt[k] &= ~msk;

        dspc->set_tab_settings ("VP", dspc->GVP_option_flags, dspc->GVP_auto_flags, dspc->GVP_glock_data);
        return 0;
}

int RPSPMC_Control::callback_change_GVP_option_flags (GtkWidget *widget, RPSPMC_Control *dspc){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->GVP_option_flags = (dspc->GVP_option_flags & (~msk)) | msk;
	else
		dspc->GVP_option_flags &= ~msk;

	if (dspc->write_vector_mode == PV_MODE_GVP)
		dspc->raster_auto_flags = dspc->GVP_auto_flags;

        dspc->set_tab_settings ("VP", dspc->GVP_option_flags, dspc->GVP_auto_flags, dspc->GVP_glock_data);
        dspc->GVP_store_vp ("VP_set_last"); // last in view
        return 0;
}

int RPSPMC_Control::callback_GVP_store_vp (GtkWidget *widget, RPSPMC_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->GVP_store_vp ((const gchar*)g_object_get_data(G_OBJECT(widget), "key"));
        return 0;
}
int RPSPMC_Control::callback_GVP_restore_vp (GtkWidget *widget, RPSPMC_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->GVP_restore_vp ((const gchar*)g_object_get_data(G_OBJECT(widget), "key"));
        return 0;
}

int RPSPMC_Control::callback_change_GVP_auto_flags (GtkWidget *widget, RPSPMC_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->GVP_auto_flags = (dspc->GVP_auto_flags & (~msk)) | msk;
	else
		dspc->GVP_auto_flags &= ~msk;

        dspc->set_tab_settings ("VP", dspc->GVP_option_flags, dspc->GVP_auto_flags, dspc->GVP_glock_data);
        return 0;
}

int RPSPMC_Control::Probing_exec_GVP_callback( GtkWidget *widget, RPSPMC_Control *dspc){
	dspc->current_auto_flags = dspc->GVP_auto_flags;

	if (dspc->check_vp_in_progress ()) 
		return -1;

        if (dspc->GVP_auto_flags & FLAG_AUTO_RUN_INITSCRIPT){
                gchar *tmp = g_strdup ("vp-gvp-initial");
                main_get_gapp()->SignalRemoteActionToPlugins (&tmp);
                g_free (tmp);
        }
      
	if (dspc->GVP_auto_flags & FLAG_AUTO_GLOCK){
		dspc->vis_Source  = dspc->GVP_glock_data[0];
		dspc->vis_XSource = dspc->GVP_glock_data[1];
		dspc->vis_PSource = dspc->GVP_glock_data[2];
		dspc->vis_XJoin   = dspc->GVP_glock_data[3];
		dspc->vis_PlotAvg = dspc->GVP_glock_data[4];
		dspc->vis_PlotSec = dspc->GVP_glock_data[5];
	} else {
		dspc->vis_Source = dspc->GVP_glock_data[0] = dspc->Source ;
		dspc->vis_XSource= dspc->GVP_glock_data[1] = dspc->XSource;
		dspc->vis_PSource= dspc->GVP_glock_data[2] = dspc->PSource;
		dspc->vis_XJoin  = dspc->GVP_glock_data[3] = dspc->XJoin  ;
		dspc->vis_PlotAvg= dspc->GVP_glock_data[4] = dspc->PlotAvg;
		dspc->vis_PlotSec= dspc->GVP_glock_data[5] = dspc->PlotSec;
	}

	dspc->current_auto_flags = dspc->GVP_auto_flags;

        // ** TEMPLATE DUMMY **
        // write and exec GVP code on controller and initiate data streaming
        
	// dspc->probe_trigger_single_shot = 1;
	dspc->write_spm_vector_program (1, PV_MODE_GVP); // Write and Exec GVP program
	rpspmc_hwi->start_data_read (0, 0,0,0,0, NULL,NULL,NULL,NULL); // init data streaming -- non blocking thread is fired up

	return 0;
}


int RPSPMC_Control::Probing_write_GVP_callback( GtkWidget *widget, RPSPMC_Control *dspc){
        // write GVP code to controller
        dspc->write_spm_vector_program (0, PV_MODE_GVP);
        return 0;
}

// Graphs Callbacks

int RPSPMC_Control::change_source_callback (GtkWidget *widget, RPSPMC_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	long channel;
	channel = (long) GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "Source_Channel"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) {
		if (channel & X_SOURCE_MSK)
			dspc->XSource |= channel;
		else if (channel & P_SOURCE_MSK)
			dspc->PSource |= channel;
		else if (channel & A_SOURCE_MSK)
			dspc->PlotAvg |= channel;
		else if (channel & S_SOURCE_MSK)
			dspc->PlotSec |= channel;
		else
			dspc->Source |= channel;
		
	}
	else {
		if (channel & X_SOURCE_MSK)
			dspc->XSource &= ~channel;
		else if (channel & P_SOURCE_MSK)
			dspc->PSource &= ~channel;
		else if (channel & A_SOURCE_MSK)
			dspc->PlotAvg &= ~channel;
		else if (channel & S_SOURCE_MSK)
			dspc->PlotSec &= ~channel;
		else
			dspc->Source &= ~channel;
	}


	dspc->vis_Source = dspc->Source;
	dspc->vis_XSource = dspc->XSource;
	dspc->vis_PSource = dspc->PSource;
	dspc->vis_XJoin = dspc->XJoin;
	dspc->vis_PlotAvg = dspc->PlotAvg;
	dspc->vis_PlotSec = dspc->PlotSec;

	return 0;
}

int RPSPMC_Control::callback_XJoin (GtkWidget *widget, RPSPMC_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->XJoin = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? TRUE : FALSE;
	dspc->vis_XJoin = dspc->XJoin;
        g_settings_set_boolean (dspc->hwi_settings, "probe-x-join", dspc->XJoin);
        return 0;
}

int RPSPMC_Control::callback_GrMatWindow (GtkWidget *widget, RPSPMC_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->GrMatWin = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? TRUE : FALSE;
        g_settings_set_boolean (dspc->hwi_settings, "probe-graph-matrix-window", dspc->GrMatWin);
        return 0;
}


void RPSPMC_Control::lockin_adjust_callback(Param_Control* pcs, gpointer data){
	RPSPMC_Control *dspc = (RPSPMC_Control*)data;
}

int RPSPMC_Control::lockin_runfree_callback(GtkWidget *widget, RPSPMC_Control *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                PI_DEBUG_GP (DBG_L1, "LockIn Modul ON");
	else
                PI_DEBUG_GP (DBG_L1, "LockIn Modul OFF");

	return 0;
}




void RPSPMC_Control::update_controller () {

        // SCAN SPEED COMPUTATIONS -- converted to 16.16 fixed point scan generator parameters (TEMPLATE, replace with what ever)
        double frac  = (1<<16);
        // *** FIX
        double fs_dx = frac * rpspmc_pacpll_hwi_pi.app->xsm->Inst->XA2Dig (scan_speed_x_requested) / 200e3; //rpspmc_hwi->get_GVP_frq_ref ();
        double fs_dy = frac * rpspmc_pacpll_hwi_pi.app->xsm->Inst->YA2Dig (scan_speed_x_requested) / 200e3; //rpspmc_hwi->get_GVP_frq_ref ();
#if 0
        if ((frac * rpspmc_hwi->Dx / fs_dx) > (1<<15) || (frac * rpspmc_hwi->Dy / fs_dx) > (1<<15)){
                main_get_gapp()->message (N_("WARNING:\n"
                                             "recalculate_dsp_scan_parameters:\n"
                                             "requested/resulting scan speed is too slow.\n"
                                             "Reaching 1<<15 steps inbetween!\n"
                                             "No change on DSP performed."));
                PI_DEBUG (DBG_EVER, "WARNING: recalculate_dsp_scan_parameters: too slow, reaching 1<<15 steps inbetween! -- no change.");
                return;
        }
#endif
        // fs_dx * N =!= frac*Dx  -> N = ceil [frac*Dx/fs_dx]  -> fs_dx' = frac*Dx/N
        
        // N: dnx
        dsp_scan_dnx = (gint32) ceil (frac * rpspmc_hwi->Dx / fs_dx);
        dsp_scan_dny = (gint32) ceil (frac * rpspmc_hwi->Dy / fs_dy);
        
        dsp_scan_fs_dx = (gint32) (frac*rpspmc_hwi->Dx / ceil (frac * rpspmc_hwi->Dx / fs_dx));
        dsp_scan_fs_dy = (gint32) (frac*rpspmc_hwi->Dy / ceil (frac * rpspmc_hwi->Dy / fs_dy));
                
        mirror_dsp_scan_dx32 = dsp_scan_fs_dx*dsp_scan_dnx; // actual DSP dx in S15.16 between pixels in X
        mirror_dsp_scan_dy32 = dsp_scan_fs_dy*dsp_scan_dny; // actual DSP dy in S15.16 between pixels in Y
        
        dsp_scan_fast_return = (gint32) (fast_return);
        if (dsp_scan_fast_return < 1)
                dsp_scan_fast_return = 1;
        if (dsp_scan_fast_return > 10000)
                dsp_scan_fast_return = 1;

        //dsp_scan_nx_pre = dsp_scan_dnx * pre_points;
        dsp_scan_fs_dy *= rpspmc_hwi->scan_direction;

        //info only, updates scan speed GUI entry with actual rates (informative only)
        // *** FIX ME !!
        scan_speed_x = rpspmc_pacpll_hwi_pi.app->xsm->Inst->Dig2XA ((long)(dsp_scan_fs_dx * 200e3 / frac)); // rpspmc_hwi->get_GVP_frq_ref ()
        // ************* FIX
        scanpixelrate = (double)dsp_scan_dnx/200e3;   // rpspmc_hwi->get_GVP_frq_ref ();
        gchar *info = g_strdup_printf (" (%g A/s, %g ms/pix)", scan_speed_x, scanpixelrate*1e3);
        scan_speed_ec->set_info (info);
        g_free (info);

        // SCAN PLANE/SLOPE COMPENSATION
        if (area_slope_compensation_flag){
                //double zx_ratio = sranger_mk2_hwi_pi.app->xsm->Inst->XResolution () / sranger_mk2_hwi_pi.app->xsm->Inst->ZModResolution (); 
                //double zy_ratio = sranger_mk2_hwi_pi.app->xsm->Inst->YResolution () / sranger_mk2_hwi_pi.app->xsm->Inst->ZModResolution ();
                //double mx = zx_ratio * area_slope_x;
                //double my = zy_ratio * area_slope_y;
                //dsp_scan_fm_dz0x = (gint32)round (fract * mx);
                //dsp_scan_fm_dz0y = (gint32)round (fract * my);

        }
        
        rpspmc_hwi->RPSPMC_set_bias (bias);
        rpspmc_hwi->RPSPMC_set_current_sp (mix_set_point[0]);
        g_message ("*** Update Controller: Bias-SP: %g V, Current-SP: %g nA", bias, mix_set_point[0]);
}





/* **************************************** END SPM Control GUI **************************************** */

/* **************************************** PAC-PLL GUI **************************************** */

#define dB_min_from_Q(Q) (20.*log10(1./((1L<<(Q))-1)))
#define dB_max_from_Q(Q) (20.*log10(1L<<((32-(Q))-1)))
#define SETUP_dB_RANGE_from_Q(PCS, Q) { PCS->setMin(dB_min_from_Q(Q)); PCS->setMax(dB_max_from_Q(Q)); }

RPspmc_pacpll::RPspmc_pacpll (Gxsm4app *app):AppBase(app),RP_JSON_talk(){
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
        bp->new_line ();
        bp->set_input_width_chars (8);
        bp->set_input_nx (2);
  	bp->grid_add_ec ("Center", Hz, &parameters.frequency_center, 0.0, 20e6, ".3lf", 0.1, 10., "FREQUENCY-CENTER");
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
        bp->grid_add_check_button ( N_("Enable"), "Enable Amplitude Controller", 2,
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
        bp->grid_add_check_button ( N_("Enable"), "Enable Phase Controller", 2,
                                    G_CALLBACK (RPspmc_pacpll::phase_controller), this);
        bp->grid_add_check_button ( N_("Invert"), "Invert Phase Controller Gain. Normally positive.", 2,
                                    G_CALLBACK (RPspmc_pacpll::phase_controller_invert), this);
        bp->new_line ();
        bp->grid_add_check_button ( N_("Unwapping"), "Always unwrap phase/auto unwrap only if controller is enabled", 2,
                                    G_CALLBACK (RPspmc_pacpll::phase_unwrapping_always), this);
        bp->grid_add_check_button ( N_("Unwap Plot"), "Unwrap plot at high level", 2,
                                    G_CALLBACK (RPspmc_pacpll::phase_unwrap_plot), this);
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
        bp->grid_add_check_button ( N_("Enable"), "Enable Dfreq Controller", 2,
                                    G_CALLBACK (RPspmc_pacpll::dfreq_controller), this);
        bp->grid_add_check_button ( N_("Invert"), "Invert Dfreq Controller Gain. Normally positive.", 2,
                                    G_CALLBACK (RPspmc_pacpll::dfreq_controller_invert), this);

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
        bp->new_line ();
        
        bp->set_input_nx (1);
        bp->grid_add_check_button ( N_("Enable"), "Enable Pulse Forming", 2,
                                    G_CALLBACK (RPspmc_pacpll::pulse_form_enable), this);

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


        // ========================================
        
        bp->pop_grid ();
        bp->new_line ();

#if 0
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
                                    G_CALLBACK (RPspmc_pacpll::connect_cb), this);
        bp->grid_add_check_button ( N_("Debug"), "Enable debugging LV1.", 1,
                                    G_CALLBACK (RPspmc_pacpll::dbg_l1), this);
        bp->grid_add_check_button ( N_("+"), "Debug LV2", 1,
                                    G_CALLBACK (RPspmc_pacpll::dbg_l2), this);
        bp->grid_add_check_button ( N_("++"), "Debug LV4", 1,
                                    G_CALLBACK (RPspmc_pacpll::dbg_l4), this);
        rp_verbose_level = 0.0;
        bp->set_input_width_chars (2);
  	bp->grid_add_ec ("V", Unity, &rp_verbose_level, 0., 10., ".0f", 1., 1., "RP-VERBOSE-LEVEL");        
        //bp->new_line ();
        //tmp=bp->grid_add_button ( N_("Read"), "TEST READ", 1,
        //                          G_CALLBACK (RPspmc_pacpll::read_cb), this);
        //tmp=bp->grid_add_button ( N_("Write"), "TEST WRITE", 1,
        //                          G_CALLBACK (RPspmc_pacpll::write_cb), this);

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
        gtk_widget_set_vexpand (scrolled_window, TRUE);
        gtk_scrolled_window_set_child ( GTK_SCROLLED_WINDOW (scrolled_window), text_status);
        bp->grid_add_widget (scrolled_window, 10);
        gtk_widget_show (scrolled_window);
        
        // ========================================
        bp->pop_grid ();
        
        bp->show_all ();
 #endif
        // save List away...
	//g_object_set_data( G_OBJECT (window), "RPSPMCONTROL_EC_list", EC_list);

	g_object_set_data( G_OBJECT (window), "RPSPMCONTROL_EC_READINGS_list", EC_R_list);

        set_window_geometry ("rpspmc-pacpll-control"); // needs rescoure entry and defines window menu entry as geometry is managed

	rpspmc_pacpll_hwi_pi.app->RemoteEntryList = g_slist_concat (rpspmc_pacpll_hwi_pi.app->RemoteEntryList, bp->remote_list_ec);

        
        // hookup to scan start and stop
        rpspmc_pacpll_hwi_pi.app->ConnectPluginToStartScanEvent (RPspmc_pacpll::scan_start_callback);
        rpspmc_pacpll_hwi_pi.app->ConnectPluginToStopScanEvent (RPspmc_pacpll::scan_stop_callback);
}

RPspmc_pacpll::~RPspmc_pacpll (){
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
        self->json_talk_connect_cb (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))); // connect (checked) or dissconnect
}


void RPspmc_pacpll::scan_start_callback (gpointer user_data){
        //rpspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        RPspmc_pacpll *self = rpspmc_pacpll;
        self->ch_freq = -1;
        self->ch_ampl = -1;
        self->streaming = 1;
        //self->operation_mode = 0;
        g_message ("RPspmc_pacpll::scan_start_callback");
#if 0
        if ((self->ch_freq=main_get_gapp()->xsm->FindChan(xsmres.extchno[0])) >= 0)
                self->setup_scan (self->ch_freq, "X+", "Ext1-Freq", "Hz", "Freq", 1.0);
        if ((self->ch_ampl=main_get_gapp()->xsm->FindChan(xsmres.extchno[1])) >= 0)
                self->setup_scan (self->ch_ampl, "X+", "Ext1-Ampl", "V", "Ampl", 1.0);
#endif
}

void RPspmc_pacpll::scan_stop_callback (gpointer user_data){
        //RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        RPspmc_pacpll *self = rpspmc_pacpll;
        self->ch_freq = -1;
        self->ch_ampl = -1;
        self->streaming = 0;
        g_message ("RPspmc_pacpll::scan_stop_callback");
}

int RPspmc_pacpll::setup_scan (int ch, 
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

void RPspmc_pacpll::pulse_form_enable (GtkWidget *widget, RPspmc_pacpll *self){
        self->parameters.pulse_form_enable = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
        self->pulse_form_parameter_changed (NULL, self);
}

void RPspmc_pacpll::save_values (NcFile *ncf){
        // store all rpspmc_pacpll's control parameters for the RP PAC-PLL
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

void RPspmc_pacpll::choice_operation_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->operation_mode = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->write_parameter ("OPERATION", self->operation_mode);
        self->write_parameter ("PACVERBOSE", (int)self->rp_verbose_level);
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
        self->write_parameter ("BRAM_SCOPE_TRIGGER_MODE", gtk_combo_box_get_active (GTK_COMBO_BOX (widget)));
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

void RPspmc_pacpll::scope_buffer_position_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("BRAM_SCOPE_SHIFT_POINTS", self->bram_shift = (int)gtk_range_get_value (GTK_RANGE (widget)));
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

void RPspmc_pacpll::update(){
	if (G_IS_OBJECT (window))
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "RPSPMCONTROL_EC_list"),
				(GFunc) App::update_ec, NULL);
}

void RPspmc_pacpll::update_monitoring_parameters(){

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
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "RPSPMCONTROL_EC_READINGS_list"),
				(GFunc) App::update_ec, NULL);
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
        if (msg){
                gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY((red_pitaya_health)))), msg, -1);
        } else {
#if 0
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
                gchar *health_string = g_strdup_printf ("CPU: %03.0f%% Free: %6.1f MB %s #%g",
                                                        pacpll_parameters.cpu_load,
                                                        pacpll_parameters.free_ram/1024/1024,
                                                        gpiox_string?gpiox_string:"[]", pacpll_parameters.counter);
                g_free (gpiox_string);
#else
                gchar *health_string = g_strdup_printf ("CPU: %03.0f%% Free: %6.1f MB #%g",
                                                        pacpll_parameters.cpu_load,
                                                        pacpll_parameters.free_ram/1024/1024,
                                                        pacpll_parameters.counter);
#endif
                gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY((red_pitaya_health)))), health_string, -1);
                g_free (health_string);
        }
}

void RPspmc_pacpll::status_append (const gchar *msg){

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

void RPspmc_pacpll::on_connect_actions(){
        status_append ("RedPitaya SPM Control, PAC-PLL loading configuration.\n ");
        send_all_parameters ();
        
        status_append ("RedPitaya SPM Control, PAC-PLL init, DEC FAST(12)...\n");
        gtk_combo_box_set_active (GTK_COMBO_BOX (update_tr_widget), 6);  // select Ph,dF,Am,Ex
        gtk_combo_box_set_active (GTK_COMBO_BOX (update_ts_widget), 12); // select 12, fast
        while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
        usleep(500000);
        
        gtk_combo_box_set_active (GTK_COMBO_BOX (update_op_widget), 3); // INIT BRAM TRANSPORT AND CLEAR FIR RING BUFFERS, give me a second...
        status_append ("RedPitaya SPM Control, PAC-PLL init, INIT-FIR... [2s Zzzz]\n");
        while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
        usleep(2000000);
        
        status_append ("RedPitaya SPM Control, PAC-PLL init, INIT-FIR completed.\n");
        usleep(1000000);
        
        status_append ("RedPitaya SPM Control, PAC-PLL normal operation, set to data streaming mode.\n");
        while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
        usleep(500000);
        
        gtk_combo_box_set_active (GTK_COMBO_BOX (update_op_widget), 5); // STREAMING OPERATION
        gtk_combo_box_set_active (GTK_COMBO_BOX (update_ts_widget), 18); // select 19 (typical scan decimation/time scale filter)
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

void RPspmc_pacpll::stream_data (){
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

double RPspmc_pacpll::unwrap (int k, double phi){
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

// END
