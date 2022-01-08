/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author          =Jacob
pluginname              =VacancyLineAnalysis
pluginmenuentry         =Vacancy Line Analysis
menupath                =Math/Statistics/
entryplace              =Statistics
shortentryplace =ST
abouttext               =Determines position of vacancy lines
smallhelp               =Use lines to mark positions of vacancy lines.
longhelp                =Rotate sample so vacancy lines are vertical.
                         Dimer size, and searching parameters can be set in
                         the configure dialog.
 * 
 * Gxsm Plugin Name: VacancyLineAnalysis.C
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
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Vacancy Line Analysis
% PlugInName: VacancyLineAnalysis
% PlugInAuthor: J.S. Palmer
% PlugInAuthorEmail: jspalmer@mines.edu
% PlugInMenuPath: Math/Statistics/Vacancy Line Analysis

% PlugInDescription
This plugin is used to find the location of dimer vacancy lines.
It was designed to determine statistics of thin layers of SiGe.  A very
important feature to note is that scans must be rotated so that the
vacancy lines are vertical. Lines are searched for vertically down the
scan.  Line objects are used to mark the start and stop positions
for the vacancy lines

Four output results can be created. The most apparent is the copy of
the scan with the position of the vacancy lines standing out as raised
up. A second layer of that scan shows a 3D histogram of the relative
horizontal positions of two vacancies on the same vacancy line
separated by $n$ dimer rows.  $n=0$ is shown at the top of the screen and
the separations are all 0.  The next row down shows a histogram for
$n=1$, the next is $n=2$ and so forth up to $N$ rows down defined in the
configuration (Dimer Rows included in histogram).  The positions of
the vacancies are also saved to a file.  The last output is an
histogram of the space between vacancy lines.

% PlugInUsage
Call it from \GxsmMenu{Math/Statistics/Vacancy Line Analysis}.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInObjects
Lines must be used to show where the vacancy lines should start and
end. Use  one line for each vacancy line to be located.

% OptPlugInDest
The position of the vacancy lines is displayed in the math channel or
a new channel if a math channel is not open.  This channel will have a
second layer showing a histogram of the vacancy line straightness. If
a histogram showing the vacancy line separation is chosen, a new
profile window is also opened.

% OptPlugInConfig
Several parameters are adjustable in the configuration window.  

\begin{description}
\item[Dimer Spacing] The spacing of the dimers -- is $7.68\:$\AA\ on Si.  
\item[Max Vacancy Line Shift] The horizontal distance searched in each
  direction for the position on the dimer vacancy.  Typical
  values are $2\dots3$ atomic rows.
\item[Pixels to Average] Number of pixels in horizontal direction to
  average in searching for vacancy.
\item[Dimer rows included in 3D Histogram] The Maximum number of dimer
  rows between two vacancies on the on the same vacancy line to be
  included in the histogram. ($N$ as described above in the Description
  area)
\item[3D Histogram Size] The size can be either $1:1$ (default), meaning
  that the horizontal scale is correct.  If $0$ is entered the output is
  scaled to fill the window.
\item[Line-Spacing Histogram Bin Width] The width of each bin in the
  histogram showing the vacancy line spacing.  If $0$ is entered no
  histogram is created.
\end{description}

% OptPlugInFiles
The user is prompted for an output file. A matrix is saved in the file
containing the positions of the vacancy lines.  Each column in the
matrix represents a vacancy line.  Each row represents a dimer row.
The value of a matrix position is the horizontal pixel coordinates of
the dimer vacancy.  A value of $-1$ means is is before the beginning of
the line or after the end of the line.  Two rows before the matrix are
used to designate the row number for the first and last dimer vacancy
in that column (vacancy line).  The first row is designated as $0$.
Also included in the heading is the dimer spacing used, which gives
the conversion for rows to vertical position, and the x and y pixel
spacing. The x pixel spacing provide a conversion from pixel position
to horizontal position.
 

%% OptPlugInRefs
%nope.

%% OptPlugInKnownBugs
%No known.

%% OptPlugInNotes
%Hmm, no notes\dots

% OptPlugInHints
Here is a quick check list:
\begin{enumerate}
\item Rotate the scan so the Vacancy lines are as close to vertical as
possible.
\item Move start and stop positions or divide a line in two using two
  objects if the vacancy line is difficult to follow.
\item Save your line objects -- it is time consuming to put them in and
  check to make sure each line is accurately located.
\item Determine the correct dimer spacing for your image using a
  Power Spectrum of SPALEED simulation.
\end{enumerate}


% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "app_profile.h"
#include "glbvars.h"
#include "surface.h"


typedef struct {
        int x,yt,yb;
} my_point;
typedef struct {
        int first,last;
} matrix_elements;

// Plugin Prototypes
static void VacancyLineAnalysis_init( void );
static void VacancyLineAnalysis_about( void );
static void VacancyLineAnalysis_configure( void );
static void VacancyLineAnalysis_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean VacancyLineAnalysis_run( Scan *Src, Scan *Dest);
#else
// "TwoSrc" Prototype
 static gboolean VacancyLineAnalysis_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin VacancyLineAnalysis_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "VacancyLineAnalysis-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-ST",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("Rotate sample so vacancy lines are vertical."
  "  Dimer size, and searching parameters can be set in "
	    " the configure dialog."),                   
  // Author(s)
  "Jacob Palmer",
  // Menupath to position where it is appendet to
  "math-statistics-section",
  // Menuentry
  N_("Vacancy Line Analysis"),
  // help text shown on menu
  N_(  "Rotate sample so vacancy lines are vertical."
       "  Dimer size, and searching parameters can be set in "
       " the configure dialog."),                   
  // more info...
  "Use lines to mark positions of vacancy lines",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  VacancyLineAnalysis_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  VacancyLineAnalysis_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  VacancyLineAnalysis_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  VacancyLineAnalysis_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin VacancyLineAnalysis_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin VacancyLineAnalysis_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin VacancyLineAnalysis_m1s_pi
#else
 GxsmMathTwoSrcPlugin VacancyLineAnalysis_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   VacancyLineAnalysis_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm VacancyLineAnalysis Plugin\n\n"
         "Determines position of vacancy lines for statistical analysis");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  VacancyLineAnalysis_pi.description = g_strdup_printf(N_("Gxsm MathOneArg VacancyLineAnalysis plugin %s"), VERSION);
  return &VacancyLineAnalysis_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &VacancyLineAnalysis_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &VacancyLineAnalysis_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void VacancyLineAnalysis_init(void)
{
  PI_DEBUG (DBG_L2, "VacancyLineAnalysis Plugin Init");
}

// about-Function
static void VacancyLineAnalysis_about(void)
{
  const gchar *authors[] = { VacancyLineAnalysis_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  VacancyLineAnalysis_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
double yaverage=7.68;  // Distance between dimer rows
double spred=11.52;     // Maximum shift in vacancy line per dimer row
double xaverage=3.;   // x distance average used to find minimum 
double bin_width=0.;  //default - no histogram of line spacing
double n_dimer_rows=10.;  // # of dimer rows included for determining straightness 
double hist_scale=1.;

static void VacancyLineAnalysis_configure(void)
{
  GtkDialogFlags flags = GTK_DIALOG_MODAL; // | GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("VacancyLineAnalysis Setup"),
						   NULL,
						   flags,
						   N_("_OK"),
						   GTK_RESPONSE_OK,
						   N_("_CANCEL"),
						   GTK_RESPONSE_CANCEL,
						   NULL);
  BuildParam bp;
  gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);
 
  bp.grid_add_label ("Enter Analysis Information");
  bp.new_line ();
  bp.grid_add_ec ("Dimer Spacing (A):", VacancyLineAnalysis_pi.app->xsm->Unity, &yaverage, .01, 100., ".2f"); bp.new_line ();
  bp.grid_add_ec ("Max Vacancy Line Shift [A]:", VacancyLineAnalysis_pi.app->xsm->Unity, &spred, 1., 100., ".2f"); bp.new_line ();
  bp.grid_add_ec ("Pixels to Average:", VacancyLineAnalysis_pi.app->xsm->Unity, &xaverage, 0., 100., ".0f"); bp.new_line ();
  bp.grid_add_ec ("Dimer rows included in 3D Histogram", VacancyLineAnalysis_pi.app->xsm->Unity, &n_dimer_rows, 1., 100., ".0f"); bp.new_line ();
  bp.grid_add_ec ("3D Histogram Size (0=Scaled,1=1:1)", VacancyLineAnalysis_pi.app->xsm->Unity, &hist_scale, 0., 1., ".0f"); bp.new_line (); 
  bp.grid_add_ec ("Line-Spacing Histogram Bin Width [A]", VacancyLineAnalysis_pi.app->xsm->Unity, &bin_width, 0., 100., ".2f"); bp.new_line ();
  bp.grid_add_label ("Enter 0 for no line-spacing histogram");
 
  gtk_widget_show(dialog);

  int response = GTK_RESPONSE_NONE;
  g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
  
  // FIX-ME GTK4 ??
  // wait here on response
  while (response == GTK_RESPONSE_NONE)
    while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
  
  return response == GTK_RESPONSE_OK ? 1 : 0;
}



// cleanup-Function
static void VacancyLineAnalysis_cleanup(void)
{
  PI_DEBUG (DBG_L2, "VacancyLineAnalysis Plugin Cleanup");
}

// run-Function
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 static gboolean VacancyLineAnalysis_run(Scan *Src, Scan *Dest)
#else
 static gboolean VacancyLineAnalysis_run(Scan *Src1, Scan *Src2, Scan *Dest)
#endif
{
        // put plugins math code here...
        // For more math-access methods have a look at xsmmath.C
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// *********************************************************************************
// Add second layer to output scan
        Dest->data.s.dz = 1;
        Dest->data.s.nvalues = 2;
        UnitObj *Events_tmp = new UnitObj("","",".0f","#");
        Dest->data.SetZUnit (Events_tmp);
        delete Events_tmp;
        Dest->mem2d->Resize (Src->mem2d->GetNx(), Src->mem2d->GetNy(), Dest->data.s.nvalues);
        Dest->mem2d->data->MkVLookup (0., 1.);

        Dest->mem2d->SetLayer (0);
// *********************************************************************************
// data from config-Function

        double dy = Src->data.s.dy;
        double stepsize = (yaverage /dy );     
        PI_DEBUG (DBG_L2, "stepsize" << stepsize);
        int xsmoothing = (int) floor((xaverage-1)/2.);
        double dx =  Src->data.s.dx;
        int width = (int) (spred / dx );

// *********************************************************************************
// Copies over data from sourse window
     for(int line=0; line<Dest->mem2d->GetNy(); line++){


             for(int col=0; col<Dest->mem2d->GetNx(); col++){

                     Dest->mem2d->PutDataPkt(Src->mem2d->
                                             GetDataPkt(col, line), col, line);
             }
     }

// *********************************************************************************
//Locate and order all Objects 
     // I need at least one object
     int n_obj = Src->number_of_object ();
     if (n_obj < 1) 
             return MATH_SELECTIONERR;

     // iterate over all objects

     int *lookup = new int[n_obj];
     for (int i=0;i<n_obj; i++) lookup[i]=0;
        
     my_point *end_points = new my_point[n_obj];

     int n_lines = n_obj; 
     for (int i=0; i<n_obj; i++){
             scan_object_data *obj_data;
             obj_data = Src->get_object_data (i);

             obj_data->dump ();

             // only lines are supported:
             if (strncmp (obj_data->get_name (), "Line", 9) ){
                     lookup[i]=n_obj+1;
                     n_lines-=1;
                     end_points[i].x=Src->mem2d->GetNx();
                     continue;
             }

             int n_pts = obj_data->get_num_points ();
             // support for point objects, only
             if (n_pts != 2){
                     lookup[i]=n_obj+1;
                     n_lines-=1;
                     end_points[i].x=Src->mem2d->GetNx();
                     continue;
             }

             // get real world coordinates
             double x0,y0,x1,y1;                
             obj_data->get_xy_i (0, x0, y0);
             obj_data->get_xy_i (1, x1, y1);
             // PI_DEBUG (DBG_L2, "x0 " << x0 << " y0 " << y0 );
             // PI_DEBUG (DBG_L2, "x1 " << x1 << " y1 " << y1 );

             // calculate pixel coordinates
             int xl = (int)( (x0 - Src->data.s.x0 + Src->data.s.rx/2) / Src->data.s.dx );
             int yt = (int)( (Src->data.s.y0 - y0) / Src->data.s.dy );
             int xr = (int)( (x1 - Src->data.s.x0 + Src->data.s.rx/2) / Src->data.s.dx );
             int yb = (int)( (Src->data.s.y0 - y1) / Src->data.s.dy );
        
             // make sure the pixel coord. lie within the scan Src2
             if (xl < 0)  xl=0;
             if (xl > Src->mem2d->GetNx()-1)  xl = Src->mem2d->GetNx()-1;
             if (yt < 0)  yt=0;
             if (yt > Src->mem2d->GetNy()-1)  yt = Src->mem2d->GetNy()-1;
             if (xr < 0)  xr=0;
             if (xr > Src->mem2d->GetNx()-1)  xr = Src->mem2d->GetNx()-1;
             if (yb < 0)  yb=0;
             if (yb > Src->mem2d->GetNy()-1)  yb = Src->mem2d->GetNy()-1;
             // PI_DEBUG (DBG_L2, "xl " << xl << " yt " << yt );
             // PI_DEBUG (DBG_L2, "xr " << xr << " yb " << yb );

             // Set (xl,yt) as upper and (xr,yb) as lower points        
             if (yt>yb){
                     int ytemp=yt;yt=yb;yb=ytemp;
                     int xtemp=xl;xl=xr;xr=xtemp;
             }
             end_points[i].x=xl;
             end_points[i].yt=yt;
             end_points[i].yb=yb;

             // Create lookup table for order of lines
             for (int j=0; j<i;j++)
             {               if ((xl<end_points[j].x)||
                                 ((xl==end_points[j].x)&&(yt<end_points[j].yt)))
                     lookup[j]++;
             else lookup[i]++;
             }
     }
// *********************************************************************************
// remove non-line objects
     for (int i=0; i< n_lines; i++)
             if (lookup[i]>n_obj){
                     for (int j=i;j<n_obj-1;j++){
                             lookup[j]=lookup[j+1];
                             end_points[j].x=end_points[j+1].x;
                             end_points[j].yt=end_points[j+1].yt;
                             end_points[j].yb=end_points[j+1].yb;
                     }
                     if (i<n_lines-1) i-=1;
             }
     n_obj=n_lines; // only include lines in future analysis
     if (n_obj < 1) 
             return MATH_SELECTIONERR;


// *********************************************************************************
// Find Minimum and Maximum line end points
     int top_line=end_points[0].yt, bottom_line=end_points[0].yb;
     for (int i=1; i<n_obj; i++){ 
             if (top_line > end_points[i].yt)
                     top_line=end_points[i].yt;
             if (bottom_line < end_points[i].yb)
                     bottom_line=end_points[i].yb;
     }
     PI_DEBUG (DBG_L2, "top " << top_line << " bottom " << bottom_line );

// *********************************************************************************
// Set up matrix for line positions in pixels
     const int n_rows = (int) ceil((bottom_line-top_line)/stepsize)+1;
     const int n_elements = n_rows*n_obj;
     int *line_matrix = new int[n_elements];
     for (int i=0; i<n_elements; i++) line_matrix[i]=-1;

     PI_DEBUG (DBG_L2, "number of rows " << n_rows );



// *********************************************************************************
//Find the Minimum in each line, diplay it on scan and record position in matrix

     for (int i=0; i<n_obj; i++){
             int minposition=end_points[i].x;
             for(int step=0; 
                 (end_points[i].yt+ceil(stepsize*step))<end_points[i].yb;
                 step++){
                     int line=(int) (end_points[i].yt+floor(stepsize*step));    
        
                     double minvalue=10000000;
                     int bound=minposition;

                     for(int col=bound-width; col<bound+width+1; col++) 
                             if(col-xsmoothing>0 && col<Src->mem2d->GetNx()-1+xsmoothing){
                                     double total=0;
                                     for(int line2=(int) (line-floor(stepsize)); (line2<Dest->mem2d->GetNy()) && 
                                                 (line2<(line+floor(2*stepsize)))
                                                 && (line2>0); line2++)
                                             for (int col2=col-xsmoothing;col2<col+xsmoothing+1;col2++)
                                                     total+=Src->mem2d->GetDataPkt(col2, line2);
                                     if (total<minvalue){
                                             minvalue=total;
                                             minposition=col;
                                     }
                             }
             // writing position to matrix
                     int element=(int) ((floor((end_points[i].yt-top_line)/stepsize)+step)*(n_obj)+lookup[i]);
                     line_matrix[element]=minposition;
             // diplaying position on scan
                     for(int line2=line; (line2<Dest->mem2d->GetNy()) && 
                                 (line2<(line+ ceil(stepsize)));line2++) 
                             Dest->mem2d->PutDataPkt (
                                     Src->mem2d->GetDataPkt(minposition,line2)+1000, 
                                     minposition,line2);
//                   std::cout << (end_points[i].yt+floor(stepsize*step))<<" "<<end_points[i].yb<<std::endl;
//                   std::cout  << " minposition for line"  << line  <<" is "<< minposition << std::endl;
             }
     }

// ********************************************************************************
// Start and stop positions of each line in the matrix
     matrix_elements *matrix_end = new matrix_elements[n_obj];
     for (int i=0; i<n_obj; i++){
             matrix_end[lookup[i]].first=(int) floor((end_points[i].yt-top_line)/stepsize);
             for(matrix_end[lookup[i]].last=(int) (floor((end_points[i].yt-top_line)/stepsize)+
                     floor((end_points[i].yb-end_points[i].yt)/stepsize));
                 line_matrix[matrix_end[lookup[i]].last*n_obj+lookup[i]]<0;
                 matrix_end[lookup[i]].last--);
             
     }

// *********************************************************************************
// Check for overlapped lines and change order
     {
             for (int col=0;col<n_obj-1; col++){ 
                     int sum=0;
                     for(int row=matrix_end[col].first; row<matrix_end[col].last+1; row++)
                             if (line_matrix[n_obj*row+col+1]!=-1)
                                     sum +=( line_matrix[n_obj*row+col+1]-line_matrix[n_obj*row+col]);


//                   for(int row=0; row<n_rows; row++)
//                           if ((line_matrix[n_obj*row+col]!=-1) && (line_matrix[n_obj*row+col+1]!=-1))
//                                   sum +=( line_matrix[n_obj*row+col+1]-line_matrix[n_obj*row+col]);
                     if (sum<0){
                             int temp = matrix_end[col].first;
                             matrix_end[col].first=matrix_end[col+1].first;
                             matrix_end[col+1].first=temp;
                             temp = matrix_end[col].last;
                             matrix_end[col].last=matrix_end[col+1].last;
                             matrix_end[col+1].last=temp;

                             for(int row=0; row<n_rows; row++){
                                     temp = line_matrix[n_obj*row+col+1];
                                     line_matrix[n_obj*row+col+1]=line_matrix[n_obj*row+col];
                                     line_matrix[n_obj*row+col]=temp;
                             }
                                                 
                     }
             }
     }       
// std::cout order of lines
     PI_DEBUG (DBG_L2, "lookup ");
     for (int i=0;i<n_obj; i++) std::cout << lookup[i] <<" "<<end_points[i].x<<" ";
     std::cout << std::endl;


// *********************************************************************************
// OutPut data to a choosen file in Angstroms
     /*
     gchar *file_name = main_get_gapp()->file_dialog("asc Export", NULL,
                                          "*.asc", 
                                          "", "asc-Export");

     ofstream file_out;
     file_out.open(file_name);
     file_out.setf(ios::fixed);
     file_out << "Vacancy Line Positions");   
     file_out << "Dimer spacing: " << stepsize << " A");
     file_out << "x pixels spacing: " << dx << " A/pixel");
     file_out << "y pixels spacing: " << dy << " A/pixel");
     for (int i=0; i<n_rows;i++){
             for (int j=0; j<n_obj; j++)
                     if (line_matrix[n_obj*i+j]==-1)
                             file_out << -1. << "\t"; 
                     else
                             file_out << line_matrix[n_obj*i+j]/dx<<"\t";
             file_out << std::endl;
     }
     file_out.close();
     PI_DEBUG (DBG_L2, "file name "<< file_name );
     */
// *********************************************************************************
// OutPut data to a choosen file in Pixels

     gchar *file_name = main_get_gapp()->file_dialog("asc Export", NULL,
                                          "*.asc", 
                                          "", "asc-Export");

     std::ofstream file_out;
     file_out.open(file_name);
     file_out.setf(std::ios::fixed);
     file_out << "Vacancy Line Positions (Pixels)" << std::endl;   
     file_out << "Dimer spacing: " << yaverage << " A" << std::endl;
     file_out << "x pixels spacing: " << dx << " A/pixel" << std::endl;
     file_out << "y pixels spacing: " << dy << " A/pixel" << std::endl;
     file_out << "First and last matrix elements containing data (0=first)" << std::endl;
     for (int i=0; i<n_obj; i++)
             file_out << matrix_end[i].first <<"\t";
     file_out << std::endl;
     for (int i=0; i<n_obj; i++)
             file_out <<matrix_end[i].last <<"\t";
     file_out << std::endl << std::endl;
     for (int i=0; i<n_rows;i++){
             for (int j=0; j<n_obj; j++)
                     file_out << line_matrix[n_obj*i+j]<<"\t";
             file_out << std::endl;
     }
     file_out.close();
     PI_DEBUG (DBG_L2, "file name "<< file_name );

// *********************************************************************************
// OutPut Line Information in pixels to screen
/*
     PI_DEBUG (DBG_L2, "Vacancy Line Positions");   
     PI_DEBUG (DBG_L2, "Dimer spacing: " << stepsize << " A");
     PI_DEBUG (DBG_L2, "x pixels spacing: " << dx << " A/pixel");
     PI_DEBUG (DBG_L2, "y pixels spacing: " << dy << " A/pixel");    
     for (int i=0; i<n_rows;i++){
             for (int j=0; j<n_obj; j++)
                     std::cout << line_matrix[(n_obj*i)+j]<<" ";
             std::cout << std::endl;
     }
*/


// *********************************************************************************
//   Analyze data
//   Line Straightness

// Initialize second layer for histogram of line straightness
     Dest->mem2d->SetLayer (1);
     for(int line=0; line<Dest->mem2d->GetNy(); line++){
             for(int col=0; col<Dest->mem2d->GetNx(); col++){
                     Dest->mem2d->PutDataPkt(0, col, line);
             }
     }
     int Ny = Dest->mem2d->GetNy();
     int Nx = Dest->mem2d->GetNx();
     int center = (int) Ny/2; 
     int n_rows_down = (int)n_dimer_rows+1;
     int x_pixel = 1;
     int y_pixel = (int) stepsize;
     if (hist_scale==0){
             // Determines number of bins to include
             int max_offset = 1;
             for (int row_down=0; row_down < n_rows_down; row_down++)
                     for (int col=0; col<n_obj; col++)
                             for (int row = matrix_end[col].first; 
                                  row < matrix_end[col].last-row_down+1; row++)
                                     if ( abs(line_matrix[n_obj*(row+row_down)+col]-
                                              line_matrix[n_obj*row+col])>max_offset)
                                             max_offset = abs(line_matrix[n_obj*(row+row_down)+col]-
                                                              line_matrix[n_obj*row+col]);      
             x_pixel = (int) floor(Nx/(2.2*max_offset));
             y_pixel = (int) floor(Ny/n_rows_down);
     }
  

     PI_DEBUG (DBG_L2, "x_pixel " << x_pixel );
     PI_DEBUG (DBG_L2, "y_pixel " << y_pixel );
     for (int row_down=0; row_down < n_rows_down; row_down++)
             for (int col=0; col<n_obj; col++)
                     for (int row = matrix_end[col].first;
                          row < matrix_end[col].last-row_down+1; row++)
                             for (int row2 = row_down*y_pixel; 
                                  row2<(row_down+1)*y_pixel; row2++)
                                     for (int col2 = center + x_pixel*(
                                                  line_matrix[n_obj*(row+row_down)+col]-
                                                  line_matrix[n_obj*row+col]);
                                          col2< center + x_pixel*(
                                                  line_matrix[n_obj*(row+row_down)+col]-
                                                  line_matrix[n_obj*row+col]+1); col2++)
                                             if ((col2<Nx) && (col2>0)) 
                                                     Dest->mem2d->data->Zadd(1,col2, row2);
     Dest->mem2d->SetLayer (0);



// ******************************************************************************************************

//  Line Spacing  // will not be performed if bin width is set to 0 in configuration
     if (bin_width>0) 
     {
             // determine the range of values for histogram
             double bin_w=bin_width/dy;
             int min_spacing=1000;
             int max_spacing=0;
             for(int row=0; row<n_rows; row++)
                     for (int col=0;col<n_obj-1; col++) 
                             if ((line_matrix[n_obj*row+col]!=-1) && (line_matrix[n_obj*row+col+1]!=-1)){
                                     if (line_matrix[n_obj*row+col+1]-line_matrix[n_obj*row+col]<min_spacing)
                                             min_spacing=(line_matrix[n_obj*row+col+1]-line_matrix[n_obj*row+col]);
                                     if (line_matrix[n_obj*row+col+1]-line_matrix[n_obj*row+col]>max_spacing)
                                             max_spacing=(line_matrix[n_obj*row+col+1]-line_matrix[n_obj*row+col]);
                             }
             // increase range on each side by 20%
             int range= max_spacing-min_spacing;
             min_spacing-= (int)(0.2*range);
             max_spacing+= (int)(0.2*range);
             
             // set the the number of bins depending on range and bin width given in configuration 
             int n_bins = (int) ceil((max_spacing-min_spacing)/bin_w);
             int *bin_cnts = new int[n_bins];
             for (int i=0; i<n_bins; i++) bin_cnts[i]=0;

             for (int col=0; col< n_obj-1; col++)
                     for (int row=0; row< n_rows; row++)
                             if (line_matrix[n_obj*row+col]!=-1)
                                     for (int nextcol=col+1; nextcol<n_obj; nextcol++)
                                             if (line_matrix[n_obj*row+nextcol]!=-1){
                                                     if((int)((line_matrix[n_obj*row+nextcol]-
                                                        line_matrix[n_obj*row+col]-min_spacing)/bin_w)<n_bins) 
                                                             bin_cnts[(int)((line_matrix[n_obj*row+nextcol]-
                                                              line_matrix[n_obj*row+col]-min_spacing)/bin_w)]+=1;
                                                     nextcol=n_obj;
                                             }
 

             UnitObj *Events = new UnitObj("","","g","#");
             gchar *txt;
             txt= g_strdup_printf ("Vacancy_Line_Spacing_Histogram");

             ProfileControl *pc2 = new ProfileControl (main_get_gapp() -> get_app (),
						       txt, 
                                                       n_bins, 
                                                       Src->data.Xunit, Events, 
                                                       min_spacing*dx, max_spacing*dx);
             for(int i = 0; i < n_bins; i++)
                     pc2->SetPoint (i, bin_cnts[i]);

             pc2->UpdateArea();
     
             for (int i=0;i<n_bins; i++) std::cout << bin_cnts[i] << " ";
             std::cout << std::endl;
             delete [] bin_cnts;
     }

     delete [] end_points;
     delete [] matrix_end;
     delete [] lookup;
     delete [] line_matrix;


#else
  // simple example for 2sourced Mathoperation: Source1 and Source2 are added.
  int line, col;

  if(Src1->data.s.nx != Src2->data.s.nx || Src1->data.s.ny != Src2->data.s.ny)
    return MATH_SELECTIONERR;

  for(line=0; line<Dest->data.s.ny; line++)
    for(col=0; col<Dest->data.s.nx; col++)
      Dest->mem2d->PutDataPkt(
                              Src1->mem2d->GetDataPkt(col, line)
                            + Src2->mem2d->GetDataPkt(col, line),
                              col, line);
#endif

  return MATH_OK;
}


