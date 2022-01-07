/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
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

/*
 * Project: Gxsm
 */

#include "app_monitorcontrol.h"
#include "glbvars.h"
#include "gxsm_window.h"

typedef struct{
  const gchar *label;
  gint  width;
  gchar *data;
} MONITORLOGENTRY;

#define MAXCOLUMNS 10
MONITORLOGENTRY tabledef[] = {
  { "time stamp", 150, NULL },
  { "action",  100, NULL },
  { "comment", 200, NULL },
  { "value1", 100, NULL },
  { "value2", 100, NULL },
  { "value3", 100, NULL },
  { NULL, 0, NULL }
};


static GActionEntry win_monitor_entries[] = {
        //        { "file-open", MonitorControl::file_open_callback, NULL, NULL, NULL },
        { "file-save", MonitorControl::file_save_callback, NULL, NULL, NULL },
        //        { "file-close", MonitorControl::file_close_callback, NULL, NULL, NULL },
        //        { "xxx", MonitorControl::xxx_action_callback, NULL, "true", NULL },
        { "logging-history", MonitorControl::set_logging_history_radio_callback, "s", "'max-500-lines'", NULL },
        { "logging-level", MonitorControl::set_logging_level_radio_callback, "s", "'normal'", NULL },
};

MonitorControl::MonitorControl (Gxsm4app *app, gint loglevel, gint maxlines):Monitor(loglevel), AppBase(app){
        set_max_lines (maxlines);

        AppWindowInit (N_("GXSM Activity Monitor and Logbook"));

        log_view = gtk_text_view_new ();
        gtk_text_view_set_editable (GTK_TEXT_VIEW (log_view), FALSE);
        log_buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (log_view));
        GtkTextIter end_iter;
        gtk_text_buffer_get_end_iter (log_buf, &end_iter);
        text_end_mark = gtk_text_buffer_create_mark (log_buf, "cursor", &end_iter, false);

        GtkWidget *scrolled_window = gtk_scrolled_window_new ();
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                        GTK_POLICY_AUTOMATIC,
                                        GTK_POLICY_AUTOMATIC);
        //gtk_scrollde_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
        gtk_widget_set_hexpand (scrolled_window, TRUE);
        gtk_widget_set_vexpand (scrolled_window, TRUE);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled_window), log_view);
        gtk_grid_attach (GTK_GRID (v_grid), scrolled_window, 1,1, 10,1);
        gtk_widget_show (scrolled_window); // FIX-ME GTK4 SHOWALL
        gtk_widget_show (log_view); // FIX-ME GTK4 SHOWALL
        gtk_widget_show (v_grid); // FIX-ME GTK4 SHOWALL

        set_window_geometry ("monitor");

        LogEvent ("MonitorControl", "startup");
}

MonitorControl::~MonitorControl (){
        LogEvent ("MonitorControl", "End of Session Log. Exit OK.");
}


void MonitorControl::AppWindowInit(const gchar *title, const gchar *sub_title){
	XSM_DEBUG (DBG_L2,  "MonitorControl::AppWindowInit -- header bar,..." );

        g_message ("MonitorControl::AppWindowInit** <%s : %s> **", title, sub_title);
        app_window = gxsm4_app_window_new (gxsm4app);
        window = GTK_WINDOW (app_window);

        header_bar = gtk_header_bar_new ();
        gtk_widget_show (header_bar);
        //gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header_bar), true);

        // link view popup actions
        g_action_map_add_action_entries (G_ACTION_MAP (app_window),
                                         win_monitor_entries, G_N_ELEMENTS (win_monitor_entries),
                                         this);
        //GtkWidget *monitor_menu = gtk_menu_new_from_model (G_MENU_MODEL (main_get_gapp ()->get_monitor_menu ()));
        //g_assert (GTK_IS_MENU (monitor_menu));
        GtkWidget *monitor_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (main_get_gapp ()->get_monitor_menu ()));

	// GtkIconSize tmp_toolbar_icon_size = GTK_ICON_SIZE_LARGE_TOOLBAR;
      
        GtkWidget *header_menu_button = gtk_menu_button_new ();
        // gtk_button_set_image (GTK_BUTTON (header_menu_button), gtk_image_new_from_icon_name ("emblem-system-symbolic", tmp_toolbar_icon_size));
        gtk_menu_button_set_popover (GTK_MENU_BUTTON (header_menu_button), monitor_menu);
        gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), header_menu_button);
        gtk_widget_show (header_menu_button);

        SetTitle (title, sub_title);
        gtk_window_set_titlebar (GTK_WINDOW (window), header_bar);

	v_grid = gtk_grid_new ();
        gtk_window_set_child (GTK_WINDOW (window), v_grid);
	g_object_set_data (G_OBJECT (window), "v_grid", v_grid);

	gtk_widget_show (GTK_WIDGET (window)); // FIX-ME GTK4 SHOWALL

}

void MonitorControl::file_open_callback (GSimpleAction *action, GVariant *parameter, 
                                         gpointer user_data){
}

// save buffer to file
void MonitorControl::file_save_callback (GSimpleAction *action, GVariant *parameter, 
                                         gpointer user_data){
        MonitorControl *mc = (MonitorControl *) user_data;

        std::ofstream f;
        gchar *fname = main_get_gapp ()->file_dialog_save ("Save Monitor Log Buffer", NULL, NULL);

        f.open (fname, std::ios::out);

        g_free (fname);
        
        if(!f.good ())
                return;
    
        GtkTextIter start_iter, end_iter;
        gtk_text_buffer_get_bounds (mc->log_buf, &start_iter, &end_iter);
        GString *output = g_string_new (gtk_text_buffer_get_text (mc->log_buf,
                                                                  &start_iter, &end_iter,
                                                                  false));
        f << output->str;
        f.close ();

        g_string_free (output, true);
}

void MonitorControl::file_close_callback (GSimpleAction *action, GVariant *parameter, 
                                          gpointer user_data){
}

void MonitorControl::set_logging_history_radio_callback (GSimpleAction *action, GVariant *parameter, 
                                                         gpointer user_data){
        MonitorControl *mc = (MonitorControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_string (g_variant_get_string (parameter, NULL));
                
        XSM_DEBUG_GP (DBG_L1, "MonitorControl History Radio action %s activated, state changes from %s to %s\n",
                      g_action_get_name (G_ACTION (action)),
                      g_variant_get_string (old_state, NULL),
                      g_variant_get_string (new_state, NULL));
        
        if (!strcmp (g_variant_get_string (new_state, NULL), "off"))
                mc->set_max_lines (-1);
        else if (!strcmp (g_variant_get_string (new_state, NULL), "max-10-lines"))
                mc->set_max_lines (10);
        else if (!strcmp (g_variant_get_string (new_state, NULL), "max-50-lines"))
                mc->set_max_lines (50);
        else if (!strcmp (g_variant_get_string (new_state, NULL), "max-500-lines"))
                mc->set_max_lines (500);
        else if (!strcmp (g_variant_get_string (new_state, NULL), "max-5000-lines"))
                mc->set_max_lines (5000);
        else if (!strcmp (g_variant_get_string (new_state, NULL), "infinit"))
                mc->set_max_lines (0);
        else // fallback
                mc->set_max_lines (100);
        
        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);
}


void MonitorControl::set_logging_level_radio_callback (GSimpleAction *action, GVariant *parameter, 
                                                       gpointer user_data){
        MonitorControl *mc = (MonitorControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_string (g_variant_get_string (parameter, NULL));
                
        XSM_DEBUG_GP (DBG_L1, "MonitorControl Logging Level Radio action %s activated, state changes from %s to %s\n",
                      g_action_get_name (G_ACTION (action)),
                      g_variant_get_string (old_state, NULL),
                      g_variant_get_string (new_state, NULL));
        
        if (!strcmp (g_variant_get_string (new_state, NULL), "off"))
                mc->set_logging_level (0);
        else if (!strcmp (g_variant_get_string (new_state, NULL), "basic"))
                mc->set_logging_level (1);
        else if (!strcmp (g_variant_get_string (new_state, NULL), "normal"))
                mc->set_logging_level (2);
        else if (!strcmp (g_variant_get_string (new_state, NULL), "verbose"))
                mc->set_logging_level (3);
        else if (!strcmp (g_variant_get_string (new_state, NULL), "all"))
                mc->set_logging_level (10);
        else // fallback
                mc->set_logging_level (2);
        
        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);
}



void MonitorControl::LogEvent (const gchar *Action, const gchar *Entry, gint level){
        if (logging_level >= level){
                // add to log file
                PutLogEvent (Action, Entry);

                // no logging into text buffer if max_lines < 0
                if (max_lines < 0)
                        return;
                
                GtkTextIter start_iter, end_trim_iter, end_iter;
                gint lines = gtk_text_buffer_get_line_count (log_buf);
         
                // append to log buffer view
#if 0 // obsoleted code:
	        GTimeVal gt;
                g_get_current_time (&gt);
                gchar *tmp = g_time_val_to_iso8601 (&gt);
#else // new -- but not yet widly available:
                GTimeZone *tz = g_time_zone_new_local ();
                GDateTime *gdt = g_date_time_new_now (tz);
                gchar *tmp = g_date_time_format_iso8601 (gdt);
                g_time_zone_unref (tz);
                g_date_time_unref (gdt);
#endif
                GString *output = g_string_new (tmp);
                g_free (tmp);
                output = g_string_append(output, ": \t");
                output = g_string_append(output, Action);
                output = g_string_append(output, ": \t");
                output = g_string_append(output, Entry);
                output = g_string_append(output, "\n");

                gtk_text_buffer_get_start_iter (log_buf, &start_iter);

                // limit buffer max_lines lines unless max_lines == 0
                if (max_lines > 1 && lines > max_lines){
                        gtk_text_buffer_get_iter_at_line_index (log_buf, &end_trim_iter, lines-max_lines, 0);
                        gtk_text_buffer_delete (log_buf,  &start_iter,  &end_trim_iter);
                }

                gtk_text_buffer_get_end_iter (log_buf, &end_iter);
                gtk_text_buffer_move_mark (log_buf, text_end_mark, &end_iter );
                // gtk_text_buffer_insert_at_cursor (log_buf, output->str, -1 );
                gtk_text_buffer_insert (log_buf, &end_iter, output->str, -1 );
                g_string_free(output, TRUE);

                gtk_text_buffer_get_end_iter (log_buf, &end_iter);
                gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (log_view), text_end_mark, 0.0, true, 0.5, 1);
        }
}
