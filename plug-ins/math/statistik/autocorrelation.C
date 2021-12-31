/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Erik Muller
pluginname		=autocorrelation
pluginmenuentry 	=Auto Correlation
menupath		=Math/Statistics/
entryplace		=Statistics
shortentryplace	=ST
abouttext		=Computes the Auto-Correlation of a scan.
smallhelp		=Calc. Auto Correlation from Scan
longhelp		=The Auto-Correlation of a scan is given by the following Z*Z=IFFT (| FT(Z) |^2).
 * 
 * Gxsm Plugin Name: autocorrelation.C
 * ========================================
 * 
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors:Erik Muller
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
% PlugInDocuCaption: Autocorrelation
% PlugInName: autocorrelation
% PlugInAuthor: Erik Muller
% PlugInAuthorEmail: emmuller@users.sourceforge.net
% PlugInMenuPath: Math/Statistic/Auto Correlation

% PlugInDescription
Computes the autocorrelation of an image.

\[ Z' = |\text{IFT} (\text{FT} (Z))| \]

% PlugInUsage
Call \GxsmMenu{Math/Statistic/Auto Correlation} to execute.

% OptPlugInSources
The active channel is used as data source.

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

// Plugin Prototypes
static void autocorrelation_init( void );
static void autocorrelation_about( void );
static void autocorrelation_configure( void );
static void autocorrelation_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean autocorrelation_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean autocorrelation_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin autocorrelation_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  g_strdup ("autocorrelation-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
	    "-ST"),
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup("Computes the Auto-Correlation of a Scan."),
  // Author(s)
  "Erik Muller",
  // Menupath to position where it is appendet to
  "math-statistics-section",
  // Menuentry
  N_("Auto Correlation"),
  // help text shown on menu
  N_("Do a Auto-Correlation of the scan"),
  // more info...
  "Calc. Auto Correlation of Scan",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  autocorrelation_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  autocorrelation_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  autocorrelation_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  autocorrelation_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin autocorrelation_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin autocorrelation_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin autocorrelation_m1s_pi
#else
 GxsmMathTwoSrcPlugin autocorrelation_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   autocorrelation_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm autocorrelation Plugin\n\n"
                                   "Computes the Auto-Correlation of a scan:\n"
				   "The Auto-Correlation of a scan is computed\n"
				   "by FT the source Scan, setting the phase\n"
				   "to zero and doing the IFT.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  autocorrelation_pi.description = g_strdup_printf(N_("Gxsm MathOneArg autocorrelation plugin %s"), VERSION);
  return &autocorrelation_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &autocorrelation_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &autocorrelation_m2s_pi; 
}
#endif


/* Here we go... */
// init-Function
static void autocorrelation_init(void)
{
  PI_DEBUG (DBG_L2, "autocorrelation Plugin Init");
}

// about-Function
static void autocorrelation_about(void)
{
  const gchar *authors[] = { autocorrelation_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  autocorrelation_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

int window_type=0;

int window_callback(GtkComboBox *widget, gpointer data){
	window_type =  gtk_combo_box_get_active (widget);
	return 0;
}

// configure-Function
static void autocorrelation_configure(void)
{


}

// cleanup-Function
static void autocorrelation_cleanup(void)
{
  PI_DEBUG (DBG_L2, "autocorrelation Plugin Cleanup");
}


// run-Function
 static gboolean autocorrelation_run(Scan *Src, Scan *Dest)
{ 

/*-------------------------------------------------*/

	// Selects the data in the rectangle if there is a rectangle
	// else, uses all the data
	int StartX=0,EndX=0,StartY=0,EndY=0;

	EndX=Src->mem2d->GetNx();
	EndY=Src->mem2d->GetNy();

	double hi, lo, z;
	int success = FALSE;
	int n_obj = Src->number_of_object ();
	Point2D p[2];

	while (n_obj--){
		scan_object_data *obj_data = Src->get_object_data (n_obj);
		
		if (strncmp (obj_data->get_name (), "Rectangle", 9) )
			continue; // only points are used!
		
		if (obj_data->get_num_points () != 2) 
			continue; // sth. is weired!
		
		double x,y;
		obj_data->get_xy_i_pixel (0, x, y);
		p[0].x = (int)x; p[0].y = (int)y;
		obj_data->get_xy_i_pixel (1, x, y);
		p[1].x = (int)x; p[1].y = (int)y;
		
		StartX=p[0].x; EndX=p[1].x;
		StartY=p[0].y; EndY=p[1].y;

		success = TRUE;
		break;
	}
	
	// If the box is outside the scan area...return an error
 	if (StartX < 0 | StartY < 0 | EndX > Src->mem2d->GetNx() | EndY > Src->mem2d->GetNy() )
 		return MATH_SELECTIONERR;

/* ---------------------------------------------------------------- */

	GtkWidget *dialog, *label, *wmenu;
//	GtkWidget *image;
//	image = gtk_image_new_from_file ("/home/emuller/Gxsm-2.0/plug-ins/math/statistik/test.png");

	dialog = gtk_dialog_new_with_buttons ("Message",
					      NULL,
					      //(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
					      GTK_DIALOG_MODAL,
					      _("_OK"), GTK_RESPONSE_ACCEPT,
					      NULL);
	
	wmenu = gtk_combo_box_text_new ();
	
	gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wmenu),"no-window", "No Window");
	gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wmenu),"hanning", "Hanning");
	gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wmenu),"hamming", "Hamming");
	gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wmenu),"bartlett", "Bartlett");
	gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wmenu),"blackman", "Blackman");
	gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wmenu),"connes", "Connes");

	g_signal_connect (G_OBJECT(wmenu),"changed", G_CALLBACK (window_callback), (gpointer) 1 );

	const gchar *message="Please chose a window function";

	/* Create the widgets */
	label = gtk_label_new (message);
	
	/* Ensure that the dialog box is destroyed when the user responds. */
	g_signal_connect_swapped (dialog,
				  "response",			    
				  G_CALLBACK (gtk_widget_destroy),
				  dialog);
	

	/* Add the label, and show everything we've added to the dialog. */

//	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
//			   image);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG(dialog))),
			   label);

	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG(dialog))),
			   wmenu);
	gtk_widget_show_all (dialog);
	gtk_dialog_run (GTK_DIALOG (dialog));
	
	std::cout << "Window Type is: " << window_type << std::endl;

/*-------------------------------------------------*/
	
	int Nx, Ny;
	Nx=(EndX-StartX);
	Ny=(EndY-StartY);


	// Set Dest to Float, Range: +/-100% =^= One Pixel
	Dest->mem2d->Resize(Nx, Ny, ZD_FLOAT);

	// allocate memory for the complex data and one line buffer
	fftw_complex *in1  = new fftw_complex [Nx*Ny];
	fftw_complex *in2  = new fftw_complex [Nx*Ny];
  
	fftw_complex *out1 = new fftw_complex [Nx*Ny];
	fftw_complex *out2 = new fftw_complex [Nx*Ny];


/*-------------------------------------------------*/

	// Applies the window function
	double xfactor=1.,yfactor=1.;

	// Fill array with data and apply image window to the image
	for (int pos=0, line=StartY; line < EndY; ++line)
		for (int col=StartX; col < EndX; ++col, ++pos){		  
		       
			switch (window_type){
			case 0:
				xfactor=1.0;
				yfactor=1.0;
				break;
			case 1:
				xfactor = 0.5*(1.0+cos(2.0*M_PI*((line-StartY)-Ny/2.0)/Ny));
				yfactor = 0.5*(1.0+cos(2.0*M_PI*((col-StartX)-Nx/2.0)/Nx));
				break;
			case 2:
				xfactor = 0.54+0.46*cos(2.0*M_PI*((line-StartY)-Ny/2.0)/Ny);
				yfactor = 0.54+0.46*cos(2.0*M_PI*((col-StartX)-Nx/2.0)/Nx);
				break;
			case 3:
				xfactor=(1.0-2.0*sqrt(((line-StartY)-Ny/2.0)*(line-StartY-Ny/2.0))/Ny);
				yfactor=(1.0-2.0*sqrt(((col-StartX)-Nx/2.0)*((col-StartX)-Nx/2.0))/Nx);
				break;
			case 4:
				xfactor = 0.42+0.5*cos(2.0*M_PI*((line-StartY)-Ny/2.0)/Ny)+0.08*cos(4.0*M_PI*((line-StartY)-Ny/2.0)/Ny);
				yfactor = 0.42+0.5*cos(2.0*M_PI*((col-StartX)-Nx/2.0)/Nx)+0.08*cos(4.0*M_PI*((col-StartX)-Nx/2.0)/Nx);
				break;
			case 5:
				xfactor = (1.0-4.0*((line-StartY)-Ny/2.0)*((line-StartY)-Ny/2.0)/Ny/Ny)*(1.0-4.0*((line-StartY)-Ny/2.0)*((line-StartY)-Ny/2.0)/Ny/Ny);
				yfactor = (1.0-4.0*((col-StartX)-Nx/2.0)*((col-StartX)-Nx/2.0)/Nx/Nx)*(1.0-4.0*((col-StartX)-Nx/2.0)*((col-StartX)-Nx/2.0)/Nx/Nx);
				break;
			}
			
			c_re(in1[pos]) =(Src->mem2d->GetDataPkt(col, line))*xfactor*yfactor;
			c_im(in1[pos]) = 0.;			
	  }


/*-------------------------------------------------*/

 	// do the fourier transform
 	// create plan for fft
 	fftw_plan plan1 = fftw_plan_dft_2d (Ny, Nx, in1, out1, FFTW_FORWARD, FFTW_ESTIMATE);
	
 	// compute fft
 	fftw_execute (plan1);

 	for(int i=0; i < Ny; ++i)
		for(int j=0; j < Nx; ++j){
			c_re(in2[j+Nx*i])=0.0;
			c_im(in2[j+Nx*i])=0.0;			      
			c_re(in2[j+Nx*i])=c_re(out1[j+Nx*i])*c_re(out1[j+Nx*i])+c_im(out1[j+Nx*i])*c_im(out1[j+Nx*i]);
			c_im(in2[j+Nx*i])=c_im(out1[j+Nx*i])*c_re(out1[j+Nx*i])-c_re(out1[j+Nx*i])*c_im(out1[j+Nx*i]);
		}


	fftw_plan plan2 = fftw_plan_dft_2d (Ny, Nx, in2, out2, FFTW_BACKWARD, FFTW_ESTIMATE);
 	// compute fft
 	fftw_execute (plan2);



	//Find Normalization factor
	double SumNorm=0;
	for(int i=0; i < Nx*Ny; i++){
		SumNorm += c_re(in1[i])*c_re(in1[i]);
	    }



	//WRITE DATA TO DEST
	for (int pos=0, i=0; i < Ny; ++i)
		for (int j=0; j < Nx; ++j, ++pos){
			Dest->mem2d->PutDataPkt(c_re(out2[pos])/SumNorm/Nx/Ny,//j, i) ;
   				     QSWP (j,Nx), 
   				     QSWP (i, Ny));
		}

					     
	// destroy plan
	fftw_destroy_plan (plan1);
	fftw_destroy_plan (plan2);

	
	// free memory
	delete in1;
	delete in2;
				     
	delete out1;
	delete out2;


	Dest->mem2d->data->MkXLookup(0., Src->data.s.rx*Nx/Src->mem2d->GetNx());
	Dest->mem2d->data->MkYLookup(0., Src->data.s.ry*Ny/Src->mem2d->GetNy());
	UnitObj *u;
//	Dest->data.SetZUnit(u=new LinUnit(" "," ",0.02328,0));	delete u;
	Dest->data.SetZUnit(u=new LinUnit(" "," ",1.,0)); 	delete u;
	Dest->data.SetXUnit(u=new LinUnit(UTF8_ANGSTROEM,"Ang","L",1.,0)); 	delete u;
	Dest->data.SetYUnit(u=new LinUnit(UTF8_ANGSTROEM,"Ang","L",1.,0));	delete u;


	Dest->data.s.dz=1.0;
	Dest->data.s.dx=1.0;
	Dest->data.s.dy=1.0;




   return MATH_OK;
	
}

