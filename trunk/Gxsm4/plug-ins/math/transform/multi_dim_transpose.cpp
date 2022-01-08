/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=multi_dim_transpose
pluginmenuentry 	=Multi Dim Transpose
menupath		=math-transformations-section
entryplace		=_Transformations
shortentryplace	=TR
abouttext		=Multi dimensional transposition
smallhelp		=Multi dimensional transposition
longhelp		=Multi dimensional transposition
 * 
 * Gxsm Plugin Name: multi_dim_transpose.C
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
% PlugInDocuCaption: Multi Dimensional Transposition
% PlugInName: multi_dim_transpose
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: math-transformations-sectionMulti Dim Transpose

% PlugInDescription
For now this tool swappes time and layer dimensions.

% PlugInUsage
Activate a channel and run it.

%% OptPlugInSection: replace this by the section caption
%all following lines until next tag are going into this section
%...

%% OptPlugInSubSection: replace this line by the subsection caption
%all following lines until next tag are going into this subsection
%...

%% you can repeat OptPlugIn(Sub)Sections multiple times!

%% OptPlugInSources
%The active channel is used as data source.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

%% OptPlugInDest
%The computation result is placed into an existing math channel, else into a new created math channel.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

%% OptPlugInRefs
%Any references?

%% OptPlugInKnownBugs
%Are there known bugs? List! How to work around if not fixed?

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
#include "glbvars.h"
#include "surface.h"


// Plugin Prototypes
static void multi_dim_transpose_init( void );
static void multi_dim_transpose_about( void );
static void multi_dim_transpose_configure( void );
static void multi_dim_transpose_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean multi_dim_transpose_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean multi_dim_transpose_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin multi_dim_transpose_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "multi_dim_transpose-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-TR",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup("Multi dimensional transposition"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-transformations-section",
  // Menuentry
  N_("Multi Dim Transpose"),
  // help text shown on menu
  N_("Multi dimensional transposition"),
  // more info...
  "Multi dimensional transposition",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  multi_dim_transpose_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  multi_dim_transpose_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  multi_dim_transpose_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  multi_dim_transpose_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin multi_dim_transpose_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin multi_dim_transpose_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin multi_dim_transpose_m1s_pi
#else
 GxsmMathTwoSrcPlugin multi_dim_transpose_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   multi_dim_transpose_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm multi_dim_transpose Plugin\n\n"
                                   "Multi dimensional transposition");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  multi_dim_transpose_pi.description = g_strdup_printf(N_("Gxsm MathOneArg multi_dim_transpose plugin %s"), VERSION);
  return &multi_dim_transpose_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &multi_dim_transpose_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &multi_dim_transpose_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void multi_dim_transpose_init(void)
{
  PI_DEBUG (DBG_L2, "Plugin Init" );
}

// about-Function
static void multi_dim_transpose_about(void)
{
  const gchar *authors[] = { multi_dim_transpose_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  multi_dim_transpose_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void multi_dim_transpose_configure(void)
{
  if(multi_dim_transpose_pi.app)
    multi_dim_transpose_pi.app->message("multi_dim_transpose Plugin Configuration");
}

// cleanup-Function
static void multi_dim_transpose_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Plugin Cleanup");
}

// run-Function
 static gboolean multi_dim_transpose_run(Scan *Src, Scan *Dest)
{
	main_get_gapp()->progress_info_new ("Transposing", 2); // setup an progress indicator
	main_get_gapp()->progress_info_set_bar_fraction (0., 1);
	main_get_gapp()->progress_info_set_bar_fraction (0., 2);
	main_get_gapp()->progress_info_set_bar_text ("Time", 1);
	main_get_gapp()->progress_info_set_bar_text ("Value", 2);

	// number of time frames stored
	int n_times  = Src->number_of_time_elements ();
	int n_layers = Src->mem2d->GetNv ();

	// resize Dest to match transposed Src
	Dest->mem2d->Resize (Src->mem2d->GetNx (), Src->mem2d->GetNy (), n_times, ZD_IDENT);

	// running indices are of source, dest are transposed in time and value
	for (int layer_index=0; layer_index < n_layers; ++layer_index){
		double frame_time=0.;
		main_get_gapp()->progress_info_set_bar_fraction ((gdouble)(layer_index+1)/(gdouble)n_layers, 2);

		for (int time_index=0; time_index < n_times; ++time_index){
			main_get_gapp()->progress_info_set_bar_fraction ((gdouble)(time_index+1)/(gdouble)n_times, 1);

			std::cout << "T:" << time_index << " V:" << layer_index << std::endl;

			Mem2d *m = Src->mem2d_time_element (time_index);
			m->data->SetLayer (layer_index);
			if (time_index==0) 
				frame_time = m->get_frame_time ();
			Dest->mem2d->SetLayer (time_index);
			std::cout << "CopyFrom..." << std::endl;
			Dest->mem2d->CopyFrom (m, 0,0, 0,0, Src->mem2d->GetNx (), Src->mem2d->GetNy ());
			std::cout << "SetVLookup..." << std::endl;
			Dest->mem2d->data->SetVLookup (time_index, m->get_frame_time ());
			std::cout << "OK..." << std::endl;

			// copy LI
			if (m->layer_information_list[layer_index]){
				int nj = g_list_length (m->layer_information_list[layer_index]);
				for (int j = 0; j<nj; ++j){
					LayerInformation *li = (LayerInformation*) g_list_nth_data (m->layer_information_list[layer_index], j);
					if (li)
						Dest->mem2d->add_layer_information (time_index, new LayerInformation (li));
				}
			}
		}

		std::cout << "Append in time..." << std::endl;
		Dest->append_current_to_time_elements (layer_index, frame_time);
		std::cout << "OK." << std::endl;
	}
//	Dest->data.s.ntimes  = n_times;
	Dest->data.s.nvalues = Dest->mem2d->GetNv ();
	
	main_get_gapp()->progress_info_close ();
	Dest->retrieve_time_element (0);
	Dest->mem2d->SetLayer(0);

	main_get_gapp()->progress_info_close (); // close progress indicator
	return MATH_OK;
}


