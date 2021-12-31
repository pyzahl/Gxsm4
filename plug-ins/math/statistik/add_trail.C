/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=add_trail
pluginmenuentry 	=Invert
menupath		=Math/Statistik/
entryplace		=Arithmetic
shortentryplace	=AR
abouttext		=Add_Trail
smallhelp		=Add_Trail
longhelp		=Add_Trail.
 * 
 * Gxsm Plugin Name: add_trail.C
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
% PlugInDocuCaption: Add Trail
% PlugInName: add_trail
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Statistics/Add Trail

% PlugInDescription
Add's trail local height to Z at positions in scan.

% PlugInUsage
Call \GxsmMenu{Math/Statistics/Add Trail}.

% OptPlugInSources
The active channel is used as data source.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

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
static void add_trail_init( void );
static void add_trail_about( void );
static void add_trail_configure( void );
static void add_trail_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean add_trail_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean add_trail_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin add_trail_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "add_trail-"
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
  g_strdup("Add Trail."),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-statistics-section",
  // Menuentry
  N_("Add Trail"),
  // help text shown on menu
  N_("Add Trail."),
  // more info...
  "add trail",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  add_trail_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  add_trail_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  add_trail_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  add_trail_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin add_trail_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin add_trail_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin add_trail_m1s_pi
#else
 GxsmMathTwoSrcPlugin add_trail_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   add_trail_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm add_trail Plugin\n\n"
                                   "Add Trail");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  add_trail_pi.description = g_strdup_printf(N_("Gxsm MathOneArg add_trail plugin %s"), VERSION);
  return &add_trail_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &add_trail_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &add_trail_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void add_trail_init(void)
{
  PI_DEBUG (DBG_L2, "add_trail Plugin Init");
}

// about-Function
static void add_trail_about(void)
{
  const gchar *authors[] = { add_trail_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  add_trail_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void add_trail_configure(void)
{
	if(add_trail_pi.app)
		add_trail_pi.app->message("add_trail Plugin Configuration");
}

// cleanup-Function
static void add_trail_cleanup(void)
{
	PI_DEBUG (DBG_L2, "add_trail Plugin Cleanup");
}

// run-Function
static gboolean add_trail_run(Scan *Src, Scan *Dest)
{
	Dest->mem2d->ConvertFrom(
		Src->mem2d, 
		0, 0, 
		0, 0, 
		Dest->mem2d->GetNx(), Dest->mem2d->GetNy()
		);


//	for (int line = 0; line < Src->mem2d->GetNy(); line++)
//		for (int col = 0; col < Src->mem2d->GetNx(); col++)
//			Dest->mem2d->data->Zmul(-1., col, line);

	std::ifstream objloadstream;
	gchar *fname = gapp->file_dialog ("Load Trail from vpdata", NULL, NULL, " ", "load trail from vpdata");
	objloadstream.open (fname, std::ios::in);

	int count=0;
	int count_off=0;
	double xy0[2] = {0.,0.};
	while (objloadstream.good ()){
		gchar line[512];
		objloadstream.getline (line, 512);

// # GXSM-Main-Offset       :: X0=185.844 Ang  Y0=176.072 Ang, iX0=68 Pix iX0=74 Pix
		if (!strncmp (line, "# GXSM-Main-Offset ", 17)){
			xy0[0] = atof (strstr (line, " X0=")+4);
			xy0[1] = atof (strstr (line, " Y0=")+4);
			std::cout << "Trail Offset from Main: XY0:" << xy0[0] << ", " << xy0[1] << std::endl;
			continue;
		}
		if (!strncmp (line, "#C Vector Probe Header List --", 30)){

// #C Vector Probe Header List -----------------
// #C # ####	 time[ms]  	 dt[ms]    	 X[Ang]   	 Y[Ang]   	 Z[Ang]    	 Sec
// #       0	          0	          0	          0	 -0.0519132	     328.58	  0	 
// #       1	    92.3733	    92.3733	   -2.64757	   -1.50548	    328.262	 25	 
// ...
	
		       objloadstream.getline (line, 512); // skip header line
		       double arr[7];
		       double xyz[3] = {0., 0., 0.};
		       double z;
		       std::cout << "Reading Trail to Objetcs..." << std::endl << line << std::endl;
		       while (objloadstream.good ()){
			   objloadstream.getline (line, 512); // skip line end to next
//			   std::cout << line << std::endl;
			   if (!strncmp (line, "#C END", 6))
				   break;
			   
			   arr[0] = atof (strtok(line, " ,;()#"));
			   for (int i=1; i<7; ++i)
				   arr[i] = atof (strtok(NULL, " ,;()#"));
//			   std::cout << arr[0] << " t" << arr[1]  << " x" << arr[3]  << " y" << arr[4] << " z" << arr[5]  << " s" << arr[6];
			   Src->World2Pixel (arr[3]+xy0[0], arr[4]+xy0[1], xyz[0], xyz[1]);
			   xyz[2] = -arr[5]; // Z
			   if ( round (xyz[0]) >= 0 &&  round (xyz[0]) < Dest->mem2d->GetNx () && round (xyz[1]) >= 0 &&  round (xyz[1]) < Dest->mem2d->GetNy ()){
				   z = Src->mem2d->GetDataPktInterpol (xyz[0], xyz[1]) * Src->data.s.dz;
//				   Dest->mem2d->PutDataPkt (xyz[2] / Src->data.s.dz, round (xyz[0]), round (xyz[1]));
				   Dest->mem2d->data->Zadd (1. / Src->data.s.dz, round (xyz[0]), round (xyz[1]));
				   ++count;
			   }
			   else{
				   ++count_off;
//				   std::cout << "Trail out of range: XY0:" << xy0[0] << ", " << xy0[1] << " ixy:" << xyz[0] << ", " << xyz[1] << " ::: " << arr[0] << " t" << arr[1]  << " x" << arr[3]  << " y" << arr[4] << " z" << arr[5]  << " s" << arr[6] << std::endl;
			   }

		       }
		}
	}

	std::cout << "Trails added from " << count << " tracking points. Out of scan range: " << count_off << std::endl;
	gchar *m = g_strdup_printf ("Trails added from %d tracking points.\nOut of scan range: %d", count, count_off);
	gapp->message (m);
	g_free (m);

	return MATH_OK;
}
