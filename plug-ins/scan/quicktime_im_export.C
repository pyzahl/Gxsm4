/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: quicktime_im_export.C
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
% PlugInDocuCaption: Quicktime
% PlugInName: quicktime_im_Export
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: File/Import/Quicktime

% PlugInDescription
 The \GxsmEmph{quicktime\_im\_export} plug-in allows exporting of single
 and multidimensional image data sets as Quicktime Movie.

% PlugInUsage
 The plug-in is called by \GxsmMenu{File/Import/Quicktime}.

\GxsmNote{Recommended Format: MJPEG-A, works fine with Power-Point and most Movie Players.}

\GxsmScreenShot{quicktime_export_file}{QT Export File Dialog}
\GxsmScreenShotDual{quicktime_export_setup1}{QT Export Setup step one.}{quicktime_export_setup2}{QT Export Setup step two.}

%% OptPlugInKnownBugs
There seam to be much more formats available from the lib-quicktime, 
but for an unknown reason some are just not working or crashing the program if used.
This seams to depend on the system and libquicktime version used, so please try it out.

%% OptPlugInRefs


% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"
#include "core-source/dataio.h"
#include "core-source/action_id.h"
#include "core-source/util.h"
#include "core-source/xsmtypes.h"
#include "core-source/glbvars.h"
#include "core-source/gapp_service.h"
#include "core-source/app_view.h"

// custom includes go here
#include "lqt/quicktime.h"
#include "lqt/colormodels.h"
#include "lqt/lqt.h"
#include "lqt/lqt_codecinfo.h"

// enable std namespace
using namespace std;

// Plugin Prototypes
static void quicktime_im_export_init (void);
static void quicktime_im_export_query (void);
static void quicktime_im_export_about (void);
static void quicktime_im_export_configure (void);
static void quicktime_im_export_cleanup (void);

static void quicktime_im_export_filecheck_load_callback (gpointer data );
static void quicktime_im_export_filecheck_save_callback (gpointer data );

static void quicktime_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void quicktime_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// Fill in the GxsmPlugin Description here
GxsmPlugin quicktime_im_export_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
// -- START EDIT --
	"QUICKTIME-ImExport",            // PlugIn name
	NULL,                   // PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	NULL,
	"Percy Zahl",
	"file-import-section,file-export-section", // sep. im/export menuentry path by comma!
	N_("Quicktime,Quicktime"), // menu entry (same for both)
	N_("Quicktime import,Quicktime export"), // short help for menu entry
	N_("Quicktime im/export filter."), // info
// -- END EDIT --
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	quicktime_im_export_init,
	quicktime_im_export_query,
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	quicktime_im_export_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	quicktime_im_export_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL,
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
        quicktime_im_export_import_callback, // direct menu entry callback1 or NULL
        quicktime_im_export_export_callback, // direct menu entry callback2 or NULL

	quicktime_im_export_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("This GXSM plugin im-/exports from/to QUICKTIME");

static const char *file_mask = "*.mov";

int video_dimension=0;

int offset_index_value=0;
int step_index_value=1;
int max_index_value=1;
double start_value=0.;
double step_value=1.;

int offset_index_time=0;
int step_index_time=1;
int max_index_time=1;
double start_time=0.;
double step_time=1.;

double frame_rate=15.;

int qt_quality = 95;
int qt_codec = 0;
gchar *qt_video_codec = NULL;

gboolean OSD_grab_mode=FALSE;
gboolean conti_autodisp_mode=FALSE;

#define FILE_DIM_V   0x01
#define FILE_DIM_T   0x02
#define FILE_ASKDIM  0x04
#define FILE_CODEC   0x08
#define FILE_DECODE  0x10
#define FILE_IMPORT  0x20
int file_dim=0;

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	quicktime_im_export_pi.description = g_strdup_printf(N_("Gxsm im_export plugin %s"), VERSION);
	return &quicktime_im_export_pi; 
}

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/PNG"
// Export Menupath is "File/Export/PNGt"
// ----------------------------------------------------------------------

static void quicktime_im_export_query(void)
{

	if(quicktime_im_export_pi.status) g_free(quicktime_im_export_pi.status); 
	quicktime_im_export_pi.status = g_strconcat (
		N_("Plugin query has attached "),
		quicktime_im_export_pi.name, 
		N_(": File IO Filters are ready to use."),
		NULL);
	

	// register this plugins filecheck functions with Gxsm now!
	// This allows Gxsm to check files from DnD, open, 
	// and cmdline sources against all known formats automatically - no explicit im/export is necessary.
	quicktime_im_export_pi.app->ConnectPluginToLoadFileEvent (quicktime_im_export_filecheck_load_callback);
	quicktime_im_export_pi.app->ConnectPluginToSaveFileEvent (quicktime_im_export_filecheck_save_callback);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void quicktime_im_export_init(void)
{
	PI_DEBUG (DBG_L2, quicktime_im_export_pi.name << " Plugin Init");
}

// about-Function
static void quicktime_im_export_about(void)
{
	const gchar *authors[] = { quicktime_im_export_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  quicktime_im_export_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void quicktime_im_export_configure(void)
{
	if(quicktime_im_export_pi.app){
		XsmRescourceManager xrm("QUICKTIME_IM_EXPORT");
		GtkWidget *dim_sel = NULL;
		GtkWidget *codec_sel = NULL;
		GtkWidget *OSD_grab_flg = NULL;
		GtkWidget *cont_autodisp_flg = NULL;
		lqt_codec_info_t **ci_list = NULL;

//		xrm.Get ("file_offset_index_value", &offset_index_value, "0");
		xrm.Get ("file_step_index_value", &step_index_value, "1");
		xrm.Get ("file_start_value", &start_value, "0");
		xrm.Get ("file_step_value", &step_value, "1");

//		xrm.Get ("file_offset_index_time", &offset_index_time, "0");
		xrm.Get ("file_step_index_time", &step_index_time, "1");
		xrm.Get ("file_start_time", &start_time, "0");
		xrm.Get ("file_step_time", &step_time, "1");

		xrm.Get ("file_frame_rate", &frame_rate, "15.");
		xrm.Get ("file_qt_codec", &qt_codec, "0");
		xrm.Get ("file_qt_quality", &qt_quality, "85");

		xrm.Get ("file_video_dimension", &video_dimension, "0");


		GtkDialogFlags flags =  (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);
		GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("Quicktime Video Export"),
								 GTK_WINDOW (gapp->get_app_window ()),
								 flags,
								 _("_OK"),
								 GTK_RESPONSE_ACCEPT,
								 _("_Cancel"),
								 GTK_RESPONSE_REJECT,
								 NULL);
		BuildParam bp;
		gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);


                ////////
                
		if (file_dim & FILE_ASKDIM){
			dim_sel = gtk_combo_box_text_new ();
			gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (dim_sel),"video-time", "Video Time Dimension: Time");
			gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (dim_sel),"video-layer", "Video Time Dimension: Layer (Value)");
			gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (dim_sel),"video-layer-time", "Video Time Dimension: Layer and Time");
			gtk_combo_box_set_active_id (GTK_COMBO_BOX (dim_sel), "video-time");
                        bp.grid_add_widget (dim_sel); bp.new_line ();
		}

		if (file_dim & FILE_CODEC){

			codec_sel = gtk_combo_box_text_new ();

			// query list of codecs for AUDIO=0, VIDEO=1, ENCODE, DECODE
			ci_list = lqt_query_registry(0, 1, (file_dim & FILE_DECODE) ? 0:1, (file_dim & FILE_DECODE) ? 1:0);
                        int i=0;
			for (lqt_codec_info_t **ci=ci_list; *ci; ++ci, ++i){
				gchar *info=NULL;
				cout << "LQT_VIDEO_CODEC [" << (*ci)->name << "]: " << (*ci)->long_name << endl;
				cout << "Description: " << (*ci)->description << endl;
				info = g_strdup_printf ("%s [%s]", (*ci)->long_name, (*ci)->name);
				gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (codec_sel), info, info);
                                if ( strcmp ( (*ci)->name, "mjpa") == 0)
                                        qt_codec = i;
				g_free (info);
			}
			gtk_combo_box_set_active (GTK_COMBO_BOX (codec_sel), qt_codec);
                        bp.grid_add_widget (codec_sel,2); bp.new_line ();

			if (file_dim & FILE_DECODE){
				bp.grid_add_ec ("Quality [%]", quicktime_im_export_pi.app->xsm->Unity, &qt_quality, 0., 100., ".0f"); bp.new_line ();
				bp.grid_add_ec ("Frame Rate [1/s]", quicktime_im_export_pi.app->xsm->Unity, &frame_rate, 1e-6, 100., ".3f"); bp.new_line ();
			}
		}
		if (file_dim & FILE_DIM_V){
                        bp.grid_add_ec ("Max Index Values", quicktime_im_export_pi.app->xsm->Unity, &max_index_value, 1., 1e6, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Index Offset", quicktime_im_export_pi.app->xsm->Unity, &offset_index_value, -1e6, 1e6, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Index Step", quicktime_im_export_pi.app->xsm->Unity, &step_index_value, -1000, 1000, ".0f"); bp.new_line ();
			if (file_dim & FILE_IMPORT){
                                bp.grid_add_ec ("Start Value", quicktime_im_export_pi.app->xsm->Unity, &start_value, -1e6, 1e6, ".3f"); bp.new_line ();
                                bp.grid_add_ec ("Step Value", quicktime_im_export_pi.app->xsm->Unity, &step_value, -1e6, 1e6, ".3f"); bp.new_line ();
			}
		}		

		if (file_dim & FILE_DIM_T){
			bp.grid_add_ec ("Max Index Times", quicktime_im_export_pi.app->xsm->Unity, &max_index_time, 1., 1e6, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Index Offset", quicktime_im_export_pi.app->xsm->Unity, &offset_index_time, -1e6, 1e6, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Index Step", quicktime_im_export_pi.app->xsm->Unity, &step_index_time, -1000, 1000, ".0f"); bp.new_line ();
			if (file_dim & FILE_IMPORT){
                                bp.grid_add_ec ("Start Time", quicktime_im_export_pi.app->xsm->Unity, &start_time, -1e6, 1e6, ".3f"); bp.new_line ();
                                bp.grid_add_ec ("Step Time", quicktime_im_export_pi.app->xsm->Unity, &step_time, -1e6, 1e6, ".3f"); bp.new_line ();
                        }
		}		
                //	bp.grid_add_ec ("Time Origin", quicktime_im_export_pi.app->xsm->TimeUnit, &realtime0_user, 0., 1e20, ".1f");

	        cont_autodisp_flg = gtk_check_button_new_with_label(N_("Contineous AutoDisp"));
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON (cont_autodisp_flg), conti_autodisp_mode);
		bp.grid_add_widget (cont_autodisp_flg); bp.new_line ();

#if 0
	        OSD_grab_flg = gtk_check_button_new_with_label(N_("Scan/OSD life grab mode"));
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON (OSD_grab_flg), OSD_grab_mode);
		bp.grid_add_widget (OSD_grab_flg); bp.new_line ();

		bp.grid_add_label ("Note: Keep the Scan/OSD window on top!");
#endif

                gtk_widget_show (dialog);

                int response = GTK_RESPONSE_NONE;
                g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data_no_destroy), &response);
        
                // FIX-ME GTK4 ??
                // wait here on response
                while (response == GTK_RESPONSE_NONE)
                        while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

		if (OSD_grab_flg)
		        OSD_grab_mode = gtk_check_button_get_active (GTK_CHECK_BUTTON (OSD_grab_flg));

		if (cont_autodisp_flg)
		  conti_autodisp_mode = gtk_check_button_get_active (GTK_CHECK_BUTTON (cont_autodisp_flg));

		if (dim_sel)
			video_dimension = gtk_combo_box_get_active (GTK_COMBO_BOX (dim_sel));

		if (codec_sel){
			qt_codec = gtk_combo_box_get_active (GTK_COMBO_BOX (codec_sel));
			if (ci_list){
				if (qt_video_codec)
					g_free (qt_video_codec);
				qt_video_codec = g_strdup ((ci_list[qt_codec])->name);
				lqt_destroy_codec_info (ci_list);
				cout << "Codec Selected: [" << qt_video_codec << "]" << endl;
			}
		}

                gtk_window_destroy (GTK_WINDOW (dialog));


		xrm.Put ("file_max_index_value", max_index_value);
		xrm.Put ("file_offset_index_value", offset_index_value);
		xrm.Put ("file_step_index_value", step_index_value);
		xrm.Put ("file_start_value", start_value);
		xrm.Put ("file_step_value", step_value);

		xrm.Put ("file_max_index_time", max_index_time);
		xrm.Put ("file_offset_index_time", offset_index_time);
		xrm.Put ("file_step_index_time", step_index_time);
		xrm.Put ("file_start_time", start_time);
		xrm.Put ("file_step_time", step_time);

		xrm.Put ("file_frame_rate", frame_rate);

		xrm.Put ("file_video_dimension", video_dimension);
		xrm.Put ("file_qt_codec", qt_codec);
		xrm.Put ("file_qt_quality", qt_quality);

	}
}

// cleanup-Function, remove all "custom" menu entrys here!
static void quicktime_im_export_cleanup(void)
{
#if 0
	gchar **path  = g_strsplit (quicktime_im_export_pi.menupath, ",", 2);
	gchar **entry = g_strsplit (quicktime_im_export_pi.menuentry, ",", 2);

	gchar *tmp = g_strconcat (path[0], entry[0], NULL);
	gnome_app_remove_menus (GNOME_APP (quicktime_im_export_pi.app->getApp()), tmp, 1);
	g_free (tmp);

	tmp = g_strconcat (path[1], entry[1], NULL);
	gnome_app_remove_menus (GNOME_APP (quicktime_im_export_pi.app->getApp()), tmp, 1);
	g_free (tmp);

	g_strfreev (path);
	g_strfreev (entry);

#endif
	PI_DEBUG (DBG_L2, "Plugin Cleanup done.");
}


// make a new derivate of the base class "Dataio"
class Quicktime_ImExportFile : public Dataio{
public:
	Quicktime_ImExportFile(Scan *s, const char *n); 
	virtual ~Quicktime_ImExportFile();
	virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
	virtual FIO_STATUS Write();
private:
};

Quicktime_ImExportFile::Quicktime_ImExportFile(Scan *s, const char *n) : Dataio(s,n){
}

Quicktime_ImExportFile::~Quicktime_ImExportFile(){
}

FIO_STATUS Quicktime_ImExportFile::Read(xsm::open_mode mode){
	FIO_STATUS ret;
	gchar *fname=NULL;

	PI_DEBUG (DBG_L2, "reading");

	fname = (gchar*)name;

	// name should have at least 4 chars: ".ext"
	if (fname == NULL || strlen(fname) < 4)
		return status=FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

	if (strncasecmp (fname+strlen(fname)-4,".mov", 4))
		return status=FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

	file_dim=0;
//	file_dim |= FILE_DIM_V;
	file_dim |= FILE_DIM_T;

	int index_time=0;
	ret=FIO_OK;
	
	quicktime_im_export_configure ();

	gapp->progress_info_new ("QT Import", 2);
	gapp->progress_info_set_bar_fraction (0., 1);
	gapp->progress_info_set_bar_fraction (0., 2);
	gapp->progress_info_set_bar_text (fname, 1);
	do {
		int index_value=0;
		do {
			gapp->progress_info_set_bar_fraction ((gdouble)index_time/(gdouble)max_index_time, 1);
			gapp->progress_info_set_bar_fraction ((gdouble)index_value/(gdouble)max_index_value, 2);
//			gapp->progress_info_set_bar_text (fname_expand, 2);
			++index_value;
			
		} while (ret == FIO_OK && index_value < max_index_value);
		
		scan->append_current_to_time_elements (index_time, start_time + step_time*(index_time));
		++index_time;
		
	} while (ret == FIO_OK && index_time < max_index_time);
	gapp->progress_info_close ();
	scan->retrieve_time_element (0);

	scan->SetVM(SCAN_V_DIRECT);
	return ret;
//	return status=FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
}

// http://libquicktime.sourceforge.net/doc/apiref/group__codec__registry.html
FIO_STATUS Quicktime_ImExportFile::Write(){
	quicktime_t *qtfile;
        // get access to Scan ViewControl
        ViewControl* vc = scan->view->Get_ViewControl();
        if (!vc) return FIO_NO_NAME;

	if (name == NULL) return FIO_NO_NAME;

	if (strncmp(name+strlen(name)-4,".mov",4))
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

	file_dim=0;
	file_dim |= FILE_ASKDIM;

	{	
		XsmRescourceManager xrm("QUICKTIME_IM_EXPORT");
		xrm.Get ("file_video_dimension", &video_dimension, "0");
	}
	quicktime_im_export_configure ();

	offset_index_time  = scan->mem2d->get_t_index ();
	offset_index_value = scan->mem2d->GetLayer ();

	file_dim=0;

	switch (video_dimension){
	case 0: // time
		file_dim |= FILE_DIM_T;
		max_index_time  = scan->data.s.ntimes-1;
		max_index_value = offset_index_value;
		break;
	case 1: // layer
		file_dim |= FILE_DIM_V;
		max_index_time  = offset_index_time;
		max_index_value = scan->mem2d->GetNv ()-1;
		break;
	default: // both
		file_dim |= FILE_DIM_V;
		file_dim |= FILE_DIM_T;
		offset_index_time  = 0;
		offset_index_value = 0;
		max_index_time  = scan->data.s.ntimes-1;
		max_index_value = scan->mem2d->GetNv ()-1;
		break;
	}

	file_dim |= FILE_CODEC;
	file_dim |= FILE_DECODE;
	quicktime_im_export_configure ();

        // and open the qtfile in write mode for video encoding

	if ((qtfile = quicktime_open (name, 0, 1)) == 0)
		return FIO_NO_NAME;

        /*
        lqt_codec_info_t** lqt_find_video_codec	(	char * 	fourcc,
                                                        int 	encode	 
                                                        )	
        int lqt_add_video_track	(	quicktime_t * 	file,
                                        int 	frame_w,
                                        int 	frame_h,
                                        int 	frame_duration,
                                        int 	timescale,
                                        lqt_codec_info_t * 	codec_info	 
                                        )			
        */

        const int timescale = 30000;
        int frame_duration = (int)round((double)timescale/frame_rate);

	g_message ("lqt_add_video_track %d x %d, ts=%d, fd=%d, codec: %s",  vc->get_npx (), vc->get_npy (), timescale, frame_duration, qt_video_codec);
        lqt_codec_info_t **lqt_codec = lqt_find_video_codec (qt_video_codec, 1); // firts on in list/configured -- for now

        if (lqt_add_video_track (qtfile, vc->get_npx (), vc->get_npy (), frame_duration, timescale, *lqt_codec)){
		quicktime_close (qtfile);
		cout << "Add track failed." << endl;
		return  FIO_NO_NAME; // Add Track error
        }

        // void lqt_set_video_parameter (quicktime_t *file, int track, const char *key, const void *value)
	g_message ("lqt_set_video_parameter jpeg_quality to %d", qt_quality);
	lqt_set_video_parameter (qtfile, 0, "jpeg_quality", &qt_quality);

        // void lqt_set_cmodel (quicktime_t *file, int track, int colormodel)
        g_message ("lqt_set_cmodel to BGRA8888");
        lqt_set_cmodel (qtfile, 0, BC_RGBA8888); // RGBA 32bit
        //lqt_set_cmodel (qtfile, 0, BC_BGR8888);  // BGRX 32bit

        gapp->progress_info_new ("QT Export", 2);
        gapp->progress_info_set_bar_text ("Time", 1);
        gapp->progress_info_set_bar_text ("Layer", 2);

        //const lqt_codec_info_t *qt_vc_info = lqt_get_video_codec_info (int index)

        /*
          int lqt_add_video_track (
                quicktime_t * 	file,
                int 	frame_w,
                int 	frame_h,
                int 	frame_duration,
                int 	timescale,
                lqt_codec_info_t * 	codec_info	 
                )
        */
        
        guchar **row_pointers = g_new0 (guchar*, vc->get_npy ());
        int stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, vc->get_npx ()); // ARGB32 or RBG24 : both 32bit wide, width
        unsigned char *data = g_new0 (unsigned char, stride * vc->get_npy ()); // stride * height
        for (int i=0; i<vc->get_npy (); ++i) row_pointers[i] = data + i*stride;
        
        g_message ("Encoding Video.");
        gint64 time_tic = 0;
	for (int time_index=offset_index_time; time_index<=max_index_time; ++time_index){
		for (int layer_index=offset_index_value; layer_index<=max_index_value; ++layer_index){

                        // select frame, auto scale
                        gapp->xsm->data.display.vlayer = layer_index;
                        gapp->xsm->data.display.vframe = time_index;
                        App::spm_select_layer (NULL, gapp);
                        App::spm_select_time (NULL, gapp);

                        gapp->check_events ();

                        gapp->progress_info_set_bar_fraction ((gdouble)time_index/(gdouble)max_index_time, 1);
                        gapp->progress_info_set_bar_fraction ((gdouble)layer_index/(gdouble)max_index_value, 2);

                        scan->mem2d_time_element (time_index)->SetLayer (layer_index);
                        
                        if (conti_autodisp_mode)
                                gapp->xsm->ActiveScan->auto_display ();
                        
                        cairo_surface_t *surface = cairo_image_surface_create_for_data (data, CAIRO_FORMAT_RGB24,
                                                                                        vc->get_npx (), vc->get_npy (), // width, height
                                                                                        stride);
                        cairo_t *cr = cairo_create (surface);
       
                        // call draw with widget=NULL assumed external rendering and omits drawing active frame
                        vc->canvas_draw_function (NULL, cr, 0,0, vc);
        
                        cairo_status_t cstatus = cairo_status(cr);
                        if (cstatus)
                                printf("CAIRO STATUS: %s\n", cairo_status_to_string (cstatus));
                        
                        cairo_destroy (cr);
                        cairo_surface_destroy (surface);

                        // fix fucked up RGB byte order
                        for (int k=0; k < (stride * vc->get_npy ()); k+=4){
                                unsigned char x=data[k];
                                data[k] = data[k+2];
                                data[k+2] = x;
                        }

                        // The framerate is passed as a rational number (timescale/frame_duration). E.g. for an NTSC stream, you'll choose timescale = 30000 and frame_duration = 1001. To set up multiple video tracks with different formats and/or codecs, use lqt_add_video_track .
                        // time:	Timestamp of the frame in timescale tics

                        g_message ("[%04d,%04d] Encoding Frame at %08ld", time_index, layer_index, time_tic);
			if ( lqt_encode_video(qtfile, row_pointers, 0, time_tic) != 0){
				quicktime_close (qtfile);
				cout << "Encode of Video Frame failed" << endl;
				return  FIO_NO_NAME; // Encode failed
			}
                        time_tic += frame_duration;
		}
	}
        
	g_message ("Video encoding complete. Cleaning up.");
	quicktime_close (qtfile);
        g_free (row_pointers);
        g_free (data);

	if (!OSD_grab_mode)
		gapp->progress_info_close ();

	return status=FIO_OK; 
}

// Plugin's Notify Cb's, registered to be called on file load/save to check file
// return via filepointer, it is set to Zero or passed as Zero if file has been processed!
// That's all fine, you should just change the Text Stings below...


static void quicktime_im_export_filecheck_load_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		PI_DEBUG (DBG_L2, "checking >" << *fn << "<" );

		Scan *dst = gapp->xsm->GetActiveScan();
		if(!dst){ 
			gapp->xsm->ActivateFreeChannel();
			dst = gapp->xsm->GetActiveScan();
		}
		Quicktime_ImExportFile fileobj (dst, *fn);

		FIO_STATUS ret = fileobj.Read(); 
		if (ret != FIO_OK){ 
			// I'am responsible! (But failed)
			if (ret != FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
				*fn=NULL;
			// no more data: remove allocated and unused scan now, force!
//			gapp->xsm->SetMode(-1, ID_CH_M_OFF, TRUE); 
			PI_DEBUG (DBG_L2, "Read Error " << ((int)ret) );
		}else{
			// got it!
			*fn=NULL;

			// Now update gxsm main window data fields
			gapp->xsm->ActiveScan->GetDataSet(gapp->xsm->data);
			gapp->spm_update_all();
			dst->draw();
		}
	}else{
		PI_DEBUG (DBG_L2, "Skipping" << *fn << "<" );
	}
}

static void quicktime_im_export_filecheck_save_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		Scan *src;
		PI_DEBUG (DBG_L2, "Saving/(checking) >" << *fn << "<" );

		Quicktime_ImExportFile fileobj (src = gapp->xsm->GetActiveScan(), *fn);

		FIO_STATUS ret;
		ret = fileobj.Write(); 

		if(ret != FIO_OK){
			// I'am responsible! (But failed)
			if (ret != FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
				*fn=NULL;
			PI_DEBUG (DBG_L2, "Write Error " << ((int)ret) );
		}else{
			// write done!
			*fn=NULL;
		}
	}else{
		PI_DEBUG (DBG_L2, "Skipping >" << *fn << "<" );
	}
}

// Menu callback functions -- usually no need to edit

static void quicktime_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (quicktime_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (quicktime_im_export_pi.name, "-import", NULL);
	gchar *fn = gapp->file_dialog_load (help[0], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
                quicktime_im_export_filecheck_load_callback (&fn );
                g_free (fn);
	}
}

static void quicktime_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (quicktime_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (quicktime_im_export_pi.name, "-export", NULL);
	gchar *fn = gapp->file_dialog_save (help[1], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
                quicktime_im_export_filecheck_save_callback (&fn );
                g_free (fn);
	}
}
