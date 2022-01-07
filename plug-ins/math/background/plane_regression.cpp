/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=plane_regression
pluginmenuentry 	=Plane Regression
menupath		=math-background-section
entryplace		=_Background
shortentryplace	=BG
abouttext		=Background Correction using a plane regression possible restricked by several rectangles
smallhelp		=Background Correction using a plane regression possible restricked by several rectangles
longhelp		=Background Correction using a plane regression possible restricked by several rectangles
 * 
 * Gxsm Plugin Name: plane_regression.C
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
% PlugInDocuCaption: Plane Regression
% PlugInName: plane_regression
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Background/Plane Regression

% PlugInDescription
Plane Regression.

% PlugInUsage
Write how to use it.

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
Hacking Version only.
%If you have any additional notes

%% OptPlugInHints
%Any tips and tricks?

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

// Plugin Prototypes
static void plane_regression_init( void );
static void plane_regression_about( void );
static void plane_regression_configure( void );
static void plane_regression_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
static gboolean plane_regression_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
static gboolean plane_regression_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin plane_regression_pi = {
        NULL,                   // filled in and used by Gxsm, don't touch !
        NULL,                   // filled in and used by Gxsm, don't touch !
        0,                      // filled in and used by Gxsm, don't touch !
        NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
        // filled in here by Gxsm on Plugin load, 
        // just after init() is called !!!
        // ----------------------------------------------------------------------
        // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
        "plane_regression-"
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
        g_strdup("Background Correction using a plane regression possible restricked by several rectangles"),                   
        // Author(s)
        "Percy Zahl",
        // Menupath to position where it is appendet to
        "math-background-section",
        // Menuentry
        N_("Plane Regression"),
        // help text shown on menu
        N_("Background Correction using a plane regression possible restricked by several rectangles"),
        // more info...
        "Background Correction using a plane regression possible restricked by several rectangles",
        NULL,          // error msg, plugin may put error status msg here later
        NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
        // init-function pointer, can be "NULL", 
        // called if present at plugin load
        plane_regression_init,  
        // query-function pointer, can be "NULL", 
        // called if present after plugin init to let plugin manage it install itself
        NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
        // about-function, can be "NULL"
        // can be called by "Plugin Details"
        plane_regression_about,
        // configure-function, can be "NULL"
        // can be called by "Plugin Details"
        plane_regression_configure,
        // run-function, can be "NULL", if non-Zero and no query defined, 
        // it is called on menupath->"plugin"
        NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
        // cleanup-function, can be "NULL"
        // called if present at plugin removal
        NULL, // direct menu entry callback1 or NULL
        NULL, // direct menu entry callback2 or NULL

        plane_regression_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin plane_regression_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin plane_regression_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin plane_regression_m1s_pi
#else
GxsmMathTwoSrcPlugin plane_regression_m2s_pi
#endif
= {
        // math-function to run, see prototype(s) above!!
        plane_regression_run
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm plane_regression Plugin\n\n"
                                   "Background Correction using a plane regression possible restricked by several rectangles");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
        plane_regression_pi.description = g_strdup_printf(N_("Gxsm MathOneArg plane_regression plugin %s"), VERSION);
        return &plane_regression_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
        return &plane_regression_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
        return &plane_regression_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void plane_regression_init(void)
{
        PI_DEBUG (DBG_L2, "Plugin Init" );
}

// about-Function
static void plane_regression_about(void)
{
        const gchar *authors[] = { plane_regression_pi.authors, NULL};
        gtk_show_about_dialog (NULL, 
                               "program-name",  plane_regression_pi.name,
                               "version", VERSION,
                               "license", GTK_LICENSE_GPL_3_0,
                               "comments", about_text,
                               "authors", authors,
                               NULL
                               );
}

// configure-Function
static void plane_regression_configure(void)
{
        if(plane_regression_pi.app)
                plane_regression_pi.app->message("plane_regression Plugin Configuration");
}

// cleanup-Function
static void plane_regression_cleanup(void)
{
        PI_DEBUG (DBG_L2, "Plugin Cleanup");
}


// run-Function
static gboolean plane_regression_run(Scan *Src, Scan *Dest)
{
	double xRight, xLeft, yTop, yBottom;
	double sumi, sumiy, sumyq, sumiq, sumy, val;
	double ysumi, ysumiy, ysumyq, ysumiq, ysumy;
	double a, b, n, nn, imean, ymean,ax,bx,ay,by;
	int line, i;
	double sum_ax, sum_ay, sum_by, num_m;

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
			main_get_gapp()->setup_multidimensional_data_copy ("Multidimensional Plane Regression", Src, ti, tf, vi, vf);
		} while (ti > tf || vi > vf);

		main_get_gapp()->progress_info_new ("Multidimenssional Plane Regression", 2);
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

			sumi = sumiq = sumy = sumyq = sumiy = ax = ay = bx = by = 0.;
			ysumi = ysumiq = ysumy = ysumyq = ysumiy = 0.;
			sum_ax = sum_by = 0.;

			int n_obj = 0;

			n_obj = Src->number_of_object ();
			n = 0.;
			while (n_obj--){
                                scan_object_data *obj_data = Src->get_object_data (n_obj);
                                if (strncmp (obj_data->get_name (), "Rectangle", 9) )
                                        continue; // no rectangle, skip
		
                                if (obj_data->get_num_points () != 2) 
                                        continue; // sth. is weired!
		
                                double x,y;
                                obj_data->get_xy_i_pixel (0, xRight, yBottom);
                                obj_data->get_xy_i_pixel (1, xLeft, yTop);
                                if (xRight < xLeft){
                                        x = xRight;
                                        xRight = xLeft;
                                        xLeft = x;
                                }
                                if (yTop > yBottom){
                                        y = yTop;
                                        yTop = yBottom;
                                        yBottom = y;
                                }

                                sumi = sumiq = 0.;
                                for (val=xLeft; val < xRight; val += 1.){
                                        sumiq += val*val;
                                        sumi  += val;
                                }

                                n = xRight-xLeft;
                                imean = sumi / n;

                                nn = 0.;
                                for (line = (int)yTop; line < (int)yBottom; ++line) {
                                        sumy = sumyq = sumiy = 0.;
                                        for (i=(int)xLeft; i < (int)xRight; ++i) {
                                                val = m->GetDataPkt(i, line);
                                                sumy   += val;
                                                sumyq  += val*val;
                                                sumiy  += val*i;
                                        }
                                        ymean = sumy / n;
                                        a = (sumiy- n * imean * ymean ) / (sumiq - n * imean * imean);
                                        b = ymean - a * imean;
                                        ax += a;
                                        bx += b;
                                        /* Werte fuer y-linregress */
                                        val = line;
                                        ysumy  += b;
                                        ysumyq += b*b;
                                        ysumiy += b*val;
                                        ysumiq += val*val;
                                        ysumi  += val;
                                }
                                nn += yTop-yBottom;
			}
			ax = ax/nn;
			bx = bx/nn;
			imean = ysumi / nn;
			ymean = ysumy / nn;
			ay = (ysumiy - nn * imean * ymean ) / (ysumiq - nn * imean * imean);
			by = ymean - ay * imean;
	
			std::cout << "PR: Ereg average is"
				  << " ax:" << sum_ax << " ay:" << sum_ay << " by:" << sum_by
				  << std::endl;

			sumy = sumi = 0.;
			for (line = 0; line<Dest->data.s.ny; ++line, sumy += ay) {
                                sumi = by + sumy + .5;
                                for (i=0; i < Dest->data.s.nx; ++i, sumi += ax)
                                        Dest->mem2d->PutDataPkt (m->GetDataPkt (i, line) - sumi,
                                                                 i, line);
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


