/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=log_z
pluginmenuentry 	=Log Z
menupath		=Math/Arithmetic/
entryplace		=Arithmetic
shortentryplace	=AR
abouttext		=apply log Z
smallhelp		=apply log Z
longhelp		=applys log to all Z values
 * 
 * Gxsm Plugin Name: log_z.C
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
% PlugInDocuCaption: Logarithm transform of Z-values
% PlugInName: log_z
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Arithmetic/Log

% PlugInDescription
Apply a logarithm transform to the Z values. It subtracts the Z
minimum, adds one computes the logarithm of that:

\[ Z_{\text{math}} = \log \left( Z_{\text{active}}
   - \text{minval} \left( Z_{\text{active}} \right) + 1 \right) \]

% PlugInUsage
Call \GxsmMenu{Math/Arithmetic/Log}.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel. The result is of type \GxsmEmph{float}.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"

// Plugin Prototypes
static void log_z_init( void );
static void log_z_about( void );
static void log_z_configure( void );
static void log_z_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean log_z_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean log_z_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin log_z_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "log_z-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-AR",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("applys log to all Z values"),
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-arithmetic-section",
  // Menuentry
  N_("Log"),
  // help text shown on menu
  N_("applies log to all Z values"),
  // more info...
  "apply log to all Z values, data is shifted to be >1 before.",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  log_z_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  log_z_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  log_z_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  log_z_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin log_z_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin log_z_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin log_z_m1s_pi
#else
 GxsmMathTwoSrcPlugin log_z_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   log_z_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm log_z Plugin\n\n"
                                   "apply log Z, data is shifted to be >1 before.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  log_z_pi.description = g_strdup_printf(N_("Gxsm MathOneArg log_z plugin %s"), VERSION);
  return &log_z_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &log_z_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &log_z_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void log_z_init(void)
{
  PI_DEBUG (DBG_L2, "log_z Plugin Init");
}

// about-Function
static void log_z_about(void)
{
  const gchar *authors[] = { log_z_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  log_z_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void log_z_configure(void)
{
  if(log_z_pi.app)
    log_z_pi.app->message("log_z Plugin Configuration");
}

// cleanup-Function
static void log_z_cleanup(void)
{
  PI_DEBUG (DBG_L2, "log_z Plugin Cleanup");
}

// run-Function
static gboolean log_z_run(Scan *Src, Scan *Dest)
{
	int line, col;
	//double hi,lo,fac;

	Dest->mem2d->Resize (Dest->data.s.nx, Dest->data.s.ny, Dest->mem2d->GetNv (), ZD_FLOAT);

	//Src->mem2d->HiLo(&hi, &lo, FALSE);
	//if(hi-lo < 1.) return MATH_OK;

	for(int v=0; v < Dest->mem2d->GetNv (); v++)
		for(line=0; line<Dest->data.s.ny; line++)
			for(col=0; col<Dest->data.s.nx; col++)
				Dest->mem2d->PutDataPkt (log (Src->mem2d->GetDataPkt (col, line, v)),
							 col, line, v);
	//				Dest->mem2d->PutDataPkt (log (Src->mem2d->GetDataPkt (col, line, v)-lo+1.),
	//							 col, line, v);
	return MATH_OK;
}


