/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=AngularAnalysis
pluginmenuentry 	=Angular Analysis
menupath		=Math/Statistics/
entryplace		=Statistics
shortentryplace	=ST
abouttext		=Calculate all gradients and present slope, direction as polar histogramm
smallhelp		=Angular Analysis in polar representation
longhelp		=Calculate all gradients and present slope, direction as polar histogramm
 * 
 * Gxsm Plugin Name: AngularAnalysis.C
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
% PlugInDocuCaption: Angular Analysis
% PlugInName: AngularAnalysis
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Statistics/Angular Analysis

% PlugInDescription
Calculate all local gradients and presents those in a polar histogram
of slope as radius and direction as polar angle.

% PlugInUsage
Call \GxsmMenu{Math/Statistics/Angular Analysis}.

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
static void AngularAnalysis_init( void );
static void AngularAnalysis_about( void );
static void AngularAnalysis_configure( void );
static void AngularAnalysis_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean AngularAnalysis_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean AngularAnalysis_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin AngularAnalysis_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  g_strdup ("AngularAnalysis-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
	    "-ST"),
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup("Calculate all gradients and present slope, direction as polar histogramm"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-statistics-section",
  // Menuentry
  N_("Angular Analysis"),
  // help text shown on menu
  N_("Calculate all gradients and present slope, direction as polar histogramm"),
  // more info...
  "Angular Analysis in polar representation",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  AngularAnalysis_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  AngularAnalysis_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  AngularAnalysis_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  AngularAnalysis_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin AngularAnalysis_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin AngularAnalysis_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin AngularAnalysis_m1s_pi
#else
 GxsmMathTwoSrcPlugin AngularAnalysis_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   AngularAnalysis_run
 };

// Plugin's config vars
int FacetRadius  = 5;
int PolSlices    = 90;
int DataChannels = 60;
double DataStart = 0.;
double DataEnd   = 25.;
int Vmode        = 1;

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm AngularAnalysis Plugin\n\n"
                                   "Calculate all gradients and present slope, direction as polar histogramm");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  AngularAnalysis_pi.description = g_strdup_printf(N_("Gxsm MathOneArg AngularAnalysis plugin %s"), VERSION);
  return &AngularAnalysis_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &AngularAnalysis_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &AngularAnalysis_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void AngularAnalysis_init(void)
{
  PI_DEBUG (DBG_L2, "AngularAnalysis Plugin Init");
}

// about-Function
static void AngularAnalysis_about(void)
{
  const gchar *authors[] = { AngularAnalysis_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  AngularAnalysis_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void AngularAnalysis_configure(void)
{
  GtkDialogFlags flags = GTK_DIALOG_MODAL; // | GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("Angular Analysis Setup"),
						   NULL,
						   flags,
						   N_("_OK"),
						   GTK_RESPONSE_OK,
						   N_("_CANCEL"),
						   GTK_RESPONSE_CANCEL,
						   NULL);
  BuildParam bp;
  gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);
 
  bp.grid_add_label ("Polar Histogramm Setup");
  bp.new_line ();
  bp.grid_add_ec ("Facet Radius", AngularAnalysis_pi.app->xsm->Unity, 
		  &FacetRadius, 
		  1., 100., ".0f");
  bp.new_line ();

  bp.grid_add_ec ("# angular slices", AngularAnalysis_pi.app->xsm->Unity, 
		  &PolSlices, 
		  2., 3600., ".0f");
  bp.new_line ();

  bp.grid_add_ec ("# data channels", AngularAnalysis_pi.app->xsm->Unity, 
		  &DataChannels,
		  2., 100000., ".0f");
  bp.new_line ();

  bp.grid_add_label ("Data Range (0...0 -> auto)");
  bp.new_line ();

  bp.grid_add_ec ("Data Start", AngularAnalysis_pi.app->xsm->Unity,
		  &DataStart,
		  -1e10, 1e10, ".3f");
  bp.new_line ();

  bp.grid_add_ec ("Data End", AngularAnalysis_pi.app->xsm->Unity, 
		  &DataEnd,
		  -1e10, 1e10, ".3f");
  bp.new_line ();

  bp.grid_add_ec ("Vmode 1=cir,2=rect", AngularAnalysis_pi.app->xsm->Unity, 
		  &Vmode,
		  0., 10., ".0f");
  
  gtk_widget_show (dialog);

  int response = GTK_RESPONSE_NONE;
  g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
  
  // FIX-ME GTK4 ??
  // wait here on response
  while (response == GTK_RESPONSE_NONE)
    while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

  return response == GTK_RESPONSE_OK ? 1 : 0;
} 

// cleanup-Function
static void AngularAnalysis_cleanup(void)
{
  PI_DEBUG (DBG_L2, "AngularAnalysis Plugin Cleanup");
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

double Phi(double dx, double dy){
	double q23=0.;
	if(dx<0.){
		q23=180.;
		if(dy<0. )
			q23=-180;
	}

	if(fabs(dx)>1e-5)
		return q23+180.*atan(dy/dx)/M_PI;
	else return dy>0.?90.:-90.;
}

void inline IncNumValPhi(double *n, int v, int p){ n[PolSlices*v + p] += 1.; }
double inline GetNumValPhi(double *n, int v, int p){ return n[PolSlices*v + p]; }

// run-Function
static gboolean AngularAnalysis_run(Scan *Src, Scan *Dest)
{
	AngularAnalysis_configure();
	int numsize = PolSlices * DataChannels;
	double *Num = new double[numsize];
	for (int i=0; i<numsize; Num[i++]=0.);

	double DataRange = DataEnd - DataStart;
	double SliceAng  = 360./(double)PolSlices;
  
	APlane ap;
	Facet  facet;
	facet.Crx   = FacetRadius;
	facet.Cry   = FacetRadius;
	double fac = Src->data.s.dz/((Src->data.s.dx+Src->data.s.dy)/2.);

	// Now do Calculation and Counting
	for(int line=FacetRadius; line<Src->mem2d->GetNy()-FacetRadius; ++line){
		gchar *mld = g_strdup_printf("Calculating Angular Dist: %d/%d", 
					     line, Dest->data.s.ny);
		main_get_gapp()->SetStatus(mld); 
		g_free(mld);
		SET_PROGRESS((double)line/(double)Dest->data.s.ny);
		for(int row=FacetRadius; row<Src->mem2d->GetNx()-FacetRadius; ++row){
			facet.Crow  = row;
			facet.Cline = line;
			FacetERegress(Src, &facet, &ap);
			double phi = Phi(ap.ax, ap.ay);
			double slp = 180.*acos(1./sqrt(1. + (ap.ax*ap.ax + ap.ay*ap.ay)*fac*fac))/M_PI;
			int v = (int)rint((slp-DataStart)*(double)DataChannels/DataRange);
			int p = (int)rint(phi/SliceAng);
			p += PolSlices; p = p % PolSlices;
			if( v < 0 || v >= DataChannels || p < 0 || p >= PolSlices)
				continue;
			IncNumValPhi(Num, v, p);
		}
	}

	// Setup Destination
	// and Fillup Polar Plot
	UnitObj *u;
	Dest->data.SetXUnit(u=new UnitObj("rPhi","rPhi"));
	Dest->data.SetYUnit(u); delete u;
	Dest->data.SetZUnit(u=new UnitObj("#","#")); delete u;
	Dest->data.s.dz = 1.;

	switch(Vmode){
	case 1:
		Dest->data.s.nx = Dest->data.s.ny = DataChannels*2-1;
		Dest->mem2d->Resize(Dest->data.s.nx, Dest->data.s.ny, ZD_FLOAT);
		Dest->data.s.rx = Dest->data.s.ry = DataRange;
		Dest->data.s.x0 = Dest->data.s.y0 = DataStart;
		Dest->mem2d->data->MkXLookup(-DataRange, DataRange);
		Dest->mem2d->data->MkYLookup(-DataRange, DataRange);
		for(int line=0; line<Dest->data.s.ny; line++)
			for(int col=0; col<Dest->data.s.nx; col++){
				int dx= col-Dest->data.s.nx/2;
				int dy=line-Dest->data.s.ny/2;
				int v = (int)rint(sqrt(dx*dx+dy*dy));
				int p = (int)rint(Phi(dx, dy)/SliceAng);
				p += PolSlices; p = p % PolSlices;
				if( v < 0 || v >= DataChannels || p < 0 || p >= PolSlices)
					continue;
				if(v > (int)((double)DataChannels/2/M_PI))
					Dest->mem2d->PutDataPkt(GetNumValPhi(Num, v, p), col, line);
				else{
					double sumup=0.;
					if(v>0){
						for(int i=p-(int)((double)DataChannels/2/M_PI/v); 
						    i<=p+(int)((double)DataChannels/2/M_PI/v); ++i)
							sumup += GetNumValPhi(Num, v, (i+PolSlices)%PolSlices);
						sumup /= (1+2*(int)((double)DataChannels/2/M_PI/v));
						sumup *= (double)DataChannels/2/M_PI/v;
					}
					else
						for(int i=0; i<PolSlices; ++i)
							sumup += GetNumValPhi(Num, v, i);
					Dest->mem2d->PutDataPkt(sumup, col, line);
				}
			}
		break;
	default:
		Dest->data.s.nx = DataChannels;
		Dest->data.s.ny = PolSlices;
		Dest->mem2d->Resize(Dest->data.s.nx, Dest->data.s.ny, ZD_LONG);
		Dest->data.s.rx = Dest->data.s.ry = DataRange;
		Dest->data.s.x0 = Dest->data.s.y0 = DataStart;
		Dest->mem2d->data->MkXLookup(0., DataRange);
		Dest->mem2d->data->MkYLookup(0., 360.);
		for(int line=0; line<Dest->data.s.ny; line++)
			for(int col=0; col<Dest->data.s.nx; col++)
				Dest->mem2d->PutDataPkt(GetNumValPhi(Num, col, line), col, line);
		break;
	}

	delete[] Num;

	return MATH_OK;
}


