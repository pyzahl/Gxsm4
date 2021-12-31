/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: nndistribution.C
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
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: OpenCV Re-Center Feature

% PlugInName: opencvrecenter

% PlugInAuthor: Percy Zahl

% PlugInAuthorEmail: zahl@users.sf.net

% PlugInMenuPath: Math/Statistics/Opencvrecenter

% PlugInDescription
The OpenCV Recenter Feature identifies the most likely position of a
given template feature (hold in a Channel set to Mode-X) in the active
channel and sets the Scan-Offset to the resulting position.

% PlugInUsage
Call it from Gxsm Math/Statistics menu.

% OptPlugInSources
The active channel is used as data source. Channel set to X-Mode is used as template.

% OptPlugInObjects

% OptPlugInDest
The computation result of matching threasholds is placed into an new math channel for reference.

%% OptPlugInConfig

%% OptPlugInFiles

%% OptPlugInRefs

%% OptPlugInKnownBugs

%% OptPlugInNotes

% OptPlugInHints

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"


#include <math.h>
#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"
#include "core-source/action_id.h"
#include "core-source/app_profile.h"
#include "core-source/view.h"
#include "core-source/app_view.h"

using namespace cv;



static void opencvrecenter_init( void );
static void opencvrecenter_about( void );
static void opencvrecenter_configure( void );
static void opencvrecenter_cleanup( void );
static gboolean opencvrecenter_run( Scan *Src, Scan *ScanRef, Scan *Dest);

GxsmPlugin opencvrecenter_pi = {
  NULL,
  NULL,
  0,
  NULL,
  "Opencvrecenter-M2S-Stat",
  NULL,
  NULL,
  "Percy Zahl",
  "math-statistics-section",
  N_("feature recenter"),
  N_("find and recenter feature"),
  "no more info",
  NULL,
  NULL,
  opencvrecenter_init,
  NULL,
  opencvrecenter_about,
  opencvrecenter_configure,
  NULL,
  NULL,
  NULL,
  opencvrecenter_cleanup
};

GxsmMathTwoSrcPlugin opencvrecenter_m2s_pi = {
  opencvrecenter_run
};

static const char *about_text = N_("Gxsm OpenCV Recenter Plugin\n\n"
                                   "recenter and mark");
static UnitObj *Events = NULL;

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  opencvrecenter_pi.description = g_strdup_printf(N_("Gxsm MathTwoArg opencvrecenter plugin %s"), VERSION);
  return &opencvrecenter_pi; 
}

GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_no_same_size_check_plugin_info( void ) { 
  return &opencvrecenter_m2s_pi; 
}

static void opencvrecenter_init(void)
{
  PI_DEBUG (DBG_L2, "Opencvrecenter Plugin Init");
}

static void opencvrecenter_about(void)
{
  const gchar *authors[] = { opencvrecenter_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  opencvrecenter_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

static void opencvrecenter_configure(void)
{
	if(opencvrecenter_pi.app)
		opencvrecenter_pi.app->message("Opencvrecenter Plugin Configuration");
}

static void opencvrecenter_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Opencvrecenter Plugin Cleanup");
	if (Events){
		delete Events;
		Events = NULL;
	}
}


void setup_opencv_recenter (const gchar *title, Scan *src, double &threshold, int &object_radius, int &method, int &max_markers, int &i_mg){
	UnitObj *Pixel = new UnitObj("Pix","Pix");
	UnitObj *Unity = new UnitObj(" "," ");

	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_(title),
							 NULL, 
							 (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                         _("_OK"), GTK_RESPONSE_ACCEPT,
							 NULL); 
	BuildParam bp;
        gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);
        bp.grid_add_label ("Recentering Method id's, see for details\n"
			   "http://docs.opencv.org/modules/imgproc/doc/object_detection.html\n"
			   "CV_TM_SQDIFF        =0,\n"
			   "CV_TM_SQDIFF_NORMED =1,\n"
			   "CV_TM_CCORR         =2,\n"
			   "CV_TM_CCORR_NORMED  =3,\n"
			   "CV_TM_CCOEFF        =4,\n"
			   "CV_TM_CCOEFF_NORMED =5 ");
	bp.new_line ();

	bp.grid_add_ec ("Recenter Threshold (0..1)", Unity, &threshold, 0., 1., ".3f"); bp.new_line ();
	bp.grid_add_ec ("Individual Object Radius", Pixel, &object_radius, 1., src->mem2d->GetNx ()/10., ".0f"); bp.new_line ();
	bp.grid_add_ec ("Method", Unity, &method, 0, 5, ".0f"); bp.new_line ();
	bp.grid_add_ec ("limit for max # markers", Unity, &max_markers, 0, 50000, ".0f"); bp.new_line ();
	bp.grid_add_ec ("marker group", Unity, &i_mg, 0, 5, ".0f"); bp.new_line ();

	gtk_widget_show_all (dialog);
	gtk_dialog_run (GTK_DIALOG(dialog));

	gtk_widget_destroy (dialog);

	delete Pixel;
	delete Unity;	
}



static gboolean opencvrecenter_run(Scan *Src, Scan *SrcRef, Scan *Dest)
{
	double recenter_threshold = 0.9; // suggestion recenter quality
	int object_radius = 4; // no more than one mark withing this radius
	int max_markers = 5000; // limit to this number -- in case things go weird
	int method = 5; //CV_TM_CCOEFF_NORMED; // recentering algorithm, see http://docs.opencv.org/modules/imgproc/doc/object_detection.html
        // ????  /usr/include/opencv4/opencv2/imgproc/types_c.h:    CV_TM_CCOEFF_NORMED =5

	int i_marker_group = 0;
	PI_DEBUG (DBG_L2, "OpenCV recenter");

	const gchar *marker_group[] = { 
		"*Marker:red", "*Marker:green", "*Marker:blue", "*Marker:yellow", "*Marker:cyan", "*Marker:magenta",  
		NULL };

	double x1=-1,y1,x2,y2;
	int n_obj = SrcRef->number_of_object ();

        int interactive = 0;
        if (interactive)
                setup_opencv_recenter ("Setup Feature Recentering", Src, recenter_threshold, object_radius, method, max_markers, i_marker_group);

	// convert Scan->Src data into OpenCV Mat => img1
	double high, low;
	Src->mem2d->HiLo(&high, &low, FALSE, NULL, NULL, 1);
	std::cout << "img1: mat src:" << std::endl;
	Mat img1 = Mat (Src->mem2d->GetNy (), Src->mem2d->GetNx (), CV_32F);
	for (int i=0; i<Src->mem2d->GetNy (); ++i){
		for (int j=0; j<Src->mem2d->GetNx (); ++j){
			double pv = Src->mem2d->data->Z(j,i);
			img1.at<float>(i,j) = (float)((pv-low)/(high-low));
		}
	}

	// convert Scan->SrcRef data into OpenCV Mat => img2
	SrcRef->mem2d->HiLo(&high, &low, FALSE, NULL, NULL, 1);
	std::cout << "img2: mat ref:" << std::endl;
	Mat img2 = Mat (SrcRef->mem2d->GetNy (), SrcRef->mem2d->GetNx (), CV_32F);
	for (int i=0; i<SrcRef->mem2d->GetNy (); ++i){
		for (int j=0; j<SrcRef->mem2d->GetNx (); ++j){
			double pv = SrcRef->mem2d->data->Z(j,i);
			img2.at<float>(i,j) = (float)((pv-low)/(high-low));
		}
	}

        // sanity check
	if(img1.empty() || img2.empty()){
		return MATH_LIB_ERR;
	}

	std::cout << "running match..." << std::endl;

	// do recentering via OpenCV library and some data post processing
	Mat img_result, img_recenter, img_tmp, img_t8, img_r8;
	matchTemplate (img1, img2, img_recenter, method);
	abs (img_recenter);
	pow (img_recenter, 3., img_result);
	threshold(img_result, img_tmp, recenter_threshold, 0., THRESH_TOZERO);

	img_tmp.convertTo(img_result, CV_32F, 1., 0.);

        // store matching threaholds to result channel as reference
	Dest->mem2d->Resize(img_result.cols, img_result.rows, ZD_FLOAT);
	for (int i=0; i<Dest->mem2d->GetNy () && i < img_result.rows; ++i){
		for (int j=0; j<Dest->mem2d->GetNx () && j < img_result.cols; ++j){
			double pv = img_result.at<float>(i,j);
			Dest->mem2d->PutDataPkt(pv, j,i);
		}
	}

	// now mark features in Src Scan using Marker Objects, avoid duplicated in a radius "r=4 pix (object_radius)"
        // and set scan offset to best match location (center of ref scan)
	gchar *tmp;
	int count=0;
	double peak=0.;
	int ip, jp;
	ip=jp=0;
	for (int i=0; i<img_result.rows; ++i){
		for (int j=0; j<img_result.cols; ++j){
			if (img_result.at<float>(i,j) < recenter_threshold)
				continue;
			if (img_result.at<float>(i,j) > peak){
			        peak = img_result.at<float>(i,j);
				ip=i, jp=j;
			}
			
		}
	}

        // center to ref box (not origin)
        jp += SrcRef->mem2d->GetNx ()/2;
        ip += SrcRef->mem2d->GetNy ()/2;
                
        if (Src->view && peak > 0.){
                if (Src->view->Get_ViewControl ()){
                        VObject *vo;
                        double xy[2];
                        gfloat c[4] = { 1.,0.,0.,1.};
                        int spc[2][2] = {{0,0},{0,0}};
                        int sp00[2] = {1,1};
                        int s = 0;
                        Src->Pixel2World (jp, ip, xy[0], xy[1]);
                        std::cout << "Center best match found at: "
                                  << xy[0] << ", " << xy[1]
                                  << " [" << jp << "," << ip << "] ==> " << peak << std::endl;
                        gchar *lab = g_strdup_printf ("Center");
                        (Src->view->Get_ViewControl ())->AddObject (vo = new VObPoint ((Src->view->Get_ViewControl ())->canvas, xy, FALSE, VOBJ_COORD_ABSOLUT, lab, 1.));
                        vo->set_obj_name (marker_group[i_marker_group]);
                        vo->set_custom_label_font ("Sans Bold 12");
                        vo->set_custom_label_color (c);
                        vo->set_on_spacetime  (sp00[0] ? FALSE:TRUE, spc[0]);
                        vo->set_off_spacetime (sp00[1] ? FALSE:TRUE, spc[1]);
                        vo->show_label (s);
                        vo->remake_node_markers ();

                        gapp->xsm->data.s.x0 = xy[0];
                        gapp->xsm->data.s.y0 = xy[1];
                        gapp->spm_offset_check (NULL, gapp);
                }
	}
	
	return MATH_OK;
}
