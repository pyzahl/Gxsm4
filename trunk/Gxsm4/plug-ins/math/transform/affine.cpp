/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: affine.C
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
% PlugInDocuCaption: Affine Transformation
% PlugInName: affine
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Transformation/Affine

% PlugInDescription
In case you want to undo a linear distorsion of an image (e.g. slow
and contineous drift/creep) this transformation helps. It applies a
affine transformation to the image, e.g. two arbitrary oriented
vectors (and the image) are transformed to be a orthogonal system
afterwards.

% PlugInUsage
Use a \GxsmEmph{Ksys} to set the vectors. The relative lenght between
both is ignored, they are normalized before.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInObjects
A Ksys object is used for setting the two vectors.

% OptPlugInDest
The computation result is placed into an existing math channel, else into a new created math channel.

%% OptPlugInConfig
%

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

%% OptPlugInKnownBugs
%

% OptPlugInNotes
If the image is rotated or flipped more than expected, try flipping the Ksys!
I know it's a bit tricky, good ideas to fix this are welcome!

%% OptPlugInHints
%

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"

static void affine_init( void );
static void affine_about( void );
static void affine_configure( void );
static void affine_cleanup( void );
static gboolean affine_run( Scan *Src, Scan *Dest );

GxsmPlugin affine_pi = {
	NULL,
	NULL,
	0,
	NULL,
	"Affine-M1S-TR",
	NULL,
	NULL,
	"Percy Zahl",
	"math-transformations-section",
	N_("Affine"),
	N_("Sorry, no help for affine filter!"),
	"no more info",
	NULL,
	NULL,
	affine_init,
	NULL,
	affine_about,
	affine_configure,
	NULL,
	NULL,
	NULL,
	affine_cleanup
};

GxsmMathOneSrcPlugin affine_m1s_pi = {
	affine_run
};

static const char *about_text = N_("Gxsm Affine Plugin\n\n"
                                   "affine trafo.");

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	affine_pi.description = g_strdup_printf(N_("Gxsm MathOneArg affine plugin %s"), VERSION);
	return &affine_pi; 
}

GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) { 
	return &affine_m1s_pi; 
}

static void affine_init(void)
{
	PI_DEBUG (DBG_L2, "Affine Plugin Init");
}

static void affine_about(void)
{
	const gchar *authors[] = { affine_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  affine_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

static void affine_configure(void)
{
	if(affine_pi.app)
		affine_pi.app->message("Affine Plugin Configuration");
}

static void affine_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Affine Plugin Cleanup");
}

static gboolean affine_run(Scan *Src, Scan *Dest)
{
	int line, col;
	double trcol, trline;
	double f11,f12,f21,f22,n1,n2,n;
	double t11,t12,t21,t22,det;
	PI_DEBUG (DBG_L2, "Affine Transformation");

	g_message ("PlugIn affine.C: Fix Me! Need mouse coordinate port top new scan_obj methods.");

	return 0;

#if 0

	if(Src->PktVal < 3){
		affine_pi.app->message("Please use object \"Ksys\" to define transformation!");
		return MATH_OK;
	}

	// Matrix F: Src --> Dest
	f11 = Src->Pkt2d[0].x-Src->Pkt2d[1].x;
	f12 = Src->Pkt2d[0].y-Src->Pkt2d[1].y;
	f21 = Src->Pkt2d[2].x-Src->Pkt2d[1].x;
	f22 = Src->Pkt2d[2].y-Src->Pkt2d[1].y;

	n1  = sqrt(f11*f11+f12*f12);
	n2  = sqrt(f21*f21+f22*f22);
	n   = (n1+n2)/2;

	f11 /= n1; f12 /= n1; f21 /= n2; f22 /= n2; // ohne "Aspekt"
	//f11 /= n; f12 /= n; f21 /= n; f22 /= n; // mit "Aspekt"

	det = f11*f22-f12*f21;

	if(fabs(det)>1e-5){
		// Matrix T: Dest --> Src
		t11 = f22/det; t12 = -f12/det; t21 = -f21/det; t22 = f11/det;

		std::cout << f11 << " " << f21 << " | " << t11  << " " << t21 << std::endl
			  << f12 << " " << f22 << " | " << t12  << " " << t22 << std::endl
			  << "det = " << det << std::endl;

		// Dest Bounds:
		int x00,y00,x01,y01,x10,y10,x11,y11,xmi,ymi,xmx,ymx;
		col=0; line=0;
		x00 = (int)rint((double)col*f11 + (double)line*f21);
		y00 = (int)rint((double)col*f12 + (double)line*f22);
		col=0; line=Src->mem2d->GetNy();
		x01 = (int)rint((double)col*f11 + (double)line*f21);
		y01 = (int)rint((double)col*f12 + (double)line*f22);
		col=Src->mem2d->GetNx(); line=0;
		x10 = (int)rint((double)col*f11 + (double)line*f21);
		y10 = (int)rint((double)col*f12 + (double)line*f22);
		col=Src->mem2d->GetNx(); line=Src->mem2d->GetNy();
		x11 = (int)rint((double)col*f11 + (double)line*f21);
		y11 = (int)rint((double)col*f12 + (double)line*f22);

		PI_DEBUG (DBG_L2, "00: " << x00 << "," << y00 << "; "
			  << "01: " << x01 << "," << y01 << "; "
			  << "10: " << x10 << "," << y10 << "; "
			  << "11: " << x11 << "," << y11);

		xmi = MIN(MIN(MIN(x00,x01),x10),x11);
		xmx = MAX(MAX(MAX(x00,x01),x10),x11);
		ymi = MIN(MIN(MIN(y00,y01),y10),y11);
		ymx = MAX(MAX(MAX(y00,y01),y10),y11);

		// new Dest size
		Dest->data.s.nx = xmx-xmi+1;
		Dest->data.s.ny = ymx-ymi+1;
		Dest->mem2d->Resize(Dest->data.s.nx, Dest->data.s.ny);
    
		// Anpassen an nächstmöglichen Wert  
		Dest->data.s.rx = Dest->data.s.nx*Dest->data.s.dx;
		Dest->data.s.ry = Dest->data.s.ny*Dest->data.s.dy;

		// calc SrcPkte from Dest Pos.:
		for(col = 0; col < Dest->data.s.nx; col++)
			for(line = 0; line < Dest->data.s.ny; line++){
				trcol  = (double)(col+xmi)*t11 + (double)(line+ymi)*t21;
				trline = (double)(col+xmi)*t12 + (double)(line+ymi)*t22;
				if(trcol >=  Src->data.s.nx)
					trcol= Src->data.s.nx-1;
				if(trcol <  0) 
					trcol=0;
				if(trline >=  Src->data.s.ny) 
					trline= Src->data.s.ny-1;
				if(trline <  0) 
					trline=0;
				Dest->mem2d->PutDataPkt(Src->mem2d->GetDataPktInterpol(trcol, trline), col, line);
			}
		return MATH_OK;
	}else
		return MATH_DIVZERO;
#endif
}
