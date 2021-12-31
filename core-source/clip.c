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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "clip.h"

#define TRUE -1
#define FALSE 0

typedef enum {
	LEFT, RIGHT, BOTTOM, TOP
} edge;
typedef long outcode;


/* Local variables for cohen_sutherland_line_clip: */
struct LOC_cohen_sutherland_line_clip {
	double xmin, xmax, ymin, ymax;
} ;

void CompOutCode (double x, double y, outcode *code, struct LOC_cohen_sutherland_line_clip *LINK)
{
	/*Compute outcode for the point (x,y) */
	*code = 0;
	if (y > LINK->ymax)
		*code = 1L << ((long)TOP);
	else if (y < LINK->ymin)
		*code = 1L << ((long)BOTTOM);
	if (x > LINK->xmax)
		*code |= 1L << ((long)RIGHT);
	else if (x < LINK->xmin)
		*code |= 1L << ((long)LEFT);
}

ClipResult cohen_sutherland_line_clip_i (int *x0_, int *y0_, int *x1_, int *y1_, 
			       int xmin_, int xmax_, int ymin_, int ymax_)
{
	ClipResult ret;
	double x0,y0,x1,y1;
	x0 = *x0_;
	y0 = *y0_;
	x1 = *x1_;
	y1 = *y1_;
	ret = cohen_sutherland_line_clip_d (&x0, &y0, &x1, &y1, 
					  (double)xmin_, (double)xmax_,
					  (double)ymin_, (double)ymax_);
	*x0_ = (int)x0;
	*y0_ = (int)y0;
	*x1_ = (int)x1;
	*y1_ = (int)y1;
	return ret;
}

/**
 * cohen_sutherland_line_clip_d:
 * @x0:  x coordinate of first point.
 * @y0:  y coordinate of first point.
 * @x1:  x coordinate of second point.
 * @y1:  y coordinate of second point.
 * @xmin:  x coordinate of lower left edge of clipping rectangle.
 * @ymin:  y coordinate of lower left edge of clipping rectangle.
 * @xmax:  x coordinate of upper right edge of clipping rectangle.
 * @ymax:  y coordinate of lower left edge of clipping rectangle.
 *
 * cohen_sutherland_line_clip_d() will clip a line from first point (@x0, @y0)
 * to (@x1, @y1) so that it fits into the given min/max bounds of the rectange
 * (@xmin, @ymin ) - (@xmax, @ymax ). The line input coordinates will be 
 * modified if necessary for clipping.
 * 
 * Returns: #ClipResult.
 */

ClipResult cohen_sutherland_line_clip_d (double *x0, double *y0, double *x1, double *y1, 
				      double xmin_, double xmax_, double ymin_, double ymax_)
{
	/* Cohen-Sutherland clipping algorithm for line P0=(x1,y0) to P1=(x1,y1)
	   and clip rectangle with diagonal from (xmin,ymin) to (xmax,ymax).*/
	struct LOC_cohen_sutherland_line_clip V;
	int done = FALSE;
	ClipResult clip = NotClipped;
	outcode outcode0, outcode1, outcodeOut;
	/*Outcodes for P0,P1, and whichever point lies outside the clip rectangle*/
	double x=0., y=0.;
	
	V.xmin = xmin_;
	V.xmax = xmax_;
	V.ymin = ymin_;
	V.ymax = ymax_;
	CompOutCode(*x0, *y0, &outcode0, &V);
	CompOutCode(*x1, *y1, &outcode1, &V);
	do {
		if (outcode0 == 0 && outcode1 == 0) {   /*Trivial accept and exit*/
			done = TRUE;
		} else if ((outcode0 & outcode1) != 0)
			done = TRUE;
		/*Logical intersection is true, so trivial reject and exit.*/
		else {
			clip = WasClipped;	
			/*Failed both tests, so calculate the line segment to clip;
			  from an outside point to an intersection with clip edge.*/
			/*At least one endpoint is outside the clip rectangle; pick it.*/
			if (outcode0 != 0)
				outcodeOut = outcode0;
			else
				outcodeOut = outcode1;
			/*Now find intersection point;
			  use formulas y=y0+slope*(x-x0),x=x0+(1/slope)*(y-y0).*/
			
			if (((1L << ((long)TOP)) & outcodeOut) != 0) {
				/*Divide line at top of clip rectangle*/
				x = *x0 + (*x1 - *x0) * (V.ymax - *y0) / (*y1 - *y0);
				y = V.ymax;
			} else if (((1L << ((long)BOTTOM)) & outcodeOut) != 0) {
				/*Divide line at bottom of clip rectangle*/
				x = *x0 + (*x1 - *x0) * (V.ymin - *y0) / (*y1 - *y0);
				y = V.ymin;
			} else if (((1L << ((long)RIGHT)) & outcodeOut) != 0) {
				/*Divide line at right edge of clip rectangle*/
				y = *y0 + (*y1 - *y0) * (V.xmax - *x0) / (*x1 - *x0);
				x = V.xmax;
			} else if (((1L << ((long)LEFT)) & outcodeOut) != 0) {
				/*Divide line at left edge of clip rectangle*/
				y = *y0 + (*y1 - *y0) * (V.xmin - *x0) / (*x1 - *x0);
				x = V.xmin;
			}
			/*Now we move outside point to intersection point to clip,
			  and get ready for next pass.*/
			if (outcodeOut == outcode0) {
				*x0 = x;
				*y0 = y;
				CompOutCode(*x0, *y0, &outcode0, &V);
			} else {
				*x1 = x;
				*y1 = y;
				CompOutCode(*x1, *y1, &outcode1, &V);
			}
		}
	} while (!done);
	return clip;
}
