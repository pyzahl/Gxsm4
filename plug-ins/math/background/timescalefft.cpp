/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sourceforge.net
pluginname		=timescalefft
pluginmenuentry 	=Timescale FFT
menupath		=math-background-section
entryplace		=_Background
shortentryplace	=BG
abouttext		=whole image Z time scale analysis
smallhelp		=whole image Z time domain analysis
longhelp		=whole image Z time domain analysis
 * 
 * Gxsm Plugin Name: timescalefft.C
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
% PlugInDocuCaption: Full Timescale FFT
% PlugInName: timescalefft
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sourceforge.net
% PlugInMenuPath: Math/Background/Timescale FFT

% PlugInDescription
Linear FT of scan data in time, takes all scan data concat as one long stream.

% PlugInUsage
Activate source channel.

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
#include "app_profile.h"
#include "glbvars.h"
#include "surface.h"


// Plugin Prototypes
static void timescalefft_init( void );
static void timescalefft_about( void );
static void timescalefft_configure( void );
static void timescalefft_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean timescalefft_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean timescalefft_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin timescalefft_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "timescalefft-"
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
  g_strdup("whole image Z time domain analysis"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-background-section",
  // Menuentry
  N_("Timescale FFT"),
  // help text shown on menu
  N_("whole image Z time domain analysis"),
  // more info...
  "whole image Z time domain analysis",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  timescalefft_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  timescalefft_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  timescalefft_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  timescalefft_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin timescalefft_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin timescalefft_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin timescalefft_m1s_pi
#else
 GxsmMathTwoSrcPlugin timescalefft_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   timescalefft_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm timescalefft Plugin\n\n"
                                   "whole image Z time scale analysis");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  timescalefft_pi.description = g_strdup_printf(N_("Gxsm MathOneArg timescalefft plugin %s"), VERSION);
  return &timescalefft_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &timescalefft_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &timescalefft_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void timescalefft_init(void)
{
  PI_DEBUG (DBG_L2, "Plugin Init" );
}

// about-Function
static void timescalefft_about(void)
{
  const gchar *authors[] = { timescalefft_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  timescalefft_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void timescalefft_configure(void)
{
	if(timescalefft_pi.app)
		timescalefft_pi.app->message("timescalefft Plugin Configuration");
}

// cleanup-Function
static void timescalefft_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Plugin Cleanup");
}

// run-Function
 static gboolean timescalefft_run(Scan *Src, Scan *Dest)
{
	int t=0;
	int points = 2 * Src->mem2d->GetNx() * Src->mem2d->GetNy();
	int xl = Src->mem2d->GetNx(); 
	fftw_complex *Z_realspace_time = new fftw_complex[points];
	fftw_complex *Z_timespace_time = new fftw_complex[points];
	fftw_plan plan = fftw_plan_dft_1d (points, Z_realspace_time, Z_timespace_time, FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_plan planback = fftw_plan_dft_1d (points, Z_timespace_time, Z_realspace_time, FFTW_BACKWARD, FFTW_ESTIMATE);

	Dest->mem2d->Resize(Dest->data.s.nx, Dest->data.s.ny, ZD_FLOAT);

	double cutoff = 1e9;
	UnitObj *U = new UnitObj(" "," ","g"," ");
	timescalefft_pi.app->ValueRequest ("Enter Value", "Cut-Off Threashold", "1D Timescale FFT Filter",
					U,
					0., 1e99, "g", &cutoff);

	for (int line=0; line < Src->mem2d->GetNy(); line++) {
		int tl = t;
		Src->mem2d->data->SetPtr(0, line);
		for (int col=0; col < Src->mem2d->GetNx(); col++) {
			c_re (Z_realspace_time [t]) = Src->mem2d->data->GetNext(); // * (1. - cos ((double)col / (double)xl * 2. * M_PI));
			c_im (Z_realspace_time [t]) = 0.;
			++t;
		}
 		for (int col=0; col < Src->mem2d->GetNx(); col++) {
 			c_re (Z_realspace_time [t]) = 0.; // c_re (Z_realspace_time [tl+xl-col]); // copy data -- mirroring
 			c_im (Z_realspace_time [t]) = 0.;
 			++t;
 		}
        }

	// FFTW
	fftw_execute (plan);


	UnitObj *PixU = new UnitObj("#","#","g","Pixel");
	PixU->SetAlias ("Pixel");

	gchar *txt = g_strdup_printf ("Time Domain of Scan");
	ProfileControl *pc = new ProfileControl (main_get_gapp() -> get_app (),
						 txt, 
						 points, 
						 PixU, Src->data.Zunit,
						 0, points-1, "TDS");
	g_free (txt);

	for(int k = 0; k < points; k++){
		double amp = c_re(Z_timespace_time[k])*c_re(Z_timespace_time[k]) + c_im(Z_timespace_time[k])*c_im(Z_timespace_time[k]);
		pc->SetPoint (k, amp);
//		pc->SetPoint (k, c_re(Z_realspace_time[k]));
//		if (amp > 1e9){
		if (amp > cutoff){
			c_re(Z_timespace_time[k]) = 0.;
			c_im(Z_timespace_time[k]) = 0.;
		}
	}

	fftw_execute (planback);

	t = 0;
	for (int line=0; line < Src->mem2d->GetNy(); line++) {
		for (int col=0; col < Src->mem2d->GetNx(); col++) {
			Dest->mem2d->PutDataPkt( c_re(Z_realspace_time[t++])/(double)points, col, line);
		}
		t += xl;
	}

	fftw_destroy_plan (plan);
	fftw_destroy_plan (planback);

	pc->UpdateArea ();
	pc->show ();

	timescalefft_pi.app->xsm->AddProfile (pc); // give it to Surface, it takes care about removing it...
	pc->unref (); // nothing depends on it, so decrement refcount


	// free memory
	delete [] Z_realspace_time;
	delete [] Z_timespace_time;
	
	return MATH_OK;
}


