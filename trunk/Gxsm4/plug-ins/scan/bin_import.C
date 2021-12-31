/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: binary_impor.C
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
% PlugInDocuCaption: Binary file Import
% PlugInName: bin_import
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: File/Import/Binary

% PlugInDescription
\label{plugins:bin_import}
The \GxsmEmph{Binary} plug-in supports reading of a simple binary volumetric data file format.

Data Set File Format:

The data files for rectilinearly sampled scalar data are written in
the following format (all fields big-endian binary): 

Resolution (number of grid points in x, y and z direction): three
32-bit int values. Let's refer to them as numX, numY, numZ.

Size of saved border around volume: one 32-bit int value. This value
is not used in the provided data sets and is set to zero.

True size (extent in x, y and z direction in some unit of
measurement): three 32-bit float values. Treat these fields like a
sort of "3D aspect ratio" - usually, medical data sets are sampled as
a stack of slices, where the distances between slices is different
from the distances between pixels in a slice.

Data values: All numX*numY*numZ data values of the volume stored as
unsigned char values in the range (0 .. 256). The values are stored in
x, y, z order, i.e., x varies slowest, z varies fastest. In other
words, they are stored in the memory order of a standard C
three-dimensional array unsigned char values\[numX\]\[numY\]\[numZ\].

% PlugInUsage
The plug-in is called by \GxsmMenu{File/Import/Binary}. 
Only import direction is implemented.

%% OptPlugInKnownBugs
%Not yet tested.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"
#include "core-source/dataio.h"
#include "core-source/action_id.h"
#include "core-source/util.h"
#include "batch.h"
#include "fileio.c"

using namespace std;

// Plugin Prototypes
static void bin_import_init (void);
static void bin_import_query (void);
static void bin_import_about (void);
static void bin_import_configure (void);
static void bin_import_cleanup (void);

static void bin_import_filecheck_load_callback (gpointer data );
static void bin_import_filecheck_save_callback (gpointer data );

static void bin_import_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void bin_import_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// Fill in the GxsmPlugin Description here
GxsmPlugin bin_import_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  "Bin_Import",
  NULL,                   // PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("Im/Export of the Binary data file format."),
  "Percy Zahl",
  "file-import-section,file-export-section",
  N_("Binary,Binary"),
  N_("Binary import,Binary export"),
  N_("Binary data file import and export filter."),
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  bin_import_init,
  bin_import_query,
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  bin_import_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  bin_import_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL,
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  bin_import_import_callback, // direct menu entry callback1 or NULL
  bin_import_export_callback, // direct menu entry callback2 or NULL

  bin_import_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("GXSM binary data file Import/Export Plugin\n\n");

static const char *file_mask = "*";

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  bin_import_pi.description = g_strdup_printf(N_("Gxsm im_export plugin %s"), VERSION);
  return &bin_import_pi; 
}

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/GME Dat"
// Export Menupath is "File/Export/GME Dat"
// ----------------------------------------------------------------------
// !!!! make sure the "bin_import_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!

static void bin_import_query(void)
{
	gchar **entry = g_strsplit (bin_import_pi.menuentry, ",", 2);

	if(bin_import_pi.status) g_free(bin_import_pi.status); 
	bin_import_pi.status = g_strconcat (
		N_("Plugin query has attached "),
		bin_import_pi.name, 
		N_(": File IO Filters are ready to use."),
		NULL);
	
	// clean up
	g_strfreev (entry);

// register this plugins filecheck functions with Gxsm now!
// This allows Gxsm to check files from DnD, open, 
// and cmdline sources against all known formats automatically - no explicit im/export is necessary.
	bin_import_pi.app->ConnectPluginToLoadFileEvent (bin_import_filecheck_load_callback);
	bin_import_pi.app->ConnectPluginToSaveFileEvent (bin_import_filecheck_save_callback);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void bin_import_init(void)
{
	PI_DEBUG (DBG_L2, bin_import_pi.name << "Plugin Init");
}

// about-Function
static void bin_import_about(void)
{
	const gchar *authors[] = { bin_import_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  bin_import_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void bin_import_configure(void)
{
	if(bin_import_pi.app)
		bin_import_pi.app->message("bin_import Plugin Configuration");
}

// cleanup-Function, make sure the Menustrings are matching those above!!!
static void bin_import_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Plugin Cleanup done.");
}

// make a new derivate of the base class "Dataio"
class binary_ImExportFile : public Dataio{
public:
	binary_ImExportFile(Scan *s, const char *n) : Dataio(s,n){ };
	virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
	virtual FIO_STATUS Write();
private:
	FIO_STATUS import(const char *fname);
};

FIO_STATUS binary_ImExportFile::Read(xsm::open_mode mode){
	FIO_STATUS ret;
	gchar *fname=NULL;

	fname = (gchar*)name;

	// name should have at least 4 chars: ".ext"
	if (fname == NULL || strlen(fname) < 4)
		return  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
 
	// check for file exists and is OK !
	// else open File Dlg
	ifstream f;
	f.open(fname, ios::in | ios::binary);
	if(!f.good()){
		PI_DEBUG (DBG_L2, "Error at file open. File not good/readable.");
		return status=FIO_OPEN_ERR;
	}
	f.close();

	// Check all known File Types:
	if ((ret=import (fname)) !=  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
		return ret;

	return  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
}

FIO_STATUS binary_ImExportFile::import(const char *fname){
        struct bin_header {
                guint32 dimensions_xyzb[4];
                guint be_extends_xyz[3];
        } h;
        gfloat extends_xyz[3];
        guint8 *data;

	// Am I resposible for that file -- can only do a dimension sanity check
	ifstream f;
	GString *FileList=NULL;

	f.open(name, ios::in);
	if (!f.good())
	        return status=FIO_OPEN_ERR;
	
	FileList = g_string_new ("Imported by GXSM from simple Binay data file.\n");

        
        f.read ((char*)&h, sizeof (bin_header));

        for (int i=0; i<4; ++i)
                h.dimensions_xyzb[i] = GUINT32_FROM_BE(h.dimensions_xyzb[i]);
        for (int i=0; i<3; ++i){
                guint32 tmp = GUINT32_FROM_BE(h.be_extends_xyz[i]);
                memcpy ((void*)&extends_xyz[i], (void*)&tmp, sizeof (gfloat));
        }
        g_message ("Binary Read. Filename: %s\nDim-XYZB: %d %d %d %d\nExtends-XYZ: %f %f %f",
                   fname,
                   (int)h.dimensions_xyzb[0], (int)h.dimensions_xyzb[1], (int)h.dimensions_xyzb[2], h.dimensions_xyzb[3], 
                   (double)extends_xyz[0],  (double)extends_xyz[1],  (double)extends_xyz[2]);
        
	// read header
	if ( f.good ()){
                g_string_append_printf (FileList, "Original Filename: %s\nDim-XYZB: %d %d %d %d\nExtends-XYZ: %f %f %f", fname,
                                   (int)h.dimensions_xyzb[0], (int)h.dimensions_xyzb[1], (int)h.dimensions_xyzb[2], h.dimensions_xyzb[3], 
                                   (double)extends_xyz[0],  (double)extends_xyz[1],  (double)extends_xyz[2]
                                   );
                if ((double)h.dimensions_xyzb[0] * (double)h.dimensions_xyzb[1] * (double)h.dimensions_xyzb[2] > 1e10){
                        f.close ();
                        return status=FIO_NOT_RESPONSIBLE_FOR_THAT_FILE; // possibly wrong. Arbitrary limits. Adjust if needed.
                }
	} else {
	        return status=FIO_OPEN_ERR;
        }

        size_t buffer_size =  (size_t)h.dimensions_xyzb[0] * (size_t)h.dimensions_xyzb[1] * (size_t)h.dimensions_xyzb[2];
        data = g_new (guint8, buffer_size);
        f.read ((char*)data, sizeof (guint8) * buffer_size);

	if (! f.good ()){
                g_warning ("Binary data read EOF.");
        }

        f.close ();

        // data[numX][numY][numZ]

	time_t t; // Scan - Startzeit eintragen 
	time(&t);
	gchar *tmp = g_strconcat ((ctime(&t)), " (Imported)", NULL); scan->data.ui.SetDateOfScan (tmp); g_free (tmp);
	scan->data.ui.SetName (fname);
	scan->data.ui.SetOriginalName (fname);
	scan->data.ui.SetType ("Binary Volume"); 


	// put some usefull values in the ui structure
	if(getlogin()){
		scan->data.ui.SetUser (getlogin());
	}
	else{
		scan->data.ui.SetUser ("unkonwn user");
	}

	// this is mandatory.
	// initialize scan structure -- this is a minimum example
	scan->data.s.ntimes  = 1;
	scan->data.s.nx = h.dimensions_xyzb[1];
	scan->data.s.ny = h.dimensions_xyzb[2];
	scan->data.s.nvalues = h.dimensions_xyzb[0];
	scan->data.s.dx = 1./extends_xyz[1]; // need Angstroems
	scan->data.s.dy = 1./extends_xyz[2];
	scan->data.s.dz = 1./extends_xyz[0];
	scan->data.s.rx = scan->data.s.dx * scan->data.s.nx;
	scan->data.s.ry = scan->data.s.dy * scan->data.s.ny;
	scan->data.s.rz = scan->data.s.dz * scan->data.s.nvalues;
	scan->data.s.x0 = 0.;
	scan->data.s.y0 = 0.;
	scan->data.s.alpha = 0.;

	// be nice and reset this to some defined state
	scan->data.display.z_high       = 0.;
	scan->data.display.z_low        = 0.;

	// set the default view parameters
	scan->data.display.bright = 32.;
	scan->data.display.contrast = 1.0;

	// FYI: (PZ)
	//  scan->data.display.vrange_z  = ; // View Range Z in base ZUnits
	//  scan->data.display.voffset_z = 0; // View Offset Z in base ZUnits
	//  scan->AutoDisplay([...]); // may be used too...
  
	UnitObj *u = gapp->xsm->MakeUnit ("um", "X");
	scan->data.SetXUnit(u);
	delete u;

	u = gapp->xsm->MakeUnit ("um", "Y");
	scan->data.SetYUnit(u);
	delete u;

	u = gapp->xsm->MakeUnit ("um", "Z");
	scan->data.SetZUnit(u);
	delete u;

	scan->data.ui.SetComment (FileList->str);
	g_string_free(FileList, TRUE); 
	FileList=NULL;

        // Read Img Data.
        scan->mem2d->Resize (scan->data.s.nx, scan->data.s.ny, scan->data.s.nvalues, ZD_FLOAT);

        // convert data
        guint8 *p = data;
        for (int v=scan->mem2d->GetNv ()-1; v>=0; --v)
                for (int col=0; col < scan->mem2d->GetNx (); ++col)
                        for (int row=0; row < scan->mem2d->GetNy (); ++row)
                                scan->mem2d->PutDataPkt ((double)*p++, col, row, v);

        g_free (data);
        
	scan->data.orgmode = SCAN_ORG_CENTER;
	scan->mem2d->data->MkXLookup (-scan->data.s.rx/2., scan->data.s.rx/2.);
	scan->mem2d->data->MkYLookup (scan->data.s.ry/2., -scan->data.s.ry/2.);
	scan->mem2d->data->MkVLookup (scan->data.s.rz/2., -scan->data.s.rz/2.);
  
	return FIO_OK; 
}


FIO_STATUS binary_ImExportFile::Write(){
#if 0
	GtkWidget *dialog = gtk_message_dialog_new (NULL,
						    GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_MESSAGE_INFO,
						    GTK_BUTTONS_OK,
						    N_("Binary export.")
						    );
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
#endif
	const gchar *fname;
	ofstream f;

	if(strlen(name)>0)
		fname = (const char*)name;
	else
		fname = gapp->file_dialog("File Export: Binary"," ",file_mask,"","Binary write");
	if (fname == NULL) return FIO_NO_NAME;

	// check if we like to handle this
	if (strncmp(fname+strlen(fname)-4,".asc",4))
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

	f.open(name, ios::out);
	if (!f.good())
	        return status=FIO_OPEN_ERR;

	for (int row=0; row < scan->mem2d->GetNy (); ++row){
		for (int col=0; col < scan->mem2d->GetNx (); ++col)
			f << scan->mem2d->GetDataPkt (col, row) << " ";
		f << std::endl;
	}

	f.close ();

	return FIO_OK; 
}

// Plugin's Notify Cb's, registered to be called on file load/save to check file
// return via filepointer, it is set to Zero or passed as Zero if file has been processed!
// That's all fine, you should just change the Text Stings below...


static void bin_import_filecheck_load_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		PI_DEBUG (DBG_L2, 
			  "Check File: bin_import_filecheck_load_callback called with >"
			  << *fn << "<" );

		Scan *dst = gapp->xsm->GetActiveScan();
		if(!dst){ 
			gapp->xsm->ActivateFreeChannel();
			dst = gapp->xsm->GetActiveScan();
		}
		binary_ImExportFile fileobj (dst, *fn);

		FIO_STATUS ret = fileobj.Read(); 
		if (ret != FIO_OK){ 
			// I'am responsible! (But failed)
			if (ret != FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
				*fn=NULL;
			// no more data: remove allocated and unused scan now, force!
//			gapp->xsm->SetMode(-1, ID_CH_M_OFF, TRUE); 
			PI_DEBUG (DBG_L2, "Read Error " << ((int)ret) << "!!!!!!!!" );
		}else{
			// got it!
			*fn=NULL;

			// Now update gxsm main window data fields
			gapp->xsm->ActiveScan->GetDataSet(gapp->xsm->data);
			gapp->spm_update_all();
			dst->draw();
		}
	}else{
		PI_DEBUG (DBG_L2, "bin_import_filecheck_load: Skipping" );
	}
}

static void bin_import_filecheck_save_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		Scan *src;
		PI_DEBUG (DBG_L2,
			  "Check File: bin_import_filecheck_save_callback called with >"
			  << *fn << "<" );

		binary_ImExportFile fileobj (src = gapp->xsm->GetActiveScan(), *fn);

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
		PI_DEBUG (DBG_L2, "bin_import_filecheck_save: Skipping >" << *fn << "<" );
	}
}

// Menu Call Back Fkte

static void bin_import_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (bin_import_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (bin_import_pi.name, "-import", NULL);
	gchar *fn = gapp->file_dialog_load (help[0], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
	        bin_import_filecheck_load_callback (&fn );
                g_free (fn);
	}
}

static void bin_import_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (bin_import_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (bin_import_pi.name, "-export", NULL);
	gchar *fn = gapp->file_dialog_save (help[1], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
	  	bin_import_filecheck_save_callback (&fn );
                g_free (fn);
	}
}
