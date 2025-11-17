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

#ifndef __APP_PROFILE_H
#define __APP_PROFILE_H

#include <iostream>
#include <fstream>
#include "cairo_item.h"
#include "lineprofile.h"
#include "gapp_service.h"
#include "app_vobj.h"
#include "scan.h"

typedef enum { EV_BUTTON_NONE=0, EV_BUTTON_1=1, EV_BUTTON_2=2, EV_BUTTON_3=3, EV_BUTTON_WHEEL_UP=4, EV_BUTTON_WHEEL_DOWN=5 } EV_BUTTON;
typedef enum { EV_NONE, EV_BUTTON_PRESS, EV_MOTION_NOTIFY, EV_BUTTON_RELEASE, EV_ENTER_NOTIFY, EV_LEAVE_NOTIFY } EV_TYPE;
typedef struct { EV_BUTTON button; EV_TYPE type; gdouble x,y; } DA_Event;


typedef struct{
	const gchar *mitem;
	int   msk;
	int   neg;
} menuoptionlist;

#define PROFILE_MODE_XGRID    (1<<0)
#define PROFILE_MODE_XLOG     (1<<1)
#define PROFILE_MODE_YGRID    (1<<2)
#define PROFILE_MODE_YLOG     (1<<3)
#define PROFILE_MODE_YLINREG  (1<<4)
#define PROFILE_MODE_POINTS   (1<<5)
#define PROFILE_MODE_CONNECT  (1<<6)
#define PROFILE_MODE_IMPULS   (1<<7)
#define PROFILE_MODE_NOTICS   (1<<8)
#define PROFILE_MODE_LEGEND   (1<<9)
#define PROFILE_MODE_SYMBOLS  (1<<10)
#define PROFILE_MODE_YPSD     (1<<11)
#define PROFILE_MODE_YDIFF    (1<<12)
#define PROFILE_MODE_BINARY8  (1<<13)
#define PROFILE_MODE_BINARY16 (1<<14)
#define PROFILE_MODE_YLOWPASS (1<<15)
#define PROFILE_MODE_HEADER   (1<<16)
#define PROFILE_MODE_STICS    (1<<17)
#define PROFILE_MODE_DECIMATE (1<<18)
#define PROFILE_MODE_XR_AB    (1<<19)
#define PROFILE_MODE_XL1000   (1<<20)
#define PROFILE_MODE_XL4000   (1<<21)

#define PROFILE_SCALE_XAUTO   (1<<0)
#define PROFILE_SCALE_YAUTO   (1<<1)
#define PROFILE_SCALE_XHOLD   (1<<3)
#define PROFILE_SCALE_YHOLD   (1<<4)
#define PROFILE_SCALE_XEXPAND (1<<5)
#define PROFILE_SCALE_YEXPAND (1<<6)


#define UTF8_DEGREE "\302\260"

#define NPI 16


class ProfileControl;

class ProfileElement{
#define PROFILE_ELEM_HIDE     (1<<0)
 public:
	ProfileElement(Scan *sc, ProfileControl *pc, const gchar *color=NULL, int Yy=0);
	ProfileElement(ProfileElement *pe, const gchar *color=NULL, int Yy=0);
	~ProfileElement();

	void unref_delete  (GtkWidget *canvas){
	        for (int i=0; i<NPI; ++i)
		        UNREF_DELETE_CAIRO_ITEM (pathitem[i], canvas);
	};

        void calc_decimation (gint64 ymode);
        void get_decimation (gint &dl, gint &nd) { dl=dec_len; nd=n_dec; };
	inline int GetNy();
	inline void SetY(int Yy=0);
	inline void SetLastY();
	inline void SetMode(long Mode);
        gint64 GetMode() { return mode; };
	void SetOptions(long Flg);
  
	double calc (gint64 ymode, int id, int binary_mask, double y_offset, GtkWidget *canvas);
	void   draw (cairo_t* cr);
        void   stream_set (std::ofstream &ofs, int id=0);
	void   update (GtkWidget *canvas, int id=0, int style=CAIRO_LINE_SOLID);

	inline void GetCurXYc(double *x, double *y, int i, int id=0);
	inline int GetCurX(double *x, double *y, int id=0);

	gchar *GetInfo(int i, gint64 ymode=0);
        //inline double GetData_dz ();
        double SetData_dz (double dz);
	inline double GetValue(int i);
	inline double GetXPos(int i);

	int nextMax(int i, int dir, double eps=1.);
	int nextMin(int i, int dir, double eps=1.);

	gchar *GetDeltaInfo(int i, int j, gint64 ymode=0);
	Scan *get_scan () { return scan; };
	const gchar *get_color () { return color; };

	double ylocmin, ylocmax;
	double xlocmin, xlocmax;
 private:
	gint n;
        gint dec_len, n_dec; // decimate for large n
	gint yy;

	ProfileControl    *pctrl;
	cairo_item_path   *pathitem[NPI];
	cairo_item_path   *pathitem_draw[NPI];
        gboolean          pathcoords_world;

	Scan *scan;
	Scan *psd_scan;
	gchar *color;
	gint64 flg;
	gint64 mode;
};

class ProfileControl : public AppBase, public LineProfile1D{

 public:
	ProfileControl (Gxsm4app *app, const gchar *titlestring=NULL, int ChNo=-1);
	ProfileControl (Gxsm4app *app, const gchar *titlestring, int n, UnitObj *ux, UnitObj *uy, double xmin=0., double xmax=1.,
                        const gchar *resid=NULL, Gxsm4appWindow *in_external_window=NULL);
	ProfileControl (Gxsm4app *app, const gchar *filename, const gchar *resource_id_string);
	~ProfileControl ();
        virtual void AppWindowInit (const gchar *title=NULL, const gchar *sub_title=NULL);

        GtkWidget *widget_cb, *widget_xr_ab;
        GtkWidget *get_pc_grid () { return pc_grid; };
        void set_pc_matrix_size (gint ncolumns=1, gint nrows=1) { pc_ncolumns=ncolumns; pc_nrows=nrows; };
        gboolean is_external_window_set () { return pc_in_window != NULL ; };
        
	void Init(const gchar *titlestring, int ChNo, const gchar *resid=NULL);

	static void file_open_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void file_open_callback_exec (GtkDialog *dialog,  int response);
	static void file_save_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void file_save_callback_exec (GtkDialog *dialog,  int response, gpointer user_data);
	static void file_save_as_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void file_save_as_callback_exec (GtkDialog *dialog,  int response, gpointer user_data);
	static void file_save_image_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void file_save_image_callback_exec (GtkDialog *dialog,  int response, gpointer user_data);
	static void file_save_data_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void file_save_data_callback_exec (GtkDialog *dialog,  int response, gpointer user_data);
	static void file_print1_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void file_print2_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void file_print3_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void file_print4_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	// experimental 5th & 6th print for xmgrace
	static void file_print5_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void file_print6_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void file_activate_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void file_close_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

        void settings_adjust_mode_callback (GSimpleAction *action, GVariant *parameter, gint64 flg);

	static void logy_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void last1000_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void last4000_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void linreg_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void psd_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void ydiff_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void ylowpass_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void ylowpass_cycle_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void decimate_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void settings_xgrid_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void settings_ygrid_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void settings_notics_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void settings_pathmode_radio_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void settings_header_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void settings_legend_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void settings_series_tics_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

	static void zoom_in_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void zoom_out_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void aspect_inc_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void aspect_dec_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void canvas_size_store_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

	static void yhold_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void yexpand_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void skl_Yauto_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void skl_Yupperup_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void skl_Yupperdn_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void skl_Ylowerup_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void skl_Ylowerdn_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void skl_Yzoomin_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void skl_Yzoomout_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void skl_Yset_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void skl_Xauto_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void skl_Xset_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void opt_xr_ab_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void skl_Binary_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
       
	static void cur_Ashow_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void cur_Bshow_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void cur_Aleft_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void cur_Almax_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void cur_Almin_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void cur_Armax_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void cur_Armin_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void cur_Aright_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void cur_Bleft_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void cur_Blmax_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void cur_Blmin_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void cur_Brmax_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void cur_Brmin_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	static void cur_Bright_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

	// canvas draw
        gboolean resize_drawing (double w, double h);
        gulong resize_cb_handler_id;
       
        static gboolean cairo_draw_profile_only_callback (cairo_t *cr, ProfileControl *pc);
	static void canvas_draw_function (GtkDrawingArea *area,
                                          cairo_t        *cr,
                                          int             width,
                                          int             height,
                                          ProfileControl *pc);

        static gint cursor_event (cairo_item *items[2], DA_Event *event, double mxy[2], ProfileControl *pc);
        static void pressed_cb (GtkGesture *gesture, int n_press, double x, double y, ProfileControl *pc);
        static void released_cb (GtkGesture *gesture, int n_press, double x, double y, ProfileControl *pc);
        static void drag_motion (GtkEventControllerMotion *motion, gdouble x, gdouble y, ProfileControl *pc);
        
	void file_print_callback (int index, ProfileControl *pc);
	void save_data (const gchar *fname);

	void showCur(int id, int show);
	void moveCur(int id, int dir=0, int search=0, double ix=0.);

	void UpdateArea();

	void auto_update(int rate=250); // auto update, rate in ms. Disable: rate=0
	void mark_for_update(int f=TRUE) {
		if (f == TRUE && new_data_flag == 111) return;
		new_data_flag=f;
	};
	int update_needed () { return new_data_flag; }
	int get_auto_update_id () { return auto_update_id; }

	void UpdateCursorInfo (double x1, double x2, double z1, double z2);

	static void destroy_item(cairo_item *gci, ProfileControl *pc){ g_object_unref (G_OBJECT (gci)); };
	static void kill_elem(ProfileElement *pe, ProfileControl *pc){ delete pe; };
	static void update_elem(ProfileElement *pe, ProfileControl *pc);

	gint NewData_redprofile(Scan* sc, int redblue='r');
	gint NewData(Scan* sc, int line=-1, int cpyscan=TRUE, VObject *vo=NULL, gboolean append=FALSE);
	gint NewData(Scan* sc, VObject *vo, gboolean append=FALSE) { return NewData (sc, -1, TRUE, vo, append); };
	void AddScan(Scan* scan, int line=0, gchar *col=NULL);
	void AddLine(int line=0, gchar *col=NULL);
	void RemoveScans();

	void SetActive(int flg);

        //double GetData_dz () { return last_pe->GetData_dz (); };
        double SetData_dz (double dz) { return last_pe->SetData_dz (dz); };

	void SetYrange(double y1, double y2);
	void SetXrange(double x1, double x2);

	void SetXlabel(const gchar *xlab = NULL);
	void SetYlabel(const gchar *ylab = NULL);
	void SetGraphTitle(const gchar *tit, gboolean append=FALSE);

	void SetMode(gint64 m) { mode=m; };
        gint64 GetMode() { return mode; };
	void SetScaling(gint64 sm) { scaleing = sm; };

	void scan2canvas(double sx, double sy, double &cx, double &cy){
		cx = (sx-xmin)*cxwidth/xrange;
		XSM_DEBUG (DBG_L2, "s2cxy"<< (mode& PROFILE_MODE_YLOG));
		if (mode & PROFILE_MODE_YLOG){
			if (fabs (sy)>0.)
				cy = cywidth*(1.-(log (sy)-lmin)/lmaxmin);
			else
				cy = 1.;
		}
		else
			cy = cywidth*(1.-(sy-ymin)/yrange);
	};

	void canvas2scan(double cx, double cy, double &sx, double &sy){
		sx = cx*xrange/cxwidth+xmin;
		if (mode & PROFILE_MODE_YLOG)
                        sy = exp ((1.-cy/cywidth)*lmaxmin+lmin);
		else
			sy = (1.-cy/cywidth)*yrange+ymin;
	};
        
	void scan2canvas(double &scx, double &scy){
		scx = (scx-xmin)*cxwidth/xrange;
		if(mode & PROFILE_MODE_YLOG){
			if (fabs(scy) > 0.)
				scy = cywidth*(1.-(log(scy)-lmin)/lmaxmin);
			else
				scy = 1.;
		}
		else
			scy = cywidth*(1.-(scy-ymin)/yrange);
	};

	double scan2canvasX(double sx){
		return (sx-xmin)*cxwidth/xrange;
	};

	double scan2canvasY(double sy){
		if (mode & PROFILE_MODE_YLOG){
			if (fabs(sy)>0.)
				return cywidth*(1.-(log (sy)-lmin)/lmaxmin);
			else
				return 1.;
		}
		else
			return cywidth*(1.-(sy-ymin)/yrange);
	};

	double getCursorPos(int id);
	void register_cursor_update_func(VObject *vo) { cursor_bound_object = vo; };

	const gchar *get_xcolor (int i);
	const gchar *get_ys_label (int i);
	void  *set_ys_label (int i, const gchar *label);

	void ref (){ ++ref_count; };
	void unref (){ --ref_count; };

	int get_scount () { return scount; };

        double get_lw1 (double x=1.) { return lw_1 * x; };
        double get_drawing_width() { return papixel*aspect; };
        
        double lp_gain;
        
	int CursorsIdx[2];

private:

	void SetSize (double new_aspect=0.);
	void drawScans (cairo_t* cr);
	static void draw_elem (ProfileElement *pe, cairo_t* cr);

	void updateScans (int flg=0);
	void updateFrame ();
	gint updateTics  (gboolean force=FALSE);
	void addTic    (cairo_item **tic, cairo_item_text **lab,
			double val, double len, int pos, const gchar *fmt=NULL, double tval=0., const gchar *color=NULL);

	// tmp cr ref for for-each actions
	cairo_t* current_draw_cr;

	int    new_data_flag;
	int    chno;
	int    SklOnly;

        gint   pc_nrows;
        gint   pc_ncolumns;
        double window_w, window_h;
	int    statusheight;
	int    bbarwidth;
        double font_size_10;
	double border;  // [=0.1] in canvas units (0..1 is drawing area range)
	double pasize;  // current actual size
	double cxwidth, cywidth; // => y=pasize, x=pasize*aspect
	double aspect;  // current drawingarea aspect ratio
        double left_border, top_border; // in broder units
	int    papixel; // actual physical screen pixels (y)
        double pixel_size; // relation to actual physical screen pixel size
        double lw_1;    // line width corresponding to "1 pixel"

	double xmin, xmax, xrange;
	double ymin, ymax, yrange;
	double lmaxmin, lmin;

	VObject *cursor_bound_object;

	gint64 mode, scaleing;

	gchar *xlabel, *xlabel0;
	gchar *ylabel, *ylabel0;
	gchar *title;

	gchar *profile_res_id;

	double tic_x1, tic_x2, tic_y1, tic_y2;
	int tic_xm, tic_ym;
	gchar *xticfmt;
	gchar *yticfmt;

        GtkWidget *p_popup_menu;
        GtkWidget *p_popup_menu_cv;
	GSimpleActionGroup *pc_action_group;

        Gxsm4appWindow *pc_in_window;
        GtkWidget *pc_grid;
	GtkWidget *canvas;
	GtkWidget *scrollarea;
	GtkWidget *statusbar;
	GtkWidget *linecounter;
	gint statusid;

	GSList *ScanList;
	ProfileElement *last_pe;
	
	cairo_item *frame, *background;
	cairo_item_text *titlelabel, *xaxislabel, *yaxislabel, *saxislabel, *infotext;

#define PC_XTN (2*Xticn+100)
#define PC_YTN (2*Xticn+100)
#define PC_XLN (Xticn+1+100)
#define PC_YLN (Yticn+1+100)
#define PC_LEGEND_ITEMS_MAX 256

	int Yticn, Xticn;
	int ixt, iyt, ixl, iyl;
	cairo_item      **Xtics, **Ytics;
	cairo_item_text **Xlabels, **Ylabels;
	cairo_item      **LegendSym;
	cairo_item_text **LegendText;
	cairo_item       *Cursor[2][2];
        cairo_item      *BBox;
        int bbox_mode;
        
	gchar *ys_legend_label_list[PC_LEGEND_ITEMS_MAX];
	int num_legend;

	int scount;

	int ref_count;
	gboolean lock;

	guint auto_update_id;

        //----- tmp data for simple event/dragging handing
        gboolean pointer_coord_display;
        int tmp_effected;
};

#endif
