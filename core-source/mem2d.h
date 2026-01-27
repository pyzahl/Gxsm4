/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyight (C) 1999,2000,2001,2002,2003 Percy Zahl
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

#ifndef __MEM2D_H
#define __MEM2D_H

#ifndef __XSMTYPES_H
#include "xsmtypes.h"
#endif

#ifndef __XSMMASKS_H
#include "xsmmasks.h"
#endif

#ifndef __XSMDEBUG_H
#include "xsmdebug.h"
#endif

#include <config.h>
#include <netcdf>
#include <vector>

#if defined HAVE_FFTW3_H
# include <complex>
# include <fftw3.h>
#define c_re(c) ((c)[0])
#define c_im(c) ((c)[1])
#endif

#include <glm/vec4.hpp>

#include <netcdf>
using namespace netCDF;

#define MEM_SET    1
#define MEM_ADDTO  2
#define MEM_AVG    3 

#define MAX_FRAME_TIMES 4  /* and frame time -- max 4 frame times */

enum MEM2D_DIM { MEM2D_DIM_XY, MEM2D_DIM_X, MEM2D_DIM_Y, MEM2D_DIM_LAYER, MEM2D_DIM_TIME };
 enum GETLINEORGMODE { GLORG_ZERO, GLORG_CENTER, 
			      GLORG_PRJX, GLORG_PRJY,
			      GLORG_HVMODE };

typedef enum { ZD_IDENT, ZD_BYTE, ZD_SHORT, ZD_LONG, ZD_ULONG, ZD_LLONG, 
	       ZD_FLOAT, ZD_DOUBLE, ZD_COMPLEX, ZD_RGBA, ZD_EVENT
} ZD_TYPE;

typedef gint8   TYPE_ZD_BYTE;
typedef gint16  TYPE_ZD_SHORT;
typedef guint16 TYPE_ZD_USHORT;
typedef gint32  TYPE_ZD_LONG;
typedef guint32 TYPE_ZD_ULONG;
typedef float   TYPE_ZD_FLOAT;
typedef double  TYPE_ZD_DOUBLE;
// used with auto fixed layer dimensions
typedef double  TYPE_ZD_DOUBLE_C;
typedef gint16  TYPE_ZD_RGBA;
typedef char    TYPE_ZD_EVENT;

typedef enum { M2D_ZERO, M2D_COPY
} MEM2D_CREATE_MODE;

typedef unsigned long ZVIEW_TYPE;




#define MEMELEMNAMELEN 14
extern const char *ZD_name[];

class Mem2d;
class ZData;

#ifndef __LINEINFO_H
#include "lineinfo.h"
#endif

#ifndef __ZDATA_H
#include "zdata.h"
#endif

#ifndef __SCAN_EVENT_H
#include "scan_event.h"
#endif

class LayerInformation{
public:

	LayerInformation (const gchar *description, const char *string=NULL);
	LayerInformation (const gchar *description, double x, const gchar *format=NULL);
	LayerInformation (const gchar *description, double x, double y, const gchar *format=NULL);
	LayerInformation (LayerInformation *li);
	// create layer info tuple from ncf file from specified coordinates ti (time-index), v (value-index), k (layer-info-index)
	LayerInformation (NcVar &ncv_description, NcVar &ncv_format, NcVar &ncv_osd_format, NcVar &ncv_values, int ti, int v, int k);
	~LayerInformation ();
	gchar *get (gboolean osd=FALSE);
	double valule (int i=0){ return i==0?val[0]:val[1]; };
	int get_len_description () { return desc?strlen (desc):0; };
	int get_len_format () { return fmt?strlen (fmt):0; };
	int get_len_osd_format () { return fmtosd?strlen (fmtosd):0; };
	// write layer info tuple to ncf file at specified coordinates ti (time-index), v (value-index), k (layer-info-index)
	void write_layer_information (NcVar &ncv_description, NcVar &ncv_format, NcVar &ncv_osd_format, NcVar &ncv_values, int ti, int v, int k);
private:
	gchar *desc;
	double val[2];
	gchar *fmt, *fmtosd;
	int num;
};



/**
 *  Mem2d:
 *
 *  Gxsm 2D x n layers data storage and handling object.
 */

class Mem2d{
public:
	Mem2d(int Nx, int Ny, ZD_TYPE type=ZD_SHORT);
	Mem2d(int Nx, int Ny, int Nv, ZD_TYPE type=ZD_SHORT);
	Mem2d(Mem2d *m, MEM2D_CREATE_MODE Option=M2D_ZERO);
	Mem2d(Mem2d *m, int Nx, int Ny, int Nv=1);
	virtual ~Mem2d();

	void copy(Mem2d *m, int x0=-1, int y0=-1, int vi=-1, int vf=-1, int nx=-1, int ny=-1, gboolean observe_shift=false);

	size_t      GetEsz(){return data->Zsize();};
	ZD_TYPE     GetTyp(){return zdtyp;};
        const char* GetTypName(){
                switch (GetTyp()){
                case ZD_DOUBLE:  return "DoubleField"; break; // Double data field ?
                case ZD_FLOAT:   return "FloatField"; break;  // Float data field ?
                case ZD_SHORT:   return "H"; break;           // standart "SHORT" Topo Scan ?
                case ZD_LONG:    return "Intensity"; break;   // used by "Intensity" -- diffraction counts
                case ZD_BYTE:    return "ByteField"; break;   // Byte ?
                case ZD_COMPLEX: return "ComplexDoubleField"; break; // Complex ?
                case ZD_RGBA:    return "RGBA_ByteField"; break;     // RGBA Byte ?
                default:    return "GXSM TYP ERROR"; break;
                }
        };
	const char* GetEname();

        inline int  GetNx(){return data->nx;};
        inline int  GetNxSub(){return data->GetNxSub();};
        inline int  GetNy(){return data->ny;};
        inline int  GetNySub(){return data->GetNySub();};
        inline int  GetNv(){return data->nv;};

	inline int  GetLayer(){return data->GetLayer();};
	inline void SetLayer(int l){data->SetLayer(l);};
	inline void SetLayerDataPut(int l){data->SetLayerDataPut (l);};

	// These are only to keep track of the time index
	int  get_t_index(){ return t_index; };
	void set_t_index(int ti){ t_index = ti; };

	// These are only to keep track of the frame time -- there may be more than one w different offsets, etc.
	double  get_frame_time(gint i=0){ if (i<0 || i >= MAX_FRAME_TIMES) i=0; return frame_time[i]; };
	void set_frame_time(double t, gint i=0){ if (i<0 || i >= MAX_FRAME_TIMES) i=0; frame_time[i] = t; };

	void Modus(int mod);
	int  Resize(int Nx, int Ny, ZD_TYPE type=ZD_IDENT, gboolean keep_layer_info=FALSE);
	int  Resize(int Nx, int Ny, int Nv, ZD_TYPE type=ZD_IDENT, gboolean keep_layer_info=FALSE);
	inline void PutDataLine(int y, void *src){ 
		data_valid=1;
		data->ZPutDataLine(y, src); 
		data->InvalidateLine (y);
	};
	inline void PutDataLine16dot16(int y, void *src, int mode=MEM_SET){ 
		data_valid=1;
		data->ZPutDataLine16dot16(y, src, mode); 
		data->InvalidateLine (y);
	};
	inline void SetData (double value, int x0, int y0, int nx, int ny){
                for(int y=y0; y<ny && y<y0+ny; ++y)
                        for(int x=x0; x<nx && x<x0+nx; ++x)
                                data->Z (value, x,y);
        };
	inline void CopyFrom(Mem2d *src, int x, int y, int tox, int toy, int nx, int ny=1, gboolean observe_shift=false){
		data->CopyFrom(src->data, x,y, tox,toy, nx,ny, observe_shift);
	};
	inline void ConvertFrom(Mem2d *src, int x, int y, int tox, int toy, int nx, int ny=1, int rev_y=0, gboolean observe_shift=false){
		// no conversion
		if (GetTyp() == src->GetTyp()){
			data->CopyFrom(src->data, x,y, tox,toy, nx,ny, observe_shift);
			return;
		}
		// special conversions
		if (GetTyp() == ZD_COMPLEX){
			// RE = Z, IM = 0
			for(int i=y, i2=toy; ny--; i++, i2++){
				for(int n=nx, j=y, j2=toy; n--; j++, j2++){
					data->Z (src->data->Z (j,i), j2,i2,0);
					data->Z (src->data->Z (j,i), j2,i2,1);
					data->Z (0., j2,i2,2);
				}
                                data->InvalidateLine (i2);
			}
			return;
		}
		if (src->GetTyp() == ZD_COMPLEX){
			// data = abs(Z)
			for(int i=y, i2=toy; ny--; i++, i2++){
				for(int n=nx, j=y, j2=toy; n--; j++, j2++)
					data->Z (sqrt ( src->data->Z (j,i,1) * src->data->Z (j,i,1)
						       +src->data->Z (j,i,2) * src->data->Z (j,i,2)),
						 j2,i2);
                                data->InvalidateLine (i2);
			}
			return;
		}
		// normal conversions
		if (!rev_y)
			for(int i=y, i2=toy; ny--; i++, i2++){
				for(int n=nx, j=y, j2=toy; n--; j++, j2++)
					data->Z(src->data->Z(j,i), j2,i2);
                                data->InvalidateLine (i2);
			}
		else
			for(int i=y, i2=toy+ny-1; ny--; i++, i2--){
				for(int n=nx, j=y, j2=toy; n--; j++, j2++)
					data->Z(src->data->Z(j,i), j2,i2);
                                data->InvalidateLine (i2);
			}
	};
	inline void PutDataLine(int y, void *src, int mode){ 
		data_valid=1;
		data->ZPutDataLine(y, src, mode); 
                data->InvalidateLine (y);
	};
	inline void   GetDataLine(int y, void *dest){ 
		data->ZGetDataLine(y, dest); 
	};
	inline double GetDataPkt(int x, int y){ 
		return data->Z(x,y); 
	};
	inline double GetDataPkt(int x, int y, int v){ 
		return data->Z(x,y,v); 
	};
        inline void GetDataPkt_vec_normal_4F(int x, int y, int v, glm::vec4 *v4, double zs=1.0, double nzs=1.0){ 
                // calculate average normal at x,y (v) vector vec4[0..2] and z at point in vec4[3]
                if (GetNx() < 4) return;
                if (x<1){ x=1; } if (x>=GetNx()-2) x=GetNx()-2;
                if (y<1){ y=1; } if (y>=GetNy()-2) y=GetNy()-2;
                double dxdz = data->Z(x+1,y,v) - data->Z(x-1,y,v);
                double dydz = data->Z(x,y+1,v) - data->Z(x,y-1,v);
                dxdz *= nzs;
                dydz *= nzs;
                // simplified cross product of vector from p(x-1,y,z) to p (x+1,y,z) X vector from p(x,y-1,z) to p(x,y+1,z) is:
                v4->x = dxdz;
                v4->y = -dydz;
                v4->z = 2.;
                v4->w = zs*data->Z(x,y,v); // "Z" height
                double d = sqrt(dxdz*dxdz + dydz*dydz + 4.); // normalize
                v4->x /= d;
                v4->y /= d;
                v4->z /= d;
        };
	inline void GetDataPktVMode_vec_normal_4F(int x, int y, int v, glm::vec4 *v4, double zs=1., double nzs=1.0){ 
                if (GetNx() < 4) return;
                if (x<1){ x=1; } if (x>=GetNx()-2) x=GetNx()-2;
                if (y<1){ y=1; } if (y>=GetNy()-2) y=GetNy()-2;
                // calculate average normal at x,y (v) vector vec4[0..2] and z at point in vec4[3]
                double dxdz = GetDataVMode (x+1,y,v) - GetDataVMode (x-1,y,v);
                double dydz = GetDataVMode (x,y+1,v) - GetDataVMode (x,y-1,v);
                dxdz *= nzs;
                dydz *= nzs;
                // simplified cross product of vector from p(x-1,y,z) to p (x+1,y,z) X vector from p(x,y-1,z) to p(x,y+1,z) is:
                v4->x = dxdz;
                v4->y = -dydz;
                v4->z = 2.;
                v4->w = zs*GetDataVMode (x,y,v); // "Z" height
                double d = sqrt(dxdz*dxdz + dydz*dydz + 4.); // normalize
                v4->x /= d;
                v4->y /= d;
                v4->z /= d;
	};
	inline void GetDataPktVModeInterpol_vec_normal_4F(double x, double y, double v, glm::vec4 *v4, double zs=1., double nzs=1.0){ 
                // calculate average normal at x,y (v) vector vec4[0..2] and z at point in vec4[3]
                double dxdz = GetDataVModeInterpol (x+1,y,v) - GetDataVModeInterpol (x-1,y,v);
                double dydz = GetDataVModeInterpol (x,y+1,v) - GetDataVModeInterpol (x,y-1,v);
                dxdz *= nzs;
                dydz *= nzs;
                v4->x = dxdz;
                v4->y = -dydz;
                v4->z = 2.;
                v4->w = zs*GetDataVModeInterpol (x,y,v); // "Z" height
                double d = sqrt(dxdz*dxdz + dydz*dydz + 4.); // normalize
                v4->x /= d;
                v4->y /= d;
                v4->z /= d;
	};
	double GetDataDiscAv(int x, int y, int v, double r);
	double GetDataWidthAv(double x, double y, int v, double w, double s, double dx, double dy);
	double GetDataPktInterpol(double x, double y);
	double GetDataPktInterpol(double x, double y, int v);
        double GetDataPktInterpol(double x, double y, double v, Scan *sct, int tn);
	double GetDataPktInterpol(double x, double y, double v);
	double GetDataPktInterpol(double x, double y, int v, double t, Scan *sct, double dt=0., double t0=0.);
	double GetDataPktInterpol(double x, double y, double dr, int ly);
	double GetDataPktInterpol(double x, double y, double dw, double ds, int ly, double dx, double dy);
	inline void   PutDataPkt(double value, int x, int y){ 
		data->Z(value,x,y); 
                data->InvalidateLine (y);
	};
	inline void   PutDataPkt(double value, int x, int y, int v){ 
		data->Z(value,x,y,v); 
                data->InvalidateLine (y,v);
	};
	inline void   PutDataPkt_ixy_sub(double value, int x, int y, int v){
		data->Z_ixy_sub(value,x,y,v); 
                data->InvalidateLineSub (y,v);
	};
	double GetDataPktLineReg(int x, int y);
	double GetDataPktHorizont(int x, int y);
	double GetDataPktLog(int x, int y);
	double GetDataPktDiff(int x, int y);
	double GetDataPktPlaneSub(int x, int y);

	void SetPlaneMxyz (double mx=0., double my=0., double z0=0.) { plane_mx = mx; plane_my = my; plane_z0=z0; };
	void GetPlaneMxyz (double &mx, double &my, double &z0) { mx = plane_mx; my = plane_my; z0 = plane_z0; };

	ZVIEW_TYPE GetDataVMode(int x, int y);
	ZVIEW_TYPE GetDataVMode(int x, int y, int v);
	ZVIEW_TYPE GetDataVModeInterpol(double x, double y, double v);
        double GetDataVModeInterpol_double (double x, double y, int v);
	inline double GetDataMode(int x, int y);

	/* *ZFkt points to one of this Mapping Functions */
	inline ZVIEW_TYPE ZQuick(int &x, int &y);
	inline ZVIEW_TYPE ZDirect(int &x, int &y);
	inline ZVIEW_TYPE ZHiLitDirect(int &x, int &y);
	inline ZVIEW_TYPE ZPlaneSub(int &x, int &y);
	inline ZVIEW_TYPE ZLog(int &x, int &y);
	inline ZVIEW_TYPE ZPeriodic(int &x, int &y);
	inline ZVIEW_TYPE ZDifferential(int &x, int &y);
	inline ZVIEW_TYPE ZHorizontal(int &x, int &y);

        void set_px_shift (double pxdxdt=0., double pxdydt=0., double pxtau=0.) {
                double dt = get_frame_time (0) - get_frame_time (1); 
                if (data)
                        data->set_shift (pxtau > 0. ? 1. - exp (-pxtau*dt) : pxtau, pxdxdt, pxdydt);
        };
        
	void SetDataPktMode(int mode);
	void SetDataRange(ZVIEW_TYPE Min, ZVIEW_TYPE Max);
	double GetDataRange() { return  (ZVmax-ZVmin); };
	void SetHiLo(double hi, double lo, gboolean expand=false);
	void SetDataSkl(double contrast, double bright){ Zcontrast=contrast; Zbright=bright; };
	void SetDataVRangeZ(double VRangeZ, double VOffsetZ, double dz);
	void SetZHiLitRange(double HiLit_low, double HiLit_hi) { ZHiLit_low = HiLit_low;  ZHiLit_hi = HiLit_hi; };
	void SetHiLitVRange(ZVIEW_TYPE HiLit_range, ZVIEW_TYPE HiLit_base) { ZVHiLitRange = HiLit_range;  ZVHiLitBase = HiLit_base; };
	void AutoDataSkl(double *contrast, double *bright);
	void SetDataFkt(ZVIEW_TYPE (Mem2d::*newZVFkt)(int &, int &), 
			double (Mem2d::*newZFkt)(int, int));

	void CalcLinRegress(int yfirst, int ylast);

	int  GetDataLineFrom(Point2D *start, Point2D *end, Mem2d *Mob, SCAN_DATA *sdata_src, SCAN_DATA *sdata_dest, GETLINEORGMODE orgmode=GLORG_ZERO, double dw=0., double ds=0., MEM2D_DIM profile_dim=MEM2D_DIM_XY, MEM2D_DIM series_dim=MEM2D_DIM_LAYER, gboolean series=FALSE, int series_i=0, int series_nt=0, gboolean append=FALSE);
	int  GetArcDataLineFrom(Point2D *center, Point2D *radius, Mem2d *Mob, SCAN_DATA *sdata, double dr=0.);
	int  GetLayerDataLineFrom(Point2D *start, Mem2d *Mob, SCAN_DATA *sdata_src, SCAN_DATA *sdata_dest, double pr=0., int iy=0, int transpose=FALSE, int v=-1);

	void HiLoMod(Point2D *p1=NULL, Point2D *p2=NULL, int Delta=4, gboolean expand=false);
	void HiLo(double *hi, double *lo, int LinReg=FALSE, Point2D *p1=0, Point2D *p2=0, int Delta=4);
	void GetZHiLo(double *hi, double *lo);
	double GetZRange();

	void AutoHistogrammEvalMode(Point2D *p1=NULL, Point2D *p2=NULL, int delta=4, double epsilon=0.05);
	int    GetHistogrammBinNumber() { return Zbin_num; }
	double GetHistogrammBinWidth() { return Zbin_width; }
	double GetZHistogrammEvents(int bin) { return Zbins ? Zbins [MIN (MAX (bin,0), Zbin_num-1)] : 0.; };
	double GetZiHistogrammEvents(int bin) { return Zibins ? Zibins [MIN (MAX (bin,0), Zbin_num-1)] : 0.; };
	void   GetHistogrammIStats(double &ih, double &im, double &il, double &ir) { 
		ih=Zihigh; im=Zimed; il=Zilow;  ir=Zirange; 
	};

	void DataRead(std::ifstream &f, int ydir=1);
	void DataD2DRead(std::ifstream &f, double gate);
	void DataWrite(std::ofstream &f);


	int  DataValid();
	int  SetDataValid();

	/* Scan Events Handling: */
	static void evl_remove(gpointer entry, gpointer from);
	void RemoveScanEvents ();
        GSList* ReportProbeEvents ();
	GSList* ReportScanEvents (GFunc report_obj_func, gpointer gp, double *xy=NULL, double distance=0., int number=0);
	GSList* ReportScanEventsUnsort ();
	GSList* ReportScanEventsXasc ();
	GSList* ReportScanEventsYasc ();
	void AttachScanEvent (ScanEvent *se);
	int WriteScanEvents (NcFile &ncf);
	int LoadScanEvents (NcFile &ncf);

	/* Layer Information handling */
	static void free_layer_information (LayerInformation *li, Mem2d *m){ if (li) delete li; };
	void add_layer_information (LayerInformation *li);
	void add_layer_information (int lv, LayerInformation *li);
	void remove_layer_information () { L_info_new (); };
	gchar *get_layer_information (int nth);
	gchar *get_layer_information_osd (int nth);
	gchar *get_layer_information (int lv, int nth);
	void copy_layer_information (Mem2d *m, int vi=0);
	void eval_max_sizes_layer_information (int* max_dim_description, int* max_dim_format, int* max_dim_format_osd, int* max_nlyi, int* max_nlyiv);
	void start_store_layer_information (NcFile &ncf, 
					    NcVar &ncv_description, NcVar &ncv_format, NcVar &ncv_osd_format, NcVar &ncv_values, 
					    int dim_ti, int dim_v, int dim_k, 
					    int dim_description, int dim_format, int dim_format_osd);
	void store_layer_information (NcVar &ncv_description, NcVar &ncv_format, NcVar &ncv_osd_format, NcVar &ncv_values, int ti);
	void start_read_layer_information (NcFile &ncf, NcVar &ncv_description, NcVar &ncv_format, NcVar &ncv_osd_format, NcVar &ncv_values);
	void add_layer_information (NcVar &ncv_description, NcVar &ncv_format, NcVar &ncv_osd_format, NcVar &ncv_values, int ti);

private:
	void Init();
	void Mnew(int Nx, int Ny, int Nv, ZD_TYPE type);
	void Mdelete();

	void L_info_delete();
	void L_info_new(gboolean keep=FALSE);

	/* Z Data Statistics: */
	double Zmin, Zmax, Zrange, logFac;
	double ZHiLit_low, ZHiLit_hi, ZHiLit_num;

	/* Z Data Statistics */
	double *Zbins, *Zibins;
	double Zbin_width;
	int    Zbin_num;
	double Zihigh, Zimed, Zilow, Zirange;

	/* Z Data View bounds, transformation constants */
	ZVIEW_TYPE ZVmax, ZVmin, ZVrange, ZVHiLitRange, ZVHiLitBase;
	double Zcontrast, Zbright;
	double plane_mx, plane_my, plane_z0;

	/* Data handling stuff */
	ZD_TYPE  zdtyp;
	int  data_valid;
	int  Mod;

	double (Mem2d::*ZFkt)(int, int);
	ZVIEW_TYPE (Mem2d::*ZVFkt)(int &, int &);

	int t_index;        /* keep a memory of the time index */
	double frame_time[MAX_FRAME_TIMES];  /* and frame time -- max 4 frame times */

	
	/* protected: */
public:
	ZData *data;
	GSList *scan_event_list;
	char  MemElemName[MEMELEMNAMELEN+5];

	GList **layer_information_list;
	int   nlyi;
};

/* 
 * digital filter math kernel for convolution -- base class 
 *  *this MemDigiFilter (Mem2d) is the kernel itself !!!
 * Ex.:
 *  KernelMem2d->Convol(SrcDataObj, DestDataObj);
 */

class MemDigiFilter : public Mem2d{
public:
	MemDigiFilter(double Xms, double Xns, int M, int N, MemDigiFilter *adaptive_kernel=NULL, double adaptive_threashold=0.);
	gboolean Convolve(Mem2d *Src, Mem2d *Dest);
	virtual gboolean CalcKernel (){ return 0; };

        void MakeKernelNormalized ();
        void InitializeKernel (){
                if (kernel_initialized) return;
                MakeKernelNormalized ();
                kernel_initialized = TRUE;
        };
        void set_kname (const gchar *id) { kname = id; };
        
protected:
        MemDigiFilter *adaptive_kernel_test;
        double adaptive_threashold_test;
	int    m,n;
	double xms,xns;
        double scalefac;
        gboolean kernel_initialized;
        const gchar *kname;
};

#endif


	
