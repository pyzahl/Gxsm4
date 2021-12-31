/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: gmeyer_im_export.C
 * ========================================
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

/* Please do not change the Begin/End lines of this comment section!
 * this is a LaTeX style section used for auto generation of the PlugIn Manual 
 * Chapter. Add a complete PlugIn documentation inbetween the Begin/End marks!
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Import/Export of G.Meyer STM/AFM Dat files
% PlugInName: gmeyer_im_export
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: File/Import/GMeyer Dat

% PlugInDescription
\label{plugins:gmeyer_im_export}
The \GxsmEmph{gmeyer\_im\_export} plug-in supports reading and writing of
Dat files used by the G.Meyer STM/AFM software.

% PlugInUsage
The plug-in is called by \GxsmMenu{File/Import/GME Dat}.

% OptPlugInKnownBugs
Not yet all very well tested.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

/* GME File Header
 * --------------------------------------------------
[Parameter]
Titel / Titel=DEFAULT
Delta X / Delta X [Dac]=16
Delta Y / Delta Y [Dac]=16
Num.X / Num.X=256
Num.Y / Num.Y=256
Delay X+ / Delay X+=350
Delay X- / Delay X-=300
Delay Y / Delay Y=400
D-DeltaX / D-DeltaX=4
GainX / GainX=10
GainY / GainY=10
GainZ / GainZ=10
Rotation / Rotation=0
BiasVoltage / BiasVolt.[mV]=-3200
Gainpreamp / GainPre 10^=6
Chan(1,2,4) / Chan(1,2,4)=1
PlanDx / PlanDx=0.0000
PlanDy / PlanDy=0.0000
Scanrotoffx / OffsetX=0.0
Scanrotoffy / OffsetY=0.0
MVolt_1 / MVolt_1=-20000
MVolt_2 / MVolt_2=-20000
MVolt_3 / MVolt_3=0
MVolt_4 / MVolt_4=0
MVolt_5 / MVolt_5=0
MVolt_6 / MVolt_6=0
RepeatRotinc / RepeatRotinc=0
RptBVoltinc / RptBVoltinc=0
Repeatinterval / Repeatinterval=0
Repeatcounter / Repeatcounter=0
RepeatXoffset / RepeatXoffset=0
RepeatYoffset / RepeatYoffset=0
Scantype / Scantype=1
Scanmode / Scanmode=1
Scancoarse / Scancoarse=1
CHMode / CHMode=0
Channels / Channels=1
Preamptype / Preamptype=4
Latmanmode=0
LatmResist=1.000E+11
LatmanVolt=1000
Latmanlgi=1
Latmangain=8
Latmanddx=1
Latmandelay=100
Latm0Delay=2
Latmanccdz=0
Latmanextension=1.000
TipForm_Volt=0
TipForm_Z=0
Tip_Delay=100
Tip_Latddx=1
Tip_LatDelay=100
Vertmangain=8
Vertmandelay=10
Vpoint0.t=0
Vpoint1.t=400
Vpoint2.t=600
Vpoint3.t=1024
Vpoint0.V=0
Vpoint1.V=-1000
Vpoint2.V=-1000
Vpoint3.V=0
Zpoint0.t=0
Zpoint1.t=400
Zpoint2.t=600
Zpoint3.t=1024
Zpoint0.z=0
Zpoint1.z=-100
Zpoint2.z=-100
Zpoint3.z=0
Zoffset=0
Zdrift=0
VertSpecBack=0
VertSpecAvrgnr=1
VertRepeatCounter=1
Imageframe=1
Imagegrayfactor=0.999
Zoom=2
OrgPlanX=0.00000
OrgPlanY=0.00000
OrgPlanOff=0.00000
Dacto[A]xy=0.03052
Dacto[A]z=0.00610
Planoff=0
Planx=0
Plany=0
Plany2=0
Planavrgnr=17
Planoffset2=0
Plano2start=0
Imagebackoffset=0
Frameimageoffset=1000
YLineOFF=1
YLineFAC=10
DigZoomX=1.00000
DigZoomZ=1.00000
FBLogIset= 500.100
FBRC=  0.000200
FBLingain=-60000.0
FBVoltRC=  0.000526
FBVoltGain=    6.0
CurrentRC= 0.010
FBLog=1
Imaxcurrent=100000.0
Imaxdelay=   1000
ImaxZret=  0.000
SpecAvrgnr=1
SpecFreq=10000.000
SpecChan=1
FFTPoints=1024
LockinFreq=50
LockinAmpl=200
LockinPhase=0
LockinPhase2=0
LockinRC= 0.00010
LockinMode=1
DSP_Clock=50000
SRS_Frequency=1000.000
SRS_ModVoltage=  1.0000
SRS_InpGain[V]=1.0E+00
SRS_InpTimeC[s]=1.0E+00
UserPreampCode=5:000000/6:000000/7:000000/8:000000/
Upinc=50
Upcount=200
Downinc=50
Downcount=200
Rotinc=50
Rotincquad=0
Rotcount=200
XYBurst=100
RotBurst=100
Aprologiset=  2.000
Aprolimit=10000
Aprodelay=300
Aprorc= 0.00020
Autolevel=1
RPTcount=0
RPTIncVolt=0
RPTIncdz=0
SDOrg%=1
SdStatDiff%=0
SDOffset=0
SDBbeta=0
CorrCheckSize=8
CorrImageSize=8
IslandLevel=0.50
Scall-Number=
VertSwitchLevel=0
ADC_0=11
ADC_1=-1
ADC_2=2
ADC_3=1
VertMaxI=11000
VertTreshimin=-20000
VertTreshimax=20000
RptCHMZinc=0
CHModeZoffset=0
CHModegainpreamp=6
ScandVinc=0
ScandIinc=0
TrackingDx=1
AltSpecCalc=0
VStart=-1000
Vend=1000
ZStart=0
ZMid=0
ZEnd=0
VoltReso=1
SpecTime=1000
MVolt_7=0
MVolt_8=0
MVolt_9=0
MVolt_10=0
MVolt_11=0
MVolt_12=0
MVolt_13=0
MVolt_14=0
MVolt_15=0
MVolt_16=0
MVolt_17=0
MVolt_18=0
MVolt_19=0
MVolt_20=0
VFBVLimit=5000
Preampoffset=-335
USBBufferFill=0
FBAFMSetpoint=-1654
FBAFMrc=0.0005260
FBAFMgain=6
THe[K]= 
TSTM[K]= 
TManip[K]= 
PLoadLock= 
PPraep= 
TSample[C]= 
TEvap[C]= 
= 
Z-Res. [A]: +/- =7.809
Length x[A]=1250.0381
Length y[A]=1250.0381
Biasvolt[mV]=-3200
Current[A]=5.0E-07
Sec/line:=7.168
Sec/Image:=3670.016
ActGainXYZ=30 30 30
Channels=1
T-STM:=0.00000
= 
HP_Ch1=0.00000
HP_Ch2=0.00000
HP_Ch3=0.00000
HP_Ch4=0.00000
= 
ZPiezoconst= 20.00
Xpiezoconst=100.00
YPiezoconst=100.00
= 
T_ADC2[K]=  0.000
T_ADC3[K]=  0.000
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
= 
memo:0=CU(211) T=20K                       
memo:1=  
memo:2=g101010 i8 
memo:3=0.585V

... junk ...
DATA 2bytes junk
	start of short field(s)

 * End of GME File Header
 * --------------------------------------------------
 */


#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"
#include "core-source/dataio.h"
#include "core-source/action_id.h"

#include <zlib.h>

using namespace std;

#define IMGMAXCOLORS 64

// Plugin Prototypes
static void gmeyer_im_export_init (void);
static void gmeyer_im_export_query (void);
static void gmeyer_im_export_about (void);
static void gmeyer_im_export_configure (void);
static void gmeyer_im_export_cleanup (void);

static void gmeyer_im_export_filecheck_load_callback (gpointer data );
static void gmeyer_im_export_filecheck_save_callback (gpointer data );

static void gmeyer_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void gmeyer_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// Fill in the GxsmPlugin Description here
GxsmPlugin gmeyer_im_export_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  "Gmeyer_Im_Export",
  NULL,                   // PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  NULL,
  "Percy Zahl",
  "file-import-section,file-export-section",
  N_("GME Dat,GME Dat"),
  N_("Import G.Meyer STMAFM data file,Export G.Meyer STMAFM data file"),
  N_("G.Meyer STMAFM software data file im/export."),
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  gmeyer_im_export_init,
  gmeyer_im_export_query,
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  gmeyer_im_export_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  gmeyer_im_export_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL,
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  gmeyer_im_export_import_callback, // direct menu entry callback1 or NULL
  gmeyer_im_export_export_callback, // direct menu entry callback2 or NULL

  gmeyer_im_export_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm G.Meyer STMAFM Data File Import/Export Plugin\n\n"
                                   " "
	);

static const char *file_mask = "*.dat";

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  gmeyer_im_export_pi.description = g_strdup_printf(N_("Gxsm gmeyer_im_export plugin %s"), VERSION);
  return &gmeyer_im_export_pi; 
}

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/GME Dat"
// Export Menupath is "File/Export/GME Dat"
// ----------------------------------------------------------------------
// !!!! make sure the "gmeyer_im_export_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!

static void gmeyer_im_export_query(void)
{
// register this plugins filecheck functions with Gxsm now!
// This allows Gxsm to check files from DnD, open, 
// and cmdline sources against all known formats automatically - no explicit im/export is necessary.
	gmeyer_im_export_pi.app->ConnectPluginToLoadFileEvent (gmeyer_im_export_filecheck_load_callback);
	gmeyer_im_export_pi.app->ConnectPluginToSaveFileEvent (gmeyer_im_export_filecheck_save_callback);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void gmeyer_im_export_init(void)
{
	PI_DEBUG (DBG_L2, "gmeyer_im_export Plugin Init");
}

// about-Function
static void gmeyer_im_export_about(void)
{
	const gchar *authors[] = { gmeyer_im_export_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  gmeyer_im_export_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void gmeyer_im_export_configure(void)
{
	if(gmeyer_im_export_pi.app)
		gmeyer_im_export_pi.app->message("gmeyer_im_export Plugin Configuration");
}

// cleanup-Function, make sure the Menustrings are matching those above!!!
static void gmeyer_im_export_cleanup(void)
{
#if 0
	gchar **path  = g_strsplit (gmeyer_im_export_pi.menupath, ",", 2);
	gchar **entry = g_strsplit (gmeyer_im_export_pi.menuentry, ",", 2);

	gchar *tmp = g_strconcat (path[0], entry[0], NULL);
	gnome_app_remove_menus (GNOME_APP (gmeyer_im_export_pi.app->getApp()), tmp, 1);
	g_free (tmp);

	tmp = g_strconcat (path[1], entry[1], NULL);
	gnome_app_remove_menus (GNOME_APP (gmeyer_im_export_pi.app->getApp()), tmp, 1);
	g_free (tmp);

	g_strfreev (path);
	g_strfreev (entry);
#endif
	
	PI_DEBUG (DBG_L2, "Plugin Cleanup done.");
}

// make a new derivate of the base class "Dataio"
class gmeyer_ImExportFile : public Dataio{
public:
	gmeyer_ImExportFile(Scan *s, const char *n) : Dataio(s,n){ };
	virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
	virtual FIO_STATUS Write();
private:
	FIO_STATUS import(const char *fname);
};

// Import GME SPM file
FIO_STATUS gmeyer_ImExportFile::Read(xsm::open_mode mode){
	FIO_STATUS ret;
	gchar *fname=NULL;

	fname = (gchar*)name;

	// name should have at least 4 chars: ".ext"
	if (fname == NULL || strlen(fname) < 4)
		return  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
 
	// check for file exists and is OK !
	// else open File Dlg
	ifstream f;
	f.open(fname, ios::in);
	if(!f.good()){
		PI_DEBUG (DBG_L2, "File Fehler");
		return status=FIO_OPEN_ERR;
	}
	f.close();

	// Check all known File Types:
	
	// is this a GME Dat file?
	if ((ret=import (fname)) !=  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
		return ret;

	return  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
}

FIO_STATUS gmeyer_ImExportFile::import(const char *fname){

	// Am I resposible for that file, is it a "Gme-Dat" format ?
	ifstream f;
	gchar line[0x4000];
	GString *FileList=NULL;
	int datamode = 16;

	f.open(fname, ios::in);
	if (!f.good())
	        return status=FIO_OPEN_ERR;

	f.getline(line, 0x4000);

	if (strncmp (line, "[Parameter]", 11) != 0){
		// check for 32 bit data version
		if (strncmp (line, "[Paramco32]", 11) != 0){
			f.close ();
			return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
		}
		datamode = 32;
	}
	
	time_t t; // Scan - Startzeit eintragen 
	time(&t);
	gchar *tmp = g_strconcat ((ctime(&t)), " (Imported)", NULL); scan->data.ui.SetDateOfScan (tmp); g_free (tmp);
	scan->data.ui.SetName (fname);
	scan->data.ui.SetOriginalName (fname);
	scan->data.ui.SetType ("G.Meyer STMAFM"); 


	// this is mandatory.
	scan->data.s.ntimes  = 1;
	scan->data.s.nvalues = 1;
  
  
	// put some usefull values in the ui structure
	if(getlogin()){
		scan->data.ui.SetUser (getlogin());
	}
	else{
		scan->data.ui.SetUser ("unkonwn user");
	}

	// initialize scan structure -- this is a minimum example
	scan->data.s.nx = 1;
	scan->data.s.ny = 1;
	scan->data.s.dx = 1;
	scan->data.s.dy = 1;
	scan->data.s.dz = -1;
	scan->data.s.rx = scan->data.s.nx;
	scan->data.s.ry = scan->data.s.ny;
	scan->data.s.x0 = 0;
	scan->data.s.y0 = 0;
	scan->data.s.alpha = 0;

	// be nice and reset this to some defined state
	scan->data.display.z_high       = 1e3;
	scan->data.display.z_low        = 1.;

	// set the default view parameters
	scan->data.display.bright = 32.;
	scan->data.display.contrast = 1.0;

	// FYI: (PZ)
	//  scan->data.display.vrange_z  = ; // View Range Z in base ZUnits
	//  scan->data.display.voffset_z = 0; // View Offset Z in base ZUnits
	//  scan->AutoDisplay([...]); // may be used too...
  

	FileList = g_string_new ("Imported by Gxsm from G.Meyer STMAFM Dat file.\n");
	g_string_append_printf (FileList, "Original Filename: %s\n", fname);
	g_string_append (FileList, "Original GME File Header follows:\n");
	
	int dxi=0, dyi=0;
	int chan=1, channels=1;
	double gx=1., gy=1., gz=1.;
	double pzAV_x=1., pzAV_y=1., pzAV_z=1.;

	int stop=0;
	gfloat MVolt[6];

	gfloat BiasVolt=0.;
	gfloat Current=0.;

	for (; f.good ();){
		f.getline (line, 0x4000);
		//                  0        1         2         3         4
		//                  1234567890123456789012345678901234567890
		if (strncmp (line, "DATA", 4) == 0) // start of data
			break;
		if (strncmp (line, "Titel / Titel=", 14) == 0)
			;
		if (strncmp (line, "Delta X / Delta X [Dac]=", 24) == 0)
			dxi = atoi (&line[24]);
		if (strncmp (line, "Delta Y / Delta Y [Dac]=", 24) == 0)
			dyi = atoi (&line[24]);
		if (strncmp (line, "Num.X / Num.X=", 14) == 0)
			scan->data.s.nx = atoi (&line[14]);
		if (strncmp (line, "Num.Y / Num.Y=", 14) == 0)
			scan->data.s.ny = atoi (&line[14]);
		if (strncmp (line, "Delay X+ / Delay X+=", 20) == 0)
			; // = atoi (&line[20]);
		if (strncmp (line, "Delay X- / Delay X-=", 20) == 0)
			; // = atoi (&line[20]);
		if (strncmp (line, "Delay Y / Delay Y=", 18) == 0)
			; // = atoi (&line[18]);
		if (strncmp (line, "D-DeltaX / D-DeltaX=", 20) == 0)
			; // = atof (&line[20]);
		if (strncmp (line, "GainX / GainX=", 14) == 0)
			gx = atof (&line[14]);
		if (strncmp (line, "GainY / GainY=", 14) == 0)
			gy = atof (&line[14]);
		if (strncmp (line, "GainZ / GainZ=", 14) == 0)
			gz = atof (&line[14]);
		if (strncmp (line, "Rotation / Rotation=", 20) == 0)
			scan->data.s.alpha = atof (&line[20]);
		if (strncmp (line, "BiasVoltage / BiasVolt.[mV]=", 28) == 0)
			; // = atof (&line[28]);
		if (strncmp (line, "Gainpreamp / GainPre 10^=", 29) == 0)
			; // = atof (&line[29]);
		if (strncmp (line, "Chan(1,2,4) / Chan(1,2,4)=", 26) == 0)
			chan = atoi (&line[26]);
		if (strncmp (line, "Scanrotoffx / OffsetX=", 22) == 0)
			scan->data.s.x0 = atof (&line[22]);
		if (strncmp (line, "Scanrotoffy / OffsetY=", 22) == 0)
			scan->data.s.y0 = atof (&line[22]);

		if (strncmp (line, "MVolt_1 / MVolt_1=", 18) == 0)
			MVolt[0] = atof (&line[18]);
		if (strncmp (line, "MVolt_2 / MVolt_2=", 18) == 0)
			MVolt[1] = atof (&line[18]);
		if (strncmp (line, "MVolt_3 / MVolt_3=", 18) == 0)
			MVolt[2] = atof (&line[18]);
		if (strncmp (line, "MVolt_4 / MVolt_4=", 18) == 0)
			MVolt[3] = atof (&line[18]);
		if (strncmp (line, "MVolt_5 / MVolt_5=", 18) == 0)
			MVolt[4] = atof (&line[18]);
		if (strncmp (line, "MVolt_6 / MVolt_6=", 18) == 0)
			MVolt[5] = atof (&line[18]);

		if (strncmp (line, "CHMode / CHMode=", 16) == 0)
			; // = atof (&line[0]);
		if (strncmp (line, "Channels / Channels=", 20) == 0)
			channels = atoi (&line[20]);
		if (strncmp (line, "Preamptype / Preamptype=", 24) == 0)
			; // = atof (&line[0]);

		if (strncmp (line, "Z-Res. [A]: +/- =", 17) == 0)
			; // = atof (&line[0]);
		if (strncmp (line, "Length x[A]=", 12) == 0)
			scan->data.s.rx = atof (&line[12]);
		if (strncmp (line, "Length y[A]=", 12) == 0)
			scan->data.s.ry = atof (&line[12]);
		if (strncmp (line, "Biasvolt[mV]=", 13) == 0)
			BiasVolt = atof (&line[13])*1e-3;
		if (strncmp (line, "Current[A]=", 11) == 0)
			Current = atof (&line[11]);
		if (strncmp (line, "ActGainXYZ=", 11) == 0)
			; // = atof (&line[0]);

		if (strncmp (line, "Channels=", 9) == 0)
			channels = atoi (&line[9]);

		if (strncmp (line, "ZPiezoconst=", 12) == 0)
			pzAV_z = atof (&line[12]);
		if (strncmp (line, "Xpiezoconst=", 12) == 0)
			pzAV_x = atof (&line[12]);
		if (strncmp (line, "YPiezoconst=", 12) == 0)
			pzAV_y = atof (&line[12]);

		if (strncmp (line, "memo:", 5) == 0){
			stop = 1;
			g_string_append(FileList, line);
			g_string_append(FileList, "\n");
		}
		if (!stop){
			g_string_append(FileList, line);
			g_string_append(FileList, "\n");
		}
	}

	scan->data.ui.SetComment (FileList->str);
	g_string_free(FileList, TRUE); 
	FileList=NULL;

	scan->data.s.dx = scan->data.s.rx / scan->data.s.nx;
	scan->data.s.dy = scan->data.s.ry / scan->data.s.ny;

	// convert to Ang
	scan->data.s.x0 *= pzAV_x / 32767.*10. * gx ;
	scan->data.s.y0 *= pzAV_y / 32767.*10. * gy ;


	// Read Img Data.
	
	// skip 2 bytes after "\r\nDATA"
	// f.read(line, 2);

	switch (datamode){
	case 16:
		scan->data.s.dz = pzAV_z / (1<<15) * 10.* gz; // adjust Z
		f.seekg(0x4004, ios::beg);
		scan->mem2d->Resize (scan->data.s.nx, scan->data.s.ny, ZD_SHORT);
		scan->mem2d->DataRead (f, 1);
		break;
	case 32: {
		gint vals = channels > 1? channels:1;
		scan->data.s.nvalues = vals;

		scan->data.s.dz = pzAV_z / (1<<19) * 10. * gz; // adjust Z
		if (vals > 1)
			scan->mem2d->Resize (scan->data.s.nx, scan->data.s.ny, vals, ZD_FLOAT);
		else
			scan->mem2d->Resize (scan->data.s.nx, scan->data.s.ny, ZD_FLOAT);

		gulong page = scan->mem2d->GetNx () * scan->mem2d->GetNy ();
		gulong sz = page * vals;

		gfloat *zbuf = g_new (gfloat, sz);
		gfloat *buf  = g_new (gfloat, sz);
		gulong nb = sz * sizeof (gfloat);
		memset ((void*) zbuf, 0, nb);
		memset ((void*) buf, 0, nb);

		// compute gzdata length from file length - headersize
		f.seekg (0, ios::end);
		gulong znb = (gulong) ((long)(f.tellg ()) - 0x4000);

		// start of gzdata
		f.seekg (0x4000, ios::beg);
		f.read ((char*) zbuf, nb);
		g_print ("GME32Z-Import: deflating Z data [%d] : zuffer-size:%d data-size:%d ratio:%g \n", 
			 uncompress ((Bytef *) buf, &nb, (Bytef *) zbuf, znb),
			 (int)znb, (int)nb, znb/nb*100.);
		g_free (zbuf);

		// convert to ZD_LONG
		for (gint vi = 0; vi < vals; ++vi){
			for (gint line = 0; line < scan->mem2d->GetNy (); ++line)
				for (gint col = 0; col < scan->mem2d->GetNx (); ++col){
					#ifdef WORDS_BIGENDIAN
						guint32 *i32 = (guint32*) &buf[col + line*scan->mem2d->GetNx () + vi*page];
						guint32 tmp = *i32;
						*i32 = (tmp>>24) | (((tmp>>16)&0xff) << 8) | (((tmp>>8)&0xff) << 16) | ((tmp&0xff) << 24);
					#endif
					scan->mem2d->PutDataPkt ((double) buf[col + line*scan->mem2d->GetNx () + vi*page], 
							 col, line, vi);
				}
			scan->mem2d->data->SetVLookup (vi, MVolt[vi]);
		}
		g_free (buf);
	}
		break;
	default:
		break; // error!!
	}

	scan->data.orgmode = SCAN_ORG_MIDDLETOP;
	scan->mem2d->data->MkXLookup (-scan->data.s.rx/2., scan->data.s.rx/2.);
	scan->mem2d->data->MkYLookup (0., -scan->data.s.ry);

	scan->mem2d->add_layer_information (new LayerInformation ("Bias", BiasVolt, "%6.3f V"));
	scan->mem2d->add_layer_information (new LayerInformation ("Current", Current, "%g A"));

	f.close ();
  
	// GME Dat read done.
	return FIO_OK; 
}


FIO_STATUS gmeyer_ImExportFile::Write(){
	gchar tmp[0x4004];
	const gchar *fname;
	ofstream f;

	memset (tmp, 0, sizeof (tmp));

	if(strlen(name)>0)
		fname = (const char*)name;
	else
		fname = gapp->file_dialog("File Export: GME-Dat"," ","*.dat","","GME-Dat write");
	if (fname == NULL) return FIO_NO_NAME;

	// check if we like to handle this
	if (strncmp(fname+strlen(fname)-4,".dat",4))
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;


	f.open(name, ios::out);
	if (!f.good())
	        return status=FIO_OPEN_ERR;

	f.write (tmp, 0x4004); // fill up with NULL
	f.seekp (0, ios::beg);

	// need to write scan size?
	// nx = scan->mem2d->GetNx(), ny = scan->mem2d->GetNy()

	// need some data like X scan range?
	// scan->data.s.rx, .ry
	// scan->data.s.dx, .dy, .dz
	// scan->data.s.x0, .y0
	// scan->data.s.alpha, ... see Gxsm/src/xsmtypes.h for a complete structure listing!

	f << "[Parameter]\r\n"
	  << "Titel / Titel=DEFAULT\r\n"
	  << "Delta X / Delta X [Dac]=" << scan->data.s.dx << "\r\n"
	  << "Delta Y / Delta Y [Dac]=" << scan->data.s.dy << "\r\n"
	  << "Num.X / Num.X=" << scan->mem2d->GetNx() << "\r\n"
	  << "Num.Y / Num.Y=" << scan->mem2d->GetNy() << "\r\n";

	// Note: X- is only MV_nRegel if no retrace scan is done!
	f << "Delay X+ / Delay X+=" << 1 << "\r\n"
	  << "Delay X- / Delay X-=" << 1 << "\r\n"
	  << "Delay Y / Delay Y=" << 1 << "\r\n"
	  << "D-DeltaX / D-DeltaX=" << 1 << "\r\n";

	/*
	f << "GainX / GainX=10\r\n"
	  << "GainY / GainY=10\r\n"
	  << "GainZ / GainZ=10\r\n";
	*/

	f << "Rotation / Rotation=" << scan->data.s.alpha << "\r\n";

	f << "BiasVoltage / BiasVolt.[mV]=" << (int)(scan->data.s.Bias*1000) << "\r\n"
       // << "Gainpreamp / GainPre 10^=8\r\n"
	  << "Chan(1,2,4) / Chan(1,2,4)=1\r\n"
	  << "PlanDx / PlanDx=0.0000\r\n"
	  << "PlanDy / PlanDy=0.0000\r\n";

	// Note: This is in [Ang]
	f << "Scanrotoffx / OffsetX=" << scan->data.s.x0 << "\r\n"
	  << "Scanrotoffy / OffsetY=" << scan->data.s.y0 << "\r\n";

	/*
	f << "MVolt_1 / MVolt_1=-20000\r\n"
	  << "MVolt_2 / MVolt_2=-20000\r\n"
	  << "MVolt_3 / MVolt_3=0\r\n"
	  << "MVolt_4 / MVolt_4=0\r\n"
	  << "MVolt_5 / MVolt_5=0\r\n"
	  << "MVolt_6 / MVolt_6=0\r\n"
	  << "RepeatRotinc / RepeatRotinc=0\r\n"
	  << "RptBVoltinc / RptBVoltinc=0\r\n"
	  << "Repeatinterval / Repeatinterval=0\r\n"
	  << "Repeatcounter / Repeatcounter=0\r\n"
	  << "RepeatXoffset / RepeatXoffset=0\r\n"
	  << "RepeatYoffset / RepeatYoffset=0\r\n"
	  << "Scantype / Scantype=1\r\n"
	  << "Scanmode / Scanmode=1\r\n"
	  << "Scancoarse / Scancoarse=1\r\n";
	*/

	f << "CHMode / CHMode=0\r\n";
	f << "Channels / Channels=1\r\n";

	/*
	f << "Preamptype / Preamptype=4\r\n"
	  << "Latmanmode=0\r\n"
	  << "LatmResist=1.000E+11\r\n"
	  << "LatmanVolt=1000\r\n"
	  << "Latmanlgi=1\r\n"
	  << "Latmangain=8\r\n"
	  << "Latmanddx=1\r\n"
	  << "Latmandelay=100\r\n"
	  << "Latm0Delay=2\r\n"
	  << "Latmanccdz=0\r\n"
	  << "Latmanextension=1.000\r\n"
	  << "TipForm_Volt=0\r\n"
	  << "TipForm_Z=0\r\n"
	  << "Tip_Delay=100\r\n"
	  << "Tip_Latddx=1\r\n"
	  << "Tip_LatDelay=100\r\n"
	  << "Vertmangain=8\r\n"
	  << "Vertmandelay=10\r\n"
	  << "Vpoint0.t=0\r\n"
	  << "Vpoint1.t=400\r\n"
	  << "Vpoint2.t=600\r\n"
	  << "Vpoint3.t=1024\r\n"
	  << "Vpoint0.V=0\r\n"
	  << "Vpoint1.V=-1000\r\n"
	  << "Vpoint2.V=-1000\r\n"
	  << "Vpoint3.V=0\r\n"
	  << "Zpoint0.t=0\r\n"
	  << "Zpoint1.t=400\r\n"
	  << "Zpoint2.t=600\r\n"
	  << "Zpoint3.t=1024\r\n"
	  << "Zpoint0.z=0\r\n"
	  << "Zpoint1.z=-100\r\n"
	  << "Zpoint2.z=-100\r\n"
	  << "Zpoint3.z=0\r\n";
	*/

	/*
	f << "Zoffset=0\r\n"
	  << "Zdrift=0\r\n"
	  << "VertSpecBack=0\r\n"
	  << "VertSpecAvrgnr=1\r\n"
	  << "VertRepeatCounter=1\r\n"
	  << "Imageframe=1\r\n"
	  << "Imagegrayfactor=0.999\r\n"
	  << "Zoom=2\r\n"
	  << "OrgPlanX=0.00000\r\n"
	  << "OrgPlanY=0.00000\r\n"
	  << "OrgPlanOff=0.00000\r\n";
	*/

	/*
	f << "Dacto[A]xy=0.03052\r\n"
	  << "Dacto[A]z=0.00610\r\n";
	*/

	/*
	f << "Planoff=0\r\n"
	  << "Planx=0\r\n"
	  << "Plany=0\r\n"
	  << "Plany2=0\r\n"
	  << "Planavrgnr=17\r\n"
	  << "Planoffset2=0\r\n"
	  << "Plano2start=0\r\n"
	  << "Imagebackoffset=0\r\n"
	  << "Frameimageoffset=1000\r\n"
	  << "YLineOFF=1\r\n"
	  << "YLineFAC=10\r\n"
	  << "DigZoomX=1.00000\r\n"
	  << "DigZoomZ=1.00000\r\n";
	*/

	/*
	f << "FBLogIset= 500.100\r\n"
	  << "FBRC=  0.000200\r\n"
	  << "FBLingain=-60000.0\r\n"
	  << "FBVoltRC=  0.000526\r\n"
	  << "FBVoltGain=    6.0\r\n"
	  << "CurrentRC= 0.010\r\n"
	  << "FBLog=1\r\n"
	  << "Imaxcurrent=100000.0\r\n"
	  << "Imaxdelay=   1000\r\n"
	  << "ImaxZret=  0.000\r\n"
	  << "SpecAvrgnr=1\r\n"
	  << "SpecFreq=10000.000\r\n"
	  << "SpecChan=1\r\n"
	  << "FFTPoints=1024\r\n"
	  << "LockinFreq=50\r\n"
	  << "LockinAmpl=200\r\n"
	  << "LockinPhase=0\r\n"
	  << "LockinPhase2=0\r\n"
	  << "LockinRC= 0.00010\r\n"
	  << "LockinMode=1\r\n"
	  << "DSP_Clock=50000\r\n"
	  << "SRS_Frequency=1000.000\r\n"
	  << "SRS_ModVoltage=  1.0000\r\n"
	  << "SRS_InpGain[V]=1.0E+00\r\n"
	  << "SRS_InpTimeC[s]=1.0E+00\r\n"
	  << "UserPreampCode=5:000000/6:000000/7:000000/8:000000/\r\n"
	  << "Upinc=50\r\n"
	  << "Upcount=200\r\n"
	  << "Downinc=50\r\n"
	  << "Downcount=200\r\n"
	  << "Rotinc=50\r\n"
	  << "Rotincquad=0\r\n"
	  << "Rotcount=200\r\n"
	  << "XYBurst=100\r\n"
	  << "RotBurst=100\r\n"
	  << "Aprologiset=  2.000\r\n"
	  << "Aprolimit=10000\r\n"
	  << "Aprodelay=300\r\n"
	  << "Aprorc= 0.00020\r\n"
	  << "Autolevel=1\r\n"
	  << "RPTcount=0\r\n"
	  << "RPTIncVolt=0\r\n"
	  << "RPTIncdz=0\r\n"
	  << "SDOrg%=1\r\n"
	  << "SdStatDiff%=0\r\n"
	  << "SDOffset=0\r\n"
	  << "SDBbeta=0\r\n"
	  << "CorrCheckSize=8\r\n"
	  << "CorrImageSize=8\r\n"
	  << "IslandLevel=0.50\r\n"
	  << "Scall-Number=\r\n"
	  << "VertSwitchLevel=0\r\n";
	*/

	/*
	f << "ADC_0=11\r\n"
	  << "ADC_1=-1\r\n"
	  << "ADC_2=2\r\n"
	  << "ADC_3=1\r\n"
	  << "VertMaxI=11000\r\n"
	  << "VertTreshimin=-20000\r\n"
	  << "VertTreshimax=20000\r\n"
	  << "RptCHMZinc=0\r\n"
	  << "CHModeZoffset=0\r\n"
	  << "CHModegainpreamp=6\r\n"
	  << "ScandVinc=0\r\n"
	  << "ScandIinc=0\r\n"
	  << "TrackingDx=1\r\n"
	  << "AltSpecCalc=0\r\n"
	  << "VStart=-1000\r\n"
	  << "Vend=1000\r\n"
	  << "ZStart=0\r\n"
	  << "ZMid=0\r\n"
	  << "ZEnd=0\r\n"
	  << "VoltReso=1\r\n"
	  << "SpecTime=1000\r\n"
	  << "MVolt_7=0\r\n"
	  << "MVolt_8=0\r\n"
	  << "MVolt_9=0\r\n"
	  << "MVolt_10=0\r\n"
	  << "MVolt_11=0\r\n"
	  << "MVolt_12=0\r\n"
	  << "MVolt_13=0\r\n"
	  << "MVolt_14=0\r\n"
	  << "MVolt_15=0\r\n"
	  << "MVolt_16=0\r\n"
	  << "MVolt_17=0\r\n"
	  << "MVolt_18=0\r\n"
	  << "MVolt_19=0\r\n"
	  << "MVolt_20=0\r\n";
	*/

	/*
	f << "VFBVLimit=5000\r\n"
	  << "Preampoffset=-335\r\n"
	  << "USBBufferFill=0\r\n"
	  << "FBAFMSetpoint=-1654\r\n"
	  << "FBAFMrc=0.0005260\r\n"
	  << "FBAFMgain=6\r\n"
	  << "THe[K]= \r\n"
	  << "TSTM[K]= \r\n"
	  << "TManip[K]= \r\n"
	  << "PLoadLock= \r\n"
	  << "PPraep= \r\n"
	  << "TSample[C]= \r\n"
	  << "TEvap[C]= \r\n"
	  << "= \r\n";
	*/

	f << "Z-Res. [A]: +/- =" << scan->data.s.rz << "\r\n"
	  << "Length x[A]=" << scan->data.s.rx << "\r\n"
	  << "Length y[A]=" << scan->data.s.ry << "\r\n";

	/*
	f << "Biasvolt[mV]=-3200\r\n"
	  << "Current[A]=5.0E-07\r\n";
	*/

	/*
	f << "Sec/line:=7.168\r\n"
	  << "Sec/Image:=3670.016\r\n"
	  << "ActGainXYZ=30 30 30\r\n"
	  << "Channels=1\r\n"
	  << "T-STM:=0.00000\r\n"
	  << "= \r\n"
	  << "HP_Ch1=0.00000\r\n"
	  << "HP_Ch2=0.00000\r\n"
	  << "HP_Ch3=0.00000\r\n"
	  << "HP_Ch4=0.00000\r\n"
	  << "= \r\n";
	*/

	/*
	f << "ZPiezoconst= 20.00\r\n"
	  << "Xpiezoconst=100.00\r\n"
	  << "YPiezoconst=100.00\r\n"
	  << "= \r\n";
	*/

	/*
	f << "T_ADC2[K]=  0.000\r\n"
	  << "T_ADC3[K]=  0.000\r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n"
	  << "= \r\n";
	*/

	int i=0;

	f << "memo:" << i++ << "=" << "Gxsm Data Export to GME-Dat File from: " << scan->data.ui.name << "\r\n";
	f << "memo:" << i++ << "=" << "Originalname: " << scan->data.ui.originalname << "\r\n";
	f << "memo:" << i++ << "=" << "User: " << scan->data.ui.user << "\r\n";
	f << "memo:" << i++ << "=" << "Host: " << scan->data.ui.host << "\r\n";
	f << "memo:" << i++ << "=" << "Title: " << scan->data.ui.title << "\r\n";
	f << "memo:" << i++ << "=" << "DateOfScan: " << scan->data.ui.dateofscan << "\r\n";
	f << "memo:" << i++ << "=" << "Comment:" << "\r\n";
	
	gchar **memo = g_strsplit (scan->data.ui.comment, "\n", 64);

	for (int j=0; j<64 && memo[j]; ++j)
		f << "memo:" << i++ << "=" << memo[j] << "\r\n";

	g_strfreev (memo);

	f.seekp (0x3ffa, ios::beg);
	f << "\r\nDATA";

	f.seekp (0x4004, ios::beg);
	scan->mem2d->DataWrite(f);

  return FIO_OK; 
}

// Plugin's Notify Cb's, registered to be called on file load/save to check file
// return via filepointer, it is set to Zero or passed as Zero if file has been processed!
// That's all fine, you should just change the Text Stings below...


static void gmeyer_im_export_filecheck_load_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		PI_DEBUG (DBG_L2, 
			  "Check File: gmeyer_im_export_filecheck_load_callback called with >"
			  << *fn << "<" );

		Scan *dst = gapp->xsm->GetActiveScan();
		if(!dst){ 
			gapp->xsm->ActivateFreeChannel();
			dst = gapp->xsm->GetActiveScan();
		}
		gmeyer_ImExportFile fileobj (dst, *fn);

		FIO_STATUS ret = fileobj.Read(); 
		if (ret != FIO_OK){ 
			// I'am responsible! (But failed)
			if (ret != FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
				*fn=NULL;
			// no more data: remove allocated and unused scan now, force!
//			gapp->xsm->SetMode(-1, ID_CH_M_OFF, TRUE); 
			PI_DEBUG (DBG_L2, "Read Error " << ((int)ret) << "!!!!!!!!" );
		}else{
			// got it!
			*fn=NULL;

			// Now update gxsm main window data fields
			gapp->xsm->ActiveScan->GetDataSet(gapp->xsm->data);
			gapp->spm_update_all();
			dst->draw();
		}
	}else{
		PI_DEBUG (DBG_L2, "gmeyer_im_export_filecheck_load: Skipping" );
	}
}

static void gmeyer_im_export_filecheck_save_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		Scan *src;
		PI_DEBUG (DBG_L2,
			  "Check File: gmeyer_im_export_filecheck_save_callback called with >"
			  << *fn << "<" );

		gmeyer_ImExportFile fileobj (src = gapp->xsm->GetActiveScan(), *fn);

		FIO_STATUS ret;
		ret = fileobj.Write(); 

		if(ret != FIO_OK){
			// I'am responsible! (But failed)
			if (ret != FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
				*fn=NULL;
			PI_DEBUG (DBG_L2, "Write Error " << ((int)ret) << "!!!!!!!!" );
		}else{
			// write done!
			*fn=NULL;
		}
	}else{
		PI_DEBUG (DBG_L2, "gmeyer_im_export_filecheck_save: Skipping >" << *fn << "<" );
	}
}

// Menu Call Back Fkte

static void gmeyer_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (gmeyer_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (gmeyer_im_export_pi.name, "-import", NULL);
	gchar *fn = gapp->file_dialog_load (help[0], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
	        gmeyer_im_export_filecheck_load_callback (&fn);
                g_free (fn);
	}
}

static void gmeyer_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (gmeyer_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (gmeyer_im_export_pi.name, "-export", NULL);
	gchar *fn = gapp->file_dialog_save (help[1], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
                gmeyer_im_export_filecheck_save_callback (&fn);
                g_free (fn);
	}
}
