/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: rotate.C
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
% PlugInDocuCaption: Rotate a scan area
% PlugInName: rotate
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Transformation/Rotate

% PlugInDescription
To rotate a selected area select a \GxsmEmph{Rectangle} of the area to
be rotated into. Think inverse, the result is the cropped area of the
source scan, which is rotated before around the center of the selected
area. If needed data points are outside, they are replaced by the
value found a the closed edge.

% PlugInUsage
Place a rectangle object and call
\GxsmMenu{Math/Transformation/Rotate}. It prompts for the rotation
angle (clockwise) if not set to any other default than zero via the
PlugIn configurator.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInObjects
A rectangle object is needed.

% OptPlugInDest
The computation result is placed into an existing math channel, else into a new created math channel.

% OptPlugInConfig
The PlugIn configurator allows to set a default angle. If this is non
zero the user is not prompted for a rotation angle!

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"

static void rotate_about( void );
static void rotate_configuration( void );
static gboolean rotate_run( Scan *Src, Scan *Dest );

GxsmPlugin rotate_pi = {
  NULL,
  NULL,
  0,
  NULL,
  "Rotate-M1S-TR",
  NULL,
  NULL,
  "Percy Zahl",
  "math-transformations-section",
  N_("Rotate"),
  N_("Rotate selection of scan"),
  "no more info",
  NULL,
  NULL,
  NULL,
  NULL,
  rotate_about,
  rotate_configuration,
  NULL,
  NULL
};

GxsmMathOneSrcPlugin rotate_m1s_pi = {
	rotate_run
};

static const char *about_text = N_("Gxsm Rotate Math-Plugin\n\n"
                                   "Affine Transformation:\n"
				   "Rotate selection of Scan by arb. angle.");

double PhiDefault = 0.;
double PhiLast = 0.;

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	rotate_pi.description = g_strdup_printf(N_("Gxsm MathOneArg rotate plugin %s"), VERSION);
	return &rotate_pi; 
}

GxsmMathOneSrcPlugin *get_gxsm_math_one_src_for_all_vt_plugin_info( void ) { 
	return &rotate_m1s_pi; 
}

static void rotate_about(void)
{
	const gchar *authors[] = { rotate_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  rotate_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

static void rotate_configuration( void ){
	rotate_pi.app->ValueRequest("Enter Value", "angle", 
				    "Default rotation angle, if set to Zero I will ask later!",
				    rotate_pi.app->xsm->ArcUnit, 
				    -360., 360., ".2f", &PhiDefault);
}

int max (int a, int b) { return a>b ? a:b; }

static gboolean rotate_run(Scan *Src, Scan *Dest)
{
	MOUSERECT msr;
	double phi = PhiLast;
	double cphi,sphi;
	int col, line, nx2, ny2;
	double drcol, drline;
	int xl, yo;
	PI_DEBUG (DBG_L2, "Rotate Scan");
	
	if ((Src ? Src->mem2d->get_t_index ():0) == 0 && (Src ? Src->mem2d->GetLayer ():0) == 0){
		if(fabs(PhiDefault) > 1e-10)
			phi = PhiDefault;
		else
			rotate_pi.app->ValueRequest("Enter Value", "angle", 
						    "I need the rotation angle.",
						    rotate_pi.app->xsm->ArcUnit, 
						    -360., 360., ".2f", &phi);
		PhiLast = phi;

		if (!Src || !Dest)
			return 0;
	}

	phi *= M_PI/180.;
	// Drehmatrix:
	cphi = cos(phi);
	sphi = sin(phi);

	gint success = MkMausSelect (Src, &msr, Dest->mem2d->GetNx(), Dest->mem2d->GetNy());
	  
	int snx =  Src->mem2d->GetNx ();
	int sny =  Src->mem2d->GetNy ();

	std::cout << "Rot: Src[" << snx << ", " << sny << ", v=" << Src->mem2d->GetLayer () << ", t=" << Src->mem2d->get_t_index () << "]" << std::endl;

	if( !success || msr.xSize  < 1 || msr.ySize < 1){ // all or use selection area only?
		nx2 =  snx/2;
		ny2 =  sny/2;
		int xy[4][2];
		for (int i=0; i<=1; ++i)
			for (int j=0; j<=1; ++j){
				xy[i+2*j][0] = (int)round ( (double)(i*snx-nx2) * cphi + (double)(j*sny-ny2) * sphi);
				xy[i+2*j][1] = (int)round (-(double)(i*snx-nx2) * sphi + (double)(j*sny-ny2) * cphi);
			}
	  
		Dest->data.s.nx = max (abs (xy[0][0] - xy[1][0]), abs (xy[2][0] - xy[3][0]));
		Dest->data.s.ny = max (abs (xy[0][1] - xy[2][1]), abs (xy[1][1] - xy[3][1]));

		xl = yo = 0;

//	  rotate_pi.app->message("Please select a Rectangle!");
//	  return MATH_SELECTIONERR;
		std::cout.width(4);
		std::cout << "Corners:[" << std::endl
			  << xy[0][0] << " " << xy[1][0] << " " << xy[2][0] << " " << xy[3][0] << std::endl
			  << xy[0][1] << " " << xy[1][1] << " " << xy[2][1] << " " << xy[3][1]
			  << "]" << std::endl;

		std::cout << "Rot All: Dest[" << Dest->data.s.nx <<", " << Dest->data.s.ny << ", " << Src->mem2d->GetNv () << "]" << std::endl;
	} else { // all

		Dest->data.s.nx = msr.xSize;
		Dest->data.s.ny = msr.ySize;
		Dest->data.s.x0 += (msr.xLeft + msr.xSize/2 - snx/2)*Src->data.s.dx;
		Dest->data.s.y0 += msr.yTop*Src->data.s.dy;
		xl = msr.xLeft;
		yo = msr.yTop;

		std::cout << "Rot Area: Dest[" << Dest->data.s.nx <<", " << Dest->data.s.ny << ", " << Src->mem2d->GetNv () << "]" << std::endl;
	}

	Dest->mem2d->Resize(Dest->data.s.nx, Dest->data.s.ny, Src->mem2d->GetNv ());

	// Anpassen an nächstmöglichen Wert  
	Dest->data.s.rx = Dest->data.s.nx*Dest->data.s.dx;
	Dest->data.s.ry = Dest->data.s.ny*Dest->data.s.dy;

	nx2 =  Dest->data.s.nx/2;
	ny2 =  Dest->data.s.ny/2;

	Dest->mem2d->SetLayer (Src->mem2d->GetLayer ());
	
	for(col = 0; col < Dest->data.s.nx; col++)
		for(line = 0; line < Dest->data.s.ny; line++){
			drcol  = (double)(xl + nx2) + (double)(col - nx2)*cphi + (double)(line - ny2)*sphi;
			drline = (double)(yo + ny2) - (double)(col - nx2)*sphi + (double)(line - ny2)*cphi;
			if((drcol+1) >=  Src->data.s.nx-1)
				drcol= Src->data.s.nx-2;
			if(drcol <  0) 
				drcol=0.;
			if((drline+1) >=  Src->data.s.ny) 
				drline= Src->data.s.ny-2;
			if(drline <  0) 
				drline=0.;
			Dest->mem2d->PutDataPkt(Src->mem2d->GetDataPktInterpol(drcol, drline), col, line);
		}
	return MATH_OK;
}
