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

#ifndef CAIRO_ITEM_H
#define CAIRO_ITEM_H

#include <math.h>
#include "gtk/gtk.h"
#include "gxsm_monitor_vmemory_and_refcounts.h"

typedef struct { double x,y; } cairo_point;
#define BASIC_COLORS 20

extern float BasicColors[][4];
int cairo_basic_color_lookup (const gchar *color);

#define UNREF_DELETE_CAIRO_ITEM(ITEM, CANVAS) { if (ITEM){ ITEM->hide (); ITEM->queue_update (CANVAS); delete ITEM; ITEM=NULL; }}
#define UNREF_DELETE_CAIRO_ITEM_NO_UPDATE(ITEM) { if (ITEM){ delete ITEM; ITEM=NULL; }}

#define CAIRO_BASIC_COLOR(I) BasicColors[(I)<0?0: (I)>BASIC_COLORS? BASIC_COLORS-1 : I]

typedef enum {
        CAIRO_COLOR_RED_ID,
        CAIRO_COLOR_GREEN_ID,
        CAIRO_COLOR_CYAN_ID,
        CAIRO_COLOR_YELLOW_ID,
        CAIRO_COLOR_BLUE_ID,
        CAIRO_COLOR_MAGENTA_ID,
        CAIRO_COLOR_GREY1_ID,
        CAIRO_COLOR_ORANGE_ID,
        CAIRO_COLOR_BLACK_ID,
        CAIRO_COLOR_WHITE_ID,
        CAIRO_COLOR_FORESTGREEN_ID,
        CAIRO_COLOR_GRAY1_ID,
        CAIRO_COLOR_GRAY2_ID,
        CAIRO_COLOR_GRAY3_ID,
        CAIRO_COLOR_GRAY4_ID,
        CAIRO_COLOR_GRAY5_ID,
        CAIRO_COLOR_GRAY6_ID,
        CAIRO_COLOR_GRAY7_ID,
        CAIRO_COLOR_GRAY8_ID,
        CAIRO_COLOR_GRAY9_ID
} CAIRO_BASIC_COLOR_IDS;

#define CAIRO_COLOR_WHITE        BasicColors[CAIRO_COLOR_WHITE_ID]
#define CAIRO_COLOR_RED          BasicColors[CAIRO_COLOR_RED_ID]
#define CAIRO_COLOR_GREEN        BasicColors[CAIRO_COLOR_GREEN_ID]
#define CAIRO_COLOR_FORESTGREEN  BasicColors[CAIRO_COLOR_FORESTGREEN_ID]
#define CAIRO_COLOR_CYAN         BasicColors[CAIRO_COLOR_CYAN_ID]
#define CAIRO_COLOR_YELLOW       BasicColors[CAIRO_COLOR_YELLOW_ID]
#define CAIRO_COLOR_BLUE         BasicColors[CAIRO_COLOR_BLUE_ID]
#define CAIRO_COLOR_MAGENTA      BasicColors[CAIRO_COLOR_MAGENTA_ID]
#define CAIRO_COLOR_GREY1        BasicColors[CAIRO_COLOR_GREY1_ID]
#define CAIRO_COLOR_ORANGE       BasicColors[CAIRO_COLOR_ORANGE_ID]
#define CAIRO_COLOR_BLACK        BasicColors[CAIRO_COLOR_BLACK_ID]
#define CAIRO_COLOR_GRAY(N)      BasicColors[CAIRO_COLOR_BLACK_ID+N] // N=1..9 -- NO CHECK!

#define CAIRO_LINE_SOLID       0
#define CAIRO_LINE_DASHED      1
#define CAIRO_LINE_ON_OFF_DASH 2

#define CAIRO_ANCHOR_N      0
#define CAIRO_ANCHOR_NW     4
#define CAIRO_ANCHOR_W      8
#define CAIRO_ANCHOR_SW     12
#define CAIRO_ANCHOR_S      16
#define CAIRO_ANCHOR_SE     20
#define CAIRO_ANCHOR_E      24
#define CAIRO_ANCHOR_NE     28
#define CAIRO_ANCHOR_CENTER -1

#define CAIRO_JUSTIFY_CENTER  0
#define CAIRO_JUSTIFY_LEFT    1
#define CAIRO_JUSTIFY_RIGHT   2
#define CAIRO_JUSTIFY_TOP     3
#define CAIRO_JUSTIFY_BOTTOM  4

// #define __CIP_DEBUG

class cairo_item{
public:
        cairo_item () { 
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "constructor");
                GXSM_REF_OBJECT (GXSM_GRC_CAIRO_ITEM);
                n=0; xy = NULL;
                v0.x = v0.y = 0.;
                angle=0.;
                set_line_width (1.0); set_stroke_rgba (0); set_fill_rgba (0); set_line_style (0);
                bbox[0]=bbox[1]=0.; bbox[2]=bbox[3]=0.; 
                grabbed = false;
                item_id=NULL;
                show();  
        };
	virtual ~cairo_item () {
                hide();
                GXSM_UNREF_OBJECT (GXSM_GRC_CAIRO_ITEM);
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "destructor");
                if (item_id) g_free (item_id);
        };
	virtual void draw (cairo_t* cr, double alpha=1.0, gboolean tr=true) {};
        virtual void set_xy_fast (int i, double x, double y) { xy[i].x=x, xy[i].y=y; };
        virtual void set_xy_fast_clip_y (int i, double x, double y, double ymax=1000.) { if (fabs(y) > ymax) y=ymax;  xy[i].x=x, xy[i].y=y; };
        virtual void set_xy_test (int i, double x, double y) { 
                if (x == NAN || x == -NAN) x = 0.; // NAN catch
                if (y == NAN || y == -NAN) y = 0.; // NAN catch
                xy[i].x=x, xy[i].y=y; 
        };
        virtual void set_xy_hold_logmin (int i, double x, double y, double logmin) { 
                if (x == NAN || x == -NAN) x = 0.; // NAN catch
                if (y == NAN || y == -NAN) y = 0.; // NAN catch
                if (y < logmin) y = i > 1 ? xy[i-1].y : logmin;
                xy[i].x=x, xy[i].y=y; 
        };
        virtual void set_xy (int i, double x, double y) { if (i >= 0 && i < n) xy[i].x=x, xy[i].y=y; };
        virtual void set_xy (int i, int j) { if (i >= 0 && i < n && j >= 0 && j < n) xy[i].x= xy[j].x, xy[i].y= xy[j].y; };
        virtual double auto_range_y (double max) {
                double amax = 0.;
                for (int i=0; i<n; ++i)
                        if (amax < fabs(xy[i].y))
                                amax = fabs(xy[i].y);
                for (int i=0; i<n; ++i)
                        xy[i].y *= max/amax;
                return amax;
        };
        virtual void get_xy (int i, double &x, double &y) { if (i >= 0 && i < n) x=xy[i].x, y=xy[i].y; };
        void map_xy (void (*map_xy_func)(double &x, double &y)) {
                for (int i=0; i<n; ++i)
                        (*map_xy_func) (xy[i].x, xy[i].y);
        };
        void set_angle(double alpha) { angle = alpha; };
        void set_id(const gchar *id) { if (!item_id) item_id = g_strdup(id); }; // set only once for life time
        const gchar *id() { return item_id; };
        
	virtual void set_anchor (int anchor) {}; 
	virtual void set_text (const gchar *text) {};
	virtual void set_text (double x, double y, const gchar *text) {};
        virtual void update_bbox (gboolean add_lw=true) {
                if (n){
                        bbox[2]=bbox[0]=xy[0].x; 
                        bbox[3]=bbox[1]=xy[0].y;
                        for (int i=1; i<n; ++i){
                                if (bbox[0] > xy[i].x) bbox[0] = xy[i].x; 
                                if (bbox[2] < xy[i].x) bbox[2] = xy[i].x; 
                                if (bbox[1] > xy[i].y) bbox[1] = xy[i].y; 
                                if (bbox[3] < xy[i].y) bbox[3] = xy[i].y; 
                        }
                        if (add_lw){
                                bbox[0] -= lw;
                                bbox[2] += lw;
                                bbox[1] -= lw;
                                bbox[3] += lw;
                        }
                        bbox[0] += v0.x;
                        bbox[1] += v0.y;
                        bbox[2] += v0.x;
                        bbox[3] += v0.y;
                }
        };
        int get_n_nodes() { return n; };
        void get_bb_min (double &x, double &y) { x=bbox[0]; y=bbox[1]; };
        void get_bb_max (double &x, double &y) { x=bbox[2]; y=bbox[3]; };
        virtual gboolean check_grab_bbox (double x, double y) {
                // g_message ("check_grab_bbox x=%g %g %g, y=%g %g %g", x, bbox[0], bbox[2], y, bbox[1], bbox[3] );
                return (x >= bbox[0] && x <= bbox[2] && y >= bbox[1] && y <= bbox[3]) ? true : false;  // *VG?uiv
        }
        virtual gboolean check_grab_bbox_dxy (double x, double y, double dx, double dy) {
                return (x >= bbox[0]-dx && x <= bbox[2]+dx && y >= bbox[1]-dy && y <= bbox[3]+dy) ? true : false;
        }
	virtual double distance (double x, double y) { 
                double dx = x-v0.x;
                double dy = y-v0.y;
                return sqrt (dx*dx+dy*dy); 
        };

        void show () { show_flag = true; };
        void hide () { show_flag = false; };
        void grab () { grabbed = true; };
        void ungrab () { grabbed = false; };
        gboolean is_grabbed () { return grabbed; };

        void set_position (double x, double y) { v0.x=x; v0.y=y; };
        void set_line_width (double line_width) { lw = line_width; };
        void set_line_style (int line_style) { ls = line_style; };
        void set_stroke_rgba (gint basic_color_index, gfloat a=1.0){
                for (int i=0; i<3; ++i)
                        stroke_rgba[i]=BasicColors[basic_color_index % BASIC_COLORS][i];
                stroke_rgba[3]=a;
        };
        void set_stroke_rgba (float c[4]) {
                for (int i=0; i<4; ++i)
                        stroke_rgba[i]=c[i];
        };
        void set_stroke_rgba (GdkRGBA *rgba) {
                stroke_rgba[0]=rgba->red; stroke_rgba[1]=rgba->green; stroke_rgba[2]=rgba->blue; stroke_rgba[3]=rgba->alpha;
        };
        void set_stroke_rgba (const gchar *color) {
                GdkRGBA rgba;
                if (gdk_rgba_parse (&rgba, color))
                        set_stroke_rgba (&rgba); 
        };
        void set_stroke_rgba (double r, double g, double b, double a=0.) {
                stroke_rgba[0]=r; stroke_rgba[1]=g; stroke_rgba[2]=b; stroke_rgba[3]=a;
        };
        void set_fill_rgba (double r, double g, double b, double a=0.) {
                fill_rgba[0]=r; fill_rgba[1]=g; fill_rgba[2]=b; fill_rgba[3]=a;
        };
        void set_fill_rgba (float c[4]) {
                for (int i=0; i<4; ++i)
                        fill_rgba[i]=c[i];
        };
        void set_fill_rgba (GdkRGBA *rgba) {
                fill_rgba[0]=rgba->red; fill_rgba[1]=rgba->green; fill_rgba[2]=rgba->blue; fill_rgba[3]=rgba->alpha;
        };
        void set_fill_rgba (const gchar *color) {
                GdkRGBA rgba;
                if (gdk_rgba_parse (&rgba, color))
                        set_fill_rgba (&rgba); 
        };
        void set_fill_rgba (gint basic_color_index, gfloat a=1.0) {
                for (int i=0; i<3; ++i)
                        fill_rgba[i]=BasicColors[basic_color_index % BASIC_COLORS][i];
                fill_rgba[3]=a;
        };
        virtual void queue_update_bbox (GtkWidget* imgarea) {
                // obsoleted in gtk4
                //gtk_widget_queue_draw_area (imgarea, bbox[0], bbox[1], bbox[2], bbox[3]);
                update_bbox ();
                //gtk_widget_queue_draw_area (imgarea, bbox[0], bbox[1], bbox[2], bbox[3]);
        }
        virtual void queue_update (GtkWidget* imgarea) {
                update_bbox ();
                gtk_widget_queue_draw (imgarea);
        };
        
protected:
        int n;
        cairo_point v0;
        cairo_point *xy;
        double angle;
        double lw;
        int ls;
        double stroke_rgba[4];
        double fill_rgba[4];
        gboolean show_flag;
        double bbox[4];
        gboolean grabbed;
        gchar *item_id;
};

class cairo_item_path : public cairo_item{
public:
	cairo_item_path (int nodes) {
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "path new");
                xy = g_new (cairo_point, nodes);
                n=nodes;
                impulse_floor=0.; mark_radius=1.; linemode=0;
        };
	cairo_item_path (cairo_item_path *cip) {
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "path new");
                n=cip->get_n_nodes();
                xy = g_new (cairo_point, n);
                impulse_floor=0.; mark_radius=1.; linemode=0;
        };
	virtual ~cairo_item_path () {
                g_free (xy);
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "path delete");
        };
	
	void set_linemode (gint m) { linemode=m; };
	void set_impulse_floor (double y) { impulse_floor=y; };
	void set_mark_radius (double r) { mark_radius=r; };
        
        virtual void draw (cairo_t* cr, double alpha=1.0, gboolean tr=true) { // add qf???
                if (show_flag && n > 1){
                        cairo_save (cr);
                        cairo_translate (cr, v0.x, v0.y);
                        if (angle != 0.)
                                cairo_rotate (cr, angle);
                        cairo_set_source_rgba (cr, stroke_rgba[0], stroke_rgba[1], stroke_rgba[2], stroke_rgba[3]);
                        cairo_set_line_width (cr, lw); 
#ifdef __CIP_DEBUG
                        g_print ("cip::draw  M %+8.3f,%+8.3fg\n", xy[0].x, xy[0].y);
#endif
                        switch (linemode){
                        case 0: // connect solid
                                cairo_move_to (cr, xy[0].x, xy[0].y);
                                for (int i=1; i<n; ++i){
                                        cairo_line_to (cr, xy[i].x, xy[i].y);
#ifdef __CIP_DEBUG
                                        g_print ("cip::draw  L %+8.3f,%+8.3fg\n", xy[i].x, xy[i].y);
#endif
                                }
                                break;
                        case 1: // dots (h-bars, lw) 
                                for (int i=0; i<n; ++i){
                                        cairo_move_to (cr, xy[i].x, xy[i].y);
                                        cairo_line_to (cr, xy[i].x+lw, xy[i].y);
                                }
                                break;
                        case 2: // impulse 
                                for (int i=0; i<n; ++i){
                                        cairo_move_to (cr, xy[i].x, impulse_floor);
                                        cairo_line_to (cr, xy[i].x, xy[i].y);
                                }
                                break;
                        case 3: // X marks
                                for (int i=0; i<n; ++i){
                                        cairo_move_to (cr, xy[i].x-mark_radius, xy[i].y-mark_radius);
                                        cairo_line_to (cr, xy[i].x+mark_radius, xy[i].y+mark_radius);
                                        cairo_move_to (cr, xy[i].x+mark_radius, xy[i].y-mark_radius);
                                        cairo_line_to (cr, xy[i].x-mark_radius, xy[i].y+mark_radius);
                                }
                                break;
                        default:
                                g_warning ("cairo_item_path::draw called with unhandled linemode.");
                                break;
                        }
                        cairo_stroke (cr);
                        cairo_restore (cr);
                } else {
                        g_warning ("cairo_item_path::draw called with insufficent node number.");
                }
        };
        
        virtual void draw_partial (cairo_t* cr, int i0, int i1) {
                if (show_flag){
                        cairo_save (cr);
                        cairo_translate (cr, v0.x, v0.y);
                        if (angle != 0.)
                                cairo_rotate (cr, angle);
                        cairo_set_source_rgba (cr, stroke_rgba[0], stroke_rgba[1], stroke_rgba[2], stroke_rgba[3]);
                        cairo_set_line_width (cr, lw); 
                        switch (linemode){
                        case 0: // connect solid
                                cairo_move_to (cr, xy[i0].x, xy[i0].y);
                                for (int i=i0+1; i<i1; ++i)
                                        cairo_line_to (cr, xy[i].x, xy[i].y);
                                break;
                        case 1: // dots (h-bars, lw) 
                                for (int i=i0; i<i1; ++i){
                                        cairo_move_to (cr, xy[i].x, xy[i].y);
                                        cairo_line_to (cr, xy[i].x+lw, xy[i].y);
                                }
                                break;
                        case 2: // impulse 
                                for (int i=i0; i<i1; ++i){
                                        cairo_move_to (cr, xy[i].x, impulse_floor);
                                        cairo_line_to (cr, xy[i].x, xy[i].y);
                                }
                                break;
                        case 3: // X marks
                                for (int i=i0; i<i1; ++i){
                                        cairo_move_to (cr, xy[i].x-mark_radius, xy[i].y-mark_radius);
                                        cairo_line_to (cr, xy[i].x+mark_radius, xy[i].y+mark_radius);
                                        cairo_move_to (cr, xy[i].x+mark_radius, xy[i].y-mark_radius);
                                        cairo_line_to (cr, xy[i].x-mark_radius, xy[i].y+mark_radius);
                                }
                                break;
                        default:
                                g_warning ("cairo_item_path::draw called with unhandled linemode.");
                                break;
                        }
                        cairo_stroke (cr);
                        cairo_restore (cr);
                }
        };

private:
        gint linemode;
        double impulse_floor;
        double mark_radius;
};

class cairo_item_segments : public cairo_item{
public:
	cairo_item_segments (int nodes) {
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "segments new");
                xy = g_new (cairo_point, nodes); n=nodes;
        };
	cairo_item_segments (cairo_item_path *cip) {
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "segments new");
                xy = g_new (cairo_point, n=cip->get_n_nodes());
        };
	virtual ~cairo_item_segments () {
                g_free (xy);
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "segments delete");
        };
	
        virtual void draw (cairo_t* cr, double alpha=1.0, gboolean tr=true) { // add qf???
                if (show_flag){
                        cairo_save (cr);
                        cairo_translate (cr, v0.x, v0.y);
                        if (angle != 0.)
                                cairo_rotate (cr, angle);
                        cairo_set_source_rgba (cr, stroke_rgba[0], stroke_rgba[1], stroke_rgba[2], stroke_rgba[3]);
                        cairo_set_line_width (cr, lw); 
                        for (int i=0; i<n; ){
                                cairo_move_to (cr, xy[i].x, xy[i].y); ++i;
                                cairo_line_to (cr, xy[i].x, xy[i].y); ++i;
                                cairo_stroke (cr);
                        }
                        cairo_restore (cr);
                }
        };
        virtual void draw_partial (cairo_t* cr, int i0, int i1) {
                if (show_flag){
                        cairo_save (cr);
                        cairo_translate (cr, v0.x, v0.y);
                        cairo_set_source_rgba (cr, stroke_rgba[0], stroke_rgba[1], stroke_rgba[2], stroke_rgba[3]);
                        cairo_set_line_width (cr, lw); 
                        for (int i=i0; i<i1; ){
                                cairo_move_to (cr, xy[i].x, xy[i].y); ++i;
                                cairo_line_to (cr, xy[i].x, xy[i].y); ++i;
                                cairo_stroke (cr);
                        }
                        cairo_restore (cr);
                }
        };

private:
};

class cairo_item_segments_wlw : public cairo_item{
public:
	cairo_item_segments_wlw (int nodes) {
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "segments new");
                xy = g_new (cairo_point, nodes); n=nodes;
                wlw = g_new0 (double, n);
                rgba = g_new0 (double, n<<2);
        };
	cairo_item_segments_wlw (cairo_item_path *cip) {
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "segments new");
                xy = g_new (cairo_point, n=cip->get_n_nodes());
                wlw = g_new0 (double, n);
                rgba = g_new0 (double, n<<2);
        };
	virtual ~cairo_item_segments_wlw () {
                g_free (xy);
                g_free (wlw);
                g_free (rgba);
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "segments delete");
        };

        void set_segment_line_width (int i, double line_width) {
                if (i >= 0 && i < n) wlw[i] = line_width;
        };
        void set_segment_rgba (int i, double r, double g, double b, double a) {
                if (i >= 0 && i < n){
                        i <<= 2;
                        rgba[i++] = r;
                        rgba[i++] = g;
                        rgba[i++] = b;
                        rgba[i] = a;
                }
        };
        
        virtual void draw (cairo_t* cr, double alpha=1.0, gboolean tr=true) { // add qf???
                gboolean flag=false;
                if (show_flag){
                        cairo_save (cr);
                        cairo_translate (cr, v0.x, v0.y);
                        if (angle != 0.)
                                cairo_rotate (cr, angle);
                        cairo_set_source_rgba (cr, stroke_rgba[0], stroke_rgba[1], stroke_rgba[2], stroke_rgba[3]);
                        cairo_set_line_width (cr, lw);
                        cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
                        for (int i=0; i<n; ){
                                if (wlw[i] > 0.)
                                        cairo_set_line_width (cr, wlw[i]); 
                                if (rgba[(i<<2)+3] > 0.0){
                                        cairo_set_source_rgba (cr, rgba[i<<2], stroke_rgba[(i<<2)+1], stroke_rgba[(i<<2)+2], stroke_rgba[(i<<2)+3]); flag=true;
                                }else if (flag){
                                        cairo_set_source_rgba (cr, stroke_rgba[0], stroke_rgba[1], stroke_rgba[2], stroke_rgba[3]); flag=false;
                                }
                                cairo_move_to (cr, xy[i].x, xy[i].y); ++i;
                                cairo_line_to (cr, xy[i].x, xy[i].y); ++i;
                                cairo_stroke (cr);
                        }
                        cairo_restore (cr);
                }
        };
        virtual void draw_partial (cairo_t* cr, int i0, int i1) {
                if (show_flag){
                        cairo_save (cr);
                        cairo_translate (cr, v0.x, v0.y);
                        if (angle != 0.)
                                cairo_rotate (cr, angle);
                        cairo_set_source_rgba (cr, stroke_rgba[0], stroke_rgba[1], stroke_rgba[2], stroke_rgba[3]);
                        cairo_set_line_width (cr, lw); 
                        for (int i=i0; i<i1; ){
                                cairo_move_to (cr, xy[i].x, xy[i].y); ++i;
                                cairo_line_to (cr, xy[i].x, xy[i].y); ++i;
                                cairo_stroke (cr);
                        }
                        cairo_restore (cr);
                }
        };

private:
        double *wlw;
        double *rgba;
};

class cairo_item_path_closed : public cairo_item{
public:
	cairo_item_path_closed (int nodes) {
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "path closed new");
                xy = g_new (cairo_point, nodes); n=nodes;
        };
	virtual ~cairo_item_path_closed () {
                g_free (xy);
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "path closed delete");
        };
	
        virtual void draw (cairo_t* cr, double alpha=1.0, gboolean tr=true) { // add qf???
                if (show_flag && n>1){
                        cairo_save (cr);
                        if (tr)
                                cairo_translate (cr, v0.x, v0.y);
                        if (angle != 0.)
                                cairo_rotate (cr, angle);
                        cairo_set_source_rgba (cr, stroke_rgba[0], stroke_rgba[1], stroke_rgba[2], alpha*stroke_rgba[3]);
                        cairo_set_line_width (cr, lw); 
                        cairo_move_to (cr, xy[0].x, xy[0].y);
                        for (int i=1; i<n; ++i){
                                cairo_line_to (cr, xy[i].x, xy[i].y);
                        }
                        cairo_close_path (cr);
                        // cairo_set_source_rgba (cr, fill_rgba[0], fill_rgba[1], fill_rgba[2], alpha*fill_rgba[3]);
                        cairo_fill (cr);
                        cairo_restore (cr);
                } else {
                        g_warning ("cairo_item_path_closed::draw called with insufficent node number.");
                }
        };

private:
};

class cairo_item_rectangle : public cairo_item{
public:
	cairo_item_rectangle () {
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "rectangle new");
                xy = g_new (cairo_point, 2); n=2;
        };
	cairo_item_rectangle (double x1, double y1, double x2, double y2) {
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "rectangle new");
                xy = g_new (cairo_point, 2); n=2; xy[0].x=x1, xy[0].y=y1; xy[1].x=x2, xy[1].y=y2;
        };
	virtual ~cairo_item_rectangle () {
                g_free (xy);
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "rectangle delete");
        };
	
        virtual void draw (cairo_t* cr, double alpha=1.0, gboolean tr=true) { // add qf???
                if (show_flag){
#ifdef __CIP_DEBUG
                        g_print ("cip::draw  R %+8.3f,%+8.3fg : ", xy[0].x, xy[0].y);
                        g_print (" %+8.3f,%+8.3fg\n", xy[1].x, xy[1].y);
#endif
                        cairo_save (cr);
                        if (tr)
                                cairo_translate (cr, v0.x, v0.y);
                        if (angle != 0.)
                                cairo_rotate (cr, angle);
                        cairo_set_line_width (cr, lw); 
                        cairo_set_source_rgba (cr, fill_rgba[0], fill_rgba[1], fill_rgba[2], alpha*fill_rgba[3]);
                        cairo_rectangle (cr,  xy[0].x, xy[0].y,   xy[1].x-xy[0].x, xy[1].y-xy[0].y);
                        cairo_fill (cr);
                        cairo_set_source_rgba (cr, stroke_rgba[0], stroke_rgba[1], stroke_rgba[2], alpha*stroke_rgba[3]);
                        cairo_rectangle (cr,  xy[0].x, xy[0].y,   xy[1].x-xy[0].x, xy[1].y-xy[0].y);
                        cairo_stroke (cr);
                        cairo_restore (cr);
                }
        };

private:
};

class cairo_item_circle : public cairo_item{
public:
	cairo_item_circle (int n_circels=1) {
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "circle new");
                xy = g_new (cairo_point, n_circels); n=n_circels; radius=1.;
        };
	cairo_item_circle (double x, double y, double r) {
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "circle new");
                xy = g_new (cairo_point, 1); n=1; xy[0].x=x, xy[0].y=y; radius=r;
        };
	virtual ~cairo_item_circle () {
                g_free (xy);
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "circle delete");
        };
        void set_radius (double r) { radius = r; };
        virtual void update_bbox (gboolean add_lw=true) {
                bbox[0] = xy[0].x-radius-lw;
                bbox[1] = xy[0].y-radius-lw;
                bbox[2] = xy[0].x+radius+lw;
                bbox[3] = xy[0].y+radius+lw;
        };

        virtual void draw (cairo_t* cr, double alpha=1.0, gboolean tr=true) { // add qf???
                if (show_flag){
                        cairo_save (cr);
                        cairo_translate (cr, v0.x, v0.y);
                        cairo_set_line_width (cr, lw);
                        for (int i=0; i<n; ++i){
                                cairo_arc (cr,  xy[i].x, xy[i].y,  radius, 0., 2.*M_PI);
                                cairo_set_source_rgba (cr, fill_rgba[0], fill_rgba[1], fill_rgba[2], fill_rgba[3]);
                                cairo_fill (cr);
                                cairo_set_source_rgba (cr, stroke_rgba[0], stroke_rgba[1], stroke_rgba[2], stroke_rgba[3]);
                                cairo_arc (cr,  xy[i].x, xy[i].y,  radius, 0., 2.*M_PI);
                                cairo_stroke (cr);
                        }
                        cairo_restore (cr);
                }
        };

private:
        double radius;
};

class cairo_item_arc : public cairo_item{
public:
	cairo_item_arc (double x, double y, double r_i, double d_r, double phi_i, double d_phi, double angle_scale=1.0, int n_=1, double nd_phi_=0.) {
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "circle new");
                xy = g_new (cairo_point, 1); n=1; xy[0].x=x, xy[0].y=y;
                arc_scale = angle_scale;
                ri=r_i; dr=d_r; phii=arc_scale*phi_i; dphi=arc_scale*d_phi; num=n_; nd_phi = arc_scale*nd_phi_;
        };
	virtual ~cairo_item_arc () {
                g_free (xy);
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "circle delete");
        };
        void set_radius (double r_i, double d_r) { ri=r_i;  dr=d_r; };
        void set_arc (double phi_i, double d_phi, int n_=1, double nd_phi_=0.) { phii=arc_scale*phi_i; dphi=arc_scale*d_phi; num=n_; nd_phi = arc_scale*nd_phi_; };
        void set_angle_scale (double f) { arc_scale =f; }; // set custom scale to radians
        virtual void update_bbox (gboolean add_lw=true) {
                bbox[0] = xy[0].x-ri-dr-lw;
                bbox[1] = xy[0].y-ri-dr-lw;
                bbox[2] = xy[0].x+ri+dr+lw;
                bbox[3] = xy[0].y+ri+dr+lw;
        };

        virtual void draw (cairo_t* cr, double alpha=1.0, gboolean tr=true) { // add qf???
                if (show_flag){
                        cairo_save (cr);
                        cairo_translate (cr, v0.x, v0.y);
                        if (angle != 0.)
                                cairo_rotate (cr, angle);
                        cairo_set_line_width (cr, dr);
                        cairo_set_source_rgba (cr, stroke_rgba[0], stroke_rgba[1], stroke_rgba[2], stroke_rgba[3]);
                        double phi=phii;
                        for(int i=0; i<num; ++i, phi+=nd_phi){
                                if (dphi > 0)
                                        cairo_arc (cr,  xy[i].x, xy[i].y,  ri+0.5*dr, phi, phi+dphi);
                                else
                                        cairo_arc_negative (cr,  xy[i].x, xy[i].y,  ri+0.5*dr, phi, phii+dphi);
                                cairo_stroke (cr);
                        }
                        
                        cairo_restore (cr);
                }
        };

private:
        double arc_scale;
        double ri, dr, phii, dphi;
        int num;
        double nd_phi;
};


// font_faces: "Georgia" "Ununtu"
class cairo_item_text : public cairo_item{
public:
	cairo_item_text () { \
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "text new");
                xy = g_new (cairo_point, 1); n=1;
                t=NULL; 
                xy[0].x=0., xy[0].y=0.; 
                pango_font = NULL;
                font_face = NULL;
                t_anchor  = CAIRO_ANCHOR_N;
                t_justify = CAIRO_JUSTIFY_CENTER;
                spacing = 1.1; 
                set_font_face_size ("Ununtu", 16.);
        };
	cairo_item_text (double x, double y, const gchar *text) { 
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "text new");
                xy = g_new (cairo_point, 1); n=1; 
                xy[0].x=0., xy[0].y=0.; 
                v0.x=x, v0.y=y; 
                t=g_strdup (text);
                pango_font = NULL;
                font_face = NULL;
                t_anchor  = CAIRO_ANCHOR_N;
                t_justify = CAIRO_JUSTIFY_CENTER;
                spacing = 1.1;
                set_font_face_size ("Ununtu", 16.);
        };
	virtual ~cairo_item_text () {
                g_free (xy); if (t) g_free (t); if (font_face) g_free (font_face); if (pango_font) pango_font_description_free (pango_font);
                // GXSM_LOG_DATAOBJ_ACTION (GXSM_GRC_CAIRO_ITEM, "text delete");
        };
	virtual void set_text (const gchar *text) { if (t) g_free (t); t=g_strdup (text); };
	virtual void set_text (double x, double y, const gchar *text) {  
                v0.x=x, v0.y=y;
                if (t)
                        g_free (t);
                t=g_strdup (text); 
        };
        virtual double set_font_face_size_from_string (const gchar *face_size, double scale=1.);
	virtual void set_font_face_size (const gchar *face, double size, cairo_font_slant_t slant = CAIRO_FONT_SLANT_NORMAL, cairo_font_weight_t weight = CAIRO_FONT_WEIGHT_NORMAL) { 
                if (font_face)
                        g_free (font_face);
                font_face = g_strdup (face); 
                font_size = size; font_slant = slant; font_weight = weight;
        };
        virtual void set_pango_font (const gchar *pango_font_name) { 
                if (pango_font) pango_font_description_free (pango_font);
                pango_font = pango_font_description_from_string (pango_font_name); 
        };
	virtual void set_anchor (int anchor) { t_anchor = anchor; }; 
	virtual void set_justify (int justify) { t_justify = justify; }; 
	virtual void set_spacing (double line_spacing) { spacing = line_spacing > 0. ? 1.+line_spacing : -1.-line_spacing; }; 
        virtual void update_bbox (gboolean add_lw=true) {
                // needs pango, cr, etc, updated on draw
        };
        virtual void queue_update (GtkWidget* imgarea) {
                        gtk_widget_queue_draw (imgarea);
        };
        virtual void queue_update_bbox (GtkWidget* imgarea) {
                //if (bbox[0] == 0 && bbox[2] == 0.)
                gtk_widget_queue_draw (imgarea);
                //else
                //gtk_widget_queue_draw_area (imgarea, bbox[0], bbox[1], bbox[2], bbox[3]);
        };

        virtual void draw (cairo_t* cr, double alpha=0.0, gboolean tr=true);
        void print () { g_message ("cairo_item_text: >%s< %s %g is %s", t, font_face, font_size, show_flag?"On":"Off"); };
        const gchar* get_text (gboolean sh=false) { return (show_flag||sh) ? t : "---"; };
private:
        gchar *t;
        gchar *font_face;
        double font_size;
        cairo_font_slant_t font_slant;
        cairo_font_weight_t font_weight;
        PangoFontDescription *pango_font;
        double spacing;
        gint    t_anchor;
        gint    t_justify;
};



#endif
