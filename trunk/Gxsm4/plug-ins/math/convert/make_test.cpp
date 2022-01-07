/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=make_test
pluginmenuentry 	=make test
menupath		=Math/Convert/
entryplace		=Convert
shortentryplace	=CV
abouttext		=Generate text data
smallhelp		=Generate text data
longhelp		=Generate text data
 * 
 * Gxsm Plugin Name: make_test.C
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
% PlugInDocuCaption: Generate test data
% PlugInName: make_test
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Convert/make test

% PlugInDescription
Makes a test data set -- for test and demostration purpose only.

% PlugInUsage
Call \GxsmMenu{Math/Convert/make test}.

% OptPlugInSources

% OptPlugInDest

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

// Plugin Prototypes
static void make_test_init( void );
static void make_test_about( void );
static void make_test_configure( void );
static void make_test_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean make_test_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean make_test_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin make_test_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "make_test-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-CV",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("Make test scan data, type layered float"),
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-convert-section",
  // Menuentry
  N_("make test"),
  // help text shown on menu
  N_("Make test data"),
  // more info...
  "Make test scan data, type is layered float",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  make_test_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  make_test_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  make_test_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  make_test_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin make_test_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin make_test_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin make_test_m1s_pi
#else
 GxsmMathTwoSrcPlugin make_test_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   make_test_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm make_test Plugin\n\n"
                                   "Makes a test scan data set. Type is layered float.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  make_test_pi.description = g_strdup_printf(N_("Gxsm MathOneArg make_test plugin %s"), VERSION);
  return &make_test_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &make_test_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &make_test_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void make_test_init(void)
{
  PI_DEBUG (DBG_L2, "make_test Plugin Init");
}

// about-Function
static void make_test_about(void)
{
  const gchar *authors[] = { make_test_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  make_test_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void make_test_configure(void)
{
	if(make_test_pi.app)
		make_test_pi.app->message("make_test Plugin Configuration");
}

// cleanup-Function
static void make_test_cleanup(void)
{
	PI_DEBUG (DBG_L2, "make_test Plugin Cleanup");
}

// ODBA: a1[0] != 0
void sq2lat (double r[2], double a1[2], double a2[2], double &i, double &j){
//	static int k=100;
// -- not a good solution if a1[0] == 0 -- ODBA a1[0] must be != 0.
	double L =  r[0]/a1[0] -  r[1]/a1[1];
	double R = a2[0]/a1[0] - a2[1]/a1[1];
	j = L/R;
//      i = r[0]/a1[0] - j*a2[0]/a1[0]; -- not a good solution if a1[0] == 0 -- ODBA a1[0] must be != 0.
	i = r[1]/a1[1] - j*a2[1]/a1[1];
//	if (k)
//		std::cout << (k--) << ": " << r[0] << ", " << r[1] << " => " << i << ", " << j << std ::endl;
}

inline double atom_function (double r, double delta_a) { double a=delta_a; return 1./(a * M_PI) * exp (-r*r/(a*a)); }

// run-Function
static gboolean make_test_run(Scan *Src, Scan *Dest)
{
#define SNX 3
#define SNY 3 
	double a = 5.06; // Ang
	make_test_pi.app->ValueRequest("Enter Value", "Lattice Param", 
				       "a [A]",
				       make_test_pi.app->xsm->LenUnit, 
				       1e-3, 1e3, ".5f", &a);
	double phi=30./180*M_PI;
	double s = sin (phi); // 0.866..
	double c = cos (phi); // 0.5
	double a1[2] = { 1.0, 0. };
	double a2[2] = { s, c };
	double F_lattice_hex[SNX][SNY] = {
		{ 0., 1., 1. },
		{ 1., 1., 1. },
		{ 1., 1., 1. }
	};

	if (Src->data.s.nx < 2 || Src->data.s.ny < 2)
		return MATH_SIZEERR;

	double delta_a = 0.3;
	make_test_pi.app->ValueRequest("Enter Value", "delta width", 
				       "atom shape",
				       make_test_pi.app->xsm->Unity, 
				       1e-3, 1e3, ".3f", &delta_a);
	double lat_tr = 1.;
	make_test_pi.app->ValueRequest("Enter Value", "lattice transformation", 
				       "do lattice trafo? (0,1)",
				       make_test_pi.app->xsm->Unity, 
				       0., 10., ".0f", &lat_tr);

	main_get_gapp()->progress_info_new ("MkLatticePot", 1);
	main_get_gapp()->progress_info_set_bar_fraction (0., 1);
	main_get_gapp()->progress_info_set_bar_text ("X", 1);


 	Dest->mem2d->Resize (Src->data.s.nx, Src->data.s.ny, 20, ZD_FLOAT);
	Dest->data.s.ntimes=1;
	Dest->data.s.nvalues=Dest->mem2d->GetNv ();
//	Dest->mem2d->data->MkXLookup(0., 1.);
//	Dest->mem2d->data->MkYLookup(0., 1.);
	Dest->mem2d->data->MkVLookup(0., 1.);
	Dest->data.s.dz=1.;
//	UnitObj *VoltUnit = new UnitObj("V","V",".3f","Volt");
//	Dest->data.SetZUnit (VoltUnit);
//	delete VoltUnit;
//	Dest->mem2d->data->SetVLookup(n, v);

	for (int i=0; i<Dest->mem2d->GetNx (); ++i)
		for (int j=0; j<Dest->mem2d->GetNy (); ++j)
			Dest->mem2d->PutDataPkt (0., i,j);

	double dri = Dest->mem2d->data->GetXLookup (1)-Dest->mem2d->data->GetXLookup (0);
	for (int i=0; i<Dest->mem2d->GetNx (); ++i){
		main_get_gapp()->progress_info_set_bar_fraction ((double)i/(double)Dest->mem2d->GetNx (), 1);
		for (int j=0; j<Dest->mem2d->GetNy (); ++j){
			double F=0.;
			double u,v;
			double r[2];
			r[0] = Dest->mem2d->data->GetXLookup (i)/a*SNX;
			r[1] = Dest->mem2d->data->GetYLookup (j)/a*SNY;
			
//#if 0 // lattice transformation ?
			if (lat_tr > 0.)
				sq2lat (r, a2, a1, u, v);
//#else // direct square lattice
			else{
				u = r[0];
				v = r[1];
			}
//#endif
			gint m,n;
			m = ((gint)round (u)+10000*SNX) % SNX;
			n = ((gint)round (v)+10000*SNX) % SNY;

			double xx = (round(u) - u);
			double yy = (round(v) - v);
			if ((xx*xx + yy*yy) > (0.2*0.2))
				F=-1.;

			if (m < 0 || m >= SNX) { m = 0; F=-1.; }
			if (n < 0 || n >= SNY) { n = 0; F=-1.; }
			if (F >= 0.){
				F = F_lattice_hex[m][n];

				const gint r_cut = a/dri * 2. * delta_a; // atom function cut off radius -- assume zero outside
				for (int di = -r_cut; di <= r_cut; ++di)
					for (int dj = -r_cut; dj <= r_cut; ++dj){
						double pot = F * atom_function (dri*sqrt ((double)di*(double)di + (double)dj*(double)dj)/a, delta_a);
						if (di+i >= 0 && di+i < Dest->mem2d->GetNx () &&
						    dj+j >= 0 && dj+j < Dest->mem2d->GetNy ())
							Dest->mem2d->data->Zadd (pot, di+i,dj+j);
					}
			}
		}
	}




#if 0   // ------------ old test stuff
	for (int i=0; i<Dest->mem2d->GetNx (); ++i)
		for (int j=0; j<Dest->mem2d->GetNy (); ++j)
			for (int k=0; k<Dest->mem2d->GetNv (); ++k){
				double v = (double)k;
				v *= v*10.;
				Dest->mem2d->PutDataPkt (Src->mem2d->GetDataPkt (i,j) + v, (i+k)%Dest->mem2d->GetNx (),j,k);
			}
#endif

	main_get_gapp()->progress_info_close ();

	return MATH_OK;
}


