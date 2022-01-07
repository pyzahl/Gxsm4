/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: lineinterpol.C
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
% PlugInDocuCaption: Lineinterpol
% PlugInName: lineinterpol
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Filter 2D/Lineinterpol

% PlugInDescription
Lineinterpol action.

% PlugInUsage
Call \GxsmMenu{Math/Filter 2D/Lineinterpol}.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

// Plugin Prototypes
static void lineinterpol_init( void );
static void lineinterpol_about( void );
static void lineinterpol_configure( void );
static void lineinterpol_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean lineinterpol_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean lineinterpol_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin lineinterpol_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "lineinterpol-"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
  "M1S"
#else
  "M2S"
#endif
  "-BG",
  // Plugin's Category - used to autodecide on Pluginloading or ignoring
  // NULL: load, else
  // example: "+noHARD +STM +AFM"
  // load only, if "+noHARD: no hardware" and Instrument is STM or AFM
  // +/-xxxHARD und (+/-INST or ...)
  NULL,
  // Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
  g_strdup("Lineinterpol math filter."),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-filter2d-section",
  // Menuentry
  N_("Lineinterpol"),
  // help text shown on menu
  N_("Lineinterpol filter action on data."),
  // more info...
  "Lineinterpol does math with data.",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  lineinterpol_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  lineinterpol_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  lineinterpol_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  lineinterpol_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin lineinterpol_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin lineinterpol_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin lineinterpol_m1s_pi
#else
 GxsmMathTwoSrcPlugin lineinterpol_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   lineinterpol_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm Lineinterpol Plugin\n\n"
                                   "This Plugin does a lineinterpol action with data:\n"
				   "..."
	);

double ConstLast = 1000.;
double constval = 1000.;

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  lineinterpol_pi.description = g_strdup_printf(N_("Gxsm MathOneArg lineinterpol plugin %s"), VERSION);
  return &lineinterpol_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &lineinterpol_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &lineinterpol_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void lineinterpol_init(void)
{
  PI_DEBUG (DBG_L2, "lineinterpol Plugin Init");
}

// about-Function
static void lineinterpol_about(void)
{
  const gchar *authors[] = { lineinterpol_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  lineinterpol_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

// configure-Function
static void lineinterpol_configure(void)
{
  if(lineinterpol_pi.app)
    lineinterpol_pi.app->message("Lineinterpol Plugin Configuration");
}

// cleanup-Function
static void lineinterpol_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Lineinterpol Plugin Cleanup");
}

double TransformFkt (double val, double range){
	return val;
}

// run-Function
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 static gboolean lineinterpol_run(Scan *Src, Scan *Dest)
#else
 static gboolean lineinterpol_run(Scan *Src1, Scan *Src2, Scan *Dest)
#endif
{
	int line, col;
	constval = ConstLast;
	
        MOUSERECT msr;
	
        MkMausSelect (Src, &msr, Dest->mem2d->GetNx(), Dest->mem2d->GetNy());
	
        if( msr.xSize  < 1)
                return MATH_SELECTIONERR;
        
        lineinterpol_pi.app->ValueRequest 
                ( "Enter threahold", "threashold", 
                  "Need Input",
                  lineinterpol_pi.app->xsm->Unity, 
                  -1e100, 1e100, "8g", &constval
                  );
	ConstLast = constval;

        double threashold = constval;

	int ti=0; 
	int tf=0;
	int vi=0;
	int vf=0;
	gboolean multidim = FALSE;
	
	if (Src->data.s.ntimes != 1 || Src->mem2d->GetNv () != 1){
		multidim = TRUE;
		do {
			ti=vi=0;
			tf=Src->number_of_time_elements ()-1;
			if (tf < 0) tf = 0;
			vf=Src->mem2d->GetNv ()-1;
			main_get_gapp()->setup_multidimensional_data_copy ("Multidimensional Lineinterpol", Src, ti, tf, vi, vf);
		} while (ti > tf || vi > vf);

		main_get_gapp()->progress_info_new ("Multidimenssional Lineinterpol", 2);
		main_get_gapp()->progress_info_set_bar_fraction (0., 1);
		main_get_gapp()->progress_info_set_bar_fraction (0., 2);
		main_get_gapp()->progress_info_set_bar_text ("Time", 1);
		main_get_gapp()->progress_info_set_bar_text ("Value", 2);
	}

	int ntimes_tmp = tf-ti+1;
	for (int time_index=ti; time_index <= tf; ++time_index){
		Mem2d *m = Src->mem2d_time_element (time_index);
		if (multidim)
			main_get_gapp()->progress_info_set_bar_fraction ((gdouble)(time_index-ti)/(gdouble)ntimes_tmp, 1);

		Dest->mem2d->Resize (m->GetNx (), m->GetNy (), vf-vi+1, m->GetTyp());

		for (int v_index = vi; v_index <= vf; ++v_index){
			m->SetLayer (v_index);
			Dest->mem2d->SetLayer (v_index-vi);

                        //			m->HiLo(&hi, &lo, FALSE);

	
        
                        // copy scan data:
                        Dest->mem2d->CopyFrom(Src->mem2d, 0,0, 0,0, Dest->mem2d->GetNx(),Dest->mem2d->GetNy());
	
                        if (fabs(msr.yBottom - msr.yTop) > 3.){
                                main_get_gapp()->ValueRequest("Enter Value", "Z threshold (DAC units)", 
                                                   "max allowed Z variation withing marked width",
                                                   main_get_gapp()->xsm->Unity, 
                                                   0., 10000., ".0f", &threashold);
                        }
	
                        // interpolation if in between first and last line
                        if(msr.yTop > 0){
                                if(msr.yTop < (Src->mem2d->GetNy()-2)){
                                        if (fabs(msr.yBottom - msr.yTop) > 3.){
                                                double zprev = 0.;
                                                for (int line = msr.yTop; line < msr.yBottom; ++line){
                                                        double z = 0., num = 0.;
                                                        for (int row = msr.xLeft; row <= msr.xRight; ++row){
                                                                num += 1.;
                                                                z += Src->mem2d->GetDataPkt (row, line);
                                                        }
                                                        z /= num;
                                                        if (line == msr.yTop || fabs (z-zprev) < threashold){
                                                                zprev = z;
                                                                continue;
                                                        }
                                                        std::cout << "Fixing jump (diff is="<< (z-zprev) <<") at line: " << line << std::endl;
					
                                                        // create help Mem2d object (only one line)
                                                        Mem2d PreLine(Src->mem2d->GetNx(), 1, Src->mem2d->GetTyp());
                                                        // with "PreLine" and set internal Ptr to start
                                                        PreLine.CopyFrom (Src->mem2d, 0,line-1, 0,0, Src->mem2d->GetNx());
                                                        PreLine.data->SetPtr (0,0);
                                                        // set Src Ptr to "PostLine"
                                                        Src->mem2d->data->SetPtr (0,line+1);
                                                        // set Dest Ptr to "Line"
                                                        Dest->mem2d->data->SetPtr (0,line);
                                                        for(int col = 0; col < Src->mem2d->GetNx (); col++)
                                                                Dest->mem2d->data->SetNext ((PreLine.data->GetNext()+Src->mem2d->data->GetNext())/2.);
                                                }
                                        } else {
                                                // create help Mem2d object (only one line)
                                                Mem2d PreLine(Src->mem2d->GetNx(), 1, Src->mem2d->GetTyp());
                                                // with "PreLine" and set internal Ptr to start
                                                PreLine.CopyFrom(Src->mem2d, 0,msr.yTop-1, 0,0, Src->mem2d->GetNx());
                                                PreLine.data->SetPtr(0,0);
                                                // set Src Ptr to "PostLine"
                                                Src->mem2d->data->SetPtr(0,msr.yTop+1);
                                                // set Dest Ptr to "Line"
                                                Dest->mem2d->data->SetPtr(0,msr.yTop);
                                                for(int col = 0; col < Src->mem2d->GetNx(); col++)
                                                        Dest->mem2d->data->SetNext((PreLine.data->GetNext()+Src->mem2d->data->GetNext())/2.);
                                        }
                                }
                                else  // last line: copy previous line
                                        Dest->mem2d->CopyFrom(Src->mem2d, 0,msr.yTop-1, 0,msr.yTop, Src->mem2d->GetNx());
                        }
                        else  // first line: copy 2nd line
                                Dest->mem2d->CopyFrom(Src->mem2d, 0,msr.yTop+1, 0,msr.yTop, Src->mem2d->GetNx());
	

#if 0                
			for (line=0; line<Dest->mem2d->GetNy (); line++)
				for (col=0; col<Dest->mem2d->GetNx (); col++)
					Dest->mem2d->PutDataPkt 
					  (TransformFkt (m->GetDataPkt (col, line)-lo, hi-lo), 
					   col, line);
#endif
		}
		Dest->append_current_to_time_elements (time_index-ti, m->get_frame_time ());
	}

	Dest->data.s.ntimes = ntimes_tmp;
	Dest->data.s.nvalues=Dest->mem2d->GetNv ();

	if (multidim){
		main_get_gapp()->progress_info_close ();
		Dest->retrieve_time_element (0);
		Dest->mem2d->SetLayer(0);
	}
	
	return MATH_OK;
}

