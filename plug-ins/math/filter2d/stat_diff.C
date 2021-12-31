/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=stat_diff
pluginmenuentry 	=Stat Diff
menupath		=Math/Filter 2D/
entryplace		=Filter 2D
shortentryplace	=F2D
abouttext		=Stationary Differentiation
smallhelp		=Stationary Differentiation
longhelp		=Stationary Differentiation
 * 
 * Gxsm Plugin Name: stat_diff.C
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
% PlugInDocuCaption: Stat Diff
% PlugInName: stat_diff
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Filter 2D/Stat Diff

% PlugInDescription
Stationary Differentation in scan direction using a convolution kernel:
\[ K_{ij} = C \cdot j \cdot e^{ - \frac{i^2}{r_y} - \frac{j^2}{r_x}} \]

% PlugInUsage
Call \GxsmMenu{Math/Filter 2D/Stat Diff}.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

% OptPlugInKnownBugs
Crashes Gxsm -- pending to fix.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

// Plugin Prototypes
static void stat_diff_init( void );
static void stat_diff_about( void );
static void stat_diff_configure( void );
static void stat_diff_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean stat_diff_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean stat_diff_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin stat_diff_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "stat_diff-"
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
  g_strdup("Stationary Differentiation"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-filter2d-section",
  // Menuentry
  N_("Stat Diff"),
  // help text shown on menu
  N_("Stationary Differentiation"),
  // more info...
  "Stationary Differentiation",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  stat_diff_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  stat_diff_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  stat_diff_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  stat_diff_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin stat_diff_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin stat_diff_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin stat_diff_m1s_pi
#else
 GxsmMathTwoSrcPlugin stat_diff_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   stat_diff_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm stat_diff Plugin\n\n"
                                   "Stationary Differentiation");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  stat_diff_pi.description = g_strdup_printf(N_("Gxsm MathOneArg stat_diff plugin %s"), VERSION);
  return &stat_diff_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &stat_diff_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &stat_diff_m2s_pi; 
}
#endif

/* Here we go... */
double       sdx_radius = 5.;
class MemDeriveXKrn *sdx_kernel=NULL;

// init-Function
static void stat_diff_init(void)
{
  PI_DEBUG (DBG_L2, "stat_diff Plugin Init");
}

// about-Function
static void stat_diff_about(void)
{
  const gchar *authors[] = { stat_diff_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  stat_diff_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void stat_diff_configure(void)
{
	if(stat_diff_pi.app)
		stat_diff_pi.app->message("stat_diff Plugin Configuration");
}

// cleanup-Function
static void stat_diff_cleanup(void)
{
	PI_DEBUG (DBG_L2, "stat_diff Plugin Cleanup");
}

/*
 * use gradient (x) as data -- derivate_x -- Kernel
 */

class MemDeriveXKrn : public MemDigiFilter{
public:
	MemDeriveXKrn (double xms, double xns, int m, int n) : MemDigiFilter (xms, xns, m, n){};
	virtual gboolean CalcKernel (){
		double tmp=0.;
		double kasump = 0.;
		double coef = 5.0;

		if (xms < 0.5)	xms = 0.5;
		if (xns < 0.5)	xns = 0.5;

		double asum = 5 + 8*xns*xms;
		do{
			kasump = 0;
			for (int i =-m; i<=m; i++)
				for (int j=0; j<=n; j++)	{
					tmp = rint (coef*j * exp(-(double)(i*i)/(xms*xms) - (double)(j*j)/(xns*xns)));
					data->Z ( -data->Z ((double)tmp, j+n, i+m), n-j, i+m);
					kasump += tmp > 0. ? tmp : -tmp;
				}
			coef /= 0.8 + 0.2*kasump/asum;
		}while (kasump < asum);
		return 0;
	};
};

// run-Function
static gboolean stat_diff_run(Scan *Src, Scan *Dest)
{
// check for multi dim calls, make sure not to ask user for paramters for every layer or time step!
	if (((Src ? Src->mem2d->get_t_index ():0) == 0 && (Src ? Src->mem2d->GetLayer ():0) == 0) || !sdx_kernel) {
		double r = sdx_radius;    // Get Radius
		gapp->ValueRequest("2D Convol. Filter Size", "Radius", "Stat.Diff. kernel size: s = 1+radius",
				   gapp->xsm->Unity, 0., Src->mem2d->GetNx()/10., ".0f", &r);
		sdx_radius = r;
		if (sdx_kernel)
			free (sdx_kernel);

		int    s = 1+(int)(r + .9); // calc. approx Matrix Radius
		sdx_kernel = new MemDeriveXKrn (r,r, s,s); // calc. convol kernel

		if (!Src || !Dest)
			return 0;
	}	

	sdx_kernel->Convolve (Src->mem2d, Dest->mem2d); // do convolution

	return MATH_OK;
}

