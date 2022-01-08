/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=shape
pluginmenuentry 	=Shape
menupath		=Math/Misc/
entryplace		=Misc
shortentryplace	=Misc
abouttext		=shape
smallhelp		=shape
longhelp		=shape
 * 
 * Gxsm Plugin Name: shape.C
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
% PlugInDocuCaption: Create a shape polygon from lines
% PlugInName: shape
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Misc/Shape

% PlugInDescription
A brief description goes here.

% PlugInUsage
Call \GxsmMenu{Math/Misc/Shape}.

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
static void shape_init( void );
static void shape_about( void );
static void shape_configure( void );
static void shape_cleanup( void );

// Math-Run-Function, use only one of (automatically done :=)
 static gboolean shape_run( Scan *Src);

// Fill in the GxsmPlugin Description here
GxsmPlugin shape_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  g_strdup ("shape-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
	    "-Misc"),
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("shape"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-misc-section",
  // Menuentry
  N_("Shape"),
  // help text shown on menu
  N_("shape"),
  // more info...
  "shape",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  shape_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  shape_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  shape_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  shape_cleanup
};

// special math Plugin-Strucure, use
GxsmMathOneSrcNoDestPlugin shape_m1s_pi = {
   // math-function to run, see prototype(s) above!!
   shape_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm shape Plugin\n\n"
                                   "shape");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  shape_pi.description = g_strdup_printf(N_("Gxsm MathOneArg shape plugin %s"), VERSION);
  return &shape_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
GxsmMathOneSrcNoDestPlugin *get_gxsm_math_one_src_no_dest_plugin_info( void ) {
  return &shape_m1s_pi; 
}

/* Here we go... */
// init-Function
static void shape_init(void)
{
  PI_DEBUG (DBG_L2, "shape Plugin Init");
}

// about-Function
static void shape_about(void)
{
  const gchar *authors[] = { shape_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  shape_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void shape_configure(void)
{
  if(shape_pi.app)
    shape_pi.app->message("shape Plugin Configuration");
}

// cleanup-Function
static void shape_cleanup(void)
{
  PI_DEBUG (DBG_L2, "shape Plugin Cleanup");
}

typedef struct {
        double x,y;
} dvec;

typedef struct {
        dvec *p1, *p2, *v;
} dseg;

void intersection (dseg *s1, dseg *s2, dvec &p){
	double la, mu;
	la=mu=0.;
	

}

// run-Function
 static gboolean shape_run(Scan *Src)
{
     int n_obj = Src->number_of_object ();
     if (n_obj < 3) 
             return MATH_SELECTIONERR;

     // count all Line objects
     int n_lines=0;
     for (int i=0; i<n_obj; i++)
             if (!strncmp ((Src->get_object_data (i))->get_name (), "Line", 4) )
		     ++n_lines;

     dvec *p1  = new dvec[n_lines];
     dvec *p2  = new dvec[n_lines];
     dvec *v12 = new dvec[n_lines];

     int k=0;
     for (int i=0; i<n_obj; i++)
             if (!strncmp ((Src->get_object_data (i))->get_name (), "Line", 4) ){
		     (Src->get_object_data (i))->get_xy_i (0, p1[k].x, p1[k].y);
		     (Src->get_object_data (i))->get_xy_i (1, p2[k].x, p2[k].y);
		     // convert to pixels
		     Point2D p;
		     Src->World2Pixel (p1[k].x, p1[k].y, p.x, p.y);
		     p1[k].x = p.x; 
		     p1[k].y = p.y;
		     Src->World2Pixel (p2[k].x, p2[k].y, p.x, p.y);
		     p2[k].x = p.x; 
		     p2[k].y = p.y;
		     v12[k].x = p2[k].x - p1[k].x; 
		     v12[k].y = p2[k].y - p1[k].y; 
		     ++k;
	     }

     delete [] p1;
     delete [] p2;
     delete [] v12;

  return MATH_OK;
}


