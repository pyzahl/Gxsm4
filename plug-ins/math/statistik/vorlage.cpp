/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Quick nine points Gxsm Plugin GUIDE by PZ:
 * ------------------------------------------------------------
 * 1.) Make a copy of this "vorlage.C" to "your_plugins_name.C"!
 * 2.) Replace all "vorlage" by "your_plugins_name" 
 *     --> please do a search and replace starting here NOW!! (Emacs doese preserve caps!)
 * 3.) Decide: One or Two Source Math: 
 *             search for "#define GXSM_ONE_SRC_PLUGIN__DEF" to locate position!
 * 4.) Fill in GxsmPlugin Structure, see below
 * 5.) Replace the "about_text" below a desired
 * 6.) * Optional: Start your Code/Vars definition below (if needed more than the run-fkt itself!), 
 *       Search for "6.)" to find the location please, and see comments there!!
 * 7.) Fill in math code in vorlage_run(), 
 *     have a look at the Data-Access methods infos at end
 * 8.) Add vorlage.C to the Makefile.am in analogy to others
 * 9.) Make a "make; make install"
 * A.) Call Gxsm->Tools->reload Plugins, be happy!
 * B.) Have a look at the PlugIn Documentation section starting at line 50
 *     and please fill out this section to provide a proper documentation.
 *     -> rebuild the Gxsm manual in Gxsm/Docs/Manual:
 *        run "./docuscangxsmplugins.pl; latex Gxsm-main" there!
 * ... That's it!
 * 
 * Gxsm Plugin Name: vorlage.C
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
% PlugInDocuCaption: Vorlage (Template) PlugIn
%%  * please replace the DocuCaption "Vorlage..." 
%%  * with a intuitive and short caption!
%%  * please also replace the entries below!
% PlugInName: vorlage
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Misc/Vorlage

\label{Gxsm-PlugIn-Vorlage}

% PlugInDescription
%% * replace this section by your PlugIn description!
 This is a ``Vorlage'' (German for template) PlugIn. It's purpose is to be a
 template for easy start with writing a new PlugIn and for
 demonstration how a simple PlugIn works as well. The code is
 extensively commented and it includes a nine step instructions list
 for staring your new PlugIn.

% PlugInUsage
%% * Write how to use it.
 This PlugIn is not build and loaded, because it is not
 listed in \GxsmFile{Makefile.am}.

%% OptPlugInSection: replace this by the section caption
%all following lines until next tag are going into this section
%...

%% OptPlugInSubSection: replace this line by the subsection caption
%all following lines until next tag are going into this subsection
%...

% OptPlugInSubSection: Building a new PlugIn in nine steps
 \begin{enumerate}
 \item Make a copy of this \GxsmFile{vorlage.C} to \GxsmFile{your\_plugins\_name.C}!
 \item Replace all ``vorlage'' by ``your\_plugins\_name''\\
   $\longrightarrow$ please do a search and replace starting here (top of file) NOW!! (Emacs doese preserve caps!)
 \item Decide: One or Two Source Math:\\ search for ``\#define GXSM\_ONE\_SRC\_PLUGIN\_\_DEF''
 \item Fill in GxsmPlugin Structure, see below
 \item Replace the ``about\_text'' below a desired
 \item Optional: Start your Code/Vars definition below (if needed more than the run-fkt itself!),
   search for ``6.)''. please, and see comment there!!
 \item Fill in math code in vorlage\_run(), have a look at the Data-Access methods infos at end
 \item Add \GxsmFile{vorlage.C} to the Makefile.am in analogy to others
 \item Make a ``make; make install''
 \item[A.] Call \GxsmMenu{Tools/reload Plugins}, be happy!
 \item[B.] Have a look at the PlugIn Documentation section starting at the beginning 
   (this is, what you are reading here!) and please fill out this section to provide a proper documentation.\\
   $\longrightarrow$ rebuild the Gxsm manual in Gxsm/Docs/Manual:\\
   run \GxsmFile{./docuscangxsmplugins.pl; latex Gxsm-main} there!
 \item[\dots] That's it!
 \end{enumerate}

%% you can repeat OptPlugIn(Sub)Sections multiple times!

% OptPlugInSources
The active channel is used as data source.

% OptPlugInObjects
A optional rectangle is used for data extraction\dots

% OptPlugInDest
The computation result is placed into an existing math channel, else into a new created math channel.

% OptPlugInConfig
Describe the configuration options of your plug in here!

% OptPlugInFiles
This PlugIn is located here: \GxsmFile{Gxsm/plug-ins/math/statistik/vorlage.C}\\
If your PlugIn uses, needs, creates any files, then put the info here!

% OptPlugInRefs
Any references about algorithm sources, etc.?

% OptPlugInKnownBugs
Are there known bugs? List! How to work around if not fixed?

% OptPlugInNotes
If you have any additional notes, place them here!

% OptPlugInHints
Any hints, tips or tricks? Yeah!\\
Check out the more automatic math PlugIn building script:
Go to dir \GxsmFile{Gxsm/plug-ins} and run \GxsmFile{generate\_math\_plugin.sh} there!\\
And never mind, use any existing PlugIn as template as well, but
please please copy and rename it properly before!

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

// Plugin Prototypes
static void vorlage_init( void );
static void vorlage_about( void );
static void vorlage_configure( void );
static void vorlage_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean vorlage_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean vorlage_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin vorlage_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "Vorlage-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-F1D",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  " no description, fill in here!",                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-filter1d-section",
  // Menuentry
  N_("Vorlage"),
  // help text shown on menu
  N_("Sorry, no help for vorlage filter!"),
  // more info...
  "no more info",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  vorlage_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  vorlage_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  vorlage_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  vorlage_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin vorlage_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin vorlage_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin vorlage_m1s_pi
#else
 GxsmMathTwoSrcPlugin vorlage_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   vorlage_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm Vorlage Plugin\n\n"
                                   "Vorlage Code for OneSrcArgMath");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  vorlage_pi.description = g_strdup_printf(N_("Gxsm MathOneArg vorlage plugin %s"), VERSION);
  return &vorlage_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!

// available PlugIn flavors:
// --------------------------------------------------------------------------------
// GxsmMathOneSrcNoDestPlugin *get_gxsm_math_one_src_no_dest_plugin_info( void ) {}
// GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {}
// GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) {}
// GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_no_same_size_check_plugin_info( void ) {}

#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &vorlage_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &vorlage_m2s_pi; 
}
#endif

// 6.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//


// init-Function
static void vorlage_init(void)
{
  PI_DEBUG (DBG_L2, "Vorlage Plugin Init");
}

// about-Function
static void vorlage_about(void)
{
  const gchar *authors[] = { vorlage_pi.authors, NULL};
  gtk_show_about_dialog (NULL, "program-name",  vorlage_pi.name,
                                  "version", VERSION,
                                    "license", GTK_LICENSE_GPL_3_0,
				    "comments", about_text,
				    "authors", authors,
				    NULL
                                    ));
}

// configure-Function
static void vorlage_configure(void)
{
  if(vorlage_pi.app)
    vorlage_pi.app->message("Vorlage Plugin Configuration");
}

// cleanup-Function
static void vorlage_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Vorlage Plugin Cleanup");
}

// run-Function
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 static gboolean vorlage_run(Scan *Src, Scan *Dest)
#else
 static gboolean vorlage_run(Scan *Src1, Scan *Src2, Scan *Dest)
#endif
{
  // put plugins math code here...

  return MATH_OK;
}

/*
// Quick overview of the mem2d class, look also at mem2d.h!
// ------------------------------------------------------------
//
// Please read instructions
// ==============================
//
// Basic Data access:
//==============================
// Scan *Src:  Data source     bzw. *Src1, *Scr2 in TwoSrc Mode
//             the original data
// Scan *Dest: destination, that is where the result of the filter
//             operation goes.
//
// ===============================
// *Dest is a new, empty scan with the same parameters as *Src (e.g. size).
// !!!!!!  *Src should NEVER be modified !!!!!!
//
// Size (number of data points):
// ===============================
// int nx = Dest->mem2d->GetNx(), ... GetNy()
//
// use the following function to resize *Dest.
// Dest->mem2dResize(Dest->data.s.nx, Dest->data.s.ny)
//
// Data access using the Mem2d Object:
// ===============================
// * to copy a rectangular area (type Src == type Dest):
// Dest->mem2d->CopyFrom(Src->mem2d, int x0, int y0, int tox, int toy, Dest->mem2d->GetNx(),Dest->mem2d->GetNy());
// * to copy a rectangular area with tpe conversion:
// Dest->mem2d->ConvertFrom(Src->mem2d, int x0, int y0, int tox, int toy, Dest->mem2d->GetNx(),Dest->mem2d->GetNy());
//
//
// [[ Src->mem2d->GetDataLine(int line, ZTYPE *buffer)   :    complete line ]] please do not use !!
// double value = Src->mem2d->GetDataPkt(int x, int y) :    one data point
// ... or (slightly faster because of direct access)
// double value = Src->mem2d->data->Z(int x, int y) : one point
// value = Src->mem2d->GetDataPkt(int x, int y) :    one point
// value = Src->mem2d->GetDataPktLineReg(int x, int y) :  one point but with linear data regession !!
//
// to write in *Dest :
// [[ Dest->mem2d->PutDataLine(int line, ZTYPE *buffer)  :    complete line ]] please do not use !!
// Dest->mem2d->PutDataPkt(ZTYPE value, int x, int y) :    one point
// ... or (faster, w/o invalidate lineregress parameters)
// Src->mem2d->data->Z(double value, int x, int y) :   one point
//
// for large Data Transfer use:
// ============================================================
// for same types:
// inline int CopyFrom(Mem2d *src, int x, int y, int tox, int toy, int nx, int ny=1)
//
// for different types:
// inline int ConvertFrom(Mem2d *src, int x, int y, int tox, int toy, int nx, int ny=1)
//
// for quick linear data access of elements in one line use:
// ============================================================
// void   mem2d->data->SetPtr(int x, int y) to set internal pointer to x,y  -- no range check !!!
// double mem2d->data->GetNext() to access Element x,y and point to next one in line y -- no range check !!!
// double mem2d->data->SetNext(double z) to access Element x,y and point to next one in line y -- no range check !!!
//
// access of surrounding data point there are access methods too: 
// to set internal ref. pointer:
// SetPtrTB(i,j)
// to access points data:
// GetThis(), GetThisLT, GetThisT, GetThisRT, GetThisR, GetThisRB, GetThisB, GetThisLB, GetThisL
// This = (i,j), ThisLT = (i-1,j-1) (LT=Left-Top), ThisT (T=Top), B=Bottom, R=Right
// increment pointer (next x):
// IncPtrTB()
//
// For Example use constructs like: (have a look at smallconvol.C plugin!)

  int line;
  ZData  *SrcZ, *DestZ;

  SrcZ  =  Src->mem2d->data;
  DestZ = Dest->mem2d->data;

  for ( line=0; line < Dest->data.s.ny; line++){ 
    DestZ->SetPtr(0, line);
    SrcZ ->SetPtr(0, line);
    DestZ->SetNext(SrcZ->GetNext() + ....));
  }

// primitive example of Two-Src adding: (Dest = Scr1 + Src2)

  int line, col;

  if(Src1->data.s.nx != Src2->data.s.nx || Src1->data.s.ny != Src2->data.s.ny)
    return MATH_SELECTIONERR;

  for(line=0; line<Dest->data.s.ny; line++)
    for(col=0; col<Dest->data.s.nx; col++)
      Dest->mem2d->PutDataPkt(
                              Src1->mem2d->GetDataPkt(col, line)
                            + Src2->mem2d->GetDataPkt(col, line),
                              col, line);

// more examples in other plugins and xsmmath.C
*/

