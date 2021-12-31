/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
 author		=Percy Zahl
 email	        	=zahl@users.sf.net
 pluginname		=scalescan
 pluginmenuentry 	=Scale Scan
 menupath		=Math/Transformations/
 entryplace		=Transformations
 shortentryplace	=TR
 abouttext		=Scan a scan in X and Y by a given factor.
 smallhelp		=Scale a scan in X and Y
 longhelp		=This is a detailed help for my Plugin.
 * 
 * Gxsm Plugin Name: scalescan.C
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
 * All "% OptPlugInXXX" tags are optional and can be removed or commented in
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Scale scan in X and Y
% PlugInName: scalescan
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Transformations/Scale Scan

% PlugInDescription
This filter scales the scan by given factors in X and Y. The resulting
pixel value is computated by using a 2d linear approximatione inbetween
neigbour points.

% PlugInUsage
Call \GxsmMenu{Math/Transformations/Scale Scan}

% OptPlugInSources
The active channel is used as data source.

% OptPlugInDest
The computation result is placed into an existing math channel, else into a new created math channel.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

// Plugin Prototypes
static void scalescan_init( void );
static void scalescan_about( void );
static void scalescan_configure( void );
static void scalescan_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
static gboolean scalescan_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
static gboolean scalescan_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin scalescan_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	g_strdup ("scalescan-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
	"M1S"
#else
	"M2S"
#endif
		  "-TR"),
	// Plugin's Category - used to autodecide on Pluginloading or ignoring
	// NULL: load, else
	// example: "+noHARD +STM +AFM"
	// load only, if "+noHARD: no hardware" and Instrument is STM or AFM
	// +/-xxxHARD und (+/-INST or ...)
	NULL,
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	g_strdup("This is a detailed help for my Plugin."),                   
	// Author(s)
	"Percy Zahl",
	// Menupath to position where it is appendet to
	"math-transformations-section",
	// Menuentry
	N_("Scale Scan"),
	// help text shown on menu
	N_("This is a detailed help for my Plugin."),
	// more info...
	"Scale a scan in X and Y",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	scalescan_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	scalescan_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	scalescan_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

	scalescan_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin scalescan_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin scalescan_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin scalescan_m1s_pi
#else
GxsmMathTwoSrcPlugin scalescan_m2s_pi
#endif
= {
	// math-function to run, see prototype(s) above!!
	scalescan_run
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm scalescan Plugin\n\n"
                                   "Scan a scan in X and Y by a given factor.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	scalescan_pi.description = g_strdup_printf(N_("Gxsm MathOneArg scalescan plugin %s"), VERSION);
	return &scalescan_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
	return &scalescan_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
	return &scalescan_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void scalescan_init(void)
{
	PI_DEBUG (DBG_L2, "scalescan Plugin Init");
}

// about-Function
static void scalescan_about(void)
{
	const gchar *authors[] = { scalescan_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  scalescan_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void scalescan_configure(void)
{
	if(scalescan_pi.app)
		scalescan_pi.app->message("scalescan Plugin Configuration");
}

// cleanup-Function
static void scalescan_cleanup(void)
{
	PI_DEBUG (DBG_L2, "scalescan Plugin Cleanup");
}

// run-Function
static gboolean scalescan_run(Scan *Src, Scan *Dest)
{
	long col, line;
	double sx=1., sy=1.;

	gapp->ValueRequest("Enter Value", "Xfactor", "Factor to scale X (>1 strech, <1 shrink).",
			   gapp->xsm->Unity, 0.01, 100., ".2f", &sx);

	gapp->ValueRequest("Enter Value", "Yfactor", "Factor to scale Y (>1 strech, <1 shrink).",
			   gapp->xsm->Unity, 0.01, 100., ".2f", &sy);

	Dest->data.s.nx = (int)((double)Src->mem2d->GetNx()*sx) & ~1;
	Dest->data.s.dx = Src->data.s.dx/sx;
	Dest->data.s.rx = Src->data.s.rx;

	Dest->data.s.ny = (int)((double)Src->mem2d->GetNy()*sy) & ~1;
	Dest->data.s.dy = Src->data.s.dy/sy;
	Dest->data.s.ry = Src->data.s.ry;

	Dest->mem2d->Resize(Dest->data.s.nx, Dest->data.s.ny);

	for(line = 0; line < Dest->mem2d->GetNy(); line++)
		for(col = 0; col < Dest->mem2d->GetNx(); col++)
			Dest->mem2d->PutDataPkt (
				Src->mem2d->GetDataPktInterpol
				(((double)col)/sx, ((double)line)/sy), 
				col, line);
	return MATH_OK;
}


