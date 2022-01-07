/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: spectrocut.C
 * ========================================
 * 
 * Copyright (C) 2002 The Free Software Foundation
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
% PlugInDocuCaption: Smoothing layers in 3D
% PlugInName: layersmooth
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Misc/Layersmooth

% PlugInDescription
This filter applies a gausian smooth to all layers of a scan. It can
also smooth accross layers in 3D, using a smoothing radius in
layer-dimension.

% PlugInUsage
Call \GxsmMenu{Math/Misc/Layersmooth}\dots

% OptPlugInSources
The active channel is used as data source.

%% OptPlugInObjects

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

%% OptPlugInConfig
%

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

%% OptPlugInKnownBugs
%

% OptPlugInNotes
Not jet finished, the 3D smooth (accross layers) is not jet
implemented. Please set Radius in Layer Dim to zero!

%% OptPlugInHints
%

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include <string.h>
#include "config.h"
#include "plugin.h"
#include "scan.h"
#include "xsmmath.h"
 
using namespace std;


#ifndef WORDS_BIGENDIAN
# define WORDS_BIGENDIAN 0
#endif

// Plugin Prototypes
static void layersmooth_init (void);
static void layersmooth_about (void);
static void layersmooth_configure (void);
static void layersmooth_cleanup (void);

// Math-Run-Function, "TwoSrc" Prototype
static gboolean layersmooth_run (Scan *Src, Scan *Dest);

// Fill in the GxsmPlugin Description here
GxsmPlugin layersmooth_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  g_strdup ("Layersmooth-"
	    "M1S"
	    "-Misc"),
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("Gausian Smooth of all layers, width given in pixels"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-misc-section",
  // Menuentry
  N_("Layersmooth"),
  // help text shown on menu
  N_("Gausian smooth of al layers"),
  // more info...
  "no more info",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  layersmooth_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  layersmooth_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  layersmooth_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  layersmooth_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin layersmooth_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin layersmooth_m2s_pi -> "TwoSrcMath"
 GxsmMathOneSrcPlugin layersmooth_m1s_pi
 = {
   // math-function to run, see prototype(s) above!!
   layersmooth_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm Layersmooth Plugin\n\n"
                                   "A gausian smooth is applied to all layers.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  layersmooth_pi.description = g_strdup_printf(N_("Gxsm MathOneArg layersmooth plugin %s"), VERSION);
  return &layersmooth_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) { 
  return &layersmooth_m1s_pi; 
}

//
// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void layersmooth_init(void)
{
  PI_DEBUG (DBG_L2, "Layersmooth Plugin Init");
}

// about-Function
static void layersmooth_about(void)
{
  const gchar *authors[] = { layersmooth_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  layersmooth_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void layersmooth_configure(void)
{
  if(layersmooth_pi.app)
    layersmooth_pi.app->message("Layersmooth Plugin Configuration");
}

// cleanup-Function
static void layersmooth_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Layersmooth Plugin Cleanup");
}

/*
 * special kernels for MemDigiFilter
 * Gausian Smooth Kernel
 */

class MemSmoothKrn : public MemDigiFilter{
public:
	MemSmoothKrn (double xms, double xns, int m, int n):MemDigiFilter (xms, xns, m, n){};
	virtual gboolean CalcKernel (){
		int i,j;
		for (i= -m; i<=m; i++)
			for (j = -n; j<=n; j++)
				data->Z (rint (4*exp (-(i*i)/(xms*xms) -(j*j)/(xns*xns))), j+n, i+m); 
		return 0;
	};
};

// run-Function
static gboolean layersmooth_run(Scan *Src, Scan *Dest)
{
	double rxy = 3.;    // Get Radius
	double rv  = 0.;    // Get Radius
	main_get_gapp()->ValueRequest ("2D Convol. Filter Size", "Radius in XY Dim", "Smooth kernel size: s = 1+radius",
			    main_get_gapp()->xsm->Unity, 0., Src->mem2d->GetNx()/10., ".0f", &rxy);
	
	main_get_gapp()->ValueRequest ("2D Convol. Filter Size", "Radius in Layer Dim", "Smooth kernel size: s = 1+radius",
			    main_get_gapp()->xsm->Unity, 0., Src->mem2d->GetNv()/10., ".0f", &rv);
	
	Dest->mem2d->Resize (Src->mem2d->GetNx(), Src->mem2d->GetNy(), Src->mem2d->GetNv());

	PI_DEBUG (DBG_L2, "mem2d->GetNv()  = " << Dest->mem2d->GetNv() << endl
			  << "data.s.nvalues = " << Dest->data.s.nvalues );

	int current_layer = Src->mem2d->GetLayer ();
	if (rv == 0.){
		int    s = 1+(int)(rxy + .9); // calc. approx Matrix Radius
		MemSmoothKrn krn(rxy,rxy, s,s); // Setup Kernelobject
		
		// apply to all layers
		for (int v = 0; v < Dest->mem2d->GetNv (); ++v){
			PI_DEBUG (DBG_L2, "Smoothing: " << v );
			Src->mem2d->SetLayer (v);
			Dest->mem2d->SetLayer (v);
			krn.Convolve(Src->mem2d, Dest->mem2d);
			PI_DEBUG (DBG_L2, "mem2d->GetNv()  = " << Dest->mem2d->GetNv() );
		}
	} else {
		int nxy, nv;
		nxy = 2*(int)rxy+1;
		nv  = 2*(int)rv+1;
//		double *kernel = new double[nv][nxy];
		for (int v = 0; v < Dest->mem2d->GetNv (); ++v){
			;
		}
//		delete[][] kernel;
	}

	Src->mem2d->SetLayer (current_layer);
	Dest->mem2d->SetLayer (current_layer);
	
	return MATH_OK;
}

// end of layersmooth.C
