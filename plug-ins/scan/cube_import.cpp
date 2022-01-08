/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: cube_import.C
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
% PlugInDocuCaption: Cube file Import
% PlugInName: cube_import
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: File/Import/Cube

% PlugInDescription
\label{plugins:cube_import}
The \GxsmEmph{Cube} plug-in supports reading of a cube volumetric data file format.
\url{http://paulbourke.net/dataformats/cube/}

CPMD CUBE FILE.\\
 OUTER LOOP: X, MIDDLE LOOP: Y, INNER LOOP: Z\\
    3    0.000000    0.000000    0.000000\\
   40    0.283459    0.000000    0.000000\\
   40    0.000000    0.283459    0.000000\\
   40    0.000000    0.000000    0.283459\\
    8    0.000000    5.570575    5.669178    5.593517\\
    1    0.000000    5.562867    5.669178    7.428055\\
    1    0.000000    7.340606    5.669178    5.111259\\
 -0.25568E-04  0.59213E-05  0.81068E-05  0.10868E-04  0.11313E-04  0.35999E-05\\
      :             :             :           :            :            :\\
      :             :             :           :            :            :\\
      :             :             :           :            :            :\\
        In this case there will be 40 x 40 x 40 floating point values\\
      :             :             :           :            :            :\\
      :             :             :           :            :            :\\
      :             :             :           :            :            :\\

% PlugInUsage
The plug-in is called by \GxsmMenu{File/Import/Cube}. 
Only import direction is implemented.

%% OptPlugInKnownBugs
%Not yet tested.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <iostream>
#include <iomanip>
#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "dataio.h"
#include "action_id.h"
#include "util.h"
#include "batch.h"
#include "fileio.c"
#include "glbvars.h"
#include "surface.h"

using namespace std;

// Plugin Prototypes
static void cube_import_init (void);
static void cube_import_query (void);
static void cube_import_about (void);
static void cube_import_configure (void);
static void cube_import_cleanup (void);

static void cube_import_filecheck_load_callback (gpointer data );
static void cube_import_filecheck_save_callback (gpointer data );

static void cube_import_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void cube_import_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// Fill in the GxsmPlugin Description here
GxsmPlugin cube_import_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  "Cube_Import",
  NULL,                   // PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("Im/Export of the Cube data file format."),
  "Percy Zahl",
  "file-import-section,file-export-section",
  N_("Cube,Cube"),
  N_("Cube import,Cube export"),
  N_("Cube data file import and export filter."),
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  cube_import_init,
  cube_import_query,
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  cube_import_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  cube_import_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL,
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  cube_import_import_callback, // direct menu entry callback1 or NULL
  cube_import_export_callback, // direct menu entry callback2 or NULL

  cube_import_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("GXSM cube volume data file Import/Export Plugin\n\n");

static const char *file_mask = "*";

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  cube_import_pi.description = g_strdup_printf(N_("Gxsm im_export plugin %s"), VERSION);
  return &cube_import_pi; 
}

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/GME Dat"
// Export Menupath is "File/Export/GME Dat"
// ----------------------------------------------------------------------
// !!!! make sure the "cube_import_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!

static void cube_import_query(void)
{
	gchar **entry = g_strsplit (cube_import_pi.menuentry, ",", 2);

	if(cube_import_pi.status) g_free(cube_import_pi.status); 
	cube_import_pi.status = g_strconcat (
		N_("Plugin query has attached "),
		cube_import_pi.name, 
		N_(": File IO Filters are ready to use."),
		NULL);
	
	// clean up
	g_strfreev (entry);

// register this plugins filecheck functions with Gxsm now!
// This allows Gxsm to check files from DnD, open, 
// and cmdline sources against all known formats automatically - no explicit im/export is necessary.
	cube_import_pi.app->ConnectPluginToLoadFileEvent (cube_import_filecheck_load_callback);
	cube_import_pi.app->ConnectPluginToSaveFileEvent (cube_import_filecheck_save_callback);
}

struct Tatoms { double nxyz[4]; };

// make a new derivate of the base class "Dataio"
class cube_ImExportFile : public Dataio{
public:
	cube_ImExportFile() : Dataio(){
                transpose_xyz[0]=0;
                transpose_xyz[1]=1;
                transpose_xyz[2]=2;
                transpose_xyz[3]=0; // mode ident
                // ***** Memory
                line1 = NULL;
                line2 = NULL;
                atoms=0; // keep last read atoms for rewite
                t_atoms_xyz=NULL;
                origin[0]=origin[1]=origin[2]=origin[3]=0.0;
        };
	cube_ImExportFile(Scan *s, const char *n) : Dataio(s,n){
                transpose_xyz[0]=0;
                transpose_xyz[1]=1;
                transpose_xyz[2]=2;
                transpose_xyz[3]=0; // mode ident
                // ***** Memory
                line1 = NULL;
                line2 = NULL;
                atoms=0; // keep last read atoms for rewite
                t_atoms_xyz=NULL;
                origin[0]=origin[1]=origin[2]=origin[3]=0.0;
        };
        virtual ~cube_ImExportFile(){
                if (t_atoms_xyz)
                        g_free (t_atoms_xyz);
                atoms=0;
        };

	virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
	virtual FIO_STATUS Write();
private:
	FIO_STATUS import(const char *fname);
        int transpose_xyz[4];
        gchar *line1;
        gchar *line2;
        gint dims[3];
        double origin[4];
        double voxels[3][3];
        gint atoms;
        double ext_min_atom_xyz[4];
        double ext_max_atom_xyz[4];
        double extends_atom_xyz[4];
        Tatoms *t_atoms_xyz;
};

FIO_STATUS cube_ImExportFile::Read(xsm::open_mode mode){
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
		PI_DEBUG (DBG_L2, "Error at file open. File not good/readable.");
		return status=FIO_OPEN_ERR;
	}
	f.close();

	// Check all known File Types:
	if ((ret=import (fname)) !=  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
		return ret;

	return  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
}

FIO_STATUS cube_ImExportFile::import(const char *fname){
        const int maxcharsperline = 0x4000;
	gchar line[maxcharsperline];

	// Am I resposible for that file -- can only do a dimension sanity check
	ifstream f;
	GString *FileList=NULL;

	if (strncasecmp(fname+strlen(fname)-5,".cube",5) && strncasecmp(fname+strlen(fname)-5,".CUBE",5)){
		f.close ();
		return status=FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
	}

	f.open(name, ios::in);
	if (!f.good())
	        return status=FIO_OPEN_ERR;
	
        g_message ("Importing from cube data file.\n");
	FileList = g_string_new ("Imported by GXSM from cube data file.\n");

        // skip first 2 lines
        f.getline (line, maxcharsperline);
        if(line1) g_free(line1); line1 = g_strdup(line);
	g_string_append (FileList, line);
	g_string_append (FileList, "\n");

        f.getline (line, maxcharsperline);
        if(line2) g_free(line2); line2 = g_strdup(line);
	g_string_append (FileList, line);
	g_string_append (FileList, "\n");

#define SKIP_EMPTY(T) { while (*T) if (strlen (*T) < 1) ++T; else break; if (!*T) { f.close(); return status=FIO_NOT_RESPONSIBLE_FOR_THAT_FILE; }}
#define SKIP_EMPTY_N(T) { while (*T) if (strlen (*T) < 1) ++T; else break; }
        
        // # atoms, origin
        {
                f.getline (line, maxcharsperline);
                g_message ("%s", line);
                g_string_append (FileList, line);
                g_string_append (FileList, "\n");
                gchar **record = g_strsplit_set (line, " \t,", 200);
                gchar **token  = record;
                SKIP_EMPTY (token);
                atoms = atoi (*token++);
                origin[3] = 1;
                for (int i=0; *token && i<4; ++token, ++i){
                        SKIP_EMPTY (token);
                        origin[i] = atof(*token);
                }
                g_strfreev (record);
                if (!f.good())
                        return status=FIO_OPEN_ERR;
        }
        g_message ("Reading cube file, %d atoms", atoms);
        for (int k=0; k<3; ++k){
                f.getline (line, maxcharsperline);
                g_message ("%s",line);
                g_string_append (FileList, line);
                g_string_append (FileList, "\n");
                gchar **record = g_strsplit_set (line, " \t,", 200);
                gchar **token  = record;
                SKIP_EMPTY (token);
                dims[k] = atoi (*token++);
                for (int i=0; *token && i<3; ++token, ++i){
                        SKIP_EMPTY (token);
                        voxels[k][i] = atof(*token);
                }
                g_message ("Voxel[%d] %g", k, voxels[k][k]);
                g_strfreev (record);
                if (!f.good())
                        return status=FIO_OPEN_ERR;
        }
        g_message ("Reading cube file, %d atoms, reading Nxyz", atoms);
        // read and skip atoms
        if (t_atoms_xyz)
                g_free (t_atoms_xyz);
        t_atoms_xyz = g_new (Tatoms, atoms) ;
        
        for (int k=0; k<atoms; ++k){
                f.getline (line, maxcharsperline);
                g_message ("%s",line);
                g_string_append (FileList, line);
                g_string_append (FileList, "\n");
                gchar **record = g_strsplit_set (line, " \t,", 200);
                gchar **token  = record;
                SKIP_EMPTY (token);
                int atom_number = atoi (*token++);
                double t_atom_xyz[4];
                for (int i=0; *token && i<4; ++token, ++i){
                        SKIP_EMPTY (token);
                        t_atom_xyz[i] = atof(*token);
                        t_atoms_xyz[k].nxyz[i] = t_atom_xyz[i]; 
                        if (k>0){
                                if (ext_min_atom_xyz[i] > t_atom_xyz[i]) ext_min_atom_xyz[i] = t_atom_xyz[i];
                                if (ext_max_atom_xyz[i] < t_atom_xyz[i]) ext_max_atom_xyz[i] = t_atom_xyz[i];
                        } else {
                                ext_min_atom_xyz[i] = t_atom_xyz[i];
                                ext_max_atom_xyz[i] = t_atom_xyz[i];
                        }
                }
                //g_message ("Atom[%d] %g", k, t_atom_xyz[0]);
                g_strfreev (record);
                if (!f.good())
                        return status=FIO_OPEN_ERR;
        }
        // calculate extends, test for auto transpose so molecule in in XY plane
        for (int i=0; i<4; ++i)
                extends_atom_xyz[i] = ext_max_atom_xyz[i] - ext_min_atom_xyz[i];

        gchar *tmp;
        tmp = g_strdup_printf("Atom MIN: %g XYZ: %g %g %g\n", ext_min_atom_xyz[0],ext_min_atom_xyz[1],ext_min_atom_xyz[2],ext_min_atom_xyz[3]);
        g_string_append (FileList, tmp); g_free(tmp);
        tmp = g_strdup_printf("Atom MAX: %g XYZ: %g %g %g\n", ext_max_atom_xyz[0],ext_max_atom_xyz[1],ext_max_atom_xyz[2],ext_max_atom_xyz[3]);
        g_string_append (FileList, tmp); g_free(tmp);
        tmp = g_strdup_printf("Atom EXT: %g XYZ: %g %g %g\n", extends_atom_xyz[0],extends_atom_xyz[1],extends_atom_xyz[2],extends_atom_xyz[3]);
        g_string_append (FileList, tmp); g_free(tmp);

        int min_idx=3;
        if (extends_atom_xyz[3] < extends_atom_xyz[2] && extends_atom_xyz[3] < extends_atom_xyz[1]){
                transpose_xyz[0]=0;
                transpose_xyz[1]=1;
                transpose_xyz[2]=2;
                transpose_xyz[3]=0; // mode ident
                g_string_append (FileList, "OK, molecule in XY plane. :) \n");
        }
        else if (extends_atom_xyz[2] < extends_atom_xyz[1] && extends_atom_xyz[2] < extends_atom_xyz[3]){
                g_string_append (FileList, "Molecule in XZ plane. Transposing YZ.\n");
                transpose_xyz[0]=0;
                transpose_xyz[1]=2;
                transpose_xyz[2]=1;
                transpose_xyz[3]=1; // mode T YZ
                for (int k=0; k<atoms; ++k){
                        double t_atom_xyz[4];
                        memcpy (t_atom_xyz, t_atoms_xyz[k].nxyz, sizeof t_atom_xyz);
                        for (int i=0; i<3; ++i)
                                t_atoms_xyz[k].nxyz[i+1] = t_atom_xyz[transpose_xyz[i]+1];
                }
        }
        else if (extends_atom_xyz[1] < extends_atom_xyz[2] && extends_atom_xyz[1] < extends_atom_xyz[3]){
                g_string_append (FileList, "Molecule in YZ plane. Transposing XZ.\n");
                transpose_xyz[0]=2;
                transpose_xyz[1]=1;
                transpose_xyz[2]=0;
                transpose_xyz[3]=2; // mode T XZ
                for (int k=0; k<atoms; ++k){
                        double t_atom_xyz[4];
                        memcpy (t_atom_xyz, t_atoms_xyz[k].nxyz, sizeof t_atom_xyz);
                        for (int i=0; i<3; ++i)
                                t_atoms_xyz[k].nxyz[i+1] = t_atom_xyz[transpose_xyz[i]+1];
                }
        } else {
                g_string_append (FileList, "Molecule in strane plane! Keeping.\n");
        }

        g_message ("Reading cube file scan setup %d x %d x %d", dims[0], dims[1], dims[2]);

	time_t t; // Scan - Startzeit eintragen 
	time(&t);
	tmp = g_strconcat ((ctime(&t)), " (Imported)", NULL); scan->data.ui.SetDateOfScan (tmp); g_free (tmp);
	scan->data.ui.SetName (fname);
	scan->data.ui.SetOriginalName (fname);
	scan->data.ui.SetType ("Cube Volume Data"); 


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
	scan->data.s.nx = dims[transpose_xyz[0]];                        
	scan->data.s.ny = dims[transpose_xyz[1]];
	scan->data.s.nvalues = dims[transpose_xyz[2]];
	scan->data.s.dx = voxels[transpose_xyz[0]][transpose_xyz[0]]; // assume cubic voxel
	scan->data.s.dy = voxels[transpose_xyz[1]][transpose_xyz[1]];
        scan->data.s.dz = voxels[transpose_xyz[2]][transpose_xyz[2]];
	scan->data.s.rx = scan->data.s.dx * scan->data.s.nx *0.52917721; // from bor to ang   1bor = 5.2917721067(12)×10−11 m
	scan->data.s.ry = scan->data.s.dy * scan->data.s.ny *0.52917721;
	scan->data.s.rz = scan->data.s.dz * scan->data.s.nvalues *0.52917721;
	scan->data.s.x0 = 0.;
	scan->data.s.y0 = 0.;
	scan->data.s.alpha = 0.;

	// be nice and reset this to some defined state
	scan->data.display.z_high       = 0.;
	scan->data.display.z_low        = 0.;

	// set the default view parameters
	scan->data.display.bright = 32.;
	scan->data.display.contrast = 1.0;

        g_message ("Setting up Dimensions rx:%g x ry:%g x rv:%g in Ang", scan->data.s.rx, scan->data.s.ry, scan->data.s.rz);


        // FYI: (PZ)
	//  scan->data.display.vrange_z  = ; // View Range Z in base ZUnits
	//  scan->data.display.voffset_z = 0; // View Offset Z in base ZUnits
	//  scan->AutoDisplay([...]); // may be used too...
  
	UnitObj *u = main_get_gapp()->xsm->MakeUnit ("AA", "X");
	scan->data.SetXUnit(u);
	delete u;

	u = main_get_gapp()->xsm->MakeUnit ("AA", "Y");
	scan->data.SetYUnit(u);
	delete u;

	u = main_get_gapp()->xsm->MakeUnit ("AA", "Z");
	scan->data.SetZUnit(u);
	delete u;

	scan->data.ui.SetComment (FileList->str);
	g_string_free(FileList, TRUE); 
	FileList=NULL;

        // Read Img Data.
        g_message ("Creating Scan nx:%d x ny:%d x nv:%d", scan->data.s.nx, scan->data.s.ny, scan->data.s.nvalues);
        scan->mem2d->Resize (scan->data.s.nx, scan->data.s.ny, scan->data.s.nvalues, ZD_FLOAT);

        g_message ("Reading cube file voxel data %d x %d x %d", dims[0], dims[1], dims[2]);
        // read volume data
        {
                gchar **record = NULL;
                gchar **token  = record;
                for (int ix=0; ix<dims[0]; ix++) {
                        //g_message ("X=%d *************\n",ix);
                        for (int iy=0; iy<dims[1]; iy++) {
                                //g_message (" Y=%d *************\n",iy);
                                for (int iz=0; iz<dims[2]; iz++) {
                                        while (!token){
                                                if (!f.good())
                                                        return status=FIO_OPEN_ERR;
                                                f.getline (line, maxcharsperline);
                                                //g_message ("  new VDataLine:\n%s", line);
                                                record = g_strsplit_set (line, " \t,", 100);
                                                token  = record;
                                                if (!*token){
                                                        g_strfreev (record);
                                                        record = token = NULL;
                                                }
                                        }
                                        SKIP_EMPTY_N(token);
                                        //g_message ("    V[%d][%d][%d]=>%s<",ix,iy,iz,*token);
                                        double value = atof (*token++);
                                        switch (transpose_xyz[3]){
                                        case 0: scan->mem2d->PutDataPkt (value, ix, iy, iz); break; // normal XYZ mapping
                                        case 1: scan->mem2d->PutDataPkt (value, ix, iz, iy); break; // YZ transpose
                                        case 2: scan->mem2d->PutDataPkt (value, iz, iy, ix); break; // XZ transpose
                                        }
                                        //g_message ("    next: %s",*token);
                                        SKIP_EMPTY_N (token);
                                        //g_message ("    next: %s",*token);
                                        if (!*token){
                                                //g_message ("token=%s", *token);
                                                g_strfreev (record);
                                                record = token = NULL;
                                        }
                                                        
                                }
                        }
                }
                if (record)
                        g_strfreev (record);
        }
                
        f.close ();
        
	scan->data.orgmode = SCAN_ORG_CENTER;
	scan->mem2d->data->MkXLookup (origin[0]-scan->data.s.rx/2., origin[0]+scan->data.s.rx/2.);
	scan->mem2d->data->MkYLookup (origin[1]+scan->data.s.ry/2., origin[1]-scan->data.s.ry/2.);
	scan->mem2d->data->MkVLookup (origin[2]+scan->data.s.rz/2., origin[2]-scan->data.s.rz/2.);

        main_get_gapp()->spm_update_all();

	return FIO_OK; 
}


FIO_STATUS cube_ImExportFile::Write(){
        double zero=0.0;
	const gchar *fname;
	ofstream f;

	if(strlen(name)>0)
		fname = (const char*)name;
	else
		fname = main_get_gapp()->file_dialog("File Export: Cube"," ",file_mask,"","Cube write");
	if (fname == NULL) return FIO_NO_NAME;

	// check if we like to handle this
	if (strncmp(fname+strlen(fname)-5,".cube",5))
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;

	f.open(name, ios::out);
	if (!f.good())
	        return status=FIO_OPEN_ERR;
        // << std::fixed << std::setw( 11 ) << std::setprecision( 6 ) << std::setfill( '0' ) << value
        f << " GXSM Cube FILE of:" << (line1?line1:" - ") << std::endl;
        f << (line2?line2:" -- no previous atoms info --") << std::endl;
        f << std::fixed;
        f << std::right << std::setw(5) << atoms                  << " " << std::setw(11) << std::setprecision(6) << origin[0]       << " " << std::setw(11) << origin[1]       << " " << std::setw(11) << origin[2] << " " << std::setw(11) << (int)(origin[3]) << std::endl;
        f << std::right << std::setw(5) << scan->mem2d->GetNx ()  << " " << std::setw(11) << std::setprecision(6) << scan->data.s.dx << " " << std::setw(11) << zero            << " " << std::setw(11) << zero            << std::endl;
        f << std::right << std::setw(5) << scan->mem2d->GetNy ()  << " " << std::setw(11) << std::setprecision(6) << zero            << " " << std::setw(11) << scan->data.s.dy << " " << std::setw(11) << zero            << std::endl;
        f << std::right << std::setw(5) << scan->mem2d->GetNv ()  << " " << std::setw(11) << std::setprecision(6) << zero            << " " << std::setw(11) << zero            << " " << std::setw(11) << scan->data.s.dz << std::endl;
        for (int k=0; k<atoms; ++k)
                f << std::right
                  << std::setw(5) << ((int)t_atoms_xyz[k].nxyz[0])
                  << " " << std::setw(11) << t_atoms_xyz[k].nxyz[0]
                  << " " << std::setw(11) << t_atoms_xyz[k].nxyz[1]
                  << " " << std::setw(11) << t_atoms_xyz[k].nxyz[2]
                  << " " << std::setw(11) << t_atoms_xyz[k].nxyz[3]
                  << std::endl;
        f << std::setprecision(6) << std::scientific;
	for (int col=0; col < scan->mem2d->GetNx (); ++col) {
		for (int row=0; row < scan->mem2d->GetNy (); ++row) {
                        for (int val=0; val < scan->mem2d->GetNv (); ++val) {
#if 1
                                f << scan->mem2d->GetDataPkt (col, row, val) << " ";
#else // zero region with erros (hack)
                                if (val > 70 && val < 330)
                                        f << scan->mem2d->GetDataPkt (col, row, val) << " ";
                                else
                                        f << (0.0) << " ";
#endif
                                if (val % 6 == 5)
                                        f << std::endl;
                        }
                        f << std::endl;
                }
                f << std::endl;
	}

	f.close ();

	return FIO_OK; 
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//

cube_ImExportFile *cube_fileobj = NULL;

// init-Function
static void cube_import_init(void)
{
	PI_DEBUG (DBG_L2, cube_import_pi.name << "Plugin Init");
        cube_fileobj = new cube_ImExportFile ();
}

// about-Function
static void cube_import_about(void)
{
	const gchar *authors[] = { cube_import_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  cube_import_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void cube_import_configure(void)
{
	if(cube_import_pi.app)
		cube_import_pi.app->message("cube_import Plugin Configuration");
}

// cleanup-Function, make sure the Menustrings are matching those above!!!
static void cube_import_cleanup(void)
{
        if (cube_fileobj)
                delete cube_fileobj;

	PI_DEBUG (DBG_L2, "Plugin Cleanup done.");
}

// Plugin's Notify Cb's, registered to be called on file load/save to check file
// return via filepointer, it is set to Zero or passed as Zero if file has been processed!
// That's all fine, you should just change the Text Stings below...


static void cube_import_filecheck_load_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		PI_DEBUG (DBG_L2, 
			  "Check File: cube_import_filecheck_load_callback called with >"
			  << *fn << "<" );

		Scan *dst = main_get_gapp()->xsm->GetActiveScan();
		if(!dst){ 
			main_get_gapp()->xsm->ActivateFreeChannel();
			dst = main_get_gapp()->xsm->GetActiveScan();
		}
		cube_fileobj->SetScan (dst);
                cube_fileobj->SetName (*fn);

		FIO_STATUS ret = cube_fileobj->Read(); 
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
		PI_DEBUG (DBG_L2, "cube_import_filecheck_load: Skipping" );
	}
}

static void cube_import_filecheck_save_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		Scan *src;
		PI_DEBUG (DBG_L2,
			  "Check File: cube_import_filecheck_save_callback called with >"
			  << *fn << "<" );

		cube_fileobj->SetScan (src = main_get_gapp()->xsm->GetActiveScan());
                cube_fileobj->SetName (*fn);

		FIO_STATUS ret;
		ret = cube_fileobj->Write(); 

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
		PI_DEBUG (DBG_L2, "cube_import_filecheck_save: Skipping >" << *fn << "<" );
	}
}

// Menu Call Back Fkte

static void cube_import_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (cube_import_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (cube_import_pi.name, "-import", NULL);
	gchar *fn = main_get_gapp()->file_dialog_load (help[0], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
	        cube_import_filecheck_load_callback (&fn );
                g_free (fn);
	}
}

static void cube_import_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (cube_import_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (cube_import_pi.name, "-export", NULL);
	gchar *fn = main_get_gapp()->file_dialog_save (help[1], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
	  	cube_import_filecheck_save_callback (&fn );
                g_free (fn);
	}
}
