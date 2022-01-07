/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Quick nine points Gxsm Plugin GUIDE by PZ:
 * ------------------------------------------------------------
 * 1.) Make a copy of this "SlopeAbs.C" to "your_plugins_name.C"!
 * 2.) Replace all "SlopeAbs" by "your_plugins_name" 
 *     --> please do a search and replace starting here NOW!! (Emacs doese preserve caps!)
 * 3.) Decide: One or Two Source Math: see line 54
 * 4.) Fill in GxsmPlugin Structure, see below
 * 5.) Replace the "about_text" below a desired
 * 6.) * Optional: Start your Code/Vars definition below (if needed more than the run-fkt itself!), 
 *       Goto Line 155 ff. please, and see comment there!!
 * 7.) Fill in math code in SlopeAbs_run(), 
 *     have a look at the Data-Access methods infos at end
 * 8.) Add SlopeAbs.C to the Makefile.am in analogy to others
 * 9.) Make a "make; make install"
 * A.) Call Gxsm->Tools->reload Plugins, be happy!
 * ... That's it!
 * 
 * Gxsm Plugin Name: SlopeAbs.C
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
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Calculate gradient (slope)

% PlugInName: SlopeAbs

% PlugInAuthor: Percy Zahl

% PlugInAuthorEmail: zahl@users.sf.net

% PlugInMenuPath: Math/Statistics/Slope Abs

% PlugInDescription
Calculation of the absolute local slope (gradient) using a user
defined facet size at each pixel as reference area.  A plane
regression is performed at each pixel to find the best matching local
facet of the given size. Its normal is used to find the gradient.

% PlugInUsage
Activate chanel to use and call it from Menu \emph{Math/Statistics/Slope Abs}.

% OptPlugInSources
The active channel is used.

% OptPlugInDest
Existing Math channel, else newly created Math channel.

% OptPlugInConfig
Can set a default facet size, if set to zero it will ask at each call.

% OptPlugInKnownBugs
No bugs known.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

// Plugin Prototypes
static void SlopeAbs_init( void );
static void SlopeAbs_about( void );
static void SlopeAbs_configure( void );
static void SlopeAbs_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean SlopeAbs_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean SlopeAbs_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin SlopeAbs_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "SlopeAbs-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-ST",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup(" Calculates absolute local slope!"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-statistics-section",
  // Menuentry
  N_("Slope Abs"),
  // help text shown on menu
  N_("Sorry, no help for SlopeAbs filter!"),
  // more info...
  "no more info",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  SlopeAbs_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  SlopeAbs_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  SlopeAbs_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  SlopeAbs_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin SlopeAbs_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin SlopeAbs_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin SlopeAbs_m1s_pi
#else
 GxsmMathTwoSrcPlugin SlopeAbs_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   SlopeAbs_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm SlopeAbs Plugin\n\n"
                                   "SlopeAbs calculates the\n"
				   "absolute local Slope.");

int FacetRadius=2;
int FacetRadiusDefault=0;

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  SlopeAbs_pi.description = g_strdup_printf(N_("Gxsm MathOneArg SlopeAbs plugin %s"), VERSION);
  return &SlopeAbs_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &SlopeAbs_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &SlopeAbs_m2s_pi; 
}
#endif

// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void SlopeAbs_init(void)
{
  PI_DEBUG (DBG_L2, "SlopeAbs Plugin Init");
}

// about-Function
static void SlopeAbs_about(void)
{
  const gchar *authors[] = { SlopeAbs_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  SlopeAbs_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void SlopeAbs_configure(void)
{
  double x = (double)FacetRadiusDefault;
  SlopeAbs_pi.app->ValueRequest("Enter Value", "FacetRadius", 
				"Default FacetRadius (Pixel), if set to Zero I will ask later!",
				SlopeAbs_pi.app->xsm->Unity, 
				1., 100., ".0f", &x);
  FacetRadiusDefault = (int)x;
}

// cleanup-Function
static void SlopeAbs_cleanup(void)
{
  PI_DEBUG (DBG_L2, "SlopeAbs Plugin Cleanup");
}

typedef struct {
  double b,ax,ay;
}APlane;

typedef struct {
  int Cline, Crow, Crx, Cry;
}Facet;

// Do E Regress
void FacetERegress(Scan *Src, Facet *fac, APlane *ap){
  double sumi, sumiy, sumyq, sumiq, sumy, val;
  double ysumi, ysumiy, ysumyq, ysumiq, ysumy;
  double a, b, nx, ny, imean, ymean,ax,bx,ay,by;
  int line, i;

  sumi = sumiq = sumy = sumyq = sumiy = ax = ay = bx = by = 0.0;
  ysumi = ysumiq = ysumy = ysumyq = ysumiy = 0.0;
  nx = ny = 0.;
  
  for (i = -fac->Crx; i <= fac->Crx; ++i){
    sumiq += (double)(i)*(double)(i);
    sumi  += (double)i;
    nx += 1.;
  }
  imean = sumi / nx;
  for (line = -fac->Cry; line <= fac->Cry; line++) {
    sumy = sumyq = sumiy = 0.0;
    for (i = -fac->Crx; i <= fac->Crx; i++) {
      val = Src->mem2d->GetDataPkt(i+fac->Crow, line+fac->Cline);
      sumy   += val;
      sumyq  += val*val;
      sumiy  += val*i;
    }
    ymean = sumy / nx;
    a = (sumiy - nx * imean * ymean ) / (sumiq - nx * imean * imean);
    b =  ymean -  a * imean;
    ax += a;
    bx += b;
    /* Werte fuer y-linregress */
    ysumy  += b;
    ysumyq += b*b;
    ysumiy += b * line;
    ysumiq += (double)(line)*(double)(line);
    ysumi  += (double)line;
    ny     += 1.;
  }
  ax = ax/ny;
  bx = bx/ny;
  imean = ysumi / ny;
  ymean = ysumy / ny;
  ay = (ysumiy - ny * imean * ymean ) / (ysumiq - ny * imean * imean);
  by =  ymean - ay * imean;

  // E-Facet: Y = by + xi*ax + yi*ay

  ap->b  = by;
  ap->ax = ax;
  ap->ay = ay;
}


// run-Function
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 static gboolean SlopeAbs_run(Scan *Src, Scan *Dest)
#else
 static gboolean SlopeAbs_run(Scan *Src1, Scan *Src2, Scan *Dest)
#endif
{
  // put plugins math code here...

  if(FacetRadiusDefault)
    FacetRadius=FacetRadiusDefault;
  else{
      double x=(double)FacetRadius;
      SlopeAbs_pi.app->ValueRequest("Enter Value", "FacetRadius", 
				    "need FacetRadius (in Pixel)!",
				    SlopeAbs_pi.app->xsm->Unity, 
				    1., 100., ".0f", &x);
      FacetRadius = (int)x;
  }

  if(   Src->mem2d->GetNx() <= 2*FacetRadius 
     || Src->mem2d->GetNy() <= 2*FacetRadius)
    return MATH_SIZEERR;

  Dest->mem2d->Resize(Dest->data.s.nx, Dest->data.s.ny, ZD_FLOAT);
  Dest->mem2d->data->MkXLookup(-Src->data.s.rx/2.,
			       +Src->data.s.rx/2.);
  Dest->mem2d->data->MkYLookup(0., Src->data.s.ry);
  UnitObj *u;
  Dest->data.SetZUnit(u=new UnitObj("\B0","\B0")); delete u;
  Dest->data.s.dz  = 1.;
  Dest->data.s.rz  = 90.;
 
  APlane ap;
  Facet  facet;
  facet.Crx   = FacetRadius;
  facet.Cry   = FacetRadius;
  double fac = Src->data.s.dz/((Src->data.s.dx+Src->data.s.dy)/2.);
  for(int line=FacetRadius; line<Src->mem2d->GetNy()-FacetRadius; ++line){
    gchar *mld = g_strdup_printf("Calculating SlopeAbs: %d/%d", 
				 line, Dest->data.s.ny);
    main_get_gapp()->SetStatus(mld); 
    g_free(mld);
    SET_PROGRESS((double)line/(double)Dest->data.s.ny);
    for(int row=FacetRadius; row<Src->mem2d->GetNx()-FacetRadius; ++row){
      facet.Crow  = row;
      facet.Cline = line;
      FacetERegress(Src, &facet, &ap);
      //      Dest->mem2d->PutDataPkt(sqrt(ap.ax*ap.ax + ap.ay*ap.ay)*fac, row, line);
      Dest->mem2d->PutDataPkt(180.*acos(1./sqrt(1. + (ap.ax*ap.ax + ap.ay*ap.ay)*fac*fac))/M_PI, row, line);
      //      Dest->mem2d->PutDataPkt(180.*atan(sqrt(ap.ax*ap.ax + ap.ay*ap.ay)*fac)/M_PI, row, line);
    }
  }
  SET_PROGRESS(0.);

  return MATH_OK;
}
