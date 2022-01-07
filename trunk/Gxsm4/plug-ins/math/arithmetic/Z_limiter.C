/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=Z_limiter
pluginmenuentry 	=Z Limiter
menupath		=Math/Arithmetic/
entryplace		=Arithmetic
shortentryplace	=AR
abouttext		=Limits Z to a given range
smallhelp		=Limits Z to a given range
longhelp		=Z is limited to a given range.
 * 
 * Gxsm Plugin Name: Z_limiter.C
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
% PlugInDocuCaption: Z Limiter
% PlugInName: Z_limiter
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Arithmetic/Z Limiter

% PlugInDescription
The Z Limiter limites the Z range to a given range defined by an selected area 
(rectangle object used before by \GxsmEmph{AutopDisplay}).

% PlugInUsage
Call \GxsmMenu{Math/Arithmetic/Z Limiter}.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInObjects
The range withing an rectange (i.e. the current \GxsmEmph{AutoDisp} settings) 
is used to obtain Z min/max for limiting.

% OptPlugInDest
The computation result is placed into an existing math channel, else into a new created math channel.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

// Plugin Prototypes
static void Z_limiter_init( void );
static void Z_limiter_about( void );
static void Z_limiter_configure( void );
static void Z_limiter_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean Z_limiter_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean Z_limiter_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin Z_limiter_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "Z_limiter-"
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
  g_strdup("Z is limited to a given range."),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-arithmetic-section",
  // Menuentry
  N_("Z Limiter"),
  // help text shown on menu
  N_("Z is limited to a given range."),
  // more info...
  "Limits Z to a given range",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  Z_limiter_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  Z_limiter_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  Z_limiter_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  Z_limiter_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin Z_limiter_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin Z_limiter_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin Z_limiter_m1s_pi
#else
 GxsmMathTwoSrcPlugin Z_limiter_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   Z_limiter_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm Z_limiter Plugin\n\n"
                                   "Limits Z to a given range (rectangle object)");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  Z_limiter_pi.description = g_strdup_printf(N_("Gxsm MathOneArg Z_limiter plugin %s"), VERSION);
  return &Z_limiter_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &Z_limiter_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &Z_limiter_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void Z_limiter_init(void)
{
	PI_DEBUG (DBG_L2, "Plugin Init" );
}

// about-Function
static void Z_limiter_about(void)
{
  const gchar *authors[] = { Z_limiter_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  Z_limiter_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void Z_limiter_configure(void)
{
  if(Z_limiter_pi.app)
    Z_limiter_pi.app->message("Z_limiter Plugin Configuration");
}

// cleanup-Function
static void Z_limiter_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Plugin Cleanup" );
}

// run-Function
 static gboolean Z_limiter_run(Scan *Src, Scan *Dest)
{
	double hi, lo, z;
	int success = FALSE;
	int n_obj = Src->number_of_object ();
	Point2D p[2];

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

	if (!success)
		return MATH_SELECTIONERR;

	Src->mem2d->HiLo (&hi, &lo, 0, &p[0], &p[1], 1);

	for(int v=0; v < Dest->mem2d->GetNv (); v++)
		for(int line=0; line < Dest->mem2d->GetNy (); line++)
			for(int col=0; col < Dest->mem2d->GetNx (); col++){
				z = Src->mem2d->GetDataPkt (line, col, v);
				Dest->mem2d->PutDataPkt (z > hi ? hi : z < lo ? lo : z, line, col, v);
			}

	return MATH_OK;
}


