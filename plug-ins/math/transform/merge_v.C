/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=merge_v
pluginmenuentry 	=Merge V
menupath		=Math/Transformations/
entryplace		=Transformations
shortentryplace	=TR
abouttext		=Merge two scans vertically.
smallhelp		=Merge two scans of same width vertically.
longhelp		=erge two scans of same width vertically.
 * 
 * Gxsm Plugin Name: merge_v.C
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
% PlugInDocuCaption: Vertical merge
% PlugInName: merge_v
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Transformations/Merge V

% PlugInDescription
Used to vertically merge two scans together. The scan are expected to have same width.

% PlugInUsage
Call \GxsmMenu{Math/Transformations/Merge V}.

% OptPlugInSources
The active and X channel are merged.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

%% OptPlugInNotes
%The scan are expected to have same height.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

// Plugin Prototypes
static void merge_v_init( void );
static void merge_v_about( void );
static void merge_v_configure( void );
static void merge_v_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
// #define GXSM_ONE_SRC_PLUGIN__DEF
#define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean merge_v_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean merge_v_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin merge_v_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "merge_v-"
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
  g_strdup("Vertically merge two scans of same width."),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-transformations-section",
  // Menuentry
  N_("Merge V"),
  // help text shown on menu
  N_("Vertical merge"),
  // more info...
  "Vertically merge two scans of same width.",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  merge_v_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  merge_v_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  merge_v_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  merge_v_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin merge_v_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin merge_v_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin merge_v_m1s_pi
#else
 GxsmMathTwoSrcPlugin merge_v_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   merge_v_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm merge_v Plugin\n\n"
                                   "Vertically merge two scan of same width.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  merge_v_pi.description = g_strdup_printf(N_("Gxsm MathOneArg merge_v plugin %s"), VERSION);
  return &merge_v_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_no_same_size_check_plugin_info( void ) { 
  return &merge_v_m2s_pi; 
}

/* Here we go... */
// init-Function
static void merge_v_init(void)
{
  PI_DEBUG (DBG_L2, "merge_v Plugin Init");
}

// about-Function
static void merge_v_about(void)
{
  const gchar *authors[] = { merge_v_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  merge_v_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void merge_v_configure(void)
{
	if(merge_v_pi.app)
		merge_v_pi.app->message("merge_v Plugin Configuration");
}

// cleanup-Function
static void merge_v_cleanup(void)
{
	PI_DEBUG (DBG_L2, "merge_v Plugin Cleanup");
}

// run-Function
static gboolean merge_v_run(Scan *Src1, Scan *Src2, Scan *Dest)
{
	if(Src1->mem2d->GetNx() != Src2->mem2d->GetNx())
		return MATH_SELECTIONERR;

	Dest->mem2d->Resize(Src1->mem2d->GetNx(), Src1->mem2d->GetNy() + Src2->mem2d->GetNy());
	Dest->data.s.nx = Dest->mem2d->GetNx();
	Dest->data.s.ny = Dest->mem2d->GetNy();
	Dest->data.s.ry = Src1->data.s.ry + Src2->data.s.ry;

	Dest->mem2d->CopyFrom(Src1->mem2d, 0,0, 0,0, Src1->mem2d->GetNx(),Src1->mem2d->GetNy());
	Dest->mem2d->CopyFrom(Src2->mem2d, 0,0, 0,Src1->mem2d->GetNy(), Src2->mem2d->GetNx(),Src2->mem2d->GetNy());

	return MATH_OK;
}


