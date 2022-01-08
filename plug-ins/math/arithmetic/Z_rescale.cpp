/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=Z_rescale
pluginmenuentry 	=Mul X
menupath		=Math/Arithmetic/
entryplace		=Arithmetic
shortentryplace	=AR
abouttext		=Rescale Z of Active channels
smallhelp		=My new Plugin does useful things
longhelp		=This is a detailed help for my Plugin.
 * 
 * Gxsm Plugin Name: Z_rescale.C
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
% PlugInDocuCaption: Multiply scans
% PlugInName: Z_rescale
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Arithmetic/Mul X

% PlugInDescription
Multiplys the Z-values of scans bt factor. May limit action to rectange -- will be used if any is found.

% PlugInUsage
Call \GxsmMenu{Math/Arithmetic/Z Rescale}.

% OptPlugInSources
The "Z" of the active channel is rescaled by a given factor.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel. The result is of type \GxsmEmph{float}.

%% OptPlugInNotes

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"

// Plugin Prototypes
static void Z_rescale_init( void );
static void Z_rescale_about( void );
static void Z_rescale_configure( void );
static void Z_rescale_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
//#define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean Z_rescale_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean Z_rescale_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin Z_rescale_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "Z_rescale-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-AR",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("This is a detailed help for my Plugin."),
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-arithmetic-section",
  // Menuentry
  N_("Z Rescale"),
  // help text shown on menu
  N_("multiply Z data with factor"),
  // more info...
  "Rescales data of active channel by factor.",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  Z_rescale_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  Z_rescale_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  Z_rescale_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  Z_rescale_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin Z_rescale_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin Z_rescale_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin Z_rescale_m1s_pi
#else
 GxsmMathTwoSrcPlugin Z_rescale_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   Z_rescale_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm Z_rescale Plugin\n\n"
                                   "Multiplies the active and X channel.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  Z_rescale_pi.description = g_strdup_printf(N_("Gxsm MathOneArg Z_rescale plugin %s"), VERSION);
  return &Z_rescale_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &Z_rescale_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &Z_rescale_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void Z_rescale_init(void)
{
  PI_DEBUG (DBG_L2, "Z_rescale Plugin Init");
}

// about-Function
static void Z_rescale_about(void)
{
  const gchar *authors[] = { Z_rescale_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  Z_rescale_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void Z_rescale_configure(void)
{
	if(Z_rescale_pi.app)
		Z_rescale_pi.app->message("Z_rescale Plugin Configuration");
}

// cleanup-Function
static void Z_rescale_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Z_rescale Plugin Cleanup");
}

// run-Function
static gboolean Z_rescale_run(Scan *Src, Scan *Dest)
{
	static double factor=1.;
	double hi, lo, z;
	int success = FALSE;
	int n_obj = Src->number_of_object ();
	Point2D p[2];

	Z_rescale_pi.app->ValueRequest("Scale Value Request", "Multiplier:", 
				       "Enter Z data scaling factor.",
				       Z_rescale_pi.app->xsm->Unity, 
				       -1e6, 1e6, ".6f", &factor);

	while (n_obj--){
		scan_object_data *obj_data = Src->get_object_data (n_obj);
		
		if (strncmp (obj_data->get_name (), "Rectangle", 9) )
			continue; // only points are used!
		
		if (obj_data->get_num_points () != 2) 
			continue; // sth. is weired!
		
		double x,y;
		obj_data->get_xy_i_pixel (0, x, y);
		p[0].x = (int)x; p[0].y = (int)y;
		obj_data->get_xy_i_pixel (1, x, y);
		p[1].x = (int)x; p[1].y = (int)y;
		
		success = TRUE;
		break;
	}

	if (!success){
		p[0].x = p[0].y = 0;
		p[1].x = Dest->mem2d->GetNx (); 
		p[1].y = Dest->mem2d->GetNy ();
	}

	Dest->mem2d->Resize (Dest->data.s.nx, Dest->data.s.ny, Dest->mem2d->GetNv (), ZD_FLOAT);

	for(int v=0; v<Dest->mem2d->GetNv (); ++v){
                Dest->mem2d->data->SetVLookup(v, Src->mem2d->data->GetVLookup(v));

		for(int line=p[0].y; line<p[1].y; ++line)
			for(int col=p[0].x; col<p[1].x; ++col)
			  Dest->mem2d->PutDataPkt(
						  factor * Src->mem2d->GetDataPkt (col, line, v),
						  col, line, v);
	}
	return MATH_OK;
}


