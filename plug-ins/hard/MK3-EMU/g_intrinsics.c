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

#ifdef DSPEMU
#define DSP_INT32_MAX  2147483647 
#define DSP_INT32_MIN -2147483647 

DSP_INT32 _abs (DSP_INT32 x){
        if (x >= 0)
                return x;
        else
                return -x;
}

DSP_INT32 _sat (DSP_INT64 x){
        if (x > DSP_INT32_MAX)
                return DSP_INT32_MAX;
        if (x < DSP_INT32_MIN)
                return DSP_INT32_MIN;
        return (DSP_INT32)(x);
}

DSP_INT32 _sadd (DSP_INT32 x, DSP_INT32 y){
        DSP_INT64 tmp = x;
        tmp += y;
        return _sat (tmp);
}

DSP_INT32 _ssub (DSP_INT32 x, DSP_INT32 y){
        DSP_INT64 tmp = x;
        tmp -= y;
        return _sat (tmp);
}

DSP_INT32 _smpy (DSP_INT32 x, DSP_INT32 y){
        DSP_INT64 tmp = (DSP_INT64)x * (DSP_INT64)y;
        return _sat (tmp);
}

DSP_INT32 _norm (DSP_INT32 x){
        if (x != 0){
                int i=0;
                if (x<0) x=-x;
                while ((x & 0x4000) == 0  && i < 16) { x <<= 1; ++i; }
                return i;
        } else return 16;
}

// --- same function available via PAC_pll lib as SineSDB()
void SineSDB(DSP_INT32 *cr, DSP_INT32 *ci, DSP_INT32 *cr2, DSP_INT32 *ci2, DSP_INT32 deltasRe, DSP_INT32 deltasIm)
{
        DSP_INT64 RealP,ImaP,Error_e;
        DSP_INT40 RealP_Hi,ImaP_Hi,Error_int;
        
        // *********** Sinus calculation

        //    Real part calculation : crdr - cidi
        //  Q31*Q31 - Q31*Q31=Q62
        //  For cr=2147483647 and deltasRe=2129111627
        //      ci=0 and  deltasIm=280302863
        //  crdr - cidi = 2129111626 Q31 (RealP_Hi)
        //  RealP=4572232401620063669

        RealP=((DSP_INT64)cr[0] * (DSP_INT64)deltasRe) - ((DSP_INT64)ci[0] * (DSP_INT64)deltasIm);
        RealP_Hi=(DSP_INT40)(RealP>>31); //Q62 to Q31

        //    Imaginary part calculation : drci + crdi
        //  Q31*Q31 - Q31*Q31=Q62
        //  drci - crdi= -280302863 Q31 (ImaP_Hi)
        //  ImaP=-601945814499781361

        ImaP=((DSP_INT64)ci[0] * (DSP_INT64)deltasRe) + ((DSP_INT64)cr[0] * (DSP_INT64)deltasIm);
        ImaP_Hi=(DSP_INT40)(ImaP>>31); //Q62 to Q31

        //    e=Error/2 = (1 - (cr^2+ci^2))/2
        //  e=(Q62 - Q62 - Q62))>>32 --> Q30 (so Q31/2)
        //  4611686018427387903 - (-280302863*-280302863) - (2129111626*2129111626)
        //  Error_e=4611686018427387903 - 78569695005996769 - 4533116315968363876
        //  Error_e = 7453027258
        //  Error_int = 1

        Error_e=0x3FFFFFFFFFFFFFFF; //4611686018427387903
        Error_e=Error_e - ((DSP_INT64)ImaP_Hi*(DSP_INT64)ImaP_Hi) - ((DSP_INT64)RealP_Hi*(DSP_INT64)RealP_Hi);
        Error_int=(DSP_INT40)(Error_e>>32); //Q62 to Q31 plus /2

        //    Update Cre and Cim
        //  Cr=Cr(1-e)=Cr-Cr*e // Q62 - Q31*Q31
        //  Ci=Ci(1-e)=Ci-Ci*e
        //  ImaP=-601945814499781361 - 1*-280302863 = -601945814219478498
        //  ci[0]=-280302862
        //  RealP=4572232401620063669 - 1*2129111626 = 4572232399490952043
        //  cr[0]=2129111625

        ImaP=ImaP + ((DSP_INT64)ImaP_Hi*(DSP_INT64)Error_int);
        ImaP_Hi=(DSP_INT40)(ImaP>>31); //Q62 to Q31
        if (ImaP_Hi>2147483647)ImaP_Hi=2147483647;
        if (ImaP_Hi<-2147483647)ImaP_Hi=-2147483647;
        ci[0]=(DSP_INT32)ImaP_Hi;
        RealP=RealP + ((DSP_INT64)RealP_Hi*(DSP_INT64)Error_int);
        RealP_Hi=(DSP_INT40)(RealP>>31); //Q62 to Q31
        if (RealP_Hi>2147483647)RealP_Hi=2147483647;
        if (RealP_Hi<-2147483647)RealP_Hi=-2147483647;
        cr[0]=(DSP_INT32)RealP_Hi;
}

#endif
