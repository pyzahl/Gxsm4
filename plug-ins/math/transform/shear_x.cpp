/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: shear_x.C
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
% PlugInDocuCaption: Shear along X
% PlugInName: shear_x
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Transformation/Shear X

% PlugInDescription
To shear a image along X use this transformation.

% PlugInUsage
Call \GxsmMenu{Math/Transformation/Shear X} and fill in the shear
angle as prompted.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInDest
The computation result is placed into an existing math channel, else into a new created math channel.

%% OptPlugInConfig
%

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"

static void shear_x_init( void );
static void shear_x_about( void );
static void shear_x_configure( void );
static void shear_x_cleanup( void );
static gboolean shear_x_run( Scan *Src, Scan *Dest );

GxsmPlugin shear_x_pi = {
  NULL,
  NULL,
  0,
  NULL,
  "Shear_X-M1S-TR",
  NULL,
  NULL,
  "Percy Zahl",
  "math-transformations-section",
  N_("Shear X"),
  N_("Sorry, no help for shear_x filter!"),
  "no more info",
  NULL,
  NULL,
  shear_x_init,
  NULL,
  shear_x_about,
  shear_x_configure,
  NULL,
  NULL,
  NULL,
  shear_x_cleanup
};

GxsmMathOneSrcPlugin shear_x_m1s_pi = {
  shear_x_run
};

static const char *about_text = N_("Gxsm Shear_X Plugin\n\n"
                                   "shear scan along X.");

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  shear_x_pi.description = g_strdup_printf(N_("Gxsm MathOneArg shear_x plugin %s"), VERSION);
  return &shear_x_pi; 
}

GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) { 
  return &shear_x_m1s_pi; 
}

static void shear_x_init(void)
{
  PI_DEBUG (DBG_L2, "Shear_X Plugin Init");
}

static void shear_x_about(void)
{
  const gchar *authors[] = { shear_x_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  shear_x_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

static void shear_x_configure(void)
{
  if(shear_x_pi.app)
    shear_x_pi.app->message("Shear_X Plugin Configuration");
}

static void shear_x_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Shear_X Plugin Cleanup");
}

static gboolean shear_x_run(Scan *Src, Scan *Dest)
{
  double phi = 45.;
  double tanphi;
  int line, len;
  PI_DEBUG (DBG_L2, "Shear Scan X");

  shear_x_pi.app->ValueRequest("Enter Value", "angle", "I need the shear-X angle",
			       shear_x_pi.app->xsm->ArcUnit, -70., 70., ".2f", &phi);

  tanphi = tan(phi*M_PI/180.);

  Dest->data.s.nx = Src->mem2d->GetNx() 
    + (len = (int)((double)Src->mem2d->GetNx()*fabs(tanphi))+1);
  Dest->data.s.ny = Src->mem2d->GetNy();
  Dest->mem2d->Resize(Dest->data.s.nx+1, Dest->data.s.ny);

  // Anpassen an nächstmöglichen Wert  
  Dest->data.s.rx = Dest->data.s.nx*Dest->data.s.dx;
  Dest->data.s.x0 -= (double)len*Dest->data.s.dx/2.;
  Dest->mem2d->data->MkXLookup(Dest->data.s.x0,Dest->data.s.x0+Dest->data.s.rx);
  Dest->data.s.ry = Dest->data.s.ny*Dest->data.s.dy;

  if(tanphi>0.)
    for(line = 0; line < Dest->data.s.ny; line++)
      Dest->mem2d->CopyFrom(Src->mem2d, 0, line, (int)rint((double)line*tanphi), line, Src->mem2d->GetNx());
  else
    for(line = 0; line < Dest->data.s.ny; line++)
      Dest->mem2d->CopyFrom(Src->mem2d, 0, line, len+(int)rint((double)line*tanphi), line, Src->mem2d->GetNx());

  return MATH_OK;
}
