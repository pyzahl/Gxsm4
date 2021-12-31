/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Quick nine points Gxsm Plugin GUIDE by PZ:
 * ------------------------------------------------------------
 * 1.) Make a copy of this "OctoCorr.C" to "your_plugins_name.C"!
 * 2.) Replace all "OctoCorr" by "your_plugins_name" 
 *     --> please do a search and replace starting here NOW!! (Emacs doese preserve caps!)
 * 3.) Decide: One or Two Source Math: see line 54
 * 4.) Fill in GxsmPlugin Structure, see below
 * 5.) Replace the "about_text" below a desired
 * 6.) * Optional: Start your Code/Vars definition below (if needed more than the run-fkt itself!), 
 *       Goto Line 155 ff. please, and see comment there!!
 * 7.) Fill in math code in OctoCorr_run(), 
 *     have a look at the Data-Access methods infos at end
 * 8.) Add OctoCorr.C to the Makefile.am in analogy to others
 * 9.) Make a "make; make install"
 * A.) Call Gxsm->Tools->reload Plugins, be happy!
 * ... That's it!
 * 
 * Gxsm Plugin Name: OctoCorr.C
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
% PlugInDocuCaption: SPA--LEED octopole distorsion correction
% PlugInName: OctoCorr
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Transformation/Octo Corr

% PlugInDescription
This is a experimental filter to reduce the SPA--LEED typical octopole
cuishion distorsion using a "ab-initio" like approch.  This plugin
applies the inverse of this distorsion to any image.

Input parameters are: Energy, Stepsize and Origin (invariant Point, if
not set the user is prompted for coordinates!).  A free parameter $b$
(distorsion strength, may be left at default value (-0.4)).

% PlugInUsage
Place a point object and call \GxsmMenu{Math/Transformation/Octo
Corr}.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInObjects
A point object is needed to mark the center of distorsion symmetry
(invariant point).

% OptPlugInDest
The computation result is placed into an existing math channel, 
else into a new created math channel.

%% OptPlugInConfig
%

%% OptPlugInNotes
%

%% OptPlugInHints
%

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

// Plugin Prototypes
static void OctoCorr_init( void );
static void OctoCorr_about( void );
static void OctoCorr_configure( void );
static void OctoCorr_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
static gboolean OctoCorr_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
static gboolean OctoCorr_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin OctoCorr_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	g_strdup ("OctoCorr-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
		  "M1S"
#else
		  "M2S"
#endif
		  "-TR"),
	// Plugin's Category - used to autodecide on Pluginloading or ignoring
	// NULL: load, else
	// example: "+noHARD +STM +AFM"
	// load only, if "+noHARD: no hardware" and Instrument is STM or AFM
	// +/-xxxHARD und (+/-INST or ...)
	NULL,
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	g_strdup ("Octopole Correction, use for SPA-LEED image geometry correction"),
	// Author(s)
	"Percy Zahl",
	// Menupath to position where it is appendet to
	"math-transformations-section",
	// Menuentry
	N_("OctoCorr"),
	// help text shown on menu
	N_("for SPA-LEED: to correct octopole distorsions"),
	// more info...
	"no more info",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	OctoCorr_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	OctoCorr_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	OctoCorr_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
	NULL, // direct menu entry callback1 or NULL
	NULL, // direct menu entry callback2 or NULL

	OctoCorr_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin OctoCorr_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin OctoCorr_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin OctoCorr_m1s_pi
#else
GxsmMathTwoSrcPlugin OctoCorr_m2s_pi
#endif
= {
	// math-function to run, see prototype(s) above!!
	OctoCorr_run
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm OctoCorr Plugin\n\n"
                                   "The SPA-LEED octopole causes a cushion distorsion\n"
				   "of the electron deflection.\n"
				   "This pluging applies the inverse of this distorsion to\n"
				   "any image.\n"
				   "Input parameters are:\n"
				   " Energy, Stepsize and Origin (invariant Point)\n"
				   " Free parameter \"b\" (distorsion strength)\n"
				   );

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	OctoCorr_pi.description = g_strdup_printf(N_("Gxsm MathOneArg OctoCorr plugin %s"), VERSION);
	return &OctoCorr_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
	return &OctoCorr_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
	return &OctoCorr_m2s_pi; 
}
#endif

// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void OctoCorr_init(void)
{
	PI_DEBUG (DBG_L2, "OctoCorr Plugin Init");
}

// about-Function
static void OctoCorr_about(void)
{
	const gchar *authors[] = { OctoCorr_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  OctoCorr_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void OctoCorr_configure(void)
{
	if(OctoCorr_pi.app)
		OctoCorr_pi.app->message("OctoCorr Plugin Configuration");
}

// cleanup-Function
static void OctoCorr_cleanup(void)
{
	PI_DEBUG (DBG_L2, "OctoCorr Plugin Cleanup");
}


// run-Function
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
static gboolean OctoCorr_run(Scan *Src, Scan *Dest)
#else
	static gboolean OctoCorr_run(Scan *Src1, Scan *Src2, Scan *Dest)
#endif
{
	// put plugins math code here...
	int i,j;
	double jj,ii;
	double Ux, Uy, U, E, s, a, b;
	double Ldx, Ldy, nX0, nY0, Ux0, Uy0;

	E = Src->data.s.Energy;
	OctoCorr_pi.app->ValueRequest("Enter Value", "Energy", 
				      "I need the scan Energy.",
				      OctoCorr_pi.app->xsm->EnergyUnit, 
				      0.1, 500., ".2f", &E);
	Ldx = Src->data.s.dx;
	OctoCorr_pi.app->ValueRequest("Enter Value", "Ldx", 
				      "dx, X step size.",
				      OctoCorr_pi.app->xsm->VoltUnit, 
				      1e-5, 10., ".2f", &Ldx);
	Ldy = Src->data.s.dy;
	OctoCorr_pi.app->ValueRequest("Enter Value", "Ldy", 
				      "dy, Y step size.",
				      OctoCorr_pi.app->xsm->VoltUnit, 
				      1e-5, 10., ".2f", &Ldy);

	g_message ("PlugIn OctoCorr.C: Fix Me! Need mouse coordinate port top new scan_obj methods.");

#if 0

  
	if(Src->PktVal == 1){
	nX0 = Src->Pkt2d[0].x;
	nY0 = Src->Pkt2d[0].y;
}else{
	nX0 = Src->data.s.x0/Ldx; // + .SPA_OrgX;
	nY0 = Src->data.s.y0/Ldy; // + .SPA_OrgX;
	OctoCorr_pi.app->ValueRequest("Enter Value", "X index",
		"invariant Point (can use Point Obj!)", 
		OctoCorr_pi.app->xsm->Unity, 
		-1000, 1000., ".2f", &nX0);
    
	OctoCorr_pi.app->ValueRequest("Enter Value", "Y index",
		"invariant Point (can use Point Obj!)", 
		OctoCorr_pi.app->xsm->Unity, 
		-1000., 1000., ".2f", &nY0);
}

	Ux0 = ((double)nX0-(double)Src->data.s.nx/2.)*Ldx;
	Uy0 = ((double)nY0-(double)Src->data.s.ny/2.)*Ldy;

#define xMap(I) (Ux0+((double)(I)-(double)Src->data.s.nx/2.)*Ldx)
#define yMap(I) (Uy0+((double)(I)-(double)Src->data.s.ny/2.)*Ldy)

	//  b = -0.22;
	b = -0.4;

	OctoCorr_pi.app->ValueRequest("Enter Value", "b (strength)", 
		"may not be changed!",
		OctoCorr_pi.app->xsm->Unity, 
		-2., 2., ".2f", &b);

	Ux = xMap(Src->data.s.nx); Uy=yMap(Src->data.s.ny);
	U  = sqrt(Ux*Ux + Uy*Uy);
	a  = 1. - b*U/E;
	for(i=0; i<Dest->mem2d->GetNy(); i++){
	Uy = yMap(i);
	for(j=0; j<Dest->mem2d->GetNx(); j++){
	Ux = xMap(j);
	U = sqrt(Ux*Ux + Uy*Uy);
	s = a + b*sqrt(U/E);
	jj = (double)(j-nX0)*s + nX0;
	ii = (double)(i-nY0)*s + nY0;
	if(ii >= (double)(Dest->mem2d->GetNy()-1) 
		|| jj >= (double)(Dest->mem2d->GetNx()-1) || ii < 1. || jj < 1.)
		Dest->mem2d->PutDataPkt(0., j,i);
	else
		Dest->mem2d->PutDataPkt(Src->mem2d->GetDataPktInterpol(jj,ii), j,i);
}
}
#endif
  
  return MATH_OK;
}

