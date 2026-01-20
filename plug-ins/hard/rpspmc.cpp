/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: rpspmc_pacpll.C
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
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: RPSPMC PACPLL
% PlugInName: rpspmc_pacpll
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: windows-section RPSPMC PACPLL
RP data streaming

% PlugInDescription

% PlugInUsage

% OptPlugInRefs

% OptPlugInNotes

% OptPlugInHints

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include <gio/gio.h>
#include <libsoup/soup.h>
#include <zlib.h>
#include <gsl/gsl_rstat.h>

#include "config.h"
#include "plugin.h"

#include "unit.h"
#include "pcs.h"
#include "xsmtypes.h"
#include "glbvars.h"
#include "action_id.h"

#include "gxsm_app.h"
#include "gxsm_window.h"
#include "surface.h"

#include "../common/pyremote.h"

#include "rpspmc_hwi_dev.h"
#include "rpspmc_control.h"
#include "rpspmc_pacpll.h"
#include "rpspmc_gvpmover.h"

// Define HwI PlugIn reference name here, this is what is listed later within "Preferenced Dialog"
// i.e. the string selected for "Hardware/Card"!
#define THIS_HWI_PLUGIN_NAME "RPSPMC:SPM"
#define THIS_HWI_PREFIX      "RPSPMC_HwI"


extern int debug_level;
extern int force_gxsm_defaults;

// RPSPM PlugIn HwI global control classes hold here
rpspmc_hwi_dev  *rpspmc_hwi = NULL;
RPSPMC_Control  *RPSPMC_ControlClass = NULL;
RPspmc_pacpll   *rpspmc_pacpll = NULL;
GVPMoverControl *rpspmc_gvpmover = NULL;



// Plugin Prototypes - default PlugIn functions
static void rpspmc_pacpll_hwi_init (void); // PlugIn init
static void rpspmc_pacpll_hwi_query (void); // PlugIn "self-install"
static void rpspmc_pacpll_hwi_about (void); // About
static void rpspmc_pacpll_hwi_configure (void); // Configure plugIn, called via PlugIn-Configurator
static void rpspmc_pacpll_hwi_cleanup (void); // called on PlugIn unload, should cleanup PlugIn rescources

// other PlugIn Functions and Callbacks (connected to Buttons, Toolbar, Menu)
static void rpspmc_pacpll_hwi_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void rpspmc_pacpll_hwi_SaveValues_callback ( gpointer );

// custom menu callback expansion to arbitrary long list
static PIMenuCallback rpspmc_pacpll_hwi_mcallback_list[] = {
		rpspmc_pacpll_hwi_show_callback,             // direct menu entry callback1
		NULL  // NULL terminated list
};

static GxsmPluginMenuCallbackList rpspmc_pacpll_hwi_menu_callback_list = {
	1,
	rpspmc_pacpll_hwi_mcallback_list
};

GxsmPluginMenuCallbackList *get_gxsm_plugin_menu_callback_list ( void ) {
	return &rpspmc_pacpll_hwi_menu_callback_list;
}

// Fill in the GxsmPlugin Description here -- see also: Gxsm/src/plugin.h
GxsmPlugin rpspmc_pacpll_hwi_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	"rpspmc_pacpll_hwi-"
	"HW-INT-NS-FLOAT",
	// Plugin's Category - used to autodecide on Pluginloading or ignoring
	// In this case of Hardware-Interface-Plugin here is the interface-name required
	// this is the string selected for "Hardware/Card"!
	THIS_HWI_PLUGIN_NAME,
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	g_strdup ("Red Pitaya SPM Control and PACPLL hardware interface."),
	// Author(s)
	"Percy Zahl",
	// Menupath to position where it is appendet to -- not used by HwI PIs
	"windows-section,windows-section,windows-section,windows-section",
	// Menuentry -- not used by HwI PIs
	N_("RPSPM Control"),
	// help text shown on menu
	N_("This is the " THIS_HWI_PLUGIN_NAME " - GXSM Hardware Interface."),
	// more info...
	"N/A",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	rpspmc_pacpll_hwi_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	rpspmc_pacpll_hwi_query, // query can be used (otherwise set to NULL) to install
	// additional control dialog in the GXSM menu
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	rpspmc_pacpll_hwi_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	rpspmc_pacpll_hwi_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
	NULL,   // direct menu entry callback1 or NULL
	NULL,   // direct menu entry callback2 or NULL
	rpspmc_pacpll_hwi_cleanup // plugin cleanup callback or NULL
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("RPSPMC PACPLL Control Plugin\n\n"
                                   "Complete FPGA SPM Control using Red Pitaya + Analog Modules.\n"
	);

// event hooks
static void RPSPMC_Control_StartScan_callback ( gpointer );
static void RPSPMC_Control_SaveValues_callback ( gpointer );
static void RPSPMC_Control_LoadValues_callback ( gpointer );

gchar *rpspmc_pacpll_hwi_configure_string = NULL;   // name of the currently in GXSM configured HwI (Hardware/Card)


// Symbol "get_gxsm_hwi_hardware_class" is resolved by dlsym from Gxsm for all HwI type PIs, 
// Essential Plugin Function!!
XSM_Hardware *get_gxsm_hwi_hardware_class ( void *data ) {
        gchar *tmp;
        main_get_gapp()->monitorcontrol->LogEvent (THIS_HWI_PREFIX " XSM_Hardware *get_gxsm_hwi_hardware_class", "Init 1");

	rpspmc_pacpll_hwi_configure_string = g_strdup ((gchar*)data);

        
	// probe for harware here...
	rpspmc_hwi = new rpspmc_hwi_dev ();
	
        main_get_gapp()->monitorcontrol->LogEvent ("HwI: probing succeeded.", "RP SPM Control System Ready.");
	return rpspmc_hwi;
}

// init-Function
void rpspmc_pacpll_hwi_init(void)
{
	PI_DEBUG (DBG_L2, "rspmc_pacpll_hwi Plugin Init");
	rpspmc_pacpll = NULL;
}


// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	rpspmc_pacpll_hwi_pi.description = g_strdup_printf(N_("Gxsm rpspmc_pacpll plugin %s"), VERSION);
	return &rpspmc_pacpll_hwi_pi; 
}



// data passed to "idle" function call, used to refresh/draw while waiting for data
typedef struct {
	GSList *scan_list; // scans to update
	GFunc  UpdateFunc; // function to call for background updating
	gpointer data; // additional data (here: reference to the current rpspmc_pacpll object)
} IdleRefreshFuncData;


// Query Function, installs Plugin's in File/Import and Export Menupaths!

static void rpspmc_pacpll_hwi_query(void)
{
	if(rpspmc_pacpll_hwi_pi.status) g_free(rpspmc_pacpll_hwi_pi.status); 
	rpspmc_pacpll_hwi_pi.status = g_strconcat (
                                                 N_("Plugin query has attached "),
                                                 rpspmc_pacpll_hwi_pi.name, 
                                                 N_(": File IO Filters are ready to use"),
                                                 NULL);

//      Setup Control Windows
// ==================================================
	PI_DEBUG (DBG_L2, "rpspmc_pacpll_query:new" );

        // first
        rpspmc_pacpll = new RPspmc_pacpll (main_get_gapp() -> get_app ());

        // second
	RPSPMC_ControlClass = new RPSPMC_Control (main_get_gapp() -> get_app ());
        
        // third
        rpspmc_gvpmover = new GVPMoverControl (main_get_gapp() -> get_app ());

	rpspmc_pacpll_hwi_pi.status = g_strconcat(N_("Plugin query has attached "),
                                                  rpspmc_pacpll_hwi_pi.name, 
                                                  N_(": " THIS_HWI_PREFIX "-Control is created."),
                                                  NULL);
        
	PI_DEBUG (DBG_L2, "rpspmc_pacpll_query:res" );
	
	rpspmc_pacpll_hwi_pi.app->ConnectPluginToCDFSaveEvent (rpspmc_pacpll_hwi_SaveValues_callback);
	//rpspmc_pacpll_hwi_pi.app->ConnectPluginToCDFLoadEvent (XXX_LoadValues_callback);
}

static void rpspmc_pacpll_hwi_SaveValues_callback ( gpointer gp_ncf ){
	NcFile *ncf = (NcFile *) gp_ncf;
        if (rpspmc_pacpll)
                rpspmc_pacpll->save_values (*ncf);
        if (RPSPMC_ControlClass)
                RPSPMC_ControlClass->save_values (*ncf);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//



// about-Function 
static void rpspmc_pacpll_hwi_about(void)
{
        const gchar *authors[] = { rpspmc_pacpll_hwi_pi.authors, NULL};
        gtk_show_about_dialog (NULL,
                               "program-name",  rpspmc_pacpll_hwi_pi.name,
                               "version", VERSION,
                               "license", GTK_LICENSE_GPL_3_0,
                               "comments", about_text,
                               "authors", authors,
                               NULL
                               );
}

// configure-Function
static void rpspmc_pacpll_hwi_configure(void)
{
	if(rpspmc_pacpll_hwi_pi.app)
		rpspmc_pacpll_hwi_pi.app->message("rpspmc_pacpll Plugin Configuration");
}

// cleanup-Function, make sure the Menustrings are matching those above!!!
static void rpspmc_pacpll_hwi_cleanup(void)
{
	// delete ...
        if (rpspmc_gvpmover)
                delete rpspmc_gvpmover ;
        
	if( rpspmc_pacpll )
		delete rpspmc_pacpll ;

	if(rpspmc_pacpll_hwi_pi.status) g_free(rpspmc_pacpll_hwi_pi.status); 

	PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::rpspmc_hwi_cleanup -- Plugin Cleanup, -- Menu [disabled]\n");

        // remove GUI
	if( RPSPMC_ControlClass ){
		delete RPSPMC_ControlClass ;
        }
	RPSPMC_ControlClass = NULL;

	if (rpspmc_hwi){
                PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::rpspmc_hwi_cleanup -- delete DSP Common-HwI-Class.");
		delete rpspmc_hwi;
        }
	rpspmc_hwi = NULL;

        PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::rpspmc_hwi_cleanup -- g_free rpspmc_pacpll_hwi_configure_string Info.");
	g_free (rpspmc_pacpll_hwi_configure_string);
	rpspmc_pacpll_hwi_configure_string = NULL;
	
        PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::rpspmc_pacpll_hwi_cleanup -- Done.");
}

// show windpow
static void rpspmc_pacpll_hwi_show_callback(GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	PI_DEBUG (DBG_L2, "rpspmc_pacpll Plugin : show" );
	if( rpspmc_pacpll )
		rpspmc_pacpll->show();
}

// END
