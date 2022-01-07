/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
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

/* The math algorithm is based on the following work, prior implemented for ImageJ: */

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


#include <gtk/gtk.h>
#include "config.h"
#include "plugin.h"

#include "autoalign_turbo_reg.h"

gpointer turboRegImage_thread (void *data);
gpointer turboRegMask_thread (void *data);

gpointer turboRegImage_thread (void *data){
	turboRegImage* tri = (turboRegImage*)data;
	tri->run ();
	return NULL;
}

gpointer turboRegMask_thread (void *data){
	turboRegMask* trm = (turboRegMask*)data;
	trm->run ();
	return NULL;
}

void turboRegAlign::run (
	Mem2d* source, int sv, Matrix_i& sourceCrop, 
	Mem2d* target, int tv, Matrix_i& targetCrop, 
	Matrix_d& sourcePoints, Matrix_d& targetPoints, 
	Mem2d* destination, int dv
	) {

	Matrix_i no_Crop(2, Vector_i(2,0));
	no_Crop[1][0] = source->GetNx ()-1;
	no_Crop[1][1] = source->GetNy ()-1;

//	turboRegImage sourceImg (source, sv, sourceCrop, transformation, false);
//	turboRegImage targetImg (target, tv, targetCrop, transformation, true);
	turboRegImage sourceImg (source, sv, no_Crop, transformation, false);
	turboRegImage targetImg (target, tv, no_Crop, transformation, true);
	int pyramidDepth = getPyramidDepth (sourceImg.getWidth (), sourceImg.getHeight (),  targetImg.getWidth (), targetImg.getHeight ());

	ProgressBar turboRegProgressBar ("turboRegAlign::turboRegAlign");
	turboRegProgressBar.Info ("REG_MODE", (double) transformation);
	
	turboRegProgressBar.pm_i ("sourceCrop", sourceCrop);
	turboRegProgressBar.pm_i ("targetCrop", targetCrop);
	
	turboRegProgressBar.pm_d ("sourcePoints in", sourcePoints);
	turboRegProgressBar.pm_d ("targetPoints in", targetPoints);

	sourceImg.setPyramidDepth (pyramidDepth);
	targetImg.setPyramidDepth (pyramidDepth);

	GThread* thread_si = g_thread_new ("turboRegImage_thread_si", turboRegImage_thread, &sourceImg);
	GThread* thread_ti = g_thread_new ("turboRegImage_thread_ti", turboRegImage_thread, &targetImg);

//	sourceImg.run ();
//	targetImg.run ();

	turboRegMask sourceMsk (sourceImg.getWidth (), sourceImg.getHeight ());
	turboRegMask targetMsk (sourceImg.getWidth (), sourceImg.getHeight ());

// all now.***
	sourceMsk.clearMask ();
	targetMsk.clearMaskWindow (targetCrop);
	
	sourceMsk.setPyramidDepth (pyramidDepth);
	targetMsk.setPyramidDepth (pyramidDepth);

	GThread* thread_sm = g_thread_new ("turboRegMask_thread_sm", turboRegMask_thread, &sourceMsk);
	GThread* thread_tm = g_thread_new ("turboRegMask_thread_tm", turboRegMask_thread, &targetMsk);

//	sourceMsk.run ();
//	targetMsk.run ();


	int tnp = transformation_num_points ();
	for (int k=0; k<tnp; ++k){
		sourcePoints[k][0] -= sourceCrop[0][0];
		sourcePoints[k][1] -= sourceCrop[0][1];
		targetPoints[k][0] -= targetCrop[0][0];
		targetPoints[k][1] -= targetCrop[0][1];
	}

// ????
//	turboRegFinalAction finalAction (
//		sourceImg, sourceMsk, sourcePoints,
//		targetImg, targetMsk, targetPoints, transformation);
// ????

	gboolean accelerated = false;
	turboRegTransform tt (&sourceImg, &sourceMsk, &sourcePoints,
			      &targetImg, &targetMsk, &targetPoints, 
			      transformation, accelerated, false);

	g_thread_join (thread_si);
	g_thread_join (thread_ti);
	g_thread_join (thread_sm);
	g_thread_join (thread_tm);

	tt.doRegistration();

	for (int k=0; k<tnp; ++k){
		sourcePoints[k][0] += sourceCrop[0][0];
		sourcePoints[k][1] += sourceCrop[0][1];
		targetPoints[k][0] += targetCrop[0][0];
		targetPoints[k][1] += targetCrop[0][1];
	}
	
	turboRegProgressBar.pm_d ("sourcePoints out", sourcePoints);
	turboRegProgressBar.pm_d ("targetPoints out", targetPoints);

	turboRegTransform regTransform (&sourceImg, &sourceMsk, &sourcePoints,
					&targetImg, &targetMsk, &targetPoints,
					transformation, false, false);

	if (destination){
		Matrix_i noCrop(2, Vector_i(2,0));
		noCrop[1][0] = source->GetNx ()-1;
		noCrop[1][1] = source->GetNy ()-1;
		turboRegImage sourceImgUnCroped (source, sv, noCrop, transformation, false);
 		regTransform.doFinalTransform (destination, dv);
	}

} /* end alignImages */

int turboRegAlign::getPyramidDepth (
	int sw,
	int sh,
	int tw,
	int th
) { 
	const int MIN_SIZE = 12;
	int pyramidDepth = 1;
	while (((2 * MIN_SIZE) <= sw) && ((2 * MIN_SIZE) <= sh)
		&& ((2 * MIN_SIZE) <= tw) && ((2 * MIN_SIZE) <= th)) {
		sw /= 2;
		sh /= 2;
		tw /= 2;
		th /= 2;
		pyramidDepth++;
	}
	return(pyramidDepth);
} /* end getPyramidDepth */

void turboRegAlign::doTransform (
	Mem2d* source, int sv,
	Matrix_d& sourcePoints, Matrix_d& targetPoints, 
	Mem2d* destination, int dv) {

	turboRegImage sourceImg (source, sv);
	turboRegMask sourceMsk (sourceImg.getWidth (), sourceImg.getHeight ());
	int pyramidDepth = getPyramidDepth (sourceImg.getWidth (), sourceImg.getHeight (), destination->GetNx (), destination->GetNy ());
	sourceImg.setPyramidDepth (pyramidDepth);
	sourceImg.run ();

	turboRegTransform regTransform (&sourceImg, &sourceMsk, &sourcePoints,
					NULL, NULL, &targetPoints,
					transformation, false, false);

	regTransform.doFinalTransform (destination, dv);
}

/*====================================================================
|	turboRegImage
\===================================================================*/

turboRegImage::turboRegImage (Mem2d* imp, int v, Matrix_i& region, REGISTER_MODE _transformation, gboolean _isTarget) {
	ProgressBar turboRegProgressBar ("turboRegImage::turboRegImage MiMib");
	transformation = _transformation;
	isTarget = _isTarget;

	width  = region[1][0]-region[0][0]+1;
	height = region[1][1]-region[0][1]+1;

	image.resize (width * height);
	int m=0;
	for (int y=0; y<height; ++y)
		for (int x=0; x<width; ++x)
			image[m++] = imp->GetDataPkt (x+region[0][0], y+region[0][1], v);

}

turboRegImage::turboRegImage (Mem2d* imp, int v) {
	ProgressBar turboRegProgressBar ("turboRegImage::turboRegImage Mi");
	transformation = REG_GENERIC_TRANSFORMATION;
	isTarget = false;

	width  = imp->GetNx ();
	height = imp->GetNy ();

	image.resize (width * height);
	int m=0;
	for (int y=0; y<height; ++y)
		for (int x=0; x<width; ++x)
			image[m++] = imp->GetDataPkt (x, y, v);

}


/*....................................................................
	Private methods
....................................................................*/

void turboRegImage::antiSymmetricFirMirrorOffBounds1D (Vector_d& h, Vector_d& c, Vector_d& s) {
	if (2 <= c.size ()) {
		s[0] = h[1] * (c[1] - c[0]);
		for (int i = 1; (i < (s.size () - 1)); i++) {
			s[i] = h[1] * (c[i + 1] - c[i - 1]);
		}
		s[s.size () - 1] = h[1] * (c[c.size () - 1] - c[c.size () - 2]);
	}
	else {
		s[0] = 0.0;
	}
}

void turboRegImage::basicToCardinal2D (
	Vector_d& basic,
	Vector_d& cardinal,
	int width,
	int height,
	int degree
) {
	Vector_d hLine (width);
	Vector_d vLine (height);
	Vector_d hData (width);
	Vector_d vData (height);
	Vector_d h;
	ProgressBar turboRegProgressBar ("turboRegImage::basicToCardinal2D");
	switch (degree) {
		case 3: {
			h.resize (2);
			h[0] = 2.0 / 3.0;
			h[1] = 1.0 / 6.0;
			break;
		}
		case 7: {
			h.resize (4);
			h[0] = 151.0 / 315.0;
			h[1] = 397.0 / 1680.0;
			h[2] = 1.0 / 42.0;
			h[3] = 1.0 / 5040.0;
			break;
		}
		default: {
			h.resize (1);
			h[0] = 1.0;
		}
	}
	int workload = width + height;
//	turboRegProgressBar.addWorkload(workload);

//	for (int y = 0; ((y < height) && (!t.isInterrupted())); y++) {
	for (int y = 0; ((y < height) && (1)); y++) {
		extractRow(basic, y, hLine);
		symmetricFirMirrorOffBounds1D(h, hLine, hData);
		putRow(cardinal, y, hData);
//		turboRegProgressBar.stepProgressBar();
		workload--;
	}
//	for (int x = 0; ((x < width) && (!t.isInterrupted())); x++) {
	for (int x = 0; ((x < width) && (1)); x++) {
		extractColumn(cardinal, width, x, vLine);
		symmetricFirMirrorOffBounds1D(h, vLine, vData);
		putColumn(cardinal, width, x, vData);
//		turboRegProgressBar.stepProgressBar();
		workload--;
	}
//	turboRegProgressBar.skipProgressBar(workload);
//	turboRegProgressBar.workloadDone(width + height);
}

void turboRegImage::buildCoefficientPyramid () {
	int fullWidth;
	int fullHeight;
	Vector_d* fullDual = new Vector_d (width * height);
	int halfWidth = width;
	int halfHeight = height;
	if (1 < pyramidDepth) {
		basicToCardinal2D(coefficient, *fullDual, width, height, 7);
	}
//	for (int depth = 1; ((depth < pyramidDepth) && (!t.isInterrupted())); depth++) {
	for (int depth = 1; ((depth < pyramidDepth) && (1)); depth++) {
		fullWidth = halfWidth;
		fullHeight = halfHeight;
		halfWidth /= 2;
		halfHeight /= 2;
		Vector_d* halfDual = getHalfDual2D (*fullDual, fullWidth, fullHeight);
		pyramid.push ((gpointer)getBasicFromCardinal2D (*halfDual, halfWidth, halfHeight, 7)); // push (Vector_d* halfCoefficient)
		pyramid.push ((gpointer)new int (halfHeight));
		pyramid.push ((gpointer)new int (halfWidth));

		delete fullDual;
		fullDual = halfDual;
	}
	delete fullDual;
}

void turboRegImage::buildImageAndGradientPyramid () {
	int fullWidth;
	int fullHeight;
	Vector_d* fullDual = new Vector_d (width * height);
	int halfWidth = width;
	int halfHeight = height;
	if (1 < pyramidDepth) {
		cardinalToDual2D(image, *fullDual, width, height, 3);
	}
//	for (int depth = 1; ((depth < pyramidDepth) && (!t.isInterrupted())); depth++) {
	for (int depth = 1; ((depth < pyramidDepth) && (1)); depth++) {
		fullWidth = halfWidth;
		fullHeight = halfHeight;
		halfWidth /= 2;
		halfHeight /= 2;
		Vector_d* halfDual = getHalfDual2D (*fullDual, fullWidth, fullHeight);
		Vector_d* halfImage = getBasicFromCardinal2D (*halfDual, halfWidth, halfHeight, 7);
		Vector_d* halfXGradient = new Vector_d (halfWidth * halfHeight);
		Vector_d* halfYGradient = new Vector_d (halfWidth * halfHeight);
		coefficientToXYGradient2D (*halfImage, *halfXGradient, *halfYGradient, halfWidth, halfHeight);
		basicToCardinal2D (*halfImage, *halfImage, halfWidth, halfHeight, 3);
		pyramid.push ((gpointer)halfYGradient);
		pyramid.push ((gpointer)halfXGradient);
		pyramid.push ((gpointer)halfImage);
		pyramid.push ((gpointer)new int (halfHeight));
		pyramid.push ((gpointer)new int (halfWidth));

		delete fullDual;
		fullDual = halfDual;
	}
	delete fullDual;
}

void turboRegImage::buildImagePyramid () {
	int fullWidth;
	int fullHeight;
	Vector_d* fullDual = new Vector_d (width * height);
	int halfWidth = width;
	int halfHeight = height;
	if (1 < pyramidDepth) {
		cardinalToDual2D(image, *fullDual, width, height, 3);
	}
//	for (int depth = 1; ((depth < pyramidDepth) && (!t.isInterrupted())); depth++) {
	for (int depth = 1; ((depth < pyramidDepth) && (1)); depth++) {
		fullWidth = halfWidth;
		fullHeight = halfHeight;
		halfWidth /= 2;
		halfHeight /= 2;
		Vector_d* halfDual = getHalfDual2D (*fullDual, fullWidth, fullHeight);
		Vector_d* halfImage = new Vector_d (halfWidth * halfHeight);
		dualToCardinal2D (*halfDual, *halfImage, halfWidth, halfHeight, 3);
		pyramid.push ((gpointer)halfImage);
		pyramid.push ((gpointer)new int (halfHeight));
		pyramid.push ((gpointer)new int (halfWidth));

		delete fullDual;
		fullDual = halfDual;
	}
	delete fullDual;
}

void turboRegImage::cardinalToDual2D (
	Vector_d& cardinal,
	Vector_d& dual,
	int width,
	int height,
	int degree
) {
	Vector_d* tmp;
	basicToCardinal2D (*(tmp=getBasicFromCardinal2D(cardinal, width, height, degree)),
			   dual, width, height, 2 * degree + 1);
	delete tmp;
}

void turboRegImage::coefficientToGradient1D (
	Vector_d& c
) {
	Vector_d h(2); h[0] = 0.0; h[1] = 1.0 / 2.0;
	Vector_d s(c.size ());
	antiSymmetricFirMirrorOffBounds1D (h, c, s);
	vector_copy (s, 0, c, 0, s.size ());
}

void turboRegImage::coefficientToSamples1D (
	Vector_d& c
) {
	Vector_d h(2); h[0] = 2.0 / 3.0; h[1] = 1.0 / 6.0;
	Vector_d s(c.size ());
	symmetricFirMirrorOffBounds1D(h, c, s);
	vector_copy(s, 0, c, 0, s.size ());
}

void turboRegImage::coefficientToXYGradient2D (
	Vector_d& basic,
	Vector_d& xGradient,
	Vector_d& yGradient,
	int width,
	int height
) {
	Vector_d hLine(width);
	Vector_d hData(width);
	Vector_d vLine(height);
	int workload = 2 * (width + height);
	ProgressBar turboRegProgressBar ("turboRegImage::coefficientToXYGradient2D");
//	turboRegProgressBar.addWorkload(workload);
//	for (int y = 0; ((y < height) && (!t.isInterrupted())); y++) {
	for (int y = 0; ((y < height) && (1)); y++) {
		extractRow(basic, y, hLine);
		vector_copy(hLine, 0, hData, 0, width);
		coefficientToGradient1D(hLine);
		turboRegProgressBar.stepProgressBar();
		workload--;
		coefficientToSamples1D(hData);
		putRow(xGradient, y, hLine);
		putRow(yGradient, y, hData);
		turboRegProgressBar.stepProgressBar();
		workload--;
	}
//	for (int x = 0; ((x < width) && (!t.isInterrupted())); x++) {
	for (int x = 0; ((x < width) && (1)); x++) {
		extractColumn(xGradient, width, x, vLine);
		coefficientToSamples1D(vLine);
		putColumn(xGradient, width, x, vLine);
		turboRegProgressBar.stepProgressBar();
		workload--;
		extractColumn(yGradient, width, x, vLine);
		coefficientToGradient1D(vLine);
		putColumn(yGradient, width, x, vLine);
		turboRegProgressBar.stepProgressBar();
		workload--;
	}
//	turboRegProgressBar.skipProgressBar(workload);
//	turboRegProgressBar.workloadDone(2 * (width + height));
}

void turboRegImage::dualToCardinal2D (
	Vector_d& dual,
	Vector_d& cardinal,
	int width,
	int height,
	int degree
) {
	Vector_d* tmp;
	basicToCardinal2D(*(tmp=getBasicFromCardinal2D (dual, width, height, 2 * degree + 1)),
			  cardinal, width, height, degree);
	delete tmp;
}

void turboRegImage::extractColumn (
	Vector_d& array,
	int width,
	int x,
	Vector_d& column
) {
	for (int i = 0; (i < column.size ()); i++) {
		column[i] = (double)array[x];
		x += width;
	}
}

void turboRegImage::extractRow (
	Vector_d& array,
	int y,
	Vector_d& row
) {
	y *= row.size ();
	for (int i = 0; (i < row.size ()); i++) {
		row[i] = (double)array[y++];
	}
}

Vector_d* turboRegImage::getBasicFromCardinal2D (
) {
	Vector_d* basic = new Vector_d (width * height);
	Vector_d hLine (width);
	Vector_d vLine (height);
	ProgressBar turboRegProgressBar ("turboRegImage::getBasicFromCardinal2D");
//	turboRegProgressBar.addWorkload(width + height);
	for (int y = 0; (y < height); y++) {
		extractRow(image, y, hLine);
		samplesToInterpolationCoefficient1D(hLine, 3, 0.0);
		putRow (*basic, y, hLine);
//		turboRegProgressBar.stepProgressBar();
	}
	for (int x = 0; (x < width); x++) {
		extractColumn (*basic, width, x, vLine);
		samplesToInterpolationCoefficient1D(vLine, 3, 0.0);
		putColumn (*basic, width, x, vLine);
//		turboRegProgressBar.stepProgressBar();
	}
//	turboRegProgressBar.workloadDone(width + height);
	return basic;
}

Vector_d* turboRegImage::getBasicFromCardinal2D (
	Vector_d& cardinal,
	int width,
	int height,
	int degree
) {
	Vector_d* basic = new Vector_d (width * height);
	Vector_d hLine (width);
	Vector_d vLine (height);
	int workload = width + height;
	ProgressBar turboRegProgressBar ("turboRegImage::getBasicFromCardinal2D");
//	turboRegProgressBar.addWorkload(workload);
//	for (int y = 0; ((y < height) && (!t.isInterrupted())); y++) {
	for (int y = 0; ((y < height) && (1)); y++) {
		extractRow(cardinal, y, hLine);
		samplesToInterpolationCoefficient1D(hLine, degree, 0.0);
		putRow (*basic, y, hLine);
//		turboRegProgressBar.stepProgressBar();
		workload--;
	}
//	for (int x = 0; ((x < width) && (!t.isInterrupted())); x++) {
	for (int x = 0; ((x < width) && (1)); x++) {
		extractColumn (*basic, width, x, vLine);
		samplesToInterpolationCoefficient1D(vLine, degree, 0.0);
		putColumn (*basic, width, x, vLine);
//		turboRegProgressBar.stepProgressBar();
		workload--;
	}
//	turboRegProgressBar.skipProgressBar(workload);
//	turboRegProgressBar.workloadDone(width + height);
	return basic;
}

Vector_d* turboRegImage::getHalfDual2D (
	Vector_d& fullDual,
	int fullWidth,
	int fullHeight
) {
	int halfWidth = fullWidth / 2;
	int halfHeight = fullHeight / 2;
	Vector_d hLine (fullWidth);
	Vector_d hData (halfWidth);
	Vector_d vLine (fullHeight);
	Vector_d vData (halfHeight);
	Vector_d demiDual (halfWidth * fullHeight);
	Vector_d* halfDual = new Vector_d (halfWidth * halfHeight);
	int workload = halfWidth + fullHeight;
	ProgressBar turboRegProgressBar ("turboRegImage::getHalfDual2D");
//	turboRegProgressBar.addWorkload(workload);
//	for (int y = 0; ((y < fullHeight) && (!t.isInterrupted())); y++) {
	for (int y = 0; ((y < fullHeight) && (1)); y++) {
		extractRow(fullDual, y, hLine);
		reduceDual1D(hLine, hData);
		putRow(demiDual, y, hData);
//		turboRegProgressBar.stepProgressBar();
		workload--;
	}
//	for (int x = 0; ((x < halfWidth) && (!t.isInterrupted())); x++) {
	for (int x = 0; ((x < halfWidth) && (1)); x++) {
		extractColumn(demiDual, halfWidth, x, vLine);
		reduceDual1D(vLine, vData);
		putColumn(*halfDual, halfWidth, x, vData);
//		turboRegProgressBar.stepProgressBar();
		workload--;
	}
//	turboRegProgressBar.skipProgressBar(workload);
//	turboRegProgressBar.workloadDone(halfWidth + fullHeight);

	return halfDual;
}

double turboRegImage::getInitialAntiCausalCoefficientMirrorOffBounds (
	Vector_d& c,
	double z,
	double tolerance
) {
	return (z * c[c.size () - 1] / (z - 1.0));
}

double turboRegImage::getInitialCausalCoefficientMirrorOffBounds (
	Vector_d& c,
	double z,
	double tolerance
) {
	double z1 = z;
	double zn = pow(z, (double)c.size ());
	double sum = (1.0 + z) * (c[0] + zn * c[c.size () - 1]);
	int horizon = c.size ();
	if (0.0 < tolerance) {
		horizon = 2 + (int)(log(tolerance) / log(fabs(z)));
		horizon = (horizon < c.size ()) ? (horizon) : (c.size ());
	}
	zn = zn * zn;
	for (int n = 1; (n < (horizon - 1)); n++) {
		z1 = z1 * z;
		zn = zn / z;
		sum = sum + (z1 + zn) * c[n];
	}
	return(sum / (1.0 - pow(z, 2. * c.size ())));
}

void turboRegImage::imageToXYGradient2D (
) {
	Vector_d hLine (width);
	Vector_d vLine (height);
	xGradient.resize (width * height);
	yGradient.resize (width * height);
	int workload = width + height;

	ProgressBar turboRegProgressBar ("turboRegImage::imageToXYGradient2D");
//	turboRegProgressBar.addWorkload(workload);
//	for (int y = 0; ((y < height) && (!t.isInterrupted())); y++) {
	for (int y = 0; ((y < height) && (1)); y++) {
		extractRow(image, y, hLine);
		samplesToInterpolationCoefficient1D(hLine, 3, 0.0);
		coefficientToGradient1D(hLine);
		putRow(xGradient, y, hLine);
//		turboRegProgressBar.stepProgressBar();
		workload--;
	}
//	for (int x = 0; ((x < width) && (!t.isInterrupted())); x++) {
	for (int x = 0; ((x < width) && (1)); x++) {
		extractColumn(image, width, x, vLine);
		samplesToInterpolationCoefficient1D(vLine, 3, 0.0);
		coefficientToGradient1D(vLine);
		putColumn(yGradient, width, x, vLine);
//		turboRegProgressBar.stepProgressBar();
		workload--;
	}
//	turboRegProgressBar.skipProgressBar(workload);
//	turboRegProgressBar.workloadDone(width + height);
}

void turboRegImage::putColumn (
	Vector_d& array,
	int width,
	int x,
	Vector_d& column
) {
	for (int i = 0; (i < column.size ()); i++) {
		array[x] = column[i];
		x += width;
	}
}

void turboRegImage::putRow (
	Vector_d& array,
	int y,
	Vector_d& row
) {
	y *= row.size ();
	for (int i = 0; (i < row.size ()); i++) {
		array[y++] = row[i];
	}
}

void turboRegImage::reduceDual1D (
	Vector_d& c,
	Vector_d& s
) {
	Vector_d h(3); h[0] = 6.0 / 16.0; h[1] = 4.0 / 16.0; h[2] = 1.0 / 16.0;
	if (2 <= s.size ()) {
		s[0] = h[0] * c[0] + h[1] * (c[0] + c[1]) + h[2] * (c[1] + c[2]);
		for (int i = 2, j = 1; (j < (s.size () - 1)); i += 2, j++) {
			s[j] = h[0] * c[i] + h[1] * (c[i - 1] + c[i + 1])
				+ h[2] * (c[i - 2] + c[i + 2]);
		}
		if (c.size () == (2 * s.size ())) {
			s[s.size () - 1] = h[0] * c[c.size () - 2] + h[1] * (c[c.size () - 3] + c[c.size () - 1])
				+ h[2] * (c[c.size () - 4] + c[c.size () - 1]);
		}
		else {
			s[s.size () - 1] = h[0] * c[c.size () - 3] + h[1] * (c[c.size () - 4] + c[c.size () - 2])
				+ h[2] * (c[c.size () - 5] + c[c.size () - 1]);
		}
	}
	else {
		switch (c.size ()) {
			case 3: {
				s[0] = h[0] * c[0] + h[1] * (c[0] + c[1]) + h[2] * (c[1] + c[2]);
				break;
			}
			case 2: {
				s[0] = h[0] * c[0] + h[1] * (c[0] + c[1]) + 2.0 * h[2] * c[1];
				break;
			}
		}
	}
}

void turboRegImage::samplesToInterpolationCoefficient1D (
	Vector_d& c,
	int degree,
	double tolerance
) {
	Vector_d z;
	double lambda = 1.0;
	switch (degree) {
		case 3: {
			z.resize (1);
			z[0] = sqrt(3.0) - 2.0;
			break;
		}
		case 7: {
			z.resize (3);
			z[0] = -0.5352804307964381655424037816816460718339231523426924148812;
			z[1] = -0.122554615192326690515272264359357343605486549427295558490763;
			z[2] = -0.0091486948096082769285930216516478534156925639545994482648003;
			break;
		}
	}
	if (c.size () == 1) {
		return;
	}
	for (int k = 0; (k < z.size ()); k++) {
		lambda *= (1.0 - z[k]) * (1.0 - 1.0 / z[k]);
	}
	for (int n = 0; (n < c.size ()); n++) {
		c[n] = c[n] * lambda;
	}
	for (int k = 0; (k < z.size ()); k++) {
		c[0] = getInitialCausalCoefficientMirrorOffBounds(c, z[k], tolerance);
		for (int n = 1; (n < c.size ()); n++) {
			c[n] = c[n] + z[k] * c[n - 1];
		}
		c[c.size () - 1] = getInitialAntiCausalCoefficientMirrorOffBounds(c, z[k], tolerance);
		for (int n = c.size () - 2; (0 <= n); n--) {
			c[n] = z[k] * (c[n+1] - c[n]);
		}
	}
}

void turboRegImage::symmetricFirMirrorOffBounds1D (
	Vector_d& h,
	Vector_d& c,
	Vector_d& s
) {
	switch (h.size ()) {
		case 2: {
			if (2 <= c.size ()) {
				s[0] = h[0] * c[0] + h[1] * (c[0] + c[1]);
				for (int i = 1; (i < (s.size () - 1)); i++) {
					s[i] = h[0] * c[i] + h[1] * (c[i - 1] + c[i + 1]);
				}
				s[s.size () - 1] = h[0] * c[c.size () - 1]
					+ h[1] * (c[c.size () - 2] + c[c.size () - 1]);
			}
			else {
				s[0] = (h[0] + 2.0 * h[1]) * c[0];
			}
			break;
		}
		case 4: {
			if (6 <= c.size ()) {
				s[0] = h[0] * c[0] + h[1] * (c[0] + c[1]) + h[2] * (c[1] + c[2])
					+ h[3] * (c[2] + c[3]);
				s[1] = h[0] * c[1] + h[1] * (c[0] + c[2]) + h[2] * (c[0] + c[3])
					+ h[3] * (c[1] + c[4]);
				s[2] = h[0] * c[2] + h[1] * (c[1] + c[3]) + h[2] * (c[0] + c[4])
					+ h[3] * (c[0] + c[5]);
				for (int i = 3; (i < (s.size () - 3)); i++) {
					s[i] = h[0] * c[i] + h[1] * (c[i - 1] + c[i + 1])
						+ h[2] * (c[i - 2] + c[i + 2]) + h[3] * (c[i - 3] + c[i + 3]);
				}
				s[s.size () - 3] = h[0] * c[c.size () - 3]
					+ h[1] * (c[c.size () - 4] + c[c.size () - 2])
					+ h[2] * (c[c.size () - 5] + c[c.size () - 1])
					+ h[3] * (c[c.size () - 6] + c[c.size () - 1]);
				s[s.size () - 2] = h[0] * c[c.size () - 2]
					+ h[1] * (c[c.size () - 3] + c[c.size () - 1])
					+ h[2] * (c[c.size () - 4] + c[c.size () - 1])
					+ h[3] * (c[c.size () - 5] + c[c.size () - 2]);
				s[s.size () - 1] = h[0] * c[c.size () - 1]
					+ h[1] * (c[c.size () - 2] + c[c.size () - 1])
					+ h[2] * (c[c.size () - 3] + c[c.size () - 2])
					+ h[3] * (c[c.size () - 4] + c[c.size () - 3]);
			}
			else {
				switch (c.size ()) {
					case 5: {
						s[0] = h[0] * c[0] + h[1] * (c[0] + c[1]) + h[2] * (c[1] + c[2])
							+ h[3] * (c[2] + c[3]);
						s[1] = h[0] * c[1] + h[1] * (c[0] + c[2]) + h[2] * (c[0] + c[3])
							+ h[3] * (c[1] + c[4]);
						s[2] = h[0] * c[2] + h[1] * (c[1] + c[3])
							+ (h[2] + h[3]) * (c[0] + c[4]);
						s[3] = h[0] * c[3] + h[1] * (c[2] + c[4]) + h[2] * (c[1] + c[4])
							+ h[3] * (c[0] + c[3]);
						s[4] = h[0] * c[4] + h[1] * (c[3] + c[4]) + h[2] * (c[2] + c[3])
							+ h[3] * (c[1] + c[2]);
						break;
					}
					case 4: {
						s[0] = h[0] * c[0] + h[1] * (c[0] + c[1]) + h[2] * (c[1] + c[2])
							+ h[3] * (c[2] + c[3]);
						s[1] = h[0] * c[1] + h[1] * (c[0] + c[2]) + h[2] * (c[0] + c[3])
							+ h[3] * (c[1] + c[3]);
						s[2] = h[0] * c[2] + h[1] * (c[1] + c[3]) + h[2] * (c[0] + c[3])
							+ h[3] * (c[0] + c[2]);
						s[3] = h[0] * c[3] + h[1] * (c[2] + c[3]) + h[2] * (c[1] + c[2])
							+ h[3] * (c[0] + c[1]);
						break;
					}
					case 3: {
						s[0] = h[0] * c[0] + h[1] * (c[0] + c[1]) + h[2] * (c[1] + c[2])
							+ 2.0 * h[3] * c[2];
						s[1] = h[0] * c[1] + (h[1] + h[2]) * (c[0] + c[2])
							+ 2.0 * h[3] * c[1];
						s[2] = h[0] * c[2] + h[1] * (c[1] + c[2]) + h[2] * (c[0] + c[1])
							+ 2.0 * h[3] * c[0];
						break;
					}
					case 2: {
						s[0] = (h[0] + h[1] + h[3]) * c[0] + (h[1] + 2.0 * h[2] + h[3]) * c[1];
						s[1] = (h[0] + h[1] + h[3]) * c[1] + (h[1] + 2.0 * h[2] + h[3]) * c[0];
						break;
					}
					case 1: {
						s[0] = (h[0] + 2.0 * (h[1] + h[2] + h[3])) * c[0];
						break;
					}
				}
			}
			break;
		}
	}
}




/*====================================================================
|	turboRegMask
\===================================================================*/

/*********************************************************************
 Converts the pixel array of the incoming <code>ImagePlus</code>
 object into a local <code>boolean</code> array.
 @param imp <code>ImagePlus</code> object to preprocess.
 ********************************************************************/
turboRegMask::turboRegMask (Mem2d* imp) {
	width  = imp->GetNx ();
	height = imp->GetNy ();
	int k = 0;

	mask.resize (width * height);
	for (int y = 0; (y < height); y++) {
		for (int x = 0; (x < width); x++, k++) {
			mask[k] = (imp->GetDataPkt (x,y) != 0);
		}
	}
} /* end turboRegMask */

turboRegMask::turboRegMask (int w, int h) {
	width  = w;
	height = h;
	mask.resize (0);
	mask.resize (width * height, true);
} /* end turboRegMask */

/*********************************************************************
 Set to <code>true</code> every pixel of the full-size mask.
 ********************************************************************/
void turboRegMask::clearMask (gboolean m) {
	mask.resize (0);
	mask.resize (width * height, m);
}
void turboRegMask::clearMaskWindow (Matrix_i& win) {
	clearMask (false);
	for (int y = win[0][1]; y < win[1][1]; y++)
		for (int x = win[0][0]; x < win[1][0]; x++)
			mask[y*width+x] = true;
}

/*....................................................................
	Private methods
....................................................................*/

void turboRegMask::buildPyramid (
) {
	int fullWidth;
	int fullHeight;
	Vector_b* fullMask = &mask;
	int halfWidth = width;
	int halfHeight = height;
	ProgressBar turboRegProgressBar ("turboRegMask::buildPyramid");
//	for (int depth = 1; ((depth < pyramidDepth) && (!t.isInterrupted())); depth++) {
	for (int depth = 1; ((depth < pyramidDepth) && (1)); depth++) {
		fullWidth = halfWidth;
		fullHeight = halfHeight;
		halfWidth /= 2;
		halfHeight /= 2;
		Vector_b* halfMask = getHalfMask2D (*fullMask, fullWidth, fullHeight);
		pyramid.push ((gpointer)halfMask);
		fullMask = halfMask;
	}
}

Vector_b* turboRegMask::getHalfMask2D (
	Vector_b& fullMask,
	int fullWidth,
	int fullHeight
) {
	int halfWidth = fullWidth / 2;
	int halfHeight = fullHeight / 2;
	gboolean oddWidth = ((2 * halfWidth) != fullWidth);
	int workload = 2 * halfHeight;
	ProgressBar turboRegProgressBar ("turboRegMask::getHalfMask2D");
//	turboRegProgressBar.addWorkload(workload);
	Vector_b* halfMask = new Vector_b (halfWidth * halfHeight);
	int k = 0;
	for (int y = 0; ((y < halfHeight) && (1)); y++) {
		for (int x = 0; (x < halfWidth); x++) {
			(*halfMask)[k++] = false;
		}
//		turboRegProgressBar.stepProgressBar();
		workload--;
	}
	k = 0;
	int n = 0;
	for (int y = 0; ((y < (halfHeight - 1)) && (1)); y++) {
		for (int x = 0; (x < (halfWidth - 1)); x++) {
			(*halfMask)[k] |= fullMask[n++];
			(*halfMask)[k] |= fullMask[n];
			(*halfMask)[++k] |= fullMask[n++];
		}
		(*halfMask)[k] |= fullMask[n++];
		(*halfMask)[k++] |= fullMask[n++];
		if (oddWidth) {
			n++;
		}
		for (int x = 0; (x < (halfWidth - 1)); x++) {
			(*halfMask)[k - halfWidth] |= fullMask[n];
			(*halfMask)[k] |= fullMask[n++];
			(*halfMask)[k - halfWidth] |= fullMask[n];
			(*halfMask)[k - halfWidth + 1] |= fullMask[n];
			(*halfMask)[k] |= fullMask[n];
			(*halfMask)[++k] |= fullMask[n++];
		}
		(*halfMask)[k - halfWidth] |= fullMask[n];
		(*halfMask)[k] |= fullMask[n++];
		(*halfMask)[k - halfWidth] |= fullMask[n];
		(*halfMask)[k++] |= fullMask[n++];
		if (oddWidth) {
			n++;
		}
		k -= halfWidth;
//		turboRegProgressBar.stepProgressBar();
		workload--;
	}
	for (int x = 0; (x < (halfWidth - 1)); x++) {
		(*halfMask)[k] |= fullMask[n++];
		(*halfMask)[k] |= fullMask[n];
		(*halfMask)[++k] |= fullMask[n++];
	}
	(*halfMask)[k] |= fullMask[n++];
	(*halfMask)[k++] |= fullMask[n++];
	if (oddWidth) {
		n++;
	}
	k -= halfWidth;
	for (int x = 0; (x < (halfWidth - 1)); x++) {
		(*halfMask)[k] |= fullMask[n++];
		(*halfMask)[k] |= fullMask[n];
		(*halfMask)[++k] |= fullMask[n++];
	}
	(*halfMask)[k] |= fullMask[n++];
	(*halfMask)[k] |= fullMask[n];
//	turboRegProgressBar.stepProgressBar();
	workload--;
//	turboRegProgressBar.skipProgressBar(workload);
//	turboRegProgressBar.workloadDone(2 * halfHeight);

	return halfMask;
}




/*====================================================================
|	turboRegTransform
\===================================================================*/

/*********************************************************************
 Keep a local copy of most everything. Select among the pre-stored
 constants.
 @param targetImg Target image pyramid.
 @param targetMsk Target mask pyramid.
 @param sourceImg Source image pyramid.
 @param sourceMsk Source mask pyramid.
 @param targetPh Target <code>turboRegPointHandler</code> object.
 @param sourcePh Source <code>turboRegPointHandler</code> object.
 @param transformation Transformation code.
 @param accelerated Trade-off between speed and accuracy.
 @param interactive Shows or hides the resulting image.
 ********************************************************************/
turboRegTransform::turboRegTransform (
	turboRegImage* _sourceImg,
	turboRegMask* _sourceMsk,
	Matrix_d* _sourcePh,
	turboRegImage* _targetImg,
	turboRegMask* _targetMsk,
	Matrix_d* _targetPh,
	REGISTER_MODE _transformation,
	gboolean _accelerated,
	gboolean _interactive
) {
	sourceImg = _sourceImg;
	sourceMsk = _sourceMsk;
	targetImg = _targetImg;
	targetMsk = _targetMsk;
	transformation = _transformation;
	accelerated = _accelerated;
	interactive = _interactive;
	sourcePoint = _sourcePh;
	targetPoint = _targetPh;

	turboRegProgressBarTr = NULL;

	dxWeight.resize (4);
	dyWeight.resize (4);
	xWeight.resize (4);
	yWeight.resize (4);
	xIndex.resize (4);
	yIndex.resize (4);


	if (accelerated) {
		pixelPrecision = PIXEL_LOW_PREC;
		maxIterations = FEW_ITERATIONS;
	}
	else {
		pixelPrecision = PIXEL_HIGH_PREC;
		maxIterations = MANY_ITERATIONS;
	}
} /* end turboRegTransform */

/*....................................................................
	Public methods
....................................................................*/

/*********************************************************************
 Append the current landmarks into a text file. Rigid format.
 @param pathAndFilename Path and name of the file where batch results
 are being written.
 @see turboRegDialog#loadLandmarks()
 ********************************************************************/
void turboRegTransform::appendTransformation (
	gchar* pathAndFilename
) {
	outNx = targetImg->getWidth ();
	outNy = targetImg->getHeight ();
	inNx = sourceImg->getWidth ();
	inNy = sourceImg->getHeight ();
	if (pathAndFilename == NULL) {
		return;
	}
	{
		ofstream fw;
		fw.open (pathAndFilename, ios::out);
		fw << std::endl;
		switch (transformation) {
			case REG_TRANSLATION: {
				fw << "TRANSLATION\n";
				break;
			}
			case REG_RIGID_BODY: {
				fw << "RIGID_BODY\n";
				break;
			}
			case REG_SCALED_ROTATION: {
				fw << "SCALED_ROTATION\n";
				break;
			}
			case REG_AFFINE: {
				fw << "AFFINE\n";
				break;
			}
			case REG_BILINEAR: {
				fw << "BILINEAR\n";
				break;
			}
		}
		fw << "\n";
		fw << "Source size\n";
		fw << inNx << "\t" << inNy << "\n";
		fw << "\n";
		fw << "Target size\n";
		fw << outNx << "\t" << outNy << "\n";
		fw << "\n";
		fw << "Refined source landmarks\n";
		if (transformation == REG_RIGID_BODY) {
			for (int i = 0; (i < transformation); i++) {
				fw << (*sourcePoint)[i][0] << "\t" << (*sourcePoint)[i][1] << "\n";
			}
		}
		else {
			for (int i = 0; (i < (transformation / 2)); i++) {
				fw << (*sourcePoint)[i][0] << "\t" << (*sourcePoint)[i][1] << "\n";
			}
		}
		fw << "\n";
		fw << "Target landmarks\n";
		if (transformation == REG_RIGID_BODY) {
			for (int i = 0; (i < transformation); i++) {
				fw << (*targetPoint)[i][0] << "\t" << (*targetPoint)[i][1] << "\n";
			}
		}
		else {
			for (int i = 0; (i < (transformation / 2)); i++) {
				fw << (*targetPoint)[i][0] << "\t" << (*targetPoint)[i][1] << "\n";
			}
		}
		fw.close();
	}
} /* end appendTransformation */

/*********************************************************************
 Compute the image.
 ********************************************************************/
void turboRegTransform::doBatchFinalTransform (
	Vector_d& pixels
) {
	ProgressBar turboRegProgressBar ("turboRegTransform::doBatchFinalTransform_V");

	if (accelerated) {
		sourceImg->getImage (inImg);
	}
	else {
		sourceImg->getCoefficient (inImg);
	}
	inNx = sourceImg->getWidth();
	inNy = sourceImg->getHeight();
	twiceInNx = 2 * inNx;
	twiceInNy = 2 * inNy;
	outNx = targetImg->getWidth();
	outNy = targetImg->getHeight();
	Matrix_d* tmp;
	Matrix_d matrix (*(tmp = getTransformationMatrix(*targetPoint, *sourcePoint))); delete tmp;
	switch (transformation) {
		case REG_TRANSLATION: {
			translationTransform(matrix);
			break;
		}
		case REG_RIGID_BODY:
		case REG_SCALED_ROTATION:
		case REG_AFFINE: {
			affineTransform(matrix);
			break;
		}
		case REG_BILINEAR: {
			bilinearTransform(matrix);
			break;
		}
	}
	pixels = outImg;
} /* end doBatchFinalTransform */

/*********************************************************************
 Compute the image.
 ********************************************************************/
void turboRegTransform::doFinalTransform (
	Mem2d* destination, int dv
	) {
	ProgressBar turboRegProgressBar ("turboRegTransform::doFinalTransform_M2");
	
	if (accelerated)
		sourceImg->getImage (inImg);
	else
		sourceImg->getCoefficient (inImg);

	sourceMsk->getMask (inMsk);

	inNx = sourceImg->getWidth();
	inNy = sourceImg->getHeight();
	twiceInNx = 2 * inNx;
	twiceInNy = 2 * inNy;

	outNx = destination->GetNx ();
	outNy = destination->GetNy ();

	outImg.resize (outNx * outNy);
	outMsk.resize (outNx * outNy);

	Matrix_d* matrix = getTransformationMatrix (*targetPoint, *sourcePoint);
	switch (transformation) {
		case REG_TRANSLATION: {
			translationTransform(*matrix, outMsk);
			break;
		}
		case REG_RIGID_BODY:
		case REG_SCALED_ROTATION:
		case REG_AFFINE: {
			affineTransform(*matrix, outMsk);
			break;
		}
		case REG_BILINEAR: {
			bilinearTransform(*matrix, outMsk);
			break;
		}
	}
	delete matrix;

	turboRegProgressBar.Info ("store final image");

	int m=0;
	for (int y=0; y<outNy; ++y)
		for (int x=0; x<outNx; ++x)
			destination->PutDataPkt (outImg[m++] ,x,y, dv);

} /* end doFinalTransform */

/*********************************************************************
 Compute the image.
 ********************************************************************/
void turboRegTransform::doFinalTransform (
	REGISTER_MODE _transformation,
	turboRegImage* _sourceImg,
	Mem2d* destination, int dv,
	gboolean _accelerated
) {
	ProgressBar turboRegProgressBar ("turboRegTransform::doFinalTransform_iIMb");

	sourceImg = _sourceImg;
	transformation = _transformation;
	accelerated = _accelerated;

	if (accelerated) {
		sourceImg->getImage(inImg);
	}
	else {
		sourceImg->getCoefficient(inImg);
	}
	inMsk.resize (outNx * outNy);
	inMsk.clear ();

	inNx = sourceImg->getWidth();
	inNy = sourceImg->getHeight();
	twiceInNx = 2 * inNx;
	twiceInNy = 2 * inNy;


	outNx = destination->GetNx ();
	outNy = destination->GetNy ();

	outImg.resize (outNx * outNy);
	outMsk.resize (outNx * outNy, true);
//	outMsk.clear ();

	Matrix_d* matrix = getTransformationMatrix(*targetPoint, *sourcePoint);
	switch (transformation) {
		case REG_TRANSLATION: {
			translationTransform(*matrix);
			break;
		}
		case REG_RIGID_BODY:
		case REG_SCALED_ROTATION:
		case REG_AFFINE: {
			affineTransform(*matrix);
			break;
		}
		case REG_BILINEAR: {
			bilinearTransform(*matrix);
			break;
		}
	}


	turboRegProgressBar.Info ("store final image");
	int m=0;
	for (int y=0; y<outNy; ++y)
		for (int x=0; x<outNx; ++x)
			destination->PutDataPkt (outImg[m++] ,x,y, dv);
} /* end doFinalTransform */

/*********************************************************************
 Refine the landmarks.
 ********************************************************************/
void turboRegTransform::doRegistration (
) {
	ProgressBar turboRegProgressBar ("turboRegTransform::doRegistration");
	turboRegProgressBar.Info ("REG_MODE", transformation);

	turboRegProgressBarTr = &turboRegProgressBar;

	pyramidDepth = targetImg->getPyramidDepth();
	turboRegProgressBar.Info ("pyramidDepth", pyramidDepth);

	iterationPower = (int)pow((double)ITERATION_PROGRESSION, (double)pyramidDepth);

	turboRegProgressBar.addWorkload(pyramidDepth * maxIterations * iterationPower
					/ ITERATION_PROGRESSION - (iterationPower - 1) / (ITERATION_PROGRESSION - 1));
	iterationCost = 1;

	turboRegProgressBar.Info ("ScaleBottomDownLandmarks");

	scaleBottomDownLandmarks();

	turboRegProgressBar.pm_d("sourcePoint", *sourcePoint);
	turboRegProgressBar.pm_d("targetPoint", *targetPoint);
	
	inMsk_valid  = false;
	outMsk_valid = false;

	while (!targetImg->pyramid_empty ()) {
		turboRegProgressBar.Info ("popping source and target Image Pyramid data");
		iterationPower /= ITERATION_PROGRESSION;
		if (transformation == REG_BILINEAR) {
			turboRegProgressBar.Info ("REG_BILINEAR");

			sourceImg->pop_pyramid (inNx, inNy, inImg);
			if (sourceMsk == NULL) {
				inMsk_valid = false;
			}
			else {
				sourceMsk->pop_pyramid (inMsk);
				inMsk_valid = true;
			}

			targetImg->pop_pyramid (outNx, outNy, outImg);
			targetMsk->pop_pyramid (outMsk);
		}
		else {
			turboRegProgressBar.Info ("REG_OTHER");

			targetImg->pop_pyramid (inNx, inNy, inImg);
			targetMsk->pop_pyramid (inMsk);
			sourceImg->pop_pyramid (outNx, outNy, outImg, xGradient, yGradient);
			if (sourceMsk == NULL) {
				outMsk_valid = false;
			}
			else {
				sourceMsk->pop_pyramid (outMsk);
				outMsk_valid = true;
			}
		}
		turboRegProgressBar.Info ("REG_OTHER - pop pyramid DONE - trafo...");
		twiceInNx = 2 * inNx;
		twiceInNy = 2 * inNy;
		switch (transformation) {
			case REG_TRANSLATION: {
				targetJacobian = 1.0;
				inverseMarquardtLevenbergOptimization(iterationPower * maxIterations - 1);
				break;
			}
			case REG_RIGID_BODY: {
				inverseMarquardtLevenbergRigidBodyOptimization(iterationPower * maxIterations - 1);
				break;
			}
			case REG_SCALED_ROTATION: {
				targetJacobian = ((*targetPoint)[0][0] - (*targetPoint)[1][0])
					* ((*targetPoint)[0][0] - (*targetPoint)[1][0])
					+ ((*targetPoint)[0][1] - (*targetPoint)[1][1])
					* ((*targetPoint)[0][1] - (*targetPoint)[1][1]);
				inverseMarquardtLevenbergOptimization(iterationPower * maxIterations - 1);
				break;
			}
			case REG_AFFINE: {
				targetJacobian = ((*targetPoint)[1][0] - (*targetPoint)[2][0]) * (*targetPoint)[0][1]
					+ ((*targetPoint)[2][0] - (*targetPoint)[0][0]) * (*targetPoint)[1][1]
					+ ((*targetPoint)[0][0] - (*targetPoint)[1][0]) * (*targetPoint)[2][1];
				inverseMarquardtLevenbergOptimization(iterationPower * maxIterations - 1);
				break;
			}
			case REG_BILINEAR: {
				MarquardtLevenbergOptimization(iterationPower * maxIterations - 1);
				break;
			}
		}
		scaleUpLandmarks();
		iterationCost *= ITERATION_PROGRESSION;
	}
	iterationPower /= ITERATION_PROGRESSION;
	if (transformation == REG_BILINEAR) {
		inNx = sourceImg->getWidth();
		inNy = sourceImg->getHeight();
		sourceImg->getCoefficient (inImg);

		if (sourceMsk == NULL) {
			inMsk_valid = false;
		}
		else {
			sourceMsk->getMask (inMsk);
			inMsk_valid = true;
		}
		outNx = targetImg->getWidth();
		outNy = targetImg->getHeight();
		targetImg->getImage (outImg);
		targetMsk->getMask (outMsk);
	}
	else {
		inNx = targetImg->getWidth();
		inNy = targetImg->getHeight();
		targetImg->getCoefficient (inImg);
		targetMsk->getMask (inMsk);
		outNx = sourceImg->getWidth();
		outNy = sourceImg->getHeight();
		sourceImg->getImage(outImg);
		sourceImg->getXGradient (xGradient);
		sourceImg->getYGradient (yGradient);
		if (sourceMsk == NULL) {
			outMsk_valid = false;
		}
		else {
			sourceMsk->getMask (outMsk);
			outMsk_valid = true;
		}
	}
	twiceInNx = 2 * inNx;
	twiceInNy = 2 * inNy;
	if (accelerated) {
		turboRegProgressBar.skipProgressBar(iterationCost * (maxIterations - 1));
	}
	else {
		switch (transformation) {
			case REG_RIGID_BODY: {
				inverseMarquardtLevenbergRigidBodyOptimization(maxIterations - 1);
				break;
			}
			case REG_TRANSLATION:
			case REG_SCALED_ROTATION:
			case REG_AFFINE: {
				inverseMarquardtLevenbergOptimization(maxIterations - 1);
				break;
			}
			case REG_BILINEAR: {
				MarquardtLevenbergOptimization(maxIterations - 1);
				break;
			}
		}
	}
	iterationPower = (int)pow((double)ITERATION_PROGRESSION, (double)pyramidDepth);
	turboRegProgressBar.workloadDone(pyramidDepth * maxIterations * iterationPower
					 / ITERATION_PROGRESSION - (iterationPower - 1) / (ITERATION_PROGRESSION - 1));

	turboRegProgressBarTr = NULL;
} /* end doRegistration */

/*********************************************************************
 Save the current landmarks into a text file and return the path
 and name of the file. Rigid format.
 @see turboRegDialog#loadLandmarks()
 ********************************************************************/
const gchar* turboRegTransform::saveTransformation (
	const gchar* filename
) {
	inNx = sourceImg->getWidth();
	inNy = sourceImg->getHeight();
	outNx = targetImg->getWidth();
	outNy = targetImg->getHeight();
	const gchar* path = "/tmp/gxsmtmp-aa-trafo-test.list";
	if (filename == NULL) {
#if 0
		Frame f = new Frame();
		FileDialog fd = new FileDialog(f, "Save landmarks", FileDialog.SAVE);
		filename = "landmarks.txt";
		fd.setFile(filename);
		fd.setVisible(true);
		path = fd.getDirectory();
		filename = fd.getFile();
		if ((path == NULL) || (filename == NULL)) {
			return("");
		}
#endif
		return NULL;
	}
	{
		ofstream fw;
		fw.open(path);
		fw << "Transformation\n";
		switch (transformation) {
			case REG_TRANSLATION: {
				fw << "TRANSLATION\n";
				break;
			}
			case REG_RIGID_BODY: {
				fw << "RIGID_BODY\n";
				break;
			}
			case REG_SCALED_ROTATION: {
				fw << "SCALED_ROTATION\n";
				break;
			}
			case REG_AFFINE: {
				fw << "AFFINE\n";
				break;
			}
			case REG_BILINEAR: {
				fw << "BILINEAR\n";
				break;
			}
		}
		fw << "\n";
		fw << "Source size\n";
		fw << inNx << "\t" << inNy << "\n";
		fw << "\n";
		fw << "Target size\n";
		fw << outNx << "\t" << outNy << "\n";
		fw << "\n";
		fw << "Refined source landmarks\n";
		if (transformation == REG_RIGID_BODY) {
			for (int i = 0; (i < transformation); i++) {
				fw << (*sourcePoint)[i][0] << "\t" << (*sourcePoint)[i][1] << "\n";
			}
		}
		else {
			for (int i = 0; (i < (transformation / 2)); i++) {
				fw << (*sourcePoint)[i][0] << "\t" << (*sourcePoint)[i][1] << "\n";
			}
		}
		fw << "\n";
		fw << "Target landmarks\n";
		if (transformation == REG_RIGID_BODY) {
			for (int i = 0; (i < transformation); i++) {
				fw << (*targetPoint)[i][0] << "\t" << (*targetPoint)[i][1] << "\n";
			}
		}
		else {
			for (int i = 0; (i < (transformation / 2)); i++) {
				fw << (*targetPoint)[i][0] << "\t" << (*targetPoint)[i][1] << "\n";
			}
		}
		fw.close();
	}
	return(path);
} /* end saveTransformation */


/*....................................................................
	Private methods
....................................................................*/

void turboRegTransform::affineTransform (
	Matrix_d& matrix
) {
	double yx;
	double yy;
	double x0;
	double y0;
	int xMsk;
	int yMsk;
	int k = 0;
	ProgressBar turboRegProgressBar ("turboRegTransform::affineTransform_M");
//	turboRegProgressBar.addWorkload(outNy);
	yx = matrix[0][0];
	yy = matrix[1][0];
	for (int v = 0; (v < outNy); v++) {
		x0 = yx;
		y0 = yy;
		for (int u = 0; (u < outNx); u++) {
			x = x0;
			y = y0;
			xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
			yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
			if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
				xMsk += yMsk * inNx;
				if (accelerated) {
					outImg[k++] = inImg[xMsk];
				}
				else {
					xIndexes();
					yIndexes();
					x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
					y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
					xWeights();
					yWeights();
					outImg[k++] = interpolate();
				}
			}
			else {
				outImg[k++] = 0.0;
			}
			x0 += matrix[0][1];
			y0 += matrix[1][1];
		}
		yx += matrix[0][2];
		yy += matrix[1][2];
//		turboRegProgressBar.stepProgressBar();
	}
//	turboRegProgressBar.workloadDone(outNy);
} /* affineTransform */

void turboRegTransform::affineTransform (
	Matrix_d& matrix,
	Vector_b& outMsk
) {
	double yx;
	double yy;
	double x0;
	double y0;
	int xMsk;
	int yMsk;
	int k = 0;
	ProgressBar turboRegProgressBar ("turboRegTransform::affineTransform_M_V");
//	turboRegProgressBar.addWorkload(outNy);
	yx = matrix[0][0];
	yy = matrix[1][0];
	for (int v = 0; (v < outNy); v++) {
		x0 = yx;
		y0 = yy;
		for (int u = 0; (u < outNx); u++) {
			x = x0;
			y = y0;
			xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
			yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
			if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
				xMsk += yMsk * inNx;
				if (accelerated) {
					outImg[k] = inImg[xMsk];
				}
				else {
					xIndexes();
					yIndexes();
					x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
					y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
					xWeights();
					yWeights();
					outImg[k] = interpolate();
				}
				outMsk[k++] = inMsk[xMsk] ? 255 : 0;
			}
			else {
				outImg[k] = 0.0;
				outMsk[k++] = 0;
			}
			x0 += matrix[0][1];
			y0 += matrix[1][1];
		}
		yx += matrix[0][2];
		yy += matrix[1][2];
//		turboRegProgressBar.stepProgressBar();
	}
//	turboRegProgressBar.workloadDone(outNy);
}

void turboRegTransform::bilinearTransform (
	Matrix_d& matrix
) {
	double yx;
	double yy;
	double yxy;
	double yyy;
	double x0;
	double y0;
	int xMsk;
	int yMsk;
	int k = 0;
	ProgressBar turboRegProgressBar ("turboRegTransform::bilinearTransform_M");
//	turboRegProgressBar.addWorkload(outNy);
	yx = matrix[0][0];
	yy = matrix[1][0];
	yxy = 0.0;
	yyy = 0.0;
	for (int v = 0; (v < outNy); v++) {
		x0 = yx;
		y0 = yy;
		for (int u = 0; (u < outNx); u++) {
			x = x0;
			y = y0;
			xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
			yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
			if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
				xMsk += yMsk * inNx;
				if (accelerated) {
					outImg[k++] = inImg[xMsk];
				}
				else {
					xIndexes();
					yIndexes();
					x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
					y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
					xWeights();
					yWeights();
					outImg[k++] = interpolate();
				}
			}
			else {
				outImg[k++] = 0.0;
			}
			x0 += matrix[0][1] + yxy;
			y0 += matrix[1][1] + yyy;
		}
		yx += matrix[0][2];
		yy += matrix[1][2];
		yxy += matrix[0][3];
		yyy += matrix[1][3];
//		turboRegProgressBar.stepProgressBar();
	}
//	turboRegProgressBar.workloadDone(outNy);
}

void turboRegTransform::bilinearTransform (
	Matrix_d& matrix,
	Vector_b& outMsk
) {
	double yx;
	double yy;
	double yxy;
	double yyy;
	double x0;
	double y0;
	int xMsk;
	int yMsk;
	int k = 0;
	ProgressBar turboRegProgressBar ("turboRegTransform::bilinearTransform_M_V");
//	turboRegProgressBar.addWorkload(outNy);
	yx = matrix[0][0];
	yy = matrix[1][0];
	yxy = 0.0;
	yyy = 0.0;
	for (int v = 0; (v < outNy); v++) {
		x0 = yx;
		y0 = yy;
		for (int u = 0; (u < outNx); u++) {
			x = x0;
			y = y0;
			xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
			yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
			if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
				xMsk += yMsk * inNx;
				if (accelerated) {
					outImg[k] = inImg[xMsk];
				}
				else {
					xIndexes();
					yIndexes();
					x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
					y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
					xWeights();
					yWeights();
					outImg[k] = interpolate();
				}
				outMsk[k++] = (inMsk[xMsk]) ? 255 : 0;
			}
			else {
				outImg[k] = 0.0;
				outMsk[k++] = 0;
			}
			x0 += matrix[0][1] + yxy;
			y0 += matrix[1][1] + yyy;
		}
		yx += matrix[0][2];
		yy += matrix[1][2];
		yxy += matrix[0][3];
		yyy += matrix[1][3];
//		turboRegProgressBar.stepProgressBar();
	}
//	turboRegProgressBar.workloadDone(outNy);
}

void turboRegTransform::computeBilinearGradientConstants (
) {
	double u1 = (*targetPoint)[0][0];
	double u2 = (*targetPoint)[1][0];
	double u3 = (*targetPoint)[2][0];
	double u4 = (*targetPoint)[3][0];
	double v1 = (*targetPoint)[0][1];
	double v2 = (*targetPoint)[1][1];
	double v3 = (*targetPoint)[2][1];
	double v4 = (*targetPoint)[3][1];
	double u12 = u1 - u2;
	double u13 = u1 - u3;
	double u14 = u1 - u4;
	double u23 = u2 - u3;
	double u24 = u2 - u4;
	double u34 = u3 - u4;
	double v12 = v1 - v2;
	double v13 = v1 - v3;
	double v14 = v1 - v4;
	double v23 = v2 - v3;
	double v24 = v2 - v4;
	double v34 = v3 - v4;
	double uv12 = u1 * u2 * v12;
	double uv13 = u1 * u3 * v13;
	double uv14 = u1 * u4 * v14;
	double uv23 = u2 * u3 * v23;
	double uv24 = u2 * u4 * v24;
	double uv34 = u3 * u4 * v34;
	double det = uv12 * v34 - uv13 * v24 + uv14 * v23 + uv23 * v14 - uv24 * v13 + uv34 * v12;
	c0 = (-uv34 * v2 + uv24 * v3 - uv23 * v4) / det;
	c0u = (u3 * v3 * v24 - u2 * v2 * v34 - u4 * v4 * v23) / det;
	c0v = (uv23 - uv24 + uv34) / det;
	c0uv = (u4 * v23 - u3 * v24 + u2 * v34) / det;
	c1 = (uv34 * v1 - uv14 * v3 + uv13 * v4) / det;
	c1u = (-u3 * v3 * v14 + u1 * v1 * v34 + u4 * v4 * v13) / det;
	c1v = (-uv13 + uv14 - uv34) / det;
	c1uv = (-u4 * v13 + u3 * v14 - u1 * v34) / det;
	c2 = (-uv24 * v1 + uv14 * v2 - uv12 * v4) / det;
	c2u = (u2 * v2 * v14 - u1 * v1 * v24 - u4 * v4 * v12) / det;
	c2v = (uv12 - uv14 + uv24) / det;
	c2uv = (u4 * v12 - u2 * v14 + u1 * v24) / det;
	c3 = (uv23 * v1 - uv13 * v2 + uv12 * v3) / det;
	c3u = (-u2 * v2 * v13 + u1 * v1 * v23 + u3 * v3 * v12) / det;
	c3v = (-uv12 + uv13 - uv23) / det;
	c3uv = (-u3 * v1 + u2 * v13 + u3 * v2 - u1 * v23) / det;
}

double turboRegTransform::getAffineMeanSquares (
	Matrix_d& sourcePoint,
	Matrix_d& matrix
) {
	double u1 = sourcePoint[0][0];
	double u2 = sourcePoint[1][0];
	double u3 = sourcePoint[2][0];
	double v1 = sourcePoint[0][1];
	double v2 = sourcePoint[1][1];
	double v3 = sourcePoint[2][1];
	double uv32 = u3 * v2 - u2 * v3;
	double uv21 = u2 * v1 - u1 * v2;
	double uv13 = u1 * v3 - u3 * v1;
	double det = uv32 + uv21 + uv13;
	double yx;
	double yy;
	double x0;
	double y0;
	double difference;
	double meanSquares = 0.0;
	long area = 0L;
	int xMsk;
	int yMsk;
	int k = 0;
	ProgressBar turboRegProgressBar ("turboRegTransform::getAffineMeanSquares");
	if (!outMsk_valid) {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	else {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (outMsk[k] && inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	return(meanSquares / ((double)area * fabs(det / targetJacobian)));
} /* getAffineMeanSquares */

/*------------------------------------------------------------------*/
double turboRegTransform::getAffineMeanSquares (
	Matrix_d& sourcePoint,
	Matrix_d& matrix,
	Vector_d& gradient
) {
	double u1 = sourcePoint[0][0];
	double u2 = sourcePoint[1][0];
	double u3 = sourcePoint[2][0];
	double v1 = sourcePoint[0][1];
	double v2 = sourcePoint[1][1];
	double v3 = sourcePoint[2][1];
	double uv32 = u3 * v2 - u2 * v3;
	double uv21 = u2 * v1 - u1 * v2;
	double uv13 = u1 * v3 - u3 * v1;
	double det = uv32 + uv21 + uv13;
	double u12 = (u1 - u2) /det;
	double u23 = (u2 - u3) /det;
	double u31 = (u3 - u1) /det;
	double v12 = (v1 - v2) /det;
	double v23 = (v2 - v3) /det;
	double v31 = (v3 - v1) /det;
	double yx;
	double yy;
	double x0;
	double y0;
	double difference;
	double meanSquares = 0.0;
	double g0;
	double g1;
	double g2;
	double dx0;
	double dx1;
	double dx2;
	double dy0;
	double dy1;
	double dy2;
	long area = 0L;
	int xMsk;
	int yMsk;
	int k = 0;
	ProgressBar turboRegProgressBar ("urboRegTransform::getAffineMeanSquares_MMV");
	uv32 /= det;
	uv21 /= det;
	uv13 /= det;
	for (int i = 0; (i < transformation); i++) {
		gradient[i] = 0.0;
	}
	if (!outMsk_valid) {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
						g0 = u23 * (double)v - v23 * (double)u + uv32;
						g1 = u31 * (double)v - v31 * (double)u + uv13;
						g2 = u12 * (double)v - v12 * (double)u + uv21;
						dx0 = xGradient[k] * g0;
						dy0 = yGradient[k] * g0;
						dx1 = xGradient[k] * g1;
						dy1 = yGradient[k] * g1;
						dx2 = xGradient[k] * g2;
						dy2 = yGradient[k] * g2;
						gradient[0] += difference * dx0;
						gradient[1] += difference * dy0;
						gradient[2] += difference * dx1;
						gradient[3] += difference * dy1;
						gradient[4] += difference * dx2;
						gradient[5] += difference * dy2;
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	else {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (outMsk[k] && inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
						g0 = u23 * (double)v - v23 * (double)u + uv32;
						g1 = u31 * (double)v - v31 * (double)u + uv13;
						g2 = u12 * (double)v - v12 * (double)u + uv21;
						dx0 = xGradient[k] * g0;
						dy0 = yGradient[k] * g0;
						dx1 = xGradient[k] * g1;
						dy1 = yGradient[k] * g1;
						dx2 = xGradient[k] * g2;
						dy2 = yGradient[k] * g2;
						gradient[0] += difference * dx0;
						gradient[1] += difference * dy0;
						gradient[2] += difference * dx1;
						gradient[3] += difference * dy1;
						gradient[4] += difference * dx2;
						gradient[5] += difference * dy2;
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	return(meanSquares / ((double)area * fabs(det / targetJacobian)));
} /* getAffineMeanSquares */

/*------------------------------------------------------------------*/
double turboRegTransform::getAffineMeanSquares (
	Matrix_d& sourcePoint,
	Matrix_d& matrix,
	Matrix_d& hessian,
	Vector_d& gradient
) {
	double u1 = sourcePoint[0][0];
	double u2 = sourcePoint[1][0];
	double u3 = sourcePoint[2][0];
	double v1 = sourcePoint[0][1];
	double v2 = sourcePoint[1][1];
	double v3 = sourcePoint[2][1];
	double uv32 = u3 * v2 - u2 * v3;
	double uv21 = u2 * v1 - u1 * v2;
	double uv13 = u1 * v3 - u3 * v1;
	double det = uv32 + uv21 + uv13;
	double u12 = (u1 - u2) /det;
	double u23 = (u2 - u3) /det;
	double u31 = (u3 - u1) /det;
	double v12 = (v1 - v2) /det;
	double v23 = (v2 - v3) /det;
	double v31 = (v3 - v1) /det;
	double yx;
	double yy;
	double x0;
	double y0;
	double difference;
	double meanSquares = 0.0;
	double g0;
	double g1;
	double g2;
	double dx0;
	double dx1;
	double dx2;
	double dy0;
	double dy1;
	double dy2;
	long area = 0L;
	int xMsk;
	int yMsk;
	int k = 0;
	ProgressBar turboRegProgressBar ("turboRegTransform::getAffineMeanSquares_MMMV");
	uv32 /= det;
	uv21 /= det;
	uv13 /= det;
	for (int i = 0; (i < transformation); i++) {
		gradient[i] = 0.0;
		for (int j = 0; (j < transformation); j++) {
			hessian[i][j] = 0.0;
		}
	}
	if (!outMsk_valid) {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
						g0 = u23 * (double)v - v23 * (double)u + uv32;
						g1 = u31 * (double)v - v31 * (double)u + uv13;
						g2 = u12 * (double)v - v12 * (double)u + uv21;
						dx0 = xGradient[k] * g0;
						dy0 = yGradient[k] * g0;
						dx1 = xGradient[k] * g1;
						dy1 = yGradient[k] * g1;
						dx2 = xGradient[k] * g2;
						dy2 = yGradient[k] * g2;
						gradient[0] += difference * dx0;
						gradient[1] += difference * dy0;
						gradient[2] += difference * dx1;
						gradient[3] += difference * dy1;
						gradient[4] += difference * dx2;
						gradient[5] += difference * dy2;
						hessian[0][0] += dx0 * dx0;
						hessian[0][1] += dx0 * dy0;
						hessian[0][2] += dx0 * dx1;
						hessian[0][3] += dx0 * dy1;
						hessian[0][4] += dx0 * dx2;
						hessian[0][5] += dx0 * dy2;
						hessian[1][1] += dy0 * dy0;
						hessian[1][2] += dy0 * dx1;
						hessian[1][3] += dy0 * dy1;
						hessian[1][4] += dy0 * dx2;
						hessian[1][5] += dy0 * dy2;
						hessian[2][2] += dx1 * dx1;
						hessian[2][3] += dx1 * dy1;
						hessian[2][4] += dx1 * dx2;
						hessian[2][5] += dx1 * dy2;
						hessian[3][3] += dy1 * dy1;
						hessian[3][4] += dy1 * dx2;
						hessian[3][5] += dy1 * dy2;
						hessian[4][4] += dx2 * dx2;
						hessian[4][5] += dx2 * dy2;
						hessian[5][5] += dy2 * dy2;
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	else {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (outMsk[k] && inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
						g0 = u23 * (double)v - v23 * (double)u + uv32;
						g1 = u31 * (double)v - v31 * (double)u + uv13;
						g2 = u12 * (double)v - v12 * (double)u + uv21;
						dx0 = xGradient[k] * g0;
						dy0 = yGradient[k] * g0;
						dx1 = xGradient[k] * g1;
						dy1 = yGradient[k] * g1;
						dx2 = xGradient[k] * g2;
						dy2 = yGradient[k] * g2;
						gradient[0] += difference * dx0;
						gradient[1] += difference * dy0;
						gradient[2] += difference * dx1;
						gradient[3] += difference * dy1;
						gradient[4] += difference * dx2;
						gradient[5] += difference * dy2;
						hessian[0][0] += dx0 * dx0;
						hessian[0][1] += dx0 * dy0;
						hessian[0][2] += dx0 * dx1;
						hessian[0][3] += dx0 * dy1;
						hessian[0][4] += dx0 * dx2;
						hessian[0][5] += dx0 * dy2;
						hessian[1][1] += dy0 * dy0;
						hessian[1][2] += dy0 * dx1;
						hessian[1][3] += dy0 * dy1;
						hessian[1][4] += dy0 * dx2;
						hessian[1][5] += dy0 * dy2;
						hessian[2][2] += dx1 * dx1;
						hessian[2][3] += dx1 * dy1;
						hessian[2][4] += dx1 * dx2;
						hessian[2][5] += dx1 * dy2;
						hessian[3][3] += dy1 * dy1;
						hessian[3][4] += dy1 * dx2;
						hessian[3][5] += dy1 * dy2;
						hessian[4][4] += dx2 * dx2;
						hessian[4][5] += dx2 * dy2;
						hessian[5][5] += dy2 * dy2;
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	for (int i = 1; (i < transformation); i++) {
		for (int j = 0; (j < i); j++) {
			hessian[i][j] = hessian[j][i];
		}
	}
	return(meanSquares / ((double)area * fabs(det / targetJacobian)));
} /* getAffineMeanSquares */

/*------------------------------------------------------------------*/
double turboRegTransform::getBilinearMeanSquares (
	Matrix_d& matrix
) {
	double yx;
	double yy;
	double yxy;
	double yyy;
	double x0;
	double y0;
	double difference;
	double meanSquares = 0.0;
	long area = 0L;
	int xMsk;
	int yMsk;
	int k = 0;
	ProgressBar turboRegProgressBar ("turboRegTransform::getBilinearMeanSquares_M");
	if (!inMsk_valid) {
		yx = matrix[0][0];
		yy = matrix[1][0];
		yxy = 0.0;
		yyy = 0.0;
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				if (outMsk[k]) {
					x = x0;
					y = y0;
					xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
					yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
					if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
						xIndexes();
						yIndexes();
						area++;
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = interpolate() - (double)outImg[k];
						meanSquares += difference * difference;
					}
				}
				x0 += matrix[0][1] + yxy;
				y0 += matrix[1][1] + yyy;
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
			yxy += matrix[0][3];
			yyy += matrix[1][3];
		}
	}
	else {
		yx = matrix[0][0];
		yy = matrix[1][0];
		yxy = 0.0;
		yyy = 0.0;
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					xMsk += yMsk * inNx;
					if (outMsk[k] && inMsk[xMsk]) {
						xIndexes();
						yIndexes();
						area++;
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = interpolate() - (double)outImg[k];
						meanSquares += difference * difference;
					}
				}
				x0 += matrix[0][1] + yxy;
				y0 += matrix[1][1] + yyy;
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
			yxy += matrix[0][3];
			yyy += matrix[1][3];
		}
	}
	return(meanSquares / (double)area);
} /* getBilinearMeanSquares */

/*------------------------------------------------------------------*/
double turboRegTransform::getBilinearMeanSquares (
	Matrix_d& matrix,
	Matrix_d& hessian,
	Vector_d& gradient
) {
	double yx;
	double yy;
	double yxy;
	double yyy;
	double x0;
	double y0;
	double uv;
	double xGradient;
	double yGradient;
	double difference;
	double meanSquares = 0.0;
	double g0;
	double g1;
	double g2;
	double g3;
	double dx0;
	double dx1;
	double dx2;
	double dx3;
	double dy0;
	double dy1;
	double dy2;
	double dy3;
	long area = 0L;
	int xMsk;
	int yMsk;
	int k = 0;
	computeBilinearGradientConstants();
	for (int i = 0; (i < transformation); i++) {
		gradient[i] = 0.0;
		for (int j = 0; (j < transformation); j++) {
			hessian[i][j] = 0.0;
		}
	}
	if (!inMsk_valid) {
		yx = matrix[0][0];
		yy = matrix[1][0];
		yxy = 0.0;
		yyy = 0.0;
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				if (outMsk[k]) {
					x = x0;
					y = y0;
					xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
					yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
					if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xDxWeights();
						yDyWeights();
						difference = interpolate() - (double)outImg[k];
						meanSquares += difference * difference;
						xGradient = interpolateDx();
						yGradient = interpolateDy();
						uv = (double)u * (double)v;
						g0 = c0uv * uv + c0u * (double)u + c0v * (double)v + c0;
						g1 = c1uv * uv + c1u * (double)u + c1v * (double)v + c1;
						g2 = c2uv * uv + c2u * (double)u + c2v * (double)v + c2;
						g3 = c3uv * uv + c3u * (double)u + c3v * (double)v + c3;
						dx0 = xGradient * g0;
						dy0 = yGradient * g0;
						dx1 = xGradient * g1;
						dy1 = yGradient * g1;
						dx2 = xGradient * g2;
						dy2 = yGradient * g2;
						dx3 = xGradient * g3;
						dy3 = yGradient * g3;
						gradient[0] += difference * dx0;
						gradient[1] += difference * dy0;
						gradient[2] += difference * dx1;
						gradient[3] += difference * dy1;
						gradient[4] += difference * dx2;
						gradient[5] += difference * dy2;
						gradient[6] += difference * dx3;
						gradient[7] += difference * dy3;
						hessian[0][0] += dx0 * dx0;
						hessian[0][1] += dx0 * dy0;
						hessian[0][2] += dx0 * dx1;
						hessian[0][3] += dx0 * dy1;
						hessian[0][4] += dx0 * dx2;
						hessian[0][5] += dx0 * dy2;
						hessian[0][6] += dx0 * dx3;
						hessian[0][7] += dx0 * dy3;
						hessian[1][1] += dy0 * dy0;
						hessian[1][2] += dy0 * dx1;
						hessian[1][3] += dy0 * dy1;
						hessian[1][4] += dy0 * dx2;
						hessian[1][5] += dy0 * dy2;
						hessian[1][6] += dy0 * dx3;
						hessian[1][7] += dy0 * dy3;
						hessian[2][2] += dx1 * dx1;
						hessian[2][3] += dx1 * dy1;
						hessian[2][4] += dx1 * dx2;
						hessian[2][5] += dx1 * dy2;
						hessian[2][6] += dx1 * dx3;
						hessian[2][7] += dx1 * dy3;
						hessian[3][3] += dy1 * dy1;
						hessian[3][4] += dy1 * dx2;
						hessian[3][5] += dy1 * dy2;
						hessian[3][6] += dy1 * dx3;
						hessian[3][7] += dy1 * dy3;
						hessian[4][4] += dx2 * dx2;
						hessian[4][5] += dx2 * dy2;
						hessian[4][6] += dx2 * dx3;
						hessian[4][7] += dx2 * dy3;
						hessian[5][5] += dy2 * dy2;
						hessian[5][6] += dy2 * dx3;
						hessian[5][7] += dy2 * dy3;
						hessian[6][6] += dx3 * dx3;
						hessian[6][7] += dx3 * dy3;
						hessian[7][7] += dy3 * dy3;
					}
				}
				x0 += matrix[0][1] + yxy;
				y0 += matrix[1][1] + yyy;
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
			yxy += matrix[0][3];
			yyy += matrix[1][3];
		}
	}
	else {
		yx = matrix[0][0];
		yy = matrix[1][0];
		yxy = 0.0;
		yyy = 0.0;
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					xMsk += yMsk * inNx;
					if (outMsk[k] && inMsk[xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xDxWeights();
						yDyWeights();
						difference = interpolate() - (double)outImg[k];
						meanSquares += difference * difference;
						xGradient = interpolateDx();
						yGradient = interpolateDy();
						uv = (double)u * (double)v;
						g0 = c0uv * uv + c0u * (double)u + c0v * (double)v + c0;
						g1 = c1uv * uv + c1u * (double)u + c1v * (double)v + c1;
						g2 = c2uv * uv + c2u * (double)u + c2v * (double)v + c2;
						g3 = c3uv * uv + c3u * (double)u + c3v * (double)v + c3;
						dx0 = xGradient * g0;
						dy0 = yGradient * g0;
						dx1 = xGradient * g1;
						dy1 = yGradient * g1;
						dx2 = xGradient * g2;
						dy2 = yGradient * g2;
						dx3 = xGradient * g3;
						dy3 = yGradient * g3;
						gradient[0] += difference * dx0;
						gradient[1] += difference * dy0;
						gradient[2] += difference * dx1;
						gradient[3] += difference * dy1;
						gradient[4] += difference * dx2;
						gradient[5] += difference * dy2;
						gradient[6] += difference * dx3;
						gradient[7] += difference * dy3;
						hessian[0][0] += dx0 * dx0;
						hessian[0][1] += dx0 * dy0;
						hessian[0][2] += dx0 * dx1;
						hessian[0][3] += dx0 * dy1;
						hessian[0][4] += dx0 * dx2;
						hessian[0][5] += dx0 * dy2;
						hessian[0][6] += dx0 * dx3;
						hessian[0][7] += dx0 * dy3;
						hessian[1][1] += dy0 * dy0;
						hessian[1][2] += dy0 * dx1;
						hessian[1][3] += dy0 * dy1;
						hessian[1][4] += dy0 * dx2;
						hessian[1][5] += dy0 * dy2;
						hessian[1][6] += dy0 * dx3;
						hessian[1][7] += dy0 * dy3;
						hessian[2][2] += dx1 * dx1;
						hessian[2][3] += dx1 * dy1;
						hessian[2][4] += dx1 * dx2;
						hessian[2][5] += dx1 * dy2;
						hessian[2][6] += dx1 * dx3;
						hessian[2][7] += dx1 * dy3;
						hessian[3][3] += dy1 * dy1;
						hessian[3][4] += dy1 * dx2;
						hessian[3][5] += dy1 * dy2;
						hessian[3][6] += dy1 * dx3;
						hessian[3][7] += dy1 * dy3;
						hessian[4][4] += dx2 * dx2;
						hessian[4][5] += dx2 * dy2;
						hessian[4][6] += dx2 * dx3;
						hessian[4][7] += dx2 * dy3;
						hessian[5][5] += dy2 * dy2;
						hessian[5][6] += dy2 * dx3;
						hessian[5][7] += dy2 * dy3;
						hessian[6][6] += dx3 * dx3;
						hessian[6][7] += dx3 * dy3;
						hessian[7][7] += dy3 * dy3;
					}
				}
				x0 += matrix[0][1] + yxy;
				y0 += matrix[1][1] + yyy;
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
			yxy += matrix[0][3];
			yyy += matrix[1][3];
		}
	}
	for (int i = 1; (i < transformation); i++) {
		for (int j = 0; (j < i); j++) {
			hessian[i][j] = hessian[j][i];
		}
	}
	return(meanSquares / (double)area);
} /* getBilinearMeanSquares */

/*------------------------------------------------------------------*/
double turboRegTransform::getRigidBodyMeanSquares (
	Matrix_d& matrix
) {
	double yx;
	double yy;
	double x0;
	double y0;
	double difference;
	double meanSquares = 0.0;
	long area = 0L;
	int xMsk;
	int yMsk;
	int k = 0;
	if (!outMsk_valid) {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	else {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (outMsk[k] && inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	return(meanSquares / (double)area);
} /* getRigidBodyMeanSquares */

/*------------------------------------------------------------------*/
double turboRegTransform::getRigidBodyMeanSquares (
	Matrix_d& matrix,
	Vector_d& gradient

) {
	double yx;
	double yy;
	double x0;
	double y0;
	double difference;
	double meanSquares = 0.0;
	long area = 0L;
	int xMsk;
	int yMsk;
	int k = 0;
	for (int i = 0; (i < transformation); i++) {
		gradient[i] = 0.0;
	}
	if (!outMsk_valid) {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
						gradient[0] += difference * (yGradient[k] * (double)u
							- xGradient[k] * (double)v);
						gradient[1] += difference * xGradient[k];
						gradient[2] += difference * yGradient[k];
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	else {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (outMsk[k] && inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
						gradient[0] += difference * (yGradient[k] * (double)u
							- xGradient[k] * (double)v);
						gradient[1] += difference * xGradient[k];
						gradient[2] += difference * yGradient[k];
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	return(meanSquares / (double)area);
} /* getRigidBodyMeanSquares */

/*------------------------------------------------------------------*/
double turboRegTransform::getRigidBodyMeanSquares (
	Matrix_d& matrix,
	Matrix_d& hessian,
	Vector_d& gradient
) {
	double yx;
	double yy;
	double x0;
	double y0;
	double dTheta;
	double difference;
	double meanSquares = 0.0;
	long area = 0L;
	int xMsk;
	int yMsk;
	int k = 0;
	for (int i = 0; (i < transformation); i++) {
		gradient[i] = 0.0;
		for (int j = 0; (j < transformation); j++) {
			hessian[i][j] = 0.0;
		}
	}
	if (!outMsk_valid) {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
						dTheta = yGradient[k] * (double)u - xGradient[k] * (double)v;
						gradient[0] += difference * dTheta;
						gradient[1] += difference * xGradient[k];
						gradient[2] += difference * yGradient[k];
						hessian[0][0] += dTheta * dTheta;
						hessian[0][1] += dTheta * xGradient[k];
						hessian[0][2] += dTheta * yGradient[k];
						hessian[1][1] += xGradient[k] * xGradient[k];
						hessian[1][2] += xGradient[k] * yGradient[k];
						hessian[2][2] += yGradient[k] * yGradient[k];
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	else {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (outMsk[k] && inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
						dTheta = yGradient[k] * (double)u - xGradient[k] * (double)v;
						gradient[0] += difference * dTheta;
						gradient[1] += difference * xGradient[k];
						gradient[2] += difference * yGradient[k];
						hessian[0][0] += dTheta * dTheta;
						hessian[0][1] += dTheta * xGradient[k];
						hessian[0][2] += dTheta * yGradient[k];
						hessian[1][1] += xGradient[k] * xGradient[k];
						hessian[1][2] += xGradient[k] * yGradient[k];
						hessian[2][2] += yGradient[k] * yGradient[k];
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	for (int i = 1; (i < transformation); i++) {
		for (int j = 0; (j < i); j++) {
			hessian[i][j] = hessian[j][i];
		}
	}
	return(meanSquares / (double)area);
} /* getRigidBodyMeanSquares */

double turboRegTransform::getScaledRotationMeanSquares (
	Matrix_d& sourcePoint,
	Matrix_d& matrix
) {
	double u1 = sourcePoint[0][0];
	double u2 = sourcePoint[1][0];
	double v1 = sourcePoint[0][1];
	double v2 = sourcePoint[1][1];
	double u12 = u1 - u2;
	double v12 = v1 - v2;
	double uv2 = u12 * u12 + v12 * v12;
	double yx;
	double yy;
	double x0;
	double y0;
	double difference;
	double meanSquares = 0.0;
	long area = 0L;
	int xMsk;
	int yMsk;
	int k = 0;
	if (!outMsk_valid) {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	else {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (outMsk[k] && inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	return(meanSquares / ((double)area * uv2 / targetJacobian));
} /* getScaledRotationMeanSquares */

double turboRegTransform::getScaledRotationMeanSquares (
	Matrix_d& sourcePoint,
	Matrix_d& matrix,
	Vector_d& gradient
) {
	double u1 = sourcePoint[0][0];
	double u2 = sourcePoint[1][0];
	double v1 = sourcePoint[0][1];
	double v2 = sourcePoint[1][1];
	double u12 = u1 - u2;
	double v12 = v1 - v2;
	double uv2 = u12 * u12 + v12 * v12;
	double c = 0.5 * (u2 * v1 - u1 * v2) / uv2;
	double c1 = u12 / uv2;
	double c2 = v12 / uv2;
	double c3 = (uv2 - u12 * v12) / uv2;
	double c4 = (uv2 + u12 * v12) / uv2;
	double c5 = c + u1 * c1 + u2 * c2;
	double c6 = c * (u12 * u12 - v12 * v12) / uv2;
	double c7 = c1 * c4;
	double c8 = c1 - c2 - c1 * c2 * v12;
	double c9 = c1 + c2 - c1 * c2 * u12;
	double c0 = c2 * c3;
	double dgxx0 = c1 * u2 + c2 * v2;
	double dgyx0 = 2.0 * c;
	double dgxx1 = c5 + c6;
	double dgyy1 = c5 - c6;
	double yx;
	double yy;
	double x0;
	double y0;
	double difference;
	double meanSquares = 0.0;
	double gxx0;
	double gxx1;
	double gxy0;
	double gxy1;
	double gyx0;
	double gyx1;
	double gyy0;
	double gyy1;
	double dx0;
	double dx1;
	double dy0;
	double dy1;
	long area = 0L;
	int xMsk;
	int yMsk;
	int k = 0;
	for (int i = 0; (i < transformation); i++) {
		gradient[i] = 0.0;
	}
	if (!outMsk_valid) {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
						gxx0 = (double)u * c1 + (double)v * c2 - dgxx0;
						gyx0 = (double)v * c1 - (double)u * c2 + dgyx0;
						gxy0 = -gyx0;
						gyy0 = gxx0;
						gxx1 = (double)v * c8 - (double)u * c7 + dgxx1;
						gyx1 = -c3 * gyx0;
						gxy1 = c4 * gyx0;
						gyy1 = dgyy1 - (double)u * c9 - (double)v * c0;
						dx0 = xGradient[k] * gxx0 + yGradient[k] * gyx0;
						dy0 = xGradient[k] * gxy0 + yGradient[k] * gyy0;
						dx1 = xGradient[k] * gxx1 + yGradient[k] * gyx1;
						dy1 = xGradient[k] * gxy1 + yGradient[k] * gyy1;
						gradient[0] += difference * dx0;
						gradient[1] += difference * dy0;
						gradient[2] += difference * dx1;
						gradient[3] += difference * dy1;
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	else {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (outMsk[k] && inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
						gxx0 = (double)u * c1 + (double)v * c2 - dgxx0;
						gyx0 = (double)v * c1 - (double)u * c2 + dgyx0;
						gxy0 = -gyx0;
						gyy0 = gxx0;
						gxx1 = (double)v * c8 - (double)u * c7 + dgxx1;
						gyx1 = -c3 * gyx0;
						gxy1 = c4 * gyx0;
						gyy1 = dgyy1 - (double)u * c9 - (double)v * c0;
						dx0 = xGradient[k] * gxx0 + yGradient[k] * gyx0;
						dy0 = xGradient[k] * gxy0 + yGradient[k] * gyy0;
						dx1 = xGradient[k] * gxx1 + yGradient[k] * gyx1;
						dy1 = xGradient[k] * gxy1 + yGradient[k] * gyy1;
						gradient[0] += difference * dx0;
						gradient[1] += difference * dy0;
						gradient[2] += difference * dx1;
						gradient[3] += difference * dy1;
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	return(meanSquares / ((double)area * uv2 / targetJacobian));
} /* getScaledRotationMeanSquares */

double turboRegTransform::getScaledRotationMeanSquares (
	Matrix_d& sourcePoint,
	Matrix_d& matrix,
	Matrix_d& hessian,
	Vector_d& gradient
) {
	double u1 = sourcePoint[0][0];
	double u2 = sourcePoint[1][0];
	double v1 = sourcePoint[0][1];
	double v2 = sourcePoint[1][1];
	double u12 = u1 - u2;
	double v12 = v1 - v2;
	double uv2 = u12 * u12 + v12 * v12;
	double c = 0.5 * (u2 * v1 - u1 * v2) / uv2;
	double c1 = u12 / uv2;
	double c2 = v12 / uv2;
	double c3 = (uv2 - u12 * v12) / uv2;
	double c4 = (uv2 + u12 * v12) / uv2;
	double c5 = c + u1 * c1 + u2 * c2;
	double c6 = c * (u12 * u12 - v12 * v12) / uv2;
	double c7 = c1 * c4;
	double c8 = c1 - c2 - c1 * c2 * v12;
	double c9 = c1 + c2 - c1 * c2 * u12;
	double c0 = c2 * c3;
	double dgxx0 = c1 * u2 + c2 * v2;
	double dgyx0 = 2.0 * c;
	double dgxx1 = c5 + c6;
	double dgyy1 = c5 - c6;
	double yx;
	double yy;
	double x0;
	double y0;
	double difference;
	double meanSquares = 0.0;
	double gxx0;
	double gxx1;
	double gxy0;
	double gxy1;
	double gyx0;
	double gyx1;
	double gyy0;
	double gyy1;
	double dx0;
	double dx1;
	double dy0;
	double dy1;
	long area = 0L;
	int xMsk;
	int yMsk;
	int k = 0;
	for (int i = 0; (i < transformation); i++) {
		gradient[i] = 0.0;
		for (int j = 0; (j < transformation); j++) {
			hessian[i][j] = 0.0;
		}
	}
	if (!outMsk_valid) {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
						gxx0 = (double)u * c1 + (double)v * c2 - dgxx0;
						gyx0 = (double)v * c1 - (double)u * c2 + dgyx0;
						gxy0 = -gyx0;
						gyy0 = gxx0;
						gxx1 = (double)v * c8 - (double)u * c7 + dgxx1;
						gyx1 = -c3 * gyx0;
						gxy1 = c4 * gyx0;
						gyy1 = dgyy1 - (double)u * c9 - (double)v * c0;
						dx0 = xGradient[k] * gxx0 + yGradient[k] * gyx0;
						dy0 = xGradient[k] * gxy0 + yGradient[k] * gyy0;
						dx1 = xGradient[k] * gxx1 + yGradient[k] * gyx1;
						dy1 = xGradient[k] * gxy1 + yGradient[k] * gyy1;
						gradient[0] += difference * dx0;
						gradient[1] += difference * dy0;
						gradient[2] += difference * dx1;
						gradient[3] += difference * dy1;
						hessian[0][0] += dx0 * dx0;
						hessian[0][1] += dx0 * dy0;
						hessian[0][2] += dx0 * dx1;
						hessian[0][3] += dx0 * dy1;
						hessian[1][1] += dy0 * dy0;
						hessian[1][2] += dy0 * dx1;
						hessian[1][3] += dy0 * dy1;
						hessian[2][2] += dx1 * dx1;
						hessian[2][3] += dx1 * dy1;
						hessian[3][3] += dy1 * dy1;
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	else {
		yx = matrix[0][0];
		yy = matrix[1][0];
		for (int v = 0; (v < outNy); v++) {
			x0 = yx;
			y0 = yy;
			for (int u = 0; (u < outNx); u++, k++) {
				x = x0;
				y = y0;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx) && (0 <= yMsk) && (yMsk < inNy)) {
					if (outMsk[k] && inMsk[yMsk * inNx + xMsk]) {
						area++;
						xIndexes();
						yIndexes();
						x -= (0.0 <= x) ? ((int)x) : ((int)x - 1);
						y -= (0.0 <= y) ? ((int)y) : ((int)y - 1);
						xWeights();
						yWeights();
						difference = (double)outImg[k] - interpolate();
						meanSquares += difference * difference;
						gxx0 = (double)u * c1 + (double)v * c2 - dgxx0;
						gyx0 = (double)v * c1 - (double)u * c2 + dgyx0;
						gxy0 = -gyx0;
						gyy0 = gxx0;
						gxx1 = (double)v * c8 - (double)u * c7 + dgxx1;
						gyx1 = -c3 * gyx0;
						gxy1 = c4 * gyx0;
						gyy1 = dgyy1 - (double)u * c9 - (double)v * c0;
						dx0 = xGradient[k] * gxx0 + yGradient[k] * gyx0;
						dy0 = xGradient[k] * gxy0 + yGradient[k] * gyy0;
						dx1 = xGradient[k] * gxx1 + yGradient[k] * gyx1;
						dy1 = xGradient[k] * gxy1 + yGradient[k] * gyy1;
						gradient[0] += difference * dx0;
						gradient[1] += difference * dy0;
						gradient[2] += difference * dx1;
						gradient[3] += difference * dy1;
						hessian[0][0] += dx0 * dx0;
						hessian[0][1] += dx0 * dy0;
						hessian[0][2] += dx0 * dx1;
						hessian[0][3] += dx0 * dy1;
						hessian[1][1] += dy0 * dy0;
						hessian[1][2] += dy0 * dx1;
						hessian[1][3] += dy0 * dy1;
						hessian[2][2] += dx1 * dx1;
						hessian[2][3] += dx1 * dy1;
						hessian[3][3] += dy1 * dy1;
					}
				}
				x0 += matrix[0][1];
				y0 += matrix[1][1];
			}
			yx += matrix[0][2];
			yy += matrix[1][2];
		}
	}
	for (int i = 1; (i < transformation); i++) {
		for (int j = 0; (j < i); j++) {
			hessian[i][j] = hessian[j][i];
		}
	}
	return(meanSquares / ((double)area * uv2 / targetJacobian));
} /* getScaledRotationMeanSquares */

/*------------------------------------------------------------------*/
Matrix_d* turboRegTransform::getTransformationMatrix (
	Matrix_d& fromCoord,
	Matrix_d& toCoord
) {
	Matrix_d* mret=NULL;
	switch (transformation) {
		case REG_TRANSLATION: {
			Matrix_d matrix(2, Vector_d(1));
			matrix[0][0] = toCoord[0][0] - fromCoord[0][0];
			matrix[1][0] = toCoord[0][1] - fromCoord[0][1];
			mret = new Matrix_d (matrix);
			break;
		}
		case REG_RIGID_BODY: {
			double angle = atan2(fromCoord[2][0] - fromCoord[1][0],
				fromCoord[2][1] - fromCoord[1][1]) - atan2(toCoord[2][0] - toCoord[1][0],
				toCoord[2][1] - toCoord[1][1]);
			double c = cos(angle);
			double s = sin(angle);
			Matrix_d matrix(2, Vector_d(3));
			matrix[0][0] = toCoord[0][0] - c * fromCoord[0][0] + s * fromCoord[0][1];
			matrix[0][1] = c;
			matrix[0][2] = -s;
			matrix[1][0] = toCoord[0][1] - s * fromCoord[0][0] - c * fromCoord[0][1];
			matrix[1][1] = s;
			matrix[1][2] = c;
			mret = new Matrix_d (matrix);
			break;
		}
		case REG_SCALED_ROTATION: {
			Matrix_d matrix(2, Vector_d(3));
			Matrix_d a(3, Vector_d(3));
			Vector_d v(3);
			a[0][0] = 1.0;
			a[0][1] = fromCoord[0][0];
			a[0][2] = fromCoord[0][1];
			a[1][0] = 1.0;
			a[1][1] = fromCoord[1][0];
			a[1][2] = fromCoord[1][1];
			a[2][0] = 1.0;
			a[2][1] = fromCoord[0][1] - fromCoord[1][1] + fromCoord[1][0];
			a[2][2] = fromCoord[1][0] + fromCoord[1][1] - fromCoord[0][0];
			invertGauss(a);
			v[0] = toCoord[0][0];
			v[1] = toCoord[1][0];
			v[2] = toCoord[0][1] - toCoord[1][1] + toCoord[1][0];
			for (int i = 0; (i < 3); i++) {
				matrix[0][i] = 0.0;
				for (int j = 0; (j < 3); j++) {
					matrix[0][i] += a[i][j] * v[j];
				}
			}
			v[0] = toCoord[0][1];
			v[1] = toCoord[1][1];
			v[2] = toCoord[1][0] + toCoord[1][1] - toCoord[0][0];
			for (int i = 0; (i < 3); i++) {
				matrix[1][i] = 0.0;
				for (int j = 0; (j < 3); j++) {
					matrix[1][i] += a[i][j] * v[j];
				}
			}
			mret = new Matrix_d (matrix);
			break;
		}
		case REG_AFFINE: {
			Matrix_d matrix(2, Vector_d(3));
			Matrix_d a(3, Vector_d(3));
			Vector_d v(3);
			a[0][0] = 1.0;
			a[0][1] = fromCoord[0][0];
			a[0][2] = fromCoord[0][1];
			a[1][0] = 1.0;
			a[1][1] = fromCoord[1][0];
			a[1][2] = fromCoord[1][1];
			a[2][0] = 1.0;
			a[2][1] = fromCoord[2][0];
			a[2][2] = fromCoord[2][1];
			invertGauss(a);
			v[0] = toCoord[0][0];
			v[1] = toCoord[1][0];
			v[2] = toCoord[2][0];
			for (int i = 0; (i < 3); i++) {
				matrix[0][i] = 0.0;
				for (int j = 0; (j < 3); j++) {
					matrix[0][i] += a[i][j] * v[j];
				}
			}
			v[0] = toCoord[0][1];
			v[1] = toCoord[1][1];
			v[2] = toCoord[2][1];
			for (int i = 0; (i < 3); i++) {
				matrix[1][i] = 0.0;
				for (int j = 0; (j < 3); j++) {
					matrix[1][i] += a[i][j] * v[j];
				}
			}
			mret = new Matrix_d (matrix);
			break;
		}
		case REG_BILINEAR: {
			Matrix_d matrix(2, Vector_d(4));
			Matrix_d a(4, Vector_d(4));
			Vector_d v(4);
			a[0][0] = 1.0;
			a[0][1] = fromCoord[0][0];
			a[0][2] = fromCoord[0][1];
			a[0][3] = fromCoord[0][0] * fromCoord[0][1];
			a[1][0] = 1.0;
			a[1][1] = fromCoord[1][0];
			a[1][2] = fromCoord[1][1];
			a[1][3] = fromCoord[1][0] * fromCoord[1][1];
			a[2][0] = 1.0;
			a[2][1] = fromCoord[2][0];
			a[2][2] = fromCoord[2][1];
			a[2][3] = fromCoord[2][0] * fromCoord[2][1];
			a[3][0] = 1.0;
			a[3][1] = fromCoord[3][0];
			a[3][2] = fromCoord[3][1];
			a[3][3] = fromCoord[3][0] * fromCoord[3][1];
			invertGauss(a);
			v[0] = toCoord[0][0];
			v[1] = toCoord[1][0];
			v[2] = toCoord[2][0];
			v[3] = toCoord[3][0];
			for (int i = 0; (i < 4); i++) {
				matrix[0][i] = 0.0;
				for (int j = 0; (j < 4); j++) {
					matrix[0][i] += a[i][j] * v[j];
				}
			}
			v[0] = toCoord[0][1];
			v[1] = toCoord[1][1];
			v[2] = toCoord[2][1];
			v[3] = toCoord[3][1];
			for (int i = 0; (i < 4); i++) {
				matrix[1][i] = 0.0;
				for (int j = 0; (j < 4); j++) {
					matrix[1][i] += a[i][j] * v[j];
				}
			}
			mret = new Matrix_d (matrix);
			break;
		}
	}
	return mret;
}

double turboRegTransform::getTranslationMeanSquares (
	Matrix_d& matrix
) {
	double dx = matrix[0][0];
	double dy = matrix[1][0];
	double dx0 = dx;
	double difference;
	double meanSquares = 0.0;
	long area = 0L;
	int xMsk;
	int yMsk;
	int k = 0;
	x = dx - floor(dx);
	y = dy - floor(dy);
	xWeights();
	yWeights();
	if (!outMsk_valid) {
		for (int v = 0; (v < outNy); v++) {
			y = dy++;
			yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
			if ((0 <= yMsk) && (yMsk < inNy)) {
				yMsk *= inNx;
				yIndexes();
				dx = dx0;
				for (int u = 0; (u < outNx); u++, k++) {
					x = dx++;
					xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
					if ((0 <= xMsk) && (xMsk < inNx)) {
						if (inMsk[yMsk + xMsk]) {
							xIndexes();
							area++;
							difference = (double)outImg[k] - interpolate();
							meanSquares += difference * difference;
						}
					}
				}
			}
			else {
				k += outNx;
			}
		}
	}
	else {
		for (int v = 0; (v < outNy); v++) {
			y = dy++;
			yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
			if ((0 <= yMsk) && (yMsk < inNy)) {
				yMsk *= inNx;
				yIndexes();
				dx = dx0;
				for (int u = 0; (u < outNx); u++, k++) {
					x = dx++;
					xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
					if ((0 <= xMsk) && (xMsk < inNx)) {
						if (outMsk[k] && inMsk[yMsk + xMsk]) {
							xIndexes();
							area++;
							difference = (double)outImg[k] - interpolate();
							meanSquares += difference * difference;
						}
					}
				}
			}
			else {
				k += outNx;
			}
		}
	}
	return(meanSquares / (double)area);
}

double turboRegTransform::getTranslationMeanSquares (
	Matrix_d& matrix,
	Vector_d& gradient
) {
	double dx = matrix[0][0];
	double dy = matrix[1][0];
	double dx0 = dx;
	double difference;
	double meanSquares = 0.0;
	long area = 0L;
	int xMsk;
	int yMsk;
	int k = 0;
	for (int i = 0; (i < transformation); i++) {
		gradient[i] = 0.0;
	}
	x = dx - floor(dx);
	y = dy - floor(dy);
	xWeights();
	yWeights();
	if (!outMsk_valid) {
		for (int v = 0; (v < outNy); v++) {
			y = dy++;
			yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
			if ((0 <= yMsk) && (yMsk < inNy)) {
				yMsk *= inNx;
				yIndexes();
				dx = dx0;
				for (int u = 0; (u < outNx); u++, k++) {
					x = dx++;
					xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
					if ((0 <= xMsk) && (xMsk < inNx)) {
						if (inMsk[yMsk + xMsk]) {
							area++;
							xIndexes();
							difference = (double)outImg[k] - interpolate();
							meanSquares += difference * difference;
							gradient[0] += difference * xGradient[k];
							gradient[1] += difference * yGradient[k];
						}
					}
				}
			}
			else {
				k += outNx;
			}
		}
	}
	else {
		for (int v = 0; (v < outNy); v++) {
			y = dy++;
			yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
			if ((0 <= yMsk) && (yMsk < inNy)) {
				yMsk *= inNx;
				yIndexes();
				dx = dx0;
				for (int u = 0; (u < outNx); u++, k++) {
					x = dx++;
					xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
					if ((0 <= xMsk) && (xMsk < inNx)) {
						if (outMsk[k] && inMsk[yMsk + xMsk]) {
							area++;
							xIndexes();
							difference = (double)outImg[k] - interpolate();
							meanSquares += difference * difference;
							gradient[0] += difference * xGradient[k];
							gradient[1] += difference * yGradient[k];
						}
					}
				}
			}
			else {
				k += outNx;
			}
		}
	}
	return(meanSquares / (double)area);
}

double turboRegTransform::getTranslationMeanSquares (
	Matrix_d& matrix,
	Matrix_d& hessian,
	Vector_d& gradient
) {
	double dx = matrix[0][0];
	double dy = matrix[1][0];
	double dx0 = dx;
	double difference;
	double meanSquares = 0.0;
	long area = 0L;
	int xMsk;
	int yMsk;
	int k = 0;

//	if (turboRegProgressBarTr) turboRegProgressBarTr->Info("getTranslationMeanSquares");

	for (int i = 0; (i < transformation); i++) {
		gradient[i] = 0.0;
		for (int j = 0; (j < transformation); j++) {
			hessian[i][j] = 0.0;
		}
	}

	x = dx - floor(dx);
	y = dy - floor(dy);
	xWeights();
	yWeights();
	if (!outMsk_valid) {
		for (int v = 0; (v < outNy); v++) {
			y = dy++;
			yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
			if ((0 <= yMsk) && (yMsk < inNy)) {
				yMsk *= inNx;
				yIndexes();
				dx = dx0;
				for (int u = 0; (u < outNx); u++, k++) {
					x = dx++;
					xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
					if ((0 <= xMsk) && (xMsk < inNx)) {
						if (inMsk[yMsk + xMsk]) {
							area++;
							xIndexes();
							difference = outImg[k] - interpolate();
							meanSquares += difference * difference;
							gradient[0] += difference * xGradient[k];
							gradient[1] += difference * yGradient[k];
							hessian[0][0] += xGradient[k] * xGradient[k];
							hessian[0][1] += xGradient[k] * yGradient[k];
							hessian[1][1] += yGradient[k] * yGradient[k];
						}
					}
				}
			}
			else {
				k += outNx;
			}
		}
	}
	else {
		for (int v = 0; (v < outNy); v++) {
			y = dy++;
			yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
			if ((0 <= yMsk) && (yMsk < inNy)) {
				yMsk *= inNx;
				yIndexes();
				dx = dx0;
				for (int u = 0; (u < outNx); u++, k++) {
					x = dx++;
					xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
					if ((0 <= xMsk) && (xMsk < inNx)) {
						if (outMsk[k] && inMsk[yMsk + xMsk]) {
							area++;
							xIndexes();
							difference = outImg[k] - interpolate();
							meanSquares += difference * difference;
							gradient[0] += difference * xGradient[k];
							gradient[1] += difference * yGradient[k];

							hessian[0][0] += xGradient[k] * xGradient[k];
							hessian[0][1] += xGradient[k] * yGradient[k];
							hessian[1][1] += yGradient[k] * yGradient[k];
						}
					}
				}
			}
			else {
				k += outNx;
			}
		}
	}
	for (int i = 1; (i < transformation); i++) {
		for (int j = 0; (j < i); j++) {
			hessian[i][j] = hessian[j][i];
		}
	}
	return(meanSquares / (double)area);
}

double turboRegTransform::interpolate () {
	t = 0.0;
	for (int j = 0; (j < 4); j++) {
		s = 0.0;
		p = yIndex[j];
		for (int i = 0; (i < 4); i++) {
			s += xWeight[i] * inImg[p + xIndex[i]];
		}
		t += yWeight[j] * s;
	}
	return(t);
}

double turboRegTransform::interpolateDx () {
	t = 0.0;
	for (int j = 0; (j < 4); j++) {
		s = 0.0;
		p = yIndex[j];
		for (int i = 0; (i < 4); i++) {
			s += dxWeight[i] * inImg[p + xIndex[i]];
		}
		t += yWeight[j] * s;
	}
	return(t);
}

double turboRegTransform::interpolateDy () {
	t = 0.0;
	for (int j = 0; (j < 4); j++) {
		s = 0.0;
		p = yIndex[j];
		for (int i = 0; (i < 4); i++) {
			s += xWeight[i] * inImg[p + xIndex[i]];
		}
		t += dyWeight[j] * s;
	}
	return(t);
}

void turboRegTransform::inverseMarquardtLevenbergOptimization (
	int workload
) {
	Matrix_d attempt (transformation / 2, Vector_d(2));
	Matrix_d hessian (transformation, Vector_d(transformation));
	Matrix_d pseudoHessian (transformation, Vector_d(transformation));
	Vector_d gradient (transformation);
	Matrix_d* matrix = getTransformationMatrix(*sourcePoint, *targetPoint);
	Vector_d update (transformation);
	double bestMeanSquares = 0.0;
	double meanSquares = 0.0;
	double lambda = FIRST_LAMBDA;
	double displacement;
	int iteration = 0;

	if (turboRegProgressBarTr) turboRegProgressBarTr->ProgressName ("turboRegTransform::inverseMarquardtLevenbergOptimization");
	if (turboRegProgressBarTr) turboRegProgressBarTr->pm_d ("Mi", *matrix);

	switch (transformation) {
		case REG_TRANSLATION: {
			bestMeanSquares = getTranslationMeanSquares(*matrix, hessian, gradient);
			break;
		}
		case REG_SCALED_ROTATION: {
			bestMeanSquares = getScaledRotationMeanSquares(*sourcePoint, *matrix, hessian, gradient);
			break;
		}
		case REG_AFFINE: {
			bestMeanSquares = getAffineMeanSquares(*sourcePoint, *matrix, hessian, gradient);
			break;
		}
	}
	iteration++;
	do {
//		if (turboRegProgressBarTr) turboRegProgressBarTr->Info("Iteration", (double)iteration);
		for (int k = 0; (k < transformation); k++) {
			pseudoHessian[k][k] = (1.0 + lambda) * hessian[k][k];
		}
		invertGauss(pseudoHessian);
		matrixMultiply(pseudoHessian, gradient, update);
		displacement = 0.0;
		for (int k = 0; (k < (transformation / 2)); k++) {
			attempt[k][0] = (*sourcePoint)[k][0] - update[2 * k];
			attempt[k][1] = (*sourcePoint)[k][1] - update[2 * k + 1];
			displacement += sqrt(update[2 * k] * update[2 * k]
				+ update[2 * k + 1] * update[2 * k + 1]);
		}
		displacement /= 0.5 * (double)transformation;

		delete matrix;
		matrix = getTransformationMatrix(attempt, *targetPoint);
//		pm_d ("Ma", *matrix);

		switch (transformation) {
			case REG_TRANSLATION: {
				if (accelerated) {
					meanSquares = getTranslationMeanSquares(*matrix, gradient);
				}
				else {
					meanSquares = getTranslationMeanSquares(*matrix, hessian, gradient);
				}
				break;
			}
			case REG_SCALED_ROTATION: {
				if (accelerated) {
					meanSquares = getScaledRotationMeanSquares(attempt, *matrix, gradient);
				}
				else {
					meanSquares = getScaledRotationMeanSquares(attempt, *matrix, hessian, gradient);
				}
				break;
			}
			case REG_AFFINE: {
				if (accelerated) {
					meanSquares = getAffineMeanSquares(attempt, *matrix, gradient);
				}
				else {
					meanSquares = getAffineMeanSquares(attempt, *matrix, hessian, gradient);
				}
				break;
			}
		}
		iteration++;
		if (meanSquares < bestMeanSquares) {
			bestMeanSquares = meanSquares;
			for (int k = 0; (k < (transformation / 2)); k++) {
				(*sourcePoint)[k][0] = attempt[k][0];
				(*sourcePoint)[k][1] = attempt[k][1];
			}
			lambda /= LAMBDA_MAGSTEP;
		}
		else {
			lambda *= LAMBDA_MAGSTEP;
		}
		if (turboRegProgressBarTr) turboRegProgressBarTr->skipProgressBar(iterationCost);
		workload--;
	} while ((iteration < (maxIterations * iterationPower - 1))
		&& (pixelPrecision <= displacement));

	invertGauss(hessian);
	matrixMultiply(hessian, gradient, update);
	for (int k = 0; (k < (transformation / 2)); k++) {
		attempt[k][0] = (*sourcePoint)[k][0] - update[2 * k];
		attempt[k][1] = (*sourcePoint)[k][1] - update[2 * k + 1];
	}

	delete matrix;
	matrix = getTransformationMatrix(attempt, *targetPoint);
	if (turboRegProgressBarTr) turboRegProgressBarTr->pm_d ("Mf", *matrix);

	switch (transformation) {
		case REG_TRANSLATION: {
			meanSquares = getTranslationMeanSquares(*matrix);
			break;
		}
		case REG_SCALED_ROTATION: {
			meanSquares = getScaledRotationMeanSquares(attempt, *matrix);
			break;
		}
		case REG_AFFINE: {
			meanSquares = getAffineMeanSquares(attempt, *matrix);
			break;
		}
	}
	iteration++;
	if (meanSquares < bestMeanSquares) {
		for (int k = 0; (k < (transformation / 2)); k++) {
			(*sourcePoint)[k][0] = attempt[k][0];
			(*sourcePoint)[k][1] = attempt[k][1];
		}
	}

	delete matrix;

	if (turboRegProgressBarTr) turboRegProgressBarTr->skipProgressBar(workload * iterationCost);
}

void turboRegTransform::inverseMarquardtLevenbergRigidBodyOptimization (
	int workload
) {
	Matrix_d attempt (2, Vector_d(3));
	Matrix_d hessian (transformation, Vector_d(transformation));
	Matrix_d pseudoHessian (transformation, Vector_d(transformation));
	Vector_d gradient (transformation);
	Matrix_d* tmp;
	Matrix_d matrix = *(tmp = getTransformationMatrix(*targetPoint, *sourcePoint)); delete tmp;
	Vector_d update (transformation);
	double bestMeanSquares = 0.0;
	double meanSquares = 0.0;
	double lambda = FIRST_LAMBDA;
	double angle;
	double c;
	double s;
	double displacement;
	int iteration = 0;
	if (turboRegProgressBarTr) turboRegProgressBarTr->ProgressName ("inverseMarquardtLevenbergRigidBodyOptimization");
	for (int k = 0; (k < transformation); k++) {
		(*sourcePoint)[k][0] = matrix[0][0] + (*targetPoint)[k][0] * matrix[0][1]
			+ (*targetPoint)[k][1] * matrix[0][2];
		(*sourcePoint)[k][1] = matrix[1][0] + (*targetPoint)[k][0] * matrix[1][1]
			+ (*targetPoint)[k][1] * matrix[1][2];
	}
	matrix = *(tmp = getTransformationMatrix (*sourcePoint, *targetPoint)); delete tmp;
	bestMeanSquares = getRigidBodyMeanSquares(matrix, hessian, gradient);
	iteration++;
	do {
		for (int k = 0; (k < transformation); k++) {
			pseudoHessian[k][k] = (1.0 + lambda) * hessian[k][k];
		}
		invertGauss(pseudoHessian);
		matrixMultiply(pseudoHessian, gradient, update);
		angle = atan2(matrix[0][2], matrix[0][1]) - update[0];
		attempt[0][1] = cos(angle);
		attempt[0][2] = sin(angle);
		attempt[1][1] = -attempt[0][2];
		attempt[1][2] = attempt[0][1];
		c = cos(update[0]);
		s = sin(update[0]);
		attempt[0][0] = (matrix[0][0] + update[1]) * c - (matrix[1][0] + update[2]) * s;
		attempt[1][0] = (matrix[0][0] + update[1]) * s + (matrix[1][0] + update[2]) * c;
		displacement = sqrt(update[1] * update[1] + update[2] * update[2])
			+ 0.25 * sqrt((double)(inNx * inNx) + (double)(inNy * inNy))
			* fabs(update[0]);
		if (accelerated) {
			meanSquares = getRigidBodyMeanSquares(attempt, gradient);
		}
		else {
			meanSquares = getRigidBodyMeanSquares(attempt, hessian, gradient);
		}
		iteration++;
		if (meanSquares < bestMeanSquares) {
			bestMeanSquares = meanSquares;
			for (int i = 0; (i < 2); i++) {
				for (int j = 0; (j < 3); j++) {
					matrix[i][j] = attempt[i][j];
				}
			}
			lambda /= LAMBDA_MAGSTEP;
		}
		else {
			lambda *= LAMBDA_MAGSTEP;
		}
		if (turboRegProgressBarTr) turboRegProgressBarTr->skipProgressBar(iterationCost);
		workload--;
	} while ((iteration < (maxIterations * iterationPower - 1))
		&& (pixelPrecision <= displacement));
	invertGauss(hessian);
	matrixMultiply(hessian, gradient, update);
	angle = atan2(matrix[0][2], matrix[0][1]) - update[0];
	attempt[0][1] = cos(angle);
	attempt[0][2] = sin(angle);
	attempt[1][1] = -attempt[0][2];
	attempt[1][2] = attempt[0][1];
	c = cos(update[0]);
	s = sin(update[0]);
	attempt[0][0] = (matrix[0][0] + update[1]) * c - (matrix[1][0] + update[2]) * s;
	attempt[1][0] = (matrix[0][0] + update[1]) * s + (matrix[1][0] + update[2]) * c;
	meanSquares = getRigidBodyMeanSquares(attempt);
	iteration++;
	if (meanSquares < bestMeanSquares) {
		for (int i = 0; (i < 2); i++) {
			for (int j = 0; (j < 3); j++) {
				matrix[i][j] = attempt[i][j];
			}
		}
	}
	for (int k = 0; (k < transformation); k++) {
		(*sourcePoint)[k][0] = ((*targetPoint)[k][0] - matrix[0][0]) * matrix[0][1]
			+ ((*targetPoint)[k][1] - matrix[1][0]) * matrix[1][1];
		(*sourcePoint)[k][1] = ((*targetPoint)[k][0] - matrix[0][0]) * matrix[0][2]
			+ ((*targetPoint)[k][1] - matrix[1][0]) * matrix[1][2];
	}
	if (turboRegProgressBarTr) turboRegProgressBarTr->skipProgressBar(workload * iterationCost);
}

void turboRegTransform::invertGauss (
	Matrix_d& matrix
) {
	int n = matrix.size ();
	Matrix_d inverse(n, Vector_d(n));
	for (int i = 0; (i < n); i++) {
		double max = matrix[i][0];
		double absMax = fabs(max);
		for (int j = 0; (j < n); j++) {
			inverse[i][j] = 0.0;
			if (absMax < fabs(matrix[i][j])) {
				max = matrix[i][j];
				absMax = fabs(max);
			}
		}
		inverse[i][i] = 1.0 / max;
		for (int j = 0; (j < n); j++) {
			matrix[i][j] /= max;
		}
	}
	for (int j = 0; (j < n); j++) {
		double max = matrix[j][j];
		double absMax = fabs(max);
		int k = j;
		for (int i = j + 1; (i < n); i++) {
			if (absMax < fabs(matrix[i][j])) {
				max = matrix[i][j];
				absMax = fabs(max);
				k = i;
			}
		}
		if (k != j) {
			Vector_d partialLine (n - j);
			Vector_d fullLine (n);
			vector_copy(matrix[j], j, partialLine, 0, n - j);
			vector_copy(matrix[k], j, matrix[j], j, n - j);
			vector_copy(partialLine, 0, matrix[k], j, n - j);
			vector_copy(inverse[j], 0, fullLine, 0, n);
			vector_copy(inverse[k], 0, inverse[j], 0, n);
			vector_copy(fullLine, 0, inverse[k], 0, n);
		}
		for (k = 0; (k <= j); k++) {
			inverse[j][k] /= max;
		}
		for (k = j + 1; (k < n); k++) {
			matrix[j][k] /= max;
			inverse[j][k] /= max;
		}
		for (int i = j + 1; (i < n); i++) {
			for (k = 0; (k <= j); k++) {
				inverse[i][k] -= matrix[i][j] * inverse[j][k];
			}
			for (k = j + 1; (k < n); k++) {
				matrix[i][k] -= matrix[i][j] * matrix[j][k];
				inverse[i][k] -= matrix[i][j] * inverse[j][k];
			}
		}
	}
	for (int j = n - 1; (1 <= j); j--) {
		for (int i = j - 1; (0 <= i); i--) {
			for (int k = 0; (k <= j); k++) {
				inverse[i][k] -= matrix[i][j] * inverse[j][k];
			}
			for (int k = j + 1; (k < n); k++) {
				matrix[i][k] -= matrix[i][j] * matrix[j][k];
				inverse[i][k] -= matrix[i][j] * inverse[j][k];
			}
		}
	}
	for (int i = 0; (i < n); i++) {
		vector_copy(inverse[i], 0, matrix[i], 0, n);
	}
}

void turboRegTransform::MarquardtLevenbergOptimization (
	int workload
) {
	Matrix_d attempt (transformation / 2, Vector_d(2));
	Matrix_d hessian (transformation, Vector_d(transformation));
	Matrix_d pseudoHessian (transformation, Vector_d(transformation));
	Vector_d gradient (transformation);
	Matrix_d* tmp;
	Matrix_d matrix; matrix = *(tmp=getTransformationMatrix (*targetPoint, *sourcePoint)); 
	delete tmp;
	Vector_d update (transformation);
	double bestMeanSquares = 0.0;
	double meanSquares = 0.0;
	double lambda = FIRST_LAMBDA;
	double displacement;
	int iteration = 0;
	if (turboRegProgressBarTr) turboRegProgressBarTr->ProgressName ("turboRegTransform::MarquardtLevenbergOptimization");
	bestMeanSquares = getBilinearMeanSquares(matrix, hessian, gradient);
	iteration++;
	do {
		for (int k = 0; (k < transformation); k++) {
			pseudoHessian[k][k] = (1.0 + lambda) * hessian[k][k];
		}
		invertGauss(pseudoHessian);
		matrixMultiply(pseudoHessian, gradient, update);
		displacement = 0.0;
		for (int k = 0; (k < (transformation / 2)); k++) {
			attempt[k][0] = (*sourcePoint)[k][0] - update[2 * k];
			attempt[k][1] = (*sourcePoint)[k][1] - update[2 * k + 1];
			displacement += sqrt(update[2 * k] * update[2 * k]
				+ update[2 * k + 1] * update[2 * k + 1]);
		}
		displacement /= 0.5 * (double)transformation;
		matrix = *(tmp = getTransformationMatrix(*targetPoint, attempt)); delete tmp;
		meanSquares = getBilinearMeanSquares(matrix, hessian, gradient);
		iteration++;
		if (meanSquares < bestMeanSquares) {
			bestMeanSquares = meanSquares;
			for (int k = 0; (k < (transformation / 2)); k++) {
				(*sourcePoint)[k][0] = attempt[k][0];
				(*sourcePoint)[k][1] = attempt[k][1];
			}
			lambda /= LAMBDA_MAGSTEP;
		}
		else {
			lambda *= LAMBDA_MAGSTEP;
		}
		if (turboRegProgressBarTr)turboRegProgressBarTr->skipProgressBar(iterationCost);
		workload--;
	} while ((iteration < (maxIterations * iterationPower - 1))
		&& (pixelPrecision <= displacement));
	invertGauss(hessian);
	matrixMultiply(hessian, gradient, update);
	for (int k = 0; (k < (transformation / 2)); k++) {
		attempt[k][0] = (*sourcePoint)[k][0] - update[2 * k];
		attempt[k][1] = (*sourcePoint)[k][1] - update[2 * k + 1];
	}
	matrix = *(tmp = getTransformationMatrix (*targetPoint, attempt)); delete tmp;
	meanSquares = getBilinearMeanSquares(matrix);
	iteration++;
	if (meanSquares < bestMeanSquares) {
		for (int k = 0; (k < (transformation / 2)); k++) {
			(*sourcePoint)[k][0] = attempt[k][0];
			(*sourcePoint)[k][1] = attempt[k][1];
		}
	}
	if (turboRegProgressBarTr) turboRegProgressBarTr->skipProgressBar(workload * iterationCost);
}

void turboRegTransform::scaleBottomDownLandmarks (
) {
	for (int depth = 1; (depth < pyramidDepth); depth++) {
		if (transformation == REG_RIGID_BODY) {
			for (int n = 0; (n < transformation); n++) {
				(*sourcePoint)[n][0] *= 0.5;
				(*sourcePoint)[n][1] *= 0.5;
				(*targetPoint)[n][0] *= 0.5;
				(*targetPoint)[n][1] *= 0.5;
			}
		}
		else {
			for (int n = 0; (n < (transformation / 2)); n++) {
				(*sourcePoint)[n][0] *= 0.5;
				(*sourcePoint)[n][1] *= 0.5;
				(*targetPoint)[n][0] *= 0.5;
				(*targetPoint)[n][1] *= 0.5;
			}
		}
	}
}

void turboRegTransform::scaleUpLandmarks (
) {
	if (transformation == REG_RIGID_BODY) {
		for (int n = 0; (n < transformation); n++) {
			(*sourcePoint)[n][0] *= 2.0;
			(*sourcePoint)[n][1] *= 2.0;
			(*targetPoint)[n][0] *= 2.0;
			(*targetPoint)[n][1] *= 2.0;
		}
	}
	else {
		for (int n = 0; (n < (transformation / 2)); n++) {
			(*sourcePoint)[n][0] *= 2.0;
			(*sourcePoint)[n][1] *= 2.0;
			(*targetPoint)[n][0] *= 2.0;
			(*targetPoint)[n][1] *= 2.0;
		}
	}
}

void turboRegTransform::translationTransform (
	Matrix_d& matrix
) {
	double dx = matrix[0][0];
	double dy = matrix[1][0];
	double dx0 = dx;
	int xMsk;
	int yMsk;
	x = dx - floor(dx);
	y = dy - floor(dy);
	if (!accelerated) {
		xWeights();
		yWeights();
	}
	int k = 0;
	ProgressBar turboRegProgressBar ("turboRegTransform::translationTransform_M");
	turboRegProgressBar.addWorkload(outNy);
	for (int v = 0; (v < outNy); v++) {
		y = dy++;
		yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
		if ((0 <= yMsk) && (yMsk < inNy)) {
			yMsk *= inNx;
			if (!accelerated) {
				yIndexes();
			}
			dx = dx0;
			for (int u = 0; (u < outNx); u++) {
				x = dx++;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx)) {
					xMsk += yMsk;
					if (accelerated) {
						outImg[k++] = inImg[xMsk];
					}
					else {
						xIndexes();
						outImg[k++] = interpolate();
					}
				}
				else {
					outImg[k++] = 0.0;
				}
			}
		}
		else {
			for (int u = 0; (u < outNx); u++) {
				outImg[k++] = 0.0;
			}
		}
		turboRegProgressBar.stepProgressBar();
	}
	turboRegProgressBar.workloadDone(outNy);
}

void turboRegTransform::translationTransform (
	Matrix_d& matrix,
	Vector_b& outMsk
) {
	double dx = matrix[0][0];
	double dy = matrix[1][0];
	double dx0 = dx;
	int xMsk;
	int yMsk;
	x = dx - floor(dx);
	y = dy - floor(dy);
	if (!accelerated) {
		xWeights();
		yWeights();
	}
	int k = 0;
	ProgressBar turboRegProgressBar ("turboRegTransform::translationTransform_M_V");
	turboRegProgressBar.addWorkload(outNy);
	for (int v = 0; (v < outNy); v++) {
		y = dy++;
		yMsk = (0.0 <= y) ? ((int)(y + 0.5)) : ((int)(y - 0.5));
		if ((0 <= yMsk) && (yMsk < inNy)) {
			yMsk *= inNx;
			if (!accelerated) {
				yIndexes();
			}
			dx = dx0;
			for (int u = 0; (u < outNx); u++, k++) {
				x = dx++;
				xMsk = (0.0 <= x) ? ((int)(x + 0.5)) : ((int)(x - 0.5));
				if ((0 <= xMsk) && (xMsk < inNx)) {
					xMsk += yMsk;
					if (accelerated) {
						outImg[k] = inImg[xMsk];
					}
					else {
						xIndexes();
						outImg[k] = interpolate();
					}
					outMsk[k] = (inMsk[xMsk]) ? 255 : 0;
				}
				else {
					outImg[k] = 0.0;
					outMsk[k] = 0;
				}
			}
		}
		else {
			for (int u = 0; (u < outNx); u++, k++) {
				outImg[k] = 0.0;
				outMsk[k] = 0;
			}
		}
		turboRegProgressBar.stepProgressBar();
	}
	turboRegProgressBar.workloadDone(outNy);
}
