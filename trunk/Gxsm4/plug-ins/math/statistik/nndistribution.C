/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: nndistribution.C
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
% PlugInDocuCaption: Nndistribution

% PlugInName: nndistribution

% PlugInAuthor: Percy Zahl

% PlugInAuthorEmail: zahl@users.sf.net

% PlugInMenuPath: Math/Statistics/Nndistribution

% PlugInDescription
The NN-distribution plugin calculates the nearest
neigbour (lateral) distribution of marker groups manually placed in
the active channel using a default (estimated from data) or user
provided number of bins.
Option to auto-recenter markers on max or min locally.

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
#include "plugin.h"
#include "action_id.h"
#include "app_profile.h"

static void nndistribution_init( void );
static void nndistribution_about( void );
static void nndistribution_configure( void );
static void nndistribution_cleanup( void );
static gboolean nndistribution_run( Scan *Src );

GxsmPlugin nndistribution_pi = {
  NULL,
  NULL,
  0,
  NULL,
  "Nndistribution-M1S-Stat",
  NULL,
  NULL,
  "Percy Zahl",
  "math-statistics-section",
  N_("NN-distribution"),
  N_("calc. nearest neigbour (lateral) distribution of marker groups"),
  "no more info",
  NULL,
  NULL,
  nndistribution_init,
  NULL,
  nndistribution_about,
  nndistribution_configure,
  NULL,
  NULL,
  NULL,
  nndistribution_cleanup
};

GxsmMathOneSrcNoDestPlugin nndistribution_m1s_pi = {
  nndistribution_run
};

static const char *about_text = N_("Gxsm NN-distribution Plugin\n\n"
                                   "calc. nearest neigbour (lateral) distribution of marker groups");
static UnitObj *Events = NULL;

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  nndistribution_pi.description = g_strdup_printf(N_("Gxsm MathOneArg nndistribution plugin %s"), VERSION);
  return &nndistribution_pi; 
}

GxsmMathOneSrcNoDestPlugin *get_gxsm_math_one_src_no_dest_plugin_info( void ) { 
  return &nndistribution_m1s_pi; 
}

static void nndistribution_init(void)
{
  PI_DEBUG (DBG_L2, "Nndistribution Plugin Init");
}

static void nndistribution_about(void)
{
  const gchar *authors[] = { nndistribution_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  nndistribution_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

static void nndistribution_configure(void)
{
	if(nndistribution_pi.app)
		nndistribution_pi.app->message("Nndistribution Plugin Configuration");
}

static void nndistribution_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Nndistribution Plugin Cleanup");
	if (Events){
		delete Events;
		Events = NULL;
	}
}


int build_marker_list(Scan *Src, scan_object_data **&objects, const gchar *type)
{
	int i;
	int num_pkte=0;
	int n_obj = Src->number_of_object ();

	int tn = strlen (type);

	// at least three...
	if (n_obj < 2) {
		nndistribution_pi.app->message("Sorry, need at least two markers.");
		objects = NULL;
		return 0;
	}

	objects = new scan_object_data*[n_obj+1];
	std::cout << "NN distribution, # all objects = " << n_obj << std::endl;
	std::cout << "NN distribution, building list of marker group " << type << " objects..." << std::endl;

	// look for marker objects, build list 
	i=0;
	while (n_obj--){
		objects[i] = Src->get_object_data (n_obj);

		std::cout << n_obj << ": " << objects[i]->get_name () << std::endl;

		//		if (strncmp (objects[i]->get_name (), "Point", 5) && strncmp (objects[i]->get_name (), type, tn))
		if (strncmp (objects[i]->get_name (), type, tn))
			continue; // only points are used!
		
		if (objects[i]->get_num_points () != 1) 
			continue; // sth. is weired!
		
		double x,y,r;
		// get real world coordinates of point
		objects[i]->get_xy_i (0, x, y);

		if (i>0)
			r = objects[i]->distance(objects[i-1]);
		else
			r = -1.;

		std::cout
			<< objects[i]->get_name ()
			<< "[" << i << "] =: (" 
			<< x << ", " 
			<< y << ") r to prev ="
			<< r
			<< std::endl;
		
		++i;
	}
	std::cout << "NN distribution, # markers of " << type << " group = " << i << std::endl;
	objects[i] = NULL;

	return i;
}


static gboolean nndistribution_run(Scan *Src)
{
	PI_DEBUG (DBG_L2, "NN Distribution");
	double high, low, rrange, dr, bin_width, dr_norm;
	int    bin_num;

	const gchar *marker_group[] = { 
		"*Marker", 
		"*Marker:red", "*Marker:green", "*Marker:blue", "*Marker:yellow", "*Marker:cyan", "*Marker:magenta",  
		NULL };
	gint nobj[7];
	scan_object_data **objects[7];


	if (!Events){
		Events = new UnitObj("#","#","g","Count");
		Events->SetAlias ("Events");
	}

	bin_width = 2.; // suggesttion estimate
	nndistribution_pi.app->ValueRequest ("Enter Value", "Bin Width", "Enter bins width:",
					     Src->data.Xunit,
					     0.1, 10000., "g", &bin_width);

	for (int mi=0; marker_group[mi]; ++mi){

		std::cout << "NN distribution: building point objects list for " << marker_group[mi] << " ..." << std::endl;

		nobj[mi] = build_marker_list(Src, objects[mi], marker_group[mi]);
		if (!nobj[mi]) {
			std::cout << "NN distribution: no objects found for marker group " << marker_group[mi] << " ." << std::endl;
			continue;
		}
	}

	for (int mi=0; marker_group[mi]; ++mi){
		if (!nobj[mi]) 
			continue;
		for (int mj=mi; marker_group[mj]; ++mj){
			if (!nobj[mj]) 
				continue;
			
			gint nnn = nobj[mi] < nobj[mj] ? nobj[mi] : nobj[mj];
			gint mk = nobj[mi] < nobj[mj] ? mi : mj; // the lesser # group
			gint ml = nobj[mi] < nobj[mj] ? mj : mi; // the other

			double *rnn = new double[nnn];
			gint *innk = new gint[nobj[mk]];

			low  = objects[mk][0]->distance(objects[ml][1]);
			high = 0.;

			std::cout << "NN distribution: locating " << nnn << " nearest neigbours in groups "
				  << marker_group[mi]
				  << " and " 
				  << marker_group[mj] 
				  << " :: [mk:" << mk << ", mj:" << mj << "] ... -- r-low=" << low
				  << std::endl;


			for (int k=0; k<nnn; ++k){
				if (k>1){
					rnn[k] = objects[mk][k]->distance(objects[ml][k-1]); innk[k]=k-1;
				} else {
					rnn[k] = objects[mk][k]->distance(objects[ml][k+1]); innk[k]=k+1;
				}
				for (int l=0; l<nobj[ml]; ++l){
					if (objects[mk][k] == objects[ml][l]) continue; // not self
					double r = objects[mk][k]->distance(objects[ml][l]);
					if (r < rnn[k]) { rnn[k] = r; innk[k] = l; }
#if 0
					std::cout << objects[mk][k]->get_name ()
						  << "[" << k << "] " 
						  << " - "
						  << objects[ml][l]->get_name ()
						  << "[" << l << "] = " 
						  << r
						  << std::endl;
#endif
				}
				if (rnn[k] < low) low = rnn[k];
				if (rnn[k] > high) high = rnn[k];
				std::cout << objects[mk][k]->get_name ()
					  << "[" << k << "] " 
					  << " - "
					  << objects[ml][innk[k]]->get_name ()
					  << "[" << innk[k] << "] is NN := " 
					  << rnn[k]
					  << std::endl;
			}

			rrange = high-low;
			bin_num   = rrange / bin_width + 1.;
			dr_norm   = 1./bin_width;
			
			std::cout << "NN distribution: binning ... " << std::endl
				  << "#bins.... = " << bin_num << std::endl
				  << "r range.. = " << rrange  << std::endl
				  << "nearest.. = " << low     << std::endl
				  << "furthest. = " << high    << std::endl;
			
			gchar* txt = g_strdup_printf ("NN distribution of %d out of %d # %s objects to %d # %s objects in %d bins, width %s", 
						      nnn,
						      nobj[mi], marker_group[mi],
						      nobj[mj], marker_group[mj],
						      bin_num, 
						      Src->data.Xunit->UsrString (bin_width));
		
			ProfileControl *pc = new ProfileControl (txt, 
								 bin_num, 
								 Src->data.Zunit, Events, 
								 low, high, "NN-distribution");
			g_free (txt);
			
			for(int i = 0; i < bin_num; i++)
				pc->SetPoint (i, 0.);
			
			for(int i = 0; i < nnn; ++i){
				double f  = (rnn[i]-low) / bin_width;
				int bin   = (int)f;

				std::cout << "++ Bin[" << bin << "] " << f << " rnn=" << rnn[i] << std::endl;

				if (bin < bin_num){
					double f1 = (bin+1) * bin_width;
					if ((f+dr_norm) > f1){ // partiell in bin, immer rechter Rand, da bin>0 && bin < f
						pc->AddPoint (bin, f-bin);
						++bin;
						if (bin < bin_num)
							pc->AddPoint (bin, bin-f); // 1.-(f-(bin-1))
					}
					else // full inside of bin
						pc->AddPoint (bin, 1.);
				}
			}
  
			delete rnn;
			delete innk;

			nndistribution_pi.app->xsm->AddProfile (pc); // give it to Surface, it takes care about removing it...
			pc->unref (); // nothing depends on it, so decrement refcount

			pc->UpdateArea ();
			pc->show ();
		}
	}


	for (int mi=0; marker_group[mi]; ++mi){
		if (nobj[mi])
		    delete objects[mi];
	}



	return MATH_OK;
}
