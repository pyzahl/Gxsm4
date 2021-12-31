/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* SRanger and Gxsm - Gnome X Scanning Microscopy Project
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * DSP tools for Linux
 *
 * Copyright (C) 1999,2000,2001,2002 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * WWW Home:
 * DSP part:  http://sranger.sf.net
 * Gxsm part: http://gxsm.sf.net
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

#ifndef __G_INTRINSICS_H
#define __G_INTRINSICS_H

#ifdef DSPEMU
#include "FB_spm_dataexchange.h"
DSP_INT32 _abs (DSP_INT32 x);
DSP_INT32 _sat (DSP_INT64 x);
DSP_INT32 _sadd (DSP_INT32 x, DSP_INT32 y);
DSP_INT32 _ssub (DSP_INT32 x, DSP_INT32 y);
DSP_INT32 _smpy (DSP_INT32 x, DSP_INT32 y);
DSP_INT32 _norm (DSP_INT32 x);
void SineSDB(DSP_INT32 *cr, DSP_INT32 *ci, DSP_INT32 *cr2, DSP_INT32 *ci2, DSP_INT32 deltasRe, DSP_INT32 deltasIm);
#endif

// SR to MK2 intrinsics -- now to be fixed for MK3!!

// _smac and similar missiong replacement macros for compatibility with C42x
#define _SAT32(X)     (_sat((long)(X)))
#define _SAT16(X)     (_SAT32((long)(X)<<16) >> 16)

#define _SMAC(A,X,Y)  (_SAT32 ((long)(A) + (long)_smpy ((X), (Y))))
#define _SMPY32(X,Y)  _smpy ((X), (Y))
#define _SSHL32(X,N)  ((N)>0 ? (_SAT32 (((X)<<(N)))) :  (_SAT32 (((X)>>(-(N))))))
#define _HI16(X)      ((X)>>16)

#define _SADD16(X,Y)  (_SAT16 ((long)(X) + (long)(Y)))
#define _SADD32(X,Y)  (_sadd ((X), (Y)))
#define _SSUB16(X,Y)  (_SAT16 ((long)(X) - (long)(Y)))
#define _SSUB32(X,Y)  (_ssub ((X), (Y)))

#define _NORM16(X)    (_norm (X))
#define _NORM32(X)    (_norm (X))

#define _ABS16(X)     (_abs(X))


#endif
