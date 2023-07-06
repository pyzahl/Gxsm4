/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Hardware Interface Plugin Name: spm_template_hwi.C
 * ===============================================
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
 * All "% OptPlugInXXX" tags are optional and can be removed or commented in
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: SPM Template Hardware Interface
% PlugInName: spm_template_hwi
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Tools/SPM Control Template

% PlugInDescription
\index{SPM HwI Template Code}

%% OptPlugInSources

%% OptPlugInDest

% OptPlugInNote

% EndPlugInDocuSection
* -------------------------------------------------------------------------------- 
*/

#include <sys/ioctl.h>

#include "config.h"
#include "plugin.h"
#include "xsmhard.h"
#include "glbvars.h"


#include "gxsm_app.h"
#include "gxsm_window.h"
#include "surface.h"

#include "../common/pyremote.h"
#include "spm_template_hwi.h"
#include "spm_template_hwi_emulator.h"

// Define HwI PlugIn reference name here, this is what is listed later within "Preferenced Dialog"
// i.e. the string selected for "Hardware/Card"!
#define THIS_HWI_PLUGIN_NAME "SPM-TEMPL:SPM"
#define THIS_HWI_PREFIX      "SPM_TEMPL_HwI"


#define UTF8_DEGREE    "\302\260"
#define UTF8_MU        "\302\265"
#define UTF8_ANGSTROEM "\303\205"

#define X_SOURCE_MSK 0x10000000 // select for X-mapping
#define P_SOURCE_MSK 0x20000000 // select for plotting
#define A_SOURCE_MSK 0x40000000 // select for Avg plotting
#define S_SOURCE_MSK 0x80000000 // select for Sec plotting

#define GVP_SHIFT_UP  1
#define GVP_SHIFT_DN -1


extern int debug_level;
extern int force_gxsm_defaults;


extern "C++" {
        extern SPM_Template_Control *Template_ControlClass;
        extern GxsmPlugin spm_template_hwi_pi;
}

SOURCE_SIGNAL_DEF source_signals[] = {
	{ 0x000010, "ADC0-I", " ", "nA", 1.0 },
        { 0x000020, "ADC1", " ", "V", 1.0 },
        { 0x000040, "ADC2", " ", "V", 1.0 },
        { 0x000080, "ADC3", " ", "V", 1.0 },
        { 0x000100, "ADC4", " ", "V", 1.0 },
        { 0x000200, "ADC5", " ", "V", 1.0 },
        { 0x000400, "ADC6", " ", "V", 1.0 },
        { 0x000800, "ADC7", " ", "V", 1.0 },
        { 0x000008, "LockIn0", " ", "V", 1.0 },
        { 0x001000, NULL, " ", "V", 1.0 }, // ** swappable **,
        { 0x002000, NULL, " ", "V", 1.0 }, // ** swappable **,
        { 0x004000, NULL, " ", "V", 1.0 }, // ** swappable **,
        { 0x008000, NULL, " ", "V", 1.0 }, // ** swappable **,
        { 0x0100000, "Time", " ", "s", 1.0 },
        { 0x0000001, "Z-mon", " ", "AA", 1.0 },
        { 0x0000002, "Bias-mon", " ", "V", 1.0 },
        { 0x1000000, "SEC", " ", "#", 1.0 },
        { 0, NULL, NULL, NULL, 0.0 }
};

SOURCE_SIGNAL_DEF swappable_signals[] = {
        { 1, "LockIn-X", " ", "V", 1.0 },
        { 2, "LockIn-Y", " ", "V", 1.0 },
	{ 3, "PLL-Freq", " ", "Hz", 1.0 },
	{ 4, "PLL-Phase", " ", "deg", 1.0 },
	{ 5, "PLL-Exec", " ", "mV", 1000.0 },
	{ 6, "PLL-Ampl", " ", "mV", 1000.0 },
        { 0, NULL, NULL, NULL, 0.0 }
};

MOD_INPUT mod_input_list[] = {
        //## [ MODULE_SIGNAL_INPUT_ID, name, actual hooked signal address ]
        { DSP_SIGNAL_Z_SERVO_INPUT_ID, "Z_SERVO", 0 },
        { DSP_SIGNAL_M_SERVO_INPUT_ID, "M_SERVO", 0 },
        { DSP_SIGNAL_MIXER0_INPUT_ID, "MIXER0", 0 },
        { DSP_SIGNAL_MIXER1_INPUT_ID, "MIXER1", 0 },
        { DSP_SIGNAL_MIXER2_INPUT_ID, "MIXER2", 0 },
        { DSP_SIGNAL_MIXER3_INPUT_ID, "MIXER3", 0 },

        { DSP_SIGNAL_DIFF_IN0_ID, "DIFF_IN0", 0 },
        { DSP_SIGNAL_DIFF_IN1_ID, "DIFF_IN1", 0 },
        { DSP_SIGNAL_DIFF_IN2_ID, "DIFF_IN2", 0 },
        { DSP_SIGNAL_DIFF_IN3_ID, "DIFF_IN3", 0 },

        { DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID, "SCAN_CHMAP0", 0 },
        { DSP_SIGNAL_SCAN_CHANNEL_MAP1_ID, "SCAN_CHMAP1", 0 },
        { DSP_SIGNAL_SCAN_CHANNEL_MAP2_ID, "SCAN_CHMAP2", 0 },
        { DSP_SIGNAL_SCAN_CHANNEL_MAP3_ID, "SCAN_CHMAP3", 0 },

        { DSP_SIGNAL_LOCKIN_A_INPUT_ID, "LOCKIN_A", 0 },
        { DSP_SIGNAL_LOCKIN_B_INPUT_ID, "LOCKIN_B", 0 },
        { DSP_SIGNAL_VECPROBE0_INPUT_ID, "VECPROBE0", 0 },
        { DSP_SIGNAL_VECPROBE1_INPUT_ID, "VECPROBE1", 0 },
        { DSP_SIGNAL_VECPROBE2_INPUT_ID, "VECPROBE2", 0 },
        { DSP_SIGNAL_VECPROBE3_INPUT_ID, "VECPROBE3", 0 },
        { DSP_SIGNAL_VECPROBE0_CONTROL_ID, "VECPROBE0_C", 0 },
        { DSP_SIGNAL_VECPROBE1_CONTROL_ID, "VECPROBE1_C", 0 },
        { DSP_SIGNAL_VECPROBE2_CONTROL_ID, "VECPROBE2_C", 0 },
        { DSP_SIGNAL_VECPROBE3_CONTROL_ID, "VECPROBE3_C", 0 },

        { DSP_SIGNAL_OUTMIX_CH5_INPUT_ID, "OUTMIX_CH5", 0 },
        { DSP_SIGNAL_OUTMIX_CH5_ADD_A_INPUT_ID, "OUTMIX_CH5_ADD_A", 0 },
        { DSP_SIGNAL_OUTMIX_CH5_SUB_B_INPUT_ID, "OUTMIX_CH5_SUB_B", 0 },
        { DSP_SIGNAL_OUTMIX_CH5_SMAC_A_INPUT_ID, "OUTMIX_CH5_SMAC_A", 0 },
        { DSP_SIGNAL_OUTMIX_CH5_SMAC_B_INPUT_ID, "OUTMIX_CH5_SMAC_B", 0 },

        { DSP_SIGNAL_OUTMIX_CH6_INPUT_ID, "OUTMIX_CH6", 0 },
        { DSP_SIGNAL_OUTMIX_CH6_ADD_A_INPUT_ID, "OUTMIX_CH6_ADD_A", 0 },
        { DSP_SIGNAL_OUTMIX_CH6_SUB_B_INPUT_ID, "OUTMIX_CH6_SUB_B", 0 },
        { DSP_SIGNAL_OUTMIX_CH6_SMAC_A_INPUT_ID, "OUTMIX_CH6_SMAC_A", 0 },
        { DSP_SIGNAL_OUTMIX_CH6_SMAC_B_INPUT_ID, "OUTMIX_CH6_SMAC_B", 0 },

        { DSP_SIGNAL_OUTMIX_CH7_INPUT_ID, "OUTMIX_CH7", 0 },
        { DSP_SIGNAL_OUTMIX_CH7_ADD_A_INPUT_ID, "OUTMIX_CH7_ADD_A", 0 },
        { DSP_SIGNAL_OUTMIX_CH7_SUB_B_INPUT_ID, "OUTMIX_CH7_SUB_B", 0 },
        { DSP_SIGNAL_OUTMIX_CH7_SMAC_A_INPUT_ID, "OUTMIX_CH7_SMAC_A", 0 },
        { DSP_SIGNAL_OUTMIX_CH7_SMAC_B_INPUT_ID, "OUTMIX_CH7_SMAC_B", 0 },

        { DSP_SIGNAL_OUTMIX_CH8_INPUT_ID, "OUTMIX_CH8_INPUT", 0 },
        { DSP_SIGNAL_OUTMIX_CH8_ADD_A_INPUT_ID, "OUTMIX_CH8_ADD_A_INPUT", 0 },
        { DSP_SIGNAL_OUTMIX_CH9_INPUT_ID, "OUTMIX_CH9_INPUT", 0 },
        { DSP_SIGNAL_OUTMIX_CH9_ADD_A_INPUT_ID, "OUTMIX_CH9_ADD_A_INPUT", 0 },

        { DSP_SIGNAL_ANALOG_AVG_INPUT_ID, "ANALOG_AVG_INPUT", 0 },
        { DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID, "SCOPE_SIGNAL1_INPUT", 0 },
        { DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID, "SCOPE_SIGNAL2_INPUT", 0 },

        { 0, "END", 0 }
};









// Plugin Prototypes
static void spm_template_hwi_init( void );
static void spm_template_hwi_about( void );
static void spm_template_hwi_configure( void );
static void spm_template_hwi_query( void );
static void spm_template_hwi_cleanup( void );

static void Template_Control_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// custom menu callback expansion to arbitrary long list
static PIMenuCallback spm_template_hwi_mcallback_list[] = {
		Template_Control_show_callback,             // direct menu entry callback1
		NULL  // NULL terminated list
};

static GxsmPluginMenuCallbackList spm_template_hwi_menu_callback_list = {
	1,
	spm_template_hwi_mcallback_list
};

GxsmPluginMenuCallbackList *get_gxsm_plugin_menu_callback_list ( void ) {
	return &spm_template_hwi_menu_callback_list;
}

// Fill in the GxsmPlugin Description here
GxsmPlugin spm_template_hwi_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	"spm_template_hwi-"
	"HW-INT-1S-SHORT",
	// Plugin's Category - used to autodecide on Pluginloading or ignoring
	// In this case of Hardware-Interface-Plugin here is the interface-name required
	// this is the string selected for "Hardware/Card"!
	THIS_HWI_PLUGIN_NAME,
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	g_strdup ("SPM template hardware interface."),
	// Author(s)
	"Percy Zahl",
	// Menupath to position where it is appendet to -- not used by HwI PIs
	"windows-section,windows-section,windows-section,windows-section",
	// Menuentry -- not used by HwI PIs
	N_("SPM Control Template"),
	// help text shown on menu
	N_("This is the " THIS_HWI_PLUGIN_NAME " - GXSM Hardware Interface."),
	// more info...
	"N/A",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	spm_template_hwi_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	spm_template_hwi_query, // query can be used (otherwise set to NULL) to install
	// additional control dialog in the GXSM menu
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	spm_template_hwi_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	spm_template_hwi_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
	NULL,   // direct menu entry callback1 or NULL
	NULL,   // direct menu entry callback2 or NULL
	spm_template_hwi_cleanup // plugin cleanup callback or NULL
};


// Text used in Aboutbox, please update!!
static const char *about_text = N_("GXSM spm_template_hwi Plugin\n\n"
                                   "Template HwI Interface.");

/* Here we go... */

#include "spm_template_hwi.h"

/*
 * PI global
 */

// #define PI_DEBUG(L, DBGTXT) std::cout << "** (" << __FILE__ << ": " << __FUNCTION__ << ") Gxsm-PI-DEBUG-MESSAGE **: " << std::endl << " - " << DBGTXT << std::endl

gchar *spm_template_hwi_configure_string = NULL;   // name of the currently in GXSM configured HwI (Hardware/Card)
spm_template_hwi_dev *spm_template_hwi = NULL; // instance of the HwI derived XSM_Hardware class

const gchar *Template_Control_menupath  = "windows-section";
const gchar *Template_Control_menuentry = N_("Control Template");
const gchar *Template_Control_menuhelp  = N_("open the SPM control template window");

SPM_Template_Control *Template_ControlClass = NULL;


// event hooks
static void Template_Control_StartScan_callback ( gpointer );
static void Template_Control_SaveValues_callback ( gpointer );
static void Template_Control_LoadValues_callback ( gpointer );

/* 
 * PI essential members
 */

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	spm_template_hwi_pi.description = g_strdup_printf(N_("GXSM HwI " THIS_HWI_PREFIX " plugin %s"), VERSION);
	return &spm_template_hwi_pi; 
}

// Symbol "get_gxsm_hwi_hardware_class" is resolved by dlsym from Gxsm for all HwI type PIs, 
// Essential Plugin Function!!
XSM_Hardware *get_gxsm_hwi_hardware_class ( void *data ) {
        gchar *tmp;
        main_get_gapp()->monitorcontrol->LogEvent (THIS_HWI_PREFIX " XSM_Hardware *get_gxsm_hwi_hardware_class", "Init 1");

	spm_template_hwi_configure_string = g_strdup ((gchar*)data);

	// probe for harware here...
	spm_template_hwi = new spm_template_hwi_dev ();
	
        main_get_gapp()->monitorcontrol->LogEvent ("HwI: probing succeeded.", "SPM Template System Ready.");
	return spm_template_hwi;
}

// init-Function
static void spm_template_hwi_init(void)
{
	PI_DEBUG (DBG_L2, "spm_template_hwi Plugin Init");
	spm_template_hwi = NULL;
}

// about-Function
static void spm_template_hwi_about(void)
{
	const gchar *authors[] = { spm_template_hwi_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  spm_template_hwi_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void spm_template_hwi_configure(void)
{
	PI_DEBUG (DBG_L2, "spm_template_hwi Plugin HwI-Configure");
	if(spm_template_hwi_pi.app)
		spm_template_hwi_pi.app->message("spm_template_hwi Plugin Configuration");
}

// query-Function
static void spm_template_hwi_query(void)
{
	PI_DEBUG_GP (DBG_L1, THIS_HWI_PREFIX "::spm_template_hwi_query:: PAC check... ");

//      Setup Control Windows
// ==================================================
	Template_ControlClass = new SPM_Template_Control (main_get_gapp() -> get_app ());
        
	spm_template_hwi_pi.status = g_strconcat(N_("Plugin query has attached "),
                                                  spm_template_hwi_pi.name, 
                                                  N_(": " THIS_HWI_PREFIX "-Control is created."),
                                                  NULL);
}


// Hwi Plugin cleanup-Function
static void spm_template_hwi_cleanup(void)
{
	PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::spm_template_hwi_cleanup -- Plugin Cleanup, -- Menu [disabled]\n");

        // remove GUI
	if( Template_ControlClass ){
		delete Template_ControlClass ;
        }
	Template_ControlClass = NULL;

	if (spm_template_hwi){
                PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::spm_template_hwi_cleanup -- delete DSP Common-HwI-Class.");
		delete spm_template_hwi;
        }
	spm_template_hwi = NULL;

        PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::spm_template_hwi_cleanup -- g_free spm_template_hwi_configure_string Info.");
	g_free (spm_template_hwi_configure_string);
	spm_template_hwi_configure_string = NULL;
	
        PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::spm_template_hwi_cleanup -- Done.");
}





// GUI Section -- custom to HwI
// ================================================================================

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
                          G_CALLBACK (SPM_Template_Control::choice_mixmode_callback), 
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
                          G_CALLBACK (SPM_Template_Control::choice_scansource_callback), 
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
        if (preset > 0)
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), preset); 
        else
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 4); // NULL SIGNAL [TESTING FALLBACK for -1/error]
                
        g_signal_connect (G_OBJECT (cbtxt), "changed",	
                          G_CALLBACK (SPM_Template_Control::choice_prbsource_callback), 
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
                         G_CALLBACK (SPM_Template_Control::Probing_save_callback), cb_data);         

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


static GActionEntry win_SPM_Template_popup_entries[] = {
        { "dsp-mover-configure", SPM_Template_Control::configure_callback, NULL, "true", NULL },
};

void Template_Control_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
        ; // show window... if dummy closed it...
}





void SPM_Template_Control::configure_callback (GSimpleAction *action, GVariant *parameter, 
                                          gpointer user_data){
        SPM_Template_Control *mc = (SPM_Template_Control *) user_data;
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
                          (GFunc) SPM_Template_Control::show_tab_to_configure, NULL
                          );
        } else  {
                g_slist_foreach
                        ( mc->bp->get_configure_list_head (),
                          (GFunc) App::hide_w, NULL
                          );
                g_slist_foreach
                        ( mc->bp->get_config_checkbutton_list_head (),
                          (GFunc) SPM_Template_Control::show_tab_as_configured, NULL
                          );
        }
        if (!action){
                g_variant_unref (new_state);
        }
}








void SPM_Template_Control::store_values (){
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
        PI_DEBUG_GM (DBG_L3, "SPM_Template_Control::store_values complete.");
}

void SPM_Template_Control::GVP_store_vp (const gchar *key){
	PI_DEBUG_GM (DBG_L3, "GVP-VP store to memo %s", key);
	g_message ("GVP-VP store to memo %s", key);
        GVariant *v = g_settings_get_value (hwi_settings, "probe-gvp-vector-program-matrix");
        //GVariant *v = g_settings_get_value (hwi_settings, "probe-lm-vector-program-matrix");
        GVariantDict *dict = g_variant_dict_new (v);
        if (!dict){
                PI_DEBUG_GW (DBG_L1, "ERROR: SPM_Template_Control::GVP_store_vp -- can't read dictionary 'probe-gvp-vector-program-matrix' a{sv}.");
                return;
        }

        gint32 vp_program_length;
        for (vp_program_length=0; GVP_points[vp_program_length] > 0; ++vp_program_length);
                
        gsize    n = MIN (vp_program_length+1, N_GVP_VECTORS);
        GVariant *pc_array_du = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_du, n, sizeof (double));
        GVariant *pc_array_dx = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_dx, n, sizeof (double));
        GVariant *pc_array_dy = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_dy, n, sizeof (double));
        GVariant *pc_array_dz = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_dz, n, sizeof (double));
        GVariant *pc_array_ds = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_dsig, n, sizeof (double));
        GVariant *pc_array_ts = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_ts, n, sizeof (double));
        GVariant *pc_array_pn = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_points, n, sizeof (gint32));
        GVariant *pc_array_op = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_opt, n, sizeof (gint32));
        GVariant *pc_array_da = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_data, n, sizeof (gint32));
        GVariant *pc_array_vn = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_vnrep, n, sizeof (gint32));
        GVariant *pc_array_vp = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_vpcjr, n, sizeof (gint32));

        GVariant *pc_array[] = { pc_array_du, pc_array_dx, pc_array_dy, pc_array_dz, pc_array_ds, pc_array_ts,
                                 pc_array_pn, pc_array_op, pc_array_da, pc_array_vn, pc_array_vp,
                                 NULL };
        const gchar *vckey[] = { "du", "dx", "dy", "dz", "ds", "ts", "pn", "op", "da", "vn", "vp", NULL };

        for (int i=0; vckey[i] && pc_array[i]; ++i){
                gchar *m_vckey = g_strdup_printf ("%s-%s", vckey[i], key);

                g_print ("GVP store: %s = %s\n", m_vckey, g_variant_print (pc_array[i], true));

                if (g_variant_dict_contains (dict, m_vckey)){
                        if (!g_variant_dict_remove (dict, m_vckey)){
                                PI_DEBUG_GW (DBG_L1, "ERROR: SPM_Template_Control::GVP_store_vp -- key '%s' found, but removal failed.", m_vckey);
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

void SPM_Template_Control::GVP_restore_vp (const gchar *key){
	// g_message ("GVP-VP restore memo key=%s", key);
	PI_DEBUG_GP (DBG_L2, "GVP-VP restore memo %s\n", key);
	g_message ( "GVP-VP restore memo %s\n", key);
        GVariant *v = g_settings_get_value (hwi_settings, "probe-gvp-vector-program-matrix");
        //GVariant *v = g_settings_get_value (hwi_settings, "probe-lm-vector-program-matrix");
        GVariantDict *dict = g_variant_dict_new (v);
        if (!dict){
                PI_DEBUG_GP (DBG_L2, "ERROR: SPM_Template_Control::GVP_restore_vp -- can't read dictionary 'probe-gvp-vector-program-matrix' a{sv}.\n");
                return;
        }
        gsize  n; // == N_GVP_VECTORS;
        GVariant *vd[6];
        GVariant *vi[5];
        double *pc_array_d[6];
        gint32 *pc_array_i[5];
        const gchar *vckey_d[] = { "du", "dx", "dy", "dz", "ds", "ts", NULL };
        const gchar *vckey_i[] = { "pn", "op", "da", "vn", "vp", NULL };
        double *GVPd[] = { GVP_du, GVP_dx, GVP_dy, GVP_dz, GVP_dsig, GVP_ts, NULL };
        gint32 *GVPi[] = { GVP_points, GVP_opt, GVP_data, GVP_vnrep, GVP_vpcjr, NULL };
        gint32 vp_program_length=0;
        
        for (int i=0; vckey_i[i]; ++i){
                gchar *m_vckey = g_strdup_printf ("%s-%s", vckey_i[i], key);
                g_message ( "GVP-VP restore %s\n", m_vckey);

                for (int k=0; k<N_GVP_VECTORS; ++k) GVPi[i][k]=0; // zero init vector
                if ((vi[i] = g_variant_dict_lookup_value (dict, m_vckey, ((const GVariantType *) "ai"))) == NULL){
                        PI_DEBUG_GP (DBG_L2, "GXSM4 DCONF: SPM_Template_Control::GVP_restore_vp -- key_i '%s' memo not found. Setting to Zero.\n", m_vckey);
                        // g_warning ("GXSM4 DCONF: SPM_Template_Control::GVP_restore_vp -- key_i '%s' memo not found. Setting to Zero.\n", m_vckey);
                        g_free (m_vckey);
                        continue;
                }
                g_print ("GVP restore: %s = %s\n", m_vckey, g_variant_print (vi[i], true));

                pc_array_i[i] = (gint32*) g_variant_get_fixed_array (vi[i], &n, sizeof (gint32));
                if (i==0) // actual length of this vector should fit all others -- verify
                        vp_program_length=n;
                else
                        if (n != vp_program_length)
                                g_warning ("GXSM4 DCONF: SPM_Template_Control::GVP_restore_vp -- key_i '%s' vector length %ld not matching program n=%d.\n", m_vckey, n, vp_program_length);
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
                        PI_DEBUG_GP (DBG_L2, "GXSM4 DCONF: SPM_Template_Control::GVP_restore_vp -- key_d '%s' memo not found. Setting to Zero.", m_vckey);
                        // g_warning ("GXSM4 DCONF: SPM_Template_Control::GVP_restore_vp -- key_d '%s' memo not found. Setting to Zero.", m_vckey);
                        g_free (m_vckey);
                        continue;
                }
                g_print ("GVP restore: %s = %s\n", m_vckey, g_variant_print (vd[i], true));

                pc_array_d[i] = (double*) g_variant_get_fixed_array (vd[i], &n, sizeof (double));
                //g_assert_cmpint (n, ==, N_GVP_VECTORS);
                if (n != vp_program_length)
                        g_warning ("GXSM4 DCONF: SPM_Template_Control::GVP_restore_vp -- key_d '%s' vector length %ld not matching program n=%d.\n", m_vckey, n, vp_program_length);
                for (int k=0; k<vp_program_length && k<N_GVP_VECTORS; ++k)
                        GVPd[i][k]=pc_array_d[i][k];
                g_free (m_vckey);
                g_variant_unref (vd[i]);
        }                        
        
        g_variant_dict_unref (dict);
        g_variant_unref (v);

	update_GUI ();
}

int SPM_Template_Control::callback_edit_GVP (GtkWidget *widget, SPM_Template_Control *dspc){
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
			dspc->GVP_dsig[k] = dspc->GVP_dsig[ks];
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
			dspc->GVP_dsig[k] = dspc->GVP_dsig[ks];
			dspc->GVP_ts[k]  = dspc->GVP_ts[ks];
			dspc->GVP_points[k] = dspc->GVP_points[ks];
			dspc->GVP_opt[k] = dspc->GVP_opt[ks];
			dspc->GVP_vnrep[k] = dspc->GVP_vnrep[ks];
			dspc->GVP_vpcjr[k] = dspc->GVP_vpcjr[ks];
		}
	dspc->update_GUI ();
        return 0;
}


// helper func
NcVar* spm_template_hwi_ncaddvar (NcFile *ncf, const gchar *varname, const gchar *varunit, const gchar *longname, const gchar *shortname, double value){
	NcVar* ncv = ncf->add_var (varname, ncDouble);
	ncv->add_att ("long_name", longname);
	ncv->add_att ("short_name", shortname);
	ncv->add_att ("var_unit", varunit);
	ncv->put (&value);
	return ncv;
}
NcVar* spm_template_hwi_ncaddvar (NcFile *ncf, const gchar *varname, const gchar *varunit, const gchar *longname, const gchar *shortname, int value){
	NcVar* ncv = ncf->add_var (varname, ncInt);
	ncv->add_att ("long_name", longname);
	ncv->add_att ("short_name", shortname);
	ncv->add_att ("var_unit", varunit);
	ncv->put (&value);
	return ncv;
}

#define SPMTMPL_ID "spm_template_hwi_"

void SPM_Template_Control::save_values (NcFile *ncf){
	NcVar *ncv;

	PI_DEBUG (DBG_L4, "SPM_Template_Control::save_values");
	gchar *i=NULL;

        i = g_strconcat ("SPM Template HwI ** Hardware-Info:\n", spm_template_hwi->get_info (), NULL);

	NcDim* infod  = ncf->add_dim("sranger_info_dim", strlen(i));
	NcVar* info   = ncf->add_var("sranger_info", ncChar, infod);
	info->add_att("long_name", "SPM_Template_Control plugin information");
	info->put(i, strlen(i));
	g_free (i);

// Basic Feedback/Scan Parameter ============================================================

	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"bias", "V", "SRanger: (Sampel or Tip) Bias Voltage", "Bias", bias);
	ncv->add_att ("label", "Bias");
	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"z_setpoint", "A", "SRanger: auxillary/Z setpoint", "Z Set Point", zpos_ref);
	ncv->add_att ("label", "Z Setpoint");

	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix0_set_point", "nA", "SRanger: Mix0: Current set point", "Current Setpt.", mix_set_point[0]);
	ncv->add_att ("label", "Current");
	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix1_set_point", "Hz", "SRanger: Mix1: Voltage set point", "Voltage Setpt.", mix_set_point[1]);
	ncv->add_att ("label", "VoltSetpt.");
	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix2_set_point", "V", "SRanger: Mix2: Aux2 set point", "Aux2 Setpt.", mix_set_point[2]);
	ncv->add_att ("label", "Aux2 Setpt.");
	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix3_set_point", "V", "SRanger: Mix3: Aux3 set point", "Aux3 Setpt.", mix_set_point[3]);
	ncv->add_att ("label", "Aux3 Setpt.");

	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix0_mix_gain", "1", "SRanger: Mix0 gain", "Current gain", mix_gain[0]);
	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix1_mix_gain", "1", "SRanger: Mix1 gain", "Voltage gain", mix_gain[1]);
	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix2_mix_gain", "1", "SRanger: Mix2 gain", "Aux2 gain", mix_gain[2]);
	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix3_mix_gain", "1", "SRanger: Mix3 gain", "Aux3 gain", mix_gain[3]);

	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix0_mix_level", "1", "SRanger: Mix0 level", "Current level", mix_level[0]);
	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix1_mix_level", "1", "SRanger: Mix1 level", "Voltage level", mix_level[1]);
	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix2_mix_level", "1", "SRanger: Mix2 level", "Aux2 level", mix_level[2]);
	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix3_mix_level", "1", "SRanger: Mix3 level", "Aux3 level", mix_level[3]);

	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix0_current_mix_transform_mode", "BC", "SRanger: Mix0 transform_mode", "Current transform_mode", (double)mix_transform_mode[0]);
	ncv->add_att ("mode_bcoding", "0:Off, 1:On, 2:Log, 4:IIR, 8:FUZZY");
	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix1_voltage_mix_transform_mode", "BC", "SRanger: Mix1 transform_mode", "Voltage transform_mode", (double)mix_transform_mode[1]);
	ncv->add_att ("mode_bcoding", "0:Off, 1:On, 2:Log, 4:IIR, 8:FUZZY");
	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix2_aux2_mix_transform_mode", "BC", "SRanger: Mix2 transform_mode", "Aux2 transform_mode", (double)mix_transform_mode[2]);
	ncv->add_att ("mode_bcoding", "0:Off, 1:On, 2:Log, 4:IIR, 8:FUZZY");
	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"mix3_aux3_mix_transform_mode", "BC", "SRanger: Mix3 transform_mode", "Aux3 transform_mode", (double)mix_transform_mode[3]);
	ncv->add_att ("mode_bcoding", "0:Off, 1:On, 2:Log, 4:IIR, 8:FUZZY");


	ncv=spm_template_hwi_ncaddvar (ncf, SPMTMPL_ID"move_speed_x", "A/s", "SRanger: Move speed X", "Xm Velocity", move_speed_x);
	ncv->add_att ("label", "Velocity Xm");



// Vector Probe ============================================================

}


#define NC_GET_VARIABLE(VNAME, VAR) if(ncf->get_var(VNAME)) ncf->get_var(VNAME)->get(VAR)

void SPM_Template_Control::load_values (NcFile *ncf){
	PI_DEBUG (DBG_L4, "SPM_Template_Control::load_values");
	// Values will also be written in old style DSP Control window for the reason of backwards compatibility
	// OK -- but will be obsoleted and removed at any later point -- PZ
	NC_GET_VARIABLE ("spm_template_hwi_bias", &bias);
	NC_GET_VARIABLE ("spm_template_hwi_bias", &main_get_gapp()->xsm->data.s.Bias);
        NC_GET_VARIABLE ("spm_template_hwi_set_point1", &mix_set_point[1]);
        NC_GET_VARIABLE ("spm_template_hwi_set_point0", &mix_set_point[0]);

	update_GUI ();
}







void SPM_Template_Control::get_tab_settings (const gchar *tab_key, guint64 &option_flags, guint64 &auto_flags, guint64 glock_data[6]) {
        PI_DEBUG_GP (DBG_L4, "SPM_Template_Control::get_tab_settings for tab '%s'\n", tab_key);

        gint dbg_level = 0;

#define PI_DEBUG_GPX(X, ARGS...) g_print (ARGS);
        
        GVariant *v = g_settings_get_value (hwi_settings, "probe-tab-options");

        // PI_DEBUG_GPX (dbg_level, "probe-tab-options type is '%s'\n", g_variant_get_type (v));
        //        PI_DEBUG_GPX (dbg_level, "probe-tab-options=%s\n", g_variant_print (v,true));
        
        GVariantDict *dict = g_variant_dict_new (v);

        if (!dict){
                PI_DEBUG_GPX (dbg_level, "ERROR: SPM_Template_Control::get_tab_settings -- can't read dictionary 'probe-tab-options' a{sv}.\n");
                return;
        }
#if 0
        if (!g_variant_dict_contains (dict, tab_key)){
                PI_DEBUG_GPX (dbg_level, "ERROR: SPM_Template_Control::get_tab_settings -- can't find key '%s' in dict 'probe-tab-options'.\n", tab_key);
        }
        PI_DEBUG_GPX (dbg_level, "SPM_Template_Control::get_tab_settings -- key '%s' found in dict 'probe-tab-options' :)\n", tab_key);
#endif
        
        GVariant *value = g_variant_dict_lookup_value (dict, tab_key, ((const GVariantType *) "at")); // array uint64
        if (!value){
                g_warning ("WARNING (Normal at first start) -- Note: only at FIRST START: Building Settings. SPM_Template_Control::get_tab_settings:\n --> can't get find array 'at' data for key '%s' in 'probe-tab-options' dictionary.\n Storing default now.", tab_key);
                g_print ("WARNING (Normal at first start) -- Note: only at FIRST START: Building Settings. SPM_Template_Control::get_tab_settings:\n --> can't get find array 'at' data for key '%s' in 'probe-tab-options' dictionary.\n Storing default now.", tab_key);

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
                PI_DEBUG_GPX (dbg_level, "SPM_Template_Control::get_tab_settings -- reading '%s' tab options [", tab_key);
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
                PI_DEBUG_GPX (dbg_level, "ERROR: SPM_Template_Control::get_tab_settings -- cant get array 'at' from key '%s' in 'probe-tab-options' dictionary. And default rewrite failed also.\n", tab_key);
        }

        GVariant *vto = g_variant_dict_end (dict);

        // testing storing
#if 0
        if (vto){
                PI_DEBUG_GPX (dbg_level, "SPM_Template_Control::get_tab_settings -- stored tab-setings dict to variant OK.\n");
        } else {
                PI_DEBUG_GPX (dbg_level, "ERROR SPM_Template_Control::get_tab_settings -- store tab-setings dict to variant failed.\n");
        }
        if (g_settings_is_writable (hwi_settings, "probe-tab-options")){
                PI_DEBUG_GPX (dbg_level, "SPM_Template_Control::get_tab_settings -- is writable\n");
        } else {
                PI_DEBUG_GPX (dbg_level, "WARNING/ERROR SPM_Template_Control::get_tab_settings -- is NOT writable\n");
        }
#endif
        if (!g_settings_set_value (hwi_settings, "probe-tab-options", vto)){
                PI_DEBUG_GPX (dbg_level, "ERROR SPM_Template_Control::get_tab_settings -- update probe-tab-options dict failed.\n");
        }
        g_variant_dict_unref (dict);
        //        g_variant_unref (vto);
        // PI_DEBUG_GPX (dbg_level, "SPM_Template_Control::get_tab_settings -- done.\n");
}

//                PI_DEBUG (DBG_L1, "key '" << tab_key << "' not found in probe-tab-options.");


void SPM_Template_Control::set_tab_settings (const gchar *tab_key, guint64 option_flags, guint64 auto_flags, guint64 glock_data[6]) {
        GVariant *v = g_settings_get_value (hwi_settings, "probe-tab-options");

        gint dbg_level = 0;

#define PI_DEBUG_GPX(X, ARGS...) g_print (ARGS);

        // PI_DEBUG_GPX (dbg_level, "probe-tab-options type is '%s'\n", g_variant_get_type (v));
        //        PI_DEBUG_GPX (dbg_level, "probe-tab-options old=%s\n", g_variant_print (v,true));

        GVariantDict *dict = g_variant_dict_new (v);

        if (!dict){
                PI_DEBUG_GPX (dbg_level, "ERROR: SPM_Template_Control::set_tab_settings -- can't read dictionary 'probe-tab-options' a{sai}.\n");
                return;
        }
#if 0
        if (!g_variant_dict_contains (dict, tab_key)){
                PI_DEBUG_GPX (dbg_level, "ERROR: SPM_Template_Control::set_tab_settings -- can't find key '%s' in dict 'probe-tab-options'.\n", tab_key);
                return;
        }
        PI_DEBUG_GPX (dbg_level, "SPM_Template_Control::set_tab_settings -- key '%s' found in dict 'probe-tab-options' :)\n", tab_key);
#endif

        GVariant *value = g_variant_dict_lookup_value (dict, tab_key, ((const GVariantType *) "at"));
        if (!value){
                PI_DEBUG_GPX (dbg_level, "ERROR: SPM_Template_Control::set_tab_settings -- can't find array 'at' data from key '%s' in 'probe-tab-options' dictionary for update.\n", tab_key);
                g_variant_dict_unref (dict);
                return;
        }
        if (value){
                gsize    ni = 9;
                guint64 *ai = g_new (guint64, ni);

                //                PI_DEBUG_GPX (dbg_level, "SPM_Template_Control::set_tab_settings ** key '%s' old v=%s\n", tab_key, g_variant_print (value, true));
#if 0
                // ---- debug read
                guint64  *aix = NULL;
                gsize     nix;
                
                aix = (guint64*) g_variant_get_fixed_array (value, &nix, sizeof (guint64));
                g_assert_cmpint (nix, ==, 9);

                PI_DEBUG_GPX (dbg_level, "SPM_Template_Control::set_tab_settings ** key '%s' old x=[", tab_key);
                for (int i=0; i<nix; ++i)
                        PI_DEBUG_GPX (DBG_L4, "0x%08llx, ", aix[i]);
                PI_DEBUG_GPX (dbg_level, "] hex\n");
                PI_DEBUG_GPX (dbg_level, "SPM_Template_Control::set_tab_settings ** key '%s' old x=[", tab_key);
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
                PI_DEBUG_GPX (dbg_level, "SPM_Template_Control::set_tab_settings ** key '%s' new x=[", tab_key);
                for (int i=0; i<ni; ++i)
                        PI_DEBUG_GPX (DBG_L4, "0x%08llx, ", ai[i]);
                PI_DEBUG_GPX (dbg_level, "] hex\n");
#endif
                
                g_variant_unref (value); // free old variant
                value = g_variant_new_fixed_array (g_variant_type_new ("t"), ai, ni, sizeof (guint64));

                // PI_DEBUG_GPX (dbg_level, "SPM_Template_Control::set_tab_settings ** key '%s' new v=%s\n", tab_key, g_variant_print (value, true));

                // remove old
                if (!g_variant_dict_remove (dict, tab_key)){
                        PI_DEBUG_GPX (dbg_level, "ERROR: SPM_Template_Control::set_tab_settings -- cant find and remove key '%s' from 'probe-tab-options' dictionary.\n", tab_key);
                }
                // insert new
                // PI_DEBUG_GPX (dbg_level, "SPM_Template_Control::set_tab_settings -- inserting new updated key '%s' into 'probe-tab-options' dictionary.\n", tab_key);
                g_variant_dict_insert_value (dict, tab_key, value);

                g_free (ai); // free array
                // must free data for ourselves
        } else {
                PI_DEBUG_GPX (dbg_level, "ERROR: SPM_Template_Control::set_tab_settings -- cant get array 'au' from key '%s' in 'probe-tab-options' dictionary.\n", tab_key);
        }

        GVariant *vto = g_variant_dict_end (dict);

        // PI_DEBUG_GPX (dbg_level, "probe-tab-options new=%s\n", g_variant_print (vto, true));

        // testing storing
        if (!vto){
                PI_DEBUG_GPX (dbg_level, "ERROR SPM_Template_Control::set_tab_settings -- store tab-setings dict to variant failed.\n");
        }
        if (!g_settings_set_value (hwi_settings, "probe-tab-options", vto)){
                PI_DEBUG_GPX (dbg_level, "ERROR SPM_Template_Control::set_tab_settings -- update probe-tab-options dict failed.\n");
        }
        g_variant_dict_unref (dict);
        //        g_variant_unref (vto);

        //        PI_DEBUG_GPX (dbg_level, "SPM_Template_Control::set_tab_settings -- done.\n");
        return;
}




void SPM_Template_Control::AppWindowInit(const gchar *title){
        if (title) { // stage 1
                PI_DEBUG (DBG_L2, "SPM-Template-Control::AppWindowInit -- header bar");

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
                PI_DEBUG (DBG_L2, "SPM_Template_Control::AppWindowInit -- header bar -- stage two, hook configure menu");

                win_SPM_Template_popup_entries[0].state = g_settings_get_boolean (hwi_settings, "configure-mode") ? "true":"false"; // get last state
                g_action_map_add_action_entries (G_ACTION_MAP (app_window),
                                                 win_SPM_Template_popup_entries, G_N_ELEMENTS (win_SPM_Template_popup_entries),
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

int SPM_Template_Control::choice_Ampl_callback (GtkWidget *widget, SPM_Template_Control *spmsc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	gint i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	gint j = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "chindex"));
	switch(j){
	case 0: spm_template_hwi_pi.app->xsm->Inst->VX(i);
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_DSP_OFFSET_ADDING)
			spm_template_hwi_pi.app->xsm->Inst->VX0(i);
		break;
	case 1: spm_template_hwi_pi.app->xsm->Inst->VY(i);
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_DSP_OFFSET_ADDING)
			spm_template_hwi_pi.app->xsm->Inst->VY0(i);
		break;
	case 2: spm_template_hwi_pi.app->xsm->Inst->VZ(i); 
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_DSP_OFFSET_ADDING)
			spm_template_hwi_pi.app->xsm->Inst->VZ0(i); 
		break;
	case 3:
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING)
			spm_template_hwi_pi.app->xsm->Inst->VX0(i);
		break;
	case 4:
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING)
			spm_template_hwi_pi.app->xsm->Inst->VY0(i);
		break;
	case 5:
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING)
			spm_template_hwi_pi.app->xsm->Inst->VZ0(i);
		break;
	}

	PI_DEBUG (DBG_L2, "Ampl: Ch=" << j << " i=" << i );
	spm_template_hwi_pi.app->spm_range_check(NULL, spm_template_hwi_pi.app);
        
	return 0;
}

int SPM_Template_Control::config_options_callback (GtkWidget *widget, SPM_Template_Control *ssc){
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                ssc->options |= GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget),"Bit_Mask"));
        else
                ssc->options &= ~GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget),"Bit_Mask"));
        
        g_message ("SPM_Template_Control::config_options_callback -> %04x", ssc->options);
        return 0;
}


void SPM_Template_Control::create_folder (){
        GtkWidget *notebook;
 	GSList *zpos_control_list=NULL;

        AppWindowInit ("SPM Tempate Control Window");
        
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


// ==== Folder: Feedback & Scan ========================================
        bp->start_notebook_tab (notebook, "Feedback & Scan", "template-tab-feedback-scan", hwi_settings);

	bp->new_grid_with_frame ("SPM Controls");
        
        // ------- FB Characteristics
        bp->start (); // start on grid top and use widget grid attach nx=4
        bp->set_scale_nx (4); // set scale width to 4
        bp->set_input_width_chars (10);

        bp->grid_add_ec_with_scale ("Bias", Volt, &bias, -10., 10., "4g", 0.001, 0.01, "fbs-bias");
        //        bp->ec->set_adjustment_mode (PARAM_CONTROL_ADJUSTMENT_LOG | PARAM_CONTROL_ADJUSTMENT_LOG_SYM | PARAM_CONTROL_ADJUSTMENT_DUAL_RANGE | PARAM_CONTROL_ADJUSTMENT_ADD_MARKS );
        bp->ec->SetScaleWidget (bp->scale, 0);
        bp->ec->set_logscale_min (1e-3);
        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);
        bp->new_line ();

        bp->set_input_width_chars (30);
        bp->set_input_nx (2);
        bp->grid_add_ec ("Z-Pos/Setpoint:", Angstroem, &zpos_ref, -100., 100., "6g", 0.01, 0.1, "adv-dsp-zpos-ref");
        ZPos_ec = bp->ec;
        zpos_control_list = g_slist_prepend (zpos_control_list, bp->ec);
                 
        bp->set_input_width_chars (12);
        bp->set_input_nx ();

	bp->grid_add_check_button ("Z-Pos Monitor",
                                   "Z Position Monitor. Disable to set Z-Position Setpoint for const height mode. "
                                   "Switch Transfer to CZ-FUZZY-LOG for Z-CONTROL Constant Heigth Mode Operation with current compliance given by Fuzzy-Level. "
                                   "Slow down feedback to minimize instabilities. "
                                   "Set Current setpoint a notch below compliance level.",
                                   1,
                                   G_CALLBACK (SPM_Template_Control::zpos_monitor_callback), this,
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
	bp->grid_add_ec_with_scale ("CP", Unity, &z_servo[SERVO_CP], 0., 200., "5g", 1.0, 0.1, "fbs-cp");
        GtkWidget *ZServoCP = bp->input;
        bp->new_line ();
        bp->set_configure_list_mode_off ();
        bp->grid_add_ec_with_scale ("CI", Unity, &z_servo[SERVO_CI], 0., 200., "5g", 1.0, 0.1, "fbs-ci");
        GtkWidget *ZServoCI = bp->input;

        g_object_set_data( G_OBJECT (ZServoCI), "HasClient", ZServoCP);
        g_object_set_data( G_OBJECT (ZServoCP), "HasMaster", ZServoCI);
        g_object_set_data( G_OBJECT (ZServoCI), "HasRatio", GUINT_TO_POINTER((guint)round(1000.*z_servo[SERVO_CP]/z_servo[SERVO_CI])));
        
        bp->set_label_width_chars ();

	// ========================================
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
                                       G_CALLBACK(SPM_Template_Control::DSP_slope_callback), this, area_slope_compensation_flag, 0);
        g_settings_bind (hwi_settings, "adv-enable-slope-compensation",
                         G_OBJECT (GTK_CHECK_BUTTON (bp->button)), "active",
                         G_SETTINGS_BIND_DEFAULT);

        bp->grid_add_check_button ("Enable automatic Tip return to center", "enable auto tip return to center after scan", 1,
                                       G_CALLBACK(SPM_Template_Control::DSP_cret_callback), this, center_return_flag, 0);
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
			    G_CALLBACK (SPM_Template_Control::ldc_callback), this);


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
                                  G_CALLBACK (SPM_Template_Control::choice_Ampl_callback),
                                  this);

                gchar *tmpkey = g_strconcat ("spm-template-h", PDR_gain_key[j], NULL); 
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
        bp->start_notebook_tab (notebook, "Lock-In", "template-tab-lockin", hwi_settings);

 	bp->new_grid_with_frame ("Digital Lock-In settings");
        bp->set_default_ec_change_notice_fkt (SPM_Template_Control::lockin_adjust_callback, this);

        bp->new_line ();

#define GET_CURRENT_LOCKING_MODE 0 // dummy
        
        LockIn_mode = bp->grid_add_check_button ("LockIn run free", "enable contineous modulation and LockIn processing.\n",
                                                     1,
                                                     GCallback (SPM_Template_Control::lockin_runfree_callback), this,
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
        bp->start_notebook_tab (notebook, "STS", "template-tab-sts", hwi_settings);
	// ==================================================
        bp->new_grid_with_frame ("I-V Type Spectroscopy");
	// ==================================================

	bp->grid_add_label ("IV Probe"); bp->grid_add_label ("Start"); bp->grid_add_label ("End");  bp->grid_add_label ("Points");
        bp->new_line ();

        bp->grid_add_ec (NULL, Volt, &IV_start, -10.0, 10., "5.3g", 0.1, 0.025, "IV-Start");
        bp->grid_add_ec (NULL, Volt, &IV_end, -10.0, 10.0, "5.3g", 0.1, 0.025, "IV-End");
        bp->grid_add_ec (NULL, Unity, &IV_points, 1, 1000, "5g", "IV-Points");
        bp->set_pcs_remote_prefix (REMOTE_PREFIX);
        
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
                                         GCallback (SPM_Template_Control::Probing_exec_IV_callback),
                                         GCallback (SPM_Template_Control::Probing_write_IV_callback),
                                         GCallback (SPM_Template_Control::Probing_graph_callback),
                                         GCallback (SPM_Template_Control::Probing_abort_callback),
                                         this,
                                         "IV");
        bp->notebook_tab_show_all ();
        bp->pop_grid ();


        
        bp->new_grid ();
        bp->start_notebook_tab (notebook, "GVP", "template-tab-gvp", hwi_settings);

	// ========================================
        PI_DEBUG (DBG_L4, "DSPC----TAB-VP ------------------------------- ");

 	bp->new_grid_with_frame ("Generic Vector Program (VP) Probe and Manipulation");
 	// g_print ("================== TAB 'GVP' ============= Generic Vector Program (VP) Probe and Manipulation\n");

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
        bp->grid_add_label ("VP-dU", "vec-du");
        bp->grid_add_label ("VP-dX", "vec-dx (default or mapped)");
        bp->grid_add_label ("VP-dY", "vec-dy (default or mapped)");
        bp->grid_add_label ("VP-dZ", "vec-dz");
        bp->set_configure_list_mode_on ();
        bp->grid_add_label ("VP-dSig", "vec-dSignal mapped");
        bp->set_configure_list_mode_off ();
        bp->grid_add_label ("time", "total time for VP section");
        bp->grid_add_label ("points", "points (# vectors to add)");
        bp->grid_add_label ("FB", "Feedback");
        bp->set_configure_list_mode_on ();
        bp->grid_add_label ("IOS", "GPIO-Set");
        bp->grid_add_label ("IOR", "GPIO-Read");
        bp->grid_add_label ("TP", "TRIGGER-POS");
        bp->grid_add_label ("TN", "TRIGGER-NEG");
        bp->grid_add_label ("GPIO", "data for GPIO / mask for trigger on GPIO");
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
		bp->grid_add_ec (NULL,    Volt, &GVP_dsig[k], -1000.0, 1000.0, "6.4g",1., 10., "gvp-dsig", k); 
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
                                               GVP_opt[k], VP_GPIO_SET);
                EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, bp->button);
                g_object_set_data (G_OBJECT (bp->button), "VPC", GINT_TO_POINTER (k));

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

	bp->new_grid_with_frame ("VP Finish Settings and Status");
        bp->set_configure_list_mode_on ();
	bp->grid_add_ec ("GVP-Final-Delay", Time, &GVP_final_delay, 0., 1., "5.3g", 0.001, 0.01, "GVP-Final-Delay");
	bp->grid_add_label ("GPIO key", "Key code to enable GPIO set operations.\n(GPIO manipulation is locked out if not set right.)");
	bp->grid_add_ec (GVP_GPIO_KEYCODE_S, Unity, &GVP_GPIO_lock, 0, 9999, "04.0f", "GVP-GPIO-Lock-" GVP_GPIO_KEYCODE_S);
        bp->new_line ();
        bp->set_configure_list_mode_off ();

	GVP_status = bp->grid_add_probe_status ("Status");

        bp->pop_grid ();
        bp->new_line ();

        bp->grid_add_probe_controls (FALSE,
                                         GVP_option_flags, GCallback (callback_change_GVP_option_flags),
                                         GVP_auto_flags,  GCallback (callback_change_GVP_auto_flags),
                                         GCallback (SPM_Template_Control::Probing_exec_GVP_callback),
                                         GCallback (SPM_Template_Control::Probing_write_GVP_callback),
                                         GCallback (SPM_Template_Control::Probing_graph_callback),
                                         GCallback (SPM_Template_Control::Probing_abort_callback),
                                         this,
                                         "VP");
        bp->notebook_tab_show_all ();
        bp->pop_grid ();



// ==== Folder: Graphs -- Vector Probe Data Visualisation setup ========================================
        bp->new_grid ();
        bp->start_notebook_tab (notebook, "Graphs", "template-tab-graphs", hwi_settings);

        PI_DEBUG (DBG_L4, "DSPC----TAB-GRAPHS ------------------------------- ");

 	bp->new_grid_with_frame ("Probe Sources & Graph Setup");

	// source channel setup -- just and example or template -- that's a reduced set of the MK3
	int   msklookup[] = { 0x000010, 0x000020, 0x000040, 0x000080, 0x000100, 0x000200, 0x000400, 0x000800,
			      0x000008, 0x001000, 0x002000, 0x004000, 0x008000,
			      0x0100000, 0x0200000, 0x0400000, 0x0800000, 0x1000000,
			      -1 
	};
	const char* lablookup[] = { "ADC0-I", "ADC1", "ADC2", "ADC3", "ADC4", "ADC5","ADC6","ADC7",
				    "LockIn0", "LockIn X",  "LockIn Y", "LockIn Mag", "Auxillary",
				    "Time", "Z-mon", "Bias-mon", "SEC",
				    NULL
	};
        
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

#if 0 // if need more
        bp->grid_add_label ("Source", "Check column to activate channel", 2, 0.);
        bp->grid_add_label ("X", "Check column to plot channel on X axis.", 1);
        bp->grid_add_label ("Y", "Check column to plot channel on Y axis.", 1);
        bp->grid_add_label ("Avg", "Check column to plot average of all spectra", 1);
        bp->grid_add_label ("Sec", "Check column to show all sections.", 1);
#endif
        bp->new_line ();

        PI_DEBUG (DBG_L4, "DSPC----TAB-GRAPHS TOGGELS  ------------------------------- ");

        gint y = bp->y;
	for (int i=0; msklookup[i] >= 0 && lablookup[i]; ++i) {
                PI_DEBUG (DBG_L4, "GRAPHS*** i=" << i << " " << lablookup[i]);
		if (!msklookup[i]) 
			continue;
		int c=i/8; 
		c*=11;
                c++;
		int m = -1;
		if (i >= 9 && i < 13) // flex sources 9..12
			m = i-8;
		else
			m = -1;
                int r = y+i%8+1;

                bp->set_xy (c, r);

                // Source
                bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (change_source_callback), this,
                                               Source, (((int) msklookup[i]) & 0xfffffff)
                                               );
                // source selection for i=9..12:
                if (m >= 0){ // flex source
                        bp->grid_add_probe_source_signal_options (m, probe_source[m], this);
                }else { // fixed assignment
                        bp->grid_add_label (lablookup[i], NULL, 1, 0.);
                }
                //fixed assignment:
                // bp->grid_add_label (lablookup[i], NULL, 1, 0.);
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER ((int) msklookup[i])); 
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // use as X-Source
                bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (change_source_callback), this,
                                               XSource, (((int) (X_SOURCE_MSK | msklookup[i])) & 0xfffffff)
                                               );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER ((int) (X_SOURCE_MSK | msklookup[i]))); 
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 

                // use as Plot (Y)-Source
                bp->grid_add_check_button ("", NULL, 1,
                                               G_CALLBACK (change_source_callback), this,
                                               PSource, (((int) (P_SOURCE_MSK | msklookup[i])) & 0xfffffff)
                                               );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER ((int) (P_SOURCE_MSK | msklookup[i]))); 
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // use as A-Source (Average)
                bp->grid_add_check_button ("", NULL, 1,
                                               G_CALLBACK (change_source_callback), this,
                                               PlotAvg, (((int) (A_SOURCE_MSK | msklookup[i])) & 0xfffffff)
                                               );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER ((int) (A_SOURCE_MSK | msklookup[i]))); 
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // use as S-Source (Section)
                bp->grid_add_check_button ("", NULL, 1,
                                               G_CALLBACK (change_source_callback), this,
                                               PlotSec, (((int) (S_SOURCE_MSK | msklookup[i])) & 0xfffffff)
                                               );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER ((int) (S_SOURCE_MSK | msklookup[i]))); 
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
                          G_CALLBACK (SPM_Template_Control::Probing_graph_callback), this);
	
        save_button = bp->grid_add_button ("Save");
	g_signal_connect (G_OBJECT (bp->button), "clicked",
                          G_CALLBACK (SPM_Template_Control::Probing_save_callback), this);

        bp->notebook_tab_show_all ();
        bp->pop_grid ();

        
        // ========================================
        // Simulator Only

        const char *TabNames[] = { "sim-STM", "sim-AFM", "sim-Config", NULL};
        const char *tab_key[] = { "template-tab-stm", "template-tab-afm", "template-tab-config", NULL};
        const char *tab_key_prefix[] = { "template-stm-", "template-afm-", "template-config-", NULL};

        for (int i=0; TabNames[i]; ++i){                
                Gtk_EntryControl *ec_axis[3];

                bp->start_notebook_tab (notebook, TabNames[i], tab_key[i], hwi_settings);
                        
                bp->set_pcs_remote_prefix (tab_key_prefix[i]);

                if (i<2){
                        bp->new_grid_with_frame ("Sim Parameters");
                        bp->set_input_width_chars(20);
                        bp->set_default_ec_change_notice_fkt (SPM_Template_Control::ChangedNotify, this);

                        bp->grid_add_ec_with_scale ("Speed", Velocity, &sim_speed[i], 0.01, 150e3, "5.2f", 1., 10., "speed");
                        g_object_set_data( G_OBJECT (bp->input), "TabIndex", GINT_TO_POINTER (i));
                        
                        bp->new_line ();

                        bp->set_configure_list_mode_on ();
                        bp->grid_add_ec_with_scale ("Bias", Volt, &sim_bias[i], -10., 10., "5.2f", 0.1, 1., "bias");
                        g_object_set_data( G_OBJECT (bp->input), "TabIndex", GINT_TO_POINTER (i));
                        bp->set_configure_list_mode_off ();

                        bp->new_line ();
                        // ...
                }                        

                if (i==2){ // Config Tab
                        // CheckButton via:
                        // GtkWidget* grid_add_check_button (const gchar* labeltxt, const char *tooltip=NULL, int bwx=1,
                        //                   GCallback cb=NULL, gpointer data=NULL, gint source=0, gint mask=-1){

                        
                        bp->grid_add_check_button ("Molecules",
                                               "Check to enable Molecules.", 1,
                                               GCallback (config_options_callback), this, 1, 0x01);
                        bp->new_line ();
                        bp->grid_add_check_button ("Blobs",
                                               "Check to enable Blobs.", 1,
                                               GCallback (config_options_callback), this, 1, 0x02);
                        bp->new_line ();
                        bp->grid_add_check_button ("Reference Scan Landscape in CH11",
                                               "Load a (large) reference image / scan int CH11.", 1,
                                               GCallback (config_options_callback), this, 1, 0x04);

                        bp->new_line ();


                        // ...
                }

                bp->notebook_tab_show_all ();
	}
  
	// ============================================================
	// save List away...
        //g_object_set_data( G_OBJECT (window), "SPM_SIM_EC_list", bp->get_ec_list_head ());
        spm_template_hwi_pi.app->RemoteEntryList = g_slist_concat (spm_template_hwi_pi.app->RemoteEntryList, bp->get_remote_list_head ());
        configure_callback (NULL, NULL, this); // configure "false"

	g_object_set_data( G_OBJECT (window), "DSP_EC_list", bp->get_ec_list_head ());
	g_object_set_data( G_OBJECT (zposmon_checkbutton), "DSP_zpos_control_list", zpos_control_list);
        
	GUI_ready = TRUE;
        
        AppWindowInit (NULL); // stage two
        set_window_geometry ("spm-template-control"); // must add key to xml file: Gxsm-3.0/gxsm4/org.gnome.gxsm4.window-geometry.gschema.xml
}

int SPM_Template_Control::DSP_slope_callback (GtkWidget *widget, SPM_Template_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->area_slope_compensation_flag = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));	
	dspc->update_controller();
        return 0;
}

int SPM_Template_Control::DSP_cret_callback (GtkWidget *widget, SPM_Template_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->center_return_flag = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));	
        return 0;
}


int SPM_Template_Control::ldc_callback(GtkWidget *widget, SPM_Template_Control *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	dspc->update_controller();
	return 0;
}


int SPM_Template_Control::ChangedAction(GtkWidget *widget, SPM_Template_Control *dspc){
	dspc->update_controller ();
	return 0;
}

void SPM_Template_Control::ChangedNotify(Param_Control* pcs, gpointer dspc){
	((SPM_Template_Control*)dspc)->update_controller (); // update basic SPM Control Parameters
}

int SPM_Template_Control::choice_mixmode_callback (GtkWidget *widget, SPM_Template_Control *dspc){
	gint channel=0;
        gint selection=0;

        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

        channel = GPOINTER_TO_INT (g_object_get_data(G_OBJECT (widget), "mix_channel"));
        selection = atoi (gtk_combo_box_get_active_id (GTK_COMBO_BOX (widget)));

        //g_print ("Choice MIX%d MT=%d\n", channel, selection);

	PI_DEBUG_GP (DBG_L3, "MixMode[%d]=0x%x\n",channel,selection);

	dspc->mix_transform_mode[channel] = selection;

        PI_DEBUG_GP (DBG_L4, "%s ** 2\n",__FUNCTION__);

	dspc->update_controller ();
        //g_print ("DSP READBACK MIX%d MT=%d\n", channel, (int)sranger_common_hwi->read_dsp_feedback ("MT", channel));

        PI_DEBUG_GP (DBG_L4, "%s **3 done\n",__FUNCTION__);
        return 0;
}

int SPM_Template_Control::choice_scansource_callback (GtkWidget *widget, SPM_Template_Control *dspc){
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

void SPM_Template_Control::update_GUI(){
        if (!GUI_ready) return;

	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "DSP_EC_list"),
			(GFunc) App::update_ec, NULL);
	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "DSP_VPC_OPTIONS_list"),
			(GFunc) callback_update_GVP_vpc_option_checkbox, this);
}

void SPM_Template_Control::update_zpos_readings(){
        double zp,a,b;
        main_get_gapp()->xsm->hardware->RTQuery ("z", zp, a, b);
        gchar *info = g_strdup_printf (" (%g Ang)", main_get_gapp()->xsm->Inst->V2ZAng(zp));
        ZPos_ec->set_info (info);
        ZPos_ec->Put_Value ();
        g_free (info);
}

guint SPM_Template_Control::refresh_zpos_readings(SPM_Template_Control *dspc){ 
	if (main_get_gapp()->xsm->hardware->IsSuspendWatches ())
		return TRUE;

	dspc->update_zpos_readings ();
	return TRUE;
}

int SPM_Template_Control::zpos_monitor_callback( GtkWidget *widget, SPM_Template_Control *dspc){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))){
                //g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (widget), "DSP_zpos_control_list"), (GFunc) App::thaw_ec, NULL);
                dspc->zpos_refresh_timer_id = g_timeout_add (200, (GSourceFunc)SPM_Template_Control::refresh_zpos_readings, dspc);
        }else{
                //g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (widget), "DSP_zpos_control_list"), (GFunc) App::freeze_ec, NULL);

                if (dspc->zpos_refresh_timer_id){
                        g_source_remove (dspc->zpos_refresh_timer_id);
                        dspc->zpos_refresh_timer_id = 0;
                }
        }
        return 0;
}


int SPM_Template_Control::choice_prbsource_callback (GtkWidget *widget, SPM_Template_Control *dspc){
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



int SPM_Template_Control::callback_change_IV_option_flags (GtkWidget *widget, SPM_Template_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->IV_option_flags = (dspc->IV_option_flags & (~msk)) | msk;
	else
		dspc->IV_option_flags &= ~msk;

        dspc->set_tab_settings ("IV", dspc->IV_option_flags, dspc->IV_auto_flags, dspc->IV_glock_data);
        return 0;
}

int SPM_Template_Control::callback_change_IV_auto_flags (GtkWidget *widget, SPM_Template_Control *dspc){
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


int SPM_Template_Control::Probing_exec_IV_callback( GtkWidget *widget, SPM_Template_Control *dspc){
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
        //dspc->write_dsp_probe (0, PV_MODE_NONE);

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
	// dspc->write_dsp_probe (1, PV_MODE_IV); // Exec STS probing here
	// sranger_common_hwi->start_fifo_read (0, 0,0,0,0, NULL,NULL,NULL,NULL);

	return 0;
}

int SPM_Template_Control::Probing_write_IV_callback( GtkWidget *widget, SPM_Template_Control *dspc){
        // write IV GVP code to controller
        // dspc->write_dsp_probe (0, PV_MODE_IV);
        return 0;
}

// GVP vector program editor 

int SPM_Template_Control::callback_update_GVP_vpc_option_checkbox (GtkWidget *widget, SPM_Template_Control *dspc){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	int  k   = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "VPC"));
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	gtk_check_button_set_active (GTK_CHECK_BUTTON(widget), (dspc->GVP_opt[k] & msk) ? 1:0);

        dspc->set_tab_settings ("VP", dspc->GVP_option_flags, dspc->GVP_auto_flags, dspc->GVP_glock_data);
        return 0;
}

int SPM_Template_Control::callback_change_GVP_vpc_option_flags (GtkWidget *widget, SPM_Template_Control *dspc){
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

int SPM_Template_Control::callback_change_GVP_option_flags (GtkWidget *widget, SPM_Template_Control *dspc){
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

int SPM_Template_Control::callback_GVP_store_vp (GtkWidget *widget, SPM_Template_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->GVP_store_vp ((const gchar*)g_object_get_data(G_OBJECT(widget), "key"));
        return 0;
}
int SPM_Template_Control::callback_GVP_restore_vp (GtkWidget *widget, SPM_Template_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->GVP_restore_vp ((const gchar*)g_object_get_data(G_OBJECT(widget), "key"));
        return 0;
}

int SPM_Template_Control::callback_change_GVP_auto_flags (GtkWidget *widget, SPM_Template_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->GVP_auto_flags = (dspc->GVP_auto_flags & (~msk)) | msk;
	else
		dspc->GVP_auto_flags &= ~msk;

        dspc->set_tab_settings ("VP", dspc->GVP_option_flags, dspc->GVP_auto_flags, dspc->GVP_glock_data);
        return 0;
}

int SPM_Template_Control::Probing_exec_GVP_callback( GtkWidget *widget, SPM_Template_Control *dspc){
	dspc->current_auto_flags = dspc->GVP_auto_flags;

	if (dspc->check_vp_in_progress ()) 
		return -1;

        if (dspc->GVP_auto_flags & FLAG_AUTO_RUN_INITSCRIPT){
                gchar *tmp = g_strdup ("vp-gvp-initial");
                main_get_gapp()->SignalRemoteActionToPlugins (&tmp);
                g_free (tmp);
        }
      
        // ** TEMPLATE DUMMY **
        // write code to controller
        //dspc->write_dsp_probe (0, PV_MODE_NONE);

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
        // exec GVP code on controller now and initiate data streaming
        
	// dspc->probe_trigger_single_shot = 1;
	// dspc->write_dsp_probe (1, PV_MODE_GVP); // Exec FZ probing here
	// sranger_common_hwi->start_fifo_read (0, 0,0,0,0, NULL,NULL,NULL,NULL);

	return 0;
}


int SPM_Template_Control::Probing_write_GVP_callback( GtkWidget *widget, SPM_Template_Control *dspc){
        // write GVP code to controller
        // dspc->write_dsp_probe (0, PV_MODE_GVP);
        return 0;
}

// Graphs Callbacks

int SPM_Template_Control::change_source_callback (GtkWidget *widget, SPM_Template_Control *dspc){
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

int SPM_Template_Control::callback_XJoin (GtkWidget *widget, SPM_Template_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->XJoin = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? TRUE : FALSE;
	dspc->vis_XJoin = dspc->XJoin;
        g_settings_set_boolean (dspc->hwi_settings, "probe-x-join", dspc->XJoin);
        return 0;
}

int SPM_Template_Control::callback_GrMatWindow (GtkWidget *widget, SPM_Template_Control *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->GrMatWin = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? TRUE : FALSE;
        g_settings_set_boolean (dspc->hwi_settings, "probe-graph-matrix-window", dspc->GrMatWin);
        return 0;
}


void SPM_Template_Control::lockin_adjust_callback(Param_Control* pcs, gpointer data){
	SPM_Template_Control *dspc = (SPM_Template_Control*)data;
}

int SPM_Template_Control::lockin_runfree_callback(GtkWidget *widget, SPM_Template_Control *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                PI_DEBUG_GP (DBG_L1, "LockIn Modul ON");
	else
                PI_DEBUG_GP (DBG_L1, "LockIn Modul OFF");

	return 0;
}



// HwI Implementation
// ================================================================================


spm_template_hwi_dev::spm_template_hwi_dev(){

        spm_emu = new SPM_emulator ();
                 
	subscan_data_y_index_offset = 0;
        ScanningFlg=0;
        KillFlg=FALSE;
        
        for (int i=0; i<4; ++i){
                srcs_dir[i] = nsrcs_dir[i] = 0;
                Mob_dir[i] = NULL;
        }
}

spm_template_hwi_dev::~spm_template_hwi_dev(){
        delete spm_emu;
}

int spm_template_hwi_dev::RotateStepwise(int exec) {
        return 0;
}

gboolean spm_template_hwi_dev::SetOffset(double x, double y){
        spm_emu->x0=x; spm_emu->y0=y; // "DAC" units
        return FALSE;
}

gboolean spm_template_hwi_dev::MovetoXY (double x, double y){
        if (!ScanningFlg){
                spm_emu->data_x_index = (int)round(Nx/2 +   x/Dx);
                spm_emu->data_y_index = (int)round(Ny/2 + (-y/Dy));
        }
        // if slow, return TRUE until completed, execuite non blocking!
        // May/Should return FALSE right away if hardware is independently executing and completing the move.
        // else blocking GUI
        return FALSE;
}




// THREAD AND FIFO CONTROL STATES


#define RET_FR_OK      0
#define RET_FR_ERROR   -1
#define RET_FR_WAIT    1
#define RET_FR_NOWAIT  2
#define RET_FR_FCT_END 3

#define FR_NO   0
#define FR_YES  1

#define FR_INIT   1
#define FR_FINISH 2
#define FR_FIFO_FORCE_RESET 3


gpointer ScanDataReadThread (void *ptr_sr);
gpointer ProbeDataReadThread (void *ptr_sr);
gpointer ProbeDataReadFunction (void *ptr_sr, int dspdev);

// ScanDataReadThread:
// Image Data read thread -- actual scan/pixel data transfer

gpointer ScanDataReadThread (void *ptr_sr){
        spm_template_hwi_dev *sr = (spm_template_hwi_dev*)ptr_sr;
        int nx,ny, x0,y0;
        int Nx, Ny;
        g_message("FifoReadThread Start");

        if (sr->Mob_dir[sr->srcs_dir[0] ? 0:1]){
                ny = sr->Mob_dir[sr->srcs_dir[0] ? 0:1][0]->GetNySub(); // number lines to transfer
                nx = sr->Mob_dir[sr->srcs_dir[0] ? 0:1][0]->GetNxSub(); // number points per lines
                x0 = sr->Mob_dir[sr->srcs_dir[0] ? 0:1][0]->data->GetX0Sub(); // number points per lines
                y0 = sr->Mob_dir[sr->srcs_dir[0] ? 0:1][0]->data->GetY0Sub(); // number points per lines

                g_message("FifoReadThread nx,ny = (%d, %d) @ (%d, %d)", nx, ny, x0, y0);

        } else return NULL; // error, no reference

	if (sr->spm_emu->data_y_count == 0){ // top-down
                g_message("FifoReadThread Scanning Top-Down... %d", sr->ScanningFlg);
		for (int yi=y0; yi < y0+ny; ++yi){ // for all lines
                        for (int dir = 0; dir < 4 && sr->ScanningFlg; ++dir){ // check for every pass -> <- 2> <2
                                //g_message("FifoReadThread ny = %d, dir = %d, nsrcs = %d, srcs = 0x%04X", yi, dir, sr->nsrcs_dir[dir], sr->srcs_dir[dir]);
                                if (sr->nsrcs_dir[dir] == 0) // direction pass active?
                                        continue; // not selected?
                                else
                                        for (int xi=x0; xi < x0+nx; ++xi) // all points per line
                                                for (int ch=0; ch<sr->nsrcs_dir[dir]; ++ch){
                                                        sr->spm_emu->data_x_index = xi;
                                                        double z = sr->spm_emu->simulate_value (sr, xi, yi, ch);
                                                        //g_print("%d",ch);
                                                        usleep (1000000 / Template_ControlClass->sim_speed[0]);
                                                        sr->Mob_dir[dir][ch]->PutDataPkt (z, xi, yi);
                                                        while (sr->PauseFlg)
                                                                usleep (1000000 / Template_ControlClass->sim_speed[0]);
                                                }
                                g_print("\n");
                        }
                        sr->spm_emu->data_y_count = yi-y0; // completed
                        sr->spm_emu->data_y_index = yi; // completed
                }
	}else{ // bottom-up
		for (int yi=y0+ny-1; yi-y0 >= 0; --yi){
                        for (int dir = 0; dir < 4 && sr->ScanningFlg; ++dir) // check for every pass -> <- 2> <2
                                if (!sr->nsrcs_dir[dir])
                                        continue; // not selected?
                                else
                                        for (int xi=x0; xi < x0+nx; ++xi)
                                                for (int ch=0; ch<sr->nsrcs_dir[dir]; ++ch){
                                                        sr->spm_emu->data_x_index = xi;
                                                        double z = sr->spm_emu->simulate_value (sr, xi, yi, ch);
                                                        usleep (1000000 / Template_ControlClass->sim_speed[0]);
                                                        sr->Mob_dir[dir][ch]->PutDataPkt (z, xi, yi);
                                                        while (sr->PauseFlg)
                                                                usleep (1000000 / Template_ControlClass->sim_speed[0]);
                                                }
                        sr->spm_emu->data_y_count = yi-y0; // completed
                        sr->spm_emu->data_y_index = yi;
                }
        }

        g_message("FifoReadThread Completed");
        return NULL;
}


// ProbeDataReadThread:
// Independent ProbeDataRead Thread (manual probe)
gpointer ProbeDataReadThread (void *ptr_sr){
	int finish_flag=FALSE;
	spm_template_hwi_dev *sr = (spm_template_hwi_dev*)ptr_sr;

	if (sr->probe_fifo_thread_active){
		//LOGMSGS ( "ProbeFifoReadThread ERROR: Attempt to start again while in progress! [#" << sr->probe_fifo_thread_active << "]" << std::endl);
		return NULL;
	}
	sr->probe_fifo_thread_active++;
	Template_ControlClass->probe_ready = FALSE;

        // normal mode, wait for finish (single shot probe exec by user)
	if (Template_ControlClass->probe_trigger_single_shot)
		 finish_flag=TRUE;

	while (sr->is_scanning () || finish_flag){ // while scanning (raster mode) or until single shot probe is finished
                if (Template_ControlClass->current_auto_flags & FLAG_AUTO_PLOT)
                        Template_ControlClass->Probing_graph_update_thread_safe ();

                if (sr->ReadProbeData ()){ // True when finished
                
                        if (finish_flag){
                                if (Template_ControlClass->current_auto_flags & FLAG_AUTO_PLOT)
                                        Template_ControlClass->Probing_graph_update_thread_safe (1);
				
                                if (Template_ControlClass->current_auto_flags & FLAG_AUTO_SAVE)
                                        Template_ControlClass->Probing_save_callback (NULL, Template_ControlClass);	

                                break;
                        }
                        Template_ControlClass->push_probedata_arrays ();
                        Template_ControlClass->init_probedata_arrays ();
                }
	}

	--sr->probe_fifo_thread_active;
	Template_ControlClass->probe_ready = TRUE;

	return NULL;
}


// ReadProbeData:
// read from probe data FIFO, this engine needs to be called several times from master thread/function
int spm_template_hwi_dev::ReadProbeData (int dspdev, int control){
#if 0
	int pvd_blk_size=0;
	static double pv[9];
	static int last = 0;
	static int last_read_end = 0;
	static DATA_FIFO_EXTERN_PCOPY fifo;
	static PROBE_SECTION_HEADER section_header;
	static int next_header = 0;
	static int number_channels = 0;
	static int data_index = 0;
	static int end_read = 0;
	static int data_block_size=0;
	static int need_fct = FR_YES;  // need fifo control
	static int need_hdr = FR_YES;  // need header
	static int need_data = FR_YES; // need data
	static int ch_lut[32];
	static int ch_msk[]  = { 0x0000001, 0x0000002,   0x0000010, 0x0000020, 0x0000040, 0x0000080,   0x0000100, 0x0000200, 0x0000400, 0x0000800,
				 0x0000008, 0x0001000, 0x0002000, 0x0004000, 0x0008000,   0x0000004,   0x0000000 };
	static int ch_size[] = {    2, 2,    2, 2, 2, 2,   2, 2, 2, 2,    4,   4,  4,  4,   4,   4,  0 };
	static const char *ch_header[] = {"Zmon-AIC5Out", "Umon-AIC6Out", "AIC0-I", "AIC1", "AIC2", "AIC3", "AIC4", "AIC5", "AIC6", "AIC7",
					  "LockIn0", "LockIn1stA", "LockIn1stB", "LockIn2ndA", "LockIn2ndB", "Count", NULL };
	static short data[EXTERN_PROBEDATAFIFO_LENGTH];
	static double dataexpanded[16];
#ifdef LOGMSGS0
	static double dbg0=0., dbg1=0.;
	static int dbgi0=0;
#endif

	switch (control){
	case FR_FIFO_FORCE_RESET: // not used normally -- FIFO is reset by DSP at probe_init (single probe)
		fifo.r_position = 0;
		fifo.w_position = 0;
		check_and_swap (fifo.r_position);
		check_and_swap (fifo.w_position);
		lseek (dspdev, magic_data.probedatafifo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
		sr_write (dspdev, &fifo, 2*sizeof(DSP_INT)); // reset positions now to prevent reading old/last dataset before DSP starts init/putting data
		return RET_FR_OK;

	case FR_INIT:
		last = 0;
		next_header = 0;
		number_channels = 0;
		data_index = 0;
		last_read_end = 0;

		need_fct  = FR_YES;
		need_hdr  = FR_YES;
		need_data = FR_YES;

		Template_ControlClass->init_probedata_arrays ();
		for (int i=0; i<16; dataexpanded[i++]=0.);

		LOGMSGS0 ( std::endl << "************** PROBE FIFO-READ INIT **************" << std::endl);
		LOGMSGS ( "FR::INIT-OK." << std::endl);
		return RET_FR_OK; // init OK.

	case FR_FINISH:
		LOGMSGS ( "FR::FINISH-OK." << std::endl);
		return RET_FR_OK; // finish OK.

	default: break;
	}

	if (need_fct){ // read and check fifo control?
		LOGMSGS2 ( "FR::NEED_FCT, last: 0x"  << std::hex << last << std::dec << std::endl);

		lseek (dspdev, magic_data.probedatafifo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
		sr_read  (dspdev, &fifo, sizeof (fifo));
		check_and_swap (fifo.w_position);
		check_and_swap (fifo.current_section_head_position);
		check_and_swap (fifo.current_section_index);
		check_and_swap (fifo.p_buffer_base);
		end_read = fifo.w_position >= last ? fifo.w_position : EXTERN_PROBEDATAFIFO_LENGTH;

		LOGMSGS2 ( "FR::NEED_FCT, magic.prbdata_fifo  @Adr: 0x"  << std::hex << magic_data.probedatafifo << std::dec << std::endl);
		LOGMSGS2 ( "FR::NEED_FCT, magic.prbdata_fifo.p_buf: 0x"  << std::hex << fifo.p_buffer_base << std::dec << std::endl);
		LOGMSGS2 ( "FR::NEED_FCT, w_position: 0x"  << std::hex << fifo.w_position << std::dec << std::endl);

		// check for new data
		if ((end_read - last) < 1)
			return RET_FR_WAIT;
		else {
			need_fct  = FR_NO;
			need_data = FR_YES;
		}
	}

	if (need_data){ // read full FIFO block
		LOGMSGS ( "FR::NEED_DATA" << std::endl);

		int database = (int)fifo.p_buffer_base;
		int dataleft = end_read;
		int position = 0;
		if (fifo.w_position > last_read_end){
//			database += last_read_end;
//			dataleft -= last_read_end;
		}
		for (; dataleft > 0; database += 0x4000, dataleft -= 0x4000, position += 0x4000){
			LOGMSGS ( "FR::NEED_DATA: B::0x" <<  std::hex << database <<  std::dec << std::endl);
			lseek (dspdev, database, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC); // ### CONSIDER NON_ATOMIC
			sr_read  (dspdev, &data[position], (dataleft >= 0x4000 ? 0x4000 : dataleft)<<1);
		}
		last_read_end = end_read;
			
		need_data = FR_NO;
	}

	if (need_hdr){ // we have enough data if control gets here!
		LOGMSGS ( "FR::NEED_HDR" << std::endl);

		if (((fifo.w_position - last) < (sizeof (section_header)>>1))){
			need_fct  = FR_YES;
			return RET_FR_WAIT;
		}
		
		// set vector of expanded data array representation, section start
		pv[0] = section_header.time;
		pv[1] = section_header.x_offset;
		pv[2] = section_header.y_offset;
		pv[3] = section_header.z_offset;
		pv[4] = section_header.x_scan;
		pv[5] = section_header.y_scan;
		pv[6] = section_header.z_scan;
		pv[7] = section_header.u_scan;
		pv[8] = section_header.section;


		Template_ControlClass->add_probe_hdr (pv);
		

		need_hdr = FR_NO;


	for (int element=0; last < end_read; ++last, ++data_index){


		int channel = ch_lut[element++];
		if (channel >= 0){ // only if valid (skip possible dummy (2) fillings)
			// check for long data (4)
			if (ch_size[channel] == 4){
				LNG *tmp = (LNG*)&data[last];
				check_and_swap (*tmp);
				dataexpanded[channel] = (double) (*tmp);
				++last; ++data_index;
			} else { // normal data (2)
				check_and_swap (data[last]);
				dataexpanded[channel] = (double) data[last];
			}
		}

		// add vector and data to expanded data array representation
		if ((data_index % pvd_blk_size) == (pvd_blk_size-1)){
			if (data_index >= pvd_blk_size) // skip to next vector
				Template_ControlClass->add_probevector ();
			else // set vector as previously read from section header (pv[]) at sec start
				Template_ControlClass->set_probevector (pv);
			// add full data vector
			Template_ControlClass->add_probedata (dataexpanded);
			element = 0;

			// check for FIFO loop now
			if (last > (EXTERN_PROBEDATAFIFO_LENGTH - EXTERN_PROBEDATA_MAX_LEFT)){
				++last;
				LOGMSGS0 ( "FR:FIFO LOOP DETECTED ** Data @ " 
					   << "0x" << std::hex << last
					   << " -2 :[" << (*((DSP_LONG*)&data[last-2]))
					   << " " << (*((DSP_LONG*)&data[last]))
					   << " " << (*((DSP_LONG*)&data[last+2]))
					   << " " << (*((DSP_LONG*)&data[last+4]))
					   << " " << (*((DSP_LONG*)&data[last+6])) 
					   << "] : FIFO LOOP MARK " << std::dec << ( *((DSP_LONG*)&data[last]) == 0 ? "OK":"ERROR")
					   << std::endl);
				next_header -= last;
				last = -1; // compensate for ++last at of for(;;)!
				end_read = fifo.w_position;
			}
		}
	}

	LOGMSGS ( "FR:FIFO NEED FCT" << std::endl);
	need_fct = FR_YES;

	return RET_FR_WAIT;


// emergency bailout and auto recovery, FIFO restart
auto_recover_and_debug:

#ifdef LOGMSGS0
	LOGMSGS0 ( "************** -- FIFO DEBUG -- **************" << std::endl);
	LOGMSGS0 ( "LastArdess: " << "0x" << std::hex << last << std::dec << std::endl);
	for (int adr=last-8; adr < last+32; adr+=8){
		while (adr < 0) ++adr;
		LOGMSGS0 ("0x" << std::hex << adr << "::"
			  << " " << (*((DSP_LONG*)&data[adr]))
			  << " " << (*((DSP_LONG*)&data[adr+2]))
			  << " " << (*((DSP_LONG*)&data[adr+4]))
			  << " " << (*((DSP_LONG*)&data[adr+6]))
			  << std::dec << std::endl);
	}
	LOGMSGS0 ( "************** TRYING AUTO RECOVERY **************" << std::endl);
	LOGMSGS0 ( "***** -- STOP * RESET FIFO * START PROBE -- ******" << std::endl);
#endif
//	LOGMSGS0 ( "***** BAILOUT ****** TERM." << std::endl);

				LOGMSGS ( "END** FR::FCT, w_position: 0x"  << std::hex << fifo.w_position << std::dec << std::endl);
				LOGMSGS ( "END** LastArdess: " << "0x" << std::hex << last << std::dec << std::endl);
				LOGMSGS ( "**FIFO::  HDR ALIGNMENT ASSUMING FOR MAPPING" << std::endl);
				LOGMSGS ( "**FIFO::  SRCS---- N------- t-------  X0-- Y0-- PHI-  XS-- YS-- ZS--  U--- SEC-" << std::endl);
				for (int adr=last-0x4000; adr < last+0x100; adr+=14){
					while (adr < 0) ++adr;
					LOGMSGS (  "0x" << std::setw(4) << std::hex << adr << "::"         // HDR
						   << " " << std::setw(8) << (*((DSP_LONG*)&data[adr]))    // srcs
						   << " " << std::setw(8) << (*((DSP_LONG*)&data[adr+2]))  // n
						   << " " << std::setw(8) << (*((DSP_LONG*)&data[adr+4]))  // t
						   << "  " << std::setw(4) << (*((DSP_INT*)&data[adr+6]))  // x0
						   << " " << std::setw(4) << (*((DSP_INT*)&data[adr+7]))   // y0
						   << " " << std::setw(4) << (*((DSP_INT*)&data[adr+8]))   // ph
						   << "  " << std::setw(4) << (*((DSP_INT*)&data[adr+9]))  // xs
						   << " " << std::setw(4) << (*((DSP_INT*)&data[adr+10]))  // ys
						   << " " << std::setw(4) << (*((DSP_INT*)&data[adr+11]))  // zs(5)
						   << "  " << std::setw(4) << (*((DSP_INT*)&data[adr+12])) // U(6)
						   << " " << std::setw(4) << (*((DSP_INT*)&data[adr+13]))  // sec
						   << std::dec << std::endl);
				}

	Template_ControlClass->dump_probe_hdr (); // TESTING
				

	// *****	exit (0);

	// STOP PROBE
	// reset positions now to prevent reading old/last dataset before DSP starts init/putting data
	DSP_INT start_stop[2] = { 0, 1 };
	lseek (dspdev, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dspdev, &start_stop, 2*sizeof(DSP_INT));

	// RESET FIFO
	// reset positions now to prevent reading old/last dataset before DSP starts init/putting data
	fifo.r_position = 0;
	fifo.w_position = 0;
	check_and_swap (fifo.r_position);
	check_and_swap (fifo.w_position);
	lseek (dspdev, magic_data.probedatafifo, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dspdev, &fifo, 2*sizeof(DSP_INT));

	// START PROBE
	// reset positions now to prevent reading old/last dataset before DSP starts init/putting data
	start_stop[0] = 1;
	start_stop[1] = 0;
	lseek (dspdev, magic_data.probe, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
	sr_write (dspdev, &start_stop, 2*sizeof(DSP_INT));

	// reset all internal and partial reinit FIFO read thread
	last = 0;
	next_header = 0;
	number_channels = 0;
	data_index = 0;
	last_read_end = 0;
	need_fct  = FR_YES;
	need_hdr  = FR_YES;
	need_data = FR_YES;
#endif
	// and start over on next call
	return RET_FR_WAIT;
}




// prepare, create and start g-thread for data transfer
int spm_template_hwi_dev::start_data_read (int y_start, 
                                            int num_srcs0, int num_srcs1, int num_srcs2, int num_srcs3, 
                                            Mem2d **Mob0, Mem2d **Mob1, Mem2d **Mob2, Mem2d **Mob3){

	PI_DEBUG_GP (DBG_L1, "HWI-SPM-TEMPL-DBGI mk2::start fifo read\n");

        g_message("Setting up FifoReadThread");


	if (num_srcs0 || num_srcs1 || num_srcs2 || num_srcs3){ // setup streaming of scan data
#if 0 // full version used by MK2/3
		fifo_data_num_srcs[0] = num_srcs0; // number sources -> (forward)
		fifo_data_num_srcs[1] = num_srcs1; // number sources <- (return)
		fifo_data_num_srcs[2] = num_srcs2; // numbre sources 2nd->  (2nd pass forward if used)
		fifo_data_num_srcs[3] = num_srcs3; // number sources <-2nd  (2nd pass return if used)
		fifo_data_Mobp[0] = Mob0; // 2d memory object list ->
		fifo_data_Mobp[1] = Mob1; // ...
		fifo_data_Mobp[2] = Mob2;
		fifo_data_Mobp[3] = Mob3;
		fifo_data_y_index = y_start; // if > 0, scan dir is "bottom-up"
		fifo_read_thread = g_thread_new ("FifoReadThread", FifoReadThread, this);

                // do also raster probing setup?
		if ((Template_ControlClass->Source & 0xffff) && Template_ControlClass->probe_trigger_raster_points > 0){
			Template_ControlClass->probe_trigger_single_shot = 0;
			ReadProbeFifo (thread_dsp, FR_FIFO_FORCE_RESET); // reset FIFO
			ReadProbeFifo (thread_dsp, FR_INIT); // init
		}
#endif
                // scan simulator
                ScanningFlg=1; // can be used to abort data read thread. (Scan Stop forced by user)
                spm_emu->data_y_count = y_start; // if > 0, scan dir is "bottom-up"
                data_read_thread = g_thread_new ("FifoReadThread", ScanDataReadThread, this);
                
	}
	else{ // expect and stream probe data
		if (Template_ControlClass->vis_Source & 0xffff)
			probe_data_read_thread = g_thread_new ("ProbeFifoReadThread", ProbeDataReadThread, this);
	}
      
	return 0;
}

// ScanLineM():
// Scan setup: (yindex=-2),
// Scan init: (first call with yindex >= 0)
// while scanning following calls are progress checks (return FALSE when yindex line data transfer is completed to go to next line for checking, else return TRUE to continue with this index!
gboolean spm_template_hwi_dev::ScanLineM(int yindex, int xdir, int muxmode,
                                          Mem2d *Mob[MAX_SRCS_CHANNELS],
                                          int ixy_sub[4]){
	static int ydir=0;
	static int running = FALSE;
	static double us_per_line;

        if (yindex == -2){ // SETUP STAGE 1
                int num_srcs_w = 0; // #words
                int num_srcs_l = 0; // #long words
                int num_srcs = 0;
                int bi = 0;

                running      = FALSE;

                do{
                        if(muxmode & (1<<bi++)) // aka "lssrcs" -- count active channels (bits=1)
                                ++num_srcs_w;
                }while(bi<12);
                
                // find # of srcs_l (32 bit data -> float ) 0x1000..0x8000 (Bits 12,13,14,15)
                do{
                        if(muxmode & (1<<bi++))
                                ++num_srcs_l;
                }while(bi<16);
                
                num_srcs = (num_srcs_l<<4) | num_srcs_w;

                switch (xdir){
                case 1: // first init step of XP (->)
                        // reset all
                        for (int i=0; i<4; ++i){
                                srcs_dir[i] = nsrcs_dir[i] = 0;
                                Mob_dir[i] = NULL;
                        }
                        // setup XP ->
                        srcs_dir[0]  = muxmode;
                        nsrcs_dir[0] = num_srcs;
                        Mob_dir[0]   = Mob;
                        return TRUE;
                case -1: // second init step of XM (<-)
                        srcs_dir[1]  = muxmode;
                        nsrcs_dir[1] = num_srcs;
                        Mob_dir[1]   = Mob;
                        return TRUE;
                case  2: // ... init step of 2ND_XP (2>)
                        srcs_dir[2]  = muxmode;
                        nsrcs_dir[2] = num_srcs;
                        Mob_dir[2]   = Mob;
                        return TRUE;
                case -2: // ... init step of 2ND_XM (<2)
                        srcs_dir[3]  = muxmode;
                        nsrcs_dir[3] = num_srcs;
                        Mob_dir[3]   = Mob;
                        return TRUE;
                }
                return FALSE; // error
        }
        
        // SETUP STAGE 2
	if (! running && yindex >= 0){ // now do final scan setup and send scan setup, start reading data fifo
		
		// Nx, Dx are number datapoints in X to take at Dx DAC increments
		// Ny, Dy are number datapoints in Y to take at Dy DAC increments

		ydir = yindex == 0 ? 1 : -1; // scan top-down or bottom-up ?

                // may compute and set if available
		// main_get_gapp()->xsm->data.s.pixeltime = (double)dsp_scan.dnx/SamplingFreq;

                // setup hardware for scan here and
                // start g-thread for data transfer now
		start_data_read (yindex, nsrcs_dir[0], nsrcs_dir[1], nsrcs_dir[2], nsrcs_dir[3], Mob_dir[0], Mob_dir[1], Mob_dir[2], Mob_dir[3]);
                
		running = TRUE; // and off we go....
                return TRUE;
	}

        // ACTUAL SCAN PROGRESS CHECK on line basis
        if (ScanningFlg){ // make sure we did not got aborted and comkpleted already!

                //g_print ("sranger_mk3_hwi_spm::ScanLineM(yindex=%d [fifo-y=%d], xdir=%d, ydir=%d, lssrcs=%x) checking...\n", yindex, data_y_index, xdir, ydir, muxmode);
                y_current = spm_emu->data_y_index;

                if (ydir > 0 && yindex <= spm_emu->data_y_count){
                        g_print ("sranger_mk3_hwi_spm::ScanLineM(yindex=%d [fifo-y=%d], xdir=%d, ydir=%d, lssrcs=%x) y done.\n", yindex, spm_emu->data_y_index, xdir, ydir, muxmode);
                        return FALSE; // line completed top-down
                }
                if (ydir < 0 && yindex >= spm_emu->data_y_count){
                        g_print ("sranger_mk3_hwi_spm::ScanLineM(yindex=%d [fifo-y=%d], xdir=%d, ydir=%d, lssrcs=%x) y done.\n", yindex, spm_emu->data_y_index, xdir, ydir, muxmode);
                        return FALSE; // line completed bot-up
                }

                // optional
                if ((ydir > 0 && yindex > 1) || (ydir < 0 && yindex < Ny-1)){
                        double x,y,z;
                        RTQuery ("W", x,y,z); // run watch dog -- may stop scan in any faulty condition
                }
                
                return TRUE;
        }
        return FALSE; // scan was stopped by users
}




/*
 Real-Time Query of DSP signals/values, auto buffered
 Propertiy hash:      return val1, val2, val3:
 "z" :                ZS, XS, YS  with offset!! -- in volts after piezo amplifier
 "o" :                Z0, X0, Y0  offset -- in volts after piezo amplifier
 "R" :                expected Z, X, Y -- in Angstroem/base unit
 "f" :                dFreq, I-avg, I-RMS
 "s","S" :                DSP Statemachine Status Bits, DSP load, DSP load peak
 "Z" :                probe Z Position
 "i" :                GPIO (high level speudo monitor)
 "A" :                Mover/Wave axis counts 0,1,2 (X/Y/Z)
 "p" :                X,Y Scan/Probe Coords in Pixel, 0,0 is center, DSP Scan Coords
 "P" :                X,Y Scan/Probe Coords in Pixel, 0,0 is top left [indices]
 */

gint spm_template_hwi_dev::RTQuery (const gchar *property, double &val1, double &val2, double &val3){
        if (*property == 'z'){
                double x = (double)(spm_emu->data_x_index-Nx/2)*Dx;
                double y = (double)(Ny/2-spm_emu->data_y_index)*Dy;
                Transform (&x, &y); // apply rotation!
		val1 =  main_get_gapp()->xsm->Inst->VZ() * spm_emu->data_z_value;
		val2 =  main_get_gapp()->xsm->Inst->VX() * (x + spm_emu->x0 ) * 10/32768; // assume "internal" non analog offset adding here (same gain)
                val3 =  main_get_gapp()->xsm->Inst->VY() * (y + spm_emu->y0 ) * 10/32768;
		return TRUE;
	}

	if (*property == 'o' || *property == 'O'){
		// read/convert and return offset
		// NEED to request 'z' property first, then this is valid and up-to-date!!!!
                // no offset simulated
		if (main_get_gapp()->xsm->Inst->OffsetMode () == OFM_ANALOG_OFFSET_ADDING){
			val1 =  main_get_gapp()->xsm->Inst->VZ0() * 0.;
			val2 =  main_get_gapp()->xsm->Inst->VX0() * spm_emu->x0*10/32768;
			val3 =  main_get_gapp()->xsm->Inst->VY0() * spm_emu->y0*10/32768;
		} else {
			val1 =  main_get_gapp()->xsm->Inst->VZ() * 0.;
			val2 =  main_get_gapp()->xsm->Inst->VX() * spm_emu->x0*10/32768;
			val3 =  main_get_gapp()->xsm->Inst->VY() * spm_emu->y0*10/32768;
		}
		
		return TRUE;
	}

        // ZXY in Angstroem
        if (*property == 'R'){
                // ZXY Volts after Piezoamp -- without analog offset -> Dig -> ZXY in Angstroem
		val1 = main_get_gapp()->xsm->Inst->V2ZAng (main_get_gapp()->xsm->Inst->VZ() * spm_emu->data_z_value);
		val2 = main_get_gapp()->xsm->Inst->V2XAng (main_get_gapp()->xsm->Inst->VX() * (double)(spm_emu->data_x_index-Nx/2)*Dx)*10/32768;
                val3 = main_get_gapp()->xsm->Inst->V2YAng (main_get_gapp()->xsm->Inst->VY() * (double)(Ny/2-spm_emu->data_y_index/2)*Dy)*10/32768;
		return TRUE;
        }

        if (*property == 'f'){
                val1 = 0.; // qf - DSPPACClass->pll.Reference[0]; // Freq Shift
		val2 = spm_emu->tip_current / main_get_gapp()->xsm->Inst->nAmpere2V(1.); // actual nA reading    xxxx V  * 0.1nA/V
		val3 = spm_emu->tip_current / main_get_gapp()->xsm->Inst->nAmpere2V(1.); // actual nA RMS reading    xxxx V  * 0.1nA/V -- N/A for simulation
		return TRUE;
	}

        if (*property == 'Z'){
                val1 = 0.; // spm_template_hwi_pi.app->xsm->Inst->Dig2ZA ((double)dsp_probe.Zpos / (double)(1<<16));
		return TRUE;
        }

	// DSP Status Indicators
	if (*property == 's' || *property == 'S' || *property == 'W'){
		if (*property == 'W'){
                        if (ScanningFlg){
                                if (0) EndScan2D(); // if any error detected
                                return TRUE;
                        }
                }
                // build status flags
		val1 = (double)(0
                                +1 // (FeedBackActive    ? 1:0)
				+ ((ScanningFlg & 3) << 1)  // Stop = 1, Pause = 2
				//+ (( ProbingSTSActive   ? 1:0) << 3)
				//+ (( MoveInProgress     ? 1:0) << 4)
				//+ (( PLLActive          ? 1:0) << 5)
				//+ (( ZPOS-Adjuster      ? 1:0) << 6)
				//+ (( AutoApproachActive ? 1:0) << 7)
			);
        }
        
	// quasi GPIO monitor/mirror -- HIGH LEVEL!!!
	if (*property == 'i'){
		//val1 = (double)gpio3_monitor_out;
		//val2 = (double)gpio3_monitor_in;
		//val3 = (double)gpio3_monitor_dir;
		return TRUE;
	}

        if (*property == 'p'){ // SCAN DATA INDEX, CENTERED range: +/- NX/2, +/-NY/2
                val1 = (double)( spm_emu->data_x_index - (main_get_gapp()->xsm->data.s.nx/2 - 1) + 1);
                val2 = (double)(-spm_emu->data_y_index + (main_get_gapp()->xsm->data.s.ny/2 - 1) + 1);
                val3 = (double)spm_emu->data_z_value;
		return TRUE;
        }
        if (*property == 'P'){ // SCAN DATA INDEX, range: 0..NX, 0..NY
                val1 = (double)(spm_emu->data_x_index);
                val2 = (double)(spm_emu->data_y_index);
                val3 = (double)spm_emu->data_z_value;
		return TRUE;
        }
//	printf ("ZXY: %g %g %g\n", val1, val2, val3);

//	val1 =  (double)dsp_analog_out.z_scan;
//	val2 =  (double)dsp_analog_out.x_scan;
//	val3 =  (double)dsp_analog_out.y_scan;

	return TRUE;
}

// template/dummy signal management



int spm_template_hwi_dev::lookup_signal_by_ptr(gint64 sigptr){
	for (int i=0; i<NUM_SIGNALS; ++i){
		if (dsp_signal_lookup_managed[i].dim == 1 && sigptr == dsp_signal_lookup_managed[i].p)
			return i;
		gint64 offset = sigptr - (gint64)dsp_signal_lookup_managed[i].p;
		if (sigptr >= dsp_signal_lookup_managed[i].p && offset < 4*dsp_signal_lookup_managed[i].dim){
			dsp_signal_lookup_managed[i].index = offset/4;
			return i;
		}
	}
	return -1;
}

int spm_template_hwi_dev::lookup_signal_by_name(const gchar *sig_name){
	for (int i=0; i<NUM_SIGNALS; ++i)
		if (!strcmp (sig_name, dsp_signal_lookup_managed[i].label))
			return i;
	return -1;
}

const gchar *spm_template_hwi_dev::lookup_signal_name_by_index(int i){
	if (i<NUM_SIGNALS && i >= 0){
		// dsp_signal_lookup_managed[i].index
		return (const gchar*)dsp_signal_lookup_managed[i].label;
	} else
		return "INVALID INDEX";
}

const gchar *spm_template_hwi_dev::lookup_signal_unit_by_index(int i){
	if (i<NUM_SIGNALS && i >= 0)
		return (const gchar*)dsp_signal_lookup_managed[i].unit;
	else
		return "INVALID INDEX";
}

double spm_template_hwi_dev::lookup_signal_scale_by_index(int i){
	if (i<NUM_SIGNALS && i >= 0)
		return dsp_signal_lookup_managed[i].scale;
	else
		return 0.;
}

int spm_template_hwi_dev::change_signal_input(int signal_index, gint32 input_id, gint32 voffset){
	gint32 si = signal_index | (voffset >= 0 && voffset < dsp_signal_lookup_managed[signal_index].dim ? voffset<<16 : 0); 
	SIGNAL_MANAGE sm = { input_id, si, 0, 0 }; // for read/write control part of signal_monitor only
	PI_DEBUG_GM (DBG_L3, "XX::change_module_signal_input");

        //
        // dummy
        // must adjust signal configurayion on controller here
        //
        
	PI_DEBUG_GM (DBG_L3, "XX::change_module_signal_input done: [%d,%d,0x%x,0x%x]",
                     sm.mindex,sm.signal_id,sm.act_address_input_set,sm.act_address_signal);
	return 0;
}

int spm_template_hwi_dev::query_module_signal_input(gint32 input_id){
        int mode=0;
        int ret;

        // template dummy -- query controller here:
        // read signal address at input_id, then lookup signal by address from map

        // use dummy list
	int signal_index = mod_input_list[input_id].id; // lookup_signal_by_ptr (sm.act_address_signal);

	return signal_index;
}

int spm_template_hwi_dev::read_signal_lookup (){
        // read actual signal list from controller and manage a copy
#if 0
	for (int i=0; i<NUM_SIGNALS; ++i){
		CONV_32 (dsp_signal_list[i].p);
		dsp_signal_lookup_managed[i].p = dsp_signal_list[i].p;
		dsp_signal_lookup_managed[i].dim   = dsp_signal_lookup[i].dim;
		dsp_signal_lookup_managed[i].label = g_strdup(dsp_signal_lookup[i].label);
                if (i==0){ // IN0 dedicated to tunnel current via IVC
                        // g_print ("1nA to Volt=%g  1pA to Volt=%g",main_get_gapp()->xsm->Inst->nAmpere2V (1.),main_get_gapp()->xsm->Inst->nAmpere2V (1e-3));
                        if (main_get_gapp()->xsm->Inst->nAmpere2V (1.) > 1.){
                                dsp_signal_lookup_managed[i].unit  = g_strdup("pA"); // use pA scale
                                dsp_signal_lookup_managed[i].scale = dsp_signal_lookup[i].scale; // -> Volts
                                dsp_signal_lookup_managed[i].scale /= main_get_gapp()->xsm->Inst->nAmpere2V (1); // values are always in nA
                        } else {
                                dsp_signal_lookup_managed[i].unit  = g_strdup("nA");
                                dsp_signal_lookup_managed[i].scale = dsp_signal_lookup[i].scale; // -> Volts
                                dsp_signal_lookup_managed[i].scale /= main_get_gapp()->xsm->Inst->nAmpere2V (1.); // nA
                        }
                } else {
                        dsp_signal_lookup_managed[i].unit  = g_strdup(dsp_signal_lookup[i].unit);
                        dsp_signal_lookup_managed[i].scale = dsp_signal_lookup[i].scale;
                }
		dsp_signal_lookup_managed[i].module  = g_strdup(dsp_signal_lookup[i].module);
		dsp_signal_lookup_managed[i].index  = -1;
                PI_DEBUG_PLAIN (DBG_L2,
                                "Sig[" << i << "]: ptr=" << dsp_signal_lookup_managed[i].p 
                                << ", " << dsp_signal_lookup_managed[i].dim 
                                << ", " << dsp_signal_lookup_managed[i].label 
                                << " [v" << dsp_signal_lookup_managed[i].index << "] "
                                << ", " << dsp_signal_lookup_managed[i].unit
                                << ", " << dsp_signal_lookup_managed[i].scale
                                << ", " << dsp_signal_lookup_managed[i].module
                                << "\n"
                                );
		for (int k=0; k<i; ++k)
			if (dsp_signal_lookup_managed[k].p == dsp_signal_lookup_managed[i].p){
                                PI_DEBUG_PLAIN (DBG_L2,
                                                "Sig[" << i << "]: ptr=" << dsp_signal_lookup_managed[i].p 
                                                << " identical with Sig[" << k << "]: ptr=" << dsp_signal_lookup_managed[k].p 
                                                << " ==> POSSIBLE ERROR IN SIGNAL TABLE <== GXSM is aborting here, suspicious DSP data.\n"
                                                );
                                g_warning ("DSP SIGNAL TABLE finding: Sig[%d] '%s': ptr=%x is identical with Sig[%d] '%s': ptr=%x",
                                           i, dsp_signal_lookup_managed[i].label, dsp_signal_lookup_managed[i].p,
                                           k, dsp_signal_lookup_managed[k].label, dsp_signal_lookup_managed[k].p);
				// exit (-1);
			}
	}
#endif
	return 0;
}

int spm_template_hwi_dev::read_actual_module_configuration (){
	for (int i=0; mod_input_list[i].id; ++i){
		int si = query_module_signal_input (mod_input_list[i].id);
		if (si >= 0 && si < NUM_SIGNALS){
                        PI_DEBUG_GM (DBG_L2, "INPUT %s (%04X) is set to %s",
                                     mod_input_list[i].name, mod_input_list[i].id,
                                     dsp_signal_lookup_managed[si].label );
		} else {
                        if (si == SIGNAL_INPUT_DISABLED)
                                PI_DEBUG_GM (DBG_L2, "INPUT %s (%04X) is DISABLED", mod_input_list[i].name, mod_input_list[i].id);
                        else
                                PI_DEBUG_GM (DBG_L2, "INPUT %s (%04X) -- ERROR DETECTED", mod_input_list[i].name, mod_input_list[i].id);
		}
	}

	return 0;
}
