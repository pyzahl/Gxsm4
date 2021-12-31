/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
 *
 * Gxsm Plugin Name: crosscorrelation.C
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
 * this is a LaTeX style section used for cross generation of the PlugIn Manual 
 * Chapter. Add a complete PlugIn documentation inbetween the Begin/End marks!
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Crosscorrelation
% PlugInName: crosscorrelation
% PlugInAuthor: Erik Muller
% PlugInAuthorEmail: emmuller@users.sourceforge.net
% PlugInMenuPath: Math/Statistic/Cross Correlation

% PlugInDescription
 Computes the crosscorrelation of two images using a masked area of first source (the active scan).

-- WORK IN PROGRESS --

%%% \[ Z' = |\text{IFT} (\text{FT} (Z_{\text{active}}) \times \text{FT} (Z_{\text{X}}) )| \]

% PlugInUsage
 Call \GxsmMenu{Math/Statistic/Cross Correlation} to execute.

% OptPlugInSources
 The active and X channel are used as data source, a rectangular
 selection mask is used for feature selection.

% OptPlugInDest
 The computation result is placed into an existing math channel, else
 into a new created math channel.

% OptPlugInNotes
 The quadrants of the resulting invers spectrum are aligned in a way,
 that the intensity of by it self correlated pixels (distance zero) is
 found at the image center and not at all four edges.


% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <complex>
#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"
#include "core-source/xsmmath.h"

#define UTF8_ANGSTROEM "\303\205"

//	*((__complex__ double*)(out1[0])) = *((__complex__ double*)(in1[0])) * *((__complex__ double*)(in1[0]));

#define ComplexP (__complex__ double*)
#define ComplexProd(A,B,C) *(ComplexP(C)) = *(ComplexP(A)) * *(ComplexP(B))



// Plugin Prototypes
static void crosscorrelation_init( void );
static void crosscorrelation_about( void );
static void crosscorrelation_configure( void );
static void crosscorrelation_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
//#define GXSM_ONE_SRC_PLUGIN__DEF
#define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (crossmatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
static gboolean crosscorrelation_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
static gboolean crosscorrelation_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin crosscorrelation_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	"crosscorrelation-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
	"M1S"
#else
	"M2S"
#endif
	"-ST",
	// Plugin's Category - used to crossdecide on Pluginloading or ignoring
	// NULL: load, else
	// example: "+noHARD +STM +AFM"
	// load only, if "+noHARD: no hardware" and Instrument is STM or AFM
	// +/-xxxHARD und (+/-INST or ...)
	NULL,
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	g_strdup("Computes the Cross-Correlation of a Scan."),
	// Author(s)
	"Percy Zahl",
	// Menupath to position where it is appendet to
	"math-statistics-section",
	// Menuentry
	N_("Cross Correlation"),
	// help text shown on menu
	N_("Do a Cross-Correlation of the scan"),
	// more info...
	"Calc. Cross Correlation of Scan",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	crosscorrelation_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	crosscorrelation_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	crosscorrelation_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

	crosscorrelation_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin crosscorrelation_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin crosscorrelation_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin crosscorrelation_m1s_pi
#else
GxsmMathTwoSrcPlugin crosscorrelation_m2s_pi
#endif
= {
	// math-function to run, see prototype(s) above!!
	crosscorrelation_run
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm crosscorrelation Plugin\n\n"
                                   "Computes the Cross-Correlation of a scan:\n"
				   "The Cross-Correlation of a scan is computed\n"
				   "by FT the source Scan, setting the phase\n"
				   "to zero and doing the IFT.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	crosscorrelation_pi.description = g_strdup_printf(N_("Gxsm MathOneArg crosscorrelation plugin %s"), VERSION);
	return &crosscorrelation_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
	return &crosscorrelation_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
	return &crosscorrelation_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void crosscorrelation_init(void)
{
	PI_DEBUG (DBG_L2, "crosscorrelation Plugin Init");
}

// about-Function
static void crosscorrelation_about(void)
{
	const gchar *authors[] = { crosscorrelation_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  crosscorrelation_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

double WindowDefault=0;
double WindowLast=0;

// configure-Function
static void crosscorrelation_configure(void)
{

// 	if(crosscorrelation_pi.app)
// 		crosscorrelation_pi.app->message("crosscorrelation Plugin Configuration");
	crosscorrelation_pi.app->ValueRequest("Enter Value", "Window", 
					      "Please enter the window type\n"
					      "0: Uniform\n"
					      "1: Hanning\n"
					      "2: Hamming\n"
					      "3: Bartlett\n"
					      "4: Blackman\n"
					      "5: Connes",
					      crosscorrelation_pi.app->xsm->Unity, 
					      0, 5, ".0f", &WindowDefault);


}

// cleanup-Function
static void crosscorrelation_cleanup(void)
{
	PI_DEBUG (DBG_L2, "crosscorrelation Plugin Cleanup");
}



// run-Function
static gboolean crosscorrelation_run(Scan *Src1, Scan *Src2, Scan *Dest)
{ 
	double window=WindowLast;
	if(fabs(WindowDefault) > 1e-10)
		window = WindowDefault;
	else
		crosscorrelation_pi.app->ValueRequest("Enter Value", "Window", 
						      "Please enter the window type\n"
						      "0: Uniform\n"
						      "1: Hanning\n"
						      "2: Hamming\n"
						      "3: Bartlett\n"
						      "4: Blackman\n"
						      "5: Connes",
						      crosscorrelation_pi.app->xsm->Unity, 
						      0, 5, ".0f", &window);


	// Set Dest to Float, Range: +/-100% =^= One Pixel
	Dest->mem2d->Resize(Dest->data.s.nx, Dest->data.s.ny, ZD_FLOAT);

	// Get the size of the two images.
	int Nx1,Ny1,Nx2,Ny2;
	Nx1=Src1->mem2d->GetNx();
	Ny1=Src1->mem2d->GetNy();
	Nx2=Src2->mem2d->GetNx();
	Ny2=Src2->mem2d->GetNy();


	// allocate memory for the complex data and one line buffer
	fftw_complex *in1  = new fftw_complex [Nx1*Ny1];
	fftw_complex *in2  = new fftw_complex [Nx2*Ny2];
	fftw_complex *in3  = new fftw_complex [Nx1*Ny1];    
	fftw_complex *out1 = new fftw_complex [Nx1*Ny1];
	fftw_complex *out2 = new fftw_complex [Nx2*Ny2];
	fftw_complex *out3 = new fftw_complex [Nx1*Ny1];    





	double xfactor=1,yfactor=1;

	// Fill array with data and apply window function to the first image
	for (int pos=0, line=0; line < Ny1; ++line)
		for (int col=0; col < Nx1; ++col, ++pos){		  
		       
			if(window == 0.){
				xfactor=1.0;
				yfactor=1.0;
			}
			else if(window == 1.){
				xfactor = 0.5*(1.0+cos(2.0*M_PI*(line-Ny1/2.0)/Ny1));
				yfactor = 0.5*(1.0+cos(2.0*M_PI*(col-Nx1/2.0)/Nx1));
			}
			else if(window == 2.){
				xfactor = 0.54+0.46*cos(2.0*M_PI*(line-Ny1/2.0)/Ny1);
				yfactor = 0.54+0.46*cos(2.0*M_PI*(col-Nx1/2.0)/Nx1);
			}
			else if(window == 3.){
				xfactor=(1.0-2.0*sqrt((line-Ny1/2.0)*(line-Ny1/2.0))/Ny1);
				yfactor=(1.0-2.0*sqrt((col-Nx1/2.0)*(col-Nx1/2.0))/Nx1);
			}
			else if(window == 4.){
				xfactor = 0.42+0.5*cos(2.0*M_PI*(line-Ny1/2.0)/Ny1)+0.08*cos(4.0*M_PI*(line-Ny1/2.0)/Ny1);
				yfactor = 0.42+0.5*cos(2.0*M_PI*(col-Nx1/2.0)/Nx1)+0.08*cos(4.0*M_PI*(col-Nx1/2.0)/Nx1);
			}
			else if(window == 5.){
				xfactor = (1.0-4.0*(line-Ny1/2.0)*(line-Ny1/2.0)/Ny1/Ny1)*(1.-4.0*(line-Ny1/2.0)*(line-Ny1/2.0)/Ny1/Ny1);
				yfactor = (1.0-4.0*(col-Nx1/2.0)*(col-Nx1/2.0)/Nx1/Nx1)*(1.-4.0*(col-Nx1/2.0)*(col-Nx1/2.0)/Nx1/Nx1);
			}
			
			c_re(in1[pos]) = Src1->mem2d->GetDataPkt(col, line)*xfactor*yfactor;
			c_im(in1[pos]) = 0.;			
		}
	

	

	// Fill array with data and apply window function to the second image
	for (int pos=0, line=0; line < Ny2; ++line)
		for (int col=0; col < Nx2; ++col, ++pos){

			if(window == 0.){
				xfactor=1.0;
				yfactor=1.0;
			}
			else if(window == 1.){
				xfactor = 0.5*(1.0+cos(2.0*M_PI*(line-Ny2/2.0)/Ny2));
				yfactor = 0.5*(1.0+cos(2.0*M_PI*(col-Nx2/2.0)/Nx2));
			}
			else if(window == 2.){
				xfactor = 0.54+0.46*cos(2.0*M_PI*(line-Ny2/2.0)/Ny2);
				yfactor = 0.54+0.46*cos(2.0*M_PI*(col-Nx2/2.0)/Nx2);
			}
			else if(window == 3.){
				xfactor=(1.0-2.0*sqrt((line-Ny2/2.0)*(line-Ny2/2.0))/Ny2);
				yfactor=(1.0-2.0*sqrt((col-Nx2/2.0)*(col-Nx2/2.0))/Nx2);
			}
			else if(window == 4.){
				xfactor = 0.42+0.5*cos(2.0*M_PI*(line-Ny2/2.0)/Ny2)+0.08*cos(4.0*M_PI*(line-Ny2/2.0)/Ny2);
				yfactor = 0.42+0.5*cos(2.0*M_PI*(col-Nx2/2.0)/Nx2)+0.08*cos(4.0*M_PI*(col-Nx2/2.0)/Nx2);
			}
			else if(window == 5.){
				xfactor = (1.0-4.0*(line-Ny2/2.0)*(line-Ny2/2.0)/Ny2/Ny2)*(1.0-4.0*(line-Ny2/2.0)*(line-Ny2/2.0)/Ny2/Ny2);
				yfactor = (1.0-4.0*(col-Nx2/2.0)*(col-Nx2/2.0)/Nx2/Nx2)*(1.0-4.0*(col-Nx2/2.0)*(col-Nx2/2.0)/Nx2/Nx2);
			}

			c_re(in2[pos]) = Src2->mem2d->GetDataPkt(col, line)*xfactor*yfactor;
			c_im(in2[pos]) = 0.;			
		}



 	// do the fourier transform
 	// create plan for fft
 	fftw_plan plan1 = fftw_plan_dft_2d (Ny1, Nx1, in1, out1, FFTW_FORWARD, FFTW_ESTIMATE);
	
 	// compute fft
 	fftw_execute (plan1);


 	
 	fftw_plan plan2 = fftw_plan_dft_2d (Ny2, Nx2, in2, out2, FFTW_FORWARD, FFTW_ESTIMATE);
	
 	// compute fft
 	fftw_execute (plan2);
	

	//Matrix Multiply -- this is not the right way!
// 	for(int i=0; i < Ny1; ++i)
// 		for(int j=0; j < Nx2; ++j){
// 			c_re(in3[j+Nx1*i])=0;
// 			c_im(in3[j+Nx1*i])=0;			      
// 				for(int k=0; k < Ny2; ++k){
// 					c_re(in3[j+Nx1*i])+=c_re(out1[k+Nx1*i])*c_re(out2[j+Nx2*k])+c_im(out1[k+Nx1*i])*c_im(out2[j+Nx2*k]);
// 					c_im(in3[j+Nx1*i])+=c_im(out1[k+Nx1*i])*c_re(out2[j+Nx2*k])-c_re(out1[k+Nx1*i])*c_im(out2[j+Nx2*k]);
// 				}
// 		}
	//Element-by-Element Multiply -- this is the right way!
 	for(int i=0; i < Ny1; ++i)
		for(int j=0; j < Nx2; ++j){
			c_re(in3[j+Nx1*i])=0;
			c_im(in3[j+Nx1*i])=0;			      
			c_re(in3[j+Nx1*i])=c_re(out1[j+Nx1*i])*c_re(out2[j+Nx2*i])+c_im(out1[j+Nx1*i])*c_im(out2[j+Nx2*i]);
			c_im(in3[j+Nx1*i])=c_im(out1[j+Nx1*i])*c_re(out2[j+Nx2*i])-c_re(out1[j+Nx1*i])*c_im(out2[j+Nx2*i]);
		}


	fftw_plan plan3 = fftw_plan_dft_2d (Ny1, Nx1, in3, out3, FFTW_BACKWARD, FFTW_ESTIMATE);
 	// compute fft
 	fftw_execute (plan3);



	//Find Normalization factor
	double SumNorm=0.0;
	for(int i=0; i < Nx1*Ny1; i++){
		SumNorm += c_re(in1[i])*c_re(in2[i]);
	    }


	//WRITE DATA TO DEST
	for (int pos=0, i=0; i < Ny1; ++i)
		for (int j=0; j < Nx1; ++j, ++pos){
			Dest->mem2d->PutDataPkt(c_re(out3[pos])/SumNorm/Nx1/Ny1,//j, i) ;
   				     QSWP (j,Nx1), 
   				     QSWP (i, Ny1));
		}

					     
	// destroy plan
	fftw_destroy_plan (plan1);
	fftw_destroy_plan (plan2);
	fftw_destroy_plan (plan3);
	
	// free memory
	delete in1;
	delete in2;
        delete in3;				     
	delete out1;
	delete out2;
	delete out3;

	Dest->mem2d->data->MkXLookup(0., Src1->data.s.rx);
	Dest->mem2d->data->MkYLookup(0., Src1->data.s.ry);
	UnitObj *u;
//	Dest->data.SetZUnit(u=new LinUnit(" "," ",0.02328,0));
	Dest->data.SetZUnit(u=new LinUnit(" "," ",1.,0));
	Dest->data.SetXUnit(u=new LinUnit(UTF8_ANGSTROEM,"Ang","L",1.,0));
	Dest->data.SetYUnit(u=new LinUnit(UTF8_ANGSTROEM,"Ang","L",1.,0));

	Dest->data.s.dz=1.0;
	Dest->data.s.dx=1.0;
	Dest->data.s.dy=1.0;
	delete u;

     	return MATH_OK;
}
