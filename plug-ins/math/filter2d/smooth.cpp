/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=smooth
pluginmenuentry 	=Smooth
menupath		=Math/Filter 2D/
entryplace		=Filter 2D
shortentryplace	=F2D
abouttext		=Gaussian smooth using 2D convolution.
smallhelp		=gaus smooth
longhelp		=Gausian smooth
 * 
 * Gxsm Plugin Name: smooth.C
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
% PlugInDocuCaption: Smooth
% PlugInName: smooth
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Filter 2D/Smooth

% PlugInDescription
A 2D Gausian smooth is calculated via convolution with a Gaus-Kernel:

\[ K_{ij} = 4*e^{-\frac{i^2+j^2}{r^2}} \]

% PlugInUsage
Call \GxsmMenu{Math/Filter 2D/Smooth}.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

% OptPlugInHints
Alternative: Use the Fourier-Filter methods Gaus-Stop/Pass for huge
convolutions, it's faster!

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "../../common/pyremote.h"
#include "glbvars.h"
#include "surface.h"

// Plugin Prototypes
static void smooth_init( void );
static void smooth_about( void );
static void smooth_configure( void );
static void smooth_cleanup( void );

static gboolean smooth_run_radius(Scan *Src, Scan *Dest);
static void smooth_non_interactive( GtkWidget *widget , gpointer pc );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
 static gboolean smooth_run( Scan *Src, Scan *Dest );

// Fill in the GxsmPlugin Description here
GxsmPlugin smooth_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "smooth-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-F2D",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup("Gaus smooth"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-filter2d-section",
  // Menuentry
  N_("Smooth"),
  // help text shown on menu
  N_("Gaus smooth"),
  // more info...
  "gaus smooth",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  smooth_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  smooth_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  smooth_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  smooth_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin smooth_m1s_pi -> "OneSrcMath"

 GxsmMathOneSrcPlugin smooth_m1s_pi
 = {
   // math-function to run, see prototype(s) above!!
   smooth_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm smooth Plugin\n\n"
                                   "Gaus smooth using a 2D convolution.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  smooth_pi.description = g_strdup_printf(N_("Gxsm MathOneArg smooth plugin %s"), VERSION);
  return &smooth_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// if "get_gxsm_math_one_for_all_vt_plugin_info" is resolved by dlsym from Gxsm for operations on all layers (values) and times, run may get called multiple times!
// used to find out which Math Type the Plugin is!! 

// Essential Math Plugin Function!!

GxsmMathOneSrcPlugin *get_gxsm_math_one_src_for_all_vt_plugin_info( void ) {
  return &smooth_m1s_pi; 
}


/* Here we go... */

double       smooth_radius = 5.;
class MemSmoothKrn *smooth_kernel=NULL;


// init-Function
static void smooth_init(void)
{
  PI_DEBUG (DBG_L2, "smooth Plugin Init");

// This is action remote stuff, stolen from the peak finder PI.
  remote_action_cb *ra;
  GtkWidget *dummywidget = NULL; // gtk_menu_item_new();

  ra = g_new( remote_action_cb, 1);
  ra -> cmd = g_strdup_printf("MATH_FILTER2D_Smooth");
  ra -> RemoteCb = &smooth_non_interactive;
  ra -> widget = dummywidget;
  ra -> data = NULL;
  main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
  PI_DEBUG (DBG_L2, "smooth-plugin: Adding new Remote Cmd: MATH_FILTER2D_Smooth");
// remote action stuff
}

static void smooth_non_interactive( GtkWidget *widget , gpointer pc )
{
  PI_DEBUG (DBG_L2, "smooth-plugin: smooth is called from script.");

//  cout << "pc: " << ((gchar**)pc)[1] << endl;
//  cout << "pc: " << ((gchar**)pc)[2] << endl;
//  cout << "pc: " << atof(((gchar**)pc)[2]) << endl;

  main_get_gapp()->xsm->MathOperation(&smooth_run_radius);
  return;

}

// about-Function
static void smooth_about(void)
{
  const gchar *authors[] = { smooth_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  smooth_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void smooth_configure(void)
{
  if(smooth_pi.app)
    smooth_pi.app->message("smooth Plugin Configuration");
}

// cleanup-Function
static void smooth_cleanup(void)
{
	PI_DEBUG (DBG_L2, "smooth Plugin Cleanup");
}

/*
 * special kernels for MemDigiFilter
 * Gausian Smooth Kernel
 */

class MemSmoothKrn : public MemDigiFilter{
public:
	MemSmoothKrn (double xms, double xns, int m, int n):MemDigiFilter (xms, xns, m, n){};
	virtual gboolean CalcKernel (){
		int i,j;
		g_message ("Calculating Gauss Smooth Kernel");
		for (i= -m; i<=m; i++)
			for (j = -n; j<=n; j++)
				data->Z (4*exp (-(i*i)/(xms*xms) -(j*j)/(xns*xns)), j+n, i+m); 
		return 0;
	};
};

static void
on_dialog_response (GtkDialog *dialog,
		    int        response,
		    gpointer   user_data)
{
        *((int *) user_data) = response;
        gtk_window_destroy (GTK_WINDOW (dialog));
}

void setup_multidimensional_data_copy (const gchar *title, Scan *src, int &ti, int &tf, int &vi, int &vf, int *crop_window_xy=NULL, gboolean crop=FALSE){
	UnitObj *Pixel = new UnitObj("Pix","Pix");
	UnitObj *Unity = new UnitObj(" "," ");
	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_(title),
							 NULL, 
							 (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
							 _("_OK"), GTK_RESPONSE_ACCEPT,
							 NULL); 
	BuildParam bp;
        gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);

        bp.grid_add_label ("New Time and Layer bounds:"); bp.new_line ();

	gint t_max = src->number_of_time_elements ()-1;
	if (t_max < 0) t_max=0;

        bp.grid_add_ec ("t-inintial",  Unity, &ti, 0, t_max, ".0f"); bp.new_line ();
        bp.grid_add_ec ("t-final",     Unity, &tf, 0, t_max, ".0f"); bp.new_line ();
#if 0
	if (tnadd){
		bp.grid_add_ec ("t-#add",  Unity, tnadd, 1, t_max, ".0f"); bp.new_line ();
	}
#endif
        bp.grid_add_ec ("lv-inintial", Unity, &vi, 0, src->mem2d->GetNv (), ".0f"); bp.new_line ();
        bp.grid_add_ec ("lv-final",    Unity, &vf, 0, src->mem2d->GetNv (), ".0f"); bp.new_line ();

#if 0
	if (vnadd){
		bp.grid_add_ec ("lv-#add",  Unity, vnadd, 1, src->mem2d->GetNv (), ".0f"); bp.new_line ();
	}
#endif

	if (crop && crop_window_xy){
                bp.grid_add_label ("Crop window bounds:"); bp.new_line ();
                bp.grid_add_ec ("X-left",  Pixel, &crop_window_xy[0], 0, src->mem2d->GetNx ()-1, ".0f"); bp.new_line ();
                bp.grid_add_ec ("Y-top",   Pixel, &crop_window_xy[1], 0, src->mem2d->GetNx ()-1, ".0f"); bp.new_line ();
                bp.grid_add_ec ("X-right", Pixel, &crop_window_xy[2], 0, src->mem2d->GetNx ()-1, ".0f"); bp.new_line ();
                bp.grid_add_ec ("Y-bottom",Pixel, &crop_window_xy[3], 0, src->mem2d->GetNx ()-1, ".0f"); bp.new_line ();
	}

        gtk_widget_show (dialog);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

        int response = GTK_RESPONSE_NONE;
        g_signal_connect (dialog, "response", G_CALLBACK (on_dialog_response), &response);

        // FIX-ME GTK4 ??
        // wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

	delete Pixel;
	delete Unity;	
}

#if 0
// run-Function
static gboolean smooth_run(Scan *Src, Scan *Dest) // for all vt
{
	double r = 5.;    // Get Radius
	main_get_gapp()->ValueRequest("2D Convol. Filter Size", "Radius", "Smooth kernel size: s = 1+radius",
			   main_get_gapp()->xsm->Unity, 0., Src->mem2d->GetNx()/10., ".0f", &r);

	int    s = 1+(int)(r + .9); // calc. approx Matrix Radius
	MemSmoothKrn krn(r,r, s,s); // Setup Kernelobject



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
			setup_multidimensional_data_copy ("Multidimensional Smooth", Src, ti, tf, vi, vf);
		} while (ti > tf || vi > vf);

		main_get_gapp()->progress_info_new ("Multidimenssional Smooth", 2);
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

			krn.Convolve(m, Dest->mem2d); // Do calculation of Kernel and then do convolution

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

#else


// run-Function -- may gets called for all layers and time if multi dim scan data is present!
static gboolean smooth_run(Scan *Src, Scan *Dest)
{

// check for multi dim calls, make sure not to ask user for paramters for every layer or time step!
	if (((Src ? Src->mem2d->get_t_index ():0) == 0 && (Src ? Src->mem2d->GetLayer ():0) == 0) || !smooth_kernel) {
		double r = smooth_radius;    // Get Radius
		main_get_gapp()->ValueRequest("2D Convol. Filter Size", "Radius", "Smooth kernel size: s = 1+radius",
				   main_get_gapp()->xsm->Unity, 0., Src->mem2d->GetNx()/10., ".0f", &r);
		smooth_radius = r;
		if (smooth_kernel)
			free (smooth_kernel);

		int    s = 1+(int)(r + .9); // calc. approx Matrix Radius
		smooth_kernel = new MemSmoothKrn (r,r, s,s); // calc. convol kernel

		if (!Src || !Dest)
			return 0;
	}	

	smooth_kernel->Convolve (Src->mem2d, Dest->mem2d); // do convolution

	return MATH_OK;
}

#endif

static gboolean smooth_run_radius(Scan *Src, Scan *Dest)
{
	// equals smooth-run except ValueRequest
	double r = 5.0;    // Get Radius

	int    s = 1+(int)(r + .9); // calc. approx Matrix Radius
	MemSmoothKrn krn(r,r, s,s); // Setup Kernelobject

	krn.Convolve(Src->mem2d, Dest->mem2d); // Do calculation of Kernel and then do convolution

	return MATH_OK;
}

