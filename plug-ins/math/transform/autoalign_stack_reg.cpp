/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * 
 * Gxsm Plugin Name: autoalign.C
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

/* The math algorithm is based on the following work, prior implemented for ImageJ: */

/*====================================================================	
| Version: February 28, 2005
\===================================================================*/

/*====================================================================
| EPFL/STI/IOA/LIB
| Philippe Thevenaz
| Bldg. BM-Ecublens 4.137
| Station 17
| CH-1015 Lausanne VD
| Switzerland
|
| phone (CET): +41(21)693.51.61
| fax: +41(21)693.37.01
| RFC-822: philippe.thevenaz@epfl.ch
| X-400: /C=ch/A=400net/P=switch/O=epfl/S=thevenaz/G=philippe/
| URL: http://bigwww.epfl.ch/
\===================================================================*/

/*====================================================================
| This work is based on the following paper:
|
| P. Thevenaz, U.E. Ruttimann, M. Unser
| A Pyramid Approach to Subpixel Registration Based on Intensity
| IEEE Transactions on Image Processing
| vol. 7, no. 1, pp. 27-41, January 1998.
|
| This paper is available on-line at
| http://bigwww.epfl.ch/publications/thevenaz9801.html
|
| Other relevant on-line publications are available at
| http://bigwww.epfl.ch/publications/
\===================================================================*/

/*====================================================================
| Additional help available at http://bigwww.epfl.ch/thevenaz/turboreg/
|
| You'll be free to use this software for research purposes, but you
| should not redistribute it without our consent. In addition, we expect
| you to include a citation or acknowledgment whenever you present or
| publish results that are based on it.
\===================================================================*/


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

#include "autoalign_stack_reg.h"
#include "autoalign_turbo_reg.h"

#define DEBUG_ON 1
#if DEBUG_ON
# define DEBUG_COUT(X) std::cout << X
#else
# define DEBUG_COUT(X)
#endif

typedef struct{
	StackReg* this_stack_reg;
	Scan* Src;
	Scan* Dest;
	int time_target;
	int n_times;
	int time_direction;
	gboolean do_layers;
	int* stop_flg;
	int layer_target;
	Matrix_d* globalTransform;
	Matrix_d* anchorPoints;
	Matrix_i* register_region;
	int job;
	int progress;
	int progress_v;
} Align_Job_Env;

typedef struct{
	StackReg* this_stack_reg;
	Mem2d* source;
	Mem2d* destination;
	int layer_target;
	int n_values;
	int layer_direction;
	int* stop_flg;
	Matrix_d* globalTransform;
	Matrix_d* anchorPoints;
	Matrix_i* register_region;
	int job;
	int* progress;
} Align_Job_Env_rl;

gpointer stack_reg_time_thread (void *env);
gpointer stack_reg_layer_thread (void *env);

gpointer stack_reg_time_thread (void *env){
	Align_Job_Env* job = (Align_Job_Env*)env;

	Mem2d* tmp_destination = new Mem2d (job->Dest->mem2d, M2D_COPY);
	int stop_flg = *(job->stop_flg);
	job->this_stack_reg->reset_shift_vectors (job->job);
	if (job->do_layers) job->this_stack_reg->reset_shift_vectors (job->job+2);
	for (int s = job->time_target + job->time_direction; 0 <= s && s < job->n_times && !stop_flg; s+=job->time_direction) {

		DEBUG_COUT( "*** Thread ::stack_reg_time_thread["<<job->job<<"] -- s=" << s << std::endl);

		Mem2d* source = job->Src->mem2d_time_element (s); 
		double real_time = source->get_frame_time ();

		tmp_destination->remove_layer_information ();
		tmp_destination->copy_layer_information (source);
		tmp_destination->data->CopyLookups (source->data);

		DEBUG_COUT( "*** Thread ::stack_reg_time_thread["<<job->job<<"] Start registering -- t=" << real_time << std::endl);
		job->this_stack_reg->registerSlice (source, job->layer_target, tmp_destination, job->layer_target, tmp_destination, job->layer_target,
						    *(job->globalTransform), *(job->anchorPoints), *(job->register_region), job->job, job->do_layers ? false:true);

		if (job->do_layers){
			job->this_stack_reg->reset_shift_vectors (job->job+2);
			job->progress_v = 0;
			job->this_stack_reg->registerLayers (source, job->layer_target, tmp_destination, job->layer_target, 
							     *(job->globalTransform), *(job->anchorPoints), *(job->register_region), 
							     true, job->job, &(job->progress_v));
		}

		// now app/prepend new time frame to Destination Scan
		if (job->time_direction > 0)
			job->Dest->append_current_to_time_elements (s, real_time, tmp_destination);
		else
			job->Dest->prepend_current_to_time_elements (s, real_time, tmp_destination);

		job->progress++;
		stop_flg = *(job->stop_flg);
	}
	delete tmp_destination;

	job->job = -1; // done indicator
	return NULL;
}

gpointer stack_reg_layer_thread (void *env){
	Align_Job_Env_rl* job = (Align_Job_Env_rl*)env;

	int stop_flg = *(job->stop_flg);
	job->this_stack_reg->reset_shift_vectors (job->job);

	for (int s = job->layer_target + job->layer_direction; 0 <= s && s < job->n_values && !stop_flg; s+=job->layer_direction) {
		DEBUG_COUT( "*** layer index: " << s << " target: " << (s+1)  << std::endl);
		
		job->this_stack_reg->registerSlice (job->source, s, job->destination, s-job->layer_direction, job->destination, s, 
						    *(job->globalTransform), *(job->anchorPoints), *(job->register_region), job->job);
		(*(job->progress))++;
		stop_flg = *(job->stop_flg);
	}

	job->job = -1; // done indicator
	return NULL;
}



static void
on_dialog_response (GtkDialog *dialog,
		    int        response,
		    gpointer   user_data)
{
        *((int *) user_data) = response;
}


// configure-Function
REGISTER_MODE StackReg::ask_for_transformation_type (TARGET_MODE &target_mode, int &time_target_initial, int &value_target_initial)
{
	int transformation = -1;

	if(autoalign_pi.app){
		XsmRescourceManager xrm("autoalign_pi");
		GtkWidget *tr_sel = NULL;
		GtkWidget *tm_sel = NULL;

		xrm.Get ("transformation", &transformation, "0");
		xrm.Get ("target_mode", (int*)&target_mode, "0");

		GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("Auto Align"),
								 NULL, 
								 (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
								 _("_OK"), GTK_RESPONSE_ACCEPT,
								 _("_CANCEL"), GTK_RESPONSE_CANCEL,
								 NULL); 
        
		BuildParam bp;
		gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);

		bp.grid_add_label ("Auto Align Mode:");
		bp.new_line ();

		tr_sel = gtk_combo_box_text_new ();
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (tr_sel),"tran", "Translation");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (tr_sel),"rigidbody", "Rigid Body");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (tr_sel),"scaledrot", "Scaled Rotation");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (tr_sel),"affine", "Affine");
		gtk_combo_box_set_active (GTK_COMBO_BOX (tr_sel), transformation);
		bp.grid_add_widget (tr_sel);
		bp.new_line ();
		
		tm_sel = gtk_combo_box_text_new ();
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (tm_sel),"t-time", "Target Time");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (tm_sel),"t-layer", "Target Layer");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (tm_sel),"t-time-layer", "Target Time,Layer");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (tm_sel),"t-test", "Target Test");
		gtk_combo_box_set_active (GTK_COMBO_BOX (tm_sel), target_mode);
		bp.grid_add_widget (tr_sel);
		bp.new_line ();

		bp.grid_add_ec ("Max Shift", autoalign_pi.app->xsm->Unity, &max_shift_allowed, 0., 100., ".1f");
		bp.new_line ();
		bp.grid_add_ec ("Debug Lvl", autoalign_pi.app->xsm->Unity, &debug_lvl, -100., 100., ".1f");
		bp.new_line ();

		GtkWidget *button = bp.grid_add_button ("About and Credits");
		bp.new_line ();

		g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (StackReg::about_callback), NULL);

		gtk_widget_show (dialog);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

		int response = GTK_RESPONSE_NONE;
		g_signal_connect (dialog, "response", G_CALLBACK (on_dialog_response), &response);

		// wait here on response
		while (response == GTK_RESPONSE_NONE)
                        while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

		transformation = gtk_combo_box_get_active (GTK_COMBO_BOX (tr_sel));
		target_mode    = (TARGET_MODE)gtk_combo_box_get_active (GTK_COMBO_BOX (tm_sel));

                gtk_window_destroy (GTK_WINDOW (dialog));

                
		xrm.Put ("transformation", transformation);
		xrm.Put ("target_mode", target_mode);

		switch (transformation){
		case 0: return REG_TRANSLATION;
		case 1: return REG_RIGID_BODY;
		case 2: return REG_SCALED_ROTATION;
		case 3: return REG_AFFINE;
//		case 4: return REG_BILINEAR; // not yet impl. here
		default: return REG_INVALID;
		}
	}
	
	return  REG_INVALID;
}



void  StackReg::run (Scan *Src, Scan *Dest) {
	TARGET_MODE target_mode=TARGET_TIME;
	int tti=0;
	int vti=0;

	transformation = ask_for_transformation_type (target_mode, tti, vti);

	if (debug_lvl){
		DEBUG_COUT( "*** DEBUG LEVEL .. " << debug_lvl << std::endl);
		Dest->mem2d->Resize (Src->mem2d->GetNx (), Src->mem2d->GetNy (), Src->mem2d->GetNv () * 2, ZD_IDENT);
	}

	DEBUG_COUT( "*** transformation mode .. " << transformation << std::endl);
	DEBUG_COUT( "*** target mode .......... " << target_mode << std::endl);
	DEBUG_COUT( "*** tti .................. " << tti << std::endl);
	DEBUG_COUT( "*** vti .................. " << vti << std::endl);


	if (transformation == REG_INVALID) return;

	Matrix_i register_region(2, Vector_i(2,0));
	int width  = Src->mem2d->GetNx ();
	int height = Src->mem2d->GetNy ();

	double x_0 = 0.;
	double y_0 = 0.;

	register_region[1][0] = width-1;;
	register_region[1][1] = height-1;;

	int n_obj = (int) Src->number_of_object ();
	while (n_obj--){
		scan_object_data *obj_data = Src->get_object_data (n_obj);

		if (strncmp (obj_data->get_name (), "Rectangle", 9) )
			continue; // only rectangels are used!

		if (obj_data->get_num_points () != 2) 
			continue; // sth. is weired!

		// get real world coordinates of rectangle
		double x1,y1,x2,y2;
		double x=0.;
		double y=0.;
		obj_data -> get_xy_i (0, x, y);
		Src->World2Pixel (x,y, x1,y1);
		obj_data -> get_xy_i (1, x, y);
		Src->World2Pixel (x,y, x2,y2);

		x_0 = x1 < x2 ? x1:x2;
		y_0 = y1 < y2 ? y1:y2;

		width  = (int) fabs (x2-x1);
		height = (int) fabs (y2-y1);

		register_region[0][0] = (int)x_0;;
		register_region[0][1] = (int)y_0;;
		register_region[1][0] = (int)x_0 + width-1;;
		register_region[1][1] = (int)y_0 + height-1;;

		break;
	}



	Matrix_d globalTransform (3, Vector_d (3, 0.0));
	for (int i=0; i<3; ++i)
		globalTransform[i][i] = 1.;

	Matrix_d *anchorPoints = NULL;
	switch (transformation) {
		case REG_TRANSLATION: {
			anchorPoints = new Matrix_d (1, Vector_d (3, 0.0));
			(*anchorPoints) [0][0] = x_0 + (double)(width / 2);
			(*anchorPoints) [0][1] = y_0 + (double)(height / 2);
			(*anchorPoints) [0][2] = 1.0;
			break;
		}
		case REG_RIGID_BODY: {
			anchorPoints = new Matrix_d (3, Vector_d (3, 0.0));
			(*anchorPoints) [0][0] = x_0 + (double)(width / 2);
			(*anchorPoints) [0][1] = y_0 + (double)(height / 2);
			(*anchorPoints) [0][2] = 1.0;
			(*anchorPoints) [1][0] = x_0 + (double)(width / 2);
			(*anchorPoints) [1][1] = y_0 + (double)(height / 4);
			(*anchorPoints) [1][2] = 1.0;
			(*anchorPoints) [2][0] = x_0 + (double)(width / 2);
			(*anchorPoints) [2][1] = y_0 + (double)((3 * height) / 4);
			(*anchorPoints) [2][2] = 1.0;
			break;
		}
		case REG_SCALED_ROTATION: {
			anchorPoints = new Matrix_d (2, Vector_d (3, 0.0));
			(*anchorPoints) [0][0] = x_0 + (double)(width / 4);
			(*anchorPoints) [0][1] = y_0 + (double)(height / 2);
			(*anchorPoints) [0][2] = 1.0;
			(*anchorPoints) [1][0] = x_0 + (double)((3 * width) / 4);
			(*anchorPoints) [1][1] = y_0 + (double)(height / 2);
			(*anchorPoints) [1][2] = 1.0;
			break;
		}
		case REG_AFFINE: {
			anchorPoints = new Matrix_d (3, Vector_d (3, 0.0));
			(*anchorPoints) [0][0] = x_0 + (double)(width / 2);
			(*anchorPoints) [0][1] = y_0 + (double)(height / 4);
			(*anchorPoints) [0][2] = 1.0;
			(*anchorPoints) [1][0] = x_0 + (double)(width / 4);
			(*anchorPoints) [1][1] = y_0 + (double)((3 * height) / 4);
			(*anchorPoints) [1][2] = 1.0;
			(*anchorPoints) [2][0] = x_0 + (double)((3 * width) / 4);
			(*anchorPoints) [2][1] = y_0 + (double)((3 * height) / 4);
			(*anchorPoints) [2][2] = 1.0;
			break;
		}
		default: {
			std::cerr << "Unexpected transformation" << std::endl;
			return;
		}
	}

	stop_flg = false;
	main_get_gapp()->progress_info_new ("Auto aligning", 3, G_CALLBACK (StackReg::cancel_callback), this);
	main_get_gapp()->progress_info_set_bar_fraction (0., 1);
	main_get_gapp()->progress_info_set_bar_fraction (0., 2);
	main_get_gapp()->progress_info_set_bar_fraction (0., 3);
	main_get_gapp()->progress_info_set_bar_text ("Time", 1);
	main_get_gapp()->progress_info_set_bar_text ("Value-1", 2);
	main_get_gapp()->progress_info_set_bar_text ("Value-2", 3);

	switch (target_mode){
	case TARGET_TIME: {
		// run registration process in time:   
		// 0 ... time_target-1, time_target (ident), tagetSlice+1 ... N_times-1

		int layer_target  = Src->mem2d->GetLayer ();
		int time_target = Src->mem2d->get_t_index ();
		int n_times = Src->number_of_time_elements ();
	
		DEBUG_COUT( "*** time_target .......: " << time_target << std::endl);

		main_get_gapp()->progress_info_set_bar_fraction ((gdouble)time_target/(gdouble)n_times, 1);
		double real_time = Src->retrieve_time_element (time_target);	

		DEBUG_COUT( "*** master time_target element index= : " << time_target << std::endl);

		for (int v=0; v < Src->mem2d->GetNv (); ++v){		
			Src->mem2d->SetLayer (v);
			Dest->mem2d->SetLayer (v);
			Dest->mem2d->CopyFrom (Src->mem2d, 0,0, 0,0, Dest->mem2d->GetNx (), Dest->mem2d->GetNy ());
		}
		Dest->mem2d->data->CopyLookups (Src->mem2d->data);
		Dest->mem2d->remove_layer_information ();
		Dest->mem2d->copy_layer_information (Src->mem2d);

		// now append new time frame to Destination Scan
		Dest->append_current_to_time_elements (time_target, real_time);
		reset_shift_vectors();

		Mem2d* target_initial = Dest->mem2d;
		Mem2d* target = target_initial;

		Align_Job_Env prepend_job;
		prepend_job.this_stack_reg = this;
		prepend_job.Src  = Src;
		prepend_job.Dest = Dest;
		prepend_job.time_target = time_target;
		prepend_job.do_layers = false;
		prepend_job.n_times = n_times;
		prepend_job.time_direction = -1;
		prepend_job.stop_flg = &stop_flg;
		prepend_job.layer_target = layer_target;
		prepend_job.globalTransform = &globalTransform;
		prepend_job.anchorPoints = anchorPoints;
		prepend_job.register_region = &register_region;
		prepend_job.job=0;
		prepend_job.progress=0;
		prepend_job.progress_v=0;

		GThread* thread_prepend = g_thread_new ("stack_reg_time_thread_prepend", stack_reg_time_thread, &prepend_job);

		Align_Job_Env append_job;
		append_job.this_stack_reg = this;
		append_job.Src  = Src;
		append_job.Dest = Dest;
		append_job.time_target = time_target;
		append_job.do_layers = false;
		append_job.n_times = n_times;
		append_job.time_direction = 1;
		append_job.stop_flg = &stop_flg;
		append_job.layer_target = layer_target;
		append_job.globalTransform = &globalTransform;
		append_job.anchorPoints = anchorPoints;
		append_job.register_region = &register_region;
		append_job.job=1;
		append_job.progress=0;
		append_job.progress_v=0;

		GThread* thread_append = g_thread_new ("stack_reg_time_thread_append", stack_reg_time_thread, &append_job);

		int progress=0;
		do {
			if (progress < (prepend_job.progress + append_job.progress)){
				progress = prepend_job.progress + append_job.progress;
				main_get_gapp()->progress_info_set_bar_fraction ((gdouble)progress/(gdouble)n_times, 1);
			} else
				main_get_gapp()->check_events ();
		} while (prepend_job.job >= 0 || append_job.job >= 0);

		g_thread_join (thread_prepend);
		g_thread_join (thread_append);

		break;
	}
	case TARGET_LAYER:
		reset_shift_vectors();
		registerLayers (Src->mem2d, Src->mem2d->GetLayer (), Dest->mem2d, Src->mem2d->GetLayer (), 
				globalTransform, *anchorPoints, register_region);
		break;

	case TARGET_TIME_LAYER: {
		// run registration process in time:   
		// 0 ... time_target-1, time_target (ident), time_taget+1 ... N_times-1

		int n_times = Src->number_of_time_elements ();
		int n_values = Src->mem2d->GetNv ();
		int time_target = Src->mem2d->get_t_index ();
		int layer_target  = Src->mem2d->GetLayer ();

		DEBUG_COUT( "*** time_target ....: " << time_target << std::endl);
		DEBUG_COUT( "*** layer_target....: " << layer_target << std::endl);

		main_get_gapp()->progress_info_set_bar_fraction ((gdouble)time_target/(gdouble)n_times, 1);
		double real_time = Src->retrieve_time_element (time_target);	

		DEBUG_COUT( "*** time element index= (Master Time Target): " << time_target << std::endl);

		registerLayers (Src->mem2d, layer_target, Dest->mem2d, layer_target, globalTransform, *anchorPoints, register_region);

		// now append new time frame to Destination Scan
		Dest->append_current_to_time_elements (time_target, real_time);


		Align_Job_Env prepend_job;
		prepend_job.this_stack_reg = this;
		prepend_job.Src  = Src;
		prepend_job.Dest = Dest;
		prepend_job.time_target = time_target;
		prepend_job.do_layers = true;
		prepend_job.n_times = n_times;
		prepend_job.time_direction = -1;
		prepend_job.stop_flg = &stop_flg;
		prepend_job.layer_target = layer_target;
		prepend_job.globalTransform = &globalTransform;
		prepend_job.anchorPoints = anchorPoints;
		prepend_job.register_region = &register_region;
		prepend_job.job=0;
		prepend_job.progress=0;
		prepend_job.progress_v=0;

		GThread* thread_prepend = g_thread_new ("stack_reg_time_thread_prepend_TL", stack_reg_time_thread, &prepend_job);

		Align_Job_Env append_job;
		append_job.this_stack_reg = this;
		append_job.Src  = Src;
		append_job.Dest = Dest;
		append_job.time_target = time_target;
		append_job.do_layers = true;
		append_job.n_times = n_times;
		append_job.time_direction = 1;
		append_job.stop_flg = &stop_flg;
		append_job.layer_target = layer_target;
		append_job.globalTransform = &globalTransform;
		append_job.anchorPoints = anchorPoints;
		append_job.register_region = &register_region;
		append_job.job=1;
		append_job.progress=0;
		append_job.progress_v=0;

		GThread* thread_append = g_thread_new ("stack_reg_time_thread_append_TL", stack_reg_time_thread, &append_job);

		int progress=0;
		int progress_v=0;
		clock_t t0=clock ();
		do {
			if (progress < (prepend_job.progress + append_job.progress)){
				double t = (double)(clock () - t0)/(double)(CLOCKS_PER_SEC);
				progress = prepend_job.progress + append_job.progress;
				gchar *tmp = g_strdup_printf ("Time %d+%d of %d, %gFps", prepend_job.progress, append_job.progress, n_times, t/(progress*n_values));
				main_get_gapp()->progress_info_set_bar_text (tmp, 1);
				g_free (tmp);
				main_get_gapp()->progress_info_set_bar_fraction ((gdouble)progress/(gdouble)n_times, 1);
			} else if (progress_v != (prepend_job.progress_v + append_job.progress_v)){
				progress_v = prepend_job.progress_v + append_job.progress_v;
				gchar *tmp = g_strdup_printf ("Value-1 %d of %d", prepend_job.progress_v, n_values);
				main_get_gapp()->progress_info_set_bar_text (tmp, 2);
				g_free (tmp);
				main_get_gapp()->progress_info_set_bar_fraction ((gdouble)prepend_job.progress_v/(gdouble)n_values, 2);
				tmp = g_strdup_printf ("Value-2 %d of %d", append_job.progress_v, n_values);
				main_get_gapp()->progress_info_set_bar_text (tmp, 3);
				g_free (tmp);
				main_get_gapp()->progress_info_set_bar_fraction ((gdouble)append_job.progress_v/(gdouble)n_values, 3);
			} else
				main_get_gapp()->check_events ();
		} while (prepend_job.job >= 0 || append_job.job >= 0);

		g_thread_join (thread_prepend);
		g_thread_join (thread_append);

		break;
	}
	case TARGET_TEST: { // test image shift generator
		// artifical drift parameters
		double d_tau=5.;
		double tcx=0.02;
		double tcy=0.01;
		double oszns=3.;
		double drift_x, drift_y;

		// number of time frames stored
		int n_times = Src->number_of_time_elements ();

		// start with zero offset
		drift_x = drift_y = 0.;
	
		// go for it....
		for(int time=0; time < n_times && !stop_flg; ++time){ // time loop
			main_get_gapp()->progress_info_set_bar_fraction ((gdouble)time/(gdouble)n_times, 1);

			// get time frame to work on
//			double real_time = Src->retrieve_time_element (time);
			double real_time = Src->retrieve_time_element (0);

			int n_values = Dest->mem2d->GetNv ();
			for(int value=0; value < n_values; ++value){ // value loop
				main_get_gapp()->progress_info_set_bar_fraction ((gdouble)value/(gdouble)n_values, 2);

//				Src->mem2d->SetLayer(value);
				Src->mem2d->SetLayer(0);
				Dest->mem2d->SetLayer(value);

				for(int line=0; line < Dest->mem2d->GetNy (); ++line) // lines (y)
					for(int col=0; col < Dest->mem2d->GetNx (); ++col){ // columns (x)
						double x = col - drift_x;
						double y = line - drift_y;

						// Lookup pixel interpolated at x,y and put to col,line
						Dest->mem2d->PutDataPkt (Src->mem2d->GetDataPktInterpol (x, y), col, line);
					}
				// making up some drift
//				drift_x += d_tau * exp(-(double)(time+(gdouble)value/(gdouble)n_values) * tcx) * sin (2.*M_PI * oszns * ((double)time+(gdouble)value/(gdouble)n_values));
//				drift_y += d_tau * exp(-(double)(time+(gdouble)value/(gdouble)n_values) * tcy);
				drift_x += 2.;
				drift_y += 0.5;
			}

			// now append new time frame to Destination Scan
			Dest->append_current_to_time_elements (time, real_time);

		}
		break;
	}
	default: break;
	}

	main_get_gapp()->progress_info_close ();

	delete anchorPoints;
}


void StackReg::registerLayers (Mem2d* source, int sv, Mem2d* destination, int dv,
			       Matrix_d& globalTransform, Matrix_d& anchorPoints, Matrix_i& register_region, 
			       gboolean seed_in_place, int job, int *progress){
	int n_values = source->GetNv ();
	int layer_target = sv;
	
	if (!seed_in_place){
		// Copy initial Target to destination
		DEBUG_COUT( "*** layer_target .......: " << layer_target << std::endl);
		DEBUG_COUT( "*** copying initial layer target to destination" << std::endl);
// MUTEX LOCK neededd if used in thered (not yet needed here)
		source->SetLayer (layer_target);
		destination->SetLayer (layer_target);
		destination->CopyFrom (source, 0,0, 0,0, destination->GetNx (), destination->GetNy ());
		destination->data->CopyLookups (source->data);
		destination->copy_layer_information (source);
// MUTEX
	}

	int progress_1=0;
	int progress_2=0;

	Align_Job_Env_rl job1;
	job1.this_stack_reg = this;
	job1.source = source;
	job1.destination = destination;
	job1.layer_target = layer_target;
	job1.n_values = n_values;
	job1.layer_direction = -1;
	job1.stop_flg = &stop_flg;
	job1.globalTransform = &globalTransform;
	job1.anchorPoints = &anchorPoints;
	job1.register_region = &register_region;
	job1.job=job;
	job1.progress = progress ? progress : &progress_1;

	GThread* thread_job1 = g_thread_new ("stack_reg_layer_thread_j1", stack_reg_layer_thread, &job1);

	Align_Job_Env_rl job2;
	job2.this_stack_reg = this;
	job2.source = source;
	job2.destination = destination;
	job2.layer_target = layer_target;
	job2.n_values = n_values;
	job2.layer_direction = 1;
	job2.stop_flg = &stop_flg;
	job2.globalTransform = &globalTransform;
	job2.anchorPoints = &anchorPoints;
	job2.register_region = &register_region;
	job2.job=job+2;
	job2.progress = progress ? progress : &progress_2;

	GThread* thread_job2 = g_thread_new ("stack_reg_layer_thread_j2", stack_reg_layer_thread, &job2);

	if (!progress){ // NOT CALLED FROM THREAD !!!
		int progress_sum=0;
		do {
			if (progress_sum < (progress_1 + progress_2)){
				progress_sum = progress_1 + progress_2;
				main_get_gapp()->progress_info_set_bar_fraction ((gdouble)progress_1/(gdouble)n_values, 2);
				main_get_gapp()->progress_info_set_bar_fraction ((gdouble)progress_2/(gdouble)n_values, 3);
			} else
				main_get_gapp()->check_events ();
		} while (job1.job >= 0 || job2.job >= 0);
	}
	
	g_thread_join (thread_job1);
	g_thread_join (thread_job2);

	source->SetLayer (layer_target);
	destination->SetLayer (layer_target);
}


void pv (const gchar* vn, Vector_d& v){
	double ss=0.;
	DEBUG_COUT( (vn?vn:"   ") << " [ ");
	for (int k = 0; k < v.size(); k++) {
		DEBUG_COUT( v[k] << ' ');
		ss += v[k]*v[k];
	}
	DEBUG_COUT( "] |" << sqrt(ss) << "|\n");
}
void pm (const gchar* mn, Matrix_d& m){
	DEBUG_COUT( mn << " [\n");
	for (int k = 0; k < m.size(); k++)
		pv (NULL, m[k]);
	DEBUG_COUT("]\n");
}

void StackReg::registerSlice (
	Mem2d* source, int sv,
	Mem2d* target, int tv,
	Mem2d* destination, int dv,
	Matrix_d& globalTransform,
	Matrix_d& anchorPoints,
	Matrix_i& register_region,
	int job,
	gboolean apply_to_all_layers
	) {

	turboRegAlign align (transformation);
	int np = align.transformation_num_points ();

	double shift=0;
	Matrix_d sourcePoints (np, Vector_d (2, 0.));
	Matrix_d targetPoints (np, Vector_d (2, 0.));
	Matrix_d shiftVectors (np, Vector_d (2, 0.));
	Matrix_d shiftVectors_d (np, Vector_d (2, 0.));
	Matrix_d localTransform;

	Matrix_i s_crop (2, Vector_i (2, 0));
	Matrix_i t_crop (2, Vector_i (2, 0));

	s_crop = register_region;
	t_crop = register_region;


//	DEBUG_COUT( "J["<<job<<"] #NP="<<np<<std::endl);

	int voff = 0;
	if (debug_lvl)
		voff = destination->GetNv () / 2;

//	pm ("sourcePoints", sourcePoints);
//	pm ("targetPoints", targetPoints);

	// set source & target points to anchor points
	for (int i=0; i<np; ++i){
		sourcePoints[i][0] = targetPoints[i][0] = anchorPoints[i][0];
		sourcePoints[i][1] = targetPoints[i][1] = anchorPoints[i][1];
		DEBUG_COUT( "J["<<job<<"] #i["<<i<<"]");
//		pv ("SP initial", sourcePoints[i]);
//		pv ("TP initial", targetPoints[i]);
	}

	if (max_shift_allowed <= 0.){
// ************* simple mode

		align.run (source, sv, s_crop, target, tv, t_crop, sourcePoints, targetPoints);
		align.doTransform (source, sv, sourcePoints, targetPoints, destination, dv); 

		if (apply_to_all_layers){
			int nv = source->GetNv ();
			for (int l=0; l<nv; ++l)
				if (l != dv)
					align.doTransform (source, l, sourcePoints, targetPoints, destination, l); 
		}
		
		shift=0;
		for (int i=0; i<np; ++i){
			double dx = shiftVectors[i][0] = sourcePoints[i][0] - targetPoints[i][0];
			double dy = shiftVectors[i][1] = sourcePoints[i][1] - targetPoints[i][1];
			shift += sqrt (dx*dx + dy*dy);
			DEBUG_COUT( "J["<<job<<"] #i["<<i<<"]");
			pv ("ShiftVec ------------->", shiftVectors[i]);
			pv ("SP final", sourcePoints[i]);
			pv ("TP final", targetPoints[i]);
		}
		
		DEBUG_COUT( "J["<<job<<"] Shiftdistance --------> [" <<sv<< ":" <<tv<< ":" <<dv<< "]: " << shift << ", delta:" << (shift - last_shift[job]) << std::endl);

		last_shift[job] = shift;		

		return;
	}

// ************* advanced mode w checks
	if (shiftVectorsLast[job]->size () != np){
		shiftVectorsLast[job]->resize (np);
		shiftVectorsLast_d[job]->resize (np);
	}

	// add initial shift guess (linear)
	for (int i=0; i<np; ++i){
		sourcePoints[i][0] += (*(shiftVectorsLast[job]))[i][0];
		sourcePoints[i][1] += (*(shiftVectorsLast[job]))[i][1];
	}
//	for (int k=0; k<2; ++k){
//		s_crop[k][0];
//		s_crop[k][1];
//	}


	pv ("SP guess", sourcePoints[0]);
	pv ("TP =====", targetPoints[0]);

	// do initial pre-alignment --> destination (temporary)
	align.doTransform (source, sv, sourcePoints, targetPoints, destination, dv+voff); 

	// start over
	for (int i=0; i<np; ++i){
		sourcePoints[i][0] = targetPoints[i][0] = anchorPoints[i][0];
		sourcePoints[i][1] = targetPoints[i][1] = anchorPoints[i][1];
	}

	// execute soure to target alignment --> shift compuation (target points)
	align.run (destination, dv+voff, s_crop, target, tv, t_crop, sourcePoints, targetPoints); 

	pv ("SP final", sourcePoints[0]);
	pv ("TP final", targetPoints[0]);
	
	shift=0;
	for (int i=0; i<np; ++i){
		double dx = shiftVectors[i][0] = sourcePoints[i][0] - targetPoints[i][0];
		double dy = shiftVectors[i][1] = sourcePoints[i][1] - targetPoints[i][1];
		shift += sqrt (dx*dx + dy*dy);
	}

	pv ("ShiftVec", shiftVectors[0]);
	DEBUG_COUT( "["<<job<<"]--Shiftdistance: " << 
		(shift+sqrt((*(shiftVectorsLast_d[job]))[0][0] * (*(shiftVectorsLast_d[job]))[0][0]) +(*(shiftVectorsLast_d[job]))[0][1] * (*(shiftVectorsLast_d[job]))[0][1]) 
		  << ", Correction: " << shift 
		  << ", delta: " << (shift - last_shift[job]) << std::endl);
	

	// start over
	for (int i=0; i<np; ++i){
		sourcePoints[i][0] = targetPoints[i][0] = anchorPoints[i][0];
		sourcePoints[i][1] = targetPoints[i][1] = anchorPoints[i][1];
	}

	// check for reasonable shift
	if (fabs(shift-last_shift[job]) < max_shift_allowed){
		last_shift[job] = shift;

		for (int i=0; i<np; ++i){
			(*(shiftVectorsLast_d[job]))[i][0] += shiftVectors[i][0]; // correction to guess
			(*(shiftVectorsLast_d[job]))[i][1] += shiftVectors[i][1];
			(*(shiftVectorsLast[job]))[i][0] += (*(shiftVectorsLast_d[job]))[i][0]; // full shift until now
			(*(shiftVectorsLast[job]))[i][1] += (*(shiftVectorsLast_d[job]))[i][1];
			sourcePoints[i][0] += (*(shiftVectorsLast[job]))[i][0];
			sourcePoints[i][1] += (*(shiftVectorsLast[job]))[i][1];
		}
		pv ("SP final full", sourcePoints[0]);
		pv ("ShiftVecL_d", (*(shiftVectorsLast_d[job]))[0]);
		pv ("ShiftVecL", (*(shiftVectorsLast[job]))[0]);
		// final transformation
		align.doTransform (source, sv, sourcePoints, targetPoints, destination, dv); 
	} else {
		DEBUG_COUT( "**Shift out of bonds, assuming constant shift, using last" << std::endl);
		for (int i=0; i<np; ++i){
			(*(shiftVectorsLast[job]))[i][0] += (*(shiftVectorsLast_d[job]))[i][0];
			(*(shiftVectorsLast[job]))[i][1] += (*(shiftVectorsLast_d[job]))[i][1];
		}
		pv ("ShiftVecL", (*(shiftVectorsLast[job]))[0]);
	}

}

