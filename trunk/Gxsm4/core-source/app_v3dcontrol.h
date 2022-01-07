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

#ifndef APP_V3DCONTROL_H
#define APP_V3DCONTROL_H

#include "view.h"

class V3dControl : public AppBase{
public:
        V3dControl(Gxsm4app *app,
                   const char *title, int ChNo, Scan *scan,
                   GCallback resize_event_cb,
                   GCallback render_event_cb,
                   GCallback realize_event_cb,
                   void *vdata);
        virtual ~V3dControl();

        virtual void AppWindowInit(const gchar *title=NULL, const gchar *sub_title=NULL);

        static void configure_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void timer_update_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void view_file_openhere_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void view_file_save_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void view_file_save_as_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void view_file_save_image_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void view_file_print_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void view_file_kill_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);

        static void view_view_mode_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void view_GL_mouse_wheel_mode_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);

        static void view_GL_nZP_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void view_GL_Mesh_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void view_GL_Ticks_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void view_GL_Smooth_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);

        static gint glarea_event_cb(GtkWidget *glarea, GdkEvent *event, V3dControl *vc);
        static void Activate_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void apply_all_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void scene_setup_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);

        static void SetOff_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void SetOn_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void SetMath_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void SetX_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);
        static void AutoDisp_callback (GSimpleAction *action, GVariant *parameter, gpointer user_data);

        void rerender_scene (){
                gtk_gl_area_queue_render (GTK_GL_AREA (glarea));
        };

        GtkGLArea* get_glarea () { return GTK_GL_AREA (glarea); };
                
        void start_auto_update (gboolean override=false){
                auto_update_mode = override;
                if (!au_timer)
                        au_timer =  g_timeout_add (20, (GSourceFunc)  v3dview_timer_callback, this);
        };
        void stop_auto_update (gboolean override=false){
                auto_update_mode = override;
        };
        static gboolean v3dview_timer_callback (gpointer data);
        void SetActive(int flg);
        void CheckOptions();

        double(Surf3d::*WheelFkt)(double);
  
private:
        gboolean auto_update_mode;
        guint au_timer;
        int chno;
        Scan *scan;
        GtkWidget *glarea;
};

#endif
