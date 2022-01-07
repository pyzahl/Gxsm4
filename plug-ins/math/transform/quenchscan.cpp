/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=quenchscan
pluginmenuentry 	=Quench Scan
menupath		=Math/Transformations/
entryplace		=Transformations
shortentryplace	=TR
abouttext		=Down size scan by factor of two, averages 2x2 points for resulting pixel.
smallhelp		=Half size of scan.
longhelp		=This is a detailed help for my Plugin.
 * 
 * Gxsm Plugin Name: quenchscan.C
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
% PlugInDocuCaption: Quench Scan
% PlugInName: quenchscan
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Transformations/Quench Scan

% PlugInDescription
This filter quenches the scan to half size, therefor 2x2 pixels are
averaged.

% PlugInUsage
Call \GxsmMenu{Math/Transformation/Quench Scan}.

% OptPlugInSources
The active channel is used as data source.

%% OptPlugInObjects

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

// Plugin Prototypes
static void quenchscan_init( void );
static void quenchscan_about( void );
static void quenchscan_configure( void );
static void quenchscan_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
static gboolean quenchscan_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
static gboolean quenchscan_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin quenchscan_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	g_strdup ("quenchscan-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
	"M1S"
#else
	"M2S"
#endif
		  "-TR"),
	// Plugin's Category - used to autodecide on Pluginloading or ignoring
	// NULL: load, else
	// example: "+noHARD +STM +AFM"
	// load only, if "+noHARD: no hardware" and Instrument is STM or AFM
	// +/-xxxHARD und (+/-INST or ...)
	NULL,
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	g_strdup("This is a detailed help for my Plugin."),                   
	// Author(s)
	"Percy Zahl",
	// Menupath to position where it is appendet to
	"math-transformations-section",
	// Menuentry
	N_("Quench Scan"),
	// help text shown on menu
	N_("This is a detailed help for my Plugin."),
	// more info...
	"Half size of scan.",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	quenchscan_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	quenchscan_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	quenchscan_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

	quenchscan_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin quenchscan_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin quenchscan_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin quenchscan_m1s_pi
#else
GxsmMathTwoSrcPlugin quenchscan_m2s_pi
#endif
= {
	// math-function to run, see prototype(s) above!!
	quenchscan_run
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm quenchscan Plugin\n\n"
                                   "Down size scan by factor of two, averages 2x2 points for resulting pixel.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	quenchscan_pi.description = g_strdup_printf(N_("Gxsm MathOneArg quenchscan plugin %s"), VERSION);
	return &quenchscan_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_for_all_vt_plugin_info( void ) {
	return &quenchscan_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
	return &quenchscan_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void quenchscan_init(void)
{
	PI_DEBUG (DBG_L2, "quenchscan Plugin Init");
}

// about-Function
static void quenchscan_about(void)
{
	const gchar *authors[] = { quenchscan_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  quenchscan_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void quenchscan_configure(void)
{
	if(quenchscan_pi.app)
		quenchscan_pi.app->message("quenchscan Plugin Configuration");
}

// cleanup-Function
static void quenchscan_cleanup(void)
{
	PI_DEBUG (DBG_L2, "quenchscan Plugin Cleanup");
}

// run-Function
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
static gboolean quenchscan_run(Scan *Src, Scan *Dest)
#else
	static gboolean quenchscan_run(Scan *Src1, Scan *Src2, Scan *Dest)
#endif
{
	long col, line;
	ZData  *SrcZ, *SrcZn, *DestZ;
	double val;
	PI_DEBUG (DBG_L2, "Quench Scan");

	if (!Src || !Dest)
			return 0;

	SrcZ  =  Src->mem2d->data;

	DestZ = Dest->mem2d->data;

	Dest->data.s.nx = (Src->data.s.nx/2) & ~1;
	Dest->data.s.dx = Src->data.s.dx*2.;
	Dest->data.s.ny = (Src->data.s.ny/2) & ~1;
	Dest->data.s.nvalues = Src->mem2d->GetNv ();
	Dest->data.s.dy = Src->data.s.dy*2.;
	Dest->mem2d->Resize(Dest->data.s.nx, Dest->data.s.ny, Src->mem2d->GetNv ());

	Mem2d NextLine(Src->mem2d->GetNx(), 1, Src->mem2d->GetTyp());
	SrcZn = NextLine.data;
	for(int v = 0; v < Dest->data.s.nvalues; v++){
                Dest->mem2d->SetLayer(v);
                Src->mem2d->SetLayer(v);
                for(line = 0; line < Dest->data.s.ny; line++){
                        NextLine.CopyFrom(Src->mem2d, 0,2*line+1, 0,0, Src->mem2d->GetNx());
                        DestZ->SetPtr(0, line);
                        SrcZ ->SetPtr(0, 2*line);
                        SrcZn->SetPtr(0, 0);
                        for(col = 0; col < Dest->data.s.nx; col++){
                                val  = SrcZ ->GetNext();
                                val += SrcZ ->GetNext();
                                val += SrcZn->GetNext();
                                val += SrcZn->GetNext();
                                val /= 4.;
                                DestZ->SetNext(val);
                        }
                }
                /*
                if (Src->mem2d->layer_information_list[v]){
                        int nj = g_list_length (m->layer_information_list[v]);
                        for (int j = 0; j<nj; ++j){
                                LayerInformation *li = (LayerInformation*) g_list_nth_data (Src->mem2d->layer_information_list[layer_index], j);
                                if (li)
                                        Dest->mem2d->add_layer_information (v, new LayerInformation (li));
                        }
                }
                */

	}
        Dest->mem2d->data->CopyLookups (Src->mem2d->data);
        Dest->mem2d->copy_layer_information (Src->mem2d);
	return MATH_OK;
}


