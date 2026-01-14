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

#ifndef __XSMHARD_H
#define __XSMHARD_H

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <asm/page.h>
#include <fcntl.h>

#include "mem2d.h"
#include "xsmtypes.h"
#include "xsm_limits.h"

/*
 * ============================================================
 * Allgemeines Basis Objekt zur Bedienung der XSM-HARDWARE 
 * ============================================================
 */

class XSM_Hardware{
 public:
	XSM_Hardware();
	virtual ~XSM_Hardware();

	virtual int update_gxsm_configurations (){ return 0; }; /* called after GUI build complete */
	virtual void hwi_init_overrides(){ }; /* if any xsmres... preferences need to be re-read or overriden, called after init and preferences changed */
        
	/* query Hardware description and features */
	const gchar* Info (int data){ return get_info(); }; /* Info on Hardware Class */
	virtual const gchar* GetStatusInfo () { return AddStatusString; }; /* Status */

	/* Hardware Limits */
	virtual long GetMaxPointsPerLine (){ return 1L<<16; };
	virtual long GetMaxLines (){ return 1L<<16; };

	/* Hardware realtime monitoring -- all optional */
	/* default properties are
	 * "X" -> current realtime tip position in X, inclusive rotation and offset
	 * "Y" -> current realtime tip position in Y, inclusive rotation and offset
	 * "Z" -> current realtime tip position in Z
	 * "xy" -> X and Y
	 * "zxy" -> Z, X, Y [mk2/3]
	 * "o" -> Z, X, Y-Offset [mk2/3]
	 * "f" -> feedback watch: f0, I, Irms as read on PanView [mk2/3]
	 * "s" -> status bits [FB,SC,VP,MV,(PAC)], [DSP load], [DSP load peak]  [mk2/3]
	 * "i" -> GPIO watch -- speudo real time, may be chached by GXSM: out, in, dir  [mk2/3]
	 * "U" -> current bias
	 */
	virtual gint RTQuery (const gchar *property, double &val) { return FALSE; };
	virtual gint RTQuery (const gchar *property, double &val1, double &val2) { return FALSE; };
	virtual gint RTQuery (const gchar *property, double &val1, double &val2, double &val3);
	virtual gint RTQuery (const gchar *property, gchar **val) { return FALSE; };
	virtual gint RTQuery (const gchar *property, int n, gfloat *data) { return FALSE; };
	virtual gint RTQuery () { return y_current; }; // actual progress on scan -- y-index mirror from FIFO read, etc. -- returns -1 if not available

	/* high level calls for instrtument condition checks */
	virtual gint RTQuery_clear_to_start_scan (){ return 1; };
	virtual gint RTQuery_clear_to_start_probe (){ return 1; };
	virtual gint RTQuery_clear_to_move_tip (){ return 1; };
	
	/* Methods for future use */
	virtual gchar* InqeueryUserParamId (gint n) { return NULL; };
	virtual gchar* InqeueryUserParamDescription (gint n) { return NULL; };
	virtual gchar* InqeueryUserParamUnit (gint n) { return NULL; };
	virtual double GetUserParam (gint n, const gchar *id=NULL) { return 0.; };
	virtual gint   SetUserParam (gint n, const gchar *id=NULL, double value=0.) { return FALSE; };

	virtual double GetScanrate () { return 0.; }; // query current set scan rate in s/pixel
	
	/* 
	 * Generic scan parameters and scanhead controls
	 * All integer values are in DAC units
	 */
	/* tell hardware about GXSM set XYZ-Scan and -Offset gains */
	virtual void UpdateScanGainMirror () {};

	/* set scan offset, HwI should move detector to this position, absolute coordinates, not rotated */
	virtual gboolean SetOffset(double x, double y);

	/* set scan step sizes, if dy is 0 or not given dy=dx is assumed */
	virtual void SetDxDy(double dx, double dy=0.);
	virtual void GetDxDy(double &dx, double &dy) { dx = Dx, dy = Dy; };

	/* set scan dimensions, if ny not given or 0 ny=nx is assumed */
	virtual void SetNxNy(long nx, long ny=0L);
	virtual void GetNxNy(long &nx, long &ny) { nx = Nx, ny = Ny; };

	/* set scan angle in degree, this affects the scan-coordinate system */
	virtual void SetAlpha(double alpha);
	virtual int RotateStepwise(int exec=1) { return 0; };

	/* perform a moveto aktion within the scan-coordinate system (rotated by alpha) */
	virtual gboolean MovetoXY (double x, double y);

	/* set/get scan direction, +1: Top-Down, -1: Bottom-Up */
	virtual int ScanDirection (int dir);

	/* prepare to Start Scan */
	virtual void StartScan2D(){;};

	/* Scan a Line, negative yindex: scan initialization phase in progress... */
	virtual gboolean ScanLineM(int yindex, int xdir, int muxmode, Mem2d *Mob[MAX_SRCS_CHANNELS], int ixy_sub[4]);

	/* End,Pause,Resume,Kill/Cancel Scan */
	virtual gboolean EndScan2D(){ return FALSE; };
	virtual void PauseScan2D(){;};
	virtual void ResumeScan2D(){;};
	virtual void KillScan2D(){;};

	virtual void EventCheckOn(){;};
	virtual void EventCheckOff(){;};

	void SetFastScan(int f=0) { fast_scan=f; };
	virtual int IsFastScan() { return fast_scan; };
	void SetSuspendWatches(int f=0) { suspend_watches=f; };
	virtual int IsSuspendWatches() { return suspend_watches; };
	virtual const gchar* get_info();

	void SetScanMode(int ssm=MEM_SET){ scanmode=ssm; };
	int  FreeOldData(){ return (scanmode == MEM_SET); };
	void Transform(double *x, double *y);
	void invTransform(double *x, double *y);

	void add_user_event_now (const gchar *message_id, const gchar *info, double value[2], gint addflag=FALSE);
	void add_user_event_now (const gchar *message_id, const gchar *info, double v1, double v2, gint addflag=FALSE){
		double v[2] = {v1, v2};
		add_user_event_now (message_id, info, v, addflag); 
	};
	void add_user_event_now (const gchar *message, double v1, double v2, gint addflag=FALSE) {
		double v[2] = {v1, v2};
		const gchar *m_id = message;
		add_user_event_now (m_id, "N/A", v, addflag); 
	};
	
	gchar *AddStatusString;

 private:
	double Simulate (int muxmode);

	double sim_xyzS[3];
	double sim_xyz0[3];
	gint   sim_mode;

 public:
	long   XAnz, Nx, Ny;
	double rx, ry, Dx, Dy;
 protected:
	double Alpha;
	double rotmxx,rotmxy,rotmyx,rotmyy,rotoffx,rotoffy;

	double irotmxx,irotmxy,irotmyx,irotmyy,irotoffx,irotoffy;
  
	int    fast_scan; // X scale is sinodial
	int    suspend_watches;
	int    scanmode;
	int    scan_direction;

	int    y_current;
};


#endif

