/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * additional features: Andreas Klust <klust@users.sf.net>
 * WWW Home: http://gxsm.sf.net
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

/* ignore this module for docuscan
% PlugInModuleIgnore
*/



#include <locale.h>
#include <libintl.h>

#include <time.h>

#include "glbvars.h"
#include "modules/dsp.h"
#include <fcntl.h>
#include <sys/ioctl.h>

#include "gxsm_app.h"
#include "gxsm_window.h"
#include "surface.h"

#include "action_id.h"
#include "../common/pyremote.h"

#include "sranger_mk2_hwi_control.h"
#include "sranger_mk23common_hwi.h"
#include "modules/sranger_mk23_ioctl.h"
#include "MK3-A810_spmcontrol/dsp_signals.h"  
#include "xsmcmd.h"

// show OUT7 (M) Motor control options
#define MOTOR_CONTROL


#define UTF8_DEGREE    "\302\260"
#define UTF8_MU        "\302\265"
#define UTF8_ANGSTROEM "\303\205"

#define X_SOURCE_MSK 0x10000000 // select for X-mapping
#define P_SOURCE_MSK 0x20000000 // select for plotting
#define A_SOURCE_MSK 0x40000000 // select for Avg plotting
#define S_SOURCE_MSK 0x80000000 // select for Sec plotting

#define GVP_SHIFT_UP  1
#define GVP_SHIFT_DN -1

#define REMOTE_PREFIX "dsp-"


extern "C++" {
        extern DSPControlContainer *DSPControlContainerClass;
        extern DSPControl *DSPControlClass;
        extern DSPMoverControl *DSPMoverClass;
        extern DSPPACControl *DSPPACClass;
        extern DSPControlUserTabs *DSPControlUserTabsClass;
        extern GxsmPlugin sranger_mk2_hwi_pi;
        extern sranger_common_hwi_dev *sranger_common_hwi; // instance of the HwI derived XSM_Hardware class
}


class DSP_GUI_Builder : public BuildParam{
	friend class sranger_mk2_hwi_dev;

public:
        DSP_GUI_Builder (GtkWidget *build_grid=NULL, GSList *ec_list_start=NULL, GSList *ec_remote_list=NULL) :
                BuildParam (build_grid, ec_list_start, ec_remote_list) {
                scrolled_contents = NULL;
                wid = NULL;
                config_checkbutton_list = NULL;
                scan_freeze_widget_list = NULL;
        };

        void start_notebook_tab (const gchar *name, const gchar *settings_name, Notebook_Tab_Name id,
                                 GSettings *settings) {
                
                scrolled_contents = gtk_scrolled_window_new ();
                gtk_widget_set_hexpand (scrolled_contents, TRUE);
                gtk_widget_set_vexpand (scrolled_contents, TRUE);
                g_object_set_data (G_OBJECT (scrolled_contents), "tab-label-widget", gtk_label_new (name));
                DSPControlContainerClass->add_tab (scrolled_contents, id);

                gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled_contents), grid);
                
                grid_add_check_button("Configuration: Enable This");
                new_line ();
                g_object_set_data (G_OBJECT (button), "TabGrid", scrolled_contents);

                config_checkbutton_list = g_slist_append (config_checkbutton_list, button);
                configure_list = g_slist_prepend (configure_list, button);

                g_settings_bind (settings, settings_name,
                                 G_OBJECT (button), "active",
                                 G_SETTINGS_BIND_DEFAULT);
        };

        void notebook_tab_show_all () {
                gtk_widget_show (scrolled_contents);
        };
        
        GtkWidget* grid_add_button_vpc (const gchar *icon_id, const gchar *tooltip, int gx, int gy, int wx, int wy, int k){
                button = gtk_button_new_from_icon_name (icon_id);
		gtk_widget_set_tooltip_text (button, tooltip);
		gtk_grid_attach (GTK_GRID (grid), button, gx, gy, wx, wy);
		g_object_set_data (G_OBJECT(button), "VPC", GINT_TO_POINTER (k));
#if 0
                // to be done...
		g_object_set_data (G_OBJECT(button), "ARROW", GINT_TO_POINTER (ARROW)); // ??? ARROW -> N????
		if (CB_FUNC)
			g_signal_connect(arrowbutton, "clicked",G_CALLBACK(CB_FUNC), CB_DATA);
#endif
                if (configure_list)
                        configure_list = g_slist_prepend (configure_list, button);

                return button;
	};

        GtkWidget* grid_add_mixer_input_options (gint channel, gint preset, gpointer ref){ // preset = mix_fbsource[CHANNEL]);
		GtkWidget *cbtxt = gtk_combo_box_text_new ();
		g_object_set_data(G_OBJECT (cbtxt), "mix_channel_fbsource", GINT_TO_POINTER (channel));
                
		for (int jj=0;  jj<NUM_SIGNALS_UNIVERSAL && sranger_common_hwi->lookup_dsp_signal_managed (jj)->p; ++jj){
			//g_print ("MIX%d S#%d : %s", channel, jj, sranger_common_hwi->lookup_dsp_signal_managed (jj)->module);
			if ( !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Analog_IN")
			     || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Control")
			     || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Counter")
			     || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "LockIn")
			     || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "PAC")
			     || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "MCBSP_LINK")
                             ){
			        if (channel == 0 && !strcmp(sranger_common_hwi->lookup_dsp_signal_managed (jj)->label, "In 0"))
                                        { gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "In0: I-Tunnel"); g_free (id); }
				else
                                        { gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, sranger_common_hwi->lookup_dsp_signal_managed (jj)->label);
                                                //g_print (" FOUND ==> %s L>%s< P#%d", id, sranger_common_hwi->lookup_dsp_signal_managed (jj)->label, preset);
                                                g_free (id); }
                        }
                        //g_print("\n");
		}
                gchar *preset_id = g_strdup_printf ("%d", preset); 
		gtk_combo_box_set_active_id (GTK_COMBO_BOX (cbtxt), preset_id);
                g_free (preset_id);
		g_signal_connect (G_OBJECT (cbtxt), "changed",
				  G_CALLBACK (DSPControl::choice_mixsource_callback),
				  ref);
                grid_add_widget (cbtxt);
                wid=cbtxt;
                return cbtxt;
        };

        GtkWidget* grid_add_mixer_options (gint channel, gint preset, gpointer ref){ // preset = mix_transform_mode[CHANNEL]
                gchar *id;
                GtkWidget *cbtxt = gtk_combo_box_text_new ();
                                                       
                g_object_set_data (G_OBJECT (cbtxt), "mix_channel", GINT_TO_POINTER (channel)); 
                                                                        
                id = g_strdup_printf ("%d", MM_OFF);         gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "OFF"); g_free (id); 
		id = g_strdup_printf ("%d", MM_ON);          gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "LIN"); g_free (id); 
		id = g_strdup_printf ("%d", MM_NEG|MM_ON);   gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "NEG-LIN"); g_free (id); 
		id = g_strdup_printf ("%d", MM_LOG|MM_ON);   gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "LOG"); g_free (id); 
									
		id = g_strdup_printf ("%d", MM_CZ_FUZZY|MM_ON); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "CZ-FUZZY-LIN"); g_free (id); 
		id = g_strdup_printf ("%d", MM_NEG|MM_CZ_FUZZY|MM_ON); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "NEG-CZ-FUZZY-LIN"); g_free (id); 
		id = g_strdup_printf ("%d", MM_CZ_FUZZY|MM_LOG|MM_ON); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "CZ-FUZZY-LOG"); g_free (id); 
		id = g_strdup_printf ("%d", MM_NEG|MM_CZ_FUZZY|MM_LOG|MM_ON); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "NEG-CZ-FUZZY-LOG"); g_free (id); 
		
		id = g_strdup_printf ("%d", MM_LV_FUZZY|MM_ON); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "LV-FUZZY-LIN"); g_free (id); 
		id = g_strdup_printf ("%d", MM_NEG|MM_LV_FUZZY|MM_ON); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "NEG-LV-FUZZY-LIN"); g_free (id); 
		//id = g_strdup_printf ("%d", MM_LV_FUZZY|MM_LOG|MM_ON); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "LV-FUZZY-LOG"); g_free (id); 
		//id = g_strdup_printf ("%d", MM_NEG|MM_LV_FUZZY|MM_LOG|MM_ON); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "NEG-LV-FUZZY-LOG"); g_free (id); 

                gchar *preset_id = g_strdup_printf ("%d", preset); 
		gtk_combo_box_set_active_id (GTK_COMBO_BOX (cbtxt), preset_id);
                //g_print ("SetActive MIX%d p=%d <%s>\n", channel, preset, preset_id);
                g_free (preset_id);
                /*
		if (preset == MM_OFF) 
			gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 0); 
		else if (preset == MM_ON)	
			gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 1); 
		else if (preset == (MM_NEG|MM_ON))	
			gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 2); 
		else if (preset == (MM_LOG|MM_ON))	
			gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 3); 
		else if (preset == (MM_CZ_FUZZY|MM_ON)) 
			gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 4); 
		else if (preset == (MM_NEG|MM_CZ_FUZZY|MM_ON)) 
			gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 5); 
		else if (preset == (MM_CZ_FUZZY|MM_LOG|MM_ON)) 
			gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 6); 
		else if (preset == (MM_NEG|MM_CZ_FUZZY|MM_LOG|MM_ON)) 
			gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 7); 

		else if (preset == (MM_LV_FUZZY|MM_ON)) 
			gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 4); 
		else if (preset == (MM_NEG|MM_LV_FUZZY|MM_ON)) 
			gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 5); 
                //		else if (preset == (MM_LV_FUZZY|MM_LOG|MM_ON)) 
                //			gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 6); 
                //		else if (preset == (MM_NEG|MM_LV_FUZZY|MM_LOG|MM_ON)) 
                //			gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 7); 
                */
                
		g_signal_connect (G_OBJECT (cbtxt),"changed",	
				  G_CALLBACK (DSPControl::choice_mixmode_callback), 
				  ref);				
                
                grid_add_widget (cbtxt);
                return cbtxt;
        };

        GtkWidget *grid_add_scan_input_signal_options (gint channel, gint preset, gpointer ref){ // preset=scan_source[CHANNEL]
		GtkWidget *cbtxt = gtk_combo_box_text_new (); 
		g_object_set_data(G_OBJECT (cbtxt), "scan_channel_source", GINT_TO_POINTER (channel)); 
		
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
		gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), preset); 
		g_signal_connect (G_OBJECT (cbtxt), "changed",	
				  G_CALLBACK (DSPControl::choice_scansource_callback), 
				  ref);
                grid_add_widget (cbtxt);
                return cbtxt;
	};

        GtkWidget *grid_add_probe_source_signal_options (gint channel, gint preset, gpointer ref){ // preset=probe_source[CHANNEL])
                GtkWidget *cbtxt = gtk_combo_box_text_new (); 
		gtk_widget_set_size_request (cbtxt, 50, -1); 
		g_object_set_data(G_OBJECT (cbtxt), "prb_channel_source", GINT_TO_POINTER (channel)); 

#if 0
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
                }
#endif
                
		for (int jj=0;  jj<NUM_SIGNALS_UNIVERSAL && sranger_common_hwi->lookup_dsp_signal_managed (jj)->p; ++jj){ 
                        gchar *id = g_strdup_printf ("%d", jj);
                        const gchar *label = sranger_common_hwi->lookup_dsp_signal_managed (jj)->label;
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, label?label:"???");
                        g_free (id);
		} 
                if (preset > 0)
                        gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), preset); 
                else
                        gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 68); // NULL SIGNAL [TESTING FALLBACK for -1/error]
                
		g_signal_connect (G_OBJECT (cbtxt), "changed",	
				  G_CALLBACK (DSPControl::choice_prbsource_callback), 
				  ref);				
                grid_add_widget (cbtxt);
                return cbtxt;
        };

        GtkWidget *grid_add_lockin_input_signal_options (gint channel, gint preset, gpointer ref){ // preset=lockin_input[CHANNEL]
		GtkWidget *cbtxt = gtk_combo_box_text_new (); 
		g_object_set_data(G_OBJECT (cbtxt), "lck_channel_source", GINT_TO_POINTER (channel)); 
#if 0
                if ( !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Analog_IN") 
                     || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Control") 
                     || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Counter") 
                     || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "Mixer") 
                     || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "LockIn") 
                     || !strcmp (sranger_common_hwi->lookup_dsp_signal_managed (jj)->module, "PAC")){
                }
#endif
		for (int jj=0;  jj<NUM_SIGNALS_UNIVERSAL && sranger_common_hwi->lookup_dsp_signal_managed (jj)->p; ++jj){ 
                        gchar *id = g_strdup_printf ("%d", jj);
                        const gchar *label = sranger_common_hwi->lookup_dsp_signal_managed (jj)->label;
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, label?label:"???");
                        g_free (id);
                } 
		gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), preset); 
		g_signal_connect (G_OBJECT (cbtxt), "changed",	
				  G_CALLBACK (DSPControl::choice_lcksource_callback), 
				  ref);				
                grid_add_widget (cbtxt);
                return cbtxt;
	};

        GtkWidget* grid_add_probe_status (const gchar *status_label) {
                GtkWidget *inp = grid_add_input (status_label);
                gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(inp))), " --- ", -1);
                gtk_editable_set_editable (GTK_EDITABLE (inp), FALSE);
                return inp;
	};

        void grid_add_probe_controls (gboolean have_dual,
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
                                 G_CALLBACK (DSPControl::Probing_save_callback), cb_data);         

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

        // tmp use
        void add_to_remote_list (Gtk_EntryControl *ecx, const gchar *rid) {
                remote_list_ec = ecx->AddEntry2RemoteList(rid, remote_list_ec);
        };

        void add_to_scan_freeze_widget_list (GtkWidget *w){
                scan_freeze_widget_list = g_slist_prepend (scan_freeze_widget_list, w);
        };
        static void update_widget (GtkWidget *w, gpointer data){
                gtk_widget_set_sensitive (w, GPOINTER_TO_INT(data));
        };
        void scan_start_gui_actions (){
                g_message ("DSP GUI UDPATE SCAN START");
                g_slist_foreach (scan_freeze_widget_list, (GFunc) DSP_GUI_Builder::update_widget, GINT_TO_POINTER(FALSE));
        };
        void scan_end_gui_actions (){
                g_message ("DSP GUI UDPATE SCAN END");
                g_slist_foreach (scan_freeze_widget_list, (GFunc) DSP_GUI_Builder::update_widget, GINT_TO_POINTER(TRUE));
        };
        
        GtkWidget *scrolled_contents;
        GtkWidget *wid;
        GSList *config_checkbutton_list;

	GSList *scan_freeze_widget_list;

};

#define OUT_OF_RANGE N_("Value out of range!")

DSPControlUserTabs::DSPControlUserTabs (Gxsm4app *app):AppBase(app)
{
        GtkWidget *notebook;
        GtkWidget *grid_base;

	gchar *tmp = g_strdup_printf ("SR DSP Control %s %s [%s]",
                                      (DSPPACClass)? "MK3-PLL/A810":"MK2/A810",
                                      N_("User Tabs"), xsmres.DSPDev);

        PI_DEBUG (DBG_L5, "DSPControlUserTabs::DSPControlUserTabs");

	AppWindowInit (tmp);

        notebook = gtk_notebook_new ();
	DSPControlContainerClass->add_notebook (notebook, NOTEBOOK_SR_CRTL_USER);
	gtk_grid_attach (GTK_GRID (v_grid), notebook, 1,1, 1,1);
	gtk_widget_show (notebook);
        gtk_widget_set_size_request  (notebook, 400, 300);

	set_window_geometry ("dsp-control-1");
}

DSPControlUserTabs::~DSPControlUserTabs ()
{
        PI_DEBUG (DBG_L5, "DSPControlUserTabs::~DSPControlUserTabs");
}


// add notebook to list and configure defaults
void DSPControlContainer::add_notebook (GtkWidget *notebook, Notebook_Window_Name id) {
        PI_DEBUG (DBG_L5, "DSPControlContainer::add_notebook"); 
	if (id < NOTEBOOK_WINDOW_NUMBER) {
		DSP_notebook [id] = notebook;
                gtk_notebook_set_group_name (GTK_NOTEBOOK (notebook),  "DSP_CONTROL_NOTEBOOK_DND_NAME"); // allow user customization

		g_signal_connect (G_OBJECT (notebook), "page-reordered", G_CALLBACK (DSPControlContainer::callback_tab_reorder), this);
		g_signal_connect (G_OBJECT (notebook), "page-added", G_CALLBACK (DSPControlContainer::callback_tab_added), this);
		g_signal_connect (G_OBJECT (notebook), "page-removed", G_CALLBACK (DSPControlContainer::callback_tab_removed), this);
		g_object_set_data(G_OBJECT (notebook), "notebook-window", GINT_TO_POINTER (id));
		gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
	}
}

// add tab to list and configure defaults
void  DSPControlContainer::add_tab (GtkWidget *tab, Notebook_Tab_Name id) {
        PI_DEBUG (DBG_L5, "DSPControlContainer::add_tab " << (gint)id); 
	if (id < NOTEBOOK_TAB_NUMBER){
		DSP_notebook_tab [id] = tab;
		g_object_set_data(G_OBJECT (tab), "notebook-tab-contents", GINT_TO_POINTER (id));
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tab), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		PI_DEBUG (DBG_L5, "add-tab Notebook id=" << id << " : " << gtk_label_get_text (GTK_LABEL (g_object_get_data (G_OBJECT (tab), "tab-label-widget"))));
	}
}

void  DSPControlContainer::populate_tabs () {
	static int i=0;
        PI_DEBUG (DBG_L5, "DSPControlContainer::populate_tabs");
	freeze_config = 1;
	dbg=10;
	i=0;
        PI_DEBUG (DBG_L5, "sranger_mk2_hwi_control::polulate tab: NOTEBOOK_WINDOW_NUMBER=" << NOTEBOOK_WINDOW_NUMBER);
	for (GtkWidget *notebook = DSP_notebook[i]; i<NOTEBOOK_WINDOW_NUMBER;){
		int j=0;
		gchar *winid = g_strdup_printf ("window-%02d-tabs", i);
                gchar *x = g_settings_get_string (hwi_settings, winid);
                g_free (winid);

                gchar key; // key = 'a'+(gchar)n_t;
		for (gchar *keyptr = x; *keyptr && *keyptr!='-'; ++keyptr){
			int j = *keyptr - 'a';
			if (j < NOTEBOOK_TAB_NUMBER && j >= 0){
				GtkWidget *tab = DSP_notebook_tab[j];
				if (!notebook) continue;
				if (!tab) continue;
				GtkWidget *label = GTK_WIDGET (g_object_get_data (G_OBJECT (tab), "tab-label-widget"));
				if (!label)
					label =  gtk_label_new ("-?-");
				PI_DEBUG (DBG_L5, "add-tab to Notebook " << i << " id=" << j << " : " << gtk_label_get_text (GTK_LABEL (label)));
				gtk_notebook_append_page (GTK_NOTEBOOK (notebook), tab, label);
                                gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (notebook),  tab, true);
                                gtk_notebook_set_tab_detachable  (GTK_NOTEBOOK (notebook),  tab, true);
				DSP_notebook_tab_placed[j] = 1;
			} else {
				PI_DEBUG_ERROR (DBG_L1, "ERROR add-tab to Notebook " << i << " id=" << j << " : invalid key = >" << *keyptr << "<");
			}
		}
		g_free (x);
		i++;
		notebook = DSP_notebook[i];
	}
	freeze_config = 0;
	populate_tabs_missing ();
}
	
void  DSPControlContainer::populate_tabs_missing () {
        //	double xxxx[10];
	static int i=0;
	static int j=0;
        //	double xxx[10];
        PI_DEBUG (DBG_L5, "DSPControlContainer::populate_tabs_missing");
        //	xxxx[0]=xxx[4]=0.; // NOTE: gcc (Ubuntu 4.8.2-19ubuntu1) 4.8.2 -- optimzing bug workaround
	i=j=0;
	GtkWidget *notebook = DSP_notebook[i];

	for (GtkWidget *tab = DSP_notebook_tab[j]; j<NOTEBOOK_TAB_NUMBER; tab = DSP_notebook_tab[++j]){
		if (DSP_notebook_tab_placed[j]) continue; // OK, placed -- else missing, append!
		if (!notebook) continue;
		if (!tab) continue;
		GtkWidget *label = GTK_WIDGET (g_object_get_data (G_OBJECT (tab), "tab-label-widget"));
		if (!label)
			label =  gtk_label_new ("-?-");

		PI_DEBUG (DBG_L5, "add-tab to Notebook (auto append as missing in config) " << i << " id=" << j << " : " << gtk_label_get_text (GTK_LABEL (label)));

		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), tab, label);
		gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (notebook),  tab, true);
		gtk_notebook_set_tab_detachable  (GTK_NOTEBOOK (notebook),  tab, true);
	}
}

// ============================================================
// Popup Menu and Object Action Map
// ============================================================

// DSP-Control Window: Configure Mode Default and binding:
static GActionEntry win_DSPControl_popup_entries[] = {
        { "dsp-control-configure", DSPControl::configure_callback, NULL, "true", NULL },
};

void DSPControl::configure_callback (GSimpleAction *action, GVariant *parameter, 
                                          gpointer user_data){
        DSPControl *dspc = (DSPControl *) user_data;
        GVariant *old_state, *new_state;

       
        if (action){
                old_state = g_action_get_state (G_ACTION (action));
                new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
                
                g_simple_action_set_state (action, new_state);
                g_variant_unref (old_state);

                g_settings_set_boolean (dspc->hwi_settings, "configure-mode", g_variant_get_boolean (new_state));
        } else {
                // new_state = g_variant_new_boolean (true);
                new_state = g_variant_new_boolean (g_settings_get_boolean (dspc->hwi_settings, "configure-mode"));
        }
	if (g_variant_get_boolean (new_state)){
                g_slist_foreach
                        ( dspc->dsp_bp->get_configure_list_head (),
                          (GFunc) App::show_w, NULL
                          );
                g_slist_foreach
                        ( dspc->dsp_bp->get_configure_hide_list_head (),
                          (GFunc) App::hide_w, NULL
                          );
                g_slist_foreach
                        (dspc->dsp_bp->config_checkbutton_list,
                          (GFunc) DSPControl::show_tab_to_configure, NULL
                          );
        } else  {
                g_slist_foreach
                        ( dspc->dsp_bp->get_configure_list_head (),
                          (GFunc) App::hide_w, NULL
                          );
                g_slist_foreach
                        ( dspc->dsp_bp->get_configure_hide_list_head (),
                          (GFunc) App::show_w, NULL
                          );
                g_slist_foreach
                        ( dspc->dsp_bp->config_checkbutton_list,
                          (GFunc) DSPControl::show_tab_as_configured, NULL
                          );
        }
        if (!action){
                g_variant_unref (new_state);
        }
}

void DSPControl::AppWindowInit(const gchar *title, const gchar *sub_title){
        if (title) { // stage 1
                PI_DEBUG (DBG_L2, "DSPControl::AppWindowInit -- header bar");

                app_window = gxsm4_app_window_new (GXSM4_APP (main_get_gapp()->get_application ()));
                window = GTK_WINDOW (app_window);

                header_bar = gtk_header_bar_new ();
                gtk_widget_show (header_bar);
                // hide close, min, max window decorations
                //gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header_bar), false);

                // == moved to stage two ==
                
                PI_DEBUG (DBG_L2,  "VC::VC setup titlbar" );

                gtk_window_set_title (GTK_WINDOW (window), title);
                gtk_window_set_titlebar (GTK_WINDOW (window), header_bar);
        
                v_grid = gtk_grid_new ();
                gtk_window_set_child (GTK_WINDOW (window), v_grid);
                g_object_set_data (G_OBJECT (window), "v_grid", v_grid);

                gtk_widget_show (GTK_WIDGET (window));

        } else {
                
                PI_DEBUG (DBG_L2, "DSPControl::AppWindowInit -- header bar -- stage two, hook configure menu");

                win_DSPControl_popup_entries[0].state = g_settings_get_boolean (hwi_settings, "configure-mode") ? "true":"false"; // get last state
                g_action_map_add_action_entries (G_ACTION_MAP (app_window),
                                                 win_DSPControl_popup_entries, G_N_ELEMENTS (win_DSPControl_popup_entries),
                                                 this);

                // create window PopUp menu  ---------------------------------------------------------------------
                dspc_popup_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (main_get_gapp()->get_hwi_control_popup_menu ()));

                // attach popup menu configuration to tool button --------------------------------
                GtkWidget *header_menu_button = gtk_menu_button_new ();
                gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (header_menu_button), "applications-utilities-symbolic");
                gtk_menu_button_set_popover (GTK_MENU_BUTTON (header_menu_button), dspc_popup_menu);
                gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), header_menu_button);
                gtk_widget_show (header_menu_button);
                g_object_set_data (G_OBJECT (window), "configure_menu", header_menu_button);
        }
}


//        PI_DEBUG (DBG_L1, "Looking for key '" << tab_key << "' in probe-tab-options.");

void DSPControl::get_tab_settings (const gchar *tab_key, guint64 &option_flags, guint64 &auto_flags, guint64 glock_data[6]) {
        PI_DEBUG_GP (DBG_L4, "DSPControl::get_tab_settings for tab '%s'\n", tab_key);

        gint dbg_level = 0;

#define PI_DEBUG_GPX(X, ARGS...) g_print (ARGS);
        
        GVariant *v = g_settings_get_value (hwi_settings, "probe-tab-options");

        // PI_DEBUG_GPX (dbg_level, "probe-tab-options type is '%s'\n", g_variant_get_type (v));
        //        PI_DEBUG_GPX (dbg_level, "probe-tab-options=%s\n", g_variant_print (v,true));
        
        GVariantDict *dict = g_variant_dict_new (v);

        if (!dict){
                PI_DEBUG_GPX (dbg_level, "ERROR: DSPControl::get_tab_settings -- can't read dictionary 'probe-tab-options' a{sv}.\n");
                return;
        }
#if 0
        if (!g_variant_dict_contains (dict, tab_key)){
                PI_DEBUG_GPX (dbg_level, "ERROR: DSPControl::get_tab_settings -- can't find key '%s' in dict 'probe-tab-options'.\n", tab_key);
        }
        PI_DEBUG_GPX (dbg_level, "DSPControl::get_tab_settings -- key '%s' found in dict 'probe-tab-options' :)\n", tab_key);
#endif
        
        GVariant *value = g_variant_dict_lookup_value (dict, tab_key, ((const GVariantType *) "at")); // array uint64
        if (!value){
                g_warning ("WARNING (Normal at first start) -- Note: only at FIRST START: Building Settings. DSPControl::get_tab_settings:\n --> can't get find array 'at' data for key '%s' in 'probe-tab-options' dictionary.\n Storing default now.", tab_key);

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
                                                            "'LM': <@at [0,0,0,0,0,0,0,0,0]>, "
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
                PI_DEBUG_GPX (dbg_level, "DSPControl::get_tab_settings -- reading '%s' tab options [", tab_key);
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
                PI_DEBUG_GPX (dbg_level, "ERROR: DSPControl::get_tab_settings -- cant get array 'at' from key '%s' in 'probe-tab-options' dictionary. And default rewrite failed also.\n", tab_key);
        }

        GVariant *vto = g_variant_dict_end (dict);

        // testing storing
#if 0
        if (vto){
                PI_DEBUG_GPX (dbg_level, "DSPControl::get_tab_settings -- stored tab-setings dict to variant OK.\n");
        } else {
                PI_DEBUG_GPX (dbg_level, "ERROR DSPControl::get_tab_settings -- store tab-setings dict to variant failed.\n");
        }
        if (g_settings_is_writable (hwi_settings, "probe-tab-options")){
                PI_DEBUG_GPX (dbg_level, "DSPControl::get_tab_settings -- is writable\n");
        } else {
                PI_DEBUG_GPX (dbg_level, "WARNING/ERROR DSPControl::get_tab_settings -- is NOT writable\n");
        }
#endif
        if (!g_settings_set_value (hwi_settings, "probe-tab-options", vto)){
                PI_DEBUG_GPX (dbg_level, "ERROR DSPControl::get_tab_settings -- update probe-tab-options dict failed.\n");
        }
        g_variant_dict_unref (dict);
        //        g_variant_unref (vto);
        // PI_DEBUG_GPX (dbg_level, "DSPControl::get_tab_settings -- done.\n");
}

//                PI_DEBUG (DBG_L1, "key '" << tab_key << "' not found in probe-tab-options.");


void DSPControl::set_tab_settings (const gchar *tab_key, guint64 option_flags, guint64 auto_flags, guint64 glock_data[6]) {
        GVariant *v = g_settings_get_value (hwi_settings, "probe-tab-options");

        gint dbg_level = 0;

#define PI_DEBUG_GPX(X, ARGS...) g_print (ARGS);

        // PI_DEBUG_GPX (dbg_level, "probe-tab-options type is '%s'\n", g_variant_get_type (v));
        //        PI_DEBUG_GPX (dbg_level, "probe-tab-options old=%s\n", g_variant_print (v,true));

        GVariantDict *dict = g_variant_dict_new (v);

        if (!dict){
                PI_DEBUG_GPX (dbg_level, "ERROR: DSPControl::set_tab_settings -- can't read dictionary 'probe-tab-options' a{sai}.\n");
                return;
        }
#if 0
        if (!g_variant_dict_contains (dict, tab_key)){
                PI_DEBUG_GPX (dbg_level, "ERROR: DSPControl::set_tab_settings -- can't find key '%s' in dict 'probe-tab-options'.\n", tab_key);
                return;
        }
        PI_DEBUG_GPX (dbg_level, "DSPControl::set_tab_settings -- key '%s' found in dict 'probe-tab-options' :)\n", tab_key);
#endif

        GVariant *value = g_variant_dict_lookup_value (dict, tab_key, ((const GVariantType *) "at"));
        if (!value){
                PI_DEBUG_GPX (dbg_level, "ERROR: DSPControl::set_tab_settings -- can't find array 'at' data from key '%s' in 'probe-tab-options' dictionary for update.\n", tab_key);
                g_variant_dict_unref (dict);
                return;
        }
        if (value){
                gsize    ni = 9;
                guint64 *ai = g_new (guint64, ni);

                //                PI_DEBUG_GPX (dbg_level, "DSPControl::set_tab_settings ** key '%s' old v=%s\n", tab_key, g_variant_print (value, true));
#if 0
                // ---- debug read
                guint64  *aix = NULL;
                gsize     nix;
                
                aix = (guint64*) g_variant_get_fixed_array (value, &nix, sizeof (guint64));
                g_assert_cmpint (nix, ==, 9);

                PI_DEBUG_GPX (dbg_level, "DSPControl::set_tab_settings ** key '%s' old x=[", tab_key);
                for (int i=0; i<nix; ++i)
                        PI_DEBUG_GPX (DBG_L4, "0x%08llx, ", aix[i]);
                PI_DEBUG_GPX (dbg_level, "] hex\n");
                PI_DEBUG_GPX (dbg_level, "DSPControl::set_tab_settings ** key '%s' old x=[", tab_key);
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
                PI_DEBUG_GPX (dbg_level, "DSPControl::set_tab_settings ** key '%s' new x=[", tab_key);
                for (int i=0; i<ni; ++i)
                        PI_DEBUG_GPX (DBG_L4, "0x%08llx, ", ai[i]);
                PI_DEBUG_GPX (dbg_level, "] hex\n");
#endif
                
                g_variant_unref (value); // free old variant
                value = g_variant_new_fixed_array (g_variant_type_new ("t"), ai, ni, sizeof (guint64));

                // PI_DEBUG_GPX (dbg_level, "DSPControl::set_tab_settings ** key '%s' new v=%s\n", tab_key, g_variant_print (value, true));

                // remove old
                if (!g_variant_dict_remove (dict, tab_key)){
                        PI_DEBUG_GPX (dbg_level, "ERROR: DSPControl::set_tab_settings -- cant find and remove key '%s' from 'probe-tab-options' dictionary.\n", tab_key);
                }
                // insert new
                // PI_DEBUG_GPX (dbg_level, "DSPControl::set_tab_settings -- inserting new updated key '%s' into 'probe-tab-options' dictionary.\n", tab_key);
                g_variant_dict_insert_value (dict, tab_key, value);

                g_free (ai); // free array
                // must free data for ourselves
        } else {
                PI_DEBUG_GPX (dbg_level, "ERROR: DSPControl::set_tab_settings -- cant get array 'au' from key '%s' in 'probe-tab-options' dictionary.\n", tab_key);
        }

        GVariant *vto = g_variant_dict_end (dict);

        // PI_DEBUG_GPX (dbg_level, "probe-tab-options new=%s\n", g_variant_print (vto, true));

        // testing storing
        if (!vto){
                PI_DEBUG_GPX (dbg_level, "ERROR DSPControl::set_tab_settings -- store tab-setings dict to variant failed.\n");
        }
        if (!g_settings_set_value (hwi_settings, "probe-tab-options", vto)){
                PI_DEBUG_GPX (dbg_level, "ERROR DSPControl::set_tab_settings -- update probe-tab-options dict failed.\n");
        }
        g_variant_dict_unref (dict);
        //        g_variant_unref (vto);

        //        PI_DEBUG_GPX (dbg_level, "DSPControl::set_tab_settings -- done.\n");
        return;
}

// Achtung: Remote is not released :=(
// DSP-Param sollten lokal werden...
DSPControl::DSPControl (Gxsm4app *app):AppBase(app) {
        int i,j;
	AmpIndex  AmpI;
	GSList *multi_IVsec_list=NULL;
	GSList *multi_Bias_list=NULL;
	GSList *zpos_control_list=NULL;

        GtkWidget *notebook;

        GtkWidget *grid_base;
        
	GtkWidget* wid, *label, *input;
	GtkWidget* menu;
	GtkWidget* menuitem;

	gchar *input_signal = NULL;

        PI_DEBUG (DBG_L5, "DSPControl::DSPControl ()");
        vpg_window = NULL;
        vpg_app_window = NULL;
        vpg_grid = NULL;

	GUI_ready = FALSE;
        idle_callback_data_fn = NULL;
        idle_id = 0;
        idle_id_update_gui = 0;

        vp_exec_mode_name = NULL;
        
        hwi_settings = g_settings_new (GXSM_RES_BASE_PATH_DOT".hwi.sranger-mk23");
        VPSig_menu = NULL;

	for (i=0; i<10; ++i)
		VPprogram[i] = NULL;

	// init pc matrix to NULL
	for (i=0; i < 2*MAX_NUM_CHANNELS; ++i) 
		for (j=0; j < 2*MAX_NUM_CHANNELS; ++j) 
			probe_pc_matrix[i][j] = NULL;

	IV_status = NULL;
	GVP_status = NULL;
	FZ_status = NULL;

	write_vector_mode=PV_MODE_NONE;

	probedata_list = NULL;
	probehdr_list = NULL;
	num_probe_events = 0;
	last_probe_data_index = 0;
	nun_valid_data_sections = 0;
	pv_lock = FALSE;

	probe_trigger_single_shot = 0;
	current_probe_data_index = 0;

	for (i=0; i<NUM_PROBEDATA_ARRAYS; ++i)
		garray_probedata [i] = NULL;

	for (i=0; i<NUM_PROBEDATA_ARRAYS; ++i)
		garray_probe_hdrlist [i] = NULL;

	DSP_vpdata_ij[0]=2; // DSP level VP data acccess indexing, global
	DSP_vpdata_ij[1]=0;

	XsmRescourceManager xrm("sranger_mk2_hwi_control");
        xrm.Get ("frq_ref", &frq_ref, "75000.0");
	xrm.Get ("bias", &bias, "0.5");
	xrm.Get ("motor", &motor, "0.0");

	xrm.Get ("mix_set_point0", &mix_set_point[0], "0.1");
	xrm.Get ("mix_set_point1", &mix_set_point[1], "0.1");
	xrm.Get ("mix_set_point2", &mix_set_point[2], "0.0");
	xrm.Get ("mix_set_point3", &mix_set_point[3], "0.0");
	xrm.Get ("mix_gain0", &mix_gain[0], "0.5");
	xrm.Get ("mix_gain1", &mix_gain[1], "0.5");
	xrm.Get ("mix_gain2", &mix_gain[2], "0.5");
	xrm.Get ("mix_gain3", &mix_gain[3], "0.5");
	xrm.Get ("mix_level0", &mix_level[0], "0.0");
	xrm.Get ("mix_level1", &mix_level[1], "0.0");
	xrm.Get ("mix_level2", &mix_level[2], "0.0");
	xrm.Get ("mix_level3", &mix_level[3], "0.0");
	xrm.Get ("mix_transform_mode_0", &mix_transform_mode[0], "3");
	xrm.Get ("mix_transform_mode_1", &mix_transform_mode[1], "0");
	xrm.Get ("mix_transform_mode_2", &mix_transform_mode[2], "0");
	xrm.Get ("mix_transform_mode_3", &mix_transform_mode[3], "0");

	// MIX/Probe/LockIn INPUTs -- override with actual DSP settings later!
	xrm.Get ("probe_source_0", &probe_source[0], "18"); // LockIn1stB
	xrm.Get ("probe_source_1", &probe_source[1], "19");
	xrm.Get ("probe_source_2", &probe_source[2], "20");
	xrm.Get ("probe_source_3", &probe_source[3], "21"); // Counter0

	xrm.Get ("probe_output_0", &probe_output[0], "0");
	xrm.Get ("probe_output_1", &probe_output[1], "0");
	xrm.Get ("probe_output_2", &probe_output[2], "0");
	xrm.Get ("probe_output_3", &probe_output[3], "0");

	xrm.Get ("lockin_input_0", &lockin_input[0], "0");
	xrm.Get ("lockin_input_1", &lockin_input[1], "0");
	xrm.Get ("lockin_input_2", &lockin_input[2], "0");
	xrm.Get ("lockin_input_3", &lockin_input[3], "0");

	xrm.Get ("move_speed_x", &move_speed_x, "2000.0");
	xrm.Get ("scan_speed_x", &scan_speed_x_requested, "2000.0");
	scan_speed_x = scan_speed_x_requested;
	xrm.Get ("gain_ratio", &gain_ratio, "1.0");
	xrm.Get ("z_servo_CP", &z_servo[SERVO_CP], (DSPPACClass) ? "25.0":"0.007");
	xrm.Get ("z_servo_CI", &z_servo[SERVO_CI], (DSPPACClass) ? "20.0":"0.004");

	xrm.Get ("dxdt", &dxdt, "0.0");
	xrm.Get ("dydt", &dydt, "0.0");
	xrm.Get ("dzdt", &dzdt, "0.0");

	xrm.Get ("IIR_f0_min", &IIR_f0_min, "200.0");
	xrm.Get ("IIR_f0_max0", &IIR_f0_max[0], "8000.0");
	xrm.Get ("IIR_f0_max1", &IIR_f0_max[1], "18000.0");
	xrm.Get ("IIR_f0_max2", &IIR_f0_max[2], "18000.0");
	xrm.Get ("IIR_f0_max3", &IIR_f0_max[3], "18000.0");
	xrm.Get ("IIR_I_crossover", &IIR_I_crossover, "100.0");
	xrm.Get ("LOG_I_offset", &LOG_I_offset, "10.0");
	xrm.Get ("dynamic_zoom", &dynamic_zoom, "1.0");
	xrm.Get ("fast_return", &fast_return, "1.0");
	xrm.Get ("scan_forward_slow_down", &scan_forward_slow_down, "1.0");
	xrm.Get ("scan_forward_slow_down_2nd", &scan_forward_slow_down_2nd, "1.0");
	xrm.Get ("pre_points", &pre_points, "0");
	xrm.Get ("x2nd_Zoff_XP", &x2nd_Zoff, "0");
	xrm.Get ("center_return_flag", &center_return_flag, "1");
	xrm.Get ("area_slope_compensation_flag", &area_slope_compensation_flag, "0");
	xrm.Get ("area_slope_x", &area_slope_x, "0");
	xrm.Get ("area_slope_y", &area_slope_y, "0");




	ue_bias = bias;
	ue_scan_speed_x = 0.;
	ue_scan_speed_x_r = 0.;
	ue_z_servo[SERVO_CI] = 0.;
	ue_z_servo[SERVO_CP] = 0.;
	ue_set_point[0] = mix_set_point[0];
	ue_set_point[1] = mix_set_point[1];

	// init to "1V/V"
	for (int i=0; i<4; ++i)
		mix_unit2volt_factor[i] = 1.;
	mix_unit2volt_factor[0] = main_get_gapp()->xsm->Inst->nAmpere2V (1.);
    
	// M-Servo Module
	xrm.Get ("m_servo_cp", &m_servo[SERVO_CP], "0.0");
	xrm.Get ("m_servo_ci", &m_servo[SERVO_CI], "0.0");
	xrm.Get ("m_servo_setpoint", &m_servo[SERVO_SETPT], "0.0");

	// LockIn and LockIn phase probe
	xrm.Get ("AC_amp", &AC_amp[0], "0.02");
	xrm.Get ("AC_amp_aux_Z", &AC_amp[1], "0.0");
	xrm.Get ("AC_lockin_shr_corrprod", &AC_amp[2], "14.0");
	xrm.Get ("AC_lockin_shr_corrsum", &AC_amp[3], "0.0");
	xrm.Get ("AC_frq", &AC_frq, "1171.88");
	xrm.Get ("AC_phaseA", &AC_phaseA, "0");
	xrm.Get ("AC_phaseB", &AC_phaseB, "90");
	xrm.Get ("AC_lockin_avg_cycels", &AC_lockin_avg_cycels, "32");
	xrm.Get ("AC_phase_span", &AC_phase_span, "360");
	xrm.Get ("AC_points", &AC_points, "720");
	xrm.Get ("AC_repetitions", &AC_repetitions, "1");
	xrm.Get ("AC_phase_slope", &AC_phase_slope, "12");
	xrm.Get ("AC_final_delay", &AC_final_delay, "0.01");
        
        get_tab_settings ("AC", AC_option_flags, AC_auto_flags, AC_glock_data);

	xrm.Get ("noise_amp", &noise_amp, "0.");

	// Probing-----------------------------
	// Graphs/Sources Tab

        // Source = 0x3030;
        Source  = g_settings_get_int (hwi_settings, "probe-sources");
        XSource = g_settings_get_int (hwi_settings, "probe-sources-x");
        XJoin   = g_settings_get_boolean (hwi_settings, "probe-x-join");
        GrMatWin= g_settings_get_boolean (hwi_settings, "probe-graph-matrix-window");
        PSource = g_settings_get_int (hwi_settings, "probe-p-sources");
        PlotAvg = g_settings_get_int (hwi_settings, "probe-pavg-sources");
        PlotSec = g_settings_get_int (hwi_settings, "probe-psec-sources");

	vis_Source = Source;
	vis_XSource = XSource;
	vis_PSource = PSource;
	vis_XJoin = XJoin;
	vis_PlotAvg = PlotAvg;
	vis_PlotSec = PlotSec;

	xrm.Get ("Probing_probe_trigger_raster_points", &probe_trigger_raster_points_user, "0");

	probe_trigger_raster_points = 0;
	probe_trigger_raster_points_b = 0;

        multiBias_mode=0;
        
	// STS I-V
	xrm.Get ("Probing_IV_sections", &IV_sections, "1");
	xrm.Get ("multiIV_mode", &multiIV_mode, "0");
	i=0;
	xrm.Get ("Probing_IV_Ustart", &IV_start[i], "-1.0");
	xrm.Get ("Probing_IV_Uend", &IV_end[i], "1.0");
	xrm.Get ("Probing_IV_points", &IV_points[i], "100");
	++i;
	xrm.Get ("Probing_IV_Ustart1", &IV_start[i], "-1.0");
	xrm.Get ("Probing_IV_Uend1", &IV_end[i], "1.0");
	xrm.Get ("Probing_IV_points1", &IV_points[i], "10");
	++i;
	xrm.Get ("Probing_IV_Ustart2", &IV_start[i], "-1.0");
	xrm.Get ("Probing_IV_Uend2", &IV_end[i], "1.0");
	xrm.Get ("Probing_IV_points2", &IV_points[i], "10");
	++i;
	xrm.Get ("Probing_IV_Ustart3", &IV_start[i], "-1.0");
	xrm.Get ("Probing_IV_Uend3", &IV_end[i], "1.0");
	xrm.Get ("Probing_IV_points3", &IV_points[i], "10");
	++i;
	xrm.Get ("Probing_IV_Ustart4", &IV_start[i], "-1.0");
	xrm.Get ("Probing_IV_Uend4", &IV_end[i], "1.0");
	xrm.Get ("Probing_IV_points4", &IV_points[i], "10");
	++i;
	xrm.Get ("Probing_IV_Ustart5", &IV_start[i], "-1.0");
	xrm.Get ("Probing_IV_Uend5", &IV_end[i], "1.0");
	xrm.Get ("Probing_IV_points5", &IV_points[i], "10");
	++i;
	xrm.Get ("Probing_IV_Ustart6", &IV_start[i], "-1.0");
	xrm.Get ("Probing_IV_Uend6", &IV_end[i], "1.0");
	xrm.Get ("Probing_IV_points6", &IV_points[i], "10");
	++i;
	xrm.Get ("Probing_IV_Ustart7", &IV_start[i], "-1.0");
	xrm.Get ("Probing_IV_Uend7", &IV_end[i], "1.0");
	xrm.Get ("Probing_IV_points7", &IV_points[i], "10");


	xrm.Get ("Probing_IV_repetitions", &IV_repetitions, "1");
	xrm.Get ("Probing_IV_dz", &IV_dz, "0.0");
	xrm.Get ("Probing_IVdz_repetitions", &IVdz_repetitions, "0");
	xrm.Get ("Probing_IV_slope", &IV_slope, "10");
	xrm.Get ("Probing_IV_slope_ramp", &IV_slope_ramp, "50");
	xrm.Get ("Probing_IV_final_delay", &IV_final_delay, "0.01");
	xrm.Get ("Probing_IV_recover_delay", &IV_recover_delay, "0.3");
	xrm.Get ("Probing_IV_dx", &IV_dx, "50.0");
	xrm.Get ("Probing_IV_dy", &IV_dy, "0.0");
	xrm.Get ("Probing_IV_dM", &IV_dM, "0.0");
	xrm.Get ("Probing_IV_dxy_slope", &IV_dxy_slope, "100.0");
	xrm.Get ("Probing_IV_dxy_delay", &IV_dxy_delay, "1.0");
	xrm.Get ("Probing_IV_dxy_points", &IV_dxy_points, "1");

        get_tab_settings ("IV", IV_option_flags, IV_auto_flags, IV_glock_data);

	// FZ
	xrm.Get ("Probing_FZ_Z_start", &FZ_start,"0.0");
	xrm.Get ("Probing_FZ_Z_end", &FZ_end,"100.0");
	xrm.Get ("Probing_FZ_points", &FZ_points,"100");
	xrm.Get ("Probing_FZ_repetitions", &FZ_repetitions,"1");
	xrm.Get ("Probing_FZ_limiter_ch", &FZ_limiter_ch,"4");
	xrm.Get ("Probing_FZ_limiter_val0", &FZ_limiter_val[0],"1.0");
	xrm.Get ("Probing_FZ_limiter_val1", &FZ_limiter_val[1],"1.0");
	xrm.Get ("Probing_FZ_slope", &FZ_slope,"100");
	xrm.Get ("Probing_FZ_slope_ramp", &FZ_slope_ramp, "100");
	xrm.Get ("Probing_FZ_final_delay", &FZ_final_delay,"0.01");

        get_tab_settings ("FZ", FZ_option_flags, FZ_auto_flags, FZ_glock_data);

	// PL
	xrm.Get ("Probing_PL_remote_set_value", &PL_remote_set_value, "0.1");
	xrm.Get ("Probing_PL_duration", &PL_duration,"10");
	xrm.Get ("Probing_PL_time_resolution", &PL_time_resolution,"0.0"); // 0=auto
	xrm.Get ("Probing_PL_volt", &PL_volt,"2");
	xrm.Get ("Probing_PL_dZ", &PL_dZ, "0.0");
	xrm.Get ("Probing_PL_dZ_ext", &PL_dZ_ext, "0.0");
	xrm.Get ("Probing_PL_slope", &PL_slope,"1e4");
	xrm.Get ("Probing_PL_step", &PL_step, "0.0");
	xrm.Get ("Probing_PL_stepZ", &PL_stepZ, "0.0");
	xrm.Get ("Probing_PL_repetitions", &PL_repetitions,"1");
	xrm.Get ("Probing_PL_initial_delay", &PL_initial_delay,"0.01");
	xrm.Get ("Probing_PL_final_delay", &PL_final_delay,"0.01");

        get_tab_settings ("PL", PL_option_flags, PL_auto_flags, PL_glock_data);

	// LP
	xrm.Get ("Probing_LP_duration", &LP_duration,"10");
	xrm.Get ("Probing_LP_triggertime", &LP_triggertime,"10");
	xrm.Get ("Probing_LP_volt", &LP_volt,"2");
	xrm.Get ("Probing_LP_slope", &LP_slope,"1e4");
	xrm.Get ("Probing_LP_repetitions", &LP_repetitions,"1");
	xrm.Get ("Probing_LP_FZ_end", &LP_FZ_end,"0.0");
	xrm.Get ("Probing_LP_final_delay", &LP_final_delay,"10");

        get_tab_settings ("LP", LP_option_flags, LP_auto_flags, LP_glock_data);

	// SP
	xrm.Get ("Probing_SP_duration", &SP_duration,"10");
	xrm.Get ("Probing_SP_volt", &SP_volt,"2");
	xrm.Get ("Probing_SP_ramptime", &SP_ramptime,"10");
	xrm.Get ("Probing_SP_flag_volt", &SP_flag_volt,"1");
	xrm.Get ("Probing_SP_repetitions", &SP_repetitions,"1");
	xrm.Get ("Probing_SP_final_delay", &SP_final_delay,"0.01");

        get_tab_settings ("SP", SP_option_flags, SP_auto_flags, SP_glock_data);

	// TS
	xrm.Get ("Probing_TS_duration", &TS_duration,"1000");
	xrm.Get ("Probing_TS_points", &TS_points,"2048");
	xrm.Get ("Probing_TS_repetitions", &TS_repetitions,"1");

        get_tab_settings ("TS", TS_option_flags, TS_auto_flags, TS_glock_data);

	// GVP
        GVP_restore_vp ("LM_set_last"); // last in view

	xrm.Get ("Probing_LM_GPIO_lock", &GVP_GPIO_lock, GVP_GPIO_KEYCODE_S);
	xrm.Get ("Probing_LM_final_delay", &GVP_final_delay,"0.01");

        get_tab_settings ("LM", GVP_option_flags, GVP_auto_flags, GVP_glock_data);

	// TK
	xrm.Get ("Probing_TK_r", &TK_r,"2.0");
	xrm.Get ("Probing_TK_r2", &TK_r2,"0.0");
	xrm.Get ("Probing_TK_speed", &TK_speed,"1000.0");
	xrm.Get ("Probing_TK_delay", &TK_delay,"1.0");
	xrm.Get ("Probing_TK_points", &TK_points,"10");
	xrm.Get ("Probing_TK_nodes", &TK_nodes,"12");
	xrm.Get ("Probing_TK_mode", &TK_mode,"-1");
	xrm.Get ("Probing_TK_ref", &TK_ref,"0");
	xrm.Get ("Probing_TK_repetitions", &TK_repetitions,"100");

        get_tab_settings ("TK", TK_option_flags, TK_auto_flags, TK_glock_data);

	// AX
	xrm.Get ("Probing_AX_start", &AX_start,"0.0");
	xrm.Get ("Probing_AX_end", &AX_end,"100.0");
	xrm.Get ("Probing_AX_points", &AX_points,"100");
	xrm.Get ("Probing_AX_repetitions", &AX_repetitions,"1");
	xrm.Get ("Probing_AX_slope", &AX_slope,"100");
	xrm.Get ("Probing_AX_slope_ramp", &AX_slope_ramp, "100");
	xrm.Get ("Probing_AX_final_delay", &AX_final_delay,"0.01");
	xrm.Get ("Probing_AX_gatetime", &AX_gatetime,"0.001");
	xrm.Get ("Probing_AX_gain", &AX_gain,"1");
	xrm.Get ("Probing_AX_resolution", &AX_resolution,"1");

        get_tab_settings ("AX", AX_option_flags, AX_auto_flags, AX_glock_data);

	// ABORT
	xrm.Get ("Probing_ABORT_final_delay", &ABORT_final_delay,"0.01");

        get_tab_settings ("AB", ABORT_option_flags, ABORT_auto_flags, ABORT_glock_data);

	if (DSPPACClass) { // MK3
                sranger_common_hwi->read_dsp_signals (); // read DSP signals or set up empty list depending on actual HW

                // setup scope/recorder defaults
#if 0 // temporary DO NOT TOUCH
                sranger_common_hwi->change_signal_input (sranger_common_hwi->lookup_signal_by_name("MIX IN 0"), DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID);
#endif
                sranger_common_hwi->change_signal_input (sranger_common_hwi->lookup_signal_by_name("Z Servo Neg"), DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID);

                
                // override configuration of MIX0..3 INPUT config by actual configured DSP signal:
                mix_fbsource[0] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_MIXER0_INPUT_ID);
                mix_fbsource[1] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_MIXER1_INPUT_ID);
                mix_fbsource[2] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_MIXER2_INPUT_ID);
                mix_fbsource[3] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_MIXER3_INPUT_ID);

                scan_source[0] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID);
                scan_source[1] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_SCAN_CHANNEL_MAP1_ID);
                scan_source[2] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_SCAN_CHANNEL_MAP2_ID);
                scan_source[3] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_SCAN_CHANNEL_MAP3_ID);

                probe_source[0] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE0_INPUT_ID);
                probe_source[1] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE1_INPUT_ID);
                probe_source[2] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE2_INPUT_ID);
                probe_source[3] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE3_INPUT_ID);
                probe_source[4] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE_TRIGGER_INPUT_ID);
                probe_source[5] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID);
                probe_source[6] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE_TRACKER_INPUT_ID);

                lockin_input[0] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_LOCKIN_A_INPUT_ID);
                lockin_input[1] = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_LOCKIN_B_INPUT_ID);

                for (int jj=0; jj<4; ++jj){
                        const gchar *l =  sranger_common_hwi->lookup_dsp_signal_managed (mix_fbsource[jj])->label;
                        int d =  sranger_common_hwi->lookup_dsp_signal_managed (mix_fbsource[jj])->dim;
                        const gchar *u =  sranger_common_hwi->lookup_dsp_signal_managed (mix_fbsource[jj])->unit;
                        //double s =  sranger_common_hwi->lookup_dsp_signal_managed (mix_fbsource[jj])->scale;
                        //double f = s*32768.*256./10.;
                        //gchar *txt = g_strdup_printf ("Mixer Signal Input[%d] = %d -> %s in %s, scale=%g, dim=%d, scale from Volts to unit: %g", jj, mix_fbsource[jj], l ? l : "?E?", u ? u : "?E?", s, d, f);
                        //PI_DEBUG_GP (DBG_L1, "%s\n", txt);
                        //main_get_gapp()->monitorcontrol->LogEvent ("MK3-SIGNAL-CONFIGURATION-INFO", txt);
                        //g_free (txt);
                        
                        // INIT mix_unit2volt_factor[] -- updated in signal change in choice_mixsource_callback()
                        if (jj > 1){
                                //const gchar *u =  sranger_common_hwi->lookup_dsp_signal_managed (mix_fbsource[jj])->unit;
                                double s =  sranger_common_hwi->lookup_dsp_signal_managed (mix_fbsource[jj])->scale;
                                if (!strncmp (sranger_common_hwi->lookup_dsp_signal_managed (mix_fbsource[jj])->label, "In ", 3))
                                        mix_unit2volt_factor[jj] = 256./s;
                                else
                                        mix_unit2volt_factor[jj] = 1./s;
                        }
                }
                        
                for (int jj=0; jj<=6; ++jj){
                        const gchar *l = sranger_common_hwi->lookup_dsp_signal_managed (probe_source[jj])->label;
                        gchar *txt = g_strdup_printf ("Probe Signal Input[%d] = %d -> %s", jj, probe_source[jj], l ? l : "?E?");
                        PI_DEBUG_GP (DBG_L1, "%s\n", txt);
                        main_get_gapp()->monitorcontrol->LogEvent ("MK3-SIGNAL-CONFIGURATION-INFO", txt);
                        g_free (txt);
                }
                for (int jj=0; jj<2; ++jj){
                        const gchar *l = sranger_common_hwi->lookup_dsp_signal_managed (lockin_input[jj])->label;
                        gchar *txt = g_strdup_printf ("LockIn Signal Input[%d] = %d -> %s", jj, lockin_input[jj], l ? : "?E?"); 
                        PI_DEBUG_GP (DBG_L1, "%s\n", txt);
                        main_get_gapp()->monitorcontrol->LogEvent ("MK3-SIGNAL-CONFIGURATION-INFO", txt);
                        g_free (txt);
                }
        }                        

	Unity    = new UnitObj(" "," ");
	//Volt     = new UnitAutoMag("V","V");
	Volt     = new UnitObj("V","V");
	Angstroem= new UnitObj(UTF8_ANGSTROEM,"A");
	Frq      = new UnitObj("Hz","Hz");
	Time     = new UnitObj("s","s");
	TimeUms  = new LinUnit("ms","ms",1e-3);
	msTime   = new UnitObj("ms","ms");
	minTime  = new UnitObj("min","min");
	Deg      = new UnitObj(UTF8_DEGREE,"Deg");
        // Current  = new UnitAutoMag("A","A"); ((UnitAutoMag*) Current)->set_mag_get_base (1e-9, 1e-9); // nA default and internal "base"
        Current  = new UnitObj("nA","nA");
	Current_pA  = new UnitObj("pA","pA");
	Speed    = new UnitObj(UTF8_ANGSTROEM"/s","A/s");
	PhiSpeed = new UnitObj(UTF8_DEGREE"/s","Deg/s");
	Vslope   = new UnitObj("V/s","V/s");
	Hex      = new UnitObj("h","h");
	SetPtUnit = sranger_mk2_hwi_pi.app->xsm->MakeUnit (xsmres.daqZunit[0], xsmres.daqZlabel[0]);

	probe_fname = g_strdup ("probe_test.data");
	probe_findex = 0;

	gchar *tmp = g_strdup_printf ("SR DSP Control %s [%s]", (DSPPACClass)? "MK3-PLL/A810":"MK2/A810", xsmres.DSPDev);

	AppWindowInit (tmp); // call one, setup window

	// update some from DSP -- new!!
	sranger_common_hwi->read_dsp_feedback ();

        // initialize GXSM4 DSP GUI builder
        dsp_bp = new DSP_GUI_Builder ();
        dsp_bp->set_pcs_remote_prefix (REMOTE_PREFIX);

        dsp_bp->set_error_text (OUT_OF_RANGE);
        dsp_bp->set_default_ec_change_notice_fkt (DSPControl::ChangedNotify, this);
        dsp_bp->data = this;

        notebook = gtk_notebook_new ();
	DSPControlContainerClass->add_notebook (notebook, NOTEBOOK_SR_CRTL_MAIN);
	gtk_grid_attach (GTK_GRID (v_grid), notebook, 1,1, 1,1);
	gtk_widget_show (notebook);

        gtk_widget_set_size_request  (notebook, 500, 450);

// ==== Folder: Feedback & Scan ========================================
        dsp_bp->start_notebook_tab ("Feedback & Scan", "tab-feedback-scan", NOTEBOOK_TAB_FBSCAN, hwi_settings);

	dsp_bp->new_grid_with_frame ("Main SPM Controls and 4-Channel Signal-Setpoint Mixer");
        
        // ------- FB Characteristics
        dsp_bp->start (); // start on grid top and use widget grid attach nx=4
        dsp_bp->set_scale_nx (4); // set scale width to 4
        dsp_bp->set_input_width_chars (10);
        GtkWidget *multiBias_checkbutton = dsp_bp->grid_add_check_button ("Multi...  Bias", "enable multi bias section mode", 1,
                                                                          G_CALLBACK(DSPControl::DSP_multiBias_callback), this);
        dsp_bp->grid_add_ec_with_scale (NULL, Volt, &bias, -10., 10., "4g", 0.001, 0.01, "fbs-bias");
        //dsp_bp->grid_add_ec_with_scale ("Bias", Volt, &bias, -10., 10., "4g", 0.001, 0.01, "fbs-bias");
        //        dsp_bp->ec->set_adjustment_mode (PARAM_CONTROL_ADJUSTMENT_LOG | PARAM_CONTROL_ADJUSTMENT_LOG_SYM | PARAM_CONTROL_ADJUSTMENT_DUAL_RANGE | PARAM_CONTROL_ADJUSTMENT_ADD_MARKS );
        dsp_bp->ec->SetScaleWidget (dsp_bp->scale, 0);
        dsp_bp->ec->set_logscale_min (1e-3);
        gtk_scale_set_digits (GTK_SCALE (dsp_bp->scale), 5);
        dsp_bp->new_line ();

        dsp_bp->grid_add_ec_with_scale ("Bias-S1", Volt, &bias_sec[1], -10., 10., "4g", 0.001, 0.01, "fbs-bias1");
        multi_Bias_list = g_slist_prepend (multi_Bias_list, dsp_bp->label);
        multi_Bias_list = g_slist_prepend (multi_Bias_list, dsp_bp->input);
        multi_Bias_list = g_slist_prepend (multi_Bias_list, dsp_bp->scale);
        //        dsp_bp->ec->set_adjustment_mode (PARAM_CONTROL_ADJUSTMENT_LOG | PARAM_CONTROL_ADJUSTMENT_LOG_SYM | PARAM_CONTROL_ADJUSTMENT_DUAL_RANGE | PARAM_CONTROL_ADJUSTMENT_ADD_MARKS );
        dsp_bp->ec->SetScaleWidget (dsp_bp->scale, 0);
        dsp_bp->ec->set_logscale_min (1e-3);
        gtk_scale_set_digits (GTK_SCALE (dsp_bp->scale), 5);
        dsp_bp->new_line ();
        dsp_bp->grid_add_ec_with_scale ("Bias-S2", Volt, &bias_sec[2], -10., 10., "4g", 0.001, 0.01, "fbs-bias2");
        multi_Bias_list = g_slist_prepend (multi_Bias_list, dsp_bp->label);
        multi_Bias_list = g_slist_prepend (multi_Bias_list, dsp_bp->input);
        multi_Bias_list = g_slist_prepend (multi_Bias_list, dsp_bp->scale);
        //        dsp_bp->ec->set_adjustment_mode (PARAM_CONTROL_ADJUSTMENT_LOG | PARAM_CONTROL_ADJUSTMENT_LOG_SYM | PARAM_CONTROL_ADJUSTMENT_DUAL_RANGE | PARAM_CONTROL_ADJUSTMENT_ADD_MARKS );
        dsp_bp->ec->SetScaleWidget (dsp_bp->scale, 0);
        dsp_bp->ec->set_logscale_min (1e-3);
        gtk_scale_set_digits (GTK_SCALE (dsp_bp->scale), 5);
        dsp_bp->new_line ();
        dsp_bp->grid_add_ec_with_scale ("Bias-S3", Volt, &bias_sec[3], -10., 10., "4g", 0.001, 0.01, "fbs-bias3");
        multi_Bias_list = g_slist_prepend (multi_Bias_list, dsp_bp->label);
        multi_Bias_list = g_slist_prepend (multi_Bias_list, dsp_bp->input);
        multi_Bias_list = g_slist_prepend (multi_Bias_list, dsp_bp->scale);
        //        dsp_bp->ec->set_adjustment_mode (PARAM_CONTROL_ADJUSTMENT_LOG | PARAM_CONTROL_ADJUSTMENT_LOG_SYM | PARAM_CONTROL_ADJUSTMENT_DUAL_RANGE | PARAM_CONTROL_ADJUSTMENT_ADD_MARKS );
        dsp_bp->ec->SetScaleWidget (dsp_bp->scale, 0);
        dsp_bp->ec->set_logscale_min (1e-3);
        gtk_scale_set_digits (GTK_SCALE (dsp_bp->scale), 5);
        dsp_bp->new_line ();

        
        dsp_bp->set_configure_list_mode_on ();
        dsp_bp->grid_add_ec_with_scale ("Motor", Volt, &motor, -10., 10., "4g", 0.001, 0.01, "fbs-motor");
        dsp_bp->ec->set_adjustment_mode (PARAM_CONTROL_ADJUSTMENT_LINEAR | PARAM_CONTROL_ADJUSTMENT_DUAL_RANGE | PARAM_CONTROL_ADJUSTMENT_ADD_MARKS );
        dsp_bp->ec->SetScaleWidget (dsp_bp->scale, 0);
        dsp_bp->ec->set_logscale_min (1e-3);
        gtk_scale_set_digits (GTK_SCALE (dsp_bp->scale), 5);
        dsp_bp->new_line ();

        dsp_bp->set_input_width_chars (30);
        dsp_bp->set_input_nx (2);
        dsp_bp->grid_add_ec ("Z-Pos/Setpoint:", Angstroem, &zpos_ref, -100., 100., "6g", 0.01, 0.1, "adv-dsp-zpos-ref");
        ZPos_ec = dsp_bp->ec;
        zpos_control_list = g_slist_prepend (zpos_control_list, dsp_bp->ec);
                 
        dsp_bp->set_input_width_chars (12);
        dsp_bp->set_input_nx ();

	dsp_bp->grid_add_check_button ("Z-Pos Monitor",
                                       "Z Position Monitor. Disable to set Z-Position Setpoint for const height mode. "
                                       "Switch Transfer to CZ-FUZZY-LOG for Z-CONTROL Constant Heigth Mode Operation with current compliance given by Fuzzy-Level. "
                                       "Slow down feedback to minimize instabilities. "
                                       "Set Current setpoint a notch below compliance level.",
                                       1,
                                       G_CALLBACK (DSPControl::zpos_monitor_callback), this,
                                       0, MD_ZPOS_ADJUSTER);
        GtkWidget *zposmon_checkbutton = dsp_bp->button;
        dsp_bp->new_line ();

        dsp_bp->set_configure_list_mode_off ();

        // MIXER headers

	dsp_bp->grid_add_label ("Source");
        dsp_bp->grid_add_label ("Setpoint", NULL, 2);

        dsp_bp->set_configure_list_mode_on ();
        dsp_bp->grid_add_label ("Gain");
        dsp_bp->grid_add_label ("Fuzzy-Level");
        dsp_bp->grid_add_label ("Transfer");
        dsp_bp->set_configure_list_mode_off ();

        dsp_bp->new_line ();

        dsp_bp->set_scale_nx (1);

        // Build MIXER CHANNELs
        UnitObj *mixer_unit[4] = { Current, Frq, Volt, Volt };
        const gchar *mixer_channel_label[4] = { "Mix0 (STM Current)", "Mix1 (AFM/Force)", "Mix2 (aux.)", "Mix3 (aux.)" };
        const gchar *mixer_remote_id_set[4] = { "fbs-mx0-current-set",  "fbs-mx1-freq-set",   "fbs-mx2-set",   "fbs-mx3-set" };
        const gchar *mixer_remote_id_gn[4]  = { "fbs-mx0-current-gain" ,"fbs-mx1-freq-gain",  "fbs-mx2-gain",  "fbs-mx3-gain" };
        const gchar *mixer_remote_id_fl[4]  = { "fbs-mx0-current-level","fbs-mx1-freq-level", "fbs-mx2-level", "fbs-mx3-level" };

        // Note: transform mode is always default [LOG,OFF,OFF,OFF] -- NOT READ BACK FROM DSP -- !!!
        for (gint ch=0; ch<4; ++ch){

                mix_transform_mode[ch] = (int)sranger_common_hwi->read_dsp_feedback ("MT", ch);
                //g_print ("INIT MIX%d MT=%d\n", ch,  mix_transform_mode[ch]);
                
                if (mix_transform_mode[ch] == MM_OFF)
                        dsp_bp->set_configure_list_mode_on (); 
                else
                        dsp_bp->set_configure_list_mode_off ();

                GtkWidget *signal_select_widget = NULL;
                UnitObj *tmp = NULL;
                if (sranger_common_hwi->check_pac() != -1) {
                        signal_select_widget = dsp_bp->grid_add_mixer_input_options (ch, mix_fbsource[ch], this);
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
                } else {
                        dsp_bp->grid_add_label (mixer_channel_label[ch]);
                }


                dsp_bp->grid_add_ec_with_scale (NULL, mixer_unit[ch], &mix_set_point[ch], ch==0? 0.0:-100.0, 100., "4g", 0.001, 0.01, mixer_remote_id_set[ch]);
                // dsp_bp->ec->set_adjustment_mode (PARAM_CONTROL_ADJUSTMENT_LOG | PARAM_CONTROL_ADJUSTMENT_ADD_MARKS );
                dsp_bp->ec->SetScaleWidget (dsp_bp->scale, 0);
                dsp_bp->ec->set_logscale_min (1e-4);
                dsp_bp->ec->set_logscale_magshift (-3);
                gtk_scale_set_digits (GTK_SCALE (dsp_bp->scale), 5);

                if (signal_select_widget)
                        g_object_set_data (G_OBJECT (signal_select_widget), "related_ec_setpoint", dsp_bp->ec);

                
                // dsp_bp->add_to_configure_hide_list (dsp_bp->scale);
                dsp_bp->set_input_width_chars (10);
                dsp_bp->set_configure_list_mode_on ();
                dsp_bp->grid_add_ec (NULL, Unity, &mix_gain[ch], -0.5, 0.5, "5g", 0.001, 0.01, mixer_remote_id_gn[ch]);
                dsp_bp->grid_add_ec (NULL, mixer_unit[ch], &mix_level[ch], -100.0, 100.0, "5g", 0.001, 0.01, mixer_remote_id_fl[ch]);

                if (tmp) delete (tmp); // done setting unit -- if custom
                
                if (signal_select_widget)
                        g_object_set_data (G_OBJECT (signal_select_widget), "related_ec_level", dsp_bp->ec);
                
                dsp_bp->grid_add_mixer_options (ch, mix_transform_mode[ch], this);
                dsp_bp->set_configure_list_mode_off ();
                dsp_bp->new_line ();
        }
        dsp_bp->set_configure_list_mode_off ();

        dsp_bp->set_input_width_chars ();
        
        // Z-Servo
        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
	input_signal = NULL;
	if (DSPPACClass) { // MK3
		int sig_i = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_Z_SERVO_INPUT_ID);
		if (sig_i >= 0)
			input_signal = g_strconcat (" on Signal ", sranger_common_hwi->lookup_signal_name_by_index (sig_i), NULL);
	}
        dsp_bp->new_grid_with_frame (g_strconcat ("Z-Servo ", input_signal, NULL));

        dsp_bp->set_label_width_chars (7);
        dsp_bp->set_input_width_chars (12);

        dsp_bp->set_configure_list_mode_on ();
	dsp_bp->grid_add_ec_with_scale ("CP", Unity, &z_servo[SERVO_CP], 0., 200., "5g", 1.0, 0.1, "fbs-cp");
        GtkWidget *ZServoCP = dsp_bp->input;
        dsp_bp->new_line ();
        dsp_bp->set_configure_list_mode_off ();
        dsp_bp->grid_add_ec_with_scale ("CI", Unity, &z_servo[SERVO_CI], 0., 200., "5g", 1.0, 0.1, "fbs-ci");
        GtkWidget *ZServoCI = dsp_bp->input;

        g_object_set_data( G_OBJECT (ZServoCI), "HasClient", ZServoCP);
        g_object_set_data( G_OBJECT (ZServoCP), "HasMaster", ZServoCI);
        g_object_set_data( G_OBJECT (ZServoCI), "HasRatio", GUINT_TO_POINTER((guint)round(1000.*z_servo[SERVO_CP]/z_servo[SERVO_CI])));
        
	input_signal = NULL;
	if (DSPPACClass) { // MK3
		int sig_i = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_M_SERVO_INPUT_ID);
		if (sig_i >= 0){
			input_signal = g_strconcat (" on Signal ", sranger_common_hwi->lookup_signal_name_by_index (sig_i), NULL);
			if (strcmp (sranger_common_hwi->lookup_signal_name_by_index (sig_i), "Null-Signal")){
                                dsp_bp->pop_grid ();
                                dsp_bp->new_line ();
                                dsp_bp->new_grid_with_frame (g_strconcat ("M-Servo on ", input_signal, NULL));

				dsp_bp->grid_add_ec_with_scale ("Setpoint", Frq, &m_servo[SERVO_SETPT], -100., 100., "5g", 0.001, 0.01, "fbs-motor-setpoint");
				//dsp_bp->grid_add_ec_with_scale ("Setpoint", Volt, &m_servo[SERVO_SETPT], -10., 10., "5g", 0.001, 0.01, "fbs-motor-setpoint");
                                dsp_bp->new_line ();
				dsp_bp->grid_add_ec_with_scale ("CP", Unity, &m_servo[SERVO_CP], -200., 200., "5g", 0.001, 0.01, "fbs-motor-cp");
                                dsp_bp->new_line ();
				dsp_bp->grid_add_ec_with_scale ("CI", Unity, &m_servo[SERVO_CI], -200., 200., "5g", 0.001, 0.01, "fbs-motor-ci");
			}
		}
	}
        dsp_bp->set_label_width_chars ();

	// ========================================
        PI_DEBUG (DBG_L4, "DSPC----SCAN ------------------------------- ");
        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->new_grid_with_frame ("Scan Characteristics");

        dsp_bp->start (4); // wx=4
        dsp_bp->set_label_width_chars (7);

        dsp_bp->set_configure_list_mode_on ();
//	dsp_bp->grid_add_ec_with_scale ("DynZoom", Unity, &dynamic_zoom, 0., 5., "5g", 0.01, 0.1, "fbs-scan-dyn-zoom");
//      dsp_bp->new_line ();

	dsp_bp->grid_add_ec_with_scale ("MoveSpd", Speed, &move_speed_x, 0.1, 10000., "5g", 1., 10., "fbs-scan-speed-move");
        dsp_bp->new_line ();
        dsp_bp->set_configure_list_mode_off ();

	dsp_bp->grid_add_ec_with_scale ("ScanSpd", Speed, &scan_speed_x_requested, 0.1, 10000., "5g", 1., 10., "fbs-scan-speed-scan");
        scan_speed_ec = dsp_bp->ec;
        dsp_bp->new_line ();
        
//	dsp_bp->grid_add_ec_with_scale ("ScanSpdReal", Speed,  &scan_speed_x, 0., 1e9, "5g", 1., 10.,  RemoteEntryList, ec->Freeze (), , "fbs-scan-speed-real");
//      dsp_bp->new_line ();


	// ======================================== Piezo Drive / Amplifier Settings
        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->new_grid_with_frame ("Piezo Drive Settings");

        const gchar *PDR_gain_label[6] = { "VX", "VY", "VZ", "VX0", "VY0", "VZ0" };
        const gchar *PDR_gain_key[6] = { "vx", "vy", "vz", "vx0", "vy0", "vz0" };
	for(j=0; j<6; j++) {
		if (j == 3 && main_get_gapp()->xsm->Inst->OffsetMode () == OFM_DSP_OFFSET_ADDING)
                        break;

                if (j == 3){
#if 0 // not needed
                        dsp_bp->grid_add_check_button ("SPD Link", "Sync Gains from Smart Piezo Drive", 1,
                                                       G_CALLBACK (DSPControl::spd_link_callback), this,
                                                       0, 1);
#endif
                        dsp_bp->new_line ();
                }

                gtk_label_set_width_chars (GTK_LABEL (dsp_bp->grid_add_label (PDR_gain_label[j])), 6);
        
		wid = gtk_combo_box_text_new ();
		g_object_set_data  (G_OBJECT (wid), "chindex", GINT_TO_POINTER (j));

		// Init gain-choicelist
		for(i=0; i<GAIN_POSITIONS; i++){
			AmpI.l = 0L;
			AmpI.s.ch = j;
			AmpI.s.x  = i;
			gchar *Vxyz = g_strdup_printf("%g",xsmres.V[i]);
			gchar *id = g_strdup_printf ("%ld",AmpI.l);
			gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), id, Vxyz);
			g_free (id);
			g_free (Vxyz);
		}

		g_signal_connect (G_OBJECT (wid), "changed",
				  G_CALLBACK (DSPControl::choice_Ampl_callback),
				  this);

                gchar *tmpkey = g_strconcat ("dsp-fbs-", PDR_gain_key[j], NULL); 
                // get last setting, will call callback connected above to update gains!
                g_settings_bind (hwi_settings, tmpkey,
                                 G_OBJECT (GTK_COMBO_BOX (wid)), "active",
                                 G_SETTINGS_BIND_DEFAULT);
                g_free (tmpkey);

                VXYZS0Gain[j] = wid; // store for remote access/manipulation

#if 0
                // obsoleted now, settings bound above
		switch(j){
		case 0: gtk_combo_box_set_active (GTK_COMBO_BOX (wid), xsmres.VXdefault); break;
		case 1: gtk_combo_box_set_active (GTK_COMBO_BOX (wid), xsmres.VYdefault); break;
		case 2: gtk_combo_box_set_active (GTK_COMBO_BOX (wid), xsmres.VZdefault); break;
		case 3: gtk_combo_box_set_active (GTK_COMBO_BOX (wid), xsmres.VX0default); break;
		case 4: gtk_combo_box_set_active (GTK_COMBO_BOX (wid), xsmres.VY0default); break;
		case 5: gtk_combo_box_set_active (GTK_COMBO_BOX (wid), xsmres.VZ0default); break;
		}
#endif
                
                dsp_bp->grid_add_widget (wid);
                dsp_bp->add_to_scan_freeze_widget_list (wid);
	}

        dsp_bp->notebook_tab_show_all ();

       
// ==== Folder: Advanced or DSP Expert (advanced Feedback & Scan settings) ========================================
        PI_DEBUG (DBG_L4, "DSPC----TAB-ADVANCED ------------------------------- ");

        dsp_bp->new_grid ();
        dsp_bp->start_notebook_tab ("Advanced", "tab-advanced", NOTEBOOK_TAB_ADVANCED, hwi_settings);
        dsp_bp->new_grid_with_frame ("SR-FB Characteristics");

	dsp_bp->grid_add_label ("Z-Control:");
	// read current FB (MD_PID) state
	gint32 dsp_state_mode;
	sranger_common_hwi->read_dsp_state (dsp_state_mode);
        PI_DEBUG (DBG_L4, "ADD_TOGGLE-DSPSTATE-PID: --- ");
	dsp_bp->grid_add_check_button ("Enable Feed Back Controller", "DSP feedback controller switch, normally on.", 1,
                                       G_CALLBACK (DSPControl::feedback_callback), this,
                                       dsp_state_mode, MD_PID);

        dsp_bp->set_configure_list_mode_on ();
	dsp_bp->grid_add_ec ("FrqRef", Frq, &frq_ref, 10000., 150000., "6g", 0.01, 0.1, "adv-dsp-freq-ref");
        dsp_bp->new_line (0,2);

        IIR_flag = IIR_I_crossover > 0 ? 1:0;
        dsp_bp->grid_add_check_button ("IIR self adaptive on MIX0", "enable self adaptive IIR filtering", 1,
                                       G_CALLBACK (DSPControl::IIR_callback), this,
                                       IIR_flag, 1);

	if (!DSPPACClass) { // obsolete on MK3
                dsp_bp->grid_add_check_button ("Random Noise Gen.", "White noise generating add mode");
		gtk_check_button_set_active (GTK_CHECK_BUTTON (dsp_bp->button), dsp_state_mode & MD_NOISE ? 1:0);
		g_signal_connect (G_OBJECT (dsp_bp->button), "toggled",
                                  G_CALLBACK (DSPControl::set_clr_mode_callback), (void*)MD_NOISE);
	}
        dsp_bp->new_line ();

	dsp_bp->grid_add_ec ("IIR0 fo min", Frq, &IIR_f0_min, 1., 10000., ".0f", 1., 100., "adv-iir0-fo-min");
	dsp_bp->grid_add_ec ("fo max", Frq, &IIR_f0_max[0], 100., 75000., ".0f", 100., 1000., "adv-iir-fo-max");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Current Crossover", Current_pA, &IIR_I_crossover, 0., 1000., ".1f", 0.1, 1., "adv-current-crossover");
	dsp_bp->grid_add_ec ("Current Offset", Current_pA, &LOG_I_offset, 0., 1000., ".1f", 0.1, 1.,  "adv-current-offset");
        dsp_bp->new_line ();
	if (DSPPACClass) { // MK3 MIXER INPUTS HAVE ALL IIR front ends
		dsp_bp->grid_add_ec ("IIR1 fo", Frq, &IIR_f0_max[1], 100., 75000., ".0f", 100., 1000.,  "adv-iir1-fo");
		dsp_bp->grid_add_ec ("IIR2 fo", Frq, &IIR_f0_max[2], 100., 75000., ".0f", 100., 1000.,  "adv-iir2-fo");
		dsp_bp->grid_add_ec ("IIR3 fo", Frq, &IIR_f0_max[3], 100., 75000., ".0f", 100., 1000.,  "adv-iir3-fo");
	}
        dsp_bp->set_configure_list_mode_off ();

	// -------- Automatic Raster Probe Mode Select
        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->new_grid_with_frame ("Automatic Raster Probe Mapping Control (DSP level) Graphs[Sources] -> DataMapN Ch");

	GtkWidget *auto_probe_menu = gtk_combo_box_text_new ();
	dsp_bp->grid_add_widget (auto_probe_menu);
	g_signal_connect (G_OBJECT (auto_probe_menu), "changed",
			  G_CALLBACK (DSPControl::auto_probe_callback),
			  this);
        {
                gchar *id = g_strdup_printf ("%d", PV_MODE_NONE);
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (auto_probe_menu), id, N_("No Auto Probe"));
                g_free (id);
        }

	dsp_bp->grid_add_ec ("RasterAB", Unity, &probe_trigger_raster_points_user, 0, 200, "5g", "adv-scan-raster");
	Gtk_EntryControl *raster_ec = dsp_bp->ec;
	dsp_bp->grid_add_ec (NULL, Unity, &probe_trigger_raster_points_b, 0, 200, "5g", "adv-scan-rasterb");
	Gtk_EntryControl *rasterb_ec = dsp_bp->ec;

	dsp_bp->grid_add_check_button ("Map Fill", "Enable to fill in between raster points if raster probe grid > 1.",
                                       1
                                       );
        g_settings_bind (hwi_settings, "probe-graph-enable-map-fill",
                         G_OBJECT (GTK_CHECK_BUTTON (dsp_bp->button)), "active",
                         G_SETTINGS_BIND_DEFAULT);
	dsp_bp->grid_add_check_button ("Add Events", "Disable to not add Probe Events to Master Scan.",
                                       1
                                       );
        g_settings_bind (hwi_settings, "probe-graph-enable-add-events",
                         G_OBJECT (GTK_CHECK_BUTTON (dsp_bp->button)), "active",
                         G_SETTINGS_BIND_DEFAULT);
	dsp_bp->grid_add_check_button ("Plot", "Plot data as profile(s) after every raster point probe.\nNot recommended for fast probing repeats.",
                                       1
                                       );
        g_settings_bind (hwi_settings, "probe-graph-enable-map-plot-events",
                         G_OBJECT (GTK_CHECK_BUTTON (dsp_bp->button)), "active",
                         G_SETTINGS_BIND_DEFAULT);
	dsp_bp->grid_add_check_button ("Save", "Save individual data as vpdata files.\nNot recommended for fast probing repeats.",
                                       1
                                       );
        g_settings_bind (hwi_settings, "probe-graph-enable-map-save-events",
                         G_OBJECT (GTK_CHECK_BUTTON (dsp_bp->button)), "active",
                         G_SETTINGS_BIND_DEFAULT);

        
	raster_ec->Freeze ();
	rasterb_ec->Freeze ();
	g_object_set_data (G_OBJECT (auto_probe_menu), "raster_ec", (gpointer)raster_ec);
	g_object_set_data (G_OBJECT (auto_probe_menu), "rasterb_ec", (gpointer)rasterb_ec);

        // performing sanity checks and auto adjustments if necessary for MK3:
	if (DSPPACClass) {
                PI_DEBUG (DBG_L4, "******* performing sanity checks of configuration and do auto adjustments if necessary for MK3:");
		// brief list of outmix configuration
		// in particular!! check for default OFFSET adding at OUTMIX_CH3/4_ADD_A
		int ch_si[8][5];
		int ns[8][5];
		int iid=DSP_SIGNAL_OUTMIX_CH0_INPUT_ID;
		gchar *outconfig=g_strdup("\n ** CURRENT MK3 OUTMIX CONFIGURATION ** ");
		gchar *tmp;
		for (int outn=0; outn<8; ++outn){
			for (int om=0; om<5; ++om){
				ch_si[outn][om] = sranger_common_hwi->query_module_signal_input (iid++);
                                if (ch_si[outn][om] < 0) // DISABLED?
                                        continue;
                                if (ch_si[outn][om] >= NUM_SIGNALS){ // ERROR
                                        g_warning ("CH_SI[ OUTn=%d, Om=%d] = %d is invalid.", outn, om, ch_si[outn][om]);
                                        ch_si[outn][om] = NUM_SIGNALS+1; // POINT TO END SIGNAL FOR SAFETY
                                        continue;
                                }
				if (sranger_common_hwi->dsp_signal_lookup_managed[ch_si[outn][om]].label && om<3) // and test for Null-Signal
					ns[outn][om] = !strcmp (sranger_common_hwi->dsp_signal_lookup_managed[ch_si[outn][om]].label, "Null-Signal") ? 1:0;
				else
					ns[outn][om] = 0;
			}

                        if (ch_si[outn][0] < 0) // DISABLED
                                tmp = g_strdup_printf ("%s\nOUT[%d] : DISABLED", outconfig, outn);
                        else
                                tmp = g_strdup_printf ("%s\nOUT[%d] := %s", outconfig, outn, sranger_common_hwi->dsp_signal_lookup_managed[ch_si[outn][0]].label); 
			g_free (outconfig); outconfig = tmp;

                        // if (sranger_common_hwi->dsp_signal_lookup_managed[ch_si[outn][3]].label) // SMAC_A ?
                        if (ch_si[outn][3] >= 0 && ch_si[outn][1] >= 0 && ch_si[outn][3] < NUM_SIGNALS && ch_si[outn][1] < NUM_SIGNALS) // SMAC_A ?
				tmp = g_strdup_printf ("%s + %s * %s", outconfig, 
						       sranger_common_hwi->dsp_signal_lookup_managed[ch_si[outn][1]].label,
						       sranger_common_hwi->dsp_signal_lookup_managed[ch_si[outn][3]].label
						       );
			else  // DBG::: ??? #8  0x00007fffc76e2572 in sranger_mk2_hwi_query () at sranger_mk2_hwi.C:1794 -- invalid params/bad pointer below eventually ?!?!
                                if (ch_si[outn][1] >= 0 && ch_si[outn][1] < NUM_SIGNALS)
                                        tmp = g_strdup_printf ("%s + %s", outconfig, sranger_common_hwi->dsp_signal_lookup_managed[ch_si[outn][1]].label);
                                else
                                        tmp = g_strdup_printf ("%s + SIGNAL-READ-ERROR[3,1?]", outconfig);
			g_free (outconfig); outconfig = tmp;

			// if (sranger_common_hwi->dsp_signal_lookup_managed[ch_si[outn][4]].label) // SMAC_B ?
			if (ch_si[outn][4] >= 0 && ch_si[outn][4] < NUM_SIGNALS) // SMAC_B ?
				tmp = g_strdup_printf ("%s + %s * %s", outconfig, 
						       sranger_common_hwi->dsp_signal_lookup_managed[ch_si[outn][2]].label,
						       sranger_common_hwi->dsp_signal_lookup_managed[ch_si[outn][4]].label
						       );
			else
                                if (ch_si[outn][2] >= 0 && ch_si[outn][2] < NUM_SIGNALS)
                                        tmp = g_strdup_printf ("%s - %s", outconfig, sranger_common_hwi->dsp_signal_lookup_managed[ch_si[outn][2]].label);
                                else
                                        tmp = g_strdup_printf ("%s - SIGNAL-READ-ERROR[4,2?]", outconfig);
			g_free (outconfig); outconfig = tmp;
		}

                // xsmres.ScannerZPolarity; // 1: pos, 0: neg (bool) -- adjust zpos_ref accordingly!
                // fix me: make zpos_ref signum manual or automatic depending on Z_SERVO - CONTROL NEG/POS setting --> verify at start, ask for correction.
                gboolean zpok;
                do{
                        zpok = false;
                        g_message ("Checking Z Polarity, Signal found: %s", sranger_common_hwi->dsp_signal_lookup_managed[sranger_common_hwi->query_module_signal_input (DSP_SIGNAL_OUTMIX_CH5_INPUT_ID)].label);
                        if (xsmres.ScannerZPolarity==0 && !strcmp (sranger_common_hwi->dsp_signal_lookup_managed[sranger_common_hwi->query_module_signal_input (DSP_SIGNAL_OUTMIX_CH5_INPUT_ID)].label, "Z Servo Neg")){
                                zpok = true;
                                g_message ("Checking Z Polarity (config=negative): OK");
                        }
                
                        if (xsmres.ScannerZPolarity==1 && !strcmp (sranger_common_hwi->dsp_signal_lookup_managed[sranger_common_hwi->query_module_signal_input (DSP_SIGNAL_OUTMIX_CH5_INPUT_ID)].label, "Z Servo")){
                                zpok = true;
                                g_message ("Checking Z Polarity (config=positive): OK");
                        }
                        if (!zpok){
#if 0
                                g_warning ("DSP Z Polarity is not matching GXSM4 configuration. Correct on user response only.");
                                if (main_get_gapp()->question_yes_no (N_("Instrument Scanner Z-Polarity Verification failed.\n"
                                                              "DSP Scanner Z Polarity signal setup is not matching GXSM4 configuration in preferences.\n"
                                                              "==> Critical Advise: Correct now?"),
                                                           NULL,
                                                           "<span foreground='red' size='large' weight='bold'>Critical Warning: %s</span>"
                                                           )){
                                        if (xsmres.ScannerZPolarity)
                                                sranger_common_hwi->change_signal_input (sranger_common_hwi->lookup_signal_by_name("Z Servo"), DSP_SIGNAL_OUTMIX_CH5_INPUT_ID);
                                        else
                                                sranger_common_hwi->change_signal_input (sranger_common_hwi->lookup_signal_by_name("Z Servo Neg"), DSP_SIGNAL_OUTMIX_CH5_INPUT_ID);
                                } else
                                        zpok = true; // ignore on user request
#else
                                g_warning ("DSP Z Polarity is not matching GXSM4 configuration. Auto adjusting DSP configuration now.");
                                if (xsmres.ScannerZPolarity)
                                        sranger_common_hwi->change_signal_input (sranger_common_hwi->lookup_signal_by_name("Z Servo"), DSP_SIGNAL_OUTMIX_CH5_INPUT_ID);
                                else
                                        sranger_common_hwi->change_signal_input (sranger_common_hwi->lookup_signal_by_name("Z Servo Neg"), DSP_SIGNAL_OUTMIX_CH5_INPUT_ID);
#endif
                        }
                } while (!zpok);
                
		int ox=0,oy=0;
		if (sranger_common_hwi->dsp_signal_lookup_managed[ch_si[3][1]].label)
			ox = !strcmp (sranger_common_hwi->dsp_signal_lookup_managed[ch_si[3][1]].label, "X Offset") ? 1:0;
		if (sranger_common_hwi->dsp_signal_lookup_managed[ch_si[3][1]].label)
			oy = !strcmp (sranger_common_hwi->dsp_signal_lookup_managed[ch_si[4][1]].label, "Y Offset") ? 1:0;

		int dsp_offset_adding = ox && oy;

		// verify DSP configuration with GXSM4 settings compatibility as good as possible (may be OK as more complicated special OK scenario possible)
		if ( (   main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING &&  dsp_offset_adding) // GXSM4  expects external/analog but DSP SET TO ADD OFFSETS?
		     || (main_get_gapp()->xsm->Inst->OffsetMode() != OFM_ANALOG_OFFSET_ADDING && !dsp_offset_adding) // GXSM4 expects internal DSP but DSP SET NOT TO ADD OFFSETS?
		     || !ns[3][2] // suspicious if any one set (custom/unsual) -- just as a warning check and cause message!
		     || sranger_common_hwi->dsp_signal_lookup_managed[ch_si[3][3]].label
		     || sranger_common_hwi->dsp_signal_lookup_managed[ch_si[3][4]].label
		     || !ns[4][2]
		     || sranger_common_hwi->dsp_signal_lookup_managed[ch_si[4][3]].label
		     || sranger_common_hwi->dsp_signal_lookup_managed[ch_si[4][4]].label
			    ){


			gchar *msg = g_strdup_printf ("Please verify your desired configuration:"
						      "\nEventual GXSM4 / DSP Offset Adding mode conflict:"
						      "\n =>  %s"
						      "\n  and"
						      "\n =>  %s"
						      "\nPlease review."
						      "\n ================================================== \n"
						      "%s"
						      "\nSet GXSM4 Preferences as desired to avoid this message or ignore if OK/custom.",
						      main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING ?
						      "Analog/AnalogOffsetAddig = TRUE: Analog OUT0/1 (X/Y Offset signal) are to be used externally or digitally via GXSM-Link to SPD"
						      : "Analog/AnalogOffsetAddig = FALSE: internal (DSP) X/Y Offset signals should be added digitally to X/Y Scan Rot.",
						      dsp_offset_adding ?
						      "Found DSP signals setup for  X/Y Scan Rot + X/Y Offset."
						      : "Found DSP signals not (or unusually) setup for Offset adding.",
						      outconfig
						      );

			main_get_gapp()->alert (N_("Warning"), N_("GXSM4->InstSPM Offset settings verification with DSP settings failed"), msg, 1);
                        main_get_gapp()->monitorcontrol->LogEvent ("GXSM4 startup MK3 DSP signal verification", "WARNING SITUATION FOUND!");
                        main_get_gapp()->monitorcontrol->LogEvent ("WARNING", msg);
			g_free (msg);
		} else {
			main_get_gapp()->message (N_(outconfig));
                        main_get_gapp()->monitorcontrol->LogEvent ("GXSM4 startup MK3 DSP signal verification", "NORMAL");
                        main_get_gapp()->monitorcontrol->LogEvent ("INFORMATION", outconfig);
                }
                
		PI_DEBUG (DBG_L1, outconfig);
		g_free (outconfig);

	} else if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING && (dsp_state_mode & MD_OFFSETADDING ? 1:0)
		   || main_get_gapp()->xsm->Inst->OffsetMode() != OFM_ANALOG_OFFSET_ADDING && (dsp_state_mode & MD_OFFSETADDING ? 0:1)
		   ) {
		gchar *msg = g_strdup_printf ("Please check and adjust:"
					      "\nGXSM4 is set for Offset Adding: %s"
					      "\nMK2-A810 DSP is configured for: %s"
					      "\nSee DSP Control->Advanced Folder for DSP reconfiguration of this,"
					      "\nelse set GXSM4 Preferences as desired..",
					      main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING ? "Analog via DAC0/1":"digital (DSP), offset outputs not used!",
					      dsp_state_mode & MD_OFFSETADDING ? "digital adding":"external/analog adding of ADC0/1_o to ADC3/4_s" );
		main_get_gapp()->alert (N_("Warning"), N_("GXSM4->InstSPM Offset settings verification with DSP settings failed"), msg, 1);
                main_get_gapp()->monitorcontrol->LogEvent ("GXSM4 startup MK2 DSP configuration verification", "WARNING SITUATION FOUND!");
                main_get_gapp()->monitorcontrol->LogEvent ("WARNING", msg);
		g_free (msg);
	} else {
                main_get_gapp()->monitorcontrol->LogEvent ("GXSM4 startup MK2 DSP configuration verification", "NORMAL");
        }

	// ========================================
        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->new_grid_with_frame ("Scan Characteristics - Expert");

        dsp_bp->set_configure_list_mode_on ();
        dsp_bp->new_line (0,2);
        dsp_bp->grid_add_check_button ("Internal Offset Adding", "set DSP internal (digital) offset adding.", 1,
                                       G_CALLBACK(DSPControl::set_clr_mode_callback), GINT_TO_POINTER (MD_OFFSETADDING),
                                       dsp_state_mode, MD_OFFSETADDING);

        dsp_bp->set_configure_list_mode_off ();

        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Fast Return", Unity, &fast_return, 1., 1000., "5g", 1., 10.,  "adv-scan-fast-return");
	dsp_bp->grid_add_ec ("Fwd Slow Down", Unity, &scan_forward_slow_down, 1, 32000, "5g", "adv-scan-fwd-slow-down");

        dsp_bp->set_configure_list_mode_on ();
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Pre Pts", Unity, &pre_points, 0, 100, "5g", "adv-scan-pre-pts");
	dsp_bp->grid_add_ec ("Fwd Slow Down 2nd", Unity, &scan_forward_slow_down_2nd, 1, 32000, "5g", "adv-scan-fwd-slow-down-2nd");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Dyn Zoom", Unity, &dynamic_zoom, 0., 5., "5g", 0.01, 0.1,  "adv-scan-dyn-zoom");
	dsp_bp->grid_add_ec ("XS 2nd ZOff", Angstroem, &x2nd_Zoff, -10000., 10000., ".2f", 1., 1., "adv-scan-xs2nd-z-offset");
        dsp_bp->new_line ();
        dsp_bp->set_configure_list_mode_off ();

        dsp_bp->set_scale_nx (2);
        dsp_bp->grid_add_ec_with_scale ("Slope X", Unity, &area_slope_x, -0.2, 0.2, ".5f", 0.0001, 0.0001,  "adv-scan-slope-x"); slope_x_ec = dsp_bp->ec;
        gtk_scale_set_digits (GTK_SCALE (dsp_bp->scale), 5);
        dsp_bp->new_line ();

        dsp_bp->grid_add_ec_with_scale ("Slope Y", Unity, &area_slope_y, -0.2, 0.2, ".5f", 0.0001, 0.0001,  "adv-scan-slope-y"); slope_y_ec = dsp_bp->ec;
        gtk_scale_set_digits (GTK_SCALE (dsp_bp->scale), 5);
        dsp_bp->set_scale_nx ();
        dsp_bp->new_line (0,2);

        dsp_bp->grid_add_check_button ("Enable Slope Compensation", "enable analog slope compensation...", 1,
                                       G_CALLBACK(DSPControl::DSP_slope_callback), this, area_slope_compensation_flag, 0);
        g_settings_bind (hwi_settings, "adv-enable-slope-compensation",
                         G_OBJECT (GTK_CHECK_BUTTON (dsp_bp->button)), "active",
                         G_SETTINGS_BIND_DEFAULT);

        dsp_bp->grid_add_check_button ("Enable automatic Tip return to center", "enable auto tip return to center after scan", 1,
                                       G_CALLBACK(DSPControl::DSP_cret_callback), this, center_return_flag, 0);
        g_settings_bind (hwi_settings, "adv-enable-tip-return-to-center",
                         G_OBJECT (GTK_CHECK_BUTTON (dsp_bp->button)), "active",
                         G_SETTINGS_BIND_DEFAULT);

        PI_DEBUG (DBG_L4, "DSPC----FB-CONTROL -- INPUT-SRCS ----------------------------- ");

	// SCAN CHANNEL INPUT SOURCE CONFIGURATION MENUS
	if (sranger_common_hwi->check_pac() != -1) {
                dsp_bp->pop_grid ();
                dsp_bp->new_line ();
                dsp_bp->new_grid_with_frame ("Scan Input Source Selection");

                dsp_bp->set_configure_list_mode_on ();
                dsp_bp->add_to_configure_list (dsp_bp->frame); // manage en block
                dsp_bp->set_configure_list_mode_off ();
        
                dsp_bp->grid_add_label ("VP section");
                dsp_bp->grid_add_label ("VP signal");
                dsp_bp->grid_add_label ("... Signal Scan Sources Selections ...", NULL, 4);

                dsp_bp->new_line ();

                dsp_bp->set_input_width_chars (4);
                dsp_bp->grid_add_ec (NULL, Unity, &DSP_vpdata_ij[0], 0, 7, ".0f", "fbs-vp-section");
                dsp_bp->set_input_width_chars ();
                
		VPSig_menu = wid = gtk_combo_box_text_new ();
		g_signal_connect (G_OBJECT (wid), "changed",
				  G_CALLBACK (DSPControl::choice_vector_index_j_callback),
				  this);

		for (int j=0; j<8; j++) {
			if (j<4){
				int si = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE0_INPUT_ID+j);
				{ gchar *id = g_strdup_printf ("%d", j); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), id, sranger_common_hwi->lookup_dsp_signal_managed (si)->label); g_free (id); }
			} else 
				switch (j){
				case 4: { gchar *id = g_strdup_printf ("%d", j); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), id, "Counter 0"); g_free (id); } break;
				case 5: { gchar *id = g_strdup_printf ("%d", j); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), id, "Counter 1"); g_free (id); } break;
				default: { gchar *id = g_strdup_printf ("%d", j); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), id, "NULL"); g_free (id); } break;
				}
		}
		gtk_combo_box_set_active (GTK_COMBO_BOX (wid), DSP_vpdata_ij[1]);
		dsp_bp->grid_add_widget (wid);

                for (int i=0; i<4; ++i){
                        dsp_bp->grid_add_scan_input_signal_options (i, scan_source[i], this);
                        VPScanSrcVPitem[i] =  dsp_bp->wid;
                }
	}

        // LDC -- Enable Linear Drift Correction -- Controls
        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->new_grid_with_frame ("Enable Linear Drift Correction (LDC)");
        dsp_bp->set_configure_list_mode_on ();

	LDC_status = dsp_bp->grid_add_check_button ("Enable Linear Drift Correction", NULL, 3);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (LDC_status), 0);
	ldc_flag = 0;
	g_signal_connect (G_OBJECT (LDC_status), "toggled",
			    G_CALLBACK (DSPControl::ldc_callback), this);

	FastScan_status = dsp_bp->grid_add_check_button ("Enable Fast Scan (sine)", NULL, 3);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(FastScan_status), 0);
	fast_scan_flag = 0;
	g_signal_connect (G_OBJECT (FastScan_status), "toggled",
			    G_CALLBACK (DSPControl::fast_scan_callback), this);

	if (sranger_common_hwi->check_pac() == -1) // MK3 only so far
		gtk_widget_set_sensitive (FastScan_status, FALSE);

        dsp_bp->new_line ();

        // LDC settings
	dsp_bp->grid_add_ec ("LDCdX", Speed, &dxdt, -100., 100., "5g", 0.001, 0.01, "fbs-scan-ldc-dx");
        dsp_bp->grid_add_ec ("dY", Speed, &dydt, -100., 100., "5g", 0.001, 0.01, "fbs-scan-ldc-dy");
        dsp_bp->grid_add_ec ("dZ", Speed, &dzdt, -100., 100., "5g", 0.001, 0.01, "fbs-scan-ldc-dz");

        dsp_bp->set_configure_list_mode_off ();

        dsp_bp->set_input_width_chars (8);
        

        dsp_bp->notebook_tab_show_all ();
        dsp_bp->pop_grid ();

// ==== Folder Set for Vector Probe ========================================
// ==== Folder: I-V STS setup ========================================
        PI_DEBUG (DBG_L4, "DSPC----TAB-IV ------------------------------- ");

        dsp_bp->new_grid ();
        dsp_bp->start_notebook_tab ("STS", "tab-sts", NOTEBOOK_TAB_STS, hwi_settings);
	// ==================================================
        dsp_bp->new_grid_with_frame ("I-V Type Spectroscopy");
	// ==================================================

	// add item to auto-probe-menu
	{ 
                gchar *id = g_strdup_printf ("%d", PV_MODE_IV);
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (auto_probe_menu), id, N_("STS-Raster"));
                g_free (id);
        }
	
	dsp_bp->grid_add_ec ("Sections", Unity, &IV_sections, 1,6, "2g", "IV-Sections");
        GtkWidget *multiIV_checkbutton = dsp_bp->grid_add_check_button ("Multi-IV mode", "enable muli section IV curve mode", 1,
                                                                        G_CALLBACK(DSPControl::DSP_multiIV_callback), this, IV_sections-1);

        dsp_bp->new_line ();
                
	dsp_bp->grid_add_label ("IV Probe"); dsp_bp->grid_add_label ("Start"); dsp_bp->grid_add_label ("End");  dsp_bp->grid_add_label ("Points");
	for (int i=0; i<6; ++i) {
                dsp_bp->new_line ();

                gchar *tmp = g_strdup_printf("IV[%d]", i+1);
                dsp_bp->grid_add_label (tmp);
                if (i>0) multi_IVsec_list = g_slist_prepend (multi_IVsec_list, dsp_bp->label);
                g_free(tmp);

                if (i > 0){
                        tmp = g_strdup_printf(REMOTE_PREFIX "%d-", i+1);
                        dsp_bp->set_pcs_remote_prefix (tmp);
                        g_free(tmp);
                }
                
                dsp_bp->grid_add_ec (NULL, Volt, &IV_start[i], -10.0, 10., "5.3g", 0.1, 0.025, "IV-Start", i);
                if (i == 5) dsp_bp->init_ec_array ();
                if (i>0) multi_IVsec_list = g_slist_prepend (multi_IVsec_list, dsp_bp->input);

                dsp_bp->grid_add_ec (NULL, Volt, &IV_end[i], -10.0, 10.0, "5.3g", 0.1, 0.025, "IV-End", i);
                if (i == 5) dsp_bp->init_ec_array ();
                if (i>0) multi_IVsec_list = g_slist_prepend (multi_IVsec_list, dsp_bp->input);

                dsp_bp->grid_add_ec (NULL, Unity, &IV_points[i], 1, 1000, "5g", "IV-Points", i);
                if (i == 5) dsp_bp->init_ec_array ();
                if (i>0) multi_IVsec_list = g_slist_prepend (multi_IVsec_list, dsp_bp->input);
        }
        dsp_bp->set_pcs_remote_prefix (REMOTE_PREFIX);
        
        dsp_bp->new_line ();

        dsp_bp->set_configure_list_mode_on ();
	dsp_bp->grid_add_ec ("dz", Angstroem, &IV_dz, -1000.0, 1000.0, "5.4g", 1., 2., "IV-dz");
	dsp_bp->grid_add_ec ("#dZ probes", Unity, &IVdz_repetitions, 0, 100, "3g", "IV-dz-rep");
        dsp_bp->set_configure_list_mode_off ();

        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Slope", Vslope, &IV_slope,0.1,1000.0, "5.3g", 1., 10., "IV-Slope");
	dsp_bp->grid_add_ec ("Slope Ramp", Vslope, &IV_slope_ramp,0.1,1000.0, "5.3g", 1., 10., "IV-Slope-Ramp");
        dsp_bp->new_line ();
        dsp_bp->set_configure_list_mode_on ();
	dsp_bp->grid_add_ec ("#IV sets", Unity, &IV_repetitions, 1, 100, "3g", "IV-rep");

        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Line-dX", Angstroem, &IV_dx, -1e6, 1e6, "5.3g", 1., 10., "IV-Line-dX");
        dsp_bp->grid_add_ec ("dY",  Angstroem, &IV_dy, -1e6, 1e6, "5.3g", 1., 10.,  "IV-Line-dY");
#ifdef MOTOR_CONTROL
	dsp_bp->grid_add_ec ("Line-dM",  Volt, &IV_dM, -1e6, 1e6, "5.3g", 1., 10.,  "IV-Line-dM");
#else
	IV_dM = 0.;
#endif
        dsp_bp->new_line ();
        dsp_bp->grid_add_ec ("Points", Unity, &IV_dxy_points, 1, 1000, "3g" "IV-Line-Pts");
	dsp_bp->grid_add_ec ("Slope", Speed, &IV_dxy_slope, -1e6, 1e6, "5.3g", 1., 10.,  "IV-Line-slope");
        dsp_bp->new_line ();
        dsp_bp->grid_add_ec ("Delay", Time, &IV_dxy_delay, 0., 10., "3g", 0.01, 0.1,  "IV-Line-Final-Delay");

        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Final Delay", Time, &IV_final_delay, 0., 1., "5.3g", 0.001, 0.01,  "IV-Final-Delay");
	dsp_bp->grid_add_ec ("IV-Recover-Delay", Time, &IV_recover_delay, 0., 1., "5.3g", 0.001, 0.01,  "IV-Recover-Delay");
        dsp_bp->set_configure_list_mode_off ();
        dsp_bp->new_line ();

	IV_status = dsp_bp->grid_add_probe_status ("Status");
        dsp_bp->new_line ();

        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->grid_add_probe_controls (TRUE,
                                         IV_option_flags, GCallback (callback_change_IV_option_flags),
                                         IV_auto_flags,  GCallback (callback_change_IV_auto_flags),
                                         GCallback (DSPControl::Probing_exec_IV_callback),
                                         GCallback (DSPControl::Probing_write_IV_callback),
                                         GCallback (DSPControl::Probing_graph_callback),
                                         GCallback (DSPControl::Probing_abort_callback),
                                         this,
                                         "IV");
        dsp_bp->notebook_tab_show_all ();
        dsp_bp->pop_grid ();


        // ==== Folder: FZ (Force-Distance/AFM) setup ========================================
        PI_DEBUG (DBG_L4, "DSPC----TAB-FZ -------------------------------- ");
        dsp_bp->new_grid ();
        dsp_bp->start_notebook_tab ("Z", "tab-z", NOTEBOOK_TAB_Z, hwi_settings);

	// ==================================================
	
	// add item to auto-probe-menu
        {
                gchar *id = g_strdup_printf ("%d", PV_MODE_FZ);
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (auto_probe_menu), id, N_("Z-Raster"));
                g_free (id);
        }

	// ========================================
 	dsp_bp->new_grid_with_frame ("Z Manipulation (F-Z, Tip tune, ...)");

	// stuff here ...
	dsp_bp->grid_add_ec ("Start", Angstroem, &FZ_start, -1000.0, 1000., "5.3g", 1., 10., "Z-start");
	dsp_bp->grid_add_ec ("End", Angstroem, &FZ_end, -1000.0, 1000.0, "5.3g", 1., 10.,  "Z-end");
	dsp_bp->grid_add_ec ("Points", Unity, &FZ_points, 1, 4000, "5g", "Z-Points");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Slope", Speed, &FZ_slope,0.1,10000.0, "5.3g", 1., 10.,  "Z-Slope");
	dsp_bp->grid_add_ec ("# probes", Unity, &FZ_repetitions, 1, 1000, "5.3g", "Z-Reps");
        dsp_bp->new_line ();

        // Limiter -- moved to Graphs as more generic
        
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Slope Ramp", Speed, &FZ_slope_ramp, 0.1,10000.0, "5.3g", 1., 10., "Z-Slope-Ramp");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Final Delay", Time, &FZ_final_delay, 0., 1., "5.3g", 0.001, 0.01, "Z-Final-Delay");

        dsp_bp->new_line ();

	FZ_status = dsp_bp->grid_add_probe_status ("Status");
        dsp_bp->new_line ();

        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->grid_add_probe_controls (TRUE,
                                         FZ_option_flags, GCallback (callback_change_FZ_option_flags),
                                         FZ_auto_flags,  GCallback (callback_change_FZ_auto_flags),
                                         GCallback (DSPControl::Probing_exec_FZ_callback),
                                         GCallback (DSPControl::Probing_write_FZ_callback),
                                         GCallback (DSPControl::Probing_graph_callback),
                                         GCallback (DSPControl::Probing_abort_callback),
                                         this,
                                         "FZ");
        dsp_bp->notebook_tab_show_all ();
        dsp_bp->pop_grid ();


        // ==== Folder: PL (Puls) setup ========================================
        PI_DEBUG (DBG_L4, "DSPC----TAB-PL ------------------------------- ");
        dsp_bp->new_grid ();
        dsp_bp->start_notebook_tab ("PL", "tab-pl", NOTEBOOK_TAB_PL, hwi_settings);

	// add item to auto-probe-menu
	{ 
                gchar *id = g_strdup_printf ("%d", PV_MODE_PL);
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (auto_probe_menu), id, N_("PL-Raster"));
                g_free (id);
        }
	// ========================================
 	dsp_bp->new_grid_with_frame ("Z-dip+Bias Puls Control (Tip tune, ...) Duration is 'Time down'");

	// stuff here ...
	dsp_bp->grid_add_ec ("Duration", msTime, &PL_duration, 0., 1000., ".2f", 1., 10., "PL-Duration");
	dsp_bp->grid_add_ec ("Slope", Vslope, &PL_slope, 0.1,1e5, "5.3g", 1., 10., "PL-Slope");
	dsp_bp->grid_add_ec ("Resolution", msTime, &PL_time_resolution, 0.0, 1e5, ".2g", 1., 10., "PL-Res");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Volts", Volt, &PL_volt, -10., 10., ".3f", .1, 1., "PL-Volts");
	dsp_bp->grid_add_ec ("dZ", Angstroem, &PL_dZ, -100., 100., ".2f", 1., 10., "PL-dZ");
	dsp_bp->grid_add_ec ("dZ ext", Unity, &PL_dZ_ext, -10., 10., ".2f", 1., 10., "PL-dZ-ext");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Set Start", Unity, &PL_remote_set_value, -1e5, 1e5, ".4f", .1, 1., "PL-SetStart");
        dsp_bp->new_line ();

	gchar *tmptxt = xrm.GetStr ("Probing_PL_remote_set_target", "none");
        PL_remote_set_target = dsp_bp->grid_add_input ("Target Param");
        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(PL_remote_set_target))), tmptxt, -1);
        gtk_editable_set_editable (GTK_EDITABLE (PL_remote_set_target), TRUE);
        g_free (tmptxt);
        dsp_bp->new_line ();

	dsp_bp->grid_add_ec ("Step", Volt, &PL_step, -10., 10., ".3f", 0.1, 1., "PL-Step");
	dsp_bp->grid_add_ec ("Step dZ", Angstroem, &PL_stepZ, -100., 100., ".3f", 0.1, 1., "PL-Step-dZ");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Initial Delay", Time, &PL_initial_delay, 0., 1., "5.3g", 0.001, 0.01, "PL-Initial-Delay");
	dsp_bp->grid_add_ec ("Final Delay", Time, &PL_final_delay, 0., 1., "5.3g", 0.001, 0.01, "PL-Final-Delay");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Repetitions", Unity, &PL_repetitions, 1, 100, ".0f", "PL-Repetitions");
        dsp_bp->new_line ();

	PL_status = dsp_bp->grid_add_probe_status ("Status");
        dsp_bp->new_line ();

        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->grid_add_probe_controls (FALSE,
                                         PL_option_flags, GCallback (callback_change_PL_option_flags),
                                         PL_auto_flags,  GCallback (callback_change_PL_auto_flags),
                                         GCallback (DSPControl::Probing_exec_PL_callback),
                                         GCallback (DSPControl::Probing_write_PL_callback),
                                         GCallback (DSPControl::Probing_graph_callback),
                                         GCallback (DSPControl::Probing_abort_callback),
                                         this,
                                         "PL");
        dsp_bp->notebook_tab_show_all ();
        dsp_bp->pop_grid ();
        
// ==== Folder: LP (Laserpuls) setup ========================================
        PI_DEBUG (DBG_L4, "DSPC----TAB-LP ------------------------------- ");
        dsp_bp->new_grid ();
        dsp_bp->start_notebook_tab ("LP", "tab-lp", NOTEBOOK_TAB_LPC, hwi_settings);
       
	// add item to auto-probe-menu
	{
                gchar *id = g_strdup_printf ("%d", PV_MODE_LP);
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (auto_probe_menu), id, N_("LP-Raster"));
                g_free (id);
        }
 
	// ========================================
 	dsp_bp->new_grid_with_frame ("Laser Pulse Control (Trigger on X0, Offset adding: OFF)");
 
	// stuff here ...
	dsp_bp->grid_add_ec ("FB Time", msTime, &LP_duration, 0., 1000., ".2f", 1., 10., "LP-FB-Time");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Trigger Volts", Volt, &LP_volt, -10, 10, ".3f", .1, 1., "LP-Trigger-Volts");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Trigger-Time", msTime, &LP_triggertime, 0., 1000., ".2f", 1., 10., "LP-Trigger-Time");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Tip-Retract", Angstroem, &LP_FZ_end, -100.0, 100.0, "5.3g", 1., 10., "LP-Tip-Retract");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Laser-Delay", msTime, &LP_final_delay, 0., 1000., ".2f", 1., 10., "LP-Laser-Delay");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("LP-Slope", Vslope, &LP_slope, 0.1,1e5, "5.3g", 1., 10., "LP-Slope");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("LP-Repetitions", Unity, &LP_repetitions, 1, 100, ".0f", "LP-Repetitions");
        dsp_bp->new_line ();

	LP_status = dsp_bp->grid_add_probe_status ("Status");
        dsp_bp->new_line ();

        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->grid_add_probe_controls (FALSE,
                                         LP_option_flags, GCallback (callback_change_LP_option_flags),
                                         LP_auto_flags,  GCallback (callback_change_LP_auto_flags),
                                         GCallback (DSPControl::Probing_exec_LP_callback),
                                         GCallback (DSPControl::Probing_write_LP_callback),
                                         GCallback (DSPControl::Probing_graph_callback),
                                         GCallback (DSPControl::Probing_abort_callback),
                                         this,
                                         "LP");
        dsp_bp->notebook_tab_show_all ();
        dsp_bp->pop_grid ();

        // ==== Folder: SP (Special/Slow Puls) setup ========================================
        PI_DEBUG (DBG_L4, "DSPC----TAB-SP ------------------------------- ");
        dsp_bp->new_grid ();
        dsp_bp->start_notebook_tab ("SP", "tab-sp", NOTEBOOK_TAB_SP, hwi_settings);
	
	// ========================================
 	dsp_bp->new_grid_with_frame ("Slow Pulse Control (Sputter Anneal, ...)");

	// stuff here ...
	dsp_bp->grid_add_ec ("Duration", minTime, &SP_duration, 0., 1000., ".2f", 1., 10., "SP-Duration");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Volts", Volt, &SP_volt, -10, 10, ".3f", .1, 1., "SP-Volts");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Ramp Time", minTime, &SP_ramptime, 0.1,1e5, ".2f", 1., 10., "SP-Ramp-Time");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Flag V on X", Volt, &SP_flag_volt, -2.,2., ".3f", 1., 10., "SP-Flag-V-on-X");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Delay", minTime, &SP_final_delay, 0., 1., ".2f", 0.001, 0.01, "SP-Delay");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Repetitions", Unity, &SP_repetitions, 1, 100, ".0f", "SP-Repetitions");
        dsp_bp->new_line ();

	SP_status = dsp_bp->grid_add_probe_status ("Status");
        dsp_bp->new_line ();

        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->grid_add_probe_controls (FALSE,
                                         SP_option_flags, GCallback (callback_change_SP_option_flags),
                                         SP_auto_flags,  GCallback (callback_change_SP_auto_flags),
                                         GCallback (DSPControl::Probing_exec_SP_callback),
                                         GCallback (DSPControl::Probing_write_SP_callback),
                                         GCallback (DSPControl::Probing_graph_callback),
                                         GCallback (DSPControl::Probing_abort_callback),
                                         this,
                                         "SP");
        dsp_bp->notebook_tab_show_all ();
        dsp_bp->pop_grid ();
        
// ==== Folder: TS (Time Spectroscopy) setup ========================================
        PI_DEBUG (DBG_L4, "DSPC----TAB-TS ------------------------------- ");

        dsp_bp->new_grid ();
        dsp_bp->start_notebook_tab ("TS", "tab-ts", NOTEBOOK_TAB_TS, hwi_settings);
	
	// add item to auto-probe-menu
	{
                gchar *id = g_strdup_printf ("%d", PV_MODE_TS);
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (auto_probe_menu), id, N_("TS-Raster"));
                g_free (id);
        }

	// ========================================
 	dsp_bp->new_grid_with_frame ("Time Spectroscopy (Signal/Noise Analysis, ...)");

	// stuff here ...
	dsp_bp->grid_add_ec ("Duration", msTime, &TS_duration, 0., 300000., ".2f", 1., 10., "TS-Duration");
	dsp_bp->grid_add_ec ("Points", Unity, &TS_points, 16., 8192., ".0f", .1, 1., "TS-Points");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Repetitions", Unity, &TS_repetitions, 1, 100, ".0f", "TS-Repetitions");
        dsp_bp->new_line ();

	TS_status = dsp_bp->grid_add_probe_status ("Status");
        dsp_bp->new_line ();

        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->grid_add_probe_controls (FALSE,
                                         TS_option_flags, GCallback (callback_change_TS_option_flags),
                                         TS_auto_flags,  GCallback (callback_change_TS_auto_flags),
                                         GCallback (DSPControl::Probing_exec_TS_callback),
                                         GCallback (DSPControl::Probing_write_TS_callback),
                                         GCallback (DSPControl::Probing_graph_callback),
                                         GCallback (DSPControl::Probing_abort_callback),
                                         this,
                                         "TS");
        dsp_bp->notebook_tab_show_all ();
        dsp_bp->pop_grid ();

// ==== Folder: LatMan (lateral manipulation) "GVP" -- now renamed into VP (Vecotr Program) setup ========================================
        dsp_bp->new_grid ();
        dsp_bp->start_notebook_tab ("GVP", "tab-gvp", NOTEBOOK_TAB_GVP, hwi_settings);

	// add item to auto-probe-menu
	{
                gchar *id = g_strdup_printf ("%d", PV_MODE_GVP);
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (auto_probe_menu), id, N_("GVP-Raster"));
                g_free (id);
        }

	// ========================================
        PI_DEBUG (DBG_L4, "DSPC----TAB-VP ------------------------------- ");

 	dsp_bp->new_grid_with_frame ("Generic Vector Program (VP) Probe and Manipulation");
 	// g_print ("================== TAB 'GVP' ============= Generic Vector Program (VP) Probe and Manipulation\n");

	// ----- VP Program Vectors Headings
	// ------------------------------------- divided view

	// ------------------------------------- VP program table, scrolled
 	GtkWidget *vp = gtk_scrolled_window_new ();
        gtk_widget_set_vexpand (vp, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (vp), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_min_content_height  (GTK_SCROLLED_WINDOW (vp), 100);
        gtk_widget_set_vexpand (dsp_bp->grid, TRUE);
        dsp_bp->grid_add_widget (vp); // only widget in dsp_bp->grid

        dsp_bp->new_grid ();
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (vp), dsp_bp->grid);
        
        // ----- VP Program Vectors Listing -- headers
	VPprogram[0] = dsp_bp->grid;

        dsp_bp->set_no_spin ();
        
        dsp_bp->grid_add_label ("Vec[PC]", "Vector Program Counter");
        dsp_bp->grid_add_label ("VP-dU", "vec-du");
        dsp_bp->grid_add_label ("VP-dX", "vec-dx (default or mapped)");
        dsp_bp->grid_add_label ("VP-dY", "vec-dy (default or mapped)");
        dsp_bp->grid_add_label ("VP-dZ", "vec-dz");
        dsp_bp->set_configure_list_mode_on ();
        dsp_bp->grid_add_label ("VP-dSig", "vec-dSignal mapped");
        dsp_bp->set_configure_list_mode_off ();
        dsp_bp->grid_add_label ("time", "total time for VP section");
        dsp_bp->grid_add_label ("points", "points (# vectors to add)");
        dsp_bp->grid_add_label ("FB", "Feedback");
        dsp_bp->set_configure_list_mode_on ();
        dsp_bp->grid_add_label ("IOS", "GPIO-Set");
        dsp_bp->grid_add_label ("IOR", "GPIO-Read");
        dsp_bp->grid_add_label ("TP", "TRIGGER-POS");
        dsp_bp->grid_add_label ("TN", "TRIGGER-NEG");
        dsp_bp->grid_add_label ("GPIO", "data for GPIO / mask for trigger on GPIO");
        dsp_bp->grid_add_label ("Nrep", "VP # repetition");
        dsp_bp->grid_add_label ("PCJR", "Vector-PC jump relative\n (example: set to -2 to repeat previous two vectors.)");
        dsp_bp->set_configure_list_mode_off ();
        dsp_bp->grid_add_label("Shift", "Edit: Shift VP Block up/down");

        dsp_bp->new_line ();
        
	GSList *EC_vpc_opt_list=NULL;
        Gtk_EntryControl *ec_iter[12];
	for (int k=0; k < N_GVP_VECTORS; ++k) {
		gchar *tmpl = g_strdup_printf ("vec[%02d]", k); 

                // g_print ("GVP-DEBUG:: %s -[%d]------> GVPdu=%g, ... points=%d,.. opt=%8x\n", tmpl, k, GVP_du[k], GVP_points[k],  GVP_opt[k]);

		dsp_bp->grid_add_label (tmpl, "Vector PC");
		dsp_bp->grid_add_ec (NULL,      Volt, &GVP_du[k], -10.0,   10.0,   "6.4g", 1., 10., "gvp-du", k); 
                if (k == (N_GVP_VECTORS-1)) dsp_bp->init_ec_array ();

		dsp_bp->grid_add_ec (NULL, Angstroem, &GVP_dx[k], -100000.0, 100000.0, "6.4g", 1., 10., "gvp-dx", k); 
                if (k == (N_GVP_VECTORS-1)) dsp_bp->init_ec_array ();

		dsp_bp->grid_add_ec (NULL, Angstroem, &GVP_dy[k], -100000.0, 100000.0, "6.4g", 1., 10., "gvp-dy", k); 
                if (k == (N_GVP_VECTORS-1)) dsp_bp->init_ec_array ();

		dsp_bp->grid_add_ec (NULL, Angstroem, &GVP_dz[k], -100000.0, 100000.0, "6.4g", 1., 10., "gvp-dz", k); 
                if (k == (N_GVP_VECTORS-1)) dsp_bp->init_ec_array ();

                dsp_bp->set_configure_list_mode_on (); // === advanced ===========================================
		dsp_bp->grid_add_ec (NULL,    Volt, &GVP_dsig[k], -1000.0, 1000.0, "6.4g",1., 10., "gvp-dsig", k); 
                if (k == (N_GVP_VECTORS-1)) dsp_bp->init_ec_array ();
                dsp_bp->set_configure_list_mode_off (); // ========================================================

		dsp_bp->grid_add_ec (NULL,      Time, &GVP_ts[k], 0., 10000.0,     "5.4g", 1., 10., "gvp-dt", k); 
                if (k == (N_GVP_VECTORS-1)) dsp_bp->init_ec_array ();

		dsp_bp->grid_add_ec (NULL,      Unity, &GVP_points[k], 0, 4000,  "5g", "gvp-n", k); 
                if (k == (N_GVP_VECTORS-1)) dsp_bp->init_ec_array ();


		//note:  VP_FEEDBACK_HOLD is only the mask, this bit in GVP_opt is set to ONE for FEEBACK=ON !! Ot is inveretd while vector generation ONLY.
		dsp_bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (callback_change_GVP_vpc_option_flags), this,
                                               GVP_opt[k], VP_FEEDBACK_HOLD);
                EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, dsp_bp->button);
                g_object_set_data (G_OBJECT (dsp_bp->button), "VPC", GINT_TO_POINTER (k));

                dsp_bp->set_configure_list_mode_on (); // ================ advanced section

                dsp_bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (callback_change_GVP_vpc_option_flags), this,
                                               GVP_opt[k], VP_GPIO_SET);
                EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, dsp_bp->button);
                g_object_set_data (G_OBJECT (dsp_bp->button), "VPC", GINT_TO_POINTER (k));

                dsp_bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (callback_change_GVP_vpc_option_flags), this,
                                               GVP_opt[k], VP_GPIO_READ);
                EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, dsp_bp->button);
                g_object_set_data (G_OBJECT (dsp_bp->button), "VPC", GINT_TO_POINTER (k));

                dsp_bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (callback_change_GVP_vpc_option_flags), this,
                                               GVP_opt[k], VP_TRIGGER_P);
                EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, dsp_bp->button);
                g_object_set_data (G_OBJECT (dsp_bp->button), "VPC", GINT_TO_POINTER (k));

                dsp_bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (callback_change_GVP_vpc_option_flags), this,
                                               GVP_opt[k], VP_TRIGGER_N);
                EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, dsp_bp->button);
                g_object_set_data (G_OBJECT (dsp_bp->button), "VPC", GINT_TO_POINTER (k));

		dsp_bp->grid_add_ec (NULL, Hex,   &GVP_data[k],   0, 0xffff,  "04X", "gvp-data", k); 
                if (k == (N_GVP_VECTORS-1)) dsp_bp->init_ec_array ();

		dsp_bp->grid_add_ec (NULL, Unity, &GVP_vnrep[k], 0., 32000.,  ".0f", "gvp-nrep", k); 
                if (k == (N_GVP_VECTORS-1)) dsp_bp->init_ec_array ();

		dsp_bp->grid_add_ec (NULL, Unity, &GVP_vpcjr[k], -50.,   0.,  ".0f", "gvp-pcjr", k); 
                if (k == (N_GVP_VECTORS-1)) dsp_bp->init_ec_array ();

                dsp_bp->set_configure_list_mode_off (); // ==================================

                GtkWidget *button;
		if (k > 0){
                        dsp_bp->grid_add_widget (button=gtk_button_new_from_icon_name ("arrow-up-symbolic"));
                        gtk_widget_set_tooltip_text (button, "Shift VP up here");
                        g_object_set_data (G_OBJECT(button), "VPC", GINT_TO_POINTER (k));
                        g_object_set_data (G_OBJECT(button), "ARROW", GINT_TO_POINTER (GVP_SHIFT_UP));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (callback_edit_GVP),
                                           this);
		} else {
                        dsp_bp->grid_add_widget (button=gtk_button_new_from_icon_name ("view-more-symbolic"));
                        gtk_widget_set_tooltip_text (button, "Toggle VP Flow Chart");
                        g_object_set_data (G_OBJECT(button), "VPC", GINT_TO_POINTER (k));
                        g_object_set_data (G_OBJECT(button), "ARROW", GINT_TO_POINTER (GVP_SHIFT_UP));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (callback_edit_GVP),
                                           this);
                }
		if (k < N_GVP_VECTORS-1){
                        dsp_bp->grid_add_widget (button=gtk_button_new_from_icon_name ("arrow-down-symbolic"));
                        gtk_widget_set_tooltip_text (button, "Shift VP down here");
                        g_object_set_data (G_OBJECT(button), "VPC", GINT_TO_POINTER (k));
                        g_object_set_data (G_OBJECT(button), "ARROW", GINT_TO_POINTER (GVP_SHIFT_DN));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (callback_edit_GVP),
                                           this);
		}
		g_free (tmpl);
		if (k==0)
			g_object_set_data (G_OBJECT (dsp_bp->grid), "CF", GINT_TO_POINTER (dsp_bp->x));

                dsp_bp->new_line ();
                
	}
	g_object_set_data( G_OBJECT (window), "DSP_VPC_OPTIONS_list", EC_vpc_opt_list);

        dsp_bp->set_no_spin (false);

        // lower part: controls
        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->new_grid_with_frame ("VP Memos");

	// MEMO BUTTONs
        gtk_widget_set_hexpand (dsp_bp->grid, TRUE);
	const gchar *keys[] = { "VPA", "VPB", "VPC", "VPD", "VPE", "VPF", "VPG", "VPH", "VPI", "VPJ", "V0", NULL };

	for (int i=0; keys[i]; ++i) {
                GdkRGBA rgba;
		gchar *gckey  = g_strdup_printf ("LM_set_%s", keys[i]);
		gchar *stolab = g_strdup_printf ("STO %s", keys[i]);
		gchar *rcllab = g_strdup_printf ("RCL %s", keys[i]);
		gchar *memolab = g_strdup_printf ("M %s", keys[i]);             
		gchar *memoid  = g_strdup_printf ("memo-vp%c", 'a'+i);             
                remote_action_cb *ra = NULL;
                gchar *help = NULL;

                dsp_bp->set_xy (i+1, 10);
                // add button with remote support for program recall
                ra = g_new( remote_action_cb, 1);
                ra -> cmd = g_strdup_printf("DSP_VP_STO_%s", keys[i]);
                help = g_strconcat ("Remote example: action (", ra->cmd, ")", NULL); 
                dsp_bp->grid_add_button (N_(stolab), help, 1,
                                         G_CALLBACK (callback_GVP_store_vp), this,
                                         "key", gckey);
                g_free (help);
                ra -> RemoteCb = (void (*)(GtkWidget*, void*))callback_GVP_store_vp;
                ra -> widget = dsp_bp->button;
                ra -> data = this;
                main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
                PI_DEBUG (DBG_L2, "Adding new Remote Cmd: " << ra->cmd ); 

                // CSS
                //                if (gdk_rgba_parse (&rgba, "tomato"))
                //                        gtk_widget_override_background_color ( GTK_WIDGET (dsp_bp->button), GTK_STATE_FLAG_PRELIGHT, &rgba);

                dsp_bp->set_xy (i+1, 11);

                // add button with remote support for program recall
                ra = g_new( remote_action_cb, 1);
                ra -> cmd = g_strdup_printf("DSP_VP_RCL_%s", keys[i]);
                help = g_strconcat ("Remote example: action (", ra->cmd, ")", NULL); 
                dsp_bp->grid_add_button (N_(rcllab), help, 1,
                                         G_CALLBACK (callback_GVP_restore_vp), this,
                                         "key", gckey);
                g_free (help);
                ra -> RemoteCb = (void (*)(GtkWidget*, void*))callback_GVP_restore_vp;
                ra -> widget = dsp_bp->button;
                ra -> data = this;
                main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
                PI_DEBUG (DBG_L2, "Adding new Remote Cmd: " << ra->cmd ); 
                
                // CSS
                //                if (gdk_rgba_parse (&rgba, "SeaGreen3"))
                //                        gtk_widget_override_background_color ( GTK_WIDGET (dsp_bp->button), GTK_STATE_FLAG_PRELIGHT, &rgba);
#if 0 // may adda memo/info button
                dsp_bp->set_xy (i+1, 12);
                dsp_bp->grid_add_button (N_(memolab), memolab, 1,
                                         G_CALLBACK (callback_GVP_memo_vp), this,
                                         "key", gckey);
#endif
                dsp_bp->set_xy (i+1, 12);
                dsp_bp->grid_add_input (NULL);
                dsp_bp->set_input_width_chars (10);
                gtk_widget_set_hexpand (dsp_bp->input, TRUE);

                g_settings_bind (hwi_settings, memoid,
                                 G_OBJECT (dsp_bp->input), "text",
                                 G_SETTINGS_BIND_DEFAULT);

                //g_free (gckey);
                //g_free (stolab);
                //g_free (rcllab);
                g_free (memolab);
                g_free (memoid);
	}

        // ===================== done with panned
        
        dsp_bp->pop_grid ();
        dsp_bp->new_line ();

	dsp_bp->new_grid_with_frame ("VP Finish Settings and Status");
        dsp_bp->set_configure_list_mode_on ();
	dsp_bp->grid_add_ec ("GVP-Final-Delay", Time, &GVP_final_delay, 0., 1., "5.3g", 0.001, 0.01, "GVP-Final-Delay");
	dsp_bp->grid_add_label ("GPIO key", "Key code to enable GPIO set operations.\n(GPIO manipulation is locked out if not set right.)");
	dsp_bp->grid_add_ec (GVP_GPIO_KEYCODE_S, Unity, &GVP_GPIO_lock, 0, 9999, "04.0f", "GVP-GPIO-Lock-" GVP_GPIO_KEYCODE_S);
        dsp_bp->new_line ();
        dsp_bp->set_configure_list_mode_off ();

	GVP_status = dsp_bp->grid_add_probe_status ("Status");

        dsp_bp->pop_grid ();
        dsp_bp->new_line ();

        dsp_bp->grid_add_probe_controls (FALSE,
                                         GVP_option_flags, GCallback (callback_change_GVP_option_flags),
                                         GVP_auto_flags,  GCallback (callback_change_GVP_auto_flags),
                                         GCallback (DSPControl::Probing_exec_GVP_callback),
                                         GCallback (DSPControl::Probing_write_GVP_callback),
                                         GCallback (DSPControl::Probing_graph_callback),
                                         GCallback (DSPControl::Probing_abort_callback),
                                         this,
                                         "GVP");
        dsp_bp->notebook_tab_show_all ();
        dsp_bp->pop_grid ();

        // ==== Folder: Track (tracking mode) setup ========================================
        dsp_bp->new_grid ();
        dsp_bp->start_notebook_tab ("TK", "tab-tk", NOTEBOOK_TAB_TK, hwi_settings);

	// ========================================
 	dsp_bp->new_grid_with_frame ("Tracking");

	// stuff here ...
	dsp_bp->grid_add_ec ("Radius", Angstroem, &TK_r, 0.0, 1000.0, "6.4g", 1., 10., "TK-rad");
	dsp_bp->grid_add_ec ("Radius far", Angstroem, &TK_r2, 0.0, 1000.0, "6.4g", 1., 10., "TK-rad2");
        //      dsp_bp->new_line ();
        //	dsp_bp->grid_add_label ("");
        //	dsp_bp->grid_add_ec ("dy", Angstroem, OUT_OF_RANGE, &TK_dy, -1000.0, 1000.0, "6.4g", 1., 10.,  RemoteEntryList);
        //	dsp_bp->grid_add_ec ("dz", Angstroem, OUT_OF_RANGE, &TK_dz, -1000.0, 1000.0, "6.4g", 1., 10.,  RemoteEntryList);
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Points", Unity, &TK_points, 2, 100, "5g", "TK-Points");
	dsp_bp->grid_add_ec ("Nodes", Unity, &TK_nodes, 3, 24, "5g", "TK-Nodes");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Repetitions", Unity, &TK_repetitions, 1, 32000, "5g", "TK-Reps");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Up/Down hill +/-", Unity, &TK_mode, -1, 1, "5g", "TK-Mode");
        //       dsp_bp->new_line ();
        //	dsp_bp->grid_add_ec ("TK-Ref", Unity, OUT_OF_RANGE, &TK_ref, 0, 3, "1g", 1., 1.,  RemoteEntryList);

	if (sranger_common_hwi->check_pac() != -1) {
		dsp_bp->grid_add_label ("Tracker on Signal:");
		dsp_bp->grid_add_probe_source_signal_options (6, probe_source[6], this);
	} else {            
		GtkWidget* TK_ref_option_menu = gtk_combo_box_text_new ();
                dsp_bp->grid_add_widget (TK_ref_option_menu, 2);
		g_signal_connect (G_OBJECT (TK_ref_option_menu), "changed",
                                  G_CALLBACK (DSPControl::callback_change_TK_ref),
                                  this);
		
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (TK_ref_option_menu), "0", N_("0: Ref on Z/Topo-FeedBack (on), mode=-1 to follow uphill"));
		// g_object_set_data(G_OBJECT (menuitem), "TK_ref_id", GINT_TO_POINTER (0));
		
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (TK_ref_option_menu), "1", N_("1: Ref on I (Current, IN0)"));
		// g_object_set_data(G_OBJECT (menuitem), "TK_ref_id", GINT_TO_POINTER (1));
		
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (TK_ref_option_menu), "2", N_("2: Ref on I (dF, IN1)"));
		// g_object_set_data(G_OBJECT (menuitem), "TK_ref_id", GINT_TO_POINTER (2));
		
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (TK_ref_option_menu), "3", N_("3: Ref on I (Aux2, IN2)"));
		// g_object_set_data(G_OBJECT (menuitem), "TK_ref_id", GINT_TO_POINTER (3));

		gtk_combo_box_set_active (GTK_COMBO_BOX (TK_ref_option_menu), TK_ref);
	}

        dsp_bp->new_line ();
        dsp_bp->grid_add_ec ("Speed", Speed, &TK_speed, 0.1,10000.0, "5.4g", 1., 10., "TK-Speed");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Delay", msTime, &TK_delay, 0., 10000., "5.2g", 0.001, 0.01, "TK-Delay");
        dsp_bp->new_line ();

	TK_status = dsp_bp->grid_add_probe_status ("Status");
        dsp_bp->new_line ();

        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->grid_add_probe_controls (FALSE,
                                         TK_option_flags, GCallback (callback_change_TK_option_flags),
                                         TK_auto_flags,  GCallback (callback_change_TK_auto_flags),
                                         GCallback (DSPControl::Probing_exec_TK_callback),
                                         GCallback (DSPControl::Probing_write_TK_callback),
                                         GCallback (DSPControl::Probing_graph_callback),
                                         GCallback (DSPControl::Probing_abort_callback),
                                         this,
                                         "TK");
        dsp_bp->notebook_tab_show_all ();
        dsp_bp->pop_grid ();


        // ==== Folder: LockIn  ========================================
        dsp_bp->new_grid ();
        dsp_bp->start_notebook_tab ("LockIn", "tab-lockin", NOTEBOOK_TAB_LOCKIN, hwi_settings);

	// ======== NOISE MODULE control in here =============
        PI_DEBUG (DBG_L4, "DSPC----TAB-LOCKIN -------------------------------");
	if (DSPPACClass) {
                dsp_bp->set_configure_list_mode_on ();
                dsp_bp->new_grid_with_frame ("Noise Generator settings");
		dsp_bp->grid_add_ec ("Noise-Amplitude", Volt, &noise_amp, 0., 1., "5g", 0.001, 0.01, "Noise-Amplitude");
                dsp_bp->set_ec_change_notice_fkt (DSPControl::lockin_adjust_callback, this);
                // dsp_bp->grid_add_ec ("Noise-Amplitude", Volt, OUT_OF_RANGE, &noise_amp, 0., 1., "5g", 0.001, 0.01);
                dsp_bp->pop_grid ();
                dsp_bp->new_line ();
                dsp_bp->set_configure_list_mode_off ();
	}

	// ======== LOCKIN A/B MODULE

 	dsp_bp->new_grid_with_frame ("Digital 2-Channel Lock-In settings");
        dsp_bp->set_default_ec_change_notice_fkt (DSPControl::lockin_adjust_callback, this);
        
	if (DSPPACClass) {
                PI_DEBUG (DBG_L4, "DSPC----TAB-LOCKIN ------------------------------- MK3 **");
                dsp_bp->new_line (0, 2);
                dsp_bp->grid_add_label ("CORRPRD-SHR");
                dsp_bp->grid_add_label ("CORRSUM-SHR");
	}

        dsp_bp->new_line ();
        LockIn_mode = dsp_bp->grid_add_check_button ("LockIn run free", "enable contineous modulation and LockIn processing.\n",
                                                     1,
                                                     GCallback (DSPControl::lockin_runfree_callback), this,
                                                     sranger_common_hwi->dsp_lockin_state(-1), 0);
	if (DSPPACClass) {
                dsp_bp->set_configure_list_mode_on ();
		dsp_bp->grid_add_ec ("CORRPRD-SHR", Unity, &AC_amp[2], 0., 32., "5g", 1., 1., "LCK-CORRPRD-SHR");
        	dsp_bp->grid_add_ec ("CORRSUM-SHR", Unity, &AC_amp[3], -32., 32., "5g", 1., 1., "LCK-CORRSUM-SHR");
                dsp_bp->set_configure_list_mode_off ();
	}

        dsp_bp->new_line (0, 2);
        dsp_bp->grid_add_label ("Bias Modulation Amp");
        dsp_bp->grid_add_label ("");
        dsp_bp->grid_add_label ("Z Modulation Amp");
        dsp_bp->grid_add_label ("");
        dsp_bp->grid_add_label ("Modulation Frequency");
        dsp_bp->new_line ();

	dsp_bp->grid_add_ec ("Bias Amp", Volt, &AC_amp[0], 0., 1., "5g", 0.001, 0.01, "LCK-AC-Bias-Amp");
	if (DSPPACClass) {
                PI_DEBUG (DBG_L4, "DSPC----TAB-LOCKIN ------------------------------- MK3 ***");
                dsp_bp->grid_add_ec ("Z Amp", Angstroem, &AC_amp[1], 0., 100., "5g", 0.01, 0.1, "LCK-AC-Z-Amp");
	}

	dsp_bp->grid_add_ec ("Frequency", Frq, &AC_frq, 100., 10000., "5g", 500., 500., "LCK-AC-Frequency");
        AC_frq_ec = dsp_bp->ec;
        dsp_bp->new_line (0, 2);
        dsp_bp->grid_add_label ("Source Signal");
        dsp_bp->grid_add_label ("");
        dsp_bp->grid_add_label ("Phase");
        dsp_bp->new_line ();
	if (DSPPACClass) {
                PI_DEBUG (DBG_L4, "DSPC----TAB-LOCKIN ------------------------------- MK3 ****");
                dsp_bp->new_line ();
		dsp_bp->grid_add_label ("LockIn-A");
		dsp_bp->grid_add_lockin_input_signal_options (0, lockin_input[0], this);
		dsp_bp->grid_add_ec ("Phase-A", Deg, &AC_phaseA, -360., 360., "5g", 1., 10., "LCK-AC-Phase-A");
                dsp_bp->new_line ();
		dsp_bp->grid_add_label ("LockIn-B");
		dsp_bp->grid_add_lockin_input_signal_options (1, lockin_input[1], this);
		dsp_bp->grid_add_ec ("Phase-B", Deg, &AC_phaseB, -360., 360., "5g", 1., 10., "LCK-AC-Phase-B");
	} else {
                dsp_bp->new_line ();
		dsp_bp->grid_add_ec ("Phase-A", Deg, &AC_phaseA, -360., 360., "5g", 1., 10., "LCK-AC-Phase-A");
                dsp_bp->ec->set_info (" :A");
                dsp_bp->new_line ();
		dsp_bp->grid_add_ec ("Phase-B", Deg, &AC_phaseB, -360., 360., "5g", 1., 10., "LCK-AC-Phase-B");
                dsp_bp->ec->set_info (" :B");
	}

        PI_DEBUG (DBG_L4, "DSPC----TAB-LOCKIN ------------------------------- MK3 * done.");
        dsp_bp->set_configure_list_mode_on ();
        dsp_bp->new_line (0, 2);
        dsp_bp->grid_add_label ("Cycles");
        dsp_bp->grid_add_label ("");
        dsp_bp->grid_add_label ("Sweep Span");
        dsp_bp->grid_add_label ("");
        dsp_bp->grid_add_label ("Sweep Points");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Avg Cycles", Unity, &AC_lockin_avg_cycels, 1, 128, "5g", "LCK-AC-avg-Cycles");
	dsp_bp->grid_add_ec ("Phase Span", Deg, &AC_phase_span, -360., 360., "5g", 1., 10., "ALCK-C-Phase-Span");
        dsp_bp->ec->set_info (" :span");
	dsp_bp->grid_add_ec ("Points", Unity, &AC_points, 1, 1440, "5g", "LCK-AC-Points");
        dsp_bp->ec->set_info (" #pts");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Slope", PhiSpeed, &AC_phase_slope, 0.01,120.0, "5.4g", 1., 10., "LCK-AC-Slope");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Final Delay", Time, &AC_final_delay, 0., 1., "5.3g", 0.001, 0.01, "LCK-AC-Final-Delay");
	dsp_bp->grid_add_ec ("Repetitions", Unity, &AC_repetitions, 1, 100, ".0f", "LCK-AC-Repetitions");
        dsp_bp->ec->set_info (" #reps");

        dsp_bp->new_line ();
        PI_DEBUG (DBG_L4, "DSPC----TAB-LOCKIN ------------------------------- Status+Controls.");

	AC_status = dsp_bp->grid_add_probe_status ("Status");
        dsp_bp->new_line ();

        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->grid_add_probe_controls (FALSE,
                                         AC_option_flags, GCallback (callback_change_AC_option_flags),
                                         AC_auto_flags,  GCallback (callback_change_AC_auto_flags),
                                         GCallback (DSPControl::LockIn_exec_callback),
                                         GCallback (DSPControl::LockIn_write_callback),
                                         GCallback (DSPControl::Probing_graph_callback),
                                         GCallback (DSPControl::Probing_abort_callback),
                                         this,
                                         "AC");
        dsp_bp->set_configure_list_mode_off ();
        
        dsp_bp->notebook_tab_show_all ();
        dsp_bp->pop_grid ();

        // revert back to normal CNF callback
        dsp_bp->set_default_ec_change_notice_fkt (DSPControl::ChangedNotify, this);


// ==== Folder: AX Auxillary Channel (QMA, CMA, ...)  ========================================
        dsp_bp->new_grid ();
        dsp_bp->start_notebook_tab ("AX", "tab-ax", NOTEBOOK_TAB_AX, hwi_settings);

	// ========================================
 	dsp_bp->new_grid_with_frame ("Auxillary Spectroscopy (Experimental/User)");

	// stuff here ...
	dsp_bp->grid_add_ec ("Start", Volt, &AX_start, -10.0, 10., "6g", 0.1, 0.025, "AX-V-Start");
	dsp_bp->grid_add_ec ("End", Volt, &AX_end, -10.0, 10.0, "6g", 0.1, 0.025, "AX-V-End");
	dsp_bp->grid_add_ec ("Points", Unity, &AX_points, 1, 1000, "5g", "AX-Points");
	dsp_bp->grid_add_ec ("# probes", Unity, &AX_repetitions, 1, 100, "5g", "AX-rep");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Slope", Vslope, &AX_slope,0.1,1000.0, "6g", 1., 10., "AX-V-Slope");
	dsp_bp->grid_add_ec ("Slope Ramp", Vslope, &AX_slope_ramp,0.1,1000.0, "6g", 1., 10., "AX-Slope-Ramp");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Final Delay", Time, &AX_final_delay, 0., 1., "6g", 0.001, 0.01, "AX-Final-Delay");
        dsp_bp->new_line ();
	dsp_bp->grid_add_ec ("Gate Time", Time, &AX_gatetime, 0.1e-6, 355., "8g", 0.01, 0.1, "AX-GateTime");
        dsp_bp->new_line ();

        //	dsp_bp->grid_add_ec ("Gain", Time, &AX_gain, 0., 10., "5.3g", 0.001, 0.01, "AX-Gain");
        //	dsp_bp->grid_add_ec ("Resolution", Time, &AX_resolution, 0., 10., "5.3g", 0.001, 0.01, "AX-Resolution");

        dsp_bp->new_line ();
	AX_status = dsp_bp->grid_add_probe_status ("Status");
        dsp_bp->new_line ();

        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->grid_add_probe_controls (TRUE,
                                         AX_option_flags, GCallback (callback_change_AX_option_flags),
                                         AX_auto_flags,  GCallback (callback_change_AX_auto_flags),
                                         GCallback (DSPControl::Probing_exec_AX_callback),
                                         GCallback (DSPControl::Probing_write_AX_callback),
                                         GCallback (DSPControl::Probing_graph_callback),
                                         GCallback (DSPControl::Probing_abort_callback),
                                         this,
                                         "AX");
        dsp_bp->notebook_tab_show_all ();
        dsp_bp->pop_grid ();


        
// ==== Folder: ABORT mode  ========================================
        dsp_bp->new_grid ();
        dsp_bp->start_notebook_tab ("X", "tab-abort", NOTEBOOK_TAB_ABORT, hwi_settings);

 	dsp_bp->new_grid_with_frame ("Kill/Abort Vector Program in progress");

	dsp_bp->grid_add_ec ("Final Delay", Time, &ABORT_final_delay, 0., 1., "6g", 0.001, 0.01, "X-Final-Delay");

        dsp_bp->new_line ();
	ABORT_status = dsp_bp->grid_add_probe_status ("Status");
        dsp_bp->new_line ();

        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->grid_add_probe_controls (FALSE,
                                         ABORT_option_flags, GCallback (callback_change_ABORT_option_flags),
                                         ABORT_auto_flags,  GCallback (callback_change_ABORT_auto_flags),
                                         GCallback (DSPControl::Probing_exec_ABORT_callback),
                                         GCallback (DSPControl::Probing_write_ABORT_callback),
                                         GCallback (DSPControl::Probing_graph_callback),
                                         GCallback (DSPControl::Probing_abort_callback),
                                         this,
                                         "ABORT");
        dsp_bp->notebook_tab_show_all ();
        dsp_bp->pop_grid ();

// ==== Folder: Graphs -- Vector Probe Data Visualisation setup ========================================
        dsp_bp->new_grid ();
        dsp_bp->start_notebook_tab ("Graphs", "tab-graphs", NOTEBOOK_TAB_GRAPHS, hwi_settings);

        PI_DEBUG (DBG_L4, "DSPC----TAB-GRAPHS ------------------------------- ");

 	dsp_bp->new_grid_with_frame ("Probe Sources & Graph Setup");

	// source channel setup
	int   msklookup[] = { 0x000010, 0x000020, 0x000040, 0x000080, 0x000100, 0x000200, 0x000400, 0x000800,
			      0x000001, 0x000002, 
			      0x000008, 0x001000, 0x002000, 0x004000, 0x008000, 0x000004,
			      0x0100000, 0x0200000, 0x0400000, 0x0800000, 0x1000000, 0x2000000, 0x4000000,
			      -1 
	};
	const char* lablookup[] = { "ADC0-I", "ADC1-SP", "ADC2-Mx2", "ADC3-Mx3", "ADC4", "ADC5","ADC6","ADC7",
				    "Zmon", "Umon",
				    "LockIn0", "LockIn A-1st",  "LockIn B-1st", "LockIn A-2nd", "LockIn B-2nd", "Counter", // last 4 in this line are "signals" for MK3 => i=12..16
				    "Time", "XS", "YS", "ZS", "U", "PHI", "SEC",
				    NULL
	};
        
        dsp_bp->grid_add_label ("Source", "Check column to activate channel", 2, 0.);
        dsp_bp->grid_add_label ("X", "Check column to plot channel on X axis.", 1);
        dsp_bp->grid_add_label ("Y", "Check column to plot channel on Y axis.", 1);
        dsp_bp->grid_add_label ("Avg", "Check column to plot average of all spectra", 1);
        dsp_bp->grid_add_label ("Sec", "Check column to show all sections.", 1);
        GtkWidget *sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
        gtk_widget_set_size_request (sep, 5, -1);
        dsp_bp->grid_add_widget (sep, 5);
        //dsp_bp->grid_add_label (" --- ", NULL, 5);

        dsp_bp->grid_add_label ("Source", "Check column to activate channel", 2, 0.);
        dsp_bp->grid_add_label ("X", "Check column to plot channel on X axis.", 1);
        dsp_bp->grid_add_label ("Y", "Check column to plot channel on Y axis.", 1);
        dsp_bp->grid_add_label ("Avg", "Check column to plot average of all spectra", 1);
        dsp_bp->grid_add_label ("Sec", "Check column to show all sections.", 1);
        sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
        gtk_widget_set_size_request (sep, 5, -1);
        dsp_bp->grid_add_widget (sep, 5);
        //dsp_bp->grid_add_label (" --- ", NULL, 5);

        dsp_bp->grid_add_label ("Source", "Check column to activate channel", 2, 0.);
        dsp_bp->grid_add_label ("X", "Check column to plot channel on X axis.", 1);
        dsp_bp->grid_add_label ("Y", "Check column to plot channel on Y axis.", 1);
        dsp_bp->grid_add_label ("Avg", "Check column to plot average of all spectra", 1);
        dsp_bp->grid_add_label ("Sec", "Check column to show all sections.", 1);

        dsp_bp->new_line ();

        PI_DEBUG (DBG_L4, "DSPC----TAB-GRAPHS TOGGELS  ------------------------------- ");

        gint y = dsp_bp->y;
	for (i=0; msklookup[i] >= 0 && lablookup[i]; ++i) {
                PI_DEBUG (DBG_L4, "GRAPHS*** i=" << i << " " << lablookup[i]);
		if (!msklookup[i]) 
			continue;
		int c=i/8; 
		c*=11;
                c++;
		int m = -1;
		if (i >= 12 && i < 16)
			m = i-12;
		else
			m = -1;
                int r = y+i%8+1;

                dsp_bp->set_xy (c, r);

                // Source
                dsp_bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (change_source_callback), this,
                                               Source, (((int) msklookup[i]) & 0xfffffff)
                                               );
                // MK3: Signal Sources i=12..16:
                if (m >= 0 && sranger_common_hwi->check_pac() != -1){
                        dsp_bp->grid_add_probe_source_signal_options (m, probe_source[m], this);
                } else { // MK2: fixed assignment
                        dsp_bp->grid_add_label (lablookup[i], NULL, 1, 0.);
                }
                g_object_set_data (G_OBJECT(dsp_bp->button), "Source_Channel", GINT_TO_POINTER ((int) msklookup[i])); 
                g_object_set_data (G_OBJECT(dsp_bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // use as X-Source
                dsp_bp->grid_add_check_button ("", NULL, 1,
                                               GCallback (change_source_callback), this,
                                               XSource, (((int) (X_SOURCE_MSK | msklookup[i])) & 0xfffffff)
                                               );
                g_object_set_data (G_OBJECT(dsp_bp->button), "Source_Channel", GINT_TO_POINTER ((int) (X_SOURCE_MSK | msklookup[i]))); 
                g_object_set_data (G_OBJECT(dsp_bp->button), "VPC", GINT_TO_POINTER (i)); 

                // use as Plot (Y)-Source
                dsp_bp->grid_add_check_button ("", NULL, 1,
                                               G_CALLBACK (change_source_callback), this,
                                               PSource, (((int) (P_SOURCE_MSK | msklookup[i])) & 0xfffffff)
                                               );
                g_object_set_data (G_OBJECT(dsp_bp->button), "Source_Channel", GINT_TO_POINTER ((int) (P_SOURCE_MSK | msklookup[i]))); 
                g_object_set_data (G_OBJECT(dsp_bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // use as A-Source (Average)
                dsp_bp->grid_add_check_button ("", NULL, 1,
                                               G_CALLBACK (change_source_callback), this,
                                               PlotAvg, (((int) (A_SOURCE_MSK | msklookup[i])) & 0xfffffff)
                                               );
                g_object_set_data (G_OBJECT(dsp_bp->button), "Source_Channel", GINT_TO_POINTER ((int) (A_SOURCE_MSK | msklookup[i]))); 
                g_object_set_data (G_OBJECT(dsp_bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // use as S-Source (Section)
                dsp_bp->grid_add_check_button ("", NULL, 1,
                                               G_CALLBACK (change_source_callback), this,
                                               PlotSec, (((int) (S_SOURCE_MSK | msklookup[i])) & 0xfffffff)
                                               );
                g_object_set_data (G_OBJECT(dsp_bp->button), "Source_Channel", GINT_TO_POINTER ((int) (S_SOURCE_MSK | msklookup[i]))); 
                g_object_set_data (G_OBJECT(dsp_bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // dsp_bp->grid_add_check_button_graph_matrix(lablookup[i], (int) msklookup[i], m, probe_source[m], i, this);
                // dsp_bp->grid_add_check_button_graph_matrix(" ", (int) (X_SOURCE_MSK | msklookup[i]), -1, i, this);
                // dsp_bp->grid_add_check_button_graph_matrix(" ", (int) (P_SOURCE_MSK | msklookup[i]), -1, i, this);
                // dsp_bp->grid_add_check_button_graph_matrix(" ", (int) (A_SOURCE_MSK | msklookup[i]), -1, i, this);
                // dsp_bp->grid_add_check_button_graph_matrix(" ", (int) (S_SOURCE_MSK | msklookup[i]), -1, i, this);
                if (c < 23){
                        sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
                        gtk_widget_set_size_request (sep, 5, -1);
                        dsp_bp->grid_add_widget (sep);
                }
	}

        
        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->new_grid_with_frame ("For all Modes: Limiter Signal, Trigger Signal and Plot Mode Configuration");

	int kk=4;
	if (sranger_common_hwi->check_pac() != -1){
		kk = 1;
		dsp_bp->grid_add_label ("Limiter on");
		dsp_bp->grid_add_probe_source_signal_options (5, probe_source[5], this);

                g_signal_connect (G_OBJECT (wid), "changed",
                                  G_CALLBACK (DSPControl::callback_change_FZ_ref),
                                  this);
	} else {
		dsp_bp->grid_add_label ("Limiter");
	}

	GtkWidget* FZ_ref_option_menu = gtk_combo_box_text_new ();
	dsp_bp->grid_add_widget (FZ_ref_option_menu);
                
        for (i = -kk; i <= kk; i++) {
                gchar *tmp = i==0 ? g_strdup("OFF") : kk == 1? g_strdup (i>0? "<":">") : g_strdup_printf ("%d: Lim IN%d %s", i, abs(i)-1, i>0? ">":"<");
                gchar *id = g_strdup_printf ("%d", 4+i);
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (FZ_ref_option_menu), id, N_(tmp));
                g_free (id);
                g_free (tmp);
        }
        //gtk_combo_box_set_active (GTK_COMBO_BOX (FZ_ref_option_menu), 4+FZ_limiter_ch);
        gtk_combo_box_set_active (GTK_COMBO_BOX (FZ_ref_option_menu), 1); // OFF
        dsp_bp->new_line ();

	dsp_bp->grid_add_ec ("Lim Val Up", Volt, &FZ_limiter_val[0], -10.0, 10.0, "5.3g", 0.1, 1.0, "VP-Lim-Val-Up");
	dsp_bp->grid_add_ec ("Lim Val Dn", Volt, &FZ_limiter_val[1], -10.0, 10.0, "5.3g", 0.1, 1.0, "VP-Lim-Val-Dn");

        dsp_bp->new_line ();

	if (sranger_common_hwi->check_pac() != -1){
		dsp_bp->grid_add_label ("Trigger on");
		dsp_bp->grid_add_probe_source_signal_options (4, probe_source[4], this);
	}

	dsp_bp->grid_add_check_button ("Join all graphs for same X", "Join all plots with same X.\n"
                                       "Note: if auto scale (default) Y-scale\n"
                                       "will only apply to 1st graph - use hold/exp. only for asolute scale.)", 1,
                                       GCallback (callback_XJoin), this,
                                       XJoin, 1
                                       );
	dsp_bp->grid_add_check_button ("Use single window", "Place all probe graphs in single window.",
                                       1,
                                       GCallback (callback_GrMatWindow), this,
                                       GrMatWin, 1
                                       );
        dsp_bp->pop_grid ();
        dsp_bp->new_line ();
        dsp_bp->new_grid_with_frame ("Plot / Save current data in buffer");

	dsp_bp->grid_add_button ("Plot");
	g_signal_connect (G_OBJECT (dsp_bp->button), "clicked",
                          G_CALLBACK (DSPControl::Probing_graph_callback), this);
	
        save_button = dsp_bp->grid_add_button ("Save");
	g_signal_connect (G_OBJECT (dsp_bp->button), "clicked",
                          G_CALLBACK (DSPControl::Probing_save_callback), this);

        dsp_bp->notebook_tab_show_all ();
        dsp_bp->pop_grid ();

        
// ==== Folder List: Vector Probes ========================================
// Automatic frame generation

// ==== finish auto_probe_menu now
	gtk_combo_box_set_active (GTK_COMBO_BOX (auto_probe_menu), 0);

// ------------------------------------------------------------
	sranger_mk2_hwi_pi.app->RemoteEntryList = g_slist_concat (sranger_mk2_hwi_pi.app->RemoteEntryList, dsp_bp->get_remote_list_head ());

	// save List away...
	g_object_set_data( G_OBJECT (window), "DSP_EC_list", dsp_bp->get_ec_list_head ());
        g_object_set_data( G_OBJECT (multiIV_checkbutton), "DSP_multiIV_list", multi_IVsec_list);
        g_object_set_data( G_OBJECT (multiBias_checkbutton), "DSP_multiBias_list", multi_Bias_list);
	g_object_set_data( G_OBJECT (zposmon_checkbutton), "DSP_zpos_control_list", zpos_control_list);

        DSP_multiIV_callback (multiIV_checkbutton, this);
        DSP_multiBias_callback (multiBias_checkbutton, this);

	GUI_ready = TRUE;

	AppWindowInit (NULL); // call two, setup header bar menu, confiugure mode needs to operate on default show/hide
        configure_callback (NULL, NULL, this);
        
	set_window_geometry ("dsp-control-0");
}



static void remove(gpointer entry, gpointer from) {
	from = (gpointer) g_slist_remove ((GSList*)from, entry);
}

DSPControl::~DSPControl (){
        // g_message ("DSPControl::~DSPContro -- store values");
        // this does not function here any more ????!!!!
	// ### store_values ();
        // g_message ("DSPControl::~DSPContro -- store values done.");

	GUI_ready = FALSE;

        g_free (vp_exec_mode_name);
 
	// remove all left over pc's from pc matrix
	for (int i=0; i < 2*MAX_NUM_CHANNELS; ++i) 
		for (int j=0; j < 2*MAX_NUM_CHANNELS; ++j) 
			if (probe_pc_matrix[i][j]){
				delete probe_pc_matrix[i][j];
				probe_pc_matrix[i][j] = NULL; // just to be clean
			}

	g_slist_foreach(dsp_bp->get_remote_list_head (), (GFunc) remove, sranger_mk2_hwi_pi.app->RemoteEntryList);
	free_probedata_arrays ();

	delete Unity;
	delete Volt;
	delete Angstroem;
	delete Frq;
	delete Deg;
	delete Current;
	delete Current_pA;
	delete Speed;
	delete PhiSpeed;
	delete Vslope;
	delete Time;
	delete TimeUms;
	delete msTime;
	delete minTime;
	delete SetPtUnit;

        delete dsp_bp;

        // g_message ("DSPControl::~DSPContro -- unref data done.");
        
        g_clear_object (&hwi_settings);

        // g_message ("DSPControl::~DSPContro -- clear hwi_settings done.");
}

void DSPControl::store_values (){
        g_settings_set_int (hwi_settings, "probe-sources", Source);
        g_settings_set_int (hwi_settings, "probe-sources-x", XSource);
        g_settings_set_boolean (hwi_settings, "probe-x-join", XJoin);
        g_settings_set_boolean (hwi_settings, "probe-graph-matrix-window", GrMatWin);
        g_settings_set_int (hwi_settings, "probe-p-sources", PSource);
        g_settings_set_int (hwi_settings, "probe-pavg-sources", PlotAvg);
        g_settings_set_int (hwi_settings, "probe-psec-sources", PlotSec);

        set_tab_settings ("AC", AC_option_flags, AC_auto_flags, AC_glock_data);
        set_tab_settings ("IV", IV_option_flags, IV_auto_flags, IV_glock_data);
        set_tab_settings ("FZ", FZ_option_flags, FZ_auto_flags, FZ_glock_data);
        set_tab_settings ("PL", PL_option_flags, PL_auto_flags, PL_glock_data);
        set_tab_settings ("LP", LP_option_flags, LP_auto_flags, LP_glock_data);
        set_tab_settings ("SP", SP_option_flags, SP_auto_flags, SP_glock_data);
        set_tab_settings ("TS", TS_option_flags, TS_auto_flags, TS_glock_data);
        set_tab_settings ("GVP", GVP_option_flags, GVP_auto_flags, GVP_glock_data);
        set_tab_settings ("TK", TK_option_flags, TK_auto_flags, TK_glock_data);
        set_tab_settings ("AX", AX_option_flags, AX_auto_flags, AX_glock_data);
        set_tab_settings ("AB", ABORT_option_flags, ABORT_auto_flags, ABORT_glock_data);

        GVP_store_vp ("LM_set_last"); // last in view
        // g_message ("DSPControl::store_values complete.");
}

void DSPControl::GVP_store_vp (const gchar *key){
	// g_message ("GVP-VP store memo key=%s", key);
	PI_DEBUG_GP (DBG_L2, "GVP-VP store to memo %s\n", key);
	g_message ( "GVP-VP store to memo %s\n", key);
        GVariant *v = g_settings_get_value (hwi_settings, "probe-gvp-vector-program-matrix");
        //GVariant *v = g_settings_get_value (hwi_settings, "probe-lm-vector-program-matrix");
        GVariantDict *dict = g_variant_dict_new (v);
        if (!dict){
                PI_DEBUG_GP (DBG_L2, "ERROR: DSPControl::GVP_store_vp -- can't read dictionary 'probe-gvp-vector-program-matrix' a{sv}.\n");
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

                // g_print ("GVP store: %s = %s\n", m_vckey, g_variant_print (pc_array[i], true));

                if (g_variant_dict_contains (dict, m_vckey)){
                        if (!g_variant_dict_remove (dict, m_vckey)){
                                PI_DEBUG_GP (DBG_L2, "ERROR: DSPControl::GVP_store_vp -- key '%s' found, but removal failed.\n", m_vckey);
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

void DSPControl::GVP_restore_vp (const gchar *key){
	// g_message ("GVP-VP restore memo key=%s", key);
	PI_DEBUG_GP (DBG_L2, "GVP-VP restore to memo %s\n", key);
	g_message ( "GVP-VP restore to memo %s\n", key);
        GVariant *v = g_settings_get_value (hwi_settings, "probe-gvp-vector-program-matrix");
        //GVariant *v = g_settings_get_value (hwi_settings, "probe-lm-vector-program-matrix");
        GVariantDict *dict = g_variant_dict_new (v);
        if (!dict){
                PI_DEBUG_GP (DBG_L2, "ERROR: DSPControl::GVP_restore_vp -- can't read dictionary 'probe-gvp-vector-program-matrix' a{sv}.\n");
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
                for (int k=0; k<N_GVP_VECTORS; ++k) GVPi[i][k]=0; // zero init vector
                if ((vi[i] = g_variant_dict_lookup_value (dict, m_vckey, ((const GVariantType *) "ai"))) == NULL){
                        PI_DEBUG_GP (DBG_L2, "GXSM4 DCONF: DSPControl::GVP_restore_vp -- key_i '%s' memo not found. Setting to Zero.\n", m_vckey);
                        // g_warning ("GXSM4 DCONF: DSPControl::GVP_restore_vp -- key_i '%s' memo not found. Setting to Zero.\n", m_vckey);
                        g_free (m_vckey);
                        continue;
                }
                // g_print ("GVP restore: %s = %s\n", m_vckey, g_variant_print (vi[i], true));

                pc_array_i[i] = (gint32*) g_variant_get_fixed_array (vi[i], &n, sizeof (gint32));
                if (i==0) // actual length of this vector should fit all others -- verify
                        vp_program_length=n;
                else
                        if (n != vp_program_length)
                                g_warning ("GXSM4 DCONF: DSPControl::GVP_restore_vp -- key_i '%s' vector length %ld not matching program n=%d.\n", m_vckey, n, vp_program_length);
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
                        PI_DEBUG_GP (DBG_L2, "GXSM4 DCONF: DSPControl::GVP_restore_vp -- key_d '%s' memo not found. Setting to Zero.", m_vckey);
                        // g_warning ("GXSM4 DCONF: DSPControl::GVP_restore_vp -- key_d '%s' memo not found. Setting to Zero.", m_vckey);
                        g_free (m_vckey);
                        continue;
                }
                // g_print ("GVP restore: %s = %s\n", m_vckey, g_variant_print (vd[i], true));

                pc_array_d[i] = (double*) g_variant_get_fixed_array (vd[i], &n, sizeof (double));
                //g_assert_cmpint (n, ==, N_GVP_VECTORS);
                if (n != vp_program_length)
                        g_warning ("GXSM4 DCONF: DSPControl::GVP_restore_vp -- key_d '%s' vector length %ld not matching program n=%d.\n", m_vckey, n, vp_program_length);
                for (int k=0; k<vp_program_length && k<N_GVP_VECTORS; ++k)
                        GVPd[i][k]=pc_array_d[i][k];
                g_free (m_vckey);
                g_variant_unref (vd[i]);
        }                        
        
        g_variant_dict_unref (dict);
        g_variant_unref (v);

	update ();
}



// helper func
NcVar* sranger_mk2_hwi_ncaddvar (NcFile *ncf, const gchar *varname, const gchar *varunit, const gchar *longname, const gchar *shortname, double value){
	NcVar* ncv = ncf->add_var (varname, ncDouble);
	ncv->add_att ("long_name", longname);
	ncv->add_att ("short_name", shortname);
	ncv->add_att ("var_unit", varunit);
	ncv->put (&value);
	return ncv;
}
NcVar* sranger_mk2_hwi_ncaddvar (NcFile *ncf, const gchar *varname, const gchar *varunit, const gchar *longname, const gchar *shortname, int value){
	NcVar* ncv = ncf->add_var (varname, ncInt);
	ncv->add_att ("long_name", longname);
	ncv->add_att ("short_name", shortname);
	ncv->add_att ("var_unit", varunit);
	ncv->put (&value);
	return ncv;
}

#define MK2ID "sranger_mk2_hwi_"

void DSPControl::save_values (NcFile *ncf){
	NcVar *ncv;

	PI_DEBUG (DBG_L4, "DSPControl::save_values");
	gchar *i=NULL;

	if (IS_AFM_CTRL) // AFM (linear)
		i = g_strconcat ("SRanger HwI interface: AFM mode selected.\nHardware-Info:\n", sranger_common_hwi->get_info (), NULL);
	else
		i = g_strconcat ("SRanger HwI interface: STM mode selected.\nHardware-Info:\n", sranger_common_hwi->get_info (), NULL);

	NcDim* infod  = ncf->add_dim("sranger_info_dim", strlen(i));
	NcVar* info   = ncf->add_var("sranger_info", ncChar, infod);
	info->add_att("long_name", "SRanger HwI plugin information");
	info->put(i, strlen(i));
	g_free (i);

// Basic Feedback/Scan Parameter ============================================================

	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"bias", "V", "SRanger: (Sampel or Tip) Bias Voltage", "Bias", bias);
	ncv->add_att ("label", "Bias");

	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"motor", "V", "SRanger: auxillary/motor control Voltage", "Motor", motor);
	ncv->add_att ("label", "Motor");

	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"z_setpoint", "A", "SRanger: auxillary/Z setpoint", "Z Set Point", zpos_ref);
	ncv->add_att ("label", "Z Setpoint");

	if (DSPPACClass) { // MK3
                ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"pll_reference", "Hz", "SRanger: PAC/PLL reference freq.", "PLL Reference", DSPPACClass->pll.Reference[0]);
                ncv->add_att ("label", "PLL Reference");
        }
        
	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix0_set_point", "nA", "SRanger: Mix0: Current set point", "Current Setpt.", mix_set_point[0]);
	ncv->add_att ("label", "Current");
	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix1_set_point", "Hz", "SRanger: Mix1: Voltage set point", "Voltage Setpt.", mix_set_point[1]);
	ncv->add_att ("label", "VoltSetpt.");
	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix2_set_point", "V", "SRanger: Mix2: Aux2 set point", "Aux2 Setpt.", mix_set_point[2]);
	ncv->add_att ("label", "Aux2 Setpt.");
	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix3_set_point", "V", "SRanger: Mix3: Aux3 set point", "Aux3 Setpt.", mix_set_point[3]);
	ncv->add_att ("label", "Aux3 Setpt.");

	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix0_mix_gain", "1", "SRanger: Mix0 gain", "Current gain", mix_gain[0]);
	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix1_mix_gain", "1", "SRanger: Mix1 gain", "Voltage gain", mix_gain[1]);
	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix2_mix_gain", "1", "SRanger: Mix2 gain", "Aux2 gain", mix_gain[2]);
	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix3_mix_gain", "1", "SRanger: Mix3 gain", "Aux3 gain", mix_gain[3]);

	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix0_mix_level", "1", "SRanger: Mix0 level", "Current level", mix_level[0]);
	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix1_mix_level", "1", "SRanger: Mix1 level", "Voltage level", mix_level[1]);
	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix2_mix_level", "1", "SRanger: Mix2 level", "Aux2 level", mix_level[2]);
	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix3_mix_level", "1", "SRanger: Mix3 level", "Aux3 level", mix_level[3]);

	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix0_current_mix_transform_mode", "BC", "SRanger: Mix0 transform_mode", "Current transform_mode", (double)mix_transform_mode[0]);
	ncv->add_att ("mode_bcoding", "0:Off, 1:On, 2:Log, 4:IIR, 8:FUZZY");
	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix1_voltage_mix_transform_mode", "BC", "SRanger: Mix1 transform_mode", "Voltage transform_mode", (double)mix_transform_mode[1]);
	ncv->add_att ("mode_bcoding", "0:Off, 1:On, 2:Log, 4:IIR, 8:FUZZY");
	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix2_aux2_mix_transform_mode", "BC", "SRanger: Mix2 transform_mode", "Aux2 transform_mode", (double)mix_transform_mode[2]);
	ncv->add_att ("mode_bcoding", "0:Off, 1:On, 2:Log, 4:IIR, 8:FUZZY");
	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"mix3_aux3_mix_transform_mode", "BC", "SRanger: Mix3 transform_mode", "Aux3 transform_mode", (double)mix_transform_mode[3]);
	ncv->add_att ("mode_bcoding", "0:Off, 1:On, 2:Log, 4:IIR, 8:FUZZY");


	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"move_speed_x", "A/s", "SRanger: Move speed X", "Xm Velocity", move_speed_x);
	ncv->add_att ("label", "Velocity Xm");

	ncv=sranger_mk2_hwi_ncaddvar (ncf, MK2ID"scan_speed_x",   gtk_check_button_get_active (GTK_CHECK_BUTTON(FastScan_status)) ? "Hz":"A/s", "SRanger: Scan speed X", "Xs Velocity", scan_speed_x);
	ncv->add_att ("label", "Velocity Xs");

	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"fast_scan_flag", "On/Off", "SRanger: Fast-Scan mode (X=sine)", "ldcf", gtk_check_button_get_active (GTK_CHECK_BUTTON(FastScan_status)) ? 1:0);

	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"z_servo_CP", "1", "SRanger: User CP", "CP", z_servo[SERVO_CP]);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"z_servo_CI", "1", "SRanger: User CI", "CI", z_servo[SERVO_CI]);

	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"ldc_flag", "On/Off", "SRanger: Drift-Correct", "ldcf", gtk_check_button_get_active (GTK_CHECK_BUTTON(LDC_status)) ? 1:0);
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON(LDC_status))){
		sranger_mk2_hwi_ncaddvar (ncf, MK2ID"dxdt", "A/s", "SRanger: X-Drift Speed", "dxdt", dxdt);
		sranger_mk2_hwi_ncaddvar (ncf, MK2ID"dydt", "A/s", "SRanger: Y-Drift Speed", "dydt", dydt);
		sranger_mk2_hwi_ncaddvar (ncf, MK2ID"dzdt", "A/s", "SRanger: Z-Drift Speed", "dzdt", dydt);
	}

	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"frq_ref", "Hz", "DSP Freq. Reference", "DSP f-ref", frq_ref);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"IIR_f0_min", "Hz", "adaptive IIR f0_min", "IIR_fmin", IIR_f0_min);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"IIR_f0_max0", "Hz", "adaptive IIR f0_max0", "IIR_fmax0", IIR_f0_max[0]);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"IIR_f0_max1", "Hz", "adaptive IIR f0_max1", "IIR_fmax1", IIR_f0_max[1]);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"IIR_f0_max2", "Hz", "adaptive IIR f0_max2", "IIR_fmax2", IIR_f0_max[2]);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"IIR_f0_max3", "Hz", "adaptive IIR f0_max3", "IIR_fmax3", IIR_f0_max[3]);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"IIR_I_crossover", "nA", "adaptive IIR I_crossover", "IIR_Ic", IIR_I_crossover);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"LOG_I_offset", "nA", "Log Offset", "Log Offset", LOG_I_offset);

#if 0
	xrm.Put ("dynamic_zoom", dynamic_zoom);
	xrm.Put ("center_return_flag", center_return_flag);
#endif
	if (area_slope_compensation_flag){
		sranger_mk2_hwi_ncaddvar (ncf, MK2ID"slope_compensation_flag", "bool", "slope comp via Z0 enabled", "slope-comp", area_slope_compensation_flag);
		sranger_mk2_hwi_ncaddvar (ncf, MK2ID"slope_x", "1", "Slope X", "dz0mdx", area_slope_x);
		sranger_mk2_hwi_ncaddvar (ncf, MK2ID"slope_y", "1", "Slope Y", "dz0mdy", area_slope_y);
	}

	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"pre_points", "1", "SRanger: Pre-Scanline points", "Pre-S points", pre_points);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"dynamic_zoom", "1", "SRanger: dynamic zoom", "dyn-zoom", dynamic_zoom);

	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"XSM_Inst_VX", "1", "FYI only::SRanger/XSM: Instrument VX (X-gain setting)", "VX", sranger_mk2_hwi_pi.app->xsm->Inst->VX ());
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"XSM_Inst_VY", "1", "FYI only::SRanger/XSM: Instrument VY (Y-gain setting)", "VY", sranger_mk2_hwi_pi.app->xsm->Inst->VY ());
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"XSM_Inst_VZ", "1", "FYI only::SRanger/XSM: Instrument VZ (Z-gain setting)", "VZ", sranger_mk2_hwi_pi.app->xsm->Inst->VZ ());

	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"XSM_Inst_XResolution", "0", "FYI only::SRanger/XSM: Instrument X Resolution (=1DAC * VX in Ang)", "XRes", sranger_mk2_hwi_pi.app->xsm->Inst->XResolution ());
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"XSM_Inst_YResolution", "0", "FYI only::SRanger/XSM: Instrument Y Resolution (=1DAC * VY in Ang)", "YRes", sranger_mk2_hwi_pi.app->xsm->Inst->YResolution ());
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"XSM_Inst_ZResolution", "0", "FYI only::SRanger/XSM: Instrument Z Resolution (=1DAC * VZ in Ang)", "ZRes", sranger_mk2_hwi_pi.app->xsm->Inst->ZResolution ());

	if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING){
		sranger_mk2_hwi_ncaddvar (ncf, MK2ID"XSM_Inst_VX0", "1", "FYI only::SRanger/XSM: Instrument VX0 (XOffset-gain setting for analog offset adding only)", 
					  "VX0", sranger_mk2_hwi_pi.app->xsm->Inst->VX0 ());
		sranger_mk2_hwi_ncaddvar (ncf, MK2ID"XSM_Inst_VY0", "1", "FYI only::SRanger/XSM: Instrument VY0 (YOffset-gain setting for analog offset adding only)", 
					  "VY0", sranger_mk2_hwi_pi.app->xsm->Inst->VY0 ());
		sranger_mk2_hwi_ncaddvar (ncf, MK2ID"XSM_Inst_VZ0", "1", "FYI only::SRanger/XSM: Instrument VZ0 (ZOffset-gain setting for analog offset adding only)", 
					  "VZ0", sranger_mk2_hwi_pi.app->xsm->Inst->VZ0 ());
	}

	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"XSM_Inst_nAmpere2V", "0", "FYI only::SRanger/XSM: Instrument 1 nA to Volt(1) factor", "nAVolt", sranger_mk2_hwi_pi.app->xsm->Inst->nAmpere2V (1.));
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"XSM_Inst_BiasGain", "0", "FYI only::SRanger/XSM: Instrument BiasGainV2V() factor", "BiasGainV2V", sranger_mk2_hwi_pi.app->xsm->Inst->BiasGainV2V ());
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"XSM_Inst_BiasOffset", "0", "FYI only::SRanger/XSM: Instrument BiasV2V(0)", "BiasOffset", sranger_mk2_hwi_pi.app->xsm->Inst->BiasV2V (0.));

// LockIn ============================================================

	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"AC_amp", "Volt", "AC Bias Amplitude (LockIn)", "ACamp", AC_amp[0]);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"AC_amp_aux_Z", "Ang", "AC Z Amplitude (LockIn)", "ACamp_aux", AC_amp[1]);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"AC_amp3_lockin_shr_corrprod", "Ang", "AC internal prec norm scaling (shr) corrprod (LockIn)", "ACamp3", AC_amp[2]);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"AC_amp4_lockin_shr_corrsum", "Ang", "AC internal prec norm scaling (shr) corrsum (LockIn)", "ACamp4", AC_amp[3]);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"AC_frq", "Hertz", "AC Amplitude (LockIn)", "ACfrq", AC_frq);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"AC_phaseA", "deg", "AC Phase A (LockIn)", "ACphA", AC_phaseA);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"AC_phaseB", "deg", "AC Phase B (LockIn)", "ACphB", AC_phaseB);
	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"AC_avg_cycels", "#", "AC average cycles (LockIn)", "ACavgcyc", AC_lockin_avg_cycels);

	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"noise_amp", "Volt", "Noise Amplitude (used with RANDNUMGEN)", "Noiseamp", noise_amp);

//	sranger_mk2_hwi_ncaddvar (ncf, MK2ID"var_id", "Unit", "Var Description", "ShortName", value);

// Vector Probe ============================================================

}


#define NC_GET_VARIABLE(VNAME, VAR) if(ncf->get_var(VNAME)) ncf->get_var(VNAME)->get(VAR)

void DSPControl::load_values (NcFile *ncf){
	PI_DEBUG (DBG_L4, "DSPControl::load_values");
	// Values will also be written in old style DSP Control window for the reason of backwards compatibility
	// OK -- but will be obsoleted and removed at any later point -- PZ
	NC_GET_VARIABLE ("sranger_mk2_hwi_bias", &bias);
	NC_GET_VARIABLE ("sranger_mk2_hwi_bias", &main_get_gapp()->xsm->data.s.Bias);
        NC_GET_VARIABLE ("sranger_mk2_hwi_set_point1", &mix_set_point[1]);
        NC_GET_VARIABLE ("sranger_mk2_hwi_set_point0", &mix_set_point[0]);

	update ();
}

double DSPControl::GetUserParam (gint n, gchar *id){
	return 0.;
}

gint DSPControl::SetUserParam (gint n, gchar *id, double value){
        double bias_list[4];
	switch (n){
	case 6: // ENERGY in V --> AIC6 
                bias_list[0] = value;
                bias_list[1] = value;
                bias_list[2] = value;
                bias_list[3] = value;
		sranger_common_hwi->write_dsp_analog (bias_list, motor);
		break;
	case 10: { // GATETIME in ms
		double gatetime = value*1e3; // value is in ms
	        }
		break;
	default: break;
	}
	return 0;
}

void DSPControl::update(){
        if (!GUI_ready) return;

	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "DSP_EC_list"),
			(GFunc) App::update_ec, NULL);
	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "DSP_VPC_OPTIONS_list"),
			(GFunc) callback_update_GVP_vpc_option_checkbox, this);
}


double factor(double N, double delta){
	while (fmod (N, delta) != 0.) 
		delta -= 1.;
	return delta;
}

void DSPControl::recalculate_dsp_scan_speed_parameters (gint32 &dsp_scan_dnx, gint32 &dsp_scan_dny, 
							gint32 &dsp_scan_fs_dx, gint32 &dsp_scan_fs_dy, 
							gint32 &dsp_scan_nx_pre, gint32 &dsp_scan_fast_return){
	double frac  = (1<<16);
	double fs_dx = frac * sranger_mk2_hwi_pi.app->xsm->Inst->XA2Dig (scan_speed_x_requested) / frq_ref;
	double fs_dy = frac * sranger_mk2_hwi_pi.app->xsm->Inst->YA2Dig (scan_speed_x_requested) / frq_ref;
	
 	if ((frac * sranger_common_hwi->Dx / fs_dx) > (1<<15) || (frac * sranger_common_hwi->Dy / fs_dx) > (1<<15)){
                main_get_gapp()->message (N_("WARNING:\n"
                                  "recalculate_dsp_scan_parameters:\n"
                                  "requested/resulting scan speed is too slow.\n"
                                  "Reaching 1<<15 steps inbetween!\n"
                                  "No change on DSP performed."));
 		PI_DEBUG (DBG_EVER, "WARNING: recalculate_dsp_scan_parameters: too slow, reaching 1<<15 steps inbetween! -- no change.");
 		return;
 	}

	// fs_dx * N =!= frac*Dx  -> N = ceil [frac*Dx/fs_dx]  -> fs_dx' = frac*Dx/N
	
	// N: dnx
        dsp_scan_dnx = (gint32) ceil (frac * sranger_common_hwi->Dx * dynamic_zoom / fs_dx);
	dsp_scan_dny = (gint32) ceil (frac * sranger_common_hwi->Dy * dynamic_zoom / fs_dy);
        
	dsp_scan_fs_dx = (gint32) (frac*sranger_common_hwi->Dx / ceil (frac * sranger_common_hwi->Dx / fs_dx));
	dsp_scan_fs_dy = (gint32) (frac*sranger_common_hwi->Dy / ceil (frac * sranger_common_hwi->Dy / fs_dy));

        mirror_dsp_scan_dx32 = dsp_scan_fs_dx*dsp_scan_dnx; // actual DSP dx in S15.16 between pixels in X
        mirror_dsp_scan_dy32 = dsp_scan_fs_dy*dsp_scan_dny; // actual DSP dy in S15.16 between pixels in Y
        
	dsp_scan_fast_return = (gint32) (fast_return);
	if (dsp_scan_fast_return < 1)
		dsp_scan_fast_return = 1;
	if (dsp_scan_fast_return > 10000)
		dsp_scan_fast_return = 1;

       
	dsp_scan_nx_pre = dsp_scan_dnx * pre_points;
	
	dsp_scan_fs_dy *= sranger_common_hwi->scan_direction;

	if ( gtk_check_button_get_active (GTK_CHECK_BUTTON(FastScan_status))) 
		dsp_scan_fast_return = -1;

	// re-update to real speed
	if ( gtk_check_button_get_active (GTK_CHECK_BUTTON(FastScan_status))){
		scan_speed_x = (2.*frq_ref)/(sranger_common_hwi->Nx*dsp_scan_dnx); // Hz (sine wave for X)
		gchar *info = g_strdup_printf (" (%g Hz, %g ms/pix)", scan_speed_x, (double)dsp_scan_dnx/(2.*frq_ref)*1e3);
		scan_speed_ec->set_info (info);
		g_free (info);
	} else {
		scan_speed_x = sranger_mk2_hwi_pi.app->xsm->Inst->Dig2XA ((long)(dsp_scan_fs_dx * frq_ref / frac));
                sranger_common_hwi->scanpixelrate = (double)dsp_scan_dnx/frq_ref*scan_forward_slow_down;
		gchar *info = g_strdup_printf (" (%g A/s, %g ms/pix)", scan_speed_x/scan_forward_slow_down, sranger_common_hwi->scanpixelrate*1e3);
		scan_speed_ec->set_info (info);
		g_free (info);
	}

#if 1
	PI_DEBUG (DBG_L1,
                  "DSPControl::recalculate_dsp_scan_speed_parameters" << std::endl
                  << "** Scan Speed and Step size ReComputation -*- best integer fit results **" << std::endl
		  << "Dx                 = " << sranger_common_hwi->Dx << std::endl
		  << "(double)fs_dx      = " << fs_dx << std::endl
		  << "(double)fs_dy      = " << fs_dy << std::endl
		  << "dsp_scan.fs_dx     = " << dsp_scan_fs_dx << std::endl
		  << "dsp_scan.fs_dy     = " << dsp_scan_fs_dy << std::endl
		  << "dsp_scan.dnx       = " << dsp_scan_dnx << std::endl
		  << "dsp_scan.dny       = " << dsp_scan_dny << std::endl
		  << "dsp_scan.nx_pre    = " << dsp_scan_nx_pre << std::endl
		  << "scan_speed_x [A/s] = " << scan_speed_x << std::endl
		  << "real stepwidth [A] = " << sranger_mk2_hwi_pi.app->xsm->Inst->Dig2XA ((long)(dsp_scan_fs_dx/frac * dsp_scan_dnx)) << std::endl
		  << "dynamic-zoom fac   = " << dynamic_zoom
                  );
#endif
	
}

void DSPControl::recalculate_dsp_scan_slope_parameters (gint32 &dsp_scan_fs_dx, gint32 &dsp_scan_fs_dy,
							gint32 &dsp_scan_fm_dz0x, gint32 &dsp_scan_fm_dz0y, 
							double swx, double swy){	// setup slope compensation parameters 
	if (area_slope_compensation_flag){
		double fract  = 2147483648.; // =1<<31;
		double zx_ratio = sranger_mk2_hwi_pi.app->xsm->Inst->XResolution () / sranger_mk2_hwi_pi.app->xsm->Inst->ZModResolution (); 
		double zy_ratio = sranger_mk2_hwi_pi.app->xsm->Inst->YResolution () / sranger_mk2_hwi_pi.app->xsm->Inst->ZModResolution ();
		double mx = zx_ratio * area_slope_x;
		double my = zy_ratio * area_slope_y;
		double mx_max = 0.99999; // zx_ratio * swx; -- ideally, but not with current DSP implemented Q32 calculus possible.
		double my_max = 0.99999; // zy_ratio * swy;
		if (mx > mx_max){
			mx = mx_max;
			gchar *info = g_strdup_printf (" MAX-LIMIT!");
			slope_x_ec->set_info (info);
			g_free (info);
		} else if (mx < -mx_max){
			mx = -mx_max;
			gchar *info = g_strdup_printf (" MAX-LIMIT!");
			slope_x_ec->set_info (info);
			g_free (info);
		} else {
			gchar *info = g_strdup_printf (" ");
			slope_x_ec->set_info (info);
			g_free (info);
		}
		if (my > my_max){
			my = my_max;
			gchar *info = g_strdup_printf (" MAX-LIMIT!");
			slope_y_ec->set_info (info);
			g_free (info);
		} else if (my < -my_max){
			my = -my_max;
			gchar *info = g_strdup_printf (" MAX-LIMIT!");
			slope_y_ec->set_info (info);
			g_free (info);
		} else {
			gchar *info = g_strdup_printf (" ");
			slope_y_ec->set_info (info);
			g_free (info);
		}

		dsp_scan_fm_dz0x = (gint32)round (fract * mx);
		dsp_scan_fm_dz0y = (gint32)round (fract * my);

#if 1
		PI_DEBUG (DBG_L1,
                          "** Slope Compensation Parameters ReComputation -*- results **" << std::endl
			  << "real max width x,y = " << swx << ", " << swy << std::endl
			  << "req. slope x,y ... = " << area_slope_x << ", " << area_slope_y << std::endl
			  << "ratio x,y / z .... = " << zx_ratio << ", " << zy_ratio << std::endl
			  << "max slope x,y .... = " << mx_max << ", " << my_max << std::endl
			  << "dsp_scan.fs_dx ... = " << dsp_scan_fs_dx << std::endl
			  << "dsp_scan.fs_dy ... = " << dsp_scan_fs_dy << std::endl
			  << "dsp_scan.fm_dz0x . = " << dsp_scan_fm_dz0x << std::endl
			  << "dsp_scan.fm_dz0y . = " << dsp_scan_fm_dz0y
                          );
#endif
	}else {
		dsp_scan_fm_dz0x = 0;
		dsp_scan_fm_dz0y = 0;
	}
}

void DSPControl::updateDSP(int FbFlg){
	if (!sranger_common_hwi) return; 
	PI_DEBUG (DBG_L2, "DSPControl::updateDSP -- Hallo SR DSP ! FB=" << FbFlg );

        if (! GUI_ready){
                PI_DEBUG (DBG_L2, "DSPControl::update GUI not yet ready.");
                return;
        }
        
// mirror basic parameters to GXSM4 main -- obsolete this soon?
	main_get_gapp()->xsm->data.s.Bias = bias;
	main_get_gapp()->xsm->data.s.SetPoint = mix_set_point[1];
	main_get_gapp()->xsm->data.s.Current = mix_set_point[0];
	main_get_gapp()->xsm->data.s.SetPoint = zpos_ref;

	switch(FbFlg){
	case DSP_FB_ON: sranger_common_hwi->ExecCmd(DSP_CMD_START); break;
	case DSP_FB_OFF: sranger_common_hwi->ExecCmd(DSP_CMD_HALT); break;
	}

	// xsmres.ScannerZPolarity; // 1: pos, 0: neg (bool) -- adjust zpos_ref accordingly!
        // fix me: make zpos_ref signum manual or automatic depending on Z_SERVO - CONTROL NEG/POS setting --> verify at start, ask for correction.
	if (DSPPACClass)
		sranger_common_hwi->write_dsp_feedback (mix_set_point,  mix_unit2volt_factor, mix_gain, mix_level, mix_transform_mode,
							IIR_I_crossover, IIR_f0_max, IIR_f0_min, LOG_I_offset, IIR_flag,
							xsmres.ScannerZPolarity ? -zpos_ref:zpos_ref, z_servo, m_servo,
							DSPPACClass->pll.Reference[0]);
	else
		sranger_common_hwi->write_dsp_feedback (mix_set_point,  mix_unit2volt_factor, mix_gain, mix_level, mix_transform_mode,
							IIR_I_crossover, IIR_f0_max, IIR_f0_min, LOG_I_offset, IIR_flag,
							xsmres.ScannerZPolarity ? -zpos_ref:zpos_ref, z_servo, m_servo);

	// Update LDC?
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (LDC_status))){
		main_get_gapp()->spm_freeze_scanparam(1);
		sranger_common_hwi->set_ldc (dxdt, dydt, dzdt);
		ldc_flag = 1;
	} else if (ldc_flag){
		main_get_gapp()->spm_thaw_scanparam(1);
		sranger_common_hwi->set_ldc ();
		ldc_flag = 0;
	}

        // Update Bias, Motor in Analog section
        double bias_list[4];
        if (multiBias_mode){
                bias_list[0] = bias;
                bias_list[1] = bias_sec[1];
                bias_list[2] = bias_sec[2];
                bias_list[3] = bias_sec[3];
        } else {
                bias_list[0] = bias;
                bias_list[1] = bias;
                bias_list[2] = bias;
                bias_list[3] = bias;
        }
        sranger_common_hwi->write_dsp_analog (bias_list, motor);

	int addflag=FALSE;
	if (fabs (ue_bias - bias) > 1e-6){
		sranger_common_hwi->add_user_event_now ("Bias_adjust", "[V] (pre,now)", ue_bias, bias, addflag);
		ue_bias = bias;
		addflag=TRUE;
	}
	if (fabs (ue_set_point[0] - mix_set_point[0]) > 1e-6){
		sranger_common_hwi->add_user_event_now ("Current_SetPt_adjust", "[nA]", ue_set_point[0], mix_set_point[0], addflag);
		ue_set_point[0] = mix_set_point[0];
		addflag=TRUE;
	}
	if (fabs (ue_set_point[1] - mix_set_point[1]) > 1e-6){
		sranger_common_hwi->add_user_event_now ("Voltage_SetPt_adjust", "[V]", ue_set_point[1], mix_set_point[1], addflag);
		ue_set_point[1] = mix_set_point[1];
		addflag=TRUE;
	}
	if (fabs (ue_z_servo[SERVO_CP] - z_servo[SERVO_CP]) > 1e-6){
		sranger_common_hwi->add_user_event_now ("CP_adjust", "[1]", ue_z_servo[SERVO_CP], z_servo[SERVO_CP], addflag);
		ue_z_servo[SERVO_CP] = z_servo[SERVO_CP];
		addflag=TRUE;
	}
	if (fabs (ue_z_servo[SERVO_CI] - z_servo[SERVO_CI]) > 1e-6){
		sranger_common_hwi->add_user_event_now ("CI_adjust", "[1]", ue_z_servo[SERVO_CI], z_servo[SERVO_CI], addflag);
		ue_z_servo[SERVO_CI] = z_servo[SERVO_CI];
		addflag=TRUE;
	}


	// Scan characteristics update while scanning
	sranger_common_hwi->read_dsp_scan (dsp_scan_pflg);
	if (dsp_scan_pflg && 
	    (fabs (ue_scan_speed_x_r - scan_speed_x_requested) 
	     + fabs(ue_slope_x-area_slope_x) 
	     + fabs(ue_slope_y-area_slope_y) 
	     + fabs((double)(ue_slope_flg-area_slope_compensation_flag))) > 1e-6){
		// compute 32bit stepsize and num steps inbetween from scan_speed_x and Dx, Dy
		// sranger_mk2_hwi_pi.app->xsm->Inst->XA2Dig, Dig2XA

		if (fabs (ue_scan_speed_x_r - scan_speed_x_requested) > 1.){
			sranger_common_hwi->recalculate_dsp_scan_speed_parameters ();

			sranger_common_hwi->add_user_event_now ("Scan_Speed_adjust", "[A/s]", ue_scan_speed_x, scan_speed_x, addflag);
			ue_scan_speed_x_r = scan_speed_x_requested;
			ue_scan_speed_x   = scan_speed_x;
			addflag=TRUE;
		}

		if ((fabs(ue_slope_x-area_slope_x) + fabs(ue_slope_y-area_slope_y) + fabs((double)(ue_slope_flg-area_slope_compensation_flag))) > 1e-6)
			sranger_common_hwi->recalculate_dsp_scan_slope_parameters ();

		sranger_common_hwi->write_dsp_scan ();

		if (area_slope_compensation_flag && fabs(ue_slope_x-area_slope_x) > 1e-6){
			sranger_common_hwi->add_user_event_now ("Slope_X", "[1]", ue_slope_x, area_slope_x, addflag);
			addflag=TRUE;
		}

		if (area_slope_compensation_flag && fabs(ue_slope_y-area_slope_y) > 1e-6){
			sranger_common_hwi->add_user_event_now ("Slope_Y", "[1]", ue_slope_y, area_slope_y, addflag);
			addflag=TRUE;
		}
		
		if (area_slope_compensation_flag && !ue_slope_flg){
			sranger_common_hwi->add_user_event_now ("Slope_Compensation", "[bool]", 0.,1., addflag);
			addflag=TRUE;
		}
		
		if (!area_slope_compensation_flag && ue_slope_flg){
			sranger_common_hwi->add_user_event_now ("Slope_Compensation", "[bool]", 1.,0., addflag);
			addflag=TRUE;
		}

		ue_slope_x = area_slope_x;
		ue_slope_y = area_slope_y;
		ue_slope_flg = area_slope_compensation_flag;
	}
}

int DSPControl::ChangedAction(GtkWidget *widget, DSPControl *dspc){
	dspc->updateDSP();
	return 0;
}

void DSPControl::ChangedNotify(Param_Control* pcs, gpointer dspc){
	//  gchar *us=pcs->Get_UsrString();
	//  PI_DEBUG (DBG_L2, "DSPC: " << us );
	//  g_free(us);
	((DSPControl*)dspc)->updateDSP();
}


void DSPControl::update_zpos_readings(){
        double zp,a,b;
        main_get_gapp()->xsm->hardware->RTQuery ("z", zp, a, b);
        gchar *info = g_strdup_printf (" (%g Ang)", main_get_gapp()->xsm->Inst->V2ZAng(zp));
        ZPos_ec->set_info (info);
        ZPos_ec->Put_Value ();
        g_free (info);

        //        zpos_ref;
}

guint DSPControl::refresh_zpos_readings(DSPControl *dspc){ 
	if (main_get_gapp()->xsm->hardware->IsSuspendWatches ())
		return TRUE;

	dspc->update_zpos_readings ();
	return TRUE;
}

int DSPControl::zpos_monitor_callback( GtkWidget *widget, DSPControl *dspc){
        // *** do not touch any more *** sranger_common_hwi->write_dsp_state (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)) ? MD_ZPOS_ADJUSTER:-MD_ZPOS_ADJUSTER);
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))){
                //g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (widget), "DSP_zpos_control_list"), (GFunc) App::thaw_ec, NULL);
                dspc->zpos_refresh_timer_id = g_timeout_add (200, (GSourceFunc)DSPControl::refresh_zpos_readings, dspc);
        }else{
                //g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (widget), "DSP_zpos_control_list"), (GFunc) App::freeze_ec, NULL);

                if (dspc->zpos_refresh_timer_id){
                        g_source_remove (dspc->zpos_refresh_timer_id);
                        dspc->zpos_refresh_timer_id = 0;
                }
        }
        return 0;
}

int DSPControl::feedback_callback( GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG (DBG_L4, "==> DSPControl::feedback_callback -- checkbutton");
        
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->updateDSP(DSP_FB_ON);
	else
		dspc->updateDSP(DSP_FB_OFF);
	return 0;
}

int DSPControl::spd_link_callback( GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG (DBG_L4, "==> DSPControl::spd_link_callback -- checkbutton");
        
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))){
                PI_DEBUG (DBG_L4, "==> enable spd_link sync -- N/A at this time");
		;
	} else {
                PI_DEBUG (DBG_L4, "==> disable spd_link sync -- N/A at this time");
		;
        }
	return 0;
}

int DSPControl::IIR_callback( GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->IIR_flag = 1;
	else
		dspc->IIR_flag = 0;
	dspc->updateDSP();
	return 0;
}

int DSPControl::set_clr_mode_callback( GtkWidget *widget, gpointer mask){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		sranger_common_hwi->SetMode (GPOINTER_TO_INT (mask));
	else	
		sranger_common_hwi->ClrMode (GPOINTER_TO_INT (mask));
	return 0;
}

int DSPControl::se_auto_trigger_callback(GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	dspc->update_trigger (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
	return 0;
}

int DSPControl::ldc_callback(GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	dspc->updateDSP();
	return 0;
}

void DSPControl::lockin_adjust_callback(Param_Control* pcs, gpointer data){
	DSPControl *dspc = (DSPControl*)data;
	static double ACf=-1;
	gint start=0;

        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	if (DSPPACClass){ // MK3
		if (fabs(ACf-dspc->AC_frq) > 1.){
			ACf = dspc->AC_frq;
			start = 4;
		}

		sranger_common_hwi->write_dsp_lockin_probe_final (
								  dspc->AC_amp, dspc->AC_frq, 
								  dspc->AC_phaseA, dspc->AC_phaseB,
								  dspc->AC_lockin_avg_cycels, dspc->FZ_limiter_val, 
								  dspc->noise_amp,
								  start);

		gchar *ACinfo = g_strdup_printf (" (%g Hz)", dspc->AC_frq);
                PI_DEBUG (DBG_L4, "DSPControl::lockin_adjust_callback -- ACinfo = " << ACinfo);
		dspc->AC_frq_ec->set_info (ACinfo);
                g_free (ACinfo);
	}
}

int DSPControl::lockin_runfree_callback(GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                sranger_common_hwi->dsp_lockin_state(1);
	else
		sranger_common_hwi->dsp_lockin_state(0);

	return 0;
}

int DSPControl::fast_scan_callback(GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		sranger_common_hwi->SetFastScan (1);
	else
		sranger_common_hwi->SetFastScan (0);
	return 0;
}

int DSPControl::check_vp_in_progress (const gchar *extra_info){
	double a,b,c;
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	if (! sranger_common_hwi->RTQuery_clear_to_start_probe ()){
	
                GtkDialogFlags flags =  (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);
                GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("Attention -- VP or Scan in progress"),
                                                                 GTK_WINDOW (main_get_gapp()->get_app_window ()),
								 flags,
                                                                 _("_OK"),
                                                                 GTK_RESPONSE_ACCEPT,
                                                                 _("_Cancel"),
                                                                 GTK_RESPONSE_REJECT,
								 NULL);

                if (extra_info)
                        gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG(dialog))), 
                                           gtk_label_new (N_(extra_info)));
                
                else
                        gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG(dialog))), 
                                           gtk_label_new (N_("Vector Probe or Scan in progress.\n"
                                                             "Cancel request and wait or force restart (OK)?\n"
                                                             "Warning: overriding this this may lead to unpredicatble results.")));
                
		gtk_widget_show (dialog);

                int response = GTK_RESPONSE_NONE;
                g_signal_connect (dialog, "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
                while (response == GTK_RESPONSE_NONE)
                        while (g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

		return response == GTK_RESPONSE_ACCEPT ? 0:-1;
	}
	return 0;
}

// LockIn write (optional)

int DSPControl::LockIn_read_callback( GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
//	dspc->read_dsp_lockin (...);
        return 0;
}

int DSPControl::LockIn_write_callback( GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
//	dspc->write_dsp_lockin_probe_final (...);
        return 0;
}

// PV write
// Note: Exec and/or Write always includes LockIn and PV data write to DSP!

int DSPControl::LockIn_exec_callback( GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	if (dspc->check_vp_in_progress ()) 
		return -1;

        dspc->write_dsp_probe (0, PV_MODE_NONE);

	if (dspc->AC_auto_flags & FLAG_AUTO_GLOCK){
		dspc->vis_Source  = dspc->AC_glock_data[0];
		dspc->vis_XSource = dspc->AC_glock_data[1];
		dspc->vis_PSource = dspc->AC_glock_data[2];
		dspc->vis_XJoin   = dspc->AC_glock_data[3];
		dspc->vis_PlotAvg = dspc->AC_glock_data[4];
		dspc->vis_PlotSec = dspc->AC_glock_data[5];
	} else {
		dspc->vis_Source = dspc->AC_glock_data[0] = dspc->Source ;
		dspc->vis_XSource= dspc->AC_glock_data[1] = dspc->XSource;
		dspc->vis_PSource= dspc->AC_glock_data[2] = dspc->PSource;
		dspc->vis_XJoin  = dspc->AC_glock_data[3] = dspc->XJoin  ;
		dspc->vis_PlotAvg= dspc->AC_glock_data[4] = dspc->PlotAvg;
		dspc->vis_PlotSec= dspc->AC_glock_data[5] = dspc->PlotSec;
	}

	dspc->current_auto_flags = dspc->AC_auto_flags;
	dspc->probe_trigger_single_shot = 1;
	dspc->write_dsp_probe (1, PV_MODE_AC); // Exec AC-phase probing here
	sranger_common_hwi->start_fifo_read (0, 0,0,0,0, NULL,NULL,NULL,NULL);

	return 0;
}

int DSPControl::Probing_exec_IV_callback( GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	dspc->current_auto_flags = dspc->IV_auto_flags;

	if (dspc->check_vp_in_progress ()) 
		return -1;

        if (dspc->IV_auto_flags & FLAG_AUTO_RUN_INITSCRIPT){
                gchar *tmp = g_strdup ("vp-sts-initial");
                main_get_gapp()->SignalRemoteActionToPlugins (&tmp);
                g_free (tmp);
        }
        
        dspc->write_dsp_probe (0, PV_MODE_NONE);

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
	dspc->probe_trigger_single_shot = 1;
	dspc->write_dsp_probe (1, PV_MODE_IV); // Exec STS probing here
	sranger_common_hwi->start_fifo_read (0, 0,0,0,0, NULL,NULL,NULL,NULL);

	return 0;
}

int DSPControl::Probing_write_IV_callback( GtkWidget *widget, DSPControl *dspc){
        dspc->write_dsp_probe (0, PV_MODE_IV);
        return 0;
}

int DSPControl::Probing_exec_FZ_callback( GtkWidget *widget, DSPControl *dspc){
	if (dspc->check_vp_in_progress ()) 
		return -1;

        if (dspc->FZ_auto_flags & FLAG_AUTO_RUN_INITSCRIPT){
                gchar *tmp = g_strdup ("vp-fz-initial");
                main_get_gapp()->SignalRemoteActionToPlugins (&tmp);
                g_free (tmp);
        }

        dspc->write_dsp_probe (0, PV_MODE_NONE);

	if (dspc->FZ_auto_flags & FLAG_AUTO_GLOCK){
		dspc->vis_Source  = dspc->FZ_glock_data[0];
		dspc->vis_XSource = dspc->FZ_glock_data[1];
		dspc->vis_PSource = dspc->FZ_glock_data[2];
		dspc->vis_XJoin   = dspc->FZ_glock_data[3];
		dspc->vis_PlotAvg = dspc->FZ_glock_data[4];
		dspc->vis_PlotSec = dspc->FZ_glock_data[5];
	} else {
		dspc->vis_Source = dspc->FZ_glock_data[0] = dspc->Source ;
		dspc->vis_XSource= dspc->FZ_glock_data[1] = dspc->XSource;
		dspc->vis_PSource= dspc->FZ_glock_data[2] = dspc->PSource;
		dspc->vis_XJoin  = dspc->FZ_glock_data[3] = dspc->XJoin  ;
		dspc->vis_PlotAvg= dspc->FZ_glock_data[4] = dspc->PlotAvg;
		dspc->vis_PlotSec= dspc->FZ_glock_data[5] = dspc->PlotSec;
	}

	dspc->current_auto_flags = dspc->FZ_auto_flags;
	dspc->probe_trigger_single_shot = 1;
	dspc->write_dsp_probe (1, PV_MODE_FZ); // Exec FZ probing here
	sranger_common_hwi->start_fifo_read (0, 0,0,0,0, NULL,NULL,NULL,NULL);

	return 0;
}

int DSPControl::Probing_write_FZ_callback( GtkWidget *widget, DSPControl *dspc){
        dspc->write_dsp_probe (0, PV_MODE_FZ);
        return 0;
}

// PL

int DSPControl::Probing_exec_PL_callback( GtkWidget *widget, DSPControl *dspc){
	if (dspc->check_vp_in_progress ()) 
		return -1;

        if (dspc->PL_auto_flags & FLAG_AUTO_RUN_INITSCRIPT){
                gchar *tmp = g_strdup ("vp-pl-initial");
                main_get_gapp()->SignalRemoteActionToPlugins (&tmp);
                g_free (tmp);
        }
        
        dspc->write_dsp_probe (0, PV_MODE_NONE);

	if (dspc->PL_auto_flags & FLAG_AUTO_GLOCK){
		dspc->vis_Source  = dspc->PL_glock_data[0];
		dspc->vis_XSource = dspc->PL_glock_data[1];
		dspc->vis_PSource = dspc->PL_glock_data[2];
		dspc->vis_XJoin   = dspc->PL_glock_data[3];
		dspc->vis_PlotAvg = dspc->PL_glock_data[4];
		dspc->vis_PlotSec = dspc->PL_glock_data[5];
	} else {
		dspc->vis_Source = dspc->PL_glock_data[0] = dspc->Source ;
		dspc->vis_XSource= dspc->PL_glock_data[1] = dspc->XSource;
		dspc->vis_PSource= dspc->PL_glock_data[2] = dspc->PSource;
		dspc->vis_XJoin  = dspc->PL_glock_data[3] = dspc->XJoin  ;
		dspc->vis_PlotAvg= dspc->PL_glock_data[4] = dspc->PlotAvg;
		dspc->vis_PlotSec= dspc->PL_glock_data[5] = dspc->PlotSec;
	}


	dspc->current_auto_flags = dspc->PL_auto_flags;
	dspc->probe_trigger_single_shot = 1;
	dspc->write_dsp_probe (1, PV_MODE_PL); // Exec FZ probing here
	sranger_common_hwi->start_fifo_read (0, 0,0,0,0, NULL,NULL,NULL,NULL);

	return 0;
}

int DSPControl::Probing_write_PL_callback( GtkWidget *widget, DSPControl *dspc){
        dspc->write_dsp_probe (0, PV_MODE_PL);
        return 0;
}

// LP

int DSPControl::Probing_exec_LP_callback( GtkWidget *widget, DSPControl *dspc){
	if (dspc->check_vp_in_progress ()) 
		return -1;

        if (dspc->LP_auto_flags & FLAG_AUTO_RUN_INITSCRIPT){
                gchar *tmp = g_strdup ("vp-lp-initial");
                main_get_gapp()->SignalRemoteActionToPlugins (&tmp);
                g_free (tmp);
        }

	dspc->write_dsp_probe (0, PV_MODE_NONE);
	
	if (dspc->LP_auto_flags & FLAG_AUTO_GLOCK){
		dspc->vis_Source  = dspc->LP_glock_data[0];
		dspc->vis_XSource = dspc->LP_glock_data[1];
		dspc->vis_PSource = dspc->LP_glock_data[2];
		dspc->vis_XJoin   = dspc->LP_glock_data[3];
		dspc->vis_PlotAvg = dspc->LP_glock_data[4];
		dspc->vis_PlotSec = dspc->LP_glock_data[5];
	} else {
		dspc->vis_Source = dspc->LP_glock_data[0] = dspc->Source ;
		dspc->vis_XSource= dspc->LP_glock_data[1] = dspc->XSource;
		dspc->vis_PSource= dspc->LP_glock_data[2] = dspc->PSource;
		dspc->vis_XJoin  = dspc->LP_glock_data[3] = dspc->XJoin  ;
		dspc->vis_PlotAvg= dspc->LP_glock_data[4] = dspc->PlotAvg;
		dspc->vis_PlotSec= dspc->LP_glock_data[5] = dspc->PlotSec;
	}

	dspc->current_auto_flags = dspc->LP_auto_flags;
	dspc->probe_trigger_single_shot = 1;
	dspc->write_dsp_probe (1, PV_MODE_LP); // Exec FZ probing here
	sranger_common_hwi->start_fifo_read (0, 0,0,0,0, NULL,NULL,NULL,NULL);

	return 0;
}

int DSPControl::Probing_write_LP_callback( GtkWidget *widget, DSPControl *dspc){
	dspc->write_dsp_probe (0, PV_MODE_LP);
        return 0;
}

// SP

int DSPControl::Probing_exec_SP_callback( GtkWidget *widget, DSPControl *dspc){
	if (dspc->check_vp_in_progress ()) 
		return -1;

        dspc->write_dsp_probe (0, PV_MODE_NONE);

	if (dspc->SP_auto_flags & FLAG_AUTO_GLOCK){
		dspc->vis_Source  = dspc->SP_glock_data[0];
		dspc->vis_XSource = dspc->SP_glock_data[1];
		dspc->vis_PSource = dspc->SP_glock_data[2];
		dspc->vis_XJoin   = dspc->SP_glock_data[3];
		dspc->vis_PlotAvg = dspc->SP_glock_data[4];
		dspc->vis_PlotSec = dspc->SP_glock_data[5];
	} else {
		dspc->vis_Source = dspc->SP_glock_data[0] = dspc->Source ;
		dspc->vis_XSource= dspc->SP_glock_data[1] = dspc->XSource;
		dspc->vis_PSource= dspc->SP_glock_data[2] = dspc->PSource;
		dspc->vis_XJoin  = dspc->SP_glock_data[3] = dspc->XJoin  ;
		dspc->vis_PlotAvg= dspc->SP_glock_data[4] = dspc->PlotAvg;
		dspc->vis_PlotSec= dspc->SP_glock_data[5] = dspc->PlotSec;
	}

	dspc->current_auto_flags = dspc->SP_auto_flags;
	dspc->probe_trigger_single_shot = 1;
	dspc->write_dsp_probe (1, PV_MODE_SP); // Exec FZ probing here
	sranger_common_hwi->start_fifo_read (0, 0,0,0,0, NULL,NULL,NULL,NULL);

	return 0;
}

int DSPControl::Probing_write_SP_callback( GtkWidget *widget, DSPControl *dspc){
        dspc->write_dsp_probe (0, PV_MODE_SP);
        return 0;
}

// TS

int DSPControl::Probing_exec_TS_callback( GtkWidget *widget, DSPControl *dspc){
	if (dspc->check_vp_in_progress ()) 
		return -1;

        if (dspc->TS_auto_flags & FLAG_AUTO_RUN_INITSCRIPT){
                gchar *tmp = g_strdup ("vp-ts-initial");
                main_get_gapp()->SignalRemoteActionToPlugins (&tmp);
                g_free (tmp);
        }

        dspc->write_dsp_probe (0, PV_MODE_NONE);

	if (dspc->TS_auto_flags & FLAG_AUTO_GLOCK){
		dspc->vis_Source  = dspc->TS_glock_data[0];
		dspc->vis_XSource = dspc->TS_glock_data[1];
		dspc->vis_PSource = dspc->TS_glock_data[2];
		dspc->vis_XJoin   = dspc->TS_glock_data[3];
		dspc->vis_PlotAvg = dspc->TS_glock_data[4];
		dspc->vis_PlotSec = dspc->TS_glock_data[5];
	} else {
		dspc->vis_Source = dspc->TS_glock_data[0] = dspc->Source ;
		dspc->vis_XSource= dspc->TS_glock_data[1] = dspc->XSource;
		dspc->vis_PSource= dspc->TS_glock_data[2] = dspc->PSource;
		dspc->vis_XJoin  = dspc->TS_glock_data[3] = dspc->XJoin  ;
		dspc->vis_PlotAvg= dspc->TS_glock_data[4] = dspc->PlotAvg;
		dspc->vis_PlotSec= dspc->TS_glock_data[5] = dspc->PlotSec;
	}

	dspc->current_auto_flags = dspc->TS_auto_flags;
	dspc->probe_trigger_single_shot = 1;
	dspc->write_dsp_probe (1, PV_MODE_TS); // Exec FZ probing here
	sranger_common_hwi->start_fifo_read (0, 0,0,0,0, NULL,NULL,NULL,NULL);

	return 0;
}

int DSPControl::Probing_write_TS_callback( GtkWidget *widget, DSPControl *dspc){
        dspc->write_dsp_probe (0, PV_MODE_TS);
        return 0;
}

// GVP

int DSPControl::Probing_exec_GVP_callback( GtkWidget *widget, DSPControl *dspc){
	dspc->current_auto_flags = dspc->GVP_auto_flags;

	if (dspc->check_vp_in_progress ()) 
		return -1;

        if (dspc->GVP_auto_flags & FLAG_AUTO_RUN_INITSCRIPT){
                gchar *tmp = g_strdup ("vp-gvp-initial");
                main_get_gapp()->SignalRemoteActionToPlugins (&tmp);
                g_free (tmp);
        }
      
        dspc->write_dsp_probe (0, PV_MODE_NONE);

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

	dspc->probe_trigger_single_shot = 1;
	dspc->write_dsp_probe (1, PV_MODE_GVP); // Exec FZ probing here
	sranger_common_hwi->start_fifo_read (0, 0,0,0,0, NULL,NULL,NULL,NULL);

	return 0;
}

int DSPControl::Probing_write_GVP_callback( GtkWidget *widget, DSPControl *dspc){
        dspc->write_dsp_probe (0, PV_MODE_GVP);
        return 0;
}

// TK

int DSPControl::Probing_exec_TK_callback( GtkWidget *widget, DSPControl *dspc){
	if (dspc->check_vp_in_progress ()) 
		return -1;

        if (dspc->TK_auto_flags & FLAG_AUTO_RUN_INITSCRIPT){
                gchar *tmp = g_strdup ("vp-tk-initial");
                main_get_gapp()->SignalRemoteActionToPlugins (&tmp);
                g_free (tmp);
        }

        dspc->write_dsp_probe (0, PV_MODE_NONE);

	if (dspc->TK_auto_flags & FLAG_AUTO_GLOCK){
		dspc->vis_Source  = dspc->TK_glock_data[0];
		dspc->vis_XSource = dspc->TK_glock_data[1];
		dspc->vis_PSource = dspc->TK_glock_data[2];
		dspc->vis_XJoin   = dspc->TK_glock_data[3];
		dspc->vis_PlotAvg = dspc->TK_glock_data[4];
		dspc->vis_PlotSec = dspc->TK_glock_data[5];
	} else {
		dspc->vis_Source = dspc->TK_glock_data[0] = dspc->Source ;
		dspc->vis_XSource= dspc->TK_glock_data[1] = dspc->XSource;
		dspc->vis_PSource= dspc->TK_glock_data[2] = dspc->PSource;
		dspc->vis_XJoin  = dspc->TK_glock_data[3] = dspc->XJoin  ;
		dspc->vis_PlotAvg= dspc->TK_glock_data[4] = dspc->PlotAvg;
		dspc->vis_PlotSec= dspc->TK_glock_data[5] = dspc->PlotSec;
	}

	dspc->current_auto_flags = dspc->TK_auto_flags;

	dspc->probe_trigger_single_shot = 1;
	dspc->write_dsp_probe (1, PV_MODE_TK); // Exec FZ probing here
	sranger_common_hwi->start_fifo_read (0, 0,0,0,0, NULL,NULL,NULL,NULL);

	return 0;
}

int DSPControl::Probing_write_TK_callback( GtkWidget *widget, DSPControl *dspc){
	dspc->dump_probe_hdr (); // TESTING
        dspc->write_dsp_probe (0, PV_MODE_TK);
        return 0;
}


// AX

int DSPControl::Probing_exec_AX_callback( GtkWidget *widget, DSPControl *dspc){
	if (dspc->check_vp_in_progress ()) 
		return -1;

        if (dspc->AX_auto_flags & FLAG_AUTO_RUN_INITSCRIPT){
                gchar *tmp = g_strdup ("vp-ax-initial");
                main_get_gapp()->SignalRemoteActionToPlugins (&tmp);
                g_free (tmp);
        }

        dspc->write_dsp_probe (0, PV_MODE_NONE);

	dspc->current_auto_flags = dspc->AX_auto_flags;

	if (dspc->AX_auto_flags & FLAG_AUTO_GLOCK){
		dspc->vis_Source  = dspc->AX_glock_data[0];
		dspc->vis_XSource = dspc->AX_glock_data[1];
		dspc->vis_PSource = dspc->AX_glock_data[2];
		dspc->vis_XJoin   = dspc->AX_glock_data[3];
		dspc->vis_PlotAvg = dspc->AX_glock_data[4];
		dspc->vis_PlotSec = dspc->AX_glock_data[5];
	} else {
		dspc->vis_Source = dspc->AX_glock_data[0] = dspc->Source ;
		dspc->vis_XSource= dspc->AX_glock_data[1] = dspc->XSource;
		dspc->vis_PSource= dspc->AX_glock_data[2] = dspc->PSource;
		dspc->vis_XJoin  = dspc->AX_glock_data[3] = dspc->XJoin  ;
		dspc->vis_PlotAvg= dspc->AX_glock_data[4] = dspc->PlotAvg;
		dspc->vis_PlotSec= dspc->AX_glock_data[5] = dspc->PlotSec;
	}

	dspc->probe_trigger_single_shot = 1;
	dspc->write_dsp_probe (1, PV_MODE_AX); // Exec AX probing here
	sranger_common_hwi->start_fifo_read (0, 0,0,0,0, NULL,NULL,NULL,NULL);

	return 0;
}

int DSPControl::Probing_write_AX_callback( GtkWidget *widget, DSPControl *dspc){
	// GateTime is auto matched to probe speed!

	// setup vector table
        dspc->write_dsp_probe (0, PV_MODE_AX);
        return 0;
}


// for TESTING and DEBUGGIG only
int DSPControl::Probing_exec_ABORT_callback( GtkWidget *widget, DSPControl *dspc){
	if (dspc->check_vp_in_progress ("GVP ABORT REQUESTED:\n"
                                        "Interrupting a Vector Program in progress will\n"
                                        "leave the DSP in any state of that moment:\n\n"
                                        "OK: Bias, Z, will auto recover."
                                        "Eventually not OK: current Feedback State will stay!"
                                        " -> check/fix manually!"
                                        "Current data stream will get corrupted.")) 
		return -1;

	dspc->write_dsp_abort_probe (); // force probe abort
        
        dspc->write_dsp_probe (0, PV_MODE_NONE); // clear vector code

#if 0
	dspc->current_auto_flags = dspc->ABORT_auto_flags;

	if (dspc->ABORT_auto_flags & FLAG_AUTO_GLOCK){
		dspc->vis_Source  = dspc->ABORT_glock_data[0];
		dspc->vis_XSource = dspc->ABORT_glock_data[1];
		dspc->vis_PSource = dspc->ABORT_glock_data[2];
		dspc->vis_XJoin   = dspc->ABORT_glock_data[3];
		dspc->vis_PlotAvg = dspc->ABORT_glock_data[4];
		dspc->vis_PlotSec = dspc->ABORT_glock_data[5];
	} else {
		dspc->vis_Source = dspc->ABORT_glock_data[0] = dspc->Source ;
		dspc->vis_XSource= dspc->ABORT_glock_data[1] = dspc->XSource;
		dspc->vis_PSource= dspc->ABORT_glock_data[2] = dspc->PSource;
		dspc->vis_XJoin  = dspc->ABORT_glock_data[3] = dspc->XJoin  ;
		dspc->vis_PlotAvg= dspc->ABORT_glock_data[4] = dspc->PlotAvg;
		dspc->vis_PlotSec= dspc->ABORT_glock_data[5] = dspc->PlotSec;
	}

	dspc->probe_trigger_single_shot = 1;
	dspc->write_dsp_probe (1, PV_MODE_ABORT); // Exec ABORT VP probe now
	sranger_common_hwi->start_fifo_read (0, 0,0,0,0, NULL,NULL,NULL,NULL);
#endif 
	return 0;
}

int DSPControl::Probing_write_ABORT_callback( GtkWidget *widget, DSPControl *dspc){
	// GateTime is auto matched to probe speed!

	// setup vector table
        dspc->write_dsp_probe (0, PV_MODE_ABORT);
        return 0;
}




void DSPControl::StartScanPreCheck (){
        dsp_bp->scan_start_gui_actions ();

	dynamic_zoom = 1.;
	update ();

	update_sourcesignals_from_DSP_callback ();

	if (write_vector_mode == PV_MODE_NONE)
		probe_trigger_raster_points = 0;
	else{
		probe_trigger_raster_points = probe_trigger_raster_points_user;
		raster_auto_flags = current_auto_flags;
		write_dsp_probe (0, write_vector_mode);
	}
}

void DSPControl::EndScanCheck (){
        dsp_bp->scan_end_gui_actions ();
}

int DSPControl::auto_probe_callback(GtkWidget *widget, DSPControl *dspc){
        dspc->write_vector_mode =  (pv_mode) atoi (gtk_combo_box_get_active_id (GTK_COMBO_BOX (widget)));
	Gtk_EntryControl* ra = (Gtk_EntryControl*) g_object_get_data( G_OBJECT (widget), "raster_ec");
	Gtk_EntryControl* rb = (Gtk_EntryControl*) g_object_get_data( G_OBJECT (widget), "rasterb_ec");
	if (!ra || !rb) return 0;

	if (dspc->write_vector_mode == PV_MODE_NONE){
		ra->Freeze ();
		rb->Freeze ();
	} else {
		ra->Thaw ();
                rb->Thaw ();
        }

	return 0;	      
}

int DSPControl::choice_mixmode_callback (GtkWidget *widget, DSPControl *dspc){
	gint channel=0;
        gint selection=0;

        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

        channel = GPOINTER_TO_INT (g_object_get_data(G_OBJECT (widget), "mix_channel"));
        selection = atoi (gtk_combo_box_get_active_id (GTK_COMBO_BOX (widget)));

        //g_print ("Choice MIX%d MT=%d\n", channel, selection);

	PI_DEBUG_GP (DBG_L3, "MixMode[%d]=0x%x\n",channel,selection);

	dspc->mix_transform_mode[channel] = selection;

        PI_DEBUG_GP (DBG_L4, "%s ** 2\n",__FUNCTION__);

	dspc->updateDSP();
        //g_print ("DSP READBACK MIX%d MT=%d\n", channel, (int)sranger_common_hwi->read_dsp_feedback ("MT", channel));

        PI_DEBUG_GP (DBG_L4, "%s **3 done\n",__FUNCTION__);
        return 0;
}

int DSPControl::choice_mixsource_callback (GtkWidget *widget, DSPControl *dspc){
	int mix_ch;
        int signal = atoi (gtk_combo_box_get_active_id (GTK_COMBO_BOX (widget)));
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	mix_ch = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "mix_channel_fbsource"));
	PI_DEBUG_GP (DBG_L3, "Mixer-Input[%d]=0x%x  <== %s\n",mix_ch, signal, sranger_common_hwi->dsp_signal_lookup_managed[signal].label);
	sranger_common_hwi->change_signal_input (signal, DSP_SIGNAL_MIXER0_INPUT_ID+mix_ch);
	dspc->mix_fbsource[mix_ch] = signal;
	PI_DEBUG_GP (DBG_L3, "MIX-a\n");
	// manage unit -- TDB
	dspc->mix_unit2volt_factor[mix_ch] = 1.;
	dspc->mix_unit2volt_factor[0] = main_get_gapp()->xsm->Inst->nAmpere2V (1.);

        if (mix_ch > 1){
                // !strncmp (sranger_common_hwi->dsp_signal_lookup_managed[signal].label, "PLL ", 4)){
                const gchar *l = sranger_common_hwi->dsp_signal_lookup_managed[signal].label;
                const gchar *u = sranger_common_hwi->dsp_signal_lookup_managed[signal].unit;
                double s =  sranger_common_hwi->dsp_signal_lookup_managed[signal].scale;
                //dsp_feedback_mixer.setpoint[i] = (int)(round(s*factor[i]*set_point[i])); // Q23

                if (!strncmp (sranger_common_hwi->dsp_signal_lookup_managed[signal].label, "In ", 3))
                        dspc->mix_unit2volt_factor[mix_ch] = 1./s/256.; // In0...7 Q23
                else
                        dspc->mix_unit2volt_factor[mix_ch] = 1./s; // other signals
                //g_print ("MIX[%d]: %s in %s scale=%g  u2v=%g\n",mix_ch,l,u,s, dspc->mix_unit2volt_factor[mix_ch] );


                UnitObj *tmp = NULL;
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
                
                Gtk_EntryControl *setpoint_ec =   ( Gtk_EntryControl *) g_object_get_data (G_OBJECT (widget), "related_ec_setpoint");
                if (setpoint_ec)
                        setpoint_ec->changeUnit (tmp);
                Gtk_EntryControl *setlevel_ec =   ( Gtk_EntryControl *) g_object_get_data (G_OBJECT (widget), "related_ec_level");
                if (setlevel_ec)
                        setlevel_ec->changeUnit (tmp);
                delete (tmp);
        }
        
	double scale_extra = 256.;
        
	if (!strncmp (sranger_common_hwi->dsp_signal_lookup_managed[signal].label, "In ", 3))
		scale_extra = 256.*256; // In 0..7 are scaled up for MIXER0-3, else raw 32bit number.

	if (mix_ch == 0){
                gchar *tmp = g_strdup_printf ("Mix-%s-ITunnel", sranger_common_hwi->dsp_signal_lookup_managed[signal].label);
              PI_DEBUG_GP (DBG_L3, "MIX[0] =>> %s\n", tmp);
	      main_get_gapp()->channelselector->SetModeChannelSignal (6+3, tmp, tmp, // "nA", 1.);  //-- fix ??? -- adjusting signal lookup at startup now!
                                                           sranger_common_hwi->dsp_signal_lookup_managed[signal].unit,
                                                           sranger_common_hwi->dsp_signal_lookup_managed[signal].scale*scale_extra);
	      g_free (tmp);
	} else {
	      gchar *tmp = g_strdup_printf ("Mix-%s", sranger_common_hwi->dsp_signal_lookup_managed[signal].label);
              PI_DEBUG_GP (DBG_L3, "MIX[%d) ->> >> %s\n", mix_ch, tmp);
	      main_get_gapp()->channelselector->SetModeChannelSignal (6+mix_ch-1, tmp, tmp,
                                                           sranger_common_hwi->dsp_signal_lookup_managed[signal].unit,
                                                           sranger_common_hwi->dsp_signal_lookup_managed[signal].scale*scale_extra
                                                           );
	      g_free (tmp);
	}
        return 0;
}

int DSPControl::choice_vector_index_j_callback (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
        gint i = atoi (gtk_combo_box_get_active_id (GTK_COMBO_BOX (widget)));
        if (i>=0){
                dspc->DSP_vpdata_ij[1] = i;
                PI_DEBUG_GP (DBG_L3, "DSPControl::choice_vector_index_j_callback -- DSP_vpdata_ij[1]=%d",i);
        } else
                g_warning ("DSPControl::choice_vector_index_j_callback -- ignoring: index < 0");
        return 0;
}

void DSPControl::update_sourcesignals_from_DSP_callback (){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	if (!DSPPACClass)
		return;

	// Channelselector signal updates MIXER0-3
	for (int i=0; i<4; ++i){
		int si = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_MIXER0_INPUT_ID+i);

		double scale_extra = 256.;
		if (!strncmp (sranger_common_hwi->lookup_dsp_signal_managed (si)->label, "In ", 3))
			scale_extra = 256.*256; // In 0..7 are scaled up for MIXER0-3, else raw 32bit number.

		if (i == 0){
                        g_message ("UPDATE SigSrc DSP: [%d] >%s< in (%s) x %g x se=%g {%g}",
                                   si,
                                   sranger_common_hwi->lookup_dsp_signal_managed (si)->label,
                                   sranger_common_hwi->lookup_dsp_signal_managed (si)->unit,
                                   sranger_common_hwi->lookup_dsp_signal_managed (si)->scale,
                                   scale_extra,
                                   sranger_common_hwi->lookup_dsp_signal_managed (si)->scale/(10.0/(32767.*(1<<16)))
                                   );
		        gchar *tmp = g_strdup_printf ("Mix-%s-ITunnel", sranger_common_hwi->lookup_dsp_signal_managed (si)->label);
			main_get_gapp()->channelselector->SetModeChannelSignal(6+3, tmp, tmp, // fix: ?? "nA", 1. ); // now adjusted for IN0 at startup in DSP signal table
								    sranger_common_hwi->lookup_dsp_signal_managed (si)->unit,
								    sranger_common_hwi->lookup_dsp_signal_managed (si)->scale*scale_extra
								    );
			g_free (tmp);
		} else {
		        gchar *tmp = g_strdup_printf ("Mix-%s", sranger_common_hwi->lookup_dsp_signal_managed (si)->label);
			main_get_gapp()->channelselector->SetModeChannelSignal(6+i-1, tmp, tmp,
								    sranger_common_hwi->lookup_dsp_signal_managed (si)->unit,
								    sranger_common_hwi->lookup_dsp_signal_managed (si)->scale*scale_extra
								    );
			g_free (tmp);
		}
	}
	// Channelselector signal updates SCAN_CHANNEL_MAP0-3[sec]-VecProbeSignal
	// and related selector pull down menu update Scan Input Source Selection0-3
	for (int i=0; i<4; ++i){ // SCAN_CHANNEL_MAP, VP SecV <-> VECPROBEn
		// Channelselector
		const gchar *vjfixedlab[] = { "Counter 0", "Counter 1", "NULL", "NULL" };
		int si = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID+i);
		if (sranger_common_hwi->lookup_dsp_signal_managed (si)->index >= 0){ // so far ONLY "VP SecV" signal is vector!
			int vi=sranger_common_hwi->lookup_dsp_signal_managed (si)->index/8;
			int vj=sranger_common_hwi->lookup_dsp_signal_managed (si)->index - 8*vi;
			gchar *tmp = g_strdup_printf("%s[%d]-%s",
						     sranger_common_hwi->lookup_dsp_signal_managed (si)->label, vi, 
						     vj < 4 
						     ? sranger_common_hwi->dsp_signal_lookup_managed[sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE0_INPUT_ID+vj)].label
						     : vjfixedlab[vj-4]
						     );
			main_get_gapp()->channelselector->SetModeChannelSignal(17+i, tmp, tmp, 
								    sranger_common_hwi->dsp_signal_lookup_managed[sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE0_INPUT_ID+vj)].unit,
								    sranger_common_hwi->dsp_signal_lookup_managed[sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE0_INPUT_ID+vj)].scale
								    );
			
			// Scan Input Source Selection update

                        // $$$$$$$$$$$$$$$$$$$ FIX ME ??? GTK3QQQ
                        {
                                int n = gtk_combo_box_get_active (GTK_COMBO_BOX (VPScanSrcVPitem[i]));
                                gchar *id = g_strdup (gtk_combo_box_get_active_id (GTK_COMBO_BOX (VPScanSrcVPitem[i])));
                                gtk_combo_box_set_active (GTK_COMBO_BOX (VPScanSrcVPitem[i]), 0);

                                gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (VPScanSrcVPitem[i]), n);
                                gtk_combo_box_text_insert (GTK_COMBO_BOX_TEXT (VPScanSrcVPitem[i]), n, id, tmp);
                                gtk_combo_box_set_active (GTK_COMBO_BOX (VPScanSrcVPitem[i]), n);
                                g_free (id);
                        }

			// GtkWidget *mitem = (GtkWidget*)g_object_get_data( G_OBJECT (VPScanSrcVPitem[i]), "optionmenuwidget");
			// int n=gtk_option_menu_get_history (GTK_OPTION_MENU (mitem));
			// gtk_option_menu_set_history (GTK_OPTION_MENU (mitem), 1);
			// gtk_menu_item_set_label (GTK_MENU_ITEM (VPScanSrcVPitem[i]), tmp);
			// gtk_option_menu_set_history (GTK_OPTION_MENU (mitem), n);

			g_free(tmp);
		} else
			main_get_gapp()->channelselector->SetModeChannelSignal(17+i,
								    sranger_common_hwi->lookup_dsp_signal_managed (si)->label,
								    sranger_common_hwi->lookup_dsp_signal_managed (si)->label,
								    sranger_common_hwi->lookup_dsp_signal_managed (si)->unit,
								    sranger_common_hwi->lookup_dsp_signal_managed (si)->scale
								    );
	}
	// VPsig selection menu -- OK, no dependency.
}

int DSPControl::choice_scansource_callback (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) == -1 || g_object_get_data( G_OBJECT (widget), "updating_in_progress")){
                PI_DEBUG_GP (DBG_L4, "%s bail out for label refresh only\n",__FUNCTION__);
                return 0;
        }
            
	gint signal  = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        gint channel = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "scan_channel_source"));

	PI_DEBUG_GP (DBG_L3, "ScanSource-Input[%d]=0x%x  <== %s\n",channel,signal, sranger_common_hwi->dsp_signal_lookup_managed[signal].label);
	sranger_common_hwi->change_signal_input (signal, DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID+channel, dspc->DSP_vpdata_ij[0]*8+dspc->DSP_vpdata_ij[1]);
	dspc->scan_source[channel] = signal;

	// manage unit -- TDB
	//	dspc->scan_unit2volt_factor[channel] = 1.;

	//	dspc->update_sourcesignals_from_DSP_callback ();

	// verify and update:
	int si = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID+channel);

	if (sranger_common_hwi->lookup_dsp_signal_managed (si)->index >= 0){
		const gchar *vjfixedlab[] = { "Counter 0", "Counter 1", "NULL", "NULL" };
		int vi=sranger_common_hwi->lookup_dsp_signal_managed (si)->index/8;
		int vj=sranger_common_hwi->lookup_dsp_signal_managed (si)->index - 8*vi;
		gchar *tmp = g_strdup_printf("%s[%d]-%s",
					     sranger_common_hwi->lookup_dsp_signal_managed (si)->label, vi, 
					     vj < 4 
					     ? sranger_common_hwi->dsp_signal_lookup_managed[sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE0_INPUT_ID+vj)].label
					     : vjfixedlab[vj-4]
					     );
		main_get_gapp()->channelselector->SetModeChannelSignal(17+channel, tmp, tmp, 
							    sranger_common_hwi->dsp_signal_lookup_managed[sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE0_INPUT_ID+vj)].unit,
							    sranger_common_hwi->dsp_signal_lookup_managed[sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE0_INPUT_ID+vj)].scale
							    );

		// ****		gtk_combo_box_set_label (GTK_COMBO_BOX (widget), si, tmp);

                { 
                        gchar *id = g_strdup_printf ("%d", si);
                        g_object_set_data( G_OBJECT (widget), "updating_in_progress", GINT_TO_POINTER (1));
                        gtk_combo_box_set_active (GTK_COMBO_BOX (widget), -1);
                        gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (widget), si);
                        gtk_combo_box_text_insert (GTK_COMBO_BOX_TEXT (widget), si, id, tmp);
                        gtk_combo_box_set_active (GTK_COMBO_BOX (widget), si);
                        g_object_set_data( G_OBJECT (widget), "updating_in_progress", NULL);
                        g_free (id);
                }

		// GtkWidget *mitem = (GtkWidget*)g_object_get_data( G_OBJECT (widget), "menuwidget");
		// gtk_option_menu_set_history (GTK_OPTION_MENU (mitem), 1);
		// gtk_menu_item_set_label (GTK_MENU_ITEM (widget), tmp);
		// gtk_option_menu_set_history (GTK_OPTION_MENU (mitem), si);

		g_free(tmp);
	} else
	        main_get_gapp()->channelselector->SetModeChannelSignal(17+channel, 
							    sranger_common_hwi->lookup_dsp_signal_managed (si)->label,
							    sranger_common_hwi->lookup_dsp_signal_managed (si)->label,
							    sranger_common_hwi->lookup_dsp_signal_managed (si)->unit,
							    sranger_common_hwi->lookup_dsp_signal_managed (si)->scale
							    );
        return 0;
}

int DSPControl::choice_prbsource_callback (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
        
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) == -1)
                return 0;

        gint selection = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	gint channel   = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "prb_channel_source"));

	if (channel >= 4){
		switch (channel){
		case 4: PI_DEBUG_GP (DBG_L3, "Probe-Trigger-Input=0x%x  <== %s\n",selection, sranger_common_hwi->dsp_signal_lookup_managed[selection].label);
			sranger_common_hwi->change_signal_input(selection, DSP_SIGNAL_VECPROBE_TRIGGER_INPUT_ID);
			break;
		case 5: PI_DEBUG_GP (DBG_L3, "Probe-Limiter-Input=0x%x  <== %s\n",selection, sranger_common_hwi->dsp_signal_lookup_managed[selection].label);
			sranger_common_hwi->change_signal_input(selection, DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID);
			break;
		case 6: PI_DEBUG_GP (DBG_L3, "Probe-Tracker-Input=0x%x  <== %s\n",selection, sranger_common_hwi->dsp_signal_lookup_managed[selection].label);
			sranger_common_hwi->change_signal_input(selection, DSP_SIGNAL_VECPROBE_TRACKER_INPUT_ID);
			break;
		default: return -1;
		}
	} else {
		PI_DEBUG_GP (DBG_L3, "Probe-Input[%d]=0x%x  <== %s\n",channel,selection, sranger_common_hwi->dsp_signal_lookup_managed[selection].label);
		sranger_common_hwi->change_signal_input(selection, DSP_SIGNAL_VECPROBE0_INPUT_ID+channel);
                dspc->probe_source[channel] = selection;
                // g_message ("Probe Signal Input[%d] = %d -> %s", channel, dspc->probe_source[channel], sranger_common_hwi->lookup_dsp_signal_managed (dspc->probe_source[channel])->label);
	}
	dspc->probe_source[channel] = selection;

	if (channel < 4){
                if (!dspc->VPSig_menu)
                        return 0;
		// VPsig item update
		int si = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE0_INPUT_ID+channel);
                if (si != selection)
                        g_warning ("Failed to verify probe source selection:");
                const gchar *label = sranger_common_hwi->lookup_dsp_signal_managed (si)->label;
                g_message ("Probe Signal Input[%d] verify := %d -> %s", channel, si, label);

                gchar *id = g_strdup_printf ("%d", channel);
                //g_message ("Probe Signal Input[%d] id -> %s", channel, id);
                gint active = gtk_combo_box_get_active (GTK_COMBO_BOX (dspc->VPSig_menu));
                //g_message ("Probe Signal Input[%d] active -> %d -- deactivating", channel, active);
                gtk_combo_box_set_active (GTK_COMBO_BOX (dspc->VPSig_menu), 6); // -1); -- -1 is supposed to deactivate but it's crashing?? GTK3QQQ
                //g_message ("Probe Signal Input[%d] active -> %d -- removing", channel, active);
                gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (dspc->VPSig_menu), channel);
                //g_message ("Probe Signal Input[%d] active -> %d -- insering -> %s", channel, active, label);
                gtk_combo_box_text_insert (GTK_COMBO_BOX_TEXT (dspc->VPSig_menu), channel, id, label?label : "???");
                //g_message ("Probe Signal Input[%d] active -> %d -- activating.", channel, active);
                gtk_combo_box_set_active (GTK_COMBO_BOX (dspc->VPSig_menu), active);
                //g_message ("Probe Signal Input[%d] active -> free id.", channel);
                g_free (id);

                //g_message ("Probe Signal Input[%d] updating.", channel);
                dspc->update_sourcesignals_from_DSP_callback ();
	} else if (channel >= 4) {
		int si = -1;
		switch (channel){
		case 4:  si = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE_TRIGGER_INPUT_ID); break;
		case 5:  si = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID); break;
		case 6:  si = sranger_common_hwi->query_module_signal_input(DSP_SIGNAL_VECPROBE_TRACKER_INPUT_ID); break;
		default: return -1;
		}
                dspc->update_sourcesignals_from_DSP_callback ();
	}

	return 0;
}

int DSPControl::choice_lcksource_callback (GtkWidget *widget, DSPControl *dspc){
	gint channel=0;
        gint selection=0;

        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) == -1)
                return 0;
            
	selection = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	channel   = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "lck_channel_source"));

	PI_DEBUG_GP (DBG_L3, "LockIn-Input[%d]=0x%x  <== %s\n",channel,selection, sranger_common_hwi->dsp_signal_lookup_managed[selection].label);
	sranger_common_hwi->change_signal_input(selection, DSP_SIGNAL_LOCKIN_A_INPUT_ID+channel);
	dspc->lockin_input[channel] = selection;
        return 0;
}

int DSPControl::choice_Ampl_callback (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	gint i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	gint j = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "chindex"));
	switch(j){
	case 0: sranger_mk2_hwi_pi.app->xsm->Inst->VX(i);
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_DSP_OFFSET_ADDING)
			sranger_mk2_hwi_pi.app->xsm->Inst->VX0(i);
		break;
	case 1: sranger_mk2_hwi_pi.app->xsm->Inst->VY(i);
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_DSP_OFFSET_ADDING)
			sranger_mk2_hwi_pi.app->xsm->Inst->VY0(i);
		break;
	case 2: sranger_mk2_hwi_pi.app->xsm->Inst->VZ(i); 
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_DSP_OFFSET_ADDING)
			sranger_mk2_hwi_pi.app->xsm->Inst->VZ0(i); 
		break;
	case 3:
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING)
			sranger_mk2_hwi_pi.app->xsm->Inst->VX0(i);
		break;
	case 4:
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING)
			sranger_mk2_hwi_pi.app->xsm->Inst->VY0(i);
		break;
	case 5:
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING)
			sranger_mk2_hwi_pi.app->xsm->Inst->VZ0(i);
		break;
	}
	PI_DEBUG (DBG_L2, "Ampl: Ch=" << j << " i=" << i );
	sranger_mk2_hwi_pi.app->spm_range_check(NULL, sranger_mk2_hwi_pi.app);
	dspc->updateDSP();

        // set gain mirroring variable on DSP -- may be read out by any other tool like the MK3-SPD HV Amplifier tool

        switch(j){
        case 0: case 1: case 3: case 4:
                sranger_common_hwi->MovetoXY (0.,0.);
                break;
        case 2: case 5:
                // Z safety: ToDo
                break;
        }

        sranger_common_hwi->UpdateScanGainMirror ();

        
	return 0;
}

int DSPControl::callback_change_AC_option_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));

        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->AC_option_flags = (dspc->AC_option_flags & (~msk)) | msk;
	else
		dspc->AC_option_flags &= ~msk;

        dspc->set_tab_settings ("AC", dspc->AC_option_flags, dspc->AC_auto_flags, dspc->AC_glock_data);
        return 0;
}

int DSPControl::callback_change_AC_auto_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->AC_auto_flags = (dspc->AC_auto_flags & (~msk)) | msk;
	else
		dspc->AC_auto_flags &= ~msk;

	if (dspc->write_vector_mode == PV_MODE_AC)
		dspc->raster_auto_flags = dspc->AC_auto_flags;

        dspc->set_tab_settings ("AC", dspc->AC_option_flags, dspc->AC_auto_flags, dspc->AC_glock_data);
        return 0;
}

int DSPControl::callback_change_IV_option_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->IV_option_flags = (dspc->IV_option_flags & (~msk)) | msk;
	else
		dspc->IV_option_flags &= ~msk;

        dspc->set_tab_settings ("IV", dspc->IV_option_flags, dspc->IV_auto_flags, dspc->IV_glock_data);
        return 0;
}

int DSPControl::callback_change_IV_auto_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->IV_auto_flags = (dspc->IV_auto_flags & (~msk)) | msk;
	else
		dspc->IV_auto_flags &= ~msk;

	if (dspc->write_vector_mode == PV_MODE_IV)
		dspc->raster_auto_flags = dspc->IV_auto_flags;

        dspc->set_tab_settings ("IV", dspc->IV_option_flags, dspc->IV_auto_flags, dspc->IV_glock_data);
        return 0;
}

int DSPControl::callback_change_FZ_option_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->FZ_option_flags = (dspc->FZ_option_flags & (~msk)) | msk;
	else
		dspc->FZ_option_flags &= ~msk;

        dspc->set_tab_settings ("FZ", dspc->FZ_option_flags, dspc->FZ_auto_flags, dspc->FZ_glock_data);
        return 0;
}

int DSPControl::callback_change_FZ_auto_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->FZ_auto_flags = (dspc->FZ_auto_flags & (~msk)) | msk;
	else
		dspc->FZ_auto_flags &= ~msk;

	if (dspc->write_vector_mode == PV_MODE_FZ)
		dspc->raster_auto_flags = dspc->FZ_auto_flags;

        dspc->set_tab_settings ("FZ", dspc->FZ_option_flags, dspc->FZ_auto_flags, dspc->FZ_glock_data);
        return 0;
}

int DSPControl::callback_change_PL_option_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));

        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->PL_option_flags = (dspc->PL_option_flags & (~msk)) | msk;
	else
		dspc->PL_option_flags &= ~msk;

	if (dspc->write_vector_mode == PV_MODE_PL)
		dspc->raster_auto_flags = dspc->PL_auto_flags;

        dspc->set_tab_settings ("PL", dspc->PL_option_flags, dspc->PL_auto_flags, dspc->PL_glock_data);
        return 0;
}

int DSPControl::callback_change_PL_auto_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	long msk = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->PL_auto_flags = (dspc->PL_auto_flags & (~msk)) | msk;
	else
		dspc->PL_auto_flags &= ~msk;

        dspc->set_tab_settings ("PL", dspc->PL_option_flags, dspc->PL_auto_flags, dspc->PL_glock_data);
        return 0;
}

int DSPControl::callback_change_LP_option_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->LP_option_flags = (dspc->LP_option_flags & (~msk)) | msk;
	else
		dspc->LP_option_flags &= ~msk;
 
	if (dspc->write_vector_mode == PV_MODE_LP)
		dspc->raster_auto_flags = dspc->LP_auto_flags;

        dspc->set_tab_settings ("LP", dspc->LP_option_flags, dspc->LP_auto_flags, dspc->LP_glock_data);
        return 0;
}
 
int DSPControl::callback_change_LP_auto_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	long msk = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->LP_auto_flags = (dspc->LP_auto_flags & (~msk)) | msk;
	else
		dspc->LP_auto_flags &= ~msk;
 
        dspc->set_tab_settings ("LP", dspc->LP_option_flags, dspc->LP_auto_flags, dspc->LP_glock_data);
        return 0;
}

int DSPControl::callback_change_SP_option_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->SP_option_flags = (dspc->SP_option_flags & (~msk)) | msk;
	else
		dspc->SP_option_flags &= ~msk;

	if (dspc->write_vector_mode == PV_MODE_SP)
		dspc->raster_auto_flags = dspc->SP_auto_flags;

        dspc->set_tab_settings ("SP", dspc->SP_option_flags, dspc->SP_auto_flags, dspc->SP_glock_data);
        return 0;
}

int DSPControl::callback_change_SP_auto_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	long msk = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->SP_auto_flags = (dspc->SP_auto_flags & (~msk)) | msk;
	else
		dspc->SP_auto_flags &= ~msk;

        dspc->set_tab_settings ("SP", dspc->SP_option_flags, dspc->SP_auto_flags, dspc->SP_glock_data);
        return 0;
}

int DSPControl::callback_change_TS_option_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->TS_option_flags = (dspc->TS_option_flags & (~msk)) | msk;
	else
		dspc->TS_option_flags &= ~msk;

	if (dspc->write_vector_mode == PV_MODE_TS)
		dspc->raster_auto_flags = dspc->TS_auto_flags;

        dspc->set_tab_settings ("TS", dspc->TS_option_flags, dspc->TS_auto_flags, dspc->TS_glock_data);
        return 0;
}

int DSPControl::callback_change_TS_auto_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->TS_auto_flags = (dspc->TS_auto_flags & (~msk)) | msk;
	else
		dspc->TS_auto_flags &= ~msk;

        dspc->set_tab_settings ("TS", dspc->TS_option_flags, dspc->TS_auto_flags, dspc->TS_glock_data);
        return 0;
}


int DSPControl::callback_change_GVP_option_flags (GtkWidget *widget, DSPControl *dspc){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->GVP_option_flags = (dspc->GVP_option_flags & (~msk)) | msk;
	else
		dspc->GVP_option_flags &= ~msk;

	if (dspc->write_vector_mode == PV_MODE_GVP)
		dspc->raster_auto_flags = dspc->GVP_auto_flags;

        dspc->set_tab_settings ("LM", dspc->GVP_option_flags, dspc->GVP_auto_flags, dspc->GVP_glock_data);
        dspc->set_tab_settings ("GVP", dspc->GVP_option_flags, dspc->GVP_auto_flags, dspc->GVP_glock_data);
        dspc->GVP_store_vp ("LM_set_last"); // last in view
        dspc->GVP_store_vp ("GVP_set_last"); // last in view
        return 0;
}

int DSPControl::callback_change_GVP_vpc_option_flags (GtkWidget *widget, DSPControl *dspc){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	int  k = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "VPC"));
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));

	if( gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->GVP_opt[k] = (dspc->GVP_opt[k] & (~msk)) | msk;
	else
		dspc->GVP_opt[k] &= ~msk;

        dspc->set_tab_settings ("LM", dspc->GVP_option_flags, dspc->GVP_auto_flags, dspc->GVP_glock_data);
        return 0;
}

int DSPControl::callback_update_GVP_vpc_option_checkbox (GtkWidget *widget, DSPControl *dspc){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	int  k   = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "VPC"));
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	gtk_check_button_set_active (GTK_CHECK_BUTTON(widget), (dspc->GVP_opt[k] & msk) ? 1:0);

        dspc->set_tab_settings ("LM", dspc->GVP_option_flags, dspc->GVP_auto_flags, dspc->GVP_glock_data);
        return 0;
}

int DSPControl::callback_edit_GVP (GtkWidget *widget, DSPControl *dspc){
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
	dspc->update ();
        return 0;
}

int DSPControl::callback_GVP_store_vp (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->GVP_store_vp ((const gchar*)g_object_get_data(G_OBJECT(widget), "key"));
        return 0;
}
int DSPControl::callback_GVP_restore_vp (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->GVP_restore_vp ((const gchar*)g_object_get_data(G_OBJECT(widget), "key"));
        return 0;
}

int DSPControl::callback_change_GVP_auto_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->GVP_auto_flags = (dspc->GVP_auto_flags & (~msk)) | msk;
	else
		dspc->GVP_auto_flags &= ~msk;

        dspc->set_tab_settings ("LM", dspc->GVP_option_flags, dspc->GVP_auto_flags, dspc->GVP_glock_data);
        return 0;
}

int DSPControl::callback_change_TK_ref(GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->TK_ref = atoi (gtk_combo_box_get_active_id( GTK_COMBO_BOX (widget)));
        //	dspc->TK_ref = atoi (gtk_combo_box_get_active_id( GTK_COMBO_BOX (widget), "TK_ref_id"));
	PI_DEBUG (DBG_L4, "TK-ref set to: " << dspc->TK_ref );

        dspc->set_tab_settings ("TK", dspc->TK_option_flags, dspc->TK_auto_flags, dspc->TK_glock_data);
        return 0;
}

int DSPControl::callback_change_FZ_ref(GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->FZ_limiter_ch = atoi (gtk_combo_box_get_active_id ( GTK_COMBO_BOX (widget)));
	PI_DEBUG (DBG_L4, "FZ-limiter-ch set to: " << dspc->FZ_limiter_ch );
        return 0;
}

int DSPControl::callback_change_TK_option_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->TK_option_flags = (dspc->TK_option_flags & (~msk)) | msk;
	else
		dspc->TK_option_flags &= ~msk;

	if (dspc->write_vector_mode == PV_MODE_TK)
		dspc->raster_auto_flags = dspc->TK_auto_flags;

        dspc->set_tab_settings ("TK", dspc->TK_option_flags, dspc->TK_auto_flags, dspc->TK_glock_data);
        return 0;
}

int DSPControl::callback_change_TK_auto_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT  (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->TK_auto_flags = (dspc->TK_auto_flags & (~msk)) | msk;
	else
		dspc->TK_auto_flags &= ~msk;

        dspc->set_tab_settings ("TK", dspc->TK_option_flags, dspc->TK_auto_flags, dspc->TK_glock_data);
        return 0;
}

int DSPControl::callback_change_AX_option_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT  (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->AX_option_flags = (dspc->AX_option_flags & (~msk)) | msk;
	else
		dspc->AX_option_flags &= ~msk;

	if (dspc->write_vector_mode == PV_MODE_AX)
		dspc->raster_auto_flags = dspc->AX_auto_flags;
        return 0;
}

int DSPControl::callback_change_AX_auto_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT  (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->AX_auto_flags = (dspc->AX_auto_flags & (~msk)) | msk;
	else
		dspc->AX_auto_flags &= ~msk;

        dspc->set_tab_settings ("AX", dspc->AX_option_flags, dspc->AX_auto_flags, dspc->AX_glock_data);
        return 0;
}

int DSPControl::callback_change_ABORT_option_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT  (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->ABORT_option_flags = (dspc->ABORT_option_flags & (~msk)) | msk;
	else
		dspc->ABORT_option_flags &= ~msk;

	if (dspc->write_vector_mode == PV_MODE_ABORT)
		dspc->raster_auto_flags = dspc->ABORT_auto_flags;
        return 0;
}

int DSPControl::callback_change_ABORT_auto_flags (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT  (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		dspc->ABORT_auto_flags = (dspc->ABORT_auto_flags & (~msk)) | msk;
	else
		dspc->ABORT_auto_flags &= ~msk;

        return 0;
}

int DSPControl::DSP_multiBias_callback (GtkWidget *widget, DSPControl *dspc){
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "DSP_multiBias_list"),
				(GFunc) gtk_widget_show, NULL);
	else
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "DSP_multiBias_list"),
				(GFunc) gtk_widget_hide, NULL);

	dspc->multiBias_mode = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
        return 0;
}

int DSPControl::DSP_multiIV_callback (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "DSP_multiIV_list"),
				(GFunc) gtk_widget_show, NULL);
	else
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "DSP_multiIV_list"),
				(GFunc) gtk_widget_hide, NULL);

	dspc->multiIV_mode = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
        return 0;
}

int DSPControl::DSP_slope_callback (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->area_slope_compensation_flag = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));	
	dspc->updateDSP();
        return 0;
}

int DSPControl::DSP_cret_callback (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->center_return_flag = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));	
        return 0;
}

int DSPControl::change_source_callback (GtkWidget *widget, DSPControl *dspc){
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

int DSPControl::callback_XJoin (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->XJoin = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? TRUE : FALSE;
	dspc->vis_XJoin = dspc->XJoin;
        g_settings_set_boolean (dspc->hwi_settings, "probe-x-join", dspc->XJoin);
        return 0;
}

int DSPControl::callback_GrMatWindow (GtkWidget *widget, DSPControl *dspc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	dspc->GrMatWin = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? TRUE : FALSE;
        g_settings_set_boolean (dspc->hwi_settings, "probe-graph-matrix-window", dspc->GrMatWin);
        return 0;
}
