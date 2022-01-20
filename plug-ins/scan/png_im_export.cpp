/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: png_im_export.C
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
% PlugInDocuCaption: PNG image Import/Export
% PlugInName: png_Im_Export
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: File/Import/PNG

% PlugInDescription
The \GxsmEmph{png\_im\_export} plug-in allows reading and writing of images using the
Portable Network Graphics (PNG) image format. 

The currently set palette is used if non is used or available a grey
scale image is generated. The current set view mode
(Direct/Quick/...) is used for automatic data transformation. The resulting image will
appear like the active view but it has the size of the original scan (no zoom/quench applies).

A special feature: If the scan has type "RGBA" (4 layers) the raw data of the channels is written
without any transformation or scaling to the PNG file in RGB mode.

In case of image series/movies this export filter automatically generates a series of png images: \GxsmFile{ bild0001.png \dots bild0099.png}.
To import image series replace 0000 \dots 0099 in your file name by any valid C-format string like \GxsmFile{bild\%04d.png}.

To scale (call AutoDisp function) each frame incude the string \"autodisp\" in the export filename.

% PlugInUsage
The plug-in is called by \GxsmMenu{File/Import/PNG} and \GxsmMenu{File/Export/PNG}.

%% OptPlugInKnownBugs

% OptPlugInRefs
pnglib documentation: \GxsmWebLink{www.libpng.org/pub/png/libpng-manual.html}


% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "dataio.h"
#include "action_id.h"
#include "util.h"
#include "xsmtypes.h"
#include "glbvars.h"
#include "surface.h"

// custom includes go here
// -- START EDIT --
#include "png.h"        /* libpng header; includes zlib.h */
#include "readpng.h"    /* typedefs, common macros, public prototypes */
#include "writepng.h"    /* typedefs, common macros, public prototypes */
// -- END EDIT --

// enable std namespace
using namespace std;

// Plugin Prototypes
static void png_im_export_init (void);
static void png_im_export_query (void);
static void png_im_export_about (void);
static void png_im_export_configure (void);
static void png_im_export_cleanup (void);

static void png_im_export_filecheck_load_callback (gpointer data );
static void png_im_export_filecheck_save_callback (gpointer data );

static void png_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void png_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// Fill in the GxsmPlugin Description here
GxsmPlugin png_im_export_pi = {
        NULL,                   // filled in and used by Gxsm, don't touch !
        NULL,                   // filled in and used by Gxsm, don't touch !
        0,                      // filled in and used by Gxsm, don't touch !
        NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
        // filled in here by Gxsm on Plugin load, 
        // just after init() is called !!!
        // -- START EDIT --
        "PNG-ImExport",            // PlugIn name
        NULL,                   // PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
        // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
        g_strdup ("Im/Export PNG images."),
        "Percy Zahl",
        "file-import-section,file-export-section", // sep. im/export menuentry path by comma!
        N_("PNG,PNG"), // menu entry (same for both)
        N_("PNG import,PNG export"), // short help for menu entry
        N_("PNG import and export filter."), // info
        // -- END EDIT --
        NULL,          // error msg, plugin may put error status msg here later
        NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
        png_im_export_init,
        png_im_export_query,
        // about-function, can be "NULL"
        // can be called by "Plugin Details"
        png_im_export_about,
        // configure-function, can be "NULL"
        // can be called by "Plugin Details"
        png_im_export_configure,
        // run-function, can be "NULL", if non-Zero and no query defined, 
        // it is called on menupath->"plugin"
        NULL,
        // cleanup-function, can be "NULL"
        // called if present at plugin removal
        png_im_export_import_callback, // direct menu entry callback1 or NULL
        png_im_export_export_callback, // direct menu entry callback2 or NULL

        png_im_export_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("This GXSM plugin allows im- and export of PNG image data files");

static const char *file_mask = "*.png";

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

double realtime0=0.;
double realtime0_user=0.;

#define FILE_DIM_V 0x01
#define FILE_DIM_T 0x02
#define FILE_DIM_VIDEO 0x04
int file_dim=0;

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  png_im_export_pi.description = g_strdup_printf(N_("Gxsm im_export plugin %s"), VERSION);
  return &png_im_export_pi; 
}

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/PNG"
// Export Menupath is "File/Export/PNGt"
// ----------------------------------------------------------------------

static void png_im_export_query(void)
{
	// register this plugins filecheck functions with Gxsm now!
	// This allows Gxsm to check files from DnD, open, 
	// and cmdline sources against all known formats automatically - no explicit im/export is necessary.
	png_im_export_pi.app->ConnectPluginToLoadFileEvent (png_im_export_filecheck_load_callback);
	png_im_export_pi.app->ConnectPluginToSaveFileEvent (png_im_export_filecheck_save_callback);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void png_im_export_init(void)
{
	PI_DEBUG (DBG_L2, png_im_export_pi.name << " Plugin Init");
}

// about-Function
static void png_im_export_about(void)
{
	const gchar *authors[] = { png_im_export_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  png_im_export_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void png_im_export_configure(void)
{
	if(png_im_export_pi.app){
		XsmRescourceManager xrm("PNG_IM_EXPORT");

		xrm.Get ("file_max_index_value", &max_index_value, "1");
		xrm.Get ("file_offset_index_value", &offset_index_value, "0");
		xrm.Get ("file_step_index_value", &step_index_value, "1");
		xrm.Get ("file_start_value", &start_value, "0");
		xrm.Get ("file_step_value", &step_value, "1");

		xrm.Get ("file_max_index_time", &max_index_time, "1");
		xrm.Get ("file_offset_index_time", &offset_index_time, "0");
		xrm.Get ("file_step_index_time", &step_index_time, "1");
		xrm.Get ("file_start_time", &start_time, "0");
		xrm.Get ("file_step_time", &step_time, "1");

		xrm.Get ("file_realttime0_user", &realtime0_user, "0.");

		GtkDialogFlags flags =  (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);
		GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("PNG Multi Image Import"),
								 GTK_WINDOW (main_get_gapp()->get_app_window ()),
								 flags,
								 _("_OK"),
								 GTK_RESPONSE_ACCEPT,
								 _("_Cancel"),
								 GTK_RESPONSE_REJECT,
								 NULL);
		BuildParam bp;
		gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);

		if (file_dim & FILE_DIM_V){
                        bp.grid_add_ec ("Max Index Values", png_im_export_pi.app->xsm->Unity, &max_index_value, 1., 1e6, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Index Offset", png_im_export_pi.app->xsm->Unity, &offset_index_value, -1e6, 1e6, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Index Step", png_im_export_pi.app->xsm->Unity, &step_index_value, -1000, 1000, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Start Value", png_im_export_pi.app->xsm->Unity, &start_value, -1e6, 1e6, ".3f"); bp.new_line ();
			bp.grid_add_ec ("Step Value", png_im_export_pi.app->xsm->Unity, &step_value, -1e6, 1e6, ".3f"); bp.new_line ();
		}		

		if (file_dim & FILE_DIM_T){
			bp.grid_add_ec ("Max Index Times", png_im_export_pi.app->xsm->Unity, &max_index_time, 1., 1e6, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Index Offset", png_im_export_pi.app->xsm->Unity, &offset_index_time, -1e6, 1e6, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Index Step", png_im_export_pi.app->xsm->Unity, &step_index_time, -1000, 1000, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Start Time", png_im_export_pi.app->xsm->Unity, &start_time, -1e6, 1e6, ".3f"); bp.new_line ();
			bp.grid_add_ec ("Step Time", png_im_export_pi.app->xsm->Unity, &step_time, -1e6, 1e6, ".3f"); bp.new_line ();
		}		
		bp.grid_add_ec ("Time Origin", png_im_export_pi.app->xsm->TimeUnit, &realtime0_user, 0., 1e20, ".1f"); bp.new_line ();

		if (file_dim & FILE_DIM_VIDEO){
		}

		gtk_widget_show (dialog);

                int response = GTK_RESPONSE_NONE;
                g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
        
                // FIX-ME GTK4 ??
                // wait here on response
                while (response == GTK_RESPONSE_NONE)
                        while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

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

		xrm.Put ("file_realttime0_user", realtime0_user);
	}
}

// cleanup-Function, remove all "custom" menu entrys here!
static void png_im_export_cleanup(void)
{
#if 0
	gchar **path  = g_strsplit (png_im_export_pi.menupath, ",", 2);
	gchar **entry = g_strsplit (png_im_export_pi.menuentry, ",", 2);

	gchar *tmp = g_strconcat (path[0], entry[0], NULL);
	gnome_app_remove_menus (GNOME_APP (png_im_export_pi.app->getApp()), tmp, 1);
	g_free (tmp);

	tmp = g_strconcat (path[1], entry[1], NULL);
	gnome_app_remove_menus (GNOME_APP (png_im_export_pi.app->getApp()), tmp, 1);
	g_free (tmp);

	g_strfreev (path);
	g_strfreev (entry);

	PI_DEBUG (DBG_L2, "Plugin Cleanup done.");
#endif
}


// make a new derivate of the base class "Dataio"
class PNG_ImExportFile : public Dataio{
public:
	PNG_ImExportFile(Scan *s, const char *n) : Dataio(s,n){; };
	virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
	virtual FIO_STATUS Write();
private:
	FIO_STATUS import_data(const char *fname, int index_value=0, int index_time=0);
};

FIO_STATUS PNG_ImExportFile::Read(xsm::open_mode mode){
	FIO_STATUS ret;
	gchar *fname=NULL;

	PI_DEBUG (DBG_L2, "reading");

	fname = (gchar*)name;

	// name should have at least 4 chars: ".ext"
	if (fname == NULL || strlen(fname) < 4)
		return status=FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

	if (strncasecmp(fname+strlen(fname)-4,".png",4)){
		return status=FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
	}
 
	realtime0=0.;
	file_dim=0;
	if (strchr (fname, '%')){
		file_dim |= FILE_DIM_T;

		int index_time=0;
		ret=FIO_OK;
		png_im_export_configure ();
		main_get_gapp()->progress_info_new ("PNG Multi Image Import", 2);
		main_get_gapp()->progress_info_set_bar_fraction (0., 1);
		main_get_gapp()->progress_info_set_bar_fraction (0., 2);
		main_get_gapp()->progress_info_set_bar_text (fname, 1);
		do {
			int index_value=0;
			main_get_gapp()->progress_info_set_bar_fraction ((gdouble)index_time/(gdouble)max_index_time, 1);
			do {
				gchar *fname_expand=NULL;
				ifstream f;

				fname_expand = g_strdup_printf (fname, index_time*step_index_time+offset_index_time);
				max_index_value = 1;

				main_get_gapp()->progress_info_set_bar_fraction ((gdouble)(index_value+1)/(gdouble)max_index_value, 2);
				main_get_gapp()->progress_info_set_bar_text (fname_expand, 2);
				f.open(fname_expand, ios::in);
				if(!f.good()){
					cout << "PNG::: ERROR while processing file >" << fname_expand << "< -- Multi Image Import Aborted." << endl;
					PI_DEBUG (DBG_L2, "Error at file open. File not good/readable.");
					main_get_gapp()->progress_info_close ();
					return status=FIO_OPEN_ERR;
				}
				f.close();
				
				// Check all known File Types:
				ret = import_data (fname_expand, index_value, index_time);
				g_free (fname_expand);
				++index_value;
				
			} while (ret == FIO_OK && index_value < max_index_value);

			scan->append_current_to_time_elements (index_time, start_time + step_time*(index_time));
			scan->mem2d->remove_layer_information ();
			++index_time;

		} while (ret == FIO_OK && index_time < max_index_time);
		main_get_gapp()->progress_info_close ();
		scan->retrieve_time_element (0);
		scan->SetVM(SCAN_V_DIRECT);
		return ret;

	} else {
		// check for file exists and is OK !
		// else open File Dlg
		ifstream f;
		f.open(fname, ios::in);
		if(!f.good()){
			PI_DEBUG (DBG_L2, "Error at file open. File not good/readable.");
			return status=FIO_OPEN_ERR;
		}
		f.close();
		scan->SetVM(SCAN_V_DIRECT);
		
		// Check all known File Types:
		if ((ret=import_data (fname)) !=  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
			return ret;
		
	}
	return  status=FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
}

FIO_STATUS PNG_ImExportFile::import_data(const char *fname, int index_value, int index_time){
	FILE *infile;
	double display_exponent=0.;
	ulg image_width, image_height, image_rowbytes;
	int image_channels;
	int rc;
	
	//gboolean byteswap = FALSE;

	// Checking resposibility for this file as good as possible, use
	// extension(s) (most simple), magic numbers, etc.
	ifstream f;
	GString *FileList=NULL;

	cout << "PNG::: importing from >" << fname << "<" << endl;
	PI_DEBUG (DBG_L2, "importing from >" << fname << "<");

	if (!(infile = fopen(fname, "rb")))
	        return status=FIO_OPEN_ERR;

	// now start importing -----------------------------------------
	if ((rc = readpng_init(infile, &image_width, &image_height)) != 0) {
		gchar *error_msg = NULL;
		switch (rc) {
                case 1:
			PI_DEBUG (DBG_L2, "[" << fname << "] is not a PNG file: incorrect signature");
			error_msg = g_strdup_printf (N_("[%s] is not a PNG file: incorrect signature"),
						     fname);
			break;
                case 2:
			PI_DEBUG (DBG_L2, "[" << fname << "] has bad IHDR (libpng longjmp)");
			error_msg = g_strdup_printf (N_("[%s] has bad IHDR (libpng longjmp)"),
						     fname);
			break;
                case 4:
			PI_DEBUG (DBG_L2, "insufficient memory");
			error_msg = g_strdup_printf (N_("insufficient memory"));
			break;
                default:
			PI_DEBUG (DBG_L2, "unknown readpng_init() error");
			error_msg = g_strdup_printf (N_("unknown readpng_init() error"));
			break;
		}
   
		cout << error_msg << endl;
	
		if (error_msg && (rc > 1 || strstr (fname, ".png"))){
			GtkWidget *dialog = gtk_message_dialog_new (NULL,
								    GTK_DIALOG_DESTROY_WITH_PARENT,
								    GTK_MESSAGE_INFO,
								    GTK_BUTTONS_OK,
								    "%s", error_msg);
                        gtk_widget_show (dialog);

                        int response = GTK_RESPONSE_NONE;
                        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
        
                        // FIX-ME GTK4 ??
                        // wait here on response
                        while (response == GTK_RESPONSE_NONE)
                                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
		}
		g_free (error_msg);
		fclose (infile);
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
	}


	if (index_value == 0 && index_time == 0){
		cout << "PNG::: scan param setup" << endl;
		// reset old scan fully to defaults
		SCAN_DATA scan_template;
                //		scan->data.CpUnits (scan_template);
		scan->data.GetScan_Param (scan_template);
		scan->data.GetUser_Info (scan_template);
		scan->data.GetDisplay_Param (scan_template);

		// update as much as we get...
		time_t t; // Scan - Startzeit eintragen 
		time(&t);
		gchar *tmp = g_strconcat ((ctime(&t)), " (Imported)", NULL); scan->data.ui.SetDateOfScan (tmp); g_free (tmp);
		scan->data.ui.SetName (name);
		scan->data.ui.SetOriginalName (name);
		scan->data.ui.SetType ("PNG color image");
		scan->data.ui.SetUser ("nobody");
		
		FileList = g_string_new ("Imported by GXSM from PNG.\n");
		g_string_append_printf (FileList, "Original Filename: %s\n", name);
		g_string_append (FileList, "blaaa:\n");
		scan->data.ui.SetComment (FileList->str);
		g_string_free(FileList, TRUE); 
		FileList=NULL;
		
		
		XsmRescourceManager xrm("PNG_IM_EXPORT");
		xrm.Get ("AngPerPixelX", &scan->data.s.dx, "1.0");
		xrm.Get ("AngPerPixelY", &scan->data.s.dy, "1.0");
			
		scan->data.s.ntimes = max_index_time;
		scan->data.s.nvalues = 4;
		scan->data.s.nx = image_width;
		scan->data.s.ny = image_height;
		scan->data.s.dz = 1.;
		scan->data.s.rx = scan->data.s.nx*scan->data.s.dx;
		scan->data.s.ry = scan->data.s.ny*scan->data.s.dy;
		scan->data.s.rz = 256.0;
		scan->data.s.x0 = 0.;
		scan->data.s.y0 = 0.;
		scan->data.s.alpha = 0.;
		scan->data.display.bright = 0.;
		scan->data.display.vrange_z = 256.;
		scan->data.display.voffset_z = 0.;
		
		cout << "PNG::: resize: " << scan->data.s.nx << "x" << scan->data.s.ny << endl;
		if (scan->data.s.nx < 1 || scan->data.s.ny < 1 || scan->data.s.nx > 20000 || scan->data.s.ny > 20000){
			cerr << "PNG size error - abort." << endl;
			fclose (infile); 
			return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
		}
		scan->mem2d->Resize (scan->data.s.nx, scan->data.s.ny, ZD_RGBA);

		scan->data.orgmode = SCAN_ORG_CENTER;
		scan->mem2d->data->MkXLookup (-scan->data.s.rx/2., scan->data.s.rx/2.);
		scan->mem2d->data->MkYLookup (scan->data.s.ry/2., -scan->data.s.ry/2.);
		scan->mem2d->data->MkVLookup (0, 4);

	}

	uch *image_data = readpng_get_image(display_exponent, &image_channels, &image_rowbytes);

	uch *src;
	ulg row;
	uch r, g, b, a;
	ulg red, green, blue;

        for (row = 0;  row < image_height;  ++row) {
		src = image_data + row*image_rowbytes;
		if (image_channels == 3) {
			for (int i = 0; i < (int)image_width; ++i) {
				red   = *src++;
				green = *src++;
				blue  = *src++;
				scan->mem2d->PutDataPkt ((double)red,   i, row, 0);
				scan->mem2d->PutDataPkt ((double)green, i, row, 1);
				scan->mem2d->PutDataPkt ((double)blue,  i, row, 2);
				scan->mem2d->PutDataPkt (0.3*red + 0.59*green + 0.11*blue, i, row, 3);
			}
		} else  /* if (image_channels == 4) */ {
			for (int i = 0; i < (int)image_width; ++i) {
				r = *src++;
				g = *src++;
				b = *src++;
				a = *src++;
				scan->mem2d->PutDataPkt ((double)r, i, row, 0);
				scan->mem2d->PutDataPkt ((double)g, i, row, 1);
				scan->mem2d->PutDataPkt ((double)b, i, row, 2);
				scan->mem2d->PutDataPkt ((double)a, i, row, 3);
                                //					alpha_composite(red,   r, a, bg_red);
                                //					alpha_composite(green, g, a, bg_green);
                                //					alpha_composite(blue,  b, a, bg_blue);
			}
		}
	}
	readpng_cleanup(TRUE);
 	fclose(infile);
  	return status=FIO_OK; 
}

FIO_STATUS PNG_ImExportFile::Write(){
	mainprog_info minfo;
	unsigned char **rgb;
	Mem2d *m = NULL;
	gint ti   = 0;
	gint tnum = scan->number_of_time_elements ();

	if (name == NULL) return FIO_NO_NAME;

	do {
		gchar *png_name ;

		if (tnum > 1){
			gchar *tmp = g_strdup (name);
			*(tmp+strlen(tmp)-4) = 0;

                        //			m = scan->mem2d_time_element (ti);
			scan->retrieve_time_element (ti);
			m = scan->mem2d;
			png_name = g_strdup_printf ("%s_%05d.png", tmp, ti);
			g_free (tmp);
			++ti;
		} else {
			m = scan->mem2d;
			png_name = g_strdup (name);
		}

		minfo.gamma   = 2.2; // may use 2.2 ??
		minfo.width   = m->GetNx ();
		minfo.height  = m->GetNy ();
		minfo.modtime = time(NULL);
		minfo.infile  = NULL;
		if (! (minfo.outfile = fopen(png_name, "wb"))){
			g_free (png_name);
			return status=FIO_OPEN_ERR;
		}
		g_free (png_name);

		rgb = new unsigned char* [minfo.height];
		for (int i=0; i<minfo.height; rgb[i++] = new unsigned char[3*minfo.width]);



		if (m->GetNv() == 4){ // use direct RGB[A] from data
			int k,j;
			for (int i=0; i<minfo.height; ++i)
				for (k=j=0; j<minfo.width; ++j){
					*(rgb[i] + k++) = (unsigned char)m->GetDataPkt (j, i, 0);
					*(rgb[i] + k++) = (unsigned char)m->GetDataPkt (j, i, 1);
					*(rgb[i] + k++) = (unsigned char)m->GetDataPkt (j, i, 2);
				}
		}else{
			
			int k,j;
			int cval;
			ifstream cpal;
			char pline[256];
			int r,g,b,nx,ny;
			double val;
			int ival;
			int maxcol=1024;
			unsigned char tgapal[1024][3];

			if (main_get_gapp()->xsm->ZoomFlg & VIEW_PALETTE){
				cpal.open(xsmres.Palette, ios::in);
				PI_DEBUG (DBG_L2, "Using Palette: " << xsmres.Palette );
				if(cpal.good()){
					cpal.getline(pline, 255);
					cpal.getline(pline, 255);
					cpal >> nx >> ny;
					cpal.getline(pline, 255);
					cpal.getline(pline, 255);
				
					for(maxcol=min(nx, 1024), cval=0; cval<maxcol; ++cval){
						cpal >> r >> g >> b ;
						tgapal[cval][0] = r;
						tgapal[cval][1] = g;
						tgapal[cval][2] = b;
					}
					cpal.close();
				} else {
					PI_DEBUG (DBG_L2, "Using Palette failed, fallback to greyscale!" );
					for (maxcol=256, cval=0; cval<maxcol; ++cval)
						tgapal[cval][2] = tgapal[cval][1] = tgapal[cval][0] = cval * 255 / maxcol; 
				}
			}else{
				PI_DEBUG (DBG_L2, "Using greyscale palette!" );
				for (maxcol=256, cval=0; cval<maxcol; ++cval)
					tgapal[cval][2] = tgapal[cval][1] = tgapal[cval][0] = cval * 255 / maxcol; 
			}
			PI_DEBUG (DBG_L2, "MaxCol#: " << maxcol );
			PI_DEBUG (DBG_L2, "VFlg:    " << scan->data.display.ViewFlg );

                        if (strstr (name, "autodisp"))
                                scan->auto_display ();

			// Set View-Mode Data Range and auto adapt Vcontrast/Bright
			m->SetDataRange(0, maxcol);
			m->AutoDataSkl(NULL, NULL);

			// use GetDataVMode to use the current set View-Mode (Direct, Quick, ...)
			for (int i=0; i<minfo.height; ++i)
				for (k=j=0; j<minfo.width; ++j){
					val = m->GetDataVMode (j,i);
					ival = (int)((val >= maxcol ? maxcol-1 : val < 0 ? 0 : val) + .5);

					*(rgb[i] + k++) = (unsigned char)tgapal[ival][0];
					*(rgb[i] + k++) = (unsigned char)tgapal[ival][1];
					*(rgb[i] + k++) = (unsigned char)tgapal[ival][2];
				}
		}

		minfo.row_pointers = rgb;
		minfo.title  = g_strdup ("GXSM-PNG-ImExport-PlugIn");
		minfo.author = g_strdup ("Percy Zahl");
		minfo.desc   = g_strdup ("GXSM PNG export");
		minfo.copyright = g_strdup ("GPL");
		minfo.email   = g_strdup ("zahl@users.sourceforge.net");
		minfo.url     = g_strdup ("http://gxsminfo.sf.net");
		minfo.have_bg   = FALSE;
		minfo.have_time = FALSE;
		minfo.have_text = FALSE;
		minfo.pnmtype   = 6; // TYPE_RGB
		minfo.sample_depth = 8;
		minfo.interlaced= FALSE;

		writepng_init (&minfo);
		writepng_encode_image (&minfo);
		writepng_encode_finish (&minfo);
		writepng_cleanup (&minfo);
		fclose (minfo.outfile);

		g_free (minfo.title);
		g_free (minfo.author);
		g_free (minfo.desc);
		g_free (minfo.copyright);
		g_free (minfo.email);
		g_free (minfo.url);

		for (int i=0; i<minfo.height; delete[] rgb[i++]);
		delete[] rgb;
		
	} while (tnum > 1 && ti < tnum);
	
	// start exporting -------------------------------------------
	return status=FIO_OK; 
}

// Plugin's Notify Cb's, registered to be called on file load/save to check file
// return via filepointer, it is set to Zero or passed as Zero if file has been processed!
// That's all fine, you should just change the Text Stings below...


static void png_im_export_filecheck_load_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		PI_DEBUG (DBG_L2, "checking >" << *fn << "<" );

		Scan *dst = main_get_gapp()->xsm->GetActiveScan();
		if(!dst){ 
			main_get_gapp()->xsm->ActivateFreeChannel();
			dst = main_get_gapp()->xsm->GetActiveScan();
		}
		PNG_ImExportFile fileobj (dst, *fn);

		FIO_STATUS ret = fileobj.Read(); 
		if (ret != FIO_OK){ 
			// I'am responsible! (But failed)
			if (ret != FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
				*fn=NULL;
			// no more data: remove allocated and unused scan now, force!
                        //			main_get_gapp()->xsm->SetMode(-1, ID_CH_M_OFF, TRUE); 
			PI_DEBUG (DBG_L2, "Read Error " << ((int)ret) );
		}else{
			// got it!
			*fn=NULL;

			// Now update gxsm main window data fields
			main_get_gapp()->xsm->ActiveScan->GetDataSet(main_get_gapp()->xsm->data);
			main_get_gapp()->spm_update_all();
			dst->draw();
		}
	}else{
		PI_DEBUG (DBG_L2, "Skipping" << *fn << "<" );
	}
}

static void png_im_export_filecheck_save_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		Scan *src;
		PI_DEBUG (DBG_L2, "Saving/(checking) >" << *fn << "<" );

		PNG_ImExportFile fileobj (src = main_get_gapp()->xsm->GetActiveScan(), *fn);

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

static void png_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (png_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (png_im_export_pi.name, "-import", NULL);
	gchar *fn = main_get_gapp()->file_dialog_load (help[0], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
                png_im_export_filecheck_load_callback (&fn );
                g_free (fn);
	}
}

static void png_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (png_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (png_im_export_pi.name, "-export", NULL);
	gchar *fn = main_get_gapp()->file_dialog_save (help[1], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
                png_im_export_filecheck_save_callback (&fn );
                g_free (fn);
	}
}
