/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: dummy.C
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
% PlugInDocuCaption: PSD add -- SARLS
% PlugInName: psdadd
% PlugInAuthor: Martin Langer, PZ
% PlugInAuthorEmail: stefan_fkp@users.sf.net
% PlugInMenuPath: Math/Misc/PSD Add

% PlugInDescription
Experimental filter for adding PSD-slope-signals.

% PlugInUsage
It is only available form \GxsmMenu{Math/Misc/PSD Add} if the
instrument type is set to SARLS or a reload of all PlugIns is forced.

% OptPlugInSources
The active and X channel is used as data sources.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

%% OptPlugInDest
%The computation result is placed into an existing math channel, else
%into a new created math channel.

%% OptPlugInRefs
%Any references?

%% OptPlugInKnownBugs
%Are there known bugs? List! How to work around if not fixed?

% OptPlugInNotes
I would appreciate if one of the current SARLS group members could
figure out more about this piece of code.

%% OptPlugInHints
%Any tips and tricks?

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

static void psdadd_init( void );
static void psdadd_about( void );
static void psdadd_configure( void );
static void psdadd_cleanup( void );
static gboolean psdadd_run( Scan *Src1, Scan *Src2, Scan *Dest );

GxsmPlugin psdadd_pi = {
  NULL,
  NULL,
  0,
  NULL,
  "PsdAdd-M2S-Misc",
  "+SARLS",
  NULL,
  "Martin Langer, PZ",
  "math-misc-section",
  N_("PSD Add"),
  N_("Sorry, no help for psdadd filter!"),
  "no more info",
  NULL,
  NULL,
  psdadd_init,
  NULL,
  psdadd_about,
  psdadd_configure,
  NULL,
  NULL,
  NULL,
  psdadd_cleanup
};

GxsmMathTwoSrcPlugin psdadd_m2s_pi = {
  psdadd_run
};

static const char *about_text = N_("Gxsm Psdadd Plugin\n\n"
                                   "PSD Add.");

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  psdadd_pi.description = g_strdup_printf(N_("Gxsm MathTwoArg psdadd plugin %s"), VERSION);
  return &psdadd_pi; 
}

GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &psdadd_m2s_pi; 
}

static void psdadd_init(void)
{
  PI_DEBUG (DBG_L2, "Psdadd Plugin Init");
}

static void psdadd_about(void)
{
  const gchar *authors[] = { psdadd_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  psdadd_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

static void psdadd_configure(void)
{
  if(psdadd_pi.app)
    psdadd_pi.app->message("Psdadd Plugin Configuration");
}

static void psdadd_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Psdadd Plugin Cleanup");
}

static gboolean psdadd_run(Scan *Src1, Scan *Src2, Scan *Dest)
{
// Dest = Src1 (X) Src2
// Experimentierfilter
  int line, col;
  long int LengthX, LengthY; 
  double ScaleFactorX, ScaleFactorY;
  PI_DEBUG (DBG_L2, "Experimental Filter -- PSD ADD");

  if(Src1->data.s.nx != Src2->data.s.nx || Src1->data.s.ny != Src2->data.s.ny)
    return MATH_SIZEERR;

/* 	Funktion zum Addieren von PSD-Bildern mit Neigungsinformationen                             */
/*	LengthX, ..Y	: Abstand zwischen 2 Datenpunkten (in 1/100 Angstroem)                      */
/*	ScaleFactorX,..Y: Skalierungsparameter 1Grad=1Count/ScaleFactor                             */

  LengthX=125000;
  LengthY=125000;
  ScaleFactorX=845112.7;
  ScaleFactorY=13636339.5;

  for(line=0; line<Dest->data.s.ny; line++)
    for(col=0; col<Dest->data.s.nx; col++)
      if (line==0)  
	if (col==0) 	
          Dest->mem2d->PutDataPkt(0,col, line);
        else 
	  Dest->mem2d->PutDataPkt( (LengthX*tan((double)
						(Src1->mem2d->GetDataPkt(col-1,line))/ScaleFactorX)
				    + (Dest->mem2d->GetDataPkt(col-1,line))), col, line);
      else
	if (col==0)
	  Dest->mem2d->PutDataPkt( (LengthY*tan((double)
						(Src2->mem2d->GetDataPkt(col,line-1))/ScaleFactorY)
				    + (Dest->mem2d->GetDataPkt(col,line-1))), col, line);
	else
/*	  die Routine für Mittelung aus x und y-Daten -> Diagonal-Verzerrung          
  	  Dest->mem2d->PutDataPkt((SHT) ((LengthX*tan((double)
	  				(Src1->mem2d->GetDataPkt(col-1, line))/ScaleFactorX)
				       + LengthY*tan((double)
				        (Src2->mem2d->GetDataPkt(col, line-1))/ScaleFactorY)
				       + (Dest->mem2d->GetDataPkt(col-1,line)
				       + (Dest->mem2d->GetDataPkt(col,line-1))))/2), col, line);  */
 	  Dest->mem2d->PutDataPkt( (LengthX*tan((double)
						(Src1->mem2d->GetDataPkt(col-1, line))/ScaleFactorX)
				    + (Dest->mem2d->GetDataPkt(col-1,line))), col, line);
  return MATH_OK;
}
