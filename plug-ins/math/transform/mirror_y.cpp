/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=mirror_y
pluginmenuentry 	=Mirror Y
menupath		=Math/Transformations/
entryplace		=Transformations
shortentryplace	=TR
abouttext		=Mirror scan along Y.
smallhelp		=Mirror scan along Y.
longhelp		=This is a detailed help for my Plugin.
 * 
 * Gxsm Plugin Name: mirror_y.C
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
% PlugInDocuCaption: Mirror Y
% PlugInName: mirror_y
% PlugInAuthor: A. Klust, P. Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Transformations/Mirror Y

% PlugInDescription
Mirror scan along Y.

% PlugInUsage
Call \GxsmMenu{Math/Transformations/Mirror Y}.

% OptPlugInSources
The active channel is used as data source.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

% OptPlugInDest
The computation result is placed into an existing math channel, else into a new created math channel.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"


// Plugin Prototypes
static void mirror_y_init( void );
static void mirror_y_about( void );
static void mirror_y_configure( void );
static void mirror_y_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
static gboolean mirror_y_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
static gboolean mirror_y_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin mirror_y_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	"mirror_y-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
	"M1S"
#else
	"M2S"
#endif
	"-TR",
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
	N_("Mirror Y"),
	// help text shown on menu
	N_("This is a detailed help for my Plugin."),
	// more info...
	"Mirror scan along Y.",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	mirror_y_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	mirror_y_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	mirror_y_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

	mirror_y_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin mirror_y_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin mirror_y_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin mirror_y_m1s_pi
#else
GxsmMathTwoSrcPlugin mirror_y_m2s_pi
#endif
= {
	// math-function to run, see prototype(s) above!!
	mirror_y_run
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm mirror_y Plugin\n\n"
                                   "Mirror scan along Y.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	mirror_y_pi.description = g_strdup_printf(N_("Gxsm MathOneArg mirror_y plugin %s"), VERSION);
	return &mirror_y_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_for_all_vt_plugin_info( void ) {
	return &mirror_y_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
	return &mirror_y_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void mirror_y_init(void)
{
	PI_DEBUG (DBG_L2, "mirror_y Plugin Init");
}

// about-Function
static void mirror_y_about(void)
{
	const gchar *authors[] = { mirror_y_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  mirror_y_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void mirror_y_configure(void)
{
	if(mirror_y_pi.app)
		mirror_y_pi.app->message("mirror_y Plugin Configuration");
}

// cleanup-Function
static void mirror_y_cleanup(void)
{
	PI_DEBUG (DBG_L2, "mirror_y Plugin Cleanup");
}

// run-Function
static gboolean mirror_y_run(Scan *Src, Scan *Dest)
{
	if (!Src || !Dest)
			return 0;

	for (int col = 0; col < Src->mem2d->GetNx(); col++)
		Dest->mem2d->CopyFrom(Src->mem2d, col,
				      0, Src->mem2d->GetNx()-col-1,
				      0, 1,Dest->mem2d->GetNy());
	return MATH_OK;
}


