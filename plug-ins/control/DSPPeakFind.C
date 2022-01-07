/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: DSPPeakFind.C
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
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: SPA--LEED peak finder and monitor (OBSOLETE)
% PlugInName: DSPPeakFind
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: windows-sectionDSP PeakFind

% PlugInDescription
 For SPA--LEED users Gxsm offers a peak intensity monitor and peak
 finder to follow a shifting peak. It is designed for both: Focus
 adjustment and peak intensity monitoring. It can handle a unlimited
 number of peaks simultaneous.


% PlugInUsage
 Open \GxsmMenu{windows-sectionDSP PeakFind}.

% OptPlugInConfig
 Lots of controls\dots Since I do not know about other SPA--LEED users I'll
 not spend more time here!

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

%% OptPlugInRefs
%Any references?

% OptPlugInKnownBugs
 The long term peak intensity monitor is unstable and beta. I need more
 feedback from SPA--LEED users\dots

% OptPlugInNotes
 This code is still beta and was not tested for a long time with newer
 Gxsm version, except in simulation mode. There is a SPA--LEED
 simulator kernel module:\\
 \GxsmFile{Gxsm/plug-ins/hard/modules/dspspaemu.o}


% OptPlugInHints
 Set the X/Y sign checkbox correct if the finder corrects in the wrong direction.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include "app_peakfind.h"

static void DSPPeakFind_about( void );
static void DSPPeakFind_query( void );
static void DSPPeakFind_cleanup( void );

static void DSPPeakFind_show_callback( GtkWidget*, void* );

GxsmPlugin DSPPeakFind_pi = {
		NULL,
		NULL,
		0,
		NULL,
		"DSPPeakFind",
		"+spaHARD +SPALEED",
		NULL,
		"Percy Zahl",
		"windows-section",
		N_("DSP PeakFind"),
		N_("open the DSP Peak Finder controlwindow"),
		"DSP peak find control",
		NULL,
		NULL,
		NULL,
		DSPPeakFind_query,
		DSPPeakFind_about,
		NULL,
		NULL,
		DSPPeakFind_cleanup
};

static const char *about_text = N_("Gxsm DSPPeakFind Plugin:\n"
								   "This plugin runs a controls window to send "
								   "general \"PeakFind-Requests\" to the DSP. "
								   "PeakFindmodes are used to follow and monitor"
								   "Peakintensities by SPA-LEED."
		);

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
		DSPPeakFind_pi.description = g_strdup_printf(N_("Gxsm DSPPeakFind plugin %s"), VERSION);
		return &DSPPeakFind_pi; 
}

DSPPeakFindControl *DSPPeakFindClass = NULL;

GSList *DSPPF_RemoteActionList = NULL;
// of remote_action_cb->{cmd, RemoteCb, widget, data}


static void DSPPeakFind_query(void)
{
		static GnomeUIInfo menuinfo[] = { 
				{ GNOME_APP_UI_ITEM, 
				  DSPPeakFind_pi.menuentry, DSPPeakFind_pi.help,
				  (gpointer) DSPPeakFind_show_callback, NULL,
				  NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK, 
				  0, GDK_CONTROL_MASK, NULL },

				GNOMEUIINFO_END
		};

		PI_DEBUG (DBG_L2, "Query: Insert Menu Entry" );

		gnome_app_insert_menus(GNOME_APP(DSPPeakFind_pi.app->getApp()), 
							   DSPPeakFind_pi.menupath, menuinfo);

		// new ...
  
		PI_DEBUG (DBG_L2, "Query: Create PF Control Class " );

		DSPPeakFindClass = new DSPPeakFindControl
				( DSPPeakFind_pi.app->xsm->hardware,
				  & DSPPeakFind_pi.app->RemoteEntryList
						);

		DSPPeakFindClass->SetResName ("WindowPeakFindControl", "false", xsmres.geomsave);

		//  DSPPeakFind_pi.app->ConnectPluginToStartScanEvent
		//    ( DSPPeakFind_StartScan_callback );

		if(DSPPeakFind_pi.status) g_free(DSPPeakFind_pi.status); 
		DSPPeakFind_pi.status = g_strconcat(N_("Plugin query has attached "),
											DSPPeakFind_pi.name, 
											N_(": DSPPeakFind is created."),
											NULL);
}

static void DSPPeakFind_about(void)
{
		const gchar *authors[] = { "Percy Zahl", NULL};
		gtk_show_about_dialog (NULL, "program-name",  DSPPeakFind_pi.name,
										"version", VERSION,
										  "license", GTK_LICENSE_GPL_3_0,
										  "comments", about_text,
										  "authors", authors,
										  NULL
								));
}

// of remote_action_cb->{cmd, RemoteCb, widget, data}
static void remove_from_gapp_ralist(remote_action_cb* ra, gpointer data){
		PI_DEBUG (DBG_L2, "Remove_Remote List" );
		main_get_gapp()->RemoteActionList = g_slist_remove 
				(main_get_gapp()->RemoteActionList, ra);
		g_free(ra->cmd);
		g_free(ra);
}

static void DSPPeakFind_cleanup( void ){
		PI_DEBUG (DBG_L2, "Cleanup: removing MenuEntry" );

		gchar *mp = g_strconcat(DSPPeakFind_pi.menupath, DSPPeakFind_pi.menuentry, NULL);
		gnome_app_remove_menus (GNOME_APP( DSPPeakFind_pi.app->getApp() ), mp, 1);
		g_free(mp);

		PI_DEBUG (DBG_L2, "DSPPeakFind Plugin Cleanup: Window" );

		// cleanup remote list entries from gapp

		if(DSPPF_RemoteActionList){
				g_slist_foreach(DSPPF_RemoteActionList, (GFunc) remove_from_gapp_ralist, NULL);
				g_slist_free(DSPPF_RemoteActionList);
		}

		//  delete ...
		if( DSPPeakFindClass )
				delete DSPPeakFindClass ;

		DSPPeakFindClass = NULL;
		PI_DEBUG (DBG_L2, "DSPPeakFind Plugin Cleanup: done." );
}

static void DSPPeakFind_show_callback( GtkWidget* widget, void* data){
		if( DSPPeakFindClass )
				DSPPeakFindClass->show();
}
