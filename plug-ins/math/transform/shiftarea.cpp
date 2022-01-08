/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=stefan
pluginname		=shiftarea
pluginmenuentry 	=Shift-Area
menupath		=Math/Transformations/
entryplace		=Transformations
shortentryplace	=TR
abouttext		=Shift area with line object.
smallhelp		=Shift area to connect correctly.
longhelp		=Shift area does this and that.
 * 
 * Gxsm Plugin Name: shiftarea.C
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
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Transformation Shift-Area
% PlugInName: shiftarea
% PlugInAuthor: Stefan Schr\"oder
% PlugInAuthorEmail: stefan_fkp@users.sf.net
% PlugInMenuPath: Math/Transformation/Shift Area

% PlugInDescription

This plugin shifts the lower part of a scan with respect to the upper
part, according to a chosen line.

% PlugInUsage
Choose a line Object an connect the two points with your line, that shall
be brought together. The green point must be lower than the red one.

% OptPlugInSources
You need one active scan and a line object. Rectangle works two, the
diagonal will work like the line.

% OptPlugInObjects
If a rectangle is selected the calculated information applies to the
content of the rectangle. Otherwise, the whole scan is analyzed.

% OptPlugInDest
A new scan will be created, which contains the unchanged upper part and the
shifted lower part, connected. 

%% OptPlugInConfig
%None.

%% OptPlugInKnownBugs
%None?

%% OptPlugInNotes
%None.
% EndPlugInDocuSection
 *
--------------------------------------------------------------------------------
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"

// Plugin Prototypes
static void shiftarea_init( void );
static void shiftarea_about( void );
static void shiftarea_configure( void );
static void shiftarea_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean shiftarea_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean shiftarea_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin shiftarea_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  g_strdup ("shiftarea-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
	    "-TR"),
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup("Shift area does this and that."),                   
  // Author(s)
  "stefan",
  // Menupath to position where it is appendet to
  "math-transformations-section",
  // Menuentry
  N_("Shift-Area"),
  // help text shown on menu
  N_("Shift area does this and that."),
  // more info...
  "The Shift-Area Plugin works with the line-object.\n It can connect to shifted areas",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  shiftarea_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  shiftarea_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  shiftarea_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  shiftarea_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin shiftarea_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin shiftarea_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin shiftarea_m1s_pi
#else
 GxsmMathTwoSrcPlugin shiftarea_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   shiftarea_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm shiftarea Plugin\n\n"
                                   "Shift area with line object.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  shiftarea_pi.description = g_strdup_printf(N_("Gxsm MathOneArg shiftarea plugin %s"), VERSION);
  return &shiftarea_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &shiftarea_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &shiftarea_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void shiftarea_init(void)
{
  PI_DEBUG (DBG_L2, "shiftarea Plugin Init");
}

// about-Function
static void shiftarea_about(void)
{
  const gchar *authors[] = { shiftarea_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  shiftarea_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void shiftarea_configure(void)
{
  if(shiftarea_pi.app)
    shiftarea_pi.app->message("shiftarea Plugin Configuration");
}

// cleanup-Function
static void shiftarea_cleanup(void)
{
  PI_DEBUG (DBG_L2, "shiftarea Plugin Cleanup");
}

// run-Function
 static gboolean shiftarea_run(Scan *Src, Scan *Dest)
{

  int x1, x2, top, bottom, hshift, vshift;
  MOUSERECT msr;
  MkMausSelect (Src, &msr, Src->mem2d->GetNx(), Src->mem2d->GetNy());

  if( msr.xSize  < 1 || msr.ySize < 1){
	  return MATH_SELECTIONERR;
  }

  x1     = msr.xLeft;
  x2     = msr.xRight;
  top    = msr.yTop;
  bottom = msr.yBottom;
  
  // PI_DEBUG (DBG_L2, "Plugin Shiftarea: " << x1 << " " << x2 << " " <<top << " " <<bottom << endl;

  vshift = bottom - top;

  if(x2>x1){   //   funktioniert 
    PI_DEBUG (DBG_L2, "x2 groesser x1 ");
    hshift = x2 - x1;
    Dest->mem2d->Resize(Dest->data.s.nx+hshift, Dest->data.s.ny);

    double dummy;
    for(int line=0; line<Dest->data.s.ny; line++){
      for(int col=0; col<Dest->data.s.nx+hshift; col++){

        if(line < top){ // obere haelfte
          if(col < hshift)
            dummy = 0;
          else
            dummy = Src->mem2d->GetDataPkt(col-hshift, line);
        }

        if( line >= top) { //untere haelfte
          if(col >= Src->data.s.nx) 
            dummy = 0;
          else
            dummy = Src->mem2d->GetDataPkt(col, line);
        }

        Dest->mem2d->PutDataPkt(dummy, col, line);
      }
    }
  }
  else if (x1>x2){  // funktioniert
    hshift = x1 - x2;
    Dest->mem2d->Resize(Dest->data.s.nx+hshift, Dest->data.s.ny);

    double dummy;
    for(int line=0; line<Dest->data.s.ny; line++){
      for(int col=0; col<Dest->data.s.nx+hshift; col++){

        if(line >= top){ //untere haelfte
          if(col < hshift)
            dummy = 0;
          else
            dummy = Src->mem2d->GetDataPkt(col-hshift, line);
        }

        if( line < top) { //obere haelfte
          if(col < Dest->data.s.nx) //kleiner xgrenze
            dummy = Src->mem2d->GetDataPkt(col, line);
          else                     //groesser xgrenze
            dummy = 0;
        }          
        //execute
        Dest->mem2d->PutDataPkt(dummy, col, line);
      }
    }
  }
  return MATH_OK;
}

