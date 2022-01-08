/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: plane3pkt.C
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
% PlugInDocuCaption: Plane three points
% PlugInName: plane3pkte
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Background/Plane 3 Points

% PlugInDescription
The filter removes a by three points defined plane from the scan.

% PlugInUsage
Define the three points using the \GxsmEmph{Point} object.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInObjects
The \GxsmEmph{Ksys} object is needet to define the plane via the three
points provided.

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

%% OptPlugInRefs
%Any references?

%% OptPlugInKnownBugs
%Are there known bugs? List! How to work around if not fixed?

%% OptPlugInNotes

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

#define LL_DEBUG(STR) PI_DEBUG (DBG_L2, "bg_3pkte_run:: " << STR << std::endl);
//#define LL_DEBUG(STR)

static void plane3pkt_init( void );
static void plane3pkt_about( void );
static void plane3pkt_configure( void );
static void plane3pkt_cleanup( void );
static gboolean plane3pkt_run( Scan *Src, Scan *Dest );

GxsmPlugin plane3pkt_pi = {
  NULL,
  NULL,
  0,
  NULL,
  "Plane3pkt-M1S-Misc",
  "+STM +AFM",
  NULL,
  "Percy Zahl",
  "math-background-section",
  N_("Plane 3 Points"),
  N_("Plane subtraction, def. by 3 points"),
  "no more info",
  NULL,
  NULL,
  plane3pkt_init,
  NULL,
  plane3pkt_about,
  plane3pkt_configure,
  NULL,
  NULL,
  NULL,
  plane3pkt_cleanup
};

GxsmMathOneSrcPlugin plane3pkt_m1s_pi = {
  plane3pkt_run
};

static const char *about_text = N_("Gxsm Plane3pkt Plugin\n\n"
                                   "Abzug einer Ebene, def. by three points.");

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  plane3pkt_pi.description = g_strdup_printf(N_("Gxsm MathOneArg plane3pkt plugin %s"), VERSION);
  return &plane3pkt_pi; 
}

GxsmMathOneSrcPlugin *get_gxsm_math_one_src_for_all_vt_plugin_info( void ) { 
  return &plane3pkt_m1s_pi; 
}

static void plane3pkt_init(void)
{
  PI_DEBUG (DBG_L2, "Plane3pkt Plugin Init");
}

static void plane3pkt_about(void)
{
  const gchar *authors[] = { plane3pkt_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  plane3pkt_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

static void plane3pkt_configure(void)
{
	;
}

static void plane3pkt_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Plane3pkt Plugin Cleanup");
}

static gboolean plane3pkt_run(Scan *Src, Scan *Dest)
{
		/* plane with  ax +by +cz +d = 0 given by 3 points */
		double a, b, c, d, asum, bsum;
		double x1, y1, z1, z2, x2, y2, z3, x3, y3;
		long i,j;
		
		int num_pkte=0;
		int n_obj = Src->number_of_object ();

		Point2D p[3];

		if (!Src || !Dest)
			return 0;

		// at least three...
		if (n_obj < 3) {
				plane3pkt_pi.app->message("Sorry, need three points:\n use 3 Point Objects!");
				return MATH_SELECTIONERR;
		}

		// look for the three point objects 
		i=0;
		while (n_obj-- && i < 3){
				scan_object_data *obj_data = Src->get_object_data (n_obj);

				if (strncmp (obj_data->get_name (), "Point", 5) )
						continue; // only points are used!

				if (obj_data->get_num_points () != 1) 
						continue; // sth. is weired!

				LL_DEBUG( "bg_3pkte_run:: processing point " << n_obj );
				
				// get real world coordinates of point
				double x,y;
				obj_data->get_xy_i_pixel (0, x, y);
				p[i].x = (int)x; p[i].y = (int)y;
				++i;
		}		

		if(i != 3){
				plane3pkt_pi.app->message("Sorry, need three points:\n use 3 Point Objects!");
				return MATH_SELECTIONERR;
		}

		/* PRODUCE IN-PLANE VECTORS */
		x3 = p[1].x;
		y3 = p[1].y;
		if (x3 < 0 || y3 < 0 || x3 > Src->mem2d->GetNx() || y3 > Src->mem2d->GetNy()) 
			return MATH_SELECTIONERR;
		z3 = Src->mem2d->GetDataPkt((int)(x3), (int)(y3));

		x1 = p[0].x-x3;
		y1 = p[0].y-y3;
		if (p[0].x < 0 || p[0].y < 0 || p[0].x > Src->mem2d->GetNx() || p[0].y > Src->mem2d->GetNy()) 
			return MATH_SELECTIONERR;
		z1 = (double)Src->mem2d->GetDataPkt((int)p[0].x, (int)p[0].y) - z3;
  
		x2 = p[2].x-x3;
		y2 = p[2].y-y3;
		if (p[0].x < 0 || p[0].y < 0 || p[2].x > Src->mem2d->GetNx() || p[2].y > Src->mem2d->GetNy()) 
			return MATH_SELECTIONERR;
		z2 = (double)Src->mem2d->GetDataPkt((int)p[2].x, (int)p[2].y) - z3;

		/* a,b,c follow from cross-produkt of 1 and 2 */
		a = y1*z2 - z1*y2;
		b = z1*x2 - x1*z2;
		c = x1*y2 - y1*x2;
		
		/* d follows from one special point, use POINT 3 */
		d = a*x3 + b*y3 + c*z3;
		d *= (-1.0);
		

		bsum = 0.0;
		d /= c;
		a /= c;
		b /= c;
		for (j=0; j < Dest->mem2d->GetNy(); ++j, bsum += b) {
				asum = bsum + d;
				for (i=0; i < Dest->mem2d->GetNx(); ++i, asum += a)
						Dest->mem2d->PutDataPkt
								( Src->mem2d->GetDataPkt(i,j) + asum, i, j);
		}
		return MATH_OK;
}
