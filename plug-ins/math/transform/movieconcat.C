/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sourceforge.net
pluginname		=movieconcat
pluginmenuentry 	=Movie Concat
menupath		=math-transformations-section
entryplace		=_Transformations
shortentryplace	=TR
abouttext		=a
smallhelp		=b
longhelp		=c
 * 
 * Gxsm Plugin Name: movieconcat.C
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
% PlugInDocuCaption: Movie Concat
% PlugInName: movieconcat
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sourceforge.net
% PlugInMenuPath: math-transformations-sectionMovie Concat

% PlugInDescription
Movie Concat allows to concatenate two movie data set in time with a
choosen range in time and in layers of both sources. The number of
layers choosen for each must match.

% PlugInUsage

%% OptPlugInSection: replace this by the section caption
%all following lines until next tag are going into this section

%% OptPlugInSubSection: replace this line by the subsection caption
%all following lines until next tag are going into this subsection

%% you can repeat OptPlugIn(Sub)Sections multiple times!

%% OptPlugInSources
The active and X marked channels are used as data sources.

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
#include "core-source/plugin.h"

// Plugin Prototypes
static void movieconcat_init( void );
static void movieconcat_about( void );
static void movieconcat_configure( void );
static void movieconcat_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
// #define GXSM_ONE_SRC_PLUGIN__DEF
#define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean movieconcat_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean movieconcat_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin movieconcat_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "movieconcat-"
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
  g_strdup("movie concatenate"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-transformations-section",
  // Menuentry
  N_("Movie Concat"),
  // help text shown on menu
  N_("concat movie in layer and/or time dimension"),
  // more info...
  "concat movie in layer and/or time dimension",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  movieconcat_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  movieconcat_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  movieconcat_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  movieconcat_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin movieconcat_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin movieconcat_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin movieconcat_m1s_pi
#else
 GxsmMathTwoSrcPlugin movieconcat_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   movieconcat_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm movieconcat Plugin\n\n"
                                   "concat movie in layer and/or time dimension");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  movieconcat_pi.description = g_strdup_printf(N_("Gxsm MathOneArg movieconcat plugin %s"), VERSION);
  return &movieconcat_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &movieconcat_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &movieconcat_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void movieconcat_init(void)
{
  PI_DEBUG (DBG_L2, "Plugin Init" );
}

// about-Function
static void movieconcat_about(void)
{
  const gchar *authors[] = { movieconcat_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  movieconcat_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void movieconcat_configure(void)
{
  if(movieconcat_pi.app)
    movieconcat_pi.app->message("movieconcat Plugin Configuration");
}

// cleanup-Function
static void movieconcat_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Plugin Cleanup");
}

static gboolean movieconcat_run(Scan *Src1, Scan *Src2, Scan *Dest)
{
	int ti1,tf1, vi1,vf1;
	int ti2,tf2, vi2,vf2;
	int time;

	if(Src1->data.s.nx != Src2->data.s.nx || Src1->data.s.ny != Src2->data.s.ny)
		return MATH_SELECTIONERR;

	if(Src1 == Src2 || Src2 == Dest)
		return MATH_SELECTIONERR;

//      copy basic data from Src1 as Masterscan
	if(Dest != Src1)
		Dest->data.copy (Src1->data);

//	setup optional cut from Src1
	do {
		ti1=vi1=0;
		tf1=Src1->number_of_time_elements ()-1;
		vf1=Src1->mem2d->GetNv ()-1;
		if(Dest != Src1)
                        if (gapp->setup_multidimensional_data_copy ("Movie concat Src1 section", Src1, ti1, tf1, vi1, vf1) == GTK_RESPONSE_CANCEL)
				return MATH_OK;
	} while (ti1 > tf1 || vi1 > vf1);
	
//	setup optional cut from Src1
	do {
		ti2=vi2=0;
		tf2=Src2->number_of_time_elements ()-1;
		vf2=Src2->mem2d->GetNv ()-1;
		if(Dest != Src1)
                        if (gapp->setup_multidimensional_data_copy ("Movie concat Src2 section", Src2, ti2, tf2, vi2, vf2) == GTK_RESPONSE_CANCEL)
				return MATH_OK;
	} while (ti2 > tf2 || vi2 > vf2 || (vf2-vi2) != (vf1-vi1));
	
	// number of time frames stored
	int n_times_src1 = tf1-ti1+1;
	int n_times_src2 = tf2-ti2+1;

//	setup an progress indicator
	gapp->progress_info_new ("Multidimenssional Concatenating Copy", 1);
	gapp->progress_info_set_bar_fraction (0., 1);
	gapp->progress_info_set_bar_text ("Time", 1);
		
	// resize Dest to match Src
	Dest->mem2d->Resize (Src1->mem2d->GetNx (), Src1->mem2d->GetNy (), vf1-vi1+1, ZD_IDENT);

	// go for it....
	int ntimes_tmp = tf1-ti1+1 + tf2-ti2+1;
	std::cout << "Appending from Src1: " << ti1 << " ... " << tf1 << std::endl;
	for (int time_index=ti1; time_index <= tf1; ++time_index){
		Mem2d *m = Src1->mem2d_time_element (time_index);
		gapp->progress_info_set_bar_fraction ((gdouble)(time_index-ti1+1)/(gdouble)ntimes_tmp, 1);
		if(Dest != Src1){
			std::cout << "Appending from Src1: " << time_index << " #Layers:" << m->GetNv () << std::endl;
			Dest->mem2d->copy(m, -1, -1, vi1, vf1);
			Dest->append_current_to_time_elements (time_index-ti1, m->get_frame_time (0));
		}
	}
	std::cout << "Appending from Src2: " << ti2 << " ... " << tf2 << std::endl;
	for (int time_index=ti2; time_index <= tf2; ++time_index){
		Mem2d *m = Src2->mem2d_time_element (time_index);
		gapp->progress_info_set_bar_fraction ((gdouble)(time_index-ti2+tf1-ti1+1)/(gdouble)ntimes_tmp, 1);
		std::cout << "Appending from Src2: " << time_index << " #Layers:" << m->GetNv () << std::endl;
		Dest->mem2d->copy(m, -1, -1, vi2, vf2);
		Dest->append_current_to_time_elements (time_index-ti2+tf1-ti1+1, m->get_frame_time (0));
	}

	Dest->data.s.ntimes = ntimes_tmp;
	Dest->data.s.nvalues=Dest->mem2d->GetNv ();
	
	gapp->progress_info_close ();
	Dest->retrieve_time_element (0);
	Dest->mem2d->SetLayer(0);

	return MATH_OK;
}


