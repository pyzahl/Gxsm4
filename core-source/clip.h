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

#ifndef __CLIP_H__
#define __CLIP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/**
 * ClipResult:
 * 
 * Result of cohen_sutherland_line_clip_d().
 */

typedef enum { NotClipped, WasClipped } ClipResult;

ClipResult cohen_sutherland_line_clip_d (double *x0, double *y0, double *x1, double *y1,
			       double xmin_, double xmax_, double ymin_, double ymax_);

ClipResult cohen_sutherland_line_clip_i (int *x0, int *y0, int *x1, int *y1,
			       int xmin_, int xmax_, int ymin_, int ymax_);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CLIP_H__ */
