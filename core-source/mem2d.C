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

#include <locale.h>
#include <libintl.h>
#include <math.h>

#include <config.h>

#include "mem2d.h"
#include "glbvars.h"
#include "clip.h"
#include "gxsm_monitor_vmemory_and_refcounts.h"

#define UTF8_DEGREE "\302\260"

// used to set progressbar of main window (P: 0..1)
#define SET_PROGRESS(P) { gapp->SetProgress((gfloat)(P)); while (gtk_events_pending()) gtk_main_iteration(); }

// #define	XSM_DEBUG(A,B) std::cout << B << std::endl


#define SAVECONVOLKERN
//#define SAVECONVOLSRC

const char *ZD_name[] = { "I", "Byte", "Short", "Long", "ULong", "LLong",
		    "Float", "Double", "Complex", "RGBA", "Event"
};


/*
 * Low Level 2d Z Data Verwaltung:
 * beliebiger skalarer ZData-Typ "ZTYP"
 *
 * Objekt: ZData abstraktes Basisobject
 */

ZData::ZData(int Nx, int Ny, int Nv){ 
        GXSM_REF_OBJECT (GXSM_GRC_ZDATA);
        XSM_DEBUG (DBG_L6, "ZData::ZData");

        if (Nx < 1) Nx=1;
        if (Ny < 1) Ny=1;
        if (Nv < 1) Nv=1;

        nx=Nx; ny=Ny; nv=Nv; vlayer_put=vlayer=0;
        cp_ixy_sub[0]=cp_ixy_sub[2]=0; cp_ixy_sub[1]=nx; cp_ixy_sub[3]=ny;
        Li = new LineInfo[ny*nv]; 
        Xlookup=new double[nx]; 
        Ylookup=new double[ny]; 
        Vlookup=new double[nv]; 
        memset (Xlookup, 0, Nx);
        memset (Ylookup, 0, Ny);
        memset (Vlookup, 0, Nv);
        creepfactor    = 0.;
        pixshift_dt[0] = 0.;
        pixshift_dt[1] = 0.;
}
 
ZData::~ZData(){ 
        XSM_DEBUG (DBG_L6, "ZData::~ZData");
        delete [] Ylookup;
        delete [] Xlookup;
        delete [] Vlookup;
        delete [] Li;
        GXSM_UNREF_OBJECT (GXSM_GRC_ZDATA);
}

void ZData::ResetLineInfo(){
	if(Li) delete [] Li;
	Li=new LineInfo[ny*nv]; 
        g_message ("ZData::ResetLineInfo %d, %d",nv, ny);
}

int ZData::ZResize(int Nx, int Ny, int Nv){ 
	XSM_DEBUG (DBG_L6, "ZData: ZResize, delete Li Li=" << Li << " " << Li[0].IsValid());
	double *xl = Xlookup;
	double *yl = Ylookup;
	double *vl = Vlookup;

	if (Nv <= 0 && nv > 1){
                Nv=nv; // keep old nv?
        } else {
                if (Nv < 1) Nv=1;
        }
	if(Li) delete [] Li;
	Li=new LineInfo[Ny*Nv]; 
	Xlookup=new double[Nx];
	Ylookup=new double[Ny];
	Vlookup=new double[Nv];
	memset (Xlookup, 0, Nx);
	memset (Ylookup, 0, Ny);
	memset (Vlookup, 0, Nv);
	if(xl){
		memcpy(Xlookup, xl, (Nx > nx ? nx:Nx)*sizeof(double));
		delete [] xl; 
	}
	if(yl){
		memcpy(Ylookup, yl, (Ny > ny ? ny:Ny)*sizeof(double));
		delete [] yl; 
	}
	if(vl){
		memcpy(Vlookup, vl, (Nv > nv ? nv:Nv)*sizeof(double));
		delete [] vl; 
	}
        
        // clip SLS
        if (cp_ixy_sub[0] >= Nx) cp_ixy_sub[0] = Nx-1;
        if (cp_ixy_sub[2] >= Ny) cp_ixy_sub[2] = Ny-1;
        if (cp_ixy_sub[0]+cp_ixy_sub[1] > Nx) cp_ixy_sub[1] = Nx-cp_ixy_sub[0];
        if (cp_ixy_sub[2]+cp_ixy_sub[3] > Ny) cp_ixy_sub[3] = Ny-cp_ixy_sub[2];
        //cp_ixy_sub[0]=cp_ixy_sub[2]=0;
        //cp_ixy_sub[1]=Nx; cp_ixy_sub[3]=Ny;
                
	nx=Nx; ny=Ny; nv=Nv; vlayer=0;
	XSM_DEBUG (DBG_L6, "ZData: ZResize OK");

	return 0;
}

void ZData::ZPutDataSetDest(int ixy_sub[4])
{ 
        XSM_DEBUG (DBG_L6, "ZData:ZPutDataSetDest");
        if((ixy_sub[0] >= 0) && (ixy_sub[1] > 0) && (ixy_sub[0]+ixy_sub[1]) <= nx 
           && (ixy_sub[2] >= 0) && (ixy_sub[2]+ixy_sub[3]) <= ny) { 
                for (int i=0; i<4; ++i)
                        cp_ixy_sub[i]=ixy_sub[i];

                XSM_DEBUG (DBG_L6, "ZData:ZPutDataSetDest => cp_ixy_sub=[" 
                           << cp_ixy_sub[0] << ", " << cp_ixy_sub[1] << ", " 
                           << cp_ixy_sub[2] << ", " << cp_ixy_sub[3]);
        } else {
                // reset
                cp_ixy_sub[0]=cp_ixy_sub[2]=0;
                cp_ixy_sub[1]=nx; cp_ixy_sub[3]=ny;
        }
};

void  ZData::set_shift (double cf_dt, double pixs_xdt, double pixs_ydt) {
        if (cf_dt > 0.0){
                pixshift_dt[0] = pixs_xdt;
                pixshift_dt[1] = pixs_ydt;
        }
        creepfactor    = cf_dt;
};

/*
 * Template Objekt: TZData
 */

template <class ZTYP> 
TZData<ZTYP>::TZData(int Nx, int Ny, int Nv)
  :ZData(Nx, Ny, Nv){ XSM_DEBUG (DBG_L6, "TZData::TZData"); Zdat=NULL; TNew(); }

template <class ZTYP> 
TZData<ZTYP>::~TZData(){ XSM_DEBUG (DBG_L6, "TZData::~TZData"); TDel(); }

template <class ZTYP> 
int TZData<ZTYP>::Resize(int Nx, int Ny, int Nv){
	if (Nv <= 0 && nv > 1) Nv=nv; // keep old nv?
        else if (Nv < 1) Nv=1;
        
	XSM_DEBUG (DBG_L6, "TZData: Resize(" << Nx << " " << Ny << " " << Nv << ")");
	XSM_DEBUG (DBG_L6, "TZData: Resize set size, reset Info");
	if(Nx != nx || Ny != ny || Nv != nv){
		if(Nx == nx && Ny < ny && Nv == nv){
			while(ny-- > Ny){
				for(int v=0; v<nv; ++v){
					delete [] Zdat[ny*nv+v];
					Zdat[ny*nv+v] = NULL;
				}
			}
			ny++;
		}else{
			if(!Zdat){
				XSM_DEBUG (DBG_L6, "TZData: Resize Del old");
				TDel();
				XSM_DEBUG (DBG_L6, "TZData: Resize New");
				TNew();
			}else{
				XSM_DEBUG (DBG_L6, "TZData: real Resize, copy old ");
				ZTYP** Zdold = Zdat;
				while(ny-- > Ny){
					for(int v=0; v<nv; ++v){
						delete [] Zdold[ny*nv+v];
						Zdold[ny*nv+v] = NULL;
					}
				}
				ny++;
				Zdat = new ZTYP*[Ny*Nv];
				for(int i=0; i<Ny; i++){
					for(int v=0; v<Nv; ++v){
                                                int q=i*Nv+v;
						Zdat[q] = new ZTYP[Nx];
						if(!Zdat[q]){
							Ny=i-1;
							XSM_DEBUG_ERROR (DBG_L1, "No Mem in Mem2d:New" );
							return 0;
						}
						if(Nx > nx || i >= ny || v >= nv)
							memset(Zdat[q], 0, sizeof(ZTYP)*Nx);
						if(i < ny && v < nv && Zdold[i*nv+v]){
                                                        memcpy (Zdat[q], Zdold[i*nv+v], sizeof(ZTYP)*(Nx < nx ? Nx : nx));
							//delete [] Zdold[q]; // obsolete
                                                        //Zdold[q] = NULL;
						}
					}
				}
				delete [] Zdold;
			}
		}
		ZResize(Nx, Ny, Nv);
		return 1;
	}
	ZResize(Nx, Ny, Nv);
	return 0;
}

// accesses via real-lookup-table for future use
template <class ZTYP> 
double TZData<ZTYP>::Z(double vx, double vy){
  return 0.;
}

template <class ZTYP> 
double TZData<ZTYP>::Z(double z, double vx, double vy){
  return 0.;
}

// add data from -- size must match!!
template <class ZTYP> 
void TZData<ZTYP>::ZFrameAddDataFrom(ZData *src){
	if (nx != src->GetNx() || ny != src->GetNy() || nv !=src->GetNv()) 
		return;
	for(int i=0; i<ny; ++i){
		src->SetPtr (0, i);
		SetPtr (0, i);
		for(int j=0; j<nx; ++j)
			AddNext (src->GetNext ());
	}
}

// set data
template <class ZTYP> 
void TZData<ZTYP>::ZPutDataLine16dot16(int y, void *src, int mode){
        y += cp_ixy_sub[2];
	switch(mode){
	case MEM_SET:
//		memcpy((void*)(Zdat[y*nv+vlayer]+cp_ix0), src, sizeof(ZTYP)*cp_num);
		for(int i=cp_ixy_sub[0]; i<(cp_ixy_sub[0]+cp_ixy_sub[1]); i++)
			Zdat[y*nv+vlayer_put][i] += (ZTYP)(*((gint32*)src+i)/(float)(1<<16));
		break;
	case MEM_ADDTO:
		for(int i=cp_ixy_sub[0]; i<(cp_ixy_sub[0]+cp_ixy_sub[1]); i++)
			Zdat[y*nv+vlayer_put][i] += (ZTYP)(*((gint32*)src+i)/(float)(1<<16));
		break;
	case MEM_AVG:
		for(int i=cp_ixy_sub[0]; i<(cp_ixy_sub[0]+cp_ixy_sub[1]); i++){
			Zdat[y*nv+vlayer_put][i] += (ZTYP)(*((gint32*)src+i)/(float)(1<<16));
			Zdat[y*nv+vlayer_put][i] /= 2;
		}
		break;
	}
	Li[y+ny*vlayer_put].invalidate();
}

// set data
template <class ZTYP> 
void TZData<ZTYP>::ZPutDataLine(int y, void *src, int mode){
        y += cp_ixy_sub[2];
        switch(mode){
        case MEM_SET:
                memcpy((void*)(Zdat[y*nv+vlayer_put]+cp_ixy_sub[0]), src, sizeof(ZTYP)*cp_ixy_sub[1]);
                break;
        case MEM_ADDTO:
                for(int i=cp_ixy_sub[0]; i<(cp_ixy_sub[0]+cp_ixy_sub[1]); i++)
                        Zdat[y*nv+vlayer_put][i] += *((ZTYP*)src+i);
                break;
        case MEM_AVG:
                for(int i=cp_ixy_sub[0]; i<(cp_ixy_sub[0]+cp_ixy_sub[1]); i++){
                        Zdat[y*nv+vlayer_put][i] += *((ZTYP*)src+i);
                        Zdat[y*nv+vlayer_put][i] /= 2;
                }
                break;
        }
        Li[y+ny*vlayer_put].invalidate();
}

template <class ZTYP> 
int TZData<ZTYP>::CopyFrom(ZData *src, int x, int y, int tox, int toy, int nx, int ny, gboolean observe_shift){
//      g_message ("TZD::CopyFrom %d %d -> %d %d [%d x %d]", x,y, tox, toy, nx, ny);
        if (observe_shift)
        	for(int i=0; i<ny; i++){
                        for(int j=0; j<nx; j++)
                                Z (src->Z(j+x,i+y), j+tox,i+toy);
                        Li[i+ny*vlayer].invalidate();
                }
        else
                for(int i=0; i<ny; i++){
                        memcpy((void*)&Zdat[(toy++)*nv+vlayer][tox], src->GetPtr(x,y++), nx*sizeof(ZTYP));
                        Li[i+ny*vlayer].invalidate();
                }

//	int it = nx/2;
//	std::cout << "TZD-CpyF " << it << "," << it << ",vd" << vlayer << ",vs" << src->GetLayer() << " Verify Src=" << src->Z(it,it)  << " Dst=" << Z(it,it) << std::endl;

  return 0;
}

template <class ZTYP> 
void TZData<ZTYP>::TNew( void ){
        XSM_DEBUG (DBG_L6, "TZData: TNew: (" << nx << "," << ny << ")");
        Zdat = new ZTYP*[ny*nv];
        for(int i=0; i<ny*nv; i++){
                Zdat[i] = new ZTYP[nx];
                if(!Zdat[i]){
                        ny=i-1;
                        XSM_DEBUG (DBG_L1, "Error: No Mem in Mem2d:New");
                        return;
                }
                memset(Zdat[i], 0, sizeof(ZTYP)*nx);
        }
}

template <class ZTYP> 
void TZData<ZTYP>::TDel( void ){
        if(!Zdat) return;
        XSM_DEBUG (DBG_L6, "TZData: TDel ny=" << ny);
        for(int i=0; i<ny*nv; i++){
                delete [] Zdat[i];
        }
        delete [] Zdat;
        Zdat = NULL;
        XSM_DEBUG (DBG_L6, "TZData: TDel OK. Zdat=" << Zdat);
}

// set Z values to const in opt. area
template <class ZTYP> 
void TZData<ZTYP>::set_all_Z (double z, int v, int x0, int y0, int xs, int ys){
	ZTYP zval = (ZTYP)z;
	int vs = 0;
	int ve = nv-1;
	if (v >= 0 && v < nv) // restrict to layer v?
		vs=ve=v;
	if (!xs) xs = nx;
	if (!ys) ys = ny;
	// set one line
	for (int i=x0; i < (x0+xs); Zdat[y0*nv+vs][i++] = zval);
	// copy rest
	for (v = vs; v<=ve; ++v)
		for (int i=y0+1; i < (y0+ys); ++i)
			memcpy (&Zdat[i*nv+v][x0], &Zdat[y0*nv+vs][x0], xs*sizeof(ZTYP));
}

// NetCDF Put / Get Methods "overloaded"
#if 1

template <typename ZTYP> 
struct NcDataUpdate_Job_Env{
        TZData<ZTYP> *self;
        NcVar *ncfield;
        int time_index;
        gboolean update;
        gboolean busy;
};
template <class ZTYP> 
gpointer TZData<ZTYP>::NcDataUpdate_thread (void *env){
        NcDataUpdate_Job_Env<ZTYP>* e_ = (NcDataUpdate_Job_Env<ZTYP>*)env;
        NcDataUpdate_Job_Env<ZTYP> e;
        memcpy (&e, e_, sizeof(e));
        TZData<ZTYP> *self = e.self;
        e_->busy = false;
        int count=0;
	for(int y=0; y<self->ny; y++){
		for(int v=0; v<self->nv; ++v){
                        int liy=y+self->ny*v;
                        if (!e.update || (e.update && (self->Li[liy].IsNew() && ! self->Li[liy].IsStored()))) {
                                count++;
                                //g_message ("NcVar update %d, %d", v,y);
                                e.ncfield->set_cur(e.time_index,v,y);
                                e.ncfield->put(self->Zdat[y*self->nv+v], 1,1, 1,self->nx);
                                self->Li[liy].SetStored();
                        }
                        //                if (y==0 && (v%10) == 0)
                        //                        gapp->progress_info_set_bar_fraction ((gdouble)v/nv, 2);
                }
	}
        if (e.update) g_message ("NcVar updated at %d lines x values", count);
        return NULL;
}

template <class ZTYP> 
void TZData<ZTYP>::NcPut(NcVar *ncfield, int time_index, gboolean update){
#if 0
        NcDataUpdate_Job_Env<ZTYP> job;
        job.self = this;
        job.ncfield = ncfield;
        job.time_index = time_index;
        job.update = update;
        job.busy = true;
        GThread* gt =  g_thread_new ("NcDataUpdate_thread",  TZData<ZTYP>::NcDataUpdate_thread, &job);
        //while (job.busy);
        g_thread_join (gt);
#else
        int count=0;
	for(int y=0; y<ny; y++){
		for(int v=0; v<nv; ++v){
                        int liy=y+ny*v;
                        if (!update || (update && (Li[liy].IsNew() && ! Li[liy].IsStored()))) {
                                count++;
                                //g_message ("NcVar update %d, %d", v,y);
                                ncfield->set_cur(time_index,v,y);
                                ncfield->put(Zdat[y*nv+v], 1,1, 1,nx);
                                Li[liy].SetStored();
                        }
                        //                if (y==0 && (v%10) == 0)
                        //                        gapp->progress_info_set_bar_fraction ((gdouble)v/nv, 2);
                }
	}
        if (update) g_message ("NcVar updated at %d lines x values", count);
#endif
}

#else

// #define SAVE_NEW_DATA_ONLY

template <class ZTYP> 
//void TZData<ZTYP>::NcPut(NcVar *ncfield, int time_index){
void TZData<ZTYP>::NcPut(NcVar *ncfield, int time_index, gboolean update){

        gdouble yy, yy2;
        yy = yy2 = 0.;
        for(int y=0; y<ny; y++){
                for(int v=0; v<nv; ++v){
#ifdef SAVE_NEW_DATA_ONLY
                        if (Li[y+ny*v].IsNew() > 0) {
                                ncfield->set_cur(time_index,v,y);
                                ncfield->put(Zdat[y*nv+v], 1,1, 1,nx);
                        }
#else
                        ncfield->set_cur(time_index,v,y);
                        ncfield->put(Zdat[y*nv+v], 1,1, 1,nx);
#endif
                }
                yy = (double)y/(double)ny;
                if (ceil(yy) - ceil(yy2) > 1.)
                        gapp->progress_info_set_bar_fraction (yy2=yy, 2);
        }
}
#endif

template <class ZTYP> 
void TZData<ZTYP>::NcGet(NcVar *ncfield, int time_index){
	gdouble yy, yy2;
	yy = yy2 = 0.;
	for(int y=0; y<ny; y++){
		for(int v=0; v<nv; ++v){
			ncfield->set_cur(time_index,v,y);
			ncfield->get(Zdat[y*nv+v], 1,1, 1,nx);
			Li[y+ny*v].invalidate();
		}
		yy = (double)y/(double)ny;
		if (ceil(yy) - ceil(yy2) > 1.)
			gapp->progress_info_set_bar_fraction (yy2=yy, 2);
	}
}

// in place operations
template <class ZTYP> 
void TZData<ZTYP>::norm (double mag, int vi, int vf){
        if (vf<vi) vf=nv-1;
        for(int v=vi; v<=vf; ++v){
                double sum=0.;
                for(int y=0; y<ny; y++)
                        for(int x=0; x<nx; x++)
                                sum += Zdat[y*nv+v][x];
                mul (mag/sum, v,v);
        }
}

template <class ZTYP> 
void TZData<ZTYP>::mabs_norm (double mag, int vi, int vf){
        if (vf<vi) vf=nv-1;
        for(int v=vi; v<=vf; ++v){
                double sum=0.;
                double asum=0.;
                for(int y=0; y<ny; y++)
                        for(int x=0; x<nx; x++){
                                asum += fabs (Zdat[y*nv+v][x]);
                                sum += Zdat[y*nv+v][x];
                        }
                sum =  sum!=0. ? fabs (sum) : (asum+1.)/2.;
                mul (mag/sum, v, v);
        }
}

template <class ZTYP> 
void TZData<ZTYP>::add (double c, int vi, int vf){
        if (vf<vi) vf=nv-1;
        for(int v=vi; v<=vf; ++v){
                for(int y=0; y<ny; y++)
                        for(int x=0; x<nx; x++){
                                double tmp = (double)Zdat[y*nv+v][x];
                                Zdat[y*nv+v][x] = (ZTYP)(tmp+c);
                        }
        }
}

template <class ZTYP> 
void TZData<ZTYP>::mul (double f, int vi, int vf){
        if (vf<vi) vf=nv-1;
        for(int v=vi; v<=vf; ++v){
                for(int y=0; y<ny; y++)
                        for(int x=0; x<nx; x++){
                                double tmp = (double)Zdat[y*nv+v][x];
                                Zdat[y*nv+v][x] = (ZTYP)(f*tmp);
                        }
        }
}

template <class ZTYP> 
void TZData<ZTYP>::replace (double zi, double zf, double eps, int vi, int vf){
        double zfr = zf;
        double zfl = zf;
        if (vf<vi) vf=nv-1;
        for(int v=vi; v<=vf; ++v){
                for(int y=0; y<ny; y++)
                        for(int x=0; x<nx; x++){
                                double tmp = (double)Zdat[y*nv+v][x];
                                if (fabs(tmp - zi) <= eps){
                                        if (zf < -1e8){
                                                zfr = zfl = 0.;
                                                int xx;
                                                for(xx=x; xx<nx && fabs((double)Zdat[y*nv+v][xx] - zi) <= eps; xx++);
                                                if (xx<nx)
                                                        zfr = (double)Zdat[y*nv+v][xx];
                                                for(xx=x; xx>=0 && fabs((double)Zdat[y*nv+v][xx] - zi) <= eps; xx--);
                                                if (xx >= 0)
                                                        zfl = (double)Zdat[y*nv+v][xx];
                                                // need to try y seek?
                                                if (zfr == 0. && zfl == 0.){
                                                        int yy;
                                                        for(yy=y; yy<ny && fabs((double)Zdat[yy*nv+v][x] - zi) <= eps; yy++);
                                                        if (yy<ny)
                                                                zfr = (double)Zdat[yy*nv+v][x];
                                                        for(yy=y; yy>=0 && fabs((double)Zdat[yy*nv+v][x] - zi) <= eps; yy--);
                                                        if (yy >= 0)
                                                                zfl = (double)Zdat[yy*nv+v][x];
                                                }
                                                zfr = zfr > zfl ? zfr : zfl;
                                        }
                                        Zdat[y*nv+v][x] = (ZTYP)(zfr);
                                }
                        }
        }
}

/*
 * Layer Information
 */

LayerInformation::LayerInformation (const gchar *description, const char *string){
        GXSM_REF_OBJECT (GXSM_GRC_LAYERINFO);
	if (string)
		desc = g_strconcat (description, ": ", string, NULL);
	else
		desc = g_strdup (description);
	fmt = NULL;
	fmtosd = NULL;
	val[0] = 0.;
	val[1] = 0.;
	num = 0;
}

LayerInformation::LayerInformation (const gchar *description, double x, const gchar *format){
        GXSM_REF_OBJECT (GXSM_GRC_LAYERINFO);
	desc = g_strdup (description);
	fmt = g_strdup_printf ("%%s: %s", format);
	fmtosd = g_strdup (format);
	val[0] = x;
	val[1] = 0.;
	num = 1;
}

LayerInformation::LayerInformation (const gchar *description, double x, double y, const gchar *format){
        GXSM_REF_OBJECT (GXSM_GRC_LAYERINFO);
	desc = g_strdup (description);
	fmt = g_strdup_printf ("%%s: %s", format);
	fmtosd = g_strdup (format);
	val[0] = x;
	val[1] = y;
	num = 2;
}

LayerInformation::LayerInformation (LayerInformation *li){
        GXSM_REF_OBJECT (GXSM_GRC_LAYERINFO);
	desc = g_strdup (li->desc);
	fmt = NULL;
	fmtosd = NULL;
	if (li->fmt)
		fmt = g_strdup (li->fmt);
	if (li->fmtosd)
		fmtosd = g_strdup (li->fmtosd);
	val[0] = li->val[0];
	val[1] = li->val[1];
	num = li->num;
}

LayerInformation::LayerInformation (NcVar* ncv_description, NcVar* ncv_format, NcVar* ncv_osd_format, NcVar* ncv_values, int ti, int v, int k){
        GXSM_REF_OBJECT (GXSM_GRC_LAYERINFO);
	int n=0;
	desc = NULL;
	fmt = NULL;
	fmtosd = NULL;
	num = 0;
	gchar *tmp=NULL;

	tmp = g_new0 (gchar, (n=ncv_description->get_dim(3)->size())+1);
	ncv_description->set_cur (ti, v, k);
	ncv_description->get (tmp, 1,1,1, n);;
	desc = g_strdup (tmp);
	g_free (tmp);
	
	if (ncv_format->get_dim(3)->size() > 0){
		tmp = g_new0 (gchar, (n=ncv_format->get_dim(3)->size())+1);
		ncv_format->set_cur (ti, v, k);
		ncv_format->get (tmp, 1,1,1, n);;
		fmt = g_strdup (tmp);
		g_free (tmp);
		gchar* p1=strchr (fmt, '%');
		if (p1){ // dsc
			gchar* p2=strchr (p1+1, '%');
			if (p2){ // #1
				gchar* p3=strchr (p2+1, '%');
				++num;
				if (p3)
					++num;
			}
		}
	}

	if (ncv_osd_format->get_dim(3)->size() > 0){
		tmp = g_new0 (gchar, (n=ncv_osd_format->get_dim(3)->size())+1);
		ncv_osd_format->set_cur (ti, v, k);
		ncv_osd_format->get (tmp, 1,1,1, n);;
		fmtosd = g_strdup (tmp);
		g_free (tmp);
	}

	ncv_values->set_cur (ti, v, k);
	ncv_values->get (val, 1,1,1, 2);
}

LayerInformation::~LayerInformation (){
	g_free (desc);
	if (fmt)
		g_free (fmt);
	if (fmtosd)
		g_free (fmtosd);
        GXSM_UNREF_OBJECT (GXSM_GRC_LAYERINFO);
}

gchar *LayerInformation::get (gboolean osd){
	if (osd){
		if (fmtosd)
			switch (num){
			case 1:	return g_strdup_printf (fmtosd, val[0]); break;
			case 2:	return g_strdup_printf (fmtosd, val[0], val[1]); break;
			default: return g_strdup (desc);
			}
		else
			switch (num){
			case 1:	return g_strdup_printf ("%g", val[0]); break;
			case 2:	return g_strdup_printf ("%g, %g", val[0], val[1]); break;
			default: return g_strdup (desc);
			}
	} else {
		if (fmt)
			switch (num){
			case 1:	return g_strdup_printf (fmt, desc, val[0]); break;
			case 2:	return g_strdup_printf (fmt, desc, val[0], val[1]); break;
			default: return g_strdup (desc);
			}
		else
			switch (num){
			case 1:	return g_strdup_printf ("%s: %g", desc, val[0]); break;
			case 2:	return g_strdup_printf ("%s: %g, %g", desc, val[0], val[1]); break;
			default: return g_strdup (desc);
			}
	}
	return NULL;
}

void LayerInformation::write_layer_information (NcVar* ncv_description, NcVar* ncv_format, NcVar* ncv_osd_format, NcVar* ncv_values, int ti, int v, int k){
	int n=0;
	gchar* tmp = g_new0 (gchar, (n=ncv_description->get_dim(3)->size())+1);
	if (desc)
		memcpy (tmp, desc, strlen (desc));
	ncv_description->set_cur (ti, v, k);
	ncv_description->put (tmp, 1,1,1, n);
	g_free (tmp);

	tmp = g_new0 (gchar, (n=ncv_format->get_dim(3)->size())+1);
	if (fmt)
		memcpy (tmp, fmt, strlen (fmt));
	ncv_format->set_cur (ti, v, k);
	ncv_format->put (tmp, 1,1,1, n);
	g_free (tmp);

	tmp = g_new0 (gchar, (n=ncv_osd_format->get_dim(3)->size())+1);
	if (fmtosd)
		memcpy (tmp, fmtosd, strlen (fmtosd));
	ncv_osd_format->set_cur (ti, v, k);
	ncv_osd_format->put (tmp, 1,1,1, n);
	g_free (tmp);

	ncv_values->set_cur (ti, v, k);
	ncv_values->put (val, 1,1,1, 2);
}

/*
 * 2d Memory Verwaltung:
 * class Mem2d
 *
 */

// make new with size and type
Mem2d::Mem2d(int Nx, int Ny, ZD_TYPE type){
        GXSM_REF_OBJECT (GXSM_GRC_MEM2D);
	data   = NULL; 
	scan_event_list = NULL;
	layer_information_list = NULL;
	nlyi=0;
	SetPlaneMxyz ();
	Mnew (Nx, Ny, 1, type);
}

Mem2d::Mem2d(int Nx, int Ny, int Nv, ZD_TYPE type){
        GXSM_REF_OBJECT (GXSM_GRC_MEM2D);
	data=NULL;
	scan_event_list = NULL;
	layer_information_list = NULL;
	nlyi=0;
	SetPlaneMxyz ();
	Mnew (Nx, Ny, Nv, type);
}

// make copy or empty
Mem2d::Mem2d(Mem2d *m, MEM2D_CREATE_MODE Option){
        GXSM_REF_OBJECT (GXSM_GRC_MEM2D);
	int ly;
	data=NULL;
	scan_event_list = NULL;
	layer_information_list = NULL;
	nlyi=0;
	GetPlaneMxyz (plane_mx, plane_my, plane_z0);
	Mnew(m->GetNx(), m->GetNy(), m->GetNv(), m->GetTyp());
	switch(Option){
	case M2D_ZERO: break;
	case M2D_COPY: 
		ly=m->data->GetLayer();
		XSM_DEBUG (DBG_L6,"Mem2d::Mem2d -- M2D_COPY");
//		std::cout << "Mem2d::NewWcopy full, all layers #" << m->GetNv() << " current layer:" << ly << " t_index: "<< m->t_index << std::endl;
		for(int v=0; v<m->GetNv(); ++v){
			m->data->SetLayer(v);
			SetLayer(v);
			CopyFrom(m, 0,0, 0,0, GetNx(),GetNy());
		}
		m->data->SetLayer(ly);
		SetLayer(ly);
		data->CopyLookups(m->data);
		t_index = m->t_index; // keep it!
		XSM_DEBUG (DBG_L6,"Mem2d::Mem2d -- M2D_COPY -- copying layer ino now:");
		copy_layer_information (m);
		break;
	}
	SetLayer(m->GetLayer());
}

// make with size from same type
Mem2d::Mem2d(Mem2d *m, int Nx, int Ny, int Nv){
        GXSM_REF_OBJECT (GXSM_GRC_MEM2D);
	data=NULL;
	scan_event_list = NULL;
	layer_information_list = NULL;
	nlyi=0;
	GetPlaneMxyz (plane_mx, plane_my, plane_z0);
	Mnew(Nx, Ny, Nv, m->GetTyp());
}

Mem2d::~Mem2d(){
	XSM_DEBUG (DBG_L6, "Mem2d::~Mem2d");
	RemoveScanEvents ();
	Mdelete();
        GXSM_UNREF_OBJECT (GXSM_GRC_MEM2D);
}

/** Mem2d::copy copy full mem2d object per default or subregion
 *  m: Source mem2d to copy from
 *  x0,y0: x,y index of upper left achor of region of size nx, ny
 *  nx,ny: size of xy subregion
 *  vi,vf: layer subset (value) initial and final layer to copy
 */
void Mem2d::copy(Mem2d *m, int x0, int y0, int vi, int vf, int nx, int ny, gboolean observe_shift){
	int ly=m->data->GetLayer();
	int lnum;
	XSM_DEBUG (DBG_L6,"Mem2d::copy full, all layers #" << m->GetNv() << " current layer src:" << ly << " t_index src: "<< m->t_index);
	if (vi == -1 || vf == -1){
		vi = 0; vf = m->GetNv()-1;
	}
	if (y0 == -1 || x0 == -1)
		x0 = y0 = 0;
	if (nx == -1 || ny == -1){
		nx = m->GetNx ();
		ny = m->GetNy ();
	}
		
	lnum = vf-vi+1;
	Resize (nx, ny, lnum, m->GetTyp());

        if (observe_shift && m->data->is_shift ())
                for(int v=vi; v<=vf; ++v)
                        for(int y=0; y<ny; ++y)
                                for(int x=0; x<nx; ++x)
                                        data->Z (m->data->Z (x+x0,y+y0,v), x,y,v-vi);
        else
                for(int v=vi; v<=vf; ++v){
                        m->data->SetLayer(v);
                        SetLayer(v-vi);
                        CopyFrom(m, x0,y0, 0,0, nx,ny);
                }

	m->data->SetLayer(ly);
	SetLayer(ly-vi < lnum ? ly-vi > 0? ly-vi : 0 : lnum-1);
	data->CopyLookups(m->data,  x0, y0, vi);
	t_index = m->t_index; // keep it!

	XSM_DEBUG (DBG_L6,"Mem2d::copy -- copying layer ino now:");
	copy_layer_information (m, vi);
}

void Mem2d::evl_remove(gpointer entry, gpointer from){
        if(debug_level > 2){
                g_message ("Removing Event:");
                ((ScanEvent*)entry) -> print ();
        }
	delete (ScanEvent*)entry;
}

void Mem2d::RemoveScanEvents (){
	g_slist_foreach(scan_event_list, (GFunc) Mem2d::evl_remove, scan_event_list);
	g_slist_free (scan_event_list);
	scan_event_list = NULL;
}

gint compare_events_distance (ScanEvent *a, ScanEvent *b, double *xy){
	double da = a->distance (xy);
	double db = b->distance (xy);
	if (da < db) return -1;
	if (da > db) return 1;
	return 0;
}

gint compare_events_xpos (ScanEvent *a, ScanEvent *b){
	double ax,ay;
        a->get_position (ax, ay);
	double bx,by;
        b->get_position (bx, by);
	if (ax < bx) return -1;
	if (ax > bx) return 1;
	return 0;
}

gint compare_events_ypos (ScanEvent *a, ScanEvent *b){
	double ax,ay;
        a->get_position (ax, ay);
	double bx,by;
        b->get_position (bx, by);
	if (ay < by) return -1;
	if (ay > by) return 1;
	return 0;
}

gint compare_user_entry_messageid (UserEntry *a, UserEntry *b) {
	return g_strcmp0 (a->message_id, b->message_id);
}


GSList* Mem2d::ReportScanEvents (GFunc report_obj_func, gpointer gp, double *xy, double distance, int number){
	if (!scan_event_list)
		return NULL;
	if (!xy && report_obj_func) // all
		g_slist_foreach(scan_event_list, (GFunc)report_obj_func, gp);
	else{ // only within distance to xy[2]
		GSList *ev_sort = g_slist_copy (scan_event_list);
		ev_sort = g_slist_sort_with_data (ev_sort, GCompareDataFunc (compare_events_distance), (gpointer)xy);
		if (!report_obj_func)
			return ev_sort;

		GSList *ev = ev_sort;
		int i=0;
		while (ev){
			if (++i > number || ((ScanEvent*)(ev->data))->distance (xy) > distance)
				break;

			(*report_obj_func)(ev->data, gp);
			ev = g_slist_next (ev);
		}
		g_slist_free (ev_sort);
		return NULL;
	}
	return NULL;
}

GSList* Mem2d::ReportScanEventsUnsort (){
        GSList *ev_unsort = g_slist_copy (scan_event_list);
        return ev_unsort;
}

GSList* Mem2d::ReportScanEventsXasc (){
        GSList *ev_sort = g_slist_copy (scan_event_list);
        ev_sort = g_slist_sort (ev_sort, GCompareFunc (compare_events_xpos));
        return ev_sort;
}

GSList* Mem2d::ReportScanEventsYasc (){
        GSList *ev_sort = g_slist_copy (scan_event_list);
        ev_sort = g_slist_sort (ev_sort, GCompareFunc (compare_events_ypos));
        return ev_sort;
}

int Mem2d::WriteScanEvents (NcFile *ncf){
	GSList *sel; // List of ScanEvents
	int p_index; // Event_Probe_####_... NcVar name counter, grouping of similar (same dimensions) data sets
	//int u_index;
	int p_dim_sets=0;
	int p_dim_samples=0;
	NcVar* evdata_var=NULL;
	NcVar* evcoords_var=NULL;

	class Event{
	public:
		Event (Event *e, ScanEvent *s, ProbeEntry *p, int _j, int _n){ 
			se=s; pe=p; n=_n; j=_j; 
			if (e)
				same_count = same (*e) ? e->same_count+1 : 1; 
			else
				same_count = 1;
		};
		~Event (){};
		gint same (Event &e) { return n == e.n && j == e.j ? TRUE:FALSE; };
		ScanEvent *se;
		ProbeEntry *pe;
		int same_count;
		int n;
		int j;
	};
	GSList *ProbeEventsList=NULL;
	Event *e=NULL;

	GSList *UserEventsList=NULL;

	//u_index=0;
	// pre parse all scan and user events
	sel = scan_event_list; // List of ScanEvents

	XSM_DEBUG (DBG_L2, "NCF Write Scan Events -- pre-parsing");
	while (sel){
		ScanEvent *se = (ScanEvent*)sel->data; // Event (contains different EventEntry types)
		GSList *eel = se->event_list; // List of all EventEnties @ ScanEvent (one Location)
		while (eel){
			EventEntry *ee = (EventEntry*)eel->data; // EventEntry (contains Data)
			switch (ee->description_id ()){
			case 'P': // "Probe" Event
			  {
				ProbeEntry *pe = (ProbeEntry*)eel->data; // it's a ProbeEntry
				p_dim_sets =  pe->get_num_sets ();
				p_dim_samples =  pe->get_chunk_size ();
				ProbeEventsList = g_slist_prepend (ProbeEventsList, e=new Event (e, se, pe, p_dim_sets, p_dim_samples));
				
				gapp->progress_info_set_bar_pulse (2);
			  }
			break;
			case 'U': // "User" Event, just store away
			  {	
				UserEntry *ue = (UserEntry*)eel->data; // it's a UserEntry
//				XSM_DEBUG (DBG_L2, "NCF Write Scan Events -- UE found with " << ue->get_num_sets () << " entries.");

// rebuild new ordered list with UEs, to be sorted by User Event message_id (what)
				for (int i=0; i<ue->get_num_sets (); ++i)
					UserEventsList = g_slist_prepend (UserEventsList, new UserEntry (ue, i, se));
				gapp->progress_info_set_bar_pulse (2);
// old
//			        ue->store_event_to_nc (ncf, ++u_index, se);
//
			  }
				break;
			}
			eel = g_slist_next (eel);
		}
		
		sel = g_slist_next (sel);
	}

	XSM_DEBUG (DBG_L2, "NCF Write Scan Events -- writing probe events");
	if (ProbeEventsList){
		// gdouble sc;
		p_index=0;
		GSList *pel=ProbeEventsList;
		Event *prev = e = (Event*) pel->data;
		if (e->pe->write_nc_variable_set (ncf, p_index, evdata_var, evcoords_var, e->same_count) == 0)
			++p_index;
		// sc = (gdouble)e->same_count;

		// only for progress info
		double sip=0.;
		double sil= (double)g_slist_length (pel);

		while (pel){
			e = (Event*) pel->data;

			// changed dimensions? Then write new nc-var set.
			if (! e->same(*prev)){
				if (e->pe->write_nc_variable_set (ncf, p_index, evdata_var, evcoords_var, e->same_count) == 0)
					++p_index;
				// sc = (gdouble)e->same_count;
			}

			// write data
			e->pe->write_nc_data (evdata_var, evcoords_var, e->se, e->same_count-1);

			prev=e;
			pel = g_slist_next (pel);

			sip += 1.;
			gapp->progress_info_set_bar_fraction (sip/sil, 2);
		}

		// free ProbeEventsList and Elements!
	}

	XSM_DEBUG (DBG_L2, "NCF Write Scan Events -- writing user events");
	if (UserEventsList){
		GSList *uel=UserEventsList;
		
#if 0
		// sort by id
		{
			GSList *l=uel;
			XSM_DEBUG (DBG_L2, "User Events List before sort");
			while (l){
				std::cout << ((UserEntry*) (l->data)) -> message_id << std::endl;
				l = g_slist_next (l);
			}
		}
#endif
		uel = g_slist_sort (uel, GCompareFunc (compare_user_entry_messageid));
#if 0
		{
			GSList *l=uel;
			XSM_DEBUG (DBG_L2, "User Events List post sort");
			while (l){
				XSM_DEBUG (DBG_L2, ((UserEntry*) (l->data)) -> message_id );
				l = g_slist_next (l);
			}
		}
#endif
		// only for progress info
		double sip=0.;
		double sil= (double)g_slist_length (uel);
		while (uel){
			int n=1;
			GSList *uel_cur  = uel;

			// look ahead for same entities, count
			while ((uel = g_slist_next (uel))){
				if (((UserEntry*) (uel_cur->data)) -> same ((UserEntry*) (uel->data)))
					++n;
				else
					break;
			}
			// store away as block
//			XSM_DEBUG (DBG_L2, "NCF Write Scan Events >" << ((UserEntry*) (uel_cur->data)) -> message_id << "< -- #UE= " << n );
			((UserEntry*) (uel_cur->data)) -> store_event_to_nc (ncf, uel_cur, n);

			// skip forward n to next
			sip += n;
			gapp->progress_info_set_bar_fraction (sip/sil, 2);
		}

		// free ProbeEventsList and Elements!
	}
	XSM_DEBUG (DBG_L2, "NCF Write Scan Events -- done.");

	return 0;
}

int Mem2d::LoadScanEvents (NcFile *ncf){
	ScanEvent *se=NULL;
	ProbeEntry *pe=NULL;
	NcVar* evdata_var=NULL;
	NcVar* evcoords_var=NULL;
	
	// scan NC-file for ScanEvents
	XSM_DEBUG (DBG_L2, "NCF Load: Scan Events -- Probe Entry...");
	for (int p_index=0; ; ++p_index){
		int limit=1;
		pe = NULL;
		se = NULL;
		for (int count=0; count<limit; ++count){
			pe = new ProbeEntry ("Probe", ncf, p_index, evdata_var, evcoords_var, se, count, limit, pe);
			if (se){
				se->add_event (pe);
				AttachScanEvent (se);
				gapp->progress_info_set_bar_fraction ((double)count/(gdouble)limit, 2);
			} else break;
		}
		if (!se) 
			break;
	}

	if (pe)
		delete pe;


	// scan NC-file for UserEvents -- look for old style
	XSM_DEBUG (DBG_L2, "NCF Load: Scan Events -- User Entry (old style)...");
	UserEntry *ue=NULL;
	for (int u_index=1; ; ++u_index){
		ue = NULL;
		se = NULL;
		ue = new UserEntry ("User", ncf, u_index, se);
		if (se){
			se->add_event (ue);
			AttachScanEvent (se);
		} else 
			break;
	}
	if (ue)
		delete ue;

	// scan NC-file for User Event Dims, extract Names
	XSM_DEBUG (DBG_L2, "NCF Load: Scan Events -- User Entry...");
        // NcDim* data_dim=NULL;
	for (int n=0; n < ncf->num_dims (); n++) {
		ue = NULL;
		se = NULL;

		NcDim* dim = ncf->get_dim(n);
		gchar *dimname = g_strdup((gchar*)dim->name());
//		XSM_DEBUG (DBG_L2, "[" << n << "] " << dimname);
		if (!g_strcmp0 (dimname, "Event_User_Adjust_Data_Dim")){ // == shoudl be 20
			// data_dim = dim;
			g_free (dimname);
			continue;
		}
		if (g_str_has_prefix (dimname, "Event_User_")){ // if match extract message_id
			if (g_str_has_suffix (dimname, "_Dim")){ // if match extract message_id
				gchar *message_id = g_strndup (dimname+11, strlen(dimname)-11-4);
//				XSM_DEBUG (DBG_L2, " ---->[" << message_id << "] ");
				ue = new UserEntry ("User", ncf, message_id, se, 0, this);
			}
		}
		g_free (dimname);
	}
	if (ue)
		delete ue;


	XSM_DEBUG (DBG_L2, "NCF Load: Scan Events -- done.");
	return 0;
}

void Mem2d::AttachScanEvent (ScanEvent *se){
	scan_event_list = g_slist_prepend(scan_event_list, se);
}

/* Speicher für 2D Daten anfordern */

void Mem2d::Mnew(int Nx, int Ny, int Nv, ZD_TYPE type){
	Init();
	zdtyp=type;
	XSM_DEBUG (DBG_L6, "Mem2d::Mnew");
	if(data) Mdelete();
	switch(zdtyp){
	case ZD_BYTE:   data = new TZData<TYPE_ZD_BYTE>(Nx, Ny, Nv); break;
	case ZD_SHORT:  data = new TZData<TYPE_ZD_SHORT>(Nx, Ny, Nv); break;
	case ZD_LONG:   data = new TZData<TYPE_ZD_LONG>(Nx, Ny, Nv); break;
		//  case ZD_ULONG:  data = new TZData<TYPE_ZD_ULONG>(Nx, Ny, Nv); break;
		// ... since not supportted by NetCDF it has to be excluded !
	case ZD_FLOAT:  data = new TZData<TYPE_ZD_FLOAT>(Nx, Ny, Nv); break;
	case ZD_DOUBLE: data = new TZData<TYPE_ZD_DOUBLE>(Nx, Ny, Nv); break;
	case ZD_COMPLEX: data = new TZData<TYPE_ZD_DOUBLE_C>(Nx, Ny, 3); break;
	case ZD_RGBA: data = new TZData<TYPE_ZD_RGBA>(Nx, Ny, 4); break;
	case ZD_EVENT: data = new TZData<TYPE_ZD_EVENT>(Nx, Ny, Nv); break;
	default:
		XSM_DEBUG_ERROR (DBG_L1, "Wrong type, using TYPE_ZD_SHORT fallback." );
		data = new TZData<TYPE_ZD_SHORT>(Nx, Ny, Nv); 
		break;
	}
	L_info_new ();
        set_px_shift ();
}

/* Speicher für 2D Daten freigeben */
void Mem2d::Mdelete(){
	XSM_DEBUG (DBG_L6, "Mem2d::Mdelete: delete data");
	L_info_delete ();
	delete data;
	data=NULL;
	// free statistics stuff
	if (Zbins) delete [] Zbins;
	Zbins = NULL;
	if (Zibins) delete [] Zibins;
	Zibins = NULL;
}

void Mem2d::Init(){
	SetDataPktMode(SCAN_V_DIRECT);
	Mod = MEM_SET;
	SetDataRange(0, 1024);
	SetDataSkl(0.1, 32.);
	SetZHiLitRange(-1., 1.);
	SetHiLitVRange(32, 1024); // must match!!!!
	data_valid=FALSE;
	Zbins  = NULL;
	Zibins = NULL;
	Zbin_num = 0;
	t_index = -1; // not set!
	frame_time[0] = 0.;
	frame_time[1] = 0.;
	frame_time[2] = 0.;
	frame_time[3] = 0.;
}

void Mem2d::L_info_new (gboolean keep){
	if (keep) {
		int ntmp = GetNv();

		// any change in size, then take action, else do nothing.
		if (ntmp != nlyi){
			GList **tmp = new GList* [ntmp];
			XSM_DEBUG (DBG_L6, "Mem2d::L_info_new w KEEP old [" << nlyi << "] -> [" << ntmp << "]");
			// preserve existing info as possible, copy over to new list
			int i;
			XSM_DEBUG (DBG_L6, "Mem2d::L_info_new w KEEP old: COPY OLD ");
			for (i=0; i<ntmp && i<nlyi; ++i)
				if (!layer_information_list)
					if (i<nlyi){
						tmp[i] = layer_information_list[i];
						XSM_DEBUG (DBG_L6, "Mem2d::L_info_new w KEEP old: COPY [" << i << "]");
					} else {
						tmp[i] = NULL;
						XSM_DEBUG (DBG_L6, "Mem2d::L_info_new w KEEP old: NULL [" << i << "]");
					}
				else {
					tmp[i] = NULL;
					XSM_DEBUG (DBG_L6, "Mem2d::L_info_new w KEEP old: BOGUS, Old List NULL, NULL [" << i << "]");
				}

			XSM_DEBUG (DBG_L6, "Mem2d::L_info_new w KEEP old: ZERO EXCESS ");
			for (; i<ntmp; ++i)
				tmp[i] = NULL;
			XSM_DEBUG (DBG_L6, "Mem2d::L_info_new w KEEP old: CLEANUP OLD EXCESS ");
			for (i=ntmp; i<nlyi; ++i)
				if (!layer_information_list && i<nlyi)
					if (layer_information_list[i]){
						g_list_foreach (layer_information_list[i], (GFunc) Mem2d::free_layer_information, this);
						g_list_free (layer_information_list[i]);
					}
			XSM_DEBUG (DBG_L6, "Mem2d::L_info_new w KEEP old: CLEANUP OLD EXCESS ARRAY ");
			if (!layer_information_list)
				delete[] layer_information_list;
			XSM_DEBUG (DBG_L6, "Mem2d::L_info_new w KEEP old - done. ");
			nlyi = ntmp;
			layer_information_list = tmp;
		}
	} else {
		// trash old list and renew.
		if (layer_information_list)
			L_info_delete ();
		XSM_DEBUG (DBG_L6, "Mem2d::L_info_new [" << GetNv () << "]");
		nlyi = GetNv ();
		layer_information_list = new GList* [nlyi];
		for (int i=0; i<nlyi; ++i)
			layer_information_list[i] = NULL;
	}
}

void Mem2d::L_info_delete (){
	if (!layer_information_list) 
		return;
	XSM_DEBUG (DBG_L6, "Mem2d::L_info_delete [" << GetNv () << "]");
	for (int i=0; i<nlyi; ++i)
		if (layer_information_list[i]){
			g_list_foreach (layer_information_list[i], (GFunc) Mem2d::free_layer_information, this);
			g_list_free (layer_information_list[i]);
		}
	delete[] layer_information_list;
	layer_information_list = NULL;
	nlyi=0;
}

void Mem2d::copy_layer_information (Mem2d *m, int vi){
	XSM_DEBUG (DBG_L6, "Mem2d::copy_layer_information");
	if (! (m->layer_information_list)) return;
	for (int i=vi; i<m->nlyi; ++i){
		if (i-vi >= nlyi) break;
		XSM_DEBUG (DBG_L6, "Mem2d::copy_layer_information - layer:" << i << " of " << m->nlyi);
		if (! (m->layer_information_list[i])) return;
		int nj = g_list_length (m->layer_information_list[i]);
		for (int j = 0; j<nj; ++j){
			LayerInformation *li = (LayerInformation*) g_list_nth_data (m->layer_information_list[i], j);
			XSM_DEBUG (DBG_L6, "Mem2d::copy_layer_information - listelm:" << j << " of " << nj);
			if (li){
				XSM_DEBUG (DBG_L6, "MLYI-CPY["<<i<<"]["<<j<<"]: " << li->get ());
				add_layer_information (i-vi, new LayerInformation (li));
			}
		}
	}
}

void  Mem2d::add_layer_information (LayerInformation *li){
	int lv=GetLayer ();
	if (lv < nlyi){
		XSM_DEBUG (DBG_L6, "Mem2d::add_layer_information[" << lv << "]:" << li->get ());
		layer_information_list[lv] = g_list_append (layer_information_list[lv], li);
	}
}

void  Mem2d::add_layer_information (int lv, LayerInformation *li){
	if (lv < nlyi){
		XSM_DEBUG (DBG_L6, "Mem2d::add_layer_information*[" << lv << "]::" << li->get ());
		layer_information_list[lv] = g_list_append (layer_information_list[lv], li);
	}
}

gchar *Mem2d::get_layer_information (int nth){
	int lv=GetLayer ();
	if (lv < nlyi){
		int num = g_list_length (layer_information_list[lv]);
		if (nth < num && nth >= 0){
			LayerInformation *li = (LayerInformation*) g_list_nth_data (layer_information_list[lv], nth);
			return li->get ();
		} else	return NULL;
	} else return NULL;
}

gchar *Mem2d::get_layer_information_osd (int nth){
	int lv=GetLayer ();
	if (lv < nlyi){
		int num = g_list_length (layer_information_list[lv]);
		if (nth < num && nth >= 0){
			LayerInformation *li = (LayerInformation*) g_list_nth_data (layer_information_list[lv], nth);
			return li->get (TRUE);
		} else	return NULL;
	} else return NULL;
}

gchar *Mem2d::get_layer_information (int lv, int nth){
	if (lv < nlyi){
		int num = g_list_length (layer_information_list[lv]);
		if (nth < num && nth >= 0){
			LayerInformation *li = (LayerInformation*) g_list_nth_data (layer_information_list[lv], nth);
			return li->get ();
		} else	return NULL;
	} else return NULL;
}

int len_max (int a, int b) { return a>b? a:b; }

void Mem2d::eval_max_sizes_layer_information (int* max_dim_description, int* max_dim_format, int* max_dim_format_osd, int* max_nlyi, int* max_nlyiv){
	int mdd  = *max_dim_description;
	int mdf  = *max_dim_format;
	int mdfo = *max_dim_format_osd;

	for (int i=0; i<nlyi; ++i){
		int nj = g_list_length (layer_information_list[i]);
		*max_nlyi = len_max(*max_nlyi, nj);
		for (int j = 0; j<nj; ++j){
			LayerInformation *li = (LayerInformation*) g_list_nth_data (layer_information_list[i], j);
			if (li){
				mdd  = len_max (mdd,  li->get_len_description ());
				mdf  = len_max (mdf,  li->get_len_format ());
				mdfo = len_max (mdfo, li->get_len_osd_format ());
			}
		}
	}
	*max_dim_description = mdd;
	*max_dim_format = mdf;
	*max_dim_format_osd = mdfo;
	*max_nlyiv = len_max(*max_nlyiv, nlyi);
}

void Mem2d::start_store_layer_information (NcFile* ncf, 
					   NcVar** ncv_description, NcVar** ncv_format, NcVar** ncv_osd_format, NcVar** ncv_values, 
					   int dim_ti, int dim_v, int dim_k, 
					   int dim_description, int dim_format, int dim_format_osd){

	NcDim* ncdim_ti  = ncf->add_dim ("LInfo_dim_ti", dim_ti);
	NcDim* ncdim_v   = ncf->add_dim ("LInfo_dim_v", dim_v);
	NcDim* ncdim_k   = ncf->add_dim ("LInfo_dim_k", dim_k);
	NcDim* ncdim_dsc = ncf->add_dim ("LInfo_dim_des", dim_description);
	NcDim* ncdim_fmt = ncf->add_dim ("LInfo_dim_fmt", dim_format);
	NcDim* ncdim_fmo = ncf->add_dim ("LInfo_dim_fmo", dim_format_osd);
	NcDim* ncdim_val = ncf->add_dim ("LInfo_dim_val", 2);

	*ncv_description = ncf->add_var ("LInfo_dsc", ncChar, ncdim_ti, ncdim_v, ncdim_k, ncdim_dsc);
	(*ncv_description)->add_att ("long_name", "Layer Information Description");

	*ncv_format = ncf->add_var ("LInfo_fmt", ncChar, ncdim_ti, ncdim_v, ncdim_k, ncdim_fmt);
	(*ncv_format)->add_att ("long_name", "Layer Information full Format String");

	*ncv_osd_format = ncf->add_var ("LInfo_fmo", ncChar, ncdim_ti, ncdim_v, ncdim_k, ncdim_fmo);
	(*ncv_osd_format)->add_att ("long_name", "Layer Information short (OSD) Format String");

	*ncv_values = ncf->add_var ("LInfo_values", ncDouble, ncdim_ti, ncdim_v, ncdim_k, ncdim_val);
	(*ncv_values)->add_att ("long_name", "Layer Information Values");
	(*ncv_values)->add_att ("extra_info", "to be formated by fprint using LInfo_format or LInfo_format_osd");
}

void Mem2d::store_layer_information (NcVar* ncv_description, NcVar* ncv_format, NcVar* ncv_osd_format, NcVar* ncv_values, int ti){
	for (int v=0; v<nlyi; ++v){
		int nj = g_list_length (layer_information_list[v]);
		for (int k = 0; k<nj; ++k){
			LayerInformation *li = (LayerInformation*) g_list_nth_data (layer_information_list[v], k);
			li->write_layer_information (ncv_description, ncv_format, ncv_osd_format, ncv_values, ti, v, k);
		}
	}
}

void Mem2d::start_read_layer_information (NcFile* ncf, NcVar** ncv_description, NcVar** ncv_format, NcVar** ncv_osd_format, NcVar** ncv_values){
	*ncv_description = ncf->get_var("LInfo_dsc");
	*ncv_format      = ncf->get_var("LInfo_fmt");
	*ncv_osd_format  = ncf->get_var("LInfo_fmo");
	*ncv_values      = ncf->get_var("LInfo_values");
}

void Mem2d::add_layer_information (NcVar* ncv_description, NcVar* ncv_format, NcVar* ncv_osd_format, NcVar* ncv_values, int ti){
	L_info_new ();
	for (int v=0; v<nlyi; ++v){
		if (v >= ncv_description->get_dim(1)->size())
			break;
		int nj = ncv_description->get_dim(2)->size();
		for (int k = 0; k<nj; ++k){
			LayerInformation* li = new LayerInformation (ncv_description, ncv_format, ncv_osd_format, ncv_values, ti, v, k);
			add_layer_information (v, li);
		}
	}
}

const char * Mem2d::GetEname(){ 
	strcpy(MemElemName, ZD_name[GetTyp()]); 
	sprintf(MemElemName+strlen(MemElemName),"[%d]", (int)GetEsz()); 
	return((const char*)MemElemName);
}

/* Speichermodus setzen */
void Mem2d::Modus(int mod){
	Mod = mod;
}

/* Neue Speichergröße für 2D Daten */
int Mem2d::Resize(int Nx, int Ny, ZD_TYPE type, gboolean keep_layer_info){
	XSM_DEBUG (DBG_L6, "Mem2d::Resize");

	if((zdtyp == type || type == ZD_IDENT) && data){
		int ret = data->Resize(Nx, Ny);
		L_info_new (keep_layer_info);
		return ret;
	}
	if(type == ZD_IDENT)
		return -1;
	else
		Mnew(Nx, Ny, GetNv (), type);

	return 0;
}

int Mem2d::Resize(int Nx, int Ny, int Nv, ZD_TYPE type, gboolean keep_layer_info){
	XSM_DEBUG (DBG_L6, "Mem2d::Resize with multiple layers");

	if((zdtyp == type || type == ZD_IDENT) && data){
		int ret = data->Resize(Nx, Ny, Nv);
		L_info_new (keep_layer_info);
		return ret;
	}
	if(type == ZD_IDENT)
		return -1;
	else
		Mnew(Nx, Ny, Nv, type);
	return 0;
}

// Get Data with LineReg !
inline double Mem2d::GetDataPktLineReg(int x, int y){
	if(!data->GetLi(y)->valid)
		CalcLinRegress(y,y);
	return(data->Z(x,y) - data->GetLi(y)->getY(data->GetXLookup(x)));
}

// Get Data with Horizontmethode !
inline double Mem2d::GetDataPktHorizont(int x, int y){
	if(!data->GetLi (y)->valid)
		CalcLinRegress(y,y);
	return(data->Z(x,y) - data->GetLi(y)->getB());
}

// Get Data with "koehler-type" differential filter method !
// KLEN=16 (default) may be adjusted for better/faster results
#define KLEN 16
double Mem2d::GetDataPktDiff(int x, int y){
	const double Lproz = 0.92;
	const double Rproz = 0.08;
	double v[KLEN];
	v[0] = GetDataPkt (x,y);
	for (int i=1; i<KLEN; ++i)
		if (x+i < GetNx())
			v[i] = Lproz * v[i-1] + Rproz * GetDataPkt (x+i,y);
		else break;
	for (int i=KLEN-2; i>=0; --i)
		if (x+i+1 < GetNx())
			v[i] = Lproz * v[i+1] + Rproz * v[i];

	return GetDataPkt (x,y) - v[0];
}

inline double Mem2d_LogLimit(double x){ return (x>0. ? x:0.); }

double Mem2d::GetDataPktLog(int x, int y){
	return logFac * log (1. + Mem2d_LogLimit (((GetDataPkt (x,y)) - Zmin)));
}

double Mem2d::GetDataPktPlaneSub(int x, int y){
	return GetDataPkt (x,y) + plane_z0 + plane_mx*x + plane_my*y;
}

double Mem2d::GetDataPktInterpol(double x, double y){
	double f1,f2,f3,f4;
	int    x1,x2,y1,y2;
	x1 = (int)x;
	x2 = x1 + 1;
	y1 = (int)y;
	y2 = y1 + 1;
	f1 = ((double)x2-x)  *(y-(double)y1);
	f2 = (x - (double)x1)*(y-(double)y1);
	f3 = (x - (double)x1)*((double)y2-y);
	f4 = ((double)x2-x)  *((double)y2-y);
	// safety check, to make sure we dont have f1 = f2 = f3 = f4 = 0 
	y2 = MIN (y2, GetNx()-1);
	x2 = MIN (x2, GetNx()-1);
	if(x1 < 0 || x1 >= GetNx() || y1 < 0 || y1 >= GetNy() ||
	   x2 < 0 || x2 >= GetNx() || y2 < 0 || y2 >= GetNy())
		return 0.;
	else
		return( f4 * (double)GetDataPkt(x1,y1) + f3 * (double)GetDataPkt(x2,y1)
			+ f1 * (double)GetDataPkt(x1,y2) + f2 * (double)GetDataPkt(x2,y2));
}

double Mem2d::GetDataPktInterpol(double x, double y, int v){
	double f1,f2,f3,f4;
	int    x1,x2,y1,y2;
	x1 = (int)x;
	x2 = x1 + 1;
	y1 = (int)y;
	y2 = y1 + 1;
	f1 = ((double)x2-x)  *(y-(double)y1);
	f2 = (x - (double)x1)*(y-(double)y1);
	f3 = (x - (double)x1)*((double)y2-y);
	f4 = ((double)x2-x)  *((double)y2-y);
	// safety check, to make sure we dont have f1 = f2 = f3 = f4 = 0 
	y2 = MIN (y2, GetNx()-1);
	x2 = MIN (x2, GetNx()-1);
	if(x1 < 0 || x1 >= GetNx() || y1 < 0 || y1 >= GetNy() ||
	   x2 < 0 || x2 >= GetNx() || y2 < 0 || y2 >= GetNy())
		return 0.;
	else
		return( f4 * (double)GetDataPkt(x1,y1,v) + f3 * (double)GetDataPkt(x2,y1,v)
			+ f1 * (double)GetDataPkt(x1,y2,v) + f2 * (double)GetDataPkt(x2,y2,v));
}

double Mem2d::GetDataPktInterpol(double x, double y, double v){
	int il=GetLayer ();
	int ifloor = (int)(floor(v));
	int iceil = (int)(ceil(v));

	if (ifloor < 0) ifloor = 0;
	if (ifloor >= GetNv ()) ifloor = GetNv () - 1;
	if (iceil >= GetNv ()) iceil = GetNv () - 1;
	if (iceil < 0) iceil = 0;

	SetLayer (ifloor);
	double val_floor = GetDataPktInterpol (x,y);
	SetLayer (iceil);
	double val_ceil = GetDataPktInterpol (x,y);
	SetLayer (il);

	double d = v - (double)ifloor;

	return val_floor * (1.-d) + val_ceil * d;
}

double Mem2d::GetDataPktInterpol(double x, double y, double v, Scan *sct, int tn){
        gint tiN = sct->number_of_time_elements ();
        if (tn >= tiN)
                return 0.;
        if (tn < 0)
                return 0.;
	int il=GetLayer ();
	int ifloor = (int)(floor(v));
	int iceil = (int)(ceil(v));

	if (ifloor < 0) ifloor = 0;
	if (ifloor >= GetNv ()) ifloor = GetNv () - 1;
	if (iceil >= GetNv ()) iceil = GetNv () - 1;
	if (iceil < 0) iceil = 0;

        Mem2d *mt = sct->mem2d_time_element (tn);

        mt->SetLayer (ifloor);
	double val_floor = mt->GetDataPktInterpol (x,y);
	mt->SetLayer (iceil);
	double val_ceil = mt->GetDataPktInterpol (x,y);
	mt->SetLayer (il);

	double d = v - (double)ifloor;

	return val_floor * (1.-d) + val_ceil * d;
}

double Mem2d::GetDataPktInterpol(double x, double y, int v, double t, Scan *sct, double dt, double t0){
        double tid;
        gint tiN = sct->number_of_time_elements ();
        if (fabs (dt) > 0.)
                tid = (t-t0)/dt;
        else{
                double ti0 = sct->mem2d_time_element (0) -> get_frame_time (); // sct->retrieve_time_element (0); // slow
                double tin = sct->mem2d_time_element (tiN-1) -> get_frame_time ();  // sct->retrieve_time_element (tiN-1);
                dt = (tin - ti0) / tiN;
                t0 = ti0;
                tid = (t-t0)/dt; // assume linear time scale fixed spacing
        }   

	// int tei=(int)round (tid);
	int ifloor = (int)(floor(tid));
	int iceil = (int)(ceil(tid));

	if (ifloor < 0) ifloor = 0;
	if (ifloor >= tiN) ifloor = tiN - 1;
	if (iceil >= tiN) iceil = tiN - 1;
	if (iceil < 0) iceil = 0;

        Mem2d *mfloor = sct->mem2d_time_element (ifloor);
	double val_floor = mfloor->GetDataPktInterpol (x,y,v);
        if (ifloor == iceil){
#if 0
        XSM_DEBUG (DBG_L2, "GetDataPktInterpol_XYvt -- integer t index ["
                   << x << ", "
                   << y << ", "
                   << v << ", "
                   << t << "] "
                   << " ifloor=" << ifloor
                   << " val=" << val_floor
                   );
#endif
                return val_floor;
        }
        Mem2d *mceil  = sct->mem2d_time_element (iceil);
	double val_ceil  = mceil->GetDataPktInterpol (x,y,v);

	double d = tid - (double)ifloor;
        double val_inter = val_floor * (1.-d) + val_ceil * d;
#if 0
        XSM_DEBUG (DBG_L2, "GetDataPktInterpol_XYvt ["
                   << x << ", "
                   << y << ", "
                   << v << ", "
                   << t << "] "
                   << " ifloor=" << ifloor
                   << " iceil=" << iceil
                   << " tid=" << tid << " tei=" << tei << " tiN=" << tiN
                   << " v_~ = " << val_floor << ", " << val_ceil
                   << " d= " << d
                   << " val_inter= " << val_inter
                   );
#endif        

        return val_inter;
}


double Mem2d::GetDataDiscAv(int x, int y, int v, double r){
	int num=0;
	int rr = (int)r;
	double r2 = r*r;
	double sum=0.;
	for (int xx=x-rr; xx<x+rr; ++xx)
		for (int yy=y-rr; yy<y+rr; ++yy){
			double dx = (double)(xx-x);
			double dy = (double)(yy-y);
			if (xx < 0 || yy < 0 || xx >= GetNx() || yy >= GetNy() || (dx*dx+dy*dy) > r2)
				continue;
			else {
				++num;
				sum += (double)GetDataPkt (xx,yy,v);
			}
		}
	return sum/num;
}


double Mem2d::GetDataWidthAv(double x, double y, int v, double w, double s, double dx, double dy){
	int num=0;
	int w2 = (int)((w+1.)/2.);
	int s2 = (int)((s+1.)/2.);
	double sum=0.;
	double nrm=sqrt (dx*dx + dy*dy);
	dx /= nrm; 
	dy /= nrm;
	if (s>1.){
		for (int m=-s2; m<=s2; ++m)
			for (int l=-w2; l<=w2; ++l){
				double xx = x + l*dy + m*dx;
				double yy = y + l*dx + m*dy;
				if (xx < 0. || yy < 0. || xx >= (double)GetNx() || yy >= (double)GetNy())
					continue;
				else {
					++num;
					sum += (double)GetDataPktInterpol (xx,yy,1.,v);
				}
			}
	} else {
		for (int l=-w2; l<=w2; ++l){
			double xx = x + l*dy;
			double yy = y + l*dx;
			if (xx < 0. || yy < 0. || xx >= (double)GetNx() || yy >= (double)GetNy())
				continue;
			else {
				++num;
				sum += (double)GetDataPktInterpol (xx,yy,1.,v);
			}
		}
	}
	return sum/num;
}


#define MEM2D_LINEAR_SCALE(Z)   ((Z)*Zcontrast+Zbright)
#define MEM2D_PERIODIC_SCALE(Z) (fmod((Z)*Zcontrast+Zbright, (double)ZVrange))
#define MEM2D_RANGE_CHECK(X)    ((val=(X)) > (double)ZVmax ? ZVmax : val < (double)ZVmin ? ZVmin : (ZVIEW_TYPE)val)

ZVIEW_TYPE Mem2d::ZQuick(int &x, int &y){
	double val;
	return MEM2D_RANGE_CHECK (MEM2D_LINEAR_SCALE (GetDataPktLineReg (x,y)));
}

ZVIEW_TYPE Mem2d::ZDirect(int &x, int &y){
	double val;
	return MEM2D_RANGE_CHECK (MEM2D_LINEAR_SCALE (GetDataPkt (x,y)));
}

ZVIEW_TYPE Mem2d::ZHiLitDirect(int &x, int &y){
        double val, z;
	ZVIEW_TYPE zv = MEM2D_RANGE_CHECK (MEM2D_LINEAR_SCALE (z=GetDataPkt (x,y)));
	if (z > ZHiLit_low && z < ZHiLit_hi)
	     zv = ZVHiLitBase + (zv-ZVmin)*ZVHiLitRange/ZVrange; 
	return zv;
}

ZVIEW_TYPE Mem2d::ZPlaneSub(int &x, int &y){
	double val;
	return MEM2D_RANGE_CHECK (MEM2D_LINEAR_SCALE (GetDataPktPlaneSub (x,y)));
}


ZVIEW_TYPE Mem2d::ZLog(int &x, int &y){
	double val;
	return MEM2D_RANGE_CHECK (MEM2D_LINEAR_SCALE (GetDataPktLog (x,y)));
}

ZVIEW_TYPE Mem2d::ZPeriodic(int &x, int &y){
	double val;
	return MEM2D_RANGE_CHECK (MEM2D_PERIODIC_SCALE (GetDataPkt (x,y)));
}

ZVIEW_TYPE Mem2d::ZDifferential(int &x, int &y){
	double val;
	return MEM2D_RANGE_CHECK (MEM2D_LINEAR_SCALE (GetDataPktDiff (x,y)));
}

ZVIEW_TYPE Mem2d::ZHorizontal(int &x, int &y){
	double val;
	return MEM2D_RANGE_CHECK (MEM2D_LINEAR_SCALE (GetDataPktHorizont (x,y)));
}

inline int clamp (int u, int min, int max){
        return u < min ? min : u > max ? max : u;
}

ZVIEW_TYPE Mem2d::GetDataVMode(int x, int y){
        return ((ZVIEW_TYPE)(this->*ZVFkt)(x,y));
}

ZVIEW_TYPE Mem2d::GetDataVMode(int x, int y, int v){
	int il=GetLayer ();
	SetLayer (v);
        // saturate indices
        if (x<1) x=1;
        if (y<1) y=1;
        if (x>=GetNx()-1) x=GetNx()-2;
        if (y>=GetNy()-1) y=GetNy()-2;
        ZVIEW_TYPE zvt = (ZVIEW_TYPE)(this->*ZVFkt)(x,y);
        SetLayer (il);
	return (zvt);
}

ZVIEW_TYPE Mem2d::GetDataVModeInterpol(double x, double y, double v){
        return (ZVIEW_TYPE) round (GetDataVModeInterpol_double (x,y,(int)round(v)));
#if 0
        ZVIEW_TYPE zvt;
	int ix, iy, iv;
	ix = (int)round(x);
	iy = (int)round(y);
	iv = (int)round(v);

	int il=GetLayer ();
	SetLayer (iv);
        zvt = (ZVIEW_TYPE)(this->*ZVFkt)(ix,iy);
        SetLayer (il);
	return (zvt);
#endif
}

double Mem2d::GetDataVModeInterpol_double (double x, double y, int v){
	double f1,f2,f3,f4;
	int    x1,x2,y1,y2;
        double z_inter = 0.;

	x1 = (int)x;
	x2 = x1 + 1;
	y1 = (int)y;
	y2 = y1 + 1;
	f1 = ((double)x2-x)  *(y-(double)y1);
	f2 = (x - (double)x1)*(y-(double)y1);
	f3 = (x - (double)x1)*((double)y2-y);
	f4 = ((double)x2-x)  *((double)y2-y);
	// safety check, to make sure we dont have f1 = f2 = f3 = f4 = 0 
	y2 = MIN (y2, GetNx()-1);
	x2 = MIN (x2, GetNx()-1);

	if(x1 < 0 || x1 >= GetNx() || y1 < 0 || y1 >= GetNy() ||
	   x2 < 0 || x2 >= GetNx() || y2 < 0 || y2 >= GetNy())
		z_inter = 0.;
	else {
                int il=GetLayer ();
                SetLayer (v);
		z_inter = f4 * (double)(this->*ZVFkt)(x1,y1) + f3 * (double)(this->*ZVFkt)(x2,y1)
			+ f1 * (double)(this->*ZVFkt)(x1,y2) + f2 * (double)(this->*ZVFkt)(x2,y2);
                SetLayer (il);
        }
	return z_inter;
}


double Mem2d::GetDataMode(int x, int y){ 
	return((double)(this->*ZFkt)(x,y));
}

void  Mem2d::SetDataRange(ZVIEW_TYPE Min, ZVIEW_TYPE Max){ 
	ZVmin=Min; 
	ZVmax=Max;
	ZVrange=ZVmax-ZVmin; 
	logFac = (double)Zrange/log(1.+fabs(Zrange));
	XSM_DEBUG(DBG_L5, "Mem2d::SetDataRange ZMin=" << ZVmin << " ZVMax=" << ZVmax << " => ZVRange=" << ZVrange);
}

void Mem2d::SetDataVRangeZ(double VRangeZ, double VOffsetZ, double dz){
	double hi, lo, avg, zvr;
	GetZHiLo (&hi, &lo);
	avg = lo + (hi-lo)/2. + VOffsetZ/dz;
	zvr = VRangeZ/dz/2.;
	SetHiLo (avg+zvr, avg-zvr);
}

void Mem2d::SetDataPktMode(int mode){
	switch (mode & SCAN_V_MASK){	
	case SCAN_V_QUICK:        SetDataFkt (&Mem2d::ZQuick, &Mem2d::GetDataPktLineReg); break;
	case SCAN_V_DIRECT:       SetDataFkt (&Mem2d::ZDirect, &Mem2d::GetDataPkt); break;
	case SCAN_V_PLANESUB:     SetDataFkt (&Mem2d::ZPlaneSub, &Mem2d::GetDataPktPlaneSub); break;
	case SCAN_V_LOG:          SetDataFkt (&Mem2d::ZLog, &Mem2d::GetDataPktLog); break;
	case SCAN_V_PERIODIC:     SetDataFkt (&Mem2d::ZPeriodic, &Mem2d::GetDataPkt); break;
	case SCAN_V_DIFFERENTIAL: SetDataFkt (&Mem2d::ZDifferential, &Mem2d::GetDataPktDiff); break;
	case SCAN_V_HORIZONTAL:   SetDataFkt (&Mem2d::ZHorizontal, &Mem2d::GetDataPktHorizont); break;
	case SCAN_V_HILITDIRECT:  SetDataFkt (&Mem2d::ZHiLitDirect, &Mem2d::GetDataPkt); break;
	}
	XSM_DEBUG (DBG_L5, "Mem2d::SetDataPktMode to " << mode);
}

void Mem2d::SetDataFkt(ZVIEW_TYPE (Mem2d::*newZVFkt)(int &, int &), 
		       double (Mem2d::*newZFkt)(int, int)){ 
	ZFkt  = newZFkt; 
	ZVFkt = newZVFkt; 
}

void Mem2d::GetZHiLo(double *hi, double *lo){
	*hi=Zmax; *lo=Zmin;
}

double Mem2d::GetZRange(){
	return Zrange;
}

void Mem2d::SetHiLo(double hi, double lo, gboolean expand){
        if (expand){
                Zmin = MIN (lo, Zmin);
                Zmax = MAX (hi, Zmax);
        } else {
                Zmin = lo;
                Zmax = hi;
        }
	Zrange = Zmax-Zmin;
}

void Mem2d::AutoDataSkl(double *contrast, double *bright){
	// maybe call HiLoMod() before !
	if(fabs(Zrange) > 0.){
		Zcontrast =  (double)ZVrange/Zrange;
		Zbright   = -(double)ZVrange*Zmin/Zrange;
		if (contrast && bright){
			*contrast=Zcontrast; *bright=Zbright;
		}
		XSM_DEBUG (DBG_L6, "AutoSkl: Zmax=" << Zmax << " Zmin=" << Zmin << " ZRange=" << Zrange << " Contrast=" << Zcontrast << " Bright=" << Zbright);
	}
}

void Mem2d::CalcLinRegress(int yfirst, int ylast)
//
// OutputFeld[i] = InputFeld[i] - (a * i + b)
//
{
	int y;
	double sumx, sumy, sumxy, sumxq; //, sumyq;
	double x;
	int i, istep;
	// short *lpi, *lpo ;
	int    imax, imin;
	//double Xmax, Xmin;
	double a, b, n, xmean, ymean;

	for(y=yfirst; y<=ylast; y++){
		n = 0;
		sumx = sumxq = sumy = sumxy = 0.0; // sumyq = 0.0;
		imax = data->GetNx()-data->GetNx()/10; // etwas Abstand (10%) vom Rand !
		imin = data->GetNx()/10;
		//Xmax = data->GetXLookup (imax = data->GetNx()-data->GetNx()/10); // etwas Abstand (10%) vom Rand !
		//Xmin = data->GetXLookup (imin = data->GetNx()/10);
		istep = (long) MAX(1,(imax-imin)/30); // 30 Samples / Line !!!
		for (i=imin; i < imax; i+=istep) { /* Rev 2 */
			sumx += data->GetXLookup (i);
			sumy += data->Z(i,y);
			n += 1.;
		}
		xmean = sumx / n;
		ymean = sumy / n;
		for (i=imin; i < imax; i+=istep) { /* Rev 2 */
			x = data->GetXLookup (i) - xmean;
			sumxq += x*x;
			a = data->Z(i,y) - ymean;
//			sumyq  += a*a; // no need
			sumxy  += a*x;
		}
		a = sumxy / sumxq;
		b = ymean - a * xmean; // a(x-xmean) + b = a x + b - a xmean
		data->GetLi(y)->set(a,b);  // ==> Yreg(x) = a*x + b
	}
}

double Mem2d::GetDataPktInterpol(double x, double y, double dr, int lv){
	if (dr < 1.5) {
		int il=GetLayer ();
		SetLayer (lv);
		double val = GetDataPktInterpol (x,y);
		SetLayer (il);
		return val;
	} else {
		return GetDataDiscAv ((int)round (x), (int)round (y), lv, dr);
	}
}

double Mem2d::GetDataPktInterpol(double x, double y, double dw, double ds, int lv, double dx, double dy){
	if (dw < 1.5 && ds < 1.5) {
		int il=GetLayer ();
		SetLayer (lv);
		double val = GetDataPktInterpol (x,y);
		SetLayer (il);
		return val;
	} else {
		if (ds < 1.5)
			return GetDataDiscAv ((int)round (x), (int)round (y), lv, dw);
		else
			return GetDataWidthAv (x, y, lv, dw, ds, dx, dy);
	}
}

int Mem2d::GetDataLineFrom(Point2D *start, Point2D *end, Mem2d *Mob, SCAN_DATA *sdata_src,  SCAN_DATA *sdata_dest, GETLINEORGMODE orgmode, double dw, double ds, MEM2D_DIM profile_dim, MEM2D_DIM series_dim, gboolean series, int series_i, int series_nt, gboolean append){
	int i, j, SamplesPerLine, SeriesPerLine;
  	double dx, dy, xy_LineLength;
  	double x,dl;
        // double y;
        
	int num_series=1, num_points=0;
	int num_points_0 = append ? GetNx () : 0;
	
  	XSM_DEBUG (DBG_L6, "--- Clipping XY Start/End if out of range ---");
  	XSM_DEBUG (DBG_L6, "Line (" 
	   << start->x << ", " << start->y << ")-(" << end->x << ", " << end->y << ")"
	   "   ClipBox: (0,0)-(" << (Mob->GetNx()-1) << ", " << (Mob->GetNx()-1) << ")");

  	// Clipping
  	cohen_sutherland_line_clip_i (&(start->x), &(start->y), &(end->x), &(end->y),
				      0, Mob->GetNx()-1,
				      0, Mob->GetNy()-1);

  	XSM_DEBUG (DBG_L6, "Result: (" << start->x << ", " << start->y << ")-(" << end->x << ", " << end->y << ")");

  	// Delta mit Vorzeichen !!! Now we got our clipped XY vector from start to end
  	dx = (double)(end->x - start->x);
  	dy = (double)(end->y - start->y);
  
	// make sure width/radius is fine
	if (dw < 1.) dw = 1.;
	if (ds < 0.) ds = 0.;

  	// XY line length in pixels
  	xy_LineLength = sqrt(dx*dx+dy*dy);
	SamplesPerLine = (int)(xy_LineLength);
	if (ds < 1.)
		SeriesPerLine = series ? (int)(xy_LineLength/(dw*2.)+1) : 1; // used for coarse stepping for series in XY
	else
		SeriesPerLine = series ? (int)(xy_LineLength/(ds*2.)+1) : 1; // used for coarse stepping for series in XY

	switch (profile_dim){ // i.e. Plot X dim, going int profile_plot'x mem2d X dim
	case MEM2D_DIM_XY:    num_points = SamplesPerLine; break;
	case MEM2D_DIM_LAYER: num_points = Mob->GetNv (); break;
	case MEM2D_DIM_TIME:  num_points = series_nt; break; // scan->number_of_time_elements ()
	default: return -1;
	}

	switch (series_dim){ // i.e. Series dim, going into profile_plot'x mem2d Y dim
	case MEM2D_DIM_XY:    if (profile_dim == MEM2D_DIM_XY) return -1; num_series = SeriesPerLine; break;
	case MEM2D_DIM_LAYER: if (profile_dim == MEM2D_DIM_LAYER) return -1; num_series = Mob->GetNv (); break;
	case MEM2D_DIM_TIME:  if (profile_dim == MEM2D_DIM_TIME) return -1; num_series = series_nt; break; // scan->number_of_time_elements ()
	default: return -1;
	}

#define DBG_GDLF  XSM_DEBUG (DBG_L1, "Mem2d::GetDataLineFrom dxy:" << dx << "," << dy << " dw:" << dw << " ds:" << ds << " num_points[d" << profile_dim<<"]:" << num_points <<"[d"<<series_dim<<",s"<<series<<"], num_series:" << num_series)

//	DBG_GDLF;

	if (num_points < 2) num_points = 2;
	
	if (series_i && append)
		num_points_0 -= num_points;
	Resize (num_points_0 + num_points, series ? num_series : 1);

	if (series_i < 0) { std::cerr << "Series Index EOR" << std::endl; DBG_GDLF; return -1; }
	if (series_dim == MEM2D_DIM_TIME && series_i >= num_series) { std::cerr << "Mem2d::GDLF T:SeriesDim Index EOR" << std::endl; DBG_GDLF; return -1; }
	if (profile_dim == MEM2D_DIM_TIME && series_i >= num_points) { std::cerr << "Mem2d::GDLF T:ProfileDim Index EOR" << std::endl; DBG_GDLF; return -1; }

//	XSM_DEBUG (DBG_L7, "Mem2d::GetDataLineFrom -- extracting data");
	
	if (series){
		if (series_dim == MEM2D_DIM_TIME && profile_dim == MEM2D_DIM_XY){
			// stepsize for XY dimension
			dx /= (double)(num_points-1);
			dy /= (double)(num_points-1);
			for (i=0; i < num_points; ++i)
				PutDataPkt (Mob->GetDataPktInterpol ((double)start->x + (double)i * dx, 
								     (double)start->y + (double)i * dy, 
								     dw, ds, Mob->GetLayer (), dx,dy),
					    num_points_0 + i, series_i);

		} else if (series_dim == MEM2D_DIM_XY && profile_dim == MEM2D_DIM_TIME){
			// stepsize for XY dimension
			dx /= (double)(num_series-1);
			dy /= (double)(num_series-1);
			for (j=0; j < num_series; ++j)
				PutDataPkt (Mob->GetDataPktInterpol ((double)start->x + (double)j * dx, 
								     (double)start->y + (double)j * dy, 
								     dw, ds, Mob->GetLayer (), dx, dy),
					    num_points_0 + series_i, j);

		} else if (series_dim == MEM2D_DIM_XY && profile_dim == MEM2D_DIM_LAYER){
			// stepsize for XY dimension
			dx /= (double)(num_series-1);
			dy /= (double)(num_series-1);
			for (j = 0; j < num_series; ++j)
				for (i=0; i < num_points; ++i)
					PutDataPkt (Mob->GetDataPktInterpol ((double)start->x + (double)j * dx, 
									     (double)start->y + (double)j * dy, 
									     dw, ds, i, dx, dy),
						    num_points_0 + i, j);

		} else if (series_dim == MEM2D_DIM_LAYER && profile_dim == MEM2D_DIM_XY){
			// stepsize for XY dimension
			dx /= (double)(num_points-1);
			dy /= (double)(num_points-1);
			for (j = 0; j < num_series; ++j)
				for (i=0; i < num_points; ++i)
					PutDataPkt (Mob->GetDataPktInterpol ((double)start->x + (double)i * dx, 
									     (double)start->y + (double)i * dy, 
									     dw, ds, j, dx, dy),
						    num_points_0 + i, j);

		} else if (series_dim == MEM2D_DIM_TIME && profile_dim == MEM2D_DIM_LAYER)
			for (i=0; i < num_points; ++i)
				PutDataPkt (Mob->GetDataPktInterpol ((double)start->x + (double)0. * dx, 
								     (double)start->y + (double)0. * dy, 
								     dw, i),
					    num_points_0 + i, series_i);

		else if (series_dim == MEM2D_DIM_LAYER && profile_dim == MEM2D_DIM_TIME)
			for (i=0; i < num_series; ++i)
				PutDataPkt (Mob->GetDataPktInterpol ((double)start->x + (double)0. * dx, 
								     (double)start->y + (double)0. * dy, 
								     dw, i),
					    num_points_0 + series_i, i);

	} else {
		if (profile_dim == MEM2D_DIM_XY){
			// stepsize for XY dimension
			dx /= (double)(num_points-1);
			dy /= (double)(num_points-1);
			for (i=0; i < num_points; ++i)
				PutDataPkt (Mob->GetDataPktInterpol ((double)start->x + (double)i * dx, 
								     (double)start->y + (double)i * dy, 
								     dw, ds, Mob->GetLayer (), dx,dy),
					    num_points_0 + i, 0);

		} else if (profile_dim == MEM2D_DIM_TIME)
			PutDataPkt (Mob->GetDataPktInterpol ((double)start->x + (double)0. * dx, 
							     (double)start->y + (double)0. * dy, 
							     dw, Mob->GetLayer ()),
				    num_points_0 + series_i, 0);

		else if (profile_dim == MEM2D_DIM_LAYER)
			for (i=0; i < num_points; ++i)
				PutDataPkt (Mob->GetDataPktInterpol ((double)start->x + (double)0. * dx, 
								     (double)start->y + (double)0. * dy, 
								     dw, i),
					    num_points_0 + i, 0);
	}


//	XSM_DEBUG (DBG_L7, "Mem2d::GetDataLineFrom -- setting up scales and lookups");
	double xy[4];
  	xy[0] = Mob->data->GetXLookup((int)start->x);
  	xy[1] = Mob->data->GetYLookup((int)start->y);
  	xy[2] = Mob->data->GetXLookup((int)end->x);
  	xy[3] = Mob->data->GetYLookup((int)end->y);
  	x = data->GetXLookup((int)(start->x+dx/2));
  	// y = data->GetYLookup((int)(start->y+dy/2)); // not used here
	dl = sqrt((xy[2]-xy[0])*(xy[2]-xy[0]) + (xy[3]-xy[1])*(xy[3]-xy[1]));

        // to be obsoleted -- size is to be set via Resize above and read back via mem2d->GetNx ()...
	sdata_dest->s.nx = num_points_0 + num_points;
	sdata_dest->s.x0 = 0.;
	sdata_dest->s.dx = 1.;
	sdata_dest->s.dl = 1.;

	switch (profile_dim){ // i.e. Plot X dim, going into profile_plot's mem2d X dim
	case MEM2D_DIM_XY:    
		if (sdata_src->Xunit){
			sdata_dest->SetXUnit (sdata_src->Xunit);
		}
		sdata_dest->s.x0 = x;
		sdata_dest->s.dx *= dx;
		sdata_dest->s.dl = dl;
		if (append)
			sdata_dest->s.rx += dl;
		else
			sdata_dest->s.rx = dl;
		
		switch(orgmode){ // only correct for last (single) segment!!!
  		case GLORG_CENTER: data->MkXLookup(-dl/2., dl/2.); break;
  		case GLORG_PRJX:   data->MkXLookup(xy[0], xy[2]); break;
  		case GLORG_PRJY:   data->MkXLookup(xy[1], xy[3]); break;
  		case GLORG_ZERO:
  		default: data->MkXLookup (0., sdata_dest->s.rx); break;
		}
		break;
	case MEM2D_DIM_LAYER: 
		sdata_dest->s.rx = num_points_0 + num_points;
		if (sdata_src->Vunit)
			sdata_dest->SetXUnit (sdata_src->Vunit);
		data->MkXLookup (Mob->data->GetVLookup (0), Mob->data->GetVLookup (Mob->GetNv()-1));
		break;
	case MEM2D_DIM_TIME:  
		sdata_dest->s.rx = num_points_0 + num_points;
		if (sdata_src->TimeUnit)
			sdata_dest->SetXUnit (sdata_src->TimeUnit);
		data->SetXLookup (series_i, Mob->get_frame_time ());
		break;
	default:
		sdata_dest->s.rx = num_points_0 + num_points;
		return -1;
	}

	if (series){
		sdata_dest->s.y0 = 0.;
		sdata_dest->s.dy = 1.;
		sdata_dest->s.ry = num_series;
		sdata_dest->s.ny = num_series;
	} else {
		sdata_dest->s.y0 = 0.;
		sdata_dest->s.dy = 0.;
		sdata_dest->s.ry = 0.;
		sdata_dest->s.ny = 1;
	}
	switch (series_dim){ // i.e. Series dim, going into profile_plot's mem2d Y dim
	case MEM2D_DIM_XY: 
		if (sdata_src->Xunit){
			sdata_dest->SetYUnit (sdata_src->Xunit);
		}
		if (series) data->MkYLookup (0., dl);
		else data->SetYLookup (0, 0.);
		break;
	case MEM2D_DIM_LAYER: 
		if (sdata_src->Vunit){
			sdata_dest->SetYUnit (sdata_src->Vunit);
		}
		if (series) data->MkYLookup (Mob->data->GetVLookup (0), Mob->data->GetVLookup (Mob->GetNv()-1));
		else data->SetYLookup (0, Mob->data->GetVLookup (Mob->GetLayer ()));
		break;
	case MEM2D_DIM_TIME:  
		if (sdata_src->TimeUnit){
			sdata_dest->SetYUnit (sdata_src->TimeUnit);
		}
		if (series) data->SetYLookup (series_i, Mob->get_frame_time ());
		else data->SetYLookup (0, Mob->get_frame_time ());
		break;
	default: return -1;
	}


  	if(fabs(dx) > 1e-5)
    		sdata_dest->s.alpha = 180./M_PI*atan(-dy/dx);
  	else
    		sdata_dest->s.alpha = 90.*(-dy>0.?1.:-1.);
  	if(dx < 0.)
    		sdata_dest->s.alpha = 180.+sdata_dest->s.alpha;
  	else
    		if(dy>0.)
      	sdata_dest->s.alpha = 360.+sdata_dest->s.alpha;

  	return 0;
}

int Mem2d::GetLayerDataLineFrom(Point2D *start, Mem2d *Mob, SCAN_DATA *sdata_src, SCAN_DATA *sdata_dest, double pr, int iy, int transpose, int v){
	if (transpose) {

                if (v < 0){
                        Resize (iy+1, Mob->GetNv());
                        if (pr <= 1.)
                                for (int i=0; i < Mob->GetNv(); ++i)
                                        PutDataPkt (Mob->GetDataPkt((int)start->x, 
                                                                    (int)start->y,
                                                                    i),
                                                    iy, i);
                        else
                                for (int i=0; i < Mob->GetNv(); ++i)
                                        PutDataPkt (Mob->GetDataDiscAv((int)start->x, 
                                                                       (int)start->y,
                                                                       i, pr),
                                                    iy, i);
                } else {
                        Resize (iy+1, 1);
                        if (pr <= 1.)
                                PutDataPkt (Mob->GetDataPkt((int)start->x, 
                                                            (int)start->y,
                                                            v),
                                            iy, 0);
                        else
                                PutDataPkt (Mob->GetDataDiscAv((int)start->x, 
                                                               (int)start->y,
                                                               v, pr),
                                            iy, 0);
                }
                
		sdata_dest->s.ny = Mob->GetNv();
		sdata_dest->s.nx = iy+1;

		sdata_dest->s.dl = 1.;

		sdata_dest->s.rx = sdata_dest->s.dl;
		sdata_dest->s.ry = iy+1.;

		sdata_dest->s.y0 = Mob->data->GetXLookup (0);
		sdata_dest->s.x0 = 0.;
		sdata_dest->s.dy = sdata_dest->s.rx/(sdata_dest->s.nx-1);
		sdata_dest->s.dx = 1.;
		sdata_dest->s.dz = sdata_src->s.dz;

		data->SetXLookup (iy, Mob->get_frame_time ());
		data->MkYLookup (Mob->data->GetVLookup (0), Mob->data->GetVLookup (Mob->GetNv()-1));
		sdata_dest->s.alpha = 0.;

		if (sdata_src->TimeUnit)
			sdata_dest->SetXUnit (sdata_src->TimeUnit);
		if (sdata_src->Vunit)
			sdata_dest->SetYUnit (sdata_src->Vunit);

	} else {
		Resize (Mob->GetNv(), iy+1);
  
		if (pr <= 1.)
			for (int i=0; i < Mob->GetNv(); ++i)
				PutDataPkt (Mob->GetDataPkt((int)start->x, 
							    (int)start->y,
							    i),
					    i, iy);
		else
			for (int i=0; i < Mob->GetNv(); ++i)
				PutDataPkt (Mob->GetDataDiscAv((int)start->x, 
							       (int)start->y,
							       i, pr),
					    i, iy);

		sdata_dest->s.nx = Mob->GetNv();
		sdata_dest->s.ny = iy+1;

		sdata_dest->s.dl = Mob->data->GetVLookup (Mob->GetNv()-1) - Mob->data->GetVLookup (0);

		sdata_dest->s.rx = sdata_dest->s.dl;
		sdata_dest->s.ry = iy+1.;

		sdata_dest->s.x0 = Mob->data->GetXLookup (0);
		sdata_dest->s.y0 = 0.;
		sdata_dest->s.dx = sdata_dest->s.rx/(sdata_dest->s.nx-1);
		sdata_dest->s.dy = 1.;
		sdata_dest->s.dz = sdata_src->s.dz;

                if (1){ // always assume layer lookup is possibly non linear
                        for (int i=0; i < Mob->GetNv(); ++i)
                                data->SetXLookup (i, Mob->data->GetVLookup (i)); // remap Layer Lookup Value to X-Lookup
                } else
                        data->MkXLookup (Mob->data->GetVLookup (0), Mob->data->GetVLookup (Mob->GetNv()-1));
		data->SetYLookup (iy, Mob->get_frame_time ());
		sdata_dest->s.alpha = 0.;

		if (sdata_src->Vunit)
			sdata_dest->SetXUnit (sdata_src->Vunit);
		if (sdata_src->TimeUnit)
			sdata_dest->SetYUnit (sdata_src->TimeUnit);

	}

	return 0;
}


int Mem2d::GetArcDataLineFrom(Point2D *center, Point2D *radius, Mem2d *Mob, SCAN_DATA *sdata, double dr){
	double dx, dy, r, circumference, dphi;
	int    n;
	
	// Radius
	dx = (double)(center->x - radius->x);
	dy = (double)(center->y - radius->y);
	r  = sqrt(dx*dx+dy*dy);
	circumference = 2.*r*M_PI;
	n  = (int)circumference;
	dphi = 2.*M_PI/(double)n;

	Resize (n, 1);

	for (int i=0; i < n; ++i)
		PutDataPkt (Mob->GetDataPktInterpol (center->x + r*cos((double)i*dphi),
						     center->y + r*sin((double)i*dphi)
						     ),
			    i, 0);
	
	sdata->s.nx = n;
	sdata->s.ny = 1;
	
	sdata->s.dl = 360.;
	
	sdata->s.rx = sdata->s.dl;
	sdata->s.ry = r*sdata->s.dx;
	
	sdata->s.x0 = center->x;
	sdata->s.y0 = center->y;
	sdata->s.dy = r*sdata->s.dx;
	sdata->s.dx = 180.*dphi/M_PI;
	
	data->MkXLookup(0., 360.);
	
	sdata->s.alpha = 0.;

	UnitObj *ArcUnit = new UnitObj (UTF8_DEGREE, "\260"); // "°"
	sdata->SetXUnit (ArcUnit);
	delete ArcUnit;
	
	return 0;
}

void Mem2d::DataRead(std::ifstream &f, int ydir){
	int y;
	if (ydir>0){
		ydir = 1;
		y = 0;
	} else {
		ydir = -1;
		y = data->GetNy()-1;
	}
	for(int x = 0, l = 0; l<data->GetNy(); l++, y+=ydir){
		if(f.good()){
			if (WORDS_BIGENDIAN){
				// correct endianess
				int  bpz = data->Zsize();
				char *tmp = new char[bpz*(data->GetNx()+1)];
				f.read(tmp+bpz, bpz*data->GetNx());
				for(int i=0; i<bpz*data->GetNx(); i+=bpz)
					for(int j=0; j<bpz; ++j)
						tmp[i+j] = tmp[i+2*bpz-j-1];
				memcpy(data->GetPtr(x,y), tmp, data->Zsize()*data->GetNx());
				delete[] tmp;
			}else{
				f.read((char*)data->GetPtr(x,y), data->Zsize()*data->GetNx());
			}
		}
		else
			XSM_DEBUG (DBG_L6, "File ends unexpectetly !");
                data->InvalidateLine (y);
	}
	data_valid=1;
}

// I'm able to import D2D !!!!
// Read: 0..65536 =(^2)=> 0 .. 2^32/2500 = 1.7e6
// CNT =  tau * XCPS*XCPS/2500
void Mem2d::DataD2DRead(std::ifstream &f, double gate){
	int i,j;
	double fac, val;
	//  unsigned short *buf;
	//  buf = new unsigned short[(nx+1)*(ny+1)];
	short *buf, *ptr;
	ptr = buf = g_new( short, (data->GetNx()+1)*(data->GetNy()+1) );
	f.read((char*)buf, sizeof(short)*(data->GetNx()+1)*data->GetNy());
	//  fac = (spa->data.GateTime*1e-3)/2500.; // spa4
	fac = (gate*1e-3)/2500.; // spa4
	XSM_DEBUG (DBG_L6, "D2DRead factor:" << fac);
	for(i=0; i<data->GetNy(); i++){
		for(j=0; j<data->GetNx(); j++,buf++){
			//      d2cnt[i][j] = (CNT)*buf*(CNT)*buf;
			if (WORDS_BIGENDIAN)
				val = (double) (GINT16_FROM_LE (*buf)) + 32768.;
			else
				val = (double) (*buf) + 32768.;
			//      d2cnt[i][j] = (CNT)(val*val*fac);
			data->Z(val*val*fac,j,i);
		}
		buf++;
                data->InvalidateLine (i);
	}
	g_free( ptr );
	data_valid=1;
}

void Mem2d::DataWrite(std::ofstream &f){
	for(int x=0, y=0; y<data->GetNy(); y++)
		f.write((const char*)data->GetPtr(x,y), data->Zsize()*data->GetNx());
}

void Mem2d::AutoHistogrammEvalMode (Point2D *p1, Point2D *p2, int delta, double epsilon){
	double dz_norm;
//	double m_imed;
	int    med_bin, lo_bin, hi_bin;
	int nx0, nx, ny0, ny;

	// find full data range in area
	HiLoMod (p1, p2, delta);

	// check area
	if(p1 && p2){
		ny0 = MIN (MAX (MIN (p1->y, p2->y), 0), (data->GetNy()-1));
		ny = MIN (MAX (MAX ((p1->y-delta), (p2->y-delta)), 0), (data->GetNy()-1));
		nx0 = MIN (MAX (MIN (p1->x, p2->x), 0), (data->GetNx()-1));
		nx = MIN (MAX (MAX ((p1->x-delta), (p2->x-delta)), 0), (data->GetNx()-1));
	}
	else{
		nx0=ny0=0;
		nx=data->GetNx()-1; 
		ny=data->GetNy()-1;
	}

        // auto add border 1px
        nx0++; nx-=2;
        ny0++; ny-=2;

        // check for sufficient area
	if (nx0 > (nx-delta) || ny0 > (ny-delta))
		return; // selected area too small. ERROR!!

	// calculate histogramm, auto bin size
	Zbin_num   = MAX((int)(Zrange/3), 10);  // +/-1 dz (3dz) in ein bin per default
	Zbin_width = Zrange / Zbin_num;
	dz_norm   = 1./Zbin_width;
		
	if (Zbins) delete [] Zbins;
	Zbins = new double[Zbin_num];
	if (Zibins) delete [] Zibins;
	Zibins = new double[Zbin_num];

	for(int i = 0; i < Zbin_num; i++)
		Zbins[i] = 0.;

	for(int col = nx0; col < nx; col+=delta)
		for(int line = ny0; line < ny; line+=delta){
			double f  = (GetDataMode (col,line) - Zmin) / Zbin_width;
			int bin   = (int)f;
				
			if (bin < Zbin_num){
				double f1 = (bin+1) * Zbin_width;
				if ((f+dz_norm) > f1){ // partiell in bin, immer rechter Rand, da bin>0 && bin < f
					Zbins [bin] += f-bin;
					++bin;
					if (bin < Zbin_num)
						Zbins [bin] += bin-f; // 1.-(f-(bin-1))
				}
				else // full inside of bin
					Zbins [bin] += 1.;
			}
		}

	// integrate histogramm data and auto evaluate relevant area around medium
	Zibins [0] = Zbins [0];
	for(int i = 1; i < Zbin_num; i++)
		Zibins[i] = Zibins[i-1] + Zbins[i];

	Zilow   = Zibins [2];
	Zihigh  = Zibins [Zbin_num-3];
	Zirange = Zihigh - Zilow;
	Zimed   = Zilow + Zirange/2.;

	
	for (med_bin = 1; Zibins [med_bin] < Zimed; ++med_bin);

	#define DIFF(I) (Zibins [I+1] - Zibins [I-1])
//	m_imed = DIFF(med_bin);

	for (lo_bin = med_bin; lo_bin > 2 && Zibins [lo_bin] > (Zilow+Zirange*epsilon); --lo_bin);
	for (hi_bin = med_bin; hi_bin < (Zbin_num-3) && Zibins [hi_bin] < (Zihigh-Zirange*epsilon); ++hi_bin);

// not reliable
//	for (lo_bin = med_bin; lo_bin > 2 && DIFF(lo_bin) > 0.; --lo_bin);
//	for (hi_bin = med_bin; hi_bin < (Zbin_num-3) && DIFF(hi_bin) > 0.; ++hi_bin);

	Zmax = hi_bin*Zbin_width + Zmin;
	Zmin = lo_bin*Zbin_width + Zmin;
	Zrange=Zmax-Zmin;

/*
	std::ofstream h; h.open ("/tmp/hh", std::ios::out);
	h << "#ZAutoStatistics  Zmin=" << Zmin << " Zmax=" << Zmax << "\n";
	h << "#ZAutoStatistics  Zilow=" << Zilow << " Zimed=" << Zimed << " Zihigh=" << Zihigh << "\n";

	{ int i=0; h << (i*Zbin_width + Zmin) << " " << Zbins[i] << " " << Zibins[i] << " " << "0 -2" << " " << i << "\n"; }
	for (int i=1; i<Zbin_num-1; ++i)
		h << (i*Zbin_width + Zmin) << " " << Zbins[i] << " " << Zibins[i] << " " << DIFF(i) << " " << (i>lo_bin && i<hi_bin ? -1:-2) << " " << i << "\n";
	{ int i=Zbin_num-1; h << (i*Zbin_width + Zmin) << " " << Zbins[i] << " " << Zibins[i] << " " << "0 -2" << " " << i << "\n"; }
	h.close ();
*/
}


void Mem2d::HiLoMod(Point2D *p1, Point2D *p2, int Delta, gboolean expand){
	int i,j,i1,i2, j1,j2;
	double p, hi, lo;
	
	if(p1 && p2){
		i1 = MIN (MAX (MIN (p1->y, p2->y), 0), (data->GetNy()-1));
		i2 = MIN (MAX (MAX ((p1->y-Delta), (p2->y-Delta)), 0), (data->GetNy()-1));
		j1 = MIN (MAX (MIN (p1->x, p2->x), 0), (data->GetNx()-1));
		j2 = MIN (MAX (MAX ((p1->x-Delta), (p2->x-Delta)), 0), (data->GetNx()-1));
	}
	else{
		i1=j1=0;
		i2=data->GetNy()-1; 
		j2=data->GetNx()-1;
	}
	
//	std::cerr << "Mem2d::HiLoMod " << i1 << " " << i2 << " " << j1 << " " << j2 << " :: " << data->GetNx() << ":" << data->GetNy() << std::endl;
	
	hi=lo=GetDataMode(j1, i1);

	if (i1 > (i2-Delta) || j1 > (j2-Delta)){
//		std::cerr << "Mem2d::HiLoMod ERROR" << std::endl;
		// bad area, make some defaults
		hi *= 1.2;
		lo *= 0.8;
	}
	else{
		for(i=i1; i<i2; i+=Delta) 
			for(j=j1; j<j2; j+=Delta){
				p=GetDataMode(j, i);
				if(hi < p) hi = p;
				else
					if(lo > p) lo = p;
			}
	}
        SetHiLo (hi, lo, expand);
	XSM_DEBUG (DBG_L6, "Mem2d::HiLoMod  Zmin=" << Zmin << " Zmax=" << Zmax);
}

void Mem2d::HiLo(double *hi, double *lo, int LinReg, Point2D *p1, Point2D *p2, int Delta){
	int i,j,i1,i2, j1,j2;
	double Hi, Lo;
//	i1 = p1 ? p1->y : 0;
//	i2 = p2 ? p2->y : data->GetNy();
//	j1 = p1 ? p1->x : 0;
//	j2 = p2 ? p2->x : data->GetNx();
	if(p1 && p2){
		i1 = MIN (MAX (MIN (p1->y, p2->y), 0), (data->GetNy()-1));
		i2 = MIN (MAX (MAX ((p1->y-Delta), (p2->y-Delta)), 0), (data->GetNy()-1));
		j1 = MIN (MAX (MIN (p1->x, p2->x), 0), (data->GetNx()-1));
		j2 = MIN (MAX (MAX ((p1->x-Delta), (p2->x-Delta)), 0), (data->GetNx()-1));
	}
	else{
		i1=j1=0;
		i2=data->GetNy()-1; 
		j2=data->GetNx()-1;
	}

	if(LinReg){
		double p;
		Hi=Lo=GetDataPktLineReg(j1, i1);
		for(i=i1; i<i2; i+=Delta) 
			for(j=j1; j<j2; j+=Delta){
				p=GetDataPktLineReg(j, i);
				if(Hi < p) Hi = p;
				else
					if(Lo > p) Lo = p;
			}
	}
	else{
		data->SetPtr(j1,i1);
		Hi = Lo = data->GetThis();
		for(i=i1; i<i2; i++)
			for(j=j1, data->SetPtr(j1,i); j<j2; j++){
				if(Hi < data->GetThis()) Hi = data->GetNext();
				else
					if(Lo > data->GetThis()) Lo = data->GetNext();
					else 
						data->GetNext();
			}
	}
	//  if(Hi < 1) Hi=1L; // Div Zero prevent....
	*hi = (double)Hi;
	*lo = (double)Lo;
}

int Mem2d::DataValid(){ return data_valid; }

int Mem2d::SetDataValid(){ return data_valid=1; }

// MemDigiFilter - public Mem2d

MemDigiFilter::MemDigiFilter(double Xms, double Xns, int M, int N, MemDigiFilter *adaptive_kernel, double adaptive_threashold):Mem2d(2*N+1, 2*M+1, ZD_DOUBLE){
        xms=Xms, xns=Xns;
        n=N, m=M;
        adaptive_kernel_test = adaptive_kernel;
        adaptive_threashold_test = adaptive_threashold;

        XSM_DEBUG (DBG_L2, "MemDigiFilter::Convolve: create kernel");
        kernel_initialized = FALSE;
        kname = "gxsm_convolution_kernel";
        
        XSM_DEBUG (DBG_L4, "MemDigiFilter: " << n << " " << m << " " << xms << " " << xns);
}

typedef struct{
        ZData *kernel;
        ZData *adaptive_kernel;
        ZData *In;
        ZData *Dest;
        int line_i, line_f, line_inc;
        int ns, ms;
        double adaptive_threashold;
        int job;
        int *status;
        double progress;
} Mem2d_Digi_Filter_Convolve_Job_Env;

inline double mem2d_digi_filter_convolve_point_kernel (int ii, int jj, Mem2d_Digi_Filter_Convolve_Job_Env* job, ZData *kern, int q=0){
        static int bail=0;
        const int tms = 2*job->ms;
        const int tns = 2*job->ns;
        double sum = 0.;
        if (ii==0 && jj==0) bail=0;
        if (q >= kern->GetNv()){
                if (bail++ < 30)
                        g_warning ("ADAPTIVE CONVOLUTION ERROR: mem2d_digi_filter_convolve_point_kernel called with q=%d but kernNv=%d",q, kern->GetNv());
                return 0.;
        }
        for (int i=0; i<=tms; ++i)
                for(int j=0; j<=tns; j++)
                        sum +=  job->In->Z (jj+j, i+ii) * kern->Z (j,i,q);
        return sum;
}                        


gpointer mem2d_digi_filter_convolve_thread (void *env){
        Mem2d_Digi_Filter_Convolve_Job_Env* job = (Mem2d_Digi_Filter_Convolve_Job_Env*)env;

        for(int ii=job->line_i; ii<job->line_f; ii+=job->line_inc){
                job->progress = (double)ii/job->line_f;
                for(int jj=0; jj<job->Dest->GetNx(); ++jj){
                        if (job->adaptive_kernel && job->adaptive_threashold > 0.){
                                double test_sum = 0.;
                                double data_sum = 0.;
                                int q=job->kernel->GetNv ();
                                int qq;
                                for (qq=0; qq<q; ++qq){
                                        test_sum = mem2d_digi_filter_convolve_point_kernel (ii, jj, job, job->adaptive_kernel, qq); // 0 or qq -- testing
                                        data_sum = mem2d_digi_filter_convolve_point_kernel (ii, jj, job, job->kernel, qq);
                                        if (fabs (test_sum - data_sum) > job->adaptive_threashold){
                                                data_sum = job->In->Z (data_sum,  jj,ii);
                                                break;
                                        }
                                }
                                //job->Dest->Z ((double)qq,  jj,ii);
                                //job->Dest->Z (test_sum - data_sum,  jj,ii);
                                job->Dest->Z (data_sum,  jj,ii);
                        } else
                                job->Dest->Z (mem2d_digi_filter_convolve_point_kernel (ii, jj, job, job->kernel),  jj,ii);
                }
                if (*(job->status)){
                        job->job = -2; // aborted
                        return NULL;
                }
        }
        
        job->job = -1;
        return NULL;
}

static void cancel_callback (void *widget, int *status);
static void cancel_callback (void *widget, int *status){
	*status = 1; 
}

void MemDigiFilter::MakeKernelNormalized (){
        int ms=m, ns=n;
        int q=0;

        g_message ("Base Kernel Size: %d, %d", 2*ns+1, 2*ms+1);
        Resize(2*ns+1, 2*ms+1, ZD_DOUBLE);
        g_message ("Calculating Kernel for Level %d", q);
        SetLayer (q);
        CalcKernel();
#if 0
        int i, j;
        gboolean again = FALSE;
        do{
                int ring_m, ring_n;
                // (Re)Calc Kernel ... xms, xns, ms, ns
                XSM_DEBUG (DBG_L6, "R " << (2*ms+1) << " " << (2*ns+1));
                n=ns; m=ms;
                
                g_message ("Base Kernel Size: %d, %d", 2*ns+1, 2*ms+1);
                Resize(2*ns+1, 2*ms+1, ZD_DOUBLE);
                XSM_DEBUG (DBG_L6, "MemDigiFilter::Resize done.");

                CalcKernel();
                XSM_DEBUG (DBG_L6, "MemDigiFilter::CalcKernel done.");

                
                // check for zero kernel elements on "ring", if all zero, reduct size to minimum needed for non-zero elements in ring
                for(ring_m=ring_n=-1, i=0; i<2*ms+1; i++)
                        for(j=0; j<2*ns+1; j++){
                                int tmp_ring_m = abs(i-m);
                                int tmp_ring_n = abs(j-n);
                                if(data->Z(j,i)!=0. && ring_m<tmp_ring_m)
                                        ring_m = tmp_ring_m;
                                if(data->Z(j,i)!=0. && ring_n<tmp_ring_n)
                                        ring_n = tmp_ring_n;
                        }
                again = FALSE;
                if(ring_m < m)	{again = TRUE; ms = ring_m;}
                if(ring_n < n)	{again = TRUE; ns = ring_n;}
        }while(again);
#endif
        
        if (adaptive_kernel_test && adaptive_threashold_test > 0.){
                q=ms > ns ? ns : ms;
                g_message ("Resize Kernel to: %d, %d, %d", 2*ns+1, 2*ms+1, q+1);
                Resize(2*ns+1, 2*ms+1, q+1, ZD_DOUBLE);
                for (int qq=1; qq<=q; qq++){
                        g_message ("Calculating Kernel for Level %d", qq);
                        SetLayer (qq);
                        CalcKernel ();
                }
        }
        data->mabs_norm (1., 0,q);
        SetLayer (0);

#ifdef SAVECONVOLKERN
        for (int qq=0; qq<q; ++qq){
                // save Kernel to /tmp/convolkern.dbl
                std::ofstream fk;
                gchar *fkn=g_strdup_printf ("/tmp/%s%02d.dbl", kname, qq);
                g_message ("Kernel saved to: %s", fkn);
                fk.open(fkn, std::ios::out);
                g_free (fkn);
                struct { short nx,ny; } fkh; 
                fkh.nx=data->GetNx();
                fkh.ny=data->GetNy();
                SetLayer (qq);
                fk.write((const char*)&fkh, sizeof(fkh));
                DataWrite(fk);
                fk.close();
        }
        SetLayer (0);
#endif

}

gboolean MemDigiFilter::Convolve(Mem2d *Src, Mem2d *Dest){
        int i0=0;
        int i, j;
        int mm=Src->data->GetNy(), nn=Src->data->GetNx(); // Src size
        int ms=m, ns=n; // kernel size
        int stop_flag = 0;
        int max_jobs = g_get_num_processors (); // default concurrency for multi threadded computation, # CPU's/cores
        
        gapp->progress_info_new ("DigiFilter Convolute", 1+(int)max_jobs, GCallback (cancel_callback), &stop_flag, false);
        gapp->progress_info_set_bar_fraction (0.1, 2);
        gapp->progress_info_set_bar_text ("Setup", 2);

        InitializeKernel ();
        
        gapp->progress_info_set_bar_fraction (0.3, 2);
        
        if(Src->data->GetNx()<1 && Src->data->GetNy()<1)
                return FALSE;
 
        XSM_DEBUG (DBG_L6, "Prep Larger Src: " << (Src->data->GetNx() + 2*ns) << " " << (Src->data->GetNy() + 2*ms));
        Mem2d x(nn + 2*ns, mm + 2*ms, Src->GetTyp()); // Plus Border: add kernel size left and right
 
        // *** WORKINGMARKER *** 11/2/1999 PZ ***
        // fill the central part of the x matrix with the data
        XSM_DEBUG (DBG_L6, "Mem2dDigi: " << ns << " " << ms << " " << nn << " " << mm);
        x.data->CopyFrom(Src->data, 0,0, ns,ms ,nn,mm, true); // total scan and observe shift
  
        // now fill edges and corners with copies of edge data
        // edge left / right
        for(i=0;i<mm;i++)
                for(j=0;j<ns;j++){
                        x.data->Z(Src->data->Z(i0,i),   j,      i+ms); // data[i+ms][j]       = Src->data[i][0];
                        x.data->Z(Src->data->Z(nn-1,i),j+ns+nn,i+ms); // data[i+ms][j+ns+nn] = Src->data[i][nn-1];
                }

        gapp->progress_info_set_bar_fraction (0.4, 2);

        // edge top / bottom and oberserve shift
        for(i=0;i<ms;i++){
                g_message ("Convol Top/Bot extend: i=%d ns=%d nn=%d mm=%d",i, ns, nn, mm);
                x.data->CopyFrom(Src->data, 0,0, ns,i ,nn, 1, true);
                x.data->CopyFrom(Src->data, 0,mm-2, ns,i+mm+ms ,nn, 1, true);
        } 
        x.data->CopyFrom(Src->data, 0,mm-2, ns,mm+ms-1 ,nn, 1, true);
        
        gapp->progress_info_set_bar_fraction (0.5, 2);

        // corners
        for(i=0;i<ms;i++)
                for(j=0;j<ns;j++){
                        x.data->Z(Src->data->Z(i0,i0), j,i);
                        x.data->Z(Src->data->Z(nn-1,i0), nn+ns+j,i);
                        x.data->Z(Src->data->Z(i0,mm-1), j,mm+ms+i);
                        x.data->Z(Src->data->Z(nn-1,mm-1), nn+ns+j,mm+ms+i);
                }
        gapp->progress_info_set_bar_fraction (0.8, 2);

// #define SAVECONVOLSRC
#ifdef  SAVECONVOLSRC
        // save datasrc with added bounds to /tmp/convolsrc.xxx
        std::ofstream fk;
        switch(x.GetTyp()){
        case ZD_BYTE: fk.open("/tmp/convolsrc.byt", std::ios::out); break;
        case ZD_SHORT: fk.open("/tmp/convolsrc.sht", std::ios::out); break;
        case ZD_LONG: fk.open("/tmp/convolsrc.lng", std::ios::out); break;
        case ZD_FLOAT: fk.open("/tmp/convolsrc.flt", std::ios::out); break;
        case ZD_DOUBLE: fk.open("/tmp/convolsrc.dbl", std::ios::out); break;
        default: break;
        }

        if(fk.good()){
                struct { short nx,ny; } fkh; 
                fkh.nx=x.data->GetNx();
                fkh.ny=x.data->GetNy();
                fk.write((const char*)&fkh, sizeof(fkh));
                x.DataWrite(fk);
                fk.close();
                g_message ("Saved tmp convolution src /tmp/covolsrc.flt/dbl/...");
        }
#endif
        
        //
        if (Dest->GetNv () == 1)
                Dest->Resize(nn,mm);
      
        // do convolution !

        if (max_jobs < 2){

                gapp->progress_info_set_bar_text ("Convolution", 2);

                // old single cpu convole code, using incremetal ZData access
                // -- note: this is NOT thread save, pointer stored/adjusted in zdata, only single access at a time!
                gint pcent=0;
                for(int ii=0; ii<mm; ++ii){
                        if(pcent < 100*ii/mm ){
                                gapp->progress_info_set_bar_fraction ((gfloat)ii/(gfloat)mm, 2);
                                //SET_PROGRESS((gfloat)ii/(gfloat)mm);
                                if (stop_flag)
                                        break;
                        }

                        for(int jj=0; jj<nn; ++jj){
                                const int tms = 2*ms;
                                const int tns = 2*ns;
                                double sum;
      
                                for(sum=0., i=0; i<=tms; ++i){
                                        data->SetPtr(0,i);
                                        x.data->SetPtr(jj,i+ii);
                                        for(j=0; j<=tns; j++)
                                                sum += x.data->GetNext() * data->GetNext();
                                }
                        }
                }
        } else {
                // new threadded convole code
                if (max_jobs > 3)
                        max_jobs--; // leave one

                // do convolution, thread up convolute jobs !
                Mem2d_Digi_Filter_Convolve_Job_Env job[max_jobs];
                GThread* tpi[max_jobs];
        
                for (int jobno=0; jobno < max_jobs && jobno < Dest->GetNy (); ++jobno){
                        // std::cout << "Job #" << jobno << std::endl;
                        job[jobno].kernel = data;
                        job[jobno].adaptive_kernel = adaptive_kernel_test ? adaptive_kernel_test->data : NULL;
                        job[jobno].adaptive_threashold = adaptive_threashold_test;
                        job[jobno].In     = x.data;
                        job[jobno].Dest   = Dest->data;
                        job[jobno].line_i = jobno;
                        job[jobno].line_inc = max_jobs;
                        job[jobno].line_f = Dest->GetNy ();
                        job[jobno].ms = ms;
                        job[jobno].ns = ns;
                        job[jobno].job = jobno+1;
                        job[jobno].progress = 0.;
                        job[jobno].status = &stop_flag;               
                        tpi[jobno] = g_thread_new ("mem2d_digi_filter_convolve_thread", mem2d_digi_filter_convolve_thread, &job[jobno]);
                }
                // std::cout << "Waiting for all threads to complete." << std::endl;
        
                for (int running=1; running;){
                        running=0;
                        for (int jobno=0; jobno < max_jobs; ++jobno){
                                if (job[jobno].job >= 0)
                                        running++;
                                gapp->progress_info_set_bar_fraction (job[jobno].progress, jobno+2);
                                gchar *info = g_strdup_printf ("ConvJob %d", jobno+1);
                                gapp->progress_info_set_bar_text (info, jobno+2);
                                g_free (info);
                        }
                }
        
                for (int jobno=0; jobno < max_jobs; ++jobno)
                        if (tpi[jobno])
                                g_thread_join (tpi[jobno]);

        }

        Dest->data->CopyLookups (Src->data);
        Dest->copy_layer_information (Src);
        
        gapp->progress_info_close ();
     
        return TRUE;  
}

