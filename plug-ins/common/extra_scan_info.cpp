/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: extra_scan_info.C
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
% PlugInDocuCaption: Query Hardware/DSP software information
% PlugInName: extra_scan_info
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Tools/Hardware Info

% PlugInDescription
\label{plugin:extra_scan_info} 

This plugin looks for a file named \GxsmFile{gxsm\_extra\_scan\_info} in the current
working directory at each file save action. It's content is added to the NetCDF file
as variable named extra\_scan\_info.


% PlugInUsage
Works automatic if loaded.

% OptPlugInNotes

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"

using namespace std;


// ========================================
// Gxsm PlugIn Setup Part
// ========================================

static void extra_scan_info_init( void );
static void extra_scan_info_query( void );
static void extra_scan_info_cleanup( void );
static void extra_scan_info_configure( void );
static void extra_scan_info_about( void );
static void extra_scan_info_run(GtkWidget *w, void *data);

static void extra_scan_info_SaveValues_callback ( gpointer );
static void extra_scan_info_LoadValues_callback ( gpointer );

                                                                                                                             
GxsmPlugin extra_scan_info_pi = {
	NULL,
	NULL,
	0,
	NULL,
	"extra_scan_info",
	NULL,
	NULL,
	"Percy Zahl",
	"tools-section",
	N_("Extra Scan Info"),
	N_("Show, save and load extra scan informations"),
	"no more info",
	NULL,
	NULL,
	extra_scan_info_init,
	extra_scan_info_query,
	extra_scan_info_about,
	extra_scan_info_configure,
	extra_scan_info_run,
	NULL,
	NULL,
	extra_scan_info_cleanup
};

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	extra_scan_info_pi.description = g_strdup_printf(N_("extra_scan_info Plugin %s"), VERSION);
	return &extra_scan_info_pi; 
}

static void extra_scan_info_init(void){
	XsmRescourceManager xrm("extra_scan_info");
//	ptr=xrm.Get ("info_0", "---");
	xrm.Put ("info_0", "extra scan info null");
}

static void extra_scan_info_query(void){
        PI_DEBUG (DBG_L4, "Extra Scan Info::Query, connecting callbacks\n");
        extra_scan_info_pi.app->ConnectPluginToCDFSaveEvent (extra_scan_info_SaveValues_callback);
        extra_scan_info_pi.app->ConnectPluginToCDFLoadEvent (extra_scan_info_LoadValues_callback);

	if (extra_scan_info_pi.status) 
		g_free (extra_scan_info_pi.status);

	extra_scan_info_pi.status = g_strdup ("query::command=install_menus");
}

static void extra_scan_info_configure(void){
}

static void extra_scan_info_about(void){
	const gchar *authors[] = { "Percy Zahl", NULL};
	gtk_show_about_dialog (NULL,
			       "program-name", extra_scan_info_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "This PlugIn does show, load and save handling of "
			       "extra scan informations.",
			       "authors", authors,
			       NULL
			       );
}

static void extra_scan_info_run(GtkWidget *w, void *data){
        PI_DEBUG (DBG_L4, "Extra Scan Info::ShowValues (run) callback\n");
	GString *gs_info=NULL;
	ifstream f_info;
	f_info.open("gxsm_extra_scan_info", ios::in);
	
	if(!f_info.good())
		gs_info = g_string_new ("Extra Scan Info: N/A");
	else
		gs_info = g_string_new ("Extra Scan Info: OK\n");

	while(f_info.good()){
		gchar line[0x4000];
		f_info.getline (line, 0x4000);

		// append to comment
		g_string_append (gs_info, line);
		g_string_append (gs_info, "\n");
	}

	f_info.close ();
	PI_DEBUG (DBG_L4, gs_info->str);

	main_get_gapp()->message (gs_info->str);

	g_string_free(gs_info, TRUE);
}

static void extra_scan_info_cleanup( void ){
}

static void extra_scan_info_SaveValues_callback ( gpointer gp_ncf ){
	GString *gs_info=NULL;
	//NcFile *ncf = (NcFile *) gp_ncf;
        NcFile *ncf = static_cast<NcFile*>(gp_ncf);
        PI_DEBUG (DBG_L4, "Extra Scan Info::SaveValues_callback\n");
	ifstream f_info;
	f_info.open("gxsm_extra_scan_info", ios::in);
	
	if(!f_info.good())
		gs_info = g_string_new ("Extra Scan Info: N/A");
	else
		gs_info = g_string_new ("Extra Scan Info: OK\n");

	while(f_info.good()){
		gchar line[0x4000];
		f_info.getline (line, 0x4000);

		// append to comment
		g_string_append (gs_info, line);
		g_string_append (gs_info, "\n");
	}

	f_info.close ();

	NcDim infod  = ncf->addDim ("extra_scan_info_dim", strlen(gs_info->str));
	NcVar info   = ncf->addVar ("extra_scan_info", ncChar, infod);
	info.putAtt ("long_name", "extra scan information");
	info.putVar (gs_info->str); //, strlen(gs_info->str));
               
	g_string_free(gs_info, TRUE);
}

#define NC_GET_VARIABLE(VNAME, VAR) if(!ncf->getVar(VNAME).isNull ()) ncf->getVar(VNAME).getVar(VAR)

static void extra_scan_info_LoadValues_callback ( gpointer gp_ncf ){
        //NcFile *ncf = (NcFile *) gp_ncf;
        PI_DEBUG (DBG_L4, "Extra Scan Info::LoadValues_callback\n");
        //load_values ((NcFile *) ncf);
}

/*
NcVar::putAtt 	( 	const std::string &  	name,
		size_t  	len,
		const char **  	dataValues 
	)
*/
