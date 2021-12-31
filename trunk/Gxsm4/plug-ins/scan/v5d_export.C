/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: v5d_export.C
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
% PlugInDocuCaption: Import/Export of old (G-) dat files
% PlugInName: v5d_Export
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: File/Import/G-dat

% PlugInDescription
\label{plugins:v5d_export}
The \GxsmEmph{g\_dat\_im\_export} plug-in supports reading and writing of
the old \GxsmFile{.dat} fileformat used in Hannover at former times. It was used by
the very first xxsm, pmstm and even older OS/2 software in Hannover, mostly by
K\"ohler et al. Also the so called \GxsmEmph{gnutools} are can use this 16-bit data format. 
To distiguish different dat files, I call it \GxsmEmph{G-dat}. 

% PlugInUsage
The plug-in is called by \GxsmMenu{File/Import/G-dat}.

% OptPlugInKnownBugs
Not yet tested.

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

// custom includes go here
// -- START EDIT --
#include "v5d_v5d.h"
#include "v5d_binio.h"
// -- END EDIT --

// enable std namespace
using namespace std;

// Plugin Prototypes
static void v5d_export_init (void);
static void v5d_export_query (void);
static void v5d_export_about (void);
static void v5d_export_configure (void);
static void v5d_export_cleanup (void);

static void v5d_export_filecheck_load_callback (gpointer data );
static void v5d_export_filecheck_save_callback (gpointer data );

static void v5d_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void v5d_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// Fill in the GxsmPlugin Description here
GxsmPlugin v5d_export_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
// -- START EDIT --
  g_strdup ("V5D-Export"),           // PlugIn name
  NULL,                   // PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("Export of the V5D (vis5d) file format."),
  "Percy Zahl",
  "file-import-section,file-export-section", // sep. im/export menuentry path by comma!
  N_("Vis5D,Vis5D"), // menu entry (same for both)
  N_("Vis5D 'v5d' file import,Vis5D 'v5d' file export"), // short help for menu entry
  N_("Vis5D export filter."), // info
// -- END EDIT --
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  v5d_export_init,
  v5d_export_query,
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  v5d_export_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  v5d_export_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL,
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  v5d_export_import_callback, // direct menu entry callback1 or NULL
  v5d_export_export_callback, // direct menu entry callback2 or NULL

  v5d_export_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("GXSM 'Vis5d' plugin for export 'v5d' data file type.\n"
	);

static const char *file_mask = "*.v5d";

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  v5d_export_pi.description = g_strdup_printf(N_("Gxsm export plugin %s"), VERSION);
  return &v5d_export_pi; 
}

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/GME Dat"
// Export Menupath is "File/Export/GME Dat"
// ----------------------------------------------------------------------
// !!!! make sure the "v5d_export_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!

static void v5d_export_query(void)
{
	if(v5d_export_pi.status) g_free(v5d_export_pi.status); 
	v5d_export_pi.status = g_strconcat (
		N_("Plugin query has attached "),
		v5d_export_pi.name, 
		N_(": File IO Filters are ready to use."),
		NULL);

	// register this plugins filecheck functions with Gxsm now!
	// This allows Gxsm to check files from DnD, open, 
	// and cmdline sources against all known formats automatically - no explicit im/export is necessary.
//	v5d_export_pi.app->ConnectPluginToLoadFileEvent (v5d_export_filecheck_load_callback);
	v5d_export_pi.app->ConnectPluginToSaveFileEvent (v5d_export_filecheck_save_callback);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void v5d_export_init(void)
{
	PI_DEBUG (DBG_L2, v5d_export_pi.name << " Plugin Init");
}

// about-Function
static void v5d_export_about(void)
{
	const gchar *authors[] = { v5d_export_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  v5d_export_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void v5d_export_configure(void)
{
	if(v5d_export_pi.app)
		v5d_export_pi.app->message("im_export Plugin Configuration");
}

// cleanup-Function, remove all "custom" menu entrys here!
static void v5d_export_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Plugin Cleanup done.");
}

// make a new derivate of the base class "Dataio"
class  v5d_ImExportFile : public Dataio{
public:
	v5d_ImExportFile(Scan *s, const char *n) : Dataio(s,n){ ; };
	virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
	virtual FIO_STATUS Write();
private:
	FIO_STATUS import_data(const char *fname); 
};

FIO_STATUS v5d_ImExportFile::Read(xsm::open_mode mode){
	FIO_STATUS ret;
	gchar *fname=NULL;

	PI_DEBUG (DBG_L2, "reading");

	fname = (gchar*)name;

	// name should have at least 4 chars: ".ext"
	if (fname == NULL || strlen(fname) < 4)
		return status=FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
 
	// check for file exists and is OK !
	// else open File Dlg
	ifstream f;
	f.open(fname, ios::in);
	if(!f.good()){
		PI_DEBUG (DBG_L2, "Error at file open. File not good/readable.");
		return status=FIO_OPEN_ERR;
	}
	f.close();

	// Check all known File Types:
	if ((ret=import_data (fname)) !=  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
		return ret;

	return  status=FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
}

FIO_STATUS v5d_ImExportFile::import_data(const char *fname){
	gboolean byteswap = FALSE;
	GtkWidget *dialog;
	int pcnt = 0;

	// Checking resposibility for this file as good as possible, use
	// extension(s) (most simple), magic numbers, etc.
	ifstream f;
	GString *FileList=NULL;

	PI_DEBUG (DBG_L2, "importing from >" << fname << "<");

	f.open(fname, ios::in);
	if (!f.good())
	        return status=FIO_OPEN_ERR;

	// reset old scan fully to defaults
	SCAN_DATA scan_template;
	scan->data.GetScan_Param (scan_template);
	scan->data.GetUser_Info (scan_template);
	scan->data.GetDisplay_Param (scan_template);

	// now start importing -----------------------------------------
	f.close ();
	return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

#if 0
	// update as much as we get...
	time_t t; // Scan - Startzeit eintragen 
	time(&t);
	gchar *tmp = g_strconcat ((ctime(&t)), " (Imported)", NULL); scan->data.ui.SetDateOfScan (tmp); g_free (tmp);
	scan->data.ui.SetName (name);
	scan->data.ui.SetOriginalName (name);
	scan->data.ui.SetType ("old G-dat");
	scan->data.ui.SetUser (Kopf.UserName);

	FileList = g_string_new ("Imported by GXSM from V5D filetype.\n");
	g_string_sprintfa (FileList, "Original Filename: %s\n", name);
	g_string_append (FileList, Kopf.comment);
	scan->data.ui.SetComment (FileList->str);
	g_string_free(FileList, TRUE); 
	FileList=NULL;

	scan->data.s.nx = Kopf.nx[pcnt];
	scan->data.s.ny = Kopf.ny[pcnt];
	scan->data.s.dx = Kopf.dx[pcnt]*Kopf.DAtoAng_X;
	scan->data.s.dy = Kopf.dy[pcnt]*Kopf.DAtoAng_Y;
	scan->data.s.dz = Kopf.DAtoAng_Z;
	scan->data.s.rx = scan->data.s.nx*scan->data.s.dx;
	scan->data.s.ry = scan->data.s.ny*scan->data.s.dy;
	scan->data.s.x0 = Kopf.x_Offset[pcnt]*Kopf.DAtoAng_X;
	scan->data.s.y0 = Kopf.y_Offset[pcnt]*Kopf.DAtoAng_Y;
	scan->data.s.alpha = (double)Kopf.Rotation;
	scan->data.display.bright = 32.; //Kopf.brightfac;
	scan->data.display.vrange_z = Contrast_to_VRangeZ (Kopf.greyfac, scan->data.s.dz);
	scan->data.display.voffset_z = 0.;
	
	scan->mem2d->Resize (scan->data.s.nx, scan->data.s.ny);
	scan->mem2d->DataRead (f);
	scan->mem2d->data->MkXLookup (-scan->data.s.rx/2., scan->data.s.rx/2.);
	scan->mem2d->data->MkYLookup (scan->data.s.ry/2., -scan->data.s.ry/2.);

	if (f.fail ()){
		f.close ();
		return status=FIO_READ_ERR; 
	}
	f.close();
  	return status=FIO_OK; 
#endif
}

FIO_STATUS v5d_ImExportFile::Write(){
	float *g;
	int it, iv, ir, ic, il;

	/** STEP 1:  The following variables must be initialized in STEP 2.  See
	 ** the README file section describing the 'v5dCreate' call for more
	 ** information.
	 **/
	int NumTimes;                      /* number of time steps */
	int NumVars;                       /* number of variables */
	int Nr, Nc, Nl[MAXVARS];           /* size of 3-D grids */
	char VarName[MAXVARS][10];         /* names of variables */
	int TimeStamp[MAXTIMES];           /* real times for each time step */
	int DateStamp[MAXTIMES];           /* real dates for each time step */
	int CompressMode;                  /* number of bytes per grid */
	int Projection;                    /* a projection number */
	float ProjArgs[100];               /* the projection parameters */
	int Vertical;                      /* a vertical coord system number */
	float VertArgs[MAXLEVELS];         /* the vertical coord sys parameters */

	int lowlev[MAXVARS];


	const gchar *outfile;

	if(strlen(name)>0)
		outfile = (const char*)name;
	else
		outfile = gapp->file_dialog("File Export: Vis5d (.v5d)"," ","*.v5d","","Vis5d write");
	if (outfile == NULL) return FIO_NO_NAME;

	// check if we like to handle this
	if (strncmp(outfile+strlen(outfile)-4,".v5d",4))
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;


	/**
	 ** STEP 2:  open your file and read the header information to initialize
	 ** the above variables.
	 **/

	// all available via gxsm core scan object!

	/** END STEP 2 **/

	NumTimes = 1; //scan->data.s.ntimes;
	NumVars  = 1;
	Nr       = scan->mem2d->GetNy () / 2;
	Nc       = scan->mem2d->GetNx () / 2;
	Nl[0]    = scan->mem2d->GetNv ();
	strcpy (VarName[0], "U");
	TimeStamp[0] = 120000; // HHMMSS  Time
	DateStamp[0] = 8001;  // YYDDD   YY:year  DDD: day of year
	CompressMode = 4;

	Projection   = 0;
	ProjArgs[0]  = 50;
	ProjArgs[1]  = 100;
	ProjArgs[2]  = 1.0;
	ProjArgs[3]  = 1.0;

	Vertical     = 1;
	for (il=0; il< Nl[0]; il++) 
		VertArgs[il] = Nl[0] - 1. * il;

//		VertArgs[il] = 1000.0 - 40.0 * il;

	/* use the v5dCreate call to create the v5d file and write the header */
	if (!v5dCreate( outfile, NumTimes, NumVars, Nr, Nc, Nl,
			VarName, TimeStamp, DateStamp, CompressMode,
			Projection, ProjArgs, Vertical, VertArgs )) {
		printf("Error: couldn't create %s\n", outfile );
		return status=FIO_WRITE_ERR; 
	}

	/*** YOU MAY CALL v5dSetLowLev() OR v5dSetUnits() HERE. SEE README FILE ***/

	/* allocate space for grid data */
	{
		int maxnl, i;
		maxnl = Nl[0];
		for (i=1;i<NumVars;i++) {
			if (Nl[i]>maxnl)
				maxnl = Nl[i];
		}
		g = (float *) malloc( Nr * Nc * maxnl * sizeof(float) );
		if (!g) {
			printf("Error: out of memory\n");
			return status=FIO_WRITE_ERR; 
		}
	}

	/*** YOU MAY CALL v5dSetLowLev() OR v5dSetUnits() HERE. SEE README FILE ***/
//	lowlev[0] = 100;
//	v5dSetLowLev(lowlev);
	
	for (it=0; it<NumTimes; it++) {

//		double ref_time = scan->mem2d_time_element(it)->get_frame_time ();
//		gapp->progress_info_set_bar_fraction ((gdouble)it/(gdouble)scan->data.s.ntimes, 2);
		
		for (iv=0;iv<NumVars;iv++) {

			/**
			 ** STEP 3:  Read your 3-D grid data for timestep it and variable
			 ** iv into the array g here.
			 ** To help with 3-D array indexing we've defined a macro G.
			 ** G(0,0,0) is the north-west-bottom corner, G(Nr-1,Nc-1,Nl-1) is
			 ** the south-east-top corner.  If you want a value to be considered
			 ** missing, assign it equal to the constant MISSING.  For example:
			 ** G(ir,ic,il) = MISSING;
			 **/
#define G(ROW, COLUMN, LEVEL)   g[ (ROW) + ((COLUMN) + (LEVEL) * Nc) * Nr ]

			for (il = 0; il < Nl[iv]; ++il)
				for (ir = 0; ir < Nr; ++ir)
					for (ic = 0; ic < Nc; ++ic){
//                                              simple MCP gain correction
						double fac = 1. + 4.* ((double)ir/Nr + 1.-(double)ic/Nc)/2.;
//						float x = (float) scan->mem2d_time_element(it)->GetDataPkt (ic,ir,2*il);
						float x = (float) fac * scan->mem2d_time_element(it)->GetDataDiscAv (ic*2,ir*2,il, 3.);
						G(ir, ic, il) = x;

//						G(ir, ic, il) = MISSING;
//						g[ir + (ic + il * Nc) * Nr] = (float) scan->mem2d_time_element(it)->GetDataPkt (ic,ir,2*il);
					}

			/** END STEP 3 **/

			/* Write data to v5d file. */
			if (!v5dWrite( it+1, iv+1, g )) {
				printf("Error while writing grid.  Disk full?\n");
				exit(1);
			}
		}
		
	}

	v5dClose();

	free (g);

	return status=FIO_OK; 
}

// Plugin's Notify Cb's, registered to be called on file load/save to check file
// return via filepointer, it is set to Zero or passed as Zero if file has been processed!
// That's all fine, you should just change the Text Stings below...


static void v5d_export_filecheck_load_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		PI_DEBUG (DBG_L2, "checking >" << *fn << "<" );

		Scan *dst = gapp->xsm->GetActiveScan();
		if(!dst){ 
			gapp->xsm->ActivateFreeChannel();
			dst = gapp->xsm->GetActiveScan();
		}
		v5d_ImExportFile fileobj (dst, *fn);

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

static void v5d_export_filecheck_save_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		Scan *src;
		PI_DEBUG (DBG_L2, "Saving/(checking) >" << *fn << "<" );

		v5d_ImExportFile fileobj (src = gapp->xsm->GetActiveScan(), *fn);

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

static void v5d_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (v5d_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (v5d_export_pi.name, "-import", NULL);
	gchar *fn = gapp->file_dialog_load (help[0], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
	        v5d_export_filecheck_load_callback (&fn );
                g_free (fn);
	}
}

static void v5d_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (v5d_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (v5d_export_pi.name, "-export", NULL);
	gchar *fn = gapp->file_dialog_save (help[1], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
	        v5d_export_filecheck_save_callback (&fn );
                g_free (fn);
	}
}
