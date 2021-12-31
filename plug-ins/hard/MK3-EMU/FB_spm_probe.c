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

#include "g_intrinsics.h"
#include "FB_spm_dataexchange.h"
#include "dataprocess.h"
#include "FB_spm_probe.h"
#include "ReadWrite_GPIO.h"


/* compensate for X AIC pipe delay */
#define PIPE_LEN (5)

#ifndef DSPEMU
extern SPM_STATEMACHINE state;
extern FEEDBACK_MIXER   feedback_mixer;
extern SERVO            z_servo;
extern SERVO            m_servo;
extern ANALOG_VALUES    analog;
extern PROBE            probe;
extern DATA_FIFO_EXTERN probe_datafifo;
extern MOVE_OFFSET      move;
extern AREA_SCAN        scan;
extern CR_GENERIC_IO    CR_generic_io;
#endif

// #define SPECT_SIN_LEN_7    128                   /* sine table length, must be a power of 2 */
// #define SPECT_SIN_LEN_P2     7
#define SPECT_SIN_LEN_9    512                   /* sine table length, must be a power of 2 */
#define SPECT_SIN_LEN_P2     9
// #define SPECT_SIN_LEN_10   1024                  /* sine table length, must be a power of 2 */
// #define SPECT_SIN_LEN_P2     10

#ifdef  SPECT_SIN_LEN_7
#define SPECT_SIN_LEN      SPECT_SIN_LEN_7
#define PHASE_FACTOR_Q19   364      /* (1<<15)*SPECT_SIN_LEN/360/16 */
#define AC_TAB_INC         1

// calc: 
// pi=acos(0)*2; for(i=0; i<128; ++i) { if(i%8==0) printf("\n\t"); printf("%6d, ",int(40000.5+(2^15-1)*sin(2*pi*i/128.))-40000); }
DSP_INT16 PRB_sintab[SPECT_SIN_LEN_7] = {
             0,   1608,   3212,   4808,   6393,   7962,   9512,  11039, 
         12539,  14010,  15446,  16846,  18204,  19519,  20787,  22005, 
         23170,  24279,  25329,  26319,  27245,  28105,  28898,  29621, 
         30273,  30852,  31356,  31785,  32137,  32412,  32609,  32728, 
         32767,  32728,  32609,  32412,  32137,  31785,  31356,  30852, 
         30273,  29621,  28898,  28105,  27245,  26319,  25329,  24279, 
         23170,  22005,  20787,  19519,  18204,  16846,  15446,  14010, 
         12539,  11039,   9512,   7962,   6393,   4808,   3212,   1608, 
             0,  -1608,  -3212,  -4808,  -6393,  -7962,  -9512, -11039, 
        -12539, -14010, -15446, -16846, -18204, -19519, -20787, -22005, 
        -23170, -24279, -25329, -26319, -27245, -28105, -28898, -29621, 
        -30273, -30852, -31356, -31785, -32137, -32412, -32609, -32728, 
        -32767, -32728, -32609, -32412, -32137, -31785, -31356, -30852, 
        -30273, -29621, -28898, -28105, -27245, -26319, -25329, -24279, 
        -23170, -22005, -20787, -19519, -18204, -16846, -15446, -14010, 
        -12539, -11039,  -9512,  -7962,  -6393,  -4808,  -3212,  -1608
};

DSP_INT16 PRB_sinreftabA[SPECT_SIN_LEN_7] = {
             0,   1608,   3212,   4808,   6393,   7962,   9512,  11039, 
         12539,  14010,  15446,  16846,  18204,  19519,  20787,  22005, 
         23170,  24279,  25329,  26319,  27245,  28105,  28898,  29621, 
         30273,  30852,  31356,  31785,  32137,  32412,  32609,  32728, 
         32767,  32728,  32609,  32412,  32137,  31785,  31356,  30852, 
         30273,  29621,  28898,  28105,  27245,  26319,  25329,  24279, 
         23170,  22005,  20787,  19519,  18204,  16846,  15446,  14010, 
         12539,  11039,   9512,   7962,   6393,   4808,   3212,   1608, 
             0,  -1608,  -3212,  -4808,  -6393,  -7962,  -9512, -11039, 
        -12539, -14010, -15446, -16846, -18204, -19519, -20787, -22005, 
        -23170, -24279, -25329, -26319, -27245, -28105, -28898, -29621, 
        -30273, -30852, -31356, -31785, -32137, -32412, -32609, -32728, 
        -32767, -32728, -32609, -32412, -32137, -31785, -31356, -30852, 
        -30273, -29621, -28898, -28105, -27245, -26319, -25329, -24279, 
        -23170, -22005, -20787, -19519, -18204, -16846, -15446, -14010, 
        -12539, -11039,  -9512,  -7962,  -6393,  -4808,  -3212,  -1608
};

DSP_INT16 PRB_sinreftabB[SPECT_SIN_LEN_7] = {
             0,   1608,   3212,   4808,   6393,   7962,   9512,  11039, 
         12539,  14010,  15446,  16846,  18204,  19519,  20787,  22005, 
         23170,  24279,  25329,  26319,  27245,  28105,  28898,  29621, 
         30273,  30852,  31356,  31785,  32137,  32412,  32609,  32728, 
         32767,  32728,  32609,  32412,  32137,  31785,  31356,  30852, 
         30273,  29621,  28898,  28105,  27245,  26319,  25329,  24279, 
         23170,  22005,  20787,  19519,  18204,  16846,  15446,  14010, 
         12539,  11039,   9512,   7962,   6393,   4808,   3212,   1608, 
             0,  -1608,  -3212,  -4808,  -6393,  -7962,  -9512, -11039, 
        -12539, -14010, -15446, -16846, -18204, -19519, -20787, -22005, 
        -23170, -24279, -25329, -26319, -27245, -28105, -28898, -29621, 
        -30273, -30852, -31356, -31785, -32137, -32412, -32609, -32728, 
        -32767, -32728, -32609, -32412, -32137, -31785, -31356, -30852, 
        -30273, -29621, -28898, -28105, -27245, -26319, -25329, -24279, 
        -23170, -22005, -20787, -19519, -18204, -16846, -15446, -14010, 
        -12539, -11039,  -9512,  -7962,  -6393,  -4808,  -3212,  -1608
};
#endif


#ifdef  SPECT_SIN_LEN_9
#define SPECT_SIN_LEN      SPECT_SIN_LEN_9
#define PHASE_FACTOR_Q19   1457     /* (1<<15)*SPECT_SIN_LEN/360/16 */
#define AC_TAB_INC         4

// pi=acos(0)*2; for(i=0; i<512; ++i) { if(i%8==0) printf("\n\t"); printf("%6d, ",int(round(2^15-1)*sin(2*pi*i/512.))); }
DSP_INT16 PRB_sintab[SPECT_SIN_LEN_9] = {
	     0,    402,    804,   1206,   1607,   2009,   2410,   2811, 
	  3211,   3611,   4011,   4409,   4807,   5205,   5601,   5997, 
	  6392,   6786,   7179,   7571,   7961,   8351,   8739,   9126, 
	  9511,   9895,  10278,  10659,  11038,  11416,  11792,  12166, 
	 12539,  12909,  13278,  13645,  14009,  14372,  14732,  15090, 
	 15446,  15799,  16150,  16499,  16845,  17189,  17530,  17868, 
	 18204,  18537,  18867,  19194,  19519,  19840,  20159,  20474, 
	 20787,  21096,  21402,  21705,  22004,  22301,  22594,  22883, 
	 23169,  23452,  23731,  24006,  24278,  24546,  24811,  25072, 
	 25329,  25582,  25831,  26077,  26318,  26556,  26789,  27019, 
	 27244,  27466,  27683,  27896,  28105,  28309,  28510,  28706, 
	 28897,  29085,  29268,  29446,  29621,  29790,  29955,  30116, 
	 30272,  30424,  30571,  30713,  30851,  30984,  31113,  31236, 
	 31356,  31470,  31580,  31684,  31785,  31880,  31970,  32056, 
	 32137,  32213,  32284,  32350,  32412,  32468,  32520,  32567, 
	 32609,  32646,  32678,  32705,  32727,  32744,  32757,  32764, 
	 32767,  32764,  32757,  32744,  32727,  32705,  32678,  32646, 
	 32609,  32567,  32520,  32468,  32412,  32350,  32284,  32213, 
	 32137,  32056,  31970,  31880,  31785,  31684,  31580,  31470, 
	 31356,  31236,  31113,  30984,  30851,  30713,  30571,  30424, 
	 30272,  30116,  29955,  29790,  29621,  29446,  29268,  29085, 
	 28897,  28706,  28510,  28309,  28105,  27896,  27683,  27466, 
	 27244,  27019,  26789,  26556,  26318,  26077,  25831,  25582, 
	 25329,  25072,  24811,  24546,  24278,  24006,  23731,  23452, 
	 23169,  22883,  22594,  22301,  22004,  21705,  21402,  21096, 
	 20787,  20474,  20159,  19840,  19519,  19194,  18867,  18537, 
	 18204,  17868,  17530,  17189,  16845,  16499,  16150,  15799, 
	 15446,  15090,  14732,  14372,  14009,  13645,  13278,  12909, 
	 12539,  12166,  11792,  11416,  11038,  10659,  10278,   9895, 
	  9511,   9126,   8739,   8351,   7961,   7571,   7179,   6786, 
	  6392,   5997,   5601,   5205,   4807,   4409,   4011,   3611, 
	  3211,   2811,   2410,   2009,   1607,   1206,    804,    402, 
	     0,   -402,   -804,  -1206,  -1607,  -2009,  -2410,  -2811, 
	 -3211,  -3611,  -4011,  -4409,  -4807,  -5205,  -5601,  -5997, 
	 -6392,  -6786,  -7179,  -7571,  -7961,  -8351,  -8739,  -9126, 
	 -9511,  -9895, -10278, -10659, -11038, -11416, -11792, -12166, 
	-12539, -12909, -13278, -13645, -14009, -14372, -14732, -15090, 
	-15446, -15799, -16150, -16499, -16845, -17189, -17530, -17868, 
	-18204, -18537, -18867, -19194, -19519, -19840, -20159, -20474, 
	-20787, -21096, -21402, -21705, -22004, -22301, -22594, -22883, 
	-23169, -23452, -23731, -24006, -24278, -24546, -24811, -25072, 
	-25329, -25582, -25831, -26077, -26318, -26556, -26789, -27019, 
	-27244, -27466, -27683, -27896, -28105, -28309, -28510, -28706, 
	-28897, -29085, -29268, -29446, -29621, -29790, -29955, -30116, 
	-30272, -30424, -30571, -30713, -30851, -30984, -31113, -31236, 
	-31356, -31470, -31580, -31684, -31785, -31880, -31970, -32056, 
	-32137, -32213, -32284, -32350, -32412, -32468, -32520, -32567, 
	-32609, -32646, -32678, -32705, -32727, -32744, -32757, -32764, 
	-32767, -32764, -32757, -32744, -32727, -32705, -32678, -32646, 
	-32609, -32567, -32520, -32468, -32412, -32350, -32284, -32213, 
	-32137, -32056, -31970, -31880, -31785, -31684, -31580, -31470, 
	-31356, -31236, -31113, -30984, -30851, -30713, -30571, -30424, 
	-30272, -30116, -29955, -29790, -29621, -29446, -29268, -29085, 
	-28897, -28706, -28510, -28309, -28105, -27896, -27683, -27466, 
	-27244, -27019, -26789, -26556, -26318, -26077, -25831, -25582, 
	-25329, -25072, -24811, -24546, -24278, -24006, -23731, -23452, 
	-23169, -22883, -22594, -22301, -22004, -21705, -21402, -21096, 
	-20787, -20474, -20159, -19840, -19519, -19194, -18867, -18537, 
	-18204, -17868, -17530, -17189, -16845, -16499, -16150, -15799, 
	-15446, -15090, -14732, -14372, -14009, -13645, -13278, -12909, 
	-12539, -12166, -11792, -11416, -11038, -10659, -10278,  -9895, 
	 -9511,  -9126,  -8739,  -8351,  -7961,  -7571,  -7179,  -6786, 
	 -6392,  -5997,  -5601,  -5205,  -4807,  -4409,  -4011,  -3611, 
	 -3211,  -2811,  -2410,  -2009,  -1607,  -1206,   -804,   -402
};
#endif

#ifdef  SPECT_SIN_LEN_10
#define SPECT_SIN_LEN      SPECT_SIN_LEN_10#
#define PHASE_FACTOR_Q19   2913     /* (1<<15)*SPECT_SIN_LEN/360/16 */
#define AC_TAB_INC         5

//  pi=acos(0)*2; for(i=0; i<1024; ++i) { if(i%8==0) printf("\n\t"); printf("%6d, ",int(40000.5+(2^15-1)*sin(2*pi*i/1024.))-40000); }
DSP_INT16 PRB_sintab[SPECT_SIN_LEN] = {
             0,    201,    402,    603,    804,   1005,   1206,   1407,
          1608,   1809,   2009,   2210,   2410,   2611,   2811,   3012,
          3212,   3412,   3612,   3811,   4011,   4210,   4410,   4609,
          4808,   5007,   5205,   5404,   5602,   5800,   5998,   6195,
          6393,   6590,   6786,   6983,   7179,   7375,   7571,   7767,
          7962,   8157,   8351,   8545,   8739,   8933,   9126,   9319,
          9512,   9704,   9896,  10087,  10278,  10469,  10659,  10849,
         11039,  11228,  11417,  11605,  11793,  11980,  12167,  12353,
         12539,  12725,  12910,  13094,  13279,  13462,  13645,  13828,
         14010,  14191,  14372,  14553,  14732,  14912,  15090,  15269,
         15446,  15623,  15800,  15976,  16151,  16325,  16499,  16673,
         16846,  17018,  17189,  17360,  17530,  17700,  17869,  18037,
         18204,  18371,  18537,  18703,  18868,  19032,  19195,  19357,
         19519,  19680,  19841,  20000,  20159,  20317,  20475,  20631,
         20787,  20942,  21096,  21250,  21403,  21554,  21705,  21856,
         22005,  22154,  22301,  22448,  22594,  22739,  22884,  23027,
         23170,  23311,  23452,  23592,  23731,  23870,  24007,  24143,
         24279,  24413,  24547,  24680,  24811,  24942,  25072,  25201,
         25329,  25456,  25582,  25708,  25832,  25955,  26077,  26198,
         26319,  26438,  26556,  26674,  26790,  26905,  27019,  27133,
         27245,  27356,  27466,  27575,  27683,  27790,  27896,  28001,
         28105,  28208,  28310,  28411,  28510,  28609,  28706,  28803,
         28898,  28992,  29085,  29177,  29268,  29358,  29447,  29534,
         29621,  29706,  29791,  29874,  29956,  30037,  30117,  30195,
         30273,  30349,  30424,  30498,  30571,  30643,  30714,  30783,
         30852,  30919,  30985,  31050,  31113,  31176,  31237,  31297,
         31356,  31414,  31470,  31526,  31580,  31633,  31685,  31736,
         31785,  31833,  31880,  31926,  31971,  32014,  32057,  32098,
         32137,  32176,  32213,  32250,  32285,  32318,  32351,  32382,
         32412,  32441,  32469,  32495,  32521,  32545,  32567,  32589,
         32609,  32628,  32646,  32663,  32678,  32692,  32705,  32717,
         32728,  32737,  32745,  32752,  32757,  32761,  32765,  32766,
         32767,  32766,  32765,  32761,  32757,  32752,  32745,  32737,
         32728,  32717,  32705,  32692,  32678,  32663,  32646,  32628,
         32609,  32589,  32567,  32545,  32521,  32495,  32469,  32441,
         32412,  32382,  32351,  32318,  32285,  32250,  32213,  32176,
         32137,  32098,  32057,  32014,  31971,  31926,  31880,  31833,
         31785,  31736,  31685,  31633,  31580,  31526,  31470,  31414,
         31356,  31297,  31237,  31176,  31113,  31050,  30985,  30919,
         30852,  30783,  30714,  30643,  30571,  30498,  30424,  30349,
         30273,  30195,  30117,  30037,  29956,  29874,  29791,  29706,
         29621,  29534,  29447,  29358,  29268,  29177,  29085,  28992,
         28898,  28803,  28706,  28609,  28510,  28411,  28310,  28208,
         28105,  28001,  27896,  27790,  27683,  27575,  27466,  27356,
         27245,  27133,  27019,  26905,  26790,  26674,  26556,  26438,
         26319,  26198,  26077,  25955,  25832,  25708,  25582,  25456,
         25329,  25201,  25072,  24942,  24811,  24680,  24547,  24413,
         24279,  24143,  24007,  23870,  23731,  23592,  23452,  23311,
         23170,  23027,  22884,  22739,  22594,  22448,  22301,  22154,
         22005,  21856,  21705,  21554,  21403,  21250,  21096,  20942,
         20787,  20631,  20475,  20317,  20159,  20000,  19841,  19680,
         19519,  19357,  19195,  19032,  18868,  18703,  18537,  18371,
         18204,  18037,  17869,  17700,  17530,  17360,  17189,  17018,
         16846,  16673,  16499,  16325,  16151,  15976,  15800,  15623,
         15446,  15269,  15090,  14912,  14732,  14553,  14372,  14191,
         14010,  13828,  13645,  13462,  13279,  13094,  12910,  12725,
         12539,  12353,  12167,  11980,  11793,  11605,  11417,  11228,
         11039,  10849,  10659,  10469,  10278,  10087,   9896,   9704,
          9512,   9319,   9126,   8933,   8739,   8545,   8351,   8157,
          7962,   7767,   7571,   7375,   7179,   6983,   6786,   6590,
          6393,   6195,   5998,   5800,   5602,   5404,   5205,   5007,
          4808,   4609,   4410,   4210,   4011,   3811,   3612,   3412,
          3212,   3012,   2811,   2611,   2410,   2210,   2009,   1809,
          1608,   1407,   1206,   1005,    804,    603,    402,    201,
             0,   -201,   -402,   -603,   -804,  -1005,  -1206,  -1407,
         -1608,  -1809,  -2009,  -2210,  -2410,  -2611,  -2811,  -3012,
         -3212,  -3412,  -3612,  -3811,  -4011,  -4210,  -4410,  -4609,
         -4808,  -5007,  -5205,  -5404,  -5602,  -5800,  -5998,  -6195,
         -6393,  -6590,  -6786,  -6983,  -7179,  -7375,  -7571,  -7767,
         -7962,  -8157,  -8351,  -8545,  -8739,  -8933,  -9126,  -9319,
         -9512,  -9704,  -9896, -10087, -10278, -10469, -10659, -10849,
        -11039, -11228, -11417, -11605, -11793, -11980, -12167, -12353,
        -12539, -12725, -12910, -13094, -13279, -13462, -13645, -13828,
        -14010, -14191, -14372, -14553, -14732, -14912, -15090, -15269,
        -15446, -15623, -15800, -15976, -16151, -16325, -16499, -16673,
        -16846, -17018, -17189, -17360, -17530, -17700, -17869, -18037,
        -18204, -18371, -18537, -18703, -18868, -19032, -19195, -19357,
        -19519, -19680, -19841, -20000, -20159, -20317, -20475, -20631,
        -20787, -20942, -21096, -21250, -21403, -21554, -21705, -21856,
        -22005, -22154, -22301, -22448, -22594, -22739, -22884, -23027,
        -23170, -23311, -23452, -23592, -23731, -23870, -24007, -24143,
        -24279, -24413, -24547, -24680, -24811, -24942, -25072, -25201,
        -25329, -25456, -25582, -25708, -25832, -25955, -26077, -26198,
        -26319, -26438, -26556, -26674, -26790, -26905, -27019, -27133,
        -27245, -27356, -27466, -27575, -27683, -27790, -27896, -28001,
        -28105, -28208, -28310, -28411, -28510, -28609, -28706, -28803,
        -28898, -28992, -29085, -29177, -29268, -29358, -29447, -29534,
        -29621, -29706, -29791, -29874, -29956, -30037, -30117, -30195,
        -30273, -30349, -30424, -30498, -30571, -30643, -30714, -30783,
        -30852, -30919, -30985, -31050, -31113, -31176, -31237, -31297,
        -31356, -31414, -31470, -31526, -31580, -31633, -31685, -31736,
        -31785, -31833, -31880, -31926, -31971, -32014, -32057, -32098,
        -32137, -32176, -32213, -32250, -32285, -32318, -32351, -32382,
        -32412, -32441, -32469, -32495, -32521, -32545, -32567, -32589,
        -32609, -32628, -32646, -32663, -32678, -32692, -32705, -32717,
        -32728, -32737, -32745, -32752, -32757, -32761, -32765, -32766,
        -32767, -32766, -32765, -32761, -32757, -32752, -32745, -32737,
        -32728, -32717, -32705, -32692, -32678, -32663, -32646, -32628,
        -32609, -32589, -32567, -32545, -32521, -32495, -32469, -32441,
        -32412, -32382, -32351, -32318, -32285, -32250, -32213, -32176,
        -32137, -32098, -32057, -32014, -31971, -31926, -31880, -31833,
        -31785, -31736, -31685, -31633, -31580, -31526, -31470, -31414,
        -31356, -31297, -31237, -31176, -31113, -31050, -30985, -30919,
        -30852, -30783, -30714, -30643, -30571, -30498, -30424, -30349,
        -30273, -30195, -30117, -30037, -29956, -29874, -29791, -29706,
        -29621, -29534, -29447, -29358, -29268, -29177, -29085, -28992,
        -28898, -28803, -28706, -28609, -28510, -28411, -28310, -28208,
        -28105, -28001, -27896, -27790, -27683, -27575, -27466, -27356,
        -27245, -27133, -27019, -26905, -26790, -26674, -26556, -26438,
        -26319, -26198, -26077, -25955, -25832, -25708, -25582, -25456,
        -25329, -25201, -25072, -24942, -24811, -24680, -24547, -24413,
        -24279, -24143, -24007, -23870, -23731, -23592, -23452, -23311,
        -23170, -23027, -22884, -22739, -22594, -22448, -22301, -22154,
        -22005, -21856, -21705, -21554, -21403, -21250, -21096, -20942,
        -20787, -20631, -20475, -20317, -20159, -20000, -19841, -19680,
        -19519, -19357, -19195, -19032, -18868, -18703, -18537, -18371,
        -18204, -18037, -17869, -17700, -17530, -17360, -17189, -17018,
        -16846, -16673, -16499, -16325, -16151, -15976, -15800, -15623,
        -15446, -15269, -15090, -14912, -14732, -14553, -14372, -14191,
        -14010, -13828, -13645, -13462, -13279, -13094, -12910, -12725,
        -12539, -12353, -12167, -11980, -11793, -11605, -11417, -11228,
        -11039, -10849, -10659, -10469, -10278, -10087,  -9896,  -9704,
         -9512,  -9319,  -9126,  -8933,  -8739,  -8545,  -8351,  -8157,
         -7962,  -7767,  -7571,  -7375,  -7179,  -6983,  -6786,  -6590,
         -6393,  -6195,  -5998,  -5800,  -5602,  -5404,  -5205,  -5007,
         -4808,  -4609,  -4410,  -4210,  -4011,  -3811,  -3612,  -3412,
         -3212,  -3012,  -2811,  -2611,  -2410,  -2210,  -2009,  -1809,
         -1608,  -1407,  -1206,  -1005,   -804,   -603,   -402,   -201
};
#endif

#define SPECT_SIN_LEN_MSK  (SPECT_SIN_LEN-1)     /* "modulo" mask */
#define LOCKIN_ILN         8                     /* LockIn Interval interleave number */
#define PRB_SE_DELAY       (LOCKIN_ILN + 2)    /* Start/End Delay */

extern DSP_INT16      prbdf;
extern DSP_INT32      FB_IN_processed[4];

int PRB_store_nx_init[3];
int PRB_store_nx_finish[3];
int PRB_store_nx;

int AC_tab_inc = 1;
int PRB_ki;
DSP_INT32 PRB_norm2;
DSP_INT32 PRB_ref1stA;
DSP_INT32 PRB_ref2ndA;
DSP_INT32 PRB_ref1stB;
DSP_INT32 PRB_ref2ndB;
DSP_INT32 PRB_AveCount;
DSP_INT32 PRB_modindex;
DSP_INT32 PRB_ACPhIdxA;
DSP_INT32 PRB_ACPhIdxB;
DSP_INT32 PRB_ACPhIdxdBA;
DSP_INT64 PRB_data_sumA;
DSP_INT64 PRB_data_sumB;
DSP_INT64 PRB_korrel_sum1stA[LOCKIN_ILN];
DSP_INT64 PRB_korrel_sum2ndA[LOCKIN_ILN];
DSP_INT64 PRB_korrel_sum1stB[LOCKIN_ILN];
DSP_INT64 PRB_korrel_sum2ndB[LOCKIN_ILN];

DSP_INT64 PRB_LockIn_0;
DSP_INT64 PRB_LockIn_1stA;
DSP_INT64 PRB_LockIn_1stB;
DSP_INT64 PRB_LockIn_2ndA;
DSP_INT64 PRB_LockIn_2ndB;

DSP_INT64 VP_sec_int0;
DSP_INT64 VP_sec_int1;
DSP_INT32 VP_sec_count;

// long??? troubel w compat.
DSP_INT32 PRB_AIC_data_sum[9];
int PRB_AIC_num_samples;

int  PRB_Trk_state;
int  PRB_Trk_i;
int  PRB_Trk_ie;
int  PRB_Trk_ref;

int  VP_sec_end_buffer[10*8];

int  GPIO_subsample;

#define HRVALUE_TO_16(VALUE32)  _SSHL32 (VALUE32, -16)


#if 0
// Hacking stuff

/*
Wird #OutChannels * #(MaxDelayFIFO-Out-MinDelayFIFO-Out) + #(InChannels + InData(DSP-Level)Channels) * #(MaxDelayFIFO-Out-MinDelayFIFO-Out + MaxDelayFIFO-In) Words benoetigen.

#OutChannels = #{ Xs, Ys, Zs, U, Phi?, XYZoffset?} = 5
#InChannels  = #{AIC0,1,2,3,4,5,  Umon, Zmon, Sec, LockIn0,1,2} = 12
*/


#endif

int  PRB_section_count;

int PRB_lockin_restart_flg;

#ifndef DSPEMU
#pragma CODE_SECTION(init_probe_module, ".text:slow")
#endif

void init_probe_module(){
	DSP_MEMORY(probe).LockInRefSinTabA = (DSP_INT16_P)PRB_sintab;
	DSP_MEMORY(probe).LockInRefSinLenA = SPECT_SIN_LEN;	
	DSP_MEMORY(probe).LockInRefSinTabB = (DSP_INT16_P)PRB_sintab;
	DSP_MEMORY(probe).LockInRefSinLenB = SPECT_SIN_LEN;
}

#ifndef DSPEMU
#pragma CODE_SECTION(probe_store, ".text:slow")
#endif

void probe_store (){
}

#ifndef DSPEMU
#pragma CODE_SECTION(probe_restore, ".text:slow")
#endif

void probe_restore (){
}

#ifndef DSPEMU
#pragma CODE_SECTION(probe_append_header_and_positionvector, ".text:slow")
#endif

void probe_append_header_and_positionvector (){ // size: 14
	// Section header: [SRCS, N, TIME]_32 :: 6
	if (DSP_MEMORY(probe).vector){
		*DSP_MEMORY(probe_datafifo).buffer_l++ = DSP_MEMORY(probe).vector->srcs; 
		*DSP_MEMORY(probe_datafifo).buffer_l++ = DSP_MEMORY(probe).vector->n; 
	} else {
		*DSP_MEMORY(probe_datafifo).buffer_l++ = 0; 
		*DSP_MEMORY(probe_datafifo).buffer_l++ = 0; 
	}
	*DSP_MEMORY(probe_datafifo).buffer_l++ = DSP_MEMORY(probe).time; 
	DSP_MEMORY(probe_datafifo).buffer_w += 6; // skip 6 words == 3 long

        // Section start situation vector [XY[Z or Phase]_Offset, XYZ_Scan, Upos(bias), 0xffff]_16 :: 8
        *DSP_MEMORY(probe_datafifo).buffer_w++ = HRVALUE_TO_16 (DSP_MEMORY(move).xyz_vec[i_X]);
        *DSP_MEMORY(probe_datafifo).buffer_w++ = HRVALUE_TO_16 (DSP_MEMORY(move).xyz_vec[i_Y]);
        *DSP_MEMORY(probe_datafifo).buffer_w++ = _SSHL32 (DSP_MEMORY(probe).PRB_ACPhaseA32, -12); // Phase    //  HRVALUE_TO_16 (DSP_MEMORY(move).xyz_vec[i_Z])

        *DSP_MEMORY(probe_datafifo).buffer_w++ = HRVALUE_TO_16 (DSP_MEMORY(scan).xy_r_vec[i_X]);
        *DSP_MEMORY(probe_datafifo).buffer_w++ = HRVALUE_TO_16 (DSP_MEMORY(scan).xy_r_vec[i_Y]);
        *DSP_MEMORY(probe_datafifo).buffer_w++ = AIC_OUT(5);  // Z: feedback.z + Z_pos>>16; *****!

        *DSP_MEMORY(probe_datafifo).buffer_w++ = AIC_OUT(6);  // Bias: U_pos>>16; *******!
        *DSP_MEMORY(probe_datafifo).buffer_w++ = PRB_section_count;
        DSP_MEMORY(probe_datafifo).buffer_l += 4; // skip 4 long == 8 words

        DSP_MEMORY(probe_datafifo).w_position += 6 + 6 + 2;
}

#ifndef DSPEMU
#pragma CODE_SECTION(init_lockin, ".text:slow")
#endif

void init_lockin (int target_mode){
	int j;
	// init LockIn variables
	DSP_MEMORY(probe).state = PROBE_INIT_LOCKIN;

	// calc pre-start pos to center integration window correct
	// PRB_xS -= PRB_dnx * LOCKIN_ILN/2;
	
	// adjust "DSP_MEMORY(probe).nx_init" -- fixme
	
	// init correlation sum buffer
	PRB_ki = 0;
	for (j = 0; j < LOCKIN_ILN; ++j){
		PRB_korrel_sum1stA[j] = 0L;
		PRB_korrel_sum1stB[j] = 0L;
		PRB_korrel_sum2ndA[j] = 0L;
		PRB_korrel_sum2ndB[j] = 0L;
	}
	PRB_ref1stA = 0;
	PRB_ref1stB = 0;
	PRB_ref2ndA = 0;
	PRB_ref2ndB = 0;
	PRB_AveCount = 0;
	PRB_modindex = 0;
	DSP_MEMORY(probe).LockIn_ref = 0;
	
	// calc phase in indices, "/" needs _idiv, so omit it here!
//				PRB_ACPhIclscdx  = (int)(DSP_MEMORY(probe).AC_phase*SPECT_SIN_LEN/360);
	PRB_ACPhIdxA   = _SSHL32 (_SMPY32 (DSP_MEMORY(probe).AC_phaseA, PHASE_FACTOR_Q19), -15);
	PRB_ACPhIdxB   = _SSHL32 (_SMPY32 (DSP_MEMORY(probe).AC_phaseB, PHASE_FACTOR_Q19), -15);
	PRB_ACPhIdxdBA = PRB_ACPhIdxB - PRB_ACPhIdxA;
	DSP_MEMORY(probe).PRB_ACPhaseA32 = _SSHL32 (DSP_MEMORY(probe).AC_phaseA, 16);
	
	// AIC samples at 150000Hz, full Sintab length is 512, so need increments of
	// 1,2,4,8 for the following discrete frequencies
	if (DSP_MEMORY(probe).AC_frq >= 8000)
		AC_tab_inc = AC_TAB_INC<<4;   // LockIn-Ref on 9375.00Hz
	else if (DSP_MEMORY(probe).AC_frq >= 4000)
		AC_tab_inc = AC_TAB_INC<<2;   // LockIn-Ref on 4687.50Hz
	else if (DSP_MEMORY(probe).AC_frq >= 2000)
		AC_tab_inc = AC_TAB_INC<<1;   // LockIn-Ref on 2343.75Hz
	else if (DSP_MEMORY(probe).AC_frq >= 1000)
		AC_tab_inc = AC_TAB_INC;      // LockIn-Ref on 1171.88Hz
	else if (DSP_MEMORY(probe).AC_frq >= 500)
		AC_tab_inc = AC_TAB_INC>>1;   // LockIn-Ref on  585.94Hz
	else    AC_tab_inc = 1;               // LockIn-Ref on  292.97Hz

	PRB_norm2 = SPECT_SIN_LEN_P2 + (30-_NORM32 (DSP_MEMORY(probe).AC_nAve)) - (30-_NORM32 (AC_tab_inc)) + DSP_MEMORY(probe).lockin_shr_corrsum; // 9 + #bits in nAve
	DSP_MEMORY(analog).debug[0] = PRB_norm2;

	DSP_MEMORY(probe).state = target_mode;
	DSP_MEMORY(probe).start = 0;
	DSP_MEMORY(probe).stop  = 0;
}

// reset probe fifo control to buffer beginnning
#ifndef DSPEMU
#pragma CODE_SECTION(init_probe_fifo, ".text:slow")
#endif

void init_probe_fifo (){
	DSP_MEMORY(probe_datafifo).buffer_w = (DSP_INT16_P) DSP_MEMORY(probe_datafifo).buffer_base;
	DSP_MEMORY(probe_datafifo).buffer_l = (DSP_INT32_P) DSP_MEMORY(probe_datafifo).buffer_base;
	DSP_MEMORY(probe_datafifo).w_position = 0;
}

/* calc of f_dx,... and num_steps by host! */
#ifndef DSPEMU
#pragma CODE_SECTION(init_probe, ".text:slow")
#endif

void init_probe (){

	probe_store ();

	// now starting...
	DSP_MEMORY(probe).start = 0;
	DSP_MEMORY(probe).stop  = 0;
	DSP_MEMORY(probe).vector = NULL;
	DSP_MEMORY(probe).time = 0;
	DSP_MEMORY(probe).lix= 0;
	DSP_MEMORY(probe).gpio_setting = 0;
        PRB_Trk_state = -1;
        PRB_Trk_ref   = 0L;

	if (DSP_MEMORY(probe).state < PROBE_RUN_LOCKIN_FREE){ // don't touch if free running.
		DSP_MEMORY(probe).state = PROBE_NO_LOCKIN; // disable, re-init
		PRB_lockin_restart_flg = 1;
		init_lockin (PROBE_RUN_LOCKIN_PROBE); // auto: on probe only level
	}

	// calc data normalization factor
//	PRB_ks_normfac  = PRB_ACMult*2.*MATH_PI/(PRB_nAve*SPECT_SIN_LEN*LOCKIN_ILN);
//	PRB_avg_normfac = PRB_ACMult/(PRB_nAve*SPECT_SIN_LEN*LOCKIN_ILN);

	// Probe data preample with full start position vector

	next_section ();
	clear_probe_data_srcs ();
	probe_append_header_and_positionvector ();

	DSP_MEMORY(probe).pflg  = 1; // enable probe
}

#ifndef DSPEMU
#pragma CODE_SECTION(stop_lockin, ".text:slow")
#endif

void stop_lockin (int level){
	if (DSP_MEMORY(probe).state == level) // don't touch if other level is running.
		DSP_MEMORY(probe).state = PROBE_NO_LOCKIN;
}

#ifndef DSPEMU
#pragma CODE_SECTION(stop_probe, ".text:slow")
#endif

void stop_probe (){
	// finish probe
	stop_lockin (PROBE_RUN_LOCKIN_PROBE); // don't touch if free ro scan level is running.
	DSP_MEMORY(probe).start = 0;
	DSP_MEMORY(probe).stop  = 0;
	DSP_MEMORY(probe).pflg = 0;
	DSP_MEMORY(probe).vector = NULL;

	// Probe Suffix with full end position vector[6]
	probe_append_header_and_positionvector ();

	probe_restore ();
}

void add_probe_vector (){
	// check for valid vector and for limiter active (freeze)
	if (!DSP_MEMORY(probe).vector || DSP_MEMORY(probe).lix) return;

// _SADD32 full differential situation vector to current situation, using saturation mode
	DSP_MEMORY(probe).Upos = _SADD32 (DSP_MEMORY(probe).Upos, DSP_MEMORY(probe).vector->f_du); // fixed f_du
	//DSP_MEMORY(scan).xyz_vec[i_X] = _SADD32 (DSP_MEMORY(scan).xyz_vec[i_X], DSP_MEMORY(probe).vector->f_dx);
	//DSP_MEMORY(scan).xyz_vec[i_Y] = _SADD32 (DSP_MEMORY(scan).xyz_vec[i_Y], DSP_MEMORY(probe).vector->f_dy);
	DSP_MEMORY(probe).Zpos = _SADD32 (DSP_MEMORY(probe).Zpos, DSP_MEMORY(probe).vector->f_dz); // fixed f_dz

	// mappable VP out components:
	*DSP_MEMORY(probe).prb_output[0] = _SADD32 (*DSP_MEMORY(probe).prb_output[0], DSP_MEMORY(probe).vector->f_dx); // "f_dx" mapped to output[0] destination
	*DSP_MEMORY(probe).prb_output[1] = _SADD32 (*DSP_MEMORY(probe).prb_output[1], DSP_MEMORY(probe).vector->f_dy); // "f_dy" mapped to output[1] destination
	*DSP_MEMORY(probe).prb_output[2] = _SADD32 (*DSP_MEMORY(probe).prb_output[2], DSP_MEMORY(probe).vector->f_dx0); // "f_dx0" mapped to output[2] destination
	*DSP_MEMORY(probe).prb_output[3] = _SADD32 (*DSP_MEMORY(probe).prb_output[3], DSP_MEMORY(probe).vector->f_dy0); // "f_dy0" mapped to output[3] destination


	//DSP_MEMORY(move).xyz_vec[i_X] = _SADD32 (DSP_MEMORY(move).xyz_vec[i_X], DSP_MEMORY(probe).vector->f_dx0);
	//DSP_MEMORY(move).xyz_vec[i_Y] = _SADD32 (DSP_MEMORY(move).xyz_vec[i_Y], DSP_MEMORY(probe).vector->f_dy0);
	//*** DSP_MEMORY(move).xyz_vec[i_Z] = _SADD32 (DSP_MEMORY(move).xyz_vec[i_Z], DSP_MEMORY(probe).vector->f_dz0);

	// reused for phase probe!
	DSP_MEMORY(probe).PRB_ACPhaseA32 = _SADD32 (DSP_MEMORY(probe).PRB_ACPhaseA32, DSP_MEMORY(probe).vector->f_dphi); // Q5 -- fixed phase mapping

	// compute new phases A and B
	if (DSP_MEMORY(probe).vector->f_dphi){
		PRB_ACPhIdxA  = _SSHL32 (_SMPY32 ( _SSHL32 (DSP_MEMORY(probe).PRB_ACPhaseA32, -16), PHASE_FACTOR_Q19), -15);
		PRB_ACPhIdxB  = PRB_ACPhIdxA + PRB_ACPhIdxdBA;
	}
}

void clear_probe_data_srcs (){
	PRB_AIC_data_sum[0] = PRB_AIC_data_sum[1] = PRB_AIC_data_sum[2] =
	PRB_AIC_data_sum[3] = PRB_AIC_data_sum[4] = PRB_AIC_data_sum[5] =
	PRB_AIC_data_sum[6] = PRB_AIC_data_sum[7] = PRB_AIC_data_sum[8] = 0;
	PRB_AIC_num_samples=DSP_MEMORY(probe).vector->dnx+1;
	
	VP_sec_int0 = VP_sec_int1 = 0L;
	VP_sec_count = 0;
}

void integrate_probe_data_srcs (){
#ifdef DSP_PROBE_AIC_AVG
	if (DSP_MEMORY(probe).vector->srcs & 0x01) // Zmonitor (AIC5 out)
		PRB_AIC_data_sum[8] += AIC_OUT(5); // ******!

	if (DSP_MEMORY(probe).vector->srcs & 0x10) // AIC0 <-- FBMix 0: I (current)
		PRB_AIC_data_sum[0] += DSP_MEMORY(feedback_mixer).FB_IN_processed[0]>>8; // AIC_IN(0);

	if (DSP_MEMORY(probe).vector->srcs & 0x20) // AIC1 <-- FBMix 1: dF (aux1)
		PRB_AIC_data_sum[1] += DSP_MEMORY(feedback_mixer).FB_IN_processed[1]>>8; // AIC_IN(1);

	if (DSP_MEMORY(probe).vector->srcs & 0x40) // AIC2 <-- FBMix 2: (aux2)
		PRB_AIC_data_sum[2] += DSP_MEMORY(feedback_mixer).FB_IN_processed[2]>>8; // AIC_IN(2);

	if (DSP_MEMORY(probe).vector->srcs & 0x80) // AIC3 <-- FBMix 3: (aux3)
		PRB_AIC_data_sum[3] += DSP_MEMORY(feedback_mixer).FB_IN_processed[3]>>8; // AIC_IN(3);

	if (DSP_MEMORY(probe).vector->srcs & 0x100) // AIC4
		PRB_AIC_data_sum[4] += AIC_IN(4);

	if (DSP_MEMORY(probe).vector->srcs & 0x200) // AIC5
		PRB_AIC_data_sum[5] += AIC_IN(5);

	if (DSP_MEMORY(probe).vector->srcs & 0x400) // AIC6
		PRB_AIC_data_sum[6] += AIC_IN(6);

	if (DSP_MEMORY(probe).vector->srcs & 0x800) // AIC7
		PRB_AIC_data_sum[7] += AIC_IN(7);
#endif
}

#ifndef DSPEMU
#pragma CODE_SECTION(store_probe_data_srcs, ".text:slow")
#endif

void store_probe_data_srcs ()
{
	int w=0;
 
	// get probe data srcs (word)

#ifdef DSP_PROBE_AIC_AVG
	if (DSP_MEMORY(probe).vector->options & VP_AIC_INTEGRATE){

		if (DSP_MEMORY(probe).vector->srcs & 0x01){ // Zmonitor (AIC5 -->)
			*DSP_MEMORY(probe_datafifo).buffer_w++ = (int)(PRB_AIC_data_sum[8]/PRB_AIC_num_samples); ++w;
		}
		if (DSP_MEMORY(probe).vector->srcs & 0x02) // Umonitor (AIC6 -->)
			*DSP_MEMORY(probe_datafifo).buffer_w++ = AIC_OUT(6), ++w;
		if (DSP_MEMORY(probe).vector->srcs & 0x10){ // AIC0 <-- FBMix 0: I (current)
			*DSP_MEMORY(probe_datafifo).buffer_w++ = (int)(PRB_AIC_data_sum[0]/PRB_AIC_num_samples); ++w;
		}
		if (DSP_MEMORY(probe).vector->srcs & 0x20){ // AIC1 <-- FBMix 1: dF
			*DSP_MEMORY(probe_datafifo).buffer_w++ = (int)(PRB_AIC_data_sum[1]/PRB_AIC_num_samples); ++w;
		}
		if (DSP_MEMORY(probe).vector->srcs & 0x40){ // AIC2 <-- FBMix 2
			*DSP_MEMORY(probe_datafifo).buffer_w++ = (int)(PRB_AIC_data_sum[2]/PRB_AIC_num_samples); ++w;
		}
		if (DSP_MEMORY(probe).vector->srcs & 0x80){ // AIC3 <-- FBMix 3
			*DSP_MEMORY(probe_datafifo).buffer_w++ = (int)(PRB_AIC_data_sum[3]/PRB_AIC_num_samples); ++w;
		}
		if (DSP_MEMORY(probe).vector->srcs & 0x100){ // AIC4
			*DSP_MEMORY(probe_datafifo).buffer_w++ = (int)(PRB_AIC_data_sum[4]/PRB_AIC_num_samples); ++w;
		}
		if (DSP_MEMORY(probe).vector->srcs & 0x200){ // AIC5
			*DSP_MEMORY(probe_datafifo).buffer_w++ = (int)(PRB_AIC_data_sum[5]/PRB_AIC_num_samples); ++w;
		}
		if (DSP_MEMORY(probe).vector->srcs & 0x400){ // AIC6
			*DSP_MEMORY(probe_datafifo).buffer_w++ = (int)(PRB_AIC_data_sum[6]/PRB_AIC_num_samples); ++w;
		}
		if (DSP_MEMORY(probe).vector->srcs & 0x800){ // AIC7
			*DSP_MEMORY(probe_datafifo).buffer_w++ = (int)(PRB_AIC_data_sum[7]/PRB_AIC_num_samples); ++w;
		}
	} else {

#endif // ------------------------------------------------------------

		if (DSP_MEMORY(probe).vector->srcs & 0x01) // Zmonitor (AIC5 -->)
			*DSP_MEMORY(probe_datafifo).buffer_w++ = AIC_OUT(5), ++w;
		if (DSP_MEMORY(probe).vector->srcs & 0x02) // Umonitor (AIC6 -->)
			*DSP_MEMORY(probe_datafifo).buffer_w++ = AIC_OUT(6), ++w;
		if (DSP_MEMORY(probe).vector->srcs & 0x10) // AIC0
			*DSP_MEMORY(probe_datafifo).buffer_w++ = DSP_MEMORY(feedback_mixer).FB_IN_processed[0]>>8, ++w; // AIC_IN(0), ++w;

		if (DSP_MEMORY(probe).vector->srcs & 0x20) // AIC1
			*DSP_MEMORY(probe_datafifo).buffer_w++ = DSP_MEMORY(feedback_mixer).FB_IN_processed[1]>>8, ++w; // AIC_IN(1), ++w;

		if (DSP_MEMORY(probe).vector->srcs & 0x40) // AIC2
			*DSP_MEMORY(probe_datafifo).buffer_w++ = DSP_MEMORY(feedback_mixer).FB_IN_processed[2]>>8, ++w; //  AIC_IN(2), ++w;

		if (DSP_MEMORY(probe).vector->srcs & 0x80) // AIC3
			*DSP_MEMORY(probe_datafifo).buffer_w++ = DSP_MEMORY(feedback_mixer).FB_IN_processed[3]>>8, ++w; // AIC_IN(3), ++w;

		if (DSP_MEMORY(probe).vector->srcs & 0x100) // AIC4
			*DSP_MEMORY(probe_datafifo).buffer_w++ = AIC_IN(4), ++w;

		if (DSP_MEMORY(probe).vector->srcs & 0x200) // AIC5
			*DSP_MEMORY(probe_datafifo).buffer_w++ = AIC_IN(5), ++w;

		if (DSP_MEMORY(probe).vector->srcs & 0x400) // AIC6
			*DSP_MEMORY(probe_datafifo).buffer_w++ = AIC_IN(6), ++w;

		if (DSP_MEMORY(probe).vector->srcs & 0x800) // AIC7
			*DSP_MEMORY(probe_datafifo).buffer_w++ = AIC_IN(7), ++w;
#ifdef DSP_PROBE_AIC_AVG
	}
#endif
	// adjust long ptr
	if (w&1) ++w, DSP_MEMORY(probe_datafifo).buffer_w++;

	DSP_MEMORY(probe_datafifo).w_position += w;

	w >>= 1;
	DSP_MEMORY(probe_datafifo).buffer_l += w;

	// read and buffer (for Rate Meter, gatetime not observed)
	DSP_MEMORY(CR_generic_io).count_0 = DSP_MEMORY(analog).counter[0];

	// LockIn data (long)
	if (DSP_MEMORY(probe).vector->srcs & 0x00008) // "--" LockIn_0
		*DSP_MEMORY(probe_datafifo).buffer_l++ = DSP_MEMORY(probe).LockIn_0A, DSP_MEMORY(probe_datafifo).buffer_w += 2,
			DSP_MEMORY(probe_datafifo).w_position += 2;
	    
	if (DSP_MEMORY(probe).vector->srcs & 0x01000) // "C1" LockIn_1st - phase A
		*DSP_MEMORY(probe_datafifo).buffer_l++ = DSP_MEMORY(probe).LockIn_1stA, DSP_MEMORY(probe_datafifo).buffer_w += 2,
			DSP_MEMORY(probe_datafifo).w_position += 2;

	if (DSP_MEMORY(probe).vector->srcs & 0x02000) // MUX: SRC-LOOKUP[0]
		*DSP_MEMORY(probe_datafifo).buffer_l++ = *DSP_MEMORY(probe).src_input[0], DSP_MEMORY(probe_datafifo).buffer_w += 2,
			DSP_MEMORY(probe_datafifo).w_position += 2;
	    
	if (DSP_MEMORY(probe).vector->srcs & 0x04000) // MUX: SRC-LOOKUP[1] 
		*DSP_MEMORY(probe_datafifo).buffer_l++ = *DSP_MEMORY(probe).src_input[1], DSP_MEMORY(probe_datafifo).buffer_w += 2,
			DSP_MEMORY(probe_datafifo).w_position += 2;

	if (DSP_MEMORY(probe).vector->srcs & 0x08000) // MUX: SRC-LOOKUP[2] 
		*DSP_MEMORY(probe_datafifo).buffer_l++ = *DSP_MEMORY(probe).src_input[2], DSP_MEMORY(probe_datafifo).buffer_w += 2,
			DSP_MEMORY(probe_datafifo).w_position += 2;
	    
	if (DSP_MEMORY(probe).vector->srcs & 0x00004){ // MUX: SRC-LOOKUP[3] 
		*DSP_MEMORY(probe_datafifo).buffer_l++ = *DSP_MEMORY(probe).src_input[3], DSP_MEMORY(probe_datafifo).buffer_w += 2,
			DSP_MEMORY(probe_datafifo).w_position += 2;
	}

	// reset counters only if requested
	if (DSP_MEMORY(probe).vector->options & VP_RESET_COUNTER_0)
		DSP_MEMORY(analog).counter[0] = 0;
	if (DSP_MEMORY(probe).vector->options & VP_RESET_COUNTER_1)
		DSP_MEMORY(analog).counter[1] = 0;
	    
	VP_sec_int0 += *DSP_MEMORY(probe).src_input[0];
	VP_sec_int1 += *DSP_MEMORY(probe).src_input[1];
	VP_sec_count++;

	// check free buffer range!!
	if (DSP_MEMORY(probe_datafifo).w_position > (EXTERN_PROBEDATAFIFO_LENGTH - EXTERN_PROBEDATA_MAX_LEFT)){
		// loop-marking
		*DSP_MEMORY(probe_datafifo).buffer_l++ = 0;
		*DSP_MEMORY(probe_datafifo).buffer_l++ = -1;
		*DSP_MEMORY(probe_datafifo).buffer_l++ = 0;
		// loop to fifo start!
		DSP_MEMORY(probe_datafifo).buffer_w = DSP_MEMORY(probe_datafifo).buffer_base;
		DSP_MEMORY(probe_datafifo).buffer_l = (DSP_INT32_P) DSP_MEMORY(probe_datafifo).buffer_base;
		DSP_MEMORY(probe_datafifo).w_position = 0;
	}
}

#ifndef DSPEMU
#pragma CODE_SECTION(buffer_probe_section_end_data_srcs, ".text:slow")
#endif

void buffer_probe_section_end_data_srcs ()
{
	int i, is0;
	if (PRB_section_count < 10){
		is0 = PRB_section_count << 3;
		for (i=0; i<4; ++i)
			VP_sec_end_buffer[is0+i] = *DSP_MEMORY(probe).src_input[i];
		VP_sec_end_buffer[is0+4] = DSP_MEMORY(analog).counter[0];
		VP_sec_end_buffer[is0+5] = DSP_MEMORY(analog).counter[1];
		VP_sec_end_buffer[is0+6] = VP_sec_int0/VP_sec_count;
		VP_sec_end_buffer[is0+7] = VP_sec_int1/VP_sec_count;
	}
	// reset counters forced at end of section
	DSP_MEMORY(analog).counter[0] = 0;
	DSP_MEMORY(analog).counter[1] = 0;
	
	VP_sec_int0 = VP_sec_int1 = 0L;
	VP_sec_count = 0;
}

// mini probe program flow interpreter kernel
#ifndef DSPEMU
#pragma CODE_SECTION(next_section, ".text:slow")
#endif

void next_section (){
	GPIO_subsample = 0;

        if (! DSP_MEMORY(probe).vector){ // initialize ?
                DSP_MEMORY(probe).vector = DSP_MEMORY(probe).vector_head;
                PRB_section_count = 0; // init PVC
//              if (DSP_MEMORY(probe).vector < (DSP_INT)(&prbdf)) // error, cancel probe now.
//                      stop_probe ();
        }
        else{
                if (!DSP_MEMORY(probe).vector->ptr_final){ // end Vector program?
                        stop_probe ();
                        return;
                }
		buffer_probe_section_end_data_srcs (); // store end of section data into matrix buffer -- used for area scan data transfer

#ifdef DSP_PROBE_VECTOR_PROGRAM // full program control capability
                if (DSP_MEMORY(probe).vector->i > 0){ // do next loop?
                        --DSP_MEMORY(probe).vector->i; // decrement loop counter
                        if (DSP_MEMORY(probe).vector->ptr_next){ // loop or branch to next
                                PRB_section_count += DSP_MEMORY(probe).vector->ptr_next; // adjust PVC (section "count" -- better section-index)
                                DSP_MEMORY(probe).vector += DSP_MEMORY(probe).vector->ptr_next; // next vector to loop
                        }else
                                stop_probe (); // error, no valid next vector: stop_probe now
                }
                else{
			DSP_MEMORY(probe).vector->i = DSP_MEMORY(probe).vector->repetitions; // loop done, reload loop counter for next time

			if (DSP_MEMORY(probe).vector->options & VP_LIMITER && (DSP_MEMORY(probe).vector->options & (VP_LIMITER_MODE)) != 0 && DSP_MEMORY(probe).lix > 0){
				DSP_MEMORY(probe).lix = 0;
                                DSP_MEMORY(probe).vector += DSP_MEMORY(probe).vector->ptr_final; // go to limiter abort vector
			} else {
//				DSP_MEMORY(probe).vector += DSP_MEMORY(probe).vector->ptr_final; // next vector -- possible...
				++DSP_MEMORY(probe).vector; // increment programm (vector) counter -- just increment!
			}
			
			++PRB_section_count;
                }
#else // simple
                ++DSP_MEMORY(probe).vector; // increment programm (vector) counter
                ++PRB_section_count;
#endif
        }
        DSP_MEMORY(probe).ix = DSP_MEMORY(probe).vector->n; // load total steps per section = # vec to add
        DSP_MEMORY(probe).iix = DSP_MEMORY(probe).vector->dnx; // do dnx steps to take data until next point!

#if 0 // NOT HERE ANY MORE!!
        if (DSP_MEMORY(probe).vector->srcs & 0x0004){ // if Counter channel requested, restart counter/timer now
                DSP_MEMORY(analog).counter[0] = 0;
                DSP_MEMORY(analog).counter[1] = 0;
        }
#endif
}

// manage conditional vector tracking mode -- atom/feature tracking
#ifndef DSPEMU
#pragma CODE_SECTION(next_track_vector, ".text:slow")
#endif

void next_track_vector(){
        int value;
        if (DSP_MEMORY(probe).vector->options & VP_TRACK_REF){ // take reference?
                PRB_Trk_state = 0;
                PRB_Trk_ie    = 0;
                PRB_Trk_i     = 999;
                PRB_Trk_ref   = *DSP_MEMORY(probe).tracker_input;
        } else if ((DSP_MEMORY(probe).vector->options & VP_TRACK_FIN) || (PRB_Trk_i <= PRB_Trk_ie )){ // follow to max/min
                if (PRB_Trk_i == 999){ // done with mov to fin now
                        ++PRB_Trk_state;
                        if (PRB_Trk_ie > 0){ // start move to condition match pos
                                PRB_Trk_i = 0;
                                DSP_MEMORY(probe).vector -= PRB_Trk_state; // skip back to v1 of TRACK vectors
                                PRB_section_count -= PRB_Trk_state; // skip back to v1 of TRACK vectors
                        } else { // done, no move, keep watching!
//                              PRB_Trk_i     = 999; // it's still this!
                                PRB_Trk_state = -1;
                                PRB_Trk_ref   = 0L;
                                clear_probe_data_srcs ();
                                probe_append_header_and_positionvector ();
                        }
                } else if (PRB_Trk_i == PRB_Trk_ie){ // done!
                        DSP_MEMORY(probe).vector += PRB_Trk_state - PRB_Trk_ie; // skip forward to end of track sequence
                        PRB_section_count += PRB_Trk_state - PRB_Trk_ie; // skip forward to end of track sequence
                        PRB_Trk_i     = 999;
                        PRB_Trk_state = -1;
                        PRB_Trk_ref   = 0L;
                        clear_probe_data_srcs ();
                        probe_append_header_and_positionvector ();
                }
                ++PRB_Trk_i;
        } else if (DSP_MEMORY(probe).vector->options & (VP_TRACK_UP | VP_TRACK_DN)){
                ++PRB_Trk_state;
                value = *DSP_MEMORY(probe).tracker_input;
                if (DSP_MEMORY(probe).vector->options & VP_TRACK_UP){
                        if (value > PRB_Trk_ref){
                                PRB_Trk_ref = value;
                                PRB_Trk_ie  = PRB_Trk_state;
                        }
                } else { // inplies VP_TRACK_DN !!
                        if (value < PRB_Trk_ref){
                                PRB_Trk_ref = value;
                                PRB_Trk_ie  = PRB_Trk_state;
                        }
                }
        }
        ++DSP_MEMORY(probe).vector; // increment programm (vector) counter to go to next TRACK check position
        ++PRB_section_count; // next
        DSP_MEMORY(probe).ix = DSP_MEMORY(probe).vector->n; // load total steps per section = # vec to add
        DSP_MEMORY(probe).iix = DSP_MEMORY(probe).vector->dnx; // do dnx steps to take data until next point!
}

// trigger condition evaluation
int wait_for_trigger (){
	if (DSP_MEMORY(probe).vector->options == (VP_TRIGGER_P | VP_TRIGGER_N)) // logic
		return ((*DSP_MEMORY(probe).trigger_input) & ((DSP_MEMORY(probe).vector->options & VP_GPIO_MSK) >> 16)) ? 0:1; // usually tigger_input would point to DSP_MEMORY(probe).gpio_data for this GPIO/logic trigger
	else if (DSP_MEMORY(probe).vector->options & VP_TRIGGER_P){ // positive
		return (*DSP_MEMORY(probe).trigger_input > 0) ? 0:1;
	} else { // VP_TRIGGER_N -> negative 
		return (*DSP_MEMORY(probe).trigger_input < 0) ? 0:1;
	}
}

// run a probe section step:
// decrement and check counter for zero, then initiate next section (...add header info)
// decrement skip data counter, check for zero then
//  -acquire data of all srcs, store away
//  -update current count, restore skip data counter

void run_one_time_step(){
	// read GPIO if requested (CPU time costy!!!)
	if (DSP_MEMORY(probe).vector->options & VP_GPIO_READ){
		if (!GPIO_subsample--){
			GPIO_subsample = DSP_MEMORY(CR_generic_io).gpio_subsample;
			WR_GPIO (GPIO_Data_0, &DSP_MEMORY(CR_generic_io).gpio_data_in, 0);
			DSP_MEMORY(probe).gpio_data = DSP_MEMORY(CR_generic_io).gpio_data_in; // mirror data in 32bit signal
		}
	}

	// unsatisfied trigger condition will also HOLD VP as of now!
	if (DSP_MEMORY(probe).vector->options & (VP_TRIGGER_P|VP_TRIGGER_N))
		if (wait_for_trigger ())
			return;

        integrate_probe_data_srcs ();

        if (! DSP_MEMORY(probe).iix-- || DSP_MEMORY(probe).lix){
                if (DSP_MEMORY(probe).ix--){
                        store_probe_data_srcs ();
                        if (!DSP_MEMORY(probe).ix){
                                // Special VP Track Mode Vector Set?
                                if (DSP_MEMORY(probe).vector->options & (VP_TRACK_UP | VP_TRACK_DN | VP_TRACK_REF | VP_TRACK_FIN)){
                                        next_track_vector ();

                                } else { // run regular VP program mode
                                        next_section ();
                                        clear_probe_data_srcs ();
                                        probe_append_header_and_positionvector ();
                                }
			}
                }
		// Limiter active?
		if (DSP_MEMORY(probe).vector->options & VP_LIMITER){	
			if (DSP_MEMORY(probe).lix > 0) { ++DSP_MEMORY(probe).lix;  goto run_one_time_step_1; }
			if (DSP_MEMORY(probe).vector->options & VP_LIMITER_UP)
				if (*DSP_MEMORY(probe).limiter_input > *DSP_MEMORY(probe).limiter_updn[0]){
					++DSP_MEMORY(probe).lix;
					goto run_one_time_step_1;
				}
			if (DSP_MEMORY(probe).vector->options & VP_LIMITER_DN)
				if (*DSP_MEMORY(probe).limiter_input < *DSP_MEMORY(probe).limiter_updn[1]){
					++DSP_MEMORY(probe).lix;
					goto run_one_time_step_1;
				}
			goto limiter_continue;
		run_one_time_step_1:
			if ((DSP_MEMORY(probe).vector->options & VP_LIMITER_MODE) != 0) {
				DSP_MEMORY(probe).lix = 1; // cancel any "undo" mode, use final vector in next section
				DSP_MEMORY(probe).vector->i = 0; // exit any loop
			}
		limiter_continue:
			;
		} else if (DSP_MEMORY(probe).lix > 0) --DSP_MEMORY(probe).lix;
		 
                DSP_MEMORY(probe).iix = DSP_MEMORY(probe).vector->dnx; // load steps inbetween "data aq" points
                clear_probe_data_srcs ();
        }
}

#define LOCKIN_ON (DSP_MEMORY(probe).vector->srcs & 0x3000)
void run_probe (){

	if (LOCKIN_ON){
		if (PRB_lockin_restart_flg){
			DSP_MEMORY(probe).AC_ix = -PRB_SE_DELAY;
			PRB_lockin_restart_flg = 0;
		}

		// wait until starting procedure (korrelation integration time) finished
		if(DSP_MEMORY(probe).AC_ix >= 0){
			run_one_time_step ();
			add_probe_vector ();
		}
//		run_lockin (); --> dataprocess controlled now

	} else {
		PRB_lockin_restart_flg = 1;

		run_one_time_step ();
		add_probe_vector ();
	}

// increment probe time
	++DSP_MEMORY(probe).time; 
}

// digital LockIn, computes 0., 1st, 2nd order Amplitudes, generates reference/bias sweep, phase correction for 1st and 2nd order
void run_lockin (){
	int i;

        // LockIn Korrel func:  Amp = 2pi/nN Sum_{i=0..nN} I(i2pi/nN) sin(i2pi/nN + Delta)
        // tab used: PRB_sintab[i] = sin(i*2.*MATH_PI/SPECT_SIN_LEN);

	DSP_INT64 UinA = *DSP_MEMORY(probe).LockIn_input[0]; // mapped DSP_MEMORY(feedback_mixer).FB_IN_processed[0..4]  Q23.8
	DSP_INT64 UinB = *DSP_MEMORY(probe).LockIn_input[1]; // mapped DSP_MEMORY(feedback_mixer).FB_IN_processed[0..4]  Q23.8
	//	DSP_INT64 Uin = DSP_MEMORY(feedback_mixer).FB_IN_processed[0]; // Q23.8
	// #define Uin         AIC_IN(0)                // LockIn Data Signal 

        /* read DAC 0 (fixed to this (may be changed) channel for LockIn)
         * summ up avgerage
         * summ up korrelation product to appropriate korrel sum interleave buffer 
         */

//	PRB_data_sum = _SADD32 (PRB_data_sum, Uin);
	PRB_data_sumA += UinA; // 32 -- s15.16  Sum_N=SPECT_SIN_LEN x nAve
	PRB_data_sumB += UinB;

//	PRB_korrel_sum1stA[PRB_ki] = _SADD32 (PRB_korrel_sum1stA[PRB_ki], _SSHL32 ( _SMPY32 (PRB_ref1stA, Uin), -16));
//	PRB_korrel_sum1stB[PRB_ki] = _SADD32 (PRB_korrel_sum1stB[PRB_ki], _SSHL32 ( _SMPY32 (PRB_ref1stB, Uin), -16));
//                                 += (..)>>16;
//      int_0..2pi dx 1/2 (1-cos (2x)) = pi
	PRB_korrel_sum1stA[PRB_ki] += ((DSP_INT64)PRB_ref1stA * (DSP_INT64)UinA) >> DSP_MEMORY(probe).lockin_shr_corrprod; //[=16] 16 + 23|32 - 16  //** Qs15.16 * Qs23.8 = Qs38
	PRB_korrel_sum1stB[PRB_ki] += ((DSP_INT64)PRB_ref1stB * (DSP_INT64)UinB) >> DSP_MEMORY(probe).lockin_shr_corrprod;

//	PRB_korrel_sum1st[PRB_ki] = _SMAC (PRB_korrel_sum1st[PRB_ki], PRB_ref1st, Uin);

//	PRB_korrel_sum2ndA[PRB_ki] = _SADD32 (PRB_korrel_sum2ndA[PRB_ki], _SSHL32 ( _SMPY32 (PRB_ref2ndA, Uin), -16));
//	PRB_korrel_sum2ndB[PRB_ki] = _SADD32 (PRB_korrel_sum2ndB[PRB_ki], _SSHL32 ( _SMPY32 (PRB_ref2ndB, Uin), -16));

	PRB_korrel_sum2ndA[PRB_ki] += ((DSP_INT64)PRB_ref2ndA * (DSP_INT64)UinA) >> DSP_MEMORY(probe).lockin_shr_corrprod;
	PRB_korrel_sum2ndB[PRB_ki] += ((DSP_INT64)PRB_ref2ndB * (DSP_INT64)UinB) >> DSP_MEMORY(probe).lockin_shr_corrprod;
                
	// average periods elapsed?
        if (PRB_AveCount >= DSP_MEMORY(probe).AC_nAve){
                PRB_AveCount = 0;
                PRB_LockIn_1stA = PRB_korrel_sum1stA[0]; 
                PRB_LockIn_1stB = PRB_korrel_sum1stB[0]; 
                PRB_LockIn_2ndA = PRB_korrel_sum2ndA[0]; 
                PRB_LockIn_2ndB = PRB_korrel_sum2ndB[0]; 

		// build korrelation sum
                for (i = 1; i < LOCKIN_ILN; ++i){
//                        DSP_MEMORY(probe).LockIn_1stA = _SADD32 (DSP_MEMORY(probe).LockIn_1stA, PRB_korrel_sum1stA[i]);
//                        DSP_MEMORY(probe).LockIn_1stB = _SADD32 (DSP_MEMORY(probe).LockIn_1stB, PRB_korrel_sum1stB[i]);
//                        DSP_MEMORY(probe).LockIn_2ndA = _SADD32 (DSP_MEMORY(probe).LockIn_2ndA, PRB_korrel_sum2ndA[i]);
//                        DSP_MEMORY(probe).LockIn_2ndB = _SADD32 (DSP_MEMORY(probe).LockIn_2ndB, PRB_korrel_sum2ndB[i]);
                        PRB_LockIn_1stA += PRB_korrel_sum1stA[i];
                        PRB_LockIn_1stB += PRB_korrel_sum1stB[i];
                        PRB_LockIn_2ndA += PRB_korrel_sum2ndA[i];
                        PRB_LockIn_2ndB += PRB_korrel_sum2ndB[i];
		}

		// data is ready now: DSP_MEMORY(probe).LockIn_1st, _2nd, _0
                if(DSP_MEMORY(probe).AC_ix >= 0){
			DSP_MEMORY(probe).LockIn_0A   = PRB_data_sumA >> PRB_norm2;
			DSP_MEMORY(probe).LockIn_0B   = PRB_data_sumB >> PRB_norm2;
		        DSP_MEMORY(probe).LockIn_1stA = PRB_korrel_sum1stA[0] >> PRB_norm2; // [8]  23|32 xN -> 16 xN 
		        DSP_MEMORY(probe).LockIn_1stB = PRB_korrel_sum1stB[0] >> PRB_norm2; 
			DSP_MEMORY(probe).LockIn_2ndA = PRB_korrel_sum2ndA[0] >> PRB_norm2; 
			DSP_MEMORY(probe).LockIn_2ndB = PRB_korrel_sum2ndB[0] >> PRB_norm2;
		}

                ++DSP_MEMORY(probe).AC_ix;

                if (++PRB_ki >= LOCKIN_ILN)
                        PRB_ki = 0;

		// reset korrelation and average sums
                PRB_korrel_sum1stA[PRB_ki] = 0L;
                PRB_korrel_sum1stB[PRB_ki] = 0L;
                PRB_korrel_sum2ndA[PRB_ki] = 0L;
                PRB_korrel_sum2ndB[PRB_ki] = 0L;
		PRB_data_sumA = 0;
		PRB_data_sumB = 0;
        }

	PRB_modindex += AC_tab_inc;
        if (PRB_modindex >= SPECT_SIN_LEN){
                PRB_modindex=0;
                ++PRB_AveCount;
        }

        DSP_MEMORY(probe).LockIn_ref = DSP_MEMORY(probe).LockInRefSinTabA[PRB_modindex];

        // correct phases
//	PRB_ref1stA = PRB_sinreftabA[(PRB_modindex + SPECT_SIN_LEN-PIPE_LEN*AC_tab_inc + PRB_ACPhIdxA) & SPECT_SIN_LEN_MSK];
	PRB_ref1stA = DSP_MEMORY(probe).LockInRefSinTabA[(PRB_modindex + SPECT_SIN_LEN-PIPE_LEN*AC_tab_inc + PRB_ACPhIdxA) & SPECT_SIN_LEN_MSK];
	PRB_ref1stB = DSP_MEMORY(probe).LockInRefSinTabB[(PRB_modindex + SPECT_SIN_LEN-PIPE_LEN*AC_tab_inc + PRB_ACPhIdxB) & SPECT_SIN_LEN_MSK];
	PRB_ref2ndA = DSP_MEMORY(probe).LockInRefSinTabA[((PRB_modindex<<1) + SPECT_SIN_LEN-PIPE_LEN*AC_tab_inc + PRB_ACPhIdxA) & SPECT_SIN_LEN_MSK];
	PRB_ref2ndB = DSP_MEMORY(probe).LockInRefSinTabB[((PRB_modindex<<1) + SPECT_SIN_LEN-PIPE_LEN*AC_tab_inc + PRB_ACPhIdxB) & SPECT_SIN_LEN_MSK];
}
