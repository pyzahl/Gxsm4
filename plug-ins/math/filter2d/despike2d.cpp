/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: despike2d.C
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
% PlugInDocuCaption: Despike 2d
% PlugInName: despike2d
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Background/Despike2d

% PlugInDescription
Despike 2d filter.

% PlugInUsage
Call \GxsmMenu{Math/Filter 2D/Despike}.

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
#include "glbvars.h"
#include "surface.h"

// Plugin Prototypes
static void despike2d_init( void );
static void despike2d_about( void );
static void despike2d_configure( void );
static void despike2d_cleanup( void );

static void despike2d_non_interactive( GtkWidget *widget , gpointer pc );

// Define Type of mat plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean despike2d_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean despike2d_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin despike2d_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "despike2d-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-BG",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup("Despike2d math filter."),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-filter2d-section",
  // Menuentry
  N_("Despike"),
  // help text shown on menu
  N_("Despike 2d filter for data."),
  // more info...
  "Despike 2d removed spikes from data.",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  despike2d_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  despike2d_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  despike2d_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  despike2d_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin despike2d_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin despike2d_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin despike2d_m1s_pi
#else
 GxsmMathTwoSrcPlugin despike2d_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   despike2d_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm Despike2d Plugin\n\n"
                                   "This Plugin does a despike2d action with data:\n"
				   "data = data - spike-detect;"
	);

double ConstLast = 0.;
double constval = 0.;

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  despike2d_pi.description = g_strdup_printf(N_("Gxsm MathOneArg despike2d plugin %s"), VERSION);
  return &despike2d_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &despike2d_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &despike2d_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void despike2d_init(void)
{
  PI_DEBUG (DBG_L2, "despike2d Plugin Init");

// This is action remote stuff, stolen from the peak finder PI.
  remote_action_cb *ra;
  GtkWidget *dummywidget = NULL; //gtk_menu_item_new();

  ra = g_new( remote_action_cb, 1);
  ra -> cmd = g_strdup_printf("MATH_FILTER2D_Despike");
  ra -> RemoteCb = &despike2d_non_interactive;
  ra -> widget = dummywidget;
  ra -> data = NULL;
  main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
  PI_DEBUG (DBG_L2, "normal_z-plugin: Adding new Remote Cmd: MATH_FILTER2D_Normal_Z");
// remote action stuff
}

static void despike2d_non_interactive( GtkWidget *widget , gpointer pc )
{
  PI_DEBUG (DBG_L2, "despike2d-plugin: despike2d is called from script.");

//  cout << "pc: " << ((gchar**)pc)[1] << endl;
//  cout << "pc: " << ((gchar**)pc)[2] << endl;
//  cout << "pc: " << atof(((gchar**)pc)[2]) << endl;

  main_get_gapp()->xsm->MathOperation(&despike2d_run);
  return;

}

// about-Function
static void despike2d_about(void)
{
  const gchar *authors[] = { despike2d_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  despike2d_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void despike2d_configure(void)
{
  if(despike2d_pi.app)
    despike2d_pi.app->message("Despike2d Plugin Configuration");
}

// cleanup-Function
static void despike2d_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Despike2d Plugin Cleanup");
}

void ChangeValue (GtkComboBox* Combo,  gchararray *string){
	GtkTreeIter iter;
	GtkTreeModel *model;
	gtk_combo_box_get_active_iter (GTK_COMBO_BOX (Combo), &iter);
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (Combo));
        if (*string){
                g_free (*string); *string=NULL;
        }
	gtk_tree_model_get (model, &iter, 0, &(*string), -1);
        std::cout << *string << std::endl;
}


static void
on_dialog_response (GtkDialog *dialog,
		    int        response,
		    gpointer   user_data)
{
        *((int *) user_data) = response;
        gtk_window_destroy (GTK_WINDOW (dialog));
}

class despike_setup : public AppBase{
public:
	despike_setup (double &nx, double &ny, gchararray *mode,
                       double &max_threads,
                       Gxsm4app *app):AppBase(app){

		GtkWidget *dialog;
                GtkWidget *choice;
                GtkWidget *chooser;

                //UnitObj *meVA    = new UnitObj("meV/" UTF8_ANGSTROEM, "meV/" UTF8_ANGSTROEM );

                GtkDialogFlags flags =  (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);
                dialog = gtk_dialog_new_with_buttons (N_("Despike Filter Setup"),
                                                      gapp->get_main_window  (),
                                                      flags,
                                                      _("_OK"),
                                                      GTK_RESPONSE_ACCEPT,
                                                      _("_Cancel"),
                                                      GTK_RESPONSE_REJECT,
                                                      NULL);
                BuildParam bp;
                gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);
                bp.set_error_text ("Value not allowed.");
                bp.input_nx = 5;

		bp.grid_add_ec_with_scale ("kernel X #px",   main_get_gapp()->xsm->Unity, &nx,    3.,   32., ".0f", 1.,  4.);
                bp.new_line ();
		bp.grid_add_ec_with_scale ("kernel Y #px",   main_get_gapp()->xsm->Unity, &ny,    3.,   32., ".0f", 1.,  4.);
                bp.new_line ();
		
                bp.grid_add_label ("Mode", NULL, 1, 1.0);
		choice = gtk_combo_box_text_new ();
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "STM", "STM");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "Median", "Median");
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (choice), "32760MAX", "32760MAX");
		g_signal_connect(G_OBJECT (choice), "changed", G_CALLBACK (ChangeValue), &(*mode));
		bp.grid_add_widget (choice, 5);
		gtk_combo_box_set_active (GTK_COMBO_BOX (choice), 0);

		bp.grid_add_ec_with_scale ("Max Thread #",   main_get_gapp()->xsm->Unity, &max_threads,    1.,   32., ".0f", 1.,  4.);
                bp.new_line ();
                        
                gtk_widget_show (dialog);
                gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

                int response = GTK_RESPONSE_NONE;
                g_signal_connect (dialog, "response", G_CALLBACK (on_dialog_response), &response);

                // wait here on response
                while (response == GTK_RESPONSE_NONE)
                        while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

                //delete meVA;

                if (response == GTK_RESPONSE_REJECT){
                        *mode = g_strdup ("NONE");
                }
        }

	~despike_setup () {};
};



void despike_d (Mem2d *in, Mem2d *out){
	int i,j,k,l,za=0,nx,ny;
	int anz=1;
	int num=1;
	double reihe1[20],reihe2[20],mi;

        nx=out->GetNx();
        ny=out->GetNy();

        double hi, lo;
        in->HiLo (&hi, &lo);
        
        for (j=anz; j<ny-anz; ++j){
                for (i=anz; i<nx-anz; ++i) {
                        for (k=j-anz, l=0; k<=j+anz; k++,l++)
                                reihe1[l] = in->data->Z(i,k);
                        for (k=0; k<2*anz+1;k++)  {
                                mi = hi;
                                for (l=0; l<2*anz+1;l++) {
                                        if (reihe1[l]<mi && reihe1[l] != 0.0) {
                                                mi=reihe1[l];
                                                reihe2[k]=mi;
                                                za=l;
                                        }
                                }
                                reihe1[za]=hi;
                        }
                        out->data->Z(reihe2[num], i,j);
                }  /*i*/
        } /*j*/
}

void despike_STM (Mem2d *in, Mem2d *out, int i, int j, int v, double hi){
	int za=0;
	int anz=1;
	int num=1;
	double reihe1[20],reihe2[20],mi;

        if (j < out->GetNy()-anz && i < out->GetNx()-anz && j >= anz && i >= anz){
                for (int k=j-anz, l=0; k<=j+anz; k++,l++){
                        //g_print ("Sd: %d %d %d\n",i,k,v);
                        reihe1[l] = in->data->Z(i,k,v);
                }
                for (int k=0; k<2*anz+1;k++)  {
                        mi = hi;
                        for (int l=0; l<2*anz+1;l++) {
                                if (reihe1[l]<mi && reihe1[l] != 0.0) {
                                        mi=reihe1[l];
                                        reihe2[k]=mi;
                                        za=l;
                                }
                        }
                        reihe1[za]=hi;
                }
                out->data->Z(reihe2[num], i,j,v);
        } else
                out->data->Z(in->data->Z(i,j,v), i,j,v); // copy for edges
}

// A utility function to swap two elements 
inline void swap(double* a, double* b) 
{ 
        double t = *a; 
        *a = *b; 
        *b = t; 
}
  
/* This function takes last element as pivot, places 
the pivot element at its correct position in sorted 
array, and places all smaller (smaller than pivot) 
to left of pivot and all greater elements to right 
of pivot */
int partition (double arr[], int low, int high) 
{ 
    double pivot = arr[high]; // pivot 
    int i = (low - 1); // Index of smaller element and indicates the right position of pivot found so far
  
    for (int j = low; j <= high - 1; j++) 
    { 
        // If current element is smaller than the pivot 
        if (arr[j] < pivot) 
        { 
            i++; // increment index of smaller element 
            swap(&arr[i], &arr[j]); 
        } 
    } 
    swap(&arr[i + 1], &arr[high]); 
    return (i + 1); 
} 
  
/* The main function that implements QuickSort 
arr[] --> Array to be sorted, 
low --> Starting index, 
high --> Ending index */
void quickSort(double arr[], int low, int high) 
{ 
    if (low < high) 
    { 
        /* pi is partitioning index, arr[p] is now 
        at right place */
        int pi = partition(arr, low, high); 
  
        // Separately sort elements before 
        // partition and after partition 
        quickSort(arr, low, pi - 1); 
        quickSort(arr, pi + 1, high); 
    } 
} 
  
void despike_median (Mem2d *in, Mem2d *out, int i, int j, int v, int n=3, int m=3){
        double *arr = new double[m*n](); // init 0
        int mm=0;
        for(int k=i-(n-1)/2; k<=i+(n-1)/2; ++k)
                for(int l=j-(m-1)/2; l<=j+(m-1)/2; ++l)
                        if (k<0 || k>=in->GetNx() || l<0 || l>=in->GetNy())
                                arr[mm++]=in->data->Z(i,j,v);
                        else
                                arr[mm++]=in->data->Z(k,l,v);
        
        quickSort(arr, 0, n*m - 1); 
        if ((n*m) & 1)
                out->data->Z(arr[(n*m-1)/2], i,j,v);
        else
                out->data->Z(0.5*(arr[(n*m-1)/2] + arr[(n*m-1)/2+1]), i,j,v);
        delete [] arr;
}



typedef struct{
	Mem2d* SrcM;
	Mem2d* DestM;
	int y_i, y_f;
	int x_i, x_f;
	int l_i, l_f;
	int Mn, Mm;
        gchar mode;
	int *status;
	int job;
	int progress;
} DESPIKE_Job_Env;



gpointer DESPIKE_thread (void *env){
	DESPIKE_Job_Env* job = (DESPIKE_Job_Env*)env;
	double valmin, val;
	double z;

        if (job->job)
                for (int v_index=job->l_i; v_index <= job->l_f; ++v_index)
                        job->DestM->data->SetVLookup(v_index, job->SrcM->data->GetVLookup(v_index));

	for (int yi=job->y_i; yi < job->y_f; yi++){
		if (*job->status) break;

		for (int xi=job->x_i; xi < job->x_f; xi++){
			if (*job->status) break;

			for (int v_index=job->l_i; v_index <= job->l_f; ++v_index){
                                //in->SetLayer (v);
                                //in->HiLo (&hi, &lo);
                                switch (job->mode){
                                case 'S': // STM mode
                                        despike_STM (job->SrcM, job->DestM, xi, yi, v_index, 1e20 ); break;
                                case 'M': // Median mode
                                        despike_median (job->SrcM, job->DestM, xi, yi, v_index, job->Mn, job->Mm); break;
                                case '3': break;
                                        despike_STM (job->SrcM, job->DestM, xi, yi, v_index, 32650.); break;
                                case 'N': break;
                                default : break;
                                }
                        }

			job->progress++;
		}
	}
	job->job = -1; // done indicator
	return NULL;
}

static void cancel_callback (GtkWidget *widget, int *status){
	*status = 1; 
}
	


// run-Function
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 static gboolean despike2d_run(Scan *Src, Scan *Dest)
#else
 static gboolean despike2d_run(Scan *Src1, Scan *Src2, Scan *Dest)
#endif
{
        int status = 0;
	int ti=0; 
	int tf=0;
	int vi=0;
	int vf=0;
	gboolean multidim = FALSE;

        double Mm=3.0;
        double Mn=3.0;
        double max_threads = 8.;
        int max_job=8;
        gchararray mode = NULL;
                
	despike_setup (Mn, Mm, &mode, max_threads, main_get_gapp() -> get_app ());
        max_job=(int)max_threads;

	if (Src->data.s.ntimes != 1 || Src->mem2d->GetNv () != 1){
		multidim = TRUE;
		do {
			ti=vi=0;
			tf=Src->number_of_time_elements ()-1;
			if (tf < 0) tf = 0;
			vf=Src->mem2d->GetNv ()-1;
			main_get_gapp()->setup_multidimensional_data_copy ("Multidimensional Despike2d", Src, ti, tf, vi, vf);
		} while (ti > tf || vi > vf);

		main_get_gapp()->progress_info_new ("Multidimenssional Despike2d", 2);
		main_get_gapp()->progress_info_set_bar_fraction (0., 1);
		main_get_gapp()->progress_info_set_bar_fraction (0., 2);
		main_get_gapp()->progress_info_set_bar_text ("Time", 1);
		main_get_gapp()->progress_info_set_bar_text ("Value", 2);
	}

	main_get_gapp()->progress_info_new (mode, 1, GCallback (cancel_callback), &status);
	main_get_gapp()->progress_info_set_bar_fraction (0., 1);
	main_get_gapp()->progress_info_set_bar_text (" ", 1);

	#define MAX_JOB 32
	DESPIKE_Job_Env despike_job[MAX_JOB];
	GThread* tpi[MAX_JOB];
        if (max_job > MAX_JOB)
                max_job=MAX_JOB;

	int ntimes_tmp = tf-ti+1;
	for (int time_index=ti; time_index <= tf; ++time_index){
		Mem2d *m = Src->mem2d_time_element (time_index);
		if (multidim)
			main_get_gapp()->progress_info_set_bar_fraction ((gdouble)(time_index-ti)/(gdouble)ntimes_tmp, 1);

		Dest->mem2d->Resize (m->GetNx (), m->GetNy (), vf-vi+1, m->GetTyp());

                /*
                for (int v_index = vi; v_index <= vf; ++v_index){
			m->SetLayer (v_index);
                        Dest->mem2d->data->SetVLookup(v_index, Src->mem2d->data->GetVLookup(v_index));
			Dest->mem2d->SetLayer (v_index-vi);	

			Dest->mem2d->CopyFrom(m, 0,0, 0,0, 
					      Dest->mem2d->GetNx(), Dest->mem2d->GetNy());
                        //despike_32650max (m, Dest->mem2d);
                        if (Mm < 3. || Mn < 3.)
                                despike_d (m, Dest->mem2d);
                        else
                                despike_median (m, Dest->mem2d, (int)Mn, (int)Mm);

		}
                */

                std::cout << "DESPIKE:run ** spaning " << max_job << " thread jobs <" << mode << "> [" << Mn << ", " <<  Mm << "] for time element " << time_index << "." << std::endl;

                // ******************************
                // to be threadded:
                //	for (int yi=0; yi < Ny; yi++)...
                int col_per_job = m->GetNx () / (max_job-1);
                int xi = 0;
                int progress_max_job = col_per_job * m->GetNy ();
                for (int jobno=0; jobno < max_job; ++jobno){
                        despike_job[jobno].SrcM  = m;
                        despike_job[jobno].DestM = Dest->mem2d;
                        despike_job[jobno].y_i = 0;
                        despike_job[jobno].y_f =  m->GetNy ();
                        despike_job[jobno].x_i = xi;
                        xi += col_per_job;
                        if (xi < 1 || xi >  m->GetNx ()) xi = m->GetNx ();
                        despike_job[jobno].x_f = xi;
                        despike_job[jobno].l_i = vi;
                        despike_job[jobno].l_f = vf;
                        despike_job[jobno].Mn = (int)Mn;
                        despike_job[jobno].Mm = (int)Mm;
                        despike_job[jobno].mode = (gchar)mode[0];
                        despike_job[jobno].status = &status;
                        despike_job[jobno].job = jobno+1;
                        despike_job[jobno].progress = 0;
                        gchar *tid = g_strdup_printf ("DESPIKE_thread_%d", jobno);
                        tpi[jobno] = g_thread_new (tid, DESPIKE_thread, &despike_job[jobno]);
                        g_free (tid);
                }

                std::cout << "DESPIKE:run ** checking on jobs progress." << std::endl;

                int job=0;
                double psum_mx=0.;
                do {
                        double psum=0.;
                        job=0;
                        for (int jobno=0; jobno < max_job; ++jobno){
                                psum += despike_job[jobno].progress;
                                job  += despike_job[0].job;
                        }
                        if (psum > psum_mx)
                                psum_mx = psum;
                        gchar *tmp = g_strdup_printf("%i %%", (int)(100.*psum_mx/max_job/progress_max_job));
                        main_get_gapp()->progress_info_set_bar_text (tmp, 1);
                        g_free (tmp);
                        main_get_gapp()->check_events ();
                } while (job >= 0);

                std::cout << "DESPIKE:run ** finishing up jobs." << std::endl;

                main_get_gapp()->progress_info_set_bar_text ("finishing up jobs", 1);
                main_get_gapp()->check_events ();

                for (int jobno=0; jobno < max_job; ++jobno)
                        g_thread_join (tpi[jobno]);


                // ******************************
                Dest->append_current_to_time_elements (time_index-ti, m->get_frame_time ());
	}


	std::cout << "DESPIKE:run ** cleaning up." << std::endl;

	main_get_gapp()->progress_info_close ();

        
	Dest->data.s.ntimes = ntimes_tmp;
	Dest->data.s.nvalues=Dest->mem2d->GetNv ();

        Dest->mem2d->data->CopyLookups (Src->mem2d->data);
        Dest->mem2d->copy_layer_information (Src->mem2d);

	if (multidim){
		main_get_gapp()->progress_info_close ();
		Dest->retrieve_time_element (0);
		Dest->mem2d->SetLayer(0);
	}
	
	return MATH_OK;
}

