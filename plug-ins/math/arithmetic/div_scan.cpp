/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=div_scan
pluginmenuentry 	=Div X
menupath		=Math/Arithmetic/
entryplace		=Arithmetic
shortentryplace	=AR
abouttext		=Divide Active by X channel
smallhelp		=My new Plugin does useful things
longhelp		=This is a detailed help for my Plugin.
 * 
 * Gxsm Plugin Name: div_scan.C
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
% PlugInDocuCaption: Divide scans
% PlugInName: div_scan
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Arithmetic/Div X

% PlugInDescription
Divide the Z-values of two scans.

% PlugInUsage
Call \GxsmMenu{Math/Arithmetic/Div X}.

% OptPlugInSources
The active channel is divided by the X channel.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel. The result is of type \GxsmEmph{float}.

% OptPlugInNotes
Both scans are required to have the same size in pixels. Thers is an
$\epsilon = 10^{-8}$ defined as minimal divisor, if the absolute value
of the divisor is smaller than $\epsilon$ the original divident data
is kept unchanged.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"

// Plugin Prototypes
static void div_scan_init( void );
static void div_scan_about( void );
static void div_scan_configure( void );
static void div_scan_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
// #define GXSM_ONE_SRC_PLUGIN__DEF
#define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean div_scan_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean div_scan_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin div_scan_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "div_scan-"
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
  g_strdup("This is a detailed help for my Plugin."),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-arithmetic-section",
  // Menuentry
  N_("Div X"),
  // help text shown on menu
  N_("This is a detailed help for my Plugin."),
  // more info...
  "My new Plugin does useful things",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  div_scan_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  div_scan_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  div_scan_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  div_scan_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin div_scan_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin div_scan_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin div_scan_m1s_pi
#else
 GxsmMathTwoSrcPlugin div_scan_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   div_scan_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm div_scan Plugin\n\n"
                                   "Divide Active by X channel");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  div_scan_pi.description = g_strdup_printf(N_("Gxsm MathOneArg div_scan plugin %s"), VERSION);
  return &div_scan_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &div_scan_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &div_scan_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void div_scan_init(void)
{
  PI_DEBUG (DBG_L2, "div_scan Plugin Init");
}

// about-Function
static void div_scan_about(void)
{
  const gchar *authors[] = { div_scan_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  div_scan_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void div_scan_configure(void)
{
  if(div_scan_pi.app)
    div_scan_pi.app->message("div_scan Plugin Configuration");
}

// cleanup-Function
static void div_scan_cleanup(void)
{
  PI_DEBUG (DBG_L2, "div_scan Plugin Cleanup");
}

// run-Function
static gboolean div_scan_run(Scan *Src1, Scan *Src2, Scan *Dest)
{
	int merr=0;
	double divisor;

	const double eps = 1e-8;

	if(   Src1->mem2d->GetNx() != Src2->mem2d->GetNx() 
	      || Src1->mem2d->GetNy() != Src2->mem2d->GetNy())
		return MATH_SIZEERR;

	Dest->mem2d->Resize (Dest->mem2d->GetNx(), Dest->mem2d->GetNy(), ZD_FLOAT);

	for(int line=0; line<Dest->mem2d->GetNy (); ++line)
		for(int col=0; col<Dest->mem2d->GetNx (); ++col){
			divisor = Src2->mem2d->GetDataPkt (col, line);
			if (fabs (divisor) > eps)
				Dest->mem2d->data->Zdiv(divisor, col, line);
		}

	for(int v=0; v<Dest->mem2d->GetNv () && v<Src1->mem2d->GetNv () && v<Src2->mem2d->GetNv (); ++v)
		for(int line=0; line<Dest->mem2d->GetNy (); ++line)
			for(int col=0; col<Dest->mem2d->GetNx (); ++col){
				divisor = Src2->mem2d->GetDataPkt (col, line, v);
				if (fabs (divisor) > eps)
					Dest->mem2d->PutDataPkt(
						Src1->mem2d->GetDataPkt (col, line, v) / divisor,
						col, line, v);
				else
					Dest->mem2d->PutDataPkt(
						Src1->mem2d->GetDataPkt (col, line, v) > 0. ? 99999e30:-99999e30,
						col, line, v);
			}
	return MATH_OK;
}


