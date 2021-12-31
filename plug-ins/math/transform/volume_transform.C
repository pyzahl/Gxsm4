/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=volume_transform
pluginmenuentry 	=Volume Transform
menupath		=math-transformations-section
entryplace		=_Transformations
shortentryplace	=TR
 * 
 * Gxsm Plugin Name: volume_transform.C
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
% PlugInDocuCaption: Volume Transform
% PlugInName: volume_transform
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: math-transformations-sectionVolume Transform

% PlugInDescription
Transform/Rotate a XY-Layer set/volume along any given axis laying in XY plane by theta. A new data set with same layer number (or any given) is computed my linear (1st order) interpolation of data.

% PlugInUsage
Activate a channel and run it. Needs volumetric data, i.e. a set of images in layer dimension

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
static void volume_transform_init( void );
static void volume_transform_about( void );
static void volume_transform_configure( void );
static void volume_transform_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean volume_transform_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean volume_transform_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin volume_transform_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "volume_transform-"
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
  g_strdup("Volume Transform"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-transformations-section",
  // Menuentry
  N_("Volume Transform"),
  // help text shown on menu
  N_("Voluem Transform"),
  // more info...
  "Transform a XY-Layer Volume along any given axis laying in XY plane by theta",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  volume_transform_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  volume_transform_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  volume_transform_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  volume_transform_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin volume_transform_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin volume_transform_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin volume_transform_m1s_pi
#else
 GxsmMathTwoSrcPlugin volume_transform_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   volume_transform_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm volume_transform Plugin\n\n"
                                   "Volume Transform: Transform a XY-Layer Volume along any given axis laying in XY plane by theta.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  volume_transform_pi.description = g_strdup_printf(N_("Gxsm MathOneArg volume_transform plugin %s"), VERSION);
  return &volume_transform_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &volume_transform_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &volume_transform_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void volume_transform_init(void)
{
  PI_DEBUG (DBG_L2, "Plugin Init" );
}

// about-Function
static void volume_transform_about(void)
{
  const gchar *authors[] = { volume_transform_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  volume_transform_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void volume_transform_configure(void)
{
  if(volume_transform_pi.app)
    volume_transform_pi.app->message("volume_transform Plugin Configuration");
}

// cleanup-Function
static void volume_transform_cleanup(void)
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

int setup_volume_transform (Scan *src, double &rx0, double &ry0, double &rphi, double &rtilt){
	double r=1.;
	UnitObj *Pixel = new UnitObj("Pix","Pix");
	UnitObj *Unity = new UnitObj(" "," ");
	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("Volume Shear-Rotation"),
							 NULL, 
							 (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
							 _("_OK"), GTK_RESPONSE_ACCEPT,
							 NULL); 
        
	BuildParam bp;
        gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);

	bp.grid_add_label ("Volume Shear"); bp.new_line ();
	bp.grid_add_ec ("rx-ref:", Unity, &rx0, 0., 10000., ".0f"); bp.new_line ();
	bp.grid_add_ec ("ry-ref:", Unity, &ry0, 0., 10000., ".0f"); bp.new_line ();
	bp.grid_add_ec ("r-phi in plane:", Unity, &rphi, 0., 360., ".3f"); bp.new_line ();
	bp.grid_add_ec ("radius in plane:", Unity, &r, 0., 10000., ".3f"); bp.new_line ();
	bp.grid_add_ec ("r-tilt:", Unity, &rtilt, -1000., 1000., ".3f"); bp.new_line ();

        gtk_widget_show (dialog);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

        int response = GTK_RESPONSE_NONE;
        g_signal_connect (dialog, "response", G_CALLBACK (on_dialog_response), &response);

        // wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

	delete Pixel;
	delete Unity;	

	rtilt /= r;

	return response;
}

// m[6] := { x0, y0, crphi, srphi, rtilt };
double transform_kernel (double x, double y, double v, double m[5]){
	// r is "y" componet after rotaion into trivial coord system:
	double r = (y - m[1]) * m[2] - (x - m[0]) * m[3];    // Vr:=(xc + ys, yc - xs)
	// z (energy correction for ARPES) ajustment is:
	return v - r*m[4];
}

// run-Function
static gboolean volume_transform_run(Scan *Src, Scan *Dest)
{
	gapp->progress_info_new ("Rotating Volumetric Data", 2); // setup an progress indicator
	gapp->progress_info_set_bar_fraction (0., 1);
	gapp->progress_info_set_bar_fraction (0., 2);
	gapp->progress_info_set_bar_text ("Time", 1);
	gapp->progress_info_set_bar_text ("Value", 2);

	// number of time frames stored
	int n_times  = Src->number_of_time_elements ();
	int n_layers = Src->mem2d->GetNv ();
	
	double x0, y0;
	double rphi, rtilt;
	setup_volume_transform (Src, x0, y0, rphi, rtilt);

	rphi *= M_PI/180.;
	double m[5] = { x0, y0, cos (-rphi), sin (-rphi), rtilt };

	// resize Dest to match transposed Src
	Dest->mem2d->Resize (Src->mem2d->GetNx (), Src->mem2d->GetNy (), n_layers, ZD_IDENT);

	for (int time_index=0; time_index < n_times; ++time_index){
		gapp->progress_info_set_bar_fraction ((gdouble)(time_index+1)/(gdouble)n_times, 1);
		Mem2d *ms = Src->mem2d_time_element (time_index);

		for (int layer_index=0; layer_index < n_layers; ++layer_index){
			gapp->progress_info_set_bar_fraction ((gdouble)(layer_index+1)/(gdouble)n_layers, 2);
			std::cout << "T:" << time_index << " V:" << layer_index << std::endl;

			ms->data->SetLayer (layer_index);

			for (int row=0; row < Src->mem2d->GetNy (); ++row)
				for (int col=0; col < Src->mem2d->GetNx (); ++col)
					Dest->mem2d->PutDataPkt (ms->GetDataPktInterpol (
									 (double)col, (double)row, 
									 transform_kernel ((double)col, (double)row, (double)layer_index, m)),
						   col, row, layer_index);

			std::cout << "SetVLookup..." << std::endl;
			Dest->mem2d->data->SetVLookup (layer_index, ms->data->GetVLookup (layer_index));
			std::cout << "OK..." << std::endl;

			// copy LI
			if (ms->layer_information_list[layer_index]){
				int nj = g_list_length (ms->layer_information_list[layer_index]);
				for (int j = 0; j<nj; ++j){
					LayerInformation *li = (LayerInformation*) g_list_nth_data (ms->layer_information_list[layer_index], j);
					if (li)
						Dest->mem2d->add_layer_information (time_index, new LayerInformation (li));
				}
			}
		}

		std::cout << "Append in time..." << std::endl;
		Dest->append_current_to_time_elements (time_index, ms->get_frame_time (time_index));
		std::cout << "OK." << std::endl;
	}
//	Dest->data.s.ntimes  = n_times;
	Dest->data.s.nvalues = Dest->mem2d->GetNv ();
	
	gapp->progress_info_close ();
	Dest->retrieve_time_element (0);
	Dest->mem2d->SetLayer(0);

	gapp->progress_info_close (); // close progress indicator
	return MATH_OK;
}


