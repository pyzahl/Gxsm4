/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: PsiHDF_im_export.C
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
% PlugInDocuCaption: Park Scientific (HDF) data Import
% PlugInName: PsiHDF_im_export
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: File/Import/PsiHDF

% PlugInDescription
\label{plugins:PsiHDF_im_export}
The \GxsmEmph{PsiHDF\_im\_export} plug-in supports reading of
Psi-HDFe files used by Park-Scientific AFM.

% PlugInUsage
The plug-in is called by \GxsmMenu{File/Import/PsiHDF}.

% OptPlugInKnownBugs
Not yet tested, porting to GXSM-2 in progress..

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
#include "psihdf.h"

using namespace std;

#define IMGMAXCOLORS 64

// Plugin Prototypes
static void PsiHDF_im_export_init (void);
static void PsiHDF_im_export_query (void);
static void PsiHDF_im_export_about (void);
static void PsiHDF_im_export_configure (void);
static void PsiHDF_im_export_cleanup (void);

static void PsiHDF_im_export_filecheck_load_callback (gpointer data );
static void PsiHDF_im_export_filecheck_save_callback (gpointer data );

static void PsiHDF_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void PsiHDF_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// Fill in the GxsmPlugin Description here
GxsmPlugin PsiHDF_im_export_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  "PsiHDF-ImExport",
  NULL,                   // PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("Im/Export of PsiHDF file format."),
  "Percy Zahl",
  "file-import-section,file-export-section",
  N_("PsiHDF,PsiHDF"),
  N_("PsiHDF import,PsiHDF export"),
  N_("Park Scientific HDF import and export filter."),
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  PsiHDF_im_export_init,
  PsiHDF_im_export_query,
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  PsiHDF_im_export_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  PsiHDF_im_export_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL,
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  PsiHDF_im_export_import_callback, // direct menu entry callback1 or NULL
  PsiHDF_im_export_export_callback, // direct menu entry callback2 or NULL

  PsiHDF_im_export_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("This GXSM plugin allows im- and export of the Park Scientifi HDF file format"
                                   " "
	);

static const char *file_mask = "*.hdf";

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  PsiHDF_im_export_pi.description = g_strdup_printf(N_("Gxsm im_export plugin %s"), VERSION);
  return &PsiHDF_im_export_pi; 
}

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/GME Dat"
// Export Menupath is "File/Export/GME Dat"
// ----------------------------------------------------------------------
// !!!! make sure the "PsiHDF_im_export_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!

static void PsiHDF_im_export_query(void)
{

	// register this plugins filecheck functions with Gxsm now!
	// This allows Gxsm to check files from DnD, open, 
	// and cmdline sources against all known formats automatically - no explicit im/export is necessary.
	PsiHDF_im_export_pi.app->ConnectPluginToLoadFileEvent (PsiHDF_im_export_filecheck_load_callback);
	PsiHDF_im_export_pi.app->ConnectPluginToSaveFileEvent (PsiHDF_im_export_filecheck_save_callback);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void PsiHDF_im_export_init(void)
{
	PI_DEBUG (DBG_L2, PsiHDF_im_export_pi.name << "Plugin Init");
}

// about-Function
static void PsiHDF_im_export_about(void)
{
	const gchar *authors[] = { PsiHDF_im_export_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  PsiHDF_im_export_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void PsiHDF_im_export_configure(void)
{
	if(PsiHDF_im_export_pi.app)
		PsiHDF_im_export_pi.app->message("PsiHDF_im_export Plugin Configuration");
}

// cleanup-Function, make sure the Menustrings are matching those above!!!
static void PsiHDF_im_export_cleanup(void)
{

	PI_DEBUG (DBG_L2, "Plugin Cleanup done.");
}

// make a new derivate of the base class "Dataio"
class PsiHDF_ImExportFile : public Dataio{
public:
	PsiHDF_ImExportFile(Scan *s, const char *n) : Dataio(s,n){ };
	virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
	virtual FIO_STATUS Write();
private:
	FIO_STATUS import(const char *fname);
	int numDD;  // number of data descriptors
};

FIO_STATUS PsiHDF_ImExportFile::Read(xsm::open_mode mode){
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
		PI_DEBUG (DBG_L2, "File Fehler");
		return status=FIO_OPEN_ERR;
	}
	f.close();

	// Check all known File Types:
	
	// is this a PsiHDF file?
	if ((ret=import (fname)) !=  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
		return ret;

	return  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
}

//=======================================================================
//
// PsiHDF  -   HDF-Format von Park Scientific Instr.
//
// up to now only reading of HDF implemented
//
//=======================================================================
FIO_STATUS PsiHDF_ImExportFile::import(const char *fname){
	ifstream f;
	gchar *tmp = NULL;
	GString *FileList=NULL;

	f.open(name, ios::in);
	if (!f.good())
	        return status=FIO_OPEN_ERR;

	// check some magics of this type...
	if (TRUE){
		f.close ();
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
	}
	
	time_t t; // Scan - Startzeit eintragen 
	time(&t);
	tmp = g_strconcat ((ctime(&t)), " (Imported)", NULL); scan->data.ui.SetDateOfScan (tmp); g_free (tmp);
	scan->data.ui.SetName (fname);
	scan->data.ui.SetOriginalName (fname);
	scan->data.ui.SetType ("old dat"); 


	// this is mandatory.
	scan->data.s.ntimes  = 1;
	scan->data.s.nvalues = 1;
  
  
	// put some usefull values in the ui structure
	if(getlogin()){
		scan->data.ui.SetUser (getlogin());
	}
	else{
		scan->data.ui.SetUser ("unkonwn user");
	}

	// initialize scan structure -- this is a minimum example
	scan->data.s.nx = 1;
	scan->data.s.ny = 1;
	scan->data.s.dx = 1;
	scan->data.s.dy = 1;
	scan->data.s.dz = 1;
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
  

	FileList = g_string_new ("Imported by GXSM from old dat filetype.\n");
	g_string_append_printf (FileList, "Original Filename: %s\n", fname);
//	g_string_append (FileList, "blaa\n");
	
	int magic;
	PsiHEADER header;

        f.open(name, std::ios::in);
	if(!f.good())
		return status=FIO_OPEN_ERR;

	// get magic number
        f.seekg(0, std::ios::beg);
	f.read((char*)(&magic), 4);
  
	if (magic != HDF_MAGIC_NUM)  {
		XSM_DEBUG_ERROR (DBG_L1, "PsiHDF: wrong magic number! (magic = " << magic << " )");
		return FIO_READ_ERR;
	}
  
	// read PSI binary Header
        f.seekg(PSI_HEADER_OFFSET, std::ios::beg);
	f.read((char*)&header, sizeof(PsiHEADER));
	// dirty hack to correct differences in the behavior of 16 and 32bit architectures:
	// under Linux, 32bit variables like int or float are moved to the beginning
	// of a 32bit segment.  unfortunately there are an odd number of shorts before the
	// float header.fXScansize :-(  This is corrected by the following hack:  the header
	// is read two times 
# define XOFFS(X) ((char*)(&X) - (char*)(&header))
        f.seekg(PSI_HEADER_OFFSET + XOFFS(header.fXScanSize) + 2, std::ios::beg);
	f.read((char*)&header.fXScanSize, sizeof(PsiHEADER) - XOFFS(header.fXScanSize));

	if(f.fail()){
		f.close();
		return status=FIO_READ_ERR; 
	}

	/* grrrrrrrrrr
	   XSM_DEBUG_ERROR (DBG_L1,  "PsiHDF: source: " << header.szSourceName );
	   XSM_DEBUG_ERROR (DBG_L1,  "PsiHDF: mode: " << header.szImageMode );
	   XSM_DEBUG_ERROR (DBG_L1,  "PsiHDF: image size: " << header.nCols << " x " << header.nRows );
	   XSM_DEBUG_ERROR (DBG_L1,  "PsiHDF: scan size in um: " << header.fXScanSize << " x " << header.fYScanSize );
	   XSM_DEBUG_ERROR (DBG_L1,  "PsiHDF: setpoint unit: " << header.szSetPointUnit );
	   XSM_DEBUG_ERROR (DBG_L1,  "PsiHDF: scan rate: " << header.fScanRate );
	   XSM_DEBUG_ERROR (DBG_L1,  "PsiHDF: data min: " << header.nDataMin << " max: " << header.nDataMax );

	   char dum[4];
	   f.seekg(0x03e2, std::ios::beg);
	   f.read(dum, 4);
	   XSM_DEBUG_ERROR (DBG_L1,  "dum: " << (int)dum[0] << " " << (int)dum[1] << " " << (int)dum[2] << " " << (int)dum[3] );
	   XSM_DEBUG_ERROR (DBG_L1,  "dum: " << *((float*)dum) );
	   f.seekg(0x03e6, std::ios::beg);
	   f.read(dum, 4);
	   XSM_DEBUG_ERROR (DBG_L1,  "dum: " << (int)dum[0] << " " << (int)dum[1] << " " << (int)dum[2] << " " << (int)dum[3] );
	   XSM_DEBUG_ERROR (DBG_L1,  "dum: " << *((float*)dum) );
	   XSM_DEBUG_ERROR (DBG_L1,  "dum size " << (int)((char*)&header.fXScanSize)[0] << " " << (int)((char*)&header.fXScanSize)[1] << " " << (int)((char*)&header.fXScanSize)[2] << " " << (int)((char*)&header.fXScanSize)[3] );
	   XSM_DEBUG_ERROR (DBG_L1,  "dum size " << (int)((char*)&header.fYScanSize)[0] << " " << (int)((char*)&header.fYScanSize)[1] << " " << (int)((char*)&header.fYScanSize)[2] << " " << (int)((char*)&header.fYScanSize)[3] );
	   XSM_DEBUG_ERROR (DBG_L1,  "sizeof(HEADER): " << sizeof(header) );
	   XSM_DEBUG_ERROR (DBG_L1,  "offs header.nCols " << XOFFS(header.nCols) );
	   XSM_DEBUG_ERROR (DBG_L1,  "offs header.nRows " << XOFFS(header.nRows) );
	   XSM_DEBUG_ERROR (DBG_L1,  "offs header.dummy " << XOFFS(header.dummy) );
	   XSM_DEBUG_ERROR (DBG_L1,  "offs header.fLowPass " << XOFFS(header.fLowPass) );
	   XSM_DEBUG_ERROR (DBG_L1,  "offs header.b3rdOrder " << XOFFS(header.b3rdOrder) );
	   XSM_DEBUG_ERROR (DBG_L1,  "offs header.bAutoFlatten " << XOFFS(header.bAutoFlatten) );
	   XSM_DEBUG_ERROR (DBG_L1,  "offs header.fXScanSize " << XOFFS(header.fXScanSize) << " " << XOFFS(header.fYScanSize) );
	*/

	// convert PSI header information to scan data
	// values in PSI header are in um, xxsm data has to be in Angstrom!
	scan->data.s.nx = header.nCols;
	scan->data.s.ny = header.nRows;
	scan->data.s.dx = header.fXScanSize * 10000. / header.nCols;
	scan->data.s.dy = header.fYScanSize * 10000. / header.nRows;
	scan->data.s.dz = header.fDataGain * 10000.;
	scan->data.s.rx = scan->data.s.nx*scan->data.s.dx;
	scan->data.s.ry = scan->data.s.ny*scan->data.s.dy;
	scan->data.s.x0 = header.fXOffset * 10000.;
	scan->data.s.y0 = header.fYOffset * 10000.;
	scan->data.s.alpha = 0;
	scan->data.ui.SetUser ("PSI SXM");
	tmp = g_strconcat ("PSI Mode: ", header.szImageMode, ", Source: ", header.szSourceName, NULL);
	scan->data.ui.SetComment (tmp);
	g_free (tmp);
  
	// read the data
	scan->mem2d->Resize (scan->data.s.nx, scan->data.s.ny);
        f.seekg (FILE_HEADER_SIZE, std::ios::beg);
	scan->mem2d->DataRead (f);

	scan->data.orgmode = SCAN_ORG_CENTER;
	scan->mem2d->data->MkXLookup (-scan->data.s.rx/2., scan->data.s.rx/2.);
	scan->mem2d->data->MkYLookup (scan->data.s.ry/2., -scan->data.s.ry/2.);

	if(f.fail()){
		f.close();
		return status=FIO_READ_ERR; 
	}






	scan->data.ui.SetComment (FileList->str);
	g_string_free(FileList, TRUE); 
	FileList=NULL;

	scan->data.s.dx = scan->data.s.rx / scan->data.s.nx;
	scan->data.s.dy = scan->data.s.ry / scan->data.s.ny;

	// convert to Ang
//	scan->data.s.x0 *= pzAV_x / 32767.*10. * gx ;
//	scan->data.s.y0 *= pzAV_y / 32767.*10. * gy ;

	// Read Img Data.
	
	scan->mem2d->Resize(scan->data.s.nx, scan->data.s.ny);
	scan->mem2d->DataRead(f, 1);

	scan->data.orgmode = SCAN_ORG_CENTER;
	scan->mem2d->data->MkXLookup (-scan->data.s.rx/2., scan->data.s.rx/2.);
	scan->mem2d->data->MkYLookup (scan->data.s.ry/2., -scan->data.s.ry/2.);

  
	return FIO_OK; 
}


FIO_STATUS PsiHDF_ImExportFile::Write(){
	gchar tmp[0x4004];
	const gchar *fname;
	ofstream f;

	memset (tmp, 0, sizeof (tmp));

	if(strlen(name)>0)
		fname = (const char*)name;
	else
		fname = gapp->file_dialog("File Export: PsiHDF"," ","*.hdf","","PsiHDF write");
	if (fname == NULL) return FIO_NO_NAME;

	// check if we like to handle this
	if (strncmp(fname+strlen(fname)-4,".hdf",4))
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

	GtkWidget *dialog = gtk_message_dialog_new (NULL,
						    GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_MESSAGE_INFO,
						    GTK_BUTTONS_OK,
						    N_("Sorry, not yet implemented.")
						    );
        gtk_widget_show (dialog);

        int response = GTK_RESPONSE_NONE;
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
        
        // FIX-ME GTK4 ??
        // wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

	return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

#if 0
	f.open(name, ios::out);
	if (!f.good())
	        return status=FIO_OPEN_ERR;

//	scan->mem2d->DataWrite(f);


	int pcnt=0;
	std::ofstream f;
    
        f.open(name, std::ios::out | std::ios::trunc);
	if(!f.good())
		return status=FIO_OPEN_ERR;

        f.seekp(0, std::ios::beg); // Auf Headerstart Positionieren

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
	//* ....->Inst wird nur wegen dat-Type ben\F6tigt .... :=(
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



#endif
  return FIO_OK; 
}

// Plugin's Notify Cb's, registered to be called on file load/save to check file
// return via filepointer, it is set to Zero or passed as Zero if file has been processed!
// That's all fine, you should just change the Text Stings below...


static void PsiHDF_im_export_filecheck_load_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		PI_DEBUG (DBG_L2, 
			  "Check File: PsiHDF_im_export_filecheck_load_callback called with >"
			  << *fn << "<" );

		Scan *dst = gapp->xsm->GetActiveScan();
		if(!dst){ 
			gapp->xsm->ActivateFreeChannel();
			dst = gapp->xsm->GetActiveScan();
		}
		PsiHDF_ImExportFile fileobj (dst, *fn);

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
		PI_DEBUG (DBG_L2, "PsiHDF_im_export_filecheck_load: Skipping" );
	}
}

static void PsiHDF_im_export_filecheck_save_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		Scan *src;
		PI_DEBUG (DBG_L2,
			  "Check File: PsiHDF_im_export_filecheck_save_callback called with >"
			  << *fn << "<" );

		PsiHDF_ImExportFile fileobj (src = gapp->xsm->GetActiveScan(), *fn);

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
		PI_DEBUG (DBG_L2, "PsiHDF_im_export_filecheck_save: Skipping >" << *fn << "<" );
	}
}

// Menu Call Back Fkte

static void PsiHDF_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (PsiHDF_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (PsiHDF_im_export_pi.name, "-import", NULL);
	gchar *fn = gapp->file_dialog(help[0], NULL, file_mask, NULL, dlgid);
	g_strfreev (help); 
	g_free (dlgid);
	PsiHDF_im_export_filecheck_load_callback (&fn );
}

static void PsiHDF_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (PsiHDF_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (PsiHDF_im_export_pi.name, "-export", NULL);
	gchar *fn = gapp->file_dialog(help[1], NULL, file_mask, NULL, dlgid);
	g_strfreev (help); 
	g_free (dlgid);
	PsiHDF_im_export_filecheck_save_callback (&fn );
}
