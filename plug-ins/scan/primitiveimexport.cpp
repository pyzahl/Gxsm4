/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: primitive_im_export.C
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
% PlugInDocuCaption: Import/export of non-SPM file formats

% PlugInName: primitiveimexport

% PlugInAuthor: Percy Zahl

% PlugInAuthorEmail: zahl@users.sf.net

% PlugInMenuPath: File/Import/Primitive Auto Import and File/Export/Primitive Auto Export

% PlugInDescription
 This plug-in is responsible for importing and exporting a wide variety of
 file formats.  Most of them are file formats not specifically designed for
 storage of scientific data such as the Portable Graymap (pgm) format.
 Nethertheless, the support of these formats allows to exchange data between
 \Gxsm\ and countless other programs, e.g.\ The GIMP for the final "touch up"
 of images.

% OptPlugInSubSection: Supported file formats

 The file formats supported by the \GxsmEmph{primitiveimexport} plug-in are
 listed in Tab.\ \ref{tab:primitiveimexport:formats}.  The ".dat" format
 refers to the old, STM specific data format used by the OS/2 software PMSTM
 and the Linux based Xxsm, both predecessors of \Gxsm.  The ".d2d" format is
 used for SPA-LEED by the SPA V program.  The primitive formats ".byt",
 ".sht", ".flt", and ".dbl" are described in more detail below.

 \begin{table}
 \begin{tabular}[h]{l|l}
 Suffix & description \\\hline\hline
 .dat   & (old) STM data format, used by e.g.\ PMSTM, dicontinued.\\
 .byt   & byte format: raw 8 Bit data\\
 .sht   & Short format: raw 16 Bit data\\
 .flt   & Float format: floats, single precision\\
 .dbl   & Double format: floats, double precision\\
 .pgm   & Portable Greymap (P5) format\\
 \hline\hline
 \multicolumn{2}{l}{Read-only supported formats:}\\
 \hline
 .d2d   & (old) SPA--LEED data format, used by SPA V\\
 .nsc   & a very old version of nanoscope import filter (obsolete),\\
 & use the new Nanoscope Import PlugIn!\\
 .h16   & Markus/Mats simulations program data import \\
 \hline\hline
 \multicolumn{2}{l}{Write-only supported formats:}\\
 \hline
 .tga   & TARGA bitmap format (8, 16 and 24 Bit colordepth)\\
 & 24bit: only option to write color images (usage of palette)\\
 \end{tabular}  
 \caption{file formats supported by the \GxsmEmph{primitiveimexport}
 plug-in.}
 \label{tab:primitiveimexport:formats}
 \end{table}

% OptPlugInSubSection: The primitive file formats
 The primitive file formats ".byt", ".sht", ".flt", and ".dbl" are supported
 by the \GxsmEmph{primitiveimexport} plug-in for both reading and writing.
 These file formats have a simple structure and therefore present a
 considerable alternative to more complex formats for quickly hacked
 programs.  They are binary files in PC byte order starting with two 16 bit
 integer numbers denoting the number of rows and columns in the image.
 Following them, the image data is written in raw binary form using the
 following data types to represent a single pixel:  char (".byt"), 16 bit
 integer (".sht"), single precision floating point (".flt"), or double
 precision floating point (".dbl") numbers.

 For the manipulation of data in these formats, the \Gxsm\ project provides
 an extensive set of small tools included in the \Gxsm\ source code.  For
 instance, these tools include programs to create, filter, and doing simple
 algebra operations.  All these tools are designed for the use in shell
 scripts.

% PlugInUsage
 The file formats are recognized by their filename suffix. For instance,
 exporting to \GxsmFile{myimage.pgm} results in a greymap image in the binary
 Portable Greymap (pgm) format.  For other supported formats and their
 correspondent suffixes see Tab.\ \ref{tab:primitiveimexport:formats}.

% OptPlugInHints
 It is always a good idea to do a background correction before
 exporting to any 8 bit format, including 24 bit color exports as well.
 Then switch to the direct view mode before exporting. If you are not
 exporting to a 24bit TARGA file, please also switch off the usage of a
 palette to assure a correct grey scaleing.\\
%
 The 16 bit TARGA bitmap format looks quite weird when watched at in an
 image viewer.  It physically outputs 24 bit TARGA files using only the
 red and green values (16 of the 24 bits).  This format is intended for
 rendering images using the POVRAY raytracer which is able to
 interprete it as a height profile with the full 16 bit resolution.\\
%
 There is also a tool \GxsmFile{nctopng} available, which by default
 creates small icons using the \GxsmFile{.png} (Portable Network Graphics)
 format, but using it with a commandline option (\GxsmFile{nctopng gxsmncfile.nc myimage.png width\_in\_pixels})
 full size images can be created as well.
%\begin{verbatim}
%pzahl@charon:~$ nctopng --help
%Usage: nctopng gxsmncfile.nc myimage.png 
%       [new_nx] [-v | --verbose]
%\end{verbatim}

%% OptPlugInKnownBugs

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "dataio.h"
#include "action_id.h"
#include "util.h"
#include "glbvars.h"
#include "surface.h"

#include "batch.h"
#include "g_dat_types.h"
#include "fileio.c"


using namespace std;


#define IMGMAXCOLORS 64

// Plugin Prototypes
static void primitive_im_export_init (void);
static void primitive_im_export_query (void);
static void primitive_im_export_about (void);
static void primitive_im_export_configure (void);
static void primitive_im_export_cleanup (void);

static void primitive_im_export_filecheck_load_callback (gpointer data );
static void primitive_im_export_filecheck_save_callback (gpointer data );

static void primitive_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void primitive_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// Fill in the GxsmPlugin Description here
GxsmPlugin primitive_im_export_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	"Primitive_Im_Export",
	NULL,
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	g_strdup ("Im/Export of primitive data formats: all gnu, pgm, tga, mats"),
	"Percy Zahl",
	"file-import-section,file-export-section", // sep. im/export menuentry path by comma!
	N_("primitive auto,primitive auto"), // menu entry (same for both)
	NULL,
	"no more info",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	primitive_im_export_init,  
	primitive_im_export_query,  
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	primitive_im_export_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	primitive_im_export_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL,
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
	primitive_im_export_import_callback, // direct menu entry callback1 or NULL
	primitive_im_export_export_callback, // direct menu entry callback2 or NULL

	primitive_im_export_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm primitve Data File Import/Export Plugin\n\n"
                                   "This plugin reads in a datafiles of various\n"
                                   "primitive formats:\n"
                                   "GNUs: .byt, .sht, .flt, .dbl, but not: .cpx\n"
                                   "The simple PGM format (8bit grey)\n"
                                   "Export only: TGA, TGA16 (for povray), TGA Color\n"
                                   "MATS format\n"
                                   ".ncd (very old DI NanoScope format)"
        );

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
	primitive_im_export_pi.description = g_strdup_printf(N_("Gxsm primitive_im_export plugin %s"), VERSION);
	return &primitive_im_export_pi; 
}

// Query Function, installs Plugin's features in Menupath(s)!
static void primitive_im_export_query(void)
{

	if(primitive_im_export_pi.status) g_free(primitive_im_export_pi.status); 
	primitive_im_export_pi.status = g_strconcat(N_("Plugin query has attached "),
						    primitive_im_export_pi.name, 
						    N_(": File IO Filters are ready to use"),
						    NULL);

	primitive_im_export_pi.app->ConnectPluginToLoadFileEvent (primitive_im_export_filecheck_load_callback);
	primitive_im_export_pi.app->ConnectPluginToSaveFileEvent (primitive_im_export_filecheck_save_callback);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//

// configure-Function
static void multidim_im_export_configure(void)
{
	if(primitive_im_export_pi.app){
		XsmRescourceManager xrm("PRIMITIVE_IM_EXPORT");

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
		GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("Primitive Image Data Im-/Export"),
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
                        bp.grid_add_ec ("Max Index Values", primitive_im_export_pi.app->xsm->Unity, &max_index_value, 1., 1e6, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Index Offset", primitive_im_export_pi.app->xsm->Unity, &offset_index_value, -1e6, 1e6, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Index Step", primitive_im_export_pi.app->xsm->Unity, &step_index_value, -1000, 1000, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Start Value", primitive_im_export_pi.app->xsm->Unity, &start_value, -1e6, 1e6, ".3f"); bp.new_line ();
			bp.grid_add_ec ("Step Value", primitive_im_export_pi.app->xsm->Unity, &step_value, -1e6, 1e6, ".3f"); bp.new_line ();
		}		

		if (file_dim & FILE_DIM_T){
			bp.grid_add_ec ("Max Index Times", primitive_im_export_pi.app->xsm->Unity, &max_index_time, 1., 1e6, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Index Offset", primitive_im_export_pi.app->xsm->Unity, &offset_index_time, -1e6, 1e6, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Index Step", primitive_im_export_pi.app->xsm->Unity, &step_index_time, -1000, 1000, ".0f"); bp.new_line ();
			bp.grid_add_ec ("Start Time", primitive_im_export_pi.app->xsm->Unity, &start_time, -1e6, 1e6, ".3f"); bp.new_line ();
			bp.grid_add_ec ("Step Time", primitive_im_export_pi.app->xsm->Unity, &step_time, -1e6, 1e6, ".3f"); bp.new_line ();
		}		
		bp.grid_add_ec ("Time Origin", primitive_im_export_pi.app->xsm->TimeUnit, &realtime0_user, 0., 1e20, ".1f"); bp.new_line ();

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


// init-Function
static void primitive_im_export_init(void)
{
	PI_DEBUG (DBG_L2, "primitive_im_export Plugin Init" );
}

// about-Function
static void primitive_im_export_about(void)
{
	const gchar *authors[] = { primitive_im_export_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  primitive_im_export_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void primitive_im_export_configure(void)
{
	if(primitive_im_export_pi.app)
		primitive_im_export_pi.app->message("primitive_im_export Plugin Configuration");
}

// cleanup-Function
static void primitive_im_export_cleanup(void)
{
	PI_DEBUG (DBG_L2, "primitive_im_export Plugin Cleanup" );
#if 0
	gnome_app_remove_menus (GNOME_APP (primitive_im_export_pi.app->getApp()),
				N_("File/Import/Primitive Auto Import"), 1);
	gnome_app_remove_menus (GNOME_APP (primitive_im_export_pi.app->getApp()),
				N_("File/Export/Primitive Auto Export"), 1);
#endif
}

class PrimitiveImExportFile : public Dataio{
public:
	PrimitiveImExportFile(Scan *s, const char *n) : Dataio(s,n){ };
	virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
	virtual FIO_STATUS Write();
private:
	FIO_STATUS gnuRead(const char *fname, int index_value=0, int index_time=0);
	FIO_STATUS d2dRead(const char *fname, int index_value=0, int index_time=0);
	FIO_STATUS nscRead(const char *fname, int index_value=0, int index_time=0);
	FIO_STATUS matsRead(const char *fname, int index_value=0, int index_time=0);
};
// ==================================================
// GNU File Types Extension
// and tga export

// f\FCr TGA Export

typedef struct { short k[6], x,y, pixsz; } TGA_head;
typedef struct { unsigned char val[3]; } TGA_pix;



// d2d import :=)
FIO_STATUS PrimitiveImExportFile::Read(xsm::open_mode mode){
        FIO_STATUS ret;
        gchar *fname=NULL;

        PI_DEBUG (DBG_L2, "Primitive Import erreicht" );

        fname = (gchar*)name;

        // name should have at least 4 chars: ".ext"
        if (fname == NULL || strlen(fname) < 4)
                return  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
 
        // check for file exists and is OK !
        // else open File Dlg

	realtime0=0.;
	file_dim=0;
	if (strchr (fname, '%')){
		file_dim |= FILE_DIM_V;

		gchar *tmp = g_strdup(fname);
		* (strchr (tmp, '%')) = 'X';
		if (strchr (tmp, '%'))
			file_dim |= FILE_DIM_T;

		int index_time=0;
		ret=FIO_OK;
		multidim_im_export_configure ();
		main_get_gapp()->progress_info_new ("PRIM.XXX Multi Image Import", 2);
		main_get_gapp()->progress_info_set_bar_fraction (0., 1);
		main_get_gapp()->progress_info_set_bar_fraction (0., 2);
		main_get_gapp()->progress_info_set_bar_text (fname, 1);
		do {
			int index_value=0;
			main_get_gapp()->progress_info_set_bar_fraction ((gdouble)index_time/(gdouble)max_index_time, 1);
			do {
				gchar *fname_expand=NULL;
				ifstream f;
				if (file_dim == 1){
					fname_expand = g_strdup_printf (fname, index_value*step_index_value+offset_index_value);
					max_index_time = 1;
				} else
					fname_expand = g_strdup_printf (fname, 
									index_time*step_index_time+offset_index_time,
									index_value*step_index_value+offset_index_value);
				main_get_gapp()->progress_info_set_bar_fraction ((gdouble)(index_value+1)/(gdouble)max_index_value, 2);
				main_get_gapp()->progress_info_set_bar_text (fname_expand, 2);
				f.open(fname_expand, ios::in);
				if(!f.good()){
					cout << "PRIM.XXX::: ERROR while processing file >" << fname_expand << "< -- Multi Image Import Aborted." << endl;
					PI_DEBUG (DBG_L2, "Error at file open. File not good/readable.");
					main_get_gapp()->progress_info_close ();
					return status=FIO_OPEN_ERR;
				}
				f.close();
				
				// Check all known File Types and read...:

				// Mats ".h16" type?
				ret=matsRead (fname_expand, index_value, index_time);
				
				// SPA4 ".d2d" type?
				ret=d2dRead (fname_expand, index_value, index_time);
				
				// very old DI NanoScope ".nsc" type?
				ret=nscRead (fname_expand, index_value, index_time);
				
				// one of the "GNU" (.byt, .sht, .dat, .flt, .dbl, not .cpx, .pgm) types?
				ret=gnuRead (fname_expand, index_value, index_time);
	
				g_free (fname_expand);
				++index_value;
				
			} while (ret == FIO_OK && index_value < max_index_value);

			// app in time
			scan->append_current_to_time_elements (index_time, start_time + step_time*(index_time));
			scan->mem2d->remove_layer_information ();
			++index_time;

		} while (ret == FIO_OK && index_time < max_index_time);
		main_get_gapp()->progress_info_close ();
		scan->retrieve_time_element (0);
		scan->SetVM(SCAN_V_DIRECT);
		return ret;

	} else {

		ifstream f;
		f.open(fname, ios::in);
		if(!f.good()){
			PI_DEBUG (DBG_L2, "File Fehler" );
			return status=FIO_OPEN_ERR;
		}
		f.close();

		// Check all known File Types:
		
		// Mats ".h16" type?
		if ((ret=matsRead (fname)) !=  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
			return ret;
		
		// SPA4 ".d2d" type?
		if ((ret=d2dRead (fname)) !=  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
			return ret;
		
		// very old DI NanoScope ".nsc" type?
		if ((ret=nscRead (fname)) !=  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
			return ret;
		
		// one of the "GNU" (.byt, .sht, .dat, .flt, .dbl, not .cpx, .pgm) types?
		if ((ret=gnuRead (fname)) !=  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
			return ret;
	}

        return  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
}

FIO_STATUS PrimitiveImExportFile::gnuRead(const char *fname, int index_value, int index_time){
// f\FCr Gnu-Paket:
	FILETYPE ft=BYTFIL;
	int i,j;
	long nb, anz;
	char *buf;
	unsigned char *pb;
	short *ps;
	float *pf;
	double *pd;
	FILE *fh;
	FILEHEAD fhead;   /* union  class */
	// const char *fname;
	char *cfname;
	int image_width, image_height;
  
	// Am I resposible for that file, is it one of the ""GNU"" formats ?
	if (strncmp (fname+strlen(fname)-4,".sht",4) &&
	    strncmp (fname+strlen(fname)-4,".dat",4) &&
	    strncmp (fname+strlen(fname)-4,".flt",4) &&
	    strncmp (fname+strlen(fname)-4,".dbl",4) &&
	    strncmp (fname+strlen(fname)-4,".byt",4) &&
	    strncmp (fname+strlen(fname)-4,".pgm",4)    )
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

	if (strncmp (fname+strlen(fname)-4,".dat",4) == 0){
		// make sure it's not a GME-Dat file!!
		ifstream f;
		gchar line[64];
		f.open (name, ios::in);
		if (!f.good ())
			return status=FIO_OPEN_ERR;
	  
		f.getline(line, 64);
		if (strncmp (line, "[Parameter]", 11) == 0){ // check for a GME Dat. 16bit (old) file
			f.close ();
			return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
		}
		if (strncmp (line, "[Paramco32]", 11) == 0){ // check for a GME Dat. 32bit zipped file
			f.close ();
			return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
		}
		if (strncmp (line, "UKSOFT2001", 10) == 0){ // check for a UKSOFT Dat. file
			f.close ();
			return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
		}
	}

	PI_DEBUG (DBG_L2, "It's a gnu File!" );
  
	cfname = strdup (fname);
	nb = FileCheck (cfname, &ft, &fhead, &fh);

	if (nb<=0)
		return FIO_NO_GNUFILE;
 
	if (nb > (4096*4096)) // just a savety limit, may be removed (no .dat valid magic available)
		return FIO_NO_GNUFILE;

	PI_DEBUG (DBG_L2, "nb=" << nb );
  
	// load additional Info from File name.xxx.sklinfo or /tmp/labinfo if exists
	ifstream labf;
	char *infoname = new char[strlen(cfname)+10];
	strcpy(infoname,cfname);
	strcat(infoname, ".sklinfo");
  
	free(cfname);
  
	labf.open(infoname,ios::in);
	PI_DEBUG (DBG_L2, "Looking for:" << infoname );
	if(labf.good()){
		PI_DEBUG (DBG_L2, "LABI:" << infoname );
	} else {
		labf.open("/tmp/labinfo.asc",ios::in);
		if(labf.good())
			PI_DEBUG (DBG_L2, "LABI:" << "/tmp/labinfo" );
	}
  
	struct { int valid; double xmin,xmax,ymin,ymax; int ydir; int xn,yn; char labx[40], laby[40], tit[40], prec[20]; } sklinfo;
	sklinfo.valid = 0;
	sklinfo.ydir  = 1;
	if(labf.good()){
		labf >> sklinfo.labx >> sklinfo.xmin >> sklinfo.xmax >> sklinfo.xn;
		labf >> sklinfo.laby >> sklinfo.ymin >> sklinfo.ymax >> sklinfo.yn;
		labf.getline(sklinfo.tit, sizeof(sklinfo.tit)); // rest of line...
		labf.getline(sklinfo.tit, sizeof(sklinfo.tit));
		if(sklinfo.ymin > sklinfo.ymax){
			double ytmp;
			sklinfo.ydir = -1;
			ytmp = sklinfo.ymax;
			sklinfo.ymax = sklinfo.ymin;
			sklinfo.ymin = ytmp;
		}
		PI_DEBUG (DBG_L2, "LabDataX:" << sklinfo.labx << ": " << sklinfo.xmin << " .. " << sklinfo.xmax );
		PI_DEBUG (DBG_L2, "LabDataY:" << sklinfo.laby << ": " << sklinfo.ymin << " .. " << sklinfo.ymax );
	}
  
	if(( buf = (char*)salloc(nb,1)) == 0){
		XSM_SHOW_ALERT(ERR_SORRY, ERR_NOMEM," ",1);
		return FIO_NO_MEM;
	}
  
	if (FileRead (fh, buf, nb) != 0) {
		XSM_SHOW_ALERT(ERR_SORRY, ERR_FILEREAD,fname,1);
		sfree (buf);
		return FIO_READ_ERR;
	}
  
	if(ft == CPXFIL){
		XSM_SHOW_ALERT(ERR_SORRY, ERR_NOGNUCPX,fname,1);
		sfree (buf);
		return FIO_NO_GNUFILE;
	}
	if (ft == DATFIL){
		anz = (long)fhead.kopf.nx[0]*(long)fhead.kopf.ny[0];
		
		image_width = (int)fhead.kopf.nx[0];
		image_height = (int)fhead.kopf.ny[0];
	}
	else{
		anz = (long)fhead.xydim.x*(long)fhead.xydim.y;
		image_width  = (int)fhead.xydim.x;
		image_height = (int)fhead.xydim.y;

		PI_DEBUG (DBG_L2, "nanz,x,y=" << anz << " " 
			  << (int)fhead.xydim.x << " " << (int)fhead.xydim.y << " " );
	}
  


	if (index_value == 0 && index_time == 0){
		cout << "PRIM.XXX::: scan param setup" << endl;
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
		scan->data.ui.SetType ("PRIMITIVE image data");
		scan->data.ui.SetUser ("nobody");
		
		XsmRescourceManager xrm("PRIMITIVE_IM_EXPORT");
		xrm.Get ("AngPerPixelX", &scan->data.s.dx, "1.0");
		xrm.Get ("AngPerPixelY", &scan->data.s.dy, "1.0");
			
		scan->data.s.ntimes = max_index_time;
		scan->data.s.nvalues = max_index_value;
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
		
		cout << "PRIM.XXX::: resize: " << scan->data.s.nx << "x" << scan->data.s.ny << endl;
		if (scan->data.s.nx < 1 || scan->data.s.ny < 1 || scan->data.s.nx > 20000 || scan->data.s.ny > 20000){
			cerr << "unreasonable size error - abort." << endl;
			return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
		}
		scan->mem2d->Resize (scan->data.s.nx, scan->data.s.ny, max_index_value,
				     ft == FLTFIL ? ZD_FLOAT :
				     ft == DBLFIL ? ZD_DOUBLE :
				     ZD_SHORT
                        );

		scan->data.orgmode = SCAN_ORG_CENTER;
		scan->mem2d->data->MkXLookup (-scan->data.s.rx/2., scan->data.s.rx/2.);
		scan->mem2d->data->MkYLookup (scan->data.s.ry/2., -scan->data.s.ry/2.);

	}

	scan->mem2d->SetLayer(index_value);

	pb = (unsigned char*)buf;
	ps = (short*)buf;
	pf = (float*)buf;
	pd = (double*)buf;
	for(j=0; j<scan->data.s.ny; j++)
		for(i=0; i<scan->data.s.nx; i++)
			switch(ft){
			case BYTFIL: case PGMFIL: 
				scan->mem2d->PutDataPkt((SHT)(*pb++), i, sklinfo.ydir>0? j:scan->data.s.ny-1-j); break;
			case DATFIL: case SHTFIL: 
				scan->mem2d->PutDataPkt((SHT)(*ps++), i, sklinfo.ydir>0? j:scan->data.s.ny-1-j);
				break;
			case FLTFIL: 
				scan->mem2d->PutDataPkt(*pf++, i, sklinfo.ydir>0? j:scan->data.s.ny-1-j); break;
			case DBLFIL: 
				scan->mem2d->PutDataPkt(*pd++, i, sklinfo.ydir>0? j:scan->data.s.ny-1-j); break;
				//      case CPXFIL: mem2d->PutDataPkt((CNT)(*pd++), i, sklinfo.ydir>0? j:scan->data.s.ny-1-j); break;
			default: break;
			}
	sfree(buf);
  
	if(getlogin()){
		scan->data.ui.SetUser (getlogin());
	}
	else{
		scan->data.ui.SetUser ("unkonwn user");
	}
	time_t t; // Scan - Startzeit eintragen 
	time(&t);
	gchar *tmp = g_strconcat ((ctime(&t)), " (Imported)", NULL); scan->data.ui.SetDateOfScan (tmp); g_free (tmp);
	scan->data.ui.SetName (fname);
	scan->data.ui.SetOriginalName (fname);
	switch(ft){
	case BYTFIL: scan->data.ui.SetType ("Gnu Type: BYT "); break;
	case PGMFIL: scan->data.ui.SetType ("Gnu Type: PGM "); break;
	case DATFIL: scan->data.ui.SetType ("Gnu Type: DAT "); break;
	case SHTFIL: scan->data.ui.SetType ("Gnu Type: SHT "); break;
	case FLTFIL: scan->data.ui.SetType ("Gnu Type: FLT "); break;
	case DBLFIL: scan->data.ui.SetType ("Gnu Type: DBL "); break;
	case CPXFIL: scan->data.ui.SetType ("Gnu Type: CPX "); break;
	default: scan->data.ui.SetType ("Gnu Type: ??? "); break;
	}
  
	if(labf.good()){
		labf.close();
		gchar *tmp=g_strconcat(scan->data.ui.comment, " + Skl.Info from File", NULL);
		scan->data.ui.SetComment (tmp);
		g_free(tmp);
		scan->data.s.rx = sklinfo.xmax-sklinfo.xmin;
		scan->data.s.ry = sklinfo.ymax-sklinfo.ymin;
		scan->data.s.dx = (sklinfo.xmax-sklinfo.xmin)/scan->data.s.nx;
		scan->data.s.dy = (sklinfo.ymin-sklinfo.ymax)/scan->data.s.ny;
		scan->data.s.dz = 1;
		scan->data.s.x0 = sklinfo.xmin;
		scan->data.s.y0 = sklinfo.ymax;
		scan->data.s.alpha = 0;
		scan->mem2d->data->MkYLookup(sklinfo.ymax, sklinfo.ymin);
		scan->mem2d->data->MkXLookup(sklinfo.xmin, sklinfo.xmax);
		tmp=g_strconcat (scan->data.ui.comment, 
				 "Imported by xxsm from\n",
				 fname, "\n",
				 sklinfo.tit,
				 NULL);
		scan->data.ui.SetComment (tmp);
		g_free(tmp);
	}
	else{
		gchar *tmp=g_strconcat (scan->data.ui.comment, 
					"Imported by xxsm from\n", fname,
					NULL);
		scan->data.ui.SetComment (tmp);
		g_free(tmp);
		scan->data.s.dx = 1;
		scan->data.s.dy = 1;
		scan->data.s.dz = 1;
		scan->data.s.rx = scan->data.s.nx;
		scan->data.s.ry = scan->data.s.ny;
		scan->data.s.x0 = 0;
		scan->data.s.y0 = 0;
		scan->data.s.alpha = 0;
		scan->mem2d->data->MkXLookup(-scan->data.s.rx/2., scan->data.s.rx/2.);
		scan->mem2d->data->MkYLookup(scan->data.s.ry/2., -scan->data.s.ry/2.);
	}
	scan->data.display.z_high       = 1e3;
	scan->data.display.z_low        = 1.;
  
	switch(ft){
	case BYTFIL: case PGMFIL:
		scan->data.display.bright = 0.;
		scan->data.display.contrast = 0.25;
		break;
	default:
		scan->data.display.bright = 32.;
		scan->data.display.contrast = 0.1;
		break;
	}
	scan->data.display.vrange_z  = scan->data.s.dz*256.;
	scan->data.display.voffset_z = 0.;

	// gnu read done.
	return FIO_OK; 
}


FIO_STATUS PrimitiveImExportFile::d2dRead(const char *fname, int index_value, int index_time){
	D2D_SPA_DAT spa4header;
	ifstream f;

	// Am I resposible for that file, is it a "d2d" ?
	if (strncasecmp(fname+strlen(fname)-3, "d2d", 3))
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
    
	f.open(fname, ios::in);
	if(!f.good())
		return FIO_OPEN_ERR;


	f.read((char*)&spa4header, sizeof(D2D_SPA_DAT));
	f.seekg(0x180, ios::beg); // Auf Datenanfang Positionieren
  
	scan->data.s.ntimes  = 1;
	scan->data.s.nvalues = 1;
	scan->data.s.nx = spa4header.PointsX;
	scan->data.s.ny = spa4header.PointsY;
	scan->data.s.x0 = spa4header.Xo;
	scan->data.s.y0 = spa4header.Yo;
	scan->data.s.rx = spa4header.XYdist  ;
	scan->data.s.ry = spa4header.XYdist*scan->data.s.ny/scan->data.s.nx;
	scan->data.s.dx = scan->data.s.rx / scan->data.s.nx;
	scan->data.s.dy = scan->data.s.ry / scan->data.s.ny;
	scan->data.s.alpha = spa4header.Alpha  ;
	scan->data.s.GateTime   = spa4header.GateTime*1e-3;
	scan->data.display.z_high         = spa4header.CpsHigh;
	scan->data.display.z_low          = spa4header.CpsLow;
	scan->data.s.Energy     = spa4header.Energy ;
	//  spa->data.s.Focus1 = spa4header.Focus1 ;
	//  spa->data.s.Current = spa4header.Current;
	//  spa->data.s.dS=spa->data.s.usr[0]  = spa4header.usr1;
	//  spa->data.s.d0=spa->data.s.usr[1]  = spa4header.usr2;
	//  spa->data.s.usr[2]  = spa4header.usr3;
	//  spa->data.s.usr[3]  = spa4header.usr4;
	//  spa->data.s.Scanmarker = spa4header.Scanmarker;
	scan->data.ui.SetDateOfScan (spa4header.dateofscan);
	gchar *tmp = g_strdup_printf ("%s\n E=%6.1feV G=%6.1fms\nCPS[%g:%g]\n"
				      "Focus=%g, Current=%g\nusr1..4: %g, %g, %g, %g",
				      spa4header.comment,
				      spa4header.Energy, 
				      spa4header.GateTime, 
				      spa4header.CpsLow, spa4header.CpsHigh,
				      spa4header.Focus1, spa4header.Current,
				      spa4header.usr1, spa4header.usr2, spa4header.usr3, spa4header.usr4
		);
  
	scan->data.ui.SetComment (tmp);
	g_free (tmp);

	//  if(*(spa4header.comment+59)=='*');
	//     strcpy(spa->data.s.dateofscan, spa4header.comment+60);
	scan->data.ui.SetUser ("D2D");
	scan->data.ui.SetName (fname);
	scan->data.ui.SetOriginalName (fname);
	scan->data.ui.SetType ("D2D type");
  
	scan->mem2d->Resize(scan->data.s.nx, scan->data.s.ny);
	// load Paraemter and Data
	scan->mem2d->DataD2DRead(f, spa4header.GateTime);
	if(f.fail()){
		f.close();
		return status=FIO_READ_ERR; 
	}
	f.close();
	scan->mem2d->data->MkXLookup(-scan->data.s.rx/2., scan->data.s.rx/2.);
	scan->mem2d->data->MkYLookup(scan->data.s.ry/2., -scan->data.s.ry/2.);
	return FIO_OK; 
}

// Nanoscope Import
FIO_STATUS PrimitiveImExportFile::nscRead(const char *fname, int index_value, int index_time){
	int i,j; 
	FIO_STATUS ret=FIO_OK;
	unsigned long npix;
	short *buf, *ps;
	double size=0., zscale=0., zmag;
	gchar line[128];

	// Am I resposible for that file, is it a "nsc" plain short ?
	if (strncasecmp(fname+strlen(fname)-3, "nsc", 3))
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

	FILE *f = fopen(fname,"r");
	for(i=0; i<145; i++){
		if (! fgets(line, 127, f))
			return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
		if(strncmp(line, "\\Samps/line: ",12) == 0){
			printf("%s\n",line);
			scan->data.s.nx = atoi(&line[12]);
		}
		if(strncmp(line, "\\Number of lines: ",17) == 0){
			printf("%s\n",line);
			scan->data.s.ny = atoi(&line[17]);
		}
		if(strncmp(line, "\\Scan size: ",11) == 0){
			printf("%s\n",line);
			size = atof(&line[11]);
		}
		if(strncmp(line, "\\Z scale: ",10) == 0){
			printf("%s\n",line);
			zscale = atof(&line[10]);
		}
		if(strncmp(line, "\\Z magnify image: ",18) == 0){
			printf("%s\n",line);
			zmag = atof(&line[18]);
		}
	}
        g_message("PrimitiveImExportFile::nscRead zmag=%g", zmag);
	fclose(f);

	if(!(buf = g_new(short, npix = scan->data.s.nx*scan->data.s.ny)))
		return FIO_NO_MEM;

	f = fopen(fname,"r");
	fseek(f, 0x2000L, SEEK_SET);
	if(fread((void*)buf, sizeof(short), npix, f) < npix){
		fprintf(stderr,"In File corrupt / too short !\n");
		//    tmp=info; info=g_str_concat(tmp, "In File corrupt / too short !\n");
		ret=FIO_NSC_ERR;
	}
	fclose(f);

	scan->mem2d->Resize(scan->data.s.nx,
			    scan->data.s.ny,
			    ZD_SHORT
		);

	scan->data.s.ntimes  = 1;
	scan->data.s.nvalues = 1;
  
	ps=buf;
	for(j=0; j<scan->data.s.ny; j++)
		for(i=0; i<scan->data.s.nx; i++)
			scan->mem2d->PutDataPkt(*ps++, i, j);
  
	g_free(buf);

	if(getlogin()){
		scan->data.ui.SetUser (getlogin());
	}
	else{
		scan->data.ui.SetUser ("unkonwn user");
	}
	time_t t; // Scan - Startzeit eintragen 
	time(&t);
	gchar *tmp = g_strconcat ((ctime(&t)), " (Imported)", NULL); scan->data.ui.SetDateOfScan (tmp); g_free (tmp);
	scan->data.ui.SetName (fname);
	scan->data.ui.SetOriginalName (fname);
	scan->data.ui.SetType ("Nanoscope Type "); 
	tmp=g_strconcat (scan->data.ui.comment,
			 "Imported by gxsm from\n",
			 fname, 
			 NULL);
	scan->data.ui.SetComment (tmp);
	g_free(tmp);
  
	scan->data.s.dx = size*10./(double)scan->data.s.nx;
	scan->data.s.dy = size*10./(double)scan->data.s.ny;
	scan->data.s.dz = zscale*10.;
	scan->data.s.rx = size*10.;
	scan->data.s.ry = size*10.;
	scan->data.s.x0 = 0;
	scan->data.s.y0 = 0;
	scan->data.s.alpha = 0;
	scan->mem2d->data->MkXLookup(-scan->data.s.rx/2., scan->data.s.rx/2.);
	scan->mem2d->data->MkYLookup(scan->data.s.ry/2., -scan->data.s.ry/2.);

	scan->data.display.bright = 32.;
	scan->data.display.contrast = 0.1;
	scan->data.display.ViewFlg = SCAN_V_QUICK;

	scan->data.display.vrange_z  = scan->data.s.dz*256.;
	scan->data.display.voffset_z = 0.;

	return ret; 
}

FIO_STATUS PrimitiveImExportFile::matsRead(const char *fname, int index_value, int index_time){
        int i,j;
        long npix;
        short *buf, *ps;
        ifstream f;
        
        // Am I resposible for that file, is it a "Mats" plain short ?
        if (strncasecmp(fname+strlen(fname)-3, "h16", 3))
                return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
        
        f.open(fname, ios::in);
        if(!f.good())
                return FIO_OPEN_ERR;
        
        double num=256.;
        main_get_gapp()->ValueRequest("Enter X Size", "# X-Point", "Number of Points in X Direction for Matsfile",
                           main_get_gapp()->xsm->Unity, 2., 4096., ".0f", &num);
        scan->data.s.nx=(int)num;
        
        num=128.;
        main_get_gapp()->ValueRequest("Enter Y Size", "# Y-Point", "Number of Points in Y Direction for Matsfile",
                           main_get_gapp()->xsm->Unity, 2., 4096., ".0f", &num);
        scan->data.s.ny=(int)num;
        
        
        if(!(buf = g_new(short, npix = scan->data.s.nx*scan->data.s.ny)))
                return FIO_NO_MEM;
        
        f.read((char*)buf, sizeof(short)*npix);
        
        
        scan->mem2d->Resize(scan->data.s.nx,
                            scan->data.s.ny,
                            ZD_SHORT
                );
        
        scan->data.s.ntimes  = 1;
        scan->data.s.nvalues = 1;
        
        ps=buf;
        for(j=0; j<scan->data.s.ny; j++)
                for(i=0; i<scan->data.s.nx; i++)
                        scan->mem2d->PutDataPkt(*ps++, i, j);
        
        g_free(buf);
        
        if(getlogin()){
                scan->data.ui.SetUser (getlogin());
        }
        else{
                scan->data.ui.SetUser ("unkonwn user");
        }
        time_t t; // Scan - Startzeit eintragen 
        time(&t);
        gchar *tmp = g_strconcat ((ctime(&t)), " (Imported)", NULL); scan->data.ui.SetDateOfScan (tmp); g_free (tmp);
        scan->data.ui.SetName (fname);
        scan->data.ui.SetOriginalName (fname);
        scan->data.ui.SetType ("Mats Type "); 
        tmp=g_strconcat (scan->data.ui.comment,
                         "Imported by gxsm from\n",
                         fname, 
                         NULL);
        scan->data.ui.SetComment (tmp);
        g_free(tmp);
        
        scan->data.s.dx = 1;
        scan->data.s.dy = 1;
        scan->data.s.dz = 1;
        scan->data.s.rx = scan->data.s.nx;
        scan->data.s.ry = scan->data.s.ny;
        scan->data.s.x0 = 0;
        scan->data.s.y0 = 0;
        scan->data.s.alpha = 0;

	scan->data.orgmode = SCAN_ORG_CENTER;
	scan->mem2d->data->MkXLookup (-scan->data.s.rx/2., scan->data.s.rx/2.);
	scan->mem2d->data->MkYLookup (scan->data.s.ry/2., -scan->data.s.ry/2.);
        
        scan->data.display.bright = 0.;
        scan->data.display.contrast = 4.;
        scan->data.display.ViewFlg = SCAN_V_DIRECT;

        scan->data.display.vrange_z  = scan->data.s.dz*100.;
        scan->data.display.voffset_z = 0.;

        return FIO_OK; 
}




FIO_STATUS PrimitiveImExportFile::Write(){
	FILETYPE ft;
	int MapMode=1;
	int i,j,Nx,Ny;
	long nb, PixSize;
	char *buf;
	char *pb;
	short *ps;
	float *pf;
	double *pd;
	FILEHEAD fhead;   /* union  class */
	const char *fname;
	double val=0.;
	int maxcol=1024;
	unsigned char tgapal[1024][3];
	PixSize=0;

	if(strlen(name)>0)
		fname = (const char*)name;
	else
		fname = main_get_gapp()->file_dialog("File Export: all GNU, PGM, TGA"," ","*.[FfBbDdSsCcPpTt][LlYyAaHhPpBbGg][TtXxLlMmAa]","","gnuwrite");
	if (fname == NULL) return FIO_NO_NAME;
	if (strncmp(fname+strlen(fname)-4,".sht",4) == 0)
		ft=SHTFIL, PixSize=sizeof(short);
	else
		if (strncmp(fname+strlen(fname)-4,".flt",4) == 0)
			ft=FLTFIL, PixSize=sizeof(float);
		else
			if (strncmp(fname+strlen(fname)-4,".dbl",4) == 0)
				ft=DBLFIL, PixSize=sizeof(double);
			else
				if (strncmp(fname+strlen(fname)-4,".byt",4) == 0)
					ft=BYTFIL, PixSize=sizeof(char);
				else
					if (strncmp(fname+strlen(fname)-4,".pgm",4) == 0)
						ft=PGMFIL, PixSize=sizeof(char);
					else
						if (strncmp(fname+strlen(fname)-4,".tga",4) == 0)
							ft=TGAFIL, PixSize=sizeof(short);
						else{
							XSM_SHOW_ALERT(ERR_SORRY, ERR_WRONGGNUTYPE,fname,1);
							return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
						}
	nb = (Nx=scan->mem2d->GetNx()) * (Ny=scan->mem2d->GetNy()) * PixSize;
	if(( buf = (char*)salloc(nb,1)) == 0){
		XSM_SHOW_ALERT(ERR_SORRY, ERR_NOMEM," ",1);
		return FIO_NO_MEM;
	}

	pb = (char*)buf;
	ps = (short*)buf;
	pf = (float*)buf;
	pd = (double*)buf;

#define MAXGREYS 255
#define GFAC     (double)((MAXGREYS+1)/IMGMAXCOLORS)

	if(scan->data.display.ViewFlg == SCAN_V_DIRECT && ft == TGAFIL){
		MapMode=1+XSM_SHOW_CHOICE(Q_DIRECTEXPORT, fname, Q_DIRECTMODE, 3, L_8bit, L_16bit, L_24bit, 1);
	}
	else
		switch(ft){
		case PGMFIL:
		case BYTFIL: MapMode=1; break;
		default: MapMode=2; break;
		}

	if(MapMode == 3){
		int cval;
  
		ifstream cpal;
		char pline[256];
		int r,g,b,nx,ny;
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
		}
		else{
			PI_DEBUG (DBG_L2, "Using Palette faild, fallback to greyscale!" );
			for (maxcol=64, cval=0; cval<maxcol; ++cval)
				tgapal[cval][2] = tgapal[cval][1] = tgapal[cval][0] = cval * 255 / maxcol; 
		}
		PI_DEBUG (DBG_L2, "MapMode: " << MapMode );
		PI_DEBUG (DBG_L2, "MaxCol#: " << maxcol );
		PI_DEBUG (DBG_L2, "VFlg:    " << scan->data.display.ViewFlg );
	}

	for(j=0; j<Ny; j++)
		for(i=0; i<Nx; i++){
			switch(scan->data.display.ViewFlg){ 
			case SCAN_V_QUICK:
				val=((val=(GFAC*(scan->mem2d->GetDataPktLineReg(i,j)*scan->data.display.contrast+scan->data.display.bright))) > MAXGREYS ? MAXGREYS : val < 0 ? 0 : val);
				break;
			case SCAN_V_DIRECT:
				if(MapMode==2) // 16 bit Pov
					val=scan->mem2d->GetDataPkt(i,j); 
				else
					if(MapMode==1) // 8 bit
						val=((val=(GFAC*(scan->mem2d->GetDataPkt(i,j)*scan->data.display.contrast+scan->data.display.bright))) > MAXGREYS ? MAXGREYS : val < 0 ? 0 : val);
					else // 24bit
						val=((val=(scan->mem2d->GetDataPkt(i,j)*scan->data.display.contrast+scan->data.display.bright)) >= maxcol ? maxcol-1 : val < 0 ? 0 : val);
				break;
			case SCAN_V_PERIODIC:
				val=(SHT)(GFAC*(scan->mem2d->GetDataPkt(i,j)*scan->data.display.contrast+scan->data.display.bright)) & MAXGREYS;
				break;
			case SCAN_V_HORIZONTAL:
				val=(val=GFAC*(scan->mem2d->GetDataPktHorizont(i,j)*scan->data.display.contrast+scan->data.display.bright)) > MAXGREYS ? MAXGREYS : val < 0 ? 0 : val;
				break;
			default: // primitive
				//      val=(scan->mem2d->GetDataPkt(i,j))&MAXGREYS; 
				break;
			}

			switch(ft){
			case BYTFIL: case PGMFIL: *pb++ = (char)val; break;
				//      case SHTFIL: *ps++ = (short)((double)(mem2d->GetDataPkt(i, j) - CntLow)*65535./CntRange - 32767.); break;
			case TGAFIL:
			case SHTFIL: *ps++ = (short)val; break;
			case FLTFIL: *pf++ = (float)scan->mem2d->GetDataPkt(i,j); break;
			case DBLFIL: *pd++ = (double)scan->mem2d->GetDataPkt(i,j); break;
				//      case CPXFIL: mem2d->PutDataPkt((CNT)(*pd++), i, j); break;
			default: break;
			}
		}
	if(ft == TGAFIL){
		ofstream f;
		TGA_head TGAhead;
		TGA_pix *TGAline;
		long LineSize;
		unsigned short Val;
		/* Speicher f\FCr eine Zeile */
		TGAline = (TGA_pix *) malloc( LineSize=Nx*sizeof(TGA_pix));
		if (TGAline == NULL) {
			return FIO_NO_MEM;
		}
		memset(TGAline, 0, LineSize);
    
		/* open file, schreibe Header */
		f.open(fname, ios::out | ios::trunc);
		if(!f.good()){
			sfree (buf);
			return FIO_WRITE_ERR;
		}

		TGAhead.k[0]=0; TGAhead.k[1]=2;
		TGAhead.k[2]=0; TGAhead.k[3]=0;
		TGAhead.k[4]=0; TGAhead.k[5]=0;
		TGAhead.pixsz = 0x2018;
		TGAhead.x = (short)Nx;
		TGAhead.y = (short)Ny;
		if (WORDS_BIGENDIAN){
			PI_DEBUG (DBG_L2, "TGA Export, fix of WORDS_BIGENDIAN!!" );
			// correct byteorder
			unsigned char *swp = (unsigned char*)&TGAhead;
			for(unsigned int ii=0; ii<sizeof(TGA_head); ii+=2){
				unsigned char tmp = *swp;
				swp[0] = swp[1];
				swp[1] = tmp;
				swp += 2;
			}
		}
		f.write((char *)&TGAhead,sizeof(TGA_head));

		ps = (short*)buf;
		for(i=0;i<Ny;i++){
			switch(scan->data.display.ViewFlg){ 
			case SCAN_V_QUICK:
				for(j=0;j<Nx;j++){
					Val = (unsigned short)(*ps++);
					(TGAline+j)->val[0] = (unsigned char)(Val&0x00ff);
					(TGAline+j)->val[1] = (unsigned char)(Val&0x00ff);
					(TGAline+j)->val[2] = (unsigned char)(Val&0x00ff);
				}
				break;
			case SCAN_V_DIRECT:
				if(MapMode==2){ // 16bit f\FCr Povray
					for(j=0;j<Nx;j++){
						Val = (unsigned short)((long)(-(*ps++))+32767L);
						(TGAline+j)->val[1] = (unsigned char)(Val&0x00ff);
						(TGAline+j)->val[2] = (unsigned char)((Val>>8)&0x00ff);
					}
				}
				else
					if(MapMode==3){ // 24bit True Color
						for(j=0;j<Nx;j++){
							Val = (unsigned short)(*ps++);
							if(Val>=maxcol) Val=maxcol-1;
							(TGAline+j)->val[0] = tgapal[Val][2];
							(TGAline+j)->val[1] = tgapal[Val][1];
							(TGAline+j)->val[2] = tgapal[Val][0];
						}
					}
					else // 8bit Grey
						for(j=0;j<Nx;j++){
							Val = (unsigned short)(*ps++);
							(TGAline+j)->val[0] = (unsigned char)(Val&0x00ff);
							(TGAline+j)->val[1] = (unsigned char)(Val&0x00ff);
							(TGAline+j)->val[2] = (unsigned char)(Val&0x00ff);
						}
				break;
			}
			f.write((char*)TGAline, LineSize); 
    
			if(f.fail()){
				f.close();
				return FIO_WRITE_ERR; // FIO_DISK_FULL
			}
		}
		f.close();
		free(TGAline); 
	}
	else{
		fhead.xydim.x = Nx;
		fhead.xydim.y = Ny;
    
		if(FileWrite((char*)fname, buf, &fhead) != 0){
			sfree (buf);
			return FIO_WRITE_ERR;
		}
	}
	sfree(buf);

	return FIO_OK; 
}

// Plugin's Notify Cb's, registeres to be called on file load/save to check file
// return via filepointer, it is set to Zero or passed as Zero if file has been processed!

static void primitive_im_export_filecheck_load_callback (gpointer data ){
	gchar **fn = (gchar**)data;
        if (*fn){
                PI_DEBUG (DBG_L2, "Check File: primitive_im_export_filecheck_load_callback called with >"
			  << *fn << "<" );

                Scan *dst = main_get_gapp()->xsm->GetActiveScan();
                if(!dst){ 
                        main_get_gapp()->xsm->ActivateFreeChannel();
                        dst = main_get_gapp()->xsm->GetActiveScan();
                }
                PrimitiveImExportFile fileobj (dst, *fn);

                FIO_STATUS ret = fileobj.Read(); 
                if (ret != FIO_OK){ 
                        // I'am responsible! (But failed)
                        if (ret != FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
                                *fn=NULL;
                        // no more data: remove allocated and unused scan now, force!
//                        main_get_gapp()->xsm->SetMode(-1, ID_CH_M_OFF, TRUE); 
                        PI_DEBUG (DBG_L2, "primitive_im_export_filecheck_load: Read Error " << ((int)ret) << "!!!!!!!!" );
                }else{
                        // got it!
                        *fn=NULL;
                        // Now update

                        dst->GetDataSet(main_get_gapp()->xsm->data);
                        main_get_gapp()->spm_update_all();
                        dst->draw();
                        PI_DEBUG (DBG_L2, "primitive_im_export_filecheck_load: got it!" );
                }
        }else{
                PI_DEBUG (DBG_L2, "primitive_im_export_filecheck_load: Skipping" );
        }
}

static void primitive_im_export_filecheck_save_callback (gpointer data ){
	gchar **fn = (gchar**)data;
        if (*fn){
                Scan *src;
                PI_DEBUG (DBG_L2, "Check File: primitive_im_export_filecheck_save_callback called with >"
			  << *fn << "<" );

                PrimitiveImExportFile fileobj (src = main_get_gapp()->xsm->GetActiveScan(), *fn);

                FIO_STATUS ret;
                ret = fileobj.Write(); 

                if(ret != FIO_OK){
                        // I'am responsible! (But failed)
                        if (ret != FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
                                *fn=NULL;
                        PI_DEBUG (DBG_L2, "Write Error " << ((int)ret) << "!!!!!!!!" );
                }else{
                        // write done!
                        *fn=NULL;
                }
        }else{
                PI_DEBUG (DBG_L2, "primitive_im_export_filecheck_save: Skipping >" << *fn << "<" );
        }
}

// Menu Cb Fkt

static void primitive_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
//                                "known extensions: pgm h16 nsc d2d dat sht byt flt dbl",
        gchar *fn = main_get_gapp()->file_dialog_load ("Primitive Auto Import", NULL,
					    "*.[PpHhNnDdSsBbFf][Gg1Ss2AaHhYyLlBb][Mm6CcDdTtLl]", 
					    NULL);
	if (fn){
	        PI_DEBUG (DBG_L2, "FLDLG-IM::" << fn );
		primitive_im_export_filecheck_load_callback (&fn );
		g_free (fn);
	}
}

static void primitive_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
//                                    "known extensions: pgm tga dat sht byt flt dbl",
        gchar *fn = main_get_gapp()->file_dialog_save ("Primitive Auto Export", NULL,
					    "*.[PpTtDdSsBbFf][GgAaHhYyLlBb][MmAaTtLl]",
					    NULL);
	if (fn){
	        PI_DEBUG (DBG_L2, "FLDLG-EX::" << fn );
		primitive_im_export_filecheck_save_callback (&fn );
		g_free (fn);
	}
}
