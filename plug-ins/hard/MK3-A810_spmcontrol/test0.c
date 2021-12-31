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

// Poly
// ln(1+x) [-1 < x <= 1] = x-x^2/2+x^3/3-x^4/4+...
// Taylor
// ln(1+x) [-1 < x <= 1] = sum[0..n] (-1)^(n-1) (n-1)!/n! x^n
// OK:
// for (x=0.1; x<2; x+=0.1){ y=0; for(i=1; i<12; i++) y += (-1)^(i-1)*(i-1)!/i!*(x-1)^i; print x,y, ln(x); }

#if 0
// for(i=1; i<12; i++) print "#define B"i,"\t", round((-1)^(i-1)*(i-1)!/i!*0x8000) , "\t// B"i, (-1)^(i-1)*(i-1)!/i!
#define B1 	 32768 	// B1 1
#define B2 	 -16384 	// B2 -0.5
#define B3 	 10923 	// B3 ~0.33333333333333333333
#define B4 	 -8192 	// B4 -0.25
#define B5 	 6554 	// B5 0.2
#define B6 	 -5461 	// B6 ~-0.16666666666666666667
#define B7 	 4681 	// B7 ~0.14285714285714285714
#define B8 	 -4096 	// B8 -0.125
#define B9 	 3641 	// B9 ~0.11111111111111111111
#define B10 	 -3277 	// B10 -0.1
#define B11 	 2979 	// B11 ~0.09090909090909090909
#define B12 	 -2731 	// B12 ~-0.08333333333333333333
#define B13 	 2521 	// B13 ~0.07692307692307692308
#define B14 	 -2341 	// B14 ~-0.07142857142857142857

int B[14] = { B1, B2, B3, B4, B5, B6, B7, B8, B9, B10, B11, B12, B13, B14 };
//int B[11] = { B1, 1.85*B2, 0.99*B3, B4, B5, B6, B7, B8, B9, B10, B11 };

#else

#define B6	 -9130  // 0xDC56 // B6, Q19
#define B5	 21677  // 0x54ad // B5, Q19
#define B4	-24950  // 0x9e8a // B4, Q17
#define B3	 20693  // 0x50d5 // B3, Q16
#define B2	-16298  // 0xc056 // B2, Q15
#define B1	 16381  // 0x3ffd // B1, Q14
#define B0	  1581  // 0x062d // B0, Q30

int B[7] = { B0, B1, B2, B3, B4, B5, B6 };

#endif

#define C1      0x4000  // 1, Q15
#define LN2	 22713  // 0x58B9 // ln2, Q15 = 0x8000 * ln(2)

//		;; y = 0.4343 * ln(x)
//		;;   |             x := M(x) * 2^P(x) --- M: Mantissa, P: Power
//		;;   = 0.4343 * ln(M*2^P)
//		;;   = 0.4343 * ( ln(1 + (2M-1)) + (P-1)ln(2) )

// calculates natural logarith, 
// uses taylor series approximation and scaling

int _norm(int x){
        if (x != 0){
                int i=0;
                if (x<0) x=-x;
                while ((x & 0x4000) == 0  && i < 16) { x <<= 1; ++i; }
                return i;
        } else return 16;
}

//#define POLY(X,Y,C) 
#define POLY(X,Y,C) (((X)>>16) * (Y) + ((C)<<16))

int spm_logn(int x){
	int u,y1,y2,p,n,m,a;
	int i;
//		;; y = 0.4343 * ln(x)
//		;;   |             x := M(x) * 2^P(x) --- M: Mantissa, P: Power
//		;;   = 0.4343 * ln(M*2^P)
//		;;   = 0.4343 * ( ln(1 + (2M-1)) + (P-1)ln(2) )

	n = _norm (x);
	p = n;
	m = x << n;
	u = 2*m - C1;  // U <- (2*M-1) Q15 (between 0.0 and 1.0)


#if 0
	double U,M,A,Y;
	double Q15=0x8000;
	double Q15S=0x8000/3151.6154;
	U = 2.*(double)m/Q15 - 1.;
	i = 7;
	A = B[--i]/Q15S;
	while (i>1)
		A = A*U + B[--i]/Q15S;
	A = 2*A*U + 2*B[--i]/Q15S;

//	Y = A;
//	Y = (p-1.)*LN2/Q15S;
//	Y = U; // 0-1 OK
//	Y = (double)m/Q15 / (1<<n);
	Y = A + -(p-1.)*LN2/Q15S;
	
	std::cout << x << " " << Y
		  << " p=" << p
		  << " n=" << n
		  << " m=" << m
		  << " u=" << u
		  << std::endl;

	return Y;
#endif

#if 0
	i = 7;
	a = B[--i] << 16;
	while (i)
		a = POLY (a, u, B[--i]);
	y1 = a >> 14;
#else
	// Polynomial approximation  f(2*M-1)
//	y1 = (((((B6*u + B5)*u + B4)*u + B3)*u + B2)*u + B1)*u + B0;
	a = B6 << 16;
	a = POLY (a,u,B5); // Q34 + Q18<<16 = Q34   
	a = POLY (a,u,B4); // Q33 + Q17<<16 = Q33  
	a = POLY (a,u,B3); // Q32 + Q16<<16 = Q32
	a = POLY (a,u,B2); // Q31 + Q15<<16 = Q31
	a = POLY (a,u,B1); // Q30 + Q14<<16 = Q30
	a = (a>>15)*u + B1;
//	a = POLY (a,u,B0); // Q30 + Q30 = Q30
	y1 = a >> 14;
#endif

	// Process exponent
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
	for (int x=0; x<0x8000; x++)
		spm_logn(x);
//		std::cout << x << " " << (spm_logn(x)) << std::endl;

}
