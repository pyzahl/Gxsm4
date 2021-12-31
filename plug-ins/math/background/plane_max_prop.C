/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * plugin_helper reports your answers as
author		=Percy Zahl
email	        	=zahl@users.sf.net
pluginname		=plane_max_prop
pluginmenuentry 	=Plane max prop
menupath		=Math/Background/
entryplace		=Background
shortentryplace	=BG
abouttext		=Calculates a max propability plane and subtracts it.
smallhelp		=Plane max propability
longhelp		=Calculates a max propability plane and subtracts it.
 * 
 * Gxsm Plugin Name: plane_max_prop.C
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
 * All "% OptPlugInXXX" tags are optional and can be removed or commented in
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Plane max. propability
% PlugInName: plane_max_prop
% PlugInAuthor: Percy Zahl, L.Anderson, Greg P. Kochanski
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Background/Plane max prop

% PlugInDescription
Calculates a max propability plane and subtracts it. It's purpose is
to find automatically the best fitting plane to orient a
stepped/vicinal surface in a way, that the terraces are horizontal.

% PlugInUsage
Call \GxsmMenu{Math/Background/Plane max prop}.

% OptPlugInSources
The active channel is used as data source.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

% OptPlugInDest
The computation result is placed into an existing math channel, else
into a new created math channel.

%% OptPlugInConfig
%describe the configuration options of your plug in here!

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

%% OptPlugInRefs
%Any references?

%% OptPlugInKnownBugs
%Are there known bugs? List! How to work around if not fixed?

% OptPlugInNotes
This code in work in progress, it is originated from PMSTM
\GxsmFile{mpplane.c} and was rewritten as a Gxsm math PlugIn. It looks
like something does not work like expected, the corrected plane is not
right for some still not found reason.

%% OptPlugInHints
%Any tips and tricks?

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"

// Plugin Prototypes
static void plane_max_prop_init( void );
static void plane_max_prop_about( void );
static void plane_max_prop_configure( void );
static void plane_max_prop_cleanup( void );

// Define Type of math plugin here, only one line should be commented in!!
#define GXSM_ONE_SRC_PLUGIN__DEF
// #define GXSM_TWO_SRC_PLUGIN__DEF

// Math-Run-Function, use only one of (automatically done :=)
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
// "OneSrc" Prototype
 static gboolean plane_max_prop_run( Scan *Src, Scan *Dest );
#else
// "TwoSrc" Prototype
 static gboolean plane_max_prop_run( Scan *Src1, Scan *Src2, Scan *Dest );
#endif

// Fill in the GxsmPlugin Description here
GxsmPlugin plane_max_prop_pi = {
  NULL,                   // filled in and used by Gxsm, don't touch !
  NULL,                   // filled in and used by Gxsm, don't touch !
  0,                      // filled in and used by Gxsm, don't touch !
  NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
                          // filled in here by Gxsm on Plugin load, 
                          // just after init() is called !!!
  // ----------------------------------------------------------------------
  // Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
  "plane_max_prop-"
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
  g_strdup("Calculates a max propability plane and subtracts it."),                   
  // Author(s)
  "Percy Zahl",
  // Menupath to position where it is appendet to
  "math-background-section",
  // Menuentry
  N_("Plane max prop"),
  // help text shown on menu
  N_("Calculates a max propability plane and subtracts it."),
  // more info...
  "Plane max propability",
  NULL,          // error msg, plugin may put error status msg here later
  NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
  // init-function pointer, can be "NULL", 
  // called if present at plugin load
  plane_max_prop_init,  
  // query-function pointer, can be "NULL", 
  // called if present after plugin init to let plugin manage it install itself
  NULL, // query should be "NULL" for Gxsm-Math-Plugin !!!
  // about-function, can be "NULL"
  // can be called by "Plugin Details"
  plane_max_prop_about,
  // configure-function, can be "NULL"
  // can be called by "Plugin Details"
  plane_max_prop_configure,
  // run-function, can be "NULL", if non-Zero and no query defined, 
  // it is called on menupath->"plugin"
  NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
  // cleanup-function, can be "NULL"
  // called if present at plugin removal
  NULL, // direct menu entry callback1 or NULL
  NULL, // direct menu entry callback2 or NULL

  plane_max_prop_cleanup
};

// special math Plugin-Strucure, use
// GxsmMathOneSrcPlugin plane_max_prop_m1s_pi -> "OneSrcMath"
// GxsmMathTwoSrcPlugin plane_max_prop_m2s_pi -> "TwoSrcMath"
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
 GxsmMathOneSrcPlugin plane_max_prop_m1s_pi
#else
 GxsmMathTwoSrcPlugin plane_max_prop_m2s_pi
#endif
 = {
   // math-function to run, see prototype(s) above!!
   plane_max_prop_run
 };

// Text used in Aboutbox, please update!!
static const char *about_text = N_("Gxsm plane_max_prop Plugin\n\n"
                                   "Calculates a max propability plane and subtracts it.");

// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  plane_max_prop_pi.description = g_strdup_printf(N_("Gxsm MathOneArg plane_max_prop plugin %s"), VERSION);
  return &plane_max_prop_pi; 
}

// Symbol "get_gxsm_math_one|two_src_plugin_info" is resolved by dlsym from Gxsm, 
// used to find out which Math Type the Plugin is!! 
// Essential Plugin Function!!
#ifdef GXSM_ONE_SRC_PLUGIN__DEF
GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) {
  return &plane_max_prop_m1s_pi; 
}
#else
GxsmMathTwoSrcPlugin *get_gxsm_math_two_src_plugin_info( void ) { 
  return &plane_max_prop_m2s_pi; 
}
#endif

/* Here we go... */
// init-Function
static void plane_max_prop_init(void)
{
	PI_DEBUG (DBG_L2, "plane_max_prop Plugin Init");
}

// about-Function
static void plane_max_prop_about(void)
{
	const gchar *authors[] = { plane_max_prop_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  plane_max_prop_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

// configure-Function
static void plane_max_prop_configure(void)
{
	if(plane_max_prop_pi.app)
		plane_max_prop_pi.app->message("plane_max_prop Plugin Configuration");
}

// cleanup-Function
static void plane_max_prop_cleanup(void)
{
	PI_DEBUG (DBG_L2, "plane_max_prop Plugin Cleanup");
}

/*
 * higher Math-Routines
 * from: Greg P. Kochanski
 */

#define F1	6
#define F2	12
#define F3	5
#define DFAC 2
#define WEIGHTFAC	3
#define EPS 1

/* Confine a to the range
 * defined by b and c.
 */
#define fbound(a,b,c)					    \
    {							    \
	double uttemp;				    \
							    \
	if ((a) < (uttemp = (b)) || (a) > (uttemp = (c)))   \
	    (a) = uttemp;				    \
    }

#define DATA double
#define LONGDATA double
#define INT int

struct mode_out	{double low, high, best; };

int random_number(int);

#define NRAND	97

#ifndef RAND_MAX
# define RAND_MAX	32767
#endif

int random_number(int x)
{
	static gboolean initted = FALSE;
	static int v[NRAND];
	static unsigned vv;

	if (x<0) x=0;

	if(!initted)	{
		int i;
		initted = TRUE;
		for(i=0;i<NRAND;i++)
			(void)rand();
		for(i=0;i<NRAND;i++)
			v[i] = rand();
		vv = rand();
	}
	{
		const unsigned y = vv%NRAND;
		vv = v[ y ];
		v[y] = rand();
	}

	{
		const int out = ((long)vv*x)/((long)RAND_MAX+1);

		return ((out>0)?out:0);
	}
}


gboolean mode(DATA *d, int n, int stop, struct mode_out *rv)
{
	DATA qmin, qmax;
	DATA split1, split2, lastqmax, lastqmin;
	INT low, med, high, nmax, i, delta;
	double tmp;


	for(i=0,qmin=qmax=d[0];i<n;i++)
		if(d[i] > qmax)	qmax = d[i];
		else if(d[i] < qmin)	qmin = d[i];
	qmax ++;	/* defined as qmin<=d[i]<qmax */
	if((LONGDATA)qmax-qmin < 3*stop)
		qmax = qmin + 3*stop;
 
	while(1)	{
		lastqmax = qmax;	
		lastqmin = qmin;
		split1 = qmin + (qmax-qmin+EPS)/3;
		split2 = qmax - (qmax-qmin+EPS)/3;
   
		for(low=med=high=0,i=0;i<n;i++)	{
			if(d[i]>=qmin && d[i]<split1)
				low++;
			else if(d[i]>=split1 && d[i]<split2)
				med++;
			else if(d[i]>=split2 && d[i]<qmax)
				high++;
		}
   
   
		/* adjust for cases where bins are of different lengths. */
		low = (INT)(((DATA)low * (qmax-qmin))/(3*(split1-qmin)));
		med = (INT)(((DATA)med * (qmax-qmin))/(3*(split2-split1)));
		high = (INT)(((DATA)high * (qmax-qmin))/(3*(qmax-split2)));
   
   
		nmax = low>med ? (low>high ? low : high) : (med>high ? med : high);
		delta = (INT)(DFAC * sqrt((double)nmax) + 0.5);
   
		if(low+delta<nmax && med+delta<nmax)	{
			qmin = split2 - ((LONGDATA)qmax-qmin+EPS)/F1;
			qmax = qmax + ((LONGDATA)qmax-qmin+EPS)/F3;
		}
		else if(med+delta<nmax && high+delta<nmax)	{
			qmin = qmin - ((LONGDATA)qmax-qmin+EPS)/F3;
			qmax = split1 + ((LONGDATA)qmax-qmin+EPS)/F1;
		}
		else if(low+delta<nmax && high+delta<nmax)	{
			qmin = split1 - ((LONGDATA)qmax-qmin+EPS)/F1;
			qmax = split2 + ((LONGDATA)qmax-qmin+EPS)/F1;
		}
		else if(low+delta<nmax)	{
			qmin = split1 - ((LONGDATA)qmax-qmin+EPS)/F2;
			qmax = qmax + ((LONGDATA)qmax-qmin+EPS)/F2;
		}
		else if(high+delta<nmax)	{
			qmin = qmin - ((LONGDATA)qmax-qmin+EPS)/F2;
			qmax = split2 + ((LONGDATA)qmax-qmin+EPS)/F2;
		}
		else break;
   
		if((LONGDATA)qmax-qmin<3*stop && (LONGDATA)lastqmax-lastqmin>=4*stop) {
			if(lastqmax-qmax>qmin-lastqmin)
				qmax = qmin+3*stop;
			else if(qmin-lastqmin>lastqmax-qmax)
				qmin = qmax-3*stop;
			else if((LONGDATA)lastqmax-lastqmin>=qmax-qmin+3*stop)
			{qmax += stop; qmin -= stop;}
			else if(qmax-qmin >= 2*stop)
				qmax += stop;
			else 	{
				qmax = lastqmax;	qmin = lastqmin;
				break;
			}
		}
		else if((LONGDATA)qmax-qmin < 3*stop)	{
			qmax = lastqmax;	qmin = lastqmin;
			break;
		}
	}
 
	tmp = WEIGHTFAC * delta/(double)DFAC;
	split1 = qmin + (qmax-qmin+EPS)/3;
	split2 = qmax - (qmax-qmin+EPS)/3;
 
	rv->best = (((LONGDATA)qmin+split1-EPS)*exp((low-nmax)/tmp) +
		    ((LONGDATA)split1+split2-EPS)* exp((med-nmax)/tmp) +
		    ((LONGDATA)split2+qmax-EPS)*exp((high-nmax)/tmp)) /
		(2.0*(exp((low-nmax)/tmp) +
		      exp((med-nmax)/tmp) +
		      exp((high-nmax)/tmp)));
	rv->low = ((LONGDATA)qmin+split1-EPS)/2.0;
	rv->high = ((LONGDATA)split2+qmax-EPS)/2.0;
 
	return (TRUE);
}


#define SAMPLE	10000

gboolean deltamode(Scan *Src, int m, int n, int dm, int dn, 
	       struct mode_out *answer)
{
	int l, sample;
	DATA *ds;
 
	sample = (long)m*n<SAMPLE ? m*n : SAMPLE;
	ds = (DATA *)malloc(sizeof(*ds) * sample);

	for(l=0;l<sample;l++)	{
		int i, j;
		i = random_number(m-abs(dm))+(dm>0 ? 0 : -dm);
		j = random_number(n-abs(dn))+(dn>0 ? 0 : -dn);
		ds[l] = Src->mem2d->GetDataPkt (j+dn, i+dm) - Src->mem2d->GetDataPkt (j, i);
	}
	if ( (l > sample) || (l<1))	{ 
		return (FALSE);
	}
	mode(ds, l, 1, answer);
	free((void *)ds);
	return (TRUE);
}



#define START	8
/* maximum probability plane */


// run-Function
static gboolean plane_max_prop_run(Scan *Src, Scan *Dest)
{
	struct mode_out x1, xy1, y1, yx1;
	int i, j, delta, n, m;
	double xs, ys, xt, yt, xmi, xma, ymi, yma, xtu, ytu, xu, yu;
	double tosub;

	m = Dest->mem2d->GetNy (); // # lines
	n = Dest->mem2d->GetNx (); // # rows
	xmi = xma = ymi = yma = xs = ys = 0.0;
	for(delta=START; delta<m/2 && delta<n/2; delta*=2)	{
		if (!deltamode(Src, m, n, delta, 0, &x1))
			return(MATH_UNDEFINED);
		if (!deltamode(Src, m, n, 0, delta, &y1))
			return(MATH_UNDEFINED);
		if (!deltamode(Src, m, n, delta, delta, &xy1))
			return(MATH_UNDEFINED);
		if (!deltamode(Src, m, n, delta, -delta, &yx1))
			return(MATH_UNDEFINED);
  
		xt = (xy1.best + yx1.best)/(2.0*delta);
		yt = (xy1.best - yx1.best)/(2.0*delta);
		xtu = (xy1.high+yx1.high-xy1.low-yx1.low)/2.0;
		ytu = (xy1.high-xy1.low-yx1.high+yx1.low)/2.0;
		xu = (x1.high-x1.low+xtu)/(2.0*delta);
		yu = (y1.high-y1.low+ytu)/(2.0*delta);
		x1.best /= (double)delta;
		y1.best /= (double)delta;

		if(delta > START) {
			if(((xt>xma || xt<xmi) && (x1.best>xma || x1.best<xmi)) ||
			   ((yt>yma || yt<ymi) && (y1.best>yma || y1.best<ymi)))
				break;
			fbound(xt, xmi, xma);
			fbound(yt, ymi, yma);
			fbound(x1.best, xmi, xma);
			fbound(y1.best, ymi, yma);
			xs = (x1.best + xt)/2.0;
			ys = (y1.best + yt)/2.0;
	  
			xmi = MAX(xmi, xs-0.60*xu);
			xma = MIN(xma, xs+0.60*xu);
			ymi = MAX(ymi, ys-0.60*yu);
			yma = MIN(yma, ys+0.60*yu);
		}
		else	{
			xs = (x1.best + xt)/2.0;
			ys = (y1.best + yt)/2.0;
			xmi = xs - 0.60*xu;
			xma = xs + 0.60*xu;
			ymi = ys - 0.60*yu;
			yma = ys + 0.60*yu;
		}
	}

	for(i=0; i<m; i++) 
		for(j=0; j<n; j++)
			Dest->mem2d->PutDataPkt (Src->mem2d->GetDataPkt (j, i) - i*ys - j*xs, j, i);

//		for(j=0, tosub=i*xs; j<n; j++, tosub+=ys)
	

	double max, min, off;
	Dest->mem2d->HiLo(&max, &min);

	off=(max+min)/2.;
	for(i=0;i<m;i++)
		for(j=0;j<n;j++) 
			Dest->mem2d->data->Zadd(-off, j, i);

	return MATH_OK;
}

