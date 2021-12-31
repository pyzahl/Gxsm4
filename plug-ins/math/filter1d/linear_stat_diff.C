/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=linear_stat_diff
pluginmenuentry 	=Lin stat diff
menupath		=Math/Filter 1D/
entryplace		=Filter 1D
shortentryplace	=F1D
abouttext		=Linear stationary differentation
smallhelp		=Linear stationary differentation
longhelp		=Linear stationary differentation
 * 
 * Gxsm Plugin Name: linear_stat_diff.C
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
% PlugInDocuCaption: Linear stationary differentation
% PlugInName: linear_stat_diff
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Filter 1D/Lin stat diff

% PlugInDescription
Edge enhancement via differentation as follows:
\[ I_i = \frac{1}{9}\sum_{k=i-4}^{i+4}Z_k \]
\[ Z'_i = \frac{1}{4}\frac{Z_i - I_i }
          {\sqrt{\frac{1}{2}\sum_{k=i-4}^{i+4}\left(Z_k-I_k\right)^2}}
        + \frac{I_i}{2}
\]

% PlugInUsage
Call \GxsmMenu{Math/Filter 1D/Lin stat diff}.

% OptPlugInSources
The active channel is used as data source.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

%% OptPlugInRefs
%Any references?

%% OptPlugInKnownBugs
%Are there known bugs? List! How to work around if not fixed?

%% OptPlugInNotes
%If you have any additional notes

%% OptPlugInHints
%Any tips and tricks?

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

// Plugin Prototypes
static void linear_stat_diff_init( void );
static void linear_stat_diff_about( void );
static void linear_stat_diff_configure( void );
static void linear_stat_diff_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean linear_stat_diff_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean linear_stat_diff_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin linear_stat_diff_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "linear_stat_diff-"
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
  g_strdup("Linear stationary differentation"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-filter1d-section",
  // Menuentry
  N_("Lin stat diff"),
  // help text shown on menu
  N_("Linear stationary differentation"),
  // more info...
  "Linear stationary differentation",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  linear_stat_diff_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  linear_stat_diff_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  linear_stat_diff_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  linear_stat_diff_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin linear_stat_diff_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin linear_stat_diff_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin linear_stat_diff_m1s_pi
#else
 GxsmMathTwoSrcPlugin linear_stat_diff_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   linear_stat_diff_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm linear_stat_diff Plugin\n\n"
                                   "Linear stationary differentation");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  linear_stat_diff_pi.description = g_strdup_printf(N_("Gxsm MathOneArg linear_stat_diff plugin %s"), VERSION);
  return &linear_stat_diff_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_for_all_vt_plugin_info( void ) {
  return &linear_stat_diff_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &linear_stat_diff_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void linear_stat_diff_init(void)
{
  PI_DEBUG (DBG_L2, "linear_stat_diff Plugin Init");
}

// about-Function
static void linear_stat_diff_about(void)
{
  const gchar *authors[] = { linear_stat_diff_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  linear_stat_diff_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void linear_stat_diff_configure(void)
{
  if(linear_stat_diff_pi.app)
    linear_stat_diff_pi.app->message("linear_stat_diff Plugin Configuration");
}

// cleanup-Function
static void linear_stat_diff_cleanup(void)
{
	PI_DEBUG (DBG_L2, "linear_stat_diff Plugin Cleanup");
}

// run-Function
static gboolean linear_stat_diff_run(Scan *Src, Scan *Dest)
{
	double stda, sum, dif;

	if (Src->mem2d->GetNx () < 5) 
		return MATH_SIZEERR;

	double *i1 = new double[Src->mem2d->GetNx ()];
	double *i2 = new double[Src->mem2d->GetNx ()];

	for(int line=0; line < Dest->mem2d->GetNy (); ++line){
		for(int i=4; i < Dest->mem2d->GetNx ()-4; ++i){
			stda = 0.;
			sum = 0.;
			for(int j=i-4; j <= i+4; ++j)
				sum += Src->mem2d->GetDataPkt (j, line);
			sum /= 9.;
			i1[i] = sum;
			for(int j=i-4; j <= i+4; ++j){
				dif = Src->mem2d->GetDataPkt (j, line) - sum;
				stda += dif * dif;
			}
			i2[i] = sqrt (stda/9.);
		}
		for(int i=0; i<4; ++i){
			i1[i] = i1[4];
			i2[i] = i2[4];
		}
		for(int i=Dest->mem2d->GetNx ()-4; i<Dest->mem2d->GetNx (); ++i){
			i1[i] = i1[Dest->mem2d->GetNx ()-4-1];
			i2[i] = i2[Dest->mem2d->GetNx ()-4-1];
		}

		for(int col=0; col < Dest->mem2d->GetNx (); ++col)
			Dest->mem2d->PutDataPkt (25.*(Src->mem2d->GetDataPkt (col, line)-i1[col])
						 /(i2[col]+2.) + i1[col]/2., 
						 col, line);
	}
	delete[] i2;
	delete[] i1;
	
	return MATH_OK;
}
