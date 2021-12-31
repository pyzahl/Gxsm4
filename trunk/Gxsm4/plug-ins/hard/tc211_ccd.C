/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Hardware Interface Plugin Name: tc211_ccd.C
 * ===============================================
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
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional and can be removed or commented in
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: TC211 CCD Interface
% PlugInName: tc211_ccd
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Hardware/TC211-CCD-HwI

% PlugInDescription
This is an experimental hardware interface plugin.  Grabbing video
data from a parport attached TC211 CCD camera based on the design
presented in the ''c't magazine f\"ur computer technik'', Januar 1992,
p.162ff.

% PlugInUsage
Configure the \GxsmPref{Hardware}{Card} to ''TC211-CCD''. Load
the ''ccd.o'' kernel module before starting the GXSM, setup /dev/ccd
correct. Check the module source code for correct LPT port.

% OptPlugInSources

% OptPlugInDest
Usual scan destination channel.

% OptPlugInNote
Used for my very old experimental Astro CCD camera. The special kernel
module ''ccd'' is used for data transfer.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <sys/ioctl.h>

#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "xsmhard.h"

#include "plug-ins/hard/modules/ccd.h"

// Define HwI PlugIn reference name here, this is what is listed later within "Preferenced Dialog"
// i.e. the string selected for "Hardware/Card"!
#define THIS_HWI_PLUGIN_NAME "TC211-CCD"

// Plugin Prototypes
static void tc211_ccd_init( void );
static void tc211_ccd_about( void );
static void tc211_ccd_configure( void );
static void tc211_ccd_cleanup( void );

// Fill in the GxsmPlugin Description here
GxsmPlugin tc211_ccd_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "tc211_ccd-"
  "HW-INT-1S-SHORT",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // In this case of Hardware-Interface-Plugin here is the interface-name required
  // this is the string selected for "Hardware/Card"!
  THIS_HWI_PLUGIN_NAME,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("TC211 CCD interface."),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to -- not used by HwI PIs
  N_("Hardware/"),
  // Menuentry -- not used by HwI PIs
  N_(THIS_HWI_PLUGIN_NAME"-HwI"),
  // help text shown on menu
  N_("This is the "THIS_HWI_PLUGIN_NAME" - GXSM Hardware Interface"),
  // more info...
  "N/A",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  tc211_ccd_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  tc211_ccd_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  tc211_ccd_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  tc211_ccd_cleanup
};


// Text used in Aboutbox, please update!!
static const char *about_text = N_("GXSM tc211_ccd Plugin\n\n"
                                   "Video for Linux frame grabber.");

/* Here we go... */

/*
 * GXSM V4L Hardware Interface Class
 * ============================================================
 */

class gxsm_TC211_CCD : public XSM_Hardware{
public:
	gxsm_TC211_CCD();
	virtual ~gxsm_TC211_CCD();
	
	/* Parameter  */
	virtual long GetMaxPointsPerLine(){ return CCD_Xpixel; };
	virtual long GetMaxLines(){ return CCD_Ypixel; };
	virtual long GetMaxChannels(){ return 1L; };
	virtual void SetDxDy(int dx, int dy);
	virtual void SetOffset(long x, long y);
	virtual void SetNx(long nx);
	virtual void SetAlpha(double alpha);
	
	virtual void MovetoXY(long x, long y);
	virtual void StartScan2D();
	virtual void ScanLineM(int yindex, int xdir, int muxmode, Mem2d *Mob[MAX_SRCS_CHANNELS], int ix0=0 );
	virtual void EndScan2D();
 
	virtual gchar* get_info(){ 
		return g_strdup("*--GXSM HwI Plugin: TC211_CCD::XSM_Hardware --*\n"
				"CCD TC211 on /dev/ccd is connected!\n"
				"*--Features--*\n"
				"SCAN: Yes\n"
				"*--ExecCMD options--*\n"
				"MONITORENABLE, EXPOSURE\n"
				"*--EOF--*\n"
			);
	};
	void ExecCmd(int Cmd);

	double CCD_exptime;
private:
  int  ccd;
  SHT  ccdline[CCD_Xpixel];
};

/*
 * PI global
 */

gxsm_TC211_CCD *TC211_CCD_hardware = NULL;

/* Konstruktor: device open
 * ==================================================
 */
gxsm_TC211_CCD::gxsm_TC211_CCD():XSM_Hardware(){
	CCD_exptime = 1.;
	ccd = open(xsmres.DSPDev, O_RDWR);
	if(ccd <= 0){ 
		printf("open %s failed, err=%d\ndo you need root to do \"insmod ccd\" ?\n", xsmres.DSPDev, ccd);
		exit(0);
	}
	ioctl(ccd, CCD_CMD_MONITORENABLE, 0);
}

/* Destruktor:
 * ==================================================
 * Hardware "abtrennen"
 */
gxsm_TC211_CCD::~gxsm_TC211_CCD(){
	close(ccd);
}

/* Übergeordnete Parameterübergabefunktionen PC => PC31/DSP
 * ========================================================
 * virtual !
 */
void gxsm_TC211_CCD::ExecCmd(int Cmd){
	switch(Cmd){
	case CCD_CMD_MONITORENABLE:
		ioctl(ccd, CCD_CMD_MONITORENABLE, 0);
		break;
	case CCD_CMD_EXPOSURE:
		ioctl(ccd, CCD_CMD_EXPOSURE, (unsigned long)(CCD_exptime));
		break;
	}
}
void gxsm_TC211_CCD::MovetoXY(long x, long y){ rx=x; ry=y; }
void gxsm_TC211_CCD::SetDxDy(int dx, int dy){  
	Dx = dx; 
	Dy = dy;
}
void gxsm_TC211_CCD::SetOffset(long x, long y){
	rotoffx = x; rotoffy = y;
}
void gxsm_TC211_CCD::SetAlpha(double alpha){ 
	Alpha=M_PI*alpha/180.;
	rotmyy = rotmxx = cos(Alpha);
	rotmyx = -(rotmxy = sin(Alpha));
}
void gxsm_TC211_CCD::SetNx(long nx){ 
	Nx=nx;
}


void gxsm_TC211_CCD::StartScan2D(){ 
	XSM_DEBUG (DBG_L4, "CCD belichten");
	// CCD löschen
	ioctl(ccd, CCD_CMD_CLEAR, 0);
	// CCD belichten
	ioctl(ccd, CCD_CMD_EXPOSURE, (unsigned long)(CCD_exptime));
	ioctl(ccd, CCD_CMD_INITLESEN, 0);
}

void gxsm_TC211_CCD::EndScan2D(){ 
	ioctl(ccd, CCD_CMD_MONITORENABLE, 0);
	XSM_DEBUG (DBG_L4, "CCD gelesen");
}




void gxsm_TC211_CCD::ScanLineM(int yindex, int xdir, int muxmode, Mem2d *Mob[MAX_SRCS_CHANNELS], int ix0 ){
	if (yindex < 0){ // 2D capture
		if (yindex != -1) return; // XP/XM init cycle
		if(Mob[0])
			for(int j=0; j<CCD_Ypixel && j<Mob[0]->GetNy (); j++){
				for(int i=0; i<CCD_Xpixel && i < Mob[0]->GetNx (); i++)
					Mob[0]->PutDataPkt ((double)ioctl (ccd, CCD_CMD_GETPIXEL,0), i, j);
				if(!(j%32))
					gapp->check_events();
			}
		return;
	}

	if(yindex >= CCD_Ypixel) 
		return;

	if(Mob[0])
		for(int i=0; i<CCD_Xpixel && i < Mob[0]->GetNx (); i++)
			Mob[0]->PutDataPkt((double)ioctl(ccd, CCD_CMD_GETPIXEL,0),i,yindex);

	if(!(yindex%32))
		gapp->check_events();
}


/* 
 * PI essential members
 */

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  tc211_ccd_pi.description = g_strdup_printf(N_("GXSM HwI tc211_ccd plugin %s"), VERSION);
  return &tc211_ccd_pi; 
}

// Symbol "get_gxsm_hwi_hardware_class" is resolved by dlsym from Gxsm for all HwI type PIs, 
// Essential Plugin Function!!
XSM_Hardware *get_gxsm_hwi_hardware_class ( void *data ) {
	return TC211_CCD_hardware;
}

// init-Function
static void tc211_ccd_init(void)
{
	PI_DEBUG (DBG_L2, "tc211_ccd Plugin Init");
	TC211_CCD_hardware = new gxsm_TC211_CCD ();
 }

// about-Function
static void tc211_ccd_about(void)
{
	const gchar *authors[] = { tc211_ccd_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  tc211_ccd_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void tc211_ccd_configure(void)
{
	if(tc211_ccd_pi.app)
		tc211_ccd_pi.app->message("tc211_ccd Plugin Configuration");
}

// cleanup-Function
static void tc211_ccd_cleanup(void)
{
	PI_DEBUG (DBG_L2, "tc211_ccd Plugin Cleanup");
	delete TC211_CCD_hardware;
	TC211_CCD_hardware = NULL;
}



