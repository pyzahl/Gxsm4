/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=pzahl
email	        	=zahl@gxsm.sf.net
pluginname		=average_profile
pluginmenuentry 	=Average X Profile
menupath		=math-statistics-section
entryplace		=_Statistics
shortentryplace	=ST
abouttext		=Compute the average X profile
smallhelp		=computes the average X profile
longhelp		=This Plugin computes the average X profile for all scanlines.
 * 
 * Gxsm Plugin Name: average_profile.C
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
 * All "% OptPlugInXXX" tags are optional and can be removed or commented in
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Average X Profile
% PlugInName: average_profile
% PlugInAuthor: P. Zahl
% PlugInAuthorEmail: zahl@gxsm.sf.net
% PlugInMenuPath: Math/Statistics/Average X Profile

% PlugInDescription
Compute the average X profile of all scanlines.

%% PlugInUsage


%% OptPlugInSection: replace this by the section caption
%all following lines until next tag are going into this section
%...

%% OptPlugInSubSection: replace this line by the subsection caption
%all following lines until next tag are going into this subsection
%...

%% you can repeat OptPlugIn(Sub)Sections multiple times!

%% OptPlugInSources
%The active channel is used as data source.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

%% OptPlugInDest
%The computation result is placed into an existing math channel, else into a new created math channel.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

%% OptPlugInRefs
%Any references?

%% OptPlugInKnownBugs
%Are there known bugs? List! How to work around if not fixed?

%% OptPlugInNotes
%If you have any additional notes

%% OptPlugInHints
%Any tips and tricks?

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"
#include "core-source/action_id.h"
#include "core-source/app_profile.h"

// Plugin Prototypes
static void average_profile_init( void );
static void average_profile_about( void );
static void average_profile_configure( void );
static void average_profile_cleanup( void );

// Math-Run-Function, use only one of (automatically done :=)
 static gboolean average_profile_run( Scan *Src);

// Fill in the GxsmPlugin Description here
GxsmPlugin average_profile_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  g_strdup ("average_profile-"
	    "M1S-ST"),
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup ("This Plugin computes the average X profile for all scanlines."),                   
  // Author(s)
  "pzahl",
  // Menupath to position where it is appendet to
  "math-statistics-section",
  // Menuentry
  N_("Average X Profile"),
  // help text shown on menu
  N_("This Plugin computes the average X profile for all scanlines."),
  // more info...
  "computes the average X profile",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  average_profile_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  average_profile_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  average_profile_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  average_profile_cleanup
};

// special math Plugin-Strucure, use
 GxsmMathOneSrcNoDestPlugin average_profile_m1s_pi
 = {
   // math-function to run, see prototype(s) above!!
   average_profile_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm average_profile Plugin\n\n"
                                   "Compute the average X profile");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  average_profile_pi.description = g_strdup_printf(N_("Gxsm MathOneArg average_profile plugin %s"), VERSION);
  return &average_profile_pi; 
}


GxsmMathOneSrcNoDestPlugin *get_gxsm_math_one_src_no_dest_plugin_info( void ) {
  return &average_profile_m1s_pi; 
}


/* Here we go... */
// init-Function
static void average_profile_init(void)
{
  PI_DEBUG (DBG_L2, "Plugin Init" );
}

// about-Function
static void average_profile_about(void)
{
  const gchar *authors[] = { average_profile_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  average_profile_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void average_profile_configure(void)
{
  if(average_profile_pi.app)
    average_profile_pi.app->message("average_profile Plugin Configuration");
}

// cleanup-Function
static void average_profile_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Plugin Cleanup");
}

// run-Function
 static gboolean average_profile_run(Scan *Src)
{
	gchar *txt = g_strdup_printf ("Average X Profile of %d lines", Src->mem2d->GetNy ());
	ProfileControl *pc = new ProfileControl (txt, 
						 Src->mem2d->GetNx (), 
						 Src->data.Xunit, Src->data.Zunit, 
						 Src->mem2d->data->GetXLookup(0), Src->mem2d->data->GetXLookup( Src->mem2d->GetNx ()-1), "XavProfile");
	g_free (txt);

	for(int i = 0; i < Src->mem2d->GetNx (); i++)
		pc->SetPoint (i, 0.);

	for(int line=0; line < Src->mem2d->GetNy (); line++)
		for(int col=0; col < Src->mem2d->GetNx (); col++)
			pc->AddPoint (col, Src->mem2d->GetDataPkt (col, line));

	for(int col=0; col < Src->mem2d->GetNx (); col++)
		pc->MulPoint (col, 1./Src->mem2d->GetNy ());

	pc->UpdateArea ();

	average_profile_pi.app->xsm->AddProfile (pc); // give it to Surface, it takes care about removing it...
	pc->unref (); // nothing depends on it, so decrement refcount

	return MATH_OK;
}

