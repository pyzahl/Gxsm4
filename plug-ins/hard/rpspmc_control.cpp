/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: rpspmc_control.cpp
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
#include "rpspmc_control.h"
#include "rpspmc_pacpll.h"
#include "rpspmc_gvpmover.h"

#include "plug-ins/control/resonance_fit.h"

// Define HwI PlugIn reference name here, this is what is listed later within "Preferenced Dialog"
// i.e. the string selected for "Hardware/Card"!
#define THIS_HWI_PLUGIN_NAME "RPSPMC:SPM"
#define THIS_HWI_PREFIX      "RPSPMC_HwI"

#define GVP_SHIFT_UP  1
#define GVP_SHIFT_DN -1


typedef union {
        struct { unsigned char ch, x, y, z; } s;
        unsigned long   l;
} AmpIndex;



// helper func to assemble mux value from selctions
guint64 __GVP_selection_muxval (int selection[6]) {
        guint64 mux=0;
        for (int i=0; i<6; i++)
                if (selection[i] < 32)
                        mux |= (guint64)(selection[i] & 0x1f)<<(8*i); // regular MUX control
        //g_message ("_GVP_selection_muxval: %016lx", mux);
        return mux;
}


// defined in rpspmc_signals.cpp
extern SOURCE_SIGNAL_DEF rpspmc_source_signals[];
extern SOURCE_SIGNAL_DEF rpspmc_swappable_signals[];
extern SOURCE_SIGNAL_DEF modulation_targets[];
extern SOURCE_SIGNAL_DEF z_servo_current_source[];
extern SOURCE_SIGNAL_DEF rf_gen_out_dest[];
//

// inverse mux helper, mux val to selection
int __GVP_muxval_selection (guint64 mux, int k) {
        return  (mux >> (8*k)) & 0x1f;
}

// make MUX HVAL for TEST
int __GVP_selection_muxHval (int selection[6]) {
        int muxh=0;
        int axis_test=0;
        int axis_value=0;
        for (int i=0; i<6; i++)
                if (selection[i] >= 32)
                        switch (selection[i]-32){
                        case 0: axis_test = i+1; axis_value = 0; break;
                        case 1: axis_test = i+1; axis_value = 1; break;
                        case 2: axis_test = i+1; axis_value = -1; break;
                        case 3: axis_test = i+1; axis_value = 99; break;
                        case 4: axis_test = i+1; axis_value = -99; break;
                        default: continue;
                        }
        return ((axis_value & 0xfffffff) << 4) | (axis_test&0xf);
}

extern int debug_level;
extern int force_gxsm_defaults;


extern GxsmPlugin rpspmc_pacpll_hwi_pi;

extern rpspmc_hwi_dev  *rpspmc_hwi;

extern RPSPMC_Control  *RPSPMC_ControlClass;
extern RPspmc_pacpll   *rpspmc_pacpll;
extern GVPMoverControl *rpspmc_gvpmover;


//FIXME WARNING WARNING WARNING.. not working life if table is initialized with this
#define BiasFac    (main_get_gapp()->xsm->Inst->BiasGainV2V ())
#define CurrFac    (main_get_gapp()->xsm->Inst->V2nAmpere (1.))
#define ZAngFac    (main_get_gapp()->xsm->Inst->Volt2ZA (1.))
#define XAngFac    (main_get_gapp()->xsm->Inst->Volt2XA (1.))
#define YAngFac    (main_get_gapp()->xsm->Inst->Volt2YA (1.))





// signal definition tables
#include "rpspmc_signals.cpp"

/* **************************************** SPM Control GUI **************************************** */

// GUI Section -- custom to HwI
// ================================================================================

// advanced GUI building support and building automatizations

GtkWidget*  GUI_Builder::grid_add_mixer_options (gint channel, gint preset, gpointer ref){
        gchar *id;
        GtkWidget *cbtxt = gtk_combo_box_text_new ();

        g_message ("RPSPMC:: GUI_Builder::grid_add_mixer_options");
        
        g_object_set_data (G_OBJECT (cbtxt), "mix_channel", GINT_TO_POINTER (channel)); 
                                                                        
        id = g_strdup_printf ("%d", MM_OFF);        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "OFF"); g_free (id); 
        id = g_strdup_printf ("%d", MM_ON);         gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "LIN"); g_free (id); 
        id = g_strdup_printf ("%d", MM_ON | MM_LOG);          gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "LOG"); g_free (id); 
        //id = g_strdup_printf ("%d", MM_ON | MM_LOG | MM_FCZ); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "FCZ-LOG"); g_free (id); 
        //id = g_strdup_printf ("%d", MM_ON | MM_FCZ); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "FCZ-LIN"); g_free (id); 

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

        for (int jj=0; rpspmc_swappable_signals[jj].label; ++jj){
                gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, rpspmc_swappable_signals[jj].label); g_free (id);
        }
        
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


        for (int jj=0; rpspmc_swappable_signals[jj].label; ++jj){
                gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, rpspmc_swappable_signals[jj].label); g_free (id);
        }

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

GtkWidget* GUI_Builder::grid_add_modulation_target_options (gint channel, gint preset, gpointer ref){
        GtkWidget *cbtxt = gtk_combo_box_text_new (); 
        gtk_widget_set_size_request (cbtxt, 50, -1); 
        g_object_set_data(G_OBJECT (cbtxt), "mod_channel", GINT_TO_POINTER (channel)); 


        for (int jj=0;  modulation_targets[jj].label; ++jj){
                gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, modulation_targets[jj].label); g_free (id);
        }

        if (preset >= 0)
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), preset); 
        else
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 4); // NULL SIGNAL [TESTING FALLBACK for -1/error]
                
        g_signal_connect (G_OBJECT (cbtxt), "changed",	
                          G_CALLBACK (RPSPMC_Control::choice_mod_target_callback), 
                          ref);				
        grid_add_widget (cbtxt);
        return cbtxt;
};

GtkWidget* GUI_Builder::grid_add_z_servo_current_source_options (gint channel, gint preset, gpointer ref){
        GtkWidget *cbtxt = gtk_combo_box_text_new (); 
        gtk_widget_set_size_request (cbtxt, 50, -1); 
        g_object_set_data(G_OBJECT (cbtxt), "z_servo_current_source_id", GINT_TO_POINTER (channel)); 


        for (int jj=0; z_servo_current_source[jj].label; ++jj){
                gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, z_servo_current_source[jj].label); g_free (id);
        }

        if (preset >= 0)
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), preset); 
        else
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 4); // NULL SIGNAL [TESTING FALLBACK for -1/error]
                
        g_signal_connect (G_OBJECT (cbtxt), "changed",	
                          G_CALLBACK (RPSPMC_Control::choice_z_servo_current_source_callback), 
                          ref);				
        grid_add_widget (cbtxt);
        return cbtxt;
};

GtkWidget* GUI_Builder::grid_add_rf_gen_out_options (gint channel, gint preset, gpointer ref){
        GtkWidget *cbtxt = gtk_combo_box_text_new (); 
        gtk_widget_set_size_request (cbtxt, 50, -1); 
        g_object_set_data(G_OBJECT (cbtxt), "rf_gen_out_dest_id", GINT_TO_POINTER (channel)); 


        for (int jj=0; rf_gen_out_dest[jj].label; ++jj){
                gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id,  rf_gen_out_dest[jj].label); g_free (id);
        }

        if (preset >= 0)
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), preset); 
        else
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 4); // NULL SIGNAL [TESTING FALLBACK for -1/error]
                
        g_signal_connect (G_OBJECT (cbtxt), "changed",	
                          G_CALLBACK (RPSPMC_Control::choice_rf_gen_out_callback), 
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
        //grid_add_check_button_guint64 ("Oversampling", "Check do enable DSP data oversampling:\n"
        //                               "integrates data at intermediate points (averaging).\n"
        //                               "(recommended)", 1,
        //                               GCallback (option_cb), cb_data, option_flags, FLAG_INTEGRATE); 
        grid_add_check_button_guint64 ("Full Ramp", "Check do enable data acquisition on all probe segments:\n"
                                       "including start/end ramps and delays.", 1,
                                       GCallback (option_cb), cb_data, option_flags, FLAG_SHOW_RAMP); 
        // AUTO MODES
        grid_add_check_button_guint64 ("Auto Plot", "Enable life data plotting:\n"
                                       "disable to save resources and CPU time for huge data sets\n"
                                       "and use manual plot update if needed.", 1,
                                       GCallback (auto_cb), cb_data, auto_flags, FLAG_AUTO_PLOT); 
        grid_add_check_button_guint64 ("Auto Save", "Enable save data automatically at competion.\n"
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



void RPSPMC_Control::store_graphs_values (){
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
        PI_DEBUG_GM (DBG_L3, "RPSPMC_Control::store_graphs_values matrix complete.");
}

void RPSPMC_Control::restore_graphs_values (){
        PI_DEBUG_GM (DBG_L3, "RPSPMC_Control::restore_graphs_values matrix settings.");
        Source = g_settings_get_int (hwi_settings, "probe-sources");
        XSource = g_settings_get_int (hwi_settings, "probe-sources-x");
        XJoin = g_settings_get_boolean (hwi_settings, "probe-x-join");
        GrMatWin = g_settings_get_boolean (hwi_settings, "probe-graph-matrix-window");
        PSource = g_settings_get_int (hwi_settings, "probe-p-sources");
        PlotAvg = g_settings_get_int (hwi_settings, "probe-pavg-sources");
        PlotSec = g_settings_get_int (hwi_settings, "probe-psec-sources");

        for (int i=0; graphs_matrix[0][i]; ++i)
                if (graphs_matrix[0][i]){
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (graphs_matrix[0][i]),  Source & rpspmc_source_signals[i].mask);
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (graphs_matrix[1][i]), XSource & rpspmc_source_signals[i].mask);
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (graphs_matrix[2][i]), PSource & rpspmc_source_signals[i].mask);
                        // ..
                 }
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
        GVariant *pc_array_dam = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_dam, n, sizeof (double));
        GVariant *pc_array_dfm = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_dfm, n, sizeof (double));
        GVariant *pc_array_ts = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_ts, n, sizeof (double));
        GVariant *pc_array_pn = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_points, n, sizeof (gint32));
        GVariant *pc_array_op = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_opt, n, sizeof (gint32));
        GVariant *pc_array_vn = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_vnrep, n, sizeof (gint32));
        GVariant *pc_array_vp = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_vpcjr, n, sizeof (gint32));

        GVP_glock_data[0] = Source; GVP_glock_data[1] = XSource; GVP_glock_data[2] = PSource; GVP_glock_data[3] = XJoin; GVP_glock_data[4] = PlotAvg; GVP_glock_data[5] = PlotSec;
        GVariant *pc_array_grm = g_variant_new_fixed_array (g_variant_type_new ("t"), GVP_glock_data, 6, sizeof (guint64));
        
        GVariant *pc_array[] = { pc_array_du, pc_array_dx, pc_array_dy, pc_array_dz, pc_array_da, pc_array_db, pc_array_dam, pc_array_dfm,
                                 pc_array_ts,
                                 pc_array_pn, pc_array_op, pc_array_vn, pc_array_vp,
                                 pc_array_grm,
                                 NULL };
        const gchar *vckey[] = { "du", "dx", "dy", "dz", "da", "db", "dam", "dfm", "ts", "pn", "op", "vn", "vp", "grm", NULL };

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
        GVariant *vd[9];
        GVariant *vi[4];
        double *pc_array_d[9];
        gint32 *pc_array_i[5];
        const gchar *vckey_d[] = { "du", "dx", "dy", "dz", "da", "db", "dam", "dfm", "ts", NULL };
        const gchar *vckey_i[] = { "pn", "op", "vn", "vp", NULL };
        double *GVPd[] = { GVP_du, GVP_dx, GVP_dy, GVP_dz, GVP_da, GVP_db, GVP_dam, GVP_dfm, GVP_ts, NULL };
        gint32 *GVPi[] = { GVP_points, GVP_opt, GVP_vnrep, GVP_vpcjr, NULL };
        gint32 vp_program_length=0;
        
        for (int i=0; vckey_i[i]; ++i){
                gchar *m_vckey = g_strdup_printf ("%s-%s", vckey_i[i], key);
                g_print ( "GVP-VP restore %s\n", m_vckey);

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

        {
                gchar *m_vckey = g_strdup_printf ("%s-%s", "grm", key);
                g_print ( "GVP-VP restore %s\n", m_vckey);
                GVariant *viG;
                if ((viG = g_variant_dict_lookup_value (dict, m_vckey, ((const GVariantType *) "at"))) == NULL){
                        PI_DEBUG_GP (DBG_L2, "GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_i '%s' memo not found. Setting to Zero.\n", m_vckey);
                        // g_warning ("GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_i '%s' memo not found. Setting to Zero.\n", m_vckey);
                        g_free (m_vckey);
                } else {
                        g_print ("GVP restore: %s = %s\n", m_vckey, g_variant_print (viG, true));
                        guint64 *gvp_grm_array_i =  (guint64*) g_variant_get_fixed_array (viG, &n, sizeof (gint64));
                        for (int k=0; k<n && k<6; ++k)
                                GVP_glock_data[k]=gvp_grm_array_i[k];
                }
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

        Source = GVP_glock_data[0]; XSource = GVP_glock_data[1]; PSource = GVP_glock_data[2]; XJoin = GVP_glock_data[3]; PlotAvg = GVP_glock_data[4];  PlotSec = GVP_glock_data[5];
        // update Graphs
        for (int i=0; graphs_matrix[0][i]; ++i)
                if (graphs_matrix[0][i]){
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (graphs_matrix[0][i]),  Source & rpspmc_source_signals[i].mask?true:false);
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (graphs_matrix[1][i]), XSource & rpspmc_source_signals[i].mask?true:false);
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (graphs_matrix[2][i]), PSource & rpspmc_source_signals[i].mask?true:false);
                        // ..
                 }


        
	update_GUI ();
}

int RPSPMC_Control::callback_edit_GVP (GtkWidget *widget, RPSPMC_Control *self){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
        int x=1, y=10;
	int ki = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "VPC"));
	int  a = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "ARROW"));

	if (ki < 0){
		const int VPT_YPAD=0;
		int c = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(self->VPprogram[0]), "CF"));
		int cw = 1;
		if (self->VPprogram[1]){
			for (int j=1; j<10; ++j)
				if (self->VPprogram[j]){
                                        // FIX-ME GTK4 ???
					gtk_window_destroy (GTK_WINDOW (self->VPprogram[j]));
					self->VPprogram[j] = NULL;
				}
		} else
			for (int k=0; k<N_GVP_VECTORS && cw < 10; ++k){
				if (self->GVP_vpcjr[k] < 0){
					int kf=k;
					int ki=k+self->GVP_vpcjr[k];
					if (kf >= 0){
                                                // fix!! todo
						//** ADD_BUTTON_GRID ("arrow-up-symbolic", "Loop",   self->VPprogram[0], x+0, y+ki+1, 1, kf-ki+2, -2, NULL, NULL, self->VPprogram[cw]);
                                                //ADD_BUTTON_TAB(GTK_ARROW_UP, "Loop",   self->VPprogram[0], c+0, c+1, ki+1, kf+2, GTK_FILL, GTK_FILL, 0, VPT_YPAD, -1, -2, NULL, NULL, self->VPprogram[cw]);
						++c; ++cw;
					}
				}
			}
		return 0;
	}

	if (a == GVP_SHIFT_UP && ki >= 1 && ki < N_GVP_VECTORS)
		for (int k=ki-1; k < N_GVP_VECTORS-1; ++k){
			int ks = k+1;
			self->GVP_du[k] = self->GVP_du[ks];
			self->GVP_dx[k] = self->GVP_dx[ks];
			self->GVP_dy[k] = self->GVP_dy[ks];
			self->GVP_dz[k] = self->GVP_dz[ks];
			self->GVP_da[k] = self->GVP_da[ks];
			self->GVP_db[k] = self->GVP_db[ks];
			self->GVP_dam[k] = self->GVP_dam[ks];
			self->GVP_dfm[k] = self->GVP_dfm[ks];
			self->GVP_ts[k]  = self->GVP_ts[ks];
			self->GVP_points[k] = self->GVP_points[ks];
			self->GVP_opt[k] = self->GVP_opt[ks];
			self->GVP_vnrep[k] = self->GVP_vnrep[ks];
			self->GVP_vpcjr[k] = self->GVP_vpcjr[ks];
		} 
	else if (a == GVP_SHIFT_DN && ki >= 0 && ki < N_GVP_VECTORS-2)
		for (int k=N_GVP_VECTORS-1; k > ki; --k){
			int ks = k-1;
			self->GVP_du[k] = self->GVP_du[ks];
			self->GVP_dx[k] = self->GVP_dx[ks];
			self->GVP_dy[k] = self->GVP_dy[ks];
			self->GVP_dz[k] = self->GVP_dz[ks];
			self->GVP_da[k] = self->GVP_da[ks];
			self->GVP_db[k] = self->GVP_db[ks];
			self->GVP_dam[k] = self->GVP_dam[ks];
			self->GVP_dfm[k] = self->GVP_dfm[ks];
			self->GVP_ts[k]  = self->GVP_ts[ks];
			self->GVP_points[k] = self->GVP_points[ks];
			self->GVP_opt[k] = self->GVP_opt[ks];
			self->GVP_vnrep[k] = self->GVP_vnrep[ks];
			self->GVP_vpcjr[k] = self->GVP_vpcjr[ks];
		}
	self->update_GUI ();
        return 0;
}


// NetCDF support for parameter storage to file

// helper func
void rpspmc_pacpll_hwi_ncaddvar (NcFile &ncf, const gchar *varname, const gchar *varunit, const gchar *longname, const gchar *shortname, const gchar *label, double value, NcVar &ncv){
	ncv = ncf.addVar (varname, ncDouble);
	ncv.putAtt ("long_name", longname);
	ncv.putAtt ("short_name", shortname);
	ncv.putAtt ("var_unit", varunit);
	if (label) ncv.putAtt ("label", label);
	ncv.putVar (&value);
}

void* rpspmc_pacpll_hwi_ncaddvar (NcFile &ncf, const gchar *varname, const gchar *varunit, const gchar *longname, const gchar *shortname, const gchar *label, int value, NcVar &ncv){
	ncv = ncf.addVar (varname, ncInt);
	ncv.putAtt ("long_name", longname);
	ncv.putAtt ("short_name", shortname);
	ncv.putAtt ("var_unit", varunit);
	if (label) ncv.putAtt ("label", label);
	ncv.putVar (&value);
}

#define SPMHWI_ID "rpspmc_hwi_"

void RPSPMC_Control::save_values (NcFile &ncf){
	NcVar ncv;

	PI_DEBUG (DBG_L4, "RPSPMC_Control::save_values");
	gchar *i=NULL;

        i = g_strconcat ("RP SPM Control HwI ** Hardware-Info:\n", rpspmc_hwi->get_info (), NULL);

	NcDim infod  = ncf.addDim("rpspmc_info_dim", strlen(i));
	NcVar info   = ncf.addVar("rpspmc_info", ncChar, infod);
	info.putAtt ("long_name", "RPSPMC_Control plugin information");
	info.putVar ({0}, {strlen(i)}, i);
	g_free (i);

// Feedback/Scan Parameters from GUI ============================================================

	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"Bias", "V", "RPSPMC: (Sampel or Tip) Bias Voltage", "Bias", "Bias", bias, ncv);
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"Z_Servo_CZ_SetPoint", "A", "RPSPMC: Z-Servo CZ Setpoint", "CZ Set Point", "Z Setpoint", zpos_ref, ncv);
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"Z_Servo_CP_in_dB", "dB", "RPSPMC: Z-Servo CP in dB", "Z SERVO CP in dB", "Z Servo CP (dB)", spmc_parameters.z_servo_cp_db, ncv);
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"Z_Servo_CI_in_dB", "dB", "RPSPMC: Z-Servo CI in dB", "Z SERVO CI in dB", "Z Servo CI (dB)", spmc_parameters.z_servo_ci_db, ncv);
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"Z_Servo_SetPoint", "nA", "RPSPMC: Z-Servo STM/Current/.. Set Point", "Current Setpt.", "Current", mix_set_point[0], ncv);
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"Z_Servo_FLevel", "1", "Z-Servo RPSPMC: FLevel", "Current/.. level", "Level", mix_level[0], ncv);
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"Z_Servo_Transfer_Mode", "BC", "RPSPMC: Z-Servo Transfer Mode", "Z-Servo Transfer Mode", NULL, (double)mix_transform_mode[0], ncv);
	ncv.putAtt ("mode_bcoding", "0:Off, 1:On, 2:Log");

	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"scan_speed_x", "A/s", "RPSPMC: Scan speed X", "Xs Velocity", "Scan Speed", scan_speed_x, ncv);
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"move_speed_x", "A/s", "RPSPMC: Move speed X", "Xm Velocity", "Move Speed", move_speed_x, ncv);

// LockIn Parameters from GUI ============================================================
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"LCK_Mod", "Hz", "RPSPMC: LockIn Mod Freq", "LockIn Mod Freq", "Lck Mod Freq", spmc_parameters.lck_frequency, ncv);

        int jj = spmc_parameters.lck_target;
        //for (int jj=1; modulation_targets[jj].label && jj < LCK_NUM_TARGETS; ++jj)
        {
                gchar *lab = g_strdup_printf ("Modulation Volume for %s", modulation_targets[jj].label);
                gchar *id = g_strdup_printf ("%sLCK_ModVolume_%s", SPMHWI_ID, modulation_targets[jj].label);
                for (gchar *c=id; *c; c++) if (*c == '-') *c='_'; // transcode '-' to '_'
                rpspmc_pacpll_hwi_ncaddvar (ncf, id, modulation_targets[jj].unit, lab, lab, lab, LCK_Volume[jj], ncv);
                g_free (lab); g_free(id);
        }
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"LCK_Sens", "mV", "RPSPMC: LockIn Sensitivity", "LockIn Sens", "Lck Sens", spmc_parameters.lck_sens, ncv);

        
// Filter Parameters from GUI ============================================================

        
        
// Vector Probe ============================================================

}


#define NC_GET_VARIABLE(VNAME, VAR) if(!ncf.getVar(VNAME).isNull ()) ncf.getVar(VNAME).getVar(VAR)

void RPSPMC_Control::load_values (NcFile &ncf){
	PI_DEBUG (DBG_L4, "RPSPMC_Control::load_values");
	// Values will also be written in old style DSP Control window for the reason of backwards compatibility
	// OK -- but will be obsoleted and removed at any later point -- PZ
	NC_GET_VARIABLE ("rpspmc_pacpll_hwi_bias", &bias);
	NC_GET_VARIABLE ("rpspmc_pacpll_hwi_bias", &main_get_gapp()->xsm->data.s.Bias);
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

        rpspmc_hwi->update_hardware_mapping_to_rpspmc_source_signals ();
        
	return 0;
}

int RPSPMC_Control::choice_VGain_callback (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	gint i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	gint j = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "VGindex"));
	switch(j){
	case 0: double g = rpspmc_pacpll_hwi_pi.app->xsm->Inst->set_current_gain_modifier (i);
                g_message ("Adjusting IVC VGain: %f nV/A x pos[%d] %f => %f nV/A", xsmres.nAmpere2Volt, i, xsmres.VG[i], g);
		break;
	}
        
        self->ZServoParamChanged(NULL, self);

        rpspmc_hwi->update_hardware_mapping_to_rpspmc_source_signals ();
        
	return 0;
}

// No-op to prevent w from propagating "scroll" events it receives.

void disable_scroll_cb( GtkWidget *w ) {}

void RPSPMC_Control::create_folder (){
        GtkWidget *notebook;
 	GSList *zpos_control_list=NULL;
	GSList *multi_IVsec_list=NULL;

        GSList *FPGA_readback_update_list=NULL;
        GSList *EC_FPGA_SPMC_server_settings_list=NULL;
        GSList *EC_MONITORS_list = NULL;
        
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

	bp->new_grid_with_frame ("SPM Controls",2);
        
        // ------- FB Characteristics
        bp->start (); // start on grid top and use widget grid attach nx=4
        bp->set_scale_nx (4); // set scale width to 4
        bp->set_input_width_chars (12);
        bp->set_label_width_chars (10);

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::BiasChanged, this);
        bp->grid_add_ec_with_scale ("Bias", Volt, &bias, -10., 10., "4g", 0.001, 0.01, "fbs-bias");
        FPGA_readback_update_list = g_slist_prepend (FPGA_readback_update_list, bp->ec); // add to FPGA reconnect readback parameter list
        //        bp->ec->set_adjustment_mode (PARAM_CONTROL_ADJUSTMENT_LOG | PARAM_CONTROL_ADJUSTMENT_LOG_SYM | PARAM_CONTROL_ADJUSTMENT_DUAL_RANGE | PARAM_CONTROL_ADJUSTMENT_ADD_MARKS ); // LIN:0, LOG:1, SYM:2, DR: 4, TICS:8
        bp->ec->set_logscale_min (1e-3);

        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);
        bp->ec->SetScaleWidget (bp->scale, 0);
        bp->new_line ();

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ZPosSetChanged, this);
        //bp->set_input_width_chars (20);
        //bp->set_input_nx (2);
        bp->grid_add_ec_with_scale ("Z-Pos/Setpoint", Angstroem, &zpos_ref, -1000., 1000., "6g", 0.01, 0.1, "adv-dsp-zpos-ref");
        FPGA_readback_update_list = g_slist_prepend (FPGA_readback_update_list, bp->ec); // add to FPGA reconnect readback parameter list
        bp->ec->SetScaleWidget (bp->scale, 0);
        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);
        //ZPos_ec = bp->ec;
        bp->grid_add_ec ("", Angstroem, &zpos_mon, -1000., 1000., "6g", 0.01, 0.1, "adv-dsp-zpos-mon");
        ZPos_ec = bp->ec;
        zpos_control_list = g_slist_prepend (zpos_control_list, bp->ec);

        bp->set_input_width_chars (12);
        bp->set_input_nx ();

        #if 0
	bp->grid_add_check_button ("Z-Pos Monitor",
                                   "Z Position Monitor. Disable to set Z-Position Setpoint for const height mode. "
                                   "Switch Transfer to CZ-FUZZY-LOG for Z-CONTROL Constant Heigth Mode Operation with current compliance given by Fuzzy-Level. "
                                   "Slow down feedback to minimize instabilities. "
                                   "Set Current setpoint a notch below compliance level.",
                                   1,
                                   G_CALLBACK (RPSPMC_Control::zpos_monitor_callback), this,
                                   0, 0);
        GtkWidget *zposmon_checkbutton = bp->button;
        #endif
        bp->new_line ();

        bp->set_configure_list_mode_off ();

        // MIXER headers

        bp->set_input_width_chars (12);
        bp->set_label_width_chars (10);
        bp->set_configure_list_mode_on ();
	bp->grid_add_label ("Z-Servo");
        z_servo_current_source_options_selector = bp->grid_add_z_servo_current_source_options (0, (int)spmc_parameters.z_servo_src_mux, this);
        bp->set_configure_list_mode_off ();

        bp->grid_add_label ("Setpoint", NULL, 4);

        bp->set_configure_list_mode_on ();
        bp->set_input_width_chars (6);
        bp->set_label_width_chars (6);
        //bp->grid_add_label ("Gain");

        bp->set_configure_list_mode_on ();
        bp->grid_add_label ("In-Offset");
        bp->set_configure_list_mode_off ();
        bp->grid_add_label ("Fuzzy-Level");
        bp->set_configure_list_mode_on ();
        bp->grid_add_label ("Transfer");
        bp->set_configure_list_mode_off ();

        bp->new_line ();

        bp->set_scale_nx (4);

        // Build MIXER CHANNELs
        UnitObj *mixer_unit[4] = { Current, Frq, Volt, Volt };
        const gchar *mixer_channel_label[4] = { "STM Current", "AFM/Force", "Mix2", "Mix3" };
        const gchar *mixer_remote_id_set[4] = { "fbs-mx0-current-set",  "fbs-mx1-freq-set",   "fbs-mx2-set",   "fbs-mx3-set" };
        const gchar *mixer_remote_id_gn[4]  = { "fbs-mx0-current-gain" ,"fbs-mx1-freq-gain",  "fbs-mx2-gain",  "fbs-mx3-gain" };
        const gchar *mixer_remote_id_oc[4]  = { "fbs-mx0-current-oc" ,"fbs-mx1-freq-oc",  "fbs-mx2-oc",  "fbs-mx3-oc" };
        const gchar *mixer_remote_id_fl[4]  = { "fbs-mx0-current-level","fbs-mx1-freq-level", "fbs-mx2-level", "fbs-mx3-level" };

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ZServoParamChanged, this);
        // Note: transform mode is always default [LOG,OFF,OFF,OFF] -- NOT READ BACK FROM DSP -- !!!
        for (gint ch=0; ch<1; ++ch){

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
                FPGA_readback_update_list = g_slist_prepend (FPGA_readback_update_list, bp->ec); // add to FPGA reconnect readback parameter list
                // bp->ec->set_adjustment_mode (PARAM_CONTROL_ADJUSTMENT_LOG | PARAM_CONTROL_ADJUSTMENT_ADD_MARKS );
                bp->ec->set_logscale_min (1e-4);
                bp->ec->set_logscale_magshift (-3);
                bp->ec->SetScaleWidget (bp->scale, 0);
                gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);

                if (signal_select_widget)
                        g_object_set_data (G_OBJECT (signal_select_widget), "related_ec_setpoint", bp->ec);

                
                // bp->add_to_configure_hide_list (bp->scale);
                bp->set_input_width_chars (6);
                bp->set_configure_list_mode_on ();
                //bp->grid_add_ec (NULL, Unity, &mix_gain[ch], -1.0, 1.0, "5g", 0.001, 0.01, mixer_remote_id_gn[ch]);
                //bp->set_configure_list_mode_on ();
                bp->grid_add_ec (NULL, mixer_unit[ch], &mix_in_offsetcomp[ch], -1.0, 1.0, "5g", 0.001, 0.01, mixer_remote_id_oc[ch]);
                bp->set_configure_list_mode_off ();
                bp->grid_add_ec (NULL, mixer_unit[ch], &mix_level[ch], -100.0, 100.0, "5g", 0.001, 0.01, mixer_remote_id_fl[ch]);
                bp->set_configure_list_mode_on ();
                if (tmp) delete (tmp); // done setting unit -- if custom
                
                if (signal_select_widget)
                        g_object_set_data (G_OBJECT (signal_select_widget), "related_ec_level", bp->ec);
                
                z_servo_options_selector[ch] = bp->grid_add_mixer_options (ch, mix_transform_mode[ch], this);
                bp->set_configure_list_mode_off ();
                bp->new_line ();
        }
        //bp->set_configure_list_mode_off ();

        bp->set_input_width_chars ();
        
        // ========== Z-Servo
        // bp->pop_grid ();
        //bp->new_line ();
        //bp->new_grid_with_frame ("Z-Servo",2);

        bp->set_label_width_chars (7);
        bp->set_input_width_chars (12);

        bp->set_configure_list_mode_on ();
	bp->grid_add_ec_with_scale ("CP", dB, &spmc_parameters.z_servo_cp_db, -100., 20., "5g", 1.0, 0.1, "fbs-cp"); // z_servo[SERVO_CP]
        GtkWidget *ZServoCP = bp->input;
        FPGA_readback_update_list = g_slist_prepend (FPGA_readback_update_list, bp->ec); // add to FPGA reconnect readback parameter list
        bp->set_configure_list_mode_off ();

        mix_level[2] = 5.;
        mix_level[3] = -5.;
        // *** spmc_parameters.z_servo_upper *** readback also, alt control via Z-Position, so must unlink here
        bp->set_configure_list_mode_on ();
        bp->grid_add_ec ("Z Upper", Volt, &z_limit_upper_v, -5.0, 5.0, "5g", 0.001, 0.01,"fbs-upper");
        bp->set_configure_list_mode_off ();
        FPGA_readback_update_list = g_slist_prepend (FPGA_readback_update_list, bp->ec); // add to FPGA reconnect readback parameter list
        ec_z_upper = bp->ec;
        
        bp->new_line ();
        bp->grid_add_ec_with_scale ("CI", dB, &spmc_parameters.z_servo_ci_db, -100., 20., "5g", 1.0, 0.1, "fbs-ci"); // z_servo[SERVO_CI
        GtkWidget *ZServoCI = bp->input;
        FPGA_readback_update_list = g_slist_prepend (FPGA_readback_update_list, bp->ec); // add to FPGA reconnect readback parameter list

        g_object_set_data( G_OBJECT (ZServoCI), "HasClient", ZServoCP);
        g_object_set_data( G_OBJECT (ZServoCP), "HasMaster", ZServoCI);
        g_object_set_data( G_OBJECT (ZServoCI), "HasRatio", GINT_TO_POINTER((guint)round(1000.*spmc_parameters.z_servo_cp_db/spmc_parameters.z_servo_ci_db)));
        
        bp->set_configure_list_mode_on ();
        bp->grid_add_ec ("Z Lower", Volt, &spmc_parameters.z_servo_lower, -5.0, 5.0, "5g", 0.001, 0.01,"fbs-lower");
        bp->set_configure_list_mode_off ();
        FPGA_readback_update_list = g_slist_prepend (FPGA_readback_update_list, bp->ec); // add to FPGA reconnect readback parameter list
        
        bp->new_line ();
        bp->set_label_width_chars ();

//#define ENABLE_ZSERVO_POLARITY_BUTTON  // only for development and testing
        // Z OUPUT POLARITY:
        spmc_parameters.z_polarity      = xsmres.ScannerZPolarity ? 1 : -1;    // 1: pos, 0: neg (bool) -- adjust zpos_ref accordingly!
        spmc_parameters.gxsm_z_polarity = -(xsmres.ScannerZPolarity ? 1 : -1); // Gxsm: negative internally required to have native Z positive or up = tip up or higher surface

#ifdef  ENABLE_ZSERVO_POLARITY_BUTTON
        bp->grid_add_check_button ("Enable", "enable Z servo feedback controller." PYREMOTE_CHECK_HOOK_KEY("MainZservo"), 1,
                                   G_CALLBACK(RPSPMC_Control::ZServoControl), this, ((int)spmc_parameters.gvp_status)&1, 0);


        bp->grid_add_check_button ( N_("Invert (Neg Polarity)"), "Z-Output Polarity. ** startup setting, does NOT for sure reflect actual setting unless toggled!", 1,
                                    G_CALLBACK (RPSPMC_Control::ZServoControlInv), this, spmc_parameters.z_polarity > 1 ? 0:1 );

#endif


	// ========================================

        PI_DEBUG (DBG_L4, "SPMC----SCAN ------------------------------- ");
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Scan Characteristics",2);

        bp->start (4); // wx=4
        bp->set_label_width_chars (7);

        bp->set_configure_list_mode_on ();

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ChangedNotifyMoveSpeed, this);
	bp->grid_add_ec_with_scale ("MoveSpd", Speed, &move_speed_x, 0.1, 10000., "5g", 1., 10., "fbs-scan-speed-move");
        bp->ec->set_logscale_min (1);
        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);
        bp->ec->SetScaleWidget (bp->scale, 0);
        bp->new_line ();
        bp->set_configure_list_mode_off ();

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ChangedNotifyScanSpeed, this);
	bp->grid_add_ec_with_scale ("ScanSpd", Speed, &scan_speed_x_requested, 0.1, 100000., "5g", 1., 10., "fbs-scan-speed-scan");
        bp->ec->set_logscale_min (1);
        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);
        bp->ec->SetScaleWidget (bp->scale, 0);
        scan_speed_ec = bp->ec;

        bp->new_line ();
	bp->grid_add_ec ("Fast Return", Unity, &fast_return, 1., 1000., "5g", 1., 10.,  "adv-scan-fast-return");

#if 0
        bp->set_configure_list_mode_on ();
	bp->grid_add_ec ("XS 2nd ZOff", Angstroem, &x2nd_Zoff, -10000., 10000., ".2f", 1., 1., "adv-scan-xs2nd-z-offset");
        bp->set_configure_list_mode_off ();
#endif

        bp->new_line ();
        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::Slope_dZX_Changed, this);
        bp->grid_add_ec_with_scale ("Slope X", Unity, &area_slope_x, -0.2, 0.2, ".5f", 0.0001, 0.0001,  "adv-scan-slope-x"); slope_x_ec = bp->ec;
        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);
        bp->new_line ();

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::Slope_dZY_Changed, this);
        bp->grid_add_ec_with_scale ("Slope Y", Unity, &area_slope_y, -0.2, 0.2, ".5f", 0.0001, 0.0001,  "adv-scan-slope-y"); slope_y_ec = bp->ec;
        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);

        bp->set_scale_nx ();
        bp->new_line (0,2);

        //bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ChangedNotify, this); // set default handler

        //bp->grid_add_check_button ("Enable automatic Tip return to center", "enable auto tip return to center after scan", 1,
        //                               G_CALLBACK(RPSPMC_Control::DSP_cret_callback), this, center_return_flag, 0);
        //g_settings_bind (hwi_settings, "adv-enable-tip-return-to-center",
        //                 G_OBJECT (GTK_CHECK_BUTTON (bp->button)), "active",
        //                 G_SETTINGS_BIND_DEFAULT);

        PI_DEBUG (DBG_L4, "SPMC----FB-CONTROL -- INPUT-SRCS ----------------------------- ");

	// ========== SCAN CHANNEL INPUT SOURCE CONFIGURATION MENUS
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Configure Scan Source GVP MUX Selectors (Swappable Signals)",2);

        bp->set_configure_list_mode_on ();
        bp->add_to_configure_list (bp->frame); // manage en block
        bp->set_configure_list_mode_off ();
        
        bp->new_line ();

        for (int i=0; i<6; ++i){
                bp->grid_add_scan_input_signal_options (i, scan_source[i], this);
                VPScanSrcVPitem[i] =  bp->wid;
        }

#if 0
        // ========== LDC -- Enable Linear Drift Correction -- Controls
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Enable Linear Drift Correction (LDC) -- N/A on RPSPMC at this time",2);
        bp->set_configure_list_mode_on ();

	LDC_status = bp->grid_add_check_button ("Enable Linear Drift Correction","Enable Linear Drift Correction" PYREMOTE_CHECK_HOOK_KEY("MainLDC"), 3);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (LDC_status), 0);
	ldc_flag = 0;
	g_signal_connect (G_OBJECT (LDC_status), "toggled",
			    G_CALLBACK (RPSPMC_Control::ldc_callback), this);


        // LDC settings
	bp->grid_add_ec ("LDCdX", Speed, &dxdt, -100., 100., "5g", 0.001, 0.01, "fbs-scan-ldc-dx");
        bp->grid_add_ec ("dY", Speed, &dydt, -100., 100., "5g", 0.001, 0.01, "fbs-scan-ldc-dy");
        bp->grid_add_ec ("dZ", Speed, &dzdt, -100., 100., "5g", 0.001, 0.01, "fbs-scan-ldc-dz");

        bp->set_configure_list_mode_off ();

#endif

	// ========== Piezo Drive / Amplifier Settings
        bp->set_input_width_chars (8);
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Piezo Drive Settings",1);
        bp->add_to_scan_freeze_widget_list (bp->frame);

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


	// ========== Various Variable Gain Settings
        bp->set_input_width_chars (8); // bp->set_xy (2,4);
        bp->new_grid_with_frame ("Gain Settings",1);
        bp->add_to_scan_freeze_widget_list (bp->frame);

        const gchar *V_gain_label[] = { "VG-IVC", NULL };
        const gchar *V_gain_unit[]  = { "V/nA", NULL };
        const gchar *V_gain_key[]   = { "vgivc", NULL };
        for(int j=0; V_gain_label[j]; j++) {
                gtk_label_set_width_chars (GTK_LABEL (bp->grid_add_label (V_gain_label[j])), 6);
        
                GtkWidget *wid = gtk_combo_box_text_new ();
                g_object_set_data  (G_OBJECT (wid), "VGindex", GINT_TO_POINTER (j));

                // Init gain-choicelist
                for(int ig=0; xsmres.VG[ig] > 0.0; ig++){
                        gchar *V_gain_value = g_strdup_printf ("%.2E V/A [x%g]", 1e9*xsmres.nAmpere2Volt*xsmres.VG[ig], xsmres.VG[ig]);
                        gchar *id = g_strdup_printf ("%02d:%02d",j,ig);
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), id, V_gain_value);
                        g_free (id);
                        g_free (V_gain_value);
                }

                g_signal_connect (G_OBJECT (wid), "changed",
                                  G_CALLBACK (RPSPMC_Control::choice_VGain_callback),
                                  this);

                gchar *tmpkey = g_strconcat ("spm-rpspmc-", V_gain_key[j], NULL); 
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
        bp->new_grid (); // x=y=1  set_xy (x,y)
        bp->start_notebook_tab (notebook, "Lock-In", "rpspmc-tab-lockin", hwi_settings);

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::lockin_adjust_callback, this);
 	bp->new_grid_with_frame ("Lock-In Control and Routing");

        LCK_unit = new UnitAutoMag ("V","V");
        LCK_unit->add_ref ();
        bp->grid_add_ec ("Magnitude Reading", LCK_unit, &spmc_parameters.lck1_bq2_mag_monitor, -10.0, 10.0, ".03g", 0.1, 1., "LCK-MAG-MONITOR");
        LCK_Reading = bp->ec;
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();

        bp->grid_add_ec ("Phase Reading", Deg, &spmc_parameters.lck1_bq2_ph_monitor, -180.0, 180.0, ".03g", 0.1, 1., "LCK-PH-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();

        bp->grid_add_ec ("aCLKs/S", Unity, &spmc_parameters.lck_aclocks_per_sample_monitor, 0.0, 1e6, "6.0f", 0.1, 1., "LCK-ACLK-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();

        bp->grid_add_ec ("LCK-Decii", Unity, &spmc_parameters.lck_decii_monitor, 0.0, 1e6, "6.0f", 0.1, 1., "LCK-DECII-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();

        bp->grid_add_ec ("LCK-ilen", Unity, &spmc_parameters.lck_ilen_monitor, 0.0, 1e6, "6.0f", 0.1, 1., "LCK-ILEN-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();

        bp->grid_add_ec ("BQDec#", Unity, &spmc_parameters.lck_bq_dec_monitor, 0.0, 1e6, "6.0f", 0.1, 1., "LCK-BQDEC-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();

	bp->grid_add_ec ("Modulation Frequency", new UnitObj("Hz","Hz"), &spmc_parameters.lck_frequency, 0.0, 10e6, "5g", 1.0, 100.0, "SPMC-LCK-FREQ");
        LCK_ModFrq = bp->ec;
        
        bp->new_line ();
        bp->grid_add_label ("Modulation on");
        bp->grid_add_modulation_target_options (0, (int)spmc_parameters.lck_target, this);

        LCK_Volume[0]=0.0; // #0 is not used
        for (int jj=1; modulation_targets[jj].label && jj < LCK_NUM_TARGETS; ++jj){
                bp->new_line ();
                gchar *lab = g_strdup_printf ("Volume for %s", modulation_targets[jj].label);
                gchar *id = g_strdup_printf ("SPMC-LCK-ModVolume-%s", modulation_targets[jj].label);
                UnitObj *u = new UnitObj(modulation_targets[jj].unit_sym, modulation_targets[jj].unit);
                bp->grid_add_ec (lab, u, &LCK_Volume[jj], 0., 1e6, "5g", 0.001, 0.01, id);
                g_free (lab);
                g_free (id);
                LCK_VolumeEntry[jj]=bp->input;
                gtk_widget_set_sensitive (bp->input, false);
        }

        bp->new_line ();
        lck_gain=1.;
	bp->grid_add_ec ("Lck Sens.", new UnitObj("mV","mV"), &spmc_parameters.lck_sens, 0.001, 5000., "5g", 1.0, 10.0, "SPMC-LCK-SENS");
        LCK_Sens = bp->ec;
        bp->new_line ();
        bp->grid_add_check_button ("CorrS PH Aligned", bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Lock-In Corr Phase Aligned","dsp-lck-phaligned"), 1,
                                   GCallback (callback_change_LCK_mode), this,
                                   &spmc_parameters.lck_mode, 4);

        // ***
        bp->pop_grid (); bp->set_xy (2,2);
 	bp->new_grid_with_frame ("Filter Section 1");

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::bq_filter_adjust_callback, this);

        bp->grid_add_label ("Filter type");

        const gchar *filter_types[] = { "None/Pass", "--", "--", "From AB", "Stop", "By-Pass", "Disable", NULL };

        GtkWidget *combo_bqfilter_type = gtk_combo_box_text_new ();
        for (int jj=0; filter_types[jj]; ++jj){
                gchar *id = g_strdup_printf ("%d", jj);
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo_bqfilter_type), id, filter_types[jj]);
                g_free (id);
        }
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo_bqfilter_type), 3);
        update_bq_filterS1_widget = combo_bqfilter_type;

        g_object_set_data (G_OBJECT (combo_bqfilter_type), "section", GINT_TO_POINTER (1)); 
        g_signal_connect (G_OBJECT (combo_bqfilter_type), "changed",
                          G_CALLBACK (RPSPMC_Control::choice_BQfilter_type_callback),
                          this);
        bp->grid_add_widget (combo_bqfilter_type);
        bp->new_line ();
        
        UnitObj *unity_unit = new UnitObj(" "," ");
        for(int jj=0; jj<6; ++jj){
                spmc_parameters.sc_bq_coef[jj]=0.;
                spmc_parameters.sc_bq_coef[0]=1.;
                gchar *l = g_strdup_printf ("BQ1 %s%d", jj<3?"b":"a", jj<3?jj:jj-3);
                gchar *id = g_strdup_printf ("SPMC-LCK-BQ1-COEF-BA");
                bp->grid_add_ec (l, unity_unit, &spmc_parameters.sc_bq1_coef[jj], -1e10, 1e10, "g", 1., 10., id, jj);
                if (jj==5) bp->init_ec_array ();
                g_object_set_data (G_OBJECT (combo_bqfilter_type), id, bp->ec);
                g_free (l);
                g_free (id);
                bp->new_line ();
        }	

        //
        bp->pop_grid (); bp->set_xy (3,2);
 	bp->new_grid_with_frame ("Filter Section 2");

        bp->grid_add_label ("Filter type");
        combo_bqfilter_type = gtk_combo_box_text_new ();
        for (int jj=0; filter_types[jj]; ++jj){
                gchar *id = g_strdup_printf ("%d", jj);
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo_bqfilter_type), id, filter_types[jj]);
                g_free (id);
        }
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo_bqfilter_type), 3);
        update_bq_filterS2_widget = combo_bqfilter_type;
        g_object_set_data (G_OBJECT (combo_bqfilter_type), "section", GINT_TO_POINTER (2)); 
        g_signal_connect (G_OBJECT (combo_bqfilter_type), "changed",
                          G_CALLBACK (RPSPMC_Control::choice_BQfilter_type_callback),
                          this);
        bp->grid_add_widget (combo_bqfilter_type);
        bp->new_line ();

        for(int jj=0; jj<6; ++jj){
                spmc_parameters.sc_bq_coef[jj]=0.;
                spmc_parameters.sc_bq_coef[0]=1.;
                gchar *l = g_strdup_printf ("BQ2 %s%d", jj<3?"b":"a", jj<3?jj:jj-3);
                gchar *id = g_strdup_printf ("SPMC-LCK-BQ2-COEF-BA");
                bp->grid_add_ec (l, unity_unit, &spmc_parameters.sc_bq2_coef[jj], -1e10, 1e10, "g", 1., 10., id, jj);
                if (jj==5) bp->init_ec_array ();
                g_object_set_data (G_OBJECT (combo_bqfilter_type), id, bp->ec);
                g_free (l);
                g_free (id);
                bp->new_line ();
        }	
        
        //
        bp->pop_grid (); bp->set_xy (4,2);
 	bp->new_grid_with_frame ("Z Servo Input BiQuad Filter");

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::zs_input_filter_adjust_callback, this);
       
        bp->grid_add_label ("Filter type");
        combo_bqfilter_type = gtk_combo_box_text_new ();
        for (int jj=0; filter_types[jj]; ++jj){
                gchar *id = g_strdup_printf ("%d", jj);
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo_bqfilter_type), id, filter_types[jj]);
                g_free (id);
        }
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo_bqfilter_type), 5);
        update_bq_filterZS_widget = combo_bqfilter_type;
        g_object_set_data (G_OBJECT (combo_bqfilter_type), "section", GINT_TO_POINTER (3)); 
        g_signal_connect (G_OBJECT (combo_bqfilter_type), "changed",
                          G_CALLBACK (RPSPMC_Control::choice_BQfilter_type_callback),
                          this);
        bp->grid_add_widget (combo_bqfilter_type);
        bp->new_line ();
        for(int jj=0; jj<6; ++jj){
                spmc_parameters.sc_zs_bq_coef[jj]=0.;
                spmc_parameters.sc_zs_bq_coef[0]=1.;
                gchar *l = g_strdup_printf ("ZSBQ %s%d", jj<3?"b":"a", jj<3?jj:jj-3);
                gchar *id = g_strdup_printf ("SPMC-ZS-BQ-COEF-BA");
                bp->grid_add_ec (l, unity_unit, &spmc_parameters.sc_zs_bq_coef[jj], -1e10, 1e10, "g", 1., 10., id, jj);
                if (jj==5) bp->init_ec_array ();
                g_object_set_data (G_OBJECT (combo_bqfilter_type), id, bp->ec);
                g_free (l);
                g_free (id);
                bp->new_line ();
        }	
        
        // ***
        bp->pop_grid (); bp->set_xy (5,2);
 	bp->new_grid_with_frame ("RF Generator and AM/FM Control Setup");

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::rfgen_adjust_callback, this);

	bp->grid_add_ec ("RF Center", new UnitObj("Hz","Hz"), &spmc_parameters.rf_gen_frequency, 0.0, 30e6, "5g", 1.0, 100.0, "SPMC-RF-GEN-FREQ");
        bp->new_line ();
	bp->grid_add_label ("AM Modulation");
        bp->grid_add_check_button ("En", bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Enable AM","dsp-gvp-RFAM"), 1,
                                   GCallback (callback_change_RF_mode), this,
                                   &spmc_parameters.rf_gen_mode, 1);
        bp->new_line ();
	bp->grid_add_ec ("FM Mod Scale", new UnitObj("Hz/V","Hz/V"), &spmc_parameters.rf_gen_fmscale, -1e9, 1e9, "6g", 0.1, 5.0, "SPMC-RF-GEN-FMSCALE");
        bp->grid_add_check_button ("En", bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Enable FM","dsp-gvp-RFFM"), 1,
                                   GCallback (callback_change_RF_mode), this,
                                   &spmc_parameters.rf_gen_mode, 2);

        bp->new_line ();
        bp->set_label_width_chars (10);
	bp->grid_add_label ("RF Gen Output on");
        bp->grid_add_rf_gen_out_options (0, (int)spmc_parameters.rf_gen_out_mux, this);
        
        //***
        
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("... for convenience: Execute GVP, IV buttons");

	bp->grid_add_button ("Execute GVP");
	g_signal_connect (G_OBJECT (bp->button), "clicked",
                          GCallback (RPSPMC_Control::Probing_exec_GVP_callback), this);
        
	bp->grid_add_button ("Execute STS");
	g_signal_connect (G_OBJECT (bp->button), "clicked",
                          GCallback (RPSPMC_Control::Probing_exec_IV_callback), this);
        
        bp->notebook_tab_show_all ();
        bp->pop_grid ();

        
// ==== Folder Set for Vector Probe ========================================
// ==== Folder: I-V STS setup ========================================
        PI_DEBUG (DBG_L4, "SPMC----TAB-IV ------------------------------- ");

        bp->new_grid ();
        bp->start_notebook_tab (notebook, "STS", "rpspmc-tab-sts", hwi_settings);
	// ==================================================
        bp->new_grid_with_frame ("I-V Type Spectroscopy");
	// ==================================================
        GtkWidget *multiIV_checkbutton = NULL;
#if 1 // SIMPLE IV
        IV_sections  = 0;  // simple mode
        multiIV_mode = -1; // simple mode
        
	bp->grid_add_label ("Start"); bp->grid_add_label ("End");  bp->grid_add_label ("Points"); bp->grid_add_label ("Slope"); bp->grid_add_label ("dZ");
        bp->new_line ();

        bp->grid_add_ec (NULL, Volt, &IV_start[0], -10.0, 10., "5.3g", 0.1, 0.025, "IV-Start");
        bp->grid_add_ec (NULL, Volt, &IV_end[0], -10.0, 10.0, "5.3g", 0.1, 0.025, "IV-End");
        bp->grid_add_ec (NULL, Unity, &IV_points[0], 1, 1000, "5g", "IV-Points");
	bp->grid_add_ec (NULL, Vslope, &IV_slope,0.1,1000.0, "5.3g", 1., 10., "IV-Slope");
	bp->grid_add_ec (NULL, Angstroem, &IV_dz, -1000.0, 1000.0, "5.4g", 1., 2., "IV-dz");
        
        bp->new_line ();
	bp->grid_add_ec ("Slope Ramp", Vslope, &IV_slope_ramp,0.1,1000.0, "5.3g", 1., 10., "IV-Slope-Ramp");
        bp->grid_add_check_button ("FB off", "disable Z Servo Feedback for going to IV Start Bias @ Slope Ramp rate", 1,
                                   G_CALLBACK(RPSPMC_Control::Probing_RampFBoff_callback), this, 0);
        bp->set_configure_list_mode_on ();
        bp->new_line ();
	bp->grid_add_ec ("#IV sets", Unity, &IV_repetitions, 1, 100, "3g", "IV-rep");
        bp->new_line ();
	bp->grid_add_ec ("IV-Recover-Delay", Time, &IV_recover_delay, 0., 10., "5.3g", 0.001, 0.01,  "IV-Recover-Delay");
        bp->set_configure_list_mode_off ();
#else
      
	bp->grid_add_ec ("Sections", Unity, &IV_sections, 1,6, "2g", "IV-Sections");
        multiIV_checkbutton = bp->grid_add_check_button ("Multi-IV mode", "enable muli section IV curve mode", 1,
                                                         G_CALLBACK(RPSPMC_Control::Probing_multiIV_callback), this, IV_sections-1);

        bp->new_line ();
	bp->grid_add_label ("IV Probe"); bp->grid_add_label ("Start"); bp->grid_add_label ("End");  bp->grid_add_label ("Points");
	for (int i=0; i<6; ++i) {
                bp->new_line ();

                gchar *tmp = g_strdup_printf("IV[%d]", i+1);
                bp->grid_add_label (tmp);
                if (i>0) multi_IVsec_list = g_slist_prepend (multi_IVsec_list, bp->label);
                g_free(tmp);

                if (i > 0){
                        tmp = g_strdup_printf("dsp-", i+1);
                        bp->set_pcs_remote_prefix (tmp);
                        g_free(tmp);
                }
                
                bp->grid_add_ec (NULL, Volt, &IV_start[i], -10.0, 10., "5.3g", 0.1, 0.025, "IV-Start", i);
                if (i == 5) bp->init_ec_array ();
                if (i>0) multi_IVsec_list = g_slist_prepend (multi_IVsec_list, bp->input);

                bp->grid_add_ec (NULL, Volt, &IV_end[i], -10.0, 10.0, "5.3g", 0.1, 0.025, "IV-End", i);
                if (i == 5) bp->init_ec_array ();
                if (i>0) multi_IVsec_list = g_slist_prepend (multi_IVsec_list, bp->input);

                bp->grid_add_ec (NULL, Unity, &IV_points[i], 1, 1000, "5g", "IV-Points", i);
                if (i == 5) bp->init_ec_array ();
                if (i>0) multi_IVsec_list = g_slist_prepend (multi_IVsec_list, bp->input);
        }
        bp->set_pcs_remote_prefix ("dsp-");
        
        bp->new_line ();

        bp->set_configure_list_mode_on ();
	bp->grid_add_ec ("dz", Angstroem, &IV_dz, -1000.0, 1000.0, "5.4g", 1., 2., "IV-dz");
	bp->grid_add_ec ("#dZ probes", Unity, &IVdz_repetitions, 0, 100, "3g", "IV-dz-rep");
        bp->set_configure_list_mode_off ();

        bp->new_line ();
	bp->grid_add_ec ("Slope", Vslope, &IV_slope,0.1,1000.0, "5.3g", 1., 10., "IV-Slope");
	bp->grid_add_ec ("Slope Ramp", Vslope, &IV_slope_ramp,0.1,1000.0, "5.3g", 1., 10., "IV-Slope-Ramp");
        bp->new_line ();
        bp->set_configure_list_mode_on ();
	bp->grid_add_ec ("#IV sets", Unity, &IV_repetitions, 1, 100, "3g", "IV-rep");

        bp->new_line ();
	bp->grid_add_ec ("Line-dX", Angstroem, &IV_dx, -1e6, 1e6, "5.3g", 1., 10., "IV-Line-dX");
        bp->grid_add_ec ("dY",  Angstroem, &IV_dy, -1e6, 1e6, "5.3g", 1., 10.,  "IV-Line-dY");
#ifdef MOTOR_CONTROL
	bp->grid_add_ec ("Line-dM",  Volt, &IV_dM, -1e6, 1e6, "5.3g", 1., 10.,  "IV-Line-dM");
#else
	IV_dM = 0.;
#endif
        bp->new_line ();
        bp->grid_add_ec ("Points", Unity, &IV_dxy_points, 1, 1000, "3g" "IV-Line-Pts");
	bp->grid_add_ec ("Slope", Speed, &IV_dxy_slope, -1e6, 1e6, "5.3g", 1., 10.,  "IV-Line-slope");
        bp->new_line ();
        bp->grid_add_ec ("Delay", Time, &IV_dxy_delay, 0., 10., "3g", 0.01, 0.1,  "IV-Line-Final-Delay");

        bp->new_line ();
	bp->grid_add_ec ("Final Delay", Time, &IV_final_delay, 0., 1., "5.3g", 0.001, 0.01,  "IV-Final-Delay");
	bp->grid_add_ec ("IV-Recover-Delay", Time, &IV_recover_delay, 0., 1., "5.3g", 0.001, 0.01,  "IV-Recover-Delay");
        bp->set_configure_list_mode_off ();
        
#endif

        // *************** IV controls
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

       
        // ==== Folder: GVP  ========================================
        bp->new_grid ();
        bp->start_notebook_tab (notebook, "GVP", "rpspmc-tab-gvp", hwi_settings);

        PI_DEBUG (DBG_L4, "SPMC----TAB-VP ------------------------------- ");

 	bp->new_grid_with_frame ("Generic Vector Program (VP) Probe and Manipulation");
 	// g_print ("================== TAB 'GVP' ============= Generic Vector Program (VP) Probe and Manipulation\n");
        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ChangedNotifyVP, this);

        bp->set_input_width_chars (6);

	// ----- VP Program Vectors Headings
	// ------------------------------------- divided view

	// ------------------------------------- VP program table, scrolled
 	GtkWidget *vp = gtk_scrolled_window_new ();
        gtk_widget_set_vexpand (vp, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (vp), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_min_content_height  (GTK_SCROLLED_WINDOW (vp), 100);
        gtk_widget_set_vexpand (bp->grid, TRUE);
        bp->grid_add_widget (vp); // only widget in bp->grid

        // catch and disable scroll on wheel
        GtkEventController *gec;
        gec = gtk_event_controller_scroll_new( GTK_EVENT_CONTROLLER_SCROLL_VERTICAL );
        gtk_event_controller_set_propagation_phase( gec, GTK_PHASE_CAPTURE );
        g_signal_connect( gec, "scroll", G_CALLBACK( disable_scroll_cb ), vp );
        gtk_widget_add_controller( vp, gec );
        
        
        bp->new_grid ();
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (vp), bp->grid);
        
        // ----- VP Program Vectors Listing -- headers
	VPprogram[0] = bp->grid;

        bp->set_no_spin ();
        
        bp->set_input_width_chars (3);
        bp->set_label_width_chars (3);

        bp->grid_add_ec ("Mon", Volt, &spmc_parameters.bias_monitor, -10.0, 10.0, ".03g", 0.1, 1., "GVP-U-MONITOR");
        //bp->grid_add_ec ("Mon:", Volt, &spmc_parameters.gvpu_monitor, -10.0, 10.0, ".03g", 0.1, 1., "GVP-U-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->grid_add_ec (NULL, Angstroem, &GVP_XYZ_mon_AA[0], -1e10, 1e10, ".03g", 0.1, 1., "GVP-XS-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->grid_add_ec (NULL, Angstroem, &GVP_XYZ_mon_AA[1], -1e10, 1e10, ".03g", 0.1, 1., "GVP-YS-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->grid_add_ec (NULL, Angstroem, &GVP_XYZ_mon_AA[2], -1e10, 1e10, ".03g", 0.1, 1., "GVP-ZS-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->set_configure_list_mode_on ();
        bp->grid_add_ec (NULL, Volt, &spmc_parameters.gvpa_monitor, -10.0, 10.0, ".03g", 0.1, 1., "GVP-A-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->grid_add_ec (NULL, Volt, &spmc_parameters.gvpb_monitor, -10.0, 10.0, ".03g", 0.1, 1., "GVP-B-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->grid_add_ec (NULL, Volt, &spmc_parameters.gvpamc_monitor, -10.0, 10.0, ".03g", 0.1, 1., "GVP-AMC-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->grid_add_ec (NULL, Volt, &spmc_parameters.gvpfmc_monitor, -10.0, 10.0, ".03g", 0.1, 1., "GVP-FMC-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->set_configure_list_mode_off ();
        mon_FB = 0;
        bp->set_input_nx (5);
        bp->grid_add_ec (" FB: ", Unity, &mon_FB, -9999, 9999, ".0f", "GVP-ZSERVO-MONITOR");
        gvp_monitor_fb_label = bp->label;
        gvp_monitor_fb_info_ec = bp->ec;

        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->set_input_nx ();
        
	g_object_set_data (G_OBJECT (window), "SPMC_MONITORS_list", EC_MONITORS_list);

        bp->new_line ();
        bp->grid_add_label ("VPC", "Vector Program Counter");

        bp->set_input_width_chars (7);
        bp->set_label_width_chars (7);

        GVP_preview_on[0]=0; // not used
        
        //bp->grid_add_label ("VP-dU", "vec-du (Bias, DAC CH4)");
        bp->grid_add_button ("VP-dU", "vec-du (Bias, DAC CH4)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "gvpcolor4"); GVP_preview_on[4]=1;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (4));
        
        //bp->grid_add_label ("VP-dX", "vec-dx (X-Scan *Mrot + X0 => DAC CH1)");
        bp->grid_add_button ("VP-dX", "vec-dx (X-Scan *Mrot + X0 => DAC CH1)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "normal"); GVP_preview_on[1]=0;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (1));

        //bp->grid_add_label ("VP-dY", "vec-dy (Y-Scan, *Mrot + Y0, DAC CH2)");
        bp->grid_add_button ("VP-dY", "vec-dy (Y-Scan *Mrot + Y0 => DAC CH2)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "normal"); GVP_preview_on[2]=0;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (2));

        //bp->grid_add_label ("VP-dZ", "vec-dz (Z-Probe + Z-Servo + Z0, DAC CH3)");
        bp->grid_add_button ("VP-dZ", "vec-dz (Z-Probe + Z-Servo + Z0, DAC CH3)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "gvpcolor3"); GVP_preview_on[3]=1;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (3));

        bp->set_configure_list_mode_on ();
        //bp->grid_add_label ("VP-dA", "vec-dA (**DAC CH5)");
        bp->grid_add_button ("VP-dA", "vec-dA (**DAC CH5)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "normal"); GVP_preview_on[5]=0;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (5));

        //bp->grid_add_label ("VP-dB", "vec-dB (**DAC CH6)");
        bp->grid_add_button ("VP-dB", "vec-dB (**DAC CH6)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "normal"); GVP_preview_on[6]=0;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (6));

        //bp->grid_add_label ("VP-dAM", "vec-dAM (**RF-AM control)");
        bp->grid_add_button ("VP-dAM", "vec-dAM (**RF-AMP control)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "normal"); GVP_preview_on[7]=0;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (7));

        //bp->grid_add_label ("VP-dFM", "vec-dFM (**RF-FM control)");
        bp->grid_add_button ("VP-dFM", "vec-dFM (**RF-FM control)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "normal"); GVP_preview_on[8]=0;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (8));

        bp->set_configure_list_mode_off ();
        bp->grid_add_label ("time", "total time for VP section");
        bp->grid_add_label ("points", "points (# vectors to add)");

        bp->set_input_width_chars (3);
        bp->set_label_width_chars (3);
        bp->grid_add_label ("FB", "Feedback (Option bit 0), CHECKED=FB-OFF (for now)");
        bp->grid_add_label ("VSet", "Treat this as a initial set position, vector differential from current position are computed!\nAttention: Does NOT apply to XYZ.");
        bp->set_configure_list_mode_on ();

        GSList *EC_vpc_exthdr_list=NULL;
        bp->grid_add_label ("OP", "X-OPCD"); EC_vpc_exthdr_list = g_slist_prepend( EC_vpc_exthdr_list, bp->label); gtk_widget_hide (bp->label);
        bp->grid_add_label ("CI", "X-RCHI"); EC_vpc_exthdr_list = g_slist_prepend( EC_vpc_exthdr_list, bp->label); gtk_widget_hide (bp->label);
        bp->grid_add_label ("VAL", "X-CMPV"); EC_vpc_exthdr_list = g_slist_prepend( EC_vpc_exthdr_list, bp->label); gtk_widget_hide (bp->label);
        bp->grid_add_label ("JR", "X-JMPR"); EC_vpc_exthdr_list = g_slist_prepend( EC_vpc_exthdr_list, bp->label); gtk_widget_hide (bp->label);

        bp->grid_add_label ("VX", "Enable VecEXtension OPCodes. Option bit 7");
        bp->grid_add_label ("HS", "Enable Max Sampling. Option bit 6");
        bp->grid_add_label ("5", "Option bit 5");
        bp->grid_add_label ("4", "Option bit 4");
        bp->grid_add_label ("3", "Option bit 3");
        bp->grid_add_label ("2", "Option bit 2");
        bp->grid_add_label ("1", "Option bit 1");

        bp->set_input_width_chars (4);
        bp->set_label_width_chars (4);
        bp->grid_add_label ("Nrep", "VP # repetition");
        bp->grid_add_label ("PCJR", "Vector-PC jump relative\n (example: set to -2 to repeat previous two vectors.)");
        bp->set_configure_list_mode_off ();
        bp->grid_add_label("Shift", "Edit: Shift VP Block up/down");

        bp->new_line ();
        
	GSList *EC_vpc_opt_list=NULL;
        Gtk_EntryControl *ec_iter[12];
	for (int k=0; k < N_GVP_VECTORS; ++k) {
		gchar *tmpl = g_strdup_printf ("%02d", k); 

                // g_print ("GVP-DEBUG:: %s -[%d]------> GVPdu=%g, ... points=%d,.. opt=%8x\n", tmpl, k, GVP_du[k], GVP_points[k],  GVP_opt[k]);

                bp->set_input_width_chars (3);
                bp->set_label_width_chars (3);
		bp->grid_add_label (tmpl, "Vector PC");
                
                bp->set_input_width_chars (7);
                bp->set_label_width_chars (7);
		bp->grid_add_ec (NULL,      Volt, &GVP_du[k], -10.0,   10.0,   "6.4g", 1., 10., "gvp-du", k);
                if (k == 0) GVP_V0Set_ec[3] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

		bp->grid_add_ec (NULL, Angstroem, &GVP_dx[k], -1000000.0, 1000000.0, "6.4g", 1., 10., "gvp-dx", k); 
                if (k == 0) GVP_V0Set_ec[0] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

		bp->grid_add_ec (NULL, Angstroem, &GVP_dy[k], -1000000.0, 1000000.0, "6.4g", 1., 10., "gvp-dy", k); 
                if (k == 0) GVP_V0Set_ec[1] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

		bp->grid_add_ec (NULL, Angstroem, &GVP_dz[k], -1000000.0, 1000000.0, "6.4g", 1., 10., "gvp-dz", k); 
                if (k == 0) GVP_V0Set_ec[2] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

                bp->set_configure_list_mode_on (); // === advanced ===========================================
		bp->grid_add_ec (NULL,    Volt, &GVP_da[k], -10.0, 10.0, "6.4g",1., 10., "gvp-da", k); 
                if (k == 0) GVP_V0Set_ec[4] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
		bp->grid_add_ec (NULL,    Volt, &GVP_db[k], -10.0, 10.0, "6.4g",1., 10., "gvp-db", k); 
                if (k == 0) GVP_V0Set_ec[5] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
		bp->grid_add_ec (NULL,    Volt, &GVP_dam[k], -10.0, 10.0, "6.4g",1., 10., "gvp-dam", k); 
                if (k == 0) GVP_V0Set_ec[6] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
		bp->grid_add_ec (NULL,    Volt, &GVP_dfm[k], -10.0, 10.0, "6.4g",1., 10., "gvp-dfm", k); 
                if (k ==0 ) GVP_V0Set_ec[7] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
                bp->set_configure_list_mode_off (); // ========================================================

		bp->grid_add_ec (NULL,      Time, &GVP_ts[k], 0., 147573952580.0,     "5.4g", 1., 10., "gvp-dt", k); // 1<<64 / 125e6 s max
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

		bp->grid_add_ec (NULL,      Unity, &GVP_points[k], 0, (1<<16)-1,  "5g", "gvp-n", k); // FPGA GVP can do 1<<32-1, currently limited by 16bit GVP stream index.
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();


                bp->set_input_width_chars (2);
                bp->set_label_width_chars (2);
		//note:  VP_FEEDBACK_HOLD is only the mask, this bit in GVP_opt is set to ONE for FEEBACK=ON !! Ot is inveretd while vector generation ONLY.
		bp->grid_add_check_button ("", bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Enable Zservo","dsp-gvp-FB",k), 1,
                                               GCallback (callback_change_GVP_vpc_option_flags), this,
                                               GVP_opt[k], VP_FEEDBACK_HOLD);
                EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, bp->button);
                g_object_set_data (G_OBJECT (bp->button), "VPC", GINT_TO_POINTER (k));

                bp->set_configure_list_mode_on (); // ================ advanced section

                bp->grid_add_check_button ("", bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Vector is a absolute reference (goto)","dsp-gvp-VS",k), 1,
                                           GCallback (callback_change_GVP_vpc_option_flags), this,
                                           GVP_opt[k], VP_INITIAL_SET_VEC);
                EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, bp->button);
                g_object_set_data (G_OBJECT (bp->button), "VPC", GINT_TO_POINTER (k));


                bp->set_input_width_chars (4);
                bp->set_label_width_chars (4);
                GSList *EC_vpc_ext_list=NULL;
		bp->grid_add_ec (NULL, Unity, &GVPX_opcd[k],   0.,  5.,  ".0f", "gvp-xopcd", k); //
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
                EC_vpc_ext_list = g_slist_prepend( EC_vpc_ext_list, bp->input);
		bp->grid_add_ec (NULL, Unity, &GVPX_rchi[k],   0., 14.,  ".0f", "gvp-xrchi", k); //
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
                EC_vpc_ext_list = g_slist_prepend( EC_vpc_ext_list, bp->input);
                bp->set_input_width_chars (7);
                bp->set_label_width_chars (7);
                bp->grid_add_ec (NULL, Unity, &GVPX_cmpv[k], -1e6, 1e6,  "6.4g", 1., 10., "gvp-xcmpv", k); //
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
                EC_vpc_ext_list = g_slist_prepend( EC_vpc_ext_list, bp->input);
                bp->set_input_width_chars (4);
                bp->set_label_width_chars (4);
		bp->grid_add_ec (NULL, Unity, &GVPX_jmpr[k], -32., 32.,  ".0f", "gvp-xjmpr", k); //
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
                EC_vpc_ext_list = g_slist_prepend( EC_vpc_ext_list, bp->input);

                
                bp->set_input_width_chars (2);
                bp->set_label_width_chars (2);
                GtkWidget *VX_b7=NULL;
                for (int bit=7; bit >= 1; --bit){
                        bp->set_label_width_chars (1);
                        bp->grid_add_check_button ("", NULL, 1,
                                                   GCallback (callback_change_GVP_vpc_option_flags), this,
                                                   GVP_opt[k], (1<<bit));
                        if (bit == 7) VX_b7 = bp->button;
                        EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, bp->button);
                        g_object_set_data (G_OBJECT (bp->button), "VPC", GINT_TO_POINTER (k));
                }
                g_object_set_data( G_OBJECT (VX_b7), "DSP_VPC_EXTOPC_list", EC_vpc_ext_list);
                g_object_set_data( G_OBJECT (VX_b7), "DSP_VPC_EXTHDR_list", EC_vpc_exthdr_list);

                bp->set_input_width_chars (4);
                bp->set_label_width_chars (4);
		bp->grid_add_ec (NULL, Unity, &GVP_vnrep[k], 0., 65536.,  ".0f", "gvp-nrep", k); // limit to 1<<16
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

		bp->grid_add_ec (NULL, Unity, &GVP_vpcjr[k], -16.,   0.,  ".0f", "gvp-pcjr", k); // -MAX NUM VECTORS at longest
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

                bp->set_configure_list_mode_off (); // ==================================

                bp->set_input_width_chars (4);
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

        bp->set_input_width_chars (6);

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

	bp->new_grid_with_frame ("GVP Wave Preview");
        WavePreview=bp->frame;
	//bp->new_grid_with_frame ("VP Status");
	//GVP_status = bp->grid_add_probe_status ("Status");

        gvp_preview_area = gtk_drawing_area_new ();
        //gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (gvp_preview_area), 512);
        gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (gvp_preview_area), 128);
        //gtk_widget_set_size_request (gvp_preview_area, -1, 128);
        //gtk_widget_set_hexpand (gvp_preview_area, true);
        //gtk_widget_compute_expand (gvp_preview_area, GTK_ORIENTATION_HORIZONTAL);

	GtkWidget *scrollarea = gtk_scrolled_window_new ();

        gtk_widget_set_hexpand (scrollarea, TRUE);
        gtk_widget_set_vexpand (scrollarea, TRUE);
        
	/* the policy is one of GTK_POLICY AUTOMATIC, or GTK_POLICY_ALWAYS.
	 * GTK_POLICY_AUTOMATIC will automatically decide whether you need
	 * scrollbars, whereas GTK_POLICY_ALWAYS will always leave the scrollbars
	 * there.  The first one is the horizontal scrollbar, the second, 
	 * the vertical. */
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollarea),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrollarea), gvp_preview_area);
        
        bp->grid_add_widget (scrollarea);
        //bp->grid_add_widget (gvp_preview_area);

        gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (gvp_preview_area),
                                        GtkDrawingAreaDrawFunc (RPSPMC_Control::gvp_preview_draw_function),
                                        this, NULL);
        
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

        PI_DEBUG (DBG_L4, "SPMC----TAB-GRAPHS ------------------------------- ");

 	bp->new_grid_with_frame ("Probe Sources & Graph Setup");

        bp->grid_add_label ("Ref-Source", "Check column to activate channel", 2, 0.);
        bp->set_input_width_chars (1);
        bp->grid_add_label ("X", "Check column to plot channel on X axis.", 1);
        bp->grid_add_label ("Y", "Check column to plot channel on Y axis.", 1);
        bp->grid_add_label ("Avg", "Check column to plot average of all spectra", 1);
        bp->grid_add_label ("Sec", "Check column to show all sections.", 1);
        GtkWidget *sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
        gtk_widget_set_size_request (sep, 5, -1);
        bp->grid_add_widget (sep, 5);
        //bp->grid_add_label (" --- ", NULL, 5);

        bp->grid_add_label ("Source", "Check column to activate channel", 2, 0.);
        bp->set_input_width_chars (1);
        bp->grid_add_label ("X", "Check column to plot channel on X axis.", 1);
        bp->grid_add_label ("Y", "Check column to plot channel on Y axis.", 1);
        bp->grid_add_label ("Avg", "Check column to plot average of all spectra", 1);
        bp->grid_add_label ("Sec", "Check column to show all sections.", 1);
        sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
        gtk_widget_set_size_request (sep, 5, -1);
        bp->grid_add_widget (sep, 5);
        //bp->grid_add_label (" --- ", NULL, 5);

        bp->grid_add_label ("Source", "Check column to activate channel", 2, 0.);
        bp->set_input_width_chars (1);
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
        bp->set_input_width_chars (1);
        bp->grid_add_label ("X", "Check column to plot channel on X axis.", 1);
        bp->grid_add_label ("Y", "Check column to plot channel on Y axis.", 1);
        bp->grid_add_label ("Avg", "Check column to plot average of all spectra", 1);
        bp->grid_add_label ("Sec", "Check column to show all sections.", 1);
        sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
        gtk_widget_set_size_request (sep, 5, -1);
        bp->grid_add_widget (sep, 5);
#endif
        bp->new_line ();

        PI_DEBUG (DBG_L4, "SPMC----TAB-GRAPHS TOGGELS  ------------------------------- ");

        gint y = bp->y;
        gint mm=0;

        for (int i=0; i<32; ++i) for (int j=0; j<5; ++j) graphs_matrix[j][i]=NULL;
        restore_graphs_values ();

        int ii=0;
        for (int i=0; rpspmc_source_signals[i].mask; ++i) {
                const int rows=11;
                int k=-1;
		int c=ii/rows; 
                int r = y+ii%rows+1; // row
                ii++;
		c*=11; // col
                c++;

                if (rpspmc_source_signals[i].scan_source_pos < 0) continue; // skip
                PI_DEBUG (DBG_L4, "GRAPHS*** i=" << i << " " << rpspmc_source_signals[i].label);

                switch (rpspmc_source_signals[i].mask){
                case 0x0100: k=0; break;
                case 0x0200: k=1; break;
                case 0x0400: k=2; break;
                case 0x0800: k=3; break;
                case 0x1000: k=4; break;
                case 0x2000: k=5; break;
                }

                if (k >= 0 && k < 6){
                        c=23; r=y+k+1;
                        if (!rpspmc_swappable_signals[k].label) { g_warning ("GVP SOURCE MUX/SWPS INIT ** i=%d k=%d SWPS invalid/NULL", i,k); break; }
                      //rpspmc_source_signals[i].name         = rpspmc_swappable_signals[k].name;
                        rpspmc_source_signals[i].label        = rpspmc_swappable_signals[k].label;
                        rpspmc_source_signals[i].unit         = rpspmc_swappable_signals[k].unit;
                        rpspmc_source_signals[i].unit_sym     = rpspmc_swappable_signals[k].unit_sym;
                        rpspmc_source_signals[i].scale_factor = rpspmc_swappable_signals[k].scale_factor;
                        PI_DEBUG (DBG_L4, "GRAPHS*** SWPS init i=" << i << " k=" << k << " " << rpspmc_source_signals[i].label << " sfac=" << rpspmc_source_signals[i].scale_factor);
                        g_message ("GRAPHS*** SWPS init i=%d k=%d {%s} sfac=%g", i, k, rpspmc_source_signals[i].label,rpspmc_source_signals[i].scale_factor);
                }
                if (rpspmc_source_signals[i].mask == 0xc000){ // Time-Mon
                        c=23; r=y+7+1;
                }

                bp->set_xy (c, r);

                g_message ("GRAPHS MATRIX SELECTOR BUILD: %d => 0x%08x",i,rpspmc_source_signals[i].mask);
                
                // Source
                bp->set_input_width_chars (1);
                graphs_matrix[0][i] = bp->grid_add_check_button ("", NULL, 1,
                                                                 GCallback (change_source_callback), this,
                                                                 Source, rpspmc_source_signals[i].mask
                                                                 );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER (rpspmc_source_signals[i].mask)); 
                g_object_set_data (G_OBJECT(bp->button), "SourceMapping",  GINT_TO_POINTER (MAP_SOURCE_REC)); 
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 

                // source selection for SWPS?:
                if (k >= 0 && k < 6){ // swappable flex source
                        //g_message("bp->grid_add_probe_source_signal_options m=%d  %s => %d %s", k, rpspmc_source_signals[i].label,  probe_source[k], rpspmc_swappable_signals[k].label);
                        probe_source_signal_selector[k] = bp->grid_add_probe_source_signal_options (k, probe_source[k], this);
                }else { // or fixed assignment
                        bp->grid_add_label (rpspmc_source_signals[i].label, NULL, 1, 0.);
                }
                //fixed assignment:
                // bp->grid_add_label (lablookup[i], NULL, 1, 0.);
                
                // use as X-Source
                graphs_matrix[1][i] = bp->grid_add_check_button ("", NULL, 1,
                                                                 GCallback (change_source_callback), this,
                                                                 XSource, X_SOURCE_MSK | rpspmc_source_signals[i].mask
                                                                 );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER (rpspmc_source_signals[i].mask)); 
                g_object_set_data (G_OBJECT(bp->button), "Source_Mapping", GINT_TO_POINTER (MAP_SOURCE_X)); 
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 

                // use as Plot (Y)-Source
                graphs_matrix[2][i] = bp->grid_add_check_button ("", NULL, 1,
                                                                 G_CALLBACK (change_source_callback), this,
                                                                 PSource, P_SOURCE_MSK | rpspmc_source_signals[i].mask
                                                                 );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER (rpspmc_source_signals[i].mask)); 
                g_object_set_data (G_OBJECT(bp->button), "Source_Mapping", GINT_TO_POINTER (MAP_SOURCE_PLOTY));
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // use as A-Source (Average)
                graphs_matrix[3][i] = bp->grid_add_check_button ("", NULL, 1,
                                                                 G_CALLBACK (change_source_callback), this,
                                                                 PlotAvg, A_SOURCE_MSK | rpspmc_source_signals[i].mask
                                                                 );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER (rpspmc_source_signals[i].mask)); 
                g_object_set_data (G_OBJECT(bp->button), "Source_Mapping", GINT_TO_POINTER (MAP_SOURCE_AVG));
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // use as S-Source (Section)
                graphs_matrix[4][i] = bp->grid_add_check_button ("", NULL, 1,
                                                                 G_CALLBACK (change_source_callback), this,
                                                                 PlotSec, S_SOURCE_MSK | rpspmc_source_signals[i].mask
                                                                 );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER (rpspmc_source_signals[i].mask)); 
                g_object_set_data (G_OBJECT(bp->button), "Source_Mapping", GINT_TO_POINTER (MAP_SOURCE_SEC));
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // bp->grid_add_check_button_graph_matrix(lablookup[i], (int) rpspmc_source_signals[i].mask, m, probe_source[m], i, this);
                // bp->grid_add_check_button_graph_matrix(" ", (int) (X_SOURCE_MSK | rpspmc_source_signals[i].mask), -1, i, this);
                // bp->grid_add_check_button_graph_matrix(" ", (int) (P_SOURCE_MSK | rpspmc_source_signals[i].mask), -1, i, this);
                // bp->grid_add_check_button_graph_matrix(" ", (int) (A_SOURCE_MSK | rpspmc_source_signals[i].mask), -1, i, this);
                // bp->grid_add_check_button_graph_matrix(" ", (int) (S_SOURCE_MSK | rpspmc_source_signals[i].mask), -1, i, this);

                sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
                gtk_widget_set_size_request (sep, 5, -1);
                bp->grid_add_widget (sep);
	}
        g_message ("GRAPHS MATRIX SELECTOR BUILD complete.");

        
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Plot Mode Configuration");

        g_message ("GRAPHS PLOT CFG");
	graphs_matrix[0][31] = bp->grid_add_check_button ("Join all graphs for same X", "Join all plots with same X.\n"
                                                          "Note: if auto scale (default) Y-scale\n"
                                                          "will only apply to 1st graph - use hold/exp. only for asolute scale.)", 1,
                                                          GCallback (callback_XJoin), this,
                                                          XJoin, 1
                                                          );
	graphs_matrix[1][31] = bp->grid_add_check_button ("Use single window", "Place all probe graphs in single window.",
                                                          1,
                                                          GCallback (callback_GrMatWindow), this,
                                                          GrMatWin, 1
                                                          );
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Plot / Save current data in buffer -- for convenience: extra Execute GVP button");

        g_message ("GRAPHS PLOT ACTIONS");
	bp->grid_add_button ("Plot");
	g_signal_connect (G_OBJECT (bp->button), "clicked",
                          G_CALLBACK (RPSPMC_Control::Probing_graph_callback), this);
	
        save_button = bp->grid_add_button ("Save");
	g_signal_connect (G_OBJECT (bp->button), "clicked",
                          G_CALLBACK (RPSPMC_Control::Probing_save_callback), this);


	bp->grid_add_button ("Execute GVP");
	g_signal_connect (G_OBJECT (bp->button), "clicked",
                          GCallback (RPSPMC_Control::Probing_exec_GVP_callback), this);
        
        bp->notebook_tab_show_all ();
        bp->pop_grid ();



// ==== Folder: RPSPMC System Connection ========================================
        g_message ("Folder: RPSPMC System Connection");
        bp->set_pcs_remote_prefix ("");
        bp->new_grid ();
        bp->start_notebook_tab (notebook, "RedPitaya Web Socket", "rpspmc-tab-system", hwi_settings);

        PI_DEBUG (DBG_L4, "SPMC----TAB-SYSTEM ------------------------------- ");

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

        g_message ("Folder: RPSPMC CHECK CONNECT");

        bp->grid_add_check_button ( N_("Restart"), "Check to reload FPGA and initiate connection, uncheck to close connection.", 1,
                                    G_CALLBACK (RPspmc_pacpll::connect_cb), rpspmc_pacpll);
	g_object_set_data( G_OBJECT (bp->button), "RESTART", GINT_TO_POINTER (1));

        bp->grid_add_check_button ( N_("Re-Connect"), "Check to re-initiate connection, uncheck to close connection.", 1,
                                    G_CALLBACK (RPspmc_pacpll::connect_cb), rpspmc_pacpll);
	g_object_set_data( G_OBJECT (bp->button), "RESTART", GINT_TO_POINTER (0));

        bp->grid_add_check_button ( N_("Connect Stream"), "Check to initiate stream connection, uncheck to close connection.", 1,
                                    G_CALLBACK (rpspmc_hwi_dev::spmc_stream_connect_cb), rpspmc_hwi);
        stream_connect_button=bp->button;

        spmc_parameters.rpspmc_dma_pull_interval = 10.0;
        bp->set_input_width_chars (2);
        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::spmc_server_control_callback, this);
  	bp->grid_add_ec ("DMA rate limit", msTime, &spmc_parameters.rpspmc_dma_pull_interval, 10., 10., ".0f", 5., 250., "RP-DMA-PULL-INTERVAL");        
        EC_FPGA_SPMC_server_settings_list = g_slist_prepend(EC_FPGA_SPMC_server_settings_list, bp->ec);
        
        bp->grid_add_widget (GVP_stop_all_zero_button=gtk_button_new_from_icon_name ("process-stopall-symbolic"));
        g_signal_connect (G_OBJECT (GVP_stop_all_zero_button), "clicked", G_CALLBACK (RPSPMC_Control::GVP_AllZero), this);
        gtk_widget_set_tooltip_text (GVP_stop_all_zero_button, "Click to force reset GVP (WARNING: XYZ JUMP possible)!");

        g_message ("Folder: RPSPMC DBG");

        bp->new_line ();
        bp->grid_add_check_button ( N_("Debug"), "Enable debugging LV1.", 1,
                                    G_CALLBACK (RPspmc_pacpll::dbg_l1), rpspmc_pacpll);
        bp->grid_add_check_button ( N_("+"), "Debug LV2", 1,
                                    G_CALLBACK (RPspmc_pacpll::dbg_l2), rpspmc_pacpll);
        bp->grid_add_check_button ( N_("++"), "Debug LV4", 1,
                                    G_CALLBACK (RPspmc_pacpll::dbg_l4), rpspmc_pacpll);
        bp->grid_add_check_button ( N_("GVP6"), "Set GVP Option Bit 6 for scan", 1,
                                    G_CALLBACK (RPspmc_pacpll::scan_gvp_opt6), rpspmc_pacpll);
        bp->grid_add_check_button ( N_("GVP7"), "Set GVP Option Bit 7 for scan", 1,
                                    G_CALLBACK (RPspmc_pacpll::scan_gvp_opt7), rpspmc_pacpll);
        rp_verbose_level = 0.0;
        bp->set_input_width_chars (2);
  	bp->grid_add_ec ("Verbosity", Unity, &rp_verbose_level, 0., 10., ".0f", 1., 1., "RP-VERBOSE-LEVEL");        
        EC_FPGA_SPMC_server_settings_list = g_slist_prepend(EC_FPGA_SPMC_server_settings_list, bp->ec);
        //bp->new_line ();
        //tmp=bp->grid_add_button ( N_("Read"), "TEST READ", 1,
        //                          G_CALLBACK (RPspmc_pacpll::read_cb), this);
        //tmp=bp->grid_add_button ( N_("Write"), "TEST WRITE", 1,
        //                          G_CALLBACK (RPspmc_pacpll::write_cb), this);

        bp->new_line ();
        bp->set_input_width_chars (80);
        rpspmc_pacpll->red_pitaya_health = bp->grid_add_input ("RedPitaya Health",10);
        gtk_widget_set_name (bp->input, "entry-mono-text-start");
        
        PangoFontDescription *fontDesc = pango_font_description_from_string ("monospace 10");
        //gtk_widget_modify_font (red_pitaya_health, fontDesc);
        // ### GTK4 ??? CSS ??? ###  gtk_widget_override_font (red_pitaya_health, fontDesc); // use CSS, thx, annyoing garbage... ??!?!?!?
        pango_font_description_free (fontDesc);

        gtk_widget_set_sensitive (bp->input, FALSE);
        gtk_editable_set_editable (GTK_EDITABLE (bp->input), FALSE); 
        rpspmc_pacpll->update_health ("Not connected.");
        bp->new_line ();

        g_message ("Folder: RPSPMC LOG");
        
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

        g_message ("*** STORE LISTS ***");

        
	// ============================================================
	// save List away...
        rpspmc_pacpll_hwi_pi.app->RemoteEntryList = g_slist_concat (rpspmc_pacpll_hwi_pi.app->RemoteEntryList, bp->get_remote_list_head ());
        configure_callback (NULL, NULL, this); // configure "false"

	g_object_set_data( G_OBJECT (window), "DSP_EC_list", bp->get_ec_list_head ());
        if (multiIV_checkbutton)
                g_object_set_data( G_OBJECT (multiIV_checkbutton), "DSP_multiIV_list", multi_IVsec_list);
	//g_object_set_data( G_OBJECT (zposmon_checkbutton), "DSP_zpos_control_list", zpos_control_list);


	g_object_set_data( G_OBJECT (window), "FPGA_readback_update_list", FPGA_readback_update_list);
        g_object_set_data( G_OBJECT (window), "EC_FPGA_SPMC_server_settings_list", EC_FPGA_SPMC_server_settings_list);
        
	GUI_ready = TRUE;

        rpspmc_pacpll->update_shm_monitors (); // init SHM monitor block
        
        AppWindowInit (NULL); // stage two
        set_window_geometry ("rpspmc-main-control"); // must add key to xml file: core-sources/org.gnome.gxsm4.window-geometry.gschema.xml

        g_message ("RPSPMC CONTROL READY -- updating");
       
        //Probing_RampFBoff_callback (multiIV_checkbutton, this); // update
        //Probing_multiIV_callback (multiIV_checkbutton, this); // update

        g_message ("RPSPMC BUILD TABS DONE.");
}

void RPSPMC_Control::Init_SPMC_on_connect (){
        // fix-me -- need life readback once life re-connect works!
        // update: mission critical readbacks done
        if (rpspmc_pacpll && rpspmc_hwi){
                rpspmc_pacpll->write_parameter ("SPMC_GVP_RESET_OPTIONS", 0x0000); // default/reset GVP OPTIONS: Feedback ON! *** WATCH THIS ***
        
                BiasChanged(NULL, this);
                Slope_dZX_Changed(NULL, this);
                Slope_dZY_Changed(NULL, this);
                ZPosSetChanged(NULL, this);
                ZServoParamChanged(NULL, this);

                // reset and zero GVP
                GVP_zero_all_smooth ();
        }
}

void RPSPMC_Control::Init_SPMC_after_cold_start (){
        // init filter sections
        if (rpspmc_pacpll && rpspmc_hwi){
                gtk_combo_box_set_active (GTK_COMBO_BOX (update_bq_filterS1_widget), 0);
                rpspmc_hwi->status_append (" * INIT-BQS1...\n"); for (int i=0; i<10; ++i){ while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE); usleep(20000); }
                gtk_combo_box_set_active (GTK_COMBO_BOX (update_bq_filterS1_widget), 3);
                rpspmc_hwi->status_append (" * INIT-BQS1 to AB.\n"); for (int i=0; i<10; ++i){ while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE); usleep(20000); }
                gtk_combo_box_set_active (GTK_COMBO_BOX (update_bq_filterS2_widget), 0);
                rpspmc_hwi->status_append (" * INIT-BQS2...\n"); for (int i=0; i<10; ++i){ while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE); usleep(20000); }
                gtk_combo_box_set_active (GTK_COMBO_BOX (update_bq_filterS2_widget), 3);
                rpspmc_hwi->status_append (" * INIT-BQS2 to AB.\n"); for (int i=0; i<10; ++i){ while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE); usleep(20000); }
                gtk_combo_box_set_active (GTK_COMBO_BOX (update_bq_filterZS_widget), 0);
                rpspmc_hwi->status_append (" * INIT-BQZS...\n"); for (int i=0; i<10; ++i){ while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE); usleep(20000); }
                gtk_combo_box_set_active (GTK_COMBO_BOX (update_bq_filterZS_widget), 5);
                rpspmc_hwi->status_append (" * INIT-BQZS to ByPass.\n"); for (int i=0; i<10; ++i){ while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE); usleep(20000); }
        }
}

void  RPSPMC_Control::spmc_server_control_callback (GtkWidget *widget, RPSPMC_Control *self){
        g_message ("RPSPMC+Control::spmc_server_control_callback DMA: %d ms, VL: %d",(int)spmc_parameters.rpspmc_dma_pull_interval, (int)self->rp_verbose_level);
        //RPSPMC_Control::GVP_AllZero (self);
        if (rpspmc_pacpll){
                rpspmc_pacpll->write_parameter ("RPSPMC_DMA_PULL_INTERVAL", (int)spmc_parameters.rpspmc_dma_pull_interval);
                rpspmc_pacpll->write_parameter ("PACVERBOSE", (int)self->rp_verbose_level);
        }
}

void RPSPMC_Control::GVP_zero_all_smooth (){
        // reset GVP
        int vector_index=0;
        int gvp_options=0;

        rpspmc_hwi->resetVPCconfirmed ();
        make_dUZXYAB_vector (vector_index++,
                             0., 0., // GVP_du[k], GVP_dz[k],
                             0., 0., 0., 0., 0., 0.,  // GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k], AM, FM
                             100, 0, 0, 0.5, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                             0, VP_INITIAL_SET_VEC | gvp_options);
        append_null_vector (vector_index, gvp_options);

        g_message ("last vector confirmed: %d, need %d", rpspmc_hwi->getVPCconfirmed (), rpspmc_hwi->last_vector_index);

        int timeout = 100;
        while (rpspmc_hwi->getVPCconfirmed () < rpspmc_hwi->last_vector_index && timeout--){
#if GVP_DEBUG_VERBOSE > 4
                g_message ("GVP-ZERO: Waiting for GVP been written and confirmed. [Vector %d]", rpspmc_hwi->getVPCconfirmed ());
#endif
                usleep(20000);
        }
        if (timeout > 0)
                g_message ("GVP-ZERO been written and confirmed for vector #%d. Init GVP. Executing GVP now.", rpspmc_hwi->getVPCconfirmed ());
        else {
                g_message ("GVP-ZERO program write and confirm failed. Aborting Scan.");
                rpspmc_hwi->EndScan2D();
                return NULL;
        }

        usleep(200000);
        rpspmc_hwi->GVP_execute_vector_program(); // non blocking
}

int RPSPMC_Control::GVP_AllZero (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
        g_message ("RPSPMC_Control::GVP_AllZero: GVP STOP and set ALL ZERO   ** DANGER IF IN OPERATION ** -- No Action here in productioin version or confirm dialog.");

        if (gapp->question_yes_no ("WARNING WARNING WARNING\n"
                                   "RPSPMC will force all GVP componet to zero (smooth).\n"
                                   "This does NOT effect the Z control/feedback Z value, only any GVP offsets.\n"
                                   "This aborts any GVP (scan/probe) in progress.\n"
                                   "Confirm to proceed with Set All GVP to Zero -- No: no action.", self->window)){
                rpspmc_hwi->GVP_abort_vector_program ();
                usleep(200000);
                self->GVP_zero_all_smooth ();
                usleep(200000);
                rpspmc_hwi->GVP_reset_UAB ();
        }
        return 0;
}

int RPSPMC_Control::DSP_cret_callback (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	self->center_return_flag = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));	
        return 0;
}


int RPSPMC_Control::ldc_callback(GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	//self->update_controller();
	return 0;
}

void RPSPMC_Control::BiasChanged(Param_Control* pcs, RPSPMC_Control* self){
        int j=0;
        spmc_parameters.bias_monitor = self->bias;
        if (rpspmc_pacpll)
                rpspmc_pacpll->write_parameter ("SPMC_BIAS", self->bias);

        // add user event, mirror basic parameters to GXSM4 main
        if (rpspmc_hwi)
                rpspmc_hwi->add_user_event_now ("Bias_adjust", "[V] (pre,now)", main_get_gapp()->xsm->data.s.Bias, self->bias);
 	main_get_gapp()->xsm->data.s.Bias = self->bias;
}

void RPSPMC_Control::Slope_dZX_Changed(Param_Control* pcs, RPSPMC_Control* self){
        if (rpspmc_pacpll){
                double zx_ratio = main_get_gapp()->xsm->Inst->XResolution () / main_get_gapp()->xsm->Inst->ZResolution (); 
                //rpspmc_pacpll->write_parameter ("SPMC_SLOPE_SLEW", 100.0);
                //usleep (100000);
                rpspmc_pacpll->write_parameter ("SPMC_SLOPE_DZX", zx_ratio * self->area_slope_x);
        }
}

void RPSPMC_Control::Slope_dZY_Changed(Param_Control* pcs, RPSPMC_Control* self){
        if (rpspmc_pacpll){
                double zy_ratio = main_get_gapp()->xsm->Inst->YResolution () / main_get_gapp()->xsm->Inst->ZResolution ();
                //rpspmc_pacpll->write_parameter ("SPMC_SLOPE_SLEW", 100.0); // MIN 10
                //usleep (100000);
                rpspmc_pacpll->write_parameter ("SPMC_SLOPE_DZY", zy_ratio * self->area_slope_y);
        }
}

void RPSPMC_Control::ZPosSetChanged(Param_Control* pcs, RPSPMC_Control *self){
        if (rpspmc_pacpll){
                const gchar *SPMC_SET_ZPOS_SERVO_COMPONENTS[] = {
                        "SPMC_Z_SERVO_SETPOINT_CZ",
                        "SPMC_Z_SERVO_SETPOINT",
                        "SPMC_Z_SERVO_UPPER", 
                        NULL };
                double jdata[3];
                jdata[0] = main_get_gapp()->xsm->Inst->ZA2Volt(-self->zpos_ref); // FPGA  internal has pos Z feedback behavior, i.e. Z pos = tip down
                jdata[1] = self->mix_level[0] > 0. ? main_get_gapp()->xsm->Inst->nAmpere2V(self->mix_level[0])
                                                   : main_get_gapp()->xsm->Inst->nAmpere2V(self->mix_set_point[0]);
                jdata[2] = self->mix_level[0] > 0.  // Manual CZ-Control of Upper limit via Z-Position?
                        ? main_get_gapp()->xsm->Inst->ZA2Volt(-self->zpos_ref)  // FPGA  internal has pos Z feedback behavior, i.e. Z pos = tip down
                        : self->z_limit_upper_v; //5.; // UPPER
                if (self->mix_level[0] > 0.)
                        self->ec_z_upper->Freeze();
                else
                        self->ec_z_upper->Thaw();

                
                rpspmc_pacpll->write_array (SPMC_SET_ZPOS_SERVO_COMPONENTS, 0, NULL,  3, jdata);
        }

        if (rpspmc_hwi){
                rpspmc_hwi->add_user_event_now ("Z_Set_Point_adjust", "[A] (pre,now)", main_get_gapp()->xsm->data.s.ZSetPoint, self->zpos_ref);
                rpspmc_hwi->add_user_event_now ("I_Set_Point_adjust", "[nA] (setpt, level)", self->mix_set_point[0], self->mix_level[0], true);
        }
        
        // mirror basic parameters to GXSM4 main
        main_get_gapp()->xsm->data.s.SetPoint  = self->zpos_ref;
        main_get_gapp()->xsm->data.s.ZSetPoint = self->zpos_ref;
}

void RPSPMC_Control::ZServoParamChanged(Param_Control* pcs, RPSPMC_Control *self){
        if (rpspmc_pacpll){

                const gchar *SPMC_SET_Z_SERVO_COMPONENTS[] = {
                        "SPMC_Z_SERVO_MODE", 
                        "SPMC_Z_SERVO_SETPOINT",
                        "SPMC_Z_SERVO_IN_OFFSETCOMP", 
                        "SPMC_Z_SERVO_CP", 
                        "SPMC_Z_SERVO_CI", 
                        "SPMC_Z_SERVO_UPPER", 
                        "SPMC_Z_SERVO_LOWER", 
                        NULL };
                double jdata[6];
                int jdata_i[1];

                // obsolete invert, critical/no good here. Add final Z-polarity at very end
                self->z_servo[SERVO_CP] = pow (10., spmc_parameters.z_servo_cp_db/20.); 
                self->z_servo[SERVO_CI] = pow (10., spmc_parameters.z_servo_ci_db/20.);

                jdata_i[0] = (self->mix_transform_mode[0] & 0xff) | ((self->I_fir & 0xff) << 8); // TRANSFORM MDOE | FIR SELECTION
                jdata[0]   = self->mix_level[0] > 0. ? main_get_gapp()->xsm->Inst->nAmpere2V(self->mix_level[0])
                                                     : main_get_gapp()->xsm->Inst->nAmpere2V(self->mix_set_point[0]);
                jdata[1]   = main_get_gapp()->xsm->Inst->nAmpere2V(self->mix_in_offsetcomp[0]);
                jdata[2]   = self->z_servo[SERVO_CP];
                jdata[3]   = self->z_servo[SERVO_CI];
                jdata[4]   = self->mix_level[0] > 0.  // Manual CZ-Control of Upper limit via Z-Position?
                        ? main_get_gapp()->xsm->Inst->ZA2Volt(-self->zpos_ref)  // FPGA  internal has pos Z feedback behavior, i.e. Z pos = tip down
                        : self->z_limit_upper_v; //5.; // UPPER
                jdata[5]   = spmc_parameters.z_servo_lower; // -5.; // LOWER

                if (self->mix_level[0] > 0.)
                        self->ec_z_upper->Freeze();
                else
                        self->ec_z_upper->Thaw();
                
                rpspmc_pacpll->write_array (SPMC_SET_Z_SERVO_COMPONENTS, 1, jdata_i,  6, jdata);
        }

        if (rpspmc_hwi){
                rpspmc_hwi->add_user_event_now ("Z_Servo_adjust", "[dB] (CP,CI)", spmc_parameters.z_servo_cp_db, spmc_parameters.z_servo_ci_db );
                rpspmc_hwi->add_user_event_now ("Z_Set_Point_adjust", "[A] (pre,now)", main_get_gapp()->xsm->data.s.ZSetPoint, self->zpos_ref, true);
                rpspmc_hwi->add_user_event_now ("I_Set_Point_adjust", "[nA] (setpt, level)", self->mix_set_point[0], self->mix_level[0], true);
        }

        // mirror basic parameters to GXSM4 main
        main_get_gapp()->xsm->data.s.Current = self->mix_set_point[0];
}


void RPSPMC_Control::ZServoControl (GtkWidget *widget, RPSPMC_Control *self){
        /*
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))){
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_MODE", self->mix_transform_mode[0]);
                // set MixMode to grey/inconsistent
        } else {
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_MODE", MM_OFF);
                // update MixMode
        }
        */
}

// ** THIS INVERTS ONYL THE Z OUTPUT, does NOT effect any internal behaviors!
void RPSPMC_Control::ZServoControlInv (GtkWidget *widget, RPSPMC_Control* self){
        int flg = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));

        // CONFIRMATION DLG
        if (flg)
                if ( gapp->question_yes_no ("WARNING: Please confirm Z-Polarity set to NEGATIVE."))
                        rpspmc_pacpll->write_parameter ("SPMC_Z_POLARITY", -1);
                else
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (widget), true);
        else
                if ( gapp->question_yes_no ("WARNING: Please confirm Z-Polarity set to POSITIV."))
                        rpspmc_pacpll->write_parameter ("SPMC_Z_POLARITY", 1);
                else
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (widget), false);
}


void RPSPMC_Control::ChangedNotifyVP(Param_Control* pcs, RPSPMC_Control* self){
        g_message ("**ChangedNotifyVP**");

	//gchar *id=pcs->get_refname();

        
        for (int j=0; j<8; ++j)
                if (pcs == self->GVP_V0Set_ec[j]){
                        const gchar *info=self->GVP_V0Set_ec[j]->get_info();
                        if (info)
                                switch (info[0]){
                                case 'X': self->GVP_V0Set_ec[j]->set_css_name("bgrey"); break;
                                case 'S': self->GVP_V0Set_ec[j]->set_css_name("orange"); break;
                                default: self->GVP_V0Set_ec[j]->set_css_name("normal"); break;
                                }
                        else
                                self->GVP_V0Set_ec[j]->set_css_name("normal");
                        self->GVP_V0Set_ec[j]->Put_Value (); // update
                }
}

int RPSPMC_Control::choice_mixmode_callback (GtkWidget *widget, RPSPMC_Control *self){
	gint channel=0;
        gint selection=0;

        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

        channel = GPOINTER_TO_INT (g_object_get_data(G_OBJECT (widget), "mix_channel"));
        selection = atoi (gtk_combo_box_get_active_id (GTK_COMBO_BOX (widget)));


	PI_DEBUG_GP (DBG_L3, "MixMode[%d]=0x%x\n",channel,selection);

	self->mix_transform_mode[channel] = selection;
        if (channel == 0){
                g_print ("Choice MIX%d MT=%d\n", channel, selection);
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_MODE", (selection & 0xff) | ((self->I_fir&0xff) << 8)); // mapped to MM_LIN/LOG (0,1) | FIR SELECTION
        }
        PI_DEBUG_GP (DBG_L4, "%s ** 2\n",__FUNCTION__);

        self->ZServoParamChanged (NULL, self);

        PI_DEBUG_GP (DBG_L4, "%s **3 done\n",__FUNCTION__);
        return 0;
}

int RPSPMC_Control::choice_scansource_callback (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) == -1 || g_object_get_data( G_OBJECT (widget), "updating_in_progress")){
                PI_DEBUG_GP (DBG_L4, "%s bail out for label refresh only\n",__FUNCTION__);
                return 0;
        }
            
	gint selection = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        gint channel   = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "scan_channel_source"));

	self->scan_source[channel] = selection;

        g_message ("RPSPMC_Control::choice_scansource_callback: scan_source[%d] = %d [%s]", channel, selection, rpspmc_swappable_signals[selection].label);

	PI_DEBUG_GP (DBG_L3, "GVP STREAM MUX SELECTOR:  [%d] %s ==> [%d]\n",signal, rpspmc_swappable_signals[selection].label, channel);

        rpspmc_hwi->set_spmc_signal_mux (RPSPMC_ControlClass->scan_source);

        if (rpspmc_pacpll)
                rpspmc_pacpll->set_stream_mux (self->scan_source);

        main_get_gapp()->channelselector->SetModeChannelSignal(13+channel,
                                                               rpspmc_swappable_signals[selection].label,
                                                               rpspmc_swappable_signals[selection].label,
                                                               rpspmc_swappable_signals[selection].unit,
                                                               1.0 //rpspmc_swappable_signals[selection].scale_factor // ** data is scaled to unit at transfer time
                                                               );

        
        return 0;
}

void RPSPMC_Control::update_GUI(){
        if (!GUI_ready) return;

	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "DSP_EC_list"),
			(GFunc) App::update_ec, NULL);
	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "DSP_VPC_OPTIONS_list"),
			(GFunc) callback_update_GVP_vpc_option_checkbox, this);
}


void RPSPMC_Control::update_FPGA_from_GUI (){ // after cold start
        //zpos_ref = 0.; // ** via limits

        z_limit_upper_v = spmc_parameters.z_servo_upper; //5.; // READBACK UPPER

        g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (window), "FPGA_readback_update_list"), (GFunc) App::update_parameter, NULL); // UPDATE PARAMETRE form GUI!

        bias = spmc_parameters.bias_monitor; // Volts direct
        mix_set_point[0] = main_get_gapp()->xsm->Inst->V2nAmpere (spmc_parameters.z_servo_setpoint); // convert nA!!

        if (spmc_parameters.z_servo_cp > 0.0)
                spmc_parameters.z_servo_cp_db = 20.* log10(spmc_parameters.z_servo_cp); // convert to dB *** pow (10., spmc_parameters.z_servo_cp_db/20.);
        if (spmc_parameters.z_servo_ci > 0.0)
                spmc_parameters.z_servo_ci_db = 20.* log10(spmc_parameters.z_servo_ci); // convert to dB

        // push to FPGA
        BiasChanged(NULL, this);
        ZServoParamChanged (NULL, this);
        Slope_dZX_Changed(NULL, this);
        Slope_dZY_Changed(NULL, this);
        ZPosSetChanged(NULL, this);
        ZServoControl (NULL, this);
}

void RPSPMC_Control::update_GUI_from_FPGA (){ // after warm start or re-connect
        //zpos_ref = 0.; // ** via limits
        bias = spmc_parameters.bias_monitor; // Volts direct
        mix_set_point[0] = main_get_gapp()->xsm->Inst->V2nAmpere (spmc_parameters.z_servo_setpoint); // convert nA!!

        if (spmc_parameters.z_servo_cp > 0.0)
                spmc_parameters.z_servo_cp_db = 20.* log10(spmc_parameters.z_servo_cp); // convert to dB *** pow (10., spmc_parameters.z_servo_cp_db/20.);
        if (spmc_parameters.z_servo_ci > 0.0)
                spmc_parameters.z_servo_ci_db = 20.* log10(spmc_parameters.z_servo_ci); // convert to dB
        
        g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (window), "FPGA_readback_update_list"), (GFunc) App::update_ec, NULL); // UPDATE GUI!
}

void RPSPMC_Control::update_zpos_readings(){
        //double zp,a,b;
        //main_get_gapp()->xsm->hardware->RTQuery ("z", zp, a, b);
        zpos_mon = main_get_gapp()->xsm->Inst->Volt2ZA(spmc_parameters.gxsm_z_polarity*spmc_parameters.z_monitor + spmc_parameters.zs_monitor); // remove slope component
        gchar *info;
        if (-spmc_parameters.zs_monitor >= 0.)
                info = g_strdup_printf (" + %g "UTF8_ANGSTROEM" Slp",  main_get_gapp()->xsm->Inst->Volt2ZA(-spmc_parameters.zs_monitor));
        else
                info = g_strdup_printf (" %g "UTF8_ANGSTROEM" Slp",  main_get_gapp()->xsm->Inst->Volt2ZA(-spmc_parameters.zs_monitor));
        ZPos_ec->set_info (info);
        ZPos_ec->Put_Value ();
        g_free (info);
}

#if 0
guint RPSPMC_Control::refresh_zpos_readings(RPSPMC_Control *self){ 
	if (main_get_gapp()->xsm->hardware->IsSuspendWatches ())
		return TRUE;

	self->update_zpos_readings ();
	return TRUE;
}

int RPSPMC_Control::zpos_monitor_callback( GtkWidget *widget, RPSPMC_Control *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))){
                //g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (widget), "DSP_zpos_control_list"), (GFunc) App::thaw_ec, NULL);
                self->zpos_refresh_timer_id = g_timeout_add (200, (GSourceFunc)RPSPMC_Control::refresh_zpos_readings, self);
        }else{
                //g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (widget), "DSP_zpos_control_list"), (GFunc) App::freeze_ec, NULL);

                if (self->zpos_refresh_timer_id){
                        g_source_remove (self->zpos_refresh_timer_id);
                        self->zpos_refresh_timer_id = 0;
                }
        }
        return 0;
}
#endif

int RPSPMC_Control::choice_prbsource_callback (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
        
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) == -1)
                return 0;

        gint selection = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	gint channel   = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "prb_channel_source"));

	self->probe_source[channel] = selection;

        if (RPSPMC_ControlClass && rpspmc_pacpll){
                rpspmc_hwi->set_spmc_signal_mux (RPSPMC_ControlClass->probe_source);
                rpspmc_pacpll->set_stream_mux (RPSPMC_ControlClass->probe_source);
        }
        g_message ("RPSPMC_Control::choice_prbsource_callback: probe_source[%d] = %d [%s] MUX=%016lx", channel, selection,
                   rpspmc_swappable_signals[selection].label,
                   __GVP_selection_muxval (self->probe_source));

        return 0;
}



int RPSPMC_Control::callback_change_IV_option_flags (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		self->IV_option_flags = (self->IV_option_flags & (~msk)) | msk;
	else
		self->IV_option_flags &= ~msk;

        self->set_tab_settings ("IV", self->IV_option_flags, self->IV_auto_flags, self->IV_glock_data);
        return 0;
}

int RPSPMC_Control::callback_change_IV_auto_flags (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		self->IV_auto_flags = (self->IV_auto_flags & (~msk)) | msk;
	else
		self->IV_auto_flags &= ~msk;

	//if (self->write_vector_mode == PV_MODE_IV)
	//	self->raster_auto_flags = self->IV_auto_flags;

        self->set_tab_settings ("IV", self->IV_option_flags, self->IV_auto_flags, self->IV_glock_data);
        return 0;
}

int RPSPMC_Control::Probing_RampFBoff_callback(GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
        self->RampFBoff_mode = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
        return 0;
}

int RPSPMC_Control::Probing_multiIV_callback(GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "DSP_multiIV_list"),
				(GFunc) gtk_widget_show, NULL);
	else
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "DSP_multiIV_list"),
				(GFunc) gtk_widget_hide, NULL);

        self->multiIV_mode = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
        return 0;
}


int RPSPMC_Control::Probing_exec_IV_callback( GtkWidget *widget, RPSPMC_Control *self){

        if (rpspmc_hwi->is_scanning()){
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is busy scanning. Please stop scanning for any GVP actions.");
                //gapp->warning ("RPSPMC is busy scanning.\nPlease stop scanning and any GVP actions.", window);
                return -1;
        }

	if (self->check_vp_in_progress ()){
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is streaming GVP data -- ABORT FIRST");
                return -1;
        }

        
        self->current_auto_flags = self->IV_auto_flags;


        rpspmc_hwi->set_spmc_signal_mux (RPSPMC_ControlClass->probe_source);
        if (rpspmc_pacpll)
                rpspmc_pacpll->set_stream_mux (self->probe_source);

        self->init_vp_signal_info_lookup_cache(); // update signal mapping cache

        if (self->IV_auto_flags & FLAG_AUTO_RUN_INITSCRIPT){
                gchar *tmp = g_strdup ("vp-sts-initial");
                main_get_gapp()->SignalRemoteActionToPlugins (&tmp);
                g_free (tmp);
        }

	if (self->IV_auto_flags & FLAG_AUTO_GLOCK){
		self->vis_Source  = self->IV_glock_data[0];
		self->vis_XSource = self->IV_glock_data[1];
		self->vis_PSource = self->IV_glock_data[2];
		self->vis_XJoin   = self->IV_glock_data[3];
		self->vis_PlotAvg = self->IV_glock_data[4];
		self->vis_PlotSec = self->IV_glock_data[5];
	} else {
		self->vis_Source = self->IV_glock_data[0] = self->Source ;
		self->vis_XSource= self->IV_glock_data[1] = self->XSource;
		self->vis_PSource= self->IV_glock_data[2] = self->PSource;
		self->vis_XJoin  = self->IV_glock_data[3] = self->XJoin  ;
		self->vis_PlotAvg= self->IV_glock_data[4] = self->PlotAvg;
		self->vis_PlotSec= self->IV_glock_data[5] = self->PlotSec;
	}

	self->current_auto_flags = self->IV_auto_flags;

        // write and exec GVP code on controller and initiate data streaming

	self->write_spm_vector_program (1, PV_MODE_IV); // Write and Exec GVP program

	return 0;
}

int RPSPMC_Control::Probing_write_IV_callback( GtkWidget *widget, RPSPMC_Control *self){
         if (rpspmc_hwi->is_scanning()){
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is busy scanning. Please stop scanning for any GVP actions.");
                //gapp->warning ("RPSPMC is busy scanning.\nPlease stop scanning and any GVP actions.", window);
                return -1;
        }

        if (rpspmc_hwi->probe_fifo_thread_active>0){
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is streaming GVP data -- FORCE UPDATE REQUESTED");
        }

        // write GVP code to controller
        self->write_spm_vector_program (0, PV_MODE_IV);
        return 0;
}

// GVP vector program editor 

int RPSPMC_Control::callback_update_GVP_vpc_option_checkbox (GtkWidget *widget, RPSPMC_Control *self){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	int  k   = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "VPC"));
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	gtk_check_button_set_active (GTK_CHECK_BUTTON(widget), (self->GVP_opt[k] & msk) ? 1:0);

	if( gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))){
		g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTOPC_list"),
				(GFunc) gtk_widget_show, NULL);
		g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTHDR_list"),
				(GFunc) gtk_widget_show, NULL);
	} else {
		g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTOPC_list"),
				(GFunc) gtk_widget_hide, NULL);
		g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTHDR_list"),
				(GFunc) gtk_widget_hide, NULL);
        }


        
        self->set_tab_settings ("VP", self->GVP_option_flags, self->GVP_auto_flags, self->GVP_glock_data);
        return 0;
}

int RPSPMC_Control::callback_change_LCK_mode (GtkWidget *widget, RPSPMC_Control *self){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(widget), "Bit_Mask"));
	if( gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                spmc_parameters.lck_mode |= msk;
        else
                spmc_parameters.lck_mode &= ~msk;

        g_message ("RPSPMC_Control::callback_change_LCK_mode %d", spmc_parameters.lck_mode);
        
        return 0;
}

int RPSPMC_Control::callback_change_RF_mode (GtkWidget *widget, RPSPMC_Control *self){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(widget), "Bit_Mask"));
	if( gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                spmc_parameters.rf_gen_mode |= msk;
        else
                spmc_parameters.rf_gen_mode &= ~msk;

        g_message ("RPSPMC_Control::callback_change_RF_mode %d", spmc_parameters.rf_gen_mode);
        
        return 0;
}

int RPSPMC_Control::callback_change_GVP_vpc_option_flags (GtkWidget *widget, RPSPMC_Control *self){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	int  k = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "VPC"));
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(widget), "Bit_Mask"));

        int x=self->GVP_opt[k];
        //g_message (" RPSPMC_Control::callback_change_GVP_vpc_option_flags :: changed [%d] %04x ^ %04x",k,x,msk);
	if( gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))){
		self->GVP_opt[k] = (self->GVP_opt[k] & (~msk)) | msk;
                if (msk & (1<<7)){
                        g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTOPC_list"),
                                        (GFunc) gtk_widget_show, NULL);
                        g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTHDR_list"),
                                        (GFunc) gtk_widget_show, NULL);
                }
	} else {
		self->GVP_opt[k] &= ~msk;
                if (msk & (1<<7) && x & (1<<7)){
                        //g_message (" RPSPMC_Control::callback_change_GVP_vpc_option_flags :: uncheck VX [%d]",k);
                        g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTOPC_list"),
                                        (GFunc) gtk_widget_hide, NULL);
                        int vx=0;
                        for (int j=0; j < N_GVP_VECTORS; ++j) 
                                if (self->GVP_opt[j]&(1<<7)) { vx++; break; }
                        if (!vx)
                                g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTHDR_list"),
                                                (GFunc) gtk_widget_hide, NULL);
                }
        }
        
        self->set_tab_settings ("VP", self->GVP_option_flags, self->GVP_auto_flags, self->GVP_glock_data);

        if (k==0)
                if ((msk & VP_INITIAL_SET_VEC) && (x & VP_INITIAL_SET_VEC)) // un-checked
                        for (int j=0; j<8; ++j){
                                self->GVP_V0Set_ec[j]->set_info(NULL);
                                self->GVP_V0Set_ec[j]->set_css_name("normal");
                                self->GVP_V0Set_ec[j]->Put_Value (); // update
                        }
                else if ((msk & VP_INITIAL_SET_VEC) && !(x & VP_INITIAL_SET_VEC)) // checked
                        for (int j=0; j<8; ++j){
                                switch (j){
                                case 0: case 1:
                                        self->GVP_V0Set_ec[j]->set_info("X");
                                        self->GVP_V0Set_ec[j]->set_css_name("bgrey");
                                        break;
                                default:
                                        self->GVP_V0Set_ec[j]->set_info("Set");
                                        self->GVP_V0Set_ec[j]->set_css_name("orange");
                                        break;
                                }
                                self->GVP_V0Set_ec[j]->Put_Value (); // update
                        }

        return 0;
}

int RPSPMC_Control::callback_change_GVP_option_flags (GtkWidget *widget, RPSPMC_Control *self){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		self->GVP_option_flags = (self->GVP_option_flags & (~msk)) | msk;
	else
		self->GVP_option_flags &= ~msk;

	if (self->write_vector_mode == PV_MODE_GVP)
		self->raster_auto_flags = self->GVP_auto_flags;

        self->set_tab_settings ("VP", self->GVP_option_flags, self->GVP_auto_flags, self->GVP_glock_data);
        self->GVP_store_vp ("VP_set_last"); // last in view
        return 0;
}

int RPSPMC_Control::callback_GVP_store_vp (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	self->GVP_store_vp ((const gchar*)g_object_get_data(G_OBJECT(widget), "key"));
        return 0;
}
int RPSPMC_Control::callback_GVP_restore_vp (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	self->GVP_restore_vp ((const gchar*)g_object_get_data(G_OBJECT(widget), "key"));
        return 0;
}


int RPSPMC_Control::callback_GVP_preview_me (GtkWidget *widget, RPSPMC_Control *self){
        int i = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "AXIS"));
        if (i>0 && i <= 8){
                self->GVP_preview_on[i] = self->GVP_preview_on[i] ? 0:1;

                if (self->GVP_preview_on[i]){
                        const gchar *gvpcolor = g_strdup_printf("gvpcolor%d",i);
                        gtk_widget_set_name (widget, gvpcolor);
                        g_free (gvpcolor);
                } else
                        gtk_widget_set_name (widget, "normal");
        }

        // auto hide/show
        int num_previews=0;
        for(int k=0; k < 8; ++k)
                num_previews += self->GVP_preview_on[k];

        if (num_previews > 0)
                gtk_widget_show (self->WavePreview);
        else
                gtk_widget_hide (self->WavePreview);
        
        return 0;
}


int RPSPMC_Control::callback_change_GVP_auto_flags (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		self->GVP_auto_flags = (self->GVP_auto_flags & (~msk)) | msk;
	else
		self->GVP_auto_flags &= ~msk;

        self->set_tab_settings ("VP", self->GVP_option_flags, self->GVP_auto_flags, self->GVP_glock_data);
        return 0;
}

int RPSPMC_Control::Probing_exec_GVP_callback( GtkWidget *widget, RPSPMC_Control *self){
	self->current_auto_flags = self->GVP_auto_flags;

        if (!self->check_GVP())
                return -1;
        
        if (rpspmc_hwi->is_scanning()){
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is busy scanning. Please stop scanning for any GVP actions.");
                if (gapp->question_yes_no ("WARNING: Scan in progress.\n"
                                           "RPSPMC is currently streaming GVP data.\n"
                                           "Abort current activity/scanning and start GVP?", self->window)){
                        rpspmc_hwi->GVP_abort_vector_program ();
                        usleep(200000);
                } else
                        return -1;
        }

	if (self->check_vp_in_progress ()){ 
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is streaming GVP data -- ABORT FIRST");
                if (gapp->question_yes_no ("WARNING: Scan/Probe in progress.\n"
                                           "RPSPMC is currently streaming GVP data.\n"
                                           "Abort current activity and start GVP?",  self->window)){
                        rpspmc_hwi->GVP_abort_vector_program ();
                        usleep(200000);
                } else
                        return -1;
        }

        // make sure all is sane and cleaned up
        rpspmc_hwi->GVP_abort_vector_program ();
        usleep(100000);

        rpspmc_hwi->set_spmc_signal_mux (RPSPMC_ControlClass->probe_source);
        if (rpspmc_pacpll)
                rpspmc_pacpll->set_stream_mux (self->probe_source);

        self->init_vp_signal_info_lookup_cache(); // update signal mapping cache


        if (self->GVP_auto_flags & FLAG_AUTO_RUN_INITSCRIPT){
                gchar *tmp = g_strdup ("vp-gvp-initial");
                main_get_gapp()->SignalRemoteActionToPlugins (&tmp);
                g_free (tmp);
        }
      
	if (self->GVP_auto_flags & FLAG_AUTO_GLOCK){
		self->vis_Source  = self->GVP_glock_data[0];
		self->vis_XSource = self->GVP_glock_data[1];
		self->vis_PSource = self->GVP_glock_data[2];
		self->vis_XJoin   = self->GVP_glock_data[3];
		self->vis_PlotAvg = self->GVP_glock_data[4];
		self->vis_PlotSec = self->GVP_glock_data[5];
	} else {
		self->vis_Source = self->GVP_glock_data[0] = self->Source ;
		self->vis_XSource= self->GVP_glock_data[1] = self->XSource;
		self->vis_PSource= self->GVP_glock_data[2] = self->PSource;
		self->vis_XJoin  = self->GVP_glock_data[3] = self->XJoin  ;
		self->vis_PlotAvg= self->GVP_glock_data[4] = self->PlotAvg;
		self->vis_PlotSec= self->GVP_glock_data[5] = self->PlotSec;
	}

	self->current_auto_flags = self->GVP_auto_flags;

        // write and exec GVP code on controller and initiate data streaming
        
	self->write_spm_vector_program (1, PV_MODE_GVP); // Write and Exec GVP program

	return 0;
}


int RPSPMC_Control::Probing_write_GVP_callback( GtkWidget *widget, RPSPMC_Control *self){

        self->check_GVP();
        
        if (rpspmc_hwi->is_scanning()){
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is busy scanning. Please stop scanning for any GVP actions.");
                //gapp->warning ("RPSPMC is busy scanning.\nPlease stop scanning and any GVP actions.", window);
                return -1;
        }

        if (rpspmc_hwi->probe_fifo_thread_active>0){
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is streaming GVP data -- FORCE UPDATE REQUESTED");
        }

        // write GVP code to controller
        self->write_spm_vector_program (0, PV_MODE_GVP);

        gtk_widget_queue_draw (self->gvp_preview_area); // update wave

        return 0;
}

// Graphs Callbacks
int RPSPMC_Control::change_source_callback (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint32 channel = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "Source_Channel"));
	guint32 mapping = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "Source_Mapping"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                switch (mapping){
                case MAP_SOURCE_REC:   self->Source  |= channel; break;
                case MAP_SOURCE_X:     self->XSource |= channel; break;
                case MAP_SOURCE_PLOTY: self->PSource |= channel; break;
                case MAP_SOURCE_AVG:   self->PlotAvg |= channel; break;
                case MAP_SOURCE_SEC:   self->PlotSec |= channel; break;
                default: break;
                }
        else
                switch (mapping){
                case MAP_SOURCE_REC:   self->Source  &= ~channel; break;
                case MAP_SOURCE_X:     self->XSource &= ~channel; break;
                case MAP_SOURCE_PLOTY: self->PSource &= ~channel; break;
                case MAP_SOURCE_AVG:   self->PlotAvg &= ~channel; break;
                case MAP_SOURCE_SEC:   self->PlotSec &= ~channel; break;
                default: break;
                }

        g_message ("CH=%08x MAP:%d *** S%08x X%08x Y%08x",channel, mapping, self->Source,self->XSource,self->PSource );
        
        //g_message ("change_source_callback: Ch: %08x => Srcs: %08x", channel, self->Source);
        
	self->vis_Source  = self->Source;
	self->vis_XSource = self->XSource;
	self->vis_PSource = self->PSource;
	self->vis_XJoin   = self->XJoin;
	self->vis_PlotAvg = self->PlotAvg;
	self->vis_PlotSec = self->PlotSec;

        // store
        self->store_graphs_values ();
        
	return 0;
}

int RPSPMC_Control::callback_XJoin (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	self->XJoin = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? TRUE : FALSE;
	self->vis_XJoin = self->XJoin;
        g_settings_set_boolean (self->hwi_settings, "probe-x-join", self->XJoin);
        return 0;
}

int RPSPMC_Control::callback_GrMatWindow (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	self->GrMatWin = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? TRUE : FALSE;
        g_settings_set_boolean (self->hwi_settings, "probe-graph-matrix-window", self->GrMatWin);
        return 0;
}

#define QQ44 (1LL<<44)

// Q44: 32766.0000 Hz -> phase_inc=4611404543  0000000112dc72ff
double dds_phaseinc (double freq){
        double fclk = 125e6; //ADC_SAMPLING_RATE;
        return QQ44*freq/fclk;
}


void RPSPMC_Control::configure_filter(int id, int mode, double sos[6], int decimation=0){
        const gchar *SPMC_SET_BQ_COMPONENTS[] = {
                "SPMC_SC_FILTER_MODE",   // INT
                "SPMC_SC_FILTER_SELECT", // INT ** SECTION / FILTER BLOCK SELECTION
                "SPMC_SC_BQ_COEF_B0", //spmc_parameters.sc_lck_bq_coef[0,1,2]
                "SPMC_SC_BQ_COEF_B1",
                "SPMC_SC_BQ_COEF_B2",
                "SPMC_SC_BQ_COEF_A0", //spmc_parameters.sc_lck_bq_coef[3,4,5] ** A0 == 1 per def, been ignored.
                "SPMC_SC_BQ_COEF_A1",
                "SPMC_SC_BQ_COEF_A2",
                NULL };

        int jdata_i[2] = { mode & 0xffff | (decimation << 16), id };
                
        g_message ("Config BQ Filter #%d (SOS AB coef)", id);
        rpspmc_pacpll->write_array (SPMC_SET_BQ_COMPONENTS, 2, jdata_i,  6, sos);
}

void RPSPMC_Control::delayed_filter_update (){
        configure_filter (1, spmc_parameters.sc_bq1mode, spmc_parameters.sc_bq1_coef, BQ_decimation);
        usleep (200000);
        configure_filter (2, spmc_parameters.sc_bq2mode, spmc_parameters.sc_bq2_coef, BQ_decimation);
        delayed_filter_update_timer_id = 0; // done.
        delayed_filter_update_ref = 0;
}

static guint RPSPMC_Control::delayed_filter_update_callback (RPSPMC_Control *self){
        self->delayed_filter_update ();
	return FALSE;
}

void RPSPMC_Control::bq_filter_adjust_callback(Param_Control* pcs, RPSPMC_Control *self){
        self->BQ_decimation = 128; //1 + (int)round(8 * 5000. / spmc_parameters.lck_frequency);
        spmc_parameters.lck_bq_dec_monitor = self->BQ_decimation;
        g_message ("BQ Filter Deciamtion: #%d",  self->BQ_decimation);
        //self->configure_filter (1, spmc_parameters.sc_bq1mode, spmc_parameters.sc_bq1_coef, decimation);
        //self->configure_filter (2, spmc_parameters.sc_bq2mode, spmc_parameters.sc_bq2_coef, decimation);

        // if not already scheduled, schedule delayed

        if (self->delayed_filter_update_timer_id) // if scheduled, remove and reset timeout next
                g_source_remove (self->delayed_filter_update_timer_id);

        self->delayed_filter_update_ref++;
        
        //if (!self->delayed_filter_update_timer_id) // if scheduled, remove and reset timeout next
        self->delayed_filter_update_timer_id = g_timeout_add (1000, (GSourceFunc)RPSPMC_Control::delayed_filter_update_callback, self);
        
}

void RPSPMC_Control::delayed_zsfilter_update (){
        configure_filter (10, spmc_parameters.sc_zs_bqmode, spmc_parameters.sc_zs_bq_coef, 50); // ZS / Notch DECIMATION
        delayed_zsfilter_update_timer_id = 0; // done.
}

static guint RPSPMC_Control::delayed_zsfilter_update_callback (RPSPMC_Control *self){
        self->delayed_zsfilter_update ();
	return FALSE;
}
void RPSPMC_Control::zs_input_filter_adjust_callback(Param_Control* pcs, RPSPMC_Control *self){
        // Update BQ SECTION ZS =10 (Z SERVO INPUT BiQuad) [can be used to program a NOTCH]
        // Test-DECII=32 for NOTCH intended IIR @ ~2MSPS / 32
        //self->configure_filter (10, spmc_parameters.sc_zs_bqmode, spmc_parameters.sc_zs_bq_coef, 128);
        // if not already scheduled, schedule delayed update

        if (self->delayed_zsfilter_update_timer_id)
                g_source_remove (self->delayed_zsfilter_update_timer_id);

        //if (!self->delayed_zsfilter_update_timer_id)
        self->delayed_zsfilter_update_timer_id = g_timeout_add (1000, (GSourceFunc)RPSPMC_Control::delayed_zsfilter_update_callback, self);
}

void RPSPMC_Control::lockin_adjust_callback(Param_Control* pcs, RPSPMC_Control *self){
        if (rpspmc_pacpll){

                double rp_fs = 125e6; // actual sampling freq
                int naclks   = (int)(spmc_parameters.lck_aclocks_per_sample_monitor) % 1000;
                int decii_mon= (int)(spmc_parameters.lck_decii_monitor);
                int decii2   = 0;
                if (naclks < 1 || naclks > 512) naclks = 66; // backup, 59 nominally
                double fn    = rp_fs / naclks / spmc_parameters.lck_frequency;

                int decii2_max = 12;

                // calculate decii2
                while (fn > 20. && decii2 < decii2_max){
                        decii2++;
                        fn /= 2.0;
                }

                g_message ("LCK Adjust Calc: RP-Fs: %g Hz, AD-NACLKs: %d, FLCK/BQ: %g Hz, Fnorm: %g @ decii2: %d", rp_fs, naclks, rp_fs/naclks/(1<<decii2), fn, decii2);

                
                const gchar *SPMC_LCK_COMPONENTS[] = {
                        "SPMC_LCK_MODE",      // INT
                        "SPMC_LCK_GAIN",      // INT (SHIFT) IN, OUT
                        "SPMC_LCK_FREQUENCY",
                        "SPMC_LCK_VOLUME",
                        NULL };
                int jdata_i[2];
                double jdata[2];
                jdata_i[0] = spmc_parameters.lck_mode;
                if (spmc_parameters.lck_sens <= 0.){
                        g_message ("LCK Gain defaulting to 1x (0)");
                        jdata_i[1] = 0;
                } else {
                        /* on RP:
                          int decii2_max   = 12;
                          int max_gain_out = 10;
                          int gain_in  =  gain      & 0xff; // 1x : == decii2
                          int gain_out = (gain>>8)  & 0xff; // 1x : == 
                          
                          gain_in  = gain_in  > decii2 ? 0 : decii2-gain_in;
                          gain_out = gain_out > max_gain_out ? max_gain_out : gain_out;

                          on FPGA:
                          ampl2_norm_shr <= 2*LCK_CORRSUM_Q_WIDTH - AM2_DATA_WIDTH - gain_out[10-1:0];
                          ... signal     <= signal_dec >>> gain_in[DECII2_MAX-1:0]; // custom reduced norm, MAX, full norm: decii2  ***** correleation producst/sums will overrun for large siganls if not at FULL NORM!!
                          ... ampl2 <= ab2 >>> ampl2_norm_shr; // Q48 for SQRT:   Norm Square, reduce to AM2 WIDTH


                        */
                        
                        double s = 1e-3*spmc_parameters.lck_sens; // mV to V
                        double g = 5.0/s; // to gain. I.e. gain 1x for 5V (5000mV)
                        if (g < 1.) g=1.;
                        int  gl2 = int(log2(g));
                        int  gain_in = gl2 <= decii2_max ? gl2 : decii2_max;
                        g_message ("LCK Gain request: %g -> shift#: %d ", g, gain_in);
                        int gain_out=0;

        
                        if (gain_out == 0 && gain_in > decii2)
                                gain_out = gain_in - decii2;

                        self->lck_gain = (1<<gain_in) * (1<<gain_out);
                        self->LCK_unit->set_gain (1./(double)((1<<gain_in) * (1<<gain_out)));
                        gchar *tmp = g_strdup_printf ("[%d x %d] %d", 1<<gain_in, 1<<gain_out, (1<<gain_in) * (1<<gain_out));
                        self->LCK_Sens->set_info (tmp);
                        self->LCK_Sens->Put_Value ();
                        g_free (tmp);
                        
                        jdata_i[1] = (decii2<<16) | (gain_out<<8) | gain_in;
                }
                jdata[0] =  spmc_parameters.lck_frequency;

                if  (self->LCK_Target > 0 && self->LCK_Target < LCK_NUM_TARGETS)
                        jdata[1] = modulation_targets[self->LCK_Target].scale_factor * self->LCK_Volume[self->LCK_Target]; // => Volts
                else
                        jdata[1] = 0.;

                g_message ("LCK Adjust SENDING UPDATE T#%d M:%d G:<<%x F:%g Hz V: %g {%g} Decii2 requested: %d => fs=%g Hz, fn=%g Hz ", self->LCK_Target, jdata_i[0], jdata_i[1], jdata[0], self->LCK_Volume[self->LCK_Target], jdata[1], decii2, rp_fs/naclks/(1<<decii2), fn);
                rpspmc_pacpll->write_array (SPMC_LCK_COMPONENTS, 2, jdata_i,  2, jdata);
        }
}


void RPSPMC_Control::rfgen_adjust_callback(Param_Control* pcs, RPSPMC_Control *self){
        if (rpspmc_pacpll){
                const gchar *SPMC_RFGEN_COMPONENTS[] = {
                        "SPMC_RF_GEN_MODE",      // INT
                        "SPMC_RF_GEN_FREQUENCY",
                        "SPMC_RF_GEN_FMSCALE",
                        NULL };
                int jdata_i[1];
                double jdata[2];
                jdata_i[0] = spmc_parameters.rf_gen_mode;
                jdata[0]   = spmc_parameters.rf_gen_frequency;
                jdata[1]   = spmc_parameters.rf_gen_fmscale;
                g_message ("RF-GEN Adjust SENDING MODE, RF-FREQ, FM-SCALE");
                rpspmc_pacpll->write_array (SPMC_RFGEN_COMPONENTS, 1, jdata_i,  2, jdata);
        }
}

int RPSPMC_Control::choice_mod_target_callback (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

	self->LCK_Target = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        
        for (int jj=1; modulation_targets[jj].label && jj < LCK_NUM_TARGETS; ++jj) //  ** omit last!
                gtk_widget_set_sensitive (self->LCK_VolumeEntry[jj], jj == self->LCK_Target);
        
        if (rpspmc_pacpll){
                g_message ("Setting LCK TARGET: %d", self->LCK_Target);
                rpspmc_pacpll->write_parameter ("SPMC_LCK_TARGET", self->LCK_Target);
                self->lockin_adjust_callback (NULL, self); // update correct volume
        }
        return 0;
}

int RPSPMC_Control::choice_BQfilter_type_callback (GtkWidget *widget, RPSPMC_Control *self){
	int id = gtk_combo_box_get_active (GTK_COMBO_BOX (widget)); // filter type/config mode PASS, IIR, BQIIR, AB, STOP

        int BQsec = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "section"))-1;

        switch (BQsec){
        case 0: spmc_parameters.sc_bq1mode = id;
                bq_filter_adjust_callback (NULL, self);
                break;
        case 1: spmc_parameters.sc_bq2mode = id;
                bq_filter_adjust_callback (NULL, self);
                break;
        case 2: spmc_parameters.sc_zs_bqmode = id;
                zs_input_filter_adjust_callback (NULL, self);
                break;
        }

        if (BQsec == 0 || BQsec == 1 || BQsec == 2){

                gchar *bqid[] = {"LCK-SEC1", "LCK-SEC2", "ZS-Input" };
                gchar *wids[3][3] = {{ "IIR1-TC", "BQ1-TC", "BQ1-Q" }, { "IIR2-TC", "BQ2-TC", "BQ2-Q" }, { "IIRZS-TC", "BQZS-TC", "BQZS-Q" }};
                switch (id){
                case 0:
                        g_message ("Set %s BQ to ONE, PASS", bqid[BQsec]);
                        break;
                case 1:
                        g_message ("BQ: N/A --- **IIR 1st");
                        break;
                case 2:
                        g_message ("BQ: N/A --- **BiQuad 2nd");
                        break;
                case 3:
                        g_message ("Set %s BiQuad SOS from AB-Coef", bqid[BQsec]);
                        break;
                case 4:
                        g_message ("Set %s BiQuad SOS to NULL (STOP)", bqid[BQsec]); break;
                        break;
                case 5:
                        g_message ("Set %s BiQuad SOS to By-Pass Mode", bqid[BQsec]); break;
                        break;
                case 6:
                        g_message ("Set %s BiQuad SOS to STOP", bqid[BQsec]);
                        break;
                }
        }
        
        return 0;
}


int RPSPMC_Control::choice_z_servo_current_source_callback (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

	int id = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

        switch (id){
        case 2: // AD3-FIR 512
                id=1;
                self->I_fir = 1;
                break;
        case 3: // AD3-FIRX 128
                id=1;
                self->I_fir = 2;
                break;
        default:
                self->I_fir = 0;
                break;
        }
        if (rpspmc_pacpll){
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_MODE", self->mix_transform_mode[0] | (self->I_fir << 8)); // mapped to MM_LIN/LOG/FCZLOG (0,1,3) | FIR SELECTION
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_SRC_MUX", id);
        }
        
        for (int k=0; rpspmc_source_signals[k].mask; ++k)
                if (rpspmc_source_signals[k].mask == 0x00000010){ // update as of 1V/5V "Current Input" Volt Scale Range Change from signal list
                        rpspmc_source_signals[k].scale_factor = z_servo_current_source[id].scale_factor; break;
                }

        // rpspmc_hwi->update_hardware_mapping_to_rpspmc_source_signals (); // not required as mapping to volts is done with the above

        return 0;
}

int RPSPMC_Control::choice_rf_gen_out_callback (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

	int id = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        
        if (rpspmc_pacpll)
                rpspmc_pacpll->write_parameter ("SPMC_RF_GEN_OUT_MUX", id);
        
        return 0;
}


void RPSPMC_Control::on_new_data (){
        int s=(int)round(spmc_parameters.gvp_status);
        int Sgvp = (s>>8) & 0xf;  // assign GVP_status = { sec[32-4:0], setvec, reset, pause, ~finished };
        int Sspm = s & 0xff;      // assign SPM status = { ...SPM Z Servo Hold, GVP-FB-Hold, GVP-Finished, Z Servo EN }

        int fb=0;
        mon_FB =  (fb=((Sspm & 0x01) && !(Sspm & 0x04) && !(Sspm & 0x04) && !(Sspm & 0x08)) ? 1:0)
                + ((Sgvp & 0x01 ? 1:0) << 1);

        gchar *gvp_status = g_strdup_printf (" VPC: %d B: %d I:%d",
                                             current_probe_section,
                                             current_probe_block_index,
                                             current_probe_data_index);

        if (fb)
                gtk_widget_set_name (gvp_monitor_fb_label, "green");
        else
                gtk_widget_set_name (gvp_monitor_fb_label, "red");

        gvp_monitor_fb_info_ec->set_info(gvp_status);
        g_free (gvp_status);

        GVP_XYZ_mon_AA[0] = main_get_gapp()->xsm->Inst->Volt2XA (spmc_parameters.xs_monitor);
        GVP_XYZ_mon_AA[1] = main_get_gapp()->xsm->Inst->Volt2YA (spmc_parameters.ys_monitor);
        GVP_XYZ_mon_AA[2] = main_get_gapp()->xsm->Inst->Volt2ZA (spmc_parameters.z0_monitor);
                
        // RPSPM-Monitors: Lck, GVP
        if (G_IS_OBJECT (window))
                g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "SPMC_MONITORS_list"),
                                (GFunc) App::update_ec, NULL);

        update_zpos_readings();
}



void RPSPMC_Control::delayed_vector_update (){
        // SCAN SPEED COMPUTATIONS
        double slew[2];
        slew[0] = scan_speed_x = scan_speed_x_requested; // FIX ME -- recalc actual scan speed from vectors -> scan_speed_x!
        slew[1] = fast_return * scan_speed_x_requested;
        double scanpixelrate = main_get_gapp()->xsm->data.s.dx/slew[0];
        
        gchar *info = g_strdup_printf (" (%g ms/pix, %g Hz)", scanpixelrate*1e3, 1./scanpixelrate);
        scan_speed_ec->set_info (info);
        g_message ("Delayed Scan Speed Update: rewriting GVP Scan code for %s", info);
        g_free (info);

        if (rpspmc_hwi->is_scanning()) // only if scanning!
                write_spm_scan_vector_program (main_get_gapp()->xsm->data.s.rx, main_get_gapp()->xsm->data.s.ry,
                                               main_get_gapp()->xsm->data.s.nx, main_get_gapp()->xsm->data.s.ny,
                                               slew, NULL, NULL);
        delayed_vector_update_timer_id = 0; // done.

        if (rpspmc_hwi)
                rpspmc_hwi->add_user_event_now ("ScanSpeed_adjust", "[A/s] (speed,fast_ret_fac)", scan_speed_x, fast_return );
        
}

guint RPSPMC_Control::delayed_vector_update_callback (RPSPMC_Control *self){
        self->delayed_vector_update ();
	return FALSE;
}

void RPSPMC_Control::ChangedNotifyScanSpeed(Param_Control* pcs, RPSPMC_Control *self){
        if (!rpspmc_hwi->is_scanning()) // only if scanning!
                return;

        // if not already scheduled, schedule delayed scan speed vector update to avoid messageover load via many slider events
        if (!self->delayed_vector_update_timer_id)
                self->delayed_vector_update_timer_id = g_timeout_add (500, (GSourceFunc)RPSPMC_Control::delayed_vector_update_callback, self);

        //if (self->delayed_vector_update_timer_id){
        //        g_source_remove (self->delayed_vector_update_timer_id);
        //        self->delayed_vector_update_timer_id = 0;
        // }
}

void RPSPMC_Control::ChangedNotifyMoveSpeed(Param_Control* pcs, RPSPMC_Control *self){
        // obsolete, always done together with XY Offset adjustments
}


int RPSPMC_Control::check_vp_in_progress (const gchar *extra_info=NULL) {
        double a,b,c;
        rpspmc_hwi->RTQuery ("s", a,b,c);

        int g = (int)c & 1; // GVP
        int p = (int)c & 2; // PAUSE/STOPPED
        int r = (int)c & 4; // RESET
        
        g_message ("RPSPMC_Control::check_vp_in_progres ** GVP status: %02x == %s %s %s", (int)c, g? "GVP":"--", p? "PAUSE":"--", r? "RESET":"--");
        
        return (g && !p && !r) ? true : false;
        //return rpspmc_hwi->probe_fifo_thread_active>0 ? true:false;
} // GVP active?



/* **************************************** END SPM Control GUI **************************************** */

void RPSPMC_Control::read_spm_vector_program (){
	if (!rpspmc_hwi) return; 
}


// END
