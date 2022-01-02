/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: g_dat_im_export.C
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
% PlugInName: G_dat_Im_Export
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: File/Import/G-dat

% PlugInDescription
\label{plugins:g_dat_im_export}
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
#include "batch.h"
#include "g_dat_types.h"
#include "fileio.c"
// -- END EDIT --

// enable std namespace
using namespace std;

// Plugin Prototypes
static void g_dat_im_export_init (void);
static void g_dat_im_export_query (void);
static void g_dat_im_export_about (void);
static void g_dat_im_export_configure (void);
static void g_dat_im_export_cleanup (void);

static void g_dat_im_export_filecheck_load_callback (gpointer data );
static void g_dat_im_export_filecheck_save_callback (gpointer data );

static void g_dat_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void g_dat_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// Fill in the GxsmPlugin Description here
GxsmPlugin g_dat_im_export_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
// -- START EDIT --
  "G-dat-ImExport",            // PlugIn name
  NULL,                   // PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("Im/Export of the old (G-) dat file format."),
  "Percy Zahl",
  "file-import-section,file-export-section", // sep. im/export menuentry path by comma!
  N_("G-dat,G-dat"), // menu entry (same for both)
  N_("Old 'dat' file import,Old 'dat' file export"), // short help for menu entry
  N_("Old 'dat' file import and export filter."), // info
// -- END EDIT --
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  g_dat_im_export_init,
  g_dat_im_export_query,
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  g_dat_im_export_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  g_dat_im_export_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL,
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  g_dat_im_export_import_callback, // direct menu entry callback1 or NULL
  g_dat_im_export_export_callback, // direct menu entry callback2 or NULL

  g_dat_im_export_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("GXSM 'G-dat' plugin for im-/export of old the 'dat' data file type.\n"
                                   "This format was used in Hannover in early times and can also be used "
				   "with the 'gnutools'. This apps were using this format: PMSTM and older "
				   "very first versions of XXSM and older GXSM versions were able to use it "
				   "instead of NetCDF if configured this way.\n\n"
				   "Now this filter is here to visit/manipulate old data files."
	);

static const char *file_mask = "*.dat";

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  g_dat_im_export_pi.description = g_strdup_printf(N_("Gxsm im_export plugin %s"), VERSION);
  return &g_dat_im_export_pi; 
}

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/GME Dat"
// Export Menupath is "File/Export/GME Dat"
// ----------------------------------------------------------------------
// !!!! make sure the "g_dat_im_export_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!

static void g_dat_im_export_query(void)
{
        if(g_dat_im_export_pi.status) g_free(g_dat_im_export_pi.status); 
	g_dat_im_export_pi.status = g_strconcat (
		N_("Plugin query has attached "),
		g_dat_im_export_pi.name, 
		N_(": File IO Filters are ready to use."),
		NULL);

	// register this plugins filecheck functions with Gxsm now!
	// This allows Gxsm to check files from DnD, open, 
	// and cmdline sources against all known formats automatically - no explicit im/export is necessary.
	g_dat_im_export_pi.app->ConnectPluginToLoadFileEvent (g_dat_im_export_filecheck_load_callback);
	g_dat_im_export_pi.app->ConnectPluginToSaveFileEvent (g_dat_im_export_filecheck_save_callback);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void g_dat_im_export_init(void)
{
	PI_DEBUG (DBG_L2, g_dat_im_export_pi.name << " Plugin Init");
}

// about-Function
static void g_dat_im_export_about(void)
{
	const gchar *authors[] = { g_dat_im_export_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  g_dat_im_export_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void g_dat_im_export_configure(void)
{
	if(g_dat_im_export_pi.app)
		g_dat_im_export_pi.app->message("im_export Plugin Configuration");
}

// cleanup-Function, remove all "custom" menu entrys here!
static void g_dat_im_export_cleanup(void)
{
#if 0
	gchar **path  = g_strsplit (g_dat_im_export_pi.menupath, ",", 2);
	gchar **entry = g_strsplit (g_dat_im_export_pi.menuentry, ",", 2);

	gchar *tmp = g_strconcat (path[0], entry[0], NULL);
	gnome_app_remove_menus (GNOME_APP (g_dat_im_export_pi.app->getApp()), tmp, 1);
	g_free (tmp);

	tmp = g_strconcat (path[1], entry[1], NULL);
	gnome_app_remove_menus (GNOME_APP (g_dat_im_export_pi.app->getApp()), tmp, 1);
	g_free (tmp);

	g_strfreev (path);
	g_strfreev (entry);

	PI_DEBUG (DBG_L2, "Plugin Cleanup done.");
#endif
}

// make a new derivate of the base class "Dataio"
class  Gdat_ImExportFile : public Dataio{
public:
	 Gdat_ImExportFile(Scan *s, const char *n) : Dataio(s,n){  memset(&Kopf, 0, sizeof(Kopf)); };
	virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
	virtual FIO_STATUS Write();
private:
	FIO_STATUS import_data(const char *fname); 
	HEADER Kopf;
};

FIO_STATUS Gdat_ImExportFile::Read(xsm::open_mode mode){
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

// Utils only used here:
double Contrast_to_VRangeZ (double contrast, double dz){
	return 64.*dz/contrast;
}
double VRangeZ_to_Contrast (double vrz, double dz){
	return 64.*dz/vrz;
}

FIO_STATUS Gdat_ImExportFile::import_data(const char *fname){
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
//	scan->data.CpUnits (scan_template);
	scan->data.GetScan_Param (scan_template);
//	scan->data.GetLayer_Param (scan_template);
	scan->data.GetUser_Info (scan_template);
//	scan->data.GetDSP_Param (scan_template);
	scan->data.GetDisplay_Param (scan_template);

	// now start importing -----------------------------------------
	f.seekg(0, ios::beg); // Auf Headerstart Positionieren
	f.read((char*)&Kopf, sizeof(HEADER));
    
	PI_DEBUG (DBG_L2, "Checking 'Kopf.Kennung'");
	switch(Kopf.Kennung){
	case 0xABCE: PI_DEBUG (DBG_L2, "Neues Dat Format detected!"); break;
	case 0xABCD: PI_DEBUG (DBG_L2, "Altes Dat Format (PMSTM) detected!"); break;
	case 0xCEAB: 
		PI_DEBUG (DBG_L2, "Neues Dat Format [LE on BE] detected!\n"
			  << "-- But sorry, wrong endianess detected, I'm not handling this yet. --");
		dialog = gtk_message_dialog_new (NULL,
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_INFO,
						 GTK_BUTTONS_OK,
						 N_("New 'dat' format [LE on BE] detected!\n"
						    "But sorry, wrong endianess detected, "
						    "I'm not handling this yet."));
                gtk_widget_show (dialog);
                {
                        int response = GTK_RESPONSE_NONE;
                        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
                
                        // FIX-ME GTK4 ??
                        // wait here on response
                        while (response == GTK_RESPONSE_NONE)
                                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
                }

		byteswap = TRUE;
		f.close ();
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
		break;
	case 0xCDAB: 
		PI_DEBUG (DBG_L2, "Altes Dat Format (PMSTM) [LE on BE] detected!\n"
			  << "-- But sorry, wrong endianess detected, I'm not handling this yet. --");
		dialog = gtk_message_dialog_new (NULL,
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_INFO,
						 GTK_BUTTONS_OK,
						 N_("New 'dat' format [LE on BE] detected!\n"
						    "But sorry, wrong endianess detected, "
						    "I'm not handling this yet."));
		gtk_widget_show (dialog);
                {
                        int response = GTK_RESPONSE_NONE;
                        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
        
                        // FIX-ME GTK4 ??
                        // wait here on response
                        while (response == GTK_RESPONSE_NONE)
                                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
		}
		byteswap = TRUE;
		f.close ();
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
		break;
	case 0:
// For some reason, I had to remove the following code part to get it to
// compile.  AK
// 		gchar *message = g_strdup_printf (
// 			N_("Malformed dat format Header detected!\n"
// 			   "You can force ignoring this in settings (not yet, sorry),\n"
// 			   "but it's not save. - import canceled.\n"
// 			   " DAT-Kopf.Kennung = 0x%4X"), Kopf.Kennung);
// 		PI_DEBUG (DBG_L2, message);

		PI_DEBUG (DBG_L2, "Malformed dat format Header detected!");

// 		dialog = gtk_message_dialog_new (NULL,
// 						 GTK_DIALOG_DESTROY_WITH_PARENT,
// 						 GTK_MESSAGE_INFO,
// 						 GTK_BUTTONS_OK,
// 						 message);
// 		gtk_dialog_run (GTK_DIALOG (dialog));
// 		gtk_widget_destroy (dialog);
// 		g_free (message);
		f.close ();
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
		break;
	default:
		f.close ();
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
		break;
	}

	// update as much as we get...
	time_t t; // Scan - Startzeit eintragen 
	time(&t);
	gchar *tmp = g_strconcat ((ctime(&t)), " (Imported)", NULL); scan->data.ui.SetDateOfScan (tmp); g_free (tmp);
	scan->data.ui.SetName (name);
	scan->data.ui.SetOriginalName (name);
	scan->data.ui.SetType ("old G-dat");
	scan->data.ui.SetUser (Kopf.UserName);

	FileList = g_string_new ("Imported by GXSM from old dat filetype.\n");
	g_string_append_printf (FileList, "Original Filename: %s\n", name);
	g_string_append (FileList, "Original Kopf.comment:\n");
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
}

FIO_STATUS Gdat_ImExportFile::Write(){
	int pcnt=0;
	ofstream f;

	PI_DEBUG (DBG_L2, "writing >" << name << "<");

	if (name == NULL) return FIO_NO_NAME;

	f.open(name, ios::out);
	if (!f.good())
	        return status=FIO_OPEN_ERR;

	// start exporting -------------------------------------------
	f.seekp(0, ios::beg); // Auf Headerstart Positionieren

	// Kopf erstellen
	scan->data.ui.SetName (name);
	Kopf.ScMode    = TopoGraphic;
	Kopf.Kennung   = 0xABCE; /* NEU !!, alt=0xABCD */
	if(strlen(scan->data.ui.user) > 39)
		XSM_SHOW_ALERT(HINT_WARN, HINT_UNAME_TRUNC, " ", 0);
	if(strlen(scan->data.ui.comment) > 255)
		XSM_SHOW_ALERT(HINT_WARN, HINT_COMMENT_TRUNC, " ", 0);
	strncpy(Kopf.UserName, scan->data.ui.user, 39);
	strncpy(Kopf.comment, scan->data.ui.comment, 255);
	Kopf.nx[pcnt] = scan->data.s.nx;
	Kopf.ny[pcnt] = scan->data.s.ny;
	// ....->Inst wird nur wegen dat-Type ben\F6tigt .... :=(
	Kopf.dx[pcnt] = R2INT(scan->data.s.dx/gapp->xsm->Inst->XResolution());
	Kopf.dy[pcnt] = R2INT(scan->data.s.dy/gapp->xsm->Inst->YResolution());
	Kopf.x_Offset[pcnt] = R2INT(scan->data.s.x0/gapp->xsm->Inst->XResolution());
	Kopf.y_Offset[pcnt] = R2INT(scan->data.s.y0/gapp->xsm->Inst->XResolution());
	Kopf.DAtoAng_X = gapp->xsm->Inst->XResolution();
	Kopf.DAtoAng_Y = gapp->xsm->Inst->YResolution();
	Kopf.DAtoAng_Z = gapp->xsm->Inst->ZResolution();
	Kopf.Rotation  = (short)(rint(scan->data.s.alpha));
	Kopf.brightfac = scan->data.display.bright;
	Kopf.greyfac   = VRangeZ_to_Contrast (scan->data.display.vrange_z, scan->data.s.dz);
	//  Kopf.linefac   = scan->data.display.gamma;
	Kopf.forw_delay=10L;
	Kopf.back_delay=10L;
	Kopf.NumOfTopAve=1;
	// Kopf noch nicht vollst\E4ndig ausgef\FCllt !

	f.write((const char*)&Kopf, sizeof(HEADER));
	if(f.fail()){
		f.close();
		return status=FIO_WRITE_ERR; 
	}

	scan->mem2d->DataWrite(f);

	if(f.fail()){
		f.close();
		return status=FIO_WRITE_ERR; 
	}
	f.close();
	return status=FIO_OK; 
}

// Plugin's Notify Cb's, registered to be called on file load/save to check file
// return via filepointer, it is set to Zero or passed as Zero if file has been processed!
// That's all fine, you should just change the Text Stings below...


static void g_dat_im_export_filecheck_load_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		PI_DEBUG (DBG_L2, "checking for g_dat file>" << *fn << "<" );

		Scan *dst = gapp->xsm->GetActiveScan();
		if(!dst){ 
			gapp->xsm->ActivateFreeChannel();
			dst = gapp->xsm->GetActiveScan();
		}
		Gdat_ImExportFile fileobj (dst, *fn);

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

static void g_dat_im_export_filecheck_save_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		Scan *src;
		PI_DEBUG (DBG_L2, "Saving/(checking) >" << *fn << "<" );

		Gdat_ImExportFile fileobj (src = gapp->xsm->GetActiveScan(), *fn);

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

static void g_dat_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (g_dat_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (g_dat_im_export_pi.name, "-import", NULL);
	gchar *fn = gapp->file_dialog_load (help[0], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
	        g_dat_im_export_filecheck_load_callback (&fn );
                g_free (fn);
	}
}

static void g_dat_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (g_dat_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (g_dat_im_export_pi.name, "-export", NULL);
	gchar *fn = gapp->file_dialog_save (help[1], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
         	g_dat_im_export_filecheck_save_callback (&fn );
                g_free (fn);
	}
}
