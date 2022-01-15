/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: WSxM_io.C
 * ========================================
 * 
 * Copyright (C) 2001 The Free Software Foundation
 *
 * Authors: Thorsten Wagner <linux@ventiotec.com> 
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
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Import/export for WSxM Nanotec Electronica SPM data
% PlugInName: WSxM_io
% PlugInAuthor: Thorsten Wagner
% PlugInAuthorEmail: stm@users.sf.net
% PlugInMenuPath: File/Im,Export/WXsM

% PlugInDescription
Data import/export of the WSxM data format (version 2) used by Nanotec
Electronica.

% PlugInUsage
Call it from \GxsmMenu{File/Im,Export/WXsM}. Also a normal open or
drag- and drop action from gmc, nautilus or Netscape (URL) will
automatic import WSxM files. 
Plugin uses nc2top to export WSxM files. Therefor, it saves a .nc file first, and then runs nc2top.
If you want, you can change the path of nc2top in the configuration menu, and you can also set if the .nc files shall be deleted after conversion


% OptPlugInRefs
\GxsmWebLink{www.nanotec.es}

%% OptPlugInKnownBugs
%Are there known bugs? List! How to work around if not fixed?
The import algorithm is not complete yet. You will get the image, but the scan data are only fake.

% OptPlugInNotes
DnD and URL drops are not tested.

%% OptPlugInHints
%Any tips and tricks?

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "dataio.h"
#include "glbvars.h"
#include "xsmtypes.h"
#include "surface.h"

#include "WSxM_header.h"                // File distributed with WSxM to make header of data 

// includes obviously not longer needed, 15.11.2009 Thorsten Wagner (STM)
//#include "action_id.h"
//#include <linux/coff.h>
//#include "plug-ins/hard/sranger_hwi.h"
//#include <netcdf.hh>                    // used to read additional data from original file

#define WSXM_MAXCHARS 1000
#define IS_PICO_AMP 100      // Check whether saving as pico is more suitable

using namespace std;


// Plugin Prototypes
static void WSxM_im_export_init (void);
static void WSxM_im_export_query (void);
static void WSxM_im_export_about (void);
static void WSxM_im_export_configure (void);
static void WSxM_im_export_cleanup (void);

static void WSxM_im_export_filecheck_load_callback (gpointer data );
static void WSxM_im_export_filecheck_save_callback (gpointer data );

static void WSxM_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void WSxM_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// Fill in the GxsmPlugin Description here
GxsmPlugin WSxM_im_export_pi = {
        NULL,                   // filled in and used by Gxsm, don't touch !
        NULL,                   // filled in and used by Gxsm, don't touch !
        0,                      // filled in and used by Gxsm, don't touch !
        NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
        // filled in here by Gxsm on Plugin load, 
        // just after init() is called !!!
        "WSxM-ImExport", 	  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
        NULL,		   	  // Plugin's Category - used to autodecide on Pluginloading or ignoring -- NULL: always
        g_strdup ("Im/Export of WSxM file format by Nanotec.es."), // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
        "Thorsten Wagner (ventiotec (R) Dolega und Wagner, Andreas Sonntag",	  // Author(s)/* Gnome gxsm - Gnome X Scanning Microscopy
        "file-import-section,file-export-section", // sep. im/export menuentry path by comma!
        N_("WSxM,WSxM"), // menu entry (same for both)
        N_("WSxM import,WSxM export"), // short help for menu entry
        N_("WSxM data file import and export filter.\n WSxM is a powerful data manipulation program for spm data. \n Visit for more details www.nanotec.es!"),
        NULL,          // error msg, plugin may put error status msg here later
        NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
        WSxM_im_export_init,  
        WSxM_im_export_query,  
        // about-function, can be "NULL"
        // can be called by "Plugin Details"
        WSxM_im_export_about,
        // configure-function, can be "NULL"
        // can be called by "Plugin Details"
        WSxM_im_export_configure,
        // run-function, can be "NULL", if non-Zero and no query defined, 
        // it is called on menupath->"plugin"
        NULL,
        // cleanup-function, can be "NULL"
        // called if present at plugin removal
        WSxM_im_export_import_callback, // direct menu entry callback1 or NULL
        WSxM_im_export_export_callback, // direct menu entry callback2 or NULL

        WSxM_im_export_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm file import/export plugin for WSxM files\n\n"
                                   "This plugin writes in WSxM datafiles using nc2top"
                                   );

static const char *file_mask = "*.top";

char converter[WSXM_MAXCHARS];

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
        WSxM_im_export_pi.description = g_strdup_printf(N_("Gxsm WSxM import/export plugin %s"), VERSION);
        return &WSxM_im_export_pi; 
}

// Query Function, installs Plugin's in File/Import and Export Menupaths!
// ----------------------------------------------------------------------
// Import Menupath is "File/Import/WSxM import"
// Export Menupath is "File/Export/WSxM import"
// ----------------------------------------------------------------------
// !!!! make sure the "WSxM_im_export_cleanup()" function (see below) !!!!
// !!!! removes the correct menuentries !!!!

static void WSxM_im_export_query(void)
{
	if(WSxM_im_export_pi.status) g_free(WSxM_im_export_pi.status); 
	WSxM_im_export_pi.status = g_strconcat (
                                                N_("Plugin query has attached "),
                                                WSxM_im_export_pi.name, 
                                                N_(": File IO Filters are ready to use"),
                                                NULL);
	
        // register this plugins filecheck functions with Gxsm now!
        // This allows Gxsm to check files from DnD, open, 
        // and cmdline sources against all known formats automatically - no explicit im/export is necessary.
	WSxM_im_export_pi.app->ConnectPluginToLoadFileEvent (WSxM_im_export_filecheck_load_callback);
	WSxM_im_export_pi.app->ConnectPluginToSaveFileEvent (WSxM_im_export_filecheck_save_callback);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void WSxM_im_export_init(void)
{
        PI_DEBUG (DBG_L2, "WSxM Import/Export Plugin Init" );
}

// about-Function
static void WSxM_im_export_about(void)
{
        const gchar *authors[] = { WSxM_im_export_pi.authors, NULL};
        gtk_show_about_dialog (NULL,
                               "program-name",  WSxM_im_export_pi.name,
                               "version", VERSION,
                               "license", GTK_LICENSE_GPL_3_0,
                               "comments", about_text,
                               "authors", authors,
                               NULL
                               );
}

// configure-Function
static void WSxM_im_export_configure(void)
{
        if(WSxM_im_export_pi.app){
		XsmRescourceManager xrm("WSXM_IM_EXPORT");
		GtkWidget *checkbox;
		GtkDialogFlags flags =  (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);
		GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("WSxM Import/Export Plugin Configuration"),
								 GTK_WINDOW (main_get_gapp()->get_app_window ()),
								 flags,
								 _("_OK"),
								 GTK_RESPONSE_ACCEPT,
								 _("_Cancel"),
								 GTK_RESPONSE_REJECT,
								 NULL);
		BuildParam bp;
		gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);
		
		snprintf (converter, WSXM_MAXCHARS,"%s", xrm.GetStr ("converter_path", "0"));
		GtkWidget *input = bp.grid_add_input ("Converter Path");
		bp.new_line ();
		
		checkbox=gtk_check_button_new_with_label ("Delete temporary .nc files");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkbox),xrm.GetBool ("remove_temp",FALSE));

		bp.grid_add_widget (checkbox);
		bp.new_line ();

		if (!strcmp(converter,"")||!strcmp(converter,"0")){
                        snprintf(converter,WSXM_MAXCHARS,"/usr/local/bin/./nc2top");
		}
                
		gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (input))), converter, -1);

		gtk_widget_show (dialog);

                int response = GTK_RESPONSE_NONE;
                g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data_no_destroy), &response);
        
                // FIX-ME GTK4 ??
                // wait here on response
                while (response == GTK_RESPONSE_NONE)
                        while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
		
		bool deltemp;		

		deltemp=gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(checkbox));
		snprintf (converter,WSXM_MAXCHARS, "%s", gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY (input)))));

		gtk_window_destroy (GTK_WINDOW(dialog));

		xrm.Put("converter_path",converter);
		xrm.PutBool ("remove_temp",deltemp);

	}

}

// cleanup-Function, make sure the Menustrings are matching those above!!!
static void WSxM_im_export_cleanup(void)
{
#if 0
	gchar **path  = g_strsplit (WSxM_im_export_pi.menupath, ",", 2);
	gchar **entry = g_strsplit (WSxM_im_export_pi.menuentry, ",", 2);

	gchar *tmp = g_strconcat (path[0], entry[0], NULL);
	gnome_app_remove_menus (GNOME_APP (WSxM_im_export_pi.app->getApp()), tmp, 1);
	g_free (tmp);

	tmp = g_strconcat (path[1], entry[1], NULL);
	gnome_app_remove_menus (GNOME_APP (WSxM_im_export_pi.app->getApp()), tmp, 1);
	g_free (tmp);

	g_strfreev (path);
	g_strfreev (entry);
#endif
	PI_DEBUG (DBG_L2, "Plugin Cleanup done.");
}


/* --------------------------------------------------------------------------------
   make a new derivate of the base class "Dataio"
   definition of all used function for im- and export
   --------------------------------------------------------------------------------*/

class WSxM_ImExportFile : public Dataio{
public:
        WSxM_ImExportFile(Scan *s, const char *n) : Dataio(s,n){ };
        virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
        virtual FIO_STATUS Write();
private:
        FIO_STATUS WSxMRead(const char *fname);
        void WriteInLowEndian (const void *buffer, FILE *stream);
        void SetUnit(gchar* sValue, gchar* sUnit, double dValae, double dFactor=1.0);
        void ReplaceChar(char *string, char oldpiece=',', char newpiece='.');
        void SeparateLine(char *sInput, char *sOutput, char cSeparator=10);
        void ClearString(char *sInput);
};

// d2d import :=)
FIO_STATUS WSxM_ImExportFile::Read(xsm::open_mode mode){
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
		PI_DEBUG (DBG_L2, "Error: Can't open file!" );
		return status=FIO_OPEN_ERR;
	}
	f.close();

	// Check all known File Types:
	
	// WSxM ".top" type?
	if ((ret=WSxMRead (fname)) !=  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE)
		return ret;

	return  FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
}


/*----------------------------------------------------------------------
  import WSxM file format
  ----------------------------------------------------------------------*/

FIO_STATUS WSxM_ImExportFile::WSxMRead(const char *fname){
	// Am I resposible for that file, is it a "top" format ?
	if (strncmp(fname+strlen(fname)-4,".top",4))
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;
	
	// feel free to do a more sophisticated file type check...
	PI_DEBUG (DBG_L2, "WSXM_IO_read: It's a WSxM File!" );
	// initialize variables first
	// file name
	char *szSourceFileName = (gchar *)fname;
	// file pointer
	FILE *pSourceFile = NULL;
	// header
	WSxM_HEADER header;
	
	// parameter for opening binary data
	// set 1 for UNIX and 0 otherwise  
	int bUnixEnv = 1;  
	// the header is ASCII. So opening the header in text mode
	pSourceFile = fopen (szSourceFileName, "rt");
	if (pSourceFile==NULL){
		return FIO_OPEN_ERR;
        }
	// initializing Header for read
	HeaderInit (&header);
	HeaderRead (&header,pSourceFile);

	// close file and open it again in binary mode
	pSourceFile = fopen (szSourceFileName,"rb");
	if (pSourceFile == NULL){
		return FIO_OPEN_ERR;
        }
		

        int iCol, iRow, iNumCols, iNumRows;
        int maxDim;
	short buffer;
	char szComments[100] = "";

	iNumCols = (int) HeaderGetAsNumber (&header, IMAGE_HEADER_GENERAL_INFO, IMAGE_HEADER_GENERAL_INFO_NUM_COLUMNS);
	iNumRows = (int) HeaderGetAsNumber (&header, IMAGE_HEADER_GENERAL_INFO, IMAGE_HEADER_GENERAL_INFO_NUM_ROWS);
        //printf("WSXM_IO_read: header size: %i\n",HeaderGetSize(&header));
        //printf( "WSXM_IO_read: rows: %i\n",iNumRows);
        //printf( "WSXM_IO_read: cols: %i\n",iNumCols);
	// Lets allocate some memory for the scan data.The data of the type short
	maxDim = iNumCols>iNumRows?iNumCols:iNumRows;
        scan->mem2d->Resize (
                             iNumCols,
                             iNumRows,
                             ZD_SHORT
                             );
        scan->data.s.nx = iNumCols;
        scan->data.s.ny = iNumRows;
        PI_DEBUG (DBG_L2, "WSXM_IO_read: Memory set to short.\n");
	// this is mandatory.
	scan->data.s.ntimes  = 1;
	scan->data.s.nvalues = 1;
        PI_DEBUG (DBG_L2, "WSXM_IO_read: only one layer.\n");

  	/* Set the position of file pointer after the image header */
	fseek (pSourceFile, -iNumCols*iNumRows*sizeof(short), SEEK_END);
        PI_DEBUG (DBG_L2, "WSXM_IO_read: set file pointer to beginning of binary data.\n");
	// printf("WSxM header size: %i\n",HeaderGetSize(&header));
	// Do not HeaderGetSize() because it does return the wrong number !!!

	/* Reads a point from the source file and writes it to the memory */
	for (iRow=0; iRow<iNumRows; iRow++)
                {
                        for (iCol=0; iCol<iNumCols; iCol++)
                                {
                                        fread (&buffer, sizeof (short), 1, pSourceFile);
                                        scan->mem2d->PutDataPkt (
                                                                 (double)buffer,
                                                                 iNumCols-iCol-1, iNumRows-iRow-1); 
                                        // You need to mirror the image in the center
				}
                }
	PI_DEBUG (DBG_L2, "WSXM_IO_read: Data has been read succesfully!\n");
	// put some usefull values in the ui structure
	if(getlogin()){
		scan->data.ui.SetUser (getlogin());
	}
	else{
		scan->data.ui.SetUser ("unkonwn user");
	}
	time_t t; // Scan - set timestamp 
	time(&t);
	gchar *tmp;
	tmp = g_strconcat ((ctime(&t)), "(Imported)", NULL);
	scan->data.ui.SetDateOfScan (tmp);
	g_free (tmp);
	scan->data.ui.SetName (fname);
	scan->data.ui.SetOriginalName (fname);
	scan->data.ui.SetType ("WSxM Type: SHORT ");
	tmp = g_strconcat (scan->data.ui.comment,
			   "Imported by gxsm WSxM import plugin from:\n",
			   fname,
			   NULL);
	scan->data.ui.SetComment (tmp);
	g_free (tmp);

	// initialize scan structure -- this is a minimum example
	scan->data.s.dx = 1;
	scan->data.s.dy = 1;
	scan->data.s.dz = 1;
	scan->data.s.rx = scan->data.s.nx;
	scan->data.s.ry = scan->data.s.ny;
	scan->data.s.x0 = 0;
	scan->data.s.y0 = 0;
	scan->data.s.alpha = 0;
	scan->mem2d->data->MkYLookup(scan->data.s.y0, scan->data.s.y0+scan->data.s.ry);
	scan->mem2d->data->MkXLookup(scan->data.s.x0+scan->data.s.rx/2., scan->data.s.x0-scan->data.s.rx/2.);

	// be nice and reset this to some defined state
	scan->data.display.z_high       = 1e3;
	scan->data.display.z_low        = 1.;

	// set the default view parameters
	scan->data.display.bright = 32.;
	scan->data.display.contrast = 1.0;
	
	/* Header destruction */
	HeaderDestroy (&header);
	/* Close the files */
	fclose (pSourceFile);
	// WSxM read done.
	return FIO_OK; 
}


/*----------------------------------------------------------------------
  export WSxM file format
  ----------------------------------------------------------------------*/

FIO_STATUS WSxM_ImExportFile::Write(){
	const gchar *fname;
	char ncfile[WSXM_MAXCHARS];
	char command[WSXM_MAXCHARS];
	bool saved=false;
	if(strlen(name)>0)
		fname = (const char*)name;
	else
		fname = main_get_gapp()->file_dialog("File Export: WSxM type"," ","*.top","","WSxM write");
	if (fname == NULL) return FIO_NO_NAME;

	// check if we like to handle this
	if (strncmp(fname+strlen(fname)-4,".top",4))
		return FIO_NOT_RESPONSIBLE_FOR_THAT_FILE;


        XsmRescourceManager xrm("WSXM_IM_EXPORT");
        snprintf(ncfile,strlen(fname)-3,"%s",fname);
        strcat(ncfile,".nc");

        FILE *testfile=fopen(ncfile,"r"); //test if nc file exists. if it does dont overwrite it.
        FIO_STATUS state;
        if (testfile==NULL)
                {
                        PI_DEBUG (DBG_L2, "Saving temporary file as nc");
                        saved =true;
                        NetCDF file(scan,ncfile);
                        state= file.Write();
                        if (state!=FIO_OK)
                                {
                                        PI_DEBUG (DBG_L2, "ERROR: write error");
                                        return FIO_WRITE_ERR;
                                }
                }
        else fclose(testfile);



        snprintf(converter,WSXM_MAXCHARS,"%s",xrm.GetStr ("converter_path", "0"));
        if (!strcmp(converter,"")||!strcmp(converter,"0"))WSxM_im_export_configure(); // if no converter path was specified, run config menu
	
	


        //check if nc2top was found
        testfile=fopen(converter,"r");
        if (testfile==NULL) { 
                //	PI_DEBUG (DBG_L2, "ERROR: could not find converter");
                printf("ERROR: could not find converter");
                return FIO_WRITE_ERR;
	}
        else fclose(testfile);
        snprintf(command, WSXM_MAXCHARS, "%s -v \"%s\" \"%s\"", converter, ncfile, fname);

        int error=system(command); // run converter)

        if (error !=0) 
                {
                        //	PI_DEBUG (DBG_L2, "ERROR: converter error");
                        printf("ERROR: converter error");
                        return FIO_STATUS(error);
                }


        if (xrm.GetBool ("remove_temp",FALSE) && saved==true)	//remove temp file
                {	
                        snprintf(command,WSXM_MAXCHARS,"rm -rf \"%s\"",ncfile);
                        system(command);
                }

        return FIO_OK; 
}


/*----------------------------------------------------------------------
  WriteInLowEndian: Write binary data in PC style (Windows compatible)
  ----------------------------------------------------------------------*/

void WSxM_ImExportFile::WriteInLowEndian(const void *buffer, FILE *stream)
{
        fwrite (&((char*)buffer)[0],sizeof (char),1,stream);
        fwrite (&((char*)buffer)[1],sizeof (char),1,stream);
        // PI_DEBUG (DBG_L2, "Write in low endian style");
	
}

/*----------------------------------------------------------------------
  SetUnit: Adds Unit to String
  ----------------------------------------------------------------------*/
void WSxM_ImExportFile::SetUnit(gchar* sValue, gchar* sUnit, double dValue, double dFactor)
{// Global unit table declared in xsm.C
        UnitsTable *UnitPtr = XsmUnitsTable;					             
        gchar prec_str[WSXM_MAXCHARS];
        gchar *finalUnit;
        dValue=dValue*dFactor; // Use dFactor for scalling
        // Checking for the correct unit of the tunneling current, because WSxM only acceptes 2 decimals
        if(!strcmp(sUnit,"nA"))
                {
                        if(((int)(dValue * IS_PICO_AMP)) == 0)
                                {// better use pA
                                        finalUnit = g_strdup("pA");
                                }
                        else
                                {// use standard unit
                                        finalUnit = g_strdup(sUnit);
                                }	
                }
        else
                finalUnit = g_strdup(sUnit);		
   
   	
        // handling the Arc Unit
        if(!strcmp(finalUnit,"Deg"))
                {
                        snprintf(sValue,WSXM_MAXCHARS,"%g %c",dValue / UnitPtr->fac, 176);   
                        g_free(finalUnit);
                        return;
                }
  
        //check if a global unit alias exists
        for(;UnitPtr->alias;UnitPtr++)				      						
                if(!strcmp(finalUnit,UnitPtr->alias))
                        break;
 
        g_free(finalUnit);
        if(!UnitPtr->alias)
                {// Default: digital units
                        snprintf(sValue,WSXM_MAXCHARS,"%g", dValue);
                        PI_DEBUG(DBG_L2,"Data will be saved in digital units");
                }
        else
                {// substituting A with 197
                        if(!strcmp(UnitPtr->alias,"AA"))
                                {// angstrom need an extra handling
                                        snprintf(prec_str,WSXM_MAXCHARS,"%%%s %%c",UnitPtr->prec2);
                                        snprintf(sValue,WSXM_MAXCHARS,prec_str,dValue / UnitPtr->fac,197);  
                                        return;
                                }
                        else {
                                snprintf(prec_str,WSXM_MAXCHARS,"%%%s %%s",UnitPtr->prec2);
                                snprintf(sValue,WSXM_MAXCHARS,prec_str,dValue / UnitPtr->fac, UnitPtr->s);	
                        }
                }
}

/*----------------------------------------------------------------------
  ReplaceChar: Replaces A Char Within A String
  Default: Replacing ',' by '.'
  ----------------------------------------------------------------------*/
void WSxM_ImExportFile::ReplaceChar(char *string, char oldpiece, char newpiece)
{
        for (;*string;string++)
                {
                        if (*string==oldpiece) *string=newpiece;
                }
}

/*----------------------------------------------------------------------
  SeparateLine: Separates Lines vom sInput to sOutput
  Default Separator is '\n'
  ----------------------------------------------------------------------*/

void WSxM_ImExportFile::SeparateLine(char *sInput, char *sOutput, char cSeparator)
{
	for (;*sInput;sInput++)
                {
                        if (*sInput==cSeparator) {
                                break;
                        }
                        else {
                                strncat(sOutput,sInput,1);
                        }
                }
}

/*----------------------------------------------------------------------
  ClearString: Fills string with  \0
  ----------------------------------------------------------------------*/
void WSxM_ImExportFile::ClearString(char *sInput){
	for( int i = 0; i < WSXM_MAXCHARS; i++){
		sInput[i]=0;
	}
}
// Plugin's Notify Cb's, registered to be called on file load/save to check file
// return via filepointer, it is set to Zero or passed as Zero if file has been processed!
// That's all fine, you should just change the Text Stings below...


static void WSxM_im_export_filecheck_load_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		PI_DEBUG (DBG_L2, "Check File: WSxM_im_export_filecheck_load_callback called with >"
                          << *fn << "<" );

		Scan *dst = main_get_gapp()->xsm->GetActiveScan();
		if(!dst){ 
			main_get_gapp()->xsm->ActivateFreeChannel();
			dst = main_get_gapp()->xsm->GetActiveScan();
		}
		WSxM_ImExportFile fileobj (dst, *fn);

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
		PI_DEBUG (DBG_L2, "WSxM_im_export_filecheck_load: Skipping" );
	}
}

static void WSxM_im_export_filecheck_save_callback (gpointer data ){
	gchar **fn = (gchar**)data;
	if (*fn){
		Scan *src;
		PI_DEBUG (DBG_L2, "Check File: WSxM_im_export_filecheck_save_callback called with >"
                          << *fn << "<" );

		WSxM_ImExportFile fileobj (src = main_get_gapp()->xsm->GetActiveScan(), *fn);

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
		PI_DEBUG (DBG_L2, "WSxM_im_export_filecheck_save: Skipping >" << *fn << "<" );
	}
}

// Menu Call Back Function

static void WSxM_im_export_import_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (WSxM_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (WSxM_im_export_pi.name, "-import", NULL);
	gchar *fn = main_get_gapp()->file_dialog_load (help[0], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
                WSxM_im_export_filecheck_load_callback (&fn );
                g_free (fn);
	}
}

static void WSxM_im_export_export_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	gchar **help = g_strsplit (WSxM_im_export_pi.help, ",", 2);
	gchar *dlgid = g_strconcat (WSxM_im_export_pi.name, "-export", NULL);
	gchar *fn = main_get_gapp()->file_dialog_save (help[1], NULL, file_mask, NULL);
	g_strfreev (help); 
	g_free (dlgid);
	if (fn){
                WSxM_im_export_filecheck_save_callback (&fn );
                g_free (fn);
	}
}
