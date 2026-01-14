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

#include <config.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "gnome-res.h"
#include "gxsm_app.h"

#include "dataio.h"
#include "util.h"
#include "glbvars.h"

#include "gxsm_resoucetable.h"
#include "surface.h"

#include "action_id.h"

//#include "tips_dialog.h"

/* File ================================================== */

void App::file_open_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	if(!gapp) return;
	main_get_gapp ()->xsm->load();
	return;
}

void App::file_open_in_new_window_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	if(!gapp) return;
	if(!main_get_gapp ()->xsm->ActivateFreeChannel())
		main_get_gapp ()->xsm->load();
	return;
}

void App::file_open_mode_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
	if(!gapp) return;

        GVariant *old_state, *new_state;
        
        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_string (g_variant_get_string (parameter, NULL));
                
        if (!strcmp (g_variant_get_string (new_state, NULL), "replace")){
                main_get_gapp ()->xsm->file_open_mode = xsm::open_mode::replace;
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "append-time")){
                main_get_gapp ()->xsm->file_open_mode = xsm::open_mode::append_time;
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "stitch-2d")){
                main_get_gapp ()->xsm->file_open_mode = xsm::open_mode::stitch_2d;
        } else g_warning ("App::file_open_mode_callback -- unhandled action.");
      
        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);
        
	return;
}

void App::file_browse_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	if(!gapp) return;

        //GtkFileFilter *filter = gtk_file_filter_new ();
        //gtk_file_filter_set_name (filter, "GXSM NetCDF or other");
        //gtk_file_filter_add_pattern (filter, "*.[nNdDsSh][cCaApPd]*");
 
	return;
}

void App::browse_callback(gchar *selection, App* ap){ 
	if(!gapp) return;
	XSM_DEBUG(DBG_L2, "browsed:" << selection );
	if(selection)
		ap->xsm->load(selection);
}

void App::file_set_datapath_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	if(!gapp) return;
	main_get_gapp ()->xsm->save (CHANGE_PATH_ONLY);
	return;
}

void App::file_set_probepath_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	XsmRescourceManager xrm("FilingPathMemory");
	gchar *path = xrm.GetStr ("Probe_DataSavePath", xsmres.DataPath);
	gchar *newpath = main_get_gapp ()->file_dialog_load (N_("Select path for probe data files"), path, NULL);

	g_free (path);
	if (newpath){
		xrm.Put ("Probe_DataSavePath", newpath);
	}
	return;
}


void App::auto_save_scans (){ // auto save or update scan(s) in progress or completed. No overwrite question, auto counter advance!
	if(!gapp) return;

        // use new auto safe
        int maxcounter_until_ask = xsm->GetFileCounter()+100;
 	for (GSList* tmp = main_get_gapp ()->xsm->GetActiveScanList(); tmp; tmp = g_slist_next (tmp)){ // for all current scans in progress or completed last
                if (((Scan*)tmp->data)->get_channel_id () >= 0){
                        if (main_get_gapp ()->xsm->ChannelASflag[((Scan*)tmp->data)->get_channel_id ()]){ // if maked for autos save (AS)
                                // full save, no user interaction -- no overwrite, but auto couter advance until clear to go. But with prev. updated file, do overwrite!
                                while (((Scan*)tmp->data)->Save (main_get_gapp ()->auto_update_all)){
                                        xsm->FileCounterInc (); // try next counter
                                        if (!main_get_gapp ()->is_thread_safe_no_gui_mode())
                                                spm_update_all();
                                        for (GSList* tmpAdjustCounter = main_get_gapp ()->xsm->GetActiveScanList(); tmpAdjustCounter; tmpAdjustCounter = g_slist_next (tmpAdjustCounter))
                                                ((Scan*)tmpAdjustCounter->data)->storage_manager.set_dataset_counter (xsm->GetFileCounter ());
                                        // may add a safety bail out??? Even should end some time....?
                                        if (xsm->GetFileCounter () >= maxcounter_until_ask){
                                                if (question_yes_no ("File Counter reached large count and still file exists? Continue?"))
                                                        maxcounter_until_ask += 100; // try few more
                                                else
                                                        break; // skip
                                        }
                                }
                                // check my python auto save hook
                                {
                                        gchar *asch = g_strdup_printf ("auto-save-ch%02d", ((Scan*)tmp->data)->get_channel_id ());
                                        g_message ("PyRemote Action Trigger: %s", asch);
                                        main_get_gapp ()->SignalRemoteActionToPlugins (&asch);
                                        g_free (asch);
                                }
                        }
                }
        }
        if (!main_get_gapp ()->is_thread_safe_no_gui_mode())
                spm_update_all();
}

void App::auto_update_scans (){ // auto save or update scan(s) in progress or completed.
	if(!gapp) return;

	for (GSList* tmp = main_get_gapp ()->xsm->GetActiveScanList(); tmp; tmp = g_slist_next (tmp)) // for all current scans in progress or completed last
		if (((Scan*)tmp->data)->get_channel_id () >= 0)
                        if (main_get_gapp ()->xsm->ChannelASflag[((Scan*)tmp->data)->get_channel_id ()]) // if maked for autos save (AS)
                                ((Scan*)tmp->data)->Update_ZData_NcFile ();

        main_get_gapp ()->set_toolbar_autosave_button (true);
        return;
}

void App::file_save_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	if(!gapp) return;

        if (main_get_gapp ()->auto_update_all){
                main_get_gapp ()->auto_update_scans ();
                return;
        }

        int i=0;
	for (GSList* tmp = main_get_gapp ()->xsm->GetActiveScanList(); tmp; tmp = g_slist_next (tmp)){ // for all current scans in progress or completed last
		if (((Scan*)tmp->data)->get_channel_id () >= 0){
                        if (main_get_gapp ()->xsm->ChannelASflag[((Scan*)tmp->data)->get_channel_id ()]){ // if maked for autos save (AS)
                                if (((Scan*)tmp->data)->Save ()){ // returns -1 if file exists and does nothing, else it's saved (or error)
                                        GtkWidget *dialog = gtk_message_dialog_new (main_get_gapp ()->get_window (),
                                                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                                                    GTK_MESSAGE_WARNING,
                                                                                    GTK_BUTTONS_YES_NO,
                                                                                    N_("File '%s' exists or can't be written, try overwrite?\nOr apply auto counter advance?"),
                                                                                    ((Scan*)tmp->data)->storage_manager.get_filename());
                                        
                                        if (i==0) gtk_dialog_add_button (GTK_DIALOG (dialog), "_Apply", GTK_RESPONSE_APPLY); // 1st time offer auto counter advance option

                                        g_signal_connect (dialog, "response", G_CALLBACK (gtk_window_destroy), NULL);
                                        gtk_widget_show (dialog);

                                        // FIX-ME-GTK4 -- overwrite always for now
                                        int overwrite =  GTK_RESPONSE_YES; 
                                        //int overwrite = gtk_dialog_run (GTK_DIALOG (dialog));
                                        //gtk_widget_destroy (dialog);

                                        switch (overwrite){
                                        case GTK_RESPONSE_NO:
                                                main_get_gapp ()->SetStatus(N_("File exists, save aborted by user."));
                                                continue; // skip!
                                        case GTK_RESPONSE_YES:
                                                ((Scan*)tmp->data)->Save (true); // force overwrite
                                                break;
                                        case GTK_RESPONSE_APPLY:
                                                main_get_gapp ()->auto_save_scans ();
                                                break;
                                        }
                                }
                                ++i;
                        }
                }
        }

        // FIX-ME-GTK4 GTK_TOOL_BUTTON
        main_get_gapp ()->tool_button_save_all = GTK_WIDGET (g_object_get_data (G_OBJECT (simple), "toolbar_button"));
        main_get_gapp ()->set_toolbar_autosave_button (true);
	return;
}

void App::set_toolbar_autosave_button (gboolean update_mode){
        if (tool_button_save_all){
                if (update_mode){
                        if (!main_get_gapp ()->is_thread_safe_no_gui_mode())
                                gtk_button_set_icon_name (GTK_BUTTON (tool_button_save_all), "view-refresh-symbolic");
                        auto_update_all = true;
                }else{
                        if (!main_get_gapp ()->is_thread_safe_no_gui_mode())
                                gtk_button_set_icon_name (GTK_BUTTON (tool_button_save_all), "document-save-symbolic");
                        auto_update_all = false;
                }
        }
}
        
void App::file_update_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	if(!gapp) return;
        main_get_gapp ()->auto_update_scans ();
	return;
}

void App::file_save_as_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	if(!gapp) return;
	main_get_gapp ()->xsm->save(MANUAL_SAVE_AS);
	return;
}

void App::file_print_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
#if 0
	if (main_get_gapp ()->PluginCallPrinter)
		(*main_get_gapp ()->PluginCallPrinter) (widget, data);
	else
		main_get_gapp ()->message(N_("Sorry, no 'Printer' plugin loaded!"));
#endif
	return;
}

void App::file_close_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	//  main_get_gapp ()->xsm->??
	return;
}


void App::file_quit_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
        gint r=true;
	if(!gapp) return;

	if (main_get_gapp ()->question_yes_no_with_action (Q_WANTQUIT, "Save Window Geometry", r) == GTK_RESPONSE_YES){
                if (r)
                        main_get_gapp ()->save_app_geometry ();

                // Unload and cleanup Plugins
                g_message ("Unloading Gxsm Plugins");
                delete gapp->GxsmPlugins;
		gapp->GxsmPlugins = NULL;

                g_message ("Unloading Gxsm HwI Plugin");
                gapp->xsm->reload_hardware_interface (NULL); // unload only
                // ---
                
                GApplication *application = (GApplication *) user_data;
                delete gapp;
                gapp=NULL;
                g_application_quit (application);
	}
}

/* Action ================================================== */


void App::action_toolbar_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
        const gchar *action = (const gchar*) g_object_get_data (G_OBJECT (simple), "toolbar_action");
        if (!action){
		XSM_DEBUG(DBG_L2, "Toolbar Plugin \"toolbar_action\" string is not set. (NULL)" );
                return;
        }
        if (!gapp) return;

        main_get_gapp ()->signal_emit_toolbar_action (action, simple);
        return;
}

/* Math, Filter, ... ================================================== */

typedef gboolean  (*MOpND)(MATHOPPARAMSNODEST);
#define M_OPND(f)                    ((MOpND) (f))

typedef gboolean  (*MOp)(MATHOPPARAMS);
#define M_OP(f)                    ((MOp) (f))

typedef gboolean  (*MOpDO)(MATHOPPARAMDONLY);
#define M_OPDO(f)                    ((MOpDO) (f))

typedef gboolean  (*M2Op)(MATH2OPPARAMS);
#define M_2OP(f)                   ((M2Op) (f))

void App::math_onearg_nodest_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){ // gboolean (*MOp)(MATHOPPARAMSNODEST)
        main_get_gapp ()->xsm->MathOperationNoDest (M_OPND (g_object_get_data (G_OBJECT (simple), "plugin_MOp")));
	return;
}

void App::math_onearg_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){ // gboolean (*MOp)(MATHOPPARAMS)){
        main_get_gapp ()->xsm->MathOperation (M_OP (g_object_get_data (G_OBJECT (simple), "plugin_MOp")));
	return;
}

void App::math_onearg_dest_only_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){ // gboolean (*MOpDO)(MATHOPPARAMDONLY)){
        main_get_gapp ()->xsm->MathOperationS (M_OPDO (g_object_get_data (G_OBJECT (simple), "plugin_MOp")));
	return;
}

void App::math_onearg_for_all_vt_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){ // gboolean (*MOp)(MATHOPPARAMS)){
	main_get_gapp ()->xsm->MathOperation_for_all_vt (M_OP (g_object_get_data (G_OBJECT (simple), "plugin_MOp")));
        return;
}

void App::math_twoarg_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){ //  gboolean (*MOp)(MATH2OPPARAMS)){
	main_get_gapp ()->xsm->MathOperationX (M_2OP (g_object_get_data (G_OBJECT (simple), "plugin_MOp")), ID_CH_M_X, TRUE);
	return;
}

void App::math_twoarg_no_same_size_check_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){ // gboolean (*MOp)(MATH2OPPARAMS)){
	main_get_gapp ()->xsm->MathOperationX (M_2OP (g_object_get_data (G_OBJECT (simple), "plugin_MOp")), ID_CH_M_X, FALSE);
	return;
}

/* Tools ================================================== */

void App::tools_monitor_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	main_get_gapp ()->monitorcontrol->show();
	return;
}

void App::tools_chanselwin_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	main_get_gapp ()->channelselector->show();
	return;
}

void App::tools_plugin_reload_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	main_get_gapp ()->reload_gxsm_plugins();
	return;
}

void App::tools_plugin_info_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	if(gapp){
		if(main_get_gapp ()->GxsmPlugins)
			main_get_gapp ()->GxsmPlugins->view_pi_info();
		else
			main_get_gapp ()->message(N_("No Plugins loaded!"));
	}
	return;
}

/* Preferenes, etc. ========================================== */

void App::options_preferences_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gxsm_search_for_palette();
	gxsm_search_for_HwI();
	GnomeResPreferences *pref = gnome_res_preferences_new (xsm_res_def, GXSM_RES_PREFERENCES_PATH);
        gnome_res_set_ok_message (pref,
                                  "Complete configuration update on gxsm restart only.\n"
                                  "On scan start the following instrument parameters are updated:"
                                  "Piezo Sensitivities, Bias Gain/Offset, Current Gain/Modifier,\n Force Scaling, Freq. Scaling, eVolt Scaling ");
        gnome_res_set_apply_message (pref,
                                  "Complete configuration update on gxsm restart only.\n"
                                  "On scan start the following instrument parameters are updated:"
                                  "Piezo Sensitivities, Bias Gain/Offset, Current Gain/Modifier,\n Force Scaling, Freq. Scaling, eVolt Scaling ");
        gnome_res_read_user_config (pref);
	gnome_res_run_change_user_config (pref, N_("Gxsm Preferences")); 
// on Dlg close pref is destroyed!

        main_get_gapp ()->xsm->hardware->hwi_init_overrides ();
        
	return;
}

void App::save_geometry_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
        main_get_gapp ()->save_app_geometry ();
}

void App::load_geometry_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
        main_get_gapp ()->load_app_geometry ();
}

void App::auto_scanview_geometry_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
        main_get_gapp ()->auto_scanview_geometry ();
}

void App::auto_probeview_geometry_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
        main_get_gapp ()->auto_probeview_geometry ();
}

/* Help ================================================== */
void App::help_about_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
        App *app = (App*)user_data;
	gchar *message;
	const gchar *authors[] = {
		/* Here should be your names */
		"Percy Zahl  (zahl@users.sourceforge.net)",
		"Andreas Klust  (klust@users.sourceforge.net)",
		"Thorsten Wagener (stm@users.sourceforge.net)",
		"Stefan Schroeder  (stefan_fkp@users.sourceforge.net)",
		"Juan de la Figuera  (johnnybegood@users.sourceforge.net)",
		"and others https://gxsm.sourceforge.net, https://github.com/pyzahl/Gxsm4",
		NULL
	};
	
	const gchar *documenters[] = {
		/* Here should be the documenters names */
		"Percy Zahl",
		"Andreas Klust",
		"Stefan Schroeder",
		NULL
	};
	
#ifdef XSM_DEBUG_OPTION
	gchar *dbg_lvl_tmp = g_strdup_printf ("\n\nGXSM / PlugIn debug level is: %d/%d", main_get_debug_level (), main_get_pi_debug_level ());
#else
	gchar *dbg_lvl_tmp = g_strdup_printf ("\n\nGXSM debug support is disabled.");
#endif

        gchar *hw_info_stripped = g_strdup (main_get_gapp ()->xsm->hardware->Info(0));
        gchar *p=hw_info_stripped;
        gchar *dirs=NULL;

        g_message ("HWInfo:\n%s", hw_info_stripped);
        
        for (int line=0; line<10 && *p; ++line, ++p)
                while (*p && *p != '\n') ++p;
        
        *p = 0; // terminate here

        if (main_get_debug_level () > 1)
                dirs = g_strconcat(
                                   "\nGXSM_RES_BASE_PATH_DOT:\n", GXSM_RES_BASE_PATH_DOT,
                                   "\nBINDIR:\n", M__BINDIR,
                                   "\nBUILD_DATADIR:\n", M__BUILD_DATADIR,
                                   "\nBUILD_PREFIX:\n", M__BUILD_PREFIX,
                                   "\nDATAROOTDIR:\n", M__DATAROOTDIR,
                                   "\nINFODIR:\n", M__INFODIR,
                                   "\nLIBDIR:\n", M__LIBDIR,
                                   "\nPACKAGE_GL400_DIR:\n", PACKAGE_GL400_DIR,
                                   "\nPACKAGE_ICON_DIR:\n", PACKAGE_ICON_DIR,
                                   "\nPACKAGE_PALETTE_DIR:\n", PACKAGE_PALETTE_DIR,
                                   "\nPACKAGE_PLUGIN_DIR:\n", PACKAGE_PLUGIN_DIR,
                                   "\nPACKAGE_PROFILEPLOT_DIR:\n", PACKAGE_PROFILEPLOT_DIR,
                                   "\n\n",
                                   NULL);
        
	message = g_strconcat
		(N_("GXSM is a Universal Scanning Probe Micoscopy Data Aquisitation and Visulaization System."
		   "\nBuild for GTK4."
		   "\nUsing NetCDF4."
		   "\n\nHardware: "),
		 hw_info_stripped, "\n[...] (see terminal for more)\n",
		 dbg_lvl_tmp,
		 N_("\n\ncompiled by "),
		 COMPILEDBYNAME,
		 dirs ? N_("\n\nCONFIGURED PATHs:") : "\n",
		 dirs ? dirs : "\n",
		 N_("\n\nGXSM4 evolved from Gxsm-3.0,2.0, Xxsm, pmstm in 1997."),
		 NULL);
        g_free (hw_info_stripped);
	g_free (dbg_lvl_tmp);
        if (dirs) g_free (dirs);
        
	gtk_show_about_dialog (GTK_WINDOW (app->app_window),
			       "program-name", "GXSM4",
			       "version", VERSION " \"" GXSM_VERSION_NAME "\"",
			       "title", N_("About GXSM"),
			       "authors", authors,
			       "documenters", documenters,
			       "translator-credits", "Juan de la Figuera",
			       "copyright", "(C) PyZahl et al 2000-2025",
			       "website", "https://github.com/pyzahl/Gxsm4",
                               "comments", message,
                               "license-type", GTK_LICENSE_GPL_3_0,
                               "logo-icon-name", "gxsm4-icon", //"org.gtk.Demo4",
			       NULL);
        
        g_free(message);
	return;
}

/* Diverse CallBack Handler */

gint App::close_scan_event_cb(GtkWidget *window){
	if(main_get_gapp ()->xsm->SetMode ((long) GPOINTER_TO_INT (g_object_get_data (G_OBJECT (window), "Ch")), ID_CH_M_OFF))
		return FALSE;
	else
		return TRUE;
}
