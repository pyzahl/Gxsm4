/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sourceforge.net
pluginname		=timedomfftfilter
pluginmenuentry 	=IFT 2D
menupath		=math-filter2d-section
entryplace		=Filter _2D
shortentryplace	=F2D
abouttext		=inverse 2D FT
smallhelp		=inverse 2D FT
longhelp		=inverse 2D FT
 * 
 * Gxsm Plugin Name: timedomfftfilter.C
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
% PlugInDocuCaption: Time Domain FFT Filter
% PlugInName: timedomfftfilter
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sourceforge.net
% PlugInMenuPath: Math/Filter 2D/t dom filter

% PlugInDescription
This filter acts in one dimension on real time domain of the whole
scan. To use it you need the gap-less data of all times, i.e. the
forward and backward scan is needed and the scan data should also be
gap-less and real time at the turn-over points at the end/start of
every scan lines -- this is the case for the "SignalRanger"
implementation.

\GxsmNote{Backward scan data must be in reverse (mirrored in X), as
provided by the SignalRanger.}

\GxsmNote{Any X-scan directional slope will be automatically removed
in time domain by this filter, also the scan DC component will be
eliminated.}

The filter assembles a one dimensional data set in time domain from
scan start to scan end using forward and backward scan data
sets. Transforms this into frequency domain, eliminates all multiples of
scan-width frequency comnponents (tilt in scan direction) and cuts out
the user provided band, then transforms it back into time domain and
reassembles one double width images containing forward and backward
data.


% PlugInUsage
Activate a channel containing the forward $\rightarrow$ scan data and
put the channel containing the backward $\leftarrow$ scan data into
mode X. Then execute the filter \GxsmMenu{Math/Filter 1D/t dom
filter}. It will ask for the stop badn position and half width in
inverse pixels.

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
This plugin is under construction.

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
static void timedomfftfilter_init( void );
static void timedomfftfilter_about( void );
static void timedomfftfilter_configure( void );
static void timedomfftfilter_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean timedomfftfilter_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean timedomfftfilter_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin timedomfftfilter_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "timedomfftfilter-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-F2D",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup("time domain filter"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-filter1d-section",
  // Menuentry
  N_("t dom filter"),
  // help text shown on menu
  N_("time domain filter"),
  // more info...
  "time domain fft filter",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  timedomfftfilter_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  timedomfftfilter_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  timedomfftfilter_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  timedomfftfilter_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin timedomfftfilter_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin timedomfftfilter_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin timedomfftfilter_m1s_pi
#else
 GxsmMathTwoSrcPlugin timedomfftfilter_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   timedomfftfilter_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm timedomfftfilter Plugin\n\n"
                                   "inverse 2D FT");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  timedomfftfilter_pi.description = g_strdup_printf(N_("Gxsm MathOneArg timedomfftfilter plugin %s"), VERSION);
  return &timedomfftfilter_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &timedomfftfilter_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &timedomfftfilter_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void timedomfftfilter_init(void)
{
  PI_DEBUG (DBG_L2, "Plugin Init" );
}

// about-Function
static void timedomfftfilter_about(void)
{
  const gchar *authors[] = { timedomfftfilter_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  timedomfftfilter_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void timedomfftfilter_configure(void)
{
  if(timedomfftfilter_pi.app)
    timedomfftfilter_pi.app->message("timedomfftfilter Plugin Configuration");
}

// cleanup-Function
static void timedomfftfilter_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Plugin Cleanup");
}

// run-Function
static gboolean timedomfftfilter_run(Scan *Src1, Scan *Src2, Scan *Dest)
{
	XSM_DEBUG (DBG_L3, "TD filter");

	if (Src1->mem2d->GetNx () != Src2->mem2d->GetNx ()
		|| Src1->mem2d->GetNy () != Src2->mem2d->GetNy ()) {
		return MATH_SIZEERR;
	}

	main_get_gapp()->progress_info_new ("Time Domain FFT Band Filter", 1);
	main_get_gapp()->progress_info_set_bar_fraction (0.);
	main_get_gapp()->progress_info_set_bar_text ("Step");

	main_get_gapp()->progress_info_set_bar_fraction (0.1);

	int length = Src1->mem2d->GetNx() * 2 * Src1->mem2d->GetNy();
	int xlen  = length/2+1;
	int xlen2 = 2*xlen;
	fftw_complex *dat =  (fftw_complex*) fftw_malloc (sizeof(fftw_complex) * xlen);
	double *in  = (double *) dat;
	double *out = (double *) dat;
	
	if (dat == NULL) {
		XSM_DEBUG (DBG_L3, "F2D FFT: libfftw: Error allocating memory for complex data");
		return MATH_NOMEM;
	}

	memset(out, 0, sizeof(out));
	Dest->mem2d->Resize (Src1->mem2d->GetNx () * 2, Src1->mem2d->GetNy (), 1, ZD_DOUBLE);
	// set "Complex" layer param defaults
	Dest->data.s.nvalues=1;
	Dest->data.s.ntimes=1;
	Dest->mem2d->data->SetVLookup(0, 0);

	Dest->data.s.x0 = 0.;
	Dest->data.s.y0 = 0.;
	Dest->data.s.dx = Src1->data.s.dx;
	Dest->data.s.dy = Src1->data.s.dy;
	Dest->data.s.rx = 2.*Src1->data.s.rx;
	Dest->data.s.ry = Src1->data.s.ry;

	Dest->mem2d->data->MkXLookup (-Src1->data.s.rx, Src1->data.s.rx);
	Dest->mem2d->data->MkYLookup (-Src1->data.s.ry/2, Src1->data.s.ry/2);

	// join images -> + <- to one long time domian string

	int ti=0;
	// convert image data to fftw_complex
	for (int line=0; line < Src1->mem2d->GetNy(); line++){
		for (int col=0; col < Src1->mem2d->GetNx(); col++)
			in[ti++] = Src1->mem2d->GetDataPkt(col,line);
		for (int col=0; col < Src2->mem2d->GetNx(); col++)
			in[ti++] = Src2->mem2d->GetDataPkt(col,line);
	}
	main_get_gapp()->progress_info_set_bar_fraction (0.2);

	std::ofstream f;
	f.open("/tmp/timedom.asc", std::ios::out | std::ios::trunc);
	for (ti = 0; ti < length; ++ti)
		f << ti << " " << in[ti] << std::endl;
	f.close ();

	std::cout << "tdom plan fwd" << std::endl;
	main_get_gapp()->progress_info_set_bar_fraction (0.22);

	// create plan for in-place forward transform
	fftw_plan plan    = fftw_plan_dft_r2c_1d (length, out, dat, FFTW_ESTIMATE);

	if (plan == NULL) {
		XSM_DEBUG (DBG_L3, "F1D FFT: libfftw: Error creating plan");
		return MATH_LIB_ERR;
	}

	std::cout << "tdom exec fwd" << std::endl;
	main_get_gapp()->progress_info_set_bar_fraction (0.24);

	// compute 1D transform using in-place fourier transform
	fftw_execute (plan);

	std::cout << "tdom dump spec" << std::endl;
	main_get_gapp()->progress_info_set_bar_fraction (0.4);

	UnitObj *ipix = new UnitObj("1","1","g","inv pixel");
	ipix->SetAlias ("Unity");

	std::cout << "tdom new pc" << std::endl;
	ProfileControl *pc = new ProfileControl (main_get_gapp() -> get_app (),
						 "Time Spectrum of Scan, display cut off at length/10", 
						 length/2/10, 
						 ipix, Src1->data.Zunit,
						 0., length/2/10., "ScanTimeSpectrum");

	std::cout << "tdom new pc OK" << std::endl;
	f.open("/tmp/freqdom.asc", std::ios::out | std::ios::trunc);
	for (ti = 0; ti < length/2/10; ++ti){
		double psdval = (c_re(dat[ti])*c_re(dat[ti])+c_im(dat[ti])*c_im(dat[ti]))/length/length;
		f << ti << " " << psdval << std::endl;	
		if (ti % 100 == 0)
			std::cout << ".";
		// auto remove tilt/scan x frequency
		for (int slp = 1; slp < 100; slp++)
			if ((ti > (Src1->mem2d->GetNx () * slp-1)) && (ti < (Src1->mem2d->GetNx () * slp+1)))
				psdval=0.;
		pc->AddPoint (ti, psdval);
	}
	f.close ();

	std::cout << "tdom drw pc" << std::endl;
	pc->UpdateArea ();
	pc->show ();

	timedomfftfilter_pi.app->xsm->AddProfile (pc); // give it to Surface, it takes care about removing it...
	pc->unref (); // nothing depends on it, so decrement refcount

	main_get_gapp()->progress_info_set_bar_fraction (0.45);


	double stopbc = 6400.;
	main_get_gapp()->ValueRequest("Filter stop band center", "inv-pixel", "Stop Band Position",
			   main_get_gapp()->xsm->Unity, 0., 1e6,".0f", &stopbc);

	double bandw2 = 2500.;
	main_get_gapp()->ValueRequest("Filter stop band width/2", "inv-pixel", "Stop Band half width",
			   main_get_gapp()->xsm->Unity, 0., 1e5,".0f", &bandw2);



// -- apply filter --

	f.open("/tmp/freqdomX.asc", std::ios::out | std::ios::trunc);
	c_re(dat[0]) = 0.;
	c_im(dat[0]) = 0.;
	c_re(dat[1]) = 0.;
	c_im(dat[1]) = 0.;
	for (ti = 0; ti < length/2/10; ++ti){
		if (fabs(ti-(int)stopbc) < (int)bandw2){
			c_re(dat[ti]) = 0.;
			c_im(dat[ti]) = 0.;
		}
		for (int slp = 1; slp < 100; slp++)
			if ((ti > (Src1->mem2d->GetNx () * slp-1)) && (ti < (Src1->mem2d->GetNx () * slp+1))){
				c_re(dat[ti]) = 0.;
				c_im(dat[ti]) = 0.;
			}
		f << ti << " " << (1.+(c_re(dat[ti])*c_re(dat[ti])+c_im(dat[ti])*c_im(dat[ti]))/length/length) << std::endl;
	}
	f.close ();

	main_get_gapp()->progress_info_set_bar_fraction (0.5);
	std::cout << "tdom aply filter plan" << std::endl;

	// destroy plan
	fftw_destroy_plan(plan); 

	// create plan for in-place forward transform
	plan    = fftw_plan_dft_c2r_1d (length, dat, out, FFTW_ESTIMATE);

	if (plan == NULL) {
		XSM_DEBUG (DBG_L3, "F2D FFT: libfftw: Error creating plan");
		return MATH_LIB_ERR;
	}

	main_get_gapp()->progress_info_set_bar_fraction (0.6);
	std::cout << "tdom exec rev" << std::endl;

	// compute 2D transform using in-place fourier transform
	fftw_execute (plan);

	main_get_gapp()->progress_info_set_bar_fraction (0.8);
	std::cout << "tdom gen img" << std::endl;

	// convert complex data to image
	// and flip image that spatial freq. (0,0) is in the middle
	// (quadrant swap using the QSWP macro defined in xsmmath.h)
  
	XSM_DEBUG (DBG_L3, "F1D FFT rev t dom to img");
	
	ti=0;
	for (int line=0; line < Dest->mem2d->GetNy(); line++) {
		for (int i,col=0; col < Dest->mem2d->GetNx(); col++) {
			Dest->mem2d->PutDataPkt(out[ti++]/length, col, line);
		}
	}

	main_get_gapp()->progress_info_set_bar_fraction (1.);

	// destroy plan
	fftw_destroy_plan(plan); 

	// free real/complex data memory
	fftw_free (dat);

	main_get_gapp()->progress_info_close ();

	return MATH_OK;
}


