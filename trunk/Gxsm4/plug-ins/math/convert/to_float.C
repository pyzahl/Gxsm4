/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=to_float
pluginmenuentry 	=to float
menupath		=Math/Convert/
entryplace		=Convert
shortentryplace	=CV
abouttext		=Convert to float
smallhelp		=convert to float
longhelp		=Converts a scan to type float.
 * 
 * Gxsm Plugin Name: to_float.C
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
% PlugInDocuCaption: Convert to float
% PlugInName: to_float
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Convert/to float

% PlugInDescription
Convert scan data type to float.

% PlugInUsage
Call \GxsmMenu{Math/Convert/to float}.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInDest
The conversion result is placed into an existing math channel, else
into a new created math channel.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

// Plugin Prototypes
static void to_float_init( void );
static void to_float_about( void );
static void to_float_configure( void );
static void to_float_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean to_float_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean to_float_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin to_float_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "to_float-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-CV",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("Converts a scan to type float."),
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-convert-section",
  // Menuentry
  N_("to float"),
  // help text shown on menu
  N_("Converts a scan to type float."),
  // more info...
  "convert to float",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  to_float_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  to_float_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  to_float_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  to_float_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin to_float_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin to_float_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin to_float_m1s_pi
#else
 GxsmMathTwoSrcPlugin to_float_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   to_float_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm to_float Plugin\n\n"
                                   "Convert to float");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  to_float_pi.description = g_strdup_printf(N_("Gxsm MathOneArg to_float plugin %s"), VERSION);
  return &to_float_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &to_float_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &to_float_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void to_float_init(void)
{
  PI_DEBUG (DBG_L2, "to_float Plugin Init");
}

// about-Function
static void to_float_about(void)
{
  const gchar *authors[] = { to_float_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  to_float_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void to_float_configure(void)
{
	if(to_float_pi.app)
		to_float_pi.app->message("to_float Plugin Configuration");
}

// cleanup-Function
static void to_float_cleanup(void)
{
	PI_DEBUG (DBG_L2, "to_float Plugin Cleanup");
}

// run-Function
static gboolean to_float_run(Scan *Src, Scan *Dest)
{
	if (Src->data.s.ntimes == 1 && Src->mem2d->GetNv () == 1){
		Dest->mem2d->Resize (Dest->data.s.nx, Dest->data.s.ny, ZD_FLOAT);
		Dest->mem2d->ConvertFrom (Src->mem2d, 
					  0,0, 
					  0,0,
					  Dest->mem2d->GetNx (), Dest->mem2d->GetNy ());
		Dest->mem2d->data->CopyLookups(Src->mem2d->data);
		Dest->mem2d->copy_layer_information (Src->mem2d);
	} else {
		int ti,tf, vi,vf;
		ti=vi=0;
		tf=Src->number_of_time_elements ()-1;
		if (tf < 0) tf = 0;
		vf=Src->mem2d->GetNv ()-1;

		gapp->progress_info_new ("Multidimenssional Conversion", 1);
		gapp->progress_info_set_bar_fraction (0., 1);
		gapp->progress_info_set_bar_text ("Time", 1);
		
		int ntimes_tmp = tf-ti+1;
		for (int time_index=ti; time_index <= tf; ++time_index){
			Mem2d *m = Src->mem2d_time_element (time_index);
			gapp->progress_info_set_bar_fraction ((gdouble)(time_index-ti)/(gdouble)ntimes_tmp, 1);

			//			Dest->mem2d->copy(m, -1, -1, vi, vf);
			
			Dest->mem2d->Resize (Dest->data.s.nx, Dest->data.s.ny, vf+1, ZD_FLOAT);
			for (int i=vi; i<=vf; ++i){
			  std::cout << "Conv2F Layer: " << i << std::endl;
			  Dest->mem2d->SetLayer (i);			  
			  m->SetLayer (i);
			  Dest->mem2d->ConvertFrom (m, 
						    0,0, 
						    0,0,
						    Dest->mem2d->GetNx (), Dest->mem2d->GetNy ());
			}
			Dest->mem2d->data->CopyLookups(m->data);
			Dest->mem2d->copy_layer_information (m);
			Dest->append_current_to_time_elements (time_index-ti, m->get_frame_time ());
		}
		Dest->data.s.ntimes = ntimes_tmp;
		Dest->data.s.nvalues=Dest->mem2d->GetNv ();

		gapp->progress_info_close ();
		Dest->retrieve_time_element (0);
		Dest->mem2d->SetLayer(0);
	}


	return MATH_OK;
}


