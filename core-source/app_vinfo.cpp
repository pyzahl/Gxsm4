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
#include "glbvars.h"

#include "app_vinfo.h"
#include "clip.h"

#include "scan.h"

ViewInfo::ViewInfo(Scan *Sc, int qf, double zf){
  Qfac = qf; Zfac = zf;
  sc=Sc; 
  ux=uy=uz=NULL; // use "Scan" Units
  userzoommode=0;
  
  EnableTimeDisplay (FALSE);
  SetPixelUnit (FALSE);
  SetCoordMode ();
}

UnitObj *ViewInfo::Ux(){
	return ux ? ux : sc->data.Xunit; 
}
UnitObj *ViewInfo::Uy(){ 
        return uy ? uy : sc->data.Yunit;
}
UnitObj *ViewInfo::Uz(){ 
	return uz ? uz : sc->data.Zunit; 
}

gchar *ViewInfo::makeXinfo(double x){ 
	x *= Qfac;
	if (x<0. || x >= sc->mem2d->GetNx())
		return g_strdup ("x out of range");
	if (pixelmode)
		return Ux ()->UsrString(x);
	return Ux()->UsrString (sc->GetWorldX ((int)x)); // only relative mode!
}

gchar *ViewInfo::makeDXYinfo (double xy1a[2], double xy2a[2], double factor){
	double xy1[2], xy2[2], dx, dy;
	gchar  *infostring = NULL;
	memcpy (xy1, xy1a, sizeof(xy1));
	memcpy (xy2, xy2a, sizeof(xy2));
	xy1[0] *= Qfac; xy1[1] *= Qfac;
	xy2[0] *= Qfac; xy2[1] *= Qfac;

	if (cohen_sutherland_line_clip_d (&xy1[0], &xy1[1], &xy2[0], &xy2[1],
				      0.,0.,
				      (double)sc->mem2d->GetNx()-1, (double)sc->mem2d->GetNy()-1)
	    != NotClipped)
		infostring = g_strdup (", line was clipped! ");

	if(pixelmode){
		dx = xy2[0]-xy1[0];
		dy = xy2[1]-xy1[1];
	}else{
		double x1,x2,y1,y2;
		sc->Pixel2World ((int)xy1[0], (int)xy1[1], x1, y1, SCAN_COORD_RELATIVE);
		sc->Pixel2World ((int)xy2[0], (int)xy2[1], x2, y2, SCAN_COORD_RELATIVE);
		dx = x2 - x1;
		dy = y2 - y1;
	}

	gchar *tmp=infostring, *tmp2;
	infostring = g_strconcat (tmp2 = Ux()->UsrString(factor * sqrt(dx*dx + dy*dy)), tmp, NULL);
	g_free (tmp);
	g_free (tmp2);

	return infostring;
}

gchar *ViewInfo::makeDnXYinfo(double *xy, int n){
	double *xyc = new double[2*n];
	double dx,dy;

	for (int i=0; i<2*n; ++i)
		xyc[i] = xy[i]*Qfac;

	dx = dy = 0.;
	if(pixelmode){
		for (int i=2; i<2*n; i+=2){
			dx += fabs (xyc[i] - xyc[i-2]);
			dy += fabs (xyc[i+1] - xyc[i-1]);
		}
	}else{
		double x1,x2,y1,y2;
		for (int i=2; i<2*n; i+=2){
			sc->Pixel2World ((int)xyc[i-2], (int)xyc[i-1], x1, y1, SCAN_COORD_RELATIVE);
			sc->Pixel2World ((int)xyc[i], (int)xyc[i+1], x2, y2, SCAN_COORD_RELATIVE);
			dx += fabs (x2 - x1);
			dy += fabs (y2 - y1);
		}
	}
	delete[] xyc;

	return Ux ()->UsrString (sqrt (dx*dx + dy*dy));
}


gchar *ViewInfo::makeA2info(double xy1a[2], double xy2a[2]){
	double xy1[2], xy2[2], dx, dy;
	memcpy(xy1, xy1a, sizeof(xy1));
	memcpy(xy2, xy2a, sizeof(xy2));
	xy1[0] *= Qfac; xy1[1] *= Qfac;
	xy2[0] *= Qfac; xy2[1] *= Qfac;
	if(xy1[0]<0. || xy1[0] >= sc->mem2d->GetNx() || xy2[0]<0. || xy2[0] >= sc->mem2d->GetNx())
		return g_strdup("x out of range");
	if(xy1[1]<0. || xy1[1] >= sc->mem2d->GetNx() || xy2[1]<0. || xy2[1] >= sc->mem2d->GetNx())
		return g_strdup("y out of range");
	if(pixelmode)
		return Ux()->UsrStringSqr((xy2[0]-xy1[0]) * (xy2[1]-xy1[1]) );

	{
		double x1,x2,y1,y2;
		sc->Pixel2World ((int)xy1[0], (int)xy1[1], x1, y1, SCAN_COORD_RELATIVE);
		sc->Pixel2World ((int)xy2[0], (int)xy2[1], x2, y2, SCAN_COORD_RELATIVE);
		dx = x2 - x1;
		dy = y2 - y1;
	}
	return Ux()->UsrStringSqr (dx*dy);
}

gchar *ViewInfo::makeXYinfo(double x, double y){ 
	double mx = x*Qfac, xx;
	double my = y*Qfac, yy;
	xx = R2INT(mx); xx=MIN(sc->mem2d->GetNx()-1, MAX(0,xx));
	yy = R2INT(my); yy=MIN(sc->mem2d->GetNy()-1, MAX(0,yy));
	
	if(!pixelmode)
		sc->Pixel2World ((int)xx, (int)yy, mx, my, sc_mode);
	
	gchar *px  = Ux()->UsrString(mx);
	gchar *py  = Uy()->UsrString(my);
	gchar *pxy = g_strconcat("(", px, ", ", py, ")", NULL);
	g_free(py);
	g_free(px);
	return pxy;
}


gchar *ViewInfo::makeZinfo(double data_z, const gchar *new_prec, double sub){
        UnitObj *u = Uz();
        UnitObj tmp(*u);
        if (new_prec)
                tmp.ChangePrec (new_prec);
        return tmp.UsrString (data_z*(pixelmode ? 1. : sc->data.s.dz) - sub); 
}

double ViewInfo::getZ(double x, double y){ 
        double mx = x*Qfac, xx;
        double my = y*Qfac, yy;
        //double my = y*Qfac/as_pixy, yy;  // TDB -- only used in "score_bond" so far
        xx = R2INT(mx); xx=MIN(sc->mem2d->GetNx()-1, MAX(0,xx));
        yy = R2INT(my); yy=MIN(sc->mem2d->GetNy()-1, MAX(0,yy));
        int ix=(int)xx, iy=(int)yy;
        return sc->mem2d->GetDataPkt(ix,iy)*sc->data.s.dz - sc->data.s.pllref;
}
      
// convert X,Y (view, mouse) to image index
void ViewInfo::XYview2pixel(double x, double y, Point2D *p){
        double mx = x*Qfac, xx;
        double my = y*Qfac/as_pixy, yy;
        xx = R2INT(mx); xx=MIN(sc->mem2d->GetNx()-1, MAX(0,xx));
        yy = R2INT(my); yy=MIN(sc->mem2d->GetNy()-1, MAX(0,yy));
        p->x = R2INT(xx);
        p->y = R2INT(yy);
}


gchar *ViewInfo::makedXdYinfo(double xy1a[2], double xy2a[2]){ 
	double dmx, dmy;
	if(pixelmode){
		dmx = (xy2a[0]-xy1a[0])*Qfac;
		dmy = (xy2a[1]-xy1a[1])*Qfac;
	}else{
		double xy1[2], xy2[2];
		memcpy(xy1, xy1a, sizeof(xy1));
		memcpy(xy2, xy2a, sizeof(xy2));
		xy1[0] *= Qfac; xy1[1] *= Qfac;
		xy2[0] *= Qfac; xy2[1] *= Qfac;
		if(xy1[0]<0. || xy1[0] >= sc->mem2d->GetNx() || xy2[0]<0. || xy2[0] >= sc->mem2d->GetNx())
			return g_strdup("x out of range");
		if(xy1[1]<0. || xy1[1] >= sc->mem2d->GetNx() || xy2[1]<0. || xy2[1] >= sc->mem2d->GetNx())
			return g_strdup("y out of range");

		{
			double x1,x2,y1,y2;
			sc->Pixel2World ((int)xy1[0], (int)xy1[1], x1, y1, SCAN_COORD_RELATIVE);
			sc->Pixel2World ((int)xy2[0], (int)xy2[1], x2, y2, SCAN_COORD_RELATIVE);
			dmx = x2 - x1;
			dmy = y2 - y1;
		}
	}
	gchar *px  = Ux()->UsrString(dmx);
	gchar *py  = Uy()->UsrString(dmy);
	gchar *pxy = g_strconcat("d(", px, ", ", py, ")", NULL);
	g_free(py);
	g_free(px);
	return pxy;
}

gchar *ViewInfo::makeXYZinfo(double x, double y){ 
	double mx = x*Qfac, xx;
	double my = y*Qfac, yy;
	gchar *us = NULL;
	gchar *pt = NULL;
	xx = R2INT(mx); xx=MIN(sc->mem2d->GetNx()-1, MAX(0,xx));
	yy = R2INT(my); yy=MIN(sc->mem2d->GetNy()-1, MAX(0,yy));

	int ix=(int)xx, iy=(int)yy;

	if(!pixelmode)
		sc->Pixel2World (ix, iy, mx, my, sc_mode);

	gchar *px  = Ux()->UsrString(mx);
	gchar *py  = Uy()->UsrString(my);
	gchar *pzh = pixelmode ? g_strdup_printf (" 0x%08x N:%d S:%d", (int)sc->mem2d->GetDataPkt(ix,iy),
                                                  sc->mem2d->data->Li[iy].IsNew(),
                                                  sc->mem2d->data->Li[iy].IsStored()
                                                  ) : NULL;
	// ignoring xdir by now, assuming -> for pixel time estimation, also assuming constant scan speed all the time
	// calcpixeltime
	if (showtime){
		double ptv =  (double)sc->data.s.tStart + (2.*sc->mem2d->GetNx()*(sc->data.s.ydir>0?iy:sc->mem2d->GetNy()-iy-1) + ix)*sc->data.s.pixeltime;
		if (sc_mode == SCAN_COORD_ABSOLUTE)
			pt = g_strdup_printf ("; %.1f s)=", ptv);
		else{
			double pdt = ptv-main_get_gapp ()->glb_ref_point_xylt_world[3];
			pt = g_strdup_printf ("; %.1f s (%g, %g)" UTF8_ANGSTROEM "/s)=", pdt,
					      (mx-main_get_gapp ()->glb_ref_point_xylt_world[0])/pdt,
					      (my-main_get_gapp ()->glb_ref_point_xylt_world[1])/pdt
				);
		}
	}
        gchar *pxy = g_strconcat("(", px, ", ", py, showtime ? pt : ")=",
				 (( ix >= 0 && iy >= 0
				    && ix < sc->mem2d->GetNx() && iy < sc->mem2d->GetNy())
				  ? us=Uz()->UsrString(sc->mem2d->GetDataPkt(ix,iy)*(pixelmode ? 1. : sc->data.s.dz))
				  : "out of scan !"),
				 pzh, NULL);
	if(us) g_free(us);
	if (pt) g_free(pt);
	g_free(py);
	g_free(px);
	if (pzh) g_free(pzh);
	return pxy;
}

// absolute Ang to "Canvas World", Qfac applies!!
void ViewInfo::Angstroem2W(double &x, double &y){
	double ix,iy;
	sc->World2Pixel (x, y, ix, iy, SCAN_COORD_ABSOLUTE);
	x = ix/(double)Qfac;
	y = iy/(double)Qfac;
}

double ViewInfo::AngstroemXRel2W(double x){
	return x / sc->data.s.dx / Qfac;
}

// "Canvas World", Qfac applies to absolute Ang!! 
void ViewInfo::W2Angstroem(double &x, double &y){
	double ix = x*Qfac;
	double iy = y*Qfac;
	sc->Pixel2World (ix, iy, x, y, SCAN_COORD_ABSOLUTE);
}

void ViewInfo::dIndex_from_BitmapPix (double &ix, double &iy, double bx, double by) { ix = Qfac*bx; iy = Qfac*by/as_pixy; }
void ViewInfo::BitmapPix_from_dIndex (double &bx, double &by, double ix, double iy) { bx = ix/Qfac; by = iy/Qfac*as_pixy; }
        
