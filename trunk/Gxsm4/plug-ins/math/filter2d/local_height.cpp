/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=local_height
pluginmenuentry 	=Local height
menupath		=Math/Filter 2D/
entryplace		=Filter 2D
shortentryplace	=F2D
abouttext		=computes locale height using a convolution filter
smallhelp		=compute local height
longhelp		=computes locale height using a convolution filter
 * 
 * Gxsm Plugin Name: local_height.C
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
% PlugInDocuCaption: Local height
% PlugInName: local_height
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Filter 2D/Local height

% PlugInDescription
Computes the local height via convolution.

% PlugInUsage
Call \GxsmMenu{Math/Filter 2D/Local height}.

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
static void local_height_init( void );
static void local_height_about( void );
static void local_height_configure( void );
static void local_height_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean local_height_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean local_height_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin local_height_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "local_height-"
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
  g_strdup("computes locale height using a convolution filter"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-filter2d-section",
  // Menuentry
  N_("Local height"),
  // help text shown on menu
  N_("computes locale height using a convolution filter"),
  // more info...
  "compute local height",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  local_height_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  local_height_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  local_height_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  local_height_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin local_height_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin local_height_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin local_height_m1s_pi
#else
 GxsmMathTwoSrcPlugin local_height_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   local_height_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm local_height Plugin\n\n"
                                   "computes locale height using a convolution filter");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  local_height_pi.description = g_strdup_printf(N_("Gxsm MathOneArg local_height plugin %s"), VERSION);
  return &local_height_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_for_all_vt_plugin_info( void ) {
  return &local_height_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &local_height_m2s_pi; 
}
#endif

/* Here we go... */
double       lh_radius = 5.;
class MemLclhtKrn *lh_kernel=NULL;

// init-Function
static void local_height_init(void)
{
  PI_DEBUG (DBG_L2, "local_height Plugin Init");
}

// about-Function
static void local_height_about(void)
{
  const gchar *authors[] = { local_height_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  local_height_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void local_height_configure(void)
{
	if(local_height_pi.app)
		local_height_pi.app->message("local_height Plugin Configuration");
}

// cleanup-Function
static void local_height_cleanup(void)
{
	PI_DEBUG (DBG_L2, "local_height Plugin Cleanup");
}

/*
 * Local Height Kernel
 */

class MemLclhtKrn : public MemDigiFilter{
public:
	MemLclhtKrn(double xms, double xns, int m, int n) : MemDigiFilter (xms, xns, m, n){};
	virtual gboolean CalcKernel(){
		int i;
		const double r_squared = 7.0;
		double c0 = 5.0;
		double c1 = c0/r_squared;
		double ksump, kasump;

		if(xms < 0.5)	xms = 0.5;
		if(xns < 0.5)	xns = 0.5;
		const double asum = 5 + 8*xns*xms;
		do{
			kasump = ksump=0.;
			for(i= -m;i<=m;i++)	{
				int j;
				const double isq = i*i/xms;
				for(j= -n;j<=n;j++)	{
					const double i_jsq = j*j/xns + isq;
					data->Z( ksump += c0*exp(-i_jsq)-c1*i_jsq*exp(-i_jsq/r_squared),
						 j+n, i+m);
					kasump += fabs(data->Z(j+n,i+m));
				}
			}
			if(kasump < asum)
				c0 /= 0.9 + 0.1*kasump/asum;
			if(ksump > 0)
				c1 += 0.3*c1*ksump/(double)kasump;
			else if(ksump < 0)	{
				c1 += 0.1*c1*ksump/(double)kasump;
				c0 -= 0.1*c0*ksump/(double)kasump;
			}
		} while(fabs(ksump) > 0.3); // != 0
		return 0;
	};
};

// run-Function
static gboolean local_height_run(Scan *Src, Scan *Dest)
{
// check for multi dim calls, make sure not to ask user for paramters for every layer or time step!
	if (((Src ? Src->mem2d->get_t_index ():0) == 0 && (Src ? Src->mem2d->GetLayer ():0) == 0) || !lh_kernel) {
		double r = lh_radius;    // Get Radius
		main_get_gapp()->ValueRequest("2D Convol. Filter Size", "Radius", "Local Height kernel size: s = 1+radius",
				   main_get_gapp()->xsm->Unity, 0., Src->mem2d->GetNx()/10., ".0f", &r);
		lh_radius = r;
		if (lh_kernel)
			free (lh_kernel);

		int    s = 1+(int)(r + .9); // calc. approx Matrix Radius
		lh_kernel = new MemLclhtKrn (r,r, s,s); // calc. convol kernel

		if (!Src || !Dest)
			return 0;
	}	

	lh_kernel->Convolve (Src->mem2d, Dest->mem2d); // do convolution

	return MATH_OK;
}


