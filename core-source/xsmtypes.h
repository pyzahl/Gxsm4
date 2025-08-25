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

#pragma once

#ifndef __XSMTYPES_H
#define __XSMTYPES_H

// "C" headers
#include <cstring>

// system headers
#include <unistd.h>

#include <config.h>

// Gxsm headers
#ifndef __XSMDEBUG_H
#include "xsmdebug.h"
#endif

#ifndef __UNIT_H
#include "unit.h"
#endif

#ifndef __XSMCLASSES_H
#include "xsmclasses.h"
#endif

#ifndef __GXSM_CONF_H
#include "gxsm_conf.h"
#endif

class XSM_Instrument;
class XSM_Hardware;


#define USHORT TYPE_ZD_USHORT
#define SHT   TYPE_ZD_SHORT
#define LNG   TYPE_ZD_LONG
#define CNT   TYPE_ZD_ULONG


#define R2INT(X)  (int)(rint(X))
#define R2LONG(X) (long)(rint(X))

#define G_FREE_STRDUP(X,T)        { g_free(X); X=g_strdup(T); }
#define G_NEWSIZE(X,N)            { g_free(X); X=g_new(gchar,N); }
#define SUI_GFREEANDNEWCPY(X)     { G_FREE_STRDUP(X,t); }
#define G_FREE_STRDUP_PRINTF(X,format,args...) { g_free(X); X=g_strdup_printf(format,##args); }


typedef struct { int x,y; } Point2D;

/*
 * X Resources
 * für Geräteparameter, etc.
 */

#define PIDCHMAX 4
#define DAQCHMAX 12
#define EXTCHMAX 8
#define CHLABELLEN 32

#define GRIMAX 6

#define GAIN_POSITIONS 9

#define PATHSIZE 256
#define STRSIZE  256
#define MAXPALANZ 32

// Gxsm rescources -- all set via Preferences, Name/Paths are similar to var name! See help there!
typedef struct{
	gchar *HardwareTypeCmd, *DSPDevCmd;
	gchar HardwareType[32];
	gchar DSPDev[PATHSIZE];
	gchar InstrumentType[32];
	gchar InstrumentName[32];
	gchar SPort[8];
	gchar SMmap[10];
	gchar DataPath[PATHSIZE];
	gchar ProgHome[PATHSIZE];
	gchar UserPluginPath[PATHSIZE];
	gchar GxsmPalettePath[PATHSIZE];
	gchar UserPalettePath[PATHSIZE];
	gchar ProfileXColorsPath[PATHSIZE];
	gchar *PalPathList[MAXPALANZ];
	gchar LogFilePath[PATHSIZE];
	float DigRangeIn, AnalogVMaxIn;
	float DigRangeOut, AnalogVMaxOut;
	float V[GAIN_POSITIONS];
	int   VXdefault, VYdefault, VZdefault;
	int   VX0default, VY0default, VZ0default;
	int   AnalogOffsetAdding;
	float XPiezoAV,YPiezoAV,ZPiezoAV;
	float XYZ_modulation_gains[3];
	gint  ScanOrgCenter;
	gint  ScannerZPolarity; // 1: pos, 0: neg (bool)
	float XCalibVA,YCalibVA;
	float Sensitivity;
	float EnergyCalibVeV;
	float ThetaChGunInt;
	float ThetaChGunExt;
	float SampleLayerDist;
	float SampleUnitLen;
	float BiasGain;
	float BiasOffset;
	float nAmpere2Volt;
        float current_gain_modifier;
	float nNewton2Volt;
	float dHertz2Volt;
	int   HiLoDelta;
	float SmartHistEpsilon;
	// Adjustment: Limits, Step (Min,Max,Step,Page)
	float ProfileLogLimit;
	gchar pidsrc[PIDCHMAX][CHLABELLEN];
	gchar pidsrcZunit[PIDCHMAX][8];
	gchar pidsrcZtype[PIDCHMAX][8];
	gchar pidsrcZlabel[PIDCHMAX][CHLABELLEN];
	double pidsrcZd2u[PIDCHMAX];
	guint64 pidsrc_msk[PIDCHMAX];
	int  pidchno[PIDCHMAX]; 
	int  piddefault;
	gchar daqsrc[DAQCHMAX][CHLABELLEN];
	int  daqchno[DAQCHMAX]; 
	gchar daqZunit[DAQCHMAX][8];
	gchar daqZtype[DAQCHMAX][8];
	gchar daqZlabel[DAQCHMAX][CHLABELLEN];
	double daqZd2u[DAQCHMAX];
	guint64 daq_msk[DAQCHMAX];
	int  daqdefault;
	gchar extchno[EXTCHMAX];
	//
	gchar *UnitCmd;
	gchar Unit[8];
	gchar FileNameConvention[12]; // digit, alpha
	gchar SliderControlType[12];     // omicron, sicaf
	gchar SPALEEDCrtl[12];        // unused
	gchar RemoteFifo[PATHSIZE];
	gchar RemoteFifoOut[PATHSIZE];
	gchar PyremoteFile[PATHSIZE];
	gchar Palette[PATHSIZE];
	gchar griplottitle[STRSIZE];
	gchar gricmd1d[GRIMAX][PATHSIZE];
	gchar gricmd2d[GRIMAX][PATHSIZE];
	gchar XSM_Version[STRSIZE];
	float ProfileFrameColor[4];
	float ProfileCanvasColor[4];
	gchar ProfileTicFont[STRSIZE];
	float ProfileTicColor[4];
	float ProfileGridColor[4];
	gchar ProfileLabelFont[STRSIZE];
	float ProfileLabelColor[4];
	gchar ProfileTitleFont[STRSIZE];
	float ProfileTitleColor[4];
	gint  ProfileLineWidth;
	gchar AutosaveUnit[12]; 	// storing Autosaveunit = percent, seconds, lines
	gchar AutosaveOverwritemode[8];
	gint  LineProfileOrgMode;
        gint  world_size_nxy;
	gint  geomsave;
	gint  datnullok;
	gint  menutooltips;
	gint  disableplugins;
	gint  force_config;
	gint  AutosaveValue;		//storing AutosaveValue for repetition of autosave
	gint  gui_layerfields;
	float HandleActBgColor[4];
	float HandleInActBgColor[4];
	gchar HandleType[32];
	gint  HandleLineWidth;
	float HandleSize;
	gint  ObjectLineWidth;
	gchar ObjectLabFont[STRSIZE];
	float ObjectLabColor[4];
	float ObjectElmColor[4];
	float ObjectIntColor[4];
	gchar OSDFont[STRSIZE];
	float OSDColor[4];
	gint  LoadDelay;
        float LoadCorrectXYZ[4];
	gchar RedLineHistoryColors[1024];
	gchar BlueLineHistoryColors[1024];
	gint  RedLineWidth;
        gboolean gxsm4_ready;
        gint  gui_pcs_force;
} XSMRESOURCES;

#define IS_FILENAME_CONVENTION_DIGIT  (!strncasecmp(xsmres.FileNameConvention,"digit",5))
#define IS_FILENAME_CONVENTION_ALPHA  (!strncasecmp(xsmres.FileNameConvention,"alpha",5))
#define IS_FILENAME_CONVENTION_DTIME  (!strncasecmp(xsmres.FileNameConvention,"date-time",9))

#define IS_FILE_TYPE_NC               (!strncasecmp(xsmres.FileType,"nc",2))
#define IS_FILE_TYPE_DAT              (!strncasecmp(xsmres.FileType,"dat",3))

#define IS_MOVER_CTRL                 (!strncasecmp(xsmres.SliderControlType, "mover",5))
#define IS_SLIDER_CTRL                (!strncasecmp(xsmres.SliderControlType, "slider",6))

#define IS_SPALEED_CTRL               (!strncasecmp(xsmres.InstrumentType, "SPALEED",7))
#define IS_SPM_CTRL                   (!strncasecmp(xsmres.InstrumentType, "STM",3) || !strncasecmp(xsmres.InstrumentType, "AFM",3) || !strncasecmp(xsmres.InstrumentType, "SNOM",4) || !strncasecmp(xsmres.InstrumentType, "SARLS",5))
#define IS_AFM_CTRL                   (!strncasecmp(xsmres.InstrumentType, "AFM",3))
#define IS_NOCARD                     (!strncasecmp(xsmres.HardwareType, "no",2))

typedef struct{ 
  const gchar *alias; const gchar *s; const gchar *pss; double fac; const gchar *prec1; const gchar *prec2; 
} UnitsTable;


/*
 * Class: Display_Param
 * Display Parameter, set by user
 */

class Display_Param{
public:
	Display_Param(){
                vframe =  vlayer = 0;
                contrast=1.; bright=0.; z_high=z_low=0.; vrange_z=10.; voffset_z=0.;
                ViewFlg=1;
                px_shift_xy[0]=px_shift_xy[1]=0.; px_shift_xy[2]=5e-5;
                use_high_low = 0;
        };
	~Display_Param(){};

	void copy(Display_Param &src){
		contrast = src.contrast;
		bright   = src.bright;
		z_high   = src.z_high;
		z_low    = src.z_low;
                use_high_low = src.use_high_low;
		vrange_z = src.vrange_z;
		voffset_z= src.voffset_z;
		vframe   = src.vframe;
		vlayer   = src.vlayer;
		ViewFlg  = src.ViewFlg;
                px_shift_xy[0] = src.px_shift_xy[0];
                px_shift_xy[1] = src.px_shift_xy[1];
                px_shift_xy[2] = src.px_shift_xy[2];
	};

	double ContrastBrightFkt(double x){
		return x*contrast+bright;
	};
	double ContrastBright_from_HiLow(double range = 64.){
		contrast = (z_high-z_low)/range;
		bright   = range/2. - z_low;
		return 0.;
	};

	double contrast, bright;            /* for Scaling by Contrast/Bright */
	double vrange_z;                    /* for Scaling by Range and Offset from "Z-Center" */
	double voffset_z;                   /* for Scaling by Range and Offset from "Z-Center" */
	double z_high, z_low;               /* for Scaling by high and low */
        gint   use_high_low;                /* 0: undetermined, 1: true, -1: false */
	int    vframe;                      /* Frame (in Time dimension) to view */
	int    vlayer;                      /* Layer (in Value dimension) to view */

	int    ViewFlg;                     /* View Mode */
        double px_shift_xy[3];              /* experimental pixel shift x,y, tau / sec */
};


/*
 * Struct: Scan_Param
 * Description: contains generic XSM Scan Parameters
 * xsm core internal base unit for all space dimensions is Angstroems
 */
typedef struct{

// scan data dimensions (quantization)
// NOTE: the dimensions below are obsoleted, plase use meme2d->GetNx (), ... please functions in new code
	int    ntimes;             /* Numer of Frames -- for future use, must be 1 so far */
	int    nvalues;            /* Numer of Values -- for varing one parameter scans */
	int    nx ,ny;             /* Number of data points in X and Y */

// scan data real world dimensions (scale)
	double rx, ry, rz;         /* Scan Ranges [Ang] */
	double dx, dy, dz, dl;     /* Step Width  [Ang] */
	double alpha;              /* Scan Angle [deg] */
	double x0,y0;              /* Offset [Ang] */
	double sx, sy;             /* Scan (Tip) Position withing scan coordinate system used for local probing, etc. -- normally 0,0 [Ang] */

// generic and often used parameters, handy to have in the main window -- mirrored to main by HwI PlugIns, not essential --
// IMPORTANT: DO NOT USE FOR ANY HARDWARE EXCEPT FOR WRITING/SHOWING TO THE USER!
	double SPA_OrgX, SPA_OrgY; /* secondary (used SPA-LEED only) Offset relativ to (0,0) [V] */
	double Energy;             /* Energy (if applicable) [eV], set to negative if not relevant */
	double GateTime;           /* GateTime (if applicable) [ms], set to negative if not relevant */
	double Bias;               /* some Bias (if applicable) [V], set to < -9.999e10 if not relevant */
        double Motor;              /* generic "Motor" (auxillary) Voltage */
	double Current;            /* some Curren (if applicable) [nA], set to < -9.999e10 if not relevant */
	double SetPoint;           /* some AFM SetPoint (if applicable) [V], set to < -9.999e10 if not relevant */
	double ZSetPoint;          /* some Z-SetPoint (if applicable) [A] */
        double pllref;             /* pllreference freq. in Hz as set */
        
// real time window of scan frame
	time_t tStart, tEnd;       /* Scan Start and end time, [UNIX time] */
	int    iyEnd, xdir, ydir;  /* last line valid/stopped if while scan, else last line, x,y scan direction -- managed by spmconacontrol */
	double pixeltime;          /* time per pixel - set by HwI */
}Scan_Param;

// ** NOW 100% OBSOLETE AND REMOVED FROM CORE -- must now be handled independently by all HwI PlugIns. **
// **
// ** /* Parameter für DSP Programm (Regler etc. ...) */
// ** /* 
// **    *** THIS "DSP_Param" struct will become obsolete ASAP --
// **        i.e. all HwIs and other PIs are independent of it!! 
// **    *** 
// ** */
// ** typedef struct DSP_Param{
//
// ... all removed from GXSM core ... -- if you need the old struct, look at code before CVS tag "Expr2DSP-OldCore-Cleanup":
//
//   Repository revision: 1.17    /cvsroot/gxsm/Gxsm-2.0/src/xsmtypes.h,v
//   Sticky Tag:          Expr2DSP-OldCore-Cleanup (branch: 1.17.4)
//
// ** };


/*
 * User Info Fields
 * totally dynamically allocated - no length limit
 */
class Scan_UserInfo{
public:
	Scan_UserInfo(){
		dateofscan = g_strdup("date");
		user       = g_strdup("user");
		host       = g_strdup("host");
		comment    = g_strdup("empty");
		title      = g_strdup("title not set");
		name       = g_strdup("nobody");
		basename   = g_strdup("specify_one");
		originalname = g_strdup("unknown (new)");
		type       = g_strdup("type not set");
		MakeUIdefaults();
	};
	~Scan_UserInfo(){
#if 0
		XsmRescourceManager xrm("UI_remember");
		xrm.Put("basename", basename);
#endif
		g_free(dateofscan);
		g_free(user);
		g_free(host);
		g_free(comment);
		g_free(title);
		g_free(name);
		g_free(basename);
		g_free(originalname);
		g_free(type);
	};

	void MakeUIdefaults(){
		char hn[256];
		time_t t;
		time(&t);
		SetDateOfScan ("-- no scan --");
		if(getlogin()){
			G_FREE_STRDUP(user, getlogin());
		}else{
			G_FREE_STRDUP(user, "Nobody");
		}
		gethostname(hn, 256);
		G_FREE_STRDUP(host, hn);
#if 0
		XsmRescourceManager xrm("UI_remember");
		gchar *tmp;
		xrm.Get("basename", &tmp, user);
		G_FREE_STRDUP(basename, tmp);
#endif
		g_free(comment); comment=g_strconcat(user, "@", hn, "\nSession Date: ", ctime(&t), NULL);
		G_FREE_STRDUP(type, "not set");
	};

	void SetDateOfScan(const gchar *t){ SUI_GFREEANDNEWCPY(dateofscan); g_strdelimit (dateofscan, "\n", ' '); };
	void SetDateOfScanNow(){ 
		time_t t;
		time(&t);
		SetDateOfScan (ctime(&t));
	};
	void SetUser(const gchar *t){ SUI_GFREEANDNEWCPY(user); };
	void SetHost(const gchar *t){ SUI_GFREEANDNEWCPY(host); };
	void SetComment(const gchar *t){ SUI_GFREEANDNEWCPY(comment); };
	void SetTitle(const gchar *t){ SUI_GFREEANDNEWCPY(title); };
	void SetName(const gchar *t){ SUI_GFREEANDNEWCPY(name); };
	void SetBaseName(const gchar *t){ 
		SUI_GFREEANDNEWCPY(basename); 
#if 0
		XsmRescourceManager xrm("UI_remember");
		xrm.Put("basename", basename);
#endif
	};
	void SetOriginalName(const gchar *t){ SUI_GFREEANDNEWCPY(originalname); };
	void SetType(const gchar *t){ SUI_GFREEANDNEWCPY(type); };

	void copy(Scan_UserInfo &ui){
		SetDateOfScan(ui.dateofscan);
		SetUser(ui.user);
		SetHost(ui.host);
		SetComment(ui.comment);
		SetTitle(ui.title);
		SetName(ui.name);
		SetBaseName(ui.basename);
		SetOriginalName(ui.originalname);
		SetType(ui.type);
	};

	gchar   *dateofscan;    /* time of measurement */
	gchar   *user;          /* Username */
	gchar   *host;          /* Hostname */
	gchar   *comment;       /* Kommentar */
	gchar   *title;         /* Scan/Profile Title */
	gchar   *name;          /* filename */
	gchar   *basename;      /* filename of the original data */
	gchar   *originalname;  /* ur-name */
	gchar   *type;          /* Scantype, DataSource-ScanDir ("X+ Topo") coding */
};

/* typedef struct{ */
extern int scandatacount;

typedef enum { SCAN_ORG_MIDDLETOP, SCAN_ORG_CENTER } SCAN_ORIGIN;
typedef enum { SCAN_MODE_SINGLE_DSPSET, SCAN_MODE_DUAL_DSPSET } SCAN_MODE;
typedef enum { SCAN_TYPE_NORMAL, SCAN_TYPE_MULTILAYER } SCAN_TYPE;
typedef enum { SCAN_REPEAT_MODE_UNIDIR, SCAN_REPEAT_MODE_BIDIR } SCAN_REPEAT_MODE;


class SCAN_DATA{
	int cnt;
public:
	SCAN_DATA();
	virtual ~SCAN_DATA();
  
	SCAN_ORIGIN orgmode;
	SCAN_MODE scan_mode;
	SCAN_REPEAT_MODE scan_repeat_mode;
	SCAN_TYPE scan_type;
  
	int UnitsAlloc;
	UnitObj *Xunit, *Yunit, *Zunit, *Vunit;
	UnitObj *Xdt_unit, *Ydt_unit, *Zdt_unit;
	UnitObj *CurrentUnit, *VoltUnit, *TimeUnit, *TimeUnitms;
	UnitObj *CPSUnit, *EnergyUnit;

	void UpdateUnits(){
		//display.cnttime = s.GateTime;
		//    CPSUnit->Change(display.cnttime); ... now dz is used !
	};

	void CpUnits(SCAN_DATA &src);

	void SetXUnit(UnitObj *u);
	void SetYUnit(UnitObj *u);
	void SetVUnit(UnitObj *u);
	void SetZUnit(UnitObj *u);
	void SetTimeUnit(UnitObj *u);

	/* Update Functions */
	void GetScan_Param(SCAN_DATA &src){ 
		scan_mode = src.scan_mode;
		scan_repeat_mode = src.scan_repeat_mode;
		memcpy((void*)&s, (void*)&src.s, sizeof(Scan_Param)); 
	};

	void GetUser_Info(SCAN_DATA &src){
		ui.copy(src.ui);
	};

	void GetDisplay_Param(SCAN_DATA &src){ 
		display.copy(src.display); 
	};

	void copy (SCAN_DATA &sd){
		CpUnits (sd);
		memcpy (&s, &sd.s, sizeof (Scan_Param));
		ui.copy (sd.ui);
		//display.copy (sd.display);
	};

	double get_x_left_absolute (){ return s.x0-s.rx/2; };
	double get_x_right_absolute (){ return s.x0+s.rx/2; };
	double get_y_top_absolute (){ return s.y0+s.ry/2; };
	double get_y_bottom_absolute (){ return s.y0-s.ry/2; };
	
	/* Save / Retrive Values from Rescource */
	void SaveValues(gchar *SetName=NULL);
	void LoadValues(XSM_Instrument *Inst, XSM_Hardware *hardware, gchar *SetName=NULL);

	/* Data */
	Scan_Param      s;              /* Scan Parameter */
	Scan_UserInfo   ui;             /* User Informations */
	Display_Param   display;        /* Display */
};



#endif



