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

// Gxsm headers 
#include "gxsm_app.h"
#include "gxsm_window.h"

#include "gapp_service.h"
#include "pcs.h"
#include "glbvars.h"


// ============================================================
// MyGnomeTools
// ============================================================



GMenuModel *MyGnomeTools::find_extension_point_section (GMenuModel  *model,
                                                        const gchar *extension_point)
{
        const gint dbg=0;
        gint i, n_items;
        GMenuModel *section = NULL;

        if (model == NULL)
                return NULL;

        if (dbg > 0) g_print ("MyGnomeTools::find_extension_point_section '%s' in menu.\n", extension_point);

        n_items = g_menu_model_get_n_items (model);

        if (dbg > 0) g_print ("MyGnomeTools::find_extension_point_section n_items=%d\n", n_items);

        for (i = 0; i < n_items && !section; i++) {
                gchar *id = NULL;
                
                gboolean ret=g_menu_model_get_item_attribute (model, i, "id", "s", &id);
                if (dbg > 0) g_print ("MyGnomeTools::find_extension_point_section i=%d  id: %s %s\n", i, id, ret?"YES":"NO");
                if (id) { g_free (id); } id = NULL;
                gboolean retx1=g_menu_model_get_item_attribute (model, i, "name", "s", &id);
                if (dbg > 0) g_print ("MyGnomeTools::find_extension_point_section i=%d name: %s %s\n", i, id, retx1?"YES":"NO");
                if (id) { g_free (id); } id = NULL;
                gboolean retx2=g_menu_model_get_item_attribute (model, i, "label", "s", &id);
                if (dbg > 0) g_print ("MyGnomeTools::find_extension_point_section i=%d label: %s %s\n", i, id, retx2?"YES":"NO");
                if (id) { g_free (id); } id = NULL;


                if (ret){
                        if (g_menu_model_get_item_attribute (model, i, "id", "s", &id) &&
                            strcmp (id, extension_point) == 0) {
                                section = g_menu_model_get_item_link (model, i, G_MENU_LINK_SECTION); 
                                if (id) g_free (id);
                                return section;
                        }
                }
                
                GMenuModel *subsection;
                GMenuModel *submenu;
                gint j, j_items;
                
                if (dbg > 0) g_print ("MyGnomeTools::find_extension_point_section subsecton lookup\n");

                subsection = g_menu_model_get_item_link (model, i, G_MENU_LINK_SECTION);
                if (subsection == NULL){ // try here:
                        submenu = g_menu_model_get_item_link (model, i, G_MENU_LINK_SUBMENU);
                        if (submenu)
                                section = find_extension_point_section (submenu, extension_point);
                } else {
                        //                        if (subsection) {
                        j_items = g_menu_model_get_n_items (subsection);
                        if (dbg > 0) g_print ("MyGnomeTools::find_extension_point_section subsecton lookup -- j_items=%d\n", j_items);
                        
                        for (j = 0; j < j_items && !section; j++) {
                                if (dbg > 0) g_print ("MyGnomeTools::find_extension_point_section submenu lookup -- j_items=%d\n", j);
                                submenu = g_menu_model_get_item_link (subsection, j, G_MENU_LINK_SUBMENU);
                                if (submenu)
                                        section = find_extension_point_section (submenu, extension_point);
                        }
                }
        }

        return section;
}


// ============================================================
// GnomeAppService
// ============================================================


gint GnomeAppService::setup_multidimensional_data_copy (const gchar *title, Scan *src, int &ti, int &tf, int &vi, int &vf,
                                                        int *tnadd, int *vnadd, int *crop_window_xy, gboolean crop){
	UnitObj *Pixel = new UnitObj("Pix","Pix");
	UnitObj *Unity = new UnitObj(" "," ");
	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_(title),
							 window, 
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
        // FIX-ME-GTK4
        g_signal_connect (dialog, "response", G_CALLBACK (GnomeAppService::on_dialog_response), this);
        while (dialog_response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

	delete Pixel;
	delete Unity;

        return dialog_response;
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
	
        dialog_action   = NULL;
        dialog_var      = 0;
        dialog_response = GTK_RESPONSE_NONE;
	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_(title),
							 window,
							 (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
							 N_(b1), 1,
							 N_(b2), 2,
							 N_(b3), 3,
							 NULL);

	gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), label);
        g_signal_connect (dialog, "response", G_CALLBACK (GnomeAppService::on_dialog_response), this);
        while (dialog_response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

	return dialog_response;
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
                                                               gapp->get_window (), 
                                                               (GtkDialogFlags)((modal ? GTK_DIALOG_MODAL:0) | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                               NULL, NULL, NULL);
                //_("_Cancel"),
                // GTK_RESPONSE_REJECT,
                // NULL);

                //progress_dialog = gtk_dialog_new ();

		for (int i=0; i<MAX_PROGRESS_LEVELS; ++i)
			progress_bar[i]	= NULL;
	} else {
                gtk_window_set_title (GTK_WINDOW (progress_dialog), N_(title?title:"Progress"));
        }

	//	Add GtkProgressBar

	if (levels>0)
		for (int i=0; i<levels && i<MAX_PROGRESS_LEVELS; ++i){

			if (!progress_bar[i]){
				progress_bar[i]	= gtk_progress_bar_new ();
				gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (progress_dialog))), progress_bar[i]);
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
        GnomeAppService *g = (GnomeAppService *) user_data;
        if (response == GTK_RESPONSE_ACCEPT){
                GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
                g_autoptr(GFile) file = gtk_file_chooser_get_file (chooser);
                g_free (g->tmp_filename);
                g->tmp_filename=g_file_get_parse_name (file);
        }
        gtk_window_destroy (GTK_WINDOW (dialog));
}

/*
  must g_free returned file name!
  GtkFileFilter *filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "all");
  gtk_file_filter_add_pattern (filter, "*");
*/
gchar *GnomeAppService::file_dialog_save (const gchar *title, 
                                          const gchar *path, 
                                          const gchar *name,
                                          GtkFileFilter **filter
                                          ){
        GtkWidget *chooser = gtk_file_chooser_dialog_new (title,
                                                          window, // parent_window,
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

        g_free (tmp_filename);
        tmp_filename = NULL;
        g_signal_connect (chooser, "response", G_CALLBACK (GnomeAppService::file_dialog_completed), this);

        // FIX-ME GTK4 non ideal rework
        while (!tmp_filename)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
        
        return g_strdup (tmp_filename); // must g_free is non NULL
}

gchar *GnomeAppService::file_dialog_load (const gchar *title, 
                                          const gchar *path, 
                                          const gchar *name,
                                          GtkFileFilter **filter
                                          ){
        GtkWidget *chooser = gtk_file_chooser_dialog_new (title,
                                                          window, // parent_window,
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
        
        g_free (tmp_filename);
        tmp_filename = NULL;
        g_signal_connect (chooser, "response", G_CALLBACK (GnomeAppService::file_dialog_completed), this);

        // FIX-ME GTK4 non ideal rework
        while (!tmp_filename)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
        
        return g_strdup (tmp_filename); // must g_free is non NULL
}


void  GnomeAppService::string_cb (gchar *string, gpointer data){
}

void GnomeAppService::destroy (GtkWidget *widget, GnomeAppService *p){
	if(p->fdlg == -1)
		p->fname=NULL;
	p->fdlg=0;
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
	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_(title),
							 window,
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
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
}

// label list must be NULL terminated array.
void GnomeAppService::ValueRequestList (const gchar *title,
                                        const gchar *label[], const gchar *infotxt[], 
                                        UnitObj *uobj[], double minv[], double maxv[], const gchar *vfmt[],
                                        double *value[]){

	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_(title),
							 window,
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
        if(window){
                GtkWidget *dialog = gtk_message_dialog_new_with_markup (window,
                                                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                                                        GTK_MESSAGE_WARNING,
                                                                        GTK_BUTTONS_CLOSE,
                                                                        "<span foreground='red' size='large' weight='bold'>%s</span>\n%s\n%s", s1, s2, s3);
                g_signal_connect_swapped (G_OBJECT (dialog), "response",
                                          G_CALLBACK (gtk_window_destroy),
                                          G_OBJECT (dialog));

                gtk_widget_show (dialog);

                if (c > 5){
                        g_message ("adding timeout for forced exit");
                        g_object_set_data (G_OBJECT (dialog), "SM", (gpointer)g_strdup_printf ("<span foreground='red' size='large' weight='bold'>%s</span>\n%s\n%s", s1, s2, s3));
                        g_object_set_data (G_OBJECT (dialog), "CM", GINT_TO_POINTER (c));
                        g_timeout_add ((guint)1000, 
                                       (GSourceFunc) GnomeAppService::terminate_timeout_func, 
                                       dialog
                                       );
                }
        }
}


// ============================================================
// AppBase
// ============================================================

AppBase::AppBase(){ 
	XSM_DEBUG(DBG_L2, "AppBase" ); 
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
	XSM_DEBUG (DBG_L2, "AppBase::~AppBase destructor for window '" << (window_key?window_key:"--") << "'."); 

        if (gapp)
                gapp->remove_appwindow_from_list (this); // remove self from list
        
	if(!nodestroy){
		XSM_DEBUG_GP (DBG_L2, "~AppBase -- calling widget destroy for window '%s'.",  (window_key?window_key:"--")); 
		destroy();
	}
        
        if (window_key)
                g_free(window_key);

        if (window_geometry)
                g_free (window_geometry);

	XSM_DEBUG (DBG_L2, "AppBase::~AppBase done." );
}

void AppBase::SetTitle(const gchar *title, const gchar *sub_title){
        if (sub_title){
                if (!title_label){
                        title_label = gtk_label_new (NULL);
                        gtk_label_set_single_line_mode (GTK_LABEL (title_label), false);
                        gtk_label_set_ellipsize (GTK_LABEL (title_label), PANGO_ELLIPSIZE_END);
                        gtk_header_bar_set_title_widget( GTK_HEADER_BAR (header_bar), title_label);
                        gtk_widget_show (title_label);
                }
                const char *format1 =
                        "<span size='large' weight='bold'>\%s</span>\n"
                        "<span size='small' weight='bold'>\%s</span>";
                char *markup;
                markup = g_markup_printf_escaped (format1, title?title:"", sub_title);
                gtk_label_set_markup (GTK_LABEL (title_label), markup);
                g_free (markup);
        }
        else
                if (title)
                        gtk_window_set_title (GTK_WINDOW (window), title);
}

void AppBase::AppWindowInit(const gchar *title, const gchar *sub_title){
	XSM_DEBUG(DBG_L2, "AppBase::WidgetInit: " << title );

        app_window =  gxsm4_app_window_new (GXSM4_APP (gapp->get_application ()));    // gtk_application_window_new (GTK_APPLICATION (gapp->get_application ()));
        window = GTK_WINDOW (app_window);

        //window = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
        header_bar = gtk_header_bar_new ();
        gtk_widget_show (header_bar);

        // gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header_bar), true);
        // gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), GtkWidget *child);

        SetTitle (title, sub_title);
        gtk_window_set_titlebar (GTK_WINDOW (window), header_bar);

	v_grid = gtk_grid_new ();
	g_object_set_data (G_OBJECT (window), "v_grid", v_grid);
        gtk_window_set_child (GTK_WINDOW (window), v_grid);
	gtk_widget_show (GTK_WIDGET (v_grid)); // FIX-ME GTK4 SHOWALL

        //g_signal_connect (G_OBJECT (window), "window-state-event", G_CALLBACK (AppBase::window_state_watch_callback), this);
        // g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (AppBase::window_close_callback), this);
        //  g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);

        // FIX-ME GTK4
        gtk_widget_show (GTK_WIDGET (window));
}

gboolean AppBase::window_close_callback (GtkWidget *widget,
                                         GdkEvent  *event,
                                         gpointer   user_data){
        AppBase *app_w = (AppBase *)user_data;
        // dop NOT close/destrry, just minimize/hide!
        app_w->hide ();

        return true; // no further actions!!
}

#if 0
gboolean AppBase::window_state_watch_callback (GtkWidget *widget,
                                               GdkEvent  *event,
                                               gpointer   user_data){
        AppBase *app_w = (AppBase *)user_data;

        GdkEventWindowState *ws = (GdkEventWindowState *)event;

	XSM_DEBUG (DBG_L2, "AppBase::window_state_watch_callback: window_new_state=" << ws->new_window_state);

        switch (ws->new_window_state){
        case GDK_WINDOW_STATE_ICONIFIED: app_w->showstate=FALSE; break;
        case GDK_WINDOW_STATE_MAXIMIZED: break;
        case GDK_WINDOW_STATE_STICKY: break;
        case GDK_WINDOW_STATE_FULLSCREEN: break;
        case GDK_WINDOW_STATE_ABOVE: break;
        case GDK_WINDOW_STATE_BELOW: break;
        case GDK_WINDOW_STATE_FOCUSED: break;
        case GDK_WINDOW_STATE_TILED: break;
        case GDK_WINDOW_STATE_WITHDRAWN: break;
        default: break;
        }
        
        return false;
}
#endif
        
void AppBase::window_action_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
        AppBase *app_w = (AppBase *)user_data;
        app_w->show ();
}

void AppBase::add_window_to_window_menu(const gchar *menu_item_label, const gchar* key){
        const gchar *menusection = "windows-section";
        
        if (strlen(menu_item_label) < 1)
                return;

        // generate valid action string from menu path
        gchar *tmp = g_strconcat (menusection, "-show-", key, NULL);
        g_strdelimit (tmp, " /:", '-');
        gchar *tmpaction = g_strconcat ( tmp, NULL);
        GSimpleAction *ti_action;
        
        XSM_DEBUG_GP (DBG_L2, "AppBase::add_window_to_window_menu <%s>  MenuItem= %s ==> label, generated action: [%s] <%s>\n", menusection, menu_item_label, tmp, tmpaction );

        if (!strcmp (menusection, "windows-section-xx")) { // add toggle -- testing, not yet working -- disabled via -xx
                ti_action = g_simple_action_new_stateful (tmpaction,
                                                          G_VARIANT_TYPE_BOOLEAN,
                                                          g_variant_new_boolean (true));
                g_signal_connect (ti_action, "toggled", G_CALLBACK (AppBase::window_action_callback), this); // GTK_APPLICATION ( gapp->get_application ()));
        } else {
                ti_action = g_simple_action_new (tmpaction, NULL);
                g_signal_connect (ti_action, "activate", G_CALLBACK (AppBase::window_action_callback), this);
        }
        
        g_object_set_data (G_OBJECT (ti_action), "AppBase", this);

        g_action_map_add_action (G_ACTION_MAP ( gapp->get_application ()), G_ACTION (ti_action));

        gchar *app_tmpaction = g_strconcat ( "app.", tmpaction, NULL);

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
        gapp->gxsm_app_extend_menu (menusection, label, app_tmpaction);

        g_free (label);
        g_free (app_tmpaction);
        g_free (tmpaction);
        g_free (tmp);
}

int AppBase::set_window_geometry (const gchar *key, gint index, gboolean add_to_menu){
        XSM_DEBUG_GP (DBG_L4, "AppBase::set_window_geometry and append '%s' to Windows Menu.\n", key);

        if (window_key){
                XSM_DEBUG_GP (DBG_L2, "AppBase::set_window_geometry geometry already setup. DUPLICATE WARNING append '%s' to Windows Menu already done.\n", key);
                // remove from menu!
                g_free (window_key);
        }
        
        XSM_DEBUG_GP (DBG_L9, "AppBase::set_window_geometry and append '%s' to Windows Menu\n", key);
        if (index >= 0 && index <= 6) // limit for now
                window_key = g_strdup_printf ("%s-%d", key, index);
        else if (index > 6)
                return -1; // do not handle at this time.
        else
                window_key = g_strdup (key);

        XSM_DEBUG_GP (DBG_L9, "AppBase::set_window_geometry and append '%s' to Windows Menu -- cpy2.\n", window_key);

	LoadGeometry ();

        XSM_DEBUG_GP (DBG_L9, "AppBase::set_window_geometry and append '%s' to Windows Menu -- add to menu.\n", window_key);

        if (add_to_menu)
                add_window_to_window_menu (window_key, window_key);

        if (gapp)
                gapp->add_appwindow_to_list (this); // add self to gapp globale list

        XSM_DEBUG_GP (DBG_L9, "AppBase::set_window_geometry and append '%s' to Windows Menu -- done.\n", window_key);
	return 0;
}

void AppBase::hide (){
        showstate=FALSE;
        //GTK4-FIX-ME
        //gtk_window_iconify (window);
}

void AppBase::show (){
        //GTK4-FIX-ME
        //gtk_window_deiconify (window);
        //gtk_window_present (window);
        showstate=TRUE;
        position_auto ();
        resize_auto ();
}

void AppBase::show_auto (){
        if (window_geometry){
                if (window_geometry[WGEO_FLAG] && window_geometry[WGEO_SHOW]){
                        show ();
                } else
                        hide ();
        } else
                hide ();
}

void AppBase::position_auto (){
        if (window_geometry)
                if (window_geometry[WGEO_FLAG])
                        g_message ("SORRY GTK4 can't do it -- no gtk_window_move (GTK_WINDOW (window), window_geometry[WGEO_XPOS], window_geometry[WGEO_YPOS]);");
                        //gtk_window_move (GTK_WINDOW (window), window_geometry[WGEO_XPOS], window_geometry[WGEO_YPOS]);
}

void AppBase::resize_auto (){
        if (window_geometry)
                if (window_geometry[WGEO_FLAG])
                        g_message ("SORRY GTK4 can't do it -- no gtk_window_resize (GTK_WINDOW (window), window_geometry[WGEO_WIDTH], window_geometry[WGEO_HEIGHT]);");
                        //gtk_window_resize (GTK_WINDOW (window), window_geometry[WGEO_WIDTH], window_geometry[WGEO_HEIGHT]);
}

void AppBase::SaveGeometryCallback(AppBase *apb){
	apb->SaveGeometry();
}

int AppBase::SaveGeometry(int savealways){
        if (! window_key){
                XSM_DEBUG_ERROR (DBG_L1, "ERROR ** AppBase::SaveGeometry called with no window_key.");
                return -1;
        }
	XSM_DEBUG (DBG_L2, "** AppBase::SaveGeometry of " << window_key);

        // g_message ("AutoSave Window Geometry: %s", window_key);

        // just in case it was not loaded right
        if (!window_geometry)
                window_geometry = g_new (gint32, WGEO_SIZE);
        
        window_geometry[WGEO_FLAG] = 1;
        window_geometry[WGEO_SHOW] = showstate;
        g_message ("SORRY GTK4 can't do it -- no gtk_window_get_position (GTK_WINDOW (window), &window_geometry[WGEO_XPOS], &window_geometry[WGEO_YPOS]);");
        g_message ("SORRY GTK4 can't do it! -- no gtk_window_get_size (GTK_WINDOW (window), &window_geometry[WGEO_WIDTH], &window_geometry[WGEO_HEIGHT]);");
        //gtk_window_get_position (GTK_WINDOW (window), &window_geometry[WGEO_XPOS], &window_geometry[WGEO_YPOS]); 
        //gtk_window_get_size (GTK_WINDOW (window), &window_geometry[WGEO_WIDTH], &window_geometry[WGEO_HEIGHT]);

        GVariant *storage = g_variant_new_fixed_array (g_variant_type_new ("i"), window_geometry, WGEO_SIZE, sizeof (gint32));
        g_settings_set_value (geometry_settings, window_key, storage);
  
        //g_free (storage); // ??
      
	return 0;
}

int AppBase::LoadGeometry(){
        if (!window_key){
                XSM_DEBUG_ERROR (DBG_L1, "AppBase::LoadGeometry -- error, no window_key set.");
                return -1;
        }
	XSM_DEBUG (DBG_L2, "AppBase::LoadGeometry -- Load Geometry for window " << window_key );

        // g_message ("AutoLoad Window Geometry: %s", window_key);

        gsize n_stores;

        GVariant *storage = g_settings_get_value (geometry_settings, window_key);
        gint32 *tmp = (gint32*) g_variant_get_fixed_array (storage, &n_stores, sizeof (gint32));

        if (!window_geometry)
                window_geometry = g_new (gint32, WGEO_SIZE);

        memcpy (window_geometry, tmp, WGEO_SIZE*sizeof (gint32));

        // g_free (storage); // ??

        g_assert_cmpint (n_stores, ==, WGEO_SIZE);

        position_auto ();
#if 0
        resize_auto ();
#endif
        show_auto ();

	return 0;
}
