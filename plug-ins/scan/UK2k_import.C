/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: UK2k_import.C
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
% PlugInDocuCaption: UK2000 v3.4 import plug-in

% PlugInName: UK2k_import

% PlugInAuthor: Juan de la Figuera

% PlugInAuthorEmail: juan.delafiguera@uam.es

% PlugInMenuPath: File/Import/Uk2k import

% PlugInDescription

This plugin is responsible for reading UK2000 v3.4 images, with the
following limitations so far: only topography, and only the forward
scan. To add support for the back scan would be pretty simple, if
someone is interested. This STM electronics is a DSP based system with
transputers as CPUs (yes, it has several). It was made by Uwe Knipping
from Arizona State University and sold by CVS (Custom Vacuum Systems
Ltd.). There are a few systems around, still running after more than
10 years. The PC version of the electronics are the basis of the DSP
STM unit sold (and further developed) by Molecular Imaging.

The files are an ASCII file with the scan details (extension .stp),
and an 16bit binary dump of the image (extension .std).

% PlugInUsage
Registers itself for loading files with the filename suffix ".std".

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "dataio.h"
#include "action_id.h"
#include "util.h"

#ifndef WORDS_BIGENDIAN 
#define WORDS_BIGENDIAN 0
#endif
 
using namespace std;


// Plugin Prototypes
static void UK2k_import_init (void);
static void UK2k_import_query (void);
static void UK2k_import_about (void);
static void UK2k_import_configure (void);
static void UK2k_import_cleanup (void);

static void UK2k_import_filecheck_load_callback (gpointer data );
//static void UK2k_import_filecheck_save_callback (gpointer data );

static void UK2k_import_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void UK2k_import_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// Fill in the GxsmPlugin Description here
GxsmPlugin UK2k_import_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  "UK2k_import",
  NULL,                // PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("Import of UK2k data format."),
  "Juan de la Figuera",
  "file-import-section,file-export-section", // sep. im/export menuentry path by comma!
  N_("UK2k,UK2k"),
  "UK2000 v3.4 image format",
  "This plugin is respnsible for reading UK2000 v3.4 images, so far no spectroscopy, and only one image",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  UK2k_import_init,  
  UK2k_import_query,  
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  UK2k_import_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  UK2k_import_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL,
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  UK2k_import_import_callback, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  UK2k_import_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm UK2k Data File Import Plugin\n\n"
                                   "This plugin reads in UK2k datafiles."
	);

static const char *file_mask = "*.[Ss][tT][dDpP]";

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){
  UK2k_import_pi.description = g_strdup_printf(N_("Gxsm UK2k_import plugin %s"), VERSION);
  return &UK2k_import_pi; 
}

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/UK2k import"
// Export Menupath is "File/Export/UK2k import"
// ----------------------------------------------------------------------
// !!!! make sure the "UK2k_import_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!

static void UK2k_import_query(void)
{
#if 0
  if(UK2k_import_pi.status) g_free(UK2k_import_pi.status);
  UK2k_import_pi.status = g_strconcat (
	  N_("Plugin query has attached "),
	  UK2k_import_pi.name,
	  N_(": File IO Filters are ready to use"),
	  NULL);
#endif
// register this plugins filecheck functions with Gxsm now!
// This allows Gxsm to check files from DnD, open,
// and cmdline sources against all known formats automatically - no explicit im/export is necessary.
  UK2k_import_pi.app->ConnectPluginToLoadFileEvent (UK2k_import_filecheck_load_callback);
//  UK2k_import_pi.app->ConnectPluginToSaveFileEvent (UK2k_import_filecheck_save_callback);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void UK2k_import_init(void)
{
  PI_DEBUG (DBG_L2, "UK2k_import Plugin Init" );
}

// about-Function
static void UK2k_import_about(void)
{
  const gchar *authors[] = { UK2k_import_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  UK2k_import_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void UK2k_import_configure(void)
{
  if(UK2k_import_pi.app)
    UK2k_import_pi.app->message("UK2k_import Plugin Configuration");
}

// cleanup-Function, make sure the Menustrings are matching those above!!!
static void UK2k_import_cleanup(void)
{
  PI_DEBUG (DBG_L2, "UK2k_import Plugin Cleanup" );
#if 0
  gnome_app_remove_menus (GNOME_APP (UK2k_import_pi.app->getApp()),
			  N_("File/Import/UK2k Import"), 1);
//s export is not available
//s  gnome_app_remove_menus (GNOME_APP (UK2k_import_pi.app->getApp()),
//s			  N_("File/Export/UK2k Export"), 1);
#endif
}

// make a new derivate of the base class "Dataio"
class UK2k_ImExportFile : public Dataio{
 public:
  UK2k_ImExportFile(Scan *s, const char *n) : Dataio(s,n){ };
  virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
  virtual FIO_STATUS Write();
 private:
  // Binary data read
  FIO_STATUS spmReadDat(const char *fname);
 // Parses ASCII parameter files
  FIO_STATUS spmReadPar(const gchar *fname, const gchar *fsuffix);
};

// d2d import :=)
FIO_STATUS UK2k_ImExportFile::Read(xsm::open_mode mode){

	FIO_STATUS ret = FIO_OK;
	gchar *fdatname = (gchar*) name;
	gchar *fparname=NULL;
	gchar *fbasename=NULL;
	gchar *fsuffix=NULL;

	// name should have at least 4 chars: ".ext"
	if (fdatname == NULL || strlen(fdatname) < 4) {
		PI_DEBUG (DBG_L2, "UK2k: problem with filename lenght " << strlen(fdatname) );
		return  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
	}
	// Check all known File Types:

 	// split filename in basename and suffix,
	// generate parameter file name
	fbasename = g_strndup (name, strlen(name)-4);
	fsuffix = g_strdup (name+strlen(name)-4);
	fparname = g_strconcat (fbasename, ".stp", NULL);

	if (!strncmp(fsuffix,".STD",4))
		fparname= g_strconcat(fbasename,".STP",NULL);
	// check for known file types
	// accepting topography forward (*.STP, *.stp) files
	if ( strncmp(fsuffix,".STD", 4) && strncmp(fsuffix,".std", 4)  ) {
		PI_DEBUG (DBG_L2, "UK2k: problem with extension " << fsuffix );
		ret = FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
	}
	else{

		// check for data and par file exists and is OK
		std::ifstream f;
		f.open(fdatname, std::ios::in | std::ios::binary);
		if(!f.good()){
			PI_DEBUG (DBG_L2, "UK2k_ImExportFile::Read: data file error" );
			ret = status = FIO_OPEN_ERR;
		}
		f.close();
		f.open(fparname, std::ios::in);
		if(!f.good()){
			PI_DEBUG (DBG_L2, "Uk2k_ImExportFile::Read: parameter file error" );
			ret = status = FIO_OPEN_ERR;
		}
		f.close();

		PI_DEBUG (DBG_L2, "UK2k_ImExportFile::Read: " << fdatname <<
			" is a UK2000 File!" );


		// parse parameter file
		if ( ret == FIO_OK )
			ret = spmReadPar (fparname, fsuffix);

		// read the binary data
		if ( ret == FIO_OK ) {
		  PI_DEBUG (DBG_L2, "UK2k: Read parameter file OK!" );
		  if ( scan->data.s.nvalues == 1 )
			ret = spmReadDat (fdatname);
		}
	}

	g_free (fbasename);
	g_free (fparname);
	g_free (fsuffix);

	scan->mem2d->data->MkYLookup(scan->data.s.y0, scan->data.s.y0+scan->data.s.ry);
	scan->mem2d->data->MkXLookup(scan->data.s.x0+scan->data.s.rx/2., scan->data.s.x0-scan->data.s.rx/2.);
	scan->mem2d->data->MkVLookup (-10., 10.);

	return ret;
}


FIO_STATUS UK2k_ImExportFile::spmReadDat(const gchar *fname)
{
	// handle scan name
	scan->data.ui.SetName (fname);
	scan->data.ui.SetOriginalName (fname);

	scan->mem2d->Resize (
		scan->data.s.nx,
		scan->data.s.ny,
		ZD_SHORT
		);

	PI_DEBUG (DBG_L2, "UK2k_ImExportFile: Resize done." );


	// prepare buffer
	short *buf;
	if(!(buf = g_new(short, scan->data.s.nx*scan->data.s.ny)))
		return FIO_NO_MEM;

	// read the actual data
	ifstream f;
	f.open(fname, ios::in | ios::binary);
	if(f.good())
		f.read((char*)buf, sizeof(short) * scan->data.s.nx * scan->data.s.ny);
	else {
		g_free(buf);
		return FIO_OPEN_ERR;
	}
	f.close();

	// and put the data into mem2d
	short *pt = buf;
	char *cpt, low;
	for (int j=0; j < scan->data.s.ny; ++j)
	  for (int i=0; i < scan->data.s.nx; ++i, ++pt) {
	  // UK2000 files are stored using small endian format
	    if (WORDS_BIGENDIAN)  {
	      cpt = (char*)pt;
	      low = cpt[0];
	      cpt[0] = cpt[1];
	      cpt[1] = low;
	    }
	if (*pt>0) {*pt-=32767;}
		else { *pt+=32767;} // Unsigned instead of signed... Oh well.
	    // Attention: Omicron seems to save last line first!
	    scan->mem2d->PutDataPkt ((double)*pt, i, scan->data.s.ny-j-1);
	  }

	g_free (buf);

	PI_DEBUG (DBG_L2, "UK2k_ImExportFile: Import done." );

	// read done.
	return FIO_OK;
}


FIO_STATUS UK2k_ImExportFile::spmReadPar(const gchar *fname, const gchar *fsuffix)
{
  char valid = FALSE; 
  	char idate[80],itime[80];

  // first set some default parameters...

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


  scan->data.ui.SetType ("UK2000 v3.4 Type: SHT ");

 gchar *comm = g_strconcat ("Imported by UK2k  import plugin from: ",
				 fname,
				 NULL);
      scan->data.ui.SetComment (comm);
      g_free (comm);
      PI_DEBUG (DBG_L2, " comment: " << scan->data.ui.comment );

  // initialize scan structure -- this is a minimum example
  scan->data.s.dx = 1;
  scan->data.s.dy = 1;
  scan->data.s.dz = 1;
  scan->data.s.rx = scan->data.s.nx;
  scan->data.s.ry = scan->data.s.ny;
  scan->data.s.rz = 1;
  scan->data.s.x0 = 0;
  scan->data.s.y0 = 0;
  scan->data.s.alpha = 0;

  // be nice and reset this to some defined state
  scan->data.display.z_high       = 1e3;
  scan->data.display.z_low        = 1.;

  scan->data.display.bright = 0.;
  scan->data.display.vrange_z = 256.;
  scan->data.display.voffset_z = 0.;

  // start the real thing:

  // open the parameter file
  std::ifstream f;
  f.open(fname, std::ios::in);
  if(!f.good())
    return FIO_OPEN_ERR;


  // read the par file line-by-line
  gchar linebuf[100];
  while (!f.eof()) {

    f.getline (linebuf, 100);


  if (!strncmp (linebuf, "software      Uscan2000  3.4",24)) {
	  valid=TRUE;
    }

#if 0
  if (!strncmp (linebuf, "date",4)) {
	sscanf (linebuf, "%*[^0-9]%80s", idate);
	PI_DEBUG (DBG_L2, "UK2k date:" << idate );
    }

    if (!strncmp (linebuf, "time",4)) {
	sscanf (linebuf, "%*[^0-9]%80s", itime);
	PI_DEBUG (DBG_L2, "UK2k time: " << itime );
    }
#else
    idate[0] = 0;
    itime[0] = 0;
#endif

    // range: x
    if ( !strncmp (linebuf, "length_x", 8) ) {
      double rx;
      sscanf (linebuf, "%*[^0-9]%lf", &rx);
      scan->data.s.rx = rx;
      PI_DEBUG (DBG_L2, "UK2k rx = " << scan->data.s.rx );
    }

    // range: y
    if ( !strncmp (linebuf, "length_y", 8) ) {
      double ry;
      sscanf (linebuf, "%*[^0-9]%lf", &ry);
      scan->data.s.ry = ry;
      PI_DEBUG (DBG_L2, "UK2k ry = " << scan->data.s.ry );
    }

    // scan size: nx
    if ( !strncmp (linebuf, "samples_x", 9) ) {
      int nx;
      sscanf (linebuf, "%*[^0-9]%d", &nx);
      scan->data.s.nx = nx;
      PI_DEBUG (DBG_L2, "UK2k nx = " << scan->data.s.nx );
    }

    // scan size: ny
    if ( !strncmp (linebuf, "samples_y", 9) ) {
      int ny;
      sscanf (linebuf, "%*[^0-9]%d", &ny);
      scan->data.s.ny = ny;
      PI_DEBUG (DBG_L2, "UK2k ny = " << scan->data.s.ny );
    }


    // scan angle
    if ( !strncmp (linebuf, "scan_rot", 8) ) {
      double alf;
      sscanf (linebuf, "%*[^0-9-]%lf", &alf);
      scan->data.s.alpha = alf;
      PI_DEBUG (DBG_L2, "UK2k alpha = " << scan->data.s.alpha );
    }

    // offset: x
    if ( !strncmp (linebuf, "start_x", 7) ) {
      double x0;
      sscanf (linebuf, "%*[^0-9-]%lf", &x0);
      scan->data.s.x0 = x0;
      PI_DEBUG (DBG_L2, "UK2k x0 = " << scan->data.s.x0 );
    }

    // offset: y
    if ( !strncmp (linebuf, "start_y", 7) ) {
      double y0;
      sscanf (linebuf, "%*[^0-9-]%lf", &y0);
      scan->data.s.y0 = y0;
      PI_DEBUG (DBG_L2, "UK2k y0 = " << scan->data.s.y0 );
    }

    // topo channel information
    if ( !strncmp (linebuf, "servo_range", 11) ) {
      double dz;
	sscanf (linebuf, "%*[^0-9-]%lf", &dz);
	// Actually, the servo_range is the full range in nm, converted into 12 bits
	dz/=16384;
	PI_DEBUG (DBG_L2, "UK2k z unit: nm" );
	PI_DEBUG (DBG_L2, "UK2k dz:" << dz );
	UnitObj *zu = main_get_gapp()->xsm->MakeUnit ("nm","nm");
	scan->data.s.dz = zu->Usr2Base(dz);
	scan->data.SetZUnit(zu);
	delete zu;
    }

    // gap voltage
    if ( !strncmp (linebuf, "z_bias", 6) ) {
      double ugap;
	sscanf (linebuf, "%*[^0-9-]%lf", &ugap);
	scan->data.s.Bias = ugap/1000.0;
	PI_DEBUG (DBG_L2, "UK2k ugap = " << scan->data.s.Bias );
    }

    // current feedback setpoint
    // missing unit support!
    if ( !strncmp (linebuf, "setpoint", 8) ) {
      double iset;
      sscanf (linebuf, "%*[^0-9-]%lf", &iset);
      scan->data.s.Current = iset;
      PI_DEBUG (DBG_L2, "UK2k iset = " << scan->data.s.Current );
    }

  }

  f.close();

 gchar *tmp=g_strconcat (itime, " ", idate, NULL);
//  scan->data.ui.SetDateOfScan (tmp);
  scan->data.ui.SetDateOfScan ("---\n");
  g_free (tmp);

// step size: x
   scan->data.s.dx = scan->data.s.rx / scan->data.s.nx;
   PI_DEBUG (DBG_L2, " dx = " << scan->data.s.dx );

// step size: y
   scan->data.s.dy = scan->data.s.ry/scan->data.s.ny;
   PI_DEBUG (DBG_L2, " dy = " << scan->data.s.dy );

  // test for various errors:
//  if (!valid) return FIO_INVALID_FILE;
//  if (!valid return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

  return FIO_OK;
}




FIO_STATUS UK2k_ImExportFile::Write()
{
  return FIO_OK;
}


// Plugin's Notify Cb's, registered to be called on file load/save to check file
// return via filepointer, it is set to Zero or passed as Zero if file has been processed!
// That's all fine, you should just change the Text Stings below...


static void UK2k_import_filecheck_load_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		PI_DEBUG (DBG_L2, "Check File: UK2k_import_filecheck_load_callback called with >"
		     << *fn << "<" );

		Scan *dst = main_get_gapp()->xsm->GetActiveScan();
		if(!dst){
			main_get_gapp()->xsm->ActivateFreeChannel();
			dst = main_get_gapp()->xsm->GetActiveScan();
		}
		UK2k_ImExportFile fileobj (dst, *fn);

		FIO_STATUS ret = fileobj.Read();
		if (ret != FIO_OK){
			// I'am responsible! (But failed)
			if (ret != FIO_NOT_RESPONSIBLE_FOR_THAT_FILE) {
				*fn=NULL;
				PI_DEBUG (DBG_L2, "UK2k: Read Error " << ((int)ret) << "!!!!!!!!" );
			} else 
				PI_DEBUG (DBG_L2, "UK2k: Not a UK2k file!" );
			// no more data: remove allocated and unused scan now, force!
//			main_get_gapp()->xsm->SetMode(-1, ID_CH_M_OFF, TRUE);
		}else{
			// got it!
			*fn=NULL;

			// Now update gxsm main window data fields
			main_get_gapp()->xsm->ActiveScan->GetDataSet(main_get_gapp()->xsm->data);
			main_get_gapp()->spm_update_all();
			dst->draw();
		}
	}else{
		PI_DEBUG (DBG_L2, "UK2k_import_filecheck_load: Skipping" );
	}
}

//static void UK2k_import_filecheck_save_callback (gpointer data ){
//	gchar **fn = (gchar**)data;
//	if (*fn){
//		Scan *src;
//		PI_DEBUG (DBG_L2, "Check File: UK2k_import_filecheck_save_callback called with >"
//		     << *fn << "<" );

//		UK2k_ImExportFile fileobj (src = main_get_gapp()->xsm->GetActiveScan(), *fn);

//		FIO_STATUS ret;
//		ret = fileobj.Write();

//		if(ret != FIO_OK){
			// I'am responsible! (But failed)
//			if (ret != FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
//				*fn=NULL;
//			PI_DEBUG (DBG_L2, "Write Error " << ((int)ret) << "!!!!!!!!" );
//		}else{
//			// write done!
//			*fn=NULL;
//		}
//	}else{
//		PI_DEBUG (DBG_L2, "UK2k_import_filecheck_save: Skipping >" << *fn << "<" );
//	}
//}

// Menu Call Back Fkte

static void UK2k_import_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
//				  "known extensions: std, stp, STD, STP",
	gchar **help = g_strsplit (UK2k_import_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (UK2k_import_pi.name, "-import\n", UK2k_import_pi.info, NULL);
	gchar *fn = main_get_gapp()->file_dialog_load (help[0], NULL, file_mask, NULL);
	//	gchar *fn = main_get_gapp()->file_dialog("UK2k Import", NULL,
	//				  "*.std",
	//				  NULL, "UK2k-Import");
	if (fn){
                PI_DEBUG (DBG_L2, "FLDLG-IM::" << fn );
                UK2k_import_filecheck_load_callback (&fn );
                g_free (fn);
	}
}


static void UK2k_import_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
//				      "known extensions: std, stp, STD, STP",
//	gchar **help = g_strsplit (UK2k_import_pi.help, ",", 2);
//	gchar *dlgid = g_strconcat (UK2k_import_pi.name, "-import\n", UK2k_import_pi.info, NULL);
//  *** gchar *fn = main_get_gapp()->file_dialog_save (help[1], NULL, file_mask, NULL);
//	gchar *fn = main_get_gapp()->file_dialog("UK2k Export", NULL,
//				      "*.std",
//				      "","UK2k-Export");

//    PI_DEBUG (DBG_L2, "FLDLG-EX::" << fn );
//   UK2k_import_filecheck_save_callback (&fn );
}
