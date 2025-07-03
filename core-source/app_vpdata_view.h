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

                AppWindowInit (N_("GXSM VP Data Viewer"), N_("VP File Preview"));

                vpdata_gr_matrix_view = true;
                
                init_vpdata_view (num_active_xmaps, num_active_sources);
        };
        virtual ~app_vpdata_view(){
                vpdata_view_destroy ();
        };

        virtual void AppWindowInit(const gchar *title, const gchar *sub_title){
                XSM_DEBUG_GM (DBG_L1, "app_vpdata_view::AppWindowInit** <%s : %s> **", title, sub_title?sub_title:"N/A");
                app_window = gxsm4_app_window_new (gxsm4app);
                window = GTK_WINDOW (app_window);

                header_bar = gtk_header_bar_new ();
                gtk_widget_show (header_bar);

                SetTitle (title, sub_title);
                gtk_window_set_titlebar (GTK_WINDOW (window), header_bar);

                v_grid = gtk_grid_new ();
                gtk_window_set_child (GTK_WINDOW (window), v_grid);
                g_object_set_data (G_OBJECT (window), "v_grid", v_grid);

                gtk_widget_show (GTK_WIDGET (window)); // FIX-ME GTK4 SHOWALL

        };


        
        void init_vpdata_view (gint num_active_xmaps, gint num_active_sources){
#if 0
                gtk_window_set_child (GTK_WINDOW (window), NULL); // remove and destroy ?? grid
                
                v_grid = gtk_grid_new ();
                gtk_window_set_child (GTK_WINDOW (window), v_grid);
                g_object_set_data (G_OBJECT (window), "v_grid", v_grid);
#endif
        };

        void vpdata_view_destroy (){
#if 0
                if (!vpdata_gr_matrix_view && vpdata_graph_app_window){
                        //gtk_window_destroy (GTK_WINDOW (vpdata_graph_window));
                        //vpdata_graph_window = NULL;
                }
#endif
        };
        

        void vpdata_add_pc (ProfileControl* &pc, const gchar *title, gint numx,
                            const gchar *xlab, UnitObj *UXaxis,
                            const gchar *ylab, UnitObj *UYaxis,
                            double xmin, double xmax,
                            int si, int nas, gboolean join_same_x,
                            gint xmap, gint src, gint num_active_xmaps, gint num_active_sources){
                if (!pc){
                        gchar *resid = g_strdelimit (g_strconcat (xlab,ylab,NULL), " ;:()[],./?!@#$%^&*()+-=<>", '_');

                        g_message( "vpdata_add_pc() new ProfileControl %s", resid);

                        pc = new ProfileControl (gxsm4app,
                                                 title,
                                                 numx, 
                                                 UXaxis, UYaxis, 
                                                 xmin, xmax,
                                                 resid,
                                                 vpdata_gr_matrix_view ? app_window : NULL);

                        SetTitle ("VP Data Viewer", title);
                        
                        if (vpdata_gr_matrix_view){
                                pc->set_pc_matrix_size (num_active_xmaps, num_active_sources);
                                gtk_grid_attach (GTK_GRID (v_grid), NULL, xmap,src, 1,1);
                                gtk_grid_attach (GTK_GRID (v_grid), pc->get_pc_grid (), xmap,src, 1,1);
                        }
                
                        g_free (resid);
                        pc->scan1d->mem2d->Resize (numx, join_same_x ? nas:1);
                        pc->SetGraphTitle (title);
                        pc->set_ys_label (0, ylab);
                        pc->SetData_dz (1.); // force dz=1.
                } else {	
                        SetTitle ("VP Data Viewer", title);
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
        gboolean vpdata_gr_matrix_view;
};

#endif // APP_VPDATA_VIEW_H
