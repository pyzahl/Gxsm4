/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * additional features: Andreas Klust <klust@users.sf.net>
 * WWW Home: http://gxsm.sf.net
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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

#ifndef XSMMATH__H
#define XSMMATH__H

#include <config.h>

# include <complex>
# include <fftw3.h>
#define c_re(c) ((c)[0])
#define c_im(c) ((c)[1])


/* Math Errors */

#define MATH_OK           0
#define MATH_SIZEERR      1
#define MATH_SELECTIONERR 2
#define MATH_DIVZERO      3
#define MATH_UNDEFINED    4
#define MATH_NOMEM        5
#define MATH_FILE_ERROR   6
#define MATH_LIB_ERR      7

/* für alle Math Funktionen diese Argumente !! */
#define MATHOPPARAMSNODEST  Scan *Src
#define MATHOPVARSNODEST    Src

#define MATHOPPARAMDONLY    Scan *Dest
#define MATHOPPARAMS        Scan *Src, Scan *Dest
#define MATHOPVARS          Src, Dest

#define MATH2OPPARAMS       Scan *Src1, Scan *Src2, Scan *Dest
#define MATH2OPVARS         Src1, Src2, Dest


#include "scan.h"

// used to set progressbar of main window (P: 0..1)
#define SET_PROGRESS(P) { main_get_gapp ()->SetProgress((gfloat)(P)); while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE); }

#define SHTRANGE   32765
#define ZEROVALUE  0.

typedef struct {
  int xLeft, xRight;
  int yBottom, yTop;
  int xSize, ySize;
  double xRatio, yRatio;
  double Aspect;
  double Area;
  double Radius2;            // circle r^2
  double xCenter, yCenter;   // and middle point coords.
} MOUSERECT;

extern void MkMausSelectP (Point2D *pkt2d, MOUSERECT *msel, int mx, int my);
extern gint MkMausSelect (Scan *sc, MOUSERECT *msel, int mx, int my);
gboolean F2D_ift_ft(MATH2OPPARAMS, gboolean (*spkfkt)(MATH2OPPARAMS, fftw_complex *dat));


/* Edit/Cut/Crop */
extern gboolean CopyScan(MATHOPPARAMS);
extern gboolean CropScan(MATHOPPARAMS);
extern gboolean ZoomInScan(MATHOPPARAMS);
extern gboolean ZoomOutScan(MATHOPPARAMS);
extern gboolean StitchScans(MATHOPPARAMS);


/* Background */
extern gboolean BgLin1DScan(MATHOPPARAMS);
extern gboolean BgParabolRegress(MATHOPPARAMS);
extern gboolean BgERegress(MATHOPPARAMS);

/* Filter 1D */
extern gboolean F1D_Despike(MATHOPPARAMS);
extern gboolean F1D_PowerSpec(MATHOPPARAMS);
extern gboolean F1D_FT_Window(MATH2OPPARAMS);
extern gboolean F1D_FT_GaussStop(MATH2OPPARAMS);
extern gboolean F1D_FT_GaussPass(MATH2OPPARAMS);
extern gboolean F1D_ydiff(MATHOPPARAMS);


/* Filter 2D */
extern gboolean F2D_Despike(MATHOPPARAMS);
extern gboolean F2D_RemoveRect(MATHOPPARAMS);
extern gboolean F2D_LineInterpol(MATHOPPARAMS);
extern gboolean F2D_LogPowerSpec(MATHOPPARAMS);
extern gboolean F2D_AutoCorr(MATHOPPARAMS);
extern gboolean F2D_iftXft(MATH2OPPARAMS);
extern gboolean F2D_FT_GaussStop(MATH2OPPARAMS);
extern gboolean F2D_FT_GaussPass(MATH2OPPARAMS);

/* Transformations */
extern gboolean TR_QuenchScan(MATHOPPARAMS); /* needed by core, has to remain here! */

gboolean SpkWindow(MATH2OPPARAMS, fftw_complex*);
gboolean SpkGaussStop(MATH2OPPARAMS, fftw_complex*);
gboolean SpkGaussPass(MATH2OPPARAMS, fftw_complex*);
gboolean SpkAutoCorr(MATH2OPPARAMS, fftw_complex*);

/* Statistics */
// all Plugins now...


// Macro for fft quadrant swap
#define QSWP(X,N) ((X) >= ((N)/2) ? ((X)-(N)/2) : ((X)+(N)/2))

// virtual base class for filters with special parameters
class Filter
{
public:
	virtual ~Filter() {};

        virtual void hide() {};
	virtual void draw() {};
	
	virtual gboolean exec(MATHOPPARAMS) {return MATH_OK;};
};

#endif
