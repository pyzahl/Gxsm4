/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: histogram.C
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
% PlugInDocuCaption: Histogram

% PlugInName: histogram

% PlugInAuthor: Percy Zahl

% PlugInAuthorEmail: zahl@users.sf.net

% PlugInMenuPath: Math/Statistics/Histogram

% PlugInDescription
The Histogram plugin calculates the Z-value distribution (typically a
height histogram) of the active channel using a default (estimated from data)
or user provided number of bins.

% PlugInUsage
Call it from Gxsm Math/Statistics menu. It will prompt for the number
of bins and provides a estimated number as default. Also the current
min/max Z-value limits and range is shown for informative purpose.

% OptPlugInSources
The active channel is used as data source.

% OptPlugInObjects
A optional rectangle can not be used. Use crop before!

% OptPlugInDest
The computation result is placed into an new profile view.

%% OptPlugInConfig
%The number of bins can be set.

%% OptPlugInFiles
%Needs/creates no files.

%% OptPlugInRefs
%nope.

%% OptPlugInKnownBugs
%No known.

%% OptPlugInNotes
%Hmm, no notes\dots

% OptPlugInHints
Find out what happenes with more or less bins!

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <math.h>
#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"
#include "core-source/action_id.h"
#include "core-source/app_profile.h"

static void histogram_init( void );
static void histogram_about( void );
static void histogram_configure( void );
static void histogram_cleanup( void );
static gboolean histogram_run( Scan *Src );

GxsmPlugin histogram_pi = {
  NULL,
  NULL,
  0,
  NULL,
  "Histogram-M1S-Stat",
  NULL,
  NULL,
  "Percy Zahl",
  "math-statistics-section",
  N_("Histogram"),
  N_("calc. Z (height) Distribution"),
  "no more info",
  NULL,
  NULL,
  histogram_init,
  NULL,
  histogram_about,
  histogram_configure,
  NULL,
  NULL,
  NULL,
  histogram_cleanup
};

GxsmMathOneSrcNoDestPlugin histogram_m1s_pi = {
  histogram_run
};

static const char *about_text = N_("Gxsm Histogram Plugin\n\n"
                                   "calculates Z (height) distribution / Histogram");
static UnitObj *Events = NULL;

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  histogram_pi.description = g_strdup_printf(N_("Gxsm MathOneArg histogram plugin %s"), VERSION);
  return &histogram_pi; 
}

GxsmMathOneSrcNoDestPlugin *get_gxsm_math_one_src_no_dest_plugin_info( void ) { 
  return &histogram_m1s_pi; 
}

static void histogram_init(void)
{
  PI_DEBUG (DBG_L2, "Histogram Plugin Init");
}

static void histogram_about(void)
{
  const gchar *authors[] = { histogram_pi.authors, NULL};
  gtk_show_about_dialog (NULL, 
			 "program-name",  histogram_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

static void histogram_configure(void)
{
	if(histogram_pi.app)
		histogram_pi.app->message("Histogram Plugin Configuration");
}

static void histogram_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Histogram Plugin Cleanup");
	if (Events){
		delete Events;
		Events = NULL;
	}
}

static gboolean histogram_run(Scan *Src)
{
	PI_DEBUG (DBG_L2, "TR Histogram");
	double high, low, zrange, dz, bin_width, dz_norm;
	double dnum;
	int    bin_num;
	gchar *lp, *hp, *rp, *dzp ;
	if (!Events){
		Events = new UnitObj("#","#","g","Count");
		Events->SetAlias ("Events");
	}


	Src->mem2d->HiLo(&high, &low, FALSE, NULL, NULL, 1);
	dz  = Src->data.s.dz;
	low  *= dz;
	high *= dz;
	zrange = high-low;
	gchar *txt = g_strdup_printf("Enter number of bins:\n"
				     "Zmin: %s (%g) ... Zmax: %s (%g)\n"
				     "Zrange: %s (%g)\n" 
				     "dZ: %s (%g)\n"
				     "Max Bins: %d\n", 
				     lp=Src->data.Zunit->UsrString(low), low,
				     hp=Src->data.Zunit->UsrString(high), high,
				     rp=Src->data.Zunit->UsrString(zrange), zrange, 
				     dzp=Src->data.Zunit->UsrString(dz), dz,
				     (int)(zrange/dz));
	g_free(lp);
	g_free(hp);
	g_free(rp);
	g_free(dzp);
	dnum = zrange/dz/3; // +/-1 dz (3dz) in ein bin per default
	histogram_pi.app->ValueRequest ("Enter Value", "Number of Bins", txt,
					Events,
					1., zrange/dz, "g", &dnum);
	g_free(txt);
	
	bin_num   = (int)dnum;
	bin_width = zrange / bin_num;
	dz_norm   = 1./bin_width;

	txt = g_strdup_printf ("Histogram of %d bins, %g width", bin_num, bin_width);
	ProfileControl *pc = new ProfileControl (txt, 
						 bin_num, 
						 Src->data.Zunit, Events, 
						 low, high, "Histogram");
	g_free (txt);

	for(int i = 0; i < bin_num; i++)
		pc->SetPoint (i, 0.);

	for(int col = 0; col < Src->mem2d->GetNx(); col++)
		for(int line = 0; line < Src->mem2d->GetNy(); line++){
			double f  = (dz*Src->mem2d->GetDataPkt(col,line) - low) / bin_width;
			int bin   = (int)f;
	
			if (bin < bin_num){
				double f1 = (bin+1) * bin_width;
				if ((f+dz_norm) > f1){ // partiell in bin, immer rechter Rand, da bin>0 && bin < f
					pc->AddPoint (bin, f-bin);
					++bin;
					if (bin < bin_num)
						pc->AddPoint (bin, bin-f); // 1.-(f-(bin-1))
				}
				else // full inside of bin
					pc->AddPoint (bin, 1.);
			}
		}
  
	pc->UpdateArea ();
	pc->show ();

	histogram_pi.app->xsm->AddProfile (pc); // give it to Surface, it takes care about removing it...
	pc->unref (); // nothing depends on it, so decrement refcount

	return MATH_OK;
}
