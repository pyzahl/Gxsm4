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

#ifndef APP_VPDATA_VIEW_H
#define APP_VPDATA_VIEW_H

#include <stdlib.h>
#include "gxsm_window.h"
#include "gapp_service.h"
#include "app_profile.h"
#include "glbvars.h"

class app_vpdata_view : public AppBase{
public:
        app_vpdata_view (Gxsm4app *app, gint num_active_xmaps, gint num_active_sources)
                : AppBase(app){
                for (int i=0; i<32; ++i) tmp_pc[i]=NULL;
                vpdata_graph_app_window = NULL;
                vpdata_graph_window = NULL;
                vpdata_graph_grid = NULL;
                vpdata_gr_matrix_view = true;

                init_vpdata_view (num_active_xmaps, num_active_sources);
        };
        virtual ~app_vpdata_view(){
                vpdata_view_destroy ();
        };

        void init_vpdata_view (gint num_active_xmaps, gint num_active_sources){
                if (vpdata_graph_app_window)
                        vpdata_view_destroy (); 

                if (!vpdata_graph_app_window){
                        vpdata_graph_app_window =  gxsm4_app_window_new (gxsm4app);
                        vpdata_graph_window = GTK_WINDOW (vpdata_graph_app_window);
                        GtkWidget *header_bar = gtk_header_bar_new ();
                        gtk_widget_show (header_bar);
                        //gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header_bar), true);

                        //gtk_header_bar_set_subtitle (GTK_HEADER_BAR  (header_bar), "last plot");
                        gtk_window_set_titlebar (GTK_WINDOW (vpdata_graph_window), header_bar);
                        SetTitle (NULL, "last plot");

                        vpdata_graph_grid = gtk_grid_new ();
                        g_object_set_data (G_OBJECT (vpdata_graph_window), "v_grid", vpdata_graph_grid);
                        gtk_box_append (GTK_BOX (vpdata_graph_window), vpdata_graph_grid);
                        gtk_widget_show (GTK_WIDGET (vpdata_graph_window)); // FIX-ME GTK4 SHOWALL

                        GtkWidget *statusbar = gtk_statusbar_new ();
                        g_object_set_data (G_OBJECT (vpdata_graph_window), "statusbar", statusbar);
                        gtk_grid_attach (GTK_GRID (vpdata_graph_grid), statusbar, 1,100, 100,1);
                        gtk_widget_show (GTK_WIDGET (statusbar)); // FIX-ME GTK4 SHOWALL
                }
                // SetTitle (vp_exec_mode_name);
        };

        void vpdata_view_destroy (){
                if (!vpdata_gr_matrix_view && vpdata_graph_app_window){
                        gtk_window_destroy (GTK_WINDOW (vpdata_graph_app_window));
                        vpdata_graph_app_window = NULL;
                        vpdata_graph_window = NULL;
                        vpdata_graph_grid = NULL;
                }
        };
        

        void vpdata_add_pc (ProfileControl* &pc, const gchar *title, gint numx,
                            const gchar *xlab, UnitObj *UXaxis,
                            const gchar *ylab, UnitObj *UYaxis,
                            double xmin, double xmax,
                            int si, int nas, gboolean join_same_x,
                            gint xmap, gint src, gint num_active_xmaps, gint num_active_sources){
                if (!pc){
                        gchar *resid = g_strdelimit (g_strconcat (xlab,ylab,NULL), " ;:()[],./?!@#$%^&*()+-=<>", '_');

                        pc = new ProfileControl (gxsm4app,
                                                 title, numx, 
                                                 UXaxis, UYaxis, 
                                                 xmin, xmax,
                                                 resid,
                                                 vpdata_gr_matrix_view ? vpdata_graph_app_window : NULL);

                        if (vpdata_gr_matrix_view){
                                pc->set_pc_matrix_size (num_active_xmaps, num_active_sources);
                                gtk_grid_attach (GTK_GRID (vpdata_graph_grid), pc->get_pc_grid (), xmap,src, 1,1);
                        }
                
                        g_free (resid);
                        pc->scan1d->mem2d->Resize (numx, join_same_x ? nas:1);
                        pc->SetGraphTitle (title);
                        pc->set_ys_label (0, ylab);
                        pc->SetData_dz (1.); // force dz=1.
                } else {	
                        if (vpdata_gr_matrix_view)
                                pc->set_pc_matrix_size (num_active_xmaps, num_active_sources);

                        if(!join_same_x || nas < pc->get_scount ()){
                                pc->RemoveScans ();
                                pc->scan1d->mem2d->Resize (numx, join_same_x ? nas:1);
                                pc->AddScan (pc->scan1d, 0);
                        } else
                                pc->scan1d->mem2d->Resize (numx, join_same_x ? nas:1);

                        pc->scan1d->data.s.ny = join_same_x ? nas:1;
                        pc->scan1d->data.s.x0=0.;
                        pc->scan1d->data.s.y0=0.;
                        pc->scan1d->data.s.nvalues=1;
                        pc->scan1d->data.s.ntimes=1;
                        pc->SetXrange (xmin, xmax);
                        pc->SetData_dz (1.); // force dz=1.

                        if (join_same_x && si >= pc->get_scount ()){
                                pc->AddLine (si);
                                pc->SetGraphTitle (", ", TRUE);
                                pc->SetGraphTitle (ylab, TRUE);
                                pc->set_ys_label (si, ylab);
                        }
                }
        }


        ProfileControl *tmp_pc[32]; 
        Gxsm4appWindow *vpdata_graph_app_window;
        GtkWindow* vpdata_graph_window;
        GtkWidget* vpdata_graph_grid;
        gboolean vpdata_gr_matrix_view;
};

#endif // APP_VPDATA_VIEW_H
