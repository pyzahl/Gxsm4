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
% PlugInDocuCaption: Opencvmatch

% PlugInName: opencvmatch

% PlugInAuthor: Percy Zahl

% PlugInAuthorEmail: zahl@users.sf.net

% PlugInMenuPath: Math/Statistics/Opencvmatch

% PlugInDescription
The OpenCV Match...

% PlugInUsage
Call it from Gxsm Math/Statistics menu. It will prompt for the number
of bins and provides a estimated number as default. Also the current
min/max Z-value limits and range is shown for informative purpose.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInObjects
A optional rectangle can not be used. Use crop before!

% OptPlugInDest
The computation result is placed into an new profile view.

%% OptPlugInConfig
%The number of bins can be set.

%% OptPlugInFiles
%Needs/creates no files.

%% OptPlugInRefs
%nope.

%% OptPlugInKnownBugs
%No known.

%% OptPlugInNotes
%Hmm, no notes\dots

% OptPlugInHints
Find out what happenes with more or less bins!

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/types_c.h"

#include <math.h>
#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "action_id.h"
#include "app_profile.h"
#include "view.h"
#include "app_view.h"

using namespace cv;



static void opencvmatch_init( void );
static void opencvmatch_about( void );
static void opencvmatch_configure( void );
static void opencvmatch_cleanup( void );
static gboolean opencvmatch_run( Scan *Src, Scan *Dest);

GxsmPlugin opencvmatch_pi = {
  NULL,
  NULL,
  0,
  NULL,
  "Opencvmatch-M1S-Stat",
  NULL,
  NULL,
  "Percy Zahl",
  "math-statistics-section",
  N_("feature match"),
  N_("find and match feature"),
  "no more info",
  NULL,
  NULL,
  opencvmatch_init,
  NULL,
  opencvmatch_about,
  opencvmatch_configure,
  NULL,
  NULL,
  NULL,
  opencvmatch_cleanup
};

GxsmMathOneSrcPlugin opencvmatch_m1s_pi = {
  opencvmatch_run
};

static const char *about_text = N_("Gxsm OpenCV Match Plugin\n\n"
                                   "match and mark");
static UnitObj *Events = NULL;

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  opencvmatch_pi.description = g_strdup_printf(N_("Gxsm MathOneArg opencvmatch plugin %s"), VERSION);
  return &opencvmatch_pi; 
}

GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) { 
  return &opencvmatch_m1s_pi; 
}

static void opencvmatch_init(void)
{
  PI_DEBUG (DBG_L2, "Opencvmatch Plugin Init");
}

static void opencvmatch_about(void)
{
  const gchar *authors[] = { opencvmatch_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  opencvmatch_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

static void opencvmatch_configure(void)
{
	if(opencvmatch_pi.app)
		opencvmatch_pi.app->message("Opencvmatch Plugin Configuration");
}

static void opencvmatch_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Opencvmatch Plugin Cleanup");
	if (Events){
		delete Events;
		Events = NULL;
	}
}


void setup_opencv_match (const gchar *title, Scan *src, double &threshold, int &object_radius, int &method, int &max_markers, int &i_mg){
	UnitObj *Pixel = new UnitObj("Pix","Pix");
	UnitObj *Unity = new UnitObj(" "," ");

	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_(title),
							 NULL, 
							 (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                         _("_OK"), GTK_RESPONSE_ACCEPT,
							 NULL); 
	BuildParam bp;
        gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);
        bp.grid_add_label ("Matching Method id's, see for details\n"
			   "http://docs.opencv.org/modules/imgproc/doc/object_detection.html\n"
			   "CV_TM_SQDIFF        =0,\n"
			   "CV_TM_SQDIFF_NORMED =1,\n"
			   "CV_TM_CCORR         =2,\n"
			   "CV_TM_CCORR_NORMED  =3,\n"
			   "CV_TM_CCOEFF        =4,\n"
			   "CV_TM_CCOEFF_NORMED =5 ");
	bp.new_line ();

	bp.grid_add_ec ("Match Threshold (0..1)", Unity, &threshold, 0., 1., ".3f"); bp.new_line ();
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



static gboolean opencvmatch_run(Scan *Src, Scan *Dest)
{
	double match_threshold = 0.75; // suggestion match quality
	int object_radius = 4; // no more than one mark withing this radius
	int max_markers = 5000; // limit to this number -- in case things go weird
	int method = CV_TM_CCOEFF_NORMED; // matching algorithm, see http://docs.opencv.org/modules/imgproc/doc/object_detection.html
	int i_marker_group = 0;
	PI_DEBUG (DBG_L2, "OpenCV match");

	const gchar *marker_group[] = { 
		"*Marker:red", "*Marker:green", "*Marker:blue", "*Marker:yellow", "*Marker:cyan", "*Marker:magenta",  
		NULL };

	double x1=-1,y1,x2,y2;
	int n_obj = Src->number_of_object ();

	setup_opencv_match ("Setup Feature Matching", Src, match_threshold, object_radius, method, max_markers, i_marker_group);

	// find reference feature (rectangle object)
	while (n_obj--){
		scan_object_data *obj;
		obj = Src->get_object_data (n_obj);
		std::cout << n_obj << ": " << obj->get_name () << std::endl;
		if (strncmp (obj->get_name (), "Rectangle", 8))
			continue; // ignore, look for rectangle
		
		if (obj->get_num_points () != 2) 
			continue; // sth. is weired!
		
		// get real world coordinates of point
		obj->get_xy_i_pixel (0, x1, y1);
		obj->get_xy_i_pixel (1, x2, y2);
		break;
	}

	std::cout << "Recr["<<x1<<","<<y1<<","<<x2<<","<<y2<<"] in ["<<Src->mem2d->GetNx ()<<", "<<Src->mem2d->GetNy () <<"]"<< std::endl;
	
	// coordinates validity check, rect points must be RHS/pos.
	if (x1 < 0 || x1 >= x2 || x2 < 0 || x1 >= Src->mem2d->GetNx () || x2 >= Src->mem2d->GetNx ()
	    || y1 < 0 || y1 >= y2 || y2 < 0 || y1 >= Src->mem2d->GetNy () || y2 >= Src->mem2d->GetNy ())
	    return MATH_SELECTIONERR;
	      
	// convert Scan->Src data into OpenCV Mat => img1
	double high, low;
	Src->mem2d->HiLo(&high, &low, FALSE, NULL, NULL, 1);
	std::cout << "mat:" << std::endl;
	Mat img1 = Mat (Src->mem2d->GetNy (), Src->mem2d->GetNx (), CV_32F);
	for (int i=0; i<Src->mem2d->GetNy (); ++i){
		for (int j=0; j<Src->mem2d->GetNx (); ++j){
			double pv = Src->mem2d->data->Z(j,i);
			//std::cout <<j<<","<<i<<" = "<< pv << ";";
			img1.at<float>(i,j) = (float)((pv-low)/(high-low));
		}
		//std::cout << std::endl;
	}
	//	imshow("img1", img1);

	// extract reference feature => img2
	double dx = x2-x1;
	double dy = y2-y1;
	Mat img2 = img1(Rect(x1,y1,dx,dy)).clone();
	//	imshow("img2", img2);

	if(img1.empty() || img2.empty()){
		return MATH_LIB_ERR;
	}

	// do matching via OpenCV library and some data post processing
	Mat img_result, img_match, img_tmp, img_t8, img_r8;
	matchTemplate (img1, img2, img_match, method);
	abs (img_match);
	pow (img_match, 3., img_result);
	threshold(img_result, img_tmp, match_threshold, 0., THRESH_TOZERO);

	//	img_tmp.convertTo(img_t8, CV_8UC1, 255., 0.);
	//	img_tmp.convertTo(img_r8, CV_8UC1, 255., 0.);
	//	imshow("t8", img_t8);
	//	adaptiveThreshold(img_t8, img_r8, 1., ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 3, 1.);
	//	img_tmp.convertTo(img_result, CV_32F, 1./255., 0.);
	//      adaptiveThreshold(InputArray src, OutputArray dst, double maxValue, int adaptiveMethod, int thresholdType, int blockSize, double C)
		
	img_tmp.convertTo(img_result, CV_32F, 1., 0.);

	//	img_result = img_tmp;
	//	normalize(img_result,img_tmp);
	//	imshow("matches", img_tmp);
	//	img_result = img_tmp;

	// put matching probability into Dest Scan -- just for reference
	//	std::cout << "Result [" << img_result.rows << "," << img_result.cols << "] :" << std::endl;
	Dest->mem2d->Resize(img_result.cols, img_result.rows, ZD_FLOAT);
	for (int i=0; i<Dest->mem2d->GetNy () && i < img_result.rows; ++i){
		for (int j=0; j<Dest->mem2d->GetNx () && j < img_result.cols; ++j){
			double pv = img_result.at<float>(i,j);
	//			std::cout << pv << ":";
			Dest->mem2d->PutDataPkt(pv, j,i);
		}
	//		std::cout << std::endl;
	}

	// now mark features in Src Scan using Marker Objects, avoid duplicated in a radius "r=4 pix (object_radius)"
	gchar *tmp;
	int count=0;
	for (int i=0; i<img_result.rows; ++i){
		for (int j=0; j<img_result.cols; ++j){
			if (count > max_markers)
				continue;
			if (img_result.at<float>(i,j) < match_threshold)
				continue;

			// mark done
			
			int flag=0;
			int r=object_radius;
			double m=img_result.at<float>(i,j);
			int km=i;
			int lm=j;
			for (int k=i-r; !flag && k<=i+r && k<img_result.rows; ++k){
				if (k<0) continue;
				for (int l=j-r; l<=j+r && j<img_result.cols; ++l){
					if (l<0) continue;
					if (img_result.at<float>(k,l) == 2.){
						flag=1; break;
					}
					if (img_result.at<float>(k,l) > m){
						km=k; lm=l;
					}
				}
			}

			if (flag) continue;
			
			img_result.at<float>(i,j) = 2.; // Flag mark this spot as marked now.

			if (Src->view){
				if (Src->view->Get_ViewControl ()){
					VObject *vo;
					double xy[2];
					gfloat c[4] = { 1.,0.,0.,1.};
					int spc[2][2] = {{0,0},{0,0}};
					int sp00[2] = {1,1};
					int s = 0;
					Src->Pixel2World (lm+dx/2., km+dy/2., xy[0], xy[1]);
					//					Src->Pixel2World (j, i, xy[0], xy[1]);
					//					std::cout << "Add Marker at " << i << "," << j << std::endl;
					gchar *lab = g_strdup_printf ("M%05d",++count);
					(Src->view->Get_ViewControl ())->AddObject (vo = new VObPoint ((Src->view->Get_ViewControl ())->canvas, xy, FALSE, VOBJ_COORD_ABSOLUT, lab, 1.));
					vo->set_obj_name (marker_group[i_marker_group]);
					vo->set_custom_label_font ("Sans Bold 12");
					vo->set_custom_label_color (c);
					vo->set_on_spacetime  (sp00[0] ? FALSE:TRUE, spc[0]);
					vo->set_off_spacetime (sp00[1] ? FALSE:TRUE, spc[1]);
					vo->show_label (s);
					vo->remake_node_markers ();
				}
			}
		}
	}

	
#if 0
	// detecting keypoints
	SurfFeatureDetector detector(400);
	vector<KeyPoint> keypoints1, keypoints2;
	detector.detect(img1, keypoints1);
	detector.detect(img2, keypoints2);

	// computing descriptors
	SurfDescriptorExtractor extractor;
	Mat descriptors1, descriptors2;
	extractor.compute(img1, keypoints1, descriptors1);
	extractor.compute(img2, keypoints2, descriptors2);
	
	// matching descriptors
	BruteForceMatcher<L2<float> > matcher;
	vector<DMatch> matches;
	matcher.match(descriptors1, descriptors2, matches);
	
	// drawing the results
	namedWindow("matches", 1);
	Mat img_matches;
	drawMatches(img1, keypoints1, img2, keypoints2, matches, img_matches);
	imshow("matches", img_matches);

#endif	

	return MATH_OK;
}
