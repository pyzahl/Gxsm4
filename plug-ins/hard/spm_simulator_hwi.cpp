/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Hardware Interface Plugin Name: sranger_mk2_hwi.C
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
% PlugInDocuCaption: SPM SIM and Template Hardware Interface
% PlugInName: spm_simulator_hwi
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Tools/SPM SIM Control

% PlugInDescription
\index{SPM SIM HwI / Template Code}

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


// Define HwI PlugIn reference name here, this is what is listed later within "Preferenced Dialog"
// i.e. the string selected for "Hardware/Card"!
#define THIS_HWI_PLUGIN_NAME "SPM-SIM:SPM"
#define THIS_HWI_PREFIX      "SPM_SIM_HwI"

extern int debug_level;
extern int force_gxsm_defaults;


// Plugin Prototypes
static void spm_simulator_hwi_init( void );
static void spm_simulator_hwi_about( void );
static void spm_simulator_hwi_configure( void );
static void spm_simulator_hwi_query( void );
static void spm_simulator_hwi_cleanup( void );

static void SIMControl_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// custom menu callback expansion to arbitrary long list
static PIMenuCallback spm_simulator_hwi_mcallback_list[] = {
		SIMControl_show_callback,             // direct menu entry callback1
		NULL  // NULL terminated list
};

static GxsmPluginMenuCallbackList spm_simulator_hwi_menu_callback_list = {
	1,
	spm_simulator_hwi_mcallback_list
};

GxsmPluginMenuCallbackList *get_gxsm_plugin_menu_callback_list ( void ) {
	return &spm_simulator_hwi_menu_callback_list;
}

// Fill in the GxsmPlugin Description here
GxsmPlugin spm_simulator_hwi_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	"spm_simulator_hwi-"
	"HW-INT-1S-SHORT",
	// Plugin's Category - used to autodecide on Pluginloading or ignoring
	// In this case of Hardware-Interface-Plugin here is the interface-name required
	// this is the string selected for "Hardware/Card"!
	THIS_HWI_PLUGIN_NAME,
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	g_strdup ("SPM Simulator and template hardware interface."),
	// Author(s)
	"Percy Zahl",
	// Menupath to position where it is appendet to -- not used by HwI PIs
	"windows-section,windows-section,windows-section,windows-section",
	// Menuentry -- not used by HwI PIs
	N_("SIM Control"),
	// help text shown on menu
	N_("This is the " THIS_HWI_PLUGIN_NAME " - GXSM Hardware Interface."),
	// more info...
	"N/A",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	spm_simulator_hwi_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	spm_simulator_hwi_query, // query can be used (otherwise set to NULL) to install
	// additional control dialog in the GXSM menu
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	spm_simulator_hwi_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	spm_simulator_hwi_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
	NULL,   // direct menu entry callback1 or NULL
	NULL,   // direct menu entry callback2 or NULL
	spm_simulator_hwi_cleanup // plugin cleanup callback or NULL
};


// Text used in Aboutbox, please update!!
static const char *about_text = N_("GXSM spm_simulator_hwi Plugin\n\n"
                                   "Simulator and template HwI Interface.");

/* Here we go... */

#include "spm_simulator_hwi.h"

/*
 * PI global
 */

// #define PI_DEBUG(L, DBGTXT) std::cout << "** (" << __FILE__ << ": " << __FUNCTION__ << ") Gxsm-PI-DEBUG-MESSAGE **: " << std::endl << " - " << DBGTXT << std::endl

gchar *spm_simulator_hwi_configure_string = NULL;   // name of the currently in GXSM configured HwI (Hardware/Card)
spm_simulator_hwi_dev *spmsim_hwi = NULL; // instance of the HwI derived XSM_Hardware class

const gchar *SIMControl_menupath  = "windows-section";
const gchar *SIMControl_menuentry = N_("SIM Control");
const gchar *SIMControl_menuhelp  = N_("open the SPM Simulator control window");

SPM_SIM_Control *SIMControlClass = NULL;


// event hooks
static void SIMControl_StartScan_callback ( gpointer );
static void SIMControl_SaveValues_callback ( gpointer );
static void SIMControl_LoadValues_callback ( gpointer );

/* 
 * PI essential members
 */

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	spm_simulator_hwi_pi.description = g_strdup_printf(N_("GXSM HwI " THIS_HWI_PREFIX " plugin %s"), VERSION);
	return &spm_simulator_hwi_pi; 
}

// Symbol "get_gxsm_hwi_hardware_class" is resolved by dlsym from Gxsm for all HwI type PIs, 
// Essential Plugin Function!!
XSM_Hardware *get_gxsm_hwi_hardware_class ( void *data ) {
        gchar *tmp;
        main_get_gapp()->monitorcontrol->LogEvent (THIS_HWI_PREFIX " XSM_Hardware *get_gxsm_hwi_hardware_class", "Init 1");

	spm_simulator_hwi_configure_string = g_strdup ((gchar*)data);

	// probe for harware here...
	spmsim_hwi = new spm_simulator_hwi_dev ();
	
        main_get_gapp()->monitorcontrol->LogEvent ("HwI: probing succeeded.", "SPM SIM System Ready.");
	return spmsim_hwi;
}

// init-Function
static void spm_simulator_hwi_init(void)
{
	PI_DEBUG (DBG_L2, "spm_simulator_hwi Plugin Init");
	spmsim_hwi = NULL;
}

// about-Function
static void spm_simulator_hwi_about(void)
{
	const gchar *authors[] = { spm_simulator_hwi_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  spm_simulator_hwi_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void spm_simulator_hwi_configure(void)
{
	PI_DEBUG (DBG_L2, "spm_simulator_hwi Plugin HwI-Configure");
	if(spm_simulator_hwi_pi.app)
		spm_simulator_hwi_pi.app->message("spm_simulator_hwi Plugin Configuration");
}

// query-Function
static void spm_simulator_hwi_query(void)
{
	PI_DEBUG_GP (DBG_L1, THIS_HWI_PREFIX "::spm_simulator_hwi_query:: PAC check... ");

//      Setup Control Windows
// ==================================================
	SIMControlClass = new SPM_SIM_Control (main_get_gapp() -> get_app ());
        
	spm_simulator_hwi_pi.status = g_strconcat(N_("Plugin query has attached "),
                                                  spm_simulator_hwi_pi.name, 
                                                  N_(": " THIS_HWI_PREFIX "-Control is created."),
                                                  NULL);
}


// Hwi Plugin cleanup-Function
static void spm_simulator_hwi_cleanup(void)
{
	PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::spm_simulator_hwi_cleanup -- Plugin Cleanup, -- Menu [disabled]\n");

        // remove GUI
	if( SIMControlClass ){
		delete SIMControlClass ;
        }
	SIMControlClass = NULL;

	if (spmsim_hwi){
                PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::spm_simulator_hwi_cleanup -- delete DSP Common-HwI-Class.");
		delete spmsim_hwi;
        }
	spmsim_hwi = NULL;

        PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::spm_simulator_hwi_cleanup -- g_free spm_simulator_hwi_configure_string Info.");
	g_free (spm_simulator_hwi_configure_string);
	spm_simulator_hwi_configure_string = NULL;
	
        PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::spm_simulator_hwi_cleanup -- Done.");
}





// GUI Section -- custom to HwI
// ================================================================================
static GActionEntry win_SPM_SIM_popup_entries[] = {
        { "dsp-mover-configure", SPM_SIM_Control::configure_callback, NULL, "true", NULL },
};

void SIMControl_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
        ; // show window... if dummy closed it...
}


void SPM_SIM_Control::configure_callback (GSimpleAction *action, GVariant *parameter, 
                                          gpointer user_data){
        SPM_SIM_Control *mc = (SPM_SIM_Control *) user_data;
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
                          (GFunc) SPM_SIM_Control::show_tab_to_configure, NULL
                          );
        } else  {
                g_slist_foreach
                        ( mc->bp->get_configure_list_head (),
                          (GFunc) App::hide_w, NULL
                          );
                g_slist_foreach
                        ( mc->bp->get_config_checkbutton_list_head (),
                          (GFunc) SPM_SIM_Control::show_tab_as_configured, NULL
                          );
        }
        if (!action){
                g_variant_unref (new_state);
        }
}

void SPM_SIM_Control::AppWindowInit(const gchar *title){
        if (title) { // stage 1
                PI_DEBUG (DBG_L2, "SPMSIMControl::AppWindowInit -- header bar");

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
                PI_DEBUG (DBG_L2, "SPM_SIM_Control::AppWindowInit -- header bar -- stage two, hook configure menu");

                win_SPM_SIM_popup_entries[0].state = g_settings_get_boolean (hwi_settings, "configure-mode") ? "true":"false"; // get last state
                g_action_map_add_action_entries (G_ACTION_MAP (app_window),
                                                 win_SPM_SIM_popup_entries, G_N_ELEMENTS (win_SPM_SIM_popup_entries),
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

int SPM_SIM_Control::choice_Ampl_callback (GtkWidget *widget, SPM_SIM_Control *spmsc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	gint i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	gint j = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "chindex"));
	switch(j){
	case 0: spm_simulator_hwi_pi.app->xsm->Inst->VX(i);
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_DSP_OFFSET_ADDING)
			spm_simulator_hwi_pi.app->xsm->Inst->VX0(i);
		break;
	case 1: spm_simulator_hwi_pi.app->xsm->Inst->VY(i);
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_DSP_OFFSET_ADDING)
			spm_simulator_hwi_pi.app->xsm->Inst->VY0(i);
		break;
	case 2: spm_simulator_hwi_pi.app->xsm->Inst->VZ(i); 
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_DSP_OFFSET_ADDING)
			spm_simulator_hwi_pi.app->xsm->Inst->VZ0(i); 
		break;
	case 3:
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING)
			spm_simulator_hwi_pi.app->xsm->Inst->VX0(i);
		break;
	case 4:
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING)
			spm_simulator_hwi_pi.app->xsm->Inst->VY0(i);
		break;
	case 5:
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING)
			spm_simulator_hwi_pi.app->xsm->Inst->VZ0(i);
		break;
	}

	PI_DEBUG (DBG_L2, "Ampl: Ch=" << j << " i=" << i );
	spm_simulator_hwi_pi.app->spm_range_check(NULL, spm_simulator_hwi_pi.app);
        
	return 0;
}

int SPM_SIM_Control::config_options_callback (GtkWidget *widget, SPM_SIM_Control *ssc){
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                ssc->options |= GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget),"Bit_Mask"));
        else
                ssc->options &= ~GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget),"Bit_Mask"));
        
        g_message ("SPM_SIM_Control::config_options_callback -> %04x", ssc->options);
        return 0;
}


void SPM_SIM_Control::create_folder (){
        GtkWidget *notebook;
 
        AppWindowInit ("SPM Simulator");
        
        // ========================================
        notebook = gtk_notebook_new ();
        gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
        gtk_notebook_popup_enable (GTK_NOTEBOOK (notebook));

        gtk_widget_show (notebook);
        gtk_grid_attach (GTK_GRID (v_grid), notebook, 1,1, 1,1);

        const char *TabNames[] = { "STM", "AFM", "Config", NULL};
        const char *tab_key[] = { "sim-tab-stm", "sim-tab-afm", "sim-tab-config", NULL};
        const char *tab_key_prefix[] = { "sim-stm-", "sim-afm-", "sim-config-", NULL};
        int i,itab;

        bp = new GUI_Builder (v_grid);
        bp->set_error_text ("Invalid Value.");
        bp->set_input_width_chars (10);
        bp->set_no_spin ();

        for (itab=i=0; TabNames[i]; ++i){                
                Gtk_EntryControl *ec_axis[3];

                bp->start_notebook_tab (notebook, TabNames[i], tab_key[i], hwi_settings);
                itab++;
                        
                bp->set_pcs_remote_prefix (tab_key_prefix[i]);

                if (i<2){
                        bp->new_grid_with_frame ("Sim Parameters");
                        bp->set_input_width_chars(20);
                        bp->set_default_ec_change_notice_fkt (SPM_SIM_Control::ChangedNotify, this);

                        bp->grid_add_ec_with_scale ("Speed", Velocity, &speed[i], 0.01, 150e3, "5.2f", 1., 10., "speed");
                        g_object_set_data( G_OBJECT (bp->input), "TabIndex", GINT_TO_POINTER (i));
                        
                        bp->new_line ();

                        bp->set_configure_list_mode_on ();
                        bp->grid_add_ec_with_scale ("Bias", Volt, &bias[i], -10., 10., "5.2f", 0.1, 1., "bias");
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
                                                  G_CALLBACK (SPM_SIM_Control::choice_Ampl_callback),
                                                  this);

                                gchar *tmpkey = g_strconcat ("spm-sim-h", PDR_gain_key[j], NULL); 
                                // get last setting, will call callback connected above to update gains!
                                g_settings_bind (hwi_settings, tmpkey,
                                                 G_OBJECT (GTK_COMBO_BOX (wid)), "active",
                                                 G_SETTINGS_BIND_DEFAULT);
                                g_free (tmpkey);

                                //VXYZS0Gain[j] = wid; // store for remote access/manipulation
                                bp->grid_add_widget (wid);
                        }

                        // ...
                }

                bp->notebook_tab_show_all ();
	}
  
	// ============================================================
	// save List away...
        //g_object_set_data( G_OBJECT (window), "SPM_SIM_EC_list", bp->get_ec_list_head ());
        spm_simulator_hwi_pi.app->RemoteEntryList = g_slist_concat (spm_simulator_hwi_pi.app->RemoteEntryList, bp->get_remote_list_head ());
        configure_callback (NULL, NULL, this); // configure "false"
        
        AppWindowInit (NULL); // stage two
        set_window_geometry ("spm-simulator-control"); // must add key to xml file: Gxsm-3.0/gxsm4/org.gnome.gxsm4.window-geometry.gschema.xml
}


// HwI Implementation
// ================================================================================


spm_simulator_hwi_dev::spm_simulator_hwi_dev(){
        data_z_value = 0.;
        data_y_count = 0;
        data_y_index = 0;
        data_x_index = 0;
        sim_current=0.;
        
	subscan_data_y_index_offset = 0;
        ScanningFlg=0;
        KillFlg=FALSE;
        
        for (int i=0; i<4; ++i){
                srcs_dir[i] = nsrcs_dir[i] = 0;
                Mob_dir[i] = NULL;
        }
}

spm_simulator_hwi_dev::~spm_simulator_hwi_dev(){
}

int spm_simulator_hwi_dev::RotateStepwise(int exec) {
        return 0;
}

gboolean spm_simulator_hwi_dev::SetOffset(double x, double y){
        x0=x; y0=y; // "DAC" units
        return FALSE;
}

gboolean spm_simulator_hwi_dev::MovetoXY (double x, double y){
        if (!ScanningFlg){
                data_x_index = (int)round(Nx/2 +   x/Dx);
                data_y_index = (int)round(Ny/2 + (-y/Dy));
        }
        // if slow, return TRUE until completed, execuite non blocking!
        // May/Should return FALSE right away if hardware is independently executing and completing the move.
        // else blocking GUI
        return FALSE;
}

class feature{
public:
        feature(double rr=400.){
                ab = 3.5;
                xy[0] = ab*g_random_double_range (-rr, rr);
                xy[1] = ab*g_random_double_range (-rr, rr);
        };
        ~feature(){};

        virtual double get_z(double x, double y, int ch) { return 0.; };
        
        double xy[2];
        double ab;
};

class feature_step : public feature{
public:
        feature_step():feature(400.){
                phi = g_random_double_range (25., 75.);
                m = atan(phi/57.);
                dz = 3.4;
        };
        ~feature_step(){};

        virtual double get_z(double x, double y, int ch) {
                x -= xy[0];
                y -= xy[1];
                return m * x > y ? dz : 0.;
        };
private:
        double phi,m;
        double dz;
};

class feature_island : public feature{
public:
        feature_island(double rr=1000.):feature(rr){
                r2 = g_random_double_range (10*ab, 100*ab);
                r2 *= r2;
        };
        ~feature_island(){};

        virtual double get_z(double x, double y, int ch)  {
                x -= xy[0];
                y -= xy[1];
                return x*x+y*y < r2 ? ab : 0.;
        };
private:
        double r2;
};

class feature_blob : public feature_island{
public:
        feature_blob():feature_island(2000.){
                bz=50.;
                double rr=bz;
                for (int i=0; i<200; ++i){
                        xx[i] = ab*g_random_double_range (-rr, rr);
                        yy[i] = ab*g_random_double_range (-rr, rr);
                        r2[i] = g_random_double_range (10*ab, bz*ab);
                        r2[i] *= r2[i];
                }
        };
        ~feature_blob(){};

        virtual double get_z(double x, double y, int ch)  {
                x -= xy[0];
                y -= xy[1];
                double z=0.;
                for (int i=0; i<200; ++i){
                        z += (x-xx[i])*(x-xx[i])+(y-yy[i])*(y-yy[i]) < r2[i] ? ab : 0.;
                }
                return z;
        };
private:
        double xx[200], yy[200];
        double r2[200];
        double bz;
};

class feature_molecule : public feature{
public:
        feature_molecule(){
                w=10.; h=30.;
                phi = M_PI/6.*g_random_int_range (0, 4);
        };
        ~feature_molecule(){};

        double shape(double x, double y){
                double zx=cos(x/w*(M_PI/2));
                double zy=cos(y/h*(M_PI/2));
                return 6.5*(zx*zx*zy*zy);
        };
        
        virtual double get_z(double x, double y, int ch) {
                x -= xy[0];
                y -= xy[1];
                double xx =  x*cos(phi)+y*sin(phi);
                double yy = -x*sin(phi)+y*cos(phi);
                double rx=w*w;
                double ry=h*h;
                return xx*xx < rx && yy*yy < ry ? shape(xx,yy) : 0.;
        };
private:
        double phi;
        double w,h;
};

class feature_lattice : public feature{
public:
        feature_lattice(){
        };
        ~feature_lattice(){};

        // create dummy data 3.5Ang period
        virtual double get_z(double x, double y, int ch) {
                return 10.*ch + SIMControlClass->bias[0] * ab * sin(x/ab*2.*M_PI) * cos(y/ab*2.*M_PI);
        };
};


double spm_simulator_hwi_dev::simulate_value (int xi, int yi, int ch){
        const int N_steps=20;
        const int N_blobs=7;
        const int N_islands=10;
        const int N_molecules=512;
        const int N_bad_spots=16;
        static feature_step stp[N_steps];
        static feature_island il[N_islands];
        static feature_blob blb[N_blobs];
        static feature_molecule mol[N_molecules];
        static feature_lattice lat;
        static double fz = 1./main_get_gapp()->xsm->Inst->Dig2ZA(1);

        double x = xi*Dx-Dx*Nx/2; // x in DAC units
        double y = (Ny-yi-1)*Dy-Dy*Ny/2; // y in DAC units, i=0 is top line, i=Ny is bottom line

        // Please Note:
        // spm_simulator_hwi_pi.app->xsm->... and main_get_gapp()->xsm->...
        // are identical pointers to the main g-application (gapp) class and it is made availabe vie the plugin descriptor
        // in case the global gapp is not exported or used in the plugin. And either one may be used to access core settings.
        
        x = main_get_gapp()->xsm->Inst->Dig2XA ((long)round(x)); // convert to anstroems for model using instrument class, use Scan Gains
        y = main_get_gapp()->xsm->Inst->Dig2YA ((long)round(y)); // convert to anstroems for model

        x += 1.5*g_random_double_range (-main_get_gapp()->xsm->Inst->Dig2XA(2), main_get_gapp()->xsm->Inst->Dig2XA(2));
        y += 1.5*g_random_double_range (-main_get_gapp()->xsm->Inst->Dig2YA(2), main_get_gapp()->xsm->Inst->Dig2YA(2));
        
        //g_print ("XY: %g %g  [%g %g %d %d]",x,y, Dx,Dy, xi,yi);
        
        invTransform (&x, &y); // apply rotation! Use invTransform for simualtion.
        x += main_get_gapp()->xsm->Inst->Dig2X0A (x0); // use Offset Gains
        y += main_get_gapp()->xsm->Inst->Dig2Y0A (y0);

        //g_print ("XYR0: %g %g",x,y);

        // use template landscape is scan loaded to CH11 !!
        if (main_get_gapp()->xsm->scan[10]){
                double ix,iy;
                main_get_gapp()->xsm->scan[10]->World2Pixel  (x, y, ix,iy);
                return main_get_gapp()->xsm->scan[10]->data.s.dz * main_get_gapp()->xsm->scan[10]->mem2d->GetDataPktInterpol (ix,iy);
        }

        double z=0.0;
        for (int i=0; i<N_steps;     ++i) z += stp[i].get_z (x,y, ch);
        for (int i=0; i<N_islands;   ++i) z +=  il[i].get_z (x,y, ch);
        if (SIMControlClass->options & 2)
                for (int i=0; i<N_blobs;     ++i) z += blb[i].get_z (x,y, ch);
        double zm=0;
        if (SIMControlClass->options & 1)
                for (int i=0; i<N_molecules; ++i) zm += mol[i].get_z (x,y, ch);
        
        return fz * (z + (zm > 0. ? zm : lat.get_z (x,y, ch)));
}

// DataReadThread:
// Image Data read thread -- actual scan/pixel data transfer

gpointer DataReadThread (void *ptr_sr){
        spm_simulator_hwi_dev *sr = (spm_simulator_hwi_dev*)ptr_sr;
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

	if (sr->data_y_count == 0){ // top-down
                g_message("FifoReadThread Scanning Top-Down... %d", sr->ScanningFlg);
		for (int yi=y0; yi < y0+ny; ++yi){ // for all lines
                        for (int dir = 0; dir < 4 && sr->ScanningFlg; ++dir){ // check for every pass -> <- 2> <2
                                //g_message("FifoReadThread ny = %d, dir = %d, nsrcs = %d, srcs = 0x%04X", yi, dir, sr->nsrcs_dir[dir], sr->srcs_dir[dir]);
                                if (sr->nsrcs_dir[dir] == 0) // direction pass active?
                                        continue; // not selected?
                                else
                                        for (int xi=x0; xi < x0+nx; ++xi) // all points per line
                                                for (int ch=0; ch<sr->nsrcs_dir[dir]; ++ch){
                                                        sr->data_x_index = xi;
                                                        sr->data_z_value = sr->simulate_value (xi, yi, ch);
                                                        //g_print("%d",ch);
                                                        usleep (1000000 / SIMControlClass->speed[0]);
                                                        sr->Mob_dir[dir][ch]->PutDataPkt (sr->data_z_value, xi, yi);
                                                        while (sr->PauseFlg)
                                                                usleep (1000000 / SIMControlClass->speed[0]);
                                                }
                                g_print("\n");
                        }
                        sr->data_y_count = yi-y0; // completed
                        sr->data_y_index = yi; // completed
                }
	}else{ // bottom-up
		for (int yi=y0+ny-1; yi-y0 >= 0; --yi){
                        for (int dir = 0; dir < 4 && sr->ScanningFlg; ++dir) // check for every pass -> <- 2> <2
                                if (!sr->nsrcs_dir[dir])
                                        continue; // not selected?
                                else
                                        for (int xi=x0; xi < x0+nx; ++xi)
                                                for (int ch=0; ch<sr->nsrcs_dir[dir]; ++ch){
                                                        sr->data_x_index = xi;
                                                        sr->data_z_value = sr->simulate_value (xi, yi, ch);
                                                        usleep (1000000 / SIMControlClass->speed[0]);
                                                        sr->Mob_dir[dir][ch]->PutDataPkt (sr->data_z_value, xi, yi);
                                                        while (sr->PauseFlg)
                                                                usleep (1000000 / SIMControlClass->speed[0]);
                                                }
                        sr->data_y_count = yi-y0; // completed
                        sr->data_y_index = yi;
                }
        }

        g_message("FifoReadThread Completed");
        return NULL;
}

// prepare, create and start g-thread for data transfer
int spm_simulator_hwi_dev::start_data_read (int y_start, 
                                            int num_srcs0, int num_srcs1, int num_srcs2, int num_srcs3, 
                                            Mem2d **Mob0, Mem2d **Mob1, Mem2d **Mob2, Mem2d **Mob3){

	PI_DEBUG_GP (DBG_L1, "HWI-SPM-SIM-DBGI mk2::start fifo read\n");

        g_message("Setting up FifoReadThread");

        ScanningFlg=1; // can be used to abort data read thread. (Scan Stop forced by user)
        data_y_count = y_start; // if > 0, scan dir is "bottom-up"
        data_read_thread = g_thread_new ("FifoReadThread", DataReadThread, this);

	return 0;
}

// ScanLineM():
// Scan setup: (yindex=-2),
// Scan init: (first call with yindex >= 0)
// while scanning following calls are progress checks (return FALSE when yindex line data transfer is completed to go to next line for checking, else return TRUE to continue with this index!
gboolean spm_simulator_hwi_dev::ScanLineM(int yindex, int xdir, int muxmode,
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
                y_current = data_y_index;

                if (ydir > 0 && yindex <= data_y_count){
                        g_print ("sranger_mk3_hwi_spm::ScanLineM(yindex=%d [fifo-y=%d], xdir=%d, ydir=%d, lssrcs=%x) y done.\n", yindex, data_y_index, xdir, ydir, muxmode);
                        return FALSE; // line completed top-down
                }
                if (ydir < 0 && yindex >= data_y_count){
                        g_print ("sranger_mk3_hwi_spm::ScanLineM(yindex=%d [fifo-y=%d], xdir=%d, ydir=%d, lssrcs=%x) y done.\n", yindex, data_y_index, xdir, ydir, muxmode);
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

gint spm_simulator_hwi_dev::RTQuery (const gchar *property, double &val1, double &val2, double &val3){
        if (*property == 'z'){
                double x = (double)(data_x_index-Nx/2)*Dx;
                double y = (double)(Ny/2-data_y_index)*Dy;
                Transform (&x, &y); // apply rotation!
		val1 =  main_get_gapp()->xsm->Inst->VZ() * data_z_value;
		val2 =  main_get_gapp()->xsm->Inst->VX() * (x + x0 ) * 10/32768; // assume "internal" non analog offset adding here (same gain)
                val3 =  main_get_gapp()->xsm->Inst->VY() * (y + y0 ) * 10/32768;
		return TRUE;
	}

	if (*property == 'o' || *property == 'O'){
		// read/convert and return offset
		// NEED to request 'z' property first, then this is valid and up-to-date!!!!
                // no offset simulated
		if (main_get_gapp()->xsm->Inst->OffsetMode () == OFM_ANALOG_OFFSET_ADDING){
			val1 =  main_get_gapp()->xsm->Inst->VZ0() * 0.;
			val2 =  main_get_gapp()->xsm->Inst->VX0() * x0*10/32768;
			val3 =  main_get_gapp()->xsm->Inst->VY0() * y0*10/32768;
		} else {
			val1 =  main_get_gapp()->xsm->Inst->VZ() * 0.;
			val2 =  main_get_gapp()->xsm->Inst->VX() * x0*10/32768;
			val3 =  main_get_gapp()->xsm->Inst->VY() * y0*10/32768;
		}
		
		return TRUE;
	}

        // ZXY in Angstroem
        if (*property == 'R'){
                // ZXY Volts after Piezoamp -- without analog offset -> Dig -> ZXY in Angstroem
		val1 = main_get_gapp()->xsm->Inst->V2ZAng (main_get_gapp()->xsm->Inst->VZ() * data_z_value);
		val2 = main_get_gapp()->xsm->Inst->V2XAng (main_get_gapp()->xsm->Inst->VX() * (double)(data_x_index-Nx/2)*Dx)*10/32768;
                val3 = main_get_gapp()->xsm->Inst->V2YAng (main_get_gapp()->xsm->Inst->VY() * (double)(Ny/2-data_y_index/2)*Dy)*10/32768;
		return TRUE;
        }

        if (*property == 'f'){
                val1 = 0.; // qf - DSPPACClass->pll.Reference[0]; // Freq Shift
		val2 = sim_current / main_get_gapp()->xsm->Inst->nAmpere2V(1.); // actual nA reading    xxxx V  * 0.1nA/V
		val3 = sim_current / main_get_gapp()->xsm->Inst->nAmpere2V(1.); // actual nA RMS reading    xxxx V  * 0.1nA/V -- N/A for simulation
		return TRUE;
	}

        if (*property == 'Z'){
                val1 = 0.; // spm_simulator_hwi_pi.app->xsm->Inst->Dig2ZA ((double)dsp_probe.Zpos / (double)(1<<16));
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
                val1 = (double)( data_x_index - (main_get_gapp()->xsm->data.s.nx/2 - 1) + 1);
                val2 = (double)(-data_y_index + (main_get_gapp()->xsm->data.s.ny/2 - 1) + 1);
                val3 = (double)data_z_value;
		return TRUE;
        }
        if (*property == 'P'){ // SCAN DATA INDEX, range: 0..NX, 0..NY
                val1 = (double)(data_x_index);
                val2 = (double)(data_y_index);
                val3 = (double)data_z_value;
		return TRUE;
        }
//	printf ("ZXY: %g %g %g\n", val1, val2, val3);

//	val1 =  (double)dsp_analog_out.z_scan;
//	val2 =  (double)dsp_analog_out.x_scan;
//	val3 =  (double)dsp_analog_out.y_scan;

	return TRUE;
}

