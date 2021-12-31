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

#include "FB_spm_dataexchange.h"
#include "g_intrinsics.h"

extern int LogFunctionASM (DSP_INT32 x, DSP_INT32 *Coeff_Log);

DSP_INT32 CoeffLog[8]={7279871,-7139840,8326142,-7755777,3657879,-5704,1211581,2525222};

// Q23
DSP_INT32 calc_mix_log(DSP_INT32 x, DSP_INT32 offset);
DSP_INT32 calc_mix_log(DSP_INT32 x, DSP_INT32 offset){
  #ifdef DSPEMU
  return (_abs (x) + offset); // fixme! (EMU only)
  #else
  return (LogFunctionASM (_abs (x) + offset, &CoeffLog[0]));
	// (LogFunctionASM (_SADD16 (_ABS16(x), offset), &CoeffLog[0]));
  #endif
}
