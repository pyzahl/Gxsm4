/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: rhk_spm32_import.C
 * ========================================
 * 
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * additional features: Andreas Klust <klust@users.sf.net>
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
% PlugInDocuCaption: Import of RHK SPM32 files (STM-1000 electronics).
% PlugInName: rhk_spm32_import
% PlugInAuthor: Andreas klust
% PlugInAuthorEmail: klust@users.sf.net
% PlugInMenuPath: File/Import/RHK SPM32

% PlugInDescription 
\label{plugins:rhk_spm32_import} 

The \GxsmEmph{rhk\_spm32\_import} plug-in supports reading of files
generated with the SPM32 software distributed by RHK Technology Inc.\
(Troy, MI, USA).  This is the software that is usually used to control
the RHK STM-1000 electronics (unless you use \Gxsm to control it :-).

% PlugInUsage

The plug-in is called by \GxsmMenu{File/Import/RHK SPM32}.  It is also
automatically invoked by \Gxsm when opening RHK SPM32 files.

% OptPlugInKnownBugs

Not yet all very well tested.  At the moment, it is somewhat
restricted: i) SPM32 files can contain several pages with data from,
e.g., forward and backward scans.  The plug-in only reads the first
page.  ii) only pages with pure images can be imported, no
spectroscopy data.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------
 */



#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "dataio.h"
#include "action_id.h"
#include "glbvars.h"
#include "surface.h"


using namespace std;

#define IMGMAXCOLORS 64

// Plugin Prototypes
static void rhk_spm32_import_init (void);
static void rhk_spm32_import_query (void);
static void rhk_spm32_import_about (void);
static void rhk_spm32_import_configure (void);
static void rhk_spm32_import_cleanup (void);

static void rhk_spm32_import_filecheck_load_callback (gpointer data );
static void rhk_spm32_import_filecheck_save_callback (gpointer data );

static void rhk_spm32_import_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void rhk_spm32_import_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// Fill in the GxsmPlugin Description here
GxsmPlugin rhk_spm32_import_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  "RHK_SPM32_Import",
  NULL,                   // PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  NULL,
  "Percy Zahl",
  "file-import-section,file-export-section",
  N_("RHK SPM32,RHK SPM32"),
  N_("Import RHK SPM32 files."),
  N_("RHK SPM32 file import."),
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  rhk_spm32_import_init,
  rhk_spm32_import_query,
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  rhk_spm32_import_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  rhk_spm32_import_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL,
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  rhk_spm32_import_import_callback, // direct menu entry callback1 or NULL
  rhk_spm32_import_export_callback, // direct menu entry callback2 or NULL

  rhk_spm32_import_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm RHK SPM32 file import plug-in\n\n"
                                   " "
	);

static const char *file_mask = "*.sm2";

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  rhk_spm32_import_pi.description = g_strdup_printf(N_("Gxsm rhk_spm32_import plugin %s"), VERSION);
  return &rhk_spm32_import_pi; 
}

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/RHK SPM32"
// Export Menupath is "File/Export/RHK SPM32"
// ----------------------------------------------------------------------
// !!!! make sure the "rhk_spm32_import_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!

static void rhk_spm32_import_query(void)
{


	if(rhk_spm32_import_pi.status) g_free(rhk_spm32_import_pi.status); 
	rhk_spm32_import_pi.status = g_strconcat (
		N_("Plugin query has attached "),
		rhk_spm32_import_pi.name, 
		N_(": File IO Filters are ready to use."),
		NULL);
	

// register this plugins filecheck functions with Gxsm now!
// This allows Gxsm to check files from DnD, open, 
// and cmdline sources against all known formats automatically - no explicit im/export is necessary.
	rhk_spm32_import_pi.app->ConnectPluginToLoadFileEvent (rhk_spm32_import_filecheck_load_callback);
	rhk_spm32_import_pi.app->ConnectPluginToSaveFileEvent (rhk_spm32_import_filecheck_save_callback);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void rhk_spm32_import_init(void)
{
	PI_DEBUG (DBG_L2, "rhk_spm32_import Plugin Init");
}

// about-Function
static void rhk_spm32_import_about(void)
{
	const gchar *authors[] = { rhk_spm32_import_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  rhk_spm32_import_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void rhk_spm32_import_configure(void)
{
	if(rhk_spm32_import_pi.app)
		rhk_spm32_import_pi.app->message("rhk_spm32_import Plugin Configuration");
}

// cleanup-Function, make sure the Menustrings are matching those above!!!
static void rhk_spm32_import_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Plugin Cleanup done.");
}

// make a new derivate of the base class "Dataio"
class RHK_SPM32_ImportFile : public Dataio{
public:
	RHK_SPM32_ImportFile(Scan *s, const char *n) : Dataio(s,n){ };
	virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
	virtual FIO_STATUS Write();
private:
	FIO_STATUS import(const char *fname);
};

// sm2 import
FIO_STATUS RHK_SPM32_ImportFile::Read(xsm::open_mode mode){
	FIO_STATUS ret;
	gchar *fname=NULL;

	fname = (gchar*)name;

	// name should have at least 4 chars: ".ext"
	if (fname == NULL || strlen(fname) < 4)
		return  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
 
	// check for file exists and is OK !
	// else open File Dlg
	ifstream f;
	f.open(fname, ios::in);
	if(!f.good()){
		PI_DEBUG (DBG_L2, "File Error");
		return status=FIO_OPEN_ERR;
	}
	f.close();

	// Check all known File Types:
	
	// is this a SPM32 file?
	if ((ret=import (fname)) !=  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
		return ret;

	return  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
}

FIO_STATUS RHK_SPM32_ImportFile::import(const char *fname){

	ifstream f;
	char header[512];
	GString *comment=NULL;
	int datamode = 16;
	
	float xscale, yscale, zscale, xyscale, angle, xoffset, yoffset, \
		zoffset, current, bias, period;
	char date[9], scantime[9], xunits[10], yunits[10], zunits[10], \
		label[10], text[160];
	long int x_size, y_size, size, type, data_type, line_type, \
		page_type, scandir, file_id, data_offset = 512;

	f.open(fname, ios::in);
	if (!f.good())
	        return status=FIO_OPEN_ERR;

	// read header
	f.read(header, 512);

	// Am I resposible for that file?
	// check for Magic No./ID at file header start:
	if (strncmp (header, "STiMage 3.1", 11) != 0){
		f.close ();
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
	}
	PI_DEBUG (DBG_L2, "rhk_spm32_import.C: Identified SPM32 file.");
	

	// put some usefull values in the ui structure
	if(getlogin()){
		scan->data.ui.SetUser (getlogin());
	}
	else{
		scan->data.ui.SetUser ("unkonwn user");
	}
	time_t t; // scan start time
	time(&t);
	gchar *tmp = g_strconcat ((ctime(&t)), " (Imported)", NULL); scan->data.ui.SetDateOfScan (tmp); g_free (tmp);
	scan->data.ui.SetName (fname);
	scan->data.ui.SetOriginalName (fname);
	scan->data.ui.SetType ("RHK SPM32"); 
	// comment; may be appended later
	comment = g_string_new ("Imported by Gxsm from RHK SPM32 file.\n");
	scan->data.ui.SetComment (comment->str);
	g_string_free(comment, TRUE); 
	comment=NULL;

	// this is mandatory.
	scan->data.s.ntimes  = 1;
	scan->data.s.nvalues = 1;
    
	// initialize scan structure -- this is a minimum example
	scan->data.s.nx = 1;
	scan->data.s.ny = 1;
	scan->data.s.dx = 1;
	scan->data.s.dy = 1;
	scan->data.s.dz = -1;
	scan->data.s.rx = scan->data.s.nx;
	scan->data.s.ry = scan->data.s.ny;
	scan->data.s.x0 = 0;
	scan->data.s.y0 = 0;
	scan->data.s.alpha = 0;

	// be nice and reset this to some defined state
	scan->data.display.z_high       = 1e3;
	scan->data.display.z_low        = 1.;

	// set the default view parameters
	scan->data.display.bright = 32.;
	scan->data.display.contrast = 1.0;

	// FYI: (PZ)
	//  scan->data.display.vrange_z  = ; // View Range Z in base ZUnits
	//  scan->data.display.voffset_z = 0; // View Offset Z in base ZUnits
	//  scan->AutoDisplay([...]); // may be used too...


	//
	// interpret header
	//
	sscanf (header + 0x20, "%ld %ld %ld %ld %ld %ld %ld",
		&type, &data_type, &line_type, &x_size, &y_size, &size, &page_type);
	// only image files (type=1) supported for now:
	if (type != 0) {
		f.close ();
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
	}
	// check data type; only 16 bit signed integer supported for now
	switch (data_type){
	case 1:
		break;
	default:
		f.close ();
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
		break;
	}
	// check plausibility:
	if ( x_size > 2048 || y_size > 2048 || x_size < 1 || y_size < 1 ) {
		f.close ();
		return FIO_INVALID_FILE;
	}
	scan->data.s.nx = x_size;
	scan->data.s.ny = y_size;
	// put page type in comment
	comment = g_string_new (scan->data.ui.comment);
	switch (page_type){
	case 0:
		g_string_append_printf (comment, "Undefined image type.\n");
		break;
	case 1:
		g_string_append_printf (comment, "Topographic image.\n");
		break;
	case 2:
		g_string_append_printf (comment, "Current image.\n");
		break;
	case 3:
		g_string_append_printf (comment, "Aux image.\n");
		break;
	case 4:
		g_string_append_printf (comment, "Force image.\n");
		break;
	case 5:
		g_string_append_printf (comment, "Signal image.\n");
		break;
	case 6:
	        g_string_append_printf (comment, "Image FFT transform.\n");
		break;
	default:
		g_string_append_printf (comment, "Unknown page type.\n");
		break;
	}
	scan->data.ui.SetComment (comment->str);
	g_string_free(comment, TRUE); 
	comment=NULL;

	sscanf (header + 0x40, "X %g %g %10s",
		&xscale, &xoffset, xunits);
	scan->data.s.dx = fabs(xscale) * 1e10;
	scan->data.s.rx = scan->data.s.dx * scan->data.s.nx;
	scan->data.s.x0 = xoffset * 1e10;

	sscanf (header + 0x60, "Y %g %g %10s",
		&yscale, &yoffset, yunits);
	scan->data.s.dy = fabs(yscale) * 1e10;
	scan->data.s.ry = scan->data.s.dy * scan->data.s.ny;
	scan->data.s.y0 = yoffset * 1e10;

	sscanf (header + 0x80, "Z %g %g %10s",
		&zscale, &zoffset, zunits);
	scan->data.s.dz = fabs(zscale) * 1e10;

	sscanf (header + 0xA0, "XY %g G %g",
		&xyscale, &angle);
	if (fabs(angle) < 1e-3)
		angle = 0.;
	scan->data.s.alpha = angle;

	sscanf (header + 0xC0, "IV %g %g",
		&current, &bias);
	scan->data.s.Bias = bias;
	scan->data.s.Current = current;

	sscanf (header + 0x100, "id %ld %ld",
		&file_id, &data_offset);
	sscanf (header + 0x140, "%20s", label);
	strncpy (text, header + 0x160, 160);
	comment = g_string_new (scan->data.ui.comment);
	g_string_append_printf (comment, "id: %ld; label: %s.\n", file_id, label);
	g_string_append_printf (comment, "%s\n", text);
	scan->data.ui.SetComment (comment->str);
	g_string_free(comment, TRUE); 
	comment=NULL;


	// Read Img Data.
	f.seekg(data_offset, ios::beg);
	scan->mem2d->Resize (scan->data.s.nx, scan->data.s.ny, ZD_SHORT);
	scan->mem2d->DataRead (f, 1);


	scan->data.orgmode = SCAN_ORG_CENTER;
	
	scan->mem2d->data->MkXLookup (-scan->data.s.rx/2., scan->data.s.rx/2.);
	scan->mem2d->data->MkYLookup (0., -scan->data.s.ry);

	f.close ();
  
	// RHK SPM32 read done.
	return FIO_OK; 
}


FIO_STATUS RHK_SPM32_ImportFile::Write(){

	return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

}

// Plugin's Notify Cb's, registered to be called on file load/save to check file
// return via filepointer, it is set to Zero or passed as Zero if file has been processed!
// That's all fine, you should just change the Text Stings below...


static void rhk_spm32_import_filecheck_load_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		PI_DEBUG (DBG_L2, 
			  "Check File: rhk_spm32_import_filecheck_load_callback called with >"
			  << *fn << "<" );

		Scan *dst = main_get_gapp()->xsm->GetActiveScan();
		if(!dst){ 
			main_get_gapp()->xsm->ActivateFreeChannel();
			dst = main_get_gapp()->xsm->GetActiveScan();
		}
		RHK_SPM32_ImportFile fileobj (dst, *fn);

		FIO_STATUS ret = fileobj.Read(); 
		if (ret != FIO_OK){ 
			// I'am responsible! (But failed)
			if (ret != FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
				*fn=NULL;
			// no more data: remove allocated and unused scan now, force!
//			main_get_gapp()->xsm->SetMode(-1, ID_CH_M_OFF, TRUE); 
			PI_DEBUG (DBG_L2, "Read Error " << ((int)ret) << "!!!!!!!!" );
		}else{
			// got it!
			*fn=NULL;

			// Now update gxsm main window data fields
			main_get_gapp()->xsm->ActiveScan->GetDataSet(main_get_gapp()->xsm->data);
			main_get_gapp()->spm_update_all();
			dst->draw();
		}
	}else{
		PI_DEBUG (DBG_L2, "rhk_spm32_import_filecheck_load: Skipping" );
	}
}

static void rhk_spm32_import_filecheck_save_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		Scan *src;
		PI_DEBUG (DBG_L2,
			  "Check File: rhk_spm32_import_filecheck_save_callback called with >"
			  << *fn << "<" );

		RHK_SPM32_ImportFile fileobj (src = main_get_gapp()->xsm->GetActiveScan(), *fn);

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
		PI_DEBUG (DBG_L2, "rhk_spm32_import_filecheck_save: Skipping >" << *fn << "<" );
	}
}

// Menu Call Back Fkte

static void rhk_spm32_import_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (rhk_spm32_import_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (rhk_spm32_import_pi.name, "-import", NULL);
	gchar *fn = main_get_gapp()->file_dialog(help[0], NULL, file_mask, NULL, dlgid);
	g_strfreev (help); 
	g_free (dlgid);
	rhk_spm32_import_filecheck_load_callback (&fn );
}

static void rhk_spm32_import_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (rhk_spm32_import_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (rhk_spm32_import_pi.name, "-export", NULL);
	gchar *fn = main_get_gapp()->file_dialog(help[1], NULL, file_mask, NULL, dlgid);
	g_strfreev (help); 
	g_free (dlgid);
	rhk_spm32_import_filecheck_save_callback (&fn );
}
