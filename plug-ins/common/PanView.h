/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 2003 Kristan Temme
 *
 * Authors: Kristan Temme <etptt@users.sf.net>
 *                                                                                
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
 *  * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <math.h>
#include "cairo_item.h"


#define WINDOW_SIZE   100
#define WINDOW_ORIGIN (WINDOW_SIZE/2)
#define TO_CANVAS_X(var) (WINDOW_ORIGIN + ((var)*(WINDOW_SIZE/max_x)))
#define TO_CANVAS_Y(var) (WINDOW_ORIGIN - ((var)*(WINDOW_SIZE/max_y)))
#define TO_CANVAS_NORMAL_X(var) (WINDOW_ORIGIN + ((var)*WINDOW_SIZE))
#define TO_CANVAS_NORMAL_Y(var) (WINDOW_ORIGIN - ((var)*WINDOW_SIZE))
#define TO_RAD(var) (M_PI * ((var) / 180))

#define DSP_STATUSIND_FB   0
#define DSP_STATUSIND_SCAN 1
#define DSP_STATUSIND_VP   2
#define DSP_STATUSIND_MOV  3
#define DSP_STATUSIND_PLL  4
#define DSP_STATUSIND_ZPADJ 5
#define DSP_STATUSIND_AAP   6
#define DSP_STATUSIND_CURRENT 9
#define DSP_STATUSIND_LOAD   10
#define DSP_STATUSIND_LOADP  11

class PanView : public AppBase{
public:
	const int WXS = 2*WINDOW_SIZE+17;
	const int WYS = 2*WINDOW_SIZE+17;


        PanView (Gxsm4app *app);
        virtual ~PanView();

        void AppWindowInit(const gchar *title, const gchar *sub_title=NULL);

        static gboolean canvas_draw_function (GtkDrawingArea *area,
                                              cairo_t        *cr,
                                              int             width,
                                              int             height,
                                              PanView *pv);
        void determine_ij_patch (gdouble x, gdouble y, int &i, int &j);
        static void motion_cb (GtkEventControllerMotion *motion, gdouble x, gdouble y, PanView *pv);
        static void motion_enter_cb (GtkEventControllerMotion *motion, gdouble x, gdouble y, PanView *pv);
        static void motion_leave_cb (GtkEventControllerMotion *motion, gdouble x, gdouble y, PanView *pv);
        static void released_cb (GtkGesture *gesture, int n_press, double x, double y, PanView *pv);
        
        void show() {
                gtk_widget_show (panwindow);
        };
        void run ();
        void refresh ();
        void update_expanded_scan_limits ();
        void tip_refresh ();

        void start_tip_monitor ();
        void stop_tip_monitor ();

        gint finish (gint flg = -99) { if (flg != -99) tip_flag = flg; return tip_flag; };

private:  
        guint      timer_id;
        gint       tip_flag;

        GtkWidget* panwindow;
        GtkWidget  *canvas;

        // remplaced with: cairo_item_rectangle / text / path
        cairo_item_text  *info;
        cairo_item_text  *infoXY0;
        cairo_item  *tip_marker;
        cairo_item  *tip_marker_zoom;
        cairo_item  *tip_marker_z;      // Z-scan pos indication
        cairo_item  *tip_marker_z0;     // Z0 pos indication
        cairo_item  *DSP_status_indicator[16]; // { FB, SCAN, PROBE, ... }
        cairo_item  *DSP_gpio_indicator[16]; // { IO bits }
        cairo_item  *pan_area;
        cairo_item  *pan_area_extends;  // extends offset + scan beyond "200V"/full gain -- possible from DSP point of view
        cairo_item  *pre_current_view;  // scan area with prescan
        cairo_item  *current_view;      // scan area without prescan
        cairo_item  *current_view_zoom; // scan area without prescan zoomed
#define N_PRESETS 15 // must be a odd number!!
        cairo_item  *pos_preset_box[N_PRESETS][N_PRESETS];
        gboolean    show_preset_grid;
        
        double get_lw (double lw=1.) { return lw*(max_x-min_x)/WXS; };
	 
        void transform(double *dest, double *src, double rot, 
                       double y_off = 0., 
                       double x_off = 0.);

        double point[4][2];       // edges of the scan area
        double pre_point[4][2]; // only x coordinate only important for pre_current_view
        double Zratio, Zxs, Zys;
	 	 
        double max_x, min_x, xsr, x0r;
        double max_y, min_y, ysr, y0r;
        double max_z, min_z, zsr, z0r;
        double corn_oo, corn_ii;
 
	double max_corn[4][2];
	double x_offset;
	double y_offset;
	 
};
