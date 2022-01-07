/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: smallconvol.C
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
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: Convolution with 3x3 kernel
% PlugInName: smallconvol
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: Math/Filter 2D/Small Convol

% PlugInDescription
\label{plugin:smallconvol}
This plug-in convolutes the active scan with a 3x3 matrix (kernel).  The
kernel can be changed using \GxsmMenu{Tools/Plugin Details} by calling the
plug-ins configure function (c.f.\ \ref{plugin:listplugins}).

% PlugInUsage
The \GxsmEmph{smallconvol} plug-in can be found in \GxsmMenu{Math/Filter
2D/Small Convol}.  It acts on the active channel and the output is put in
the math channel.

%% OptPlugInKnownBugs
%No known.

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"


static void smallconvol_init( void );
static void smallconvol_about( void );
static void smallconvol_configure( void );
static void smallconvol_cleanup( void );
static gboolean smallconvol_run( Scan *Src, Scan *Dest );

GxsmPlugin smallconvol_pi = {
	NULL,
	NULL,
	0,
	NULL,
	"Smallconvol-M1S-F1D",
	NULL,
	NULL,
	"Percy Zahl",
	"math-filter2d-section",
	N_("Small Convol"),
	N_("Sorry, no help for smallconvol filter!"),
	"no more info",
	NULL,
	NULL,
	smallconvol_init,
	NULL,
	smallconvol_about,
	smallconvol_configure,
	NULL,
	NULL,
	NULL,
	smallconvol_cleanup
};

GxsmMathOneSrcPlugin smallconvol_m1s_pi = {
	smallconvol_run
};

static const char *about_text = N_("Gxsm Smallconvol Plugin\n\n"
                                   "Matrix Convolution PlugIn.");

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	smallconvol_pi.description = g_strdup_printf(N_("Gxsm MathOneArg smallconvol plugin %s"), VERSION);
	return &smallconvol_pi; 
}

GxsmMathOneSrcPlugin *get_gxsm_math_one_src_for_all_vt_plugin_info( void ) { 
	return &smallconvol_m1s_pi; 
}

// Filter Code...

#define SET_SMC_PROGRESS(P) { smallconvol_pi.app->SetProgress((gfloat)(P)); while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE); }

double kernel[3][3];

static void smallconvol_init(void)
{
	PI_DEBUG (DBG_L2, "Smallconvol Plugin Init");
	// init kernel:
	kernel[0][0] = 1.0;
	kernel[0][1] = 1.0;
	kernel[0][2] = 1.0;
	kernel[1][0] = 1.0;
	kernel[1][1] = 1.0;
	kernel[1][2] = 1.0;
	kernel[2][0] = 1.0;
	kernel[2][1] = 1.0;
	kernel[2][2] = 1.0;
}

static void smallconvol_about(void)
{
	const gchar *authors[] = { smallconvol_pi.authors, NULL};
	gtk_show_about_dialog (NULL, 
			       "program-name",  smallconvol_pi.name,
			       "version", VERSION,
			       "license", GTK_LICENSE_GPL_3_0,
			       "comments", about_text,
			       "authors", authors,
			       NULL
			       );
}

static void
on_dialog_response (GtkDialog *dialog,
		    int        response,
		    gpointer   user_data)
{
        *((int *) user_data) = response;
        gtk_window_destroy (GTK_WINDOW (dialog));
}

static void smallconvol_configure(void)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *info;
	GtkWidget *input;
	GtkDialogFlags flags = GTK_DIALOG_MODAL; // | GTK_DIALOG_DESTROY_WITH_PARENT;
 
	dialog = gtk_dialog_new_with_buttons ("Small Convol Kernel Setup",
					      NULL,
					      flags,
					      N_("_OK"),
					      GTK_RESPONSE_OK,
					      N_("_CANCEL"),
					      GTK_RESPONSE_CANCEL,
					      NULL);
  
	BuildParam bp;
	gtk_box_append(GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), bp.grid);
	bp.grid_add_label ("3x3 Kernel");
	bp.new_line ();

	for (int i=0; i<3; ++i){
		gchar *l = g_strdup_printf ("%d", i+1);
		for (int j=0; j<3; ++j){
			bp.grid_add_ec (j==0? l:NULL, smallconvol_pi.app->xsm->Unity, &kernel[i][j], -999, 999, ".0f");
		}
		g_free (l);
		bp.new_line ();
	}

        gtk_widget_show (dialog);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

        int response = GTK_RESPONSE_NONE;
        g_signal_connect (dialog, "response", G_CALLBACK (on_dialog_response), &response);

        // wait here on response
        while (response == GTK_RESPONSE_NONE)
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

}

static void smallconvol_cleanup(void)
{
	PI_DEBUG (DBG_L2, "Smallconvol Plugin Cleanup");
}

// Do 2D Convolution with 3x3 kernel --------------------------------------
static gboolean smallconvol_run(Scan *Src, Scan *Dest)
{
	int line, col;
	ZData *src;

	if (!Src || !Dest)
		return 0;

	// new dest size:
	// (the scan size is cropped by one line at each side of the scan
	//  to get rid of border Problems during the convolution)
	Dest->mem2d->Resize (Src->mem2d->GetNx () - 2, Src->mem2d->GetNy () - 2);
	// new, but to be obsoleted next
	Dest->data.s.nx = Dest->mem2d->GetNx ();
	Dest->data.s.ny = Dest->mem2d->GetNy ();

	// compute normalization factors:
	double norm   = 1. / (kernel[0][0] + kernel[0][1] + kernel[0][2] +
			      kernel[1][0] + kernel[1][1] + kernel[1][2] +
			      kernel[2][0] + kernel[2][1] + kernel[2][2]);

	PI_DEBUG (DBG_L2, "Norm of Kernel is " << norm);
  
	// Get help object... for fast access access to surrounding data
	src  = Src->mem2d->data;
	for (line = 0; line < Dest->mem2d->GetNy(); ++line) {
		if(!(line%32)) SET_SMC_PROGRESS( line / Dest->mem2d->GetNy() );
		src->SetPtrTB(1, line+1); // set to x=1 of second line
		for (col = 0; col < Dest->mem2d->GetNx(); ++col) {
			Dest->mem2d->PutDataPkt
				( norm * 
				  ( kernel[0][0] * src->GetThisLT() + // access This-Left-Top
				    kernel[1][0] * src->GetThisT() + 
				    kernel[2][0] * src->GetThisRT() + 
	    
				    kernel[0][1] * src->GetThisL() + 
				    kernel[1][1] * src->GetThis() + 
				    kernel[2][1] * src->GetThisR() + 

				    kernel[0][2] * src->GetThisLB() + 
				    kernel[1][2] * src->GetThisB() + 
				    kernel[2][2] * src->GetThisRB() 
				    ), col, line );
			src->IncPtrTB(); // go to next x ...
		}
	}
	SET_SMC_PROGRESS( 0. );
	return MATH_OK;
}
