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

/****************************************************************************/
/*   VALUES.H								    */
/****************************************************************************/

//#define BITS	  23               /* There are 23 bits in the mantissa     */
//#define MAXX	  88.72283906      /* ln(HUGE_VAL)                          */
//#define MAXH	  89.41598624	   /* ln(HUGE_VAL) + ln(2)                  */
//#define TWO23	  8388608	   /* 2 ^ BITS                              */
//#define XBIG	  8.664339757	   /* (BITS/2 + 1) * ln(2)                  */

#define BITS	  42               /* There are 23 bits in the mantissa     */
#define MAXX	  321.54           /* ln(HUGE_VAL)                          */
#define MAXH	  322.23  	   /* ln(HUGE_VAL) + ln(2)                  */
#define TWO23	  4398046511104	   /* 2 ^ BITS                              */
#define XBIG	  15.2492379723	   /* (BITS/2 + 1) * ln(2)                  */

/****************************************************************************/
/*  The following macros define constants used throughout the functions.    */
/****************************************************************************/

/* macros used in asin and acos */

#define SQRTWO	  1.4142135623730950
#if BITS<=24
#define ASP1	  0.933935835
#define ASP2	 -0.504400557
#define ASQ0	  0.560363004e1
#define ASQ1	 -0.554846723e1
#elif BITS>=25 && BITS<=36
#define ASP1	 -0.27516555290596e1
#define ASP2	  0.29058762374859e1
#define ASP3	 -0.59450144193246
#define ASQ0	 -0.16509933202424e2
#define ASQ1	  0.24864728969164e2
#define ASQ2	 -0.10333867072113e2
#elif BITS>=37 && BITS<=48
#define ASP1	  0.85372164366771950e1
#define ASP2	 -0.13428707913425312e2
#define ASP3	  0.59683157617751534e1
#define ASP4	 -0.65404068999335009
#define ASQ0	  0.51223298620109691e2
#define ASQ1	 -0.10362273186401480e3
#define ASQ2	  0.68719597653808806e2
#define ASQ3	 -0.16429557557495170e2
#else
#define ASP1	 -0.27368494524164255994e2
#define ASP2	  0.57208227877891731407e2
#define ASP3	 -0.39688862997504877339e2
#define ASP4	  0.10152522233806463645e2
#define ASP5	 -0.69674573447350646411
#define ASQ0	 -0.16421096714498560795e3
#define ASQ1	  0.41714430248260412556e3
#define ASQ2	 -0.38186303361750149284e3
#define ASQ3	  0.15095270841030604719e3
#define ASQ4	 -0.23823859153670238830e2
#endif

/* macros used in atan and atan2 */

#define TWO_SQRT3 0.26794919243112270647
#define SQRTTHREE 1.73205080756887729353
#define PI	  3.14159265358979323846
#if BITS<=24
#define ATP0	 -0.4708325141
#define ATP1	 -0.5090958253e-1
#define ATQ0	  0.1412500740e1
#elif BITS>=25 && BITS<=32
#define ATP0	 -0.144008344874e1
#define ATP1	 -0.720026848898
#define ATQ0	  0.432025038919e1
#define ATQ1	  0.475222584599e1
#elif BITS>=33 && BITS<=50
#define ATP0	 -0.427432672026241096e1
#define ATP1	 -0.427444985367930329e1
#define ATP2	 -0.794391295408336251
#define ATQ0	  0.128229801607919841e2
#define ATQ1	  0.205171376564218456e2
#define ATQ2	  0.919789364835039806e1
#else
#define ATP0	 -0.13688768894191926929e2
#define ATP1	 -0.20505855195861651981e2
#define ATP2	 -0.84946240351320683534e1
#define ATP3	 -0.83758299368150059274
#define ATQ0	  0.41066306682575781263e2
#define ATQ1	  0.86157349597130242515e2
#define ATQ2	  0.59578436142597344465e2
#define ATQ3	  0.15024001160028576121e2
#endif

/* macros used in sin and cos */

#define INVSPI	  0.31830988618379067154
#define HALFPI	  1.57079632679489661923
#if BITS<=32
#define C1	  3.140625
#define C2	  9.67653589793e-4
#else
#define C1	  3.1416015625
#define C2	 -8.908910206761537356617e-6
#endif
#if BITS<=24
#define R1	 -0.1666665668e+0
#define R2	  0.8333025139e-2
#define R3	 -0.1980741872e-3
#define R4	  0.2601903036e-5
#elif BITS>=25 && BITS<=32
#define R1	 -0.1666666660883
#define R2	  0.8333330720556e-2
#define R3	 -0.1984083282313e-3
#define R4	  0.2752397106775e-5
#define R5	 -0.2386834640601e-7
#elif BITS>=33 && BITS<=50
#define R1	 -0.166666666666659653
#define R2	  0.833333333327592139e-2
#define R3	 -0.198412698232225068e-3
#define R4	  0.275573164212926457e-5
#define R5	 -0.250518708834705760e-7
#define R6	  0.160478446323816900e-9
#define R7	 -0.737066277507114174e-12
#else
#define R1	 -0.16666666666666665052
#define R2	  0.83333333333331650314e-2
#define R3	 -0.19841269841201840457e-3
#define R4	  0.27557319210152756119e-5
#define R5	 -0.25052106798274584544e-7
#define R6	  0.16058936490371589114e-9
#define R7	 -0.76429178068910467734e-12
#define R8	  0.27204790957888846175e-14
#endif

/* macros used in exp, cosh, and sinh */

#define LOGe2	  0.6931471805599453094172321
#define LOG102    0.301029995663981198017
#define INVLOGe2  1.4426950408889634074
#if BITS<=29
#define EXP0	  0.24999999950
#define EXP1	  0.41602886268e-2
#define EXQ0	  0.5
#define EXQ1	  0.49987178778e-1
#elif BITS>=30 && BITS<=42
#define EXP0	  0.24999999999992
#define EXP1	  0.59504254977591e-2
#define EXQ0	  0.5
#define EXQ1	  0.53567517645222e-1
#define EXQ2	  0.29729363682238e-3
#elif BITS>=43 && BITS<=56
#define EXP0	  0.249999999999999993
#define EXP1	  0.694360001511792852e-2
#define EXP2	  0.165203300268279130e-4
#define EXQ0	  0.5
#define EXQ1	  0.555538666969001188e-1
#define EXQ2	  0.495862884905441294e-3
#else
#define EXP0	  0.25
#define EXP1	  7.5753180159422776666e-3
#define EXP2	  3.1555192765684646356e-5
#define EXQ0	  0.5
#define EXQ1	  5.6817302698551221787e-2
#define EXQ2	  6.3121894374398503557e-4
#define EXQ3	  7.5104028399870046114e-7
#endif
#if BITS<=24
#define SHP0	 -0.713793159e1
#define SHP1	 -0.190333399
#define SHQ0	 -0.428277109e2
#elif BITS>=25 && BITS<=40
#define SHP0	  0.10622288837151e4
#define SHP1	  0.31359756456058e2
#define SHP2	  0.34364140358506
#define SHQ0	  0.63733733021822e4
#define SHQ1	 -0.13051012509199e3
#elif BITS>=41 && BITS<=50
#define SHP0	  0.23941435923050069e4
#define SHP1	  0.85943284838549010e2
#define SHP2	  0.13286428669224229e1
#define SHP3	  0.77239398202941923e-2
#define SHQ0	  0.14364861553830292e5
#define SHQ1	 -0.20258336866427869e3
#else
#define SHP0	 -0.35181283430177117881e6
#define SHP1	 -0.11563521196851768270e5
#define SHP2	 -0.16375798202630751372e3
#define SHP3	 -0.78966127417357099479
#define SHQ0	 -0.21108770058106271242e7
#define SHQ1	  0.36162723109421836460e5
#define SHQ2	 -0.27773523119650701667e3
#endif

/* macros used in log10 and log */

#define SQRTHALF  0.70710678118654752440
#define LOG10e	  0.4342944819032518
#define LNC3	  0.693359375
#define LNC4	 -2.121944400546905827679e-4
#if BITS<=24
#define LNA0	 -0.5527074855
#define LNB0	 -0.6632718214e1
#elif BITS>=25 && BITS<=32
#define LNA0	 -0.4649062303464
#define LNA1	  0.1360095468621e-1
#define LNB0	 -0.5578873750242e1
#elif BITS>=33 && BITS<=48
#define LNA0	  0.37339168963160866e1
#define LNA1	 -0.63260866233859665
#define LNA2	  0.44445515109803323e-2
#define LNB0	  0.44807002755736436e2
#define LNB1	 -0.14312354355885324e2
#else
#define LNA0	 -0.64124943423745581147e2
#define LNA1	  0.16383943563021534222e2
#define LNA2	 -0.78956112887491257267
#define LNB0	 -0.76949932108494879777e3
#define LNB1	  0.31203222091924532844e3
#define LNB2	 -0.35667977739034646171e2
#endif

/* macros used in pow */

#define L1	  2.885390072738
#define L3	  0.961800762286
#define L5	  0.576584342056
#define L7	  0.434259751292
#define T6	  0.0002082045327
#define T5	  0.001266912225
#define T4	  0.009656843287
#define T3	  0.05549288453
#define T2	  0.2402279975
#define T1	  0.6931471019

/* macros used in tan */

#define TWOINVPI  0.63661977236758134308
#if BITS<=32
#define C5	  1.5703125
#define C6	  4.83826794897e-4
#else
#define C5	  1.57080078125
#define C6	 -4.454455103380768678308e-6
#endif
#if BITS<=24
#define TAP1	 -0.958017723e-1
#define TAQ1	 -0.429135777e+0
#define TAQ2	  0.971685835e-2
#elif BITS>=25 && BITS<=32
#define TAP1	 -0.1113614403566
#define TAP2	  0.1075154738488e-2
#define TAQ1	 -0.4446947720281
#define TAQ2	  0.1597339213300e-1
#elif BITS>=33 && BITS<=52
#define TAP1	 -0.1282834704095743847
#define TAP2	  0.2805918241169988906e-2
#define TAP3	 -0.7483634966612065149e-5
#define TAQ1	 -0.4616168037429048840
#define TAQ2	  0.2334485282206872802e-1
#define TAQ3	 -0.2084480442203870948e-3
#else
#define TAP1	 -0.13338350006421960681
#define TAP2	  0.34248878235890589960e-2
#define TAP3	 -0.17861707342254426711e-4
#define TAQ1	 -0.46671683339755294240
#define TAQ2	  0.25663832289440112864
#define TAQ3	 -0.31181531907010027307e-3
#define TAQ4	  0.49819433993786512270e-6
#endif

/* macros used in tanh */

#define LOGe3by2  0.54930614433405484570
#if BITS<=24
#define THP0	 -0.8237728127
#define THP1	 -0.3831010665e-2
#define THQ0	  0.2471319654e1
#elif BITS>=25 && BITS<=36
#define THP0	 -0.21063958000245e2
#define THP1	 -0.93363475652401
#define THQ0	  0.63191874015582e2
#define THQ1	  0.28077653470471e2
#elif BITS>=37 && BITS<=48
#define THP0	 -0.19059522426982292e2
#define THP1	 -0.92318689451426177
#define THP2	 -0.36242421934642173e-3
#define THQ0	  0.57178567280965817e2
#define THQ1	  0.25640987595178975e2
#else
#define THP0	 -0.16134119023996228053e4
#define THP1	 -0.99225929672236083313e2
#define THP2	 -0.96437492777225469787
#define THQ0	  0.48402357071988688686e4
#define THQ1	  0.22337720718962312926e4
#define THQ2	  0.11274474380534949335e3
#endif
