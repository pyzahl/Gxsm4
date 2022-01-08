/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Quick nine points Gxsm Plugin GUIDE by PZ:
 * ------------------------------------------------------------
 * 1.) Make a copy of this "nanoimport.C" to "your_plugins_name.C"!
 * 2.) Replace all "nanoimport" by "your_plugins_name" 
 *     --> please do a search and replace starting here NOW!! (Emacs doese preserve caps!)
 * 3.) Decide: One or Two Source Math: see line 54
 * 4.) Fill in GxsmPlugin Structure, see below
 * 5.) Replace the "about_text" below a desired
 * 6.) * Optional: Start your Code/Vars definition below (if needed more than the run-fkt itself!), 
 *       Goto Line 155 ff. please, and see comment there!!
 * 7.) Fill in math code in sdfimport_run(), 
 *     have a look at the Data-Access methods infos at end
 * 8.) Add sdfimport.C to the Makefile.am in analogy to others
 * 9.) Make a "make; make install"
 * A.) Call Gxsm->Tools->reload Plugins, be happy!
 * ... That's it!
 * 
 * Gxsm Plugin Name: sdfimport.C
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
% PlugInDocuCaption: Import of Surface Data Format files
% PlugInName: sdfimport
% PlugInAuthor: Stefan Schroeder
% PlugInAuthorEmail: stefan_fkp@users.sf.net
% PlugInMenuPath: File/Import/SDF-import

% PlugInDescription
\label{plugins:sdfimport}
The \GxsmEmph{sdfimport} plug-in supports reading files
as defined by the surfstand group (search internet for 'surfstand'),
a group, sponsored by the EU to develop a basis for 3D surface
roughness standards.

% PlugInUsage
The plug-in is called by \GxsmMenu{File/Import/SDF-import}.

% OptPlugInNotes
No endianess independent code -- no cross platform data exchange possible.

% OptPlugInKnownBugs
The scaling of the data, especially in Z direction is not well tested.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "dataio.h"
//#include "action_id.h" // should not be needed, 15.11.2009 Thorsten Wagner (STM)

#include "glbvars.h"
#include "surface.h"


using namespace std;

// Plugin Prototypes
static void sdfimport_init( void );
static void sdfimport_about( void );
static void sdfimport_configure( void );
static void sdfimport_cleanup( void );

static void sdfimport_run(GtkWidget *w, void *data);

// Fill in the GxsmPlugin Description here
GxsmPlugin sdfimport_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "SDFimport",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
 g_strdup ("Imports from SDF Data."),
  // Author(s)
  "Stefan Schroeder",
  // Menupath to position where it is appendet to
  "file-import-section",
  // Menuentry
  N_("SDF"),
  // help text shown on menu
  N_("Import SDF File"),
  // more info...
  "no more info",
  NULL, // error msg, plugin may put error status msg here later
  NULL, // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  sdfimport_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  sdfimport_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  sdfimport_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  sdfimport_run, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  sdfimport_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm SDF Data Import Plugin\n\n"
                                   "This plugin reads in a datafile\n"
				   "in sdf-format.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  sdfimport_pi.description = g_strdup_printf(N_("Gxsm MathOneArg sdfimport plugin %s"), VERSION);
  return &sdfimport_pi; 
}

// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void sdfimport_init(void)
{
  PI_DEBUG (DBG_L2, "sdfimport Plugin Init" );
}

// about-Function
static void sdfimport_about(void)
{
  const gchar *authors[] = { sdfimport_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  sdfimport_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void sdfimport_configure(void)
{
  if(sdfimport_pi.app)
    sdfimport_pi.app->message("sdfimport Plugin Configuration");
}

// cleanup-Function
static void sdfimport_cleanup(void)
{
  PI_DEBUG (DBG_L2, "sdfimport Plugin Cleanup" );
}

class SDFFile : public Dataio{
public:
  SDFFile(Scan *s, const char *n) : Dataio(s,n){};
  virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
  virtual FIO_STATUS Write(){ return FIO_OK; };
private:
  int dummy;
};

FIO_STATUS SDFFile::Read(xsm::open_mode mode){

  PI_DEBUG (DBG_L1, "***** Importing SDF-file. *****" );

  ifstream SDFinputstream;
  char SDFversionnumber [9];
  char SDFmanufacturerid [11];
  char SDFcreationdate [13];
  char SDFmodificationtime [13];
  short SDFnumpoints;
  short SDFnumprofiles;
  double SDFxscale;
  double SDFyscale;
  double SDFzscale;
  double SDFzresolution;
  char SDFcompression [2];
  char SDFdatatype [2];
  char SDFchecktype [2];

  SDFinputstream.open(name, ios::in);

  // mostly copied from nanoscope importfilter.
  if(!SDFinputstream.good())
    return status=FIO_OPEN_ERR;
  // Initialize with default!

  PI_DEBUG (DBG_L1, "Reading data... -- this is as worse as it could be to be portable:-(" );;
  SDFinputstream.read((char*)&SDFversionnumber, 8);
  SDFversionnumber[8] = '\0';
  SDFinputstream.read((char*)&SDFmanufacturerid, 10);
  SDFmanufacturerid[10] = '\0';
  SDFinputstream.read((char*)&SDFcreationdate, 12);
  SDFcreationdate[12] = '\0';
  SDFinputstream.read((char*)&SDFmodificationtime, 12);
  SDFmodificationtime[12] = '\0';
  SDFinputstream.read((char*)&SDFnumpoints, 2);
  SDFinputstream.read((char*)&SDFnumprofiles, 2);
  SDFinputstream.read((char*)&SDFxscale, 8);
  SDFinputstream.read((char*)&SDFyscale, 8);
  SDFinputstream.read((char*)&SDFzscale, 8);
  SDFinputstream.read((char*)&SDFzresolution, 8);
  SDFinputstream.read((char*)&SDFcompression, 1);
  SDFcompression[1] = '\0';
  SDFinputstream.read((char*)&SDFdatatype, 1);
  SDFdatatype[1] = '\0';
  SDFinputstream.read((char*)&SDFchecktype, 1);
  SDFchecktype[1] = '\0';

  PI_DEBUG (DBG_L1, "Filename = " << name );
  PI_DEBUG (DBG_L1, "Versionnumber = " << SDFversionnumber );
  PI_DEBUG (DBG_L1, "Manufacturer = " << SDFmanufacturerid );
  PI_DEBUG (DBG_L1, "Creationdate = " << SDFcreationdate );
  PI_DEBUG (DBG_L1, "Modificationtime = " << SDFmodificationtime );
  PI_DEBUG (DBG_L1, "Number of points = " << SDFnumpoints );
  PI_DEBUG (DBG_L1, "Number of profiles = " << SDFnumprofiles );
  PI_DEBUG (DBG_L1, "Xscale = " << SDFxscale );
  PI_DEBUG (DBG_L1, "Yscale = " << SDFyscale );
  PI_DEBUG (DBG_L1, "Zscale = " << SDFzscale );
  PI_DEBUG (DBG_L1, "Zresolution = " << SDFzresolution );
  PI_DEBUG (DBG_L1, "Compression = " << SDFcompression );
  PI_DEBUG (DBG_L1, "Datatype = " << SDFdatatype );
  PI_DEBUG (DBG_L1, "Checktype = " << SDFchecktype );

  /* convert Header data to mem2D data. */
  scan->data.s.nx = SDFnumpoints;
  scan->data.s.ny = SDFnumprofiles;
  scan->data.s.dx = SDFxscale * 10e9; // unit?
  scan->data.s.dy = SDFyscale * 10e9; // unit?
  scan->data.s.dz = SDFzscale * 10e9;
  scan->data.s.rx = SDFxscale * 10e9 * SDFnumpoints;
  scan->data.s.ry = SDFyscale * 10e9 * SDFnumprofiles;

  scan->data.ui.SetUser (SDFmanufacturerid);

  gchar *tmp=g_strconcat ("SDF-fileimport: ",
			  "Compression: ,", SDFcompression,
			  "Datatype: ,", SDFdatatype,
			  "Creationdate: ", SDFcreationdate,
			  NULL);
  scan->data.ui.SetComment (tmp);
  g_free (tmp);

  PI_DEBUG (DBG_L1, "Reading data...");

  scan->mem2d->Resize (scan->data.s.nx, scan->data.s.ny);
  scan->mem2d->DataRead (SDFinputstream);

  scan->data.orgmode = SCAN_ORG_CENTER;
  scan->mem2d->data->MkXLookup (-scan->data.s.rx/2., scan->data.s.rx/2.);
  scan->mem2d->data->MkYLookup (scan->data.s.ry/2., -scan->data.s.ry/2.);

  if(SDFinputstream.fail()){
    SDFinputstream.close();
    return status=FIO_READ_ERR; 
  }

  PI_DEBUG (DBG_L1, "***** Finished importing SDF-file. *****");
  SDFinputstream.close(); // finished all ops with file.

  return status=FIO_OK;
}


// run-Function
static void sdfimport_run(GtkWidget *w, void *data)
{
    Scan *dst;
    gchar *nfname = main_get_gapp()->file_dialog("sdf-file to load", NULL, 
				      "*.sdf", NULL, "sdf-Import");
    if( !nfname ) return;

    main_get_gapp()->xsm->ActivateFreeChannel();
    SDFFile sdfFile(dst = main_get_gapp()->xsm->GetActiveScan(), nfname);

    if(!dst){ 
      main_get_gapp()->xsm->ActivateFreeChannel();
      dst = main_get_gapp()->xsm->GetActiveScan();
    }
    if(sdfFile.Read() != FIO_OK){ 
      // no more data: remove allocated and unused scan now, force!
//      main_get_gapp()->xsm->SetMode(-1, ID_CH_M_OFF, TRUE); 
    }
    main_get_gapp()->spm_update_all();
    dst->draw();
    dst=NULL;
}
