/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=percy
email	        	=zahl@users.sourceforge.net
pluginname		=removelineshifts
pluginmenuentry 	=Rm Line Shifts
menupath		=_Math/Background/
entryplace		=Background
shortentryplace	=BG
abouttext		=remove line shifts
smallhelp		=removes line shifts by determining jumps in average height inbetween lines using a threshold
longhelp		=removes line shifts by determining jumps in average height inbetween lines using a threshold
 * 
 * Gxsm Plugin Name: removelineshifts.C
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
% PlugInDocuCaption: Remove Line Shifts
% PlugInName: removelineshifts
% PlugInAuthor: percy
% PlugInAuthorEmail: zahl@users.sourceforge.net
% PlugInMenuPath: _Math/Background/Rm Line Shifts

% PlugInDescription
This filter removes $Z$ line shifts from the image background -- most
commonly due to tip changes or any other sudden Z changes from one to
the next line. It works by comparing the 2nd order change of average
scan line $Z$ value to an given threashold value, i.e. it determines a
jump in the $\partial_Y Z$ (Y-slope), if so, this jump is evaluated and $Z$ of
all following lines is ajusted. The the filter asks for the treashold value.

% PlugInUsage
Activate source scan and call it filter from \GxsmMenu{Math/Background/Rm Line Shifts} menu.
Input the desired threashold value.

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
#include "core-source/plugin.h"

// Plugin Prototypes
static void removelineshifts_init( void );
static void removelineshifts_about( void );
static void removelineshifts_configure( void );
static void removelineshifts_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean removelineshifts_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean removelineshifts_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin removelineshifts_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "removelineshifts-"
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
  g_strdup("removes line shifts by determining jumps in average height inbetween lines using a threshold"),                   
  // Author(s)
  "percy",
  // Menupath to position where it is appendet to
  "math-background-section",
  // Menuentry
  N_("Rm Line Shifts"),
  // help text shown on menu
  N_("removes line shifts by determining jumps in average height inbetween lines using a threshold"),
  // more info...
  "removes line shifts by determining jumps in average height inbetween lines using a threshold",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  removelineshifts_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  removelineshifts_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  removelineshifts_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  removelineshifts_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin removelineshifts_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin removelineshifts_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin removelineshifts_m1s_pi
#else
 GxsmMathTwoSrcPlugin removelineshifts_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   removelineshifts_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm removelineshifts Plugin\n\n"
                                   "remove line shifts");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  removelineshifts_pi.description = g_strdup_printf(N_("Gxsm MathOneArg removelineshifts plugin %s"), VERSION);
  return &removelineshifts_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &removelineshifts_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &removelineshifts_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void removelineshifts_init(void)
{
  PI_DEBUG (DBG_L2, "Plugin Init" );
}

// about-Function
static void removelineshifts_about(void)
{
  const gchar *authors[] = { removelineshifts_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  removelineshifts_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void removelineshifts_configure(void)
{
  if(removelineshifts_pi.app)
    removelineshifts_pi.app->message("removelineshifts Plugin Configuration");
}

// cleanup-Function
static void removelineshifts_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Plugin Cleanup");
}


// run-Function
static gboolean removelineshifts_run(Scan *Src, Scan *Dest)
{
	double avg, avg_prev, avg_delta, Z_adjust;
	int nx,ny;
	int x1,x2,y1,y2,dx;
	double threashold = 10.;

	ZData  *SrcZ, *DestZ;
	
	nx=Dest->mem2d->GetNx();
	ny=Dest->mem2d->GetNy();
	
	x1=0; x2=nx; y1=0; y2=ny;
	
	
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
			gapp->setup_multidimensional_data_copy ("Multidimensional Rm Line Shifts", Src, ti, tf, vi, vf);
		} while (ti > tf || vi > vf);

		gapp->progress_info_new ("Multidimenssional Rm Line Shifts", 2);
		gapp->progress_info_set_bar_fraction (0., 1);
		gapp->progress_info_set_bar_fraction (0., 2);
		gapp->progress_info_set_bar_text ("Time", 1);
		gapp->progress_info_set_bar_text ("Value", 2);
	}

	gapp->ValueRequest("Z shift threashold", "Level", "Set threashold [raw]",
			   gapp->xsm->Unity, -1e4, 1e4, ".2f", &threashold);
		
	if (Src->number_of_object () > 0){
		XSM_DEBUG(DBG_L2, "found object(s)" );
		if( Src->get_object_data (0) -> get_num_points () == 2){
			double x=0.;
			double y=0.;
			Src->get_object_data (0) -> get_xy_i (0, x, y);
			Src->World2Pixel (x,y, x1,y1);
			Src->get_object_data (0) -> get_xy_i (1, x, y);
			Src->World2Pixel (x,y, x2,y2);
			XSM_DEBUG(DBG_L2, "got x range " << x1 << " .. " << x2 );
		}
		
		if (x1 > x2){
			int tmp = x1;
			x1 = x2;
			x2 = tmp;
		}
		if (y1 > y2){
			int tmp = y1;
			y1 = y2;
			y2 = tmp;
		}
                if (x1 < 0) x1=0;
                if (x2 > nx) x2=nx;
                if (y1 < 0) y1=0;
                if (y2 > ny) y2=ny;
	}
        dx = x2-x1;

        g_message ("X: %d .. %d ->  dx = %d", x1,x2,dx);
        g_message ("Y: %d .. %d ", y1,y2);
        
	int ntimes_tmp = tf-ti+1;
	for (int time_index=ti; time_index <= tf; ++time_index){
		Mem2d *m = Src->mem2d_time_element (time_index);
		if (multidim)
			gapp->progress_info_set_bar_fraction ((gdouble)(time_index-ti)/(gdouble)ntimes_tmp, 1);

		Dest->mem2d->Resize (m->GetNx (), m->GetNy (), vf-vi+1, m->GetTyp());
		for (int v_index = vi; v_index <= vf; ++v_index){
			m->SetLayer (v_index);
			Dest->mem2d->SetLayer (v_index-vi);

			Dest->mem2d->CopyFrom(m, 0,0, 0,0, nx,ny);
			SrcZ  =  m->data;
			DestZ = Dest->mem2d->data;
			
			Z_adjust = 0.;
			
                        g_message ("avg_start = %g", avg_prev);
			for (int line = y1; line < ny; line++) {
				if (line < y2){

                                        avg_prev = 0.;
                                        if (line > 0){
                                                DestZ->SetPtr(x1,line-1);
                                                for (int i = 0; i < dx; ++i)
                                                        avg_prev += DestZ->GetNext();
                                                avg_prev /= dx;
                                        }
                                        
                                        avg = 0.;
                                        SrcZ->SetPtr (x1,line);
					for (int i = 0; i < dx; ++i)
						avg += SrcZ->GetNext();
                                        avg /= dx;

					avg_delta = avg - avg_prev;
                                        // g_message ("avg[%d] = %g dd=%g", line, avg, avg_delta);

					if ( fabs (avg_delta) > threashold) // 2nd order check
						Z_adjust = avg_delta;

				}
				SrcZ->SetPtr(0,line);
				DestZ->SetPtr(0,line);
				
				for (int col = 0; col < nx; col++)
                                        DestZ->SetNext ( SrcZ->GetNext() - Z_adjust);
                        }
		}
		Dest->append_current_to_time_elements (time_index-ti, m->get_frame_time ());
	}

	Dest->data.s.ntimes = ntimes_tmp;
	Dest->data.s.nvalues=Dest->mem2d->GetNv ();

	if (multidim){
		gapp->progress_info_close ();
		Dest->retrieve_time_element (0);
		Dest->mem2d->SetLayer(0);
	}

	return MATH_OK;
}


