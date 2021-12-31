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

#include <locale.h>
#include <libintl.h>
#include <time.h>

#include <climits>    // for limits macrors
#include <iomanip>    // for setfill, setprecision, setw
#include <ios>        // for dec, hex, oct, showbase, showpos, etc.
#include <iostream>   // for cout
#include <sstream>    // for istringstream

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

// Logn Poly Coeff
// Taylor
// ln(1+x) [-1 < x <= 1] = sum[0..n] (-1)^(n-1) (n-1)!/n! x^n
// OK:
// for (x=0.1; x<2; x+=0.1){ y=0; for(i=1; i<12; i++) y += (-1)^(i-1)*(i-1)!/i!*(x-1)^i; print x,y, ln(x); }
// to gen test coef, not scaled
// for(i=1; i<12; i++) print "#define B"i,"\t", round((-1)^(i-1)*(i-1)!/i!*0x8000) , "\t// B"i, (-1)^(i-1)*(i-1)!/i!

#define B6	 -9130  // 0xDC56 // B6, Q19
#define B5	 21677  // 0x54ad // B5, Q18
#define B4	-24950  // 0x9e8a // B4, Q17
#define B3	 20693  // 0x50d5 // B3, Q16
#define B2	-16298  // 0xc056 // B2, Q15
#define B1	 16381  // 0x3ffd // B1, Q14
#define B0	  1581  // 0x062d // B0, Q30

int B[7] = { B0, B1, B2, B3, B4, B5, B6 };

#define C1      0x4000  // 1, Q15
#define LN2	 22713  // 0x58B9 // ln2, Q15 = 0x8000 * ln(2)

// y = 0.4343 * ln(x)
//   |             x := M(x) * 2^P(x) --- M: Mantissa, P: Power
//   = 0.4343 * ln(M*2^P)
//   = 0.4343 * ( ln(1 + (2M-1)) + (P-1)ln(2) )

// calculate natural logarith, 
// uses taylor series approximation and scaling

// norm (# leading bits)
int _norm(int x){
        if (x != 0){
                int i=0;
                if (x<0) x=-x;
                while ((x & 0x4000) == 0  && i < 16) { x <<= 1; ++i; }
                return i;
        } else return 16;
}

#define POLY(X,Y,C) (((X)>>16) * (Y) + ((C)<<16))

int spm_logn(int x){
	int u,y1,y2,p,n,m,a;
	int i;

	n = _norm (x);
	p = n;
	m = x << n;
	u = 2*m - C1;  // U <- (2*M-1) Q15 (between 0.0 and 1.0)


#if 1
	i = 7;
	a = B[--i] << 16;
	while (i)
		a = POLY (a, u, B[--i]);
	y1 = a >> 14;
#else
	// polynomial/taylor approximation  f(2*M-1)
//	y1 = (((((B6*u + B5)*u + B4)*u + B3)*u + B2)*u + B1)*u + B0;
	a = B6 << 16;
	a = POLY (a,u,B5); // Q34 + Q18<<16 = Q34   
	a = POLY (a,u,B4); // Q33 + Q17<<16 = Q33  
	a = POLY (a,u,B3); // Q32 + Q16<<16 = Q32
	a = POLY (a,u,B2); // Q31 + Q15<<16 = Q31
	a = POLY (a,u,B1); // Q30 + Q14<<16 = Q30
	a = (a>>15)*u + B0;
//	a = POLY (a,u,B0); // Q30 + Q30 = Q30
	y1 = a >> 15;
#endif

	// compute exponent
	y2 = -(p-1)*LN2;

	std::cout << x << " " << (y1+y2)
		  << " y=" << (y1 + y2)
		  << " p=" << p
		  << " n=" << n
		  << " m=" << m
		  << " u=" << u
		  << " y1=" << y1
		  << " y2=" << y2
		  << std::endl;

	return (y1 + y2);
}

int main(void){
	int i;
	unsigned char d,t,n;
	int ld;
	
	t=d=n=0;

	n=254;
	
	for (i=0; i<280; i++){
		t++;
		d=n-t;
		ld=(int)n-(int)t;
		std::cout << i << ", t=" << (int)t << " d=20-t=" << (int)d << " ld=" << ld << std::endl;
	}
	//	for (int x=0; x<0x8000; x++)
	//	spm_logn(x);
//		std::cout << x << " " << (spm_logn(x)) << std::endl;

}
