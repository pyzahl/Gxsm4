/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=bg_z_drift
pluginmenuentry 	=Z drift correct
menupath		=Math/Background/
entryplace		=Background
shortentryplace	=BG
abouttext		=Corrects a slow and smooth Z drifting using a polynominal fit.
smallhelp		=smooth Z drift correction
longhelp		=Corrects a slow and smooth Z drifting using a polynominal fit.
 * 
 * Gxsm Plugin Name: bg_z_drift.C
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
% PlugInDocuCaption: Smooth Z drift correction
% PlugInName: bg_z_drift
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Background/Z drift correct

% PlugInDescription
Corrects a slow and smooth variing Z drift using a polynominal fit for
averaged line heights.

% PlugInUsage
Call \GxsmMenu{Math/Background/Z drift correct}. It asks for the
degree ($2, 3, 4, \dots 13$) used for the polynominal (least squares)
fit, $5$th order is the default.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInObjects
A optional rectangle can be used to restrict the X range used for
calculation the average scan line height. The Y coordinates of the
rectangle are ignored.

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

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
#include "regress.h"
#include "glbvars.h"
#include "surface.h"

// Plugin Prototypes
static void bg_z_drift_init( void );
static void bg_z_drift_about( void );
static void bg_z_drift_configure( void );
static void bg_z_drift_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean bg_z_drift_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean bg_z_drift_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin bg_z_drift_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "bg_z_drift-"
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
  g_strdup ("Corrects a slow and smooth Z drifting using a polynominal fit."),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-background-section",
  // Menuentry
  N_("Z drift correct"),
  // help text shown on menu
  N_("Corrects a slow and smooth Z drifting using a polynominal fit."),
  // more info...
  "smooth Z drift correction",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  bg_z_drift_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  bg_z_drift_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  bg_z_drift_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  bg_z_drift_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin bg_z_drift_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin bg_z_drift_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin bg_z_drift_m1s_pi
#else
 GxsmMathTwoSrcPlugin bg_z_drift_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   bg_z_drift_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm bg_z_drift Plugin\n\n"
                                   "Corrects a slow and smooth Z drifting using a polynominal fit.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  bg_z_drift_pi.description = g_strdup_printf(N_("Gxsm MathOneArg bg_z_drift plugin %s"), VERSION);
  return &bg_z_drift_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
	return &bg_z_drift_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
	return &bg_z_drift_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void bg_z_drift_init(void)
{
	PI_DEBUG (DBG_L2, "bg_z_drift Plugin Init");
}

// about-Function
static void bg_z_drift_about(void)
{
	const gchar *authors[] = { bg_z_drift_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  bg_z_drift_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void bg_z_drift_configure(void)
{
	if(bg_z_drift_pi.app)
		bg_z_drift_pi.app->message("bg_z_drift Plugin Configuration");
}

// cleanup-Function
static void bg_z_drift_cleanup(void)
{
	PI_DEBUG (DBG_L2, "bg_z_drift Plugin Cleanup");
}

// run-Function
static gboolean bg_z_drift_run(Scan *Src, Scan *Dest)
{
	int line, col, order=3;
	double UsrOrder;
	double *YProfile;
	double *YProfileX;
	double py;
	MOUSERECT msr;

	MkMausSelect (Src, &msr, Dest->mem2d->GetNx(), Dest->mem2d->GetNy());

	if( msr.xSize  < 1 || msr.ySize < 1)
		return MATH_SELECTIONERR;

	YProfile  = new double[Src->mem2d->GetNy()];
	YProfileX = new double[Src->mem2d->GetNy()];

	PI_DEBUG (DBG_L3, "X-left:" << msr.xLeft << "  X-right:" << msr.xRight);

	for(line=0; line < Dest->mem2d->GetNy(); line++){
		YProfile[line] = 0.;
		YProfileX[line] = (double)line;
		for(col=msr.xLeft; col < msr.xRight; col++)
			YProfile[line] += Src->mem2d->GetDataPkt(col,line);
		YProfile[line] /= (double)msr.xSize;
	}

	UsrOrder=5.;
	main_get_gapp()->ValueRequest("Enter Value", "Order", "I need the order of regression polynom.",
			   main_get_gapp()->xsm->Unity, 2., 13., ".0f", &UsrOrder);
  
	order = (int)UsrOrder; if(order < 2 || order > 13) order=3;
	StatInp s(order, YProfileX, YProfile, Src->mem2d->GetNy()); 

	std::ofstream poly;
	poly.open("/tmp/poly.asc", std::ios::out);

	for(line=0; line<Dest->data.s.ny; line++){
		py = s.GetPoly((double)line);
		poly << YProfileX[line] << " " << YProfile[line] << " " << py << std::endl;
		for(col=0; col<Dest->data.s.nx; col++)
			Dest->mem2d->PutDataPkt(Src->mem2d->GetDataPkt(col,line) - py, col, line);
	}

	poly.close();

	return MATH_OK;
}
