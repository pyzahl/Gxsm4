/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Gxsm Plugin Name: histohop.C
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

#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"
#include "glbvars.h"
#include "surface.h"

static void histohop_init( void );
static void histohop_about( void );
static void histohop_configure( void );
static void histohop_cleanup( void );
static gboolean histohop_run( Scan *Src, Scan *Dest );

GxsmPlugin histohop_pi = {
  NULL,
  NULL,
  0,
  NULL,
  "Histohop-M1S-F1D",
  "-spaHARD",
  NULL,
  "HOP, AK, PZ",
  "math-statistics-section",
  N_("Histo HOP"),
  N_("Histo HOP Stat."),
  "no more info",
  NULL,
  NULL,
  histohop_init,
  NULL,
  histohop_about,
  histohop_configure,
  NULL,
  NULL,
  NULL,
  histohop_cleanup
};

GxsmMathOneSrcPlugin histohop_m1s_pi = {
  histohop_run
};

static const char *about_text = N_("Gxsm Histohop Plugin\n\n"
                                   "calculates histohop.");

GxsmPlugin *get_gxsm_plugin_info ( void ){ 
  histohop_pi.description = g_strdup_printf(N_("Gxsm MathOneArg histohop plugin %s"), VERSION);
  return &histohop_pi; 
}

GxsmMathOneSrcPlugin *get_gxsm_math_one_src_plugin_info( void ) { 
  return &histohop_m1s_pi; 
}

static void histohop_init(void)
{
  PI_DEBUG (DBG_L2, "Histohop Plugin Init");
}

static void histohop_about(void)
{
  const gchar *authors[] = { histohop_pi.authors, NULL};
  gtk_show_about_dialog (NULL,
			 "program-name",  histohop_pi.name,
			 "version", VERSION,
			 "license", GTK_LICENSE_GPL_3_0,
			 "comments", about_text,
			 "authors", authors,
			 NULL
			 );
}

static void histohop_configure(void)
{
  if(histohop_pi.app)
    histohop_pi.app->message("Histohop Plugin Configuration");
}

static void histohop_cleanup(void)
{
  PI_DEBUG (DBG_L2, "Histohop Plugin Cleanup");
}

static gboolean histohop_run(Scan *Src, Scan *Dest)
{
  if(histohop_pi.app)
    histohop_pi.app->message("make it alive again!");
  return MATH_OK;
}


#ifdef Who_makes_Histo_HOP_gxsm_conform_qqq


// histoHOP filter
typedef enum {
  F2D_HISTOHOP_PCS_RX, F2D_HISTOHOP_PCS_RY,
  F2D_HISTOHOP_PCS_THRES,
  F2D_HISTOHOP_PCS_ANZAHL
} F2D_HISTOHOP_PCS_LIST;

class F2D_HistoHOP : public Filter
{
friend void histoHOPAction(FL_OBJECT *ob, long data);
 public:
  F2D_HistoHOP();
  ~F2D_HistoHOP();

  void hide();
  void draw();

  gboolean exec(MATHOPPARAMS);

 private:
  double rx, ry, thres;
  char hopFile[256];
  int hopFileOK;
  ofstream *hopStream;
  int detectOnly;          // TRUE: do only step detection
  /*
  FD_HistoHOP *fd_HistoHOP;
  FD_HistoHOP *fd_HistoHOPCpy;
  Param_Control *pcs[F2D_HISTOHOP_PCS_ANZAHL];
  */
};


// F2D_Histohop
// Filter zur Berechnung von Terrassenlängen und Stufenhöhenverteilungen
// war: histoHOP Filter in PMSTM
// entwickelt von Holger Pietsch (HOP), s.a. seine Diss
F2D_HistoHOP::F2D_HistoHOP()
{
  // create form and copy
  fd_HistoHOP = create_form_HistoHOP();
  fd_HistoHOPCpy = new FD_HistoHOP;  // Shadow of Pointers to Objects
  memcpy(fd_HistoHOPCpy, fd_HistoHOP, sizeof(FD_HistoHOP));     // copy Pointers
  fl_set_form_atclose(fd_HistoHOPCpy->HistoHOP, cb_do_close, 0);

  // init private variables
  rx = 1.0; ry = 1.0;
  thres = 10.0;
  *hopFile = '\0';
  hopFileOK = 0;
  
  // init parameter control system:
  pcs[F2D_HISTOHOP_PCS_RX]     = new Param_Control(xsm->Unity,   MLD_WERT_NICHT_OK, &rx, 1., 9., "2.0f");
  pcs[F2D_HISTOHOP_PCS_RY]     = new Param_Control(xsm->Unity,   MLD_WERT_NICHT_OK, &ry, 1., 9., "2.0f");
  pcs[F2D_HISTOHOP_PCS_THRES]  = new Param_Control(xsm->Unity,   MLD_WERT_NICHT_OK, &thres, -999., 999., "3.1f");

  pcs[F2D_HISTOHOP_PCS_RX]->Put_Value(fd_HistoHOPCpy->kern_rx);
  pcs[F2D_HISTOHOP_PCS_RY]->Put_Value(fd_HistoHOPCpy->kern_ry);
  pcs[F2D_HISTOHOP_PCS_THRES]->Put_Value(fd_HistoHOPCpy->threshold);
}

F2D_HistoHOP::~F2D_HistoHOP()
{
  delete fd_HistoHOPCpy;

  for (int i = 0; i < F2D_HISTOHOP_PCS_ANZAHL; i++)
    if (pcs[i]) delete pcs[i];
}

void F2D_HistoHOP::hide()
{
  fl_hide_form(fd_HistoHOPCpy->HistoHOP);
}

void F2D_HistoHOP::draw()
{
  fl_show_form(fd_HistoHOPCpy->HistoHOP, FL_PLACE_FREE | FL_FIX_SIZE, FL_TRANSIENT, F2D_TITLE);
}

gboolean F2D_HistoHOP::exec(MATHOPPARAMS)
{
  XSMDEBUG("F2D HistoHOP");

  SHT *linebuf = new SHT[Src->mem2d->GetNx()];
  SHT *srcbuf  = new SHT[Src->mem2d->GetNx()];

  // open hopfile for writing
  if (hopFile[0] != '\0') {
    hopStream = new ofstream(hopFile);
    if (!hopStream->good()) {
      hopFileOK = 0;
      // Error message ????
      return MATH_FILE_ERROR;
    }
    else {
      hopFileOK = 1;

      *hopStream << "# histoHOP data file\n";
      *hopStream << "# generated by $Name: not supported by cvs2svn $ $Revision: 1.5 $\n";
      *hopStream << "# \n";
      *hopStream << "# format: facet length, facet height, terrace length, and terrace heigth\n";
      *hopStream << "# (all values in Angstrom)\n";
      *hopStream << "# \n";
    }
  }


  // threshold:
  //* PZ: es wäre schön hier NICHT ...Inst-> verwenden zu müssen... 
  //* PZ: tut´s nicht auch data.s... ?
  SHT lim = (SHT)(main_get_gapp()->xsm->Inst->XA2Dig(1000) / main_get_gapp()->xsm->Inst->ZA2Dig(1000) * thres);

  // compute derivative in x-direction, store derivative in Dest
  // (StatDiff filter)
  int    sx = 1+(int)(rx + .9);
  int    sy = 1+(int)(ry + .9);
  MemDeriveXKrn krn(rx,ry, sx,sy);
  krn.Convolve(Src->mem2d, Dest->mem2d);

  // look for steps
  for (int line=0; line < Dest->mem2d->GetNy(); line++) {
    Dest->mem2d->GetDataLine(line, linebuf);

    for (int col = 0; col < Dest->mem2d->GetNx(); col++)
      if ( abs(*(linebuf+col)) > lim )
	*(linebuf+col) = 1;
      else
	*(linebuf+col) = 0;

    Dest->mem2d->PutDataLine(line, linebuf);
  }

  if (!detectOnly) {

    // compute terraces and facettes
    for (int line=0; line < Dest->mem2d->GetNy(); line++) {

      if (hopFileOK)  *hopStream << "# scan row: " << line << '\n';

      Dest->mem2d->GetDataLine(line, linebuf);
      Src->mem2d->GetDataLine(line, srcbuf);
      int xfs = 0;  // facet start pos
      int xfe = 0;  // facet end pos
      int xte = 0;  // terrace end pos
      int x   = 0;  // current pos

      while (x < Dest->mem2d->GetNx()-1) {
      
	// find facette
	xfs = x;
	while ( *(linebuf+x) && x < Dest->mem2d->GetNx()-1)  x++;
	xfe = x;

	// find terrace
	while ( ! *(linebuf+x) && x < Dest->mem2d->GetNx()-1)  x++;
	xte = x;

	// interpolate data on facet
	for (int col=xfs; col < xfe; col++)
	  *(linebuf+col) = *(srcbuf+xfs) + 
	    (SHT)( (double)(*(srcbuf+xfe)-*(srcbuf+xfs)) / (double)(xfe-xfs) * (col-xfs) );

	// interpolate data on terrace
	for (int col=xfe; col < xte; col++)
	  *(linebuf+col) = *(srcbuf+xfe) + 
	    (SHT)( (double)(*(srcbuf+xte)-*(srcbuf+xfe)) / (double)(xte-xfe) * (col-xfe) );

	// write data to statistic file
	if (hopFileOK) {
	  *hopStream << Src->data.s.dx*(xfe-xfs);                                // facet length
	  *hopStream << "  " << Src->data.s.dz*(*(srcbuf+xfe) - *(srcbuf+xfs));  // facet height
	  *hopStream << "  " << Src->data.s.dx*(xte-xfe);                        // terrace length
	  *hopStream << "  " << Src->data.s.dz*(*(srcbuf+xte) - *(srcbuf+xfe));  // terrace height
	  *hopStream << '\n';
	}

      }
      
      Dest->mem2d->PutDataLine(line, linebuf);
    }

  }  // end if(!detectOnly)

  delete linebuf;
  delete srcbuf;

  if (hopFileOK)  delete hopStream;

  return MATH_OK;
}

gboolean ST_HistoHOP_FD(MATHOPPARAMS)
{
  return xsm->f2D_HistoHOP->exec(Src, Dest);
}

#endif // Histo HOP
