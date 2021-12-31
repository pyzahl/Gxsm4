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

#ifndef APP_VOBJ_H
#define APP_VOBJ_H

#include <config.h>

#include <gtk/gtk.h>

#include "xsmtypes.h"
#include "gapp_service.h"
#include "scan.h"

#include "gxsm_app.h"
#include "app_vinfo.h"

#include "scan_object_data.h"

#include "cairo_item.h"

typedef enum { VOBJ_EV_BUTTON_NONE=0, VOBJ_EV_BUTTON_1=1, VOBJ_EV_BUTTON_2=2, VOBJ_EV_BUTTON_3=3, VOBJ_EV_BUTTON_WHEEL_UP=4, VOBJ_EV_BUTTON_WHEEL_DOWN=5 } VOBJ_EV_BUTTON;
typedef enum { VOBJ_EV_NONE, VOBJ_EV_BUTTON_PRESS, VOBJ_EV_MOTION_NOTIFY, VOBJ_EV_BUTTON_RELEASE, VOBJ_EV_ENTER_NOTIFY, VOBJ_EV_LEAVE_NOTIFY } VOBJ_EV_TYPE;
typedef struct { VOBJ_EV_BUTTON button; VOBJ_EV_TYPE type; gdouble x,y; } VObjectEvent;

class ProfileControl;

typedef enum { VOBJ_COORD_FROM_MOUSE, VOBJ_COORD_ABSOLUT, VOBJ_COORD_RELATIV } VOBJ_COORD_MODE;

#define MAX_NODES 16

class VObject : public scan_object_data{
 public:
	VObject (GtkWidget *canvas, double *xy0, int npkt, int pflg=FALSE, VOBJ_COORD_MODE cmode=VOBJ_COORD_FROM_MOUSE, const gchar *lab=NULL, double Marker_scale=1.);
	virtual ~VObject ();

	virtual gboolean check_event(VObjectEvent *event, double xy[2]);

	virtual void Update()=0;
	virtual void draw_extra(cairo_t *cr) {};
	virtual void SetUpPos(VOBJ_COORD_MODE cmode=VOBJ_COORD_FROM_MOUSE, int node=-1);
	virtual void SetUpScan();
	virtual void set_offset();
	virtual void set_global_ref();
	virtual void properties();
        static void properties_callback (GtkDialog *dialog,  int response, gpointer user_data);
	virtual void GoLocMax(int r=10);
	virtual void follow_on(){};
	virtual void follow_off(){};
	virtual int follow(){ return FALSE; };
	virtual void AddNode(){};
	virtual void DelNode(){};

	static void colorchange_callback(GtkColorChooser *colorsel, VObject *vo);
	static void fontchange_callback(GtkFontChooser *gfp, VObject *vo);
	static void label_changed_cb (GtkEditable *e, VObject *vo);
        static void selection_dim_changed_cb (GtkComboBox *cb, VObject *vo);
        static void selection_dimp_changed_cb (GtkComboBox *cb, VObject *vo);
        static void selection_dims_changed_cb (GtkComboBox *cb, VObject *vo);
        static void selection_data_changed_cb (GtkComboBox *cb, VObject *vo);
        static void selection_data_plot_changed_cb (GtkComboBox *cb, VObject *vo);
        static void selection_grid_plot_changed_cb (GtkComboBox *cb, VObject *vo);
        static void selection_info_options_changed_cb (GtkComboBox *cb, VObject *vo);
        
	void copy_xsmres_to_GdkRGBA (GdkRGBA &color, float c[4]){
                color.red   = c[0];
                color.green = c[1];
                color.blue  = c[2];
                color.alpha = c[3];
	};

	static void show_label_cb (GtkWidget *widget, VObject *vo);
	void show_label (gboolean flg=true);
	void update_label () { if (object_label) show_label (); };

	static void lock_position_cb (GtkWidget *widget, VObject *vo);
	void lock_object (gboolean l) { lock = l; };

	static void show_profile_cb (GtkWidget *widget,  VObject *vo);
	void show_profile (gboolean flg=true);

	static void run_m_param_optimization_cb (GtkWidget *widget, VObject *vo);
        virtual void run_m_param_opt (){};
	static void dump_param_cb (GtkWidget *widget, VObject *vo);
        virtual void dump_param (){};
        
	void build_properties_view (gboolean add=true);
        static void ec_properties_changed (Param_Control *pcs, gpointer data) { VObject* vob = (VObject*)data; vob->Update (); vob->remake_node_markers (); };
	void set_xy_node(double *xy_node, VOBJ_COORD_MODE cmode, int node=0);
	void insert_node(double *xy_node=NULL);

	cairo_item* node_marker (cairo_item* item, double *xy, int i);

	void remake_node_markers ();

	void draw (cairo_t *cr);
        void show () { show_flag = TRUE; };
        void hide () { show_flag = FALSE; };

	void Activate ();

	void set_color_to_active (); // "blue"
	void set_color_to_inactive (); // "red"
	void set_color_to_hilit ();
	void set_color_to_custom (gfloat fillcolor[4], gfloat outlinecolor[4]);

	void set_marker_scale (double ms) { marker_scale=ms; };
	double get_marker_scale () { return marker_scale/vinfo->GetZfac(); };
	void set_label_offset (double *xy) { label_offset_xy[0]=xy[0]; label_offset_xy[1]=xy[1]; };
	void set_object_label(const gchar *lab){ if (text) g_free (text); text = g_strdup (lab); }

	void set_custom_label_font (const gchar *f) { if (custom_label_font) g_free (custom_label_font); custom_label_font = g_strdup (f); };
	void set_custom_label_color (gfloat c[4]) { copy_xsmres_to_GdkRGBA (custom_label_color, c); };
	void set_custom_label_anchor (int anchor) { label_anchor = anchor; };
	void set_custom_element_color (gfloat c[4]) { copy_xsmres_to_GdkRGBA (custom_element_color, c); };
	void set_custom_element_b_color (gfloat c[4]) { copy_xsmres_to_GdkRGBA (custom_element_b_color, c); };

	// Show/Hide controls for spacetime (Layer, Time)
	void set_on_spacetime (gboolean flag, int spacetime[2], int id=0);
	void set_off_spacetime (gboolean flag, int spacetime[2], int id=0);
	void set_spacetime (int spacetime[2]);
	void get_spacetime (int spacetime[2]);
	gboolean is_spacetime (); // is showtime?
	
	// holds ScanEvent, non if NULL
	void set_scan_event (ScanEvent *se) { scan_event = se; };
	ScanEvent *get_scan_event () { return scan_event; };

	int GetRn(double *xyi, int n){ 
		if(n<np && n>=0){
			xyi[0] = xy[n<<1]; xyi[1] = xy[(n<<1)+1]; 
			return FALSE;
		}
		else return TRUE;
	};

	double Dist(int i=0, int j=1){
		double dx=xy[2*j  ]-xy[2*i  ];
		double dy=xy[2*j+1]-xy[2*i+1];
		return sqrt(dx*dx+dy*dy);
	}

	double Area(int i=0, int j=1){
		double dx=xy[2*j  ]-xy[2*i  ];
		double dy=xy[2*j+1]-xy[2*i+1];
		return fabs(dx*dy);
	}

	double Phi(double dx, double dy){
		double q23=0.;
		if(dx<0.)
			q23=180.;
		if(fabs(dx)>1e-5)
			return q23+180.*atan(dy/dx)/M_PI;
		else return dy>0.?90.:-90.;
	}

	double Phi(int i=0, int j=1){
		double dx=xy[2*j  ]-xy[2*i  ];
		double dy=xy[2*j+1]-xy[2*i+1];
		return Phi(dx, dy);
	}

	void save(std::ofstream &o){
		o << "(VObject \"" << name << "\" CustomColor (" << custom_element_color.red << " " << custom_element_color.green << " " << custom_element_color.blue << " " << custom_element_color.alpha << ")"
		  << std::endl
		  << "   (Label \"" << text << "\" "
		  << "Font \"" << custom_label_font << "\" "
		  << "MarkerSkl (" << marker_scale << ") "
		  << "Color (" << custom_label_color.red << " " << custom_label_color.blue << " " << custom_label_color.green << " " << custom_label_color.alpha << ") "
		  << "SpaceTimeOnOff ((" << space_time_on[0] << " " << space_time_on[1] << ") (" << space_time_off[0] << " " << space_time_off[1] << ")) "
		  << "SpaceTimeFromUntil (" << (space_time_from_0 ? 1:0) << " " << (space_time_until_00 ? 1:0) << ") "
		  << "Show (" << (object_label? 1:0) << ") "
		  << ")" << std::endl
		  << "   (NPkte " << np << ")" << std::endl
		  << "   (Coords i X Y (XAng YAng)" << std::endl;
		for(int i=0; i<np; ++i) {
			double x,y;
			x=xy[2*i]; y=xy[2*i+1];
			o << "     (" << i << " " << x << " " << y;
			vinfo->W2Angstroem (x,y);
			o << " (" << x << " " << y << "))" << std::endl;
		}
		if (profile){
			o << "   )" << std::endl 
			  << "   (ProfileActive PathWidthStep (" << path_width << " " << path_step << ") PathSerDimAllG2d (" << path_dimension << " " << series_dimension << " " << series_all << " " << plot_g2d << "))"
			  << ")" << std::endl 
			  << std::endl;
		} else
			o << "))" << std::endl 
			  << std::endl;
	};

	void saveHPGL(std::ofstream &o){
		double x,y;
		int i;
		for(i=0; i<np; ++i){
			x=xy[2*i]; y=xy[2*i+1];
			vinfo->W2Angstroem (x,y);
			if(i == 0) 
				o << "PU " << x << "," << y << ";" << std::endl;
			else
				o << "PD " << x << "," << y << ";" << std::endl;
		}
		if(i == 1) 
			o << "PD " << x << "," << y << ";" << std::endl;
	};
	int obj_id (int newid=0) { if (newid) id = newid; return id; };
	V_OBJECT_TYPE obj_type_id (V_OBJECT_TYPE newtid=O_NONE) { if (newtid!=O_NONE) type_id = newtid; return type_id; };
	gchar *obj_name () { return name; };
	void set_obj_name (const gchar *newname) { if(name) g_free(name); name = g_strdup(newname); };
	gchar *obj_text ( const gchar *t=NULL ) { 
		if (t) { 
			if (text) g_free (text);
			text = g_strdup (t);
			return NULL; 
		} else return text; 
	};

	// profile
	void   set_profile_path_width (double w=1.) { path_width = w; };
	double get_profile_path_width () { return path_width; };
	void   set_profile_path_step (double s=1.) { path_step = s; };
	double get_profile_path_step () { return path_step; };
	void   set_profile_series_limit (int i, int s=0) { if (i>=0 && i<2) series_limits[i] = s; };
	int    get_profile_series_limit (int i) { if (i>=0 && i<2) return (int)series_limits[i]; else return 0; };
	void   set_profile_path_dimension  (MEM2D_DIM d=MEM2D_DIM_XY) { path_dimension = d; };
	MEM2D_DIM get_profile_path_dimension  () { return path_dimension; };
	void   set_profile_series_dimension  (MEM2D_DIM d=MEM2D_DIM_LAYER) { series_dimension = d; };
	MEM2D_DIM get_profile_series_dimension  () { return series_dimension; };
	void   set_profile_series_all (gboolean all=FALSE) { series_all = all; };
	gboolean get_profile_series_all () { return series_all; };
	void   set_profile_series_pg2d (gboolean pg2d=FALSE) { plot_g2d = pg2d; };
	gboolean get_profile_series_pg2d () { return plot_g2d; };

	void set_osd_style (gboolean flg);
	gboolean get_osd_style () { return label_osd_style; };

	void destroy_properties_bp ();
        
	GtkWidget *canvas;
	GSimpleActionGroup *gs_action_group;


        // scan_object_data implementation
        virtual gchar *get_name () { return name; };
        virtual gchar *get_text () { return text; };
        virtual gint get_num_points () { return np; };
        virtual gint get_id () { return id; };
	virtual void get_xy_i (int i, double &x, double &y) { 
		if (i>=0) { // default (i=0...np) is Angstroems -- conveniance hack
			if (i<np){ 
				x=xy[2*i]; y=xy[2*i+1]; 
				vinfo->W2Angstroem (x,y);
			}
		} else { // but -1, ... -np-1 is in pixels!
			i = -i-1;
			if (i<np){ 
				x=xy[2*i] * vinfo->GetQfac (); 
				y=xy[2*i+1] * vinfo->GetQfac (); 
			}
		} 
	};
        virtual void get_xy_i_pixel2d (int i, Point2D *p){
                if (i>=0 && i<np)
                        vinfo->XYview2pixel (xy[2*i], xy[2*i+1], p); 
        };
        virtual gboolean selected() { return is_selected; };

 protected:
	gchar *name;
	gchar *text;
	int np;

	UnitObj *Pixel;
	UnitObj *Unity;
	BuildParam *properties_bp;
	ProfileControl *profile;
	double path_width; // path width or area radius
	double path_step; // path step (average length in path direction)
	double series_limits[2];
	MEM2D_DIM path_dimension;
	MEM2D_DIM series_dimension;
	gboolean series_all; // FALSE: current only, TRUE: plot all series
	gboolean plot_g2d; // FALSE: no 2d grey 2d plot, TRUE: plot also in as 2d grey/false color image

	gint grid_mode; // GRID/CIRC/...
	gint info_option; // Inof options
	double grid_aspect;
	double grid_base;
	gint grid_multiples;
	gint grid_size;
        GSList *ECP_list;
        double m_parameter[10];
        double m_dopt[10];
        double m_dopt_r[5];
        double m_n_opt;
        int m_verbose;
        double total_score;
        
	double marker_scale;
	double label_offset_xy[2];
	double *xy;
        double m_phi;
	cairo_item **abl;
	cairo_item_text *object_label;
        int label_anchor;
	cairo_item *arrow_head[MAX_NODES];
	cairo_item *cursors[2];
	cairo_item *avg_area_marks[2*MAX_NODES];
	cairo_item *avg_circ_marks[2*MAX_NODES];
	cairo_item *selected_bbox;
	GtkWidget *popup;
	GtkWidget *statusbar;
	gint statusid;
	ViewInfo *vinfo;

	cairo_item *touched_item;
	double touched_xy[2];

	ScanEvent *scan_event;
	gboolean label_osd_style;

	GdkRGBA custom_element_color;
	GdkRGBA custom_element_b_color;

	gboolean dragging_active;

        gboolean is_selected;
 private:
	int id;
	V_OBJECT_TYPE type_id;
	gboolean lock;
        gboolean show_flag;

	int space_time_now[2], space_time_on[2], space_time_off[2];
	gboolean space_time_from_0, space_time_until_00;

	gchar *custom_label_font;
	GdkRGBA custom_label_color;

	GtkWidget *obj_popup_menu;
};


class VObPoint : public VObject{
 public:
	VObPoint(GtkWidget *canvas, double *xy0, int pflg=FALSE, VOBJ_COORD_MODE cmode=VOBJ_COORD_FROM_MOUSE, const gchar *lab=NULL, double Marker_scale=1.);
	virtual ~VObPoint();

	virtual void draw_extra(cairo_t *cr);
	
	virtual void Update();
	virtual void follow_on();
	virtual void follow_off();
	virtual int follow_me(){ return follow; };

 private:
	void update_offset();
	void update_scanposition();
	int follow;
};


class VObLine : public VObject{
 public:
	VObLine(GtkWidget *canvas, double *xy0, int pflg=FALSE, VOBJ_COORD_MODE cmode=VOBJ_COORD_FROM_MOUSE, const gchar *lab=NULL, double Marker_scale=1.);
	virtual ~VObLine();
	virtual void AddNode();

	virtual void Update();

 private:
	double posA, posB;
};

class VObPolyLine : public VObject{
 public:
	VObPolyLine(GtkWidget *canvas, double *xy0, int pflg=FALSE, VOBJ_COORD_MODE cmode=VOBJ_COORD_FROM_MOUSE, const gchar *lab=NULL, double Marker_scale=1.);
	virtual ~VObPolyLine();
	virtual void AddNode();
	virtual void DelNode();

	virtual void Update();
};

class VObTrace : public VObject{
 public:
	VObTrace(GtkWidget *canvas, double *xy0, int pflg=FALSE, VOBJ_COORD_MODE cmode=VOBJ_COORD_FROM_MOUSE, const gchar *lab=NULL, double Marker_scale=1.);
	virtual ~VObTrace();
	virtual void Change(double *xy0);

	virtual void Update();
	private:
	int trlen;
	cairo_item *trail;
};

class VObKsys : public VObject{
 public:
	VObKsys(GtkWidget *canvas, double *xy0, int pflg=FALSE, VOBJ_COORD_MODE cmode=VOBJ_COORD_FROM_MOUSE, const gchar *lab=NULL, double Marker_scale=1.);
	virtual ~VObKsys();

	virtual void Update();
	virtual void draw_extra(cairo_t *cr) {
	  if (atoms) atoms->draw (cr);
	  if (lines) lines->draw (cr);
	  if (bonds) bonds->draw (cr);
          if (info)
                  for(int i=0; i<n_info; ++i)
                          if (info[i]) info[i]->draw (cr);
	};
        void print_xyz (double x, double y);
        void add_bond_len (cairo_item *bonds, int i1, int i2, cairo_item_text **cit, const gchar *lab=NULL);
        double score_bond (cairo_item *bonds, int i1, int i2, int n);
        void adjust_bond_aromatic_index (cairo_item_segments_wlw *bonds, int i1, int i2, double ai=10.);
        void bonds_matchup (cairo_item *bonds);
        virtual void dump_param ();
        virtual void run_m_param_opt ();
        void opt_adjust_xy (int k, double d);
        
 private:
	void calc_grid();
	void destroy_atoms();
	void update_grid();
	cairo_item_segments *lines;
	cairo_item_circle *atoms;
	cairo_item_segments_wlw *bonds;
	cairo_item_text **info;
	gint n_atoms;
	gint n_lines;
	gint n_bonds;
	gint n_info;
};

class VObParabel : public VObject{
 public:
	VObParabel(GtkWidget *canvas, double *xy0, int pflg=FALSE, VOBJ_COORD_MODE cmode=VOBJ_COORD_FROM_MOUSE, const gchar *lab=NULL, double Marker_scale=1.);
	virtual ~VObParabel();

	virtual void Update();

 private:
};

class VObRectangle : public VObject{
 public:
	VObRectangle(GtkWidget *canvas, double *xy0, int pflg=FALSE, VOBJ_COORD_MODE cmode=VOBJ_COORD_FROM_MOUSE, const gchar *lab=NULL, double Marker_scale=1.);
	virtual void SetUpScan();
	virtual ~VObRectangle();
	virtual void Update();

};

class VObCircle : public VObject{
 public:
	VObCircle(GtkWidget *canvas, double *xy0, int pflg=FALSE, VOBJ_COORD_MODE cmode=VOBJ_COORD_FROM_MOUSE, const gchar *lab=NULL, double Marker_scale=1.);
	virtual ~VObCircle();

	double Radius(){
		return Dist();
	}

	virtual void Update();
};

class VObEvent : public VObject{
 public:
	VObEvent(GtkWidget *canvas, double *xy0, int pflg=FALSE, VOBJ_COORD_MODE cmode=VOBJ_COORD_FROM_MOUSE, const gchar *lab=NULL, double marker_scale=0.4);
	virtual ~VObEvent();
	
	virtual void Update();

 private:
};

#endif
