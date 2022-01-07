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

#ifndef __VIEW_H
#define __VIEW_H

class ScanEvent;
class ViewControl;
class ProfileControl;
class V3dControl;

#include "gnome-res.h"

#include "mem2d.h"
#include "xshmimg.h"
#include "gxsm_app.h"

#include <stdlib.h>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>


#define TOPO_BORDER_WIDTH 14
#define TOPO_MINSIZE      400 // Default Wert

#define NUM_TRACE_VALUES 4
typedef struct{
	double x,y,z;
	double v[NUM_TRACE_VALUES];
	int mode;
	clock_t t;
} Trace_Data;

typedef enum { DEFAULT_GREY, USER_FALSE_COLOR, NATIVE_4L_RGBA } COLORMODE;

/*
 * XSM View Basis-Objekt:
 * Basis aller Arten von View's
 * ============================================================
 */

class View{
public:
        View();
        View(Scan *sc, int ChNo);
        virtual ~View();

        virtual void hide();
        virtual ViewControl* Get_ViewControl () { return NULL; };

        virtual int draw(int zoomoverride=FALSE);
        virtual int update(int y1, int y2);
        virtual void add_object(int type, gpointer data){};
        virtual void setup_data_transformation(){};

        virtual void update_mxyz_from_points ();

        virtual void ZoomIn(){};
        virtual void ZoomOut(){};
  

        // manipulation visualisation
        void abs_move_tip(double xa, double ya, int mode=0);
        void rel_move_tip(double dxa, double dya, int mode=0);
        virtual void show_tip(){};
        virtual void hide_tip(){};

        void update_position(Trace_Data *td);
        void add_to_trace(Trace_Data *td);
        void print_trace();
        void save_trace();
        void reset_trace();
        static void print_td(Trace_Data *td, gpointer data);
        static void delete_td(Trace_Data *td, gpointer data) { delete td; };
        virtual void show_trace(){};
        virtual void hide_trace(){};

        virtual void update_trace(){};

        virtual void update_events(){};
        virtual void remove_events(){};

        virtual void update_event_info(ScanEvent *se){};

        virtual gpointer grab_OSD_canvas(){ return NULL; };

        virtual void color_mode(COLORMODE cm){ cmode = cm; };
        
        Scan* get_scan() { return scan; };
        
        int ZoomFac;
        int QuenchFac;
        COLORMODE cmode;

protected:
        Scan *scan;
        SCAN_DATA *data;
        int ChanNo;
        GSList *trace;
private:
        Trace_Data current_td;
        clock_t trace_t0;
};

/*
 *
 * Folgende Viewing-Modelle sind geplant:
 * ========================================
 * 2D Graubild, Darstellungsmode "Quick"/"Direct" über mem2d-Mapping
 * class Grey2D    : public View{};
 *
 * Linien-Profil-Dartsellung aus 2D Z Daten
 * class Line      : public View{};
 *
 * Schnelle Profil-Dartsellung aus 2D Z Daten, Höhenprofilen untereinander
 * class Profiles  : public View{};
 *
 * Echte-3D-Dartsellung ... + povray Unterstützung, etc....
 * class Surface3D : public View{};
 *
 */


/*
 * Grey2D Ableitung
 * ============================================================
 */

class Grey2D : public View{
public:
        Grey2D();
        Grey2D(Scan *sc, int ChNo=-1);
        virtual ~Grey2D();
        virtual ViewControl* Get_ViewControl () { return viewcontrol; };

        virtual void hide();
        virtual int draw(int zoomoverride=FALSE);
        virtual int update(int y1, int y2);
        virtual void add_object(int type, gpointer data);
        virtual void setup_data_transformation();
        static int SetZF(int zf, int qf, Grey2D *p);

        virtual void ZoomIn();
        virtual void ZoomOut();

        // manipulation visualisation
        virtual void show_tip();
        virtual void hide_tip();

        virtual void show_trace();
        virtual void hide_trace();

        virtual void update_trace();

        virtual void update_events();
        virtual void remove_events();

        virtual void update_event_info(ScanEvent* se);

        virtual gpointer grab_OSD_canvas();

private:  
        int  MaxColor;
        int  XPM_x, XPM_y;
        int  oXPMx,oXPMy,oZ,oQ,oMC; /* old Values */
        int  oVm;
        int  userzoom;
			 
        ViewControl *viewcontrol;
        ShmImage2D *XImg;
};

typedef struct{
	float trans[3];
	float rot[3];
	float fov;
	float camera[3];
        float ffar,fnear;
        
        gchar height_scale_mode[30];
	float hskl;
	float tskl;
	float slice_offset;

	gchar view_preset[30];
	gchar look_at[30];
	gchar vertex_source[30];
	float slice_start_n[4];
	float slice_plane_index[4];

	float light_global_ambient[4];
	gchar light[3][8];
	float light_position[3][4];
	float light_ambient[3][4];
	float light_specular[3][4];
	float light_diffuse[3][4];
	int   light_spot[3];	
	float light_spot_direction[3][3];
	float light_spot_cutoff[3];
	float light_spot_exponent[3];

        gchar tip_display[8];
        float tip_geometry[2][4];
        float tip_colors[4][4];
        
	float surf_mat_color[4];
	float surf_mat_backside_color[4];
	float surf_mat_ambient[4];
	float surf_mat_specular[4];
	float surf_mat_diffuse[4];
	float surf_mat_shininess[1];

	float anno_zero_plane_color[4];
	float anno_title_color[4];
	float anno_label_color[4];
        gchar anno_title[60];
        gchar anno_xaxis[60];
        gchar anno_yaxis[60];
        gchar anno_zaxis[60];
	gchar anno_show_title[8];
	gchar anno_show_axis_labels[8];
	gchar anno_show_axis_dimensions[8];
	gchar anno_show_bearings[8];
	gchar anno_show_zero_planes[8];
        
	int Fog;
	float fog_color[4];
	float fog_density;

	float clear_color[4];

	int Texture;
	int Mesh;
	int Cull;
	int Emission;
	int Smooth;
	int Ortho;
	int Ticks;
        int ZeroOld;
        float probe_atoms;
	gchar TickFrameOptions[30];
	int TransparentSlices;
	float transparency;
	float transparency_offset;
	
	float shader_mode;
        float tess_level;
        float tex3d_lod;
        float base_plane_size;
        
	gchar ShadeModel[30];
	gchar ColorSrc[30];	

	float Lightness, ColorOffset, ColorSat;

	int ZeroPlane;
	int InstantUpdate;

        float animation_index;
        gchar animation_file[256];

} Surf3d_GLview_data;

class gl_400_3D_visualization;

class Surf3d  : public View{
public:
        Surf3d();
        Surf3d(Scan *sc, int ChNo);
        virtual ~Surf3d();

        void end_gl ();
        virtual void hide();
        virtual int draw(int zoomoverride=FALSE);
        virtual int update(int y1, int y2);
        virtual void ZoomIn(){};
        virtual void ZoomOut(){};
        virtual void setup_data_transformation();

        static void GLupdate(void* data);

        void SaveImagePNG(GtkGLArea *area, const gchar *fname_png); 
        
        double GetXYZNormalized (float *r, gboolean z_scale_abs=false);
        double GetCurrent();
        double GetForce();
        void GetXYZScale (float *s, gboolean z_scale_abs=false);
        void MouseControl (int mouse, double x, double y);
        void UpdateGLv_control(){
                v3dControl_pref_dlg->block = TRUE;
                gnome_res_update_all (v3dControl_pref_dlg);
                v3dControl_pref_dlg->block = FALSE;
        };
        void Rotate(int n, double dphi);
        void RotateAbs(int n, double phi);
        double RotateX(double dphi);
        double RotateY(double dphi);
        double RotateZ(double dphi);

        void Translate(int n, double delta);
        double Zoom(double x);
        double HeightSkl(double x);

        void ColorSrc();
        void GLModes(int n, int m);

        void preferences();
        gboolean is_ready() { return (size > 0 && QuenchFac > 0 && scan != NULL && XPM_x > 1 && XPM_y > 1);  };

private: 
        void PutPointMode(int k, int j, int v=-1);
        void ReadPalette(char *name);

        Mem2d *mem2d_x;
        Surf3d *self;
        
        GnomeResPreferences *v3dControl_pref_dlg;
        GnomeResEntryInfoType *v3dControl_pref_def;

        void GLvarinit ();
        void delete_surface_buffer ();
        void GLdrawsurface (int y_to_update=-1, int refresh_all=FALSE);
        void GLdrawGimmicks ();
        void printstring (void *font, char *string);
        void calccolor (float height, glm::vec4 &c);

#define GXSM_GPU_PALETTE_ENTRIES 8192
        glm::vec4 *surface_z_data_buffer;
        glm::vec4 ColorLookup[GXSM_GPU_PALETTE_ENTRIES];
        
public:
        void create_surface_buffer ();
        void set_gl_data ();
        gboolean check_dimension_changed();
        
        gint XPM_x, XPM_y, XPM_v;
        int maxcolors;
        size_t size;
        gint scrwidth;
        gint scrheight;

        Surf3d_GLview_data GLv_data;

        gl_400_3D_visualization *gl_tess;
        
private:
        V3dControl *v3dcontrol;
};

#endif



