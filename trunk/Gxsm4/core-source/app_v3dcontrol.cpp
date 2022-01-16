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

#include <locale.h>
#include <libintl.h>

#include "gnome-res.h"

#include "gxsm_app.h"

#include "unit.h"
#include "pcs.h"
#include "xsmtypes.h"
#include "action_id.h"
#include "glbvars.h"

#include "gxsm_app.h"
#include "gxsm_window.h"

#include "app_profile.h"
#include "app_vobj.h"
#include "app_v3dcontrol.h"
#include "surface.h"

#include <gtk/gtk.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <gdk/gdkkeysyms.h>


void V3dControl::configure_callback (GSimpleAction *action, GVariant *parameter, 
                                     gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
                
        g_print ("Toggle action %s activated, state changes from %d to %d\n",
                 g_action_get_name (G_ACTION (action)),
                 g_variant_get_boolean (old_state),
                 g_variant_get_boolean (new_state));

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

	if (g_variant_get_boolean (new_state)){
                g_print ("running GLupdate in manual request.\n");
                Surf3d *s = ((Surf3d*)g_object_get_data (G_OBJECT (vc->glarea), "vdata"));
                Surf3d::GLupdate (s);
	}
}

void V3dControl::timer_update_callback (GSimpleAction *action, GVariant *parameter, 
                                     gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
                
        g_print ("Toggle action %s activated, state changes from %d to %d\n",
                 g_action_get_name (G_ACTION (action)),
                 g_variant_get_boolean (old_state),
                 g_variant_get_boolean (new_state));

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

	if (g_variant_get_boolean (new_state)){
                g_print ("auto timer update on.\n");
                vc->start_auto_update (true);
	} else {
                vc->stop_auto_update ();
        }
}

static GActionEntry win_v3d_gxsm_action_entries[] = {
	{ "view3d-activate", V3dControl::Activate_callback, NULL, NULL, NULL },
	{ "view3d-autodisp", V3dControl::AutoDisp_callback, NULL, NULL, NULL },
	{ "view3d-view-mode", V3dControl::view_view_mode_callback, "s", "'direct'", NULL },
	{ "view3d-gl-mw-mode", V3dControl::view_GL_mouse_wheel_mode_callback, "s", "'zoom'", NULL },
	{ "view3d-gl-mesh", V3dControl::view_GL_Mesh_callback,  NULL, "false", NULL },
	{ "view3d-gl-smooth", V3dControl::view_GL_Smooth_callback, NULL, "true", NULL },
	{ "view3d-gl-ticks", V3dControl::view_GL_Ticks_callback, NULL, "true", NULL },
	{ "view3d-gl-n-zp", V3dControl::view_GL_nZP_callback, NULL, "false", NULL },
	{ "view3d-configure-scene", V3dControl::scene_setup_callback, NULL, NULL, NULL },
	{ "view3d-open", V3dControl::view_file_openhere_callback, NULL, NULL, NULL },
	{ "view3d-save-auto", V3dControl::view_file_save_callback, NULL, NULL, NULL },
	{ "view3d-save-as", V3dControl::view_file_save_as_callback, NULL, NULL, NULL },
	{ "view3d-save-as-image", V3dControl::view_file_save_image_callback, NULL, NULL, NULL },
	{ "view3d-close", V3dControl::view_file_kill_callback, NULL, NULL, NULL },
	{ "view3d-configure", V3dControl::configure_callback, NULL, "false", NULL },
	{ "view3d-auto-update", V3dControl::timer_update_callback, NULL, "false", NULL },
};

// V3dControl
// ========================================

V3dControl::V3dControl (Gxsm4app *app,
                        const char *title, int ChNo, Scan *sc, 
			GCallback resize_event_cb,
			GCallback render_event_cb,
			GCallback realize_event_cb,
			void *vdata):AppBase(app){
	GtkWidget *statusbar;

	XSM_DEBUG(DBG_L2, "V3dControl::V3dControl" );

	WheelFkt = &Surf3d::Zoom;
	scan = sc;
	chno=ChNo;
        au_timer = 0;
        
	AppWindowInit (title);
	g_object_set_data  (G_OBJECT (window), "Ch", GINT_TO_POINTER (ChNo));
	g_object_set_data  (G_OBJECT (window), "ChNo", GINT_TO_POINTER (ChNo+1));

        // FIX-ME GTK4 add new DND
        // main_get_gapp ()->configure_drop_on_widget (GTK_WIDGET (window));
        // gtk_window_set_default_size(GTK_WINDOW(window), 500, 500);

        glarea = gtk_gl_area_new ();
        gtk_gl_area_set_has_depth_buffer (GTK_GL_AREA(glarea), true);

        // set required GL version (4.0)
        gtk_gl_area_set_required_version (GTK_GL_AREA(glarea), 4, 0);
        // gtk_gl_area_set_use_es (GTK_GL_AREA(glarea), true);
        g_object_set_data (G_OBJECT(glarea), "Surf3D", vdata); // Surf3D reference
        
        gtk_widget_set_size_request (glarea, 500, 500);
        gtk_widget_set_hexpand (glarea, TRUE);
        gtk_widget_set_vexpand (glarea, TRUE);

#if 0
        gtk_widget_add_events (glarea,
                               GDK_BUTTON1_MOTION_MASK    |
                               GDK_BUTTON2_MOTION_MASK    |
                               GDK_BUTTON3_MOTION_MASK    |
                               GDK_SCROLL_MASK            |
                               GDK_SHIFT_MASK            |
                               GDK_CONTROL_MASK            |
                               GDK_BUTTON_PRESS_MASK      |
                               GDK_BUTTON_RELEASE_MASK    |
                               GDK_VISIBILITY_NOTIFY_MASK);

	g_signal_connect (G_OBJECT (glarea), "event",
			  (GCallback) V3dControl::glarea_event_cb,
                          this);
#endif
        
        g_signal_connect (G_OBJECT (glarea), "realize",
                          G_CALLBACK (realize_event_cb), vdata);
                          
        g_signal_connect (G_OBJECT (glarea), "resize",
				  G_CALLBACK (resize_event_cb), vdata);

        g_signal_connect (G_OBJECT (glarea), "render",
                          G_CALLBACK (render_event_cb), vdata);

        // put glarea into window and show it all
	gtk_grid_attach (GTK_GRID (v_grid), glarea, 1,1, 1,1);

	// New Statusbar
	statusbar = gtk_statusbar_new ();
	gtk_grid_attach (GTK_GRID (v_grid), statusbar, 1, 10, 10,1);

	g_object_set_data (G_OBJECT (glarea), "vdata", vdata);
	g_object_set_data (G_OBJECT (glarea), "statusbar", statusbar);
	g_object_set_data (G_OBJECT (glarea), "Ch", GINT_TO_POINTER (ChNo));
	g_object_set_data (G_OBJECT (glarea), "V3dControl", this);

	gtk_widget_show (v_grid); // FIX-ME GTK4 SHOWALL

	XSM_DEBUG(DBG_L2, "V3dControl::V3dControl done." );

        if (vdata)
                ((Surf3d*)vdata) -> preferences();

	XSM_DEBUG(DBG_L2, "V3dControl::V3dControl show 3D preferences." );
}

V3dControl::~V3dControl (){
        // remove timeout
        if (au_timer){
                g_source_remove (au_timer);
                au_timer = 0;
        }

        // must terminate as long as glarea is exiting to clean up GL context
        ((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->end_gl () ;
        
	XSM_DEBUG(DBG_L2, "~V3dControl in" );
	XSM_DEBUG(DBG_L2, "~V3dControl out" );
}




void V3dControl::AppWindowInit(const gchar *title, const gchar *sub_title){
	XSM_DEBUG (DBG_L1,  "V3dControl::AppWindowInit** <%s : %s>", title, sub_title?sub_title:"N/A");

        app_window = gxsm4_app_window_new (GXSM4_APP (main_get_gapp ()->get_application ()));
        window = GTK_WINDOW (app_window);

        header_bar = gtk_header_bar_new ();
        gtk_widget_show (header_bar);
        // hide close, min, max window decorations
        //gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header_bar), false);

        g_action_map_add_action_entries (G_ACTION_MAP (app_window),
                                         win_v3d_gxsm_action_entries, G_N_ELEMENTS (win_v3d_gxsm_action_entries),
                                         this);

        // create window PopUp menu  ---------------------------------------------------------------------
        GtkWidget *file_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (main_get_gapp ()->get_view3d_menu ()));
        //g_assert (GTK_IS_MENU (file_menu));

        // attach popup file menu button --------------------------------
        GtkWidget *header_menu_button = gtk_menu_button_new ();
        gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (header_menu_button), "emblem-system-symbolic");
        gtk_menu_button_set_popover (GTK_MENU_BUTTON (header_menu_button), file_menu);
        gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), header_menu_button);
        gtk_widget_show (header_menu_button);

        gtk_window_set_titlebar (GTK_WINDOW (window), header_bar);
        SetTitle (title, sub_title);

	v_grid = gtk_grid_new ();
        gtk_widget_show (v_grid);
        gtk_window_set_child (GTK_WINDOW (window), v_grid);
	g_object_set_data (G_OBJECT (window), "v_grid", v_grid);

	gtk_widget_show (GTK_WIDGET (window)); // FIX-ME GTK4 SHOWALL

	XSM_DEBUG(DBG_L2, "V3dControl::WidgetInit done." );
}

gboolean V3dControl::v3dview_timer_callback (gpointer data){
        V3dControl *vc =  (V3dControl *) data;

        if (!vc->au_timer)
                return false;
        
        if (vc->scan->is_scanning () || vc->auto_update_mode){
                gtk_gl_area_queue_render (GTK_GL_AREA (vc->glarea));
                return true;
        } else {
                vc->au_timer = 0;
                return false;
        }
        
}

void V3dControl::SetActive(int flg){
	GtkWidget *statusbar = (GtkWidget*)g_object_get_data (G_OBJECT (glarea), "statusbar");
	gint statusid  = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "drag");

        gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), statusid);
	if(flg){
		gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, "channel is active now");
	}else{
		gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, "inactive");
	}
}


#if 0
gint V3dControl::glarea_event_cb(GtkWidget *glarea, GdkEvent *event, V3dControl *vc){
	static int dragging=FALSE;
	double x,y;
	x = event->button.x;
	y = event->button.y;
	XSM_DEBUG(DBG_L2, "V3dControl::glarea_event_cb mouse: " << x << ", " << y );

	switch (event->type) {
        case GDK_BUTTON_PRESS:
                ((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->MouseControl ('i', x, y); // init mouse pos
		switch(event->button.button) 
		{
		case 1: 
			dragging=TRUE;
			break;
		case 2:
			dragging=TRUE;
			break;
		case 3: 
			dragging=TRUE;
			break;
		case 4:
			(((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->*(vc->WheelFkt))(1.0);
			break;
		case 5:
			(((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->*(vc->WheelFkt))(-1.0);
			break;
		}
		break;

	case GDK_MOTION_NOTIFY:
                if (dragging && (event->motion.state & GDK_BUTTON1_MASK)){
                        ((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->MouseControl ('R', x, y); // do update
                        gtk_gl_area_queue_render (GTK_GL_AREA (glarea));
                }
                if (dragging && (event->motion.state & GDK_BUTTON2_MASK)){
                        ((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->MouseControl ('T', x, y);
                        gtk_gl_area_queue_render (GTK_GL_AREA (glarea));
                }
                if (dragging && (event->motion.state & GDK_BUTTON3_MASK)){
                        ((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->MouseControl ('M', x, y);
                        gtk_gl_area_queue_render (GTK_GL_AREA (glarea));
                }
		break;
                
	case GDK_SCROLL:
                {
                        GdkScrollDirection direction;
                        GdkModifierType state;
                        gdk_event_get_scroll_direction (event, &direction);
                        gdk_event_get_state (event, &state);
                        switch (direction){
                        case GDK_SCROLL_UP:
                                g_message ("Scroll-Up");
                                if (state & GDK_CONTROL_MASK)
                                        ((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->HeightSkl (0.01);
                                else if (state & GDK_SHIFT_MASK)
                                        ((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->MouseControl ('Z', 1.0, 0.0);
                                else
                                        ((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->MouseControl ('Z', 0., 1.0);
                                gtk_gl_area_queue_render (GTK_GL_AREA (glarea));
                                break;
                        case GDK_SCROLL_DOWN:
                                g_message ("Scroll-Dn");
                                if (state & GDK_CONTROL_MASK)
                                        ((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->HeightSkl (-0.01);
                                else if (state & GDK_SHIFT_MASK)
                                        ((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->MouseControl ('Z', -1.0, 0.0);
                                else
                                        ((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->MouseControl ('Z', 0., -1.0);
                                gtk_gl_area_queue_render (GTK_GL_AREA (glarea));
                                break;
                        case GDK_SCROLL_LEFT:
                                g_message ("Scroll-L");
                                break;
                        case GDK_SCROLL_RIGHT:
                                g_message ("Scroll-R");
                                break;
                        case GDK_SCROLL_SMOOTH:
                                g_message ("Scroll-Smmoth");
                                break;
                        }

                }
                break;
                
	case GDK_BUTTON_RELEASE:
		switch(event->button.button){
		case 1:
			dragging=FALSE;
                        ((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->MouseControl ('r', x, y); // set current as new ref
			break;
		case 2:
			dragging=FALSE;
                        ((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->MouseControl ('t', x, y);
			break;
		case 3:
			dragging=FALSE;
                        ((Surf3d*)g_object_get_data (G_OBJECT (glarea), "vdata"))->MouseControl ('m', x, y);
			break;
		}
		break;
	default: break;
	}
	return 0;
}
#endif

void V3dControl::Activate_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
	main_get_gapp ()->xsm->ActivateChannel(vc->chno);
}
void V3dControl::SetOff_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
	main_get_gapp ()->xsm->SetMode(vc->chno, ID_CH_M_OFF);
}
void V3dControl::SetOn_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
	main_get_gapp ()->xsm->SetMode(vc->chno, ID_CH_M_ON);
}
void V3dControl::SetMath_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
	main_get_gapp ()->xsm->SetMode(vc->chno, ID_CH_M_MATH);
}
void V3dControl::SetX_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
	main_get_gapp ()->xsm->SetMode(vc->chno, ID_CH_M_X);
}

void V3dControl::AutoDisp_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
        main_get_gapp ()->xsm->ActivateChannel(vc->chno);
	//main_get_gapp ()->xsm->AutoDisplay();
	((Surf3d*)g_object_get_data (G_OBJECT (vc->glarea), "vdata"))->update(0, vc->scan->mem2d->GetNy());
        //        ((Surf3d*)g_object_get_data (G_OBJECT (vc->glarea), "vdata"))->GLdrawscene(-1., true);
        gtk_gl_area_queue_render (GTK_GL_AREA (vc->glarea));
}

void V3dControl::apply_all_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
	((Surf3d*)g_object_get_data (G_OBJECT (vc->glarea), "vdata"))
		->GLupdate(g_object_get_data (G_OBJECT (vc->glarea), "vdata"));
}

void V3dControl::view_file_openhere_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
	main_get_gapp ()->xsm->ActivateChannel(vc->chno);
	main_get_gapp ()->xsm->load();
}

void V3dControl::view_file_save_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
	main_get_gapp ()->xsm->save(AUTO_NAME_SAVE, NULL, vc->chno);
}

void V3dControl::view_file_save_as_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
	main_get_gapp ()->xsm->save(MANUAL_SAVE_AS, NULL, vc->chno);
}

void V3dControl::view_file_save_image_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;

	gchar *suggest = g_strdup_printf ("%s-GL3D.png", vc->scan->data.ui.originalname);
        //gchar *name = g_path_get_basename (suggest);
        gchar *path = g_path_get_dirname (suggest);
        
        GtkFileFilter *f0 = gtk_file_filter_new ();
        gtk_file_filter_set_name (f0, "All");
        gtk_file_filter_add_pattern (f0, "*");

        GtkFileFilter *fpng = gtk_file_filter_new ();
        gtk_file_filter_add_mime_type (fpng, "image/png");
        gtk_file_filter_set_name (fpng, "Images");
        gtk_file_filter_add_pattern (fpng, "*.png");

        GtkFileFilter *filter[] = { fpng, f0, NULL };
                
        gchar *imgname = main_get_gapp ()->file_dialog_save ("Save GL 3D View as png file", path, suggest, filter);
	g_free (suggest);

	if (imgname == NULL || strlen(imgname) < 5) 
		return;

        ((Surf3d*)g_object_get_data (G_OBJECT (vc->glarea), "vdata"))->
                SaveImagePNG (GTK_GL_AREA (vc->glarea), imgname);
}

void V3dControl::view_file_print_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
	main_get_gapp ()->xsm->ActivateChannel(vc->chno);
	//	main_get_gapp ()->file_print_callback(widget, NULL);
	XSM_DEBUG(DBG_L2, "VIEW PRINT CALLBACK!\n" );
}

void V3dControl::view_file_kill_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
	main_get_gapp ()->xsm->SetMode(vc->chno, ID_CH_M_OFF);
}

void V3dControl::view_view_mode_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
        GVariant *old_state, *new_state;

        gint mode=SCAN_V_QUICK; // fall back
        
        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_string (g_variant_get_string (parameter, NULL));
                
        g_print ("VIEW-MODE Radio action %s activated, state changes from %s to %s\n",
                 g_action_get_name (G_ACTION (action)),
                 g_variant_get_string (old_state, NULL),
                 g_variant_get_string (new_state, NULL));


        if (!strcmp (g_variant_get_string (new_state, NULL), "quick")){
                mode = SCAN_V_QUICK;
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "direct")){
                mode = SCAN_V_DIRECT;
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "hilit")){
                mode = SCAN_V_HILITDIRECT;
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "plane")){
                mode = SCAN_V_PLANESUB;
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "horizontal")){
                mode = SCAN_V_HORIZONTAL;
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "periodic")){
                mode = SCAN_V_PERIODIC;
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "log")){
                mode = SCAN_V_LOG;
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "diff")){
                mode = SCAN_V_DIFFERENTIAL;
        }

   	if (vc->chno < 0){
                vc->scan->SetVM(mode);
        } else {
                main_get_gapp ()->xsm->ActivateChannel(vc->chno);
		main_get_gapp ()->xsm->SetVM(mode);
        }
      
        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);
}

void V3dControl::view_GL_Mesh_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
                
        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        ((Surf3d*)g_object_get_data (G_OBJECT (vc->glarea), "vdata"))->
		GLModes(ID_GL_Mesh, g_variant_get_boolean (new_state));
}

void V3dControl::view_GL_Ticks_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        ((Surf3d*)g_object_get_data (G_OBJECT (vc->glarea), "vdata"))->
		GLModes(ID_GL_Ticks, g_variant_get_boolean (new_state));
}

void V3dControl::view_GL_Smooth_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
                
        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        ((Surf3d*)g_object_get_data (G_OBJECT (vc->glarea), "vdata"))->
		GLModes(ID_GL_Smooth, g_variant_get_boolean (new_state));
}

void V3dControl::view_GL_nZP_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
        GVariant *old_state, *new_state;

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
                
        g_print ("Toggle action %s activated, state changes from %d to %d\n",
                 g_action_get_name (G_ACTION (action)),
                 g_variant_get_boolean (old_state),
                 g_variant_get_boolean (new_state));

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);

        ((Surf3d*)g_object_get_data (G_OBJECT (vc->glarea), "vdata"))->
		GLModes(ID_GL_nZP, g_variant_get_boolean (new_state));
}

void V3dControl::view_GL_mouse_wheel_mode_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
        GVariant *old_state, *new_state;

        // default
        vc->WheelFkt = &Surf3d::Zoom; 

        old_state = g_action_get_state (G_ACTION (action));
        new_state = g_variant_new_string (g_variant_get_string (parameter, NULL));
                
        g_print ("GL-MW-MODE Radio action %s activated, state changes from %s to %s\n",
                 g_action_get_name (G_ACTION (action)),
                 g_variant_get_string (old_state, NULL),
                 g_variant_get_string (new_state, NULL));


        if (!strcmp (g_variant_get_string (new_state, NULL), "zoom")){
                vc->WheelFkt = &Surf3d::Zoom; 
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "rot-x")){
                vc->WheelFkt = &Surf3d::RotateX; 
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "rot-y")){
                vc->WheelFkt = &Surf3d::RotateY; 
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "rot-z")){
                vc->WheelFkt = &Surf3d::RotateZ; 
        } else if (!strcmp (g_variant_get_string (new_state, NULL), "z-scale")){
		vc->WheelFkt = &Surf3d::HeightSkl; 
        }

        g_simple_action_set_state (action, new_state);
        g_variant_unref (old_state);
}

void V3dControl::scene_setup_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data){
        V3dControl *vc = (V3dControl *) user_data;
	((Surf3d*)g_object_get_data (G_OBJECT (vc->glarea), "vdata"))->
		preferences();
}
