/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: autoalign.C
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

#ifndef __AUTOALIGN_STACK_REG_H
#define __AUTOALIGN_STACK_REG_H


#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"
#include "autoalign_vector_types.h"
#include "autoalign_turbo_reg.h"

/* The math algorithm is based on the following work, prior implemented for ImageJ in JAVA: */

/*====================================================================	
| Version: February 28, 2005
\===================================================================*/

/*====================================================================
| EPFL/STI/IOA/LIB
| Philippe Thevenaz
| Bldg. BM-Ecublens 4.137
| Station 17
| CH-1015 Lausanne VD
| Switzerland
|
| phone (CET): +41(21)693.51.61
| fax: +41(21)693.37.01
| RFC-822: philippe.thevenaz@epfl.ch
| X-400: /C=ch/A=400net/P=switch/O=epfl/S=thevenaz/G=philippe/
| URL: http://bigwww.epfl.ch/
\===================================================================*/

/*====================================================================
| This work is based on the following paper:
|
| P. Thevenaz, U.E. Ruttimann, M. Unser
| A Pyramid Approach to Subpixel Registration Based on Intensity
| IEEE Transactions on Image Processing
| vol. 7, no. 1, pp. 27-41, January 1998.
|
| This paper is available on-line at
| http://bigwww.epfl.ch/publications/thevenaz9801.html
|
| Other relevant on-line publications are available at
| http://bigwww.epfl.ch/publications/
\===================================================================*/

/*====================================================================
| Additional help available at http://bigwww.epfl.ch/thevenaz/turboreg/
|
| You'll be free to use this software for research purposes, but you
| should not redistribute it without our consent. In addition, we expect
| you to include a citation or acknowledgment whenever you present or
| publish results that are based on it.
\===================================================================*/

/*====================================================================
|	StackReg
\===================================================================*/

/********************************************************************/
class StackReg{
public: 
	typedef enum { TARGET_TIME, TARGET_LAYER, TARGET_TIME_LAYER, TARGET_TEST } TARGET_MODE;

	StackReg () { 
		TINY = intBitsToFloat ((int)0x33FFFFFF); 
		for (int job=0; job<4; ++job){
			shiftVectorsLast[job] = new Matrix_d (1, Vector_d (2, 0.));
			shiftVectorsLast_d[job] = new Matrix_d (1, Vector_d (2, 0.));
			last_shift[job] = 0.;
		}
		transformation = REG_INVALID;
		max_shift_allowed = 0;
		debug_lvl = 0;
		stop_flg = false;
	};
	virtual ~StackReg () { 
		for (int job=0; job<4; ++job){
			delete shiftVectorsLast[job];
			delete shiftVectorsLast_d[job]; 
		}
	};
	
	void run(Scan *Src, Scan *Dest);
	void stop () { stop_flg = true; };
        static int cancel_callback (GtkWidget *widget, StackReg *sr) { sr->stop (); return 0; };
	static int about_callback (GtkWidget *widget, gpointer x) { 	
		const gchar *authors[] = { "P. Zahl", "P. Th\303\251venaz", "U.E. Ruttimann", "M. Unser", NULL};
		gtk_show_about_dialog (NULL, 
				       "program-name",  "Multi dimensional image auto-alignment",
				       "version", VERSION,
				       "license", GTK_LICENSE_GPL_3_0,
				       "comment", N_("The GXSM autoalign Plug-In does automatic image alignment for translation, "
						     "scaled rotation, ridig body and affine transformation, of an image series "
						     "in time and layer domain (i.e. for drift correction). "
						     "The underlying algorithm of this code is based on the following paper, "
						     "implemented as GXSM-Plugin, \n"
						     "ported from JAVA to C++, optimized and multithreaded by P. Zahl:\n"
						     "\n"
						     " P. Th\303\251venaz, U.E. Ruttimann, M. Unser\n"
						     " A Pyramid Approach to Subpixel Registration Based on Intensity\n"
						     " IEEE Transactions on Image Processing\n"
						     " vol. 7, no. 1, pp. 27-41, January 1998.\n"
						     "\n and the original GXSM paper\n"
						     " P. Zahl, M. Bierkandt, S. Schr\303\266der, and A. Klust,\n"
						     " Rev. Sci. Instr. 74 (2003) 1222.\n"
						     "\n"
						     "This paper is available on-line at\n"
						     " http://bigwww.epfl.ch/publications/thevenaz9801.html\n"
						     "\n"
						     "Other relevant on-line publications are available at\n"
						     " http://bigwww.epfl.ch/publications/\n"
						     "\n"
						     "Additional help available at\n"
						     " http://bigwww.epfl.ch/thevenaz/stackreg/\n"
						     " http://gxsm.sourceforge.net\n"
						     "\n"
						     "Ancillary TurboReg_ plugin available at\n"
						     "http://bigwww.epfl.ch/thevenaz/turboreg/\n"
						     "\n"
						     "You'll be free to use this software for research purposes, but "
						     "you should not redistribute it without our consent. In addition, "
						     "we expect you to include a citation or acknowledgment of both projects whenever "
						     "you present or publish results that are based on it."
						     "\n"
							  ),
				       "authors", authors,
				       NULL
				       );
		return 0;
	};

	void registerSlice (
		Mem2d* source, int sv,
		Mem2d* target, int tv,
		Mem2d* destination, int dv,
		Matrix_d& globalTransform,
		Matrix_d& anchorPoints,
		Matrix_i& register_region,
		int job=0,
		gboolean apply_to_all_layers=false);

	void registerLayers (Mem2d* source, int sv, Mem2d* destination, int dv,
			     Matrix_d& globalTransform, Matrix_d& anchorPoints, Matrix_i& register_region,
			     gboolean seed_in_place=false, int job=0, int *progress=NULL);


	void reset_shift_vectors(int job=0){
		Vector_d v0(2, 0.);
		for (int k=0; k < shiftVectorsLast[job]->size (); ++k){
			(*(shiftVectorsLast[job]))[k] = v0;
			(*(shiftVectorsLast_d[job]))[k] = v0;
		}
		last_shift[job] = 0.;
	};

private:
	REGISTER_MODE ask_for_transformation_type (TARGET_MODE &target_mode, int &time_target_initial, int &value_target_initial);

	double intBitsToFloat (int bits){

		double s = ((bits >> 31) == 0) ? 1. : -1.;
		int e = ((bits >> 23) & 0xff);
		double m = (double) ((e == 0) ?
				     (bits & 0x7fffff) << 1 :
				     (bits & 0x7fffff) | 0x800000);
		
		return (s*m*pow (2., (double)(e-150)));
	};

	double TINY; // private static double TINY = (double)Float.intBitsToFloat((int)0x33FFFFFF);
	Matrix_d *shiftVectorsLast[4];
	Matrix_d *shiftVectorsLast_d[4];
	double last_shift[4];
	double max_shift_allowed;
	REGISTER_MODE transformation;

	int stop_flg;
	int debug_lvl;
};

#endif










/**** NOT NEEDED ****/




#if 0
	void computeStatistics (Scan* imp, Vector_d& average, Matrix_d& scatterMatrix);
	Vector_d* getColorWeightsFromPrincipalComponents ( Mem2d* imp );
	Vector_d* getEigenvalues ( Matrix_d& scatterMatrix );
	Vector_d* getEigenvector ( Matrix_d& scatterMatrix, Vector_d& eigenvalue );
	Mem2d& getGray32 ( );
	double getLargestAbsoluteEigenvalue (Vector_d& eigenvalue);
	Vector_d getLuminanceFromCCIR601 ();
	Vector_d linearLeastSquares (Matrix_d& A, Vector_d& b);
	void QRdecomposition (Matrix_d& Q, Matrix_d& R);
	Vector_d* getLuminanceFromCCIR601 () {
		Vector_d weights (3);
//		weights[] = {0.299, 0.587, 0.114};
		weights[0] = 0.299;
		weights[1] = 0.587;
		weights[2] = 0.114;
		return new Vector_d (weights);
	};
#endif
