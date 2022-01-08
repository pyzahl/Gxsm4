/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: ascii_data_im_export.C
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
% PlugInDocuCaption: ASCII data file Import/Export
% PlugInName: ascii_data_im_export
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: File/Import/ASCII

% PlugInDescription
\label{plugins:ascii_data_im_export}
The \GxsmEmph{ascii\_data\_im\_export} plug-in supports reading of ASCII data files.

It auto detects the text header "Reuben-NSNOM" as described below -- if this fails, it attempts to auto read a plain matrix of values, line by line.
Space or "CSV" separated numbers, dimensions are auto determined by max elements per line and number of lines found. File extension must be ".asc" or ".csv".

Special but simple text header "Reuben-NSNOM" is suggested. Here is a sample file:

{\tiny
\begin{verbatim}
scan length = 25.000000
points/line = 25
point spacing = 1.041667
# lines =  25
line spacing = 1.041667
points in average = 1
velocity = 2.000000
lock-in time constant (msec) = 30.000000
X origin = -1883.596000
Y origin = 1321.116000

end

-1.5850 -1.5889 -1.5967 -1.5737 -1.5610 -1.5581 -1.5518 -1.5537 -1.9194 -2.2808 -2.2148 -2.1709 -2.1382 -2.1367 -2.1123 -2.0864 -2.0684 -2.0498 -2.0376 -2.0171 -1.9990 -1.9912 -1.9824 -1.9741 -1.9575
-1.5796 -1.5811 -1.5684 -1.5718 -1.5625 -1.5552 -1.5425 -1.5488 -1.9272 -2.2764 -2.2104 -2.1660 -2.1387 -2.1294 -2.1094 -2.0801 -2.0645 -2.0464 -2.0337 -2.0229 -1.9961 -1.9883 -1.9824 -1.9717 -1.9604
-1.5776 -1.5747 -1.5854 -1.5718 -1.5610 -1.5488 -1.5400 -1.5474 -1.9253 -2.2651 -2.2124 -2.1597 -2.1338 -2.1309 -2.0996 -2.0728 -2.0601 -2.0420 -2.0293 -2.0205 -1.9839 -1.9839 -1.9790 -1.9683 -1.9561
-1.5688 -1.5840 -1.5825 -1.5640 -1.5518 -1.5488 -1.5361 -1.5430 -1.9395 -2.2607 -2.2070 -2.1548 -2.1309 -2.1250 -2.0942 -2.0708 -2.0557 -2.0391 -2.0264 -2.0156 -1.9912 -1.9810 -1.9712 -1.9653 -1.9502
-1.5674 -1.5776 -1.5776 -1.5610 -1.5474 -1.5459 -1.5288 -1.5347 -1.9136 -2.2549 -2.1963 -2.1519 -2.1250 -2.1182 -2.0913 -2.0664 -2.0537 -2.0327 -2.0254 -2.0098 -1.9897 -1.9780 -1.9687 -1.9590 -1.9487
-1.5610 -1.5732 -1.5732 -1.5566 -1.5439 -1.5391 -1.5259 -1.5337 -1.9502 -2.2471 -2.1919 -2.1445 -2.1196 -2.1123 -2.0850 -2.0620 -2.0420 -2.0278 -2.0146 -2.0054 -1.9868 -1.9712 -1.9609 -1.9531 -1.9424
-1.5610 -1.5640 -1.5684 -1.5547 -1.5381 -1.5332 -1.5229 -1.5308 -1.9697 -2.2422 -2.1782 -2.1387 -2.1157 -2.1094 -2.0820 -2.0576 -2.0405 -2.0220 -2.0146 -2.0034 -1.9805 -1.9687 -1.9580 -1.9502 -1.9390
-1.5562 -1.5625 -1.5640 -1.5483 -1.5308 -1.5283 -1.5171 -1.5107 -1.9854 -2.2378 -2.1797 -2.1367 -2.1123 -2.1030 -2.0742 -2.0542 -2.0337 -2.0171 -2.0098 -1.9961 -1.9741 -1.9624 -1.9517 -1.9453 -1.9316
-1.5503 -1.5562 -1.5610 -1.5410 -1.5283 -1.5244 -1.5073 -1.5181 -1.9443 -2.2300 -2.1768 -2.1309 -2.1094 -2.1030 -2.0664 -2.0464 -2.0312 -2.0146 -2.0020 -1.9897 -1.9717 -1.9561 -1.9497 -1.9409 -1.9316
-1.5459 -1.5532 -1.5581 -1.5400 -1.5239 -1.5171 -1.5073 -1.5044 -1.9302 -2.2227 -2.1738 -2.1260 -2.1021 -2.0967 -2.0693 -2.0435 -2.0264 -2.0083 -1.9990 -1.9849 -1.9653 -1.9531 -1.9453 -1.9395 -1.9253
-1.5410 -1.5483 -1.5483 -1.5317 -1.5181 -1.5181 -1.5015 -1.4585 -1.9780 -2.2178 -2.1670 -2.1216 -2.0972 -2.0894 -2.0645 -2.0391 -2.0249 -2.0054 -1.9941 -1.9819 -1.9624 -1.9512 -1.9404 -1.9302 -1.9224
-1.5337 -1.5410 -1.5425 -1.5244 -1.5137 -1.5132 -1.4951 -1.4966 -1.9224 -2.2114 -2.1597 -2.1172 -2.0957 -2.0864 -2.0601 -2.0327 -2.0171 -1.9990 -1.9912 -1.9741 -1.9561 -1.9453 -1.9375 -1.9287 -1.9165
-1.5322 -1.5332 -1.5425 -1.5210 -1.5107 -1.5093 -1.4907 -1.4917 -1.9575 -2.2061 -2.1567 -2.1104 -2.0894 -2.0845 -2.0527 -2.0278 -2.0142 -1.9971 -1.9868 -1.9712 -1.9526 -1.9409 -1.9316 -1.9238 -1.9116
-1.5308 -1.5181 -1.5361 -1.5088 -1.5059 -1.5015 -1.4873 -1.4873 -1.9653 -2.2036 -2.1523 -2.1060 -2.0864 -2.0786 -2.0464 -2.0229 -2.0142 -1.9941 -1.9790 -1.9634 -1.9482 -1.9360 -1.9297 -1.9194 -1.9058
-1.5259 -1.5229 -1.5317 -1.5181 -1.4985 -1.4922 -1.4780 -1.4834 -1.9834 -2.1978 -2.1509 -2.1021 -2.0820 -2.0708 -2.0420 -2.0176 -2.0054 -1.9868 -1.9746 -1.9624 -1.9453 -1.9297 -1.9253 -1.9131 -1.9009
-1.5229 -1.5225 -1.5273 -1.5117 -1.4873 -1.4937 -1.4780 -1.4785 -2.0308 -2.1885 -2.1489 -2.0952 -2.0757 -2.0679 -2.0405 -2.0127 -2.0010 -1.9819 -1.9712 -1.9561 -1.9395 -1.9282 -1.9180 -1.9116 -1.9009
-1.5244 -1.5054 -1.5181 -1.4937 -1.4771 -1.4873 -1.4722 -1.4766 -2.0483 -2.1855 -2.1401 -2.0923 -2.0708 -2.0591 -2.0327 -2.0098 -1.9976 -1.9761 -1.9697 -1.9546 -1.9365 -1.9243 -1.9175 -1.9087 -1.8950
-1.5229 -1.4893 -1.5103 -1.4180 -1.4687 -1.4771 -1.4687 -1.4722 -2.0327 -2.1812 -2.1279 -2.0864 -2.0635 -2.0542 -2.0234 -2.0000 -1.9941 -1.9717 -1.9624 -1.9487 -1.9331 -1.9175 -1.9102 -1.9023 -1.8887
-1.4497 -1.4687 -1.4858 -1.4414 -1.4614 -1.4736 -1.4614 -1.4644 -2.0039 -2.1704 -2.1187 -2.0757 -2.0605 -2.0493 -2.0229 -2.0020 -1.9897 -1.9683 -1.9590 -1.9443 -1.9282 -1.9136 -1.9087 -1.8979 -1.8843
-1.5229 -1.4478 -1.4917 -1.4893 -1.4766 -1.4736 -1.4497 -1.4429 -1.9790 -2.1523 -2.1050 -2.0786 -2.0566 -2.0513 -2.0205 -1.9990 -1.9854 -1.9653 -1.9590 -1.9409 -1.9243 -1.9131 -1.9014 -1.8921 -1.8809
-1.5088 -1.3389 -1.4658 -1.4321 -1.4687 -1.4507 -1.4385 -1.4121 -2.0327 -2.1611 -2.1182 -2.0728 -2.0513 -2.0420 -2.0127 -1.9912 -1.9790 -1.9634 -1.9482 -1.9346 -1.9194 -1.9058 -1.8979 -1.8877 -1.8750
-1.4707 -1.4678 -1.4507 -1.4443 -1.4707 -1.4067 -1.4521 -1.4463 -2.0117 -2.1494 -2.1123 -2.0684 -2.0508 -2.0376 -2.0083 -1.9897 -1.9741 -1.9590 -1.9453 -1.9302 -1.9160 -1.8994 -1.8921 -1.8857 -1.8706
-1.4565 -1.5117 -1.4946 -1.4824 -1.4497 -1.4541 -1.4463 -1.4189 -2.0576 -2.1431 -2.0894 -2.0664 -2.0435 -2.0264 -2.0034 -1.9824 -1.9663 -1.9531 -1.9409 -1.9243 -1.9097 -1.8979 -1.8887 -1.8794 -1.8647
-1.4893 -1.5044 -1.4858 -1.4658 -1.4722 -1.4551 -1.4307 -1.2671 -2.0464 -2.1357 -2.0967 -2.0576 -2.0356 -2.0205 -1.9990 -1.9780 -1.9683 -1.9487 -1.9365 -1.9194 -1.9067 -1.8931 -1.8843 -1.8735 -1.8628
-1.4902 -1.5044 -1.4824 -1.4707 -1.4663 -1.4404 -1.4141 -1.3501 -2.1660 -2.1338 -2.0952 -2.0513 -2.0356 -2.0269 -1.9961 -1.9717 -1.9609 -1.9453 -1.9297 -1.9160 -1.9009 -1.8877 -1.8765 -1.8701 -1.8613
\end{verbatim}
}

% PlugInUsage
The plug-in is called by \GxsmMenu{File/Import/ASCII}. 
Only import direction is implemented.

%% OptPlugInKnownBugs
%Not yet tested.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

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
static void ascii_data_im_export_init (void);
static void ascii_data_im_export_query (void);
static void ascii_data_im_export_about (void);
static void ascii_data_im_export_configure (void);
static void ascii_data_im_export_cleanup (void);

static void ascii_data_im_export_filecheck_load_callback (gpointer data );
static void ascii_data_im_export_filecheck_save_callback (gpointer data );

static void ascii_data_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void ascii_data_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// Fill in the GxsmPlugin Description here
GxsmPlugin ascii_data_im_export_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  "Ascii_Data_Im_Export",
  NULL,                   // PlugIn's Categorie, set to NULL for all, I just don't want this always to be loaded!
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("Im/Export of the ASCII data file format."),
  "Percy Zahl",
  "file-import-section,file-export-section",
  N_("ASCII,ASCII"),
  N_("ASCII import,ASCII export"),
  N_("ASCII data file import and export filter, using header for Reuben NSNOM."),
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  ascii_data_im_export_init,
  ascii_data_im_export_query,
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  ascii_data_im_export_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  ascii_data_im_export_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL,
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  ascii_data_im_export_import_callback, // direct menu entry callback1 or NULL
  ascii_data_im_export_export_callback, // direct menu entry callback2 or NULL

  ascii_data_im_export_cleanup
};

// Text used in Aboutbox, please update!!
  static const char *about_text = N_("GXSM ASCII data file Import/Export Plugin\n\n");

static const char *file_mask = "*.asc";

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  ascii_data_im_export_pi.description = g_strdup_printf(N_("Gxsm im_export plugin %s"), VERSION);
  return &ascii_data_im_export_pi; 
}

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/GME Dat"
// Export Menupath is "File/Export/GME Dat"
// ----------------------------------------------------------------------
// !!!! make sure the "ascii_data_im_export_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!

static void ascii_data_im_export_query(void)
{
	gchar **entry = g_strsplit (ascii_data_im_export_pi.menuentry, ",", 2);

	if(ascii_data_im_export_pi.status) g_free(ascii_data_im_export_pi.status); 
	ascii_data_im_export_pi.status = g_strconcat (
		N_("Plugin query has attached "),
		ascii_data_im_export_pi.name, 
		N_(": File IO Filters are ready to use."),
		NULL);
	
	// clean up
	g_strfreev (entry);

// register this plugins filecheck functions with Gxsm now!
// This allows Gxsm to check files from DnD, open, 
// and cmdline sources against all known formats automatically - no explicit im/export is necessary.
	ascii_data_im_export_pi.app->ConnectPluginToLoadFileEvent (ascii_data_im_export_filecheck_load_callback);
	ascii_data_im_export_pi.app->ConnectPluginToSaveFileEvent (ascii_data_im_export_filecheck_save_callback);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void ascii_data_im_export_init(void)
{
	PI_DEBUG (DBG_L2, ascii_data_im_export_pi.name << "Plugin Init");
}

// about-Function
static void ascii_data_im_export_about(void)
{
	const gchar *authors[] = { ascii_data_im_export_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  ascii_data_im_export_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void ascii_data_im_export_configure(void)
{
	if(ascii_data_im_export_pi.app)
		ascii_data_im_export_pi.app->message("ascii_data_im_export Plugin Configuration");
}

// cleanup-Function, make sure the Menustrings are matching those above!!!
static void ascii_data_im_export_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Plugin Cleanup done.");
}

// make a new derivate of the base class "Dataio"
class ascii_ImExportFile : public Dataio{
public:
	ascii_ImExportFile(Scan *s, const char *n) : Dataio(s,n){ };
	virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
	virtual FIO_STATUS Write();
private:
	FIO_STATUS import(const char *fname);
};

FIO_STATUS ascii_ImExportFile::Read(xsm::open_mode mode){
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

FIO_STATUS ascii_ImExportFile::import(const char *fname){
	double length = 1.;
	int columns = 1;
	int rows = 1;
	double dx = 1.;
	double dy = 1.;
	double x0 = 0.;
	double y0 = 0.;
	gchar line[0x4000];

	// Am I resposible for that file, is it a "Ascii" format ?
	ifstream f;
	GString *FileList=NULL;

	if (strncasecmp(fname+strlen(fname)-4,".asc",4) && strncasecmp(fname+strlen(fname)-4,".csv",4)){
		f.close ();
		return status=FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
	}

	f.open(name, ios::in);
	if (!f.good())
	        return status=FIO_OPEN_ERR;
	
	FileList = g_string_new ("Imported by GXSM from ASCII/NSNOM-Reuben data file.\n");
	g_string_append_printf (FileList, "Original Filename: %s\n", fname);
	g_string_append (FileList, "Orig. Header: \n");

	// read header
	for (; f.good ();){
		f.getline (line, 0x4000);

		// append to comment
		g_string_append (FileList, line);
		g_string_append (FileList, "\n");

		//                  0        1         2         3         4
		//                  1234567890123456789012345678901234567890
		if (strncmp (line, "scan length = ", 14) == 0)
			length = atof (&line[14]);
		if (strncmp (line, "points/line = ", 14) == 0)
			columns = atoi (&line[14]);
		if (strncmp (line, "# lines = ", 10) == 0)
			rows = atoi (&line[10]);
		if (strncmp (line, "point spacing = ", 16) == 0)
			dx = atof (&line[16]);
		if (strncmp (line, "line spacing = ", 15) == 0)
			dy = atof (&line[15]);
		if (strncmp (line, "X origin = ", 11) == 0)
			x0 = atof (&line[11]);
		if (strncmp (line, "Y origin = ", 11) == 0)
			y0 = atof (&line[11]);

		// header end mark? then skip to data start
		if (strncmp (line, "end", 3) == 0){
			f.getline (line, 0x4000);
			break;
		}
	}

	time_t t; // Scan - Startzeit eintragen 
	time(&t);
	gchar *tmp = g_strconcat ((ctime(&t)), " (Imported)", NULL); scan->data.ui.SetDateOfScan (tmp); g_free (tmp);
	scan->data.ui.SetName (fname);
	scan->data.ui.SetOriginalName (fname);
	scan->data.ui.SetType ("ASCII-NSNOM"); 


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
	scan->data.s.nx = columns;
	scan->data.s.ny = rows;
	scan->data.s.dx = dx * 1e4; // need Angstroems
	scan->data.s.dy = dy * 1e4;
	scan->data.s.dz = 1. * 1e4;
	scan->data.s.rx = length * 1e4;
	scan->data.s.ry = length * 1e4;
	scan->data.s.x0 = x0 * 1e4;
	scan->data.s.y0 = y0 * 1e4;
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
  
	UnitObj *u = main_get_gapp()->xsm->MakeUnit ("um", "X");
	scan->data.SetXUnit(u);
	delete u;

	u = main_get_gapp()->xsm->MakeUnit ("um", "Y");
	scan->data.SetYUnit(u);
	delete u;

	u = main_get_gapp()->xsm->MakeUnit ("um", "Z");
	scan->data.SetZUnit(u);
	delete u;

	scan->data.ui.SetComment (FileList->str);
	g_string_free(FileList, TRUE); 
	FileList=NULL;

//	scan->data.s.dx = scan->data.s.rx / scan->data.s.nx;
//	scan->data.s.dy = scan->data.s.ry / scan->data.s.ny;

	// check for unkown ascii, attempt to read numbers line by line and auto adjust scan size
	if (scan->data.s.nx == 1 && scan->data.s.ny == 1){
		scan->data.s.dx = 1.; // all in Angstroems
		scan->data.s.dy = 1.;
		scan->data.s.dz = 1.;
		scan->data.s.x0 = 0.;
		scan->data.s.y0 = 0.;
		f.close ();
		f.open(name, ios::in);
		std::cout << "detect NSOM ascii header failed. Attempting auto read ascii matrix" << std::endl;
		do{
			gchar linebuffer[10000];
			gchar *p;
			f.getline (linebuffer, 10000);

			p = strtok (linebuffer, " \t'\",;");
			int col = 0;
			while (p){
				if (scan->data.s.nx < ++col)
					++scan->data.s.nx;
				scan->mem2d->Resize (scan->data.s.nx, scan->data.s.ny, ZD_FLOAT);
				scan->mem2d->PutDataPkt (atof (p), col-1, scan->data.s.ny-1);
//				std::cout << p << " ::(" << scan->data.s.nx << " :" << col << ", " << scan->data.s.ny  << ") = " << atof (p) << std::endl;
				p = strtok (NULL, " \t'\",;");
			}
			++scan->data.s.ny;
		}while(f.good ());
		scan->data.s.rx = scan->data.s.dx * scan->data.s.nx;
		scan->data.s.ry = scan->data.s.dy * scan->data.s.ny;
	} else {

		// Read Img Data.
		scan->mem2d->Resize (scan->data.s.nx, scan->data.s.ny, ZD_FLOAT);

		for (int row=scan->mem2d->GetNy ()-1; row >= 0; --row){
			for (int col=0; col < scan->mem2d->GetNx (); ++col){
				double value = 0.;
				f >> value;
				scan->mem2d->PutDataPkt (value, col, row);
				if (! f.good ())
					break;
			}
			if (! f.good ())
				break;
		}
	}

	f.close ();

	scan->data.orgmode = SCAN_ORG_CENTER;
	scan->mem2d->data->MkXLookup (-scan->data.s.rx/2., scan->data.s.rx/2.);
	scan->mem2d->data->MkYLookup (scan->data.s.ry/2., -scan->data.s.ry/2.);
  
	return FIO_OK; 
}


FIO_STATUS ascii_ImExportFile::Write(){
#if 0
	GtkWidget *dialog = gtk_message_dialog_new (NULL,
						    GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_MESSAGE_INFO,
						    GTK_BUTTONS_OK,
						    N_("ASCII export.")
						    );
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
#endif
	const gchar *fname;
	ofstream f;

	if(strlen(name)>0)
		fname = (const char*)name;
	else
		fname = main_get_gapp()->file_dialog("File Export: ASCII"," ",file_mask,"","ASCII write");
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


static void ascii_data_im_export_filecheck_load_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		PI_DEBUG (DBG_L2, 
			  "Check File: ascii_data_im_export_filecheck_load_callback called with >"
			  << *fn << "<" );

		Scan *dst = main_get_gapp()->xsm->GetActiveScan();
		if(!dst){ 
			main_get_gapp()->xsm->ActivateFreeChannel();
			dst = main_get_gapp()->xsm->GetActiveScan();
		}
		ascii_ImExportFile fileobj (dst, *fn);

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
		PI_DEBUG (DBG_L2, "ascii_data_im_export_filecheck_load: Skipping" );
	}
}

static void ascii_data_im_export_filecheck_save_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		Scan *src;
		PI_DEBUG (DBG_L2,
			  "Check File: ascii_data_im_export_filecheck_save_callback called with >"
			  << *fn << "<" );

		ascii_ImExportFile fileobj (src = main_get_gapp()->xsm->GetActiveScan(), *fn);

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
		PI_DEBUG (DBG_L2, "ascii_data_im_export_filecheck_save: Skipping >" << *fn << "<" );
	}
}

// Menu Call Back Fkte

static void ascii_data_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (ascii_data_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (ascii_data_im_export_pi.name, "-import", NULL);
	gchar *fn = main_get_gapp()->file_dialog_load (help[0], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
	        ascii_data_im_export_filecheck_load_callback (&fn );
                g_free (fn);
	}
}

static void ascii_data_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (ascii_data_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (ascii_data_im_export_pi.name, "-export", NULL);
	gchar *fn = main_get_gapp()->file_dialog_save (help[1], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
	  	ascii_data_im_export_filecheck_save_callback (&fn );
                g_free (fn);
	}
}
