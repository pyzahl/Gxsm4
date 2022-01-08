/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * 
 * Gxsm Plugin Name: rotate90.C
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
% PlugInDocuCaption: 90$^\circ$ clockwise rotation
% PlugInName: rotate90
% PlugInAuthor: Andreas Klust
% PlugInAuthorEmail: klust@users.sf.net
% PlugInMenuPath: math-transformations-sectionRotate 90deg

% PlugInDescription
This plug-in rotates the active scan clockwise by 90$^\circ$.

% PlugInUsage


%% OptPlugInSection: replace this by the section caption
%all following lines until next tag are going into this section
%...

%% OptPlugInSubSection: replace this line by the subsection caption
%all following lines until next tag are going into this subsection
%...

%% you can repeat OptPlugIn(Sub)Sections multiple times!

%% OptPlugInSources
%The active channel is used as data source.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

%% OptPlugInDest
%The computation result is placed into an existing math channel, else into a new created math channel.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

%% OptPlugInRefs
%Any references?

%% OptPlugInKnownBugs
%Are there known bugs? List! How to work around if not fixed?

%% OptPlugInNotes The plug-in was mainly written to facilitate
background corrections such as line regression of scans with the scan
direction up-down instead of left-right.  Just rottate the scan and
apply the usual background correction functions.

%% OptPlugInHints
%Any tips and tricks?

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"


// Plugin Prototypes
static void rotate90_init( void );
static void rotate90_about( void );
static void rotate90_configure( void );
static void rotate90_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean rotate90_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean rotate90_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin rotate90_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  g_strdup ("rotate90-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
	    "-F2D"),
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("Rotates the image 90deg clockwise."),                   
  // Author(s)
  "Andreas Klust",
  // Menupath to position where it is appendet to
  "math-transformations-section",
  // Menuentry
  N_("Rotate 90deg"),
  // help text shown on menu
  N_("Rotates the image 90deg clockwise."),
  // more info...
  "Rotates the image 90deg clockwise.",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  rotate90_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  rotate90_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  rotate90_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  rotate90_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin rotate90_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin rotate90_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin rotate90_m1s_pi
#else
 GxsmMathTwoSrcPlugin rotate90_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   rotate90_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm rotate90 Plugin\n\n"
                                   "Rotates the image 90deg clockwise.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  rotate90_pi.description = g_strdup_printf(N_("Gxsm MathOneArg rotate90 plugin %s"), VERSION);
  return &rotate90_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_for_all_vt_plugin_info( void ) {
  return &rotate90_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &rotate90_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void rotate90_init(void)
{
  PI_DEBUG (DBG_L2, "Plugin Init" );
}

// about-Function
static void rotate90_about(void)
{
  const gchar *authors[] = { rotate90_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  rotate90_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void rotate90_configure(void)
{
  if(rotate90_pi.app)
    rotate90_pi.app->message("rotate90 Plugin Configuration");
}

// cleanup-Function
static void rotate90_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Plugin Cleanup");
}

// run-Function
static gboolean rotate90_run(Scan *Src, Scan *Dest)
{
	if (!Src || !Dest)
		return 0;

	Dest->mem2d->Resize(Src->mem2d->GetNy (), Src->mem2d->GetNx ());

	for(int line=0; line < Dest->mem2d->GetNy (); line++)
		for(int col=0; col < Dest->mem2d->GetNx (); col++)
			Dest->mem2d->PutDataPkt (Src->mem2d->GetDataPkt (line, col), col, line);

	return MATH_OK;
}


