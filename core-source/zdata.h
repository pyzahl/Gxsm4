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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

#ifndef __ZDATA_H
#define __ZDATA_H

#include <gsl/gsl_spline2d.h>
#include <gsl/gsl_interp.h> // For gsl_interp_accel
#include <stdio.h>
#include <stdlib.h> // For malloc, free


#include <netcdf>
using namespace netCDF;

/**
 *  ZData:
 *
 *  Obj. zur 2D Speicherverwaltung
 *  ACHTUNG: keine Indexkontrolle !!!
 *  bei ungültigen Indizes droht Seg Fault !!
 */

class ZData{
	friend class Mem2d; 
public:
	ZData(int Nx, int Ny, int Nv);
	virtual ~ZData();

	inline int GetNx(){ return nx; };
	inline int GetX0Sub(){ return cp_ixy_sub[0]; };
	inline int GetNxSub(){ return cp_ixy_sub[1]; };
	inline int GetNy(){ return ny; };
	inline int GetY0Sub(){ return cp_ixy_sub[2]; };
	inline int GetNySub(){ return cp_ixy_sub[3]; };
	inline int GetNv(){ return nv; };
        inline void InvalidateLine(int yi){ Li[yi+ny*vlayer].invalidate(); };
        inline void InvalidateLine(int yi, int vi){ Li[yi+ny*vi].invalidate(); };
        inline void InvalidateLineSub(int yi, int vi){ yi+=cp_ixy_sub[2]; Li[yi+ny*vi].invalidate(); };
        
	double GetXLookup(int i){ if(i>=0 && i<nx) return Xlookup[i]; else return 0.; };
	double GetYLookup(int i){ if(i>=0 && i<ny) return Ylookup[i]; else return 0.; };
	double GetVLookup(int i){ if(i>=0 && i<nv) return Vlookup[i]; else return 0.; };
	void SetXLookup(int i, double lv){ if(i>=0 && i<nx) Xlookup[i]=lv; };
	void SetYLookup(int i, double lv){ if(i>=0 && i<ny) Ylookup[i]=lv; };
	void SetVLookup(int i, double lv){ if(i>=0 && i<nv) Vlookup[i]=lv; };
	void MkXLookup(double start, double end){ 
		double d=(end-start)/(nx-1); 
		for(int i=0; i<nx; ++i) Xlookup[i]=start+d*i;
	};
	void MkYLookup(double start, double end){ 
		double d=(end-start)/(ny-1); 
		for(int i=0; i<ny; ++i) Ylookup[i]=start+d*i;
	};
	void MkVLookup(double start, double end){ 
		double d=(end-start)/(nv-1); 
		for(int i=0; i<nv; ++i) Vlookup[i]=start+d*i;
	};
	void CopyLookups (ZData *zd, int xi=0, int yi=0, int vi=0){
		int i;
		for(i=0; i<nx; ++i) Xlookup[i]=zd->GetXLookup(i+xi);
		for(i=0; i<ny; ++i) Ylookup[i]=zd->GetYLookup(i+yi);
		for(i=0; i<nv; ++i) Vlookup[i]=zd->GetVLookup(i+vi);
	};
	void CopyXLookup (ZData *zd, int xi=0){
		int i;
		for(i=0; i<nx; ++i) Xlookup[i]=zd->GetXLookup(i+xi);
	};
	int i_from_XLookup (double x){ // converges only if monotoneous Lookup function
		double dx = (Xlookup[nx-1] - Xlookup[0])/(double)(nx-1);
		double eps = 0.2;
		double edx = fabs (eps * dx);
		// starting guess (linear)
		int iguess = (int)round ((x-Xlookup[0])/dx);
		double err;
		int tests = 100;
		int igp=-1;

		if (iguess < 0) iguess = 0;
		if (iguess >= nx) iguess = nx-1;

		// interpolate
		while (fabs (err = x-Xlookup[iguess]) > edx && igp != iguess && --tests){
			igp = iguess;
			iguess += err/dx;
			// limit to range
			if (iguess < 0) iguess = 0;
			if (iguess >= nx) iguess = nx-1;
		}
		return iguess;
	};

	virtual size_t Zsize()=0;
	virtual double Z(int x, int y)=0;
	virtual double Z(int x, int y, int v)=0;
	virtual double Z(double z, int x, int y)=0;
	virtual double ZSave(double z, int x, int y)=0;
	virtual double Z(double z, int x, int y, int v)=0;
	virtual double Z_ixy_sub(double z, int x, int y, int v)=0;
	virtual double Z(double vx, double vy)=0;
	virtual double Z(double z, double vx, double vy)=0;
	virtual void   Zadd(double z, int x, int y)=0;
	virtual void   ZaddSave(double z, int x, int y)=0;
	virtual void   Zmul(double z, int x, int y)=0;
	virtual void   ZmulSave(double z, int x, int y)=0;
	virtual void   Zdiv(double z, int x, int y)=0;

	virtual void ZFrameAddDataFrom(ZData *src)=0;

	virtual void ResetLineInfo();
	virtual int  Resize(int Nx, int Ny, int Nv=1)=0;

	void         ZPutDataSetDest(int ixy_sub[4]);
	virtual void ZPutDataLine(int y, void *src)=0;
	virtual void ZPutDataLine(int y, void *src, int mode)=0;
	virtual void ZPutDataLine16dot16(int y, void *src, int mode=MEM_SET)=0;
	virtual void ZGetDataLine(int y, void *dest)=0;

	virtual void           SetPtr(int x, int y)=0;
	virtual void           SetPtrT(int x, int y)=0;
	virtual void           SetPtrTB(int x, int y)=0;
	virtual double         GetNext()=0;
	virtual double         GetThis(double x=0.)=0;
	virtual double         GetThisL()=0;
	virtual double         GetThisLT()=0;
	virtual double         GetThisT()=0;
	virtual double         GetThisRT()=0;
	virtual double         GetThisR()=0;
	virtual double         GetThisRB()=0;
	virtual double         GetThisB()=0;
	virtual double         GetThisLB()=0;
	virtual void           IncPtrT()=0;
	virtual void           IncPtrTB()=0;

	virtual TYPE_ZD_FLOAT  GetThis(TYPE_ZD_FLOAT x)=0;
	virtual TYPE_ZD_LONG   GetThis(TYPE_ZD_LONG x)=0;
	virtual TYPE_ZD_ULONG  GetThis(TYPE_ZD_ULONG x)=0;
	virtual TYPE_ZD_SHORT  GetThis(TYPE_ZD_SHORT x)=0;
	virtual TYPE_ZD_BYTE   GetThis(TYPE_ZD_BYTE x)=0;
	virtual TYPE_ZD_EVENT  GetThis(TYPE_ZD_EVENT x)=0;

	virtual void           SetNext(double z)=0;
	virtual void           AddNext(double z)=0;
	virtual void           SetThis(double z)=0;  // Island-Labeling MB

	virtual int            CopyFrom(ZData *src, int x, int y, int tox, int toy, int nx, int ny=1, gboolean observe_shift=false)=0;
	virtual void           set_all_Z (double z, int v=-1, int x0=0, int y0=0, int xs=0, int ys=0)=0;

	virtual void*           GetPtr(int x, int y)=0;
	virtual TYPE_ZD_FLOAT*  GetPtr(int x, int y, TYPE_ZD_FLOAT z){return NULL;};
	virtual TYPE_ZD_LONG*   GetPtr(int x, int y, TYPE_ZD_LONG  z){return NULL;};
	virtual TYPE_ZD_ULONG*  GetPtr(int x, int y, TYPE_ZD_ULONG  z){return NULL;};
	virtual TYPE_ZD_SHORT*  GetPtr(int x, int y, TYPE_ZD_SHORT z){return NULL;};
	virtual TYPE_ZD_BYTE*   GetPtr(int x, int y, TYPE_ZD_BYTE z){return NULL;};
	virtual TYPE_ZD_EVENT*  GetPtr(int x, int y, TYPE_ZD_EVENT z){return NULL;};

	virtual void operator =  (ZData &rhs)=0;
	virtual void operator += (ZData &rhs)=0;
	virtual void operator -= (ZData &rhs)=0;
	virtual void operator *= (ZData &rhs)=0;
	virtual void operator /= (ZData &rhs)=0;
	virtual void operator ++ ()=0;
	virtual void operator -- ()=0;

	virtual inline double operator [] (int idx)=0;

	virtual void NcPut(NcVar &ncfield, int time_index=0, gboolean update=false)=0;
	virtual void NcGet(NcVar &ncfield, int time_index=0)=0;

	virtual void norm (double mag=1., int vi=0, int vf=-1)=0;
	virtual void mabs_norm (double mag=1., int vi=0, int vf=-1)=0;
	virtual void add (double c=1., int vi=0, int vf=-1)=0;
	virtual void mul (double f=1., int vi=0, int vf=-1)=0;
	virtual void replace (double zi, double zf, double eps=0.0, int vi=0, int vf=-1)=0;
	
	void SetLayer(int l){ vlayer=l; };
	void SetLayerDataPut(int l){ vlayer_put=l; };
	void StoreLayer(){ vlayerstore=vlayer; };
	void RestoreLayer(){ vlayer=vlayerstore; };
	int GetLayer(void){ return vlayer; };
	LineInfo *GetLi (int y) { return &Li[y+ny*vlayer]; };
	LineInfo *Li;

        void copy_ranges (ZData *z){
                zmin=z->zmin, zmax=z->zmax, zcenter=z->zcenter, zrange=z->zrange;
        };
	gboolean update_ranges (int iv, gboolean extend=false){
                double last_min = zmin;
                double last_range = zrange;
                if (!extend)
                        zmin=zmax=Z (0,0,iv);

                int marx = nx > 4 ? 2 : 0;
                int mary = ny > 4 ? 2 : 0;
                for (int ix=marx; ix<nx-marx; ++ix)
                        for (int iy=mary; iy<ny-mary; ++iy){
                                double z=Z (ix,iy,iv);
                                if (zmin > z) zmin = z;
                                if (zmax < z) zmax = z;
                        }
                zrange  = 1. + zmax - zmin; // never < 1!
                zcenter = zmin + 0.5*zrange;
                double eps = zrange/100.;
                return (fabs(last_min-zmin) > eps || fabs(last_range-zrange) > eps);
	};
	double zmin, zmax, zcenter, zrange;

        void set_shift (double cf_dt=0., double pixs_xdt=0., double pixs_ydt=0.);
        void set_shift_px (double dxm=0., double dym = 0.);
        void get_shift_px (double &dxm, double &dym) { dxm = XYpixshift_manual[0]; dym = XYpixshift_manual[1]; };
        gboolean is_shift () { return creepfactor != 0.0 || pixshift_dt[0] != 0. || pixshift_dt[1] != 0. || XYpixshift_manual[0] != 0. ||  XYpixshift_manual[1] != 0. ? true : false; };
        double creepfactor, pixshift_dt[2];
        gboolean enable_drift_correction;
        
        inline int clampx (int x) { return x < 0 ? 0 : x >= nx ? nx-1 : x; };
        inline int clampy (int y) { return y < 0 ? 0 : y >= ny ? ny-1 : y; };
        inline int clampv (int v) { return v < 0 ? 0 : v >= nv ? nv-1 : v; };

        inline int pbcx (int x) { while (x < 0) x+=nx; while (x >= nx) x-=nx; return x; };
        inline int pbcy (int y) { while (y < 0) y+=ny; while (y >= ny) y-=ny; return y; };
        inline int pbcv (int v) { while (v < 0) v+=nv; while (v >= nv) v-=nv; return v; };

protected:
	int  ZResize(int Nx, int Ny, int Nv=0);
	int  nx, ny, nv;
	int  vlayer, vlayerstore;
	int  vlayer_put;
	int  cp_ixy_sub[4];
        double XYpixshift_manual[2];

private:
	double *Xlookup, *Ylookup, *Vlookup;
};

/*
 * Subclass of ZData
 * with arbitrary scalar Element Data Type <ZTYP>
 */
template <class ZTYP> class TZData : public ZData{
	friend class Mem2d;
public:
	TZData(int Nx=1, int Ny=1, int Nv=1);
	virtual ~TZData();
       
	size_t Zsize(){ return sizeof(ZTYP); };

        inline void Z_M2x2(int x, int y, int v, double M[2][2]){
                for (int i=0; i<2; ++i)
                        for (int j=0; j<2; ++j)
                                M[j][i] = (double)Zdat[pbcy(y+j)*nv+v][pbcx(x+i)];

        };
        inline void Z_M3x3(int x, int y, int v, double M[3][3]){
                for (int i=0; i<3; ++i)
                        for (int j=0; j<3; ++j)
                                M[j][i] = (double)Zdat[pbcy(y+j-1)*nv+v][pbcx(x+i-1)];

        };
        inline void Z_M5x5(int x, int y, int v, double M[5][5]){
                for (int i=0; i<5; ++i)
                        for (int j=0; j<5; ++j)
                                M[j][i] = (double)Zdat[pbcy(y+j-1)*nv+v][pbcx(x+i-1)];

        };
        inline double Z_interpol2d(double x, double y, int v, int order=1){
                switch (order){
                case 0: return (double)Zdat[pbcy(int(round(y)))*nv+v][pbcx(int(round(x)))];
                case 1:
                        {
                                double M22[2][2];
                                int x0=int(floor(x));
                                int y0=int(floor(y));
                                double dx=x-x0;
                                double dy=y-y0;
                                Z_M2x2 (x0, y0, v, M22);
                                return (1.-dx)*(1.-dy)*M22[0][0] + dx*dy*M22[1][1] +  (1.-dx)*dy*M22[1][0] +  dx*(1.-dy)*M22[0][1];
                        }
                case 2:
                        {
                                // use GSL for qubic or bi-cubic! https://www.gnu.org/software/gsl/doc/html/interp.html

                                const gsl_interp2d_type *T = gsl_interp2d_bilinear;
                                const size_t N = 100;             /* number of points to interpolate */
                                const double xa[] = { -1.0, 0.0, 1.0, 2.0 }; /* define unit squares */
                                const double ya[] = { -1.0, 0.0, 1.0, 2.0 };
                                const size_t nx = sizeof(xa) / sizeof(double); /* x grid points */
                                const size_t ny = sizeof(ya) / sizeof(double); /* y grid points */
                                double *za = malloc(nx * ny * sizeof(double));
                                gsl_spline2d *spline = gsl_spline2d_alloc(T, nx, ny);
                                gsl_interp_accel *xacc = gsl_interp_accel_alloc();
                                gsl_interp_accel *yacc = gsl_interp_accel_alloc();
                                size_t i, j;

                                int x0=int(floor(x));
                                int y0=int(floor(y));

                                /* set z grid values */
                                for (int i=-1; i<3; ++i)
                                        for (int j=-1; j<3; ++j)
                                                gsl_spline2d_set(spline, za, i, j, (double)Zdat[pbcy(y0+j)*nv+v][pbcx(x0+i)]);

                                /* initialize interpolation */
                                gsl_spline2d_init(spline, xa, ya, za, nx, ny);
                                                                 
                                /* interpolate Z value */
                                double interpolated_z = gsl_spline2d_eval(spline, x-x0, y-y0, xacc, yacc);

                                gsl_spline2d_free(spline);
                                gsl_interp_accel_free(xacc);
                                gsl_interp_accel_free(yacc);
                                free(za);

                                return interpolated_z;
                        }
                default:
                        return (double)Zdat[pbcy(int(round(y)))*nv+v][pbcx(int(round(x)))];
                }
        };

        inline double Z(int x, int y){
                if (! enable_drift_correction)
                        return (double)Zdat[y*nv+vlayer][x];

                if (creepfactor < 0.)
                        return Z_interpol2d (x + pixshift_dt[0] + XYpixshift_manual[0], y + pixshift_dt[1] + XYpixshift_manual[1], vlayer);
                else
                        return Z_interpol2d (x + creepfactor*pixshift_dt[0] + XYpixshift_manual[0], y + creepfactor*pixshift_dt[1] + XYpixshift_manual[1], vlayer);
                //return (double)Zdat[clampy(y + round (pixshift_dt[1]))*nv+vlayer][clampx(x + round (pixshift_dt[0]))];
                //else return (double)Zdat[clampy(y + round (creepfactor*pixshift_dt[1]))*nv+vlayer][clampx(x + round (creepfactor*pixshift_dt[0]))];
        };
	inline double Z(int x, int y, int v){
                if (! enable_drift_correction)
                        return (double)Zdat[y*nv+v][x];

                if (creepfactor < 0.)
                        return Z_interpol2d (x + pixshift_dt[0] + XYpixshift_manual[0], y + pixshift_dt[1] + XYpixshift_manual[1], v);
                else
                        return Z_interpol2d (x + pixshift_dt[0] + XYpixshift_manual[0], y + pixshift_dt[1] + XYpixshift_manual[1], v);
                //if (creepfactor < 0.) 
                //        return (double)Zdat[clampy(y + round (pixshift_dt[1]))*nv+v][clampx(x + round (pixshift_dt[0]))];
                //else return (double)Zdat[clampy(y + round (creepfactor*pixshift_dt[1]))*nv+v][clampx(x + round (creepfactor*pixshift_dt[0]))];
        };
	inline double Z(double z, int x, int y){ return (double)(Zdat[y*nv+vlayer][x]=(ZTYP)z); };
	inline double ZSave(double z, int x, int y){  if (x>=0 && x < nx && y >= 0 && y < ny) return (double)(Zdat[y*nv+vlayer][x]=(ZTYP)z); else return 0.; };
	inline double Z(double z, int x, int y, int v){ return (double)(Zdat[y*nv+v][x]=(ZTYP)z); };
	inline double Z_ixy_sub(double z, int x, int y, int v){ return (double)(Zdat[(y+cp_ixy_sub[2])*nv+v][x+cp_ixy_sub[0]]=(ZTYP)z); };
	inline double Z(double vx, double vy);
	inline double Z(double z, double vx, double vy);
	inline void Zadd(double z, int x, int y){ Zdat[y*nv+vlayer][x]+=(ZTYP)z; };
	inline void ZaddSave(double z, int x, int y){ if (x>=0 && x < nx && y >= 0 && y < ny) Zdat[y*nv+vlayer][x]+=(ZTYP)z; };
	inline void Zmul(double z, int x, int y){ Zdat[y*nv+vlayer][x]*=(ZTYP)z; };
	inline void ZmulSave(double z, int x, int y){ if (x>=0 && x < nx && y >= 0 && y < ny) Zdat[y*nv+vlayer][x]*=(ZTYP)z; };
	inline void Zdiv(double z, int x, int y){ Zdat[y*nv+vlayer][x]/=(ZTYP)z; };

	void ZFrameAddDataFrom(ZData *src);

	int  Resize(int Nx, int Ny, int Nv=1);
	void ZPutDataLine(int y, void *src){
		memcpy((void*)(Zdat[(y+cp_ixy_sub[2])*nv+vlayer] + cp_ixy_sub[0]), src, sizeof(ZTYP)*cp_ixy_sub[1]);
	};
	void ZPutDataLine16dot16(int y, void *src, int mode=MEM_SET);
	void ZPutDataLine(int y, void *src, int mode);
	void ZGetDataLine(int y, void *dest){
		memcpy(dest, (void*)(Zdat[(y+cp_ixy_sub[2])*nv+vlayer]+cp_ixy_sub[0]), sizeof(ZTYP)*cp_ixy_sub[1]);
	};
	void set_all_Z (double z, int v=-1, int x0=0, int y0=0, int xs=0, int ys=0);

	// Achtung keine Bereichkontrolle !! fastest...
	inline void   SetPtr(int x, int y){ zptr = &Zdat[y*nv+vlayer][x];};
	inline void   SetPtrT(int x, int y){ 
		zptr = &Zdat[y*nv+vlayer][x]; zptrT = &Zdat[(y-1)*nv+vlayer][x]; 
	};
	inline void   SetPtrTB(int x, int y){ 
		zptr = &Zdat[y*nv+vlayer][x]; 
		zptrT = &Zdat[(y-1)*nv+vlayer][x]; 
		zptrB = &Zdat[(y+1)*nv+vlayer][x]; 
	};
	inline double GetNext(){ return (double)*zptr++; };
	inline double        GetThis(double x=0.){ return (double)*zptr; };
	inline double        GetThisL(){ return (double)*(zptr-1); };
	inline double        GetThisLT(){ return (double)*(zptrT-1); };
	inline double        GetThisT(){ return (double)*zptrT; };
	inline double        GetThisRT(){ return (double)*(zptrT+1); };
	inline double        GetThisR(){ return (double)*(zptr+1); };
	inline double        GetThisRB(){ return (double)*(zptrB+1); };
	inline double        GetThisB(){ return (double)*(zptrB); };
	inline double        GetThisLB(){ return (double)*(zptrB-1); };
	inline void          IncPtrT(){ ++zptr; ++zptrT; };
	inline void          IncPtrTB(){ ++zptr; ++zptrT; ++zptrB; };

	inline TYPE_ZD_FLOAT GetThis(TYPE_ZD_FLOAT x=0.){ return (TYPE_ZD_FLOAT)*zptr; };
	inline TYPE_ZD_LONG  GetThis(TYPE_ZD_LONG x){ return (TYPE_ZD_LONG)*zptr; };
	inline TYPE_ZD_ULONG GetThis(TYPE_ZD_ULONG x){ return (TYPE_ZD_ULONG)*zptr; };
	inline TYPE_ZD_SHORT GetThis(TYPE_ZD_SHORT x){ return (TYPE_ZD_SHORT)*zptr; };
	inline TYPE_ZD_BYTE  GetThis(TYPE_ZD_BYTE x){ return (TYPE_ZD_BYTE)*zptr; };
	inline TYPE_ZD_EVENT GetThis(TYPE_ZD_EVENT x){ return (TYPE_ZD_EVENT)*zptr; };

	inline void          SetThis(double z){ *zptr=(ZTYP)z; };  // für Island-Labeling MB

	inline void          SetNext(double z){ *zptr++=(ZTYP)z; };
	inline void          AddNext(double z){ *zptr+++=(ZTYP)z; };

	// ==> rhs.GetThis((ZTYP)1)
	// Der Aufruf mit dem "Dummy" Argument dient zur Typenselection des Rückgabewertes
	inline void operator =  (ZData &rhs) { *zptr  = (ZTYP)rhs.GetThis((ZTYP)1); };
	inline void operator += (ZData &rhs) { *zptr += (ZTYP)rhs.GetThis((ZTYP)1); };
	inline void operator -= (ZData &rhs) { *zptr -= (ZTYP)rhs.GetThis((ZTYP)1); };
	inline void operator *= (ZData &rhs) { *zptr *= (ZTYP)rhs.GetThis((ZTYP)1); };
	inline void operator /= (ZData &rhs) { *zptr /= (ZTYP)rhs.GetThis((ZTYP)1); };
	inline void operator ++ () { zptr++; };
	inline void operator -- () { zptr--; };

	inline double operator [] (int idx) { return (double) *(zptr+idx); };

	// Rectangular Copy
	// Copy Src-Rect(x,y - x+nx,y+ny) to Dest-Rect(tox,toy - tox+nx, toy+ny)
	// Achtung: TYPEN muessen passen !!!!!!
	int  CopyFrom(ZData *src, int x, int y, int tox, int toy, int nx, int ny=1, gboolean observe_shift=false);
	inline void* GetPtr(int x, int y){ return (void*)&Zdat[y*nv+vlayer][x]; };

	inline ZTYP* GetPtr(int x, int y, ZTYP z){ return &Zdat[y*nv+vlayer][x]; };

	void NcPut(NcVar &ncfield, int time_index=0, gboolean update=false);
	void NcGet(NcVar &ncfield, int time_index=0);
        static gpointer NcDataUpdate_thread (void *env);
        
	void norm (double mag=1., int vi=0, int vf=-1);
	void mabs_norm (double mag=1., int vi=0, int vf=-1);
	void add (double c=1., int vi=0, int vf=-1);
	void mul (double f=1., int vi=0, int vf=-1);
	void replace (double zi, double zf, double eps=0.0, int vi=0, int vf=-1);

protected:
	ZTYP **Zdat;
private:
	virtual void TNew( void );
	virtual void TDel( void );

	ZTYP *zptr, *zptrT, *zptrB;
};

#endif
