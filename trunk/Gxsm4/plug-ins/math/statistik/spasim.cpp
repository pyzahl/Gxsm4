/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: spasim.C
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
% PlugInDocuCaption: SPA--LEED simulation
% PlugInName: spasim
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Statistik/SPALEED Sim.

% PlugInDescription
 This Plugin simulates a SPA-LEED measurement (asks for the electron
 wave lenght $\lambda$ as input) using the following transformation:

\begin{enumerate}
\item phase transformation, using 
  \[e^\frac{2\pi i Z(x,y)}{\lambda}\]
\item 2 dim. fourier transformation of the phase transformed image
\item the resulting intensity is stored: $|FT(\text{phase trans. image})|^2$
\end{enumerate}

The resulting scan is scaled to have a size of $\pm100$ (full width is
one. To automatically calculate realspace dimensions $\Gamma$ from
inverse of spot separation (e.g. from center (0,0) to some spot or
lenght of the line object) enable \GxsmEmph{InvAng} with the PlugIn configurator!
By default coordinates in pixels  are used (center is (0,0)).

\GxsmNote{Remember: One pixel distance (e.g. in X) corresponds to the
full width of the original picture!}

% PlugInUsage
Call from \GxsmMenu{Math/Statistics/SPALEED Sim.} and input $\lambda$
in Angstroems.

% OptPlugInSources
The active channel is used as data source.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

% OptPlugInDest
The computation result is placed into an existing math channel, else into a new created math channel.

% OptPlugInConfig
Using the Plug-In configurator you can preset the wave lenght $\lambda$.

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

% OptPlugInKnownBugs
The show-line object did not work with the \GxsmEmph{InvAng} setting
-- sorry, but you can still save the data, but it will not show any
profile.

%% OptPlugInNotes

% OptPlugInHints
You may want to carefully background correct you image before! Check
also for correct step heights, if applicable.


% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"

static void spasim_init( void );
static void spasim_about( void );
static void spasim_configure( void );
static void spasim_cleanup( void );
static gboolean spasim_run( Scan *Src, Scan *Dest );

GxsmPlugin spasim_pi = {
	NULL,
	NULL,
	0,
	NULL,
	"Spasim-M1S-F1D",
	NULL, //"-SPALEED -ELSLEED",
	NULL,
	"Percy Zahl",
	"math-statistics-section",
	N_("SPALEED Sim."),
	N_("Simulate SPALEED!"),
	"no more info",
	NULL,
	NULL,
	spasim_init,
	NULL,
	spasim_about,
	spasim_configure,
	NULL,
	NULL,
	NULL,
	spasim_cleanup
};

GxsmMathOneSrcPlugin spasim_m1s_pi = {
	spasim_run
};

static const char *about_text = N_("Gxsm Spasim Plugin\n\n"
                                   "This Plugin simulates a SPA-LEED measurement (WaveLenght as input)\n"
                                   "on the Src-Channel's Scan:\n"
                                   "1.) Phase Transformation, using e^(2pi i Z(x,y)/WaveLength)\n"
                                   "2.) 2D FT( Phase Transformed Image)\n"
                                   "3.) Result = |FT(\")|^2");

double WaveLengthDefault=0.0;
int EnableInvAng=0;
double WaveLengthLast=3.141;

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	spasim_pi.description = g_strdup_printf(N_("Gxsm MathOneArg spasim plugin %s"), VERSION);
	return &spasim_pi; 
}

GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) { 
	return &spasim_m1s_pi; 
}

static void spasim_init(void)
{
	PI_DEBUG (DBG_L2, "Spasim Plugin Init");
}

static void spasim_about(void)
{
	const gchar *authors[] = { spasim_pi.authors, NULL};
	gtk_show_about_dialog (NULL,
			       "program-name",  spasim_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

static void spasim_configure(void)
{
	double flg;
	spasim_pi.app->ValueRequest("Enter Value", "WaveLength", 
				    "Default WaveLength, if set to Zero I will ask later!",
				    spasim_pi.app->xsm->Z_Unit, 
				    0., 1000., ".3f", &WaveLengthDefault);
	flg = EnableInvAng;
	spasim_pi.app->ValueRequest("Configure", "Enable InvAng (0/1)?", 
				    "Use inv Ang for lenght scale?\n"
				    "(Note: sorry, but profile won't work w this!)",
				    spasim_pi.app->xsm->Unity, 
				    0., 1., ".0f", &flg);
	EnableInvAng=flg>0?1:0;
}

static void spasim_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Spasim Plugin Cleanup");
}

static gboolean spasim_run(Scan *Src, Scan *Dest)
{
	double WaveLength = WaveLengthLast;

	if(fabs(WaveLengthDefault) > 1e-10)
		WaveLength = WaveLengthDefault;
	else
		spasim_pi.app->ValueRequest("Enter Value", "WaveLength", 
					    "I need WaveLength for SpaSim!",
					    spasim_pi.app->xsm->Z_Unit, 
					    1e-3, 1e3, ".3f", &WaveLength);
	WaveLengthLast = WaveLength;

// SPA-LEED image simulation
	PI_DEBUG (DBG_L2, "F2D SpaSim");

	// Set Dest to Float, Range: +/-100% =^= One Pixel
	Dest->mem2d->Resize(Dest->data.s.nx, Dest->data.s.ny, ZD_FLOAT);
//	Dest->mem2d->data->MkXLookup(-100., 100.);
//	Dest->mem2d->data->MkYLookup(-100., 100.);
	Dest->mem2d->data->MkXLookup(-Src->mem2d->GetNx()/2, Src->mem2d->GetNx()/2);
	Dest->mem2d->data->MkYLookup(-Src->mem2d->GetNy()/2, Src->mem2d->GetNy()/2);
	if (EnableInvAng){
		InvUnit ux("invA","invA", "X Lenght", Src->data.s.rx);
		InvUnit uy("invA","invA", "Y Lenght", Src->data.s.ry);
		Dest->data.SetXUnit(&ux);
		Dest->data.SetYUnit(&uy);
	}else{
		LinUnit ux("pix","pix", "X pos");
		LinUnit uy("pix","pix", "Y pos");
		Dest->data.SetXUnit(&ux);
		Dest->data.SetYUnit(&uy);
	}
	LinUnit lu("CPS","CPS", "Int.");
	Dest->data.SetZUnit(&lu);

	// allocate memory for the complex data and one line buffer
	fftw_complex *in  = new fftw_complex [Src->mem2d->GetNy() * Src->mem2d->GetNx()];
	fftw_complex *out = new fftw_complex [Src->mem2d->GetNy() * Src->mem2d->GetNx()];
	// uses row-major format:
	// pos = Src->mem2d->GetNy ()*col + line;

	// transform scan data to complex data with correct phase
	for (int pos=0, col=0; col < Src->mem2d->GetNx (); ++col)
		for (int line=0; line < Src->mem2d->GetNy (); ++line, ++pos){
			double arg = 2.*M_PI * Src->data.s.dz 
				* Src->mem2d->GetDataPkt(col, line) 
				/ WaveLength;
			c_re(in[pos]) = cos(arg);
			c_im(in[pos]) = sin(arg);
		}

	// do the fourier transform
	// create plan for fft
	fftw_plan plan = fftw_plan_dft_2d (Src->mem2d->GetNx (), Src->mem2d->GetNy (), in, out, FFTW_FORWARD, FFTW_ESTIMATE);
	// compute fft
	fftw_execute (plan);

	// take the square value of the complex data and store to Dest [double]
	for (int pos=0, col=0; col < Src->mem2d->GetNx (); ++col)
		for (int line=0; line < Src->mem2d->GetNy (); ++line, ++pos)
			Dest->mem2d->PutDataPkt (  c_re (out[pos]) * c_re (out[pos]) 
						 + c_im (out[pos]) * c_im (out[pos]),
						   QSWP (col, Src->mem2d->GetNx ()), 
						   QSWP (line, Src->mem2d->GetNy ()) );

	// destroy plan
	fftw_destroy_plan (plan);

	// free memory
	delete in;
	delete out;

	return MATH_OK;
}
