/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: workfuncextract.C
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
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Smoothing layers in 3D
% PlugInName: workfuncextract
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Misc/Workfuncextract

% PlugInDescription
This filter extracts from a series of images in energy
(layer-dimension) the workfunction assuming the image intensity
dropping all sudden by a given percentage from it's near zero energy
value. (LEEM Mirror-Mode to Regular imaging transition detection) and
puts teh resulting energy (layer corresponding value) into the
destination image.

% PlugInUsage
Call \GxsmMenu{Math/Misc/Workfuncextract}\dots

% OptPlugInSources
The active channel is used as data source.

%% OptPlugInObjects

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

%% OptPlugInConfig
%

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

%% OptPlugInKnownBugs
%

% OptPlugInNotes

%% OptPlugInHints
%

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
static void workfuncextract_init (void);
static void workfuncextract_about (void);
static void workfuncextract_configure (void);
static void workfuncextract_cleanup (void);

// Math-Run-Function, "TwoSrc" Prototype
static gboolean workfuncextract_run (Scan *Src, Scan *Dest);

// Fill in the GxsmPlugin Description here
GxsmPlugin workfuncextract_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  g_strdup ("Workfuncextract-"
	    "M1S"
	    "-Misc"),
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("LEEM analysis: Workfunction extract."),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-misc-section",
  // Menuentry
  N_("Workfuncextract"),
  // help text shown on menu
  N_("Workfunction (Energy) extractiong from series of layers in energy. (LEEM)"),
  // more info...
  "no more info",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  workfuncextract_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  workfuncextract_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  workfuncextract_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  workfuncextract_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin workfuncextract_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin workfuncextract_m2s_pi -> "TwoSrcMath"
 GxsmMathOneSrcPlugin workfuncextract_m1s_pi
 = {
   // math-function to run, see prototype(s) above!!
   workfuncextract_run
 };

typedef struct{
	Scan* Src;
	Scan* Dest;
	int y_i, y_f;
	int x_i, x_f;
	int l_i, l_f;
	double threshold;
	double radius;
	double energy_res;	
	int *status;
	int job;
	int progress;
} WFE_Job_Env;

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm Workfuncextract Plugin\n\n"
                                   "Workfunction extract from layers.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  workfuncextract_pi.description = g_strdup_printf(N_("Gxsm MathOneArg workfuncextract plugin %s"), VERSION);
  return &workfuncextract_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) { 
  return &workfuncextract_m1s_pi; 
}

//
// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void workfuncextract_init(void)
{
  PI_DEBUG (DBG_L2, "Workfuncextract Plugin Init");
}

// about-Function
static void workfuncextract_about(void)
{
  const gchar *authors[] = { workfuncextract_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  workfuncextract_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void workfuncextract_configure(void)
{
  if(workfuncextract_pi.app)
    workfuncextract_pi.app->message("Workfuncextract Plugin Configuration");
}

// cleanup-Function
static void workfuncextract_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Workfuncextract Plugin Cleanup");
}

gpointer WFE_thread (void *env){
	WFE_Job_Env* job = (WFE_Job_Env*)env;
	double val, val_p, val_threshold;
	int delta = (job->l_f - job->l_i) > 16 ? 16:1;

//	std::cout << "WFE:thread " << job->x_i << ":" << job->y_i  << std::endl;

	for (int yi=job->y_i; yi < job->y_f; yi++){
		if (*job->status) break;

		for (int xi=job->x_i; xi < job->x_f; xi++){
			if (*job->status) break;
			
			val_threshold = job->Src->mem2d->GetDataPktInterpol(xi, yi, job->radius, job->l_i) * job->threshold;
//			std::cout << "WFE:thread " << job->x_i << ":" << job->y_i  << " search VT<" << val_threshold << std::endl;
			for (int lv=job->l_i+delta; lv < job->l_f; lv+=delta){
				if (lv > job->Src->mem2d->GetNv ()-2){
//					std::cout << "WFE:thread " << job->x_i << ":" << job->y_i  << " search VT_lv=NONE" << std::endl;
					job->Dest->mem2d->PutDataPkt (0., xi, yi, 0);
					break;
				}
				if (job->Src->mem2d->GetDataPktInterpol(xi, yi, job->radius, lv) < val_threshold){
					if (delta > 2){
						lv -= delta;
						delta /= 2;
					} else {
						if (lv > 0 && lv < job->Src->mem2d->GetNv ()){
//							std::cout << "WFE:thread " << job->x_i << ":" << job->y_i  << " search VT_lv=" << lv << std::endl;

							job->Dest->mem2d->PutDataPkt (job->Src->mem2d->data->GetVLookup(lv), xi, yi, 0);
						} else { 
//							std::cout << "WFE:thread " << job->x_i << ":" << job->y_i  << " search VT_lv=NONE" << std::endl;
							job->Dest->mem2d->PutDataPkt (0., xi, yi, 0);
						}
						break;
					}
				}
					
			}
			job->progress++;
		}
	}
	job->job = -1; // done indicator
	return NULL;
}

static void cancel_callback (GtkWidget *widget, int *status){
	*status = 1; 
}
	

// run-Function
static gboolean workfuncextract_run(Scan *Src, Scan *Dest)
{
	int status = 0;			// set to 1 to stop the plugin
	double xy[2]; 			// real world position of a probe_event
	int Nx = Src->mem2d->GetNx ();	// total Number of points in x
	int Ny = Src->mem2d->GetNy ();	// total Number of points in y

	double rxy = 1.;    // Get Radius
	double rv  = 1.;    // Get Radius
	double threshold = 0.9; // Workfunction threshold

		
	main_get_gapp()->ValueRequest ("WFExt Filter Size", "Radius in XY Dim", "Smooth kernel size: s = 1+radius",
			    main_get_gapp()->xsm->Unity, 0., Src->mem2d->GetNx()/10., ".0f", &rxy);
	
//	main_get_gapp()->ValueRequest ("WFExt Filter Size", "Radius in Layer Dim", "Smooth kernel size: s = 1+radius",
//			    main_get_gapp()->xsm->Unity, 0., Src->mem2d->GetNv()/10., ".0f", &rv);
	
	main_get_gapp()->ValueRequest ("WFExt Filter Thres", "Workfunc. threshold 0..1", "Threshold",
			    main_get_gapp()->xsm->Unity, 0., 1., ".3f", &threshold);
	
	Dest->mem2d->Resize (Src->mem2d->GetNx(), Src->mem2d->GetNy(), 1);

	main_get_gapp()->progress_info_new ("WFE...", 1, GCallback (cancel_callback), &status);
	main_get_gapp()->progress_info_set_bar_fraction (0., 1);
	main_get_gapp()->progress_info_set_bar_text (" ", 1);

	#define MAX_JOB 16
	WFE_Job_Env wfe_job[MAX_JOB];
	GThread* tpi[MAX_JOB];

	std::cout << "WFE:run ** spaning thread jobs." << std::endl;

	// to be threadded:
	//	for (int yi=0; yi < Ny; yi++)...
	int col_per_job = Nx / (MAX_JOB-1);
	int xi = 0;
	int progress_max_job = col_per_job * Ny;
	for (int jobno=0; jobno < MAX_JOB; ++jobno){
		wfe_job[jobno].Src  = Src;
		wfe_job[jobno].Dest = Dest;
		wfe_job[jobno].y_i = 0;
		wfe_job[jobno].y_f = Ny;
		wfe_job[jobno].x_i = xi;
		xi += col_per_job;
		if (xi < 1 || xi > Nx) xi = Nx;
		wfe_job[jobno].x_f = xi;
		wfe_job[jobno].l_i = 0;
		wfe_job[jobno].l_f = Src->mem2d->GetNv();
		wfe_job[jobno].radius = 1+rxy;
		wfe_job[jobno].energy_res = 0.;
		wfe_job[jobno].threshold = threshold;
		wfe_job[jobno].status = &status;
		wfe_job[jobno].job = jobno+1;
		wfe_job[jobno].progress = 0;
		gchar *tid = g_strdup_printf ("WFE_thread_%d", jobno);
		tpi[jobno] = g_thread_new (tid, WFE_thread, &wfe_job[jobno]);
		g_free (tid);
	}

	std::cout << "WFE:run ** checking on jobs progress." << std::endl;

	int job=0;
	double psum_mx=0.;
	do {
		double psum=0.;
		job=0;
		for (int jobno=0; jobno < MAX_JOB; ++jobno){
			psum += wfe_job[jobno].progress;
			job  += wfe_job[0].job;
		}
		if (psum > psum_mx)
			psum_mx = psum;
		gchar *tmp = g_strdup_printf("%i %%", (int)(100.*psum_mx/MAX_JOB/progress_max_job));
		main_get_gapp()->progress_info_set_bar_text (tmp, 1);
		g_free (tmp);
		main_get_gapp()->check_events ();
	} while (job >= 0);

	std::cout << "WFE:run ** finishing up jobs." << std::endl;

	main_get_gapp()->progress_info_set_bar_text ("finishing up jobs", 1);
	main_get_gapp()->check_events ();

	for (int jobno=0; jobno < MAX_JOB; ++jobno)
		g_thread_join (tpi[jobno]);

	std::cout << "WFE:run ** cleaning up." << std::endl;

	main_get_gapp()->progress_info_close ();

	return MATH_OK;
}

// end of workfuncextract.C
