/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=unwrap
pluginmenuentry 	=Unwrap
menupath		=Math/Transformations/
entryplace		=Transformations
shortentryplace	=TR
abouttext		=Unwraps Z data in given range
smallhelp		=Unwraps Z data in given range
longhelp		=Unwraps Z data in given range
 * 
 * Gxsm Plugin Name: unwrap.C
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
% PlugInDocuCaption: Unwraps Z data in given range
% PlugInName: unwrap
% PlugInAuthor: P. Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Transformations/Unwrap

% PlugInDescription
Flips an image along it's diagonale.

% PlugInUsage
Call \GxsmMenu{Math/Transformations/Unwarp}.

% OptPlugInSources
The active channel is used as data source.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

%% OptPlugInDest
%The computation result is placed into an existing math channel, else into a new created math channel.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

// Plugin Prototypes
static void unwrap_init( void );
static void unwrap_about( void );
static void unwrap_configure( void );
static void unwrap_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
static gboolean unwrap_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
static gboolean unwrap_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin unwrap_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	"unwrap-"
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
	N_("Unwrap"),
	// help text shown on menu
	N_("This is a detailed help for my Plugin."),
	// more info...
	"Unwraps Z data in given range",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	unwrap_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	unwrap_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	unwrap_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

	unwrap_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin unwrap_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin unwrap_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin unwrap_m1s_pi
#else
GxsmMathTwoSrcPlugin unwrap_m2s_pi
#endif
= {
	// math-function to run, see prototype(s) above!!
	unwrap_run
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm unwrap Plugin\n\n"
                                   "Flips an image along it's diagonale.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	unwrap_pi.description = g_strdup_printf(N_("Gxsm MathOneArg unwrap plugin %s"), VERSION);
	return &unwrap_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_for_all_vt_plugin_info( void ) {
	return &unwrap_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
	return &unwrap_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void unwrap_init(void)
{
	PI_DEBUG (DBG_L2, "unwrap Plugin Init");
}

// about-Function
static void unwrap_about(void)
{
	const gchar *authors[] = { unwrap_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  unwrap_pi.name,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void unwrap_configure(void)
{
	if(unwrap_pi.app)
		unwrap_pi.app->message("unwrap Plugin Configuration");
}

// cleanup-Function
static void unwrap_cleanup(void)
{
	PI_DEBUG (DBG_L2, "unwrap Plugin Cleanup");
}

// must be called consecutively
double uw (double z, double diffmax, double jump){
        double dz;
        static double wrap=0.;
        static double z1=0.;

        // init?
        if (jump < 0.){
                wrap = 0.;
                z1 = z;
        }
                        
        dz = z-z1;
        z1=z;
#if 1
        if (z < 0.)
                return (z+jump);
        else
                return (z);
#else
       
        if (dz > diffmax)
                wrap -= jump;
        else if (dz < -diffmax)
                wrap += jump;
        
        return (z+wrap);
#endif
}

// run-Function
static gboolean unwrap_run(Scan *Src, Scan *Dest)
{
        double min, max, jump, dmax;
	if (!Src || !Dest)
		return 0;

        min = -20.296/Dest->data.s.dz;
        max =  20.664/Dest->data.s.dz;
        jump = max-min;
        dmax = 0.5*jump;

        uw (Src->mem2d->GetDataPkt (0, 0), dmax, -1.);
        for (int line = 0; line < Dest->mem2d->GetNy (); line++){
                if (line > 0)
                        uw (Src->mem2d->GetDataPkt (0, line-1), dmax, -1.);
                for (int col = 0; col < Dest->mem2d->GetNx (); col++){
                        Dest->mem2d->PutDataPkt (uw (Src->mem2d->GetDataPkt (col, line), dmax, jump), 
                                                 col, line);
                }
        }

	return MATH_OK;
}


