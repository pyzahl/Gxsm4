/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@gxsm.sf.net
pluginname		=autoalign
pluginmenuentry 	=Auto Align
menupath		=math-transformations-section
entryplace		=_Transformations
shortentryplace	=TR
abouttext		=Automatic align an image series in time domain (drift correction)
smallhelp		=Automatic align an image series in time domain
longhelp		=Automatic align an image series in time domain (drift correction)
 * 
 * Gxsm Plugin Name: autoalign.C
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

/* The math algorithm is based on the following work, prior implemented for ImageJ: */

/*====================================================================	
| Version: February 28, 2005
\===================================================================*/

/*====================================================================
| EPFL/STI/IOA/LIB
| Philippe Thevenaz
| Bldg. BM-Ecublens 4.137
| Station 17
| CH-1015 Lausanne VD
| Switzerland
|
| phone (CET): +41(21)693.51.61
| fax: +41(21)693.37.01
| RFC-822: philippe.thevenaz@epfl.ch
| X-400: /C=ch/A=400net/P=switch/O=epfl/S=thevenaz/G=philippe/
| URL: http://bigwww.epfl.ch/
\===================================================================*/

/*====================================================================
| This work is based on the following paper:
|
| P. Thevenaz, U.E. Ruttimann, M. Unser
| A Pyramid Approach to Subpixel Registration Based on Intensity
| IEEE Transactions on Image Processing
| vol. 7, no. 1, pp. 27-41, January 1998.
|
| This paper is available on-line at
| http://bigwww.epfl.ch/publications/thevenaz9801.html
|
| Other relevant on-line publications are available at
| http://bigwww.epfl.ch/publications/
\===================================================================*/

/*====================================================================
| Additional help available at http://bigwww.epfl.ch/thevenaz/turboreg/
|
| You'll be free to use this software for research purposes, but you
| should not redistribute it without our consent. In addition, we expect
| you to include a citation or acknowledgment whenever you present or
| publish results that are based on it.
\===================================================================*/


/* Please do not change the Begin/End lines of this comment section!
 * this is a LaTeX style section used for auto generation of the PlugIn Manual 
 * Chapter. Add a complete PlugIn documentation inbetween the Begin/End marks!
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional and can be removed or commented in
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Auto Align Multi Dimensional Movie
% PlugInName: autoalign
% PlugInAuthor: Percy Zahl, P. Thev\'enaz, U.E. Ruttimann, M. Unser
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Transformations/Auto Align

% PlugInDescription
The purpose of this method is to autoalign image series in multiple dimensions, i.e. to correct for image drift.
The GXSM autoalign Plug-In does automatic image alignment for translation,
scaled rotation, ridig body and affine transformation, of an image series
in time and layer domain (i.e. for drift correction).
The underlying algorithm of this code is based on the following paper,
implemented as GXSM-Plugin, ported from JAVA to C++, optimized and multithreaded by P. Zahl.

% PlugInUsage
Activate the movie and run it.

%% OptPlugInSection: replace this by the section caption

%% OptPlugInSubSection: replace this line by the subsection caption

%% you can repeat OptPlugIn(Sub)Sections multiple times!

%% OptPlugInSources

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

%% OptPlugInDest
%The computation result is placed into an existing math channel, else into a new created math channel.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

%% OptPlugInRefs
This work is based on the following paper:

P. Thev\'enaz, U.E. Ruttimann, M. Unser
A Pyramid Approach to Subpixel Registration Based on Intensity
IEEE Transactions on Image Processing
vol. 7, no. 1, pp. 27-41, January 1998.

This paper is available on-line at
http://bigwww.epfl.ch/publications/thevenaz9801.html

Other relevant on-line publications are available at
http://bigwww.epfl.ch/publications/

Additional help available at
http://bigwww.epfl.ch/thevenaz/turboreg/

You'll be free to use this software for research purposes, but
you should not redistribute it without our consent. In addition,
we expect you to include a citation or acknowledgment of both projects whenever
you present or publish results that are based on it.

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
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"


#include "autoalign_stack_reg.h"

// Plugin Prototypes
static void autoalign_init( void );
static void autoalign_about( void );
static void autoalign_configure( void );
static void autoalign_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
static gboolean autoalign_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
static gboolean autoalign_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin autoalign_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	"autoalign-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
	"M1S"
#else
	"M2S"
#endif
	"-TR",
	// Plugin's Category - used to autodecide on Pluginloading or ignoring
	// NULL: load, else
	// example: "+noHARD +STM +AFM"
	// load only, if "+noHARD: no hardware" and Instrument is STM or AFM
	// +/-xxxHARD und (+/-INST or ...)
	NULL,
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	g_strdup ("Automatic align an image series in time domain (drift correction)"),                   
	// Author(s)
	"Percy Zahl, algorithm by P. Th\303\251venaz, U.E. Ruttimann, M. Unser",
	// Menupath to position where it is appendet to
	"math-transformations-section",
	// Menuentry
	N_("Auto Align"),
	// help text shown on menu
	N_("Automatic align an image series in time domain (drift correction)"),
	// more info...
	"Automatic align an image series in time domain",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	autoalign_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	autoalign_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	autoalign_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

	autoalign_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin autoalign_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin autoalign_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin autoalign_m1s_pi
#else
GxsmMathTwoSrcPlugin autoalign_m2s_pi
#endif
= {
	// math-function to run, see prototype(s) above!!
	autoalign_run
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm autoalign Plugin\n\n"
                                   "Automatic align an image series in time&layer domain (drift correction).\n\n"
				   "This underlying algorithm of this work is based on the following paper,\n"
				   "implemented into GXSM-Plugin and ported from JAVA to C++ by P. Zahl:\n"
				   "\n"
				   " P. Th\303\251venaz, U.E. Ruttimann, M. Unser\n"
				   " A Pyramid Approach to Subpixel Registration Based on Intensity\n"
				   " IEEE Transactions on Image Processing\n"
				   " vol. 7, no. 1, pp. 27-41, January 1998.\n"
				   "\n"
				   " This paper is available on-line at\n"
				   " http://bigwww.epfl.ch/publications/thevenaz9801.html\n"
				   "\n"
				   " Other relevant on-line publications are available at\n"
				   " http://bigwww.epfl.ch/publications/\n"
				   "\n"
				   " Additional help available at\n"
				   " http://bigwww.epfl.ch/thevenaz/stackreg/\n"
				   "\n"
				   " Ancillary TurboReg_ plugin available at\n"
				   " http://bigwww.epfl.ch/thevenaz/turboreg/\n"
				   "\n"
				   " You'll be free to use this software for research purposes, but\n"
				   " you should not redistribute it without our consent. In addition,\n"
				   " we expect you to include a citation or acknowledgment whenever\n"
				   " you present or publish results that are based on it.\n"
	);

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	autoalign_pi.description = g_strdup_printf(N_("Gxsm MathOneArg autoalign plugin %s"), VERSION);
	return &autoalign_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
	return &autoalign_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
	return &autoalign_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void autoalign_init(void)
{
	PI_DEBUG (DBG_L2, "Plugin Init" );
}

// about-Function
static void autoalign_about(void)
{
	const gchar *authors[] = { autoalign_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  autoalign_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void autoalign_configure(void)
{
	if(autoalign_pi.app)
		autoalign_pi.app->message("autoalign Plugin Configuration");
}

// cleanup-Function
static void autoalign_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Plugin Cleanup");
}

// run-Function
static gboolean autoalign_run(Scan *Src, Scan *Dest)
{
	StackReg stack_register;

	// put plugins math code here...
	// For more math-access methods have a look at xsmmath.C
	// simple example for 1sourced Mathoperation: Source is taken and 100 added.


	// resize Dest to match Src
	Dest->mem2d->Resize (Src->mem2d->GetNx (), Src->mem2d->GetNy (), Src->mem2d->GetNv (), ZD_IDENT);

	stack_register.run (Src, Dest);

	Dest->data.s.nx = Dest->mem2d->GetNx ();
	Dest->data.s.ny = Dest->mem2d->GetNy ();
	Dest->data.s.nvalues = Dest->mem2d->GetNv ();
	Dest->data.s.ntimes = Dest->number_of_time_elements ();

	return MATH_OK; // done OK.
}


