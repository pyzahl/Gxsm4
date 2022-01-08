/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: spectrocut.C
 * ========================================
 * 
 * Copyright (C) 2002 The Free Software Foundation
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
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Cut spectra from 3D data
% PlugInName: spectrocut
% PlugInAuthor: Andreas Klust
% PlugInAuthorEmail: klust@users.sf.net
% PlugInMenuPath: Math/Misc/Spectrocut

% PlugInDescription
\label{plugin:spectrocut}
This plug-in is made for choosing areas from a topograhic scan using the
\GxsmEmph{rectangle} object.  The spectra in the correspondent spectroscopic
data taken simultaneously with the topographic data are then cut out and
saved in a file.  Currently, only an index vector is saved in the GNU Octave
ASCII format for further analysis of the data.  Its filename is hard coded
("/tmp/spec.ivec").

% OptPlugInHints
This plug-in is in a very alpha stage.

% PlugInUsage
The topographic scan must be the active, the scan with the spectoscopic data
the X channel.  The plug-in is called via \GxsmMenu{Math/Misc/Spectrocut}.

%% OptPlugInKnownBugs
%No known.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include <string.h>
#include "config.h"
#include "plugin.h"
#include "scan.h"
#include "xsmmath.h"
#include "glbvars.h"
#include "surface.h"


using namespace std;


#ifndef WORDS_BIGENDIAN
# define WORDS_BIGENDIAN 0
#endif

// Plugin Prototypes
static void spectrocut_init( void );
static void spectrocut_about( void );
static void spectrocut_configure( void );
static void spectrocut_cleanup( void );

// Math-Run-Function, "TwoSrc" Prototype
static gboolean spectrocut_run( Scan *Src1, Scan *Src2, Scan *Dest );

// Fill in the GxsmPlugin Description here
GxsmPlugin spectrocut_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  g_strdup ("Spectrocut-"
	    "M2S"
	    "-Misc"),
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("uses marked areas in topographic scans (active) to cut corresponding areas from chan. X."),                   
  // Author(s)
  "Andreas Klust",
  // Menupath to position where it is appendet to
  "math-misc-section",
  // Menuentry
  N_("Spectrocut"),
  // help text shown on menu
  N_("cut areas marked in active scan out of X channel."),
  // more info...
  "no more info",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  spectrocut_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  spectrocut_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  spectrocut_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  spectrocut_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin spectrocut_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin spectrocut_m2s_pi -> "TwoSrcMath"
 GxsmMathTwoSrcPlugin spectrocut_m2s_pi
 = {
   // math-function to run, see prototype(s) above!!
   spectrocut_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm Spectrocut Plugin\n\n"
                                   "Spectrocut Code for OneSrcArgMath");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  spectrocut_pi.description = g_strdup_printf(N_("Gxsm MathTwoArg spectrocut plugin %s"), VERSION);
  return &spectrocut_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_no_same_size_check_plugin_info( void ) { 
  return &spectrocut_m2s_pi; 
}

//
// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void spectrocut_init(void)
{
  PI_DEBUG (DBG_L2, "Spectrocut Plugin Init");
}

// about-Function
static void spectrocut_about(void)
{
  const gchar *authors[] = { spectrocut_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  spectrocut_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void spectrocut_configure(void)
{
  if(spectrocut_pi.app)
    spectrocut_pi.app->message("Spectrocut Plugin Configuration");
}

// cleanup-Function
static void spectrocut_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Spectrocut Plugin Cleanup");
}

// run-Function
static gboolean spectrocut_run(Scan *Src1, Scan *Src2, Scan *Dest)
{
  // reduce Dest size to save memory
  Dest->mem2d->Resize(0, 0);

  // first check some prerequisites:

  // both sources must have the same real space size, but may
  // have different resolution
  if (Src1->data.s.rx != Src2->data.s.rx || Src1->data.s.ry != Src2->data.s.ry)
    return MATH_SELECTIONERR;

  // I need at least one object
  int n_obj = Src1->number_of_object ();
  if (n_obj < 1) 
    return MATH_SELECTIONERR;

  // iterate over all objects
  for (int i=0; i<n_obj; i++)
    {
      scan_object_data *obj_data;
      obj_data = Src1->get_object_data (i);

      obj_data->dump ();

      // only rectangles are supported:
      //      if (!strncmp (obj_data->get_name (), "Rectangle", 9) )
      //	return MATH_SELECTIONERR;

      int n_pts = obj_data->get_num_points ();
      // support for two-point objects, only
      if (n_pts != 2)
	return MATH_SELECTIONERR;

      // get real world coordinates
      double x0, y0, x1, y1;
      obj_data->get_xy_i (0, x0, y0);
      obj_data->get_xy_i (1, x1, y1);

      // sort the points, that point 0 becomes the lower left corner
      if (x1 < x0)
	{
	  double swap = x0;
	  x0 = x1;
	  x1 = swap;
	}
      if (y1 < y0)
	{
	  double swap = y0;
	  y0 = y1;
	  y1 = swap;
	}

      cout << x0 << " " << y0 << endl;
      cout << x1 << " " << y1 << endl;

      // calculate pixel coordinates
      int xl = (int)( (x0 - Src2->data.s.x0 + Src2->data.s.rx/2) / Src2->data.s.dx );
      int xr = (int)( (x1 - Src2->data.s.x0 + Src2->data.s.rx/2) / Src2->data.s.dx );
      int yb = (int)( (Src2->data.s.y0 - y0) / Src2->data.s.dy );
      int yt = (int)( (Src2->data.s.y0 - y1) / Src2->data.s.dy );

      cout << xl << " " << yb << endl;
      cout << xr << " " << yt << endl;

      // make sure the pixel coord. lie within the scan Src2
      if (xl < 0)  xl=0;
      if (xl > Src2->mem2d->GetNx()-1)  xl = Src2->mem2d->GetNx()-1;
      if (xr < 0)  xr=0;
      if (xr > Src2->mem2d->GetNx()-1)  xr = Src2->mem2d->GetNx()-1;
      if (yb < 0)  yb=0;
      if (yb > Src2->mem2d->GetNy()-1)  yb = Src2->mem2d->GetNy()-1;
      if (yt < 0)  yt=0;
      if (yt > Src2->mem2d->GetNy()-1)  yt = Src2->mem2d->GetNy()-1;

      cout << xl << " " << yb << endl;
      cout << xr << " " << yt << endl;


      // let's dump the spectroscopic data

      short *buf;
      if(!(buf = g_new(short, (xr-xl+1)*(yb-yt+1))))
	return MATH_NOMEM;
      
      ofstream f;
      f.open("/tmp/spec.ivec", ios::out);

      // write header for octave file
      f << "# Created by the Gxsm spectrocut plug-in" << endl;
      f << "# name: index_vector" << endl;
      f << "# type: matrix" << endl;
      f << "# rows: 1" << endl;
      f << "# columns: " << (xr-xl+1)*(yb-yt+1) << endl;

      for (int j=yt; j<=yb; j++)
	for (int i=xl; i<=xr; i++)
	  f << i + Src2->mem2d->GetNx() * (Src2->mem2d->GetNy()-j-1) << " ";

      f << endl;

//       for (int v=0; v < Src2->data.lp.nvalues; v++) 
// 	{
// 	  short *pt = buf;	  
// 	  for (int i=xl; i<=xr; i++)
// 	    for (int j=yt; j<=yb; j++, pt++)
// 	      {
// 		*pt = (short) Src2->mem2d->GetDataPkt (i, j, v);
// 		// dump using big endian format
// 		// to maintain compatibility with SCALA SPM files
// 		if (!WORDS_BIGENDIAN)  {
// 		  char *cpt = (char*)pt;
// 		  char low = cpt[0];
// 		  cpt[0] = cpt[1];
// 		  cpt[1] = low;
// 		}		
// 	      }
	  
// 	  if(f.good())
// 	    f.write((void*)buf, sizeof(short) * (xr-xl+1)*(yb-yt+1));
// 	  else {
// 	    g_free(buf);
// 	    return MATH_FILE_ERROR;  
// 	  }

// 	}

      g_free(buf);
      f.close();

    }

  return MATH_OK;
}

// end of spectrocut.C
