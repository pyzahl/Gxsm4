/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=invert_z
pluginmenuentry 	=Invert
menupath		=Math/Arithmetic/
entryplace		=Arithmetic
shortentryplace	=AR
abouttext		=Inverts Z
smallhelp		=invert Z
longhelp		=Inverts Z values.
 * 
 * Gxsm Plugin Name: invert_z.C
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
% PlugInDocuCaption: Invert Z
% PlugInName: invert_z
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Arithmetic/Invert

% PlugInDescription
Invert the Z values of a scan. \[ Z_{\text{math}} = -Z_{\text{active}} \]

% PlugInUsage
Call \GxsmMenu{Math/Arithmetic/Invert}.

% OptPlugInSources
The active channel is used as data source.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"

// Plugin Prototypes
static void invert_z_init( void );
static void invert_z_about( void );
static void invert_z_configure( void );
static void invert_z_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean invert_z_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean invert_z_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin invert_z_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "invert_z-"
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
  g_strdup("Inverts Z values."),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-arithmetic-section",
  // Menuentry
  N_("Invert"),
  // help text shown on menu
  N_("Inverts Z values."),
  // more info...
  "invert Z",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  invert_z_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  invert_z_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  invert_z_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  invert_z_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin invert_z_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin invert_z_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin invert_z_m1s_pi
#else
 GxsmMathTwoSrcPlugin invert_z_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   invert_z_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm invert_z Plugin\n\n"
                                   "Inverts Z");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  invert_z_pi.description = g_strdup_printf(N_("Gxsm MathOneArg invert_z plugin %s"), VERSION);
  return &invert_z_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &invert_z_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &invert_z_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void invert_z_init(void)
{
  PI_DEBUG (DBG_L2, "invert_z Plugin Init");
}

// about-Function
static void invert_z_about(void)
{
  const gchar *authors[] = { invert_z_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  invert_z_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void invert_z_configure(void)
{
	if(invert_z_pi.app)
		invert_z_pi.app->message("invert_z Plugin Configuration");
}

// cleanup-Function
static void invert_z_cleanup(void)
{
	PI_DEBUG (DBG_L2, "invert_z Plugin Cleanup");
}

// run-Function
static gboolean invert_z_run(Scan *Src, Scan *Dest)
{
	Dest->mem2d->ConvertFrom(
		Src->mem2d, 
		0, 0, 
		0, 0, 
		Dest->mem2d->GetNx(), Dest->mem2d->GetNy()
		);


	for (int line = 0; line < Src->mem2d->GetNy(); line++)
		for (int col = 0; col < Src->mem2d->GetNx(); col++)
			Dest->mem2d->data->Zmul(-1., col, line);

	return MATH_OK;
}
