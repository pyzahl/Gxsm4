/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=stop_ccr
pluginmenuentry 	=Stop CC
menupath		=Math/Background/
entryplace		=Background
shortentryplace	=BG
abouttext		=Stop band zeroing, C conj., all rectangles
smallhelp		=Stop band zeroing, CC
longhelp		=Stop band zeroing, C conj., all rectangles
 * 
 * Gxsm Plugin Name: stop_ccr.C
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
% PlugInDocuCaption: Stop band removal
% PlugInName: stop_ccr
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Background/Stop CC

% PlugInDescription
Used for zeroing data in selected rectangles and automatically complex
conjugated (CC) rectangles. It is in particular made to be used for
marking areas in frequency space (e.g. in a calculated Filter 2D/Power
Spectrum, this generates a PDS and preserves the full orgininal Re/Im
data for manipulation and optional back-transform) for filtering of
data in frequency domain.  The result can then be transformed back
using Filter 2D/IFT 2D (inverse FT) \GxsmEmph{IFT(FT())} 2D filter.

% PlugInUsage
Call \GxsmMenu{Math/Background/Stop CC}.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInObjects
All data in marked rectangles and CC rectangles are zeroed.

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"

// Plugin Prototypes
static void stop_ccr_init( void );
static void stop_ccr_about( void );
static void stop_ccr_configure( void );
static void stop_ccr_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
static gboolean stop_ccr_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
static gboolean stop_ccr_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin stop_ccr_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	"stop_ccr-"
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
	g_strdup("Stop band zeroing, C conj., all rectangles"),                   
	// Author(s)
	"Percy Zahl",
	// Menupath to position where it is appendet to
	"math-background-section",
	// Menuentry
	N_("Stop CC"),
	// help text shown on menu
	N_("Stop band zeroing, C conj., all rectangles"),
	// more info...
	"Stop band zeroing, CC",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	stop_ccr_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	stop_ccr_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	stop_ccr_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

	stop_ccr_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin stop_ccr_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin stop_ccr_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin stop_ccr_m1s_pi
#else
GxsmMathTwoSrcPlugin stop_ccr_m2s_pi
#endif
= {
	// math-function to run, see prototype(s) above!!
	stop_ccr_run
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm stop_ccr Plugin\n\n"
                                   "Stop band zeroing, C conj., all rectangles");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	stop_ccr_pi.description = g_strdup_printf(N_("Gxsm MathOneArg stop_ccr plugin %s"), VERSION);
	return &stop_ccr_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
	return &stop_ccr_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
	return &stop_ccr_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void stop_ccr_init(void)
{
	PI_DEBUG (DBG_L2, "stop_ccr Plugin Init");
}

// about-Function
static void stop_ccr_about(void)
{
	const gchar *authors[] = { stop_ccr_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  stop_ccr_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void stop_ccr_configure(void)
{
	if(stop_ccr_pi.app)
		stop_ccr_pi.app->message("stop_ccr Plugin Configuration");
}

// cleanup-Function
static void stop_ccr_cleanup(void)
{
	PI_DEBUG (DBG_L2, "stop_ccr Plugin Cleanup");
}

// run-Function
// #define LL_DEBUG(STR) PI_DEBUG (DBG_L2, "stop_ccr_run:: " << STR << endl;
#define LL_DEBUG(STR) std::cout << STR << std::endl;
static gboolean stop_ccr_run(Scan *Src, Scan *Dest)
{
	int num_rects=0;
	int n_obj = (int) Src->number_of_object ();

	LL_DEBUG( "stop_ccr_run:: numobj=" << n_obj );
	// make a copy
	CopyScan(Src, Dest);
	LL_DEBUG( "stop_ccr_run:: copy done" );

	if (n_obj < 1) 
		return MATH_SELECTIONERR;

	while (n_obj--){
		LL_DEBUG( "stop_ccr_run:: checking obj " << n_obj );
		scan_object_data *obj_data = Src->get_object_data (n_obj);

		if (strncmp (obj_data->get_name (), "Rectangle", 9) )
			continue; // only rectangels are used!

		if (obj_data->get_num_points () != 2) 
			continue; // sth. is weired!

		LL_DEBUG( "stop_ccr_run:: processing rect" << n_obj );

		// get real world coordinates of rectangle
		double x0,y0,x1,y1;
		obj_data->get_xy_i (0, x0, y0);
		obj_data->get_xy_i (1, x1, y1);

		LL_DEBUG( "AA rect sel: "
			"(" << x0 << ", " << y0 << "),"
			"(" << x1 << ", " << y1 << ")" );

		// convert to pixels
		Point2D p[2];
		Src->World2Pixel (x0, y0, p[0].x, p[0].y);
		Src->World2Pixel (x1, y1, p[1].x, p[1].y);

		LL_DEBUG( "Pix rect sel: "
			"(" << p[0].x << ", " << p[0].y << "),"
			"(" << p[1].x << ", " << p[1].y << ")" );

		// make selection, assure top/bottom and limits!
		MOUSERECT msr, msrPktSym;
		MkMausSelectP (p, &msr, Dest->mem2d->GetNx(), Dest->mem2d->GetNy());
		MkMausSelectP (p, &msrPktSym, Dest->mem2d->GetNx(), Dest->mem2d->GetNy()); // copy
		
		if (msr.xSize < 1 || msr.ySize < 1)
			continue; // size is weired!

		msr.yTop    = Dest->mem2d->GetNy()-msrPktSym.yBottom-1;

		// zero area
		LL_DEBUG( "stop_ccr_run:: zero rect" );
		LL_DEBUG( "(" << msr.xLeft << ", " << msr.yTop << "),(" 
		     << msr.xSize << ", " << msr.ySize << ")" );
		Dest->mem2d->data->set_all_Z (ZEROVALUE, -1, msr.xLeft,msr.yTop, msr.xSize, msr.ySize);
		
		// make CC rectangle
		msr.yTop    = msrPktSym.yBottom-msrPktSym.ySize;
		msr.xLeft   = Dest->mem2d->GetNx()-msrPktSym.xRight-1;
		msr.xRight  = Dest->mem2d->GetNx()-msrPktSym.xLeft-1;

		// zero CC area
		LL_DEBUG( "stop_ccr_run:: zero CC rect" );
		LL_DEBUG( "(" << msr.xLeft << ", " << msr.yTop << "),(" 
		     << msr.xSize << ", " << msr.ySize << ")" );
		Dest->mem2d->data->set_all_Z (ZEROVALUE, -1,  msr.xLeft,msr.yTop, msr.xSize, msr.ySize);

		// inc # processed rectangles
		++num_rects;
		LL_DEBUG( "stop_ccr_run:: processer rects=" << num_rects );
	}

	if (!num_rects)
		return MATH_SELECTIONERR; // no rectangles found!

	return MATH_OK;
}


