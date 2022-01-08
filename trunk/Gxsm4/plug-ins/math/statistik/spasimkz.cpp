/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: spasimkz.C
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
% PlugInDocuCaption: SPA--LEED 1D profile simulation $k_z$.
% PlugInName: spasimkz
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Statistics/SPALEED Simkz

% PlugInDescription
Calculates SPA--LEED Profiles over phase (S) e.g. a $k_z$-plot:\\
For each phase S (S=2 StepHeight/WaveLength) a 1D fourier
transformation is calculated for all image lines, which are phase
transformed before. All transformed lines for this phase are summed up
and stored to the new image as Line 'S'.

Def. Phase Transformation:
\[e^{2\pi i S Z(x,y)}\]

%\clearpage
Algorithm (shortened, extracted from source):

\begin{verbatim}
 for(i=0, S=PhaseStart; i<Dest->data.s.ny; S+=PhaseStep, ++i){
    // PhaseTrans:
    // transform scan data to complex data with correct phase
    double sf = 2. * M_PI * S * Src->data.s.dz / StepHeight;
    for (int line=0; line < Src->mem2d->GetNy(); line++) {
        Src->mem2d->data->SetPtr(0, line);
        for (int col=0; col < Src->mem2d->GetNx(); col++) {
            double arg = sf * Src->mem2d->data->GetNext();
            c_re(htrans[col]) = cos(arg);
            c_im(htrans[col]) = sin(arg);
        }

      // do FFT
      fftw( plan, 1, htrans, 1, 0, hkspc, 1, 0);

      // StoreAbsolute, Add to Dest [double]
      Dest->mem2d->data->SetPtr(0, i);
      for (int j = 0; j<Src->mem2d->GetNx(); ++j){
          int k=QSWP(j, Dest->mem2d->GetNx());
          Dest->mem2d->data->SetNext( 
              Dest->mem2d->data->GetThis()
              + c_re(hkspc[k])*c_re(hkspc[k]) 
              + c_im(hkspc[k])*c_im(hkspc[k])
                                    );
      }
    }
  }
\end{verbatim}


% PlugInUsage
Call from \GxsmMenu{Math/Statistics/SPALEED Simkz.} and input the step
height, phase range and phase step size.

% OptPlugInSources
The active channel is used as data source.

%% OptPlugInObjects
%A optional rectangle is used for data extraction...

% OptPlugInDest
The computation result is placed into an existing math channel, else into a new created math channel.

% OptPlugInConfig
Use the Plug-In configurator to set default values. Use the entry
\GxsmEmph{Ask Next} to prevent or reenable further asking for
parameters (1 will ask, 0 not).

%% OptPlugInFiles
%Does it uses, needs, creates any files? Put info here!

%% OptPlugInKnownBugs
%

%% OptPlugInNotes
%

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

static void spasimkz_init( void );
static void spasimkz_about( void );
static void spasimkz_configure( void );
static void spasimkz_cleanup( void );
static gboolean spasimkz_run( Scan *Src, Scan *Dest );

GxsmPlugin spasimkz_pi = {
        NULL,
        NULL,
        0,
        NULL,
        "SpasimKZ-M1S-ST",
        NULL, //"-SPALEED -ELSLEED",
        NULL,
        "Percy Zahl",
        "math-statistics-section",
        N_("SPALEED Simkz"),
        N_("Simulate SPALEED, kz-Scan!"),
        "no more info",
        NULL,
        NULL,
        spasimkz_init,
        NULL,
        spasimkz_about,
        spasimkz_configure,
        NULL,
        NULL,
        NULL,
        spasimkz_cleanup
};

GxsmMathOneSrcPlugin spasimkz_m1s_pi = {
        spasimkz_run
};

static const char *about_text = N_("Gxsm Spasimkz Plugin\n\n"
                                   "Calculates SPALEED Profiles over S\n"
                                   "e.g. a kz-plot:\n"
                                   "For each phase S (S=2 StepHeight/WaveLength)\n"
                                   "a 1D FT is done for all before Phase Transformed\n"
                                   "Lines. All transformed lines for this phase\n"
                                   "are summed up and stored to the new image as Line 'S'.");

double StepHeight=3.141;
double PhaseStart=0.5, PhaseEnd=3.5, PhaseStep=0.01;
int    ask=1;

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
        spasimkz_pi.description = g_strdup_printf(N_("Gxsm MathOneArg spasimkz plugin %s"), VERSION);
        return &spasimkz_pi; 
}

GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) { 
        return &spasimkz_m1s_pi; 
}

static void spasimkz_init(void)
{
        PI_DEBUG (DBG_L2, "Spasimkz Plugin Init");
}

static void spasimkz_about(void)
{
        const gchar *authors[] = { spasimkz_pi.authors, NULL};
        gtk_show_about_dialog (NULL, 
                               "program-name",  spasimkz_pi.name,
                               "version", VERSION,
                               "license", GTK_LICENSE_GPL_3_0,
                               "comments", about_text,
                               "authors", authors,
                               NULL
                               );
}

static void spasimkz_configure(void)
{
        GtkDialogFlags flags =  (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT);
	GtkWidget *dialog = gtk_dialog_new_with_buttons (N_("SPA-LEED kz-Scan simulation parameters"),
							 GTK_WINDOW (main_get_gapp()->get_app_window ()),
							 flags,
							 _("_OK"),
							 GTK_RESPONSE_ACCEPT,
							 _("_Cancel"),
							 GTK_RESPONSE_REJECT,
							 NULL);
	BuildParam bp;
	gtk_box_append (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);

	bp.grid_add_label ("SPA-LEED kz-Scan simulation parameters"); bp.new_line ();
        bp.grid_add_ec ("Step Height", spasimkz_pi.app->xsm->Z_Unit, 
                        &StepHeight, 
                        0., 1000., ".3f");
        bp.new_line ();

        bp.grid_add_ec ("Phase Start", spasimkz_pi.app->xsm->Unity, 
                        &PhaseStart,
                        -1., 100., ".3f");

        bp.grid_add_ec ("Phase End", spasimkz_pi.app->xsm->Unity, 
                        &PhaseEnd,
                        0., 100., ".3f");

        bp.grid_add_ec("Phase Step", spasimkz_pi.app->xsm->Unity, 
                       &PhaseStep,
                       1e-4, 1., ".4f");

        bp.grid_add_ec ("Ask next [1,0]", spasimkz_pi.app->xsm->Unity, 
                        //                           "only True=1 or False=0 !",
                        &ask,
                        0., 1., ".0f");


        gtk_widget_show (dialog);
        int response = GTK_RESPONSE_NONE;
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (GnomeAppService::on_dialog_response_to_user_data), &response);
        
        // FIX-ME GTK4 ??
        // wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

        return response == GTK_RESPONSE_OK;
} 

static void spasimkz_cleanup(void)
{
        PI_DEBUG (DBG_L2, "Spasimkz Plugin Cleanup");
}

static gboolean spasimkz_run(Scan *Src, Scan *Dest)
{
        double S;
        int i;

        // SPA-LEED image simulation
        PI_DEBUG (DBG_L2, "F2D Spasimkz");

        if(ask){
                spasimkz_configure();
        }while(PhaseEnd < PhaseStart || (PhaseEnd - PhaseStart) < PhaseStep );

        if(PhaseEnd < PhaseStart || (PhaseEnd - PhaseStart) < PhaseStep )
                return MATH_SIZEERR;

        // allocate memory for the complex data and one line buffer
        Dest->data.s.ny = (int)((PhaseEnd - PhaseStart)/PhaseStep+0.5);

        Dest->mem2d->Resize(Dest->data.s.nx, Dest->data.s.ny, ZD_FLOAT);
        Dest->mem2d->data->MkXLookup(-100., 100.);
        Dest->mem2d->data->MkYLookup(PhaseStart, PhaseEnd);

        fftw_complex *htrans = new fftw_complex[Src->mem2d->GetNx()];
        fftw_complex *hkspc  = new fftw_complex[Src->mem2d->GetNx()];
        fftw_plan plan = fftw_plan_dft_1d (Src->mem2d->GetNx(), htrans, hkspc, FFTW_FORWARD, FFTW_ESTIMATE);

        PI_DEBUG (DBG_L2, "SIMKZ START: " << PhaseStart << " ..[" << PhaseStep << "].. " << PhaseEnd );

        for(i=0, S=PhaseStart; i<Dest->data.s.ny; S+=PhaseStep, ++i){
                gchar *mld = g_strdup_printf("SIMKZ: %d/%d S=%5.2f", 
                                             i, Dest->data.s.ny, S);
                main_get_gapp()->SetStatus(mld); 
                g_free(mld);
                SET_PROGRESS((gfloat)i/(gfloat)Dest->data.s.ny);

                // PhaseTrans:
                // transform scan data to complex data with correct phase
                double sf = 2. * M_PI * S * Src->data.s.dz / StepHeight;
                for (int line=0; line < Src->mem2d->GetNy(); line++) {
                        Src->mem2d->data->SetPtr(0, line);
                        for (int col=0; col < Src->mem2d->GetNx(); col++) {
                                double arg = sf * Src->mem2d->data->GetNext();
                                c_re(htrans[col]) = cos(arg);
                                c_im(htrans[col]) = sin(arg);
                        }

                        // FFTW
                        fftw_execute (plan);

                        // StoreAbsolute, Add to Dest [double]
                        Dest->mem2d->data->SetPtr(0, i);
                        for (int j = 0; j<Src->mem2d->GetNx(); ++j){
                                int k=QSWP(j, Dest->mem2d->GetNx());
                                Dest->mem2d->data->SetNext( Dest->mem2d->data->GetThis()
                                                            + c_re(hkspc[k])*c_re(hkspc[k]) 
                                                            + c_im(hkspc[k])*c_im(hkspc[k])
                                                            );
                        }
                }
        }

        fftw_destroy_plan (plan);
        SET_PROGRESS(0);

        // free memory
        delete htrans;
        delete hkspc;

        return MATH_OK;
}
