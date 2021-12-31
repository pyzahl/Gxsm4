/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=max
pluginmenuentry 	=Max
menupath		=Math/Arithmetic/
entryplace		=Arithmetic
shortentryplace	=AR
abouttext		=max of two sources
smallhelp		=max of two sources
longhelp		=max
 * 
 * Gxsm Plugin Name: max.C
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
% PlugInDocuCaption: Max of two sources
% PlugInName: max
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Arithmetic/Max

% PlugInDescription
This filter merges two (same sized and aligned) scans by using the max
$Z$ value of source one (actice) and two (X) as resulting $Z$.

% PlugInUsage
Select two same sized sources: One should be "Active" and the other in
mode "X", assure there is only one mode "X" channel around -- always
the first "X" marked channel (lowest channel number) will be used!
And run \GxsmMenu{Math/Arithmetic/Max}

% OptPlugInSources
The active channel and X-channel are used.

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

// Plugin Prototypes
static void max_init( void );
static void max_about( void );
static void max_configure( void );
static void max_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
// #define GXSM_ONE_SRC_PLUGIN__DEF
#define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean max_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean max_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin max_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "max-"
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
  g_strdup("max"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-arithmetic-section",
  // Menuentry
  N_("Max"),
  // help text shown on menu
  N_("max"),
  // more info...
  "max of two sources",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  max_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  max_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  max_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  max_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin max_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin max_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin max_m1s_pi
#else
 GxsmMathTwoSrcPlugin max_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   max_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm max Plugin\n\n"
                                   "max of two sources");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  max_pi.description = g_strdup_printf(N_("Gxsm MathOneArg max plugin %s"), VERSION);
  return &max_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &max_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &max_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void max_init(void)
{
  PI_DEBUG (DBG_L2, "max Plugin Init");
}

// about-Function
static void max_about(void)
{
  const gchar *authors[] = { max_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  max_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void max_configure(void)
{
  if(max_pi.app)
    max_pi.app->message("max Plugin Configuration");
}

// cleanup-Function
static void max_cleanup(void)
{
  PI_DEBUG (DBG_L2, "max Plugin Cleanup");
}

// run-Function
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 static gboolean max_run(Scan *Src, Scan *Dest)
#else
 static gboolean max_run(Scan *Src1, Scan *Src2, Scan *Dest)
#endif
{
	int line, col;

	if(Src1->data.s.nx != Src2->data.s.nx || Src1->data.s.ny != Src2->data.s.ny)
		return MATH_SELECTIONERR;

	for(line=0; line<Dest->mem2d->GetNy (); line++)
		for(col=0; col<Dest->mem2d->GetNx (); col++){
			double z1 = Src1->mem2d->GetDataPkt (col, line);
			double z2 = Src2->mem2d->GetDataPkt (col, line);
			if (z2 < z1)
				Dest->mem2d->PutDataPkt(z1, col, line);
			else
				Dest->mem2d->PutDataPkt(z2, col, line);
		}
  return MATH_OK;
}


