/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Stefan
pluginname		=stepcount
pluginmenuentry 	=Stepcounter
menupath		=Math/Statistics/
entryplace		=Statistics
shortentryplace	=ST
abouttext		=Stepcounter
smallhelp		=Stepcounter - counts step of height 256
longhelp		=Step counter
 * 
 * Gxsm Plugin Name: stepcount.C
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
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Stepcount
% PlugInName: baseinfo
% PlugInAuthor: Stefan Schr\"oder
% PlugInAuthorEmail: stefan_fkp@users.sf.net
% PlugInMenuPath: Math/Statistics/stepcounter

% PlugInDescription
This is a primitive plugin for the analysis of artificially
generated scans. It counts the number of steps in x-direction,
higher than 255 counts.

% PlugInUsage
Use with
active scan or a selected rectangle within the active scan.

% OptPlugInSources
You need one active scan.

% OptPlugInObjects
If a rectangle is selected the calculated information applies to the
content of the rectangle. Otherwise, the whole scan is analyzed.

% OptPlugInDest
The result is printed on the console, so you better have one open!

% OptPlugInConfig
None.

% OptPlugInKnownBugs
None

% OptPlugInNotes
Is there interest in a more general approach? 

% EndPlugInDocuSection
 *
--------------------------------------------------------------------------------
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

// Plugin Prototypes
static void stepcount_init( void );
static void stepcount_about( void );
static void stepcount_configure( void );
static void stepcount_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean stepcount_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean stepcount_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin stepcount_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  g_strdup ("stepcount-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
	    "-ST"),
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup("Step counter"),                   
  // Author(s)
  "Stefan",
  // Menupath to position where it is appendet to
  "math-statistics-section",
  // Menuentry
  N_("Stepcounter"),
  // help text shown on menu
  N_("Step counter"),
  // more info...
  "Stepcounter - counts step of height 256",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  stepcount_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  stepcount_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  stepcount_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  stepcount_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin stepcount_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin stepcount_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin stepcount_m1s_pi
#else
 GxsmMathTwoSrcPlugin stepcount_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   stepcount_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm stepcount Plugin\n\n"
                                   "Stepcounter");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  stepcount_pi.description = g_strdup_printf(N_("Gxsm MathOneArg stepcount plugin %s"), VERSION);
  return &stepcount_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &stepcount_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &stepcount_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void stepcount_init(void)
{
  PI_DEBUG (DBG_L2, "stepcount Plugin Init");
}

// about-Function
static void stepcount_about(void)
{
  const gchar *authors[] = { stepcount_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  stepcount_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void stepcount_configure(void)
{
  if(stepcount_pi.app)
    stepcount_pi.app->message("stepcount Plugin Configuration");
}

// cleanup-Function
static void stepcount_cleanup(void)
{
  PI_DEBUG (DBG_L2, "stepcount Plugin Cleanup");
}

// run-Function
 static gboolean stepcount_run(Scan *Src, Scan *Dest)
{
  int akt, nachbar, line, col;
  int sumsteps_up, sumsteps_down;
  int left, right, top, bottom;

  MOUSERECT msr;
  MkMausSelect (Src, &msr, Src->mem2d->GetNx(), Src->mem2d->GetNy());

  if( msr.xSize  < 1 || msr.ySize < 1){
    left   = 0;
    right  = Src->mem2d->GetNx();
    top    = 0;
    bottom = Src->mem2d->GetNy();
  }
  else{
    left   = msr.xLeft;
    right  = msr.xRight;
    top    = msr.yTop;
    bottom = msr.yBottom;
  }
  sumsteps_up = sumsteps_down = 0;

  for(col = left; col < (right-1); col++){
    for(line = top; line < (bottom-1); line++){

        akt = (int)Src->mem2d->GetDataPkt(col, line);
        nachbar = (int)Src->mem2d->GetDataPkt(col+1, line);

        if(nachbar - akt > 256){
           sumsteps_up ++;
        }
        if(akt - nachbar > 256){
           sumsteps_down ++;
        }
    } 
  }
  PI_DEBUG (DBG_L2, "Summe der Stufen (auf) = " << sumsteps_up );
  PI_DEBUG (DBG_L2, "Summe der Stufen (ab ) = " << sumsteps_down );

  return MATH_OK;
}

