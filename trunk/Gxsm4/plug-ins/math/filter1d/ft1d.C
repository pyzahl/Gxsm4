/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=ft1d
pluginmenuentry 	=Ft1d
menupath		=Math/Filter 1D/
entryplace		=Filter 1D
shortentryplace	=F1D
abouttext		=This is the Ft1d filter, it does 1D edge enhancement.
smallhelp		=Ft1d filter
longhelp		=Ft1d filter
 * 
 * Gxsm Plugin Name: ft1d.C
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
% PlugInDocuCaption: Ft1d filter
% PlugInName: ft1d
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Filter 1D/Ft1d

% PlugInDescription
\label{PlugIn-F1D-Ft1d}
The ft1d filter: line by line FT.

% PlugInUsage
Call \GxsmMenu{Math/Filter 1D/Ft1d}.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

// Plugin Prototypes
static void ft1d_init( void );
static void ft1d_about( void );
static void ft1d_configure( void );
static void ft1d_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean ft1d_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean ft1d_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin ft1d_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "ft1d-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-F1D",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup("Ft1d filter"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-filter1d-section",
  // Menuentry
  N_("FT 1d"),
  // help text shown on menu
  N_("Ft1d filter"),
  // more info...
  "Ft1d filter",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  ft1d_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  ft1d_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  ft1d_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  ft1d_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin ft1d_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin ft1d_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin ft1d_m1s_pi
#else
 GxsmMathTwoSrcPlugin ft1d_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   ft1d_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm ft1d Plugin\n\n"
                                   "This is the Ft1d filter.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  ft1d_pi.description = g_strdup_printf(N_("Gxsm MathOneArg ft1d plugin %s"), VERSION);
  return &ft1d_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_for_all_vt_plugin_info( void ) {
  return &ft1d_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &ft1d_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void ft1d_init(void)
{
  PI_DEBUG (DBG_L2, "ft1d Plugin Init");
}

// about-Function
static void ft1d_about(void)
{
  const gchar *authors[] = { ft1d_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  ft1d_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void ft1d_configure(void)
{
  if(ft1d_pi.app)
    ft1d_pi.app->message("ft1d Plugin Configuration");
}

// cleanup-Function
static void ft1d_cleanup(void)
{
  PI_DEBUG (DBG_L2, "ft1d Plugin Cleanup");
}

// run-Function
static gboolean ft1d_run(Scan *Src, Scan *Dest)
{
	if (!Src || !Dest)
		return 0;

	if (Src->mem2d->GetNx () < 3) 
		return MATH_SIZEERR;

	return F1D_PowerSpec (Src, Dest);
}


