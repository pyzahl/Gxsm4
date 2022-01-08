/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@gxsm.sf.net
pluginname		=mandriftfix
pluginmenuentry 	=Auto Align
menupath		=math-transformations-section
entryplace		=_Transformations
shortentryplace	=TR
abouttext		=Automatic align an image series in time domain (drift correction)
smallhelp		=Automatic align an image series in time domain
longhelp		=Automatic align an image series in time domain (drift correction)
 * 
 * Gxsm Plugin Name: mandriftfix.C
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
% PlugInDocuCaption: Auto Align
% PlugInName: mandriftfix
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@gxsm.sf.net
% PlugInMenuPath: Math/Transformations/Auto Align

% PlugInDescription
Apply a simple drift in pixels per frame.

% PlugInUsage
Figure out the drift inbetween frames in pixels in X and Y dimension to compensate for.

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
static void mandriftfix_init( void );
static void mandriftfix_about( void );
static void mandriftfix_configure( void );
static void mandriftfix_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
static gboolean mandriftfix_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
static gboolean mandriftfix_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin mandriftfix_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	"mandriftfix-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
	"M1S"
#else
	"M2S"
#endif
	"-TR",
	// Plugin's Category - used to autodecide on Pluginloading or ignoring
	// NULL: load, else
	// example: "+noHARD +STM +AFM"
	// load only, if "+noHARD: no hardware" and Instrument is STM or AFM
	// +/-xxxHARD und (+/-INST or ...)
	NULL,
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	g_strdup("Automatic align an image series in time domain (drift correction)"),                   
	// Author(s)
	"Percy Zahl",
	// Menupath to position where it is appendet to
	"math-transformations-section",
	// Menuentry
	N_("Manual Drift Fix/Align"),
	// help text shown on menu
	N_("manual fix drift/align an image series in time domain (drift correction)"),
	// more info...
	"manual drift fix/ align an image series in time domain",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	mandriftfix_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	mandriftfix_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	mandriftfix_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

	mandriftfix_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin mandriftfix_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin mandriftfix_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin mandriftfix_m1s_pi
#else
GxsmMathTwoSrcPlugin mandriftfix_m2s_pi
#endif
= {
	// math-function to run, see prototype(s) above!!
	mandriftfix_run
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm mandriftfix Plugin\n\n"
                                   "Automatic align an image series in time domain (drift correction)");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	mandriftfix_pi.description = g_strdup_printf(N_("Gxsm MathOneArg mandriftfix plugin %s"), VERSION);
	return &mandriftfix_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
	return &mandriftfix_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
	return &mandriftfix_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void mandriftfix_init(void)
{
	PI_DEBUG (DBG_L2, "Plugin Init" );
}

// about-Function
static void mandriftfix_about(void)
{
	const gchar *authors[] = { mandriftfix_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  mandriftfix_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void mandriftfix_configure(void)
{
	if(mandriftfix_pi.app)
		mandriftfix_pi.app->message("mandriftfix Plugin Configuration");
}

// cleanup-Function
static void mandriftfix_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Plugin Cleanup");
}

static void
on_dialog_response (GtkDialog *dialog,
		    int        response,
		    gpointer   user_data)
{
        *((int *) user_data) = response;
        gtk_window_destroy (GTK_WINDOW (dialog));
}

int setup_manual_align (Scan *src, int &ti, int &tf, int &vi, int &vf, double &dx, double &dy){
  UnitObj *PixelF = new UnitObj("Pix/Frame","Pix/Frame");
	UnitObj *Unity = new UnitObj(" "," ");
	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("Multidimensional Driftcorrection/Image Align"),
							 NULL, 
							 (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
							 _("_OK"), GTK_RESPONSE_ACCEPT,
							 _("_CANCEL"), GTK_RESPONSE_CANCEL,
							 NULL); 
        
	BuildParam bp;
        gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);

        bp.grid_add_label ("Time and Layer bounds:"); bp.new_line ();

	int note = src->number_of_time_elements ();
	if (note < 1) note = 1;

        bp.grid_add_ec ("t-inintial",  Unity, &ti, 0, note, ".0f"); bp.new_line ();
        bp.grid_add_ec ("t-final",     Unity, &tf, 0, note, ".0f"); bp.new_line ();

        bp.grid_add_ec ("lv-inintial", Unity, &vi, 0, src->mem2d->GetNv (), ".0f"); bp.new_line ();
        bp.grid_add_ec ("lv-final",    Unity, &vf, 0, src->mem2d->GetNv (), ".0f"); bp.new_line ();

        bp.grid_add_label ("Image shift to apply:"); bp.new_line ();
        bp.grid_add_ec ("dx", PixelF, &dx, -100., 100., ".2f"); bp.new_line ();
        bp.grid_add_ec ("dy", PixelF, &dy, -100., 100., ".2f"); bp.new_line ();
     
        gtk_widget_show (dialog);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

        int response = GTK_RESPONSE_NONE;
        g_signal_connect (dialog, "response", G_CALLBACK (on_dialog_response), &response);

        // FIX-ME GTK4 ??
        // wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

	delete PixelF;
	delete Unity;	

	return response;
}

// run-Function
static gboolean mandriftfix_run(Scan *Src, Scan *Dest)
{
	// put plugins math code here...
	// For more math-access methods have a look at xsmmath.C
	// simple example for 1sourced Mathoperation: Source is taken and 100 added.

	main_get_gapp()->progress_info_new ("Manual Drift Fix", 2);
	main_get_gapp()->progress_info_set_bar_fraction (0., 1);
	main_get_gapp()->progress_info_set_bar_fraction (0., 2);
	main_get_gapp()->progress_info_set_bar_text ("Time", 1);
	main_get_gapp()->progress_info_set_bar_text ("Value", 2);

	// number of time frames stored
	int n_times = Src->number_of_time_elements ();

	// resize Dest to match Src
	Dest->mem2d->Resize (Src->mem2d->GetNx (), Src->mem2d->GetNy (), Src->mem2d->GetNv (), ZD_IDENT);

	// artifical drift parameters

	double tcx=1.0;
	double tcy=0.0;
	double drift_x, drift_y;

	int ti,tf, vi,vf;

	ti=0; tf=n_times-1;
	vi=0; vf=Dest->mem2d->GetNv ();
	setup_manual_align (Src, ti, tf, vi, vf, tcx, tcy);

	// start with zero offset
	drift_x = drift_y = 0.;
	
	// go for it....
	for(int time=0; time < n_times; ++time){ // time loop
		main_get_gapp()->progress_info_set_bar_fraction ((gdouble)time/(gdouble)n_times, 1);

		// get time frame to work on
		double real_time = Src->retrieve_time_element (time);

		int n_values = Dest->mem2d->GetNv ();
		for(int value=0; value < n_values; ++value){ // value loop
			main_get_gapp()->progress_info_set_bar_fraction ((gdouble)value/(gdouble)n_values, 2);

			Src->mem2d->SetLayer(value);
			Dest->mem2d->SetLayer(value);

			for(int line=0; line < Dest->mem2d->GetNy (); ++line) // lines (y)
				for(int col=0; col < Dest->mem2d->GetNx (); ++col){ // columns (x)
					double x = col - drift_x;
					double y = line - drift_y;

					// Lookup pixel interpolated at x,y and put to col,line
					Dest->mem2d->PutDataPkt (Src->mem2d->GetDataPktInterpol (x, y), col, line);
				}
		}

		// now append new time frame to Destination Scan
		Dest->append_current_to_time_elements (time, real_time);

		// driftcorrection
		if (time >= ti && time <= tf){
			drift_x += tcx;
			drift_y += tcy;
		}
	}

	main_get_gapp()->progress_info_close ();

	return MATH_OK; // done OK.
}


