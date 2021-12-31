/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=short_to_short
pluginmenuentry 	=to short
menupath		=Math/Convert/
entryplace		=Convert
shortentryplace	=CV
abouttext		=Convert to short
smallhelp		=convert to short
longhelp		=Converts a scan to type short.
 * 
 * Gxsm Plugin Name: short_to_short.C
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
% PlugInDocuCaption: Convert to short, apply custom fix filter
% PlugInName: short_to_short
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Convert/to short fix

% PlugInDescription
Convert scan data type to short and apply a custom fix (-32768 $\rightarrow$ +32766).

% PlugInUsage
Call \GxsmMenu{Math/Convert/to short fix}.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInDest
The conversion result is placed into an existing math channel, else
into a new created math channel.

% OptPlugInNote
This is a special temporary hack, only loaded if Instrument is set to SNOM.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

// Plugin Prototypes
static void short_to_short_init( void );
static void short_to_short_about( void );
static void short_to_short_configure( void );
static void short_to_short_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean short_to_short_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean short_to_short_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin short_to_short_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "short_to_short-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-CV",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  "+noHARD +SNOM",
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("Converts a scan to type short, apply fix."),
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-convert-section",
  // Menuentry
  N_("to short fix"),
  // help text shown on menu
  N_("Converts a scan to type short, apply fix."),
  // more info...
  "convert to short, apply fix",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  short_to_short_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  short_to_short_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  short_to_short_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  short_to_short_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin short_to_short_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin short_to_short_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin short_to_short_m1s_pi
#else
 GxsmMathTwoSrcPlugin short_to_short_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   short_to_short_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm short_to_short Plugin\n\n"
                                   "Convert to short");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  short_to_short_pi.description = g_strdup_printf(N_("Gxsm MathOneArg short_to_short plugin %s"), VERSION);
  return &short_to_short_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &short_to_short_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &short_to_short_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void short_to_short_init(void)
{
  PI_DEBUG (DBG_L2, "short_to_short Plugin Init");
}

// about-Function
static void short_to_short_about(void)
{
  const gchar *authors[] = { short_to_short_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  short_to_short_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void short_to_short_configure(void)
{
	if(short_to_short_pi.app)
		short_to_short_pi.app->message("short_to_short Plugin Configuration");
}

// cleanup-Function
static void short_to_short_cleanup(void)
{
	PI_DEBUG (DBG_L2, "short_to_short Plugin Cleanup");
}

// run-Function
static gboolean short_to_short_run(Scan *Src, Scan *Dest)
{
 	Dest->mem2d->Resize (Dest->data.s.nx, Dest->data.s.ny, ZD_SHORT);
	Dest->mem2d->ConvertFrom (Src->mem2d, 
				  0,0, 
				  0,0, 
				  Dest->mem2d->GetNx (), Dest->mem2d->GetNy ());
	Dest->mem2d->data->CopyLookups(Src->mem2d->data);
	Dest->mem2d->copy_layer_information (Src->mem2d);

        for(int line=0; line<Dest->mem2d->GetNy (); line++)
                for(int col=0; col<Dest->mem2d->GetNx (); col++){
                        double z = Dest->mem2d->GetDataPkt (col, line);
			if (z < -32767.99)
				z = 32766.;
			Dest->mem2d->PutDataPkt(z, col, line);
                }

	return MATH_OK;
}


