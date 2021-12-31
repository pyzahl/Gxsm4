/*
 * Copyright (C) 1999,2000,2001 FSF
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

#include "mathvalues.h"

/****************************************************************************/
/*  SIN() - sine							    */
/*									    */
/*  Based on the algorithm from "Software Manual for the Elementary         */
/*  Functions", Cody and Waite, Prentice Hall 1980, chapter 8.              */
/*									    */
/*  N = round(x / PI)							    */
/*  f = x - N * PI							    */
/*  g = f * f								    */
/*  R = polynomial expansion 						    */
/*									    */
/*  result = f + f * R							    */
/*									    */
/*  if x < 0, result = - result 					    */
/*  if N is even, result = - result					    */
/*									    */
/*  This will return the wrong result for x >= MAXINT * PI		    */
/****************************************************************************/

double sin(double x)
{
    double  xn, f, g, rg;
    double  sgn = (x < 0) ? -1.0 : 1.0;
    int     n;

    x  = fabs(x);
    n  = (int) ((x * INVSPI) + 0.5);
    xn = (double) n;

    /*************************************************************************/
    /* if n is odd, negate the sign                                          */
    /*************************************************************************/
    if (n % 2) sgn = -sgn;

    /*************************************************************************/
    /* f = x - xn * PI (but mathematically more stable)                      */
    /*************************************************************************/
    f = (x - xn * C1) - xn * C2;

    /*************************************************************************/
    /* determine polynomial expression                                       */
    /*************************************************************************/
    g = f * f;

#if BITS<=24
    rg = (((R4 * g + R3) * g + R2) * g + R1) * g;
#elif BITS>=25 && BITS<=32
    rg = ((((R5 * g + R4) * g + R3) * g + R2) * g + R1) * g;
#elif BITS>=33 && BITS<=50
    rg = ((((((R7*g+R6)*g+R5)*g+R4)*g+R3)*g+R2)*g+R1)*g;
#else
    rg = (((((((R8*g+R7)*g+R6)*g+R5)*g+R4)*g+R3)*g+R2)*g+R1)*g;
#endif

    return (sgn * (f + f * rg));
}


/****************************************************************************/
/*  COS() - Cosine							    */
/*									    */
/*  Based on the algorithm from "Software Manual for the Elementary         */
/*  Functions", Cody and Waite, Prentice Hall 1980, chapter 8.              */
/*									    */
/*  N = round(x / PI + 1/2) - 0.5					    */
/*  f = x - N * PI							    */
/*  g = f * f								    */
/*  R = polynomial expression						    */
/*									    */
/*  result = f + f * R							    */
/*  if N is even, result = - result					    */
/*									    */
/*  This will return the wrong result for x >= MAXINT * PI		    */
/****************************************************************************/
double cos(double x)
{
  double sgn;           /* the sign of the result */
  double xn, f, g, rg;
  int n;
  
  /**************************************************************************/
  /* cos(x) = cos(-x)                                                       */
  /**************************************************************************/
  x = fabs(x);
  
  /**************************************************************************/
  /* n = round(x/PI + 1/2) (can be rounded this way, since positive number) */
  /**************************************************************************/
  n  = (int) (((x + HALFPI) * INVSPI) + 0.5);
  xn = (double) n - 0.5;

  /**************************************************************************/
  /* if n is odd, negate the sign                                           */
  /**************************************************************************/
  sgn = (n % 2) ? -1.0 : 1.0;

  /**************************************************************************/
  /* f = x - xn * PI (but more mathematically stable)                       */
  /**************************************************************************/
  f = (x - xn * C1) - xn * C2;

  /**************************************************************************/
  /* determine polynomial expression                                        */
  /**************************************************************************/
  g = f * f;

#if BITS<=24
  rg = (((R4 * g + R3) * g + R2) * g + R1) * g;
#elif BITS>=25 && BITS<=32
  rg = ((((R5 * g + R4) * g + R3) * g + R2) * g + R1) * g;
#elif BITS>=33 && BITS<=50
  rg = ((((((R7*g+R6)*g+R5)*g+R4)*g+R3)*g+R2)*g+R1)*g;
#else
  rg = (((((((R8*g+R7)*g+R6)*g+R5)*g+R4)*g+R3)*g+R2)*g+R1)*g;
#endif

  return (sgn * (f + f * rg));
}



/****************************************************************************/
/*  ATAN() - Arctangent							    */
/*									    */
/*  Based on the algorithm from "Software Manual for the Elementary         */
/*  Functions", Cody and Waite, Prentice Hall 1980, chapter 11.             */
/*									    */
/*  if x > 1, x = 1 / x 						    */
/*  if x > 2 - sqrt(3), x = (x * sqrt(3) - 1) / (sqrt(3) + x)		    */
/*  g = x * x								    */
/*  R = polynomial expression						    */
/*									    */
/*  result = (t * (x + x * R) + an) * s					    */
/****************************************************************************/
double atan(double x)
{
    double g, p, q;
    double  s = (x < 0.0) ? -1.0F : 1.0F;            /* sign */
    double  t = 1.0;
    int    n = 0;

    static double a[4] = {0.0, 0.52359877559829887308, 1.57079632679489661923,
		   1.04719755119659774615};

    if ((x = fabs(x)) > 1.0)
    {
	x = 1.0 / x;	
	n = 2;	

	/******************************************************************/
	/* the partial result needs to be negated                         */
	/******************************************************************/
	t = -1.0;
    }

    /**********************************************************************/
    /* for x > (2 - sqrt(3)  )                                            */
    /**********************************************************************/
    if (x > TWO_SQRT3)			
    {
	/******************************************************************/
	/* x = (x * sqrt(3) -1)	/ (sqroot(3) + x)                         */
	/******************************************************************/
	x = (x * SQRTTHREE - 1.0) / (SQRTTHREE + x);
	++n; 			
    }

    /*********************************************************************/
    /* determine polynomial expression                                   */
    /*********************************************************************/
    g = x * x;	

#if BITS<=24
    p = (ATP1 * g + ATP0) * g;
    q = g + ATQ0;
#elif BITS>=25 && BITS<=32
    p = (ATP1 * g + ATP0) * g;
    q = (g + ATQ1) * g + ATQ0;
#elif BITS>=33 && BITS<=50
    p = ((ATP2 * g + ATP1) * g + ATP0) * g;
    q = ((g + ATQ2) * g + ATQ1) * g + ATQ0;
#else
    p = (((ATP3 * g + ATP2) * g + ATP1) * g + ATP0) * g;
    q = (((g + ATQ3) * g + ATQ2) * g + ATQ1) * g + ATQ0;
#endif

    /**********************************************************************/
    /* calculate the result multiplied by the correct sign                */
    /**********************************************************************/
    return ((((p / q) * x + x) * t + a[n]) * s);  
}


/****************************************************************************/
/*  FMOD() - Doubleing point remainder                                       */
/*									    */
/*  Returns the remainder after dividing x by y an integral number of times.*/
/*                                                                          */
/****************************************************************************/
double fmod(double x, double y)
{
   double d = fabs(x); 
   double e = fabs(y);

   /*************************************************************************/
   /* if y is too small or y == x, any remainder is negligible.             */
   /*************************************************************************/
   if (d - e == d || d == e) return (0.0);

   /*************************************************************************/
   /* if x and y are integers, just do a %.                                 */
   /*************************************************************************/
   if (((double)(int)d == d) && ((double)(int)e == e))
   	return ((double)((int)x % (int)y));

   /*************************************************************************/
   /* otherwise, divide; result = dividend - (quotient * divisor)           */
   /*************************************************************************/
   /*   modf(x/y, &d); */
   return (x - (double)((int)(x/y)) * y);
}



/****************************************************************************/
/*  EXP() - e ^ x							    */
/*									    */
/*  Based on the algorithm from "Software Manual for the Elementary         */
/*  Functions", Cody and Waite, Prentice Hall 1980, chapter 6.              */
/*									    */
/*  N = round(x / ln(2))						    */
/*  g = x - N * ln(2)							    */
/*  z = g * g								    */
/*									    */
/*  R = polynomial expansion						    */
/*									    */
/*  result = R * 2 ^ (N	+ 1)						    */
/****************************************************************************/
double exp(double x)
{
    double g, z, q, p;
    int n;

    /*************************************************************************/
    /* check if input would produce output out of the range of this function */
    /*************************************************************************/
    if (x > MAXX) { return (HUGE_VAL); }

    if (x < 0) n = (int) (x * INVLOGe2 - 0.5);    /* since (int) -1.5 = -1.0 */
    else       n = (int) (x * INVLOGe2 + 0.5);

    /*************************************************************************/
    /* g = x - n * ln(2) (but more mathematically stable)                    */
    /*************************************************************************/
    g  = (x - n * LNC3) - n * LNC4;

    /*************************************************************************/
    /* determine polynomial expression                                       */
    /*************************************************************************/
    z  = g * g;

#if BITS <=29
    p = (EXP1 * z + EXP0) * g;
    q = EXQ1 * z + EXQ0;
#elif BITS>=30 && BITS<=42
    p = (EXP1 * z + EXP0) * g;
    q = (EXQ2 * z + EXQ1) * z + EXQ0;
#elif BITS>=43 && BITS<=56
    p = ((EXP2 * z + EXP1) * z + EXP0) * g;
    q = (EXQ2 * z + EXQ1) * z + EXQ0;
#else
    p = ((EXP2 * z + EXP1) * z + EXP0) * g;
    q = ((EXQ3 * z + EXQ2) * z + EXQ1) * z + EXQ0;
#endif

    /*************************************************************************/
    /* exp(x) = exp(g) * 2 ^ (n + 1)                                         */
    /*************************************************************************/
    /*    return ldexp(0.5 + p / (q - p), n + 1);  */
    return (0.5 + p / (q - p)) * (1 << ( n + 1)); 
}

double sqrt(double x) {
#define DBITS2 (BITS/2+1)
	double a=x;
	double y=x;
	double diff;
	int i;
//  b = 64;
//  a = 2^ceil(b/2); //b is the wordlength you are using
//  y = a;           //a is the first estimation value for sqrt(x)
//	for (i=0; i<ceil(b/2); i++)   //each iteration achieves one bit
	for (i=0; i<DBITS2; i++)   //each iteration achieves one bit
	{                          //of accuracy
		a    *= 0.5;
		diff = x - y*y;
		if (diff > 0)
		{
			y = y + a;
		}
		else if (diff < 0)
		{
			y = y - a;
		}
	}
	return y;
}

#if 0 // did not work for PPC

#define itable ((double *)xtable)

static int xtable[16] = {
0x540bcb0d,0x3fe56936, 0x415a86d3,0x3fe35800, 0xd9ac3519,0x3fe1c80d,
0x34f91569,0x3fe08be9, 0x8f3386d8,0x3fee4794, 0x9ea02719,0x3feb5b28,
0xe4ff9edc,0x3fe92589, 0x1c52539d,0x3fe76672 };

static int norm2(double *t) {
unsigned int e,f,g;
    f = ((((unsigned int *)t)[1])>>1);
    e = ((unsigned int *)t)[1];
    f += 0x1FF80000;
    g = e&0x000FFFFF;
    f &= 0xFFF00000;
    ((int *)t)[1] = g+0x40000000-(e&0x00100000);

    return f;
}

double sqrt(double y) {
double a;
int e,c;

    e = norm2(&y);
    c = (((int *)&y)[1])>>(18)&(7);
    a = itable[c];

    for(c=0;c<6;c++) a = 0.5*a*(3.0-y*a*a);
    a*=y;

    ((int *)&a)[1] &= 0x000FFFFF;
    ((int *)&a)[1] |= e;

    return a;
}
#endif
