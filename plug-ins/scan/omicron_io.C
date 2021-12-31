/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: omicron_io.C
 * ========================================
 * 
 * Copyright (C) 2001 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Andreas Klust <klust@fkp.uni-hannover.de>
 * some enhancements/changes:	Jan Beyer <beyer@ifm.liu.se>
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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Please do not change the Begin/End lines of this comment section!
 * this is a LaTeX style section used for auto generation of the PlugIn Manual 
 * Chapter. Add a complete PlugIn documentation inbetween the Begin/End marks!
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Import/export of Scala SPM files (Omicron)

% PlugInName: Omicron_IO

% PlugInAuthor: Andreas Klust

% PlugInAuthorEmail: klust@users.sf.net

% PlugInMenuPath: File/Import/Omicron_SPM_Import

% PlugInDescription

This plug-in is responsible for the import and export of data in the SPM
format used by Omicron's Scala software.  It supports both conventional,
two-dimensional data such as topographic images and gridded spectroscopic
data.  Data acquired using the point spectroscopy mode of the Scala system
is not written in the SPM format.  Therefore, point spectroscopy data is
ignored by this plug-in.

In the Scala SPM format, data is saved in two parts:  Firstly, the data
itself as binary file.  Secondly, additional information, e.g.\ scan size,
in an extra ASCII file with the filename suffix ".par".  This parameter file
can contain information on several images.  Therefore, for importing SPM
files, the binary file must be selected.  The parameter file contains many
comments making it human readable.

% PlugInUsage
When the Omicron\_IO plug-in is installed, Scala SPM files can be loaded like
any other files supported by \Gxsm.  Alternatively, 
\GxsmMenu{File/Import/Omicron SPM Import} can be used.


% OptPlugInKnownBugs
Exporting data in the Scala SPM format is not yet implemented.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include <locale.h>

#include "config.h"
#include "core-source/plugin.h"
#include "core-source/dataio.h"
#include "core-source/action_id.h"
#include "core-source/mem2d.h"
#include "core-source/unit.h"

#ifndef WORDS_BIGENDIAN
# define WORDS_BIGENDIAN 0
#endif

using namespace std;


// Plugin Prototypes
static void omicron_io_init (void);
static void omicron_io_query (void);
static void omicron_io_about (void);
static void omicron_io_configure (void);
static void omicron_io_cleanup (void);

static void omicron_io_filecheck_load_callback (gpointer data );
static void omicron_io_filecheck_save_callback (gpointer data );

static void omicron_io_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
//static void omicron_io_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);




// GxsmPlugin Description

GxsmPlugin omicron_io_pi = {
        NULL,                   // filled in and used by Gxsm, don't touch !
        NULL,                   // filled in and used by Gxsm, don't touch !
        0,                      // filled in and used by Gxsm, don't touch !
        NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
        // filled in here by Gxsm on Plugin load, 
        // just after init() is called !!!
        "Omicron_IO",
        NULL,                   // PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
        // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
        g_strdup ("Im/Export of Omicron SPM files."),
        "Andreas Klust",
        "file-import-section,file-export-section", // sep. im/export menuentry path by comma!
        N_("Omicron Scala,Omicron Scala"),
        "Omicron Scala format",
        "This plug-in is responsible for the im/export of data in the file format used by Omicron's SCALA software.",
        NULL,          // error msg, plugin may put error status msg here later
        NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
        omicron_io_init,  
        omicron_io_query,  
        // about-function, can be "NULL"
        // can be called by "Plugin Details"
        omicron_io_about,
        // configure-function, can be "NULL"
        // can be called by "Plugin Details"
        omicron_io_configure,
        // run-function, can be "NULL", if non-Zero and no query defined, 
        // it is called on menupath->"plugin"
        NULL,
        // cleanup-function, can be "NULL"
        // called if present at plugin removal
        omicron_io_import_callback, // direct menu entry callback1 or NULL
        NULL, // direct menu entry callback2 or NULL

        omicron_io_cleanup
};




// Text used in Aboutbox
static const char *about_text = N_("Gxsm data file import/export plugin\n\n"
                                   "This plugin reads Omicron SCALA SPM files."
                                   );




// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
        omicron_io_pi.description = g_strdup_printf(N_("Gxsm omicron_io plugin %s"), VERSION);
        return &omicron_io_pi; 
}





// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/Omicron SPM import"
// Export Menupath is "File/Export/Omicron SPM export"
// ----------------------------------------------------------------------
// !!!! make sure the "omicron_io_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!

static void omicron_io_query(void)
{

        if(omicron_io_pi.status) g_free(omicron_io_pi.status); 
        omicron_io_pi.status = g_strconcat (
                                            N_("Plugin query has attached "),
                                            omicron_io_pi.name, 
                                            N_(": File IO Filters are ready to use"),
                                            NULL);

        // register this plugins filecheck functions with Gxsm now!
        // This allows Gxsm to check files from DnD, open, 
        // and cmdline sources against all known formats automatically - no explicit im/export is necessary.
        omicron_io_pi.app->ConnectPluginToLoadFileEvent (omicron_io_filecheck_load_callback);
        omicron_io_pi.app->ConnectPluginToSaveFileEvent (omicron_io_filecheck_save_callback);
}




// init-Function

static void omicron_io_init(void)
{
        PI_DEBUG (DBG_L2, "omicron_io Plugin Init" );
}





// about-Function

static void omicron_io_about(void)
{
        const gchar *authors[] = { omicron_io_pi.authors, NULL};
        gtk_show_about_dialog (NULL, 
                               "program-name",  omicron_io_pi.name,
                               "version", VERSION,
                               "license", GTK_LICENSE_GPL_3_0,
                               "comments", about_text,
                               "authors", authors,
                               NULL
                               );
}





// configure-Function

static void omicron_io_configure(void)
{
        if(omicron_io_pi.app)
                omicron_io_pi.app->message("omicron_io Plugin Configuration");
}





// cleanup-Function, make sure the Menustrings are matching those above!!!

static void omicron_io_cleanup(void)
{
	PI_DEBUG (DBG_L2, "omicron_io Plugin Cleanup" );
#if 0
        gnome_app_remove_menus (GNOME_APP (omicron_io_pi.app->getApp()),
                                N_("File/Import/Omicron SPM Import"), 1);
        //   gnome_app_remove_menus (GNOME_APP (omicron_io_pi.app->getApp()),
        // 			  N_("File/Export/Omicron SPM Export"), 1);
#endif
}





// make a new derivate of the base class "Dataio"

class Omicron_SPM_ImExportFile : public Dataio{
public:
        Omicron_SPM_ImExportFile(Scan *s, const char *n) : Dataio(s,n){ };
        virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
        virtual FIO_STATUS Write();
private:
        // binary data read
        FIO_STATUS spmReadDat(const gchar *fname);
        // gridded spectroscopy binary data read
        FIO_STATUS spmReadGridSpecDat(const gchar *fname);
        // parses ASCII parameter files
        FIO_STATUS spmReadPar(const gchar *fname, const gchar *fsuffix);

};



// spm import
FIO_STATUS Omicron_SPM_ImExportFile::Read(xsm::open_mode mode){
	FIO_STATUS ret = FIO_OK;
	gchar *fdatname = (gchar*) name;
	gchar *fparname=NULL;
	gchar *fbasename=NULL;
	gchar *fsuffix=NULL;
	
	// name should have at least 4 chars: ".ext"
	if (name == NULL || strlen(name) < 4){
		ret = FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
		return ret;
	}
	
	// split filename in basename and suffix,
	// generate parameter file name
	fbasename = g_strndup (name, strlen(name)-4);
	fsuffix = g_strdup (name+strlen(name)-4);
	fparname = g_strconcat (fbasename, ".par", NULL);
	
	// check for known file types
	// accepting topography forward (*.tf?) and backward (*.tb?) files
	// as well as (gridded) spectroscopy data (*.sf? and *.sb?)
	if ( strncmp(fsuffix,".tf", 3) && strncmp(fsuffix,".tb", 3) && \
	     strncmp(fsuffix,".sf", 3) && strncmp(fsuffix,".sb", 3) )
		ret = FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
	else{  
		
		// check for data and par file exists and is OK
		std::ifstream f;
		f.open(fdatname, std::ios::in | std::ios::binary);
		if(!f.good()){
			PI_DEBUG (DBG_L2, "Omicron_SPM_ImExportFile::Read: data file error" );
			ret = status = FIO_OPEN_ERR;
		}
		f.close();
		f.open(fparname, std::ios::in);
		if(!f.good()){
			PI_DEBUG (DBG_L2, "Omicron_SPM_ImExportFile::Read: parameter file error" );
			ret = status = FIO_OPEN_ERR;
		}
		f.close();
		
		
		PI_DEBUG (DBG_L2, "Omicron_SPM_ImExportFile::Read: " << fdatname << 
                          " is a Omicron SPM File!" );
		
		
		// parse parameter file
		if ( ret == FIO_OK )
			ret = spmReadPar (fparname, fsuffix);
		
		// read the binary data
		if ( ret == FIO_OK ) {
                        if ( scan->data.s.nvalues == 1 )
                                ret = spmReadDat (fdatname);
                        else
                                ret = spmReadGridSpecDat (fdatname);
		}
	}

	g_free (fbasename);
	g_free (fparname);
	g_free (fsuffix);

	scan->mem2d->data->MkYLookup(scan->data.s.y0, scan->data.s.y0+scan->data.s.ry);
	scan->mem2d->data->MkXLookup(scan->data.s.x0-scan->data.s.rx/2., scan->data.s.x0+scan->data.s.rx/2.);
	scan->mem2d->data->MkVLookup (-10., 10.);
	
	return ret;
}


FIO_STATUS Omicron_SPM_ImExportFile::spmReadDat(const gchar *fname)
{
	// handle scan name
	scan->data.ui.SetName (fname);
	scan->data.ui.SetOriginalName (fname);
	
	// lets make a scan, and SPM files are of type short!
	scan->mem2d->Resize (
                             scan->data.s.nx,
                             scan->data.s.ny,
                             ZD_SHORT
                             );

	PI_DEBUG (DBG_L2, "Omicron_SPM_ImExportFile: Resize done." );
	
	
	// prepare buffer
	short *buf;
	if(!(buf = g_new(short, scan->data.s.nx*scan->data.s.ny)))
		return FIO_NO_MEM;
	
	// read the actual data  
	std::ifstream f;
	f.open(fname, std::ios::in | std::ios::binary);
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
                        // Scala SPM files are stored using big endian format
                        if (!WORDS_BIGENDIAN)  {
                                cpt = (char*)pt;
                                low = cpt[0];
                                cpt[0] = cpt[1];
                                cpt[1] = low;
                        }

                        // Attention: Omicron seems to save last line first!
                        scan->mem2d->PutDataPkt ((double)*pt, i, scan->data.s.ny-j-1);
                }

	g_free (buf);
	
	PI_DEBUG (DBG_L2, "Omicron_SPM_ImExportFile: Import done." );
	
	// read done.
	return FIO_OK; 
}


FIO_STATUS Omicron_SPM_ImExportFile::spmReadGridSpecDat(const gchar *fname) 
{
        // handle scan name
	scan->data.ui.SetName (fname);
	scan->data.ui.SetOriginalName (fname);
	
	// lets make a spectro grid, and SPM files are of type short!
	scan->mem2d->Resize (
                             scan->data.s.nx,
                             scan->data.s.ny,
                             scan->data.s.nvalues,
                             ZD_SHORT
                             );

	PI_DEBUG (DBG_L2, "Omicron_SPM_ImExportFile: Resize done." );
	
       	// prepare buffer
	short *buf;
	if(!(buf = g_new(short, scan->data.s.nx*scan->data.s.ny)))
		return FIO_NO_MEM;

	// read the actual data
	// and put the data into mem2d
	ifstream f;
	f.open(fname, ios::in | ios::binary);
	for (int v=0; v < scan->data.s.nvalues; ++v) {
                if(f.good())
                        f.read((char*)buf, sizeof(short) * scan->data.s.nx * scan->data.s.ny);
                else {
                        g_free(buf);
                        return FIO_OPEN_ERR;  
                }

                // and put the data into mem2d
                short *pt = buf;
                char *cpt, low;
                for (int j=0; j < scan->data.s.ny; ++j)
                        for (int i=0; i < scan->data.s.nx; ++i, ++pt) {
                                // Scala SPM files are stored using big endian format
                                if (!WORDS_BIGENDIAN)  {
                                        cpt = (char*)pt;
                                        low = cpt[0];
                                        cpt[0] = cpt[1];
                                        cpt[1] = low;
                                }

                                // Attention: Omicron seems to save last line first!
                                scan->mem2d->PutDataPkt ((double)*pt, i, scan->data.s.ny-j-1, v);
                        }
	  
	}
	f.close();
	
	g_free (buf);
	
	PI_DEBUG (DBG_L2, "Omicron_SPM_ImExportFile: Import done." );
	
	// read done.
	return FIO_OK; 
}




FIO_STATUS Omicron_SPM_ImExportFile::spmReadPar(const gchar *fname, const gchar *fsuffix)
{
        char valid = FALSE, spectro = FALSE, spectrogrid = FALSE, dualmode = FALSE;
        int xspecpoints, yspecpoints;
        GString *comment = g_string_new(NULL);

        // first set some default parameters...
        setlocale(LC_NUMERIC, "C");		/* Scala SPM files have dots as decimal separator */

        // this is mandatory.
        scan->data.s.ntimes  = 1;
        scan->data.s.nvalues = 1;

        // put some usefull values in the ui structure
        if(getlogin()){
                scan->data.ui.SetUser (getlogin());
        }
        else{
                scan->data.ui.SetUser ("unknown user");
        }
        /* time_t t; // Scan - Startzeit eintragen 
           time(&t);
           gchar *tmp=g_strconcat ((ctime(&t)), " (Imported)", NULL);
           scan->data.ui.SetDateOfScan (tmp);
           g_free (tmp); */
  
        scan->data.ui.SetType ("Omicron SPM Type: SHT ");

        /* tmp=g_strconcat ("Imported by gxsm Omicron SPM import plugin from: ",
           fname,
           NULL);
           scan->data.ui.SetComment (tmp);
           g_free (tmp); */
  
        // initialize scan structure -- this is a minimum example
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
  
        // start the real thing:

        // open the parameter file
        std::ifstream f;
        f.open(fname, std::ios::in);
        if(!f.good())
                return FIO_OPEN_ERR;  

        UnitObj *xu = gapp->xsm->MakeUnit ("nm", "X");
        scan->data.SetXUnit(xu);
        delete xu;

        UnitObj *yu = gapp->xsm->MakeUnit ("nm", "Y");
        scan->data.SetYUnit(yu);
        delete yu;

        /* read the par file line-by-line */
        gchar linebuf[100];
        while (!f.eof()) {
    
                f.getline (linebuf, 100);

                /* Scan Date */
                if ( !strncmp (linebuf, "Date", 4)) {
                        gchar scandate[11];
                        gchar scantime[6];
                        sscanf (linebuf, "%*[^0-9]%10s %6s", scandate, scantime);
                        scan->data.ui.SetDateOfScan ( g_strconcat(scandate, " ", scantime, NULL) );
                }

                /* user name */
                if ( !strncmp (linebuf, "User", 4) ) {
                        char user[80];
                        sscanf (linebuf, "%*[^:]: %80s", user);
                        scan->data.ui.SetUser (user);
                        PI_DEBUG (DBG_L2, " user: " << scan->data.ui.user );
                }

                /* Comment */
                if ( !strncmp (linebuf, "Comment", 7) ) {
                        /*		GString *comment = g_string_new(NULL);*/
                        gchar c[100];
                        sscanf (linebuf, "%*[^:]: %[^\n]100s", c);
                        g_string_append(comment, c);
                        //g_free(c);
                        f.getline(linebuf,100);
                        while (strncmp (linebuf,";",1) != 0) {
                                sscanf(linebuf,"%*[ ]%[^\n]100s",linebuf);		/* Remove leading whitespaces*/
                                g_string_append (comment, linebuf);
                                f.getline(linebuf,100);
                        }
                        scan->data.ui.SetComment (comment->str);	/* This will destroy the former "imported by..." comment */
                        g_string_free(comment, TRUE);
                        PI_DEBUG (DBG_L2, " comment: " << scan->data.ui.comment );
                }

                // range: x
                if ( !strncmp (linebuf, "Field X Size in nm", 18) ) {
                        double rx;
                        sscanf (linebuf, "%*[^0-9]%lf", &rx);
                        scan->data.s.rx = rx * 10;
                        PI_DEBUG (DBG_L2, " rx = " << scan->data.s.rx );
                }

                // range: y
                if ( !strncmp (linebuf, "Field Y Size in nm", 18) ) {
                        double ry;
                        sscanf (linebuf, "%*[^0-9]%lf", &ry);
                        scan->data.s.ry = ry * 10;
                        PI_DEBUG (DBG_L2, " ry = " << scan->data.s.ry );
                }

                // scan size: nx
                if ( !strncmp (linebuf, "Image Size in X", 15) ) {
                        int nx;
                        sscanf (linebuf, "%*[^0-9]%d", &nx);
                        scan->data.s.nx = nx;
                        PI_DEBUG (DBG_L2, " nx = " << scan->data.s.nx );
                }

                // scan size: ny
                if ( !strncmp (linebuf, "Image Size in Y", 15) ) {
                        int ny;
                        sscanf (linebuf, "%*[^0-9]%d", &ny);
                        scan->data.s.ny = ny;
                        PI_DEBUG (DBG_L2, " ny = " << scan->data.s.ny );
                }

                // step size: x
                if ( !strncmp (linebuf, "Increment X", 11) ) {
                        double dx;
                        sscanf (linebuf, "%*[^0-9]%lf", &dx);
                        scan->data.s.dx = dx * 10;
                        PI_DEBUG (DBG_L2, " dx = " << scan->data.s.dx );
                }

                // step size: y
                if ( !strncmp (linebuf, "Increment Y", 11) ) {
                        double dy;
                        sscanf (linebuf, "%*[^0-9]%lf", &dy);
                        scan->data.s.dy = dy * 10;
                        PI_DEBUG (DBG_L2, " dy = " << scan->data.s.dy );
                }

                // scan angle
                if ( !strncmp (linebuf, "Scan Angle", 10) ) {
                        double alf;
                        sscanf (linebuf, "%*[^0-9-]%lf", &alf);
                        scan->data.s.alpha = alf;
                        PI_DEBUG (DBG_L2, " alpha = " << scan->data.s.alpha );
                }

                // offset: x
                if ( !strncmp (linebuf, "X Offset", 8) ) {
                        double x0;
                        sscanf (linebuf, "%*[^0-9-]%lf", &x0);
                        scan->data.s.x0 = x0 * 10;
                        PI_DEBUG (DBG_L2, " x0 = " << scan->data.s.x0 );
                }

                // offset: y
                if ( !strncmp (linebuf, "Y Offset", 8) ) {
                        double y0;
                        sscanf (linebuf, "%*[^0-9-]%lf", &y0);
                        scan->data.s.y0 = y0 * 10;
                        PI_DEBUG (DBG_L2, " y0 = " << scan->data.s.y0 );
                }


                // topo channel information
                if ( !strncmp (linebuf, "Topographic Channel", 19) ) {
                        double dz;
                        double lrz, urz;			/* lower and upper Range Z */
                        char fn[100], zunit[3], zlabel[2];
                        gchar scantype[20], datatype[5];
                        gchar *fs;
      
                        f.getline (linebuf, 100); // forward vs. backward
                        sscanf(linebuf, "%*[ ]%8s",scantype);
                        f.getline (linebuf, 100); // min raw val
                        f.getline (linebuf, 100); // max raw val
      
                        f.getline (linebuf, 100); // min phys val
                        sscanf (linebuf, " %lf", &lrz);
                        f.getline (linebuf, 100); // max phys val
                        sscanf (linebuf, " %lf", &urz);
                        f.getline (linebuf, 100); // resolution
                        sscanf (linebuf, " %lf", &dz);
                        f.getline (linebuf, 100); // phys. unit
                        sscanf (linebuf, "%*[ ]%2s", zunit);
                        f.getline (linebuf, 100); // dat filename
                        sscanf (linebuf, " %100s", fn);
                        f.getline (linebuf, 100); // display name
                        sscanf (linebuf, "%*[ ]%5s", datatype);
 
                        // do we read the correct par set?
                        fs = g_strdup (fn+strlen(fn)-4);
                        if (!strcmp(fs,fsuffix)) {  // Yes, we do!
                                valid = TRUE;
                                scan->data.ui.SetType (g_strconcat (scantype, " ", datatype, NULL));

                                if (strcmp(zunit,"nm") == 0) {
                                        scan->data.s.rz = (abs(lrz) + abs(urz))*10;
                                        scan->data.s.dz = dz*10;
                                        strcpy(zunit, "AA");
                                        strcpy(zlabel, "Z");
                                }
                                else {
                                        scan->data.s.rz = (abs(lrz) + abs(urz));
                                        scan->data.s.dz = dz;
                                        strcpy(zlabel, "I");		/* Topography always in nm, otherwise current ??? At least up to my little experience...*/
                                }

                                PI_DEBUG (DBG_L2, " z unit: " << zunit );
                                UnitObj *zu = gapp->xsm->MakeUnit (zunit, zlabel);
                                scan->data.SetZUnit(zu);
                                delete zu;
                        }
                }
    
    
                // spectroscopic channel information
                if ( !strncmp (linebuf, "Spectroscopy Channel", 20) ) {
                        double ds;
                        char fn[100], zunit[6], vunit[6];
                        gchar *fs;
                        int nv;
      
                        f.getline (linebuf, 100); // parameter
                        f.getline (linebuf, 100); // forward vs. backward
                        f.getline (linebuf, 100); // min raw val
                        f.getline (linebuf, 100); // max raw val
                        f.getline (linebuf, 100); // min phys val
                        f.getline (linebuf, 100); // max phys val
                        f.getline (linebuf, 100); // resolution
                        sscanf (linebuf, " %lf", &ds);
                        f.getline (linebuf, 100); // phys. unit
                        sscanf (linebuf, " %5s", zunit);
                        f.getline (linebuf, 100); // number of spectro points
                        sscanf (linebuf, "%*[^0-9]%d", &nv);
                        f.getline (linebuf, 100); // start point spectroscopy
                        f.getline (linebuf, 100); // end point spectroscopy
                        f.getline (linebuf, 100); // increment point spectroscopy
                        f.getline (linebuf, 100); // acquisition time per spectroscopy point
                        f.getline (linebuf, 100); // delay time per spectroscopy point
                        f.getline (linebuf, 100); // feedback on/off
                        f.getline (linebuf, 100); // dat filename
                        sscanf (linebuf, " %100s", fn);
                        f.getline (linebuf, 100); // display name
      
                        // do we read the correct par set?
                        fs = g_strdup (fn+strlen(fn)-4);
                        if (!strcmp(fs,fsuffix)) {  // Yes, we do!
                                valid = TRUE;
                                spectro = TRUE;
	
                                scan->data.s.nvalues = nv;
	
                                PI_DEBUG (DBG_L2, " zunit: " << zunit );
                                UnitObj *zu = gapp->xsm->MakeUnit (zunit, "I");
                                scan->data.SetZUnit(zu);
                                scan->data.s.dz = zu->Usr2Base(ds);	  
                                delete zu;
                        }
                }

                // spectro points in x direction
                if ( !strncmp (linebuf, "Spectroscopy Points in X", 24) ) {
                        sscanf (linebuf, "%*[^0-9]%d", &xspecpoints);
                        PI_DEBUG (DBG_L2, " xspecpoints: " << xspecpoints );
                }
    
                // spectro points in y direction
                if ( !strncmp (linebuf, "Spectroscopy Lines in Y", 23) ) {
                        sscanf (linebuf, "%*[^0-9]%d", &yspecpoints);
                        PI_DEBUG (DBG_L2, " yspecpoints: " << yspecpoints );
                }

                // dual voltage mode
                if ( !strncmp (linebuf, "Dual mode", 9) ) {
                        char buf[4];
                        sscanf (linebuf, "%*[^:]: %3s", buf);
                        if ( !strncmp (buf, "On", 2) ) dualmode = TRUE;
                }

                // gap voltage
                if ( !strncmp (linebuf, "Gap Voltage", 11) ) {
                        double ugap;
                        if (!dualmode || fsuffix[2] == 'f')  // either single volt or forw. dir
                                sscanf (linebuf, "%*[^0-9-]%lf", &ugap);
                        else {
                                f.getline (linebuf, 100);
                                sscanf (linebuf, "%*[^0-9-]%lf", &ugap);
                        }
                        scan->data.s.Bias = ugap;
                        PI_DEBUG (DBG_L2, " ugap = " << scan->data.s.Bias );
                }

                // current feedback setpoint
                // missing unit support!
                if ( !strncmp (linebuf, "Feedback Set", 12) ) {
                        double iset;
                        /*	  gchar tmp[100];*/
                        sscanf (linebuf, "%*[^0-9-]%lf", &iset);
                        scan->data.s.Current = iset;
                        /*	doesn't work (yet) */
                        /*	  sprintf(tmp, "I_set: %4.2lf nA", iset);		
                                  printf("Current Setpoint is %4.2lf.\n", iset);
                                  printf("as a string: %14s\n",tmp);
                                  g_string_append(comment, tmp);
                                  scan->data.ui.SetComment(comment->str);
                                  g_free(tmp);*/
                        PI_DEBUG (DBG_L2, " iset = " << scan->data.s.Current );
                }

        }

        f.close();

  
        // reading a spectro grid?
        if (spectro && xspecpoints > 0 && yspecpoints > 0 && \
            scan->data.s.nvalues > 1) {
                spectrogrid = TRUE;
                scan->data.s.nx = xspecpoints;
                scan->data.s.ny = yspecpoints;
                scan->data.s.dx = scan->data.s.rx / scan->data.s.nx;
                scan->data.s.dy = scan->data.s.ry / scan->data.s.ny;
        }

        // test for various errors:
        if (!valid) return FIO_INVALID_FILE;
        // single point spectra not (yet) implemented
        if (spectro && !spectrogrid) return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

        return FIO_OK; 
}



FIO_STATUS Omicron_SPM_ImExportFile::Write()
{
        // to be done...

	return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
}

// Plugin's Notify Cb's, registered to be called on file load/save to check file
// return via filepointer, it is set to Zero or passed as Zero if file has been processed!
// That's all fine, you should just change the Text Stings below...


static void omicron_io_filecheck_load_callback (gpointer data){
        gchar **fn = (gchar**)data;
        if (*fn){
                PI_DEBUG (DBG_L2, "Check File: omicron_io_filecheck_load_callback called with >"
                          << *fn << "<" );
    
                Scan *dst = gapp->xsm->GetActiveScan();
                if(!dst){ 
                        gapp->xsm->ActivateFreeChannel();
                        dst = gapp->xsm->GetActiveScan();
                }
                Omicron_SPM_ImExportFile fileobj (dst, *fn);
    
                FIO_STATUS ret = fileobj.Read(); 
                if (ret != FIO_OK){ 
                        // I'am responsible! (But failed)
                        if (ret != FIO_NOT_RESPONSIBLE_FOR_THAT_FILE){
                                *fn=NULL;
                                PI_DEBUG (DBG_L2, "Read Error " << ((int)ret) << "!!!!!!!!" );
                        }else{
                                PI_DEBUG (DBG_L2, "No Omicron Scala File!" );
                        }
                        // no more data: remove allocated and unused scan now, force!
                        //	    gapp->xsm->SetMode(-1, ID_CH_M_OFF, TRUE); 
                }else{
                        // got it!
                        *fn=NULL;
      
                        // Now update gxsm main window data fields
                        gapp->xsm->ActiveScan->GetDataSet(gapp->xsm->data);
                        gapp->xsm->ActiveScan->auto_display ();
                        gapp->spm_update_all();
                        dst->draw();
                }
        }else{
                PI_DEBUG (DBG_L2, "omicron_io_filecheck_load: Skipping" );
        }
}



static void omicron_io_filecheck_save_callback ( gpointer data ){
        gchar **fn = (gchar**)data;
        if (*fn){
                Scan *src;
                PI_DEBUG (DBG_L2, "Check File: omicron_io_filecheck_save_callback called with >"
                          << *fn << "<" );
    
                Omicron_SPM_ImExportFile fileobj (src = gapp->xsm->GetActiveScan(), *fn);
    
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
                PI_DEBUG (DBG_L2, "omicron_io_filecheck_save: Skipping >" << *fn << "<" );
        }
}


// Menu Call Back Fkte

static void omicron_io_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
        gchar *fn = gapp->file_dialog_load ("Omicron SPM Import", NULL, "*.*", NULL);
  
        if (fn){
                PI_DEBUG (DBG_L2, "FLDLG-IM::" << fn );
                omicron_io_filecheck_load_callback (&fn );
                g_free (fn);
        }
}


// static void omicron_io_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
// {
//   gchar *fn = gapp->file_dialog("Omicron SPM Export", NULL,
// 				"*.*",
// 				"","SPM-Export");
  
//   PI_DEBUG (DBG_L2, "FLDLG-EX::" << fn );
//   omicron_io_filecheck_save_callback (&fn );
// }

