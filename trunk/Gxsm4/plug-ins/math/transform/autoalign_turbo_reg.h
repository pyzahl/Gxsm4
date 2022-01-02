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

#ifndef __AUTOALIGN_TURBO_REG_H
#define __AUTOALIGN_TURBO_REG_H

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

#include <time.h>

#include <gtk/gtk.h>
#include "config.h"
#include "core-source/plugin.h"
#include "autoalign_vector_types.h"

extern GxsmPlugin autoalign_pi;

/*********************************************************************
 A single points determines the translation component of a rigid-body
 transformation. As the rotation is given by a scalar number, it is
 not natural to represent this number by coordinates of a point. The
 rigid-body transformation is determined by 3 parameters.
 ********************************************************************/
//public static final int RIGID_BODY = 3;

/*********************************************************************
 A pair of points determines the combination of a translation, of
 a rotation, and of an isotropic scaling. Angles are conserved. A
 scaled rotation is determined by 4 parameters.
 ********************************************************************/
//public static final int SCALED_ROTATION = 4;

/*********************************************************************
 A translation is described by a single point. It keeps area, angle,
 and orientation. A translation is determined by 2 parameters.
 ********************************************************************/
//public static final int TRANSLATION = 2;

/*********************************************************************
 Three points generate an affine transformation, which is any
 combination of translation, rotation, isotropic scaling, anisotropic
 scaling, shearing, and skewing. An affine transformation maps
 parallel lines onto parallel lines and is determined by 6 parameters.
 ********************************************************************/
//public static final int AFFINE = 6;

/*********************************************************************
 Generic geometric transformation.
 ********************************************************************/
//public static final int GENERIC_TRANSFORMATION = -1;

typedef enum { REG_GENERIC_TRANSFORMATION = -1, 
	       REG_TRANSLATION = 2, 
	       REG_RIGID_BODY  = 3,
	       REG_SCALED_ROTATION = 4, 
	       REG_AFFINE = 6, 
	       REG_BILINEAR = 7, 
	       REG_INVALID = 0
} REGISTER_MODE;


class ProgressBar{
#define PB_VERBOSE 0
public:
	ProgressBar(const gchar *l=NULL) { workload_set=workload=0; label = g_strdup(l ? l:"W"); if (PB_VERBOSE>1) { std::cout << "Progress**" << l << std::endl; } ti=clock (); };
	virtual ~ProgressBar() { if (PB_VERBOSE>1) { std::cout << "Progress**" << label << " done. load ["; time_elapsed (); std::cout << "]" << std::endl; } if (label) g_free (label); };

	void ProgressName(const gchar *l) { 
		if (PB_VERBOSE>2) { std::cout << "ProgressRename**" << label << " --> " << l << std::endl; }
		if (label) g_free (label);
		label = g_strdup(l ? l:"W"); 
	};
	void addWorkload (int w) { workload += w; if (PB_VERBOSE>10) { std::cout << label << "+" << workload << std::endl; } workload_set = workload; 
		if (PB_VERBOSE>2) gapp->progress_info_set_bar_fraction ((gdouble)workload/(gdouble)workload_set, 2);
	};
	void stepProgressBar (gboolean print=false) { --workload; if (PB_VERBOSE>3) { if(print) std::cout << label << "--" << workload << std::endl; }
		if (PB_VERBOSE>4) gapp->progress_info_set_bar_fraction ((gdouble)workload/(gdouble)workload_set, 2);
	};
	void skipProgressBar (int w) { workload -= w;if (PB_VERBOSE>4) {  std::cout << label << "-" << workload << std::endl; }
		if (PB_VERBOSE>3) gapp->progress_info_set_bar_fraction ((gdouble)workload/(gdouble)workload_set, 2);
	};
	void workloadDone(int w) { workload = 0; if (PB_VERBOSE>5) { std::cout << label << "**done:" << w << std::endl; }
		if (PB_VERBOSE>2) gapp->progress_info_set_bar_fraction (1., 2);
	};

	void time_elapsed () { tf = clock (); if(PB_VERBOSE>1) { std::cout << ((double)(tf-ti)/(double)(CLOCKS_PER_SEC)); } };

	void Info(const gchar *i) { if (PB_VERBOSE>2) {  std::cout << label << " **INFO: " << i << std::endl; }};
	void Info(const gchar *i, double d) { if (PB_VERBOSE>2) { std::cout << label << " **INFO: " << i << "=" << d << std::endl; }};
	void InfoP0(const gchar *i, double d) { if (PB_VERBOSE>0) { std::cout << label << " **INFO**: " << i << "=" << d << std::endl; }};
	void pv_d (const gchar* vn, Vector_d& v){
		if (PB_VERBOSE<3) return;
		cout << (vn?vn:"   ") << " [ ";
		for (int k = 0; k < v.size(); k++) {
			cout << v[k] << ' ';
		}
		cout << "]\n";
	};

	void pm_d (const gchar* mn, Matrix_d& m){
		if (PB_VERBOSE<3) return;
		cout << mn << " [\n";
		for (int k = 0; k < m.size(); k++)
			pv_d (NULL, m[k]);
		cout << "]\n";
	};

	void pv_i (const gchar* vn, Vector_i& v){
		if (PB_VERBOSE<3) return;
		cout << (vn?vn:"   ") << " [ ";
		for (int k = 0; k < v.size(); k++) {
			cout << v[k] << ' ';
		}
		cout << "]\n";
	};

	void pm_i (const gchar* mn, Matrix_i& m){
		if (PB_VERBOSE<3) return;
		cout << mn << " [\n";
		for (int k = 0; k < m.size(); k++)
			pv_i (NULL, m[k]);
		cout << "]\n";
	};

	void pv_b (const gchar* vn, Vector_b& v){
		if (PB_VERBOSE<3) return;
		cout << (vn?vn:"   ") << " [ ";
		for (int k = 0; k < v.size(); k++) {
			cout << (v[k]?"1":"0");
		}
		cout << "]\n";
	};

	void pm_b (const gchar* mn, Matrix_b& m){
		if (PB_VERBOSE<3) return;
		cout << mn << " [\n";
		for (int k = 0; k < m.size(); k++)
			pv_i (NULL, m[k]);
		cout << "]\n";
	};

private:
	int workload;
	int workload_set;
	gchar* label;
	clock_t ti, tf;
};

class turboRegAlign{
public:
	turboRegAlign (REGISTER_MODE t){ transformation=t; };
	virtual ~turboRegAlign () {};

	void run (
		Mem2d* source, int sv, Matrix_i& sourceCrop, 
		Mem2d* target, int tv, Matrix_i& targetCrop, 
		Matrix_d& sourcePoints, Matrix_d& targetPoints, 
		Mem2d* destination=NULL, int dv=-1
		);
	void doTransform (
		Mem2d* source, int sv,
		Matrix_d& sourcePoints, Matrix_d& targetPoints, 
		Mem2d* destination, int dv);

	int getPyramidDepth (int sw, int sh, int tw, int th); 
	int transformation_num_points (){
		switch (transformation) {
		case REG_TRANSLATION: return 1;
		case REG_RIGID_BODY: return 3;
		case REG_SCALED_ROTATION: return 2;
		case REG_AFFINE: return 3;
		default: return 0;
		}
	};
private:
	REGISTER_MODE transformation;
};

/*====================================================================
|	turboRegImage
\===================================================================*/

/*********************************************************************
 This class is responsible for the image preprocessing that takes
 place concurrently with user-interface events. It contains methods
 to compute B-spline coefficients and their pyramids, image pyramids,
 gradients, and gradient pyramids.
 ********************************************************************/

class turboRegImage{
public:

/*********************************************************************
 Converts the pixel array of the incoming <code>ImagePlus</code>
 object into a local <code>float</code> array.
 @param imp <code>ImagePlus</code> object to preprocess.
 @param transformation Transformation code.
 @param isTarget Tags the current object as a target or source image.
 ********************************************************************/
	turboRegImage (Mem2d* imp, int v, Matrix_i& region, REGISTER_MODE _transformation, gboolean _isTarget);
	turboRegImage (Mem2d* imp, int v);
	virtual ~turboRegImage (){};

/*********************************************************************
 Return the B-spline coefficients of the full-size image.
 ********************************************************************/
	void getCoefficient (Vector_d& coef) { coef = coefficient; };

/*********************************************************************
 Return the full-size image height.
 ********************************************************************/
	int getHeight () { return height; };

/*********************************************************************
 Return the full-size image array.
 ********************************************************************/
	void getImage (Vector_d &img) { img = image; };

/*********************************************************************
 Return the image pyramid as a <code>Stack</code> object. The organization
 of the stack depends on whether the <code>turboRegImage</code>
 object corresponds to the target or the source image, and on the
 transformation (ML* = {<code>TRANSLATION</code>,<code>RIGID_BODY</code>,
 <code>SCALED_ROTATION</code>, <code>AFFINE</code>} vs.
 ML = {<code>BILINEAR<code>}). A single pyramid level consists of
 <p>
 <table border="1">
 <tr><th><code>isTarget</code></th>
 <th>ML*</th>
 <th>ML</th></tr>
 <tr><td>true</td>
 <td>width<br>height<br>B-spline coefficients</td>
 <td>width<br>height<br>samples</td></tr>
 <tr><td>false</td>
 <td>width<br>height<br>samples<br>horizontal gradients<br>vertical gradients</td>
 <td>width<br>height<br>B-spline coefficients</td></tr>
 </table>
 ********************************************************************/
//	stack_pyr* getPyramid () { return new stack_pyr (pyramid); };

	gboolean pyramid_empty () { return pyramid.empty (); };
	gboolean pop_pyramid (int &width, int &height, Vector_d& img){
		int* ip;
		width  = *(ip=(int*) (pyramid.top())); pyramid.pop(); delete ip;
		height = *(ip=(int*) (pyramid.top())); pyramid.pop(); delete ip;
		Vector_d* tmp;
		img    = *(tmp = (Vector_d*) (pyramid.top())); pyramid.pop();
		delete tmp;
		return pyramid_empty ();
	};
	gboolean pop_pyramid (int &width, int &height, Vector_d& img, Vector_d& x_grad, Vector_d& y_grad){
		int* ip;
		width  = *(ip=(int*) (pyramid.top())); pyramid.pop(); delete ip;
		height = *(ip=(int*) (pyramid.top())); pyramid.pop(); delete ip;
		Vector_d* tmp;
		img    = *(tmp = (Vector_d*) (pyramid.top())); pyramid.pop();
		delete tmp;
		x_grad = *(tmp = (Vector_d*) (pyramid.top())); pyramid.pop();
		delete tmp;
		y_grad = *(tmp = (Vector_d*) (pyramid.top())); pyramid.pop();
		delete tmp;
		return pyramid_empty ();
	};

/*********************************************************************
 Return the depth of the image pyramid. A depth <code>1</code> means
 that one coarse resolution level is present in the stack. The
 full-size level is not placed on the stack.
 ********************************************************************/
	int getPyramidDepth () { return(pyramidDepth); };

/*********************************************************************
 Return the thread associated with this <code>turboRegImage</code>
 object.
 ********************************************************************/
//	Thread getThread () { return(t); };

/*********************************************************************
 Return the full-size image width.
 ********************************************************************/
	int getWidth () { return(width); };

/*********************************************************************
 Return the full-size horizontal gradient of the image, if available.
 @see turboRegImage#getPyramid()
 ********************************************************************/
	void getXGradient (Vector_d& xgrad) { xgrad = xGradient; };

/*********************************************************************
 Return the full-size vertical gradient of the image, if available.
 @see turboRegImage#getImage()
 ********************************************************************/
	void getYGradient (Vector_d& ygrad) { ygrad = yGradient; };

/*********************************************************************
 Start the image precomputations. The computation of the B-spline
 coefficients of the full-size image is not interruptible; all other
 methods are.
 ********************************************************************/
	void run () {
		ProgressBar turboRegProgressBar ("turboRegImage::run");
		Vector_d *tmp;
		coefficient = *(tmp = getBasicFromCardinal2D());
		delete tmp;
		switch (transformation) {
		case REG_GENERIC_TRANSFORMATION: {
			break;
		}
		case REG_TRANSLATION:
		case REG_RIGID_BODY:
		case REG_SCALED_ROTATION:
		case REG_AFFINE: {
			if (isTarget) {
				buildCoefficientPyramid();
			}
			else {
				imageToXYGradient2D();
				buildImageAndGradientPyramid();
			}
			break;
		}
		case REG_BILINEAR: {
			if (isTarget) {
				buildImagePyramid();
			}
			else {
				buildCoefficientPyramid();
			}
			break;
		}
		}
	};

/*********************************************************************
 Sets the depth up to which the pyramids should be computed.
 @see turboRegImage#getImage()
 ********************************************************************/
	void setPyramidDepth (int _pyramidDepth) { pyramidDepth = _pyramidDepth; };

/*********************************************************************
 Set or modify the transformation.
 ********************************************************************/
	void setTransformation (REGISTER_MODE _transformation) { transformation = _transformation; };


private:
	void antiSymmetricFirMirrorOffBounds1D (Vector_d& h, Vector_d& c, Vector_d& s);
	void basicToCardinal2D (
		Vector_d& basic,
		Vector_d& cardinal,
		int width,
		int height,
		int degree
		);
	void buildCoefficientPyramid ();
	void buildImageAndGradientPyramid ();
	void buildImagePyramid ();
	void cardinalToDual2D (
		Vector_d& cardinal,
		Vector_d& dual,
		int width,
		int height,
		int degree
		);
	void coefficientToGradient1D (Vector_d& c);
	void coefficientToSamples1D (Vector_d& c);
	void coefficientToXYGradient2D (
		Vector_d& basic,
		Vector_d& xGradient,
		Vector_d& yGradient,
		int width,
		int height
		);
	void dualToCardinal2D (
		Vector_d& dual,
		Vector_d& cardinal,
		int width,
		int height,
		int degree
		);
	void extractColumn (
		Vector_d& array,
		int width,
		int x,
		Vector_d& column
		);
	void extractRow (
		Vector_d& array,
		int y,
		Vector_d& row
		);

	// NOTE: all Vector_d* returns are new instances given back to the caller to take care of.
	Vector_d* getBasicFromCardinal2D ();
	Vector_d* getBasicFromCardinal2D (
		Vector_d& cardinal,
		int width,
		int height,
		int degree
		);
	Vector_d* getHalfDual2D (
		Vector_d& fullDual,
		int fullWidth,
		int fullHeight
		);
	double getInitialAntiCausalCoefficientMirrorOffBounds (
		Vector_d& c,
		double z,
		double tolerance
		);
	double getInitialCausalCoefficientMirrorOffBounds (
		Vector_d& c,
		double z,
		double tolerance
		);
	void imageToXYGradient2D ();
	void putColumn (
		Vector_d& array,
		int width,
		int x,
		Vector_d& column
		);

	void putRow (
		Vector_d& array,
		int y,
		Vector_d& row
		);
	void reduceDual1D (
		Vector_d& c,
		Vector_d& s
		);
	void samplesToInterpolationCoefficient1D (
		Vector_d& c,
		int degree,
		double tolerance
		);
	void symmetricFirMirrorOffBounds1D (
		Vector_d& h,
		Vector_d& c,
		Vector_d& s
		);

protected:
	stack_pyr pyramid;
	Vector_d image;
	Vector_d coefficient;
	Vector_d xGradient;
	Vector_d yGradient;
	int width;
	int height;
	int pyramidDepth;
	REGISTER_MODE transformation;
	gboolean isTarget;
};  /* end class turboRegImage */





/*====================================================================
|	turboRegMask
\===================================================================*/

/*********************************************************************
 This class is responsible for the mask preprocessing that takes
 place concurrently with user-interface events. It contains methods
 to compute the mask pyramids.
 ********************************************************************/
class turboRegMask{

public:
/*********************************************************************
 Converts the pixel array of the incoming <code>ImagePlus</code>
 object into a local <code>boolean</code> array.
 @param imp <code>ImagePlus</code> object to preprocess.
 ********************************************************************/
	turboRegMask (Mem2d* imp);
	turboRegMask (int w, int h);

	virtual ~turboRegMask () {};

/*********************************************************************
 Set to <code>true</code> every pixel of the full-size mask.
 ********************************************************************/
	void clearMask (gboolean m=true);
	void clearMaskWindow (Matrix_i& win);


/*********************************************************************
 Return the full-size mask array.
 ********************************************************************/
	void getMask (Vector_b& msk) { msk = mask; };

/*********************************************************************
 Return the pyramid as a <code>Stack</code> object. A single pyramid
 level consists of
 <p>
 <table border="1">
 <tr><th><code>isTarget</code></th>
 <th>ML*</th>
 <th>ML</th></tr>
 <tr><td>true</td>
 <td>mask samples</td>
 <td>mask samples</td></tr>
 <tr><td>false</td>
 <td>mask samples</td>
 <td>mask samples</td></tr>
 </table>
 @see turboRegImage#getPyramid()
 ********************************************************************/
//	stack_pyr* getPyramid () { return new stack_pyr (pyramid);	};

	gboolean pyramid_empty () { return pyramid.empty (); };
	gboolean pop_pyramid (Vector_b& msk){
		Vector_b* tmp;
		msk = *(tmp = (Vector_b*) (pyramid.top())); pyramid.pop();
		delete tmp;
		return true;
	};


/*********************************************************************
 Return the thread associated with this <code>turboRegMask</code>
 object.
 ********************************************************************/
//	Thread getThread () { return(t); };

/*********************************************************************
 Start the mask precomputations, which are interruptible.
 ********************************************************************/
	void run () {
		buildPyramid();
	};

/*********************************************************************
 Set the depth up to which the pyramids should be computed.
 @see turboRegMask#getPyramid()
 ********************************************************************/
	void setPyramidDepth (int _pyramidDepth) {
		pyramidDepth = _pyramidDepth;
	};
	

private:
	void buildPyramid ();
	Vector_b* getHalfMask2D (
		Vector_b& fullMask,
		int fullWidth,
		int fullHeight
		);

protected:
	stack_pyr pyramid;
	Vector_b mask;
	int width;
	int height;
	int pyramidDepth;

}; /* end class turboRegMask */


/*====================================================================
|	turboRegTransform
\===================================================================*/

/*********************************************************************
 This class implements the algorithmic methods of the plugin. It
 refines the landmarks and computes the images.
 ********************************************************************/
class turboRegTransform{

public:
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
	turboRegTransform (
		turboRegImage* sourceImg,
		turboRegMask* sourceMsk,
		Matrix_d* sourcePh,
		turboRegImage* targetImg,
		turboRegMask* targetMsk,
		Matrix_d* targetPh,
		REGISTER_MODE transformation,
		gboolean accelerated,
		gboolean interactive
		);
	~turboRegTransform () {
	};
	

/*********************************************************************
 Append the current landmarks into a text file. Rigid format.
 @param pathAndFilename Path and name of the file where batch results
 are being written.
 @see turboRegDialog#loadLandmarks()
 ********************************************************************/
	void appendTransformation (gchar* pathAndFilename);
	
/*********************************************************************
 Compute the image.
 ********************************************************************/
	void doBatchFinalTransform (Vector_d& pixels);

/*********************************************************************
 Compute the image.
 ********************************************************************/
	void doFinalTransform (Mem2d* destination, int dv);

/*********************************************************************
 Compute the image.
 ********************************************************************/
	void doFinalTransform (
		REGISTER_MODE transformation,
		turboRegImage* sourceImg,
		Mem2d* destination, int dv,
		gboolean accelerated=false
		);
#if 0
	Vector_d* doFinalTransform (
		turboRegImage* sourceImg,
		Matrix_d* sourcePh,
		turboRegImage* targetImg,
		Matrix_d* targetPh,
		REGISTER_MODE transformation,
		gboolean accelerated
		);
#endif
/*********************************************************************
 Refine the landmarks.
 ********************************************************************/
	void doRegistration ();
	
/*********************************************************************
 Save the current landmarks into a text file and return the path
 and name of the file. Rigid format.
 @see turboRegDialog#loadLandmarks()
 ********************************************************************/
	const gchar* saveTransformation (const gchar* filename);

private:
	void affineTransform (Matrix_d& matrix);
	void affineTransform (Matrix_d& matrix, Vector_b& outMsk);
	void bilinearTransform (Matrix_d& matrix);
	void bilinearTransform (
		Matrix_d& matrix,
		Vector_b& outMsk
		);
	void computeBilinearGradientConstants ();
	double getAffineMeanSquares (
		Matrix_d& sourcePoint,
		Matrix_d& matrix
		);
	double getAffineMeanSquares (
		Matrix_d& sourcePoint,
		Matrix_d& matrix,
		Vector_d& gradient
		);
	double getAffineMeanSquares (
		Matrix_d& sourcePoint,
		Matrix_d& matrix,
		Matrix_d& hessian,
		Vector_d& gradient
		);
	double getBilinearMeanSquares (
		Matrix_d& matrix
		);
	double getBilinearMeanSquares (
		Matrix_d& matrix,
		Matrix_d& hessian,
		Vector_d& gradient
		);
	double getRigidBodyMeanSquares (
		Matrix_d& matrix
		);
	double getRigidBodyMeanSquares (
		Matrix_d& matrix,
		Vector_d& gradient
		
		);
	double getRigidBodyMeanSquares (
		Matrix_d& matrix,
		Matrix_d& hessian,
		Vector_d& gradient
		);
	double getScaledRotationMeanSquares (
		Matrix_d& sourcePoint,
		Matrix_d& matrix
		);
	double getScaledRotationMeanSquares (
		Matrix_d& sourcePoint,
		Matrix_d& matrix,
		Vector_d& gradient
		);
	double getScaledRotationMeanSquares (
		Matrix_d& sourcePoint,
		Matrix_d& matrix,
		Matrix_d& hessian,
		Vector_d& gradient
		);
	Matrix_d* getTransformationMatrix (
		Matrix_d& fromCoord,
		Matrix_d& toCoord
		);
	double getTranslationMeanSquares (
		Matrix_d& matrix
		);
	double getTranslationMeanSquares (
		Matrix_d& matrix,
		Vector_d& gradient
		);
	double getTranslationMeanSquares (
		Matrix_d& matrix,
		Matrix_d& hessian,
		Vector_d& gradient
		);

	double interpolate ();
	double interpolateDx ();
	double interpolateDy ();
	void inverseMarquardtLevenbergOptimization (int workload);
	void inverseMarquardtLevenbergRigidBodyOptimization (int workload);
	void invertGauss (Matrix_d& matrix);
	void MarquardtLevenbergOptimization (int workload);

	void matrixMultiply (
		Matrix_d& matrix,
		Vector_d& vector,
		Vector_d& result
		){
		result.resize (matrix.size ());
		for (int i = 0; (i < matrix.size ()); i++) {
			result[i] = 0.0;
			for (int j = 0; (j < vector.size ()); j++) {
				result[i] += matrix[i][j] * vector[j];
			}
		}
	};

	void scaleBottomDownLandmarks ();
	void scaleUpLandmarks ();
	void translationTransform (
		Matrix_d& matrix
		);
	void translationTransform (
		Matrix_d& matrix,
		Vector_b& outMsk
		);
	void xDxWeights () {
		s = 1.0 - x;
		dxWeight[0] = 0.5 * x * x;
		xWeight[0] = x * dxWeight[0] / 3.0;
		dxWeight[3] = -0.5 * s * s;
		xWeight[3] = s * dxWeight[3] / -3.0;
		dxWeight[1] = 1.0 - 2.0 * dxWeight[0] + dxWeight[3];
		xWeight[1] = 2.0 / 3.0 + (1.0 + x) * dxWeight[3];
		dxWeight[2] = 1.5 * x * (x - 4.0/ 3.0);
		xWeight[2] = 2.0 / 3.0 - (2.0 - x) * dxWeight[0];
	};
	
	void xIndexes () {
		p = (0.0 <= x) ? ((int)x + 2) : ((int)x + 1);
		for (int k = 0; (k < 4); p--, k++) {
			q = (p < 0) ? (-1 - p) : (p);
			if (twiceInNx <= q) {
				q -= twiceInNx * (q / twiceInNx);
			}
			xIndex[k] = (inNx <= q) ? (twiceInNx - 1 - q) : (q);
		}
	};

	void xWeights () {
		s = 1.0 - x;
		xWeight[3] = s * s * s / 6.0;
		s = x * x;
		xWeight[2] = 2.0 / 3.0 - 0.5 * s * (2.0 - x);
		xWeight[0] = s * x / 6.0;
		xWeight[1] = 1.0 - xWeight[0] - xWeight[2] - xWeight[3];
	};

	void yDyWeights () {
		t = 1.0 - y;
		dyWeight[0] = 0.5 * y * y;
		yWeight[0] = y * dyWeight[0] / 3.0;
		dyWeight[3] = -0.5 * t * t;
		yWeight[3] = t * dyWeight[3] / -3.0;
		dyWeight[1] = 1.0 - 2.0 * dyWeight[0] + dyWeight[3];
		yWeight[1] = 2.0 / 3.0 + (1.0 + y) * dyWeight[3];
		dyWeight[2] = 1.5 * y * (y - 4.0/ 3.0);
		yWeight[2] = 2.0 / 3.0 - (2.0 - y) * dyWeight[0];
	};

	void yIndexes () {
		p = (0.0 <= y) ? ((int)y + 2) : ((int)y + 1);
		for (int k = 0; (k < 4); p--, k++) {
			q = (p < 0) ? (-1 - p) : (p);
			if (twiceInNy <= q) {
				q -= twiceInNy * (q / twiceInNy);
			}
			yIndex[k] = (inNy <= q) ? ((twiceInNy - 1 - q) * inNx) : (q * inNx);
		}
	};

	void yWeights () {
		t = 1.0 - y;
		yWeight[3] = t * t * t / 6.0;
		t = y * y;
		yWeight[2] = 2.0 / 3.0 - 0.5 * t * (2.0 - y);
		yWeight[0] = t * y / 6.0;
		yWeight[1] = 1.0 - yWeight[0] - yWeight[2] - yWeight[3];
	};


private:

/*********************************************************************
 Maximal number of registration iterations per level, when
 speed is requested at the expense of accuracy. This number must be
 corrected so that there are more iterations at the coarse levels
 of the pyramid than at the fine levels.
 @see turboRegTransform#ITERATION_PROGRESSION
********************************************************************/
	#define FEW_ITERATIONS 5

/*********************************************************************
 Initial value of the Marquardt-Levenberg fudge factor.
********************************************************************/
	#define FIRST_LAMBDA 1.0

/*********************************************************************
 Update parameter of the Marquardt-Levenberg fudge factor.
********************************************************************/
	#define LAMBDA_MAGSTEP 4.0

/*********************************************************************
 Maximal number of registration iterations per level, when
 accuracy is requested at the expense of speed. This number must be
 corrected so that there are more iterations at the coarse levels
 of the pyramid than at the fine levels.
 @see turboRegTransform#ITERATION_PROGRESSION
********************************************************************/
	#define MANY_ITERATIONS 10

/*********************************************************************
 Minimal update distance of the landmarks, in pixel units, when
 accuracy is requested at the expense of speed. This distance does
 not depend on the pyramid level.
********************************************************************/
	#define PIXEL_HIGH_PREC 0.001

/*********************************************************************
 Minimal update distance of the landmarks, in pixel units, when
 speed is requested at the expense of accuracy. This distance does
 not depend on the pyramid level.
********************************************************************/
	#define PIXEL_LOW_PREC 0.1

/*********************************************************************
 Multiplicative factor that determines how many more iterations
 are allowed for a pyramid level one unit coarser.
********************************************************************/
	#define ITERATION_PROGRESSION 2

	Vector_d dxWeight; // [4]
	Vector_d dyWeight; // [4]
	Vector_d xWeight; // [4]
	Vector_d yWeight; // [4]
	Vector_i xIndex; // [4]
	Vector_i yIndex; // [4]
	turboRegImage* sourceImg;
	turboRegImage* targetImg;
	turboRegMask* sourceMsk;
	turboRegMask* targetMsk;
	Matrix_d* sourcePoint;
	Matrix_d* targetPoint;
	Vector_d inImg;
	Vector_d outImg;
	Vector_d xGradient;
	Vector_d yGradient;
	Vector_b inMsk;
	gboolean inMsk_valid;
	Vector_b outMsk;
	gboolean outMsk_valid;
	double targetJacobian;
	double s;
	double t;
	double x;
	double y;
	double c0;
	double c0u;
	double c0v;
	double c0uv;
	double c1;
	double c1u;
	double c1v;
	double c1uv;
	double c2;
	double c2u;
	double c2v;
	double c2uv;
	double c3;
	double c3u;
	double c3v;
	double c3uv;
	double pixelPrecision;
	int maxIterations;
	int p;
	int q;
	int inNx;
	int inNy;
	int outNx;
	int outNy;
	int twiceInNx;
	int twiceInNy;
	REGISTER_MODE transformation;
	int pyramidDepth;
	int iterationPower;
	int iterationCost;
	gboolean accelerated;
	gboolean interactive;

	ProgressBar* turboRegProgressBarTr;
}; /* end class turboRegTransform */

#endif
