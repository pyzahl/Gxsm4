/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: probe_image_extract.C
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
% PlugInDocuCaption: Probe Image Extract
% PlugInName: probe_image_extract
% PlugInAuthor: Bastian Weyers
% PlugInAuthorEmail: weyers@users.sf.net
% PlugInMenuPath: Math/Probe/Probe_Image_Extract

% PlugInDescription
Extract selected probe data/values and generates a new image from that.
Select a \GxsmEmph{Rectangle}.

% PlugInUsage
Call \GxsmMenu{Math/Probe/Probe\_Image\_Extract}

% OptPlugInSources
The active channel is used as data source.

% OptPlugInObjects
A rectangle object is needed. Probe events must be present in the active scan.

% OptPlugInDest
The computation result is placed into an existing math channel, else into a new created math channel.

% OptPlugInConfig
The PlugIn configurator allows to set a default index andan chanel. If this is non
zero the user is not prompted!

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include <glib.h>
#include <math.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"


static void cancel_callback (GtkWidget *widget, int *status);
static void probe_image_extract_about( void );
static void probe_image_extract_configuration( void );
static gboolean probe_image_extract_run( Scan *Src, Scan *Dest );


GxsmPlugin probe_image_extract_pi = {
	NULL,
	NULL,
	0,
	NULL,
	"Probe_Image_Extract-M1S-PX",
	NULL,
	NULL,
	"Bastian Weyers",
	"math-probe-section",
	N_("Image Extract"),
	N_("Probe Image Extract of selection of scan"),
	"no more info",
	NULL,
	NULL,
	NULL,
	NULL,
	probe_image_extract_about,
	probe_image_extract_configuration,
	NULL,
	NULL
};

GxsmMathOneSrcPlugin probe_image_extract_m1s_pi = {
	probe_image_extract_run
};

static const char *about_text = N_("Gxsm (Raster-) Probe Image Extraction Math-Plugin\n\n"
                                   "Probe Event Data extraction:\n"
				   "A new image is generated from specified probe data.");

double IndexDefault = 1.;

typedef struct{
	Scan* Src;
	Scan* Dest;
	int y_i, y_f;
	int x_i, x_f;
	int numsets;
	int ref_i;
	int norm_i[2];
	double* bg_array;
	double radius;
        double remap[4];
	gchar *Source2;	
        int iSource1;
	ProbeEntry* defaultprobe;
	int *matrix;
	int *status;
	int dead_pixel;
	int job;
	int progress;
	int verbose_level;
        double *cache;
} Probe_Interpolate_Job_Env;

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	probe_image_extract_pi.description = g_strdup_printf(N_("Gxsm MathOneArg probe_image_extract plugin %s"), VERSION);
	return &probe_image_extract_pi; 
}

GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) { 
	return &probe_image_extract_m1s_pi; 
}

static void probe_image_extract_about(void)
{
	const gchar *authors[] = { probe_image_extract_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  probe_image_extract_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

static void probe_image_extract_configuration( void ){
	probe_image_extract_pi.app->ValueRequest("Enter Number", "Index", 
						 "Default set index for data extraction.",
						 probe_image_extract_pi.app->xsm->Unity, 
						 1., 10., ".0f", &IndexDefault);
}

static void cancel_callback (GtkWidget *widget, int *status){
	*status = 1; 
}
	
gint find_probe (gpointer *se, gpointer *default_probe_event){
	return (((EventEntry*) ((ScanEvent*)se)->event_list->data)->description_id () == 'P' ? 0:-1);
}

gint compare_events_distance (ScanEvent *a, ScanEvent *b, double *xy){
	double da = a->distance2 (xy);
	double db = b->distance2 (xy);
	if (da < db) return -1;
	if (da > db) return 1;
	return 0;
}

gint check_probe (ProbeEntry *a, ProbeEntry *b, char *Source2){
	//if (! 
//	std::cout << "check probe: chunk_size=" << b->get_chunk_size () << std::endl;
	for (int j=0; j<b->get_chunk_size (); j++){
	  //		std::cout << j << " : " << b->get_label (j) << std::endl;
		if (! strcmp (Source2, b->get_label (j))) return j;
	}

	return -1;
}

void pe_ChangeValue (GtkComboBox* Combo,  gchar **string){
	GtkTreeIter iter;
	GtkTreeModel *model;
	gtk_combo_box_get_active_iter (GTK_COMBO_BOX (Combo), &iter);
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (Combo));
	gtk_tree_model_get (model, &iter, 0, &(*string), -1);
	//std::cout << "Channel: " << *string << std::endl;
}

void pe_ChangeIndex (GtkSpinButton *spinner, double *index){
	*index = gtk_spin_button_get_value (spinner);

	std::cout << "Change Index: " << ((int)*index) << std::endl;
}

class Dialog : public AppBase{
public:
        Dialog (ProbeEntry* defaultprobe, gchar **Source1, gchar **Source2, double *index, double *qc, double *ref_i, double norm_i[2], double *bg_sub, double *verbose_level, double remap[4], double NSets, Gxsm4app *app):AppBase(app){

		GtkWidget *dialog;
		GtkWidget *source1_combo;
		GtkWidget *source2_combo;
		GtkWidget *spinner;
		GtkAdjustment *spinner_adj;
		GtkWidget *spinner_qc;
		GtkAdjustment *spinner_qc_adj;
		GtkWidget *spinner_r;
		GtkAdjustment *spinner_r_adj;
		GtkWidget *spinner_s;
		GtkAdjustment *spinner_s_adj;

		UnitObj *Unity    = new UnitObj(" "," ");
		UnitObj *Volt     = new UnitObj("V","V");
	
		const char* Source1_lookup[]  = { "Zmon", "Umon", "Time", "XS", "YS", 
						  "ZS", "Bias", "Phase", NULL};

		const char* Source2_lookup[]  = { "ADC0", "ADC1", "ADC2", "ADC3", "ADC4", "ADC5-I","ADC6","ADC7",
						  "Zmon",
						  "LockIn0", "LockIn1stA", "LockIn1stB", "LockIn2ndA", "LockIn2ndB",
						  "In_0", "In_1", "In_2", "In_3", "In_4", "In_5", "In_6", "In_7",
						  "Counter_0", "Counter_1",
						  "PLL_Exci_Frq", "PLL_Exci_Frq_LP", "PLL_Exci_Amp", "PLL_Exci_Amp_LP",
						  NULL};
	
		dialog = gtk_dialog_new_with_buttons (N_("Settings"),
						      NULL,
						      (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
						      _("_OK"), GTK_RESPONSE_ACCEPT,
						      NULL);
		BuildParam bp;
		gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);
		
		bp.grid_add_label ("Source for layer:");
		source1_combo = gtk_combo_box_text_new ();
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (source1_combo), "index", "Index");
		for (int i=0; i<defaultprobe->get_chunk_size (); i++){
			int j=0;
			while (Source1_lookup[j]){
				if (!strncmp (defaultprobe->get_label (i), Source1_lookup[j], 4)){
				  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (source1_combo), defaultprobe->get_label (i), defaultprobe->get_label (i));
					break;
				}
				j++;
			}
		}
		gtk_combo_box_set_active (GTK_COMBO_BOX (source1_combo), 0);
		pe_ChangeValue(GTK_COMBO_BOX (source1_combo), &(*Source1));		//init Source1
		g_signal_connect(G_OBJECT (source1_combo), "changed", G_CALLBACK (pe_ChangeValue), &(*Source1));
		bp.grid_add_widget (source1_combo);
		bp.new_line ();

		bp.grid_add_label ("Channel to map:");
		source2_combo = gtk_combo_box_text_new ();
		for (int i=0; i<defaultprobe->get_chunk_size (); i++){
			int j=0;
			g_message ("Source2: %s", defaultprobe->get_label (i));
			while (Source2_lookup[j]){
				if (!strncmp (defaultprobe->get_label (i), Source2_lookup[j], 4)){
				  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (source2_combo), defaultprobe->get_label (i), defaultprobe->get_label (i));
					break;
				}
				j++;
			}
		}
		gtk_combo_box_set_active (GTK_COMBO_BOX (source2_combo), 0);
		pe_ChangeValue(GTK_COMBO_BOX (source2_combo), &(*Source2));		//init Source2
		gtk_widget_show (source2_combo);			
		g_signal_connect(G_OBJECT (source2_combo), "changed", G_CALLBACK (pe_ChangeValue), &(*Source2));
		bp.grid_add_widget (source2_combo);
		bp.new_line ();

		bp.set_error_text ("Value not allowed.");
		bp.grid_add_ec_with_scale ("Image Filter:\n0 (sharp) ... 10 (smooth)",          Unity, &(*index),     0.,   10., ".1f", 0.1,  1.); bp.new_line ();
		bp.grid_add_ec_with_scale ("Quality Control:\n0 (non) ... 1 (precise match)",   Unity, &(*qc),        0.,    1., ".2f", 0.1,  1.); bp.new_line ();
		bp.grid_add_ec_with_scale ("\nRef. Index for Subtract:\n-1 (non) 0..N-1",       Unity, &(*ref_i),    -1., NSets, ".0f"); bp.new_line ();
		bp.grid_add_ec_with_scale ("\nRef. Index  for Normalization:\n-1 (non) 0..N-1", Unity, &(norm_i[0]), -1., NSets, ".0f", 1.0,  1.); bp.new_line ();
		bp.grid_add_ec_with_scale ("Ref. Index2 for Normalization:\n-1 (non) 0..N-1",   Unity, &(norm_i[1]), -1., NSets, ".0f", 1.0,  1.); bp.new_line ();
		bp.grid_add_ec_with_scale ("\nFlag Backgound RefSpec Subtract:\n0 (non) 1 use all rectangle selecttions as ref.", 
   				                                                                Unity, &(*bg_sub),    0.,    1., ".0f", 0.1,  1.); bp.new_line ();
		bp.grid_add_ec_with_scale ("\nRemap from:", Unity, &(remap[0]), -1e10, 1e10, "g", 0.01,  1.); bp.new_line ();
		bp.grid_add_ec_with_scale ("\nRemap to:",   Unity, &(remap[1]), -1e10, 1e10, "g", 0.01,  1.); bp.new_line ();
		bp.grid_add_ec_with_scale ("\nRemap step:", Unity, &(remap[2]), -1e10, 1e10, "g", 0.01,  1.); bp.new_line ();
		bp.grid_add_ec_with_scale ("\nRemap default:", Unity, &(remap[3]), -1e10, 1e10, "g", 0.01,  1.); bp.new_line ();
		bp.grid_add_ec_with_scale ("\nVerbose Level 0..10", Unity, &(*verbose_level), 0., 10., ".0f", 1.0,  1.); bp.new_line ();
                
                gtk_widget_show (dialog);

                int response = GTK_RESPONSE_NONE;
                g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
                
                // FIX-ME GTK4 ??
                // wait here on response
                while (response == GTK_RESPONSE_NONE)
                        while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

		*ref_i     = round (*ref_i);
		norm_i[0] = round (norm_i[0]);
		norm_i[1] = round (norm_i[1]);
		*bg_sub    = round (*bg_sub);
	}

	~Dialog () {};
};

gpointer probe_interpolate_thread (void *env){
	Probe_Interpolate_Job_Env* job = (Probe_Interpolate_Job_Env*)env;

	int jSource2 = -1;		// -1 means no channel to map -> break
	int pe_number = -1;		// position of the probe event in the event list
	int xmin, xmax, ymin, ymax;	// region that is taken into account for averaging
	int found_a_vector = 0;		// number of vectors found in the given perimeter round (xi,yi)
	double weighting;		// holds the weighting of the current value
	double avgvalues[job->numsets], avgnorm;
	double sigma = job->radius*job->radius/4.;
	int Nx = job->Src->mem2d->GetNx ();
	int Ny = job->Src->mem2d->GetNy ();

	if (job->verbose_level)
		std::cout << "PI_THREAD#" << job->job 
			  << " S2=" << job->Source2
			  << " ["<<job->y_i<< ":" <<job->y_f<< "]:["<<job->x_i<< ":" <<job->x_f<< "] refi=" << job->ref_i << " nrmi=" << job->norm_i[0] << " N=" << job->numsets
			  << std::endl;

	job->dead_pixel = 0;	//dead pixel indicator. Did not found a vektor in the near? Shit we have a dead pixel with all values equal to zero :(
	for (int yi=job->y_i; yi < job->y_f; yi++){
		if (*job->status) break;

		for (int xi=job->x_i; xi < job->x_f; xi++){
			if (*job->status) break;
			job->progress++;

			found_a_vector = 0;		// number of vectors found in the given perimeter round (xi,yi)
			for (int i=0; i<job->numsets; i++) avgvalues[i] = avgnorm = 0;	// initialises avg vectors and the calibration value

			if ( (xmin = xi - (int)job->radius) < 0) xmin = 0;
			if ( (xmax = xi + (int)job->radius) > Nx) xmax = Nx;
			if ( (ymin = yi - (int)job->radius) < 0) ymin = 0;
			if ( (ymax = yi + (int)job->radius) > Ny) ymax = Ny;

			for (int ypos=ymin; ypos < ymax; ypos++){
				for (int xpos=xmin; xpos < xmax; xpos++){
					if ( (pe_number = job->matrix[ypos*Nx+xpos]) >= 0){
						// get the probe data of the scan_event no found in matrix
						ProbeEntry* pe = (ProbeEntry*) ((ScanEvent*) g_slist_nth_data (job->Src->mem2d->scan_event_list, pe_number))->event_list->data;	
						jSource2 = check_probe(job->defaultprobe, pe, job->Source2);
						weighting = exp(-((double)((xpos-xi)*(xpos-xi)+(ypos-yi)*(ypos-yi))/sigma));
						if ( (jSource2 >= 0) && (weighting > 0.) ){
							// double norm = exp(-sqrt((double)((xpos-xi)*(xpos-xi)+(ypos-yi)*(ypos-yi))/smoothness));
							avgnorm += weighting;
                                                        if (fabs (job->remap[2]) > 0.){
                                                                int numsets = job->defaultprobe->get_num_sets ();
                                                                for (int zri=0; zri < job->numsets; zri++){
                                                                        double zavg;
                                                                        if (job->cache[job->numsets*pe_number+zri] < 0.){
                                                                                zavg = job->remap[3];
                                                                                int    n_zavg = 0;
                                                                                double zr_low = job->remap[0] +  ((double)zri-0.5) * job->remap[2];
                                                                                double zr_hi  = job->remap[0] +  ((double)zri+0.5) * job->remap[2];
                                                                                for (int zi=0; zi < numsets; zi++){
                                                                                        double pzv = job->defaultprobe->get (zi, job->iSource1);
                                                                                        //g_message ("pzv=%g [%g:%g]",pzv,zr_low, zr_hi);
                                                                                        if (pzv >= zr_low && pzv <= zr_hi){
                                                                                                if (n_zavg == 0)
                                                                                                        zavg = pe->get(zi, jSource2);
                                                                                                else
                                                                                                        zavg += pe->get(zi, jSource2);
                                                                                                ++n_zavg;
                                                                                        }
                                                                                }
                                                                                if (n_zavg > 0)
                                                                                        zavg /= n_zavg;
                                                                                job->cache[job->numsets*pe_number+zri] = zavg;
                                                                                // g_message ("cache[%d]=%g [%g:%g] #%d",job->numsets*pe_number+zri, zavg,zr_low, zr_hi, n_zavg);
                                                                        } else {
                                                                                zavg = job->cache[job->numsets*pe_number+zri];
                                                                        }
                                                                        avgvalues[zri] += weighting * zavg;
                                                                        found_a_vector++;
                                                                }
                                                        } else {
                                                                for (int zi=0; zi < job->numsets; zi++){
                                                                        double bg = 0.;
                                                                        if (job->bg_array)
                                                                                bg = job->bg_array[zi];
                                                                        if (job->ref_i >= 0 && job->ref_i < job->numsets){
                                                                                double x = pe->get(zi, jSource2) - bg - pe->get(job->ref_i, jSource2);
                                                                                if (job->norm_i[0] >= 0 && job->norm_i[0] < job->numsets)
                                                                                        x /= pe->get(job->norm_i[0], jSource2);
                                                                                avgvalues[zi] += weighting * x;
                                                                        } else if (job->norm_i[0] >= 0 && job->norm_i[0] < job->numsets)
                                                                                avgvalues[zi] += weighting * (pe->get(zi, jSource2) - bg) / pe->get(job->norm_i[0], jSource2);
                                                                        else
                                                                                avgvalues[zi] += weighting * (pe->get(zi, jSource2) - bg);
                                                                }
                                                                found_a_vector++;
                                                        }
						}
					}
			
				}
			}
			
			if (job->verbose_level > 2)
				std::cout << "found_a_vector: " << found_a_vector << std::endl
					  << "Avgnorm: " << avgnorm << std::endl;
			if (found_a_vector == 0){
				for (int zi=0; zi<job->numsets; zi++){
					job->Dest->mem2d->PutDataPkt ( job->remap[3], xi, yi, zi);
				}
				job->dead_pixel++;
				if (job->verbose_level > 2)
					std::cout << "dead_pixel at X" << xi << " Y" << yi << std::endl;
				PI_DEBUG (DBG_L2, "dead_pixel at X" << xi << " Y" << yi << std::endl);
			}
			else {
				for (int zi=0; zi<job->numsets; zi++){
					job->Dest->mem2d->PutDataPkt ( avgvalues[zi]/avgnorm, xi, yi, zi);
				}
				if (job->verbose_level > 5)
					std::cout << "(" << xi << "|" << yi << ")" ;
			}
			if (job->verbose_level > 2)
				std::cout << "avgvalue: " << avgvalues[0] << "avgnorm: " << avgnorm << std::endl;
		
		}
	}
	PI_DEBUG (DBG_L2, "Total number of dead_pixel: " << job->dead_pixel << std::endl);

	if (job->verbose_level)
		std::cout << "PI_THREAD#" << job->job << " DONE." << std::endl;
	job->job = -1; // done indicator
	return NULL;
}

double compare_probes (ProbeEntry* peDefault, ProbeEntry* pe1, ProbeEntry* pe2, char *s2, int numsets){
	double compare_value = 0.;
	int jS21 = check_probe (peDefault, pe1, s2);
	int jS22 = check_probe (peDefault, pe2, s2);

	if (jS21 >= 0 && jS22 >= 0){
		for (int zi=0; zi < numsets; zi++){
			double diff = pe1->get(zi, jS21) - pe2->get(zi, jS22);
			compare_value += diff*diff;
		}
		return compare_value;
	} else 
		return -1.;
}


double* compute_background_data (Scan* Src, int *matrix, ProbeEntry* peDefault, char *s2, int numsets, int vb){
	double* bg = g_new0 (double, numsets);
	double num_spek=0;
	int num_rects=0;
	int n_obj = (int) Src->number_of_object ();

	while (n_obj--){
		scan_object_data *obj_data = Src->get_object_data (n_obj);

		if (strncmp (obj_data->get_name (), "Rectangle", 9) )
			continue; // only rectangels are used!

		if (obj_data->get_num_points () != 2) 
			continue; // sth. is weired!

		// get real world coordinates of rectangle
		double x0,y0,x1,y1;
		obj_data->get_xy_i (0, x0, y0);
		obj_data->get_xy_i (1, x1, y1);

		// convert to pixels
		Point2D p[2];
		Src->World2Pixel (x0, y0, p[0].x, p[0].y);
		Src->World2Pixel (x1, y1, p[1].x, p[1].y);
		
		int i0,i1,j0,j1;
		if (p[0].x < p[1].x) i0=0,i1=1;
		else i0=1,i1=0;
		if (p[0].y < p[1].y) j0=0,j1=1;
		else j0=1,j1=0;

		for (int ri=p[i0].x;  ri<=p[i1].x; ++ri){
			for (int ci=p[j0].y; ci<p[j1].y; ++ci){
				if (matrix[ci*Src->mem2d->GetNx()+ri] > 0){
					ProbeEntry* pe = (ProbeEntry*) ((ScanEvent*) g_slist_nth_data (Src->mem2d->scan_event_list, matrix[ci*Src->mem2d->GetNx()+ri]))->event_list->data;
					int jS2 = check_probe (peDefault, pe, s2);
					if (jS2 >= 0){
						for (int zi=0; zi < numsets; zi++)
							bg[zi] += pe->get(zi, jS2);
						num_spek += 1.;
					}
				}
			}
		}

		++num_rects;
	}

	// normalize
	if (num_spek > 0)
		for (int zi=0; zi < numsets; zi++){
			bg[zi] /= (double)num_spek;
			if (vb > 5)
				std::cout << zi << " " << bg[zi] << std::endl;
		}
	
	return bg;
}

// PlugIn run function, called by menu via PlugIn mechanism.
static gboolean probe_image_extract_run(Scan *Src, Scan *Dest)
{
	PI_DEBUG (DBG_L2, "Probe_Image_Extract Scan");

	int status = 0;			// set to 1 to stop the plugin
	int numsets = Dest->mem2d->GetNv ();
	int avgvectors = 6;		// minimum number of vectors used for interpolation
	double xy[2]; 			// real world position of a probe_event
	static double verbose_level=0;
	static double index = 0.;
	static double qc = 0.7;                // data match quality control
	static double ref_i = -1.;                // ref i -> offset correct
	static double norm_i[2] = { -1., -1. };   // norm i -> data normalization ref point(s)
	static double bg_sub = 0.;                // sub i -> sub ref spectrum
	double *bg_array = NULL;
	gchar *Source1=NULL;			// is the channel for layers V
	gchar *Source2=NULL;			// is the channel, that will be mapped
	int iSource1 = -1;		// -1 means "Index" that is not available as a real channel
	int iSource2 = -1;		// -1 means no channel to map -> break
	int Nx = Src->mem2d->GetNx ();	// total Number of points in x
	int Ny = Src->mem2d->GetNy ();	// total Number of points in y
	double remap[4] = {0., 0., 0., 0.};
        double remap_dv = 0.;
        double *cache=NULL;
	int vb;

	GSList *ProbeEventList = g_slist_find_custom (Src->mem2d->scan_event_list, NULL, (GCompareFunc)find_probe);
	ScanEvent* default_probe_event = (ScanEvent*) ProbeEventList->data;
	if (!default_probe_event) return MATH_OK;  // in case of no probe event in the scan
	ProbeEntry* defaultprobe = (ProbeEntry*) default_probe_event->event_list->data;	// define a default pattern all probes should match

	double NSets = (double)defaultprobe->get_num_sets (); 

	ref_i = (ref_i < -1 || ref_i >= NSets) ? -1. : ref_i;

	for (int k=0; k<2; ++k)
		norm_i[k] = (norm_i[k] < -1 || norm_i[k] >= NSets) ? -1. : norm_i[k];
	
        Dialog *dlg = new Dialog (defaultprobe, &Source1, &Source2, &index, &qc, &ref_i, norm_i, &bg_sub, &verbose_level,
                                  remap, NSets,
                                  main_get_gapp() -> get_app ());  // Asks for channels to use
	vb = (int)verbose_level;

	// if no valid channel is found for Source1, we will use a numerator #
	PI_DEBUG (DBG_L2, "Source1 = " << Source1 << std::endl);
	PI_DEBUG (DBG_L2, "Source2 = " << Source2 << std::endl);
	PI_DEBUG (DBG_L2, "Index: " << index << std::endl);
	PI_DEBUG (DBG_L2, "ref-i: " << ref_i << std::endl);
	PI_DEBUG (DBG_L2, "norm-i0: " << norm_i[0] << std::endl);
	PI_DEBUG (DBG_L2, "norm-i1: " << norm_i[1] << std::endl);

	if (vb){
		std::cout << "Requested data remapping for" << std::endl 
			  << "Source1 = " << Source1  << std::endl 
			  << "Source2 = " << Source2  << std::endl 
			  << "Index   = " << index  << std::endl
			  << "qc      = " << qc << std::endl
			  << "ref-i   = " << ref_i << std::endl
			  << "norm-i0  = " << norm_i[0] << std::endl
			  << "norm-i1  = " << norm_i[1] << std::endl
			  << "bg_sub  = " << bg_sub << std::endl
			  << std::endl;
                g_message ("Remapping on: #=%d %g..%g [%g] {%g}",  Dest->data.s.nvalues, remap[0],remap[1],remap[2],remap[3]);
        }
	
	for (int j=0; j<defaultprobe->get_chunk_size (); j++){
		if (vb > 1)
			std::cout << "Label = " << defaultprobe->get_label (j) << std::endl;
		if (! strncmp (defaultprobe->get_label (j), Source1, 4)) iSource1 = j;
		if (! strncmp (defaultprobe->get_label (j), Source2, 4)) iSource2 = j;
	}
	if (iSource2 < 0) return MATH_OK;  // No valid channel found for Source2, so stop at this point


	if (fabs (remap[2]) > 0.){
                remap_dv = remap[1]-remap[0];
                Dest->data.s.nvalues = 1+fabs (remap_dv) / fabs (remap[2]);
                Dest->data.s.ntimes=1;
                Dest->data.s.dz=1.; //Src->data.s.dz;
                //remap[3] /= Src->data.s.dz; 
                numsets = Dest->data.s.nvalues;        // #points of a probe = #layers
                Dest->mem2d->Resize(Dest->mem2d->GetNx (), Dest->mem2d->GetNy (), numsets, ZD_FLOAT);
                Dest->mem2d->data->MkVLookup(remap[0],remap[1]);
                g_message ("Remapping on: #=%d %g..%g [%g] {%g}",  Dest->data.s.nvalues, remap[0],remap[1],remap[2],remap[3]);
	} else {
                numsets = defaultprobe->get_num_sets ();             // #points of a probe = #layers
                Dest->mem2d->Resize(Dest->mem2d->GetNx (), Dest->mem2d->GetNy (), numsets, ZD_FLOAT);
                Dest->mem2d->data->MkVLookup(0., (double)numsets);
                Dest->data.s.nvalues=numsets;
                Dest->data.s.ntimes=1;
                Dest->data.s.dz=1.;
	}
        
	gchar *s12tmp = g_strdup_printf ("Src: %s:%s", Source1, Source2);
	Dest->mem2d->add_layer_information (new LayerInformation (s12tmp));
	g_free (s12tmp);
	Dest->mem2d->add_layer_information (new LayerInformation ("Smooth:", index, "Sm: %4.2f"));
	Dest->mem2d->add_layer_information (new LayerInformation ("Qc:", qc, "Qc: %4.1f"));
	if (ref_i >= 0.)
		Dest->mem2d->add_layer_information (new LayerInformation ("ref i:", ref_i, "ref#: %4.0f"));
	if (norm_i[0] >= 0.)
		Dest->mem2d->add_layer_information (new LayerInformation ("norm i:", norm_i[0], "nrm#: %4.0f"));
	if (bg_sub > 0.)
		Dest->mem2d->add_layer_information (new LayerInformation ("Backgorund Ref. Spectra Subtracted"));



	if (iSource1 < 0){
		UnitObj *NUnit = new UnitObj( " "," ",".0f", "#");
		Dest->data.SetVUnit (NUnit);
		delete NUnit;
		for (int n=0; n<numsets; n++){
			Dest->mem2d->data->SetVLookup(n, n);
			Dest->mem2d->SetLayer (n);
			Dest->mem2d->add_layer_information (new LayerInformation ("#", (double)n, "%05.0f"));
		}
	} else {
		UnitObj *NUnit = new UnitObj( defaultprobe->get_unit_symbol(iSource1), defaultprobe->get_unit_symbol(iSource1),".3f", defaultprobe->get_label (iSource1));
		Dest->data.SetVUnit (NUnit);
		delete NUnit;
		for (int n=0; n<numsets; n++){
                        double lval;
                        if (fabs (remap[2]) > 0.)
                                Dest->mem2d->data->SetVLookup(n, lval = remap[0]+n*remap[2]);
                        else
                                Dest->mem2d->data->SetVLookup(n, lval = defaultprobe->get (n, iSource1));
			Dest->mem2d->SetLayer (n);
			Dest->mem2d->add_layer_information (new LayerInformation (defaultprobe->get_label (iSource1), lval, "%.4f V"));
		}
	}		

	UnitObj *VoltUnit = new UnitObj( defaultprobe->get_unit_symbol (iSource2), defaultprobe->get_unit_symbol (iSource2),".3f", defaultprobe->get_label (iSource2));
	Dest->data.SetZUnit (VoltUnit);
	delete VoltUnit;


/*	double smoothness = pow(2., index-1.);
  radius = sqrt( (double)(Dest->mem2d->GetNx () * Dest->mem2d->GetNy () * avgvectors)/(double)(3*g_slist_length (Src->mem2d->scan_event_list)))+smoothness;
  PI_DEBUG (DBG_L2, "radius = " << radius << std::endl);
*/

	// radius is the catchment area of each point
	double radius = ((double)Nx-1.)/(1024.-1.)*pow(2., index)+(1024.-(double)Nx)/(1024.-1.);
	double min_radius = sqrt( (double)(Dest->mem2d->GetNx () * Dest->mem2d->GetNy () * avgvectors)/(double)(g_slist_length (Src->mem2d->scan_event_list)));
	if (radius < min_radius) radius = min_radius;	//take care that min 1 vector is in range

	if (vb)
		std::cout << "Smoothing: Index -> Radius (pix) = " << radius << std::endl
			  << "Smoothing:      min Radius       = " << min_radius << std::endl
			  << "... analyzing Probe Events     # = " << (g_slist_length (Src->mem2d->scan_event_list))
			  << std::endl ;


	PI_DEBUG (DBG_L2, "Initialising lookup table for " << g_slist_length (Src->mem2d->scan_event_list) << " probe events." << std::endl);

	// a 'matrix' with information about the position of probe events.
	int* matrix = new int [Nx * Ny];
	for (int i=0; i < Nx * Ny; i++) matrix[i]=-1;	// set all elements to -1, after analysing all probe events -1 will mean no probe at this point

	for (int n=0; n < g_slist_length (Src->mem2d->scan_event_list); n++){
		ScanEvent* se = (ScanEvent*) g_slist_nth_data (Src->mem2d->scan_event_list, n);	// get the nth scan_event
		if (!se) {
			PI_DEBUG (DBG_L2, "Empty Scan_Event found!");
		}
		else if (((EventEntry*) (se->event_list->data))->description_id () == 'P'){	// let's look if it is a valid probe_event
			int xn,yn = 0;
			ProbeEntry* pe = (ProbeEntry*) se->event_list->data;
			se->get_position(xy[0], xy[1]);
			Dest->World2Pixel(xy[0],xy[1], xn , yn);
			if ( xn >= 0 && xn < Nx && yn >= 0 && yn < Ny ){
				matrix[yn*Nx+xn] = n;
				if (vb > 6){
					std::cout << n << " @ ( " << xn+1 << " | " << yn+1 << " ) " << std::endl;
					PI_DEBUG (DBG_L2, "PE" << n << " @ ( " << xn << " | " << yn << " ) " );
				}
			}
		}
	}

	if (qc > 0.){
		int qc_flagged = 0;
		if (vb)
			std::cout << "QC filtering on [" << qc << "]:" << std::endl;
		for (int column=0; column < Ny; column++){
			for (int row=0; row < Nx; row++){
				int pe_test=matrix[column*Nx+row];
				if (pe_test >= 0){
					int pe_ref_i  = 0;
					#define QCN 9
					int pe_ref[QCN] = {-1,-1,-1, -1,-1,-1, -1,-1,-1 };
					double qcv[QCN] = {0.,0.,0., 0.,0.,0., 0.,0.,0.};
				
					for (int si=1;  pe_ref_i < QCN && si<Nx/4; ++si){
						for (int ri=row-si;  pe_ref_i < QCN && ri<row+si && ri<Nx; ++ri){
							while (ri < 0) ++ri;
							for (int ci=column-si; pe_ref_i < QCN && ci<column+si && ci < Ny; ++ci){
								while (ci < 0) ++ci;
								if (ci == column && ri == row) continue;
								if (matrix[ci*Nx+ri] > 0) pe_ref[pe_ref_i++] = matrix[ci*Nx+ri];
							}
						}
					}
					
					ProbeEntry* pe1 = (ProbeEntry*) ((ScanEvent*) g_slist_nth_data (Src->mem2d->scan_event_list, pe_test))->event_list->data;	
					double qc_best = -1.;
					for (int ri=0; ri<QCN; ++ri){
						if (pe_ref[ri] >= 0){
							ProbeEntry* pe2 = (ProbeEntry*) ((ScanEvent*) g_slist_nth_data (Src->mem2d->scan_event_list, pe_ref[ri]))->event_list->data;
							qcv[ri] = compare_probes (defaultprobe, pe1, pe2, Source2, numsets);
							if (qc_best < 0.) qc_best = qcv[ri];
							if (qc_best > qcv[ri]) qc_best = qcv[ri];
						}
					}
					// assuming qc_best as good match quality out of QCN, one allowed to be off 
					int qc_ok=0;
					for (int ri=0; ri<QCN; ++ri){
						if (qcv[ri] <= (qc_best / qc))
							qc_ok++;
					}

					if (qc_ok < QCN/3){ // flag bad? (take out)
						matrix[column*Nx+row] = -2-qc_ok;
						++qc_flagged;
						if (vb > 2)
							std::cout << "QC flag #[" << qc_flagged << "] at [" << column << ", " << row 
								  << "]: QC best=" << qc_best 
								  << "  qc_ok=" << qc_ok
								  << "  qc_lim=" << (qc_best/qc)
								  << std::endl;
					}
				}
			}
		}
		if (vb)
			std::cout << "QC filtering flagged " << qc_flagged << " probes." << std::endl;
	}

	if (bg_sub)
		bg_array = compute_background_data (Src, matrix, defaultprobe, Source2, numsets, vb);
	
	if (vb > 5){
		std::cout << "Probe matrix:" << std::endl;
		for (int column=0; column < Ny; column++){
			for (int row=0; row < Nx; row++){
				if (matrix[column*Nx+row] < 0)
					std::cout << (abs(matrix[column*Nx+row]));
				else std::cout << "+";
			};
			std::cout << std::endl;
		}
	}
	
//	main_get_gapp()->progress_info_new ("Converting Probe Data...", 1);
	main_get_gapp()->progress_info_new ("Converting Probe Data...", 1, GCallback (cancel_callback), &status);
	main_get_gapp()->progress_info_set_bar_fraction (0., 1);
	main_get_gapp()->progress_info_set_bar_text (" ", 1);
//	main_get_gapp()->progress_info_add_info ("Info test text");
//	main_get_gapp()->progress_info_set_bar_fraction (0., 2);
//	main_get_gapp()->progress_info_set_bar_text ("0 %% ", 2);
/*
  std::cout << "Total rows: " << Dest->mem2d->GetNy () << std::endl;
  std::cout << "Total columns: " << Dest->mem2d->GetNx () << std::endl;
  std::cout << "Total spectra: " << g_slist_length (Src->mem2d->scan_event_list) << std::endl;
*/

	#define MAX_JOB 12
	Probe_Interpolate_Job_Env interpolate_job[MAX_JOB];
	GThread* tpi[MAX_JOB];

	std::cout << "PI:run ** spaning thread jobs for interpolation." << std::endl;

	if (fabs (remap[2]) > 0.){
                // setup cache
                int csize = numsets * g_slist_length (Src->mem2d->scan_event_list);
                        cache = g_new0 (double,  csize);
                for (int k=0; k<csize; ++k)
                        cache[k] = -1.;
        }
	// to be threadded:
	//	for (int yi=0; yi < Ny; yi++)...
	int col_per_job = Nx / (MAX_JOB-1);
	int xi = 0;
	int progress_max_job = col_per_job * Ny;
	for (int jobno=0; jobno < MAX_JOB; ++jobno){
		interpolate_job[jobno].Src  = Src;
		interpolate_job[jobno].Dest = Dest;
		interpolate_job[jobno].cache = cache;
		interpolate_job[jobno].defaultprobe = defaultprobe;
		interpolate_job[jobno].Source2 = Source2;
		interpolate_job[jobno].iSource1 = iSource1;
		interpolate_job[jobno].y_i = 0;
		interpolate_job[jobno].y_f = Ny;
		interpolate_job[jobno].x_i = xi;
		xi += col_per_job;
		if (xi < 1 || xi > Nx) xi = Nx;
		interpolate_job[jobno].x_f = xi;
		interpolate_job[jobno].numsets = numsets;
		interpolate_job[jobno].ref_i = (int)ref_i;
		interpolate_job[jobno].norm_i[0] = (int)norm_i[0];
		interpolate_job[jobno].norm_i[1] = (int)norm_i[1];
		interpolate_job[jobno].remap[0] = remap[0];
		interpolate_job[jobno].remap[1] = remap[1];
		interpolate_job[jobno].remap[2] = remap[2];
		interpolate_job[jobno].remap[3] = remap[3];
		interpolate_job[jobno].bg_array = bg_array;
		interpolate_job[jobno].radius = radius;
		interpolate_job[jobno].matrix = matrix;
		interpolate_job[jobno].status = &status;
		interpolate_job[jobno].job = jobno+1;
		interpolate_job[jobno].progress = 0;
		interpolate_job[jobno].verbose_level = (int)verbose_level;
		tpi[jobno] = g_thread_new ("probe_interpolate_thread", probe_interpolate_thread, &interpolate_job[jobno]);
	}

	std::cout << "PI:run ** checking on jobs progress." << std::endl;

	int job=0;
	double psum_mx=0.;
	do {
		double psum=0.;
		job=0;
		for (int jobno=0; jobno < MAX_JOB; ++jobno){
			psum += interpolate_job[jobno].progress;
			job  += interpolate_job[0].job;
		}
		if (psum > psum_mx)
			psum_mx = psum;
		gchar *tmp = g_strdup_printf("%i %%", (int)(100.*psum_mx/MAX_JOB/progress_max_job));
		main_get_gapp()->progress_info_set_bar_text (tmp, 1);
		g_free (tmp);
		main_get_gapp()->check_events ();
	} while (job >= 0);

	std::cout << "PI:run ** finishing up jobs." << std::endl;

	main_get_gapp()->progress_info_set_bar_text ("finishing up jobs", 1);
	main_get_gapp()->check_events ();

	for (int jobno=0; jobno < MAX_JOB; ++jobno)
		g_thread_join (tpi[jobno]);

	std::cout << "PI:run ** cleaning up." << std::endl;

	delete [] matrix;
        if (cache)
                g_free (cache);
	main_get_gapp()->progress_info_close ();
	return MATH_OK;
}

/*		for (int n=0; n < g_slist_length (Src->mem2d->scan_event_list); n++){	// have a look on all available vectors and check if they are matching
  ScanEvent* se = (ScanEvent*) g_slist_nth_data (Src->mem2d->scan_event_list, n);	// get the nth scan_event
  if (!se) {
  PI_DEBUG (DBG_L2, "Empty Scan_Event found!");
  }
  else if (((EventEntry*) (se->event_list->data))->description_id () == 'P'){	// let's look if it is a probe_event
  int xn,yn = 0;
  ProbeEntry* pe = (ProbeEntry*) se->event_list->data;
  se->get_position(xy[0], xy[1]);
  Dest->World2Pixel(xy[0],xy[1], xn , yn);
  jSource2 = check_probe(defaultprobe, pe, Source2);
  if ( jSource2 >= 0 && (xn-xi)*(xn-xi)+(yn-yi)*(yn-yi) < (int) (radius*radius)){
  double norm = exp(-sqrt((double)((xn-xi)*(xn-xi)+(yn-yi)*(yn-yi))/smoothness));
  avgnorm += norm;
  for (int zi=0; zi < numsets; zi++){
  avgvalues[zi] += norm * pe->get(zi, jSource2);
  }
  found_a_vector++;
  }
  //std::cout << ((EventEntry*) (se->event_list->data))->description_id () << std::endl;
  //std::cout << "x: " << xy[0] << "  y: " << xy[1] << std::endl;
  //std::cout << "x: " << xn << "  y: " << yn << std::endl;
  }
  }
*/
