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

#include "gxsm_app.h"

#include "unit.h"
#include "pcs.h"
#include "xsmtypes.h"
#include "action_id.h"
#include "glbvars.h"
#include "surface.h"

#include "app_profile.h"
#include "app_vobj.h"
#include "app_view.h"

#include "gtk/gtk.h"

#include "clip.h"
#include "gxsm_monitor_vmemory_and_refcounts.h"

#define MAXHANDLECOLORS 4

#define OBJECT_LINE_WIDTH     xsmres.ObjectLineWidth
#define SQR_HANDLE_SIZE       5
#define SQR_HANDLE_LINE_WIDTH xsmres.HandleLineWidth
#define TRI_HANDLE_SIZE       15
#define TRI_HANDLE_LINE_WIDTH xsmres.HandleLineWidth

#define LABEL_XOFF 0.
#define LABEL_YOFF -25.

inline guint32 RGBAColor (float c[4]) { 
	if (c[3] < 0.01) return 0;
	return	(guint32)(c[3]*0xff)
		| ((guint32)(c[2]*0xff)<<8)
		| ((guint32)(c[1]*0xff)<<16)
		| ((guint32) (c[0]*0xff)<<24)
		;	
}



typedef struct
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
}RGB_color;

typedef struct
{
    unsigned char h;
    unsigned char s;
    unsigned char v;
}HSV_color;

void HsvToRgb(HSV_color &hsv, RGB_color &rgb){ // Conversion Utility
        unsigned char region, remainder, p, q, t;

        if (hsv.s == 0){
                rgb.r = hsv.v;
                rgb.g = hsv.v;
                rgb.b = hsv.v;
        }

        region = hsv.h / 43;
        remainder = (hsv.h - (region * 43)) * 6;

        p = (hsv.v * (255 - hsv.s)) >> 8;
        q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
        t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

        switch (region){
        case 0:
                rgb.r = hsv.v; rgb.g = t; rgb.b = p;
                break;
        case 1:
                rgb.r = q; rgb.g = hsv.v; rgb.b = p;
                break;
        case 2:
                rgb.r = p; rgb.g = hsv.v; rgb.b = t;
                break;
        case 3:
                rgb.r = p; rgb.g = q; rgb.b = hsv.v;
                break;
        case 4:
                rgb.r = t; rgb.g = p; rgb.b = hsv.v;
                break;
        default:
                rgb.r = hsv.v; rgb.g = p; rgb.b = q;
                break;
        }
};

static const struct {
  guint  	 width;
  guint  	 height;
  guint  	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  guint8 	 pixel_data[256 * 1 * 3 + 1];
} pal_image = {
  256, 1, 3,
  "Z[\335dd\377bf\375dd\376ed\375di\375er\377br\374ep\376cq\374cu\374g|\377"
  "f~\376e}\376f~\375f~\372g\207\374e\214\375e\213\375e\213\374d\213\375f\222"
  "\377d\231\376d\230\376f\227\376d\230\376d\231\373g\244\375e\246\376d\245"
  "\377d\246\376c\247\375d\257\377c\261\376e\261\377d\261\377c\261\376c\267"
  "\376e\277\376c\277\376d\277\376e\277\377d\303\376e\312\377c\313\376d\313"
  "\376c\312\376b\314\375e\325\377d\330\377d\330\377e\330\377d\330\376e\341"
  "\377d\346\377e\345\376d\345\376e\346\376d\350\374e\361\377c\363\376d\361"
  "\376e\362\376d\363\374g\375\377e\377\377g\376\377e\377\376e\377\376e\377"
  "\370d\376\362d\376\363e\376\362f\376\362f\376\355e\377\350e\377\350e\377"
  "\346e\377\350f\377\345e\376\335d\376\331d\376\330d\376\332d\376\330f\376"
  "\321e\377\315e\377\313f\376\313f\376\313f\377\310d\376\301e\377\275d\376"
  "\276d\376\276e\376\274d\376\266d\377\262d\377\263d\376\264e\377\261e\376"
  "\254d\376\246d\376\245e\375\246e\376\245e\376\240e\376\232d\377\232d\377"
  "\230f\376\231f\377\227e\377\217d\377\212d\377\211d\377\212e\376\213e\377"
  "\205d\377\200d\377\177e\376~e\376\200e\377zd\376se\376pe\376oe\376re\376"
  "pd\377ie\376df\376ed\377fe\377el\377er\377cs\376cp\377bs\376c{\376e\200\376"
  "e\177\377d~\377d\200\377d\202\376d\216\377d\220\377c\220\377c\220\377c\220"
  "\377c\227\376c\235\377d\237\376d\237\376d\236\377d\243\376c\252\377d\253"
  "\376d\254\376d\253\376d\254\376d\264\377f\272\377e\271\376c\270\377d\272"
  "\376e\302\377f\310\377f\311\377e\307\377e\310\377f\314\375e\324\376f\325"
  "\377e\324\377e\327\376g\330\375d\341\376e\345\376d\343\377d\344\377d\343"
  "\376e\353\377e\363\376d\362\377d\361\377e\361\376f\367\376e\377\376e\377"
  "\375d\377\376e\376\377e\376\375f\376\365c\377\362e\377\362f\376\363f\376"
  "\362e\376\352d\376\346f\376\345f\375\347g\375\347f\377\342h\376\330d\377"
  "\327e\377\327e\375\327d\376\325e\376\314d\377\312f\377\311e\377\313e\377"
  "\313f\376\304g\377\277e\375\300e\376\300d\375\277f\376\272e\376\262d\376"
  "\261g\374\262g\374\262d\376\261e\375\250e\377\246e\377\245c\376\247b\377"
  "\246d\376\234d\377\230e\376\230c\377\227e\376\231e\377\223d\377\213c\377"
  "\213e\377\213d\375\214d\376\207d\377\177d\377}f\377~e\377~d\377}d\376vc\377"
  "re\377qe\376re\377pg\376kf\376cd\377be\376dd\377ae\355ba",
};




VObject *current_vobject2 = NULL;

void VObject::set_osd_style (gboolean flg){
	label_osd_style = flg;

	if (custom_label_font)
		g_free (custom_label_font);
	custom_label_font = g_strdup (label_osd_style? xsmres.OSDFont : xsmres.ObjectLabFont);
        copy_xsmres_to_GdkRGBA (custom_label_color, xsmres.ObjectLabColor);
}

VObject::VObject(GtkWidget *Canvas, double *xy0, int npkt, int pflg, VOBJ_COORD_MODE cmode, const gchar *lab, double Marker_scale){
        GXSM_REF_OBJECT (GXSM_GRC_VOBJ);
	static int obj_count = 0;
	static int event_count = 0;
	int i;
	XSM_DEBUG(DBG_L2, "VObject::VObject");
        show ();

        properties_bp = NULL;
	Pixel = new UnitObj("Pix","Pix");
	Unity = new UnitObj(" "," ");

	scan_event=NULL;
	lock=false;
	id=0;
	label_osd_style = FALSE;
        set_custom_label_anchor (CAIRO_ANCHOR_N);

        dragging_active = false;
        
	grid_multiples = 4;
	grid_size = 1;
	grid_mode = 2;
	info_option = 1;
	grid_aspect=1.0;
	grid_base=1.0;
        m_n_opt=10;
        m_verbose=1;
        m_phi=0.;
        for (int i=0; i<10; ++i) m_parameter[i]=0.0, m_dopt[i]=0.02;
        for (int i=0; i<2; ++i) m_dopt_r[i]=2.0;
        for (int i=2; i<5; ++i) m_dopt_r[i]=0.02;
        
	space_time_now[0]=space_time_now[1]=0;
	space_time_on[0]=space_time_on[1]=0;
	space_time_off[0]=space_time_off[1]=0;
	space_time_from_0 = TRUE;
	space_time_until_00 = TRUE;

	custom_label_font = g_strdup (label_osd_style? xsmres.OSDFont : xsmres.ObjectLabFont);
        copy_xsmres_to_GdkRGBA (custom_label_color, label_osd_style? xsmres.OSDColor : xsmres.ObjectLabColor);
	copy_xsmres_to_GdkRGBA (custom_element_color, xsmres.ObjectElmColor);
	copy_xsmres_to_GdkRGBA (custom_element_b_color, xsmres.ObjectElmColor);
	custom_element_b_color.alpha = 1.;
	custom_element_b_color.red   *= 0.66;
	custom_element_b_color.green *= 0.66;
	custom_element_b_color.blue  *= 0.66;

	type_id=O_NONE;
	set_marker_scale (Marker_scale);
	canvas = Canvas;
	statusbar = (GtkWidget*)g_object_get_data (G_OBJECT (canvas), "statusbar");
	statusid  = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "drag");
	vinfo = (ViewInfo*)g_object_get_data (G_OBJECT (canvas), "ViewInfo");

	name = g_strdup("VObject");
	if (lab)
		text = g_strdup(lab);
	else
		text = npkt == 0 
			? g_strdup_printf("Ev[%d]", ++event_count)
			: g_strdup_printf("Object[%d]", ++obj_count);

	np=npkt > 0 ? npkt : 1;
	abl = new cairo_item* [np+MAX_NODES+1];

        selected_bbox = NULL;

	object_label = NULL;
        label_anchor = CAIRO_ANCHOR_CENTER;
        cursors[0] = NULL;
	cursors[1] = NULL;
	for (i=0; i<MAX_NODES; ++i){
		arrow_head[i]= NULL;
		avg_area_marks[2*i+0] = NULL;
		avg_area_marks[2*i+1] = NULL;
		avg_circ_marks[2*i+0] = NULL;
		avg_circ_marks[2*i+1] = NULL;
	}
	label_offset_xy[0]=LABEL_XOFF;
	label_offset_xy[1]=LABEL_YOFF;

	xy = new double[2*np];
	memcpy(xy, xy0, 2*np*sizeof(double));

	touched_item=NULL;
	touched_xy[0]=touched_xy[1]=0.;

	SetUpPos (cmode);

	for(i=0; i<np; i++){
		abl[i] = node_marker (NULL, &xy[2*i], i);
                abl[i]->queue_update (canvas);
	}
	for(; i<np+MAX_NODES;)
                abl[i++]=NULL;

	abl[i++]=NULL; // never use!! END mark.

	profile = NULL;

	set_profile_path_width ();
	set_profile_path_step ();
	set_profile_path_dimension ();
	set_profile_series_dimension ();
	set_profile_series_all ();
	set_profile_series_pg2d ();

	show_profile (pflg);

        // new action group
	gs_action_group = g_simple_action_group_new ();
        
        // setup object menu
        {
                GObject *ctx_popup=NULL;
                
                switch (npkt){
                case 0:  ctx_popup = main_get_gapp ()->get_vobj_ctx_menu_event (); break;
                case 1:  ctx_popup = main_get_gapp ()->get_vobj_ctx_menu_1p (); break;
                case 2:  ctx_popup = main_get_gapp ()->get_vobj_ctx_menu_2p (); break;
                default: ctx_popup = main_get_gapp ()->get_vobj_ctx_menu_np (); break;
                }

                //**** https://devdocs.io/gtk~4.0/gtkpopovermenu#gtk-popover-menu-new-from-model
                obj_popup_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (ctx_popup));
                //gtk_popover_set_default_widget (GTK_POPOVER (obj_popup_menu), canvas);
                //gtk_popover_set_child (GTK_POPOVER (obj_popup_menu), canvas);
                gtk_popover_set_has_arrow (GTK_POPOVER (obj_popup_menu), TRUE);
                gtk_widget_set_parent (obj_popup_menu, canvas);
 
        }


}


VObject::~VObject(){
        int i;
        XSM_DEBUG_GP (DBG_L4, "VObject::~VObject  UNREF_DELETE_CAIRO_ITEM: object %s [%s]\n", name, text);

	if(profile) 
		delete profile;
        
	profile=NULL;

        if (properties_bp)
                g_object_unref (GTK_GRID (properties_bp->grid));

        destroy_properties_bp ();
	delete Pixel;
	delete Unity;

        
        UNREF_DELETE_CAIRO_ITEM (selected_bbox, canvas);

	for(i=0; abl[i]; i++)
                UNREF_DELETE_CAIRO_ITEM (abl[i], canvas);

        UNREF_DELETE_CAIRO_ITEM (object_label, canvas);

	for (i=0; i<MAX_NODES; ++i){
                UNREF_DELETE_CAIRO_ITEM (arrow_head[i], canvas);
        }
        
	for (i=0; i<2; ++i){
                UNREF_DELETE_CAIRO_ITEM (cursors[i], canvas);
        }

        for (i=0; i<2*MAX_NODES; ++i){
		UNREF_DELETE_CAIRO_ITEM (avg_area_marks[i], canvas);
		UNREF_DELETE_CAIRO_ITEM (avg_circ_marks[i], canvas);
	}

	delete[] xy;

	delete[] abl;

	g_free(name);
	g_free(text);
	if (custom_label_font)
		g_free (custom_label_font);


        g_object_unref (gs_action_group);

        GXSM_UNREF_OBJECT (GXSM_GRC_VOBJ);
}

void VObject::show_profile_cb (GtkWidget *widget, VObject *vo){
        if (widget)
                vo->show_profile (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        else 
                vo->show_profile ();
        vo->Update ();
}

void VObject::show_profile (gboolean pflg){
	if(pflg){
		if (profile) // already exists, done
			return;
		if (np == 1){
			set_profile_path_dimension (MEM2D_DIM_LAYER);
			set_profile_series_dimension (MEM2D_DIM_TIME); // always other
		} else {
			set_profile_path_dimension (MEM2D_DIM_XY);
			set_profile_series_dimension (MEM2D_DIM_LAYER);
		}
                // create new
		gchar *proftit = g_strdup_printf("%s Profile from Ch%d : w%g:d%d:i%d", 
						 np>1 ? "Path":"Slice", 
						 1 + GPOINTER_TO_INT (g_object_get_data (G_OBJECT (canvas), "Ch")),
						 get_profile_path_width (),
						 get_profile_path_dimension (),
						 get_profile_series_dimension ()
						 );
		profile = new ProfileControl((Gxsm4app *) g_object_get_data (G_OBJECT (canvas), "MAIN_APP"), proftit);        
		g_free(proftit);

	} else { // clenanup

                if(profile){
			delete profile;
			profile = NULL;
		}
	}
}

void VObject::set_xy_node(double *xy_node, VOBJ_COORD_MODE cmode, int node){
	if (node < np){
		xy[2*node]   = xy_node[0]; xy[2*node+1] = xy_node[1];
		SetUpPos(cmode, node);
		node_marker (abl[node], &xy[node], node);
                abl[node]->queue_update (canvas);
		Update ();
	}
}

void VObject::insert_node(double *xy_node){
	double *xy_tmp = new double[2*np+2];
	cairo_item** abl_tmp = new cairo_item* [np+MAX_NODES+1];
	for(int i=0; i<np; i++){
		abl_tmp[i]  = abl[i];
		xy_tmp[2*i] = xy[2*i];
		xy_tmp[2*i+1] = xy[2*i+1];
	}
	if (xy_node){
		xy_tmp[2*np] = xy_node[0];
		xy_tmp[2*np+1] = xy_node[1];
		vinfo->Angstroem2W (xy_tmp[2*np], xy_tmp[2*np+1]);
	} else {
		xy_tmp[2*np] = xy[2*(np-1)] + 30;
		xy_tmp[2*np+1] = xy[2*(np-1)+1] + 30;
	}
	abl_tmp[np] = node_marker (NULL, &xy[2*np], np);
        abl_tmp[np]->queue_update (canvas);

        for (int k=0; k<MAX_NODES; ++k)
                abl_tmp[np+k+1] = abl[np+k];

	delete[] xy;
	delete[] abl;
	
	xy = xy_tmp;
	abl = abl_tmp;

	np++;

	remake_node_markers ();
}

/*
 * make/update object handle markers
 * if item == NULL, it is created new, else updated!
 */
cairo_item* VObject::node_marker (cairo_item* item, double *xy, int i){
	double rsz = TRI_HANDLE_SIZE * get_marker_scale ();
        if (!item)
                item = new cairo_item_path_closed(3);

        item->set_position (xy[0], xy[1]);
        item->set_xy (0, 0., 0.);
        item->set_xy (1, -rsz/3., -rsz);
        item->set_xy (2,  rsz/3., -rsz);

        item->set_stroke_rgba (i);  // "outline_color", HandleColors[i%MAXHANDLECOLORS],
        item->set_line_width (TRI_HANDLE_LINE_WIDTH);  // "width_pixels", TRI_HANDLE_LINE_WIDTH,

        return item;
}

void VObject::SetUpPos(VOBJ_COORD_MODE cmode, int node){
	switch (cmode){
	case VOBJ_COORD_FROM_MOUSE:
		{
                        double *mxy = (double*) g_object_get_data (G_OBJECT (canvas), "mouse_pix_xy"); // get mouse coordinates on cairo drawing area / canvas
                        // g_print ("SETUP OBJECT COORDINATES ++ %g %g\n", mxy[0], mxy[1]);

			if (node >= 0 && node < np){
				xy[2*node]   += mxy[0];
				xy[2*node+1] += mxy[1];
			} else
				for(int i=0; i<np; ++i){
					xy[2*i]   += mxy[0];
					xy[2*i+1] += mxy[1];
				}
		} 
		break;
	
	case VOBJ_COORD_ABSOLUT:
		if (node >= 0 && node < np)
			vinfo->Angstroem2W (xy[2*node], xy[2*node+1]);
		else 
			for(int i=0; i<np; ++i)
				vinfo->Angstroem2W (xy[2*i], xy[2*i+1]);
		break;
	case VOBJ_COORD_RELATIV:
		; // not available
		break;
	}
}

void VObject::draw (cairo_t *cr){
        // XSM_DEBUG_GM (DBG_L3, "DBG: VObject::draw name=%s, label=%s, show=%d, osd=%d", name, object_label ? object_label->get_text() : "N/A", show_flag, label_osd_style);
        if (show_flag){ // master show/hide for all
                //  g_print ("vobj::draw %s at %g %g\n", name, xy[0], xy[1]);
                if (!label_osd_style)
                        for(int i=0; i<np+MAX_NODES; i++)
                                if (abl[i]) 
                                        abl[i]->draw (cr);
                
                if (object_label){
                        object_label->draw (cr);
                        // object_label->print ();
                }
                if (cursors[0])
                        cursors[0]->draw (cr);
                if (cursors[1])
                        cursors[1]->draw (cr);
                for (int i=0; i<MAX_NODES; ++i)
                        if (arrow_head[i])
                                arrow_head[i]->draw (cr);
                for (int i=0; i<2*MAX_NODES; ++i){
                        if (avg_area_marks[i])
                                avg_area_marks[i]->draw (cr);
                        if (avg_circ_marks[i])
                                avg_circ_marks[i]->draw (cr);
                }
                if (selected_bbox) 
                        selected_bbox->draw (cr);
                
                if (profile)
                        profile->NewData (vinfo->sc, this);

                draw_extra (cr);
        }
}


void VObject::set_color_to_active (){ // Active
	for(int i=0; i<np; i++){
                abl[i]->set_fill_rgba (xsmres.HandleActBgColor);
                abl[i]->set_stroke_rgba (
                                         name[0] == '*' ? cairo_basic_color_lookup(&name[8]) : // *Marker:[r]ed,...
                                         name[2] == '-' ? 3 : i%4
                                          );
                abl[i]->queue_update (canvas);
        }
        if (!selected_bbox){
                double x,y;
                double xy[4];
                abl[0]->get_bb_min (xy[0], xy[1]);
                abl[0]->get_bb_max (xy[2], xy[3]);
                for (int i=0; i<np; i++){
                        abl[i]->get_bb_min (x, y);
                        if (xy[0] > x) xy[0] = x;
                        if (xy[1] > y) xy[1] = y;
                        abl[i]->get_bb_max (x, y);
                        if (xy[2] < x) xy[2] = x;
                        if (xy[3] < y) xy[3] = y;
                }
                selected_bbox = new cairo_item_rectangle (xy[0],xy[1], xy[2],xy[3]);
                selected_bbox->set_line_width (1.);
                selected_bbox->set_stroke_rgba (1., 1., 0., 0.3);
                selected_bbox->set_fill_rgba (0., 0., 0., 0.);
                selected_bbox->queue_update (canvas);
        }
        is_selected = true;
}

void VObject::set_color_to_inactive (){ // Inactive
	for(int i=0; i<np; i++){
                abl[i]->set_fill_rgba (xsmres.HandleInActBgColor);
                int ci = name[0] == '*' ? cairo_basic_color_lookup(&name[8]) : // *Marker:[r]ed,...
                        name[2] == '-' ? name[3] == 'P' ? 1 : 4 : 6;  // "green" : "blue" : "grey"
                if (ci != 6)
                        abl[i]->set_stroke_rgba (ci);
                else
                        abl[i]->set_stroke_rgba (xsmres.HandleInActBgColor);
                abl[i]->queue_update (canvas);
        }
        UNREF_DELETE_CAIRO_ITEM (selected_bbox, canvas);
        selected_bbox = NULL;
        is_selected = false;
}

void VObject::set_color_to_hilit (){ // Hilight
	for(int i=0; i<np; i++){
                abl[i]->set_fill_rgba (xsmres.HandleInActBgColor);
                abl[i]->set_stroke_rgba (3); // yellow
                abl[i]->queue_update (canvas);
	}
}

void VObject::set_color_to_custom (gfloat fillcolor[4], gfloat outlinecolor[4]){
	for(int i=0; i<np; i++){
                abl[i]->set_fill_rgba (fillcolor);
                abl[i]->set_stroke_rgba (outlinecolor);
                abl[i]->queue_update (canvas);
        }
}

void VObject::Activate (){
        //        XSM_DEBUG_GM (DBG_L3, "app_vobj.C: VObject::Activate  canvas=0x%x", canvas);
        if (!G_IS_OBJECT (canvas)){
                g_error ("app_vobj.C: VObject::Activate  ERROR, canvas is no G_OBJECT.");
                PI_DEBUG_GP_ERROR (DBG_L1, "app_vobj.C: VObject::Activate  ERROR, canvas is no G_OBJECT.");
                return;
        }
	ViewControl* vc=(ViewControl*)g_object_get_data (G_OBJECT (canvas), "ViewControl"); 
	if (vc) vc->PaintAllRegionsInactive();
        else {
                g_error ("app_vobj.C: VObject::Activate  ERROR, no valid VC set.");
                PI_DEBUG_GP_ERROR (DBG_L1, "app_vobj.C: VObject::Activate  ERROR, no valid ViewControl property set.");
                return;
        }
	set_color_to_active();

        // update properties view for this object
        build_properties_view ();
}

void VObject::label_changed_cb (GtkEditable *e, VObject *vo){
	if (vo->text) g_free (vo->text);
	vo->text = gtk_editable_get_chars (GTK_EDITABLE (e), 0, -1);
        vo->Update ();
}

void VObject::colorchange_callback(GtkColorChooser *colorsel, VObject *vo){
        GdkRGBA *color = (GdkRGBA *) g_object_get_data  (G_OBJECT (colorsel), "COLOR_ELEMENT_DATA");
        if (color){
                gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (colorsel), color);
                vo->Update ();
        }
}

void VObject::fontchange_callback(GtkFontChooser *gfp, VObject *vo){ // gchar **font){
	if (vo->custom_label_font) g_free (vo->custom_label_font);
	vo->custom_label_font = g_strdup (gtk_font_chooser_get_font (GTK_FONT_CHOOSER (gfp)));
        vo->Update ();
}

void VObject::selection_dim_changed_cb (GtkComboBox *cb, VObject *vo){
        switch (gtk_combo_box_get_active (GTK_COMBO_BOX (cb))){
        case 0: vo->set_profile_path_dimension (MEM2D_DIM_LAYER); break;
        case 1: vo->set_profile_path_dimension (MEM2D_DIM_TIME); break;
        default: vo->set_profile_path_dimension (MEM2D_DIM_LAYER); break;
        }
        vo->Update ();
}

void VObject::selection_dimp_changed_cb (GtkComboBox *cb, VObject *vo){
        switch (gtk_combo_box_get_active (GTK_COMBO_BOX (cb))){
        case 0: vo->set_profile_path_dimension (MEM2D_DIM_XY); break;
        case 1: vo->set_profile_path_dimension (MEM2D_DIM_LAYER); break;
        case 2: vo->set_profile_path_dimension (MEM2D_DIM_TIME); break;
        default: vo->set_profile_path_dimension (MEM2D_DIM_XY); break;
        }
        vo->Update ();
}

void VObject::selection_dims_changed_cb (GtkComboBox *cb, VObject *vo){
        switch (gtk_combo_box_get_active (GTK_COMBO_BOX (cb))){
        case 0: vo->set_profile_series_dimension (MEM2D_DIM_XY); break;
        case 1: vo->set_profile_series_dimension (MEM2D_DIM_LAYER); break;
        case 2: vo->set_profile_series_dimension (MEM2D_DIM_TIME); break;
        default: vo->set_profile_series_dimension (MEM2D_DIM_LAYER); break;
        }
        vo->Update ();
}

void VObject::selection_data_changed_cb (GtkComboBox *cb, VObject *vo){
        switch (gtk_combo_box_get_active (GTK_COMBO_BOX (cb))){
        case 0: vo->set_profile_series_all (FALSE); break;
        case 1: vo->set_profile_series_all (TRUE); break;
        default: vo->set_profile_series_all (FALSE); break;
        }
        vo->Update ();
}

void VObject::selection_data_plot_changed_cb (GtkComboBox *cb, VObject *vo){
        switch (gtk_combo_box_get_active (GTK_COMBO_BOX (cb))){
        case 0: vo->set_profile_series_pg2d (FALSE); break;
        case 1: vo->set_profile_series_pg2d (TRUE); break;
        default: vo->set_profile_series_pg2d (FALSE); break;
        }
        vo->Update ();
}

void VObject::selection_grid_plot_changed_cb (GtkComboBox *cb, VObject *vo){
        vo->grid_mode = gtk_combo_box_get_active (GTK_COMBO_BOX (cb));
        vo->Update ();
}

void VObject::selection_info_options_changed_cb (GtkComboBox *cb, VObject *vo){
        vo->info_option  = gtk_combo_box_get_active (GTK_COMBO_BOX (cb));
        vo->Update ();
}

void VObject::build_properties_view (gboolean add){
	GtkWidget *textinput = NULL;
	GtkWidget *dim_sel = NULL;
	GtkWidget *dimp_sel = NULL;
	GtkWidget *dims_sel = NULL;
	GtkWidget *data_sel = NULL;
	GtkWidget *data_plot = NULL;
	GtkWidget *grid_plot = NULL;
        GtkWidget *info_options = NULL;
	GtkWidget *showlabel_checkbutton = NULL;
	GtkWidget *showprofile_checkbutton = NULL;
	GtkWidget *lock_checkbutton = NULL;
	GtkWidget *opt_button = NULL;
	GtkWidget *dump_button = NULL;

#if 0
        // FIX!!!
	++space_time_on[0];
	++space_time_on[1];
	++space_time_off[0];
	++space_time_off[1];
#endif

	if (space_time_from_0)
		space_time_on[0]=space_time_on[1]=-1;
	if (space_time_until_00)
		space_time_off[0]=space_time_off[1]=-1;
        
#if 0
	--space_time_on[0];
	--space_time_on[1];
	--space_time_off[0];
	--space_time_off[1];
#endif

        if (properties_bp){
                properties_bp->update_all_ec ();
        } else {
                properties_bp = new BuildParam;
                properties_bp->set_no_spin (true);
                properties_bp->set_default_ec_change_notice_fkt (VObject::ec_properties_changed, this);

                gchar *objident = g_strdup_printf ("Edit Object \"%s\"", name);
                properties_bp->grid_add_label (objident);
                g_free (objident);
                properties_bp->new_line ();

                if (text){
                        properties_bp->new_grid_with_frame ("Object Label", 10);
                        textinput = properties_bp->grid_add_input (NULL, 2);
                        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (textinput))), text, -1);

                        GtkWidget *fpick = gtk_font_button_new_with_font (custom_label_font);
                        //gnome_font_picker_set_title (GNOME_FONT_PICKER (fpick), "Label Font");
                        g_signal_connect (G_OBJECT (fpick), "font_set",
                                          G_CALLBACK (fontchange_callback), this);
                        properties_bp->grid_add_widget (fpick);

                        GtkWidget *cpick =  gtk_color_button_new_with_rgba (&custom_label_color);
                        gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER(cpick), true);
                        g_object_set_data  (G_OBJECT (cpick), "COLOR_ELEMENT_DATA", &custom_label_color);
                        gtk_color_button_set_title (GTK_COLOR_BUTTON (cpick), "Label Color");
                        g_signal_connect (G_OBJECT (cpick), "color_set",
                                          G_CALLBACK (colorchange_callback), this);
                        properties_bp->grid_add_widget (cpick, 2);

                        showlabel_checkbutton = gtk_check_button_new_with_label( N_("On"));
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (showlabel_checkbutton), object_label ? TRUE:FALSE);
                        properties_bp->grid_add_widget (showlabel_checkbutton, 2);
                        g_signal_connect (G_OBJECT (showlabel_checkbutton), "toggled",
                                          G_CALLBACK (VObject::show_label_cb), this);

                        properties_bp->new_line ();
                
                        if (obj_type_id () != O_POINT) {
                                properties_bp->grid_add_label ("Line/Fill Color:");

                                GtkWidget *cpick =  gtk_color_button_new_with_rgba (&custom_element_color);
                                g_object_set_data  (G_OBJECT (cpick), "COLOR_ELEMENT_DATA", &custom_element_color);
                                gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (cpick), true);
                                gtk_color_button_set_title (GTK_COLOR_BUTTON (cpick), "Element Color");
                                g_signal_connect (G_OBJECT (cpick), "color_set",
                                                  G_CALLBACK (colorchange_callback), this);
                                properties_bp->grid_add_widget (cpick, 2);

                                cpick =  gtk_color_button_new_with_rgba (&custom_element_b_color);
                                g_object_set_data  (G_OBJECT (cpick), "COLOR_ELEMENT_DATA", &custom_element_b_color);
                                gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (cpick), true);
                                gtk_color_button_set_title (GTK_COLOR_BUTTON (cpick), "Element B Color");
                                g_signal_connect (G_OBJECT (cpick), "color_set",
                                                  G_CALLBACK (colorchange_callback), this);
                                properties_bp->grid_add_widget (cpick, 2);
                        }
                        properties_bp->pop_grid ();
                }
                properties_bp->new_line ();

                // Profile enable
                showprofile_checkbutton = gtk_check_button_new_with_label( N_("Show Profile"));
                gtk_check_button_set_active (GTK_CHECK_BUTTON (showprofile_checkbutton), profile ? TRUE:FALSE);
                properties_bp->grid_add_widget (showprofile_checkbutton, 2);
                g_signal_connect (G_OBJECT (showprofile_checkbutton), "toggled",
                                  G_CALLBACK (VObject::show_profile_cb), this);
		
                lock_checkbutton = gtk_check_button_new_with_label( N_("Lock Position"));
                gtk_check_button_set_active (GTK_CHECK_BUTTON (lock_checkbutton), lock);
                properties_bp->grid_add_widget (lock_checkbutton, 2);
                g_signal_connect (G_OBJECT (lock_checkbutton), "toggled",
                                  G_CALLBACK (VObject::lock_position_cb), this);

                properties_bp->new_line ();

                properties_bp->set_label_width_chars (15);
                properties_bp->set_input_width_chars (15);

                gchar *tmp = g_strdup_printf ("Show SpcT[%d:%d]", space_time_now[1], space_time_now[0]);
                properties_bp->grid_add_ec (tmp, Unity, &space_time_on[1], -1, 1000, ".0f");
                properties_bp->grid_add_ec (NULL,  Unity, &space_time_on[0], -1, 1000, ".0f");
                g_free (tmp);
                properties_bp->new_line ();
                
                tmp = g_strdup_printf ("Hide SpcT[%d:%d]", space_time_now[1], space_time_now[0]);
                properties_bp->grid_add_ec (tmp, Unity, &space_time_off[1], -1, 1000, ".0f");
                properties_bp->grid_add_ec (NULL,  Unity, &space_time_off[0], -1, 1000, ".0f");
                g_free (tmp);
                properties_bp->new_line ();

                // marker_scale
                properties_bp->grid_add_ec ("Marker Scale", Unity, &marker_scale, 0., 1000., ".2f");
                properties_bp->new_line ();

                for (int n=0; n<np; ++n){
                        int i=2*n;
                        gchar *tmp = g_strdup_printf ("P%d:", n);
                        properties_bp->grid_add_ec (tmp, Pixel, &xy[i],   -32767, 32767, ".0f");
                        properties_bp->grid_add_ec (NULL,  Pixel, &xy[i+1], -32767, 32767, ".0f");
                        g_free (tmp);
                        properties_bp->new_line ();
                }
                if (obj_type_id () == O_POINT && profile){ // Point Path at XY in Layer, Series in Time or in Time, Series in Layer
                        properties_bp->grid_add_ec ("Path Width", Pixel, &path_width, 0, 1000, ".0f");
                        properties_bp->new_line ();

                        properties_bp->grid_add_ec ("Series Limits", Unity, &series_limits[0], 0, 1000, ".0f");
                        properties_bp->grid_add_ec (NULL,            Unity, &series_limits[1], 0, 1000, ".0f");
                        properties_bp->new_line ();

                        dim_sel = gtk_combo_box_text_new ();
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (dim_sel), "path-layer", "Dimension for Path: Layer (Value)");
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (dim_sel), "path-time", "Dimension for Path: Time");
                        gtk_combo_box_set_active (GTK_COMBO_BOX (dim_sel), get_profile_path_dimension () == MEM2D_DIM_LAYER ? 0:1);
                        properties_bp->grid_add_widget (dim_sel, 10);
                        g_signal_connect (G_OBJECT (dim_sel), "changed",
                                          G_CALLBACK (VObject::selection_dim_changed_cb), this);
                        properties_bp->new_line ();
                               
                        data_sel = gtk_combo_box_text_new ();
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (data_sel), "plot-curview", "Plot data of current view");
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (data_sel), "plot-all", "Plot all data series");
                        gtk_combo_box_set_active (GTK_COMBO_BOX (data_sel), get_profile_series_all () ? 1:0);
                        properties_bp->grid_add_widget (data_sel, 10);
                        g_signal_connect (G_OBJECT (data_sel), "changed",
                                          G_CALLBACK (VObject::selection_data_changed_cb), this);
                        properties_bp->new_line ();

                        data_plot = gtk_combo_box_text_new ();
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (data_plot), "no-2d", "No Grey 2D view");
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (data_plot), "add-2d", "Add Grey 2D view");
                        gtk_combo_box_set_active (GTK_COMBO_BOX (data_plot), plot_g2d? 1:0);
                        properties_bp->grid_add_widget (data_plot, 10);
                        g_signal_connect (G_OBJECT (data_plot), "changed",
                                          G_CALLBACK (VObject::selection_data_plot_changed_cb), this);
                        properties_bp->new_line ();
                }
                if (obj_type_id () == O_LINE && profile){ // Profile Path in XY, series in Layer/Time
                        properties_bp->grid_add_ec ("Path Width", Pixel, &path_width, 0, 1000, ".0f");
                        properties_bp->new_line ();

                        properties_bp->grid_add_ec ("Path Step", Pixel, &path_step, 0, 1000, ".0f");
                        properties_bp->new_line ();

                        properties_bp->grid_add_ec ("Series Limits", Unity, &series_limits[0], 0, 1000, ".0f");
                        properties_bp->grid_add_ec (NULL,            Unity, &series_limits[1], 0, 1000, ".0f");
                        properties_bp->new_line ();

                        dimp_sel = gtk_combo_box_text_new ();
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (dimp_sel), "dim-path-xy", "Dimension for Path: XY");
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (dimp_sel), "dim-path-layer","Dimension for Path: Layer (Value)");
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (dimp_sel), "dim-parth-time","Dimension for Path: Time");
                        int ii = (int)path_dimension - 2; if (ii<0) ii=0;
                        gtk_combo_box_set_active (GTK_COMBO_BOX (dimp_sel), ii);
                        properties_bp->grid_add_widget (dimp_sel, 10);
                        g_signal_connect (G_OBJECT (dimp_sel), "changed",
                                          G_CALLBACK (VObject::selection_dimp_changed_cb), this);
                        properties_bp->new_line ();

                        dims_sel = gtk_combo_box_text_new ();
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (dims_sel), "dim-ser-xy","Dimension for Series: XY");
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (dims_sel), "dim-ser-layer","Dimension for Series: Layer (Value)");
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (dims_sel), "dim-ser-timexy","Dimension for Series: Time");
                        ii= (int)series_dimension - 2; if (ii<0) ii=0;
                        gtk_combo_box_set_active (GTK_COMBO_BOX (dims_sel), ii);
                        properties_bp->grid_add_widget (dims_sel, 10);
                        g_signal_connect (G_OBJECT (dims_sel), "changed",
                                          G_CALLBACK (VObject::selection_dims_changed_cb), this);
                        properties_bp->new_line ();

                        data_sel = gtk_combo_box_text_new ();
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (data_sel), "plot-curview","Plot data of current view");
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (data_sel), "plot-all-sets","Plot all data sets");
                        gtk_combo_box_set_active (GTK_COMBO_BOX (data_sel), series_all? 1:0);
                        properties_bp->grid_add_widget (data_sel, 10);
                        g_signal_connect (G_OBJECT (data_sel), "changed",
                                          G_CALLBACK (VObject::selection_data_changed_cb), this);
                        properties_bp->new_line ();

                        data_plot = gtk_combo_box_text_new ();
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (data_plot), "no-2d", "No Grey 2D view");
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (data_plot), "add-2d", "Add Grey 2D view");
                        gtk_combo_box_set_active (GTK_COMBO_BOX (data_plot), plot_g2d? 1:0);
                        properties_bp->grid_add_widget (data_plot, 10);
                        g_signal_connect (G_OBJECT (data_plot), "changed",
                                          G_CALLBACK (VObject::selection_data_plot_changed_cb), this);
                        properties_bp->new_line ();
                }
                if (obj_type_id () == O_KSYS){ // Koord-system / grid
                        grid_plot = gtk_combo_box_text_new ();
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-free-none", "Grid: free none");       // 0
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-free-lines","Grid: free Lines");      // 1
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-free-circ","Grid: free Circels");    // 2
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-free-line+circ","Grid: free Lines + Circles"); // 3
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-111-none","Grid: (111) none");      // 4
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-111-lines","Grid: (111) Lines");     // 5
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-111-circ","Grid: (111) Circles");   // 6
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-111-line+circ","Grid: (111) Lines + Circles");    // 7
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-100-none","Grid: (100) none");      // 8
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-100-lines","Grid: (100) Lines");     // 9
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-100-circles","Grid: (100) Circles");   // 10
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-100-lines+circ","Grid: (100) Lines + Circles");    // 11
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-111-hexas","Grid: (111) Hexas");    // 12
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-111-hexash","Grid: (111) Hexas-H");    // 13
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-111-coronene","Grid: (111) Coronene");    // 14
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (grid_plot), "grid-111-bkf","Grid: (111) BKF");    // 15
                        gtk_combo_box_set_active (GTK_COMBO_BOX (grid_plot), grid_mode);
                        properties_bp->grid_add_widget (grid_plot, 10);
                        g_signal_connect (G_OBJECT (grid_plot), "changed",
                                          G_CALLBACK (VObject::selection_grid_plot_changed_cb), this);
                        properties_bp->new_line ();

                        properties_bp->grid_add_ec ("Grid Multiples", Unity, &grid_multiples, 1, 1000, ".0f");
                        properties_bp->new_line ();

                        properties_bp->grid_add_ec ("Grid Size", Unity, &grid_size, 1, 1000, ".0f");
                        info_options = gtk_combo_box_text_new ();
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (info_options), "info-none", "Info: none");  // 0
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (info_options), "info-none", "Info: BLen");  // 1
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (info_options), "info-none", "Info: Score/BdF");  // 2
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (info_options), "info-none", "Info: i1:i2");  // 3
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (info_options), "info-none", "Info: -");  // 4
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (info_options), "info-none", "Info: -");  // 5
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (info_options), "info-none", "Info: Score Fref=0");  // 6
                        gtk_combo_box_set_active (GTK_COMBO_BOX (info_options), info_option);
                        properties_bp->grid_add_widget (info_options, 1);
                        g_signal_connect (G_OBJECT (info_options), "changed",
                                          G_CALLBACK (VObject::selection_info_options_changed_cb), this);
                        properties_bp->new_line ();

                        ECP_list = NULL;
                        properties_bp->grid_add_ec ("Grid Aspect", Unity, &grid_aspect, 0., 2., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->new_line ();

                        properties_bp->grid_add_ec ("RXpos, d-opt", Unity, &xy[2], -1000., 1000., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->grid_add_ec (NULL, Unity, &m_dopt_r[0], -10., 10., ".4f");
                        properties_bp->new_line ();
                        properties_bp->grid_add_ec ("RYpos, d-opt", Unity, &xy[3], -1000., 1000., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->grid_add_ec (NULL, Unity, &m_dopt_r[1], -10., 10., ".4f");
                        properties_bp->new_line ();
                        properties_bp->grid_add_ec ("RRot, d-opt", Unity, &m_phi, -180., 180., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->grid_add_ec (NULL, Unity, &m_dopt_r[2], -10., 10., ".4f");
                        properties_bp->new_line ();
                        properties_bp->grid_add_ec ("RWid, d-opt", Unity, &xy[5], -1000., 1000., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->grid_add_ec (NULL, Unity, &m_dopt_r[3], -1., 1., ".4f");
                        properties_bp->new_line ();
                        properties_bp->grid_add_ec ("RLen, d-opt", Unity, &xy[1], -1000., 1000., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->grid_add_ec (NULL, Unity, &m_dopt_r[4], -1., 1., ".4f");
                        properties_bp->new_line ();

                        properties_bp->grid_add_ec ("Parm 1, d-opt", Unity, &m_parameter[0], -100., 100., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->grid_add_ec (NULL, Unity, &m_dopt[0], -100., 100., ".4f");
                        properties_bp->new_line ();
                        properties_bp->grid_add_ec ("Parm 2, d-opt", Unity, &m_parameter[1], -100., 100., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->grid_add_ec (NULL, Unity, &m_dopt[1], -100., 100., ".4f");
                        properties_bp->new_line ();
                        properties_bp->grid_add_ec ("Parm 3, d-opt", Unity, &m_parameter[2], -100., 100., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->grid_add_ec (NULL, Unity, &m_dopt[2], -100., 100., ".4f");
                        properties_bp->new_line ();
                        properties_bp->grid_add_ec ("Parm 4, d-opt", Unity, &m_parameter[3], -100., 100., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->grid_add_ec (NULL, Unity, &m_dopt[3], -100., 100., ".4f");
                        properties_bp->new_line ();
                        properties_bp->grid_add_ec ("Parm 5, d-opt", Unity, &m_parameter[4], -100., 100., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->grid_add_ec (NULL, Unity, &m_dopt[4], -100., 100., ".4f");
                        properties_bp->new_line ();
                        properties_bp->grid_add_ec ("Parm 6, d-opt", Unity, &m_parameter[5], -100., 100., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->grid_add_ec (NULL, Unity, &m_dopt[5], -100., 100., ".4f");
                        properties_bp->new_line ();
                        properties_bp->grid_add_ec ("Parm 7, d-opt", Unity, &m_parameter[6], -100., 100., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->grid_add_ec (NULL, Unity, &m_dopt[6], -100., 100., ".4f");
                        properties_bp->new_line ();
                        properties_bp->grid_add_ec ("Parm 8, d-opt", Unity, &m_parameter[7], -100., 100., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->grid_add_ec (NULL, Unity, &m_dopt[7], -100., 100., ".4f");
                        properties_bp->new_line ();
                        properties_bp->grid_add_ec ("Parm 9, d-opt", Unity, &m_parameter[8], -100., 100., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->grid_add_ec (NULL, Unity, &m_dopt[8], -100., 100., ".4f");
                        properties_bp->new_line ();
                        properties_bp->grid_add_ec ("Parm 10, d-opt", Unity, &m_parameter[9], -100., 100., ".4f");
                        ECP_list = g_slist_prepend( ECP_list, properties_bp->ec);
                        properties_bp->grid_add_ec (NULL, Unity, &m_dopt[9], -100., 100., ".4f");
                        properties_bp->new_line ();

                        properties_bp->grid_add_ec ("#Steps", Unity, &m_n_opt, 0., 1000., ".0f");
                        opt_button = gtk_button_new_with_label( N_("Run Opt"));
                        properties_bp->grid_add_widget (opt_button, 2);
                        g_signal_connect (G_OBJECT (opt_button), "clicked",
                                          G_CALLBACK (VObject::run_m_param_optimization_cb), this);

                        properties_bp->new_line ();
                        dump_button = gtk_button_new_with_label( N_("Dump"));
                        properties_bp->grid_add_widget (dump_button, 2);
                        g_signal_connect (G_OBJECT (dump_button), "clicked",
                                          G_CALLBACK (VObject::dump_param_cb), this);
                }

                if (textinput)
                        g_signal_connect (G_OBJECT (textinput), "changed", G_CALLBACK (label_changed_cb), this);

                g_object_set_data  (G_OBJECT (properties_bp->grid), "VOBJECT", this);

        }

        GtkWidget *current_view = gtk_grid_get_child_at (GTK_GRID (((ViewControl*)g_object_get_data (G_OBJECT (canvas), "ViewControl"))->side_pane_tab_objects), 1,1);
        if (current_view != properties_bp->grid){
                if (current_view){
                        VObject *vcc = (VObject*)g_object_get_data  (G_OBJECT (current_view), "VOBJECT");
                        if (vcc)
                                vcc->destroy_properties_bp ();
                }
                if (add){
                        gtk_grid_attach (GTK_GRID (((ViewControl*)g_object_get_data (G_OBJECT (canvas), "ViewControl"))->side_pane_tab_objects), properties_bp->grid, 1,1, 1,1);
                }
        }
        properties_bp->show_all ();

}

void VObject::properties_callback (GtkDialog *dialog,  int response, gpointer user_data){
	XSM_DEBUG ( DBG_L4, "VObject::properties_callback ... pressed OK");
        VObject *vc = (VObject *) user_data;
        //{} while ((obj_type_id () == O_LINE) && (get_profile_series_dimension () == get_profile_path_dimension ())); 

        // I clean up here, make sure destructor is called.
        g_object_ref (vc->properties_bp->grid); // KEEP ME! this is hooked to side pane when object is active
        gtk_box_remove (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), GTK_WIDGET (vc->properties_bp->grid));
        delete (vc->properties_bp);
	XSM_DEBUG ( DBG_L4, "VObject::properties_callback ... calling destroy dialog.");
        gtk_window_destroy (GTK_WINDOW (dialog));
        
        vc->properties_bp = NULL;

	++vc->space_time_on[0];
	++vc->space_time_on[1];
	++vc->space_time_off[0];
	++vc->space_time_off[1];
        
	if (!vc->space_time_on[0] || !vc->space_time_on[1])
		vc->space_time_from_0=TRUE;
	else
		vc->space_time_from_0=FALSE;

	if (!vc->space_time_off[0] || !vc->space_time_off[1])
		vc->space_time_until_00=TRUE;
	else
		vc->space_time_until_00=FALSE;

	--vc->space_time_on[0];
	--vc->space_time_on[1];
	--vc->space_time_off[0];
	--vc->space_time_off[1];
        
        //**	if (profile)
        //**		profile->NewData (vinfo->sc, this);

	vc->remake_node_markers ();
}

void VObject::properties(){
        GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("Object Properties"), 
                                                         GTK_WINDOW (gtk_widget_get_root (canvas)), 
                                                         (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                         _("_OK"), GTK_RESPONSE_ACCEPT,
                                                         NULL); 
        destroy_properties_bp ();
        build_properties_view (false);
        gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), properties_bp->grid);
                
        gtk_widget_show (dialog);
        g_signal_connect (dialog, "response",
                          G_CALLBACK (VObject::properties_callback),
                          this);
}

void  VObject::destroy_properties_bp (){
        if (properties_bp){ // clean up
                g_object_ref (properties_bp->grid);
                gtk_grid_remove_row (GTK_GRID (((ViewControl*)g_object_get_data (G_OBJECT (canvas), "ViewControl"))->side_pane_tab_objects), 1);
                delete properties_bp;
                properties_bp = NULL;
        }
}


void VObject::remake_node_markers (){
	++space_time_on[0];
	++space_time_on[1];
	++space_time_off[0];
	++space_time_off[1];

	if (space_time_from_0)
		space_time_on[0]=space_time_on[1]=0;
	if (space_time_until_00)
		space_time_off[0]=space_time_off[1]=0;

	// ---

	if (!space_time_on[0] || !space_time_on[1])
		space_time_from_0=TRUE;
	else
		space_time_from_0=FALSE;

	if (!space_time_off[0] || !space_time_off[1])
		space_time_until_00=TRUE;
	else
		space_time_until_00=FALSE;

	--space_time_on[0];
	--space_time_on[1];
	--space_time_off[0];
	--space_time_off[1];

	for(int i=0; i<np; i++){
		node_marker (abl[i], &xy[2*i], i);
                abl[i]->queue_update (canvas);
	}

	Update ();
}

void VObject::set_on_spacetime (gboolean flag, int spacetime[2], int id){
	if (flag)
		space_time_from_0=FALSE;
	else
		space_time_from_0=TRUE;

	space_time_on[0]=spacetime[0];
	space_time_on[1]=spacetime[1];
}
void VObject::set_off_spacetime (gboolean flag, int spacetime[2], int id){
	if (flag)
		space_time_until_00=FALSE;
	else
		space_time_until_00=TRUE;

	space_time_off[0]=spacetime[0];
	space_time_off[1]=spacetime[1];
}
void VObject::set_spacetime (int spacetime[2]){
	space_time_now[0]=spacetime[0];
	space_time_now[1]=spacetime[1];
}
void VObject::get_spacetime (int spacetime[2]){
	spacetime[0] = space_time_now[0];
	spacetime[1] = space_time_now[1];
}
gboolean VObject::is_spacetime (){
	if (((space_time_now[0] >= space_time_on[0] && space_time_now[1] >= space_time_on[1]) 
	     || space_time_from_0)
	    &&
	    ((space_time_now[0] <= space_time_off[0] && space_time_now[1] <= space_time_off[1]) 
	     || space_time_until_00))
		return TRUE;
	else
		return FALSE;

}

void VObject::set_offset(){
	double x,y;
	x = xy[0]*vinfo->GetQfac();
	y = xy[1]*vinfo->GetQfac();

	if (x < 0. || x >= vinfo->sc->mem2d->GetNx())
		return;
	if (y < 0. || y >= vinfo->sc->mem2d->GetNy())
		return;

	vinfo->sc->Pixel2World (R2INT (x), R2INT (y), 
				main_get_gapp ()->xsm->data.s.x0, main_get_gapp ()->xsm->data.s.y0, 
				SCAN_COORD_ABSOLUTE);

	main_get_gapp ()->spm_update_all();
}

void VObject::set_global_ref(){
	double x,y;
	x = xy[0]*vinfo->GetQfac();
	y = xy[1]*vinfo->GetQfac();

	if (x < 0. || x >= vinfo->sc->mem2d->GetNx())
		return;
	if (y < 0. || y >= vinfo->sc->mem2d->GetNy())
		return;

	main_get_gapp ()->glb_ref_point_xylt_index[0]=R2INT (x);
	main_get_gapp ()->glb_ref_point_xylt_index[1]=R2INT (y);
	main_get_gapp ()->glb_ref_point_xylt_index[2]=0;
	main_get_gapp ()->glb_ref_point_xylt_index[3]=0;
	
	vinfo->sc->Pixel2World (R2INT (x), R2INT (y), 
				main_get_gapp ()->glb_ref_point_xylt_world[0], main_get_gapp ()->glb_ref_point_xylt_world[1],
				SCAN_COORD_ABSOLUTE);

	main_get_gapp ()->glb_ref_point_xylt_world[2]=0.;
	// calcpixeltime
	main_get_gapp ()->glb_ref_point_xylt_world[3] = (double)vinfo->sc->data.s.tStart + 
		(2.*vinfo->sc->mem2d->GetNx()*(vinfo->sc->data.s.ydir>0 ? y : vinfo->sc->mem2d->GetNy()-y-1) + x)
		* vinfo->sc->data.s.pixeltime;

	std::cout <<
		"SET GLOBAL REFERENCE POINT TO:\n"
		"PixelTime assumed is: " << vinfo->sc->data.s.pixeltime << " s" << std::endl
		  << "index XYLT = ("
		  << main_get_gapp ()->glb_ref_point_xylt_index[0] << ", "
		  << main_get_gapp ()->glb_ref_point_xylt_index[1] << ", "
		  << main_get_gapp ()->glb_ref_point_xylt_index[2] << "*, "
		  << main_get_gapp ()->glb_ref_point_xylt_index[3] << "*)\n"
		  << "world XYLT = ("
		  << main_get_gapp ()->glb_ref_point_xylt_world[0] << ", "
		  << main_get_gapp ()->glb_ref_point_xylt_world[1] << ", "
		  << main_get_gapp ()->glb_ref_point_xylt_world[2] << "*, "
		  << main_get_gapp ()->glb_ref_point_xylt_world[3] << ")\n"
		  << std::endl;

}

void VObject::SetUpScan(){
	double x0,y0,x1,y1,dx,dy;
	double xyq[4];

	xyq[0] = xy[0]*vinfo->GetQfac();
	xyq[1] = xy[1]*vinfo->GetQfac();
	xyq[2] = xy[2*(np-1)]*vinfo->GetQfac();
	xyq[3] = xy[2*(np-1)+1]*vinfo->GetQfac();

	if(xyq[0]<0. || xyq[0] >= vinfo->sc->mem2d->GetNx() ||
	   xyq[2]<0. || xyq[2] >= vinfo->sc->mem2d->GetNx())
		return;
	if(xyq[1]<0. || xyq[1] >= vinfo->sc->mem2d->GetNy() || 
	   xyq[3]<0. || xyq[3] >= vinfo->sc->mem2d->GetNy())
		return;

	// new offset
	vinfo->sc->Pixel2World (R2INT((xyq[0] + xyq[2])/2.), R2INT((xyq[1] + xyq[3])/2.),
				main_get_gapp ()->xsm->data.s.x0, main_get_gapp ()->xsm->data.s.y0, 
				SCAN_COORD_ABSOLUTE);

	// calc new size
	vinfo->sc->Pixel2World (R2INT(xyq[0]), R2INT(xyq[1]),
				x0, y0,
				SCAN_COORD_RELATIVE);

	vinfo->sc->Pixel2World (R2INT(xyq[2]), R2INT(xyq[3]),
				x1, y1,
				SCAN_COORD_RELATIVE);

	XSM_DEBUG ( DBG_L3, "SetUpScan:" << np << ":" << x0 << "," << y0 << ", " << x1 << "," << y1 );
	dx = x1-x0;
	dy = y1-y0;
	XSM_DEBUG (DBG_L3, "SetUpScan:" << dx << "," << dy );
	main_get_gapp ()->xsm->data.s.rx = sqrt(dx*dx+dy*dy);

	main_get_gapp ()->xsm->data.s.nx = R2INT(Dist(0,np-1));
	main_get_gapp ()->xsm->data.s.ny = 1;
	main_get_gapp ()->xsm->data.s.dx = main_get_gapp ()->xsm->data.s.rx/(main_get_gapp ()->xsm->data.s.nx-1);
	main_get_gapp ()->xsm->data.s.dy = main_get_gapp ()->xsm->data.s.dx;
	main_get_gapp ()->xsm->data.s.ry = main_get_gapp ()->xsm->data.s.dx;
	main_get_gapp ()->xsm->data.s.alpha = -Phi(dx,dy);
	main_get_gapp ()->spm_update_all();
}

void VObject::show_label_cb(GtkWidget *widget, VObject *vo){
        vo->show_label (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
}

void VObject::run_m_param_optimization_cb (GtkWidget *widget, VObject *vo){
        vo->run_m_param_opt ();
}

void VObject::dump_param_cb (GtkWidget *widget, VObject *vo){
        vo->dump_param ();
}


void VObject::show_label(gboolean flg){
	if (flg){
		if (!object_label)
			object_label = new cairo_item_text ();

                if (label_osd_style)
                        object_label->set_text (xy[0] + label_offset_xy[0]*vinfo->sc->mem2d->GetNx()/2+vinfo->sc->mem2d->GetNx()/2,
                                                xy[1] + label_offset_xy[1]*vinfo->sc->mem2d->GetNy()/2+vinfo->sc->mem2d->GetNy()/2,
                                                text);
                else
                        object_label->set_text (xy[0] + label_offset_xy[0]*get_marker_scale (), 
                                                xy[1] + label_offset_xy[1]*get_marker_scale (),
                                                text);
                
                object_label->set_anchor (label_anchor);
                object_label->set_pango_font (custom_label_font);
                object_label->set_stroke_rgba (&custom_label_color);

#if 0
                XSM_DEBUG_GM (DBG_L3, "VObject show label {%s} F:%s Crgba:%g %g %g %g SPC:%s XY(%g,%g)",
                           text,custom_label_font,
                           custom_label_color.red,custom_label_color.green,custom_label_color.blue,custom_label_color.alpha,
                           is_spacetime ()?"on":"off",
                           xy[0] + label_offset_xy[0]*get_marker_scale (),
                           xy[1] + label_offset_xy[1]*get_marker_scale ()
                           );
#endif
                
                if (is_spacetime ())
                        object_label->show ();
                else
                        object_label->hide ();
                
                object_label->queue_update (canvas);
	} else if (object_label){
                object_label->hide ();
                object_label->queue_update (canvas);
                delete object_label;
                object_label = NULL;
        }

}

void VObject::lock_position_cb (GtkWidget *widget, VObject *vo){
        vo->lock_object (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
}

void VObject::GoLocMax(int r){
	for(int n=0; n<np; ++n){
		int i=2*n;
		//  XSM_DEBUG(DBG_L2,< "VObject::GoLocMax" );
		if(xy[i] > r && xy[i] < (vinfo->sc->mem2d->GetNx()-r) &&
		   xy[i+1] > r && xy[i+1] < (vinfo->sc->mem2d->GetNy()-r)){
			int lmx, lmy;
			double val=vinfo->sc->mem2d->GetDataPkt(lmx=R2INT(xy[i]), 
								lmy=R2INT(xy[i+1]));
			//    XSM_DEBUG(DBG_L2,< "VObject::GoLocMax: OK start is " << lmx << ", " << lmy << " r: " << r << " z=" << val );
			for(int x=R2INT(xy[i])-r; x<(R2INT(xy[i])+r); ++x)
				for(int y=R2INT(xy[i+1])-r; y<(R2INT(xy[i+1])+r); ++y){
					double z=vinfo->sc->mem2d->GetDataPkt(x,y);
					if(z > val) lmx=x, lmy=y, val=z;
				}
			//    XSM_DEBUG(DBG_L2,< "VObject::GoLocMax: Max in r at " << lmx << ", " << lmy << " z=" << val );
			xy[i]=lmx, xy[i+1]=lmy;
			node_marker (abl[i], &xy[i], i);
                        abl[i]->queue_update (canvas);
			Update();
		}
		break;
	}
}


// Check Events Object Callback fro Dragging and Context Popup Menu Handling
gboolean VObject::check_event(VObjectEvent *event, double mxy[2]){
	static double x, y;
	double new_x, new_y;
        //	GdkCursor *cursor;
	static int dragging;
	double item_x=0., item_y=0.;
        cairo_item* item = NULL;
	
	item_x = mxy[0];
	item_y = mxy[1];

        if (touched_item && dragging)
                item = touched_item;
        else {
                // check items
                for(int i=0; i<np+MAX_NODES; i++){
                        if (abl[i])
                                if (abl[i]->check_grab_bbox (mxy[0], mxy[1])){
                                        item = abl[i];
                                        break;
                                }
                        if (arrow_head[i])
                                if (arrow_head[i]->check_grab_bbox (mxy[0], mxy[1])){
                                        item = arrow_head[i];
                                        break;
                                }
                }
                if (object_label)
                        if (object_label->check_grab_bbox (mxy[0], mxy[1]))
                                item = object_label;
        }

        XSM_DEBUG_GM (DBG_L3, "CHECK_EVENT_VOBJ, ITEM %s at %g %g", item?"found":"not found",x,y);
        if (!item)
                return false;
        
        XSM_DEBUG_GM (DBG_L3, "CHECK_EVENT_VOBJ TYPE");
	switch (event->type){
        case VOBJ_EV_BUTTON_PRESS:
                XSM_DEBUG_GM (DBG_L3, "CHECK_EVENT_VOBJ, BTN PRESSED");
                switch(event->button){
                case VOBJ_EV_BUTTON_1:
                        x = item_x;
                        y = item_y;

                        XSM_DEBUG_GM (DBG_L3, "CHECK_EVENT_VOBJ, BTN1 PRESSED on ITEM at %g %g", x,y);
   
                        touched_item  = item;
                        touched_xy[0] = item_x;
                        touched_xy[1] = item_y;
                                        
                        // cursor = gdk_cursor_new(GDK_FLEUR);
                        // gdk_cursor_destroy(cursor);

                        item->grab ();
                        dragging_active = dragging = true;
                        Update();
                        break;

                case VOBJ_EV_BUTTON_2:
                case VOBJ_EV_BUTTON_3:
                        touched_item  = item;
                        touched_xy[0] = item_x;
                        touched_xy[1] = item_y;
                        g_object_set_data (G_OBJECT (canvas), "VObject", this);
                        // FIX-ME-GTK4 x,y!!
                        gtk_popover_set_pointing_to (GTK_POPOVER (obj_popup_menu), &(GdkRectangle){ event->x, event->y, 1, 1});
                        gtk_popover_popup (GTK_POPOVER (obj_popup_menu));
                        break;

                default:
                        break;
                }
                break;
		
        case VOBJ_EV_MOTION_NOTIFY:
                XSM_DEBUG_GM (DBG_L3, "CHECK_EVENT_VOBJ, MOTION");
                if (!lock && item->is_grabbed ()){ //FIX-ME ... && dragging && (event->motion.state & VOBJ_BUTTON1_MASK)){
                        new_x = item_x;
                        new_y = item_y;

                        gboolean abl_grab=false;
                        gboolean arrow_grab=false;
                        for (int node=0; node < MAX_NODES && (abl[node] || arrow_head[node]); ++node){
                                if (item == abl[np+node]) { abl_grab=true; break; }
                                if (item == arrow_head[node]) { arrow_grab=true; break; }
                        }
                                
                        if (arrow_grab || abl_grab || item == object_label){
                                for(int i=0; i<np; i++){
                                        xy[2*i]+=new_x-x; // Move Object
                                        xy[2*i+1]+=new_y-y;
                                        node_marker (abl[i], &xy[2*i], i);
                                        abl[i]->queue_update (canvas);
                                }
                        }else{
                                // Move one Handle
                                for(int i=0; i<np; i++)
                                        if(item == abl[i]){ // Move Object Element
                                                xy[2*i]   += new_x-x;
                                                xy[2*i+1] += new_y-y;
                                                node_marker (item, &xy[2*i], i);
                                                item->queue_update (canvas);
                                        }
                        }
                                
                        Update();
                        x = new_x;
                        y = new_y;
                }
                break;
		
        case VOBJ_EV_BUTTON_RELEASE:
                if (item->is_grabbed () && dragging)
                        item->ungrab ();

                dragging_active = dragging = false;
                return false;

#if 0
        case VOBJ_EV_ENTER_NOTIFY:
                set_color_to_hilit ();
                break;
        case VOBJ_EV_LEAVE_NOTIFY:
                set_color_to_inactive ();
                break;
#endif

        default:
                break;
        }
        
	return true;
}


VObPoint::VObPoint(GtkWidget *canvas, double *xy0, int pflg, VOBJ_COORD_MODE cmode, const gchar *lab, double Marker_scale)
	: VObject(canvas, xy0, 1, pflg, cmode, lab, Marker_scale){
	if(name) g_free(name);
	name = g_strdup("Point");
	obj_type_id (O_POINT);
	follow = FALSE;
	set_profile_path_step (0.);
}

void VObPoint::Update(){
	gchar *s1;
	gchar *mld = g_strconcat("Point: ",
				 s1=vinfo->makeXYZinfo(xy[0],xy[1]),
				 NULL);
	g_free(s1);

        update_label ();
        
        gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), statusid);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, mld);
	//vinfo->sc->PktVal=1;

	if (get_profile_path_width () > 1){
		double r = get_profile_path_width ()/2.;
		if (!abl[1]){
                        abl[1] = new cairo_item_circle (xy[0], xy[1], r);
                        abl[1]->set_stroke_rgba (0); // red
                        abl[1]->set_fill_rgba (0., 0., 0., 0.33);
                        abl[1]->set_line_width (OBJECT_LINE_WIDTH);
		} else {
			abl[1]->set_xy (0, xy[0], xy[1]);
			((cairo_item_circle*)abl[1])->set_radius (r);
		}
                abl[1]->show ();
		if (is_spacetime ())
			show ();
		else 
                        hide ();
	} else
		if (abl[1])
			abl[1]->hide ();

        if (abl[1])
                abl[1]->queue_update (canvas);

#if 0
        if (profile){
                profile->show();
                profile->NewData(vinfo->sc, this);
        }
#endif
        //	if (follow)
        //		update_scanposition ();

        if (follow)
                if (((ViewControl*)g_object_get_data (G_OBJECT (canvas), "ViewControl"))->tip_follow_mode ())
                        update_scanposition ();

	Activate ();
	g_free(mld);
}

VObPoint::~VObPoint(){}

// draw extra "tip" marker symbol if tip dragging is active
void VObPoint::draw_extra(cairo_t *cr){
        if (follow)
                if (dragging_active && ((ViewControl*)g_object_get_data (G_OBJECT (canvas), "ViewControl"))->tip_follow_mode ()){
                        cairo_save (cr);
                        cairo_translate (cr, xy[0], xy[1]-2*14.);
                        cairo_scale (cr, 2.,2.);

                        cairo_move_to (cr, 0, 0);
                        cairo_rel_curve_to (cr,
                                            -2.76142,0,
                                            -5,2.23858,
                                            -5,5);
                        cairo_rel_curve_to (cr,
                                            0,0.17259,
                                            0.0142,0.33191,
                                            0.0312,0.5);
                        cairo_rel_curve_to (cr,
                                            0.0137,0.16725,
                                            0.0358,0.33617,
                                            0.0625,0.5);
                        cairo_rel_curve_to (cr,
                                            0.57248,3.51444,
                                            2.9063,6.00336,
                                            4.9063,8.00336);
                        cairo_rel_curve_to (cr,
                                            2,-2,
                                            4.33372,-4.48892,
                                            4.9062,-8.00336);
                        cairo_rel_curve_to (cr,
                                            0.0267,-0.16383,
                                            0.0488,-0.33275,
                                            0.0625,-0.5);
                        cairo_rel_curve_to (cr,
                                            0.0171,-0.16809,
                                            0.0312,-0.32741,
                                            0.0312,-0.5);
                        cairo_rel_curve_to (cr,
                                            0,-2.76142,
                                            -2.23858,-5,
                                            -5,-5);
                        cairo_close_path (cr);
                        cairo_move_to (cr, 0.,3.);
                        cairo_rel_curve_to (cr,
                                            1.10457,0,
                                            2,0.89543,
                                            2,2);
                        cairo_rel_curve_to (cr,
                                            0,1.10457,
                                            -0.89543,2,
                                            -2,2);
                        cairo_rel_curve_to (cr,
                                            -1.10457,0,
                                            -2,-0.89543,
                                            -2,-2);
                        cairo_rel_curve_to (cr,
                                            0,-1.10457,
                                            0.89543,-2,
                                            2,-2); 
                        cairo_close_path (cr);
                        cairo_set_source_rgba (cr, 0.8, 0., 0., 0.5);
                        cairo_fill (cr);
                        cairo_stroke (cr);

                        cairo_restore (cr);
                }
/*
<path
       style="color:#000000;clip-rule:nonzero;display:inline;overflow:visible;visibility:visible;opacity:1;isolation:auto;mix-blend-mode:normal;color-interpolation:sRGB;color-interpolation-filters:linearRGB;solid-color:#000000;solid-opacity:1;fill:#bebebe;fill-opacity:1;fill-rule:nonzero;stroke:none;stroke-width:1;stroke-linecap:butt;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1;marker:none;color-rendering:auto;image-rendering:auto;shape-rendering:auto;text-rendering:auto;enable-background:new"
       id="path5874"
       d="m 169.0003,806.99664 c -2.76142,0 -5,2.23858 -5,5 0,0.17259 0.0142,0.33191 0.0312,0.5 0.0137,0.16725 0.0358,0.33617 0.0625,0.5 0.57248,3.51444 2.9063,6.00336 4.9063,8.00336 2,-2 4.33372,-4.48892 4.9062,-8.00336 0.0267,-0.16383 0.0488,-0.33275 0.0625,-0.5 0.0171,-0.16809 0.0312,-0.32741 0.0312,-0.5 0,-2.76142 -2.23858,-5 -5,-5 z m 0,3 c 1.10457,0 2,0.89543 2,2 0,1.10457 -0.89543,2 -2,2 -1.10457,0 -2,-0.89543 -2,-2 0,-1.10457 0.89543,-2 2,-2 z" />
*/
}


void VObPoint::follow_on(){
	follow = TRUE;
}

void VObPoint::follow_off(){
	follow = FALSE;
}

void VObPoint::update_offset(){
	double x,y;
	x = (double)xy[0]*vinfo->GetQfac ();
	y = (double)xy[1]*vinfo->GetQfac ();

	if (x < 0. || x >= vinfo->sc->mem2d->GetNx ())
		return;
	if (y < 0. || y >= vinfo->sc->mem2d->GetNy ())
		return;

	vinfo->sc->Pixel2World (R2INT (x), R2INT (y), 
				main_get_gapp ()->xsm->data.s.x0, main_get_gapp ()->xsm->data.s.y0, 
				SCAN_COORD_ABSOLUTE);

	main_get_gapp ()->xsm->hardware->SetOffset
		(R2INT (main_get_gapp ()->xsm->Inst->X0A2Dig (main_get_gapp ()->xsm->data.s.x0)),
		 R2INT (main_get_gapp ()->xsm->Inst->Y0A2Dig (main_get_gapp ()->xsm->data.s.y0)));

	main_get_gapp ()->spm_update_all ();
}

void VObPoint::update_scanposition(){
	double x,y;
	x = (double)xy[0]*vinfo->GetQfac ();
	y = (double)xy[1]*vinfo->GetQfac ();

	if (x < 0. || x >= vinfo->sc->mem2d->GetNx ())
		return;
	if (y < 0. || y >= vinfo->sc->mem2d->GetNy ())
		return;

	vinfo->sc->Pixel2World (R2INT (x), R2INT (y), 
				main_get_gapp ()->xsm->data.s.sx, main_get_gapp ()->xsm->data.s.sy, 
				SCAN_COORD_RELATIVE);

	while (main_get_gapp ()->xsm->hardware->MovetoXY
               (R2INT(main_get_gapp ()->xsm->Inst->XA2Dig(main_get_gapp ()->xsm->data.s.sx)),
                R2INT(main_get_gapp ()->xsm->Inst->YA2Dig(main_get_gapp ()->xsm->data.s.sy)))
               ); // G-IDLE ME

	main_get_gapp ()->spm_update_all ();
}


VObLine::VObLine(GtkWidget *canvas, double *xy0, int pflg, VOBJ_COORD_MODE cmode, const gchar *lab, double Marker_scale) 
	: VObject(canvas, xy0, 2, pflg, cmode, lab, Marker_scale){
	if(name) g_free(name);
	name = g_strdup("Line");
	obj_type_id (O_LINE);

	posA=posB=0.;

	XSM_DEBUG (DBG_L3,  "VObLine::VObLine - adding line" );

	Update();
}

VObLine::~VObLine(){
	XSM_DEBUG(DBG_L2, "VObLine::~VObLine");
}

void VObLine::AddNode(){
        if (np < MAX_NODES){
		insert_node ();
		Update ();
	}
}

void VObLine::Update(){
        gchar *mld = g_strdup ("Line:");

	for (int segment=0; segment < np-1; ++segment){
		gchar *s1, *s2, *s3;
		gchar *phitxt = g_strdup_printf("phi=%g" UTF8_DEGREE, Phi(segment, segment+1));
		gchar *mld_s = g_strconcat(mld, " |",
					   s1=vinfo->makeXYinfo(xy[2*segment],xy[2*segment+1]),
					   ": ",
					   s2=vinfo->makedXdYinfo(xy+segment*2,xy+segment*2+2),
					   "|=",
					   s3=vinfo->makeDXYinfo(xy+segment*2,xy+segment*2+2), ", ", phitxt,
					   NULL);
		g_free (mld);
		mld = mld_s;
		g_free(s3); g_free(s2); g_free(s1);
		g_free(phitxt);

		double phi;
		double l  = 12. * get_marker_scale ();
		double M[4];

		if (!abl[np+segment])
                        abl[np+segment] = new cairo_item_path (2);
                        
		phi = M_PI*(Phi (segment, segment+1)+45)/180.;
		M[0] = M[3] = cos(phi);
		M[2] = -(M[1] = sin(phi));

		// Line
		abl[np+segment]->set_xy (0, xy[segment*2+0], xy[segment*2+1]); 
		abl[np+segment]->set_xy (1, xy[segment*2+2]-(l*M[0] + l*M[1]), xy[segment*2+3]+(l*M[2] + l*M[3]));
                abl[np+segment]->set_stroke_rgba (&custom_element_color);
                abl[np+segment]->set_line_width (OBJECT_LINE_WIDTH);

                abl[np+segment]->queue_update (canvas);

		if(profile){
			profile->register_cursor_update_func (NULL);
                        //** profile->show();
			//** profile->NewData (vinfo->sc, this, segment>0?TRUE:FALSE);
			profile->register_cursor_update_func (this);
		}

		// show Width / Area / Cursor Markers?
		if(profile){

			if (get_profile_path_width () > 1){
				double r = get_profile_path_width ()/2. / vinfo->GetQfac();

				if (get_profile_path_step () >= 1.){
					double s = get_profile_path_step ()/2. / vinfo->GetQfac();
					double dx=xy[segment*2+2]-xy[segment*2+0];
					double dy=xy[segment*2+3]-xy[segment*2+1];
					double len=sqrt(dx*dx+dy*dy);
					dx /= len; dy /= len;
					
					if (avg_circ_marks[segment*2]){
						UNREF_DELETE_CAIRO_ITEM (avg_circ_marks[segment*2], canvas);
						UNREF_DELETE_CAIRO_ITEM (avg_circ_marks[segment*2+1], canvas);
					}

                                        if (avg_area_marks[segment*2] == NULL){
                                                avg_area_marks[segment*2]   = new cairo_item_path (5);
                                                avg_area_marks[segment*2+1] = new cairo_item_path (5);
                                        }
                                        avg_area_marks[segment*2]->set_stroke_rgba (CAIRO_COLOR_RED);
                                        avg_area_marks[segment*2]->set_line_width (OBJECT_LINE_WIDTH);
                                        avg_area_marks[segment*2]->set_xy (0, xy[segment*2+0]+dx*(-s)-dy*r, xy[segment*2+1]+dy*(-s)+dx*r);
					avg_area_marks[segment*2]->set_xy (1, xy[segment*2+0]+dx*(-s)+dy*r, xy[segment*2+1]+dy*(-s)-dx*r);

					avg_area_marks[segment*2]->set_xy (2, xy[segment*2+2]+dx*(s)+dy*r, xy[segment*2+3]+dy*(s)-dx*r);
					avg_area_marks[segment*2]->set_xy (3, xy[segment*2+2]+dx*(s)-dy*r, xy[segment*2+3]+dy*(s)+dx*r);

					avg_area_marks[segment*2]->set_xy (4, 0); 
                                        avg_area_marks[segment*2]->queue_update (canvas);
                                     
                                        avg_area_marks[segment*2+1]->set_stroke_rgba (CAIRO_COLOR_YELLOW);
                                        avg_area_marks[segment*2+1]->set_line_width (OBJECT_LINE_WIDTH);
                                        avg_area_marks[segment*2+1]->set_xy (0, xy[segment*2+0]+dx*(s)-dy*r, xy[segment*2+1]+dy*(s)+dx*r);
                                        avg_area_marks[segment*2+1]->set_xy (1, xy[segment*2+0]+dx*(s)+dy*r, xy[segment*2+1]+dy*(s)-dx*r);

					avg_area_marks[segment*2+1]->set_xy (2, xy[segment*2+2]+dx*(-s)+dy*r, xy[segment*2+3]+dy*(-s)-dx*r);
					avg_area_marks[segment*2+1]->set_xy (3, xy[segment*2+2]+dx*(-s)-dy*r, xy[segment*2+3]+dy*(-s)+dx*r);

					avg_area_marks[segment*2+1]->set_xy (4, 0);
                                        avg_area_marks[segment*2+1]->queue_update (canvas);

				} else {
					if (avg_area_marks[segment*2]){
						UNREF_DELETE_CAIRO_ITEM (avg_area_marks[segment*2], canvas);
						UNREF_DELETE_CAIRO_ITEM (avg_area_marks[segment*2+1], canvas);
					}
					if (avg_circ_marks[segment*2] == NULL){
                                                avg_circ_marks[segment*2] = new cairo_item_circle (xy[segment*2+0], xy[segment*2+1], r);
                                                avg_circ_marks[segment*2]->set_stroke_rgba (CAIRO_COLOR_RED);
                                                avg_circ_marks[segment*2]->set_fill_rgba (0.,0.,0., 0.25);
                                                avg_circ_marks[segment*2]->set_line_width (OBJECT_LINE_WIDTH);

                                                avg_circ_marks[segment*2+1] = new cairo_item_circle (xy[segment*2+2], xy[segment*2+3], r);
                                                avg_circ_marks[segment*2+1]->set_stroke_rgba (CAIRO_COLOR_RED);
                                                avg_circ_marks[segment*2+1]->set_fill_rgba (0.,0.,0., 0.25);
                                                avg_circ_marks[segment*2+1]->set_line_width (OBJECT_LINE_WIDTH);                                                
                                                
						// g_signal_connect(G_OBJECT(avg_circ_marks[segment*2+1]), "event",
						//		   (GCallback) VObject::item_event,
						//		   this);
					} else {
						UNREF_DELETE_CAIRO_ITEM (avg_circ_marks[segment*2], canvas);
						UNREF_DELETE_CAIRO_ITEM (avg_circ_marks[segment*2+1], canvas);
					}
				}
			} else {
                                UNREF_DELETE_CAIRO_ITEM (avg_area_marks[segment*2], canvas);
                                UNREF_DELETE_CAIRO_ITEM (avg_area_marks[segment*2+1], canvas);
                                UNREF_DELETE_CAIRO_ITEM (avg_circ_marks[segment*2], canvas);
                                UNREF_DELETE_CAIRO_ITEM (avg_circ_marks[segment*2+1], canvas);
			}

			posA = profile->getCursorPos(0);
		
			if (posA >= 0.){
				double dx=xy[2]-xy[0];
				double dy=xy[3]-xy[1];
				double len=sqrt(dx*dx+dy*dy);
				int    seg=0;

				posA = vinfo->AngstroemXRel2W (posA);

				while (posA > len && seg < np-1){
					++seg;
					posA -= len;
					dx=xy[2*seg+2]-xy[2*seg];
					dy=xy[2*seg+3]-xy[2*seg+1];
					len=sqrt(dx*dx+dy*dy);
				}

				dx /= len; dy /= len;

                                if (cursors[0] == NULL){
                                        cursors[0]   = new cairo_item_path (2);
                                }
                                cursors[0]->set_stroke_rgba (CAIRO_COLOR_BLUE);
                                cursors[0]->set_line_width (OBJECT_LINE_WIDTH);
                                cursors[0]->set_xy (0, xy[2*seg+0] + dx*posA - dy*l, xy[2*seg+1] + dy*posA + dx*l);
                                cursors[0]->set_xy (1, xy[2*seg+0] + dx*posA + dy*l, xy[2*seg+1] + dy*posA - dx*l);
			} else
				if (cursors[0])
					UNREF_DELETE_CAIRO_ITEM (cursors[0], canvas);

			posB = profile->getCursorPos(1);
		
			if (posB >= 0.){
				double dx=xy[2]-xy[0];
				double dy=xy[3]-xy[1];
				double len=sqrt(dx*dx+dy*dy);
				int    seg=0;

				posB = vinfo->AngstroemXRel2W (posB);

				while (posB > len && seg < np-1){
					++seg;
					posB -= len;
					dx=xy[2*seg+2]-xy[2*seg];
					dy=xy[2*seg+3]-xy[2*seg+1];
					len=sqrt(dx*dx+dy*dy);
				}

				dx /= len; dy /= len;

                                if (cursors[1] == NULL){
                                        cursors[1]   = new cairo_item_path (2);
                                }
                                cursors[1]->set_stroke_rgba (CAIRO_COLOR_GREEN);
                                cursors[1]->set_line_width (OBJECT_LINE_WIDTH);
                                cursors[1]->set_xy (0, xy[2*seg+0] + dx*posB - dy*l, xy[2*seg+1] + dy*posB + dx*l);
                                cursors[1]->set_xy (1, xy[2*seg+0] + dx*posB + dy*l, xy[2*seg+1] + dy*posB - dx*l);

			} else
				if (cursors[1])
                                        UNREF_DELETE_CAIRO_ITEM (cursors[1], canvas);
		}

                if (!arrow_head[segment])
                        arrow_head[segment] = new cairo_item_path_closed (3);

                arrow_head[segment]->set_position (xy[segment*2+2], xy[segment*2+3]);
		phi = M_PI*(Phi(segment, segment+1)+30.)/180.;
		M[0] = M[3] = cos(phi);
		M[2] = -(M[1] = sin(phi));
		arrow_head[segment]->set_xy (0, -(l*M[0] + l*M[1]), (l*M[2] + l*M[3]));
                arrow_head[segment]->queue_update (canvas);

		phi = M_PI*(Phi(segment, segment+1)+60.)/180.;
		M[0] = M[3] = cos(phi);
		M[2] = -(M[1] = sin(phi));
                arrow_head[segment]->set_xy (1, -(l*M[0] + l*M[1]), (l*M[2] + l*M[3]));

		arrow_head[segment]->set_xy (2, 0., 0.);
                arrow_head[segment]->set_stroke_rgba (&custom_element_color);
                arrow_head[segment]->set_fill_rgba (0., 0., 0., 0.33);
                arrow_head[segment]->set_line_width (OBJECT_LINE_WIDTH);
                arrow_head[segment]->queue_update (canvas);

		if (is_spacetime ())
                        show ();
                else
                        hide ();
	}
  
        update_label ();

        gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), statusid);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, mld);
	//vinfo->sc->PktVal=2;

	g_free(mld);
	Activate ();
}

VObPolyLine::VObPolyLine(GtkWidget *canvas, double *xy0, int pflg, VOBJ_COORD_MODE cmode, const gchar *lab, double Marker_scale) 
	: VObject(canvas, &xy0[1], (int) xy0[0], pflg, cmode, lab, Marker_scale){
	if(name) g_free(name);
	name = g_strdup("PolyLine");
	obj_type_id (O_POLYLINE);
	XSM_DEBUG (DBG_L3,  "VObPolyLine::VObPolyLine - adding poly line, np=" << np );

        abl[np] = new cairo_item_path (np);
	for( int i=0; i<np; ++i)
		abl[np]->set_xy (i, xy[2*i], xy[1+2*i]);
        abl[np]->set_stroke_rgba (4); // blue
        abl[np]->set_line_width (OBJECT_LINE_WIDTH);
        abl[np]->queue_update (canvas);

	Update();
}

VObPolyLine::~VObPolyLine(){
	XSM_DEBUG(DBG_L2, "VObPolyLine::~VObPolyLine");
}

void VObPolyLine::AddNode(){
 	XSM_DEBUG(DBG_L2, "Add Node, Item: " << touched_item 
		  << " XY: " <<  touched_xy[0] << "," << touched_xy[1] );
	XSM_DEBUG(DBG_L2, "-- not jet implemented, have to change xy and abl to G_SLIST --" );
}

void VObPolyLine::DelNode(){
 	XSM_DEBUG(DBG_L2, "Del Node, Item: " << touched_item 
		  << " XY: " <<  touched_xy[0] << "," << touched_xy[1] );
	XSM_DEBUG(DBG_L2, "-- not jet implemented, have to change xy and abl to G_SLIST --" );
}

void VObPolyLine::Update(){
	gchar *s1, *s2, *s3;
	gchar *mld = g_strconcat("PolyLine: |",
				 s1=vinfo->makeXYinfo(xy[0],xy[1]),
				 ":..: ",
				 s2=vinfo->makedXdYinfo(xy,xy+2*(np-1)),
				 "|=",
				 s3=vinfo->makeDnXYinfo(xy,np),
				 NULL);
	g_free(s3); g_free(s2); g_free(s1);

	for( int i=0; i<np; ++i){
		g_free (vinfo->makeXYinfo (xy[2*i],xy[1+2*i]));
		abl[np]->set_xy (i, xy[2*i], xy[1+2*i]);
	}

        update_label ();

        gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), statusid);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, mld);
	//vinfo->sc->PktVal=np;
	g_free(mld);

	Activate ();
}

VObTrace::VObTrace(GtkWidget *canvas, double *xy0, int pflg, VOBJ_COORD_MODE cmode, const gchar *lab, double Marker_scale) 
	: VObject(canvas, &xy0[1], 1, pflg, cmode, lab, Marker_scale){
	if(name) g_free(name);
	name = g_strdup("Trace");
	obj_type_id (O_TRACE);
	XSM_DEBUG (DBG_L3,  "VObTrace::VObTrace - adding poly line, np=" << np );

	trlen = (int)xy0[0];
        abl[np] = new cairo_item_path (trlen);
	for( int i=0; i<trlen; ++i){
                double xy[2];
                xy[0] = xy0[1+2*i], xy[1] = xy0[1+2*i+1];
		vinfo->Angstroem2W (xy[0], xy[1]);
		abl[np]->set_xy (i, xy[0], xy[1]);
	}
	abl[np]->set_stroke_rgba (4); // blue
        abl[np]->set_line_width (OBJECT_LINE_WIDTH);
        abl[np]->queue_update (canvas);

	Update();
}

VObTrace::~VObTrace(){
	XSM_DEBUG(DBG_L2, "VObTrace::~VObTrace");
}

void VObTrace::Change(double *xy0){
	trlen = (int)xy0[0];
	for( int i=0; i<trlen; ++i){
                double x=xy0[1+2*i];
                double y=xy0[1+2*i+1];
		vinfo->Angstroem2W (x, y);
		abl[np]->set_xy (i, x, y);
	}
}

void VObTrace::Update(){
	gchar *s1;
	gchar *mld = g_strconcat("Tracehead: ",
				 s1=vinfo->makeXYinfo(xy[0],xy[1]),
				 NULL);
	g_free(s1);

	for( int i=0; i<np; ++i)
               	abl[np]->set_xy (i, xy[2*i], xy[1+2*i]);

        update_label ();

        gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), statusid);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, mld);
	//vinfo->sc->PktVal=np;
	g_free(mld);

	Activate ();
}



VObKsys::VObKsys(GtkWidget *_canvas, double *xy0, int pflg, VOBJ_COORD_MODE cmode, const gchar *lab, double Marker_scale) 
	: VObject(_canvas, xy0, 3, pflg, cmode, lab, Marker_scale){

	XSM_DEBUG (DBG_L3,  "VObKsys::VObKsys - adding Ksys" );
	name = g_strdup("Ksys");
	obj_type_id (O_KSYS);

	abl[3] = new cairo_item_path (3);
	atoms = NULL;
	lines = NULL;
	bonds = NULL;
	info  = NULL;
	n_atoms = 0;
	n_lines = 0;
	n_bonds = 0;
	n_info  = 0;
	grid_mode = 6;

	custom_element_b_color.red=200./255.;
	custom_element_b_color.green=255./255.;
	custom_element_b_color.blue=100./255.;
	custom_element_b_color.alpha=100./255.;

	abl[3]->set_xy (0, xy[0], xy[1]);	/* end of first vector, initally looking to the right */ 
        abl[3]->set_xy (1, xy[2], xy[3]);	/* origin */
        abl[3]->set_xy (2, xy[4], xy[5]);	/* end of second vector, initally looking down */ 
        abl[3]->set_stroke_rgba (4); // blue
        abl[3]->set_line_width (OBJECT_LINE_WIDTH);
        abl[3]->queue_update (canvas);

	abl[4] = NULL;

        canvas = _canvas;
	Update();
}

VObKsys::~VObKsys(){
	destroy_atoms ();
	XSM_DEBUG(DBG_L2, "VObKsys::~VObKsys");
}



void VObKsys::destroy_atoms(){
        if (atoms){
                atoms->hide ();
                atoms->queue_update (canvas);
                delete atoms;
                n_atoms = 0;
                atoms = NULL;
        }

        if (lines){
                lines->hide ();
                lines->queue_update (canvas);
                delete lines;
                n_lines = 0;
                lines = NULL;
        }

        if (bonds){
                bonds->hide ();
                bonds->queue_update (canvas);
                delete bonds;
                n_bonds = 0;
                bonds = NULL;
        }
        if (info){
                for (int i=0; i<n_info; ++i){
                        info[i]->hide ();
                        info[i]->queue_update (canvas);
                        delete info[i];
                }
                g_free (info);
                n_info = 0;
                info = NULL;
        }
}


/* GRID PARAMS:
	grid_multiples = 10;
	grid_size = 1;
	grid_mode = 1;
*/

void VObKsys::print_xyz (double x, double y){
        vinfo->W2Angstroem (x,y);
        g_print (" C %g %g 0\n",x,y);
}

void VObKsys::add_bond_len (cairo_item *bonds, int i1, int i2, cairo_item_text **cit, const gchar *lab){
        double x1,x2,y1,y2;
        double dx,dy,bl,x,y;
       
        bonds->get_xy(i1, x1,y1);
        bonds->get_xy(i2, x2,y2);
        
        x = 0.5*(x1+x2);
        y = 0.5*(y1+y2);

        vinfo->W2Angstroem (x1,y1);
        vinfo->W2Angstroem (x2,y2);
        dx=x2-x1; dy=y2-y1; bl=sqrt(dx*dx+dy*dy);
        gchar *txt=NULL;
        switch (info_option){
        case 0: txt = g_strdup_printf ("%s", lab?lab:""); break;
        case 1: txt = g_strdup_printf ("%.1f pm",bl*100.); break;
        case 2: txt = g_strdup_printf ("%.3f Hz",score_bond (bonds, i1,i2, 2)); break;
        case 3: txt = g_strdup_printf ("%s %d:%d",lab?lab:"", i1,i2); break;
        case 6: txt = g_strdup_printf ("%.3f Hz",score_bond (bonds, i1,i2, 2)); break;
        default: txt = g_strdup_printf ("%s", lab?lab:""); break;
        }
        
        if (*cit){
                (*cit)->set_text (x,y,txt);
        }else{
                *cit = new cairo_item_text (x,y,txt);
                (*cit)->set_font_face_size ("Ununtu", 6.);
                (*cit)->set_stroke_rgba (CAIRO_COLOR_CYAN);
                if(lab){
                        gchar *tmp=g_strdup_printf ("%s:%d:%d",lab, i1,i2); (*cit)->set_id(tmp); g_free(tmp);
                }
        }
        
        g_print ("%s : %s\n", lab?lab:"", txt);
        // g_print ("%s [%d:%d] (%g,%g -- %g,%g) %s\n", lab?lab:"", i1,i2, x1,y1, x2,y2, txt);

        g_free(txt);
}

double VObKsys::score_bond (cairo_item *bonds, int i1, int i2, int n){
        double score=0.;
        double x1,x2,y1,y2;

        bonds->get_xy(i1, x1,y1);
        bonds->get_xy(i2, x2,y2);
        double dx = x2-x1;
        double dy = y2-y1;
        int norm=0;
        if (info_option & 4)
                vinfo->sc->data.s.pllref = 0.; // *** force 0 for ADC df data hack
        for (double l=0.0; l<=1.001; l+=1.0/(n-1), ++norm){
                score += vinfo->getZ (x1+l*dx, y1+l*dy);
        }
        return score/norm;
}

void VObKsys::adjust_bond_aromatic_index (cairo_item_segments_wlw *bonds, int i1, int i2, double ai){
        double x1,x2,y1,y2;
        bonds->get_xy(i1, x1,y1);
        bonds->get_xy(i2, x2,y2);

        double dx = x2-x1;
        double dy = y2-y1;
        double adj = 1.+ai; 
        
        x1 += dx*adj;
        y1 += dy*adj;
        x2 -= dx*adj;
        y2 -= dy*adj;
        
        bonds->set_xy(i1, x1,y1);
        bonds->set_xy(i2, x2,y2);
        bonds->set_segment_line_width(i1, m_parameter[2]+adj*m_parameter[3]);
}

void VObKsys::bonds_matchup (cairo_item *bonds){
        int n = bonds->get_n_nodes ();

        for (int i=0; i<n; ++i){
                for (int j=1; j<n; ++j){
                        if (i==j) continue;
                        double x1,x2,y1,y2;
                        bonds->get_xy(i, x1,y1);
                        bonds->get_xy(j, x2,y2);

                        double x1a,y1a,x2a,y2a;
                        x1a=x1; x2a=x2;y1a=y1;y2a=y2;
                        vinfo->W2Angstroem (x1a,y1a);
                        vinfo->W2Angstroem (x2a,y2a);
                        double dx = x2a-x1a;
                        double dy = y2a-y1a;
                        double dr =sqrt(dx*dx+dy*dy);
                
                        if (dr > 0.5 && dr < 3.5)
                                g_print ("Bond[%d,%d] = (%g, %g) - (%g, %g) = |%g|\n", i,j, x1,y1, x2,y2, dr);
                        else
                                g_print ("XYXY[%d,%d] = (%g, %g) - (%g, %g) = |%g|\n", i,j, x1,y1, x2,y2, dr);
                        
                        if (dr < 0.5){
                                double x = 0.5*(x1+x2);
                                double y = 0.5*(y1+y2);
                                bonds->set_xy(i, x,y);
                                bonds->set_xy(j, x,y);
                        }
                }
        }
}

void VObKsys::opt_adjust_xy (int k, double d){
        switch (k){
        case 0: for (int l=0; l<3; l++) xy[2*l] += d; break; // position X
        case 1: for (int l=0; l<3; l++) xy[1+2*l] += d; break; // position Y
        case 2: // diff rot of xy[0,1]
                { 
                        double dx = xy[0]-xy[2];	/* x-component of first vector */
                        double dy = xy[1]-xy[3];	/* y-component of first vector */
                        double phi=atan2(dy,dx);
                        double r=sqrt(dx*dx+dy*dy);
                        m_phi = phi*180.0/M_PI;
                        //XSM_DEBUG_GM (DBG_L3, "phi=%g", phi*180.0/M_PI);
                        double c=cos(phi+d);
                        double s=sin(phi+d);
                        xy[0] = xy[2] + r*c;
                        xy[1] = xy[3] + r*s;
                }
                break;
        case 3: // width
                {
                        double dx = xy[0]-xy[2];	/* x-component of first vector */
                        double dy = xy[1]-xy[3];	/* y-component of first vector */
                        xy[0] = xy[2] + dx*(1.+d);
                        xy[1] = xy[3] + dy*(1.+d);
                        
                } break;
        case 4: // length
                {
                        double dx = xy[4]-xy[2];	/* x-component of 2nd vector */
                        double dy = xy[5]-xy[3];	/* y-component of 2nd vector */
                        xy[4] = xy[2] + dx*(1.+d);
                        xy[5] = xy[3] + dy*(1.+d);
                        
                } break;
        }       
}

void VObKsys::dump_param (){
        gchar lab[3];
        int i1, i2;
        double x1,x2,y1,y2;
        double dx,dy,bl;
        double score;

        if (!bonds || n_bonds<1){
                XSM_DEBUG_GM (DBG_L3, "no bond info to dump.");
                return;
        }
        for (int i=0; i<n_info; ++i){
                //g_print ("%d {%s} ",i,  info[i]->id());
                if (!info[i]->id()) continue;
                sscanf (info[i]->id(), "%1s:%d:%d", lab, &i1, &i2);
                //g_print (" => %s  %d :: %d\n", lab, i1, i2);

                score=score_bond (bonds, i1,i2, 2);
                bonds->get_xy(i1, x1,y1);
                bonds->get_xy(i2, x2,y2);
        
                vinfo->W2Angstroem (x1,y1);
                vinfo->W2Angstroem (x2,y2);
                dx=x2-x1; dy=y2-y1; bl=sqrt(dx*dx+dy*dy)*100.0;
                g_print ("%s: %.1f pm, %.3f Hz\n", lab, bl, score);
        }
        
        for (int i=0; i<n_bonds; i+=2){
                bonds->get_xy(i, x1,y1);
                bonds->get_xy(i+1, x2,y2);
        
                vinfo->W2Angstroem (x1,y1);
                vinfo->W2Angstroem (x2,y2);
                dx=x2-x1; dy=y2-y1; bl=sqrt(dx*dx+dy*dy)*100.0;
                if (bl < 160){
                        double ai = 1.0+(160.-bl)/20.;
                        int k = 255*ai/30;
                        if (k>=0 && k <256) k*=3; else k=0;
                        //bonds->set_segment_rgba (i, pal_image.pixel_data[k]/255., pal_image.pixel_data[k+1]/255., pal_image.pixel_data[k+2]/255., 0.8);
                        bonds->set_segment_line_width(i, ai*OBJECT_LINE_WIDTH);
                } else {
                        //bonds->set_segment_rgba (i, 0., 0., 0., 0.); // Alpha=0 uses default
                        bonds->set_segment_line_width(i, OBJECT_LINE_WIDTH);
                }
        }
        bonds->show ();
        bonds->queue_update (canvas);
}

void VObKsys::run_m_param_opt (){
        double dr[5];
        double d[10];
        m_verbose = 0;
        calc_grid();
        double last_score = total_score;

        for (int k=0; k<5; k++)  dr[k]=m_dopt_r[k];
        for (int k=0; k<10; k++) d[k]=m_dopt[k];

        int n_op=0;
        for (int i=0; i<m_n_opt; ++i){
                n_op=0;
                for (int k=0; k<5; k++){ // position x,y, angle, width, len
                        if (fabs(dr[k]) > 1e-5){
                                n_op++;
                                opt_adjust_xy (k, dr[k]);
                                Update();
                                if (total_score < last_score){ // not good?
                                        opt_adjust_xy (k, -dr[k]); // undo
                                        dr[k] *= -1.0;
                                        opt_adjust_xy (k, dr[k]);
                                        Update();
                                        if (total_score < last_score){ // not good?
                                                opt_adjust_xy (k, -dr[k]); // undo
                                                Update();
                                                dr[k] *= -1.0;
                                                dr[k] *= 0.9;
                                                total_score = last_score;
                                        } else {
                                                last_score = total_score;
                                        }
                                } else {
                                        last_score = total_score;
                                }
                                g_slist_foreach (ECP_list, (GFunc) App::update_ec, NULL);
                                g_print ("#%d %d xy = %g %g %g {%g}, score=%g Hz\n", i, k,  xy[2], xy[3], m_phi, dr[k], total_score);
                        }
                }
                for (int k=0; k<10; k++){ // parameters
                        if (fabs(d[k]) > 1e-5){
                                n_op++;
                                m_parameter[k] += d[k];
                                calc_grid();
                                if (total_score < last_score){ // not good?
                                        m_parameter[k] -= d[k]; // undo, try differnt dir
                                        d[k] *= -1.0;
                                        calc_grid();
                                        m_parameter[k] += d[k];
                                        if (total_score < last_score){ // not good?
                                                m_parameter[k] -= d[k]; // undo, decrese step, cont.
                                                d[k] *= -1.0;
                                                d[k] *= 0.9;
                                                total_score = last_score;
                                        } else {
                                                last_score = total_score;
                                        }
                                } else {
                                        last_score = total_score;
                                }
                                g_slist_foreach (ECP_list, (GFunc) App::update_ec, NULL);
                                g_print ("#%d m_parameter[%d] = %g {%g}, score=%g Hz\n", i, k,  m_parameter[k], d[k], total_score);
                        }
                }
                if (n_op == 0)
                        break; // done
        }
        g_print("Residuals after %d parameter optimizations:", n_op);
        for (int k=0; k<5; k++) g_print ("dr[%d] = %g\n",k, dr[k]);
        for (int k=0; k<10; k++) g_print ("d[%d] = %g\n",k, d[k]);
        m_verbose = 1;
}

void VObKsys::calc_grid(){
	const int gm = grid_mode;
	const int gridfactor = grid_multiples;
	int num_grid_lines = (2*grid_size)*gridfactor+1; // must be odd!
	int nl = 4*num_grid_lines;
        int np = 2*grid_multiples*grid_size+1;
	int i,j,k,l;
        double q,v,qvl;
	double rx[2], ry[2];
        gboolean enable_scoring=true;
        
        np *= np;

	rx[0] = (xy[2]-xy[0])/gridfactor;	/* x-component of first vector */
	rx[1] = (xy[3]-xy[1])/gridfactor;	/* y-component of first vector */
	ry[0] = (xy[2]-xy[4])/gridfactor;
	ry[1] = (xy[3]-xy[5])/gridfactor;

        if (gm >= 12){ // core molecules
                if (gm == 15) {  // BKF core
                        double score = 0.;
#define NI_BKF 14
                        nl = 2*(6+5+2+6+5);
                        if (bonds && nl != n_bonds){
                                bonds->queue_update (canvas);
                                delete bonds;
                                n_bonds = 0;
                                bonds = NULL;
                        }
                        if (bonds == NULL){
                                n_bonds = nl;
                                bonds = new cairo_item_segments_wlw (n_bonds);
                                bonds->set_stroke_rgba (&custom_element_b_color);
                                bonds->set_fill_rgba (0.,0.,0.,0.);
                                bonds->set_line_width (OBJECT_LINE_WIDTH);
                        }

                        if (n_info != NI_BKF && info){
                                for (int i=0; i<n_info; ++i){
                                        info[i]->hide ();
                                        info[i]->queue_update (canvas);
                                }
                                g_free (info); n_info=0; info=NULL;
                        }
                        if (n_info == 0 || info == NULL){
                                n_info = NI_BKF;
                                info = g_new0 (cairo_item_text*, n_info);
                        }
                        j = 0;

                        if (m_verbose)
                                XSM_DEBUG_GM (DBG_L3, "** BKF **");
                        
                        // r1, r2, p1, p2, p3, p4

                        // BKF
                        //     a
                        //   b   b
                        //   c   c
                        //     d
                        //   e   e
                        //   f   f
                        //     g
                        //   h   h
                        // i  n n  i
                        //j    m    j
                        // k l   l k

                        double x1,x2,y1,y2;
                        double dx = xy[2]-xy[0];	/* x-component of first vector */
                        double dy = xy[3]-xy[1];	/* y-component of first vector */
                        double ddx = xy[2]-xy[4];	/* x-component of 2nd vector */
                        double ddy = xy[3]-xy[5];	/* y-component of 2nd vector */
                        double ap =  (1+m_parameter[6])*sqrt (ddx*ddx+ddy*ddy)/sqrt(dx*dx+dy*dy);
                        const double s30 = 0.5;
                        const double c30 = 0.86602562491683672008;
                        const double s18 = 0.30901674200355010095;
                        const double c18 = 0.95105659829555430867;
                        //const double s36 = 0.58778482293254258497;
                        const double c18_s36 = 1.53884142122809689364;
        ;
#define b0(s) (s*s30*rx[0] + c30*(1.0+m_parameter[0])*ry[0])
#define b1(s) (s*s30*rx[1] + c30*(1.0+m_parameter[0])*ry[1])
                        double c0 = 1.732*ry[0];
                        double c1 = 1.732*ry[1];
#define e0(s) (c0+s*s30*rx[0] + c30*(1.0+m_parameter[1])*ry[0])
#define e1(s) (c1+s*s30*rx[1] + c30*(1.0+m_parameter[1])*ry[1])
                        double f0 = 2*(2*c30)*ry[0];
                        double f1 = 2*(2*c30)*ry[1];
#define h0(s) (f0+s*s18*(1.0+m_parameter[2])*rx[0] + c18*(1.0+m_parameter[3])*ry[0]) // !!!
#define h1(s) (f1+s*s18*(1.0+m_parameter[2])*rx[1] + c18*(1.0+m_parameter[3])*ry[1])
                        double n0 = f0+c18_s36*(1.0+m_parameter[3]+m_parameter[4])*ry[0]; // !!! 
                        double n1 = f1+c18_s36*(1.0+m_parameter[3]+m_parameter[4])*ry[1];
                        double m0 = f0+(c18_s36+1.0+m_parameter[5])*ry[0]; // !!! 
                        double m1 = f1+(c18_s36+1.0+m_parameter[5])*ry[1];
#define i0(s) (n0+s*ap*(2*c30)*(1+m_parameter[7])*rx[0]) // !!! 
#define i1(s) (n1+s*ap*(2*c30)*(1+m_parameter[7])*rx[1]) // !!! 
#define j0(s) (m0+s*ap*(2*c30)*rx[0]) // !!! 
#define j1(s) (m1+s*ap*(2*c30)*rx[1]) // !!! 
#define k0(s) (m0+s30*ry[0]*(1.0+m_parameter[8])+s*c30*ap*(1.0+m_parameter[9])*rx[0]) // !!! 
#define k1(s) (m1+s30*ry[1]*(1.0+m_parameter[8])+s*c30*ap*(1.0+m_parameter[9])*rx[1])

                        // a
                        bonds->set_xy (j++, x1=xy[2] - 0.5*rx[0], y1=xy[3] - 0.5*rx[1]);
                        bonds->set_xy (j++, x2=xy[2] + 0.5*rx[0], y2=xy[3] + 0.5*rx[1]);
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        add_bond_len (bonds, j-2, j-1, &info[0], "a");
                        // b
                        bonds->set_xy (j++, x1=xy[2] - 0.5*rx[0], y1=xy[3] - 0.5*rx[1]);
                        bonds->set_xy (j++, x1=xy[2] - 0.5*rx[0] + b0(-1), y2=xy[3] - 0.5*rx[1] + b1(-1));
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        add_bond_len (bonds, j-2, j-1, &info[1], "b");
                        // b'
                        bonds->set_xy (j++, x2=xy[2] + 0.5*rx[0], y1=xy[3] + 0.5*rx[1]);
                        bonds->set_xy (j++, x1=xy[2] + 0.5*rx[0] + b0(1), y2=xy[3] + 0.5*rx[1] + b1(1));
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        // c
                        bonds->set_xy (j++, x1=xy[2] - 0.5*rx[0] + b0(-1), y1=xy[3] - 0.5*rx[1] + b1(-1));
                        bonds->set_xy (j++, x1=xy[2] - 0.5*rx[0] + c0, y2=xy[3] - 0.5*rx[1] + c1);
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        add_bond_len (bonds, j-2, j-1, &info[2], "c");
                        // c'
                        bonds->set_xy (j++, x1=xy[2] + 0.5*rx[0] + b0(1), y1=xy[3] + 0.5*rx[1] + b1(1));
                        bonds->set_xy (j++, x2=xy[2] + 0.5*rx[0] + c0, y2=xy[3] + 0.5*rx[1] + c1);
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);

                        // d
                        bonds->set_xy (j++, x1=xy[2] - 0.5*rx[0] + c0, y1=xy[3] - 0.5*rx[1] + c1);
                        bonds->set_xy (j++, x2=xy[2] + 0.5*rx[0] + c0, y2=xy[3] + 0.5*rx[1] + c1);
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        add_bond_len (bonds, j-2, j-1, &info[3], "d");
                        // e
                        bonds->set_xy (j++, x1=xy[2] - 0.5*rx[0] + c0, y1=xy[3] - 0.5*rx[1] + c1);
                        bonds->set_xy (j++, x1=xy[2] - 0.5*rx[0] + e0(-1), y2=xy[3] - 0.5*rx[1] + e1(-1));
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        add_bond_len (bonds, j-2, j-1, &info[4], "e");
                        // e'
                        bonds->set_xy (j++, x2=xy[2] + 0.5*rx[0] + c0, y1=xy[3] + 0.5*rx[1] + c1);
                        bonds->set_xy (j++, x1=xy[2] + 0.5*rx[0] + e0(1), y2=xy[3] + 0.5*rx[1] + e1(1));
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        // f
                        bonds->set_xy (j++, x1=xy[2] - 0.5*rx[0] + e0(-1), y1=xy[3] - 0.5*rx[1] + e1(-1));
                        bonds->set_xy (j++, x1=xy[2] - 0.5*rx[0] + f0, y2=xy[3] - 0.5*rx[1] + f1);
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        add_bond_len (bonds, j-2, j-1, &info[5], "f");
                        // f'
                        bonds->set_xy (j++, x1=xy[2] + 0.5*rx[0] + e0(1), y1=xy[3] + 0.5*rx[1] + e1(1));
                        bonds->set_xy (j++, x2=xy[2] + 0.5*rx[0] + f0, y2=xy[3] + 0.5*rx[1] + f1);
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);

                        // g
                        bonds->set_xy (j++, x1=xy[2] - 0.5*rx[0] + f0, y1=xy[3] - 0.5*rx[1] + f1);
                        bonds->set_xy (j++, x1=xy[2] + 0.5*rx[0] + f0, y2=xy[3] + 0.5*rx[1] + f1);
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        add_bond_len (bonds, j-2, j-1, &info[6], "g");
                        // h
                        bonds->set_xy (j++, x1=xy[2] - 0.5*rx[0] + f0, y1=xy[3] - 0.5*rx[1] + f1);
                        bonds->set_xy (j++, x2=xy[2] - 0.5*rx[0] + h0(-1), y2=xy[3] - 0.5*rx[1] + h1(-1));
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        add_bond_len (bonds, j-2, j-1, &info[7], "h");
                        // h'
                        bonds->set_xy (j++, x1=xy[2] + 0.5*rx[0] + f0, y1=xy[3] + 0.5*rx[1] + f1);
                        bonds->set_xy (j++, x2=xy[2] + 0.5*rx[0] + h0(1), y2=xy[3] + 0.5*rx[1] + h1(1));
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);

                        // n
                        bonds->set_xy (j++, x2=xy[2] - 0.5*rx[0] + h0(-1), y2=xy[3] - 0.5*rx[1] + h1(-1));
                        bonds->set_xy (j++, x2=xy[2] + n0, y2=xy[3] + n1);
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        add_bond_len (bonds, j-2, j-1, &info[8], "n");
                        // n'
                        bonds->set_xy (j++, x2=xy[2] + 0.5*rx[0] + h0(1), y2=xy[3] + 0.5*rx[1] + h1(1));
                        bonds->set_xy (j++, x2=xy[2] + n0, y2=xy[3] + n1);
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        // i
                        bonds->set_xy (j++, x2=xy[2] - 0.5*rx[0] + h0(-1), y2=xy[3] - 0.5*rx[1] + h1(-1));
                        bonds->set_xy (j++, x2=xy[2] + i0(-1), y2=xy[3] + i1(-1));
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        add_bond_len (bonds, j-2, j-1, &info[9], "i");
                        // i'
                        bonds->set_xy (j++, x2=xy[2] + 0.5*rx[0] + h0(1), y2=xy[3] + 0.5*rx[1] + h1(1));
                        bonds->set_xy (j++, x2=xy[2] + i0(1), y2=xy[3] + i1(1));
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        // j
                        bonds->set_xy (j++, x2=xy[2] + i0(-1), y2=xy[3] + i1(-1));
                        bonds->set_xy (j++, x2=xy[2] + j0(-1), y2=xy[3] + j1(-1));
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        add_bond_len (bonds, j-2, j-1, &info[10], "j");
                        // j'
                        bonds->set_xy (j++, x2=xy[2] + i0(1), y2=xy[3] + i1(1));
                        bonds->set_xy (j++, x2=xy[2] + j0(1), y2=xy[3] + j1(1));
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        // m
                        bonds->set_xy (j++, x2=xy[2] + n0, y2=xy[3] + n1);
                        bonds->set_xy (j++, x2=xy[2] + m0, y2=xy[3] + m1);
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        add_bond_len (bonds, j-2, j-1, &info[11], "m");
                        // k
                        bonds->set_xy (j++, x2=xy[2] + j0(-1), y2=xy[3] + j1(-1));
                        bonds->set_xy (j++, x2=xy[2] + k0(-1), y2=xy[3] + k1(-1));
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        add_bond_len (bonds, j-2, j-1, &info[12], "k");
                        // k'
                        bonds->set_xy (j++, x2=xy[2] + j0(1), y2=xy[3] + j1(1));
                        bonds->set_xy (j++, x2=xy[2] + k0(1), y2=xy[3] + k1(1));
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        // l
                        bonds->set_xy (j++, x2=xy[2] + k0(-1), y2=xy[3] + k1(-1));
                        bonds->set_xy (j++, x2=xy[2] + m0, y2=xy[3] + m1);
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        add_bond_len (bonds, j-2, j-1, &info[13], "l");
                        // l'
                        bonds->set_xy (j++, x2=xy[2] + k0(1), y2=xy[3] + k1(1));
                        bonds->set_xy (j++, x2=xy[2] + m0, y2=xy[3] + m1);
                        if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                        
                        bonds->show ();
                        bonds->set_stroke_rgba (&custom_element_b_color);
                        bonds->queue_update (canvas);
                        for (int i=0; i<n_info; ++i){
                                info[i]->show ();
                                info[i]->queue_update (canvas);
                        }
                        if (enable_scoring){
                                total_score = score/24;
                                if (m_verbose)
                                        XSM_DEBUG_GM (DBG_L3, "Total Bond Score = %.3f Hz", total_score);
                        }
                        
                        bonds->show ();
                        bonds->set_stroke_rgba (&custom_element_b_color);
                        bonds->queue_update (canvas);
                        for (int i=0; i<n_info; ++i){
                                info[i]->show ();
                                info[i]->queue_update (canvas);
                        }
                } else {
                        if (info && n_info == NI_BKF){
                                for (int i=0; i<n_info; ++i){
                                        info[i]->hide ();
                                        info[i]->queue_update (canvas);
                                }
                        }
                }
                if (gm == 14) {  // Coronene core
                        double score = 0.;
#define NI_COR 4
                        nl = 2*6*7;
                        if (bonds && nl != n_bonds){
                                bonds->queue_update (canvas);
                                delete bonds;
                                n_bonds = 0;
                                bonds = NULL;
                        }
                        if (bonds == NULL){
                                n_bonds = nl;
                                bonds = new cairo_item_segments_wlw (n_bonds);
                                bonds->set_stroke_rgba (&custom_element_b_color);
                                bonds->set_fill_rgba (0.,0.,0.,0.);
                                bonds->set_line_width (OBJECT_LINE_WIDTH);
                        }

                        if (n_info != 4 && info){
                                for (int i=0; i<n_info; ++i){
                                        info[i]->hide ();
                                        info[i]->queue_update (canvas);
                                }
                                g_free (info); n_info=0; info=NULL;
                        }
                        if (n_info == 0 || info == NULL){
                                n_info = NI_COR;
                                info = g_new0 (cairo_item_text*, n_info);
                        }
                        j=0;
                        XSM_DEBUG_GM (DBG_L3, "**Updating Coronene Ksys Object**%s, %s (%g,%g)**", name, text, xy[2], xy[3]);
                        for (int phii=0; phii<6; ++phii){
                                double ri = sqrt(rx[0]*rx[0]+rx[1]*rx[1]);
                                double x,y;
                                x=3*rx[0]+ry[0]/grid_aspect;
                                y=3*rx[1]+ry[1]/grid_aspect;
                                double ro = 2.*sqrt(ry[0]*ry[0]+ry[1]*ry[1]);
                                double re = (grid_aspect+m_parameter[0])*sqrt(x*x+y*y); //(ry[0]*ry[0]+ry[1]*ry[1]);
                                double rh = (0.2+grid_aspect+m_parameter[1])*sqrt(x*x+y*y); //(ry[0]*ry[0]+ry[1]*ry[1]);
                                double s0=sin (phii*60./180.*M_PI);
                                double c0=cos (phii*60./180.*M_PI);
                                double s1=sin ((phii+1)*60./180.*M_PI);
                                double c1=cos ((phii+1)*60./180.*M_PI);
                                double s2=sin ((phii*60.+30.-8.-m_parameter[2])/180.*M_PI);
                                double c2=cos ((phii*60.+30.-8.-m_parameter[2])/180.*M_PI);
                                double s3=sin ((phii*60.+30.+8.+m_parameter[2])/180.*M_PI);
                                double c3=cos ((phii*60.+30.+8.+m_parameter[2])/180.*M_PI);
                                double sh2=sin ((phii*60.+30.-10.-m_parameter[3])/180.*M_PI);
                                double ch2=cos ((phii*60.+30.-10.-m_parameter[3])/180.*M_PI);
                                double sh3=sin ((phii*60.+30.+10.+m_parameter[3])/180.*M_PI);
                                double ch3=cos ((phii*60.+30.+10.+m_parameter[3])/180.*M_PI);
                                // inner ring
                                double x1,x2,y1,y2;
                                bonds->set_xy (j++, x1=xy[2] + c0*rx[0] + s0*rx[1], y1=xy[3] + c0*rx[1] - s0*rx[0]);
                                bonds->set_xy (j++, x2=xy[2] + c1*rx[0] + s1*rx[1], y2=xy[3] + c1*rx[1] - s1*rx[0]);
                                if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                                if (phii==0) add_bond_len (bonds, j-2, j-1, &info[0], "a");
                                print_xyz (x2, y2);
                                // spoke
                                bonds->set_xy (j++, x1=xy[2] + c0*rx[0] + s0*rx[1], y1=xy[3] + c0*rx[1] - s0*rx[0]);
                                bonds->set_xy (j++, x2=xy[2] + ro*(c0*rx[0] + s0*rx[1])/ri, y2=xy[3] + ro*(c0*rx[1] - s0*rx[0])/ri);
                                if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                                if (phii==0) add_bond_len (bonds, j-2, j-1, &info[1], "b");
                                // spoke - outher segment
                                bonds->set_xy (j++, x1=xy[2] + ro*(c0*rx[0] + s0*rx[1])/ri, y1=xy[3] + ro*(c0*rx[1] - s0*rx[0])/ri);
                                bonds->set_xy (j++, x2=xy[2] + re*(c2*rx[0] + s2*rx[1])/ri, y2=xy[3] + re*(c2*rx[1] - s2*rx[0])/ri);
                                if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                                if (phii==0) add_bond_len (bonds, j-2, j-1, &info[2], "c");
                                print_xyz (x2, y2);
                                // outher segment
                                bonds->set_xy (j++, x1=xy[2] + re*(c2*rx[0] + s2*rx[1])/ri, y1=xy[3] + re*(c2*rx[1] - s2*rx[0])/ri);
                                bonds->set_xy (j++, x2=xy[2] + re*(c3*rx[0] + s3*rx[1])/ri, y2=xy[3] + re*(c3*rx[1] - s3*rx[0])/ri);
                                if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                                if (phii==0) add_bond_len (bonds, j-2, j-1, &info[3], "d");
                                print_xyz (x2, y2);
                                // outher segment - spoke next
                                bonds->set_xy (j++, x1=xy[2] + re*(c3*rx[0] + s3*rx[1])/ri, y1=xy[3] + re*(c3*rx[1] - s3*rx[0])/ri);
                                bonds->set_xy (j++, x2=xy[2] + ro*(c1*rx[0] + s1*rx[1])/ri, y2=xy[3] + ro*(c1*rx[1] - s1*rx[0])/ri);
                                if (enable_scoring) score += score_bond (bonds, j-2, j-1, 2);
                                print_xyz (x2, y2);
                                // H's
                                bonds->set_xy (j++, xy[2] + re*(c2*rx[0] + s2*rx[1])/ri, xy[3] + re*(c2*rx[1] - s2*rx[0])/ri);
                                bonds->set_xy (j++, xy[2] + rh*(ch2*rx[0] + sh2*rx[1])/ri, xy[3] + rh*(ch2*rx[1] - sh2*rx[0])/ri);
                                bonds->set_xy (j++, xy[2] + re*(c3*rx[0] + s3*rx[1])/ri, xy[3] + re*(c3*rx[1] - s3*rx[0])/ri);
                                bonds->set_xy (j++, xy[2] + rh*(ch3*rx[0] + sh3*rx[1])/ri, xy[3] + rh*(ch3*rx[1] - sh3*rx[0])/ri);
                        }
                        if (enable_scoring){
                                total_score = score/(5*6);
                                if (m_verbose)
                                        XSM_DEBUG_GM (DBG_L3, "Total Bond Score = %.3f Hz", total_score);
                        }
                        
                        bonds->show ();
                        bonds->set_stroke_rgba (&custom_element_b_color);
                        bonds->queue_update (canvas);
                        for (int i=0; i<n_info; ++i){
                                info[i]->show ();
                                info[i]->queue_update (canvas);
                        }
                } else {
                        if (info && n_info == NI_COR){
                                for (int i=0; i<n_info; ++i){
                                        info[i]->hide ();
                                        info[i]->queue_update (canvas);
                                }
                        }
                }
                if (gm == 12 || gm == 13){ // Pentacene like linear array of hexas w / wo H
                        double score = 0.;
#define NI_PENTA 2
                        if (gm == 12)
                                nl = 6*num_grid_lines;
                        else
                                nl = 12*num_grid_lines;
                
                        if (bonds && nl != n_bonds){
                                bonds->queue_update (canvas);
                                delete bonds;
                                n_bonds = 0;
                                bonds = NULL;
                        }

                        if (bonds == NULL){
                                n_bonds = nl;
                                bonds = new cairo_item_segments_wlw (n_bonds);
                                bonds->set_stroke_rgba (&custom_element_b_color);
                                bonds->set_fill_rgba (0.,0.,0.,0.);
                                bonds->set_line_width (OBJECT_LINE_WIDTH);
                        }
                        if (n_info != 2 && info){
                                for (int i=0; i<n_info; ++i){
                                        info[i]->hide ();
                                        info[i]->queue_update (canvas);
                                }
                                g_free (info); n_info=0; info=NULL;
                        }
                        if (n_info == 0 || info == NULL){
                                n_info = NI_PENTA;
                                info = g_new0 (cairo_item_text*, n_info);
                        }

                        j=0;

                        k=-2; l=-1; qvl=109./139.;
                        for (int ir=0; ir<num_grid_lines/2; ++ir){
                                if (gm == 13){
                                        q=-1; v=-1; q*=qvl; v*=qvl;
                                        bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                        bonds->set_xy (j++, xy[2] + (k+q)*rx[0] + (l+v)*ry[0], xy[3] + (k+q)*rx[1] + (l+v)*ry[1]);
                                }
                                bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                ++k; 
                                bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                adjust_bond_aromatic_index (bonds, j-2,j-1, m_parameter[0]);
                                if (gm == 13){
                                        q=0; v=-1; q*=qvl; v*=qvl;
                                        bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                        bonds->set_xy (j++, xy[2] + (k+q)*rx[0] + (l+v)*ry[0], xy[3] + (k+q)*rx[1] + (l+v)*ry[1]);
                                }
                                bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                ++k; ++l;
                                bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                adjust_bond_aromatic_index (bonds, j-2,j-1, m_parameter[1]);
                                if (gm == 13){
                                        q=1; v=0; q*=qvl; v*=qvl;
                                        bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                        bonds->set_xy (j++, xy[2] + (k+q)*rx[0] + (l+v)*ry[0], xy[3] + (k+q)*rx[1] + (l+v)*ry[1]);
                                }
                                bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                ++l;
                                bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                adjust_bond_aromatic_index (bonds, j-2,j-1, m_parameter[0]);
                                if (gm == 13){
                                        q=1; v=1; q*=qvl; v*=qvl;
                                        bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                        bonds->set_xy (j++, xy[2] + (k+q)*rx[0] + (l+v)*ry[0], xy[3] + (k+q)*rx[1] + (l+v)*ry[1]);
                                }
                                bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                --k;
                                bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                adjust_bond_aromatic_index (bonds, j-2,j-1, m_parameter[1]);
                                if (gm == 13){
                                        q=0; v=1; q*=qvl; v*=qvl;
                                        bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                        bonds->set_xy (j++, xy[2] + (k+q)*rx[0] + (l+v)*ry[0], xy[3] + (k+q)*rx[1] + (l+v)*ry[1]);
                                }
                                bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                --k; --l;
                                bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                adjust_bond_aromatic_index (bonds, j-2,j-1, m_parameter[0]);
                                if (gm == 13){
                                        q=-1; v=0; q*=qvl; v*=qvl;
                                        bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                        bonds->set_xy (j++, xy[2] + (k+q)*rx[0] + (l+v)*ry[0], xy[3] + (k+q)*rx[1] + (l+v)*ry[1]);
                                }
                                bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                --l;
                                bonds->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                ++k; ++k; ++l;
                                adjust_bond_aromatic_index (bonds, j-2,j-1, m_parameter[1]);

                        }
                        bonds_matchup (bonds);
                        add_bond_len (bonds,0,1, &info[0]);
                        add_bond_len (bonds,2,3, &info[1]);
                        if (enable_scoring){
                                for (int i=0; i<j; i+=2)
                                        score += score_bond (bonds,i,i+1, 2);
                                XSM_DEBUG_GM (DBG_L3, "Score = %.3f Hz", score/(j/2));
                        }
                        bonds->show ();
                        bonds->set_stroke_rgba (&custom_element_b_color);
                        bonds->queue_update (canvas);
                        for (int i=0; i<n_info; ++i){
                                info[i]->show ();
                                info[i]->queue_update (canvas);
                        }
                }
        } else {
                if (bonds){
                           bonds->hide ();
                           bonds->queue_update (canvas);
                }
                if (info && n_info == NI_PENTA){
                        for (int i=0; i<n_info; ++i){
                                info[i]->hide ();
                                info[i]->queue_update (canvas);
                        }
                }
        }
        
        if (gm & 2 && gm < 12){
                if (atoms && np != n_atoms){
                        atoms->queue_update (canvas);
                        delete atoms;
                        n_atoms = 0;
                        atoms = NULL;
                }

		if (atoms == NULL){
                        n_atoms = np;
                        XSM_DEBUG_GP (DBG_L1, "calc atoms -- new atoms n=%d\n", n_atoms); 
                        atoms = new cairo_item_circle (n_atoms);
                        atoms->set_stroke_rgba (&custom_element_b_color);
                        atoms->set_fill_rgba (0.,0.,0.,0.);
                        atoms->set_line_width (OBJECT_LINE_WIDTH);
                        atoms->set_radius (rx[0]/2.);
		}

		j = 0;


		double arx=0.5 * sqrt (rx[0]*rx[0] + rx[1]*rx[1]);
		double ary=0.5 * sqrt (ry[0]*ry[0] + ry[1]*ry[1]);
		if (arx > ary) arx = ary;
		if (ary > arx) ary = arx;

		for (k=-num_grid_lines/2; k<=num_grid_lines/2; ++k)
			for (l=-num_grid_lines/2; l<=num_grid_lines/2; ++l)
				if (j < n_atoms){
                                        atoms->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
                                }

                atoms->set_radius (arx > ary ? arx : ary);
                atoms->set_stroke_rgba (&custom_element_b_color);
                atoms->show ();
                atoms->queue_update (canvas);
	} else {
                if (atoms){
                           atoms->hide ();
                           atoms->queue_update (canvas);
                }
        }

	if (gm & 1 && gm < 12) {
		j = 0;
		l = num_grid_lines/2;
                
                if (lines && nl != n_lines){
                        lines->queue_update (canvas);
                        delete lines;
                        n_lines = 0;
                        lines = NULL;
                }

		if (lines == NULL){
                        n_lines = nl;
                        lines = new cairo_item_segments (n_lines);
                        lines->set_stroke_rgba (&custom_element_b_color);
                        lines->set_fill_rgba (0.,0.,0.,0.);
                        lines->set_line_width (OBJECT_LINE_WIDTH);
		}

		j = 0;
		
		for (i=-num_grid_lines/2; i<=num_grid_lines/2; ++i){
			k = i; 
                        if (j < n_lines)
                                lines->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
			l = -l;
                        if (j < n_lines)
                                lines->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
		}
		k = num_grid_lines/2;
		for (i=-num_grid_lines/2; i<=num_grid_lines/2; ++i){
			l = i; 
                        if (j < n_lines)
                                lines->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
			k = -k;
                        if (j < n_lines)
                                lines->set_xy (j++, xy[2] + k*rx[0] + l*ry[0], xy[3] + k*rx[1] + l*ry[1]);
		}  

                lines->show ();
                lines->set_stroke_rgba (&custom_element_b_color);
                lines->queue_update (canvas);

	} else {
                if (lines){
                        lines->hide ();
                        lines->queue_update (canvas);
                }
        }
}

void VObKsys::update_grid(){
	calc_grid ();
}

void VObKsys::Update(){
	gchar *s1;

        double dx = xy[0]-xy[2];	/* x-component of first vector */
        double dy = xy[1]-xy[3];	/* y-component of first vector */
        m_phi = atan2(dy,dx)*180.0/M_PI;

	if (grid_mode > 3){
		double dx = xy[2]-xy[0];	/* x-component of first vector */
		double dy = xy[3]-xy[1];	/* y-component of first vector */
		double phi=0.;

                if (grid_mode == 15){
                        double ddx = xy[2]-xy[4];	/* x-component of 2nd vector */
                        double ddy = xy[3]-xy[5];	/* y-component of 2nd vector */
                        double ap =  sqrt (ddx*ddx+ddy*ddy)/sqrt(dx*dx+dy*dy);
                        phi = -90;
                        phi=M_PI*phi/180;
                        double c=cos(phi);
                        double s=sin(phi);

                        xy[4] = xy[2] + ap * (dx*c + dy*s);
                        xy[5] = xy[3] + ap * (dy*c - dx*s);
                }else{
                        if (grid_mode >= 12)
                                phi = -60;
                        else if (grid_mode > 7)
                                phi = -90.;
                        else
                                phi = -60.;
                        phi=M_PI*phi/180;
                        double c=cos(phi);
                        double s=sin(phi);

                        xy[4] = xy[2] + grid_aspect * (dx*c + dy*s);
                        xy[5] = xy[3] + grid_aspect * (dy*c - dx*s);
                
                }
		node_marker(abl[2], &xy[4], 2);
	}

	// arrows
	for (int axis=0; axis<2; ++axis){
		int segment = axis;
		double l  = 12. * get_marker_scale ();

		// Arrowhead at end
                if (!arrow_head[segment])
                        arrow_head[segment] = new cairo_item_path_closed (3);

		double phi = M_PI*(Phi(segment, segment+1)+30.)/180.;
		double M[4];
		M[0] = M[3] = cos(phi);
		M[2] = -(M[1] = sin(phi));
		arrow_head[segment]->set_xy (0, xy[segment*2+2]-(l*M[0] + l*M[1]), xy[segment*2+3]+(l*M[2] + l*M[3]));

		phi = M_PI*(Phi(segment, segment+1)+60.)/180.;
		M[0] = M[3] = cos(phi);
		M[2] = -(M[1] = sin(phi));
                arrow_head[segment]->set_xy (1, xy[segment*2+2]-(l*M[0] + l*M[1]), xy[segment*2+3]+(l*M[2] + l*M[3]));

		arrow_head[segment]->set_xy (2, xy[segment*2+2], xy[segment*2+3]);

                arrow_head[segment]->set_stroke_rgba (&custom_element_color);
                arrow_head[segment]->set_fill_rgba (0.,0.,0., 0.33);
                arrow_head[segment]->set_line_width (OBJECT_LINE_WIDTH);
                arrow_head[segment]->queue_update (canvas);
	}

	// spacetime check on visibility
	if (is_spacetime ())
                show (); // globale
        else
                hide ();

	// status info
	gchar *phi = g_strdup_printf("phi=%g" UTF8_DEGREE, Phi()-Phi(xy[2]-xy[4], xy[3]-xy[5]));
	gchar *rot = g_strdup_printf("rot=%g" UTF8_DEGREE, Phi());
	gchar *e1, *e2;
	e1 = vinfo->makeDXYinfo(xy,xy+2, 1./grid_multiples);		/* length of unit vector 1 */
	e2 = vinfo->makeDXYinfo(xy+4,xy+2, 1./grid_multiples);	/* length of unit vector 2 */
	gchar *mld = g_strconcat("Line: o:",
				 s1=vinfo->makeXYinfo(xy[2],xy[3]), 
				 ", ",
				 rot,
				 ", ",
				 phi,"; ",
				 "e1=", e1, ", ",
				 "e2=", e2,
				 NULL);
	g_free(s1);
	g_free(vinfo->makeXYinfo(xy[0],xy[1]));
	g_free(vinfo->makeXYinfo(xy[4],xy[5]));

	abl[np]->set_xy (0, xy[0], xy[1]);
	abl[np]->set_xy (1, xy[2], xy[3]);
        abl[np]->set_xy (2, xy[4], xy[5]);
        abl[np]->set_fill_rgba (&custom_element_color);

	calc_grid ();
	update_grid ();
  
        update_label ();

	//vinfo->sc->PktVal=3;

        gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), statusid);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, mld);
	if(profile){
                //**		profile->show();
                //**		profile->NewData(vinfo->sc, this);
	}

	g_free(mld);
	g_free(phi);
	g_free(rot);

	Activate ();
}

VObParabel::VObParabel(GtkWidget *canvas, double *xy0, int pflg, VOBJ_COORD_MODE cmode, const gchar *lab, double Marker_scale) 
	: VObject(canvas, xy0, 3, pflg, cmode, lab, Marker_scale){
	if(name) g_free(name);
	name = g_strdup("Parabel");
	obj_type_id (O_PARABEL);

	abl[3] = new cairo_item_path (3); // GTK3QQQ fix parabel path
	abl[3]->set_xy (0, xy[0], xy[1]); 
        abl[3]->set_xy (0, xy[2], xy[3]);
        abl[3]->set_xy (0, xy[4], xy[5]);
        abl[3]->set_stroke_rgba (&custom_element_color);
        abl[3]->set_line_width (OBJECT_LINE_WIDTH);
        abl[3]->queue_update (canvas);

	Update();
}

VObParabel::~VObParabel(){
	XSM_DEBUG(DBG_L2, "VObParabel::~VObParabel");
}


void VObParabel::Update(){
	double phi;
	gchar *s1, *s2, *s3, *s4;
	gchar *sphi = g_strdup_printf("phi=%g" UTF8_DEGREE, phi=Phi());
	gchar *mld = g_strconcat("Line: |",
				 s1=vinfo->makeXYinfo(xy[0],xy[1]),
				 ",",
				 s2=vinfo->makeXYinfo(xy[2],xy[3]),
				 ",",
				 s3=vinfo->makeXYinfo(xy[4],xy[5]),
				 "|=",
				 s4=vinfo->makeXinfo(Dist()), ", ", sphi,
				 NULL);
	g_free(s4); g_free(s3); g_free(s2); g_free(s1); g_free(sphi);

#define NSEGS 20
	double dx,x;
	double c = cos(phi/180.*M_PI);
	double s = sin(phi/180.*M_PI);
	double p2[2], pt2[2];
	double skl;
	p2[0] = xy[4]-xy[2];
	p2[1] = xy[5]-xy[3];
	pt2[0] =  c*p2[0] + s*p2[1];
	pt2[1] = -s*p2[0] + c*p2[1];
	x  = c*(xy[0]-xy[2]) + s*(xy[1]-xy[3]);
	dx = (pt2[0]-x)/(NSEGS-1);
	skl= pt2[1]/pt2[0]/pt2[0];
	c = cos(-phi/180.*M_PI);
	s = sin(-phi/180.*M_PI);
	for( int i=0; i<NSEGS; ++i, x+=dx){
		abl[np]->set_xy (i, xy[2] + c*x + s*x*x*skl, xy[3] - s*x + c*x*x*skl);
	}

        abl[np]->set_fill_rgba (&custom_element_color);
        abl[np]->queue_update (canvas);

        gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), statusid);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, mld);

        update_label ();

	if(profile){
                //**		profile->show();
                //**		profile->NewData(vinfo->sc, this);
	}
	g_free(mld);


	Activate ();
}

VObRectangle::VObRectangle(GtkWidget *canvas, double *xy0, int pflg, VOBJ_COORD_MODE cmode, const gchar *lab, double Marker_scale) 
	: VObject(canvas, xy0, 2, pflg, cmode, lab, Marker_scale){
	if(name) g_free(name);
	name = g_strdup("Rectangle");
	obj_type_id (O_RECTANGLE);

        abl[2] = new cairo_item_rectangle (xy[0], xy[1], xy[2], xy[3]);
        abl[2]->set_stroke_rgba (&custom_element_color);
        abl[2]->set_fill_rgba (0.,0.,0.,0.2);
        abl[2]->set_line_width (OBJECT_LINE_WIDTH);
        abl[2]->queue_update (canvas);
}

VObRectangle::~VObRectangle(){};

void VObRectangle::Update(){
	gchar *s1, *s2, *s3;
	gchar *mld = g_strconcat("Rect: ",
				 s1=vinfo->makeXYinfo(xy[0],xy[1]),
				 "-",
				 s2=vinfo->makedXdYinfo(xy,xy+2),
				 " A=",
				 s3=vinfo->makeA2info(xy,xy+2),
				 NULL);
	g_free(s3); g_free(s2); g_free(s1);
	g_free(vinfo->makeXYinfo(xy[2],xy[3]));
        abl[np]->set_xy (0, xy[0], xy[1]);
        abl[np]->set_xy (1, xy[2], xy[3]);
        abl[np]->set_stroke_rgba (&custom_element_color);

	if (is_spacetime ())
                show ();
        else
                hide ();

        abl[np]->queue_update (canvas);

        update_label ();
                
        gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), statusid);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, mld);
	//vinfo->sc->PktVal=2;

	main_get_gapp ()->xsm->MausMode(MRECTANGLE);
	g_free(mld);

	Activate ();
}

//FIXME - does not work on rotated images
void VObRectangle::SetUpScan()
{
	double x0,y0,x1,y1,dx,dy;
	double xyq[4];

	xyq[0] = xy[0]*vinfo->GetQfac();
	xyq[1] = xy[1]*vinfo->GetQfac();
	xyq[2] = xy[2]*vinfo->GetQfac();
	xyq[3] = xy[3]*vinfo->GetQfac();
	
	/* abort if center of rectangle is outside current scan window
	 * it seems Pixel2World does not handle that as i would expect
	 */
	if ((xyq[0] + xyq[2])/2. < 0 || (xyq[0] + xyq[2])/2. >= vinfo->sc->mem2d->GetNx() ||
	    (xyq[1] + xyq[3])/2. < 0 || (xyq[1] + xyq[3])/2. >= vinfo->sc->mem2d->GetNy())
		return;

	// new offset
	vinfo->sc->Pixel2World (R2INT((xyq[0] + xyq[2])/2.), R2INT((xyq[1] + xyq[3])/2.),
				main_get_gapp ()->xsm->data.s.x0, main_get_gapp ()->xsm->data.s.y0, 
				SCAN_COORD_ABSOLUTE);
	// calc new size
	vinfo->sc->Pixel2World (R2INT(xyq[0]), R2INT(xyq[1]),
				x0, y0,
				SCAN_COORD_RELATIVE);

	vinfo->sc->Pixel2World (R2INT(xyq[2]), R2INT(xyq[3]),
				x1, y1,
				SCAN_COORD_RELATIVE);

	XSM_DEBUG ( DBG_L3, "SetUpScan:" << np << ":" << x0 << "," << y0 << ", " << x1 << "," << y1 );
	dx = fabs(x1-x0);
	dy = fabs(y1-y0);
	XSM_DEBUG (DBG_L3, "SetUpScan:" << dx << "," << dy );
	main_get_gapp ()->xsm->data.s.rx = dx;
	main_get_gapp ()->xsm->data.s.ry = dy;

	main_get_gapp ()->xsm->data.s.nx = R2INT(fabs(xyq[2]-xyq[0]));
	main_get_gapp ()->xsm->data.s.ny = R2INT(fabs(xyq[3]-xyq[1]));
	main_get_gapp ()->xsm->data.s.dx = main_get_gapp ()->xsm->data.s.rx/(main_get_gapp ()->xsm->data.s.nx-1);
	main_get_gapp ()->xsm->data.s.dy = main_get_gapp ()->xsm->data.s.ry/(main_get_gapp ()->xsm->data.s.ny-1);
	//main_get_gapp ()->xsm->data.s.alpha = -Phi(dx,dy); // leave same angle
	main_get_gapp ()->spm_update_all();
}

VObCircle::VObCircle(GtkWidget *canvas, double *xy0, int pflg, VOBJ_COORD_MODE cmode, const gchar *lab, double Marker_scale) 
	: VObject(canvas, xy0, 2, pflg, cmode, lab, Marker_scale){
	if(name) g_free(name);
	name = g_strdup("Circle");
	obj_type_id (O_CIRCLE);

        double dx = xy[0] - xy[2];
        double dy = xy[1] - xy[3];
	abl[2] = new cairo_item_circle (xy[0], xy[1], sqrt (dx*dx+dy*dy));
        abl[2]->set_stroke_rgba (&custom_element_color);
        abl[2]->set_fill_rgba (0.,0.,0.,0.);
        abl[2]->set_line_width (OBJECT_LINE_WIDTH);
        abl[2]->queue_update (canvas);
}

VObCircle::~VObCircle(){};

void VObCircle::Update(){
	gchar *s1, *s2;
	gchar *mld = g_strconcat("Circle: ",
				 s1=vinfo->makeXYinfo(xy[0],xy[1]),
				 ", r=",
				 s2=vinfo->makeDXYinfo(xy, xy+2),
				 NULL);
	g_free(s2); g_free(s1);
	g_free(vinfo->makeXYinfo(xy[2],xy[3]));

        double dx = xy[0] - xy[2];
        double dy = xy[1] - xy[3];
	abl[2]->set_xy (0, xy[0], xy[1]);
	((cairo_item_circle*)abl[2])->set_radius (sqrt (dx*dx+dy*dy)); // radius
        abl[2]->set_stroke_rgba (&custom_element_color);
        abl[2]->set_line_width (OBJECT_LINE_WIDTH);
        abl[2]->queue_update (canvas);

        update_label ();

        gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), statusid);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, mld);
	//vinfo->sc->PktVal=2;

	if (profile){
                //**		profile->show();
                //**		profile->NewData(vinfo->sc, this);
	}

	g_free(mld);
	Activate ();
}


VObEvent::VObEvent(GtkWidget *canvas, double *xy0, int pflg, VOBJ_COORD_MODE cmode, const gchar *lab, double marker_scale)
	: VObject(canvas, xy0, 0, pflg, cmode, lab, marker_scale){
	set_obj_name ("Event");
	obj_type_id (O_EVENT);
	lock_object (true);
	Update();
}

void VObEvent::Update(){
	gchar *s1;
	gchar *mld = g_strconcat("Event: ",
				 s1=vinfo->makeXYZinfo(xy[0],xy[1]),
				 NULL);
	g_free(s1);
        gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), statusid);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, mld);
	//vinfo->sc->PktVal=1;

	if (vinfo->sc->view)
		vinfo->sc->view->update_event_info (scan_event);

        update_label ();

	Activate ();

	g_free(mld);
}

VObEvent::~VObEvent(){};
