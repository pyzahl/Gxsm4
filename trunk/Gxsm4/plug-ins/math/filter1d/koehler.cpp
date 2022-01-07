/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=koehler
pluginmenuentry 	=Koehler
menupath		=Math/Filter 1D/
entryplace		=Filter 1D
shortentryplace	=F1D
abouttext		=This is the Koehler filter, it does 1D edge enhancement.
smallhelp		=Koehler filter
longhelp		=Koehler filter
 * 
 * Gxsm Plugin Name: koehler.C
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
% PlugInDocuCaption: Koehler filter
% PlugInName: koehler
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Filter 1D/Koehler

% PlugInDescription
\label{PlugIn-F1D-Koehler}
The Koehler filter differentiates the image data line by line (1D).
It uses a local weightened differentation and floating averagening.

Starting with the left value $Z_0$
\[ V_0 = Z_0 \]
and then using $0.92$ of the left and $0.08$ of the following $Z$ value:
\[ V_i = 0.92 V_{i-1} + 0.08 Z_i \quad \text{for all} \quad i\in{1, 2, \dots, N_x-1}\]

The next iteration doese the same starting at the right site with the current results:
\[ V_i = 0.92 V_{i+1} + 0.08 V_i \quad \text{for all} \quad i\in{N_x-2, N_x-3, \dots, 0}\]

Finally the difference of all $Z$ values with the weightened and averaged value is computed:
\[ Z'_i = Z_i - V_i \quad \text{for all} \quad i\in{0, 1, 2, \dots, N_x-1}\]

% PlugInUsage
Call \GxsmMenu{Math/Filter 1D/Koehler}.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

% OptPlugInConfig
No -- the koefficients are constants and can only be changed in the PlugIn itself.

% OptPlugInRefs
Filter is originated to PMSTM and Ulli Koehler?

% OptPlugInNotes
A similar effect (on a limited lenght) is used the the differential view now on the fly.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

// Plugin Prototypes
static void koehler_init( void );
static void koehler_about( void );
static void koehler_configure( void );
static void koehler_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean koehler_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean koehler_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin koehler_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "koehler-"
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
  g_strdup("Koehler filter"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-filter1d-section",
  // Menuentry
  N_("Koehler"),
  // help text shown on menu
  N_("Koehler filter"),
  // more info...
  "Koehler filter",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  koehler_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  koehler_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  koehler_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  koehler_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin koehler_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin koehler_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin koehler_m1s_pi
#else
 GxsmMathTwoSrcPlugin koehler_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   koehler_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm koehler Plugin\n\n"
                                   "This is the Koehler filter, it does 1D edge enhancement.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  koehler_pi.description = g_strdup_printf(N_("Gxsm MathOneArg koehler plugin %s"), VERSION);
  return &koehler_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_for_all_vt_plugin_info( void ) {
  return &koehler_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &koehler_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void koehler_init(void)
{
  PI_DEBUG (DBG_L2, "koehler Plugin Init");
}

// about-Function
static void koehler_about(void)
{
  const gchar *authors[] = { koehler_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  koehler_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void koehler_configure(void)
{
  if(koehler_pi.app)
    koehler_pi.app->message("koehler Plugin Configuration");
}

// cleanup-Function
static void koehler_cleanup(void)
{
  PI_DEBUG (DBG_L2, "koehler Plugin Cleanup");
}

// run-Function
static gboolean koehler_run(Scan *Src, Scan *Dest)
{
	if (!Src || !Dest)
		return 0;

	if (Src->mem2d->GetNx () < 3) 
		return MATH_SIZEERR;

	double Lproz = 0.92;
	double Rproz = 0.08;

	for(int line=0; line < Dest->mem2d->GetNy (); ++line){
		Dest->mem2d->PutDataPkt (Src->mem2d->GetDataPkt (0, line), 0, line);

		for(int col=1; col < Dest->mem2d->GetNx (); ++col)
			Dest->mem2d->PutDataPkt (Lproz * Dest->mem2d->GetDataPkt (col-1, line)
						 + Rproz * Src->mem2d->GetDataPkt (col, line),
						 col, line);

		for(int col=Dest->mem2d->GetNx ()-2; col >= 0; --col)
			Dest->mem2d->PutDataPkt (Lproz * Dest->mem2d->GetDataPkt (col+1, line)
						 + Rproz * Dest->mem2d->GetDataPkt (col, line),
						 col, line);

		for(int col=0; col < Dest->mem2d->GetNx (); ++col)
			Dest->mem2d->PutDataPkt (Src->mem2d->GetDataPkt (col, line)
						 - Dest->mem2d->GetDataPkt (col, line),
						 col, line);
	}

	return MATH_OK;
}


