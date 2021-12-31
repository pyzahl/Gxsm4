/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: queryDSPinfo.C
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
% PlugInName: queryDSPinfo
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Tools/Hardware Info

% PlugInDescription
\label{plugin:queryDSPinfo} 

This plugin asks the DSP, e.g the connected hardware or driver/module,
to report informations about the current running software. This is
used by Gxsm for automatic configuration of several features. -- This
new feature will be expanded and used more intens future versions.

% PlugInUsage
Call it using the menu \GxsmMenu{Tools/Hardware Info}.

% OptPlugInNotes
This works only with the second DSP software generation, starting with xsm CVS V1.20!
And it works with the kernel DSP emulation modules!

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

// ========================================
// Gxsm PlugIn Setup Part
// ========================================

static void queryDSPinfo_about( void );
static void queryDSPinfo_run(GtkWidget *w, void *data);
static void queryDSPinfo_cleanup( void );

GxsmPlugin queryDSPinfo_pi = {
	NULL,
	NULL,
	0,
	NULL,
	"queryDSPinfo",
	NULL,
	NULL,
	"Percy Zahl",
	"tools-section",
	N_("Hardware Info"),
	N_("Ask the DSP/hardware driver to report informations"),
	"no more info",
	NULL,
	NULL,
	NULL,
	NULL,
	queryDSPinfo_about,
	NULL,
	queryDSPinfo_run,
	NULL,
	NULL,
	queryDSPinfo_cleanup
};

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	queryDSPinfo_pi.description = g_strdup_printf(N_("queryDSPinfo Plugin %s"), VERSION);
	return &queryDSPinfo_pi; 
}

static void queryDSPinfo_about(void){
	const gchar *authors[] = { "Percy Zahl", NULL};
	gtk_show_about_dialog (NULL,
			       "program-name", queryDSPinfo_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "This PlugIn asks the Hardware/DSP to report "
			       "the current software version, features and configuration.",
			       "authors", authors,
			       NULL
			       );
}

static void queryDSPinfo_run(GtkWidget *w, void *data){
  gapp->message (queryDSPinfo_pi.app->xsm->hardware->get_info ());
}

static void queryDSPinfo_cleanup( void ){
}

