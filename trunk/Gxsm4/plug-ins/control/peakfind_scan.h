/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Andreas Klust <klust@fkp.uni-hannover.de>
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

#ifndef __PEAKFIND_SCAN_H
#define __PEAKFIND_SCAN_H

#include <gtk/gtk.h>
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/page.h>
#include <fcntl.h>
#include <iostream.h>

#include "gxsm/mem2d.h"
#include "gxsm/scan.h"
#include "gxsm/xsmhard.h"
#include "gxsm/glbvars.h"
#include "gxsm/plugin.h"
#include "gxsm/regress.h"
#include "include/dsp-pci32/spa/spacmd.h"

extern GxsmPlugin DSPPeakFind_pi;

#include "probe_base.h"

// special Actions
#define ACTION_SPM_PROBE      1
#define ACTION_SPA_PF_FIT     2

/* SPM Hardware auf /dev/dspdev -- xafm.out running on dsp 
 * used by AFM, STM, SARLS
 */

// Probe: Peak Finder Class
#define PF_FITMODE_FIT2ND    1
#define PF_FITMODE_FITGAUS   2
#define PF_FITMODE_FITLORENZ 3

//template <class FPIXTYP> class Focus;
class Focus;

/*
 * Probe (Force/Dist, STS, Oszi...)) Ableitung
 * this "scan" is handled seperatly by ProbeControl
 * e.g. is not managed by the "surface-channel-system"
 * ============================================================
 */

class SPA_PeakFind_p : public ProbeBase {
 public:
    SPA_PeakFind_p(int idx=0, gchar *SetName = "PeakFindCntrl");
    virtual ~SPA_PeakFind_p();

    void SetFitMode(int m){ fitmode = m; };
    int Resize(){
	xyscans->Resize(npkte_alloc=npkte, 4);
	return 0;
    };
    double skltr(double &x){ x -= (double)npkte/2.; x *= radius/(npkte/2.); return 0.; };
    double skl(double &x){ x *= radius/(npkte/2.); return 0.; };
    int DoFit();
    int Mode(void){ return fitmode; };


    void ResizeBuffer(){
      if(buffer) g_free(buffer);
      buffer = g_new(long, buffersize=npkte2d*npkte2d);
    };

    // hold x and y scans here
    Mem2d *xyscans;
    //    Focus<long> *focview;
    Focus *focview;

    // hold 2d FocusViewData
    long *buffer;
    int buffersize;
    
    // new position, shiftdistance
    double xnew,ynew,shift;
    // found Intensity
    double I0x, I0y;
    
    // probe at
    double xorg, yorg; // initial point
    double x0, y0; // new start at
    double rellim, abslim;
    double xsig, ysig;

    // with radius
    double radius;
    // and num points of
    int    npkte, npkte2d;
    // and scan with gatetime [s]
    double gate, gate2d;
    double energy;
    // use kovergenz factor
    double konvfac;
    double sleeptime;
    int enabled;
    int maxloops;
    double offsetcorrection;
    
    time_t ptime;
    int    count;
    
    int   number;
    int   index;
    gchar *name;

    int follow;
    
    // calculated stuff
    double fwhm, xfwhm, yfwhm;
    double I0, xI0, yI0;
    double xf0,  xxf0,  yxf0;

 private:
    gchar *resName;
    int    npkte_alloc;
    int    fitmode;
    
    // Fit and return poly´s maximum
    int FitFind(int xy, double &m){
	switch(fitmode){
	case PF_FITMODE_FIT2ND: return FitFind2nd(xy, m);
	case PF_FITMODE_FITGAUS: return FitFindGaus(xy, m);
	case PF_FITMODE_FITLORENZ: return FitFindLorenz(xy, m);
	}
	return 0;
    };
    int FitFind2nd(int xy, double &m);
    int FitFindGaus(int xy, double &m);
    int FitFindLorenz(int xy, double &m);
};


#include "app_databox.h"

class PeakFindScan : public Scan{
 public:
    PeakFindScan();
    ~PeakFindScan();
    
    int Save(gchar *fname);
    int Load(gchar *fname);
    
    int Stop(){ 
	StopPeakFindFlg=TRUE; 
	return 0;
    };
    int PFrun(XSM_Hardware *hw, SPA_PeakFind_p *pfp); // run Peak Find: XY-scans
    int PFrunI0(XSM_Hardware *hw, SPA_PeakFind_p *pfp); // Peak Find, Log Ixy0
    int PFhwrun(XSM_Hardware *hw, SPA_PeakFind_p *pfp); // HardCalls, used by PFrun() and PFrunI0()
    int PFget2d(XSM_Hardware *hw, SPA_PeakFind_p *pfp); // Run small and fastest 2d Scan
    long PFget0d(XSM_Hardware *hw, SPA_PeakFind_p *pfp); // Get Counts at Position
    
 private:
    int StopPeakFindFlg;
};

#endif
