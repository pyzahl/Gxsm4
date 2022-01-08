/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: repair_cs.C
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
% PlugInDocuCaption: Repair filter
% PlugInName: repair_cs
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Filter 1D/Repair

% PlugInDescription
This filter is obsolete for all Gxsm only users. It just fixes a
high/low words order bug -- occured while data transport from DSP to
host -- in line data, e.g. data points on columns $X_2n$ and
$X_{2n+1}$ are swapped, which happened in one special old version of
Xxsm (the predictor of Gxsm). It's here, because the original old data
are backed up and may be needed to be processed with this filter to
fix.

% PlugInUsage
Call \GxsmMenu{Math/Filter 1D/Repair} to execute.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInDest
The computation result is placed into an existing math channel, else into a new created math channel.

%% OptPlugInNotes
%If you have any additional notes

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"

static void repair_cs_init( void );
static void repair_cs_about( void );
static void repair_cs_configure( void );
static void repair_cs_cleanup( void );
static gboolean repair_cs_run( Scan *Src, Scan *Dest );

GxsmPlugin repair_cs_pi = {
  NULL,
  NULL,
  0,
  NULL,
  "Repair_Cs-M1S-F1D",
  "+noHARD +STM",
  NULL,
  "Percy Zahl",
  "math-filter1d-section",
  N_("Repair"),
  N_("Fix buggy Scans (from special xxsm version): do column swap"),
  "no more info",
  NULL,
  NULL,
  repair_cs_init,
  NULL,
  repair_cs_about,
  repair_cs_configure,
  NULL,
  NULL,
  NULL,
  repair_cs_cleanup
};

GxsmMathOneSrcPlugin repair_cs_m1s_pi = {
  repair_cs_run
};

static const char *about_text = N_("Gxsm Repair_Cs Plugin\n\n"
                                   "Bug Fix for some old scans (Xxsm).");

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  repair_cs_pi.description = g_strdup_printf(N_("Gxsm MathOneArg repair_cs plugin %s"), VERSION);
  return &repair_cs_pi; 
}

GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) { 
  return &repair_cs_m1s_pi; 
}

static void repair_cs_init(void)
{
  PI_DEBUG (DBG_L2, "Repair_Cs Plugin Init");
}

static void repair_cs_about(void)
{
  const gchar *authors[] = { repair_cs_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  repair_cs_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

static void repair_cs_configure(void)
{
  if(repair_cs_pi.app)
    repair_cs_pi.app->message("Repair_Cs Plugin Configuration");
}

static void repair_cs_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Repair_Cs Plugin Cleanup");
}

static gboolean repair_cs_run(Scan *Src, Scan *Dest)
{
// Column Swap
// (was used to fix a bug in some early version of the DSP software)
  int col, line;

  for(line = 0; line < Src->mem2d->GetNy(); line++)
    for(col = 0; col < Src->mem2d->GetNx(); col++)
      Dest->mem2d->PutDataPkt(Src->mem2d->GetDataPkt(col^1, line), col, line);
  return MATH_OK;
}
