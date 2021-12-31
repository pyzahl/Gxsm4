/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
 * 
 * Gxsm Plugin Name: parabolregress.C
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
% PlugInDocuCaption: 2nd order scanline correction
% PlugInName: parabolregress
% PlugInAuthor: Stefan Schr\"oder
% PlugInAuthorEmail: stefan_fkp@users.sf.net
% PlugInMenuPath: Math/Background/Line: 2nd order

% PlugInDescription
Second order line by line background correction: The 2nd order best fit of the
Z data of each line is computed and subtracted as background.

This filter can be used for a quick and easy background correction of
sample tilt and possible offset changes inbetween lines. Additionally, the
correction of an overlaying parabola, e.g. due to the geometry of the 
experimental setup (bending piezo tube) is 

% PlugInUsage
Activate a scan and select \GxsmMenu{Math/Background/Line: 2nd order}.

%% OptPlugInSection: replace this by the section caption
%all following lines until next tag are going into this section
%...

%% OptPlugInSubSection: replace this line by the subsection caption
%all following lines until next tag are going into this subsection
%...

%% you can repeat OptPlugIn(Sub)Sections multiple times!

% OptPlugInSources
The active channel is used as data source.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

% OptPlugInDest
The result is put into a new created math channel.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

%% OptPlugInRefs
%Any references?

%% OptPlugInKnownBugs
%Are there known bugs? List! How to work around if not fixed?

% OptPlugInNotes
The algorithm is unchecked.

%% OptPlugInHints
%Any tips and tricks?

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

// Plugin Prototypes
static void parabolregress_init( void );
static void parabolregress_about( void );
static void parabolregress_configure( void );
static void parabolregress_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean parabolregress_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean parabolregress_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin parabolregress_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "parabolregress-"
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
  g_strdup("Line by line 2nd order background correction."),                   
  // Author(s)
  "stefan",
  // Menupath to position where it is appendet to
  "math-background-section",
  // Menuentry
  N_("Line: 2nd order"),
  // help text shown on menu
  N_("Line by line 2nd order regression"),
  // more info...
  "This plugin does a second order regression on the active scan, line by line.",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  parabolregress_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  parabolregress_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  parabolregress_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  parabolregress_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin parabolregress_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin parabolregress_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin parabolregress_m1s_pi
#else
 GxsmMathTwoSrcPlugin parabolregress_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   parabolregress_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm parabolregress Plugin\n");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  parabolregress_pi.description = g_strdup_printf(N_("Gxsm MathOneArg parabolregress plugin %s"), VERSION);
  return &parabolregress_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &parabolregress_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &parabolregress_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void parabolregress_init(void)
{
  PI_DEBUG (DBG_L2, "parabolregress Plugin Init");
}

// about-Function
static void parabolregress_about(void)
{
	const gchar *authors[] = { parabolregress_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  parabolregress_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void parabolregress_configure(void)
{
	if(parabolregress_pi.app)
		parabolregress_pi.app->message("parabolregress Plugin Configuration");
}

// cleanup-Function
static void parabolregress_cleanup(void)
{
	PI_DEBUG (DBG_L2, "parabolregress Plugin Cleanup");
}

// run-Function
static gboolean parabolregress_run(Scan *Src, Scan *Dest)
{
// For more math-access methods have a look at xsmmath.C
// simple example for 1sourced Mathoperation: Source is taken and 100 added.
//  for(int line=0; line < Dest->mem2d->GetNy (); line++)
//   for(int col=0; col < Dest->mem2d->GetNx (); col++)
//   Dest->mem2d->PutDataPkt (Src->mem2d->GetDataPkt (line, col) + 100, line, col);

	PI_DEBUG (DBG_L2, "Bg Poly2nd 1D Plugin parabolregress");

	double M0, M1, M2, li;
	double N = (double)Src->mem2d->GetNx();
	double D = (N*N-1.0)*(N*N-4.0);
	double a, b, c, lb;

	ZData  *SrcZ, *DestZ;
	SrcZ  =  Src->mem2d->data;
	DestZ = Dest->mem2d->data;


	for (int line=0; line < Src->mem2d->GetNy(); line++) {

		SrcZ->SetPtr(0,line);
		M0 = 0.0; M1 = 0.0; M2 = 0.0;

		for (int col = 0; col < Src->mem2d->GetNx(); col++) {
			li = (double)col;
			M0 += lb = SrcZ->GetNext();
			M1 += lb * li;
			M2 += lb * li*li;
		}

		M0 /= N;
		M1 /= N;
		M2 /= N;

		a = (3.*(N+1.)*(N+2.)/D)*((3.*N*N+3.*N+2.)*M0-6*(2*N+1.)*M1+10.*M2);
		b =
			-6./D*(3*(N+1.)*(N+2.)*(2.*N+1.)*M0-2.*(2.*N+1.)*(8.*N+11.)*M1+30.*(N+1.)*M2);
		c = 30./D*((N+1.)*(N+2.)*M0-6.*(N+1.)*M1+6.*M2);

		SrcZ->SetPtr(0,line);
		DestZ->SetPtr(0,line);

		for (int col = 0; col < Src->mem2d->GetNx(); col++) {
			li = (double)col;
			DestZ->SetNext(SrcZ->GetNext() - (a + b*li + c*li*li));
		}
	}
	return MATH_OK;
}


