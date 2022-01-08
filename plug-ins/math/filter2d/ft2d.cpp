/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sourceforge.net
pluginname		=ft2d
pluginmenuentry 	=FT 2D
menupath		=math-filter2d-section
entryplace		=Filter _2D
shortentryplace	=F2D
abouttext		=2D Fourier Transform
smallhelp		=2D Fourier Transform
longhelp		=2D Fourier Transform
 * 
 * Gxsm Plugin Name: ft2d.C
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
% PlugInDocuCaption: FT 2D
% PlugInName: ft2d
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sourceforge.net
% PlugInMenuPath: math-filter2d-sectionFT 2D

% PlugInDescription
Two dimensional forward Fourier Transformation. Results in three layers (PSD, Re, Im):

Layer 0 is the Power Spectral Density, layer 1 (Re) and 2 (Im) are the
Complex numbers of the FT data. This data can be used for reverse
transformation after (background) stopp and/or pass operations.

% PlugInUsage

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
static void ft2d_init( void );
static void ft2d_about( void );
static void ft2d_configure( void );
static void ft2d_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean ft2d_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean ft2d_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin ft2d_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "ft2d-"
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
  g_strdup("2D Fourier Transform"),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-filter2d-section",
  // Menuentry
  N_("FT 2D"),
  // help text shown on menu
  N_("2D Fourier Transform"),
  // more info...
  "2D Fourier Transform",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  ft2d_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  ft2d_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  ft2d_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  ft2d_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin ft2d_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin ft2d_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin ft2d_m1s_pi
#else
 GxsmMathTwoSrcPlugin ft2d_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   ft2d_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm ft2d Plugin\n\n"
                                   "2D Fourier Transform");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  ft2d_pi.description = g_strdup_printf(N_("Gxsm MathOneArg ft2d plugin %s"), VERSION);
  return &ft2d_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &ft2d_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &ft2d_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void ft2d_init(void)
{
  PI_DEBUG (DBG_L2, "Plugin Init" );
}

// about-Function
static void ft2d_about(void)
{
  const gchar *authors[] = { ft2d_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  ft2d_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void ft2d_configure(void)
{
  if(ft2d_pi.app)
    ft2d_pi.app->message("ft2d Plugin Configuration");
}

// cleanup-Function
static void ft2d_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Plugin Cleanup");
}

// run-Function
static gboolean ft2d_run(Scan *Src, Scan *Dest)
{
	XSM_DEBUG (DBG_L3, "F2D FFT");
	ZData  *SrcZ;
        gboolean flg_cpx=true;
        
	SrcZ  =  Src->mem2d->data;
	Dest->data.s.nvalues = Src->mem2d->GetNv ();
        if (Src->mem2d->GetNv () > 1){
                Dest->mem2d->Resize (Src->mem2d->GetNx (), Src->mem2d->GetNy (), Src->mem2d->GetNv (), ZD_DOUBLE);
                flg_cpx=false;
        } else {
                Dest->mem2d->Resize (Src->mem2d->GetNx (), Src->mem2d->GetNy (), ZD_COMPLEX);
                Dest->data.s.nvalues=3;
                Dest->mem2d->data->SetVLookup(0, 0);
                Dest->mem2d->data->SetVLookup(1, 1);
                Dest->mem2d->data->SetVLookup(2, 2);
        }
        
	// set "Complex" layer param defaults
	Dest->data.s.ntimes=1;

	Dest->data.s.x0 = 0.;
	Dest->data.s.y0 = 0.;
	Dest->data.s.dx = 200./Dest->mem2d->GetNx ();
	Dest->data.s.dy = 200./Dest->mem2d->GetNy ();
  	Dest->data.s.rx = 200.;
	Dest->data.s.ry = 200.; // Dest->data.s.ny*Dest->data.s.dy;

	Dest->mem2d->data->MkXLookup(-100, 100);
	Dest->mem2d->data->MkYLookup(-100, 100);


	XSM_DEBUG (DBG_L3, "F2D FFT: using libfftw");

	// allocate memory for real and inplace complex data, add padding if nx is even
	int xlen  = Src->mem2d->GetNx ()/2+1;
	int xlen2 = 2*xlen;
	fftw_complex *dat =  (fftw_complex*) fftw_malloc (sizeof(fftw_complex) * xlen * Src->mem2d->GetNy ());
	double *in = (double *) dat;

	if (dat == NULL) {
                XSM_DEBUG (DBG_L3, "F2D FFT: libfftw: Error allocating memory for complex data");
                return MATH_NOMEM;
	}

	memset(in, 0, sizeof(in));

	// create plan for in-place transform
	fftw_plan plan    = fftw_plan_dft_r2c_2d (Src->mem2d->GetNy (), Src->mem2d->GetNx (), 
						  in, dat, FFTW_ESTIMATE);

	if (plan == NULL) {
                XSM_DEBUG (DBG_L3, "F2D FFT: libfftw: Error creating plan");
                return MATH_LIB_ERR;
	}

        double tmp=0.0;
	for(int v = 0; v < Src->mem2d->GetNv(); v++){
                Dest->mem2d->SetLayer(v);
                Src->mem2d->SetLayer(v);
                //g_print ("V %d [%d]\n",v, Src->mem2d->GetNv());

	
                // convert image data to fftw_real
                for (int line=0; line < Src->mem2d->GetNy(); line++) {
                        SrcZ ->SetPtr(0, line);
                        for (int col=0; col < Src->mem2d->GetNx(); col++){
#if 0
                                //g_print ("XY %d %d [%d,%d]\n",col, line, Src->mem2d->GetNx(), Src->mem2d->GetNy());
                                double tmp2 = SrcZ->GetNext();
                                tmp2 = tmp2 == 0.0 ? tmp : tmp2;
                                tmp = tmp2;
                                in[line*xlen2 + col] = tmp;
#else
                                in[line*xlen2 + col] = SrcZ->GetNext();
#endif
                        }
                }

                // compute 2D transform using in-place fourier transform
                fftw_execute (plan);

                // convert complex data to image
                // and flip image that spatial freq. (0,0) is in the middle
                // (quadrant swap using the QSWP macro defined in xsmmath.h)
  
                XSM_DEBUG (DBG_L3, "F2D FFT done, converting data to ABS/RE/IM");

                double I;
                int m,n;
                double norm=Src->mem2d->GetNx() * Src->mem2d->GetNy();

                for (int line=0; line < Src->mem2d->GetNy(); line++) {
                        for (int i,col=0; col < xlen; col++) {
                                i=line*xlen + col;
                                m = QSWP(col, Src->mem2d->GetNx());
                                n = QSWP(line, Src->mem2d->GetNy());
                                I = sqrt (c_re(dat[i]) * c_re(dat[i]) +  c_im(dat[i]) * c_im(dat[i])) / norm;
                                if (flg_cpx){
                                        Dest->mem2d->PutDataPkt(I, m, n, 0);
                                        Dest->mem2d->PutDataPkt(c_re(dat[i]) / norm, m, n, 1);
                                        Dest->mem2d->PutDataPkt(c_im(dat[i]) / norm, m, n, 2);
                                }
                                else
                                        Dest->mem2d->PutDataPkt(I, m, n);
                                if(line != (Src->mem2d->GetNy()/2)){
                                        m = Src->mem2d->GetNx() - m;
                                        n = Src->mem2d->GetNy() - n;
                                        if (flg_cpx){
                                                Dest->mem2d->PutDataPkt(I, m, n, 0);
                                                Dest->mem2d->PutDataPkt(c_re(dat[i]) / norm, m, n, 1);
                                                Dest->mem2d->PutDataPkt(c_im(dat[i]) / norm, m, n, 2);
                                        }
                                        else
                                                Dest->mem2d->PutDataPkt(I, m, n);
                                }
                        }
                }
	}  

        XSM_DEBUG (DBG_L3, "F2D FFT done, cleaning up plan, data 1");
	// destroy plan
        if (plan)
                fftw_destroy_plan (plan); 

        XSM_DEBUG (DBG_L3, "F2D FFT done, cleaning up plan, data 2");
	// free real/complex data memory
        if (dat)
                fftw_free (dat);

        Dest->mem2d->data->CopyLookups (Src->mem2d->data);
        Dest->mem2d->copy_layer_information (Src->mem2d);
	return MATH_OK;
}


