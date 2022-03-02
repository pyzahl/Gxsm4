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

#include <math.h>

#include "xsmmath.h"
#include "xsmtypes.h"
#include "glbvars.h"
#include "surface.h"

#include "bench.h"
#include "regress.h"


void MkMausSelectP(Point2D *Pkt2d, MOUSERECT *msel, int mx, int my){
        // order,clip,calc ranges,..
        mx--; my--;
        msel->xLeft   = MAX(0, MIN( MIN(Pkt2d[0].x, Pkt2d[1].x), mx));
	msel->xRight  = MAX(0, MIN( MAX(Pkt2d[0].x, Pkt2d[1].x), mx));
	msel->xSize   = msel->xRight - msel->xLeft + 1;
	msel->xRatio  = (double)msel->xSize / (double)(mx+1);

	msel->yTop    = MAX(0, MIN( MIN(Pkt2d[0].y, Pkt2d[1].y), my));
	msel->yBottom = MAX(0, MIN( MAX(Pkt2d[0].y, Pkt2d[1].y), my));
	msel->ySize   = msel->yBottom - msel->yTop + 1;
	msel->yRatio  = (double)msel->ySize / (double)(my+1);

	if(msel->ySize > 0)
		msel->Aspect  = (double)msel->xSize / (double)msel->ySize;
	else
		msel->Aspect  = 0.;

	msel->Area    = msel->xSize * msel->ySize;
	msel->Radius2 = (msel->xSize)*(msel->xSize) + (msel->ySize)*(msel->ySize);
	msel->xCenter = msel->xLeft + msel->xSize/2;
	msel->yCenter = msel->yTop  + msel->ySize/2;

	XSM_DEBUG (DBG_L3, "MkMausSelect: (" << msel->xLeft << ", " << msel->yBottom << ")-(" << msel->xRight  << ", " << msel->yTop << ")"); 
	XSM_DEBUG (DBG_L3, "MkMausSelect Size: (" << msel->xSize << ", " << msel->ySize << ")");
}

gint MkMausSelect(Scan *sc, MOUSERECT *msel, int mx, int my){
        // convenience wrapper to get corners/area of current/last rectangle object

        int success = FALSE;
        int n_obj = sc->number_of_object ();
        Point2D Pkt2d[2];

        mx--; my--;
        while (n_obj--){
                scan_object_data *scan_obj = sc->get_object_data (n_obj);
		
                if (strncmp (scan_obj->get_name (), "Rectangle", 9) )
                        continue; // only points are used!

                if (scan_obj->get_num_points () != 2) 
                        continue; // check, must have two coordinates

                scan_obj->get_xy_i_pixel2d (0, &Pkt2d[0]);
                scan_obj->get_xy_i_pixel2d (1, &Pkt2d[1]);
                success = TRUE;
                break;
        }

        if (!success) return success;
        
        msel->xLeft   = MAX(0, MIN( MIN(Pkt2d[0].x, Pkt2d[1].x), mx));
	msel->xRight  = MAX(0, MIN( MAX(Pkt2d[0].x, Pkt2d[1].x), mx));
	msel->xSize   = msel->xRight - msel->xLeft + 1;
	msel->xRatio  = (double)msel->xSize / (double)(mx+1);

	msel->yTop    = MAX(0, MIN( MIN(Pkt2d[0].y, Pkt2d[1].y), my));
	msel->yBottom = MAX(0, MIN( MAX(Pkt2d[0].y, Pkt2d[1].y), my));
	msel->ySize   = msel->yBottom - msel->yTop + 1;
	msel->yRatio  = (double)msel->ySize / (double)(my+1);

	if(msel->ySize > 0)
		msel->Aspect  = (double)msel->xSize / (double)msel->ySize;
	else
		msel->Aspect  = 0.;

	msel->Area    = msel->xSize * msel->ySize;
	msel->Radius2 = (msel->xSize)*(msel->xSize) + (msel->ySize)*(msel->ySize);
	msel->xCenter = msel->xLeft + msel->xSize/2;
	msel->yCenter = msel->yTop  + msel->ySize/2;

	XSM_DEBUG (DBG_L3, "MkMausSelect: (" << msel->xLeft << ", " << msel->yBottom << ")-(" << msel->xRight  << ", " << msel->yTop << ")"); 
	XSM_DEBUG (DBG_L3, "MkMausSelect Size: (" << msel->xSize << ", " << msel->ySize << ")");

        return success;
}

//
// a template for implementing new filters.
//
// pattern filter "ZERO EFFECT"
//==============================
// MATHOPPARAMS:  
// Scan *Src:  Datasource
// Scan *Dest: Datatarget
//
// ===============================
// *Dest is a new empty scan with the same parameters like *Src !
// *Src shall NEVER be altered !!!!
//
// Size:
// ===============================
// nx = Dest->mem2d->GetNx(), ... GetNy()
// Dest->Resize(Dest->data.s.nx, Dest->data.s.ny) :  Change size of destination scan.
//
// Dataaccess via Mem2d object:
// ===============================
// * Copying a rectangular area (type src == type dest):
// Dest->mem2d->CopyFrom(Src->mem2d, int x0, int y0, int tox, int toy, Dest->mem2d->GetNx(),Dest->mem2d->GetNy());
// * Copying a rectangular area eventually including type conversion.
// Dest->mem2d->ConvertFrom(Src->mem2d, int x0, int y0, int tox, int toy, Dest->mem2d->GetNx(),Dest->mem2d->GetNy());
//
//
// [[ Src->mem2d->GetDataLine(int line, ZTYPE *buffer)   :    full line ]] please do not use !!
// double value = Src->mem2d->GetDataPkt(int x, int y) :    single point
// ... or faster:
// double value = Src->mem2d->data->Z(int x, int y) :    single point
// value = Src->mem2d->GetDataPkt(int x, int y) :    single point
// value = Src->mem2d->GetDataPktLineReg(int x, int y) :    single point with line regression 
// [[ Dest->mem2d->PutDataLine(int line, ZTYPE *buffer)  :    full line ]] please do not use !!
// Dest->mem2d->PutDataPkt(ZTYPE value, int x, int y) :    single point
// ... or faster, skipping check of lineregress parameters.
// Src->mem2d->data->Z(double value, int x, int y) :    single point
//
// for large Data Transfer use:
// ============================================================
// for same types:
// inline int CopyFrom(Mem2d *src, int x, int y, int tox, int toy, int nx, int ny=1)
//
// for different types:
// inline int ConvertFrom(Mem2d *src, int x, int y, int tox, int toy, int nx, int ny=1)
//
// for quick linear data access of elements in one line use:
// ============================================================
// void   mem2d->data->SetPtr(int x, int y) to set internal pointer to x,y  -- no range check !!!
// double mem2d->data->GetNext() to access Element x,y and point to next one in line y -- no range check !!!
// double mem2d->data->SetNext(double z) to access Element x,y and point to next one in line y -- no range check !!!
//
/* For example use constructs like:
   ZData  *SrcZ, *DestZ;

   SrcZ  =  Src->mem2d->data;
   DestZ = Dest->mem2d->data;

   for ( line=0; line < Dest->data.s.ny; line++){ 
   DestZ->SetPtr(0, line);
   SrcZ ->SetPtr(0, line);
   DestZ->SetNext(SrcZ->GetNext() + ....));
   }

   or a advanced example:  => F2D_LineInterpol(MATHOPPARAMS) 

*/
// Parameter:
// ===============================
// Dest->data.s.xxx:  Scan parameter structure
// Src->Pkt2d[0..15]: mouse pixels


// uses new Object methods now !
gboolean CopyScan(MATHOPPARAMS){

	Dest->data.copy (Src->data);

	// make sure nx,ny match to memory object
	Dest->data.s.nx = Src->mem2d->GetNx();
	Dest->data.s.ny = Src->mem2d->GetNy();

	if (Src->data.s.ntimes == 1 && Src->mem2d->GetNv () == 1){
		Dest->mem2d->copy(Src->mem2d);
		Dest->data.s.ntimes  = 1;
		Dest->data.s.nvalues = 1;
	} else {
		int ti,tf, vi,vf, tnadd, vnadd;
		do {
			tnadd=vnadd=1;
			ti=vi=0;
			tf=Src->number_of_time_elements ()-1;
			if (tf < 0) tf = 0;
			vf=Src->mem2d->GetNv ()-1;
			main_get_gapp ()->setup_multidimensional_data_copy ("Multidimensional Copy", Src, ti, tf, vi, vf, &tnadd, &vnadd);
		} while (ti > tf || vi > vf || tnadd<1 || vnadd<1);

		main_get_gapp ()->progress_info_new ("Multidimenssional Copy", 1);
		main_get_gapp ()->progress_info_set_bar_fraction (0., 1);
		main_get_gapp ()->progress_info_set_bar_text ("Time", 1);
		
		int ntimes_tmp = tf-ti+1;
		for (int time_index=ti; time_index <= tf; ++time_index){
			Mem2d *m = Src->mem2d_time_element (time_index);
			main_get_gapp ()->progress_info_set_bar_fraction ((gdouble)(time_index-ti)/(gdouble)ntimes_tmp, 1);
			Dest->mem2d->copy(m, -1, -1, vi, vf, -1, -1, true);
			if (tnadd > 1 || vnadd > 1){
				for (int time_index_ahead = time_index; time_index_ahead < time_index+tnadd; ++time_index_ahead){
					// simply limit to end, to preseve statistics
					int tmp_ti = time_index_ahead >= Src->number_of_time_elements () ? Src->number_of_time_elements ()-1 : time_index_ahead;
					Mem2d *ms = Src->mem2d_time_element (tmp_ti);
					for (int v=vi; v<=vf; ++v){
						Dest->mem2d->SetLayer (v);
						std::cout << "Processing v,v+vnadd:" << v << ", " << (v+vnadd) << std::endl;
						for (int v_ahead = v; v_ahead < v + vnadd; ++v_ahead){
							std::cout << "v,v_ahead:" << v << ", " << v_ahead << std::endl;
							int tmp_v = v_ahead >= Src->mem2d->GetNv () ? Src->mem2d->GetNv ()-1 : v_ahead;
							ms->SetLayer (tmp_v);
							if (tmp_v == v && tmp_ti == time_index) {
								std::cout << "Self, skipping. tv:[" << time_index << ", " << v << "], to add[" << tmp_ti << ", " << tmp_v <<  "]" << std::endl;
								continue;
							} else {
								std::cout << "Adding Data.    tv:[" << time_index << ", " << v << "], to add[" << tmp_ti << ", " << tmp_v <<  "]" << std::endl;
								Dest->mem2d->data->ZFrameAddDataFrom (ms->data);
                                                                Dest->mem2d->data->SetVLookup (v, (double)v);
							}
						}
					}
				}
			}
			Dest->append_current_to_time_elements (time_index-ti, m->get_frame_time ());
		}
		Dest->data.s.ntimes = ntimes_tmp;
		Dest->data.s.nvalues=Dest->mem2d->GetNv ();

		main_get_gapp ()->progress_info_close ();
		Dest->retrieve_time_element (0);
		Dest->mem2d->SetLayer(0);
	}
	return MATH_OK;
}

// Cut out area
gboolean CropScan(MATHOPPARAMS){
	MOUSERECT msr;
	XSM_DEBUG (DBG_L3, "Crop Scan");

	Dest->data.copy (Src->data);
	MkMausSelect (Src, &msr, Src->mem2d->GetNx(), Src->mem2d->GetNy());
    
	if( msr.xSize  < 1 || msr.ySize < 1){
		XSM_DEBUG (DBG_L3, "Crop:" << msr.xSize << " " << msr.ySize);
		return MATH_SELECTIONERR;
	}

	if(main_get_gapp ()->xsm->MausMode() != MRECTANGLE && main_get_gapp ()->xsm->MausMode() != MCIRCLE)
		return MATH_SELECTIONERR;

	Dest->mem2d->Resize (msr.xSize, msr.ySize);
	Dest->data.s.nx = msr.xSize;
	Dest->data.s.ny = msr.ySize;
	Dest->data.s.x0 += (msr.xLeft + msr.xSize/2 - Src->data.s.nx/2)*Src->data.s.dx;
	if (Src->data.orgmode == SCAN_ORG_CENTER)
		Dest->data.s.y0 -= (msr.yTop + msr.ySize/2 - Src->data.s.ny/2)*Src->data.s.dy;
	else
		Dest->data.s.y0 -= msr.yTop*Src->data.s.dy;

	// Adapt to next possible value  
	Dest->data.s.rx = Dest->mem2d->GetNx () * Dest->data.s.dx;
	Dest->data.s.ry = Dest->mem2d->GetNy () * Dest->data.s.dy;
    

        g_message ("CROP: SRC(%d,%d, %d, %d)",
                   Src->data.s.nx, Src->data.s.ny, Src->data.s.nvalues, Src->data.s.ntimes);
        g_message ("CROP: NEW(%d,%d) = from [(%d,%d) : (%d,%d)]",
                   Dest->data.s.nx, Dest->data.s.ny,
                   msr.xLeft, msr.yTop,
                   msr.xRight, msr.yBottom);
        
	if (Src->data.s.ntimes == 1 && Src->mem2d->GetNv () == 1){
		Dest->mem2d->CopyFrom (Src->mem2d, msr.xLeft,msr.yTop,
                                       0,0, Dest->mem2d->GetNx (), Dest->mem2d->GetNy ());
	} else {
		int Radius = 0;
		int ti,tf, vi,vf;
		int xyc[4];

		if (main_get_gapp ()->xsm->MausMode() == MCIRCLE){ // crop circle
			Radius = (int)round (sqrt ((double)msr.Radius2)); 
			Dest->data.s.nx = 2*Radius;
			Dest->data.s.ny = 2*Radius;
			xyc[0] = (int)msr.xCenter-Radius;
			xyc[1] = (int)msr.yCenter-Radius;
			xyc[2] = xyc[0]+Dest->data.s.nx;
			xyc[3] = xyc[1]+Dest->data.s.ny;
		} else {
			xyc[0] = msr.xLeft;
			xyc[1] = msr.yTop;
			xyc[2] = msr.xRight;
			xyc[3] = msr.yBottom;
		}
		do {
			ti=vi=0;
			tf=Src->number_of_time_elements ()-1;
			if (tf < 0) tf = 0;
			vf=Src->mem2d->GetNv ()-1;
			main_get_gapp ()->setup_multidimensional_data_copy ("Multidimensional Crop", Src, ti, tf, vi, vf, NULL, NULL, xyc, TRUE);
		} while (ti > tf || vi > vf || xyc[0] >= xyc[2] || xyc[1] >= xyc[3]);
			
		main_get_gapp ()->progress_info_new ("Multidimenssional Crop", 1);
		main_get_gapp ()->progress_info_set_bar_fraction (0., 1);
		main_get_gapp ()->progress_info_set_bar_text ("Time", 1);
			
		int ntimes_tmp = tf-ti+1;
		for (int time_index=ti; time_index <= tf; ++time_index){
			Mem2d *m = Src->mem2d_time_element (time_index);
			main_get_gapp ()->progress_info_set_bar_fraction ((gdouble)(time_index-ti)/(gdouble)ntimes_tmp, 1);
			Dest->mem2d->copy(m, xyc[0], xyc[1], vi, vf, xyc[2]-xyc[0]+1, xyc[3]-xyc[1]+1, true);

			if (main_get_gapp ()->xsm->MausMode() == MCIRCLE){ // crop circle
				double r2 = Radius * Radius;

				// Adapt to next possible value  
				Dest->data.s.rx = Dest->data.s.nx*Dest->data.s.dx;
				Dest->data.s.ry = Dest->data.s.ny*Dest->data.s.dy;
    
				for(int line = 0; line < Dest->mem2d->GetNy (); line++)
					for(int col = 0; col < Dest->mem2d->GetNx (); col++){
						double x=(double)(col-Radius);
						double y=(double)(line-Radius);
						if (x*x + y*y > r2)
							for (int v = vi; v <= vf; ++v)
								Dest->mem2d->PutDataPkt(0., col, line, v);
					}
			}
			Dest->append_current_to_time_elements (time_index-ti, m->get_frame_time ());
		}
		Dest->data.s.ntimes = ntimes_tmp;
		Dest->data.s.nvalues=Dest->mem2d->GetNv ();
			
		main_get_gapp ()->progress_info_close ();
		Dest->retrieve_time_element (0);
		Dest->mem2d->SetLayer(0);
	}

	return MATH_OK;
}

// Half size , Average 2x2 
// Essential for Gxsm core!!!
gboolean TR_QuenchScan(MATHOPPARAMS){
	long col, line;
	ZData  *SrcZ, *SrcZn, *DestZ;
	double val;
	XSM_DEBUG (DBG_L3, "Quench Scan");

	SrcZ  =  Src->mem2d->data;

	DestZ = Dest->mem2d->data;

	Dest->data.s.nx = (Src->data.s.nx/2) & ~1;
	Dest->data.s.dx = Src->data.s.dx*2.;
	Dest->data.s.ny = (Src->data.s.ny/2) & ~1;
	Dest->data.s.dy = Src->data.s.dy*2.;
	Dest->mem2d->Resize(Dest->data.s.nx, Dest->data.s.ny);

	Mem2d NextLine(Src->mem2d->GetNx(), 1, Src->mem2d->GetTyp());
	SrcZn = NextLine.data;
	for(line = 0; line < Dest->data.s.ny; line++){
		NextLine.CopyFrom(Src->mem2d, 0,2*line+1, 0,0, Src->mem2d->GetNx());
		DestZ->SetPtr(0, line);
		SrcZ ->SetPtr(0, 2*line);
		SrcZn->SetPtr(0, 0);
		for(col = 0; col < Dest->data.s.nx; col++){
			val  = SrcZ ->GetNext();
			val += SrcZ ->GetNext();
			val += SrcZn->GetNext();
			val += SrcZn->GetNext();
			val /= 4.;
			DestZ->SetNext(val);
		}
	}

	return MATH_OK;
}

// zoom into area 
gboolean ZoomInScan(MATHOPPARAMS){
	long col, line;
	MOUSERECT msr;
	double FXY, facx, facy;

	MkMausSelect (Src, &msr, Dest->mem2d->GetNx(), Dest->mem2d->GetNy());

	if( msr.xSize  < 1 || msr.ySize < 1)
		return MATH_SELECTIONERR;
   
	// Keep aspect ratio and area
	FXY = (double)(Src->data.s.nx*Src->data.s.ny);
	Dest->data.s.nx = (int)(sqrt(FXY * msr.Aspect));
	Dest->data.s.ny = (int)(sqrt(FXY / msr.Aspect));

	Dest->data.s.x0 += (msr.xLeft + msr.xSize/2 - Src->data.s.nx/2)*Src->data.s.dx;
	if (Src->data.orgmode == SCAN_ORG_CENTER)
		Dest->data.s.y0 -= (msr.yTop + msr.ySize/2 - Src->data.s.ny/2)*Src->data.s.dy;
	else
		Dest->data.s.y0 -= msr.yTop*Src->data.s.dy;

	facx = (double)Dest->data.s.nx/msr.xSize;
	facy = (double)Dest->data.s.ny/msr.ySize;
	Dest->data.s.dx /= facx;
	Dest->data.s.dy /= facy;

	Dest->data.s.rx = (Dest->data.s.nx-1)*Dest->data.s.dx;
	Dest->data.s.ry = (Dest->data.s.ny-1)*Dest->data.s.dy;

	Dest->mem2d->Resize(Dest->data.s.nx, Dest->data.s.ny);

	for(line=0; line<Dest->data.s.ny; line++)
		for(col=0; col<Dest->data.s.nx; col++)
			Dest->mem2d->PutDataPkt(Src->mem2d->GetDataPkt((int)(msr.xLeft+col/facx),
								       (int)(msr.yTop+line/facy)),
						col, line);
	return MATH_OK;
}

// zoom out of area (*1/3)
gboolean ZoomOutScan(MATHOPPARAMS){
	int col, line;
	int x0,y0;
	XSM_DEBUG(DBG_L2, "ZoomOut" );

	// keep aspect ratio and area:
	Dest->data.s.nx *= 3;
	Dest->data.s.ny *= 3;
	Dest->mem2d->Resize(Dest->data.s.nx, Dest->data.s.ny);

	// center zoom out
	//  Dest->data.s.x0 += 0;
	if (! (Src->data.orgmode == SCAN_ORG_CENTER))
		Dest->data.s.y0 += Src->data.s.ny*Src->data.s.dy;

	// Adapt to next possible value
	Dest->data.s.rx = Dest->data.s.nx*Dest->data.s.dx;
	Dest->data.s.ry = Dest->data.s.ny*Dest->data.s.dy;

	x0=Src->data.s.nx;
	y0=Src->data.s.ny;
	for(line = 0; line < Src->mem2d->GetNy(); line++)
		for(col = 0; col < Src->mem2d->GetNx(); col++)
			Dest->mem2d->PutDataPkt(Src->mem2d->GetDataPktLineReg(col, line), col+x0, line+y0);
	return MATH_OK;
}

// Src, Dest
gboolean StitchScans(MATHOPPARAMS){

        double dz_adjust = Dest->data.s.dz/Src->data.s.dz;
        Src->mem2d->data->update_ranges (0);
        double z_adjust = 0.;
        //double z_adjust = -( Src->mem2d->data->zcenter*dz_adjust - Dest->mem2d->data->zcenter );
        //if (z_adjust * Dest->data.s.dz > 1.);
        
        double ox = Dest->data.get_x_left_absolute ();
        double oy = Dest->data.get_y_top_absolute ();
        Mem2d mtmp (Dest->mem2d, M2D_COPY);

        // 1. evalute new origin and size
        double left, bottom;
        
	double new_rx = fmax (Src->data.get_x_right_absolute (), Dest->data.get_x_right_absolute ())
                - (left = fmin (Src->data.get_x_left_absolute (), Dest->data.get_x_left_absolute ()));

	double new_ry = fmax (Src->data.get_y_top_absolute (), Dest->data.get_y_top_absolute ())
                - (bottom = fmin (Src->data.get_y_bottom_absolute (), Dest->data.get_y_bottom_absolute ()));

        // set new origin and range
        Dest->data.s.x0 = left + new_rx/2;
	Dest->data.s.y0 = bottom + new_ry/2;

	Dest->data.s.rx = new_rx;
	Dest->data.s.ry = new_ry;

	Dest->data.s.dx = Src->data.s.dx; // copy
	Dest->data.s.dy = Src->data.s.dy; // copy
	Dest->data.s.dz = Src->data.s.dz; // fix, recompute Z if differs!
        // rotation (alpha) -- don't care for now

        // --> keep resolution of Src Scan
        Dest->data.s.nx = std::max(Dest->mem2d->GetNx(), 1+(int)round(Dest->data.s.rx / Src->data.s.dx));
	Dest->data.s.ny = std::max(Dest->mem2d->GetNy(), 1+(int)round(Dest->data.s.ry / Src->data.s.dy));

        g_message ("Stitch: new size: (%g, %g)Ang : (%d, %d)px",
                   Dest->data.s.rx, Dest->data.s.ry,
                   Dest->data.s.nx, Dest->data.s.ny); 

        // 2. resize in place
	Dest->mem2d->Resize (Dest->data.s.nx, Dest->data.s.ny);
        Dest->mem2d->data->MkXLookup (-Dest->data.s.rx/2, Dest->data.s.rx/2);

	switch(Dest->data.orgmode){
	case SCAN_ORG_MIDDLETOP:
		Dest->mem2d->data->MkYLookup (0., -Dest->data.s.ry); break;
	case SCAN_ORG_CENTER:
		Dest->mem2d->data->MkYLookup (Dest->data.s.ry/2, -Dest->data.s.ry/2); break;
	}

        g_message ("Stitch: Dest->mem2d new size: (%d, %d)px",
                   Dest->mem2d->GetNx (), Dest->mem2d->GetNy ());

        
        // 3. stitch/blend
        int ili, ilf, ici, icf;

        Dest->World2Pixel (Src->data.get_x_left_absolute (), Src->data.get_y_top_absolute (), ici, ili);
        Dest->World2Pixel (Src->data.get_x_right_absolute (), Src->data.get_y_bottom_absolute (), icf, ilf);

        g_message ("Stitch: sub range: (%d, %d) - (%d, %d)", ici, ili, icf, ilf);

        // move previous scan data to new origin
        int mvtox, mvtoy;
        g_message ("Stitch: original origin: (%g, %g)Ang", ox, oy);
        Dest->World2Pixel (ox, oy, mvtox, mvtoy);

        int nx2c = mtmp.GetNx ();
        int ny2c = mtmp.GetNy ();
        // auto adjust for rounding / limits
        g_message ("Stitch: relocate original to: (%g, %g)Ang in [%g, %g * %g, %g]Ang => to (%d, %d)px {%d x %d}",
                   ox, oy,
                   Dest->data.get_x_left_absolute (), Dest->data.get_y_bottom_absolute (),
                   Dest->data.get_x_right_absolute (), Dest->data.get_y_top_absolute (),
                   mvtox, mvtoy, nx2c, ny2c);
        
        if (mvtox < 0) mvtox=0;
        if (mvtoy < 0) mvtoy=0;
        if (mvtox+nx2c > Dest->mem2d->GetNx()){
                nx2c = std::min (Dest->mem2d->GetNx ()-mvtox, mtmp.GetNx ());
                g_message ("Adjusting limits nx2c: %d", nx2c);
        }
        if (mvtoy+ny2c > Dest->mem2d->GetNy()){
                ny2c = std::min (Dest->mem2d->GetNy ()-mvtoy, mtmp.GetNy ());
                g_message ("Adjusting limits ny2c: %d", ny2c);
        }

        g_message ("Stitch: relocate original to: (%g, %g)Ang in [%g, %g * %g, %g]Ang => to (%d, %d)px {%d x %d} -- limited",
                   ox, oy,
                   Dest->data.get_x_left_absolute (), Dest->data.get_y_bottom_absolute (),
                   Dest->data.get_x_right_absolute (), Dest->data.get_y_top_absolute (),
                   mvtox, mvtoy, nx2c, ny2c);
        g_message ("Stitch: max: (%d, %d)", mvtox+nx2c, mvtoy+ny2c);
        
        Dest->mem2d->SetData (0., 0,0, nx2c, ny2c);
        Dest->mem2d->CopyFrom (&mtmp, 0,0, mvtox, mvtoy, nx2c, ny2c);
        
        z_adjust = 0.;
        int count = 0;
        for(int line = ili; line < ilf; line++)
		for(int col = ici; col < icf; col++){
                        double x,y;
                        double ix,iy;
                        Dest->Pixel2World (col, line, x,y);
                        Src->World2Pixel (x,y, ix,iy);
                        if (col < 0 || line < 0 || col >= Dest->mem2d->GetNx() || line >= Dest->mem2d->GetNy()){
                                g_message ("WARNING: INDEX OUT OF RANGE [%d, %d] : %g, %g :=> %g %g", col, line, x,y, ix,iy);
                                return MATH_SIZEERR;
                        }
                        if (Dest->mem2d->GetDataPkt (col, line) != 0.){
                                z_adjust += Dest->mem2d->GetDataPkt (col, line) - dz_adjust*Src->mem2d->GetDataPktInterpol (ix,iy);
                                count++;
                        }
                }
        if (count > 1)
                z_adjust /= count;

        
        for(int line = ili; line < ilf-2; line++)
		for(int col = ici; col < icf; col++){
                        double x,y;
                        double ix,iy;
                        Dest->Pixel2World (col, line, x,y);
                        Src->World2Pixel (x,y, ix,iy);
                        if (col < 0 || line < 0 || col >= Dest->mem2d->GetNx() || line >= Dest->mem2d->GetNy()){
                                g_message ("WARNING: INDEX OUT OF RANGE [%d, %d] : %g, %g :=> %g %g", col, line, x,y, ix,iy);
                                return MATH_SIZEERR;
                        }
			Dest->mem2d->PutDataPkt (z_adjust + dz_adjust*Src->mem2d->GetDataPktInterpol (ix,iy), col, line);
                }
        
        return MATH_OK;
}




#if 0  // --> turned into plugins

// Copy Scan, do Line regresssion
gboolean BgLin1DScan(MATHOPPARAMS){
	int line, col;
	XSM_DEBUG (DBG_L3, "BgLin1D Scan");

	int ti=0; 
	int tf=0;
	int vi=0;
	int vf=0;
	gboolean multidim = FALSE;
	
	if (Src->data.s.ntimes != 1 || Src->mem2d->GetNv () != 1){
		multidim = TRUE;
		do {
			ti=vi=0;
			tf=Src->number_of_time_elements ()-1;
			if (tf < 0) tf = 0;
			vf=Src->mem2d->GetNv ()-1;
			main_get_gapp ()->setup_multidimensional_data_copy ("Multidimensional Line Regression", Src, ti, tf, vi, vf);
		} while (ti > tf || vi > vf);

		main_get_gapp ()->progress_info_new ("Multidimenssional Plane Regression", 2);
		main_get_gapp ()->progress_info_set_bar_fraction (0., 1);
		main_get_gapp ()->progress_info_set_bar_fraction (0., 2);
		main_get_gapp ()->progress_info_set_bar_text ("Time", 1);
		main_get_gapp ()->progress_info_set_bar_text ("Value", 2);
	}

	int ntimes_tmp = tf-ti+1;
	for (int time_index=ti; time_index <= tf; ++time_index){
		Mem2d *m = Src->mem2d_time_element (time_index);
		if (multidim)
			main_get_gapp ()->progress_info_set_bar_fraction ((gdouble)(time_index-ti)/(gdouble)ntimes_tmp, 1);

		Dest->mem2d->Resize (Src->mem2d->GetNx (), Src->mem2d->GetNy (), vf-vi+1, Src->mem2d->GetTyp());
		for (int v_index = vi; v_index <= vf; ++v_index){
			m->SetLayer (v_index);
			Dest->mem2d->SetLayer (v_index-vi);


			for(line=0; line<Dest->data.s.ny; line++)
				for(col=0; col<Dest->data.s.nx; col++)
					Dest->mem2d->PutDataPkt(m->GetDataPktLineReg(col, line), col, line);


		}
		Dest->append_current_to_time_elements (time_index-ti, m->get_frame_time ());
	}

	Dest->data.s.ntimes = ntimes_tmp;
	Dest->data.s.nvalues=Dest->mem2d->GetNv ();

	if (multidim){
		main_get_gapp ()->progress_info_close ();
		Dest->retrieve_time_element (0);
		Dest->mem2d->SetLayer(0);
	}

	return MATH_OK;
}

// Do plane regression
gboolean BgERegress(MATHOPPARAMS){
	double sumi, sumiy, sumyq, sumiq, sumy, val;
	double ysumi, ysumiy, ysumyq, ysumiq, ysumy;
	double a, b, n, imean, ymean,ax,bx,ay,by;
	long line, i;
	MOUSERECT msr;

	XSM_DEBUG (DBG_L3, "BgERegress Scan");

	MkMausSelect(Src->Pkt2d, &msr, Dest->mem2d->GetNx(), Dest->mem2d->GetNy());

	if( msr.xSize  < 1 || msr.ySize < 1)
		return MATH_SELECTIONERR;


	int ti=0; 
	int tf=0;
	int vi=0;
	int vf=0;
	gboolean multidim = FALSE;
	
	if (Src->data.s.ntimes != 1 || Src->mem2d->GetNv () != 1){
		multidim = TRUE;
		do {
			ti=vi=0;
			tf=Src->number_of_time_elements ()-1;
			if (tf < 0) tf = 0;
			vf=Src->mem2d->GetNv ()-1;
			main_get_gapp ()->setup_multidimensional_data_copy ("Multidimensional Plane Regression", Src, ti, tf, vi, vf);
		} while (ti > tf || vi > vf);

		main_get_gapp ()->progress_info_new ("Multidimenssional Plane Regression", 2);
		main_get_gapp ()->progress_info_set_bar_fraction (0., 1);
		main_get_gapp ()->progress_info_set_bar_fraction (0., 2);
		main_get_gapp ()->progress_info_set_bar_text ("Time", 1);
		main_get_gapp ()->progress_info_set_bar_text ("Value", 2);
	}

	int ntimes_tmp = tf-ti+1;
	for (int time_index=ti; time_index <= tf; ++time_index){
		Mem2d *m = Src->mem2d_time_element (time_index);
		if (multidim)
			main_get_gapp ()->progress_info_set_bar_fraction ((gdouble)(time_index-ti)/(gdouble)ntimes_tmp, 1);

		Dest->mem2d->Resize (m->GetNx (), m->GetNy (), vf-vi+1, m->GetTyp());
		for (int v_index = vi; v_index <= vf; ++v_index){
			m->SetLayer (v_index);
			Dest->mem2d->SetLayer (v_index-vi);

			sumi = sumiq = sumy = sumyq = sumiy = ax = ay = bx = by = 0.;
			ysumi = ysumiq = ysumy = ysumyq = ysumiy = 0.;
			for (val=msr.xLeft; val < msr.xRight; val += 1.){
				sumiq += val*val;
				sumi  += val;
			}
			n = msr.xSize;
			imean = sumi / n;
			for (line = msr.yTop; line<msr.yBottom; ++line) {
				sumy = sumyq = sumiy = 0.;
				for (i=msr.xLeft; i < msr.xRight; ++i) {
					val = m->GetDataPkt(i, line);
					sumy   += val;
					sumyq  += val*val;
					sumiy  += val*i;
				}
				ymean = sumy / n;
				a = (sumiy- n * imean * ymean ) / (sumiq - n * imean * imean);
				b = ymean - a * imean;
				ax += a;
				bx += b;
				/* Werte fuer y-linregress */
				val = line;
				ysumy  += b;
				ysumyq += b*b;
				ysumiy += b*val;
				ysumiq += val*val;
				ysumi  += val;
			}
			n = msr.ySize;
			ax = ax/n;
			bx = bx/n;
			imean = ysumi / n;
			ymean = ysumy/  n;
			ay = (ysumiy- n * imean * ymean ) / (ysumiq - n * imean * imean);
			by = ymean - ay * imean;
			sumy = sumi = 0.;

			for (line = 0; line<Dest->data.s.ny; ++line, sumy += ay) {
				sumi = by + sumy + .5;
				for (i=0; i < Dest->data.s.nx; ++i, sumi += ax)
					Dest->mem2d->PutDataPkt (m->GetDataPkt (i, line) - sumi,
								 i, line);
			}
		}
		Dest->append_current_to_time_elements (time_index-ti, m->get_frame_time ());
	}

	Dest->data.s.ntimes = ntimes_tmp;
	Dest->data.s.nvalues=Dest->mem2d->GetNv ();

	if (multidim){
		main_get_gapp ()->progress_info_close ();
		Dest->retrieve_time_element (0);
		Dest->mem2d->SetLayer(0);
	}

	return MATH_OK;
}

// Do Parabol regression
gboolean BgParabolRegress(MATHOPPARAMS){
	double sumi, sumiy, sumyq, sumiq, sumy, val;
	double ysumi, ysumiy, ysumyq, ysumiq, ysumy;
	double a, b, n, imean, ymean,ax,bx,ay,by;
	int line, i;
	MOUSERECT msr;

	XSM_DEBUG (DBG_L3, "BgParabolRegress Scan");

	MkMausSelect(Src->Pkt2d, &msr, Dest->mem2d->GetNx(), Dest->mem2d->GetNy());

	if( msr.xSize  < 1 || msr.ySize < 1)
		return MATH_SELECTIONERR;


	// plane regress first

	sumi = sumiq = sumy = sumyq = sumiy = ax = ay = bx = by = 0.;
	ysumi = ysumiq = ysumy = ysumyq = ysumiy = 0.;

	for (val=msr.xLeft; val < msr.xRight; val += 1.){
		sumiq += val*val;
		sumi  += val;
	}
	n = msr.xSize;
	imean = sumi / n;
	for (line = msr.yTop; line<msr.yBottom; ++line) {
		sumy = sumyq = sumiy = 0.;
		for (i=msr.xLeft; i < msr.xRight; ++i) {
			val = Src->mem2d->GetDataPkt(i, line);
			sumy   += val;
			sumyq  += val*val;
			sumiy  += val*i;
		}
		ymean = sumy / n;
		a = (sumiy- n * imean * ymean ) / (sumiq - n * imean * imean);
		b = ymean - a * imean;
		ax += a;
		bx += b;
		/* values for y-linear regression */
		val = line;
		ysumy  += b;
		ysumyq += b*b;
		ysumiy += b*val;
		ysumiq += val*val;
		ysumi  += val;
	}
	n = msr.ySize;
	ax = ax/n;
	bx = bx/n;
	imean = ysumi / n;
	ymean = ysumy/  n;
	ay = (ysumiy- n * imean * ymean ) / (ysumiq - n * imean * imean);
	by = ymean - ay * imean;
	sumy = sumi = 0.;

	for (line = 0; line<Dest->data.s.ny; ++line, sumy += ay) {
		sumi = by + sumy + .5;
		for (i=0; i < Dest->data.s.nx; ++i, sumi += ax)
			Dest->mem2d->PutDataPkt (Src->mem2d->GetDataPkt (i, line) + sumi,
						 i, line);
	}

	// now evaluate 2nd order terms
	double M0, M1, M2, li;
	double D, N;
	double qx0, qx, qxx, qy0, qy, qyy, lb;

	// avg. 2nd order terms in X
	N = (double)Src->mem2d->GetNx();
	D = (N*N-1.0)*(N*N-4.0);
	qx0 = qx = qxx = 0.;
	for (int line=0; line < Src->mem2d->GetNy(); line++) {
		M0 = 0.0; M1 = 0.0; M2 = 0.0;

		for (int col = 0; col < Src->mem2d->GetNx(); col++) {
			li = (double)col;
			M0 += lb = Src->mem2d->GetDataPkt (col, line);
			M1 += lb * li;
			M2 += lb * li*li;
		}

		M0 /= N;
		M1 /= N;
		M2 /= N;

		qx0 += (3.*(N+1.)*(N+2.)/D)*((3.*N*N+3.*N+2.)*M0-6*(2*N+1.)*M1+10.*M2);
		qx  += -6./D*(3*(N+1.)*(N+2.)*(2.*N+1.)*M0-2.*(2.*N+1.)*(8.*N+11.)*M1+30.*(N+1.)*M2);
		qxx += 30./D*((N+1.)*(N+2.)*M0-6.*(N+1.)*M1+6.*M2);
	}
	qx0 /= (double) Src->mem2d->GetNy();
	qx  /= (double) Src->mem2d->GetNy();
	qxx /= (double) Src->mem2d->GetNy();
	 
	// avg. 2nd order terms in Y
        // swaped line/col here
	N = (double)Src->mem2d->GetNy();
	D = (N*N-1.0)*(N*N-4.0);
	qy0 = qy = qyy = 0.;
	for (int line=0; line < Src->mem2d->GetNx(); line++) {
		M0 = 0.0; M1 = 0.0; M2 = 0.0;

		for (int col = 0; col < Src->mem2d->GetNy(); col++) {
			li = (double)col;
			M0 += lb = Src->mem2d->GetDataPkt (line, col);
			M1 += lb * li;
			M2 += lb * li*li;
		}

		M0 /= N;
		M1 /= N;
		M2 /= N;

		qy0 += (3.*(N+1.)*(N+2.)/D)*((3.*N*N+3.*N+2.)*M0-6*(2*N+1.)*M1+10.*M2);
		qy  += -6./D*(3*(N+1.)*(N+2.)*(2.*N+1.)*M0-2.*(2.*N+1.)*(8.*N+11.)*M1+30.*(N+1.)*M2);
		qyy += 30./D*((N+1.)*(N+2.)*M0-6.*(N+1.)*M1+6.*M2);
	}
	qy0 /= (double) Src->mem2d->GetNx();
	qy  /= (double) Src->mem2d->GetNx();
	qyy /= (double) Src->mem2d->GetNx();
	

	for (line = 0; line<Dest->data.s.ny; ++line)
		for (i=0; i < Dest->data.s.nx; ++i, sumi += ax){
			sumi = qx0 + qy0 + qx*i + qy*line + qxx*i*i + qyy*line*line;
			Dest->mem2d->PutDataPkt (Dest->mem2d->GetDataPkt (i, line) - sumi, i, line);
		}

	return MATH_OK;
}
#endif

//======================================================================================
//
// 1D Filter functions 
//
//======================================================================================

#if 0
gboolean F1D_Despike(MATHOPPARAMS){

	XSM_DEBUG (DBG_L3, "F1D Despike");


	int ti=0; 
	int tf=0;
	int vi=0;
	int vf=0;
	gboolean multidim = FALSE;
	
	if (Src->data.s.ntimes != 1 || Src->mem2d->GetNv () != 1){
		multidim = TRUE;
		do {
			ti=vi=0;
			tf=Src->number_of_time_elements ()-1;
			if (tf < 0) tf = 0;
			vf=Src->mem2d->GetNv ()-1;
			main_get_gapp ()->setup_multidimensional_data_copy ("Multidimensional 1D Despike", Src, ti, tf, vi, vf);
		} while (ti > tf || vi > vf);

		main_get_gapp ()->progress_info_new ("Multidimenssional Plane Regression", 2);
		main_get_gapp ()->progress_info_set_bar_fraction (0., 1);
		main_get_gapp ()->progress_info_set_bar_fraction (0., 2);
		main_get_gapp ()->progress_info_set_bar_text ("Time", 1);
		main_get_gapp ()->progress_info_set_bar_text ("Value", 2);
	}

	int ntimes_tmp = tf-ti+1;
	for (int time_index=ti; time_index <= tf; ++time_index){
		Mem2d *m = Src->mem2d_time_element (time_index);
		if (multidim)
			main_get_gapp ()->progress_info_set_bar_fraction ((gdouble)(time_index-ti)/(gdouble)ntimes_tmp, 1);

		Dest->mem2d->Resize (m->GetNx (), m->GetNy (), vf-vi+1, m->GetTyp());
		for (int v_index = vi; v_index <= vf; ++v_index){
			m->SetLayer (v_index);
			Dest->mem2d->SetLayer (v_index-vi);

			int nx=Dest->mem2d->GetNx();
			int my=Dest->mem2d->GetNy();
			Dest->mem2d->CopyFrom(m, 0,0, 0,0, Dest->mem2d->GetNx(),Dest->mem2d->GetNy());
			for(int i=1;i<my;i++){
				int q, j;
				double avg, avgdev;
    
				{
					double sum=0.;
					double absdevsum=0.;
					double diff;
					for(j=0;j<nx;j++){
						diff = m->GetDataPkt(j,i) - m->GetDataPkt(j,i-1);
						sum += diff;
						absdevsum += fabs(diff);
					}
					avg = sum/nx;
					avgdev = absdevsum/(nx-1);
				}
				for(q=0;q<4;q++)	{
					double sum, dsum, na;
					for(na=dsum=sum=0.0,j=0;j<nx;j++)	{
						const double sdev = (m->GetDataPkt(j,i) - m->GetDataPkt(j,i-1) - avg)/avgdev;
						const double k = 1/(3+sdev*sdev);
						sum += k*sdev;
						dsum += k*sdev*sdev;
						na += k;
					}
					avg += avgdev*sum/na;
					avgdev = 0.2*avgdev + 0.8*sqrt(dsum/(na-1));
				}
    
				{
					SHT iavg = (SHT)(rint(avg));
					for(j=0;j<nx;j++)
						Dest->mem2d->PutDataPkt(m->GetDataPkt(j,i) - iavg, j,i);
				}
			}

		}
		Dest->append_current_to_time_elements (time_index-ti, m->get_frame_time ());
	}

	Dest->data.s.ntimes = ntimes_tmp;
	Dest->data.s.nvalues=Dest->mem2d->GetNv ();

	if (multidim){
		main_get_gapp ()->progress_info_close ();
		Dest->retrieve_time_element (0);
		Dest->mem2d->SetLayer(0);
	}

	return MATH_OK;
}
#endif

// compute 1D power spectrum (row by row)
gboolean F1D_PowerSpec(MATHOPPARAMS)
{
	XSM_DEBUG (DBG_L3, "F1D LogPowerSpec");

	XSM_DEBUG (DBG_L3, "F1D LogPowerSpec: using libfftw");

	// get memory for complex data
        double        *in = (double*)       fftw_malloc(sizeof(double) * Src->mem2d->GetNx());
        fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * Src->mem2d->GetNx());

	// create plan for fft
	fftw_plan plan = fftw_plan_dft_r2c_1d (Src->mem2d->GetNx(), in, out, FFTW_ESTIMATE);
	if (plan == NULL) {
		XSM_DEBUG (DBG_L3, "F1D LogPowerSpec: libfftw: Error creating plan");
		return MATH_LIB_ERR;
	}

	Dest->mem2d->Resize (Src->mem2d->GetNx()/2, Src->mem2d->GetNy(), ZD_DOUBLE);
	Dest->data.s.nx = Dest->mem2d->GetNx();
	Dest->data.s.ny = Dest->mem2d->GetNy();
	Dest->data.s.x0 = 0.;
	Dest->data.s.y0 = 0.;
	Dest->data.s.rx = Src->data.s.rx/2.;
	Dest->data.s.ry = Src->data.s.ry;
	Dest->data.s.dl = Src->data.s.rx/2.;
	Dest->data.s.dz = Src->data.s.dz;
	Dest->data.s.dy = Src->data.s.dy;
 
	int n = Dest->mem2d->GetNx();
        double norm = 1./n;
	// compute 1D fourier transform for every row
	for (int line = 0; line < Src->mem2d->GetNy(); line++) {
		Src->mem2d->data->SetPtr(0, line);
    
		// prepare image row data for fftw
		for (int col = 0; col < Src->mem2d->GetNx(); col++)
			in[col] = Src->mem2d->data->GetNext();

		// compute transform
		fftw_execute (plan);

		// convert complex data to image row
		// scale data to 16Bit integer
		// reorder data that freq 0 is in the middle and positive freq go to the right

		Dest->mem2d->data->SetPtr(0, line);

		for (int col = Dest->mem2d->GetNx()-1; col >= 0; --col)
			Dest->mem2d->data->SetNext ( ZEROVALUE + norm * sqrt( c_re(out[col])*c_re(out[col]) + c_im(out[col])*c_im(out[col]) ));
	}

	double s = 1./(fabs(Src->mem2d->data->GetXLookup(Src->mem2d->GetNx()-1) - Src->mem2d->data->GetXLookup(0)));
	for (int i=0; i<n; ++i)
		Dest->mem2d->data->SetXLookup(i, (n-i)*s);
    
	// delete plan:
	fftw_destroy_plan (plan);

	// free temp data memory
        fftw_free(in);
        fftw_free(out); 

	return MATH_OK;
}

// 2D Filterfunktionen

// remove bad area
gboolean F2D_RemoveRect(MATHOPPARAMS)
{
	XSM_DEBUG (DBG_L3, "F2D Remove Rect");

	// get selection
	MOUSERECT msr;
	MkMausSelect (Src, &msr, Dest->mem2d->GetNx(), Dest->mem2d->GetNy());

	// check for valid selection
	if( msr.xSize  < 1 || msr.yBottom > Src->mem2d->GetNy() || msr.yTop < 0)
		return MATH_SELECTIONERR;

	// resize Dest
	Dest->mem2d->Resize(Src->mem2d->GetNx(), Src->mem2d->GetNy()-msr.ySize);
	Dest->data.s.ny = Dest->mem2d->GetNy ();

	// copy data except selection
	for (int line = 0; line < msr.yTop; line++)
		Dest->mem2d->CopyFrom(Src->mem2d, 0,line, 0, line, Dest->mem2d->GetNx());

	for (int line = msr.yBottom+1; line < Src->mem2d->GetNy(); line++)
		Dest->mem2d->CopyFrom(Src->mem2d, 0,line, 0, line-msr.ySize-1, Dest->mem2d->GetNx());

	return MATH_OK;
}


// Do 2D DeSpike
gboolean F2D_Despike(MATHOPPARAMS){
	int i,j,k,l,za=0,nx,ny;
	int anz=1;
	int num=1;
	double reihe1[20],reihe2[20],mi;

	XSM_DEBUG (DBG_L3, "F2D Despike");

	BenchStart(mbs,"2D Despike",Dest->mem2d->GetNx()*Dest->mem2d->GetNy());
  

	int ti=0; 
	int tf=0;
	int vi=0;
	int vf=0;
	gboolean multidim = FALSE;
	
	if (Src->data.s.ntimes != 1 || Src->mem2d->GetNv () != 1){
		multidim = TRUE;
		do {
			ti=vi=0;
			tf=Src->number_of_time_elements ()-1;
			if (tf < 0) tf = 0;
			vf=Src->mem2d->GetNv ()-1;
			main_get_gapp ()->setup_multidimensional_data_copy ("Multidimensional 2D Despike", Src, ti, tf, vi, vf);
		} while (ti > tf || vi > vf);

		main_get_gapp ()->progress_info_new ("Multidimenssional Plane Regression", 2);
		main_get_gapp ()->progress_info_set_bar_fraction (0., 1);
		main_get_gapp ()->progress_info_set_bar_fraction (0., 2);
		main_get_gapp ()->progress_info_set_bar_text ("Time", 1);
		main_get_gapp ()->progress_info_set_bar_text ("Value", 2);
	}

	int ntimes_tmp = tf-ti+1;
	for (int time_index=ti; time_index <= tf; ++time_index){
		Mem2d *m = Src->mem2d_time_element (time_index);
		if (multidim)
			main_get_gapp ()->progress_info_set_bar_fraction ((gdouble)(time_index-ti)/(gdouble)ntimes_tmp, 1);

		Dest->mem2d->Resize (m->GetNx (), m->GetNy (), vf-vi+1, m->GetTyp());
		for (int v_index = vi; v_index <= vf; ++v_index){
			m->SetLayer (v_index);
			Dest->mem2d->SetLayer (v_index-vi);	

			Dest->mem2d->CopyFrom(m, 0,0, 0,0, 
					      nx=Dest->mem2d->GetNx(),ny=Dest->mem2d->GetNy());

			for(j=anz; j<ny-anz; ++j)  {
				for(i=anz; i<nx-anz; ++i) {
					for(k=j-anz, l=0; k<=j+anz; k++,l++)
						reihe1[l] = m->data->Z(i,k);
					for(k=0; k<2*anz+1;k++)  {
						mi = 32650;
						for(l=0; l<2*anz+1;l++) {
							if(reihe1[l]<mi) {  mi=reihe1[l]; reihe2[k]=mi; za=l;	}
	  
						}
						reihe1[za]=32650;
					}
					Dest->mem2d->data->Z(reihe2[num], i,j);
				}  /*i*/
			} /*j*/

		}
		Dest->append_current_to_time_elements (time_index-ti, m->get_frame_time ());
	}

	Dest->data.s.ntimes = ntimes_tmp;
	Dest->data.s.nvalues=Dest->mem2d->GetNv ();

	if (multidim){
		main_get_gapp ()->progress_info_close ();
		Dest->retrieve_time_element (0);
		Dest->mem2d->SetLayer(0);
	}

	BenchStop(mbs);

	return MATH_OK;
}




// "fix" bad line
// using an interpolation between the neighbour lines
// uses new Object methods now !
extern gboolean F2D_LineInterpol(MATHOPPARAMS)
{
	double threashold = 1000.;
	XSM_DEBUG (DBG_L3, "F2D LineInterpol");
	
	MOUSERECT msr;
	
	MkMausSelect (Src, &msr, Dest->mem2d->GetNx(), Dest->mem2d->GetNy());
	
	if( msr.xSize  < 1)
		return MATH_SELECTIONERR;
	
	// copy scan data:
	Dest->mem2d->CopyFrom(Src->mem2d, 0,0, 0,0, Dest->mem2d->GetNx(),Dest->mem2d->GetNy());
	
	if (fabs(msr.yBottom - msr.yTop) > 3.){
		main_get_gapp ()->ValueRequest("Enter Value", "Z threshold (DAC units)", 
				   "max allowed Z variation withing marked width",
				   main_get_gapp ()->xsm->Unity, 
				   0., 10000., ".0f", &threashold);
	}
	
	// interpolation if in between first and last line
	if(msr.yTop > 0){
		if(msr.yTop < (Src->mem2d->GetNy()-2)){
			if (fabs(msr.yBottom - msr.yTop) > 3.){
				double zprev = 0.;
				for (int line = msr.yTop; line < msr.yBottom; ++line){
					double z = 0., num = 0.;
					for (int row = msr.xLeft; row <= msr.xRight; ++row){
						num += 1.;
						z += Src->mem2d->GetDataPkt (row, line);
					}
					z /= num;
					if (line == msr.yTop || fabs (z-zprev) < threashold){
						zprev = z;
						continue;
					}
					std::cout << "Fixing jump (diff is="<< (z-zprev) <<") at line: " << line << std::endl;
					
					// create help Mem2d object (only one line)
					Mem2d PreLine(Src->mem2d->GetNx(), 1, Src->mem2d->GetTyp());
					// with "PreLine" and set internal Ptr to start
					PreLine.CopyFrom (Src->mem2d, 0,line-1, 0,0, Src->mem2d->GetNx());
					PreLine.data->SetPtr (0,0);
					// set Src Ptr to "PostLine"
					Src->mem2d->data->SetPtr (0,line+1);
					// set Dest Ptr to "Line"
					Dest->mem2d->data->SetPtr (0,line);
					for(int col = 0; col < Src->mem2d->GetNx (); col++)
						Dest->mem2d->data->SetNext ((PreLine.data->GetNext()+Src->mem2d->data->GetNext())/2.);
				}
			} else {
				// create help Mem2d object (only one line)
				Mem2d PreLine(Src->mem2d->GetNx(), 1, Src->mem2d->GetTyp());
				// with "PreLine" and set internal Ptr to start
				PreLine.CopyFrom(Src->mem2d, 0,msr.yTop-1, 0,0, Src->mem2d->GetNx());
				PreLine.data->SetPtr(0,0);
				// set Src Ptr to "PostLine"
				Src->mem2d->data->SetPtr(0,msr.yTop+1);
				// set Dest Ptr to "Line"
				Dest->mem2d->data->SetPtr(0,msr.yTop);
				for(int col = 0; col < Src->mem2d->GetNx(); col++)
					Dest->mem2d->data->SetNext((PreLine.data->GetNext()+Src->mem2d->data->GetNext())/2.);
			}
		}
		else  // last line: copy previous line
			Dest->mem2d->CopyFrom(Src->mem2d, 0,msr.yTop-1, 0,msr.yTop, Src->mem2d->GetNx());
	}
	else  // first line: copy 2nd line
		Dest->mem2d->CopyFrom(Src->mem2d, 0,msr.yTop+1, 0,msr.yTop, Src->mem2d->GetNx());
	
	return MATH_OK;
}




// 2D power spectrum in logarithmic scale
gboolean F2D_LogPowerSpec(MATHOPPARAMS)
{
	XSM_DEBUG (DBG_L3, "F2D FFT");
	ZData  *SrcZ;

	SrcZ  =  Src->mem2d->data;
	Dest->mem2d->Resize (Dest->data.s.nx, Dest->data.s.ny, ZD_COMPLEX);
	// set "Complex" layer param defaults
	Dest->data.s.nvalues=3;
	Dest->data.s.ntimes=1;
	Dest->mem2d->data->MkXLookup(-1., 1.);
	Dest->mem2d->data->MkYLookup(-1., 1.);
	Dest->data.s.dx = 2./Src->mem2d->GetNx ();
	Dest->data.s.dy = 2./Src->mem2d->GetNy ();
	Dest->mem2d->data->SetVLookup(0, 0);
	Dest->mem2d->data->SetVLookup(1, 1);

	XSM_DEBUG (DBG_L3, "F2D FFT: using libfftw");

	// allocate memory for real and inplace complex data
	int xlen=2*(Src->mem2d->GetNx ()/2+1);
        int size_in = Src->mem2d->GetNx () * Src->mem2d->GetNy ();
	double *in  = new double [ size_in ];
	fftw_complex *dat  = new fftw_complex [ xlen*Src->mem2d->GetNy () ];

	if (dat == NULL) {
		XSM_DEBUG (DBG_L3, "F2D FFT: libfftw: Error allocating memory for complex data");
		return MATH_NOMEM;
	}

        //	memset(in, 0, size_in * sizeof (double));

	// create plan for in-place transform
	fftw_plan plan    = fftw_plan_dft_r2c_2d (Src->mem2d->GetNy (), Src->mem2d->GetNx (), 
						  in, dat, FFTW_ESTIMATE);

	if (plan == NULL) {
		XSM_DEBUG (DBG_L3, "F2D FFT: libfftw: Error creating plan");
		return MATH_LIB_ERR;
	}

	// convert image data to fftw_real
	for (int line=0; line < Src->mem2d->GetNy(); line++) {
		SrcZ ->SetPtr(0, line);
		for (int col=0; col < Src->mem2d->GetNx(); col++)
			in[line*Src->mem2d->GetNx() + col] = SrcZ->GetNext();
	}

	// compute 2D transform using in-place fourier transform
	fftw_execute (plan);

	// convert complex data to image
	// and flip image that spatial freq. (0,0) is in the middle
	// (quadrant swap using the QSWP macro defined in xsmmath.h)
  
	XSM_DEBUG (DBG_L3, "F2D FFT done, converting data to ABS/RE/IM");

	xlen /= 2; // jetzt für complex

	double I;
	int m,n;
 
	for (int line=0; line < Src->mem2d->GetNy(); line++) {
		for (int i,col=0; col < Src->mem2d->GetNx()/2; col++) {
			i=line*xlen + col;
			m = QSWP(col, Src->mem2d->GetNx());
			n = QSWP(line, Src->mem2d->GetNy());
			I = ZEROVALUE + sqrt (c_re(dat[i]) * c_re(dat[i]) +  c_im(dat[i]) * c_im(dat[i]));
			Dest->mem2d->PutDataPkt(I, m, n, 0);
			Dest->mem2d->PutDataPkt(c_re(dat[i]), m, n, 1);
			Dest->mem2d->PutDataPkt(c_im(dat[i]), m, n, 2);
			if(line != (Src->mem2d->GetNy()/2)){
				m = Src->mem2d->GetNx() - m;
				n = Src->mem2d->GetNy() - n;
				Dest->mem2d->PutDataPkt(I, m, n, 0);
				Dest->mem2d->PutDataPkt(c_re(dat[i]), m, n, 1); 
				Dest->mem2d->PutDataPkt(c_im(dat[i]), m, n, 2);  
			}
		}
	}
  
	// destroy plan
	fftw_destroy_plan(plan); 

	// free real/complex data memory
	delete in;
	delete dat;

	return MATH_OK;
}


// Real 1D fourier filter :=)

// FFTW-Kernel, applys some function to the spectra
// Forward FT => do something with spectra => backward FT
gboolean F1D_ift_ft(MATH2OPPARAMS,
		    gboolean (*spkfkt)(MATH2OPPARAMS,
				       fftw_complex *dat, 
				       int line)
	){
	XSM_DEBUG (DBG_L3, "Doing iftXft: RE( ift(X * FT(Active)) )");
	gboolean ret=MATH_OK;
	ZData  *SrcZ, *DestZ;

	SrcZ  = Src1->mem2d->data;
	DestZ = Dest->mem2d->data;

	// error reporting for invalid selections:
	// Src1 and Src2 scans must have the same dimensions
	// if Src2 == NULL filter spkfkt needs no selection, see e.g. SpkAutoCorr
	if(Src2 != NULL && (Src1->data.s.nx != Src2->data.s.nx || Src1->data.s.ny != Src2->data.s.ny))
		return MATH_SELECTIONERR;

	// ======== 1. fourier transform Src1 => complex data spectrum =======

	int line, col;

	// allocate memory for complex data
        int size_in = Src1->mem2d->GetNx ();
	double *in  = new double [ size_in ];
	fftw_complex *dat = new fftw_complex [ Src1->mem2d->GetNx() ];
	double scale;

	fftw_plan plan    = fftw_plan_dft_r2c_1d (Src1->mem2d->GetNx(), in, dat, FFTW_ESTIMATE);
	fftw_plan planinv = fftw_plan_dft_c2r_1d (Src1->mem2d->GetNx(), dat, in, FFTW_ESTIMATE);

	if (!plan || !planinv) {
		XSM_DEBUG (DBG_L3, "iftXft: libfftw: Error creating plans");
		if(plan)
			fftw_destroy_plan (plan); 
		if(planinv)
			fftw_destroy_plan (planinv); 
		return MATH_LIB_ERR;
	}



	if (!in || !dat) {
		XSM_DEBUG (DBG_L3, "iftXft: libfftw: Error allocating memory for complex data");
		if(in)  delete in;
		if(dat) delete dat;
		return MATH_NOMEM;
	}

	// convert image data to fftw_real
	for (line=0; line < Src1->mem2d->GetNy(); line++) {
		SrcZ ->SetPtr(0, line);
		for (col=0; col < Src1->mem2d->GetNx(); col++)
			in[col] = SrcZ->GetNext();
    
		// 1. compute 1D transform using in-place fourier transform
		fftw_execute (plan);
	  
		// now the complex spectrum is in dat too !
	  
		// 2. apply something to the spektra
		if (spkfkt (MATH2OPVARS, dat, line) == MATH_OK){
      
			// 3. inverse fourier transform modified spectrum
      
			fftw_execute (planinv);
      
			// 4. take the real part as data and apply the fft normalization factor scale
      
			scale = 1. / Src1->mem2d->GetNx();
			DestZ->SetPtr(0, line);
			for(col=0; col<Dest->mem2d->GetNx(); col++)
				DestZ->SetNext(scale*(in[col]));
		}
		else
			ret=MATH_SELECTIONERR;
	}

	// free [complex] data memory
	delete in;

	// destroy plan
	fftw_destroy_plan (plan); 
	fftw_destroy_plan (planinv); 
  
	return ret;
}


// filter kernel 1D "Window"
gboolean SpkWindow1D(MATH2OPPARAMS, fftw_complex *dat, int line){
	int col;
	// ====== 2. multiply complex data spectrum with Src2, eg. windowing =====
	// and take into account that spatial freq. (0,0) is in the middle

	// apply Window

	int n    = Src1->mem2d->GetNx();
	for (col=0; col < n/2; col++)
		if (Src2->mem2d->GetDataPkt(n/2-col-1, line) == ZEROVALUE){
			c_re (dat[col]) = 0.;
			c_im (dat[col]) = 0.;
			c_re (dat[n-col-1]) = 0.;
			c_im (dat[n-col-1]) = 0.;
		}
	return MATH_OK;
}


// filter kernel "Gauss Stopp"
gboolean SpkGaussStop1D(MATH2OPPARAMS, fftw_complex *dat, int line){
	MOUSERECT msr;
	MkMausSelect (Src2, &msr, Dest->mem2d->GetNx(), Dest->mem2d->GetNy());

	if( msr.xSize  < 1 || msr.ySize < 1)
		return MATH_SELECTIONERR;

	// apply gaussian window to complex data
	// scale = 1 - exp(-r2/msr.Radius2)
	double r, scale;
	int n = Src1->mem2d->GetNx();
	for (int col = 0; col < n/2; col++) {
		r = (n/2-col-1) - msr.xCenter; r *= r;
		scale = 1 - exp(-r / msr.Radius2);
		c_re (dat[col])     *= scale;
		c_re (dat[n-col-1]) *= scale;
	}
	return MATH_OK;
}

// filter kernel "Gauss Pass"
gboolean SpkGaussPass1D(MATH2OPPARAMS, fftw_complex *dat, int line){
	MOUSERECT msr;
	MkMausSelect (Src2, &msr, Dest->mem2d->GetNx(), Dest->mem2d->GetNy());

	if( msr.xSize  < 1 || msr.ySize < 1)
		return MATH_SELECTIONERR;

	// apply gaussian window to complex data
	// scale = exp(-r2/msr.Radius2)
	double r, scale;
	int n = Src1->mem2d->GetNx();
	for (int col = 0; col < n/2; col++) {
		r = (n/2-col-1) - msr.xCenter; r *= r;
		scale = exp(-r / msr.Radius2);
		c_re (dat[col])   *= scale;
		c_re (dat[n-col-1]) *= scale;
	}
	return MATH_OK;
}


// Fourierfilter:
// Pass frequency window with gaussian profile
gboolean F1D_FT_Window(MATH2OPPARAMS)
{
	XSM_DEBUG (DBG_L3, "F1D FT Window");
	return(F1D_ift_ft(MATH2OPVARS, SpkWindow1D));
	return MATH_OK;
}

gboolean F1D_FT_GaussStop(MATH2OPPARAMS)
{
	XSM_DEBUG (DBG_L3, "F1D FT GaussStop");
	return(F1D_ift_ft(MATH2OPVARS, SpkGaussStop1D));
	return MATH_OK;
}

gboolean F1D_FT_GaussPass(MATH2OPPARAMS)
{
	XSM_DEBUG (DBG_L3, "F1D FT GaussPass");
	return(F1D_ift_ft(MATH2OPVARS, SpkGaussPass1D));
	return MATH_OK;
}

gboolean F1D_ydiff(MATHOPPARAMS)
{
	XSM_DEBUG (DBG_L3, "F1D ydiff");
	for (int i=0; i < Src->mem2d->GetNx ()-1; ++i)
		Dest->mem2d->PutDataPkt (Src->mem2d->GetDataPkt (i,0)-Src->mem2d->GetDataPkt (i+1,0), i,0);

	Dest->mem2d->PutDataPkt (0., Src->mem2d->GetNx ()-1,0);
	return MATH_OK;
}


// Echte 2D Fourier Filter :=)

// FFTW-Kernel, applys some function to the spectra
// Forward FT => do something with spectra => backward FT
gboolean F2D_ift_ft(MATH2OPPARAMS, gboolean (*spkfkt)(MATH2OPPARAMS, fftw_complex *dat)){
	XSM_DEBUG (DBG_L3, "Doing iftXft: RE( ift(X * FT(Active)) )");
	gboolean ret=MATH_OK;
	ZData  *SrcZ, *DestZ;

	SrcZ  = Src1->mem2d->data;
	DestZ = Dest->mem2d->data;

	// error reporting for invalid selections:
	// Src1 and Src2 scans must have the same dimensions
	// if Src2 == NULL filter spkfkt needs no selection, see e.g. SpkAutoCorr
	if(Src2 != NULL && (Src1->data.s.nx != Src2->data.s.nx || Src1->data.s.ny != Src2->data.s.ny))
		return MATH_SELECTIONERR;

	// ======== 1. fourier transform Src1 => complex data spectrum =======

	int line, col;

	// allocate memory for complex data
	int xlen=2*(Src1->mem2d->GetNx()/2+1);
	double    *in  = new double [ xlen*Src1->mem2d->GetNy() ];
	fftw_complex *dat = (fftw_complex*)(in); // use same memory - saves a lot space...
	double scale;

	if (!in || !dat) {
		XSM_DEBUG (DBG_L3, "iftXft: libfftw: Error allocating memory for complex data");
		if(in)  delete in;
		if(dat) delete dat;
		return MATH_NOMEM;
	}

	// create plan for in-place transform
	fftw_plan plan    = fftw_plan_dft_r2c_2d (Src1->mem2d->GetNy(), Src1->mem2d->GetNx(), in, dat, FFTW_ESTIMATE);
	fftw_plan planinv = fftw_plan_dft_c2r_2d (Src1->mem2d->GetNy(), Src1->mem2d->GetNx(), dat, in, FFTW_ESTIMATE);

	if (!plan || !planinv) {
		XSM_DEBUG (DBG_L3, "iftXft: libfftw: Error creating plans");
		if(plan)
			fftw_destroy_plan (plan); 
		if(planinv)
			fftw_destroy_plan (planinv); 
		return MATH_LIB_ERR;
	}

	// memset(in, 0, sizeof(in));

	// convert image data to fftw_real
	for (line=0; line < Src1->mem2d->GetNy(); line++) {
		SrcZ ->SetPtr(0, line);
		for (col=0; col < Src1->mem2d->GetNx(); col++)
			in[line*xlen + col] = SrcZ->GetNext();
	}

	// 1. compute 2D transform using in-place fourier transform

	fftw_execute (plan);
	// now the complex spectrum is in dat too !

	// 2. apply something to the spektra
	if (spkfkt (MATH2OPVARS, dat) == MATH_OK){

		// 3. inverse fourier transform modified spectrum

		fftw_execute (planinv);

		// 4. take the real part as data and apply the fft normalization factor scale

		scale = 1. / Src1->mem2d->GetNx() / Src1->mem2d->GetNy();
		for(line=0; line<Dest->mem2d->GetNy(); line++){
			DestZ->SetPtr(0, line);
			for(col=0; col<Dest->mem2d->GetNx(); col++)
				DestZ->SetNext(scale*(in[line*xlen + col]));
		}
	}
	else
		ret=MATH_SELECTIONERR;

	// free [complex] data memory
	delete in;

	// destroy plan
	fftw_destroy_plan (plan); 
	fftw_destroy_plan (planinv); 

	return ret;
}

// filter kernel "Window"
gboolean SpkWindow(MATH2OPPARAMS, fftw_complex *dat){
	int line, col;
	// ====== 2. multiply complex data spectrum with Src2, eg. windowing =====
	// and take into account that spatial freq. (0,0) is in the middle

	// apply Window

	int xlen = Src1->mem2d->GetNx()/2+1;
	for (line=0; line < Src1->mem2d->GetNy(); line++)
		for (col=0; col < xlen; col++)
			if(Src2->mem2d->GetDataPkt(QSWP(col,Src1->mem2d->GetNx()), 
						   QSWP(line,Src1->mem2d->GetNy()))
			   == ZEROVALUE){
				c_re(dat[line*xlen + col]) = 0.;
				c_im(dat[line*xlen + col]) = 0.;
			}
	return MATH_OK;
}


// Src1 : data source
// Src2 : window function
//        might be power spectrum from above with 
//        Dest = Re(IFT( FT(Src1) * (Src2 == ZEROVALUE ? ZEROVALUE : 1) ))
//	  that is: all spectral parts, which art zero in Src will be
//        suppressed in the complex spectrum.
// 	  To achieve this, you have to set the appropriate area to zero
//	  in the power spectrum.
//        --- that's it --- PZ
gboolean F2D_iftXft(MATH2OPPARAMS){
	XSM_DEBUG (DBG_L3, "Doing iftXft: RE( ift(X * FT(Active)) )  -- Window(s)");
	return(F2D_ift_ft(MATH2OPVARS, SpkWindow));
	return MATH_OK;
}


// filter kernel "Gauss Stopp"
gboolean SpkGaussStop(MATH2OPPARAMS, fftw_complex *dat){
	MOUSERECT msr;
	MkMausSelect (Src2, &msr, Dest->mem2d->GetNx(), Dest->mem2d->GetNy());

	if( msr.xSize  < 1 || msr.ySize < 1)
		return MATH_SELECTIONERR;

	// apply gaussian window to complex data
	// scale = 1 - exp(-r2/msr.Radius2)
	double r2, scale;
	int xlen = Src1->mem2d->GetNx()/2+1;
	for (int line = 0; line < Src1->mem2d->GetNy(); line++)
		for (int col = 0; col < xlen; col++) {
			r2 = (QSWP(line,Src1->mem2d->GetNy()) - msr.yCenter) * (QSWP(line,Src1->mem2d->GetNy()) - msr.yCenter) + 
				(QSWP(col,Src1->mem2d->GetNx()) - msr.xCenter) * (QSWP(col,Src1->mem2d->GetNx()) - msr.xCenter);
			scale = 1 - exp(-r2 / msr.Radius2);
			c_re(dat[line*xlen + col]) *= scale;
			c_im(dat[line*xlen + col]) *= scale;
		}
	return MATH_OK;
}

// Fourierfilter:
// Pass frequency window with gaussian profile
gboolean F2D_FT_GaussStop(MATH2OPPARAMS)
{
	XSM_DEBUG (DBG_L3, "F2D FT GaussStop");
	return(F2D_ift_ft(MATH2OPVARS, SpkGaussStop));
	return MATH_OK;
}


// filter kernel "Gauss Pass"
gboolean SpkGaussPass(MATH2OPPARAMS, fftw_complex *dat){
	MOUSERECT msr;
	MkMausSelect (Src2, &msr, Dest->mem2d->GetNx(), Dest->mem2d->GetNy());

	if( msr.xSize  < 1 || msr.ySize < 1)
		return MATH_SELECTIONERR;

	// apply gaussian window to complex data
	// scale = exp(-r2/msr.Radius2)
	double r2, scale;
	int xlen = Src1->mem2d->GetNx()/2+1;
	for (int line = 0; line < Src1->mem2d->GetNy(); line++)
		for (int col = 0; col < xlen; col++) {
			r2 = (QSWP(line,Src1->mem2d->GetNy()) - msr.yCenter) * (QSWP(line,Src1->mem2d->GetNy()) - msr.yCenter) + 
				(QSWP(col,Src1->mem2d->GetNx()) - msr.xCenter) * (QSWP(col,Src1->mem2d->GetNx()) - msr.xCenter);
			scale = exp(-r2 / msr.Radius2);
			c_re(dat[line*xlen + col]) *= scale;
			c_im(dat[line*xlen + col]) *= scale;
		}
	return MATH_OK;
}


// Fourierfilter:
// Pass frequency window with gaussian profile
gboolean F2D_FT_GaussPass(MATH2OPPARAMS)
{
	XSM_DEBUG (DBG_L3, "F2D FT GaussPass");
	return(F2D_ift_ft(MATH2OPVARS, SpkGaussPass));
}


// filter kernel "Abs. Square"
// (for auto correlation)
gboolean SpkAutoCorr(MATH2OPPARAMS, fftw_complex *dat)
{
	// take abs. value of complex data
	int xlen = Src1->mem2d->GetNx()/2+1;
	for (int line = 0; line < Src1->mem2d->GetNy(); line++)
		for (int col = 0; col < xlen; col++) {
			c_re(dat[line*xlen + col]) = sqrt( c_re(dat[line*xlen + col])*c_re(dat[line*xlen + col]) +
							   c_im(dat[line*xlen + col])*c_im(dat[line*xlen + col]) );
			c_im(dat[line*xlen + col]) = 0.0;
		}


	return MATH_OK;
}


// auto correlation
gboolean F2D_AutoCorr(MATHOPPARAMS)
{
	XSM_DEBUG (DBG_L3, "F2D AutoCorr");
	return(F2D_ift_ft(Src, NULL, Dest, SpkAutoCorr));
}


