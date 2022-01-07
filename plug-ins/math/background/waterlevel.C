/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * 
 * Gxsm Plugin Name: waterlevel.C
 * ========================================
 * 
 * Copyright (C) 2004 The Free Software Foundation
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
% PlugInDocuCaption: Waterlevel
% PlugInName: waterlevel
% PlugInAuthor: Andreas Klust
% PlugInAuthorEmail: klust@users.sourceforge.net
% PlugInMenuPath: Math/Background/Waterlevel

% PlugInDescription
This plugin adds a waterlevel to the active scan.  Everything below
this level in the resulting scan will become invisible.  This is achieved
by setting the z value of all points with original z values below the 
waterlevel to the waterlevel: if $z(x,y) < \mbox{waterlevel}$ then 
$z(x,y) = \mbox{waterlevel}$.

% PlugInUsage
Write how to use it.

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
The computation result is placed into an existing math channel, else 
into a new created math channel.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

%% OptPlugInRefs
%Any references?

%% OptPlugInKnownBugs
This plug-in is still under construction!

%% OptPlugInNotes
%If you have any additional notes

%% OptPlugInHints
%Any tips and tricks?

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

// Plugin Prototypes
static void waterlevel_init( void );
static void waterlevel_about( void );
static void waterlevel_configure( void );
static void waterlevel_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean waterlevel_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean waterlevel_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin waterlevel_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "waterlevel-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-BG",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup("Adds a waterlevel to scan, i.e., everything below this level becomes invisible."),                   
  // Author(s)
  "Andreas Klust",
  // Menupath to position where it is appendet to
  "math-background-section",
  // Menuentry
  N_("Waterlevel"),
  // help text shown on menu
  N_("Adds a waterlevel to scan, i.e., everything below this level becomes invisible."),
  // more info...
  "Adds a waterlevel to scan.",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  waterlevel_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  waterlevel_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  waterlevel_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  waterlevel_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin waterlevel_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin waterlevel_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin waterlevel_m1s_pi
#else
 GxsmMathTwoSrcPlugin waterlevel_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   waterlevel_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm waterlevel Plugin\n\n"
                                   "Adds a waterlevel to scan.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  waterlevel_pi.description = g_strdup_printf(N_("Gxsm MathOneArg waterlevel plugin %s"), VERSION);
  return &waterlevel_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &waterlevel_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &waterlevel_m2s_pi; 
}
#endif

/* Here we go... */
double waterlevel = 0;

// init-Function
static void waterlevel_init(void)
{
  PI_DEBUG (DBG_L2, "Plugin Init" );
}

// about-Function
static void waterlevel_about(void)
{
  const gchar *authors[] = { waterlevel_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  waterlevel_pi.name,
			 "version", VERSION,
			 N_("(C) 2004 the Free Software Foundation"),
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void waterlevel_configure(void)
{
  if(waterlevel_pi.app)
    waterlevel_pi.app->message("waterlevel Plugin Configuration");
}

// cleanup-Function
static void waterlevel_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Plugin Cleanup");
}

// run-Function
static gboolean waterlevel_run(Scan *Src, Scan *Dest)
{
	if ((Src ? Src->mem2d->get_t_index ():0) < 1 && (Src ? Src->mem2d->GetLayer ():0) < 2){
		main_get_gapp()->ValueRequest("Waterlevel", "Level", "Set waterlevel",
				   main_get_gapp()->xsm->Unity, -1e4, 1e4, ".0f", &waterlevel);
		if (!Src || !Dest)
			return 0;
	}

	for(int line=0; line < Dest->mem2d->GetNy (); line++)
		for(int col=0; col < Dest->mem2d->GetNx (); col++) {
			if(Src->mem2d->GetDataPkt(col, line) < waterlevel)
				Dest->mem2d->PutDataPkt (waterlevel, col, line);
			else
				Dest->mem2d->PutDataPkt (Src->mem2d->GetDataPkt(col, line), col, line);
		}

  return MATH_OK;
}


