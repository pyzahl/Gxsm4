/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * 
 * Gxsm Plugin Name: nanoimport.C
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
% PlugInDocuCaption: Import of Digital Instruments Nanoscope files
% PlugInName: nanoimport
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: File/Import/Nano Scope

% PlugInDescription
The \GxsmEmph{nanoimport} plug-in supports reading files of Digital
Instruments' Nanoscope systems (Veeco Metrology Group).

This plugin can import original Nanoscope-data files, for older
versions below 0x04300000 (Version: 0x04220200 tested) single channel
data is assumed, for all newer versions (Version: 0x04320003,
0x04430006 tested) multiple channels of data can be present and are
all imported at once using different channels in GXSM. Version below
0x03000000 are rejected (do not know about). The channel type is shown
as scan title. The full ASCII header is appended to the comment.

% PlugInUsage
The plug-in is called by \GxsmMenu{File/Import/Nano Scope}.

% OptPlugInKnownBugs
In some cases the Z-scaling is wrong. The reason for that is, I did
not yet figured out the correct way how to interpret several numbers
given in the di-file header. Any help to fix it is welcome! -- I
changed the calculation once more, look OK for newew files and is
still off for some others, I hate it\dots

According to the manual (V4.1):
\[ H_z = \frac{\text{DAC-value}}{65536} Z_{\text{scale value}} \]

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "dataio.h"
#include "action_id.h"

using namespace std;

// Plugin Prototypes
static void nanoimport_init( void );
static void nanoimport_about( void );
static void nanoimport_configure( void );
static void nanoimport_cleanup( void );

static void nanoimport_run(GtkWidget *w, void *data);

static void nano_import_filecheck_load_callback (gpointer data); // gchar **fn


// Fill in the GxsmPlugin Description here
GxsmPlugin nanoimport_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	g_strdup ("Nanoimport"),
	// Plugin's Category - used to autodecide on Pluginloading or ignoring
	// NULL: load, else
	// example: "+noHARD +STM +AFM"
	// load only, if "+noHARD: no hardware" and Instrument is STM or AFM
	// +/-xxxHARD und (+/-INST or ...)
	NULL,
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	g_strdup ("Imports from Nanoscope Data"),                   
	// Author(s)/* Gnome gxsm - Gnome X Scanning Microscopy
	"Percy Zahl",
	// Menupath to position where it is appendet to
	"file-import-section",
	// Menuentry
	N_("Nano Scope"),
	// help text shown on menu
	N_("Import Digital NanoScope File"),
	// more info...
	"no more info",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	nanoimport_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	nanoimport_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	nanoimport_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	nanoimport_run, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
	NULL, // direct menu entry callback1 or NULL
	NULL, // direct menu entry callback2 or NULL

	nanoimport_cleanup
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm NanoScope Data Import Plugin\n\n"
                                   "This plugin reads in a orig. Data File\n"
				   "from Digital Nano Scope to Gxsm.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	nanoimport_pi.description = g_strdup_printf(N_("Gxsm MathOneArg nanoimport plugin %s"), VERSION);
	return &nanoimport_pi; 
}

// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void nanoimport_init(void)
{
	PI_DEBUG (DBG_L2, "Nanoimport Plugin Init" );
	PI_DEBUG (DBG_L2, "-> connecting to load list" );
	main_get_gapp()->ConnectPluginToLoadFileEvent (nano_import_filecheck_load_callback);
}

// about-Function
static void nanoimport_about(void)
{
	const gchar *authors[] = { nanoimport_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  nanoimport_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void nanoimport_configure(void)
{
	if(nanoimport_pi.app)
		nanoimport_pi.app->message("Nanoimport Plugin Configuration");

}

// cleanup-Function
static void nanoimport_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Nanoimport Plugin Cleanup" );
}

class NanoScopeFile : public Dataio{
public:
	NanoScopeFile(Scan *s, const char *n, int i=0) : Dataio(s,n){ imgindex=i; DIversion=0x00; };
	void  SetIndex(int i){ imgindex=i; };
	int   MultiImage(){ return DIversion < 0x04300000 ? 0:1; }
	virtual FIO_STATUS Read(xsm::open_mode mode=xsm::open_mode::replace);
	virtual FIO_STATUS Write(){ return FIO_OK; };
private:
	int imgindex;
	int DIversion;
};

FIO_STATUS NanoScopeFile::Read(xsm::open_mode mode){
	size_t offset=0, length=0;
	gchar *line=NULL;
	double Zscale=1., Zmag=1.;
	double ZSens_V_to_nm=14.;
	double ZDAtoVolt=0.006713867;
	double CurrentSens_V_to_nA = 10.;
	int bps=2;
	int imgno=0;

	ifstream f;
	GString *FileList=NULL;

	f.open(name, ios::in);
	if(!f.good())
		return status=FIO_OPEN_ERR;

	// Initialize with default!
	scan->data.LoadValues(main_get_gapp()->xsm->Inst, main_get_gapp()->xsm->hardware);

	scan->data.ui.SetName (strrchr(name,'/')?strrchr(name,'/')+1:name);
	scan->data.ui.SetOriginalName ("--NA--");
	scan->data.ui.SetUser ("somebody@nanoscope");
	FileList = g_string_new("Import from Nanoscope Data\n");
	g_string_append_printf (FileList, "Channel No: %d\n", imgindex+1);
	g_string_append (FileList, "Original File Info Header follows:\n");

	scan->data.s.rx = 1.;
	scan->data.s.ry = 1.;
	scan->data.s.nx = 1;
	scan->data.s.ny = 1;
	scan->data.s.x0 = 0.;
	scan->data.s.y0 = 0.;
	scan->data.s.alpha = 0.;

	// you may want to calc this from some Header data...
	scan->data.display.bright = 32.;
	scan->data.display.contrast = 0.01;

	// old: \Version: 0x04220200

	// new: \Version: 0x04320003

	// Get Basic Info
	for(; f.good();){
		gchar tmpline[256];
		f.getline(tmpline, 255);
		g_free (line);
		line = g_strstrip (g_strdup (tmpline));
		if (!line) continue;
		if (strlen(line) < 2) continue;
		g_string_append(FileList, line);
		g_string_append(FileList, "\n");

		if(g_ascii_strncasecmp(line, "\\Version: 0x", 12) == 0){
			sscanf(&line[12],"%x", &DIversion);
			continue;
		}
		if(g_ascii_strncasecmp(line, "\\Date: ", 6) == 0){
			scan->data.ui.SetDateOfScan (&line[6]);
			continue;
		}
		if(g_ascii_strncasecmp(line, "\\Scan size: ",11) == 0){ // assumed in nm
			scan->data.s.rx = scan->data.s.ry = 10.*atof(&line[11]);
			continue;
		}
		if(g_ascii_strncasecmp(line, "\\Samps/line: ",12) == 0){
			scan->data.s.nx = atoi(&line[12]);
			continue;
		}
		if(g_ascii_strncasecmp(line, "\\Lines: ",7) == 0){
			scan->data.s.ny = atoi(&line[7]);
			continue;
		}
		if(g_ascii_strncasecmp(line, "\\X offset: ",10) == 0){
			scan->data.s.x0 = atof(&line[10]);
			continue;
		}
		if(g_ascii_strncasecmp(line, "\\Y offset: ",10) == 0){
			scan->data.s.y0 = atof(&line[10]);
			continue;
		}
		if(g_ascii_strncasecmp(line, "\\Rotate Ang.: ",13) == 0){
			scan->data.s.alpha = 180./M_PI*atof(&line[13]);
			continue;
		}
		if(g_ascii_strncasecmp(line, "\\@Sens. Zscan: V ",16) == 0){
			ZSens_V_to_nm = atof(&line[16]);
			g_string_append_printf (FileList, "--->got ZSens(scan): %fV/nm\n", ZSens_V_to_nm);
			continue;
		}
		if(g_ascii_strncasecmp(line, "\\@Sens. Zsens: V ",16) == 0){
			ZSens_V_to_nm = atof(&line[16]);
			g_string_append_printf (FileList, "--->got ZSens: %fnm/V\n", ZSens_V_to_nm);
			ZSens_V_to_nm = 1./ZSens_V_to_nm;
			g_string_append_printf (FileList, "--->got ZSens: %fV/nm\n", ZSens_V_to_nm);
			continue;
		}
		if(g_ascii_strncasecmp(line, "\\@Sens. Current: V ",18) == 0){
			CurrentSens_V_to_nA = atof(&line[18]);
			g_string_append_printf (FileList, "--->got CurrentSens: %fV/nA\n", CurrentSens_V_to_nA);
			continue;
		}
		if(g_ascii_strncasecmp(line, "\\@Sens. CurrentSens: V ",22) == 0){
			CurrentSens_V_to_nA = atof(&line[18]);
			g_string_append_printf (FileList, "--->got CurrentSens: %fnA/V\n", CurrentSens_V_to_nA);
			CurrentSens_V_to_nA = 1./CurrentSens_V_to_nA;
			g_string_append_printf (FileList, "--->got CurrentSens: %fV/nA\n", CurrentSens_V_to_nA);
			continue;
		}
		if(g_ascii_strncasecmp(line, "\\@1:Z limit: V [Sens. Zscan] (",30) == 0){
			ZDAtoVolt = atof(&line[30]); // 0.006713867V/LSB , 440V (+16bit)
			g_string_append_printf (FileList, "--->got ZDAtoVolt: %fV/LSB\n", ZDAtoVolt);
			continue;
		}

		// until \*Ciao image list
		if(g_ascii_strncasecmp(line, "\\*Ciao image list",16) == 0){
			break;
		}
		if(g_ascii_strncasecmp(line, "\\*File list end ",15) == 0){
			break;
		}
	}
	g_free (line);
	line = NULL;

	scan->data.s.dx = scan->data.s.rx/(double)scan->data.s.nx;
	scan->data.s.dy = scan->data.s.ry/(double)scan->data.s.ny;
	scan->data.s.ntimes  = 1;
	scan->data.s.nvalues = 1;

	if (DIversion < 0x03000000 || !f.good()) // no Nanoscope file or bad/unsupported file format
		return FIO_INVALID_FILE;

	if (DIversion < 0x04300000){
		// Get Img Offset, Length, Bpp
		// Get Basic Info
		g_string_append(FileList, "----------\nImage Information (DI Version < 0x04300000) follows:\n");
		// reopen file
		f.close();
		f.open(name, ios::in);
		for(; f.good();){
			gchar tmpline[256];
			f.getline(tmpline, 255);
			g_free (line);
			line = g_strstrip (g_strdup (tmpline));
			if (!line) continue;
			if (strlen(line) < 2) continue;
			g_string_append(FileList, line);
			g_string_append(FileList, "\n");
      
			if(g_ascii_strncasecmp(line, "\\Data offset: ",13) == 0){
				offset = (size_t)atoi(&line[13]);
				continue;
			}
			if(g_ascii_strncasecmp(line, "\\Data length: ",13) == 0){
				length = (size_t)atoi(&line[13]);
				continue;
			}
			if(g_ascii_strncasecmp(line, "\\Z magnify image: ",17) == 0){
				Zmag = atof(&line[17]);
				continue;
			}
			if(g_ascii_strncasecmp(line, "\\Z scale: ",9) == 0){
				Zscale = atof(&line[9]);
				g_string_append(FileList, "----------\n got Z scale, stopping here reading Header.\n");
				break;
			}
			if(g_ascii_strncasecmp(line, "\\*File list end ",15) == 0)
				break;
		}
		g_free (line); 
		line = NULL;
		g_string_append(FileList, "----------\nEnd of File List.\n");
		g_string_append(FileList, 
				"** Note: I'm unsure about the Z-scale, I use:\n"
				"** Z scale * Z magnify * 0.1 = 1DA/Angstroem\n");
		// may be wrong in this way, I don't know...
		g_string_append_printf (FileList, "--->Z magn. = %f\n", Zmag);
		g_string_append_printf (FileList, "--->Z scale = %f\n", Zscale);
		//    scan->data.s.dz = 0.1*Zscale*Zmag; // in Ang/DA
		scan->data.s.dz = 10.*Zscale/65536.; // in Ang/DA

	}else{

		// skip to wanted Get Image (imgindex)
		while(imgno < imgindex && f.good()){
			gchar tmpline[256];
			f.getline(tmpline, 255);
			g_free (line);
			line = g_strstrip (g_strdup (tmpline));
			if(g_ascii_strncasecmp(line, "\\*Ciao image list",16) == 0)
				++imgno;
			if(g_ascii_strncasecmp(line, "\\*File list end ",15) == 0) 
				break;
		}
		g_free (line); 
		line = NULL;

		if( imgno < imgindex ){
			f.close();
			g_string_free(FileList, TRUE); 
			FileList=NULL;
			return status=FIO_READ_ERR;
		}
    
		// Get Img Offset, Length, Bpp
		// Get Basic Info
		g_string_append(FileList, "----------\nImage Channel Information follows:\n");
		for(; f.good();){
			gchar tmpline[256];
			f.getline(tmpline, 255);
			g_free (line);
			line = g_strstrip (g_strdup (tmpline));
			if (!line) continue;
			if (strlen(line) < 2) continue;
			g_string_append(FileList, line);
			g_string_append(FileList, "\n");
      
			if(g_ascii_strncasecmp(line, "\\Data offset: ",13) == 0){
				offset = (size_t)atoi(&line[13]);
				continue;
			}
			if(g_ascii_strncasecmp(line, "\\Data length: ",13) == 0){
				length = (size_t)atoi(&line[13]);
				continue;
			}
			if(g_ascii_strncasecmp(line, "\\Bytes/pixel: ",13) == 0){
				bps = (size_t)atoi(&line[13]);
				continue;
			}
			if(g_ascii_strncasecmp(line, "\\Valid data start: ",18) == 0){
				continue;
			}
			if(g_ascii_strncasecmp(line, "\\Valid data len X: ",18) == 0){
				continue;
			}
			if(g_ascii_strncasecmp(line, "\\Valid data len Y: ",18) == 0){
				continue;
			}
			if(g_ascii_strncasecmp(line, "\\@2:Image Data: ",15) == 0){
				continue; // Image Data Type/Name
			}
			if(g_ascii_strncasecmp(line, "\\@2:Z scale: V [Sens.",21) == 0){
				gchar *pos = strchr(&line[22], ']');
				if(pos){
					*pos = 0;
					gchar *ScanInfo = g_strdup(&line[22]);
					g_string_append_printf (FileList, "--->got ZScan Info: %s\n", ScanInfo);
					scan->data.ui.SetBaseName (scan->data.ui.name);
					ZDAtoVolt = atof(pos+3); // 0.006713867V/LSB , 440V (+16bit)
					g_string_append_printf (FileList, "--->got ZScan Sens: %fV/LSB\n", ZDAtoVolt);
	  
					gchar *nameext = g_strdup_printf("%s[%d]%s", 
									 scan->data.ui.name, 
									 imgindex+1, 
									 ScanInfo);
					scan->data.ui.SetName (nameext);
					scan->data.ui.SetBaseName (nameext);
					g_free(nameext);
					g_free(ScanInfo);
				}
				else
					g_string_append_printf (FileList, "---> Error finding matching ']'!\n");
				continue;
			}
      
			if(g_ascii_strncasecmp(line, "\\*Ciao image list",16) == 0)
				break;
			if(g_ascii_strncasecmp(line, "\\*File list end ",15) == 0) 
				break;
		}
		g_free (line);
		line = NULL;
		g_string_append(FileList, "----------\nEnd of File List.\n");
// --buggy--
// dZ-calculation, this is really strange, see above for tokens read from header.
//		scan->data.s.dz = 0.1*ZSens_V_to_nm*ZDAtoVolt; // in Ang/DA
		scan->data.s.dz = 10.*ZSens_V_to_nm*ZDAtoVolt; // in Ang/DA
//		scan->data.s.dz = 10.*ZSens_V_to_nm/65536.; // in Ang/DA
    
	}
  
	g_string_append_printf (FileList, "--->Calculated dz = %f Ang/DAunit\n", scan->data.s.dz);

	// fix ugly ASCII in place... conversion below fails otherwise
	for(gchar *p=FileList->str; *p; ++p)
	  if (*p == '\BA') *p='d'; // <== !!!! >> unknown escape sequence: '\B'  ??? no idea what it should be!

	gsize br, bw;
	GError *error = NULL;
//	gchar *utf8_text = g_convert_with_fallback (FileList->str, -1, "UTF-8", "US-ASCII", "?",
//	                                            &br, &bw, &error);
	gchar *utf8_text = g_convert (FileList->str, -1, "UTF-8", "ISO-8859-1", 
                            &br, &bw, &error);

	if (error != NULL){
		gchar *info = g_strdup_printf (
			"Nanoscope header to comment conversion failed:\n"
			"Sorry, an ISO-8859-1 to UTF-8 conversion error occured,\n"
			"g_convert retured an error code.\n"
			"Bytes read: %d, written: %d",
			(int)br, (int)bw
			);
		scan->data.ui.SetComment (info);
		g_free (info);
		g_error_free (error);
	} else
		scan->data.ui.SetComment (utf8_text);

	g_free (utf8_text);
	g_string_free(FileList, TRUE); 
	FileList=NULL;

	// Read Img Data.
    
	f.seekg(offset, ios::beg);
    
	scan->mem2d->Resize (scan->data.s.nx, scan->data.s.ny);
	scan->mem2d->DataRead (f, -1);
	scan->mem2d->data->MkXLookup (-scan->data.s.rx/2., scan->data.s.rx/2.);
	scan->mem2d->data->MkYLookup (scan->data.s.ry/2., -scan->data.s.ry/2.);
  
	if(f.fail()){
		f.close();
		return status=FIO_READ_ERR; 
	}
	f.close();
	return status=FIO_OK; 
}


static void nano_import_filecheck_load_callback (gpointer data){
	gchar **fn = (gchar**)data;
	if (*fn){
		Scan *dst;
		int ret, ret0;
		PI_DEBUG (DBG_L2, "Check File: nano_import_filecheck_load_calback >"
			  << *fn << "< SORRY NO AUTDETECT FOR NANOSCOPE FILES." );

		return;
		// ---------------- not working, need proper file type identification here!!!

		main_get_gapp()->xsm->ActivateFreeChannel();
		NanoScopeFile NanoFile(dst = main_get_gapp()->xsm->GetActiveScan(), *fn);
		for(int i=0; i<3; ++i){
			if(!dst){
				main_get_gapp()->xsm->ActivateFreeChannel();
				dst = main_get_gapp()->xsm->GetActiveScan();
			}
			NanoFile.SetIndex(i);
			NanoFile.SetScan(dst);
			if((ret=NanoFile.Read()) != FIO_OK){ 
				// no more data: remove allocated and unused scan now, force!
				if(i==0) ret0 = ret;
//				main_get_gapp()->xsm->SetMode(-1, ID_CH_M_OFF, TRUE); 
				break; 
			}
			if(i==0) ret0 = ret;
			main_get_gapp()->xsm->ActiveScan->GetDataSet(main_get_gapp()->xsm->data);
			main_get_gapp()->xsm->ActiveScan->auto_display ();
			main_get_gapp()->spm_update_all();
			dst->draw();
			dst=NULL;
			if(! NanoFile.MultiImage() ) break;
		}

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
			// Now update

			main_get_gapp()->xsm->ActiveScan->GetDataSet(main_get_gapp()->xsm->data);
			main_get_gapp()->xsm->ActiveScan->auto_display ();
			main_get_gapp()->spm_update_all();
			dst->draw();
		}
	}else{
		PI_DEBUG (DBG_L2, "nano_import_filecheck_load: Skipping" );
	}
}

// run-Function
static void nanoimport_run(GtkWidget *w, void *data)
{
	Scan *dst;
	gchar *nfname = main_get_gapp()->file_dialog("NanoScope file to load", NULL, 
					  "*", NULL, "NanoImport");
	if( !nfname ) return;

	main_get_gapp()->xsm->ActivateFreeChannel();
	NanoScopeFile NanoFile(dst = main_get_gapp()->xsm->GetActiveScan(), nfname);
	for(int i=0; i<3; ++i){
		if(!dst){
			main_get_gapp()->xsm->ActivateFreeChannel();
			dst = main_get_gapp()->xsm->GetActiveScan();
		}
		NanoFile.SetIndex(i);
		NanoFile.SetScan(dst);
		if(NanoFile.Read() != FIO_OK){ 
			// no more data: remove allocated and unused scan now, force!
//			main_get_gapp()->xsm->SetMode(-1, ID_CH_M_OFF, TRUE); 
			break; 
		}
		main_get_gapp()->xsm->ActiveScan->GetDataSet(main_get_gapp()->xsm->data);
		main_get_gapp()->xsm->ActiveScan->auto_display ();
		main_get_gapp()->spm_update_all();
		dst->draw();
		dst=NULL;
		if(! NanoFile.MultiImage() ) break;
	}
}
