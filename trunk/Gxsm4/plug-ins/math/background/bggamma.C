/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
pluginname		=bggamma
pluginmenuentry 	=Gamma
menupath		=Math/Background/
entryplace		=Background
shortentryplace	=BG
abouttext		=This Plugin does a gamma scaling of the Data.
smallhelp		=Apply Gamma Korrection to Data
longhelp		=Apply Gamma
 * 
 * Gxsm Plugin Name: bggamma.C
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
% PlugInDocuCaption: Gamma correction
% PlugInName: bggamma
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Background/Gamma

% PlugInDescription
Applys a gamma correction as it is defined here:
\[ Z_{\text{range}} := Z_{\text{minval}} - Z_{\text{maxval}}\]
\[ Z_{\text{math}} = \frac{Z_{\text{range}} Z_{\text{active}}^{\gamma}}
                          {Z_{\text{range}}^{\gamma}} \]

% PlugInUsage
Call \GxsmMenu{Math/Background/Gamma} and give the gamma value
$\gamma$ if prompted.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

// Plugin Prototypes
static void bggamma_init( void );
static void bggamma_about( void );
static void bggamma_configure( void );
static void bggamma_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean bggamma_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean bggamma_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin bggamma_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "bggamma-"
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
  g_strdup("Apply Gamma"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-background-section",
  // Menuentry
  N_("Gamma"),
  // help text shown on menu
  N_("Apply Gammatransformation to Data"),
  // more info...
  "Apply Gammatransformation to the Data",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  bggamma_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  bggamma_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  bggamma_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  bggamma_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin bggamma_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin bggamma_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin bggamma_m1s_pi
#else
 GxsmMathTwoSrcPlugin bggamma_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   bggamma_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm bggamma Plugin\n\n"
                                   "This Plugin does a gamma scaling of the Data:\n"
				   "range/pow(range,gamma)*pow(dataval-datamin,gamma);"
	);

double GammaDefault = 1.;
double GammaLast = 1.;
double gammaval = 1.;

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  bggamma_pi.description = g_strdup_printf(N_("Gxsm MathOneArg bggamma plugin %s"), VERSION);
  return &bggamma_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &bggamma_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &bggamma_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void bggamma_init(void)
{
  PI_DEBUG (DBG_L2, "bggamma Plugin Init");
}

// about-Function
static void bggamma_about(void)
{
  const gchar *authors[] = { bggamma_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  bggamma_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void bggamma_configure(void)
{
  if(bggamma_pi.app)
    bggamma_pi.app->message("bggamma Plugin Configuration");
}

// cleanup-Function
static void bggamma_cleanup(void)
{
  PI_DEBUG (DBG_L2, "bggamma Plugin Cleanup");
}

double GammaFkt(double val, double range){
	double gamma=gammaval;
	if (gamma != 1.)
		return (range/pow(range,gamma)*pow(val,gamma));
	return val;
}

// run-Function
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 static gboolean bggamma_run(Scan *Src, Scan *Dest)
#else
 static gboolean bggamma_run(Scan *Src1, Scan *Src2, Scan *Dest)
#endif
{
	int line, col;
	double hi,lo;
	gammaval = GammaLast;
	
	if (fabs(GammaDefault) > 10.)
		gammaval = GammaDefault;
	else
		bggamma_pi.app->ValueRequest 
			( "Enter Value", "gamma", 
			  "I need the Gamma value.",
			  bggamma_pi.app->xsm->Unity, 
			  0., 10., ".3f", &gammaval
				);
	GammaLast = gammaval;




	int ti=0; 
	int tf=0;
	int vi=0;
	int vf=0;
	gboolean multidim = FALSE;
	
	if (Src->data.s.ntimes != 1 || Src->mem2d->GetNv () != 1){
		multidim = TRUE;
		do {
			ti=vi=0;
			tf=Src->number_of_time_elements ()-1;
			if (tf < 0) tf = 0;
			vf=Src->mem2d->GetNv ()-1;
			main_get_gapp()->setup_multidimensional_data_copy ("Multidimensional Smooth", Src, ti, tf, vi, vf);
		} while (ti > tf || vi > vf);

		main_get_gapp()->progress_info_new ("Multidimenssional Plane Regression", 2);
		main_get_gapp()->progress_info_set_bar_fraction (0., 1);
		main_get_gapp()->progress_info_set_bar_fraction (0., 2);
		main_get_gapp()->progress_info_set_bar_text ("Time", 1);
		main_get_gapp()->progress_info_set_bar_text ("Value", 2);
	}

	int ntimes_tmp = tf-ti+1;
	for (int time_index=ti; time_index <= tf; ++time_index){
		Mem2d *m = Src->mem2d_time_element (time_index);
		if (multidim)
			main_get_gapp()->progress_info_set_bar_fraction ((gdouble)(time_index-ti)/(gdouble)ntimes_tmp, 1);

		Dest->mem2d->Resize (m->GetNx (), m->GetNy (), vf-vi+1, m->GetTyp());

		for (int v_index = vi; v_index <= vf; ++v_index){
			m->SetLayer (v_index);
			Dest->mem2d->SetLayer (v_index-vi);

			m->HiLo(&hi, &lo, FALSE);
	
			for (line=0; line<Dest->mem2d->GetNy (); line++)
				for (col=0; col<Dest->mem2d->GetNx (); col++)
					Dest->mem2d->PutDataPkt 
						(GammaFkt (m->GetDataPkt (col, line)-lo, hi-lo), 
						 col, line);
		}
		Dest->append_current_to_time_elements (time_index-ti, m->get_frame_time ());
	}

	Dest->data.s.ntimes = ntimes_tmp;
	Dest->data.s.nvalues=Dest->mem2d->GetNv ();

	if (multidim){
		main_get_gapp()->progress_info_close ();
		Dest->retrieve_time_element (0);
		Dest->mem2d->SetLayer(0);
	}
	
	return MATH_OK;
}

