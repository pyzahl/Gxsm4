/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Quick nine points Gxsm Plugin GUIDE by PZ:
 * ------------------------------------------------------------
 * 1.) Make a copy of this "PolarHist.C" to "your_plugins_name.C"!
 * 2.) Replace all "PolarHist" by "your_plugins_name" 
 *     --> please do a search and replace starting here NOW!! (Emacs doese preserve caps!)
 * 3.) Decide: One or Two Source Math: see line 54
 * 4.) Fill in GxsmPlugin Structure, see below
 * 5.) Replace the "about_text" below a desired
 * 6.) * Optional: Start your Code/Vars definition below (if needed more than the run-fkt itself!), 
 *       Goto Line 155 ff. please, and see comment there!!
 * 7.) Fill in math code in PolarHist_run(), 
 *     have a look at the Data-Access methods infos at end
 * 8.) Add PolarHist.C to the Makefile.am in analogy to others
 * 9.) Make a "make; make install"
 * A.) Call Gxsm->Tools->reload Plugins, be happy!
 * ... That's it!
 * 
 * Gxsm Plugin Name: PolarHist.C
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
% PlugInDocuCaption: Generate a polar histogramm
% PlugInName: polarhist
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Statistics/Polar Histogram

% PlugInDescription
This PlugIn generates from two data sets one (polar) histogram...

% PlugInUsage
Example:
\begin{enumerate}
\item open the demo Ge pyramid image
\item optional for slow machines, scale by x and y 0.2
  (\GxsmMenu{Math/Transformation/Scan Scan}), then activate this
  channel
\item call \GxsmMenu{Math/Statistics/Slope Abs}, use Facet Radius = 5
    (with scale down image)
\item do \GxsmEmph{Autodisp} on resulting \GxsmEmph{Math} channel
\item activate the original (e.g. scaled down image)
\item set a free channel to \GxsmEmph{Math}
\item call \GxsmMenu{Math/Statistics/Slope Dir}
\item the ``Slope Abs'' resulting channel to \GxsmEmph{Mode-Active}
\item the ``Slope Dir'' resulting channel to \GxsmEmph{Mode-X}
\item call \GxsmMenu{Math/Statistics/Polar Hist}\\
angular slices 180\\
data channels  200\\
data start 0\\
data end   45\\
Vmode 1\\
\end{enumerate}

data start = data end = 0 : auto ranging (min/max) is used

% OptPlugInSources
The active channel is used as histogramm data source, the X-channel is used as bin

% OptPlugInObjects
A optional rectangle is used for data extraction...

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

%% OptPlugInConfig
%

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

% OptPlugInKnownBugs
Produces sometimes strange output... pending to fix!

% OptPlugInNotes
Docu not finished jet, PlugIn makes Gxsm unstable after usage -- work in progress.

%% OptPlugInHints
%

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"

// Plugin Prototypes
static void PolarHist_init( void );
static void PolarHist_about( void );
static void PolarHist_configure( void );
static void PolarHist_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
// #define GXSM_ONE_SRC_PLUGIN__DEF
#define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean PolarHist_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean PolarHist_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin PolarHist_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	g_strdup ("PolarHist-"
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
	g_strdup("Calculates a Polar Histogramm from two Srcs: data, angle"),
	// Author(s)
	"Percy Zahl",
	// Menupath to position where it is appendet to
	"math-statistics-section",
	// Menuentry
	N_("Polar Histogramm"),
	// help text shown on menu
	N_("from two Channels: 1.: data 2.:polar angle!"),
	// more info...
	"no more info",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	PolarHist_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	PolarHist_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	PolarHist_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

	PolarHist_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin PolarHist_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin PolarHist_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin PolarHist_m1s_pi
#else
GxsmMathTwoSrcPlugin PolarHist_m2s_pi
#endif
= {
	// math-function to run, see prototype(s) above!!
	PolarHist_run
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm PolarHist Plugin\n\n"
                                   "This Plugin calculates a polar\n"
                                   "Histogramm of data in Src1,\n"
                                   "polar angle in Src2.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	PolarHist_pi.description = g_strdup_printf(N_("Gxsm MathOneArg PolarHist plugin %s"), VERSION);
	return &PolarHist_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
	return &PolarHist_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
	return &PolarHist_m2s_pi; 
}
#endif

// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//

int PolSlices    = 180;
int DataChannels = 200;
double DataStart = 0.;
double DataEnd   = 90.;
int Vmode        = 2;

// init-Function
static void PolarHist_init(void)
{
	PI_DEBUG (DBG_L2, "PolarHist Plugin Init");
}

// about-Function
static void PolarHist_about(void)
{
	const gchar *authors[] = { PolarHist_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  PolarHist_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void PolarHist_configure(void)
{
	GtkDialogFlags flags =  (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);
	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("Polar Histogramm Setup"),
							 GTK_WINDOW (main_get_gapp()->get_app_window ()),
							 flags,
							 _("_OK"),
							 GTK_RESPONSE_ACCEPT,
							 _("_Cancel"),
							 GTK_RESPONSE_REJECT,
							 NULL);
	BuildParam bp;
  	gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);
  
	bp.grid_add_label ("Polar Histogramm Setup"); bp.new_line ();
	bp.grid_add_ec ("# angular slices", PolarHist_pi.app->xsm->Unity, 
			&PolSlices, 
			2., 3600., ".0f");
	bp.grid_add_ec ("# data channels", PolarHist_pi.app->xsm->Unity, 
			&DataChannels,
			2., 100000., ".0f");

	bp.grid_add_label ("Data Range (0...0 -> auto)");
	bp.new_line ();
	bp.grid_add_ec ("Data Start", PolarHist_pi.app->xsm->Unity, 
			&DataStart,
			-1e10, 1e10, ".3f");
	bp.new_line ();

	bp.grid_add_ec ("Data End", PolarHist_pi.app->xsm->Unity, 
			&DataEnd,
			-1e10, 1e10, ".3f");
	bp.new_line ();

	bp.grid_add_ec ("Vmode 1=cir,2=rect", PolarHist_pi.app->xsm->Unity, 
			&Vmode,
			0., 10., ".0f");
	bp.new_line ();
	
        gtk_widget_show (dialog);
        int response = GTK_RESPONSE_NONE;
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
        
        // FIX-ME GTK4 ??
        // wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

} 

// cleanup-Function
static void PolarHist_cleanup(void)
{
	PI_DEBUG (DBG_L2, "PolarHist Plugin Cleanup");
}

double Phi(double dx, double dy){
	double q23=0.;
	if(dx<0.){
		q23=180.;
		if(dy<0. )
			q23=-180.;
	}

	if(fabs(dx)>1e-6)
		return q23+180.*atan(dy/dx)/M_PI;
	else return dy>0.?90.:-90.;
}

// run-Function
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
static gboolean PolarHist_run(Scan *Src, Scan *Dest)
#else
	static gboolean PolarHist_run(Scan *Src1, Scan *Src2, Scan *Dest)
#endif
{
	// put plugins math code here...
	if(Src1->data.s.nx != Src2->data.s.nx || Src1->data.s.ny != Src2->data.s.ny)
		return MATH_SELECTIONERR;

	PolarHist_configure();

	// easy and fast acess macro
#define GetNumValPhi(V,P)  Num[PolSlices*(V) + (P)]
#define IncNumValPhi(V,P)  ++Num[PolSlices*(V) + (P)]
	long *Num = new long[PolSlices * DataChannels];
	memset((void *)Num, 0, sizeof(Num));

	// Autorange?
	if((DataStart == 0. && DataEnd == 0.) || DataStart > DataEnd)
		Src1->mem2d->HiLo (&DataEnd, &DataStart, FALSE, NULL, NULL, 1);

	double DataRange = DataEnd - DataStart;
	double SliceAng  = 360./(double)PolSlices;

	// Now do Counting
	for(int line=0; line<Src1->data.s.ny; line++)
		for(int col=0; col<Src1->data.s.nx; col++){
			int v = (int)rint((Src1->mem2d->GetDataPkt(col, line)-DataStart)*(double)DataChannels/DataRange);
			int p = (int)rint(Src2->mem2d->GetDataPkt(col, line)/SliceAng);
			p += PolSlices; p = p % PolSlices;
			if( v < 0 || v >= DataChannels || p < 0 || p >= PolSlices)
				continue;
			IncNumValPhi(v, p);
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
					Dest->mem2d->PutDataPkt((double)GetNumValPhi(v, p), col, line);
				else{
					double sumup=0.;
					if(v>0){
						for(int i=p-(int)((double)DataChannels/2/M_PI/v); 
						    i<=p+(int)((double)DataChannels/2/M_PI/v); ++i)
							sumup += GetNumValPhi(v, (i+PolSlices)%PolSlices);
						sumup /= (1+2*(int)((double)DataChannels/2/M_PI/v));
						sumup *= (double)DataChannels/2/M_PI/v;
					}
					else
						for(int i=0; i<PolSlices; ++i)
							sumup += GetNumValPhi(v, i);
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
				Dest->mem2d->PutDataPkt((double)GetNumValPhi(col, line), col, line);
		break;
	}

	delete[] Num;

	return MATH_OK;
}

// END
