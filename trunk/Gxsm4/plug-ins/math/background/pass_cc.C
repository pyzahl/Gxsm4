/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=pass_cc
pluginmenuentry 	=Pass CC
menupath		=Math/Background/
entryplace		=Background
shortentryplace	=BG
abouttext		=Pass band selection, C conj, all rectangles
smallhelp		=Pass band data copy, CC
longhelp		=Pass band selection, C conj, all rectangles
 * 
 * Gxsm Plugin Name: pass_cc.C
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
% PlugInDocuCaption: Pass band copy
% PlugInName: pass_cc
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Background/Pass CC

% PlugInDescription
Used for copying data in selected rectangles and automatically complex
conjugated (CC) rectangles. It is in particular made to be used for
marking areas in frequency space (e.g. in a calculated Filter 2D/Power
Spectrum, this generates a PDS and preserves the full orgininal Re/Im
data for manipulation and optional back-transform) for filtering of
data in frequency domain.  The result can then be transformed back
using Filter 2D/IFT 2D (inverse FT) \GxsmEmph{IFT(FT())} 2D filter.

% PlugInUsage
Call \GxsmMenu{Math/Background/Pass CC}.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInObjects
All data in marked rectangles and CC rectangles is copied.

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

// Plugin Prototypes
static void pass_cc_init( void );
static void pass_cc_about( void );
static void pass_cc_configure( void );
static void pass_cc_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
static gboolean pass_cc_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
static gboolean pass_cc_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin pass_cc_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	"pass_cc-"
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
	g_strdup("Pass band selection, C conj, all rectangles"),                   
	// Author(s)
	"Percy Zahl",
	// Menupath to position where it is appendet to
	"math-background-section",
	// Menuentry
	N_("Pass CC"),
	// help text shown on menu
	N_("Pass band selection, C conj, all rectangles"),
	// more info...
	"Pass band data copy, CC",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	pass_cc_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	pass_cc_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	pass_cc_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

	pass_cc_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin pass_cc_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin pass_cc_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin pass_cc_m1s_pi
#else
GxsmMathTwoSrcPlugin pass_cc_m2s_pi
#endif
= {
	// math-function to run, see prototype(s) above!!
	pass_cc_run
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm pass_cc Plugin\n\n"
                                   "Pass band selection, C conj, all rectangles");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	pass_cc_pi.description = g_strdup_printf(N_("Gxsm MathOneArg pass_cc plugin %s"), VERSION);
	return &pass_cc_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
	return &pass_cc_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
	return &pass_cc_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void pass_cc_init(void)
{
	PI_DEBUG (DBG_L2, "pass_cc Plugin Init");
}

// about-Function
static void pass_cc_about(void)
{
	const gchar *authors[] = { pass_cc_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  pass_cc_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void pass_cc_configure(void)
{
	if(pass_cc_pi.app)
		pass_cc_pi.app->message("pass_cc Plugin Configuration");
}

// cleanup-Function
static void pass_cc_cleanup(void)
{
	PI_DEBUG (DBG_L2, "pass_cc Plugin Cleanup");
}

// run-Function
static gboolean pass_cc_run(Scan *Src, Scan *Dest)
{
	int num_rects=0;
	int n_obj = Src->number_of_object ();

	Dest->mem2d->Resize (Src->mem2d->GetNx(), Src->mem2d->GetNy(), Src->mem2d->GetNv(), ZD_IDENT);
	// clear all
	Dest->mem2d->data->set_all_Z (ZEROVALUE);

	if (n_obj < 1) 
		return MATH_SELECTIONERR;

	while (n_obj--){
		scan_object_data *obj_data = Src->get_object_data (n_obj);

		if (strncmp (obj_data->get_name (), "Rectangle", 9) )
			continue; // only rectangels are used!

		if (obj_data->get_num_points () != 2) 
			continue; // sth. is weired!

		// get real world coordinates of rectangle
		double x0,y0,x1,y1;
		obj_data->get_xy_i (0, x0, y0);
		obj_data->get_xy_i (1, x1, y1);

		// convert to pixels
		Point2D p[2];
		Src->World2Pixel (x0, y0, p[0].x, p[0].y);
		Src->World2Pixel (x1, y1, p[1].x, p[1].y);

		// make selection, assure top/bottom and limits!
		MOUSERECT msr, msrPktSym;
		MkMausSelectP (p, &msr, Dest->mem2d->GetNx(), Dest->mem2d->GetNy());
		MkMausSelectP (p, &msrPktSym, Dest->mem2d->GetNx(), Dest->mem2d->GetNy()); // copy
		
		if (msr.xSize < 1 || msr.ySize < 1)
			continue; // size is weired!

		msr.yTop    = Dest->mem2d->GetNy()-msrPktSym.yBottom-1;

		// copy area
		int ly=Src->mem2d->GetLayer();
		for(int v=0; v<=Src->mem2d->GetNv (); ++v){
			Src->mem2d->SetLayer (v);
			Dest->mem2d->SetLayer (v);
			Dest->mem2d->CopyFrom (Src->mem2d, msr.xLeft,msr.yTop, msr.xLeft,msr.yTop, msr.xSize, msr.ySize);
		}
		
		// make CC rectangle
		msr.yTop    = msrPktSym.yBottom-msrPktSym.ySize;
		msr.xLeft   = Dest->mem2d->GetNx()-msrPktSym.xRight-1;
		msr.xRight  = Dest->mem2d->GetNx()-msrPktSym.xLeft-1;

		// copy CC area
		for(int v=0; v<=Src->mem2d->GetNv (); ++v){
			Src->mem2d->SetLayer (v);
			Dest->mem2d->SetLayer (v);
			Dest->mem2d->CopyFrom (Src->mem2d, msr.xLeft,msr.yTop, msr.xLeft,msr.yTop, msr.xSize, msr.ySize);
		}
		
		// restore current layer
		Src->mem2d->SetLayer (ly);
		Dest->mem2d->SetLayer (ly);

		// inc # processed rectangles
		++num_rects;
	}

	if (!num_rects)
		return MATH_SELECTIONERR; // no rectangles found!

	return MATH_OK;
}


