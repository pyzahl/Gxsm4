/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=diff
pluginmenuentry 	=Diff
menupath		=Math/Filter 1D/
entryplace		=Filter 1D
shortentryplace	=F1D
abouttext		=1D Differentation
smallhelp		=1D Differentation
longhelp		=1D Differentation
 * 
 * Gxsm Plugin Name: diff.C
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
% PlugInDocuCaption: 1dim differentation
% PlugInName: diff
% PlugInAuthor: Percy Zahl, Stefan Schroeder
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Filter 1D/Diff

% PlugInDescription
One dimensional differentation\dots

% PlugInUsage
Call \GxsmMenu{Math/Filter 1D/Diff}.
This plugin can be called from a
remote-control script with the command \GxsmTT{action('diff\_PI')}.
The kernel-size is then set to '5+1'.

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
#include "../../common/pyremote.h"

// Plugin Prototypes
static void diff_init( void );
static void diff_about( void );
static void diff_configure( void );
static void diff_cleanup( void );

static void diff_non_interactive( GtkWidget *widget , gpointer pc);

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype

 static gboolean diff_run( Scan *Src, Scan *Dest );

 static gboolean diff_run_radius( Scan *Src, Scan *Dest);

#else
// "TwoSrc" Prototype
 static gboolean diff_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin diff_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "diff-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-F1D",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup("1D Differentation"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-filter1d-section",
  // Menuentry
  N_("Diff"),
  // help text shown on menu
  N_("1D Differentation"),
  // more info...
  "1D Differentation",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  diff_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  diff_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  diff_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  diff_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin diff_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin diff_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin diff_m1s_pi
#else
 GxsmMathTwoSrcPlugin diff_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   diff_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm diff Plugin\n\n"
                                   "1D Differentation");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  diff_pi.description = g_strdup_printf(N_("Gxsm MathOneArg diff plugin %s"), VERSION);
  return &diff_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_for_all_vt_plugin_info( void ) {
  return &diff_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &diff_m2s_pi; 
}
#endif

/* Here we go... */
double       d_radius = 5.;
class Mem1DDiffKrn *d_kernel=NULL;

// init-Function
static void diff_init(void)
{
  PI_DEBUG (DBG_L2, "diff Plugin Init");
 // This is action remote stuff, stolen from the peak finder PI.
   remote_action_cb *ra;
   //GtkWidget *dummywidget = gtk_menu_item_new();
 
   ra = g_new( remote_action_cb, 1);
   ra -> cmd = g_strdup_printf("diff_PI");
   ra -> RemoteCb = &diff_non_interactive;
   ra -> widget = NULL; // dummywidget;
   ra -> data = NULL;
   main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
   PI_DEBUG (DBG_L2, "diff-plugin: Adding new Remote Cmd: diff_PI");
 // remote action stuff
}
 
static void diff_non_interactive( GtkWidget *widget , gpointer pc )
{
   PI_DEBUG (DBG_L2, "diff-plugin: diff is called from script.");
   main_get_gapp()->xsm->MathOperation(&diff_run_radius);
   return;
}

// about-Function
static void diff_about(void)
{
  const gchar *authors[] = { diff_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  diff_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void diff_configure(void)
{
	if(diff_pi.app)
		diff_pi.app->message("diff Plugin Configuration");
}

// cleanup-Function
static void diff_cleanup(void)
{
	PI_DEBUG (DBG_L2, "diff Plugin Cleanup");
}

/*
 * 1D differentiation kernel
 */

class Mem1DDiffKrn : public MemDigiFilter{
public:
	Mem1DDiffKrn(double xms, double xns, int m, int n) : MemDigiFilter (xms, xns, m, n){};
	virtual gboolean CalcKernel (){
		int i,j;
		for (i= -m; i<=m; i++)
			for (j= -n; j<=n; j++)
				data->Z(rint(0), j+n, i+m); 

		for (i = 0, j= -n; j<=n;j++)
			data->Z (rint (sin (j)*exp (-(j*j)/(xns*xns))), j+n, i+m); 

		return 0;
	};
};

// run-Function
static gboolean diff_run(Scan *Src, Scan *Dest)
{
	if (((Src ? Src->mem2d->get_t_index ():0) == 0 && (Src ? Src->mem2d->GetLayer ():0) == 0) || !d_kernel) {
		double r = d_radius;
		main_get_gapp()->ValueRequest("1D Convol. Filter Size", "Length", "Diff kernel size: s = 1+length",
				   main_get_gapp()->xsm->Unity, 0., Src->mem2d->GetNx()/10., ".0f", &r);
		d_radius = r;
		if (d_kernel)
			free (d_kernel);

		int    s = 1+(int)(r + .9); // calc. approx Matrix Radius
		d_kernel = new Mem1DDiffKrn (r,r, s,s); // calc. convol kernel

		if (!Src || !Dest)
			return 0;
	}	

	d_kernel->Convolve (Src->mem2d, Dest->mem2d); // do convolution

	return MATH_OK;
}



static gboolean diff_run_radius(Scan *Src, Scan *Dest)
{ // equals diff_run except dialog
 	double r = 5.;    // Get Radius
 	int    s = 1+(int)(r + .9); // calc. approx Matrix Radius
 	Mem1DDiffKrn krn (r,r, s,s); // Setup Kernelobject
 	krn.Convolve (Src->mem2d, Dest->mem2d); // Do calculation of Kernel and then do convolution
 	return MATH_OK;
}
