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


#define XSHMIMG_MAXZOOM 10


#include <locale.h>
#include <libintl.h>

#include "view.h"
#include "mem2d.h"
#include "xsmmasks.h"
#include "glbvars.h"
#include "scan_event.h"
#include "surface.h"

#include "bench.h"
#include "util.h"

#include "app_view.h"

//==================================================
// Posthandler
//==================================================


#define L_NULL    0
#define L_START   1
#define L_CHANGE  2
#define L_MOVE    3
#define L_END     4

#define QSIZE   5  // Size of Line-Handles (Squares)

//==================================================
// View Class
//==================================================

View::View(){
	XSM_DEBUG (DBG_L2, "View::View");
	scan=NULL;
	data=NULL;

	trace=NULL;
	reset_trace();
	abs_move_tip(0.,0.);

	cmode = main_get_gapp ()->xsm->ZoomFlg & VIEW_PALETTE ? USER_FALSE_COLOR : DEFAULT_GREY;
}

View::View(Scan *sc, int ChNo){
	XSM_DEBUG (DBG_L2, "View::View(m2d)");
	scan  = sc;
	data  = &scan->data;
	//-  data  = scan->vdata;
	ChanNo=ChNo;

	trace=NULL;
	reset_trace();
	abs_move_tip(scan->data.s.x0, scan->data.s.y0);

	cmode = main_get_gapp ()->xsm->ZoomFlg & VIEW_PALETTE ? USER_FALSE_COLOR : DEFAULT_GREY;
}

View::~View(){
	XSM_DEBUG (DBG_L2, "View::~View -- destroy");
}

void View::update_mxyz_from_points (){
	/* plane with  ax +by +cz +d = 0 given by 3 points */
	double a, b, c, d;
	double x1, y1, z1, z2, x2, y2, z3, x3, y3;
	long i;
	int n_obj = scan->number_of_object ();
	
	Point2D p[3];
	
	// at least three...
	if (n_obj < 3) {
		scan->mem2d-> SetPlaneMxyz ();
		return;
	}

	// look for the three point objects 
	i=0;
	while (n_obj-- && i < 3){
		scan_object_data *obj_data = scan->get_object_data (n_obj);
		
		if (strncmp (obj_data->get_name (), "Point", 5) )
			continue; // only points are used!
		
		if (obj_data->get_num_points () != 1) 
			continue; // sth. is weired!

		// get real world coordinates of point
		obj_data->get_xy_i_pixel2d (0, &p[i]); // get point 0 coordinates in pixels!!
		++i;
	}		

	if(i != 3){
		scan->mem2d-> SetPlaneMxyz ();
		return;
	}

	/* PRODUCE IN-PLANE VECTORS */
	x3 = p[1].x;
	y3 = p[1].y;
	if (x3 < 0 || y3 < 0 || x3 > scan->mem2d->GetNx() || y3 > scan->mem2d->GetNy()) 
		return;
	z3 = scan->mem2d->GetDataPkt((int)(x3), (int)(y3));
	
	x1 = p[0].x-x3;
	y1 = p[0].y-y3;
	if (p[0].x < 0 || p[0].y < 0 || p[0].x > scan->mem2d->GetNx() || p[0].y > scan->mem2d->GetNy()) 
		return;
	z1 = (double)scan->mem2d->GetDataPkt((int)p[0].x, (int)p[0].y) - z3;
	
	x2 = p[2].x-x3;
	y2 = p[2].y-y3;
	if (p[0].x < 0 || p[0].y < 0 || p[2].x > scan->mem2d->GetNx() || p[2].y > scan->mem2d->GetNy()) 
		return;
	z2 = (double)scan->mem2d->GetDataPkt((int)p[2].x, (int)p[2].y) - z3;
	
	/* a,b,c follow from cross-produkt of 1 and 2 */
	a = y1*z2 - z1*y2;
	b = z1*x2 - x1*z2;
	c = x1*y2 - y1*x2;
	
	/* d follows from one special point, use POINT 3 */
	d = -(a*x3 + b*y3 + c*z3);
	
	d /= c;
	a /= c;
	b /= c;

	scan->mem2d-> SetPlaneMxyz (a,b,d);
}

int View::draw(int zoomoverride){
	XSM_DEBUG (DBG_L2, "View Base: draw !");
	return 0;
}

int View::update(int y1, int y2){
	XSM_DEBUG (DBG_L2, "View Base: update !");
	return 0;
}

void View::abs_move_tip(double xa, double ya, int mode){
	Trace_Data *td = new Trace_Data;
	current_td.x = xa;
	current_td.y = ya;
	current_td.mode = mode;
	current_td.t = clock () - trace_t0;
	memcpy (td, &current_td, sizeof (Trace_Data));
	add_to_trace (td);
	update_trace ();
}

void View::rel_move_tip(double dxa, double dya, int mode){
	current_td.x += dxa;
	current_td.y += dya;
	abs_move_tip(current_td.x, current_td.y, mode);
}

void View::update_position(Trace_Data *td){
	memcpy (&current_td, td, sizeof (Trace_Data));	
}

void View::save_trace(){
	g_slist_foreach((GSList*) trace, 
			(GFunc) View::print_td, this);
}

void View::print_trace(){
	g_slist_foreach((GSList*) trace, 
			(GFunc) View::print_td, this);
}

void View::add_to_trace(Trace_Data *td){
	trace = g_slist_prepend (trace, td);
}

void View::print_td(Trace_Data *td, gpointer data){
	XSM_DEBUG(DBG_L1, "("
		  << ((double)td->t/CLOCKS_PER_SEC) << ": " 
		  << td->x << ", " 
		  << td->y << ", " 
		  << td->z << ", " 
		  << td->v[0] << ", " 
		  << td->mode
		  << ")" );
}

void View::reset_trace(){
	if (trace){
		g_slist_foreach((GSList*) trace, 
				(GFunc) View::delete_td, this);
		g_slist_free(trace);
		trace = NULL;
	}
	trace_t0 = clock ();
}


// ============================== Class Grey2D ==============================

Grey2D::Grey2D(Scan *sc, int ChNo):View(sc, ChNo){
	XSM_DEBUG (DBG_L2, "Grey2D::Grey2D");
	XImg = NULL;
	oMC=oZ=oQ=oXPMx=oXPMy=oVm=0;
	viewcontrol=NULL;
	userzoom = USER_ZOOM_AUTO;
}

Grey2D::Grey2D():View(){
	oMC=oZ=oQ=oXPMx=oXPMy=oVm=0;
	XImg = NULL;
	viewcontrol=NULL;
	userzoom = USER_ZOOM_AUTO;
}

Grey2D::~Grey2D(){
	XSM_DEBUG (DBG_L2, "Grey2D::~Grey2D -- destroy");
	if(viewcontrol)
		delete viewcontrol;
	viewcontrol=NULL;
	XImg=NULL;
}

int Grey2D::update(int y1, int y2){
	if (!XImg) 
		return 0;
       
	if (y2 < y1 && viewcontrol){
		if (ChanNo == main_get_gapp ()->xsm->ActiveChannel)
			viewcontrol->SetActive (TRUE);
		else
			viewcontrol->SetActive (FALSE);

                //XSM_DEBUG_GM (DBG_L3,  "Grey2D::update** A");
		if (scan->ixy_sub[1]>0)
                        XImg->ShowSubPic(scan->ixy_sub[0]/QuenchFac, y1/QuenchFac,scan->ixy_sub[1]/QuenchFac,(y2+QuenchFac-y1-1)/QuenchFac, scan->ixy_sub[2]/QuenchFac, scan->ixy_sub[3]/QuenchFac);
                else
                        XImg->ShowPic ();

                //viewcontrol -> force_redraw ();

                return 0;
	}
	int j,k;
	int nx;

	Mem2d *m = scan->mem2d;
	if (y1 >= m->GetNy())  // Py 20130820
	        return 0;

	if ((y1%QuenchFac) == 0){
		nx=m->GetNx ();

		if (cmode == NATIVE_4L_RGBA && m->GetNv () == 4)
			for (j=y1; j<y2; j+=QuenchFac)
				for (k=0; k<nx; k+=QuenchFac){
					m->SetLayer (0);
					unsigned long r = m->GetDataVMode (k,j);
					m->SetLayer (1);
					unsigned long g = m->GetDataVMode (k,j);
					m->SetLayer (2);
					unsigned long b = m->GetDataVMode (k,j);
					XImg->PutPixel_RGB ((unsigned long)(k/QuenchFac),(unsigned long)(j/QuenchFac), r,g,b);
				}
		else 
			if (scan->x_linearize ()){
				double xl, xr, dx, x;
				xl = m->data->GetXLookup (0);
				xr = m->data->GetXLookup (nx-1);
                                //if (xl > xr) { xl = xr; xr = m->data->GetXLookup (0); } // make sure left is left!
				dx = (xr-xl)/((double)(nx-1)*QuenchFac);
                                // g_message ("scan->x_linearize active xl0= %g A -> xrN= %g A dx=%g xl->i=%d", xl ,xr, dx,  m->data->i_from_XLookup(xl));
				for (x=xl, k=0; k<nx; k+=QuenchFac, x+=dx){
					int kl = m->data->i_from_XLookup(x);
					for (j=y1; j<y2; j+=QuenchFac)
						XImg->PutPixel ((unsigned long)(k/QuenchFac),(unsigned long)(j/QuenchFac),
								m->GetDataVMode (kl, j));
				}
			} else {
                                // g_message ("scan->x_linearize not active");
				for (j=y1; j<y2; j+=QuenchFac)
					for (k=0; k<nx; k+=QuenchFac)
						XImg->PutPixel ((unsigned long)(k/QuenchFac),(unsigned long)(j/QuenchFac),
								m->GetDataVMode (k,j));
			}

                //XSM_DEBUG_GM (DBG_L3,  "Grey2D::update** B");
		if (scan->ixy_sub[1]>0)
                        XImg->ShowSubPic(scan->ixy_sub[0]/QuenchFac, y1/QuenchFac,scan->ixy_sub[1]/QuenchFac,(y2+QuenchFac-y1-1)/QuenchFac, scan->ixy_sub[2]/QuenchFac, scan->ixy_sub[3]/QuenchFac);
                else
                        XImg->ShowSubPic(0, y1/QuenchFac,nx/QuenchFac,(y2+QuenchFac-y1-1)/QuenchFac);
	}
	
	// Show Red Line ...
	--y2;
	if (y1 == y2 && viewcontrol){
		scan->Pkt2dScanLine[0].x = 0;
		scan->Pkt2dScanLine[0].y = y1;
		scan->Pkt2dScanLine[1].x = m->GetNx () - 1;
		scan->Pkt2dScanLine[1].y = y1;
//		XImg->update_bbox ((y1-1)/QuenchFac, (y1+1)/QuenchFac);
		viewcontrol -> CheckRedLine ();
	}
        
        //viewcontrol -> force_redraw ();
	return 0;
}

void Grey2D::add_object(int type, gpointer data){
        if (viewcontrol){
                double *xy = (double*) data;
                viewcontrol -> AddObject(
                                         new VObPolyLine (viewcontrol->GetCanvas(),
                                                          xy,
                                                          FALSE, VOBJ_COORD_ABSOLUT)
                                         );
        }
}

void Grey2D::update_events(){
	if (viewcontrol) 
		viewcontrol->events_update (); 
}

void Grey2D::update_event_info(ScanEvent* se){
	if (viewcontrol) 
                viewcontrol->update_event_panel (se);
}

void Grey2D::ZoomIn(){
	if(QuenchFac>1)
		--QuenchFac;
	else
		++ZoomFac;
	draw(TRUE);
}

void Grey2D::ZoomOut(){
	if(ZoomFac>1)
		--ZoomFac;
	else
		++QuenchFac;
	draw(TRUE);
}

int Grey2D::SetZF(int zf, int qf, Grey2D *p){
        if (!zf && !qf){
                p->userzoom = USER_ZOOM_AUTO;
                p->draw();
                return 0;
        }
        if (zf==1 && !qf){
                p->userzoom = USER_ZOOM_WIDTH;
                p->draw();
                return 0;
        }
	if(!zf){
		p->ZoomIn();
		return 0;
	}
	if(!qf){
		p->ZoomOut();
		return 0;
	}
	p->ZoomFac = zf;
	p->QuenchFac = qf;
	p->userzoom = USER_ZOOM_FIXED;
	p->draw();
	return 0;
}

void Grey2D::setup_data_transformation(){
	if (XImg)
		MaxColor=XImg->GetMaxCol();
	else 
		MaxColor=64;
	update_mxyz_from_points ();
	scan->mem2d->SetDataPktMode (data->display.ViewFlg);
	scan->mem2d->SetDataRange (0, MaxColor-1);
}


int Grey2D::draw(int zoomoverride){
	int nx,ny;
	gchar *title=NULL;
	gchar *subtitle=NULL;
  
	int NewFlg=0;

	if(!scan->mem2d) { 
		XSM_DEBUG (DBG_L2, "Grey2D: no mem2d !"); 
		return 1; 
	}

	nx=scan->mem2d->GetNx();
	ny=scan->mem2d->GetNy();
      
	XSM_DEBUG (DBG_L2, "NX: " << nx << " NY: " << ny);
	if(nx<1 || ny<1) return 1;

        if(!zoomoverride && !userzoom){
		ZoomFac = 1;
		QuenchFac = 1;
		if(main_get_gapp ()->xsm->ZoomFlg & VIEW_ZOOM){
			if(main_get_gapp ()->xsm->ZoomFlg & VIEW_Z400)
				ZoomFac   = 400/nx;
			if(main_get_gapp ()->xsm->ZoomFlg & VIEW_Z600)
				ZoomFac   = 600/nx;
			QuenchFac = nx/400;
      
			if(ZoomFac   < 1 )
				ZoomFac = 1;
			if(ZoomFac > XSHMIMG_MAXZOOM )
				ZoomFac = XSHMIMG_MAXZOOM;
			if(QuenchFac < 1 )
				QuenchFac = 1;
		}
	}
  
	XPM_x = nx*ZoomFac/QuenchFac + (QuenchFac > 1 ? 1:0);
	XPM_y = ny*ZoomFac/QuenchFac + (QuenchFac > 1 ? 1:0);

	if(oXPMx != XPM_x || oXPMy != XPM_y || oQ != QuenchFac || oZ != ZoomFac 
	   || oMC != MaxColor || (oVm & VIEW_INFO) != (main_get_gapp ()->xsm->ZoomFlg & VIEW_INFO) ){
		if(!zoomoverride){
			oXPMx=XPM_x;
			oXPMy=XPM_y;
		}
		oZ=ZoomFac;
		oQ=QuenchFac;
		oMC=MaxColor;
		oVm=main_get_gapp ()->xsm->ZoomFlg;
		NewFlg  = 1;
	}

	if (ChanNo < 0){
		title = g_strdup_printf("Momentary Grey 2D View * Q%d/%d", ZoomFac,QuenchFac);
		subtitle = g_strdup_printf("%s", scan->mem2d->GetEname());
	} else {
		gchar **path_split=g_strsplit (data->ui.name, "/", 0);
		gchar *name = NULL;
		
		for (gchar **x=path_split; *x; ++x)
			name = *x;

		if (strcmp(data->ui.originalname, "unknown (not saved)") == 0)
			name = data->ui.name;

		title = g_strdup_printf("Ch%d:%d:%d%s %s Q%d/%d %s", 
					ChanNo+1, scan->mem2d->get_t_index()+1, scan->mem2d->GetLayer()+1,
					strcmp(data->ui.originalname, "unknown (not saved)") == 0 ? " *NOT-SAVED* :" : ":",
					name ? name : "-?-", ZoomFac,QuenchFac, scan->mem2d->GetEname());
		subtitle = g_strdup_printf("%s", data->ui.originalname);
		g_strfreev (path_split);
	}
		
	if(NewFlg){ // new size ?
		XSM_DEBUG (DBG_L2, "Grey2D::draw ResizeVC");
		if(viewcontrol)
			viewcontrol->Resize(title, XPM_x , XPM_y, ChanNo, scan, ZoomFac, QuenchFac);
		NewFlg=0;
	}

	if(!viewcontrol){
                if (!scan->get_app ()){
                        g_warning ("ERROR Grey2D::DRAW -- invalid request: no app reference provided.");
                        return -1;
                }
		XSM_DEBUG (DBG_L2, "Grey2D::draw NewVC");
		viewcontrol =  new ViewControl(scan->get_app (), title, XPM_x , XPM_y, ChanNo, scan, ZoomFac, QuenchFac);
		viewcontrol->SetZoomQFkt(SetZF, this);
		viewcontrol->show();
		XImg = viewcontrol->GetXImg();
	}

        viewcontrol->update_XYpixshift();

	if(ChanNo == main_get_gapp ()->xsm->ActiveChannel)
		viewcontrol->SetActive(TRUE);
	else
		viewcontrol->SetActive(FALSE);

	switch (cmode){
	case DEFAULT_GREY:     XImg->MkPalette (); break;
	case USER_FALSE_COLOR: XImg->MkPalette (xsmres.Palette); break;
	case NATIVE_4L_RGBA:   XImg->MkPalette ("NATIVE_L4_RGBA"); break;
	default: XImg->MkPalette (); break;
	}
    
	MaxColor=XImg->GetMaxCol();

//  XSM_DEBUG (DBG_L2, "Grey2D::draw update 0..ny=" << ny << " MaxCol=" << MaxColor);
	BenchStart(vbm,"View Update All",nx*ny);
	update(0, ny);
	BenchStop(vbm);

	viewcontrol->SetTitle (title, subtitle);  
	g_free(title);
	g_free(subtitle);

// Update Overlay if info is existing

	gchar *ly_info = scan->mem2d->get_layer_information (0);
	if (ly_info){
		int jj=0;
		while (ly_info){
			viewcontrol->set_osd (ly_info, jj);
			g_free (ly_info);
			ly_info = scan->mem2d->get_layer_information (++jj);
		}
	}
	XSM_DEBUG (DBG_L5,"Grey2D::draw ret");

	viewcontrol->UpdateObjects ();

	return 0;
}

void Grey2D::show_tip(){
}

void Grey2D::hide_tip(){
}


void Grey2D::show_trace(){
	update_trace();
}

void Grey2D::hide_trace(){
	if (viewcontrol) 
                viewcontrol->remove_trace ();
}

void Grey2D::update_trace(){
        //	if (viewcontrol) 
        //	viewcontrol->update_trace (double *xy, int len);
}

void Grey2D::remove_events(){
	if (viewcontrol){
		viewcontrol->RemoveEventObjects ();
		scan->mem2d->RemoveScanEvents ();
	}
}

gpointer Grey2D::grab_OSD_canvas(){
        return GTK_WIDGET (viewcontrol->grab_canvas());
}
