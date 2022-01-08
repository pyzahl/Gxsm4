/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: despike1d.C
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
% PlugInDocuCaption: Despike 1d
% PlugInName: despike1d
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Background/Despike1d

% PlugInDescription
Despike 1d filter.

% PlugInUsage
Call \GxsmMenu{Math/Filter 1D/Despike}.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"

// Plugin Prototypes
static void despike1d_init( void );
static void despike1d_about( void );
static void despike1d_configure( void );
static void despike1d_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean despike1d_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean despike1d_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin despike1d_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "despike1d-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-BG",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup("Despike1d math filter."),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-filter1d-section",
  // Menuentry
  N_("Despike"),
  // help text shown on menu
  N_("Despike 1d filter action on data."),
  // more info...
  "Despike 1d filter data.",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  despike1d_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  despike1d_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  despike1d_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  despike1d_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin despike1d_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin despike1d_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin despike1d_m1s_pi
#else
 GxsmMathTwoSrcPlugin despike1d_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   despike1d_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm Despike 1d Plugin\n\n"
                                   "This Plugin does a despike1d action with data:\n"
				   "data = data or average of surrounding pixels if spike detected;"
	);

double ConstLast = 0.;
double constval = 0.;

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  despike1d_pi.description = g_strdup_printf(N_("Gxsm MathOneArg despike1d plugin %s"), VERSION);
  return &despike1d_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &despike1d_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &despike1d_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void despike1d_init(void)
{
  PI_DEBUG (DBG_L2, "despike1d Plugin Init");
}

// about-Function
static void despike1d_about(void)
{
  const gchar *authors[] = { despike1d_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  despike1d_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void despike1d_configure(void)
{
  if(despike1d_pi.app)
    despike1d_pi.app->message("Despike1d Plugin Configuration");
}

// cleanup-Function
static void despike1d_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Despike1d Plugin Cleanup");
}

// run-Function
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 static gboolean despike1d_run(Scan *Src, Scan *Dest)
#else
 static gboolean despike1d_run(Scan *Src1, Scan *Src2, Scan *Dest)
#endif
{
	int line, col;

	int ti=0; 
	int tf=0;
	int vi=0;
	int vf=0;
	gboolean multidim = FALSE;
	
	if (Src->data.s.ntimes != 1 || Src->mem2d->GetNv () != 1){
		multidim = TRUE;
		do {
			ti=vi=0;
			tf=Src->number_of_time_elements ()-1;
			if (tf < 0) tf = 0;
			vf=Src->mem2d->GetNv ()-1;
			main_get_gapp()->setup_multidimensional_data_copy ("Multidimensional Despike1d", Src, ti, tf, vi, vf);
		} while (ti > tf || vi > vf);

		main_get_gapp()->progress_info_new ("Multidimenssional Despike1d", 2);
		main_get_gapp()->progress_info_set_bar_fraction (0., 1);
		main_get_gapp()->progress_info_set_bar_fraction (0., 2);
		main_get_gapp()->progress_info_set_bar_text ("Time", 1);
		main_get_gapp()->progress_info_set_bar_text ("Value", 2);
	}

	int ntimes_tmp = tf-ti+1;
	for (int time_index=ti; time_index <= tf; ++time_index){
		Mem2d *m = Src->mem2d_time_element (time_index);
		if (multidim)
			main_get_gapp()->progress_info_set_bar_fraction ((gdouble)(time_index-ti)/(gdouble)ntimes_tmp, 1);

		Dest->mem2d->Resize (m->GetNx (), m->GetNy (), vf-vi+1, m->GetTyp());
		for (int v_index = vi; v_index <= vf; ++v_index){
			m->SetLayer (v_index);
			Dest->mem2d->SetLayer (v_index-vi);

			int nx=Dest->mem2d->GetNx();
			int my=Dest->mem2d->GetNy();
			Dest->mem2d->CopyFrom(m, 0,0, 0,0, Dest->mem2d->GetNx(),Dest->mem2d->GetNy());
			for(int i=1;i<my;i++){
				int q, j;
				double avg, avgdev;
    
				{
					double sum=0.;
					double absdevsum=0.;
					double diff;
					for(j=0;j<nx;j++){
						diff = m->GetDataPkt(j,i) - m->GetDataPkt(j,i-1);
						sum += diff;
						absdevsum += fabs(diff);
					}
					avg = sum/nx;
					avgdev = absdevsum/(nx-1);
				}
				for(q=0;q<4;q++)	{
					double sum, dsum, na;
					for(na=dsum=sum=0.0,j=0;j<nx;j++)	{
						const double sdev = (m->GetDataPkt(j,i) - m->GetDataPkt(j,i-1) - avg)/avgdev;
						const double k = 1/(3+sdev*sdev);
						sum += k*sdev;
						dsum += k*sdev*sdev;
						na += k;
					}
					avg += avgdev*sum/na;
					avgdev = 0.2*avgdev + 0.8*sqrt(dsum/(na-1));
				}
                                for(j=0;j<nx;j++)
                                        Dest->mem2d->PutDataPkt(m->GetDataPkt(j,i) - avg, j,i);
#if 0
				{
					SHT iavg = (SHT)(rint(avg));
					for(j=0;j<nx;j++)
						Dest->mem2d->PutDataPkt(m->GetDataPkt(j,i) - iavg, j,i);
				}
#endif
			}

		}

		Dest->append_current_to_time_elements (time_index-ti, m->get_frame_time ());
	}

	Dest->data.s.ntimes = ntimes_tmp;
	Dest->data.s.nvalues=Dest->mem2d->GetNv ();

	if (multidim){
		main_get_gapp()->progress_info_close ();
		Dest->retrieve_time_element (0);
		Dest->mem2d->SetLayer(0);
	}
	
	return MATH_OK;
}

