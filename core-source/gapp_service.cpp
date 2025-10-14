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

#include <json-glib/json-glib.h>
   
#ifdef ENABLE_GXSM_WINDOW_MANAGEMENT
# ifdef GDK_WINDOWING_X11
#  include <gdk/x11/gdkx.h>
# endif
# ifdef GDK_WINDOWING_WAYLAND
#  include <gdk/wayland/gdkwayland.h>
# endif
#endif

// Gxsm headers 
#include "gxsm_app.h"
#include "gxsm_window.h"

#include "gapp_service.h"
#include "pcs.h"
#include "glbvars.h"
#include "surface.h"
#include "remote.h"


// ============================================================
// GnomeAppService
// ============================================================

gint GnomeAppService::setup_multidimensional_data_copy (const gchar *title, Scan *src, int &ti, int &tf, int &vi, int &vf,
                                                        int *tnadd, int *vnadd, int *crop_window_xy, gboolean crop){
	UnitObj *Pixel = new UnitObj("Pix","Pix");
	UnitObj *Unity = new UnitObj(" "," ");
	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_(title),
							 gapp->get_main_window  (), // default to main app window 
							 (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
							 _("_OK"), GTK_RESPONSE_ACCEPT,
							 _("_CANCEL"), GTK_RESPONSE_CANCEL,
							 NULL); 
        
	BuildParam bp;
        gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);

        bp.grid_add_label ("New Time and Layer bounds:"); bp.new_line ();

	gint t_max = src->number_of_time_elements ()-1;
	if (t_max < 0) t_max=0;

        bp.grid_add_ec ("t-inintial",  Unity, &ti, 0, t_max, ".0f"); bp.new_line ();
        bp.grid_add_ec ("t-final",     Unity, &tf, 0, t_max, ".0f"); bp.new_line ();
	if (tnadd){
		bp.grid_add_ec ("t-#add",  Unity, tnadd, 1, t_max, ".0f"); bp.new_line ();
	}
        bp.grid_add_ec ("lv-inintial", Unity, &vi, 0, src->mem2d->GetNv (), ".0f"); bp.new_line ();
        bp.grid_add_ec ("lv-final",    Unity, &vf, 0, src->mem2d->GetNv (), ".0f"); bp.new_line ();

	if (vnadd){
		bp.grid_add_ec ("lv-#add",  Unity, vnadd, 1, src->mem2d->GetNv (), ".0f"); bp.new_line ();
	}

	if (crop && crop_window_xy){
                bp.grid_add_label ("Crop window bounds:"); bp.new_line ();
                bp.grid_add_ec ("X-left",  Pixel, &crop_window_xy[0], 0, src->mem2d->GetNx ()-1, ".0f"); bp.new_line ();
                bp.grid_add_ec ("Y-top",   Pixel, &crop_window_xy[1], 0, src->mem2d->GetNx ()-1, ".0f"); bp.new_line ();
                bp.grid_add_ec ("X-right", Pixel, &crop_window_xy[2], 0, src->mem2d->GetNx ()-1, ".0f"); bp.new_line ();
                bp.grid_add_ec ("Y-bottom",Pixel, &crop_window_xy[3], 0, src->mem2d->GetNx ()-1, ".0f"); bp.new_line ();
	}
        bp.show_all ();

	gtk_widget_show (dialog);
        int response = GTK_RESPONSE_NONE;
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
        // FIX-ME GTK4 ?? -- wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

	delete Pixel;
	delete Unity;

        return response;
}

void GnomeAppService::warning(const char *mld, GtkWindow *parent){
        if (is_thread_safe_no_gui_mode()) return;
        GtkWidget *dialog = gtk_message_dialog_new_with_markup
                (parent,
                 GTK_DIALOG_DESTROY_WITH_PARENT,
                 GTK_MESSAGE_WARNING,
                 GTK_BUTTONS_CLOSE,
                 "<span foreground='red' size='large' weight='bold'>%s</span>\n%s", N_("Warning!"),mld);
        gtk_widget_show (dialog);
        main_get_gapp () -> monitorcontrol->LogEvent ("Warning", mld);

        int response = GTK_RESPONSE_NONE;
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
        // FIX-ME GTK4 ?? -- wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
}

void GnomeAppService::errormsg(const char *mld, GtkWindow *parent){
        if (is_thread_safe_no_gui_mode()) return;
        GtkWidget *dialog = gtk_message_dialog_new_with_markup
                (parent,
                 GTK_DIALOG_DESTROY_WITH_PARENT,
                 GTK_MESSAGE_ERROR,
                 GTK_BUTTONS_CLOSE,
                 "<span foreground='red' size='large' weight='bold'>%s</span>\n%s", N_("Error!"), mld);

        gtk_widget_show (dialog);
        int response = GTK_RESPONSE_NONE;
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
        // FIX-ME GTK4 ?? -- wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
}

void GnomeAppService::message(const char *mld, GtkWindow *parent){
        if (is_thread_safe_no_gui_mode()) return;
        GtkWidget *dialog = gtk_message_dialog_new_with_markup
                (parent,
                 GTK_DIALOG_DESTROY_WITH_PARENT,
                 GTK_MESSAGE_INFO,
                 GTK_BUTTONS_CLOSE,
                 "<span foreground='blue' size='large' weight='bold'>%s</span>\n%s", N_("Info"), mld);

        main_get_gapp ()->monitorcontrol->LogEvent ("Message", mld);

        gtk_widget_show (dialog);
        int response = GTK_RESPONSE_NONE;
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
        // FIX-ME GTK4 -- wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
}

gboolean GnomeAppService::question_yes_no (const gchar *question, GtkWindow *parent, const gchar *format){
        // Yes / No ?
        if (is_thread_safe_no_gui_mode()) return TRUE;
        GtkWidget *dialog = gtk_message_dialog_new_with_markup
                (parent,
                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_USE_HEADER_BAR,
                 GTK_MESSAGE_QUESTION,
                 GTK_BUTTONS_YES_NO,
                 format ? format : "<span foreground='blue' size='large' weight='bold'>%s</span>", question);

        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

        gtk_widget_show (dialog);
        int response = GTK_RESPONSE_NONE;
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
        // FIX-ME GTK4 -- wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

        return response == GTK_RESPONSE_YES;
}

gint GnomeAppService::question_yes_no_with_action (const gchar *question, const gchar *action_label, gint &var, GtkWindow *parent){
        // Yes / No ?
        if (is_thread_safe_no_gui_mode()) return TRUE;
        GtkWidget *dialog_action;
        GtkWidget *dialog = gtk_message_dialog_new_with_markup
                (parent,
                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_USE_HEADER_BAR,
                 GTK_MESSAGE_QUESTION,
                 GTK_BUTTONS_YES_NO,
                 "<span foreground='blue' size='large' weight='bold'>%s</span>\n", question);
        dialog_action = gtk_check_button_new_with_label( N_(action_label));
        gtk_check_button_set_active (GTK_CHECK_BUTTON (dialog_action), var? true:false);
        gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), dialog_action);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
        gtk_widget_show (dialog);

        int response = GTK_RESPONSE_NONE;
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data_no_destroy), &response);                // FIX-ME GTK4 -- wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

        var = gtk_check_button_get_active (GTK_CHECK_BUTTON (dialog_action));
        gtk_window_destroy (GTK_WINDOW (dialog)); // now destroy!

        return response;
}


int GnomeAppService::choice(const char *s1, const char *s2, const char *s3, int numb, const char *b1, const char *b2, const char *b3, int def){
	static gchar *s = NULL;
	if(s){ g_free(s); s=NULL; }
	if(!s1 && !s2) return 0;
	s = g_strconcat( s2, "\n", s3, NULL);
	return dialog(s1, s, b1, b2, b3, TRUE);
}

int GnomeAppService::dialog(const char *title, const char *content, 
			    const char *b1, const char *b2, const char *b3, 
			    int wait){
	GtkWidget *label = gtk_label_new (N_(content));
	gtk_widget_show (label);
	
	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_(title),
                                                         gapp->get_main_window  (),
							 (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
							 N_(b1), 1,
							 N_(b2), 2,
							 N_(b3), 3,
							 NULL);

	gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), label);

        int response = GTK_RESPONSE_NONE;
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
        // FIX-ME GTK4 -- wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

	return response;
}

GtkWidget* GnomeAppService::progress_info_new (const gchar *title, gint levels, GCallback cancel_cb, gpointer data, gboolean modal){
	static gint last_levels=0;
	static GCallback last_cancel_cb=NULL;
	static gpointer last_data=NULL;
        gboolean new_dialog=false;
        
	progress_dialog_schedule_close = 0;

	if (progress_dialog && (last_levels != levels || last_cancel_cb != cancel_cb || last_data != data))
		progress_info_destroy_now();

	last_levels = levels;
	last_cancel_cb = cancel_cb;
	last_data = data;

	if (!progress_dialog){
                new_dialog=true;
                progress_dialog = gtk_dialog_new_with_buttons (N_(title?title:"Progress"),
                                                               gapp->get_main_window  (),
                                                               (GtkDialogFlags)((modal ? GTK_DIALOG_MODAL:0) | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                               NULL, NULL, NULL);
                
		for (int i=0; i<MAX_PROGRESS_LEVELS; ++i)
			progress_bar[i]	= NULL;
	} else {
                gtk_window_set_title (GTK_WINDOW (progress_dialog), N_(title?title:"Progress"));
        }

	//	Add GtkProgressBar

        GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
        gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (progress_dialog))), vbox);
	if (levels>0)
		for (int i=0; i<levels && i<MAX_PROGRESS_LEVELS; ++i){

			if (!progress_bar[i]){
				progress_bar[i]	= gtk_progress_bar_new ();
				gtk_box_append (GTK_BOX (vbox), progress_bar[i]);
                                gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR (progress_bar[i]), PANGO_ELLIPSIZE_START);
			}
		}

	if (cancel_cb && new_dialog){
                GtkWidget* button = gtk_dialog_add_button (GTK_DIALOG (progress_dialog), N_("Cancel"), 100);
		g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (cancel_cb), data);
        }

        gtk_widget_show (progress_dialog); // FIX-ME GTK4 SHOWALL
	check_events_self();
	return progress_dialog;
}

int GnomeAppService::progress_info_set_bar_fraction (gdouble fraction, gint level){
        if (is_thread_safe_no_gui_mode()) return -1;
	if (!progress_dialog) return -1;
	if (level<1 || level > MAX_PROGRESS_LEVELS) return -1;

	if (progress_bar[level-1] && fraction >= 0.)
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar[level-1]), fraction);

	check_events_self();
	return 0;
}

int GnomeAppService::progress_info_set_bar_pulse (gint level, gdouble fraction){
        if (is_thread_safe_no_gui_mode()) return -1;
	if (!progress_dialog) return -1;
	if (level<1 || level > MAX_PROGRESS_LEVELS) return -1;

	if (progress_bar[level-1]){
		if (fraction >= 0.)
			gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (progress_bar[level-1]), fraction);
		else
			gtk_progress_bar_pulse (GTK_PROGRESS_BAR (progress_bar[level-1]));
		check_events_self();
	}
	return 0;
}

int GnomeAppService::progress_info_set_bar_text (const gchar* text, gint level){
        if (is_thread_safe_no_gui_mode()) return -1;
	if (!progress_dialog) return -1;
	if (level<1 || level > MAX_PROGRESS_LEVELS) return -1;

	if (progress_bar[level-1]){
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress_bar[level-1]), text);
                gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (progress_bar[level-1]), TRUE);
        }

	check_events_self();
	return 0;
}

int GnomeAppService::progress_info_add_info (const gchar* info){
        if (is_thread_safe_no_gui_mode()) return -1;
	if (!progress_dialog)
		return -1;

	GtkWidget *label = gtk_label_new (N_(info));
	gtk_widget_show (label);
	gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (progress_dialog))), label);

	check_events_self();
	return 0;
}

static guint gas_close_progress (GnomeAppService *gas){
        if (gas->is_thread_safe_no_gui_mode()) return -1;
	if (gas->progress_info_close_scheduled () > 1){
		gas->progress_info_close_schedule_dec ();
		return TRUE;
	}
	if (gas->progress_info_close_scheduled () == 1)
		gas->progress_info_destroy_now();
	return FALSE; // terminate timeout call
}

void GnomeAppService::progress_info_destroy_now(){
	if (progress_dialog){
		progress_dialog_schedule_close = 0;
		gtk_window_destroy (GTK_WINDOW (progress_dialog));
		progress_dialog=NULL;
	}
}

// schedules dealyed progress infor window destruction, it may be reused automatically if more similar progress requests
void GnomeAppService::progress_info_close (){
	if (progress_dialog){
		progress_dialog_schedule_close = 2;
		g_timeout_add (1000, (GSourceFunc)gas_close_progress, this);
	}
}

void GnomeAppService::file_dialog_completed (GtkDialog *dialog,  int response, gpointer user_data){
        gchar *fn = (gchar*) user_data;
        if (response == GTK_RESPONSE_ACCEPT){
                GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
                g_autoptr(GFile) file = gtk_file_chooser_get_file (chooser);
                fn = g_file_get_parse_name (file);
        }
        gtk_window_destroy (GTK_WINDOW (dialog));
}

gchar *GnomeAppService::file_dialog_save (const gchar *title, 
                                          const gchar *path, 
                                          const gchar *name,
                                          GtkFileFilter **filter
                                          ){
        GtkWidget *chooser = gtk_file_chooser_dialog_new (title,
                                                          gapp->get_main_window  (),
                                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                                          N_("_Cancel"), GTK_RESPONSE_CANCEL,
                                                          N_("_Save"), GTK_RESPONSE_ACCEPT,
                                                          NULL);
        
        // FIX-ME GTK4 anything a like??
        //gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

        if (path){
                GFile *default_file_for_saving = g_file_new_for_path (path);
                gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), default_file_for_saving, NULL);
                g_object_unref (default_file_for_saving);
        }
        
        if (name){
                gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), name);
        }

        if (filter)
                while (*filter)
                        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), *filter++);

        gchar *file_name=NULL;
        g_signal_connect (chooser, "response", G_CALLBACK (GnomeAppService::file_dialog_completed), file_name);

        // FIX-ME GTK4 non ideal rework
        while (!file_name)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
        
        return file_name; // must g_free
}

gchar *GnomeAppService::file_dialog_load (const gchar *title, 
                                          const gchar *path, 
                                          const gchar *name,
                                          GtkFileFilter **filter
                                          ){
        GtkWidget *chooser = gtk_file_chooser_dialog_new (title,
                                                          gapp->get_main_window  (),
                                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                                          N_("_Cancel"), GTK_RESPONSE_CANCEL,
                                                          N_("_Load"), GTK_RESPONSE_ACCEPT,
                                                          NULL);
        if (path){
                GFile *default_file_for_saving = g_file_new_for_path (path);
                gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), default_file_for_saving, NULL);
                g_object_unref (default_file_for_saving);
        }

        if (name){
                gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), name);
        }

        if (filter)
                while (*filter)
                        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), *filter++);
        
        gchar *file_name=NULL;
        g_signal_connect (chooser, "response", G_CALLBACK (GnomeAppService::file_dialog_completed), file_name);

        // FIX-ME GTK4 non ideal rework
        while (!file_name)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
        
        return file_name; // must g_free
}


int GnomeAppService::check_file(gchar *fn){
	int r = 3;
	while (fn && r==3){
		std::ifstream f;
		f.open(fn, std::ios::in);
		if(f.good()){
			f.close();
			if((r=choice(WRN_WARNING, fn, WRN_FILEEXISTS, 2, L_CANCEL, L_OVERWRITE, L_RETRY, 1)) == 1)
				return FALSE;
			else if(r == 2)
				return TRUE;
		}
		else 
			return TRUE;
	}
	return FALSE;
}


void GnomeAppService::ValueRequest(const gchar *title, const gchar *label, const gchar *infotxt, 
				   UnitObj *uobj, double minv, double maxv, const gchar *vfmt,
				   double *value){
        g_message ("GnomeAppService::ValueRequest >%s<", title);
	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_(title),
                                                         gapp->get_main_window  (),
							 (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
							 _("_OK"), GTK_RESPONSE_ACCEPT,
							 NULL);
       
        BuildParam bp;

	gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);

        bp.grid_add_ec (infotxt, uobj, value, minv, maxv, vfmt, 0.1, 1.0);
	bp.show_all ();
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	gtk_widget_show(dialog);
        
        int response = GTK_RESPONSE_NONE;
        g_signal_connect (dialog, "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);

        // FIX-ME GTK4 ??
        // wait here on response
        g_message ("GnomeAppService::ValueRequest >%s< -- waiting for response", title);
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
}

// label list must be NULL terminated array.
void GnomeAppService::ValueRequestList (const gchar *title,
                                        const gchar *label[], const gchar *infotxt[], 
                                        UnitObj *uobj[], double minv[], double maxv[], const gchar *vfmt[],
                                        double *value[]){

        g_message ("GnomeAppService::ValueRequestList >%s<", title);
	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_(title),
                                                         gapp->get_main_window (),
							 (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
							 _("_OK"), GTK_RESPONSE_ACCEPT,
							 NULL);
       
        BuildParam bp;

	gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);

        for (; *label; ++label, ++infotxt, ++uobj, ++minv, ++maxv, ++vfmt, ++value){
                bp.grid_add_ec (*infotxt, *uobj, *value, *minv, *maxv, *vfmt, 0.1, 1.0);
                bp.new_line ();
        }

        bp.show_all ();

        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	gtk_widget_show(dialog);
        
        int response = GTK_RESPONSE_NONE;
        g_signal_connect (dialog, "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);

        // FIX-ME GTK4 ??
        // wait here on response
        g_message ("GnomeAppService::ValueRequestList >%s< -- waiting for response", title);
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
}


gint GnomeAppService::terminate_timeout_func (gpointer data){
        gchar *m  = (gchar*)g_object_get_data (G_OBJECT (data), "SM");
        gint c = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (data), "CM"));
        --c; // c=0 if dialog closed, will terminate rigth away.
        if (c > 0){
                gchar *message = g_strdup_printf ("%s\nTerminating in %d s.", m, c);
                // g_message (message);
                g_object_set_data (G_OBJECT (data), "CM", GINT_TO_POINTER (c));
                gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (data), message);
                gtk_window_present (GTK_WINDOW (data));
                g_free (message);
                return true;
        }
        if (m) g_free (m);
        g_critical ("GXSM is terminating now due to DSP software version mismatch. Please update DSP.");
        exit (-1);
        return false;
}

void GnomeAppService::alert(const gchar *s1, const gchar *s2, const gchar *s3, int c){
        if (is_thread_safe_no_gui_mode()) return;
        GtkWidget *dialog = gtk_message_dialog_new_with_markup (gapp->get_main_window  (),
                                                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                                                GTK_MESSAGE_WARNING,
                                                                GTK_BUTTONS_CLOSE,
                                                                "<span foreground='red' size='large' weight='bold'>%s</span>\n%s\n%s", s1, s2, s3);

        gtk_widget_show (dialog);

        int response = GTK_RESPONSE_NONE;
        g_signal_connect (dialog, "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);

        if (c > 5){
                g_message ("adding timeout for forced exit");
                g_object_set_data (G_OBJECT (dialog), "SM", (gpointer)g_strdup_printf ("<span foreground='red' size='large' weight='bold'>%s</span>\n%s\n%s", s1, s2, s3));
                g_object_set_data (G_OBJECT (dialog), "CM", GINT_TO_POINTER (c));
                g_timeout_add ((guint)1000, 
                               (GSourceFunc) GnomeAppService::terminate_timeout_func, 
                               dialog
                               );
        }

        // FIX-ME GTK4 ??
        // wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

}


// ============================================================
// AppBase
// ============================================================

AppBase::AppBase (Gxsm4app *app){ 
	XSM_DEBUG_GM(DBG_L3, "AppBase::AppBase" ); 

        gxsm4app = GXSM4_APP(app);
        main_title_buffer = g_strdup ("AppBase: Main Window title is not set.");
        sub_title_buffer = NULL;

        app_window = NULL;
        window = NULL;
	window_key=NULL;
        window_geometry=NULL;
	showstate=FALSE; 
	nodestroy=FALSE;
        title_label = NULL;
        geometry_settings=g_settings_new (GXSM_RES_BASE_PATH_DOT".window-geometry");
       
}

AppBase::~AppBase(){ 
	XSM_DEBUG (DBG_L3, "AppBase::~AppBase destructor for '" << (window_key?window_key:"--") << "'."); 

        if (g_object_get_data (G_OBJECT (gxsm4app), "APP-MAIN"))
                ((App*)g_object_get_data (G_OBJECT (gxsm4app), "APP-MAIN")) -> remove_appwindow_from_list (this); // remove self from list
        
	if(!nodestroy){
		XSM_DEBUG_GP (DBG_L3, "~AppBase -- destroy window '%s'. [DISABLED HERE HANDLED AT BASE]",  (window_key?window_key:"--")); 
                //gtk_window_destroy (window); // final destory
	}
        
        if (window_key)
                g_free(window_key);

        if (window_geometry)
                g_free (window_geometry);

        if (main_title_buffer)
                g_free (main_title_buffer);
        if (sub_title_buffer)
                g_free (sub_title_buffer);
        main_title_buffer = sub_title_buffer = NULL;

        XSM_DEBUG_GM (DBG_L3, "AppBase::~AppBase done." );
}

void AppBase::SetTitle(const gchar *title, const gchar *sub_title){
        if (!title)
                return;
	XSM_DEBUG_GM (DBG_L3, "AppBase::SetTitle >%s<_%s_", title, sub_title?sub_title:"-/-");

        if (main_title_buffer)
                g_free (main_title_buffer);
        main_title_buffer = g_strdup (title);

        gtk_window_set_title (GTK_WINDOW (window), main_title_buffer);

        if (sub_title){
                g_free (sub_title_buffer);
                sub_title_buffer = g_strdup (sub_title);
        }
        if (sub_title_buffer){
                if (!title_label){
                        title_label = gtk_label_new (NULL);
                        gtk_label_set_single_line_mode (GTK_LABEL (title_label), false);
                        gtk_label_set_ellipsize (GTK_LABEL (title_label), PANGO_ELLIPSIZE_END);
                        if (header_bar)
                                gtk_header_bar_set_title_widget( GTK_HEADER_BAR (header_bar), title_label);
                        gtk_widget_show (title_label);
                }
                const char *format1 =
                        "<span size='large' weight='bold'>\%s</span>\n"
                        "<span size='small' weight='bold'>\%s</span>";
                char *markup;
                markup = g_markup_printf_escaped (format1, main_title_buffer?main_title_buffer:"", sub_title_buffer?sub_title_buffer:"");
                gtk_label_set_markup (GTK_LABEL (title_label), markup);
                g_free (markup);
        }
        else
                if (main_title_buffer)
                        gtk_window_set_title (GTK_WINDOW (window), main_title_buffer);
}

void AppBase::AppWindowInit(const gchar *title, const gchar *sub_title){
	XSM_DEBUG_GM (DBG_L2, "AppBase::WindowInit: <%s : %s>", title, sub_title?sub_title:"N/A");
        app_window =  gxsm4_app_window_new (gxsm4app);
        window = GTK_WINDOW (app_window);

        header_bar = gtk_header_bar_new ();
        gtk_widget_show (header_bar);

        gtk_window_set_titlebar (GTK_WINDOW (window), header_bar);
        SetTitle (title, sub_title);

	v_grid = gtk_grid_new ();
	g_object_set_data (G_OBJECT (window), "v_grid", v_grid);
        gtk_window_set_child (GTK_WINDOW (window), v_grid);

        gtk_widget_show (GTK_WIDGET (window));
}

gboolean AppBase::window_close_callback (GtkWidget *widget, AppBase *self){
        self->hide ();
        return true; // no further actions!!
}

        
void AppBase::window_action_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
        AppBase *app_w = (AppBase *)user_data;
        app_w->show ();
}

GMenuModel *AppBase::find_extension_point_section (GMenuModel  *model,
                                                   const gchar *extension_point)
{
        const gint dbg=0;
        gint i, n_items;
        GMenuModel *section = NULL;

        if (model == NULL)
                return NULL;

        XSM_DEBUG_GM (DBG_L7, "MyGnomeTools::find_extension_point_section '%s' in menu.", extension_point);

        n_items = g_menu_model_get_n_items (model);

        XSM_DEBUG_GM (DBG_L9, "MyGnomeTools::find_extension_point_section n_items=%d", n_items);

        for (i = 0; i < n_items && !section; i++) {
                gchar *id = NULL;
                
                gboolean ret=g_menu_model_get_item_attribute (model, i, "id", "s", &id);
                XSM_DEBUG_GM (DBG_L9, "MyGnomeTools::find_extension_point_section i=%d  id: %s %s", i, id, ret?"YES":"NO");
                if (id) { g_free (id); } id = NULL;
                gboolean retx1=g_menu_model_get_item_attribute (model, i, "name", "s", &id);
                XSM_DEBUG_GM (DBG_L9, "MyGnomeTools::find_extension_point_section i=%d name: %s %s", i, id, retx1?"YES":"NO");
                if (id) { g_free (id); } id = NULL;
                gboolean retx2=g_menu_model_get_item_attribute (model, i, "label", "s", &id);
                XSM_DEBUG_GM (DBG_L9, "MyGnomeTools::find_extension_point_section i=%d label: %s %s", i, id, retx2?"YES":"NO");
                if (id) { g_free (id); } id = NULL;


                if (ret){
                        if (g_menu_model_get_item_attribute (model, i, "id", "s", &id) &&
                            strcmp (id, extension_point) == 0) {
                                section = g_menu_model_get_item_link (model, i, G_MENU_LINK_SECTION); 
                                if (id) g_free (id);
                                XSM_DEBUG_GM (DBG_L7, "MyGnomeTools::find_extension_point_section ... %s", section?"OK":"INSERT FAILED");
                                return section;
                        }
                }
                
                GMenuModel *subsection;
                GMenuModel *submenu;
                gint j, j_items;
                
                XSM_DEBUG_GM (DBG_L9, "MyGnomeTools::find_extension_point_section subsecton lookup");

                subsection = g_menu_model_get_item_link (model, i, G_MENU_LINK_SECTION);
                if (subsection == NULL){ // try here:
                        submenu = g_menu_model_get_item_link (model, i, G_MENU_LINK_SUBMENU);
                        if (submenu)
                                section = find_extension_point_section (submenu, extension_point);
                } else {
                        //                        if (subsection) {
                        j_items = g_menu_model_get_n_items (subsection);
                        XSM_DEBUG_GM (DBG_L9, "MyGnomeTools::find_extension_point_section subsecton lookup -- j_items=%d", j_items);
                        
                        for (j = 0; j < j_items && !section; j++) {
                                XSM_DEBUG_GM (DBG_L9, "MyGnomeTools::find_extension_point_section submenu lookup -- j_items=%d", j);
                                submenu = g_menu_model_get_item_link (subsection, j, G_MENU_LINK_SUBMENU);
                                if (submenu)
                                        section = find_extension_point_section (submenu, extension_point);
                        }
                }
        }

        XSM_DEBUG_GM (DBG_L7, "MyGnomeTools::find_extension_point_section ... %s", section?"OK":"INSERT FAILED");
        return section;
}

void AppBase::add_window_to_window_menu(const gchar *menu_item_label, const gchar* key){
        const gchar *menusection = "windows-section";
        
        if (strlen(menu_item_label) < 1)
                return;

        // generate valid action string from menu path
        gchar *tmp = g_strconcat (menusection, "-show-", key, NULL);
        g_strdelimit (tmp, " /:", '-');
        gchar *tmpaction = g_strdup (tmp);
        gchar *app_tmpaction = g_strconcat ( "app.", tmpaction, NULL);
        GSimpleAction *ti_action;
        
        XSM_DEBUG_GM (DBG_L5, "AppBase::add_window_to_window_menu <%s>  MenuItem= %s ==> label, generated actions: [%s] <%s>, <%s>",
                      menusection, menu_item_label, tmp, tmpaction, app_tmpaction);

        if (!strcmp (menusection, "windows-section-xx")) { // add toggle -- testing, not yet working -- disabled via -xx
                ti_action = g_simple_action_new_stateful (tmpaction,
                                                          G_VARIANT_TYPE_BOOLEAN,
                                                          g_variant_new_boolean (true));
                g_signal_connect (ti_action, "toggled", G_CALLBACK (AppBase::window_action_callback), this);
        } else {
                ti_action = g_simple_action_new (tmpaction, NULL);
                g_signal_connect (ti_action, "activate", G_CALLBACK (AppBase::window_action_callback), this);
        }
        
        g_object_set_data (G_OBJECT (ti_action), "AppBase", this);

        g_action_map_add_action (G_ACTION_MAP ( main_get_gapp ()->get_application ()), G_ACTION (ti_action));


#if 1 // pretty rewrite name
        gchar *label = g_strdelimit (g_strdup (menu_item_label), "-:._/", ' ');
        gchar *p = label;
        *p = g_ascii_toupper (*p); ++p;
        for (; *p; ++p)
                if (*p == ' ' && *(p+1))
                        *(p+1) = g_ascii_toupper (*(p+1));
#else
        gchar *label = g_strdup (menu_item_label);
#endif
        XSM_DEBUG_GM (DBG_L3, "AppBase::add_window_to_window_menu <%s>  MenuItem= %s ==> label, generated actions: [%s] <%s>",
                      menusection, menu_item_label, tmp, app_tmpaction);
        
        main_get_gapp ()->gxsm_app_extend_menu (menusection, label, app_tmpaction);

        g_free (label);
        g_free (app_tmpaction);
        g_free (tmpaction);
        g_free (tmp);
}

int AppBase::set_window_geometry (const gchar *key, gint index, gboolean add_to_menu){
        XSM_DEBUG_GM (DBG_L3, "AppBase::set_window_geometry ** setup and append '%s' to Windows Menu.", key?key:"!! window key N/A !!");

        if (window_key){
                XSM_DEBUG_GM (DBG_L2, "AppBase::set_window_geometry ... geometry already setup. DUPLICATE WARNING append '%s' to Windows Menu already done.", key);
                // remove from menu!
                g_free (window_key);
        }
        
        if (index >= 0 && index <= 12) // limit for now
                window_key = g_strdup_printf ("%s-%d", key, index);
        else if (index > 12)
                return -1; // do not handle at this time.
        else
                window_key = g_strdup (key);

        XSM_DEBUG_GM (DBG_L5, "AppBase::set_window_geometry ... load geometry");

	LoadGeometry ();

        XSM_DEBUG_GM (DBG_L5, "AppBase::set_window_geometry ... add to menu");

        if (add_to_menu)
                add_window_to_window_menu (window_key, window_key);

        if (g_object_get_data (G_OBJECT (gxsm4app), "APP-MAIN"))
                ((App*)g_object_get_data (G_OBJECT (gxsm4app), "APP-MAIN")) -> add_appwindow_to_list (this); // add self to gapp globale list

        XSM_DEBUG_GM (DBG_L5, "AppBase::set_window_geometry ... done. >%s<", key);
	return 0;
}

void AppBase::hide (){
        gtk_widget_hide (GTK_WIDGET (window));
        showstate=FALSE;
}

void AppBase::show (){
        gtk_widget_show (GTK_WIDGET (window));
        showstate=TRUE;
        position_auto ();
        resize_auto ();
}

void AppBase::show_auto (){
        if (!window_geometry) return;
	XSM_DEBUG_GM (DBG_L2, "AppBase::show_auto ****** Show/Hide window ** %s **", window_key );
        
        if (window_geometry[WGEO_FLAG] && window_geometry[WGEO_SHOW])
                show ();
        else
                hide ();
}

void AppBase::position_auto (){
#ifdef ENABLE_GXSM_WINDOW_MANAGEMENT
        if (window_geometry)
                if (window_geometry[WGEO_FLAG]){
                        // GTK3:
                        // gtk_window_move (GTK_WINDOW (window), window_geometry[WGEO_XPOS], window_geometry[WGEO_YPOS]);
                        // g_message ("SORRY GTK4 can't do it -- Requested Window Position [%s: %d, %d]   -- no gtk_window_move ().", window_key, window_geometry[WGEO_XPOS], window_geometry[WGEO_YPOS]);
                        XSM_DEBUG_GM (DBG_L1, "AppBase::position_auto ** Requested Window Position ** %s ** XY (%d, %d)", window_key, window_geometry[WGEO_XPOS], window_geometry[WGEO_YPOS]);
# ifdef GDK_WINDOWING_X11
                        if (GDK_IS_X11_DISPLAY (gdk_display_get_default ())){
                                Window   xw = GDK_SURFACE_XID (GDK_SURFACE (gtk_native_get_surface(GTK_NATIVE (window))));
                                Display *xd = GDK_SURFACE_XDISPLAY (GDK_SURFACE (gtk_native_get_surface(GTK_NATIVE (window))));
                                XMoveWindow (xd, xw, (int)window_geometry[WGEO_XPOS], (int)window_geometry[WGEO_YPOS]);
                        } else
# endif
# ifdef GDK_WINDOWING_WAYLAND
                                if (GDK_IS_WAYLAND_DISPLAY (gdk_display_get_default ())){
                                        const gchar *wayland_display = g_getenv("WAYLAND_DISPLAY");
                                        const gchar *hyprland_signature = g_getenv("HYPRLAND_INSTANCE_SIGNATURE");

                                        if (wayland_display != NULL && hyprland_signature != NULL) {
                                                g_message ("Hyprland is likely running.\n");
                                                g_message ("WAYLAND_DISPLAY: %s\n", wayland_display);
                                                g_message ("HYPRLAND_INSTANCE_SIGNATURE: %s\n", hyprland_signature);
                                                g_message ("Attempting hyprctrl... hang in there\n");

                                                gchar *stdout_buf = NULL;
                                                gchar *stderr_buf = NULL;
                                                gint exit_status;
                                                GError *error = NULL;

                                                gchar *title = NULL;
                                                if (strncmp(main_title_buffer, "Ch", 2) == 0){
                                                        gchar *tmp = g_strndup (main_title_buffer,3);
                                                        title = g_strdup_printf("^(%s.*)$", tmp); g_free (tmp);
                                                } else  title = g_strdup (main_title_buffer);

                                                // Execute shell command
                                                gchar *hyprctl_cmdline = g_strdup_printf ("hyprctl 'dispatch setfloating title:%s'", title);
                                                g_print("Attempting: %s\n", hyprctl_cmdline);
                                                g_spawn_command_line_sync (hyprctl_cmdline, &stdout_buf, &stderr_buf, &exit_status, &error);
                                                
                                                if (error != NULL) {
                                                        g_error ("Sorry I tried. Error executing command: %s E: %s\n", hyprctl_cmdline, error->message);
                                                        g_free (hyprctl_cmdline);
                                                        g_error_free (error);
                                                        return;
                                                }
                                                
                                                g_print ("Stdout: %s\n", stdout_buf);
                                                g_print ("Stderr: %s\n", stderr_buf);
                                                g_print ("Exit Status: %d\n", exit_status);
                                                
                                                g_free (hyprctl_cmdline);
                                                g_free (stdout_buf);
                                                g_free (stderr_buf);

                                                
                                                // Execute shell command
                                                hyprctl_cmdline = g_strdup_printf ("hyprctl dispatch movewindowpixel exact %d %d, title:'%s'",
                                                                                   (int)window_geometry[WGEO_XPOS],
                                                                                   (int)window_geometry[WGEO_YPOS],
                                                                                   title
                                                                                   );
                                                g_print("Attempting: %s\n", hyprctl_cmdline);
                                                g_spawn_command_line_sync (hyprctl_cmdline, &stdout_buf, &stderr_buf, &exit_status, &error);
                                                
                                                if (error != NULL) {
                                                        g_error ("Sorry I tried. Error executing command: %s E: %s\n", hyprctl_cmdline, error->message);
                                                        g_free (hyprctl_cmdline);
                                                        g_error_free (error);
                                                        return;
                                                }
                                                
                                                g_print ("Stdout: %s\n", stdout_buf);
                                                g_print ("Stderr: %s\n", stderr_buf);
                                                g_print ("Exit Status: %d\n", exit_status);
                                                
                                                g_free (hyprctl_cmdline);
                                                g_free (stdout_buf);
                                                g_free (stderr_buf);

                                                g_free (title);
                                                
                                        } else if (wayland_display != NULL) {
                                                g_message ("A Wayland compositor is running, but it might not be Hyprland.\n");
                                                g_message ("WAYLAND_DISPLAY: %s\n", wayland_display);
                                                g_message ("SORRY WAYLAND DOES NOT GIVE ANY ACCESS TO WINDOW GEOMETRY. Hint: try Hyprland!");
                                        } else {
                                                g_message ("Wayland some what, but No Wayland compositor detected.\n");
                                        }
                                } else
                                        g_error ("Unsupported GDK backend");
                }
# endif
#else
        XSM_DEBUG_GM (DBG_L2, "AppBase::position_auto ** ENABLE_GXSM_WINDOW_MANAGEMENT is disabled.");
#endif
}

void AppBase::resize_auto (){
#ifdef ENABLE_GXSM_WINDOW_MANAGEMENT
        if (window_geometry)
                if (window_geometry[WGEO_FLAG]){
                        // GTK3:
                        // gtk_window_resize (GTK_WINDOW (window), (int)window_geometry[WGEO_WIDTH], (int)window_geometry[WGEO_HEIGHT]);

                        // trying this... not working
                        // gtk_window_set_default_size (GTK_WINDOW (window), (int)window_geometry[WGEO_WIDTH], (int)window_geometry[WGEO_HEIGHT]);

                        // g_message ("SORRY GTK4 can't do it -- Requested Window Resize [%s: %d, %d]   -- no gtk_window_resize ().", window_key, window_geometry[WGEO_WIDTH], window_geometry[WGEO_HEIGHT]);
                         XSM_DEBUG_GM (DBG_L1, "AppBase::resize_auto **** Requested Window Resize   ** %s ** WH (%d, %d)", window_key, window_geometry[WGEO_WIDTH], window_geometry[WGEO_HEIGHT]);
# ifdef GDK_WINDOWING_X11
                        if (GDK_IS_X11_DISPLAY (gdk_display_get_default ())){
                                Window   xw = GDK_SURFACE_XID (GDK_SURFACE (gtk_native_get_surface(GTK_NATIVE (window))));
                                Display *xd = GDK_SURFACE_XDISPLAY (GDK_SURFACE (gtk_native_get_surface(GTK_NATIVE (window))));
                                if ((unsigned int)window_geometry[WGEO_WIDTH] > 0 && (unsigned int)window_geometry[WGEO_HEIGHT] > 0)
                                        XResizeWindow (xd, xw,  (unsigned int)window_geometry[WGEO_WIDTH],  (unsigned int)window_geometry[WGEO_HEIGHT]);
                        } else
# endif
# ifdef GDK_WINDOWING_WAYLAND
                                if (GDK_IS_WAYLAND_DISPLAY (gdk_display_get_default ())){
                                        const gchar *wayland_display = g_getenv("WAYLAND_DISPLAY");
                                        const gchar *hyprland_signature = g_getenv("HYPRLAND_INSTANCE_SIGNATURE");

                                        if (wayland_display != NULL && hyprland_signature != NULL) {
                                                g_message ("Hyprland is likely running.\n");
                                                g_message ("WAYLAND_DISPLAY: %s\n", wayland_display);
                                                g_message ("HYPRLAND_INSTANCE_SIGNATURE: %s\n", hyprland_signature);
                                                g_message ("Attempting hyprctrl... hang in there\n");

                                                gchar *stdout_buf = NULL;
                                                gchar *stderr_buf = NULL;
                                                gint exit_status;
                                                GError *error = NULL;

                                                gchar *title = NULL;
                                                if (strncmp(main_title_buffer, "Ch", 2) == 0){
                                                        gchar *tmp = g_strndup (main_title_buffer,3);
                                                        title = g_strdup_printf("^(%s.*)$", tmp); g_free (tmp);
                                                } else  title = g_strdup (main_title_buffer);

                                                // Execute shell command
                                                gchar *hyprctl_cmdline = g_strdup_printf ("hyprctl dispatch resizewindowpixel exact %d %d, title:%s",
                                                                                          (int)window_geometry[WGEO_WIDTH],
                                                                                          (int)window_geometry[WGEO_HEIGHT],
                                                                                          title
                                                                                          );
                                                g_print("Attempting: %s\n", hyprctl_cmdline);
                                                g_spawn_command_line_sync (hyprctl_cmdline, &stdout_buf, &stderr_buf, &exit_status, &error);
                                                
                                                if (error != NULL) {
                                                        g_error ("Sorry I tried. Error executing command: %s E: %s\n", hyprctl_cmdline, error->message);
                                                        g_free (hyprctl_cmdline);
                                                        g_error_free (error);
                                                        return;
                                                }
                                                
                                                g_print ("Stdout: %s\n", stdout_buf);
                                                g_print ("Stderr: %s\n", stderr_buf);
                                                g_print ("Exit Status: %d\n", exit_status);
                                                
                                                g_free (hyprctl_cmdline);
                                                g_free (stdout_buf);
                                                g_free (stderr_buf);
                                                g_free (title);
                                        } else if (wayland_display != NULL) {
                                                g_message ("A Wayland compositor is running, but it might not be Hyprland.\n");
                                                g_message ("WAYLAND_DISPLAY: %s\n", wayland_display);
                                                g_message ("SORRY WAYLAND DOES NOT GIVE ANY ACCESS TO WINDOW GEOMETRY. Hint: try Hyprland!");
                                        } else {
                                                g_message ("Wayland some what, but No Wayland compositor detected.\n");
                                        }
                                } else
# endif
                                        g_error ("Unsupported GDK backend");
                }
#else
        XSM_DEBUG_GM (DBG_L2, "AppBase::resize_auto ** ENABLE_GXSM_WINDOW_MANAGEMENT is disabled.");
#endif
}

void AppBase::SaveGeometryCallback(AppBase *apb){
	apb->SaveGeometry();
}

int read_xy_array (JsonReader *reader, int xy[2]){
        if (json_reader_is_array(reader)) {
                gint num_elements = json_reader_count_elements(reader);
                //g_print("Reading array with #%d elements\n", num_elements);
                for (gint i = 0; i < num_elements; i++) {
                        if (json_reader_read_element(reader, i)) {
                                // Now the reader is positioned on the i-th element of the array
                                // You can retrieve its value based on its type
                                if (json_reader_is_value(reader)) { // Check if it's a simple value
                                        gint64 int_value = json_reader_get_int_value(reader);
                                        g_print("Array element %d (integer): %lld\n", i, int_value);
                                        if (i<2) xy[i] = int_value;
                                }
                                json_reader_end_element(reader); // Move cursor back to the array
                        } else {
                                g_warning("Failed to read array element at index %d\n", i);
                        }
                }
        } else {
                g_warning("Reader is not positioned on an array.\n");
        }
        return 0;
}

void json_filter_window (const char *json_string, const char *window_to_find, int at[2], int wh[2]) {
        g_autoptr(JsonParser) parser = json_parser_new();
        GError *error = NULL;

        if (!json_parser_load_from_data (parser, json_string, -1, &error)) {
                g_error("Unable to parse JSON: %s", error->message);
                g_clear_error(&error);
                return;
        }

        JsonReader *reader = json_reader_new (json_parser_get_root (parser));

        // Enter the "users" array
        json_reader_read_member(reader, "");
        //json_reader_read_array(reader);

        int num_elements = json_reader_count_elements(reader);
        for (int i = 0; i < num_elements; i++) {
                json_reader_read_element(reader, i);
                json_reader_read_member(reader, "title");
                const char *current_name = json_reader_get_string_value(reader);
                json_reader_end_member(reader); // End "name"

                if (g_strcmp0(current_name, window_to_find) == 0) {
                        g_print("Found window with name '%s'\n", window_to_find);

                        // You can now extract other properties of the found user
                        g_print("Reading .at'\n");
                        json_reader_read_member(reader, "at");
                        //int id = json_reader_get_int_value(reader);
                        read_xy_array (reader, at);
           
                        //g_print("At: %s\n", id);
                        json_reader_end_member(reader); // End "id"

                        g_print("Reading .size'\n");
                        json_reader_read_member(reader, "size");
                        //int id = json_reader_get_int_value(reader);
                        read_xy_array (reader, wh);
           
                        //g_print("At: %s\n", id);
                        json_reader_end_member(reader); // End "id"

            
                        // Exit early since we found our match
                        break;
                }

                json_reader_end_element(reader); // End the object in the array
        }

        g_object_unref(reader);
        g_object_unref(parser);
}

void AppBase::SaveGeometry(gboolean store_to_settings){
	XSM_DEBUG_GM (DBG_L2, "AppBase::SaveGeometry *** for window ** %s **", window_key);
#ifdef ENABLE_GXSM_WINDOW_MANAGEMENT

        // g_message ("AutoSave Window Geometry: %s", window_key);

        // just in case it was not loaded right
        if (!window_geometry)
                window_geometry = g_new (gint32, WGEO_SIZE);
        
        window_geometry[WGEO_FLAG] = 1;
        window_geometry[WGEO_SHOW] = showstate;
        //g_message ("SORRY GTK4 can't do it -- no gtk_window_get_position (GTK_WINDOW (window), &window_geometry[WGEO_XPOS], &window_geometry[WGEO_YPOS]);");
        //g_message ("SORRY GTK4 can't do it -- no gtk_window_get_size (GTK_WINDOW (window), &window_geometry[WGEO_WIDTH], &window_geometry[WGEO_HEIGHT]);");
        //gtk_window_get_position (GTK_WINDOW (window), &window_geometry[WGEO_XPOS], &window_geometry[WGEO_YPOS]); 

# ifdef GDK_WINDOWING_X11
        if (GDK_IS_X11_DISPLAY (gdk_display_get_default ())){
                Window   xw = GDK_SURFACE_XID (GDK_SURFACE (gtk_native_get_surface(GTK_NATIVE (window))));
                Display *xd = GDK_SURFACE_XDISPLAY (GDK_SURFACE (gtk_native_get_surface(GTK_NATIVE (window))));
                Window root;
                int x, y;
                unsigned int width, height;
                unsigned int border_width;
                unsigned int depth;
                XGetGeometry (xd, xw, &root, &x, &y, &width, &height, &border_width, &depth);
                window_geometry[WGEO_XPOS]=x;
                window_geometry[WGEO_YPOS]=y;
                window_geometry[WGEO_WIDTH]=width;
                window_geometry[WGEO_HEIGHT]=height;
                // g_message ("Window Geometry [%s]: XY %d, %d; WH %d, %d", window_key, x, y, width, height);
                // Status = XGetGeometry (display, d, root_return, x_return, y_return, width_return, height_return, border_width_return, depth_return)
                //        Display *display;
                //        Drawable d;
                //        Window *root_return;
                //        int *x_return, *y_return;
                //        unsigned int *width_return, *height_return;
                //        unsigned int *border_width_return;
                //        unsigned int *depth_return;
        } else
# endif
# ifdef GDK_WINDOWING_WAYLAND
                if (GDK_IS_WAYLAND_DISPLAY (gdk_display_get_default ())){
                        const gchar *wayland_display = g_getenv("WAYLAND_DISPLAY");
                        const gchar *hyprland_signature = g_getenv("HYPRLAND_INSTANCE_SIGNATURE");

                        if (wayland_display != NULL && hyprland_signature != NULL) {
                                g_message ("Hyprland is likely running.\n");
                                g_message ("WAYLAND_DISPLAY: %s\n", wayland_display);
                                g_message ("HYPRLAND_INSTANCE_SIGNATURE: %s\n", hyprland_signature);
                                g_message ("Attempting hyprctrl... hang in there\n");

                                static gint64 tlast=0;
                                static gchar *stdout_buf = NULL; // auto-cached

                                if (stdout_buf && g_get_real_time () - tlast > 3e6){ // last position reading older then 3s?
                                        g_free (stdout_buf); 
                                        stdout_buf = NULL;
                                        tlast = g_get_real_time ();
                                }

                                if (!stdout_buf){
                                        gchar *stderr_buf = NULL;
                                        gint exit_status;
                                        GError *error = NULL;

                                        // Execute command
                                        gchar *hyprctl_cmdline = g_strdup_printf ("hyprctl clients -j");
                                        g_print("Attempting: %s\n", hyprctl_cmdline);
                                        g_spawn_command_line_sync (hyprctl_cmdline, &stdout_buf, &stderr_buf, &exit_status, &error);
                                        if (error != NULL) {
                                                g_error ("Sorry I tried. Error executing command: %s E: %s\n", hyprctl_cmdline, error->message);
                                                g_free (hyprctl_cmdline);
                                                g_error_free (error);
                                                return;
                                        }
                                
                                        g_print ("Stdout: %s\n", stdout_buf);
                                        g_print ("Stderr: %s\n", stderr_buf);
                                        g_print ("Exit Status: %d\n", exit_status);
                                        g_free (stderr_buf);
                                        g_free (hyprctl_cmdline);
                                }                                                

                                int at[2]={-1,-1};
                                int wh[2]={-1,-1};
                                json_filter_window (stdout_buf, main_title_buffer, at, wh);

                                if (at[0] >= 0 && at[1] >= 0){
                                        window_geometry[WGEO_XPOS]=at[0];
                                        window_geometry[WGEO_YPOS]=at[1];
                                }
                                if (wh[0] >= 0 && wh[1] >= 0){
                                        window_geometry[WGEO_WIDTH]=wh[0];
                                        window_geometry[WGEO_HEIGHT]=wh[1];
                                }

                                g_print ("Window at %d %d, WH %d %d\n",
                                         window_geometry[WGEO_XPOS], window_geometry[WGEO_YPOS],
                                         window_geometry[WGEO_WIDTH], window_geometry[WGEO_HEIGHT]
                                         );
                                
                                //g_free (stdout_buf); // static, caching
                                
                        } else if (wayland_display != NULL) {
                                g_message ("A Wayland compositor is running, but it might not be Hyprland.\n");
                                g_message ("WAYLAND_DISPLAY: %s\n", wayland_display);
                                g_message ("SORRY WAYLAND DOES NOT GIVE ANY ACCESS TO WINDOW GEOMETRY. Hint: try Hyprland!");
                        } else {
                                g_message ("Wayland some what, but No Wayland compositor detected.\n");
                        }
                } else
# endif
                        g_error ("Unsupported GDK backend");

        if (store_to_settings && window_key){
                GVariant *storage = g_variant_new_fixed_array (g_variant_type_new ("i"), window_geometry, WGEO_SIZE, sizeof (gint32));
                g_settings_set_value (geometry_settings, window_key, storage);
                //g_free (storage); // ??
        }
#else
        XSM_DEBUG_GM (DBG_L2, "AppBase::save_geometry ** ENABLE_GXSM_WINDOW_MANAGEMENT is disabled.");
#endif
}

void AppBase::LoadGeometryWRefAutoPlace(const gchar *wref_key, const gchar *wref_key2nd){
        int nth = 0;
        if (strncmp (window_key, wref_key, strlen(wref_key))) // only proceed if match (same window base key)
                return;
        if (strlen (window_key) > strlen(wref_key)+1){ // check for channel id
                nth = atoi (window_key+strlen(wref_key)+1);
        } else
                return;
        if (nth <= 0 || nth > 16)
                return;
                        
        if (!wref_key){
                XSM_DEBUG_ERROR (DBG_L1, "AppBase::LoadGeometry ... error, no window_ref_key given.");
                return;
        }
        XSM_DEBUG_GM (DBG_L2, "AppBase::LoadGeometry *** Load Geometry from window  ** %s +==> #%d **", wref_key, nth );

        //g_signal_connect (window, "close-request",  G_CALLBACK (AppBase::window_close_callback), this);

        gsize n_stores;

        double stack = 0.95;
        double href = 0.;
        gchar *wk=NULL;

        if (wref_key2nd){
                GVariant *storage = g_settings_get_value (geometry_settings, window_key); // original geom
                gint32 *tmp = (gint32*) g_variant_get_fixed_array (storage, &n_stores, sizeof (gint32));
                if (!window_geometry)
                        window_geometry = g_new (gint32, WGEO_SIZE);
                memcpy (window_geometry, tmp, WGEO_SIZE*sizeof (gint32));
                g_variant_unref (storage);
                g_assert_cmpint (n_stores, ==, WGEO_SIZE);
                href = window_geometry[WGEO_HEIGHT]; // keep

                wk = g_strdup_printf("%s", wref_key2nd);
                XSM_DEBUG_GM (DBG_L2, "AppBase::LoadGeometry *** Load Geometry relative to window  ** %s **", wk );
                nth += 6;
        } else {
                if (nth == 1) return;
                wk = g_strdup_printf("%s-1", wref_key);
        }
        nth--;
        GVariant *storage = g_settings_get_value (geometry_settings, wk);
        g_free (wk);
        gint32 *tmp = (gint32*) g_variant_get_fixed_array (storage, &n_stores, sizeof (gint32));

        if (!window_geometry)
                window_geometry = g_new (gint32, WGEO_SIZE);

        memcpy (window_geometry, tmp, WGEO_SIZE*sizeof (gint32));

        g_variant_unref (storage);

        g_assert_cmpint (n_stores, ==, WGEO_SIZE);

        //if (nth > 6) stack = 0.5;
        
        window_geometry[WGEO_XPOS] += (nth%6)*window_geometry[WGEO_WIDTH]*stack;
        if (nth > 5)
                window_geometry[WGEO_YPOS] += (nth/6)*window_geometry[WGEO_HEIGHT]*stack;

        if (href > 0)
                window_geometry[WGEO_HEIGHT] = href;
        
        position_auto ();
        resize_auto ();

        if (strcmp (window_key, "main"))
                show_auto ();
        else
                XSM_DEBUG_GM (DBG_L2, "AppBase::AutoLoadGeometry ... MAIN window: show=always");
}

void AppBase::LoadGeometryWRefAutoPlaceABmode(const gchar *wref_key){
        int nth = 0;
        int mth = 0;
        double stackx = 0.95;
        double stacky = 0.92;
        gchar *wk=NULL;
        gsize n_stores;

        if (strncmp (window_key, wref_key, strlen(wref_key))) // only proceed if match (same window base key)
                return;

        // check for '-AB' style id presence
        if (strlen(wref_key) + strlen(wref_key)+2)
                if (*(window_key+strlen(wref_key)+1) >= 'a' &&
                    *(window_key+strlen(wref_key)+2) >= 'a'){  // check for view-gvp-aa style indexing and eval 2d grid pos indexing
                        nth = 1 + (*(window_key+strlen(wref_key)+1)-'a');
                        mth = 1 + (*(window_key+strlen(wref_key)+2)-'a');
                        g_message ("LGeoAuto %s %d %d",window_key, nth, mth);
                }
                        
        if (!wref_key || mth < 1 || nth < 1){
                XSM_DEBUG_ERROR (DBG_L1, "AppBase::LoadGeometry ... error, no window_ref_key given.");
                return;
        }
        XSM_DEBUG_GM (DBG_L2, "AppBase::LoadGeometry *** Load Geometry from window  ** %s +==> #%d **", wref_key, nth );

        if (nth == 1 && mth == 1) return; // do not move ref window

        wk = g_strdup_printf("%s-aa", wref_key);

        XSM_DEBUG_GM (DBG_L2, "AppBase::LoadGeometry *** Load Geometry relative to window  ** %s **", wk );

        mth--;
        nth--;

        // get reference window geometry from '-aa'
        GVariant *storage = g_settings_get_value (geometry_settings, wk);
        g_free (wk);
        gint32 *tmp = (gint32*) g_variant_get_fixed_array (storage, &n_stores, sizeof (gint32));

        if (!window_geometry)
                window_geometry = g_new (gint32, WGEO_SIZE);

        memcpy (window_geometry, tmp, WGEO_SIZE*sizeof (gint32));

        g_variant_unref (storage);
        g_assert_cmpint (n_stores, ==, WGEO_SIZE);

        // shift to grid according to 2d labels 'AA' style
        window_geometry[WGEO_XPOS] += nth*window_geometry[WGEO_WIDTH]*stackx;
        window_geometry[WGEO_YPOS] += mth*window_geometry[WGEO_HEIGHT]*stacky;
        
        position_auto ();
        resize_auto ();

        if (strcmp (window_key, "main"))
                show_auto ();
        else
                XSM_DEBUG_GM (DBG_L2, "AppBase::AutoLoadGeometry ... MAIN window: show=always");
}

void AppBase::LoadGeometry(){
        if (!window_key){
                XSM_DEBUG_ERROR (DBG_L1, "AppBase::LoadGeometry ... error, no window_key set.");
                return;
        }
	XSM_DEBUG_GM (DBG_L2, "AppBase::LoadGeometry *** Load Geometry for window  ** %s **", window_key );

        g_signal_connect (window, "close-request",  G_CALLBACK (AppBase::window_close_callback), this);

        gsize n_stores;

        GVariant *storage = g_settings_get_value (geometry_settings, window_key);
        gint32 *tmp = (gint32*) g_variant_get_fixed_array (storage, &n_stores, sizeof (gint32));

        if (!window_geometry)
                window_geometry = g_new (gint32, WGEO_SIZE);

        memcpy (window_geometry, tmp, WGEO_SIZE*sizeof (gint32));

        g_variant_unref (storage);

        g_assert_cmpint (n_stores, ==, WGEO_SIZE);

        position_auto ();
        resize_auto ();

        if (strcmp (window_key, "main"))
                show_auto ();
        else
                XSM_DEBUG_GM (DBG_L2, "AppBase::LoadGeometry ... MAIN window: show=always");
}


void AppBase::drop_list_file_open_gfunc (GFile *gf,  gpointer *data){
        //int channel = GPOINTER_TO_INT (data);
        gchar *fn = g_file_get_parse_name (gf);
        g_message ("Loading File %s, ", fn);
        //main_get_gapp () -> xsm->ActivateChannel (channel);
        main_get_gapp () -> xsm->load (fn);
        g_free (fn);
}
                
gboolean AppBase::gapp_load_on_drop_files (GtkDropTarget *target, const GValue  *value, double x, double y, gpointer data){
        // Call the appropriate setter depending on the type of data
        // that we received
        // gchar * strVal = g_strdup_value_contents (value); g_message ("target gvalue: %s\n", strVal); free (strVal);
        int channel = GPOINTER_TO_INT (data);
        if (channel >= 0 && channel < 99){
                main_get_gapp () -> xsm->ActivateChannel (channel);
        }else{
                channel = 0;
                main_get_gapp () -> xsm->ActivateFreeChannel ();
        }
        
        if (G_VALUE_HOLDS (value, G_TYPE_BOXED)){
                g_message ("FILE LIST DROPPED... ");
                g_slist_foreach (g_value_get_boxed (value), AppBase::drop_list_file_open_gfunc, GINT_TO_POINTER (channel));
        } else if (G_VALUE_HOLDS (value, G_TYPE_FILE)){
                gchar *fn = g_file_get_parse_name (g_value_get_object (value));
                g_message ("FILE DROPPED %s", fn);
                main_get_gapp () -> xsm->load (fn);
                g_free (fn);
        }
        else
                return FALSE;
        
        return TRUE;
}
     


GtkWidget* BuildParam::grid_add_exec_button (const gchar* labeltxt,
                                                  GCallback exec_cb, gpointer cb_data, const gchar *control_id,
                                                  int bwx,
                                                  const gchar *data_key, gpointer key_data){

        remote_action_cb *ra = g_new( remote_action_cb, 1);     
        ra -> cmd = g_strdup_printf("EXECUTE_%s", control_id); 
        gchar *tooltip = g_strconcat ("Remote example: action (", ra->cmd, ")", NULL); 

        button = gtk_button_new_with_label (N_(labeltxt));
        g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK(exec_cb), cb_data);

        ra -> RemoteCb = (void (*)(GtkWidget*, void*))exec_cb;  
        ra -> widget = button;                                  
        ra -> data = cb_data;                                      
        ra -> return_data = NULL;
        
        if (data_key)
                g_object_set_data (G_OBJECT (button), data_key, key_data);
        gtk_widget_set_tooltip_text (button, tooltip);
        g_free (tooltip);
                
        grid_add_widget (button, bwx);

        gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, ra ); 
                
        return button;
}


static void remote_cb_check( GtkWidget *widget, gpointer *data ){
	gtk_check_button_set_active (GTK_CHECK_BUTTON (widget), true); 
}
static void remote_cb_uncheck( GtkWidget *widget, gpointer *data ){
	gtk_check_button_set_active (GTK_CHECK_BUTTON (widget), false); 
}

GtkWidget* BuildParam::grid_add_check_button_remote_enabled (const gchar* labeltxt, const gchar *tooltip, int bwx,
                                                             GCallback cb, gpointer cb_data, guint64 source, guint64 mask, const gchar *control_id){

        button = gtk_check_button_new_with_label (N_(labeltxt));
        if (!control_id && tooltip){ // tooltip with REMOTEID must be given like "Tooltip Text .... PyRemote:action([UN]CHECK-REMOTEID)" <= PYREMOTE_CHECK_HOOK_KEY
                const gchar *cmd = strstr (tooltip, PYREMOTE_CHECK_HOOK_KEY_PREFIX); // -xxxx).  remote IDs: -> CHECK-REMOTEID, UNCHECK-REMOTEID
                //if (cmd)
                //        g_message ("check button L=(%s) TT=(%s) with hook: %s", labeltxt, tooltip, cmd);
                //else
                //        g_message ("check button L=(%s) TT=(%s) with no RemoteID hook", labeltxt, tooltip);
                if (cmd){
                        const gchar *cmd1 = strstr (cmd, "-");
                        const gchar *cmd2 = strstr (cmd, ")");
                        control_id = g_strndup(cmd1+1, cmd2-cmd1-1);
                        //g_message ("Setting up check button (%s) with RemoteID: [UN]CHECK-%s", tooltip, control_id);
                }
        }
        if (control_id){
                remote_action_cb *raC = g_new( remote_action_cb, 1);     
                raC -> cmd = g_strdup_printf("CHECK-%s", control_id);
                gchar *autotooltip = g_strconcat ("Remote example: action ([UN]", raC->cmd, ")", NULL); 
                gtk_widget_set_tooltip_text (button, autotooltip);
                g_free (autotooltip);
                raC -> RemoteCb = (void (*)(GtkWidget*, void*))remote_cb_check;  
                raC -> widget = button;                                  
                raC -> data = cb_data;                                      
                raC -> return_data = NULL;
                gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, raC ); 

                remote_action_cb *raUC = g_new( remote_action_cb, 1);     
                raUC -> cmd = g_strdup_printf("UNCHECK-%s", control_id);
                raUC -> RemoteCb = (void (*)(GtkWidget*, void*))remote_cb_uncheck;  
                raUC -> widget = button;                                  
                raUC -> data = cb_data;                                      
                raUC -> return_data = NULL;
                gapp->RemoteActionList = g_slist_prepend ( gapp->RemoteActionList, raUC ); 
        } else
                if (tooltip)
                        gtk_widget_set_tooltip_text (button, tooltip);
        if (mask > 0){
                g_object_set_data(G_OBJECT(button), "Bit_Mask", GUINT_TO_POINTER (mask));
                gtk_check_button_set_active (GTK_CHECK_BUTTON (button), (((source) & (mask)) ? 1:0));
        }
        if (mask == 0){
                gtk_check_button_set_active (GTK_CHECK_BUTTON (button), (source)? 1:0);
        }
        if (cb){
                g_signal_connect(G_OBJECT (button), "toggled", G_CALLBACK(cb), cb_data);
        }
        grid_add_widget (button, bwx);
        return button;
}

GtkWidget* BuildParam::grid_add_check_button (const gchar* labeltxt, const gchar *tooltip, int bwx,
                                              GCallback cb, gpointer data, int source, int mask){
        return grid_add_check_button_remote_enabled (labeltxt, tooltip, bwx, cb, data, (guint64)source, (guint64)mask, NULL);
}
GtkWidget* BuildParam::grid_add_check_button_guint64(const gchar* labeltxt, const gchar *tooltip, int bwx,
                                                     GCallback cb, gpointer data, guint64 source, guint64 mask, const gchar *control_id){
        return grid_add_check_button_remote_enabled (labeltxt, tooltip, bwx, cb, data, source, mask, control_id);
}
GtkWidget* BuildParam::grid_add_check_button_simple (const gchar* labeltxt, const gchar *tooltip=NULL, gboolean checked=false){
        return grid_add_check_button_remote_enabled (labeltxt, tooltip, 1, NULL, NULL, checked?1:0, 0);
}
