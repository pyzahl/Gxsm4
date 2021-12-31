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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

#include "FB_spm_dataexchange.h"
#include "dataprocess.h"

/* compensate for X AIC pipe delay */
#define NULL ((void *) 0)
#define PIPE_LEN (23)

extern SPM_STATEMACHINE state;
extern SPM_PI_FEEDBACK  feedback;
extern ANALOG_VALUES    analog;
extern PROBE            probe;
extern DATA_FIFO_EXTERN probe_datafifo;
extern MOVE_OFFSET      move;
extern AREA_SCAN        scan;
extern CR_GENERIC_IO    CR_generic_io;

ioport   unsigned int Port0;  /*CPLD port*/
ioport   unsigned int Port1;  /*CPLD port*/
ioport   unsigned int Port2;  /*CPLD port*/
ioport   unsigned int Port3;  /*CPLD port*/
ioport   unsigned int Port4;  /*CPLD port*/
ioport   unsigned int Port5;  /*CPLD port*/
ioport   unsigned int Port6;  /*CPLD port*/
ioport   unsigned int Port7;  /*CPLD port*/
ioport   unsigned int Port8;  /*CPLD port*/
ioport   unsigned int Port9;  /*CPLD port*/
ioport   unsigned int PortA;  /*CPLD port*/

#define PROBE_READY         0
#define PROBE_START_RAMP    1
#define PROBE_START_LOCKIN  2
#define PROBE_RUN_RAMP      10
#define PROBE_RUN_LOCKIN    11
#define PROBE_FINISH        20


#define SPECT_SIN_LEN_7    128                   /* sine table length, must be a power of 2 */
// #define SPECT_SIN_LEN_9    512                   /* sine table length, must be a power of 2 */
// #define SPECT_SIN_LEN_10   1024                  /* sine table length, must be a power of 2 */

#ifdef  SPECT_SIN_LEN_7
#define SPECT_SIN_LEN      SPECT_SIN_LEN_7
#define PHASE_FACTOR_Q15   11651    /* (1<<15)*SPECT_SIN_LEN/360 */
#define PHASE_FACTOR_Q19   364      /* (1<<15)*SPECT_SIN_LEN/360/(1<<4) */
#define AC_TAB_INC         1

// calc: 
// pi=acos(0)*2; for(i=0; i<128; ++i) { if(i%8==0) printf("\n\t"); printf("%6d, ",int(40000.5+(2^15-1)*sin(2*pi*i/128.))-40000); }
int PRB_sintab[SPECT_SIN_LEN_7] = {
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
int PRB_sinreftabA[SPECT_SIN_LEN_7] = {
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
int PRB_sinreftabB[SPECT_SIN_LEN_7] = {
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
#define PHASE_FACTOR_Q15   46603    /* (1<<15)*SPECT_SIN_LEN/360 */
#define AC_TAB_INC         4

// pi=acos(0)*2; for(i=0; i<512; ++i) { if(i%8==0) printf("\n\t"); printf("%6d, ",int(40000.5+(2^15-1)*sin(2*pi*i/512.))-40000); }
int PRB_sintab[SPECT_SIN_LEN_9] = {
             0,    402,    804,   1206,   1608,   2009,   2410,   2811,
          3212,   3612,   4011,   4410,   4808,   5205,   5602,   5998,
          6393,   6786,   7179,   7571,   7962,   8351,   8739,   9126,
          9512,   9896,  10278,  10659,  11039,  11417,  11793,  12167,
         12539,  12910,  13279,  13645,  14010,  14372,  14732,  15090,
         15446,  15800,  16151,  16499,  16846,  17189,  17530,  17869,
         18204,  18537,  18868,  19195,  19519,  19841,  20159,  20475,
         20787,  21096,  21403,  21705,  22005,  22301,  22594,  22884,
         23170,  23452,  23731,  24007,  24279,  24547,  24811,  25072,
         25329,  25582,  25832,  26077,  26319,  26556,  26790,  27019,
         27245,  27466,  27683,  27896,  28105,  28310,  28510,  28706,
         28898,  29085,  29268,  29447,  29621,  29791,  29956,  30117,
         30273,  30424,  30571,  30714,  30852,  30985,  31113,  31237,
         31356,  31470,  31580,  31685,  31785,  31880,  31971,  32057,
         32137,  32213,  32285,  32351,  32412,  32469,  32521,  32567,
         32609,  32646,  32678,  32705,  32728,  32745,  32757,  32765,
         32767,  32765,  32757,  32745,  32728,  32705,  32678,  32646,
         32609,  32567,  32521,  32469,  32412,  32351,  32285,  32213,
         32137,  32057,  31971,  31880,  31785,  31685,  31580,  31470,
         31356,  31237,  31113,  30985,  30852,  30714,  30571,  30424,
         30273,  30117,  29956,  29791,  29621,  29447,  29268,  29085,
         28898,  28706,  28510,  28310,  28105,  27896,  27683,  27466,
         27245,  27019,  26790,  26556,  26319,  26077,  25832,  25582,
         25329,  25072,  24811,  24547,  24279,  24007,  23731,  23452,
         23170,  22884,  22594,  22301,  22005,  21705,  21403,  21096,
         20787,  20475,  20159,  19841,  19519,  19195,  18868,  18537,
         18204,  17869,  17530,  17189,  16846,  16499,  16151,  15800,
         15446,  15090,  14732,  14372,  14010,  13645,  13279,  12910,
         12539,  12167,  11793,  11417,  11039,  10659,  10278,   9896,
          9512,   9126,   8739,   8351,   7962,   7571,   7179,   6786,
          6393,   5998,   5602,   5205,   4808,   4410,   4011,   3612,
          3212,   2811,   2410,   2009,   1608,   1206,    804,    402,
             0,   -402,   -804,  -1206,  -1608,  -2009,  -2410,  -2811,
         -3212,  -3612,  -4011,  -4410,  -4808,  -5205,  -5602,  -5998,
         -6393,  -6786,  -7179,  -7571,  -7962,  -8351,  -8739,  -9126,
         -9512,  -9896, -10278, -10659, -11039, -11417, -11793, -12167,
        -12539, -12910, -13279, -13645, -14010, -14372, -14732, -15090,
        -15446, -15800, -16151, -16499, -16846, -17189, -17530, -17869,
        -18204, -18537, -18868, -19195, -19519, -19841, -20159, -20475,
        -20787, -21096, -21403, -21705, -22005, -22301, -22594, -22884,
        -23170, -23452, -23731, -24007, -24279, -24547, -24811, -25072,
        -25329, -25582, -25832, -26077, -26319, -26556, -26790, -27019,
        -27245, -27466, -27683, -27896, -28105, -28310, -28510, -28706,
        -28898, -29085, -29268, -29447, -29621, -29791, -29956, -30117,
        -30273, -30424, -30571, -30714, -30852, -30985, -31113, -31237,
        -31356, -31470, -31580, -31685, -31785, -31880, -31971, -32057,
        -32137, -32213, -32285, -32351, -32412, -32469, -32521, -32567,
        -32609, -32646, -32678, -32705, -32728, -32745, -32757, -32765,
        -32767, -32765, -32757, -32745, -32728, -32705, -32678, -32646,
        -32609, -32567, -32521, -32469, -32412, -32351, -32285, -32213,
        -32137, -32057, -31971, -31880, -31785, -31685, -31580, -31470,
        -31356, -31237, -31113, -30985, -30852, -30714, -30571, -30424,
        -30273, -30117, -29956, -29791, -29621, -29447, -29268, -29085,
        -28898, -28706, -28510, -28310, -28105, -27896, -27683, -27466,
        -27245, -27019, -26790, -26556, -26319, -26077, -25832, -25582,
        -25329, -25072, -24811, -24547, -24279, -24007, -23731, -23452,
        -23170, -22884, -22594, -22301, -22005, -21705, -21403, -21096,
        -20787, -20475, -20159, -19841, -19519, -19195, -18868, -18537,
        -18204, -17869, -17530, -17189, -16846, -16499, -16151, -15800,
        -15446, -15090, -14732, -14372, -14010, -13645, -13279, -12910,
        -12539, -12167, -11793, -11417, -11039, -10659, -10278,  -9896,
         -9512,  -9126,  -8739,  -8351,  -7962,  -7571,  -7179,  -6786,
         -6393,  -5998,  -5602,  -5205,  -4808,  -4410,  -4011,  -3612
};
#endif

#ifdef  SPECT_SIN_LEN_10
#define SPECT_SIN_LEN      SPECT_SIN_LEN_10
#define PHASE_FACTOR_Q15   93207    /* (1<<15)*SPECT_SIN_LEN/360 */
#define AC_TAB_INC         5

//  pi=acos(0)*2; for(i=0; i<1024; ++i) { if(i%8==0) printf("\n\t"); printf("%6d, ",int(40000.5+(2^15-1)*sin(2*pi*i/1024.))-40000); }
int PRB_sintab[SPECT_SIN_LEN] = {
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

int PRB_store_nx_init[3];
int PRB_store_nx_finish[3];
int PRB_store_nx;

int PRB_ki;
int PRB_ref1stA;
int PRB_ref2ndA;
int PRB_ref1stB;
int PRB_ref2ndB;
int PRB_AveCount;
int PRB_modindex;
int PRB_ACPhIdxA;
int PRB_ACPhIdxB;
int PRB_ACPhIdxdBA;
long PRB_ACPhaseA32;
long PRB_data_sum;
long PRB_korrel_sum1stA[LOCKIN_ILN];
long PRB_korrel_sum2ndA[LOCKIN_ILN];
long PRB_korrel_sum1stB[LOCKIN_ILN];
long PRB_korrel_sum2ndB[LOCKIN_ILN];

long PRB_AIC_data_sum[9];
int PRB_AIC_num_samples;

#if 0
// Hacking stuff

/*
Wird #OutChannels * #(MaxDelayFIFO-Out-MinDelayFIFO-Out) + #(InChannels + InData(DSP-Level)Channels) * #(MaxDelayFIFO-Out-MinDelayFIFO-Out + MaxDelayFIFO-In) Words benoetigen.

#OutChannels = #{ Xs, Ys, Zs, U, Phi?, XYZoffset?} = 5
#InChannels  = #{AIC0,1,2,3,4,5,  Umon, Zmon, Sec, LockIn0,1,2} = 12
MaxDelayFIFO-Out = 18    ... not 100% sure, have to check the AIC doc
MinDelayFIFO-Out = 2  .."""
MaxDelayFIFO-In = 18 ..""" (FIR on, 4x oversampling + 2)
MinDelayFIFO-in = 2 .. """ (FIR off)
*/

/* AIC DELAYS */
#define AIC_DELAY_IN0 18
#define AIC_DELAY_IN1 18
#define AIC_DELAY_IN2 18
#define AIC_DELAY_IN3 18
#define AIC_DELAY_IN4 18
#define AIC_DELAY_IN5 2

#define AIC_DELAY_OUT0 18
#define AIC_DELAY_OUT1 18
#define AIC_DELAY_OUT2 18
#define AIC_DELAY_OUT3 18
#define AIC_DELAY_OUT4 18
#define AIC_DELAY_OUT5 2
#define AIC_DELAY_OUT6 18
#define AIC_DELAY_OUT7 18

#define AIC_SYNC_LENGTH_IN  16
#define AIC_SYNC_LENGTH_OUT 16

int AIC_sync_fifo_in[6][AIC_SYNC_LENGTH_IN];
int AIC_sync_fifo_out[8][AIC_SYNC_LENGTH_OUT];
int AIC_sync_index;

#endif

int  PRB_section_count;

int PRB_lockin_restart_flg;

void clear_probe_data_srcs ();
void next_section ();

void init_probe_module(){
	probe.LockInRefSinTabA = (unsigned int)PRB_sinreftabA;
	probe.LockInRefSinLenA = SPECT_SIN_LEN;	
	probe.LockInRefSinTabB = (unsigned int)PRB_sinreftabB;
	probe.LockInRefSinLenB = SPECT_SIN_LEN;
}

void probe_store (){
}

void probe_restore (){
}

void probe_append_header_and_positionvector (){ // size: 14
	// Section header: [SRCS, N, TIME]_32 :: 6
	if (probe.vector){
		*probe_datafifo.buffer_l++ = probe.vector->srcs; 
		*probe_datafifo.buffer_l++ = probe.vector->n; 
	} else {
		*probe_datafifo.buffer_l++ = 0; 
		*probe_datafifo.buffer_l++ = 0; 
	}
	*probe_datafifo.buffer_l++ = probe.time; 
	probe_datafifo.buffer_w += 6; // skip 6 words == 3 long

	// Section start situation vector [XYZ_Offset, XYZ_Scan, Upos(bias), 0xffff]_16 :: 8
	*probe_datafifo.buffer_w++ = analog.x_offset;
	*probe_datafifo.buffer_w++ = analog.y_offset;
	*probe_datafifo.buffer_w++ = _lsshl (PRB_ACPhaseA32, -12); // Phase    // analog.z_offset;

	*probe_datafifo.buffer_w++ = analog.x_scan;
	*probe_datafifo.buffer_w++ = analog.y_scan;
	*probe_datafifo.buffer_w++ = iobuf.mout5;  // Z: feedback.z + Z_pos>>16;

	*probe_datafifo.buffer_w++ = iobuf.mout6;  // Bias: U_pos>>16;
	*probe_datafifo.buffer_w++ = PRB_section_count;
	probe_datafifo.buffer_l += 4; // skip 4 long == 8 words

	probe_datafifo.w_position += 6 + 6 + 2;
}


void init_lockin (){
	int j;
	if (probe.AC_amp > 0){
		probe.state = PROBE_START_LOCKIN;

		// init LockIn variables

		// calc pre-start pos to center integration window correct
		// PRB_xS -= PRB_dnx * LOCKIN_ILN/2;

		// adjust "probe.nx_init" -- fixme

		// init correlation sum buffer
		PRB_ki = 0;
		for (j = 0; j < LOCKIN_ILN; ++j){
			PRB_korrel_sum1stA[j] = 0;
			PRB_korrel_sum1stB[j] = 0;
			PRB_korrel_sum2ndA[j] = 0;
			PRB_korrel_sum2ndB[j] = 0;
		}
		PRB_ref1stA = 0;
		PRB_ref1stB = 0;
		PRB_ref2ndA = 0;
		PRB_ref2ndB = 0;
		PRB_AveCount = 0;
		PRB_modindex = 0;
		// calc phase in indices, "/" needs _idiv, so omit it here!
//				PRB_ACPhIclscdx  = (int)(probe.AC_phase*SPECT_SIN_LEN/360);
		PRB_ACPhIdxA   = _lsshl (_lsmpy (probe.AC_phaseA, PHASE_FACTOR_Q19), -15);
		PRB_ACPhIdxB   = _lsshl (_lsmpy (probe.AC_phaseB, PHASE_FACTOR_Q19), -15);
		PRB_ACPhIdxdBA = PRB_ACPhIdxB - PRB_ACPhIdxA;
		PRB_ACPhaseA32 = _lsshl (probe.AC_phaseA, 16);

		// AIC samples at 22000Hz, full Sintab length is 128, so need increments of
		// 1,2,4,8 for the following discrete frequencies
		if (probe.AC_frq > 1000)
			probe.AC_frq = 8*AC_TAB_INC;     // LockIn-Ref on 1375Hz
		else if (probe.AC_frq > 500)
			probe.AC_frq = 4*AC_TAB_INC;     // LockIn-Ref on 687.5Hz
		else if (probe.AC_frq > 250)
			probe.AC_frq = 2*AC_TAB_INC;     // LockIn-Ref on 343.75Hz
		else  	probe.AC_frq = 1*AC_TAB_INC;     // LockIn-Ref on 171.875Hz

	}
}

void init_lockin_for_bgjob (){
	init_lockin ();
}

// reset probe fifo control to buffer beginnning
void init_probe_fifo (){
	probe_datafifo.buffer_w = probe_datafifo.buffer_base;
	probe_datafifo.buffer_l = (DSP_LONG*) probe_datafifo.buffer_base;
	probe_datafifo.w_position = 0;
}

/* calc of f_dx,... and num_steps by host! */
void init_probe (){

	probe_store ();

	// now starting...
	probe.start = 0;
	probe.stop  = 0;
	probe.vector = NULL;
	probe.time = 0; 

	PRB_lockin_restart_flg = 1;

	init_lockin ();
//	probe.f_dx = _lsshl ( probe.f_dx, -8+probe.AC_frq); // smooth bias ramp compu for LockIn

	// calc data normalization factor
//	PRB_ks_normfac  = PRB_ACMult*2.*MATH_PI/(PRB_nAve*SPECT_SIN_LEN*LOCKIN_ILN);
//	PRB_avg_normfac = PRB_ACMult/(PRB_nAve*SPECT_SIN_LEN*LOCKIN_ILN);

	// Probe data preample with full start position vector

	next_section ();
	clear_probe_data_srcs ();
	probe_append_header_and_positionvector ();

	probe.pflg  = 1; // enable probe
}

void stop_probe (){
	// now stop
	probe.state = PROBE_READY;
	probe.start = 0;
	probe.stop  = 0;
	probe.pflg = 0;
	probe.vector = NULL;

	// Probe Suffix with full end position vector[6]
	probe_append_header_and_positionvector ();

	probe_restore ();
}

void add_probe_vector (){
	if (!probe.vector) return;

// _lsadd full differential situation vector to current situation, using saturation mode
	probe.Upos = _lsadd (probe.Upos, probe.vector->f_du);

	scan.Xpos = _lsadd (scan.Xpos, probe.vector->f_dx);
	scan.Ypos = _lsadd (scan.Ypos, probe.vector->f_dy);

	probe.Zpos = _lsadd (probe.Zpos, probe.vector->f_dz);

	move.XPos = _lsadd (move.XPos, probe.vector->f_dx0);
	move.YPos = _lsadd (move.YPos, probe.vector->f_dy0);
	//	move.ZPos = _lsadd (move.ZPos, probe.vector->f_dz0);

	// reused for phase probe!
	PRB_ACPhaseA32 = _lsadd (PRB_ACPhaseA32, probe.vector->f_dphi); // Q4

	// compute new phases A and B
	if (probe.vector->f_dphi){
		PRB_ACPhIdxA  = _lsshl (_lsmpy ( _lsshl (PRB_ACPhaseA32, -16), PHASE_FACTOR_Q19), -15);
		PRB_ACPhIdxB  = PRB_ACPhIdxA + PRB_ACPhIdxdBA;
	}
}

void clear_probe_data_srcs (){
	PRB_AIC_data_sum[0] = PRB_AIC_data_sum[1] = PRB_AIC_data_sum[2] =
	PRB_AIC_data_sum[3] = PRB_AIC_data_sum[4] = PRB_AIC_data_sum[5] =
	PRB_AIC_data_sum[6] = PRB_AIC_data_sum[7] = PRB_AIC_data_sum[8] = 0;
	PRB_AIC_num_samples=probe.vector->dnx;
}

// Dummy Delay
void dummy(){;}

// setup GateTime/mode and open gate
void setup_counter(){ // update count and restart
	PortA = CR_generic_io.gatetime; // set gate time counter value
	dummy ();
	Port4 = 1; // open hard gate (wired to In0)
}

// close Gate
void close_counter(){ // update count and restart
	Port4 = 0; // close hard gate (wired to In0)
}

// update GateTime/mode, reset SR-CoolRunner Counter and restart Timer
void restart_counter(){ // update count and restart
	Port9 = CR_generic_io.mode; // set timer mode (gate time range)
	dummy();
	Port9 = 0; // start timer and counting
}

// read SR-CoolRunner Counter and restart Timer
void update_count(){ // update count and restart
	union { unsigned int i[2]; unsigned long l; } cnt;
	int status;
	status = Port3;
	CR_generic_io.rw = status;
	dummy();
	if (status & 1){ // poll status, ready?
		cnt.i[1] = Port7;
		cnt.i[0] = Port8;
		CR_generic_io.count = cnt.l;
		++CR_generic_io.xtime;
		restart_counter();
	}
}

// mini probe program kernel
void next_section (){
	if (! probe.vector){ // initialize ?
		probe.vector = probe.vector_head;
		PRB_section_count = 0; // init PVC
		if (probe.vector < EXTERN_PROBE_VECTOR_HEAD_DEFAULT_P) // error, cancel probe now.
			stop_probe ();
	}
	else{
		if (probe.vector->srcs & 0x0004) // if Counter channel was requested, close gate
			close_counter();

		if (!probe.vector->ptr_final){ // end Vector program?
			stop_probe ();
			return;
		}
#ifdef DSP_PROBE_VECTOR_PROGRAM
		if (probe.vector->i > 0){ // do next loop?
			--probe.vector->i; // decrement loop counter
			if (probe.vector->ptr_next){ // loop or branch to next
				PRB_section_count += probe.vector->ptr_next; // adjust PVC (section "count" -- better section-index)
				probe.vector += probe.vector->ptr_next; // next vector to loop
			}else
				stop_probe (); // error, no valid next vector: stop_probe now
		}
		else{
			probe.vector->i = probe.vector->repetitions; // loop done, reload loop counter for next time
//			probe.vector += probe.vector->ptr_final; // next vector -- possible...
			++probe.vector; // increment programm (vector) counter -- just increment!
			++PRB_section_count;
		}
#else
		++probe.vector; // increment programm (vector) counter
		++PRB_section_count;
#endif
	}
	probe.ix = probe.vector->n; // load total steps per section = # vec to add
	probe.iix = probe.vector->dnx; // do dnx steps to take data until next point!

	if (probe.vector->srcs & 0x0004){ // if Counter channel requested, restart counter/timer now
		setup_counter();
		restart_counter();
	}
}

void integrate_probe_data_srcs (){
#ifdef DSP_PROBE_AIC_AVG
	if (probe.vector->srcs & 0x01) // Zmonitor (AIC5 out)
		PRB_AIC_data_sum[8] += iobuf.mout5;

	if (probe.vector->srcs & 0x10) // AIC5 <-- I (current)
		PRB_AIC_data_sum[5] += iobuf.min5;

	if (probe.vector->srcs & 0x20) // AIC0
		PRB_AIC_data_sum[0] += iobuf.min0;

	if (probe.vector->srcs & 0x40) // AIC1
		PRB_AIC_data_sum[1] += iobuf.min1;

	if (probe.vector->srcs & 0x80) // AIC2
		PRB_AIC_data_sum[2] += iobuf.min2;

	if (probe.vector->srcs & 0x100) // AIC3
		PRB_AIC_data_sum[3] += iobuf.min3;

	if (probe.vector->srcs & 0x200) // AIC4
		PRB_AIC_data_sum[4] += iobuf.min4;

	if (probe.vector->srcs & 0x400) // AIC6
		PRB_AIC_data_sum[6] += iobuf.min6;

	if (probe.vector->srcs & 0x800) // AIC7
		PRB_AIC_data_sum[7] += iobuf.min7;
#endif
}

void store_probe_data_srcs ()
{
	int w=0;
 
	// get probe data srcs (word)

#ifdef DSP_PROBE_AIC_AVG
	if (probe.vector->options & VP_AIC_INTEGRATE){

		if (probe.vector->srcs & 0x01){ // Zmonitor (AIC5 -->)
			*probe_datafifo.buffer_w++ = (int)(PRB_AIC_data_sum[8]/PRB_AIC_num_samples); ++w;
		}
		if (probe.vector->srcs & 0x02) // Umonitor (AIC6 -->)
			*probe_datafifo.buffer_w++ = iobuf.mout6, ++w;
		if (probe.vector->srcs & 0x10){ // AIC5 <-- I (current)
			*probe_datafifo.buffer_w++ = (int)(PRB_AIC_data_sum[5]/PRB_AIC_num_samples); ++w;
		}
		if (probe.vector->srcs & 0x20){ // AIC0
			*probe_datafifo.buffer_w++ = (int)(PRB_AIC_data_sum[0]/PRB_AIC_num_samples); ++w;
		}
		if (probe.vector->srcs & 0x40){ // AIC1
			*probe_datafifo.buffer_w++ = (int)(PRB_AIC_data_sum[1]/PRB_AIC_num_samples); ++w;
		}
		if (probe.vector->srcs & 0x80){ // AIC2
			*probe_datafifo.buffer_w++ = (int)(PRB_AIC_data_sum[2]/PRB_AIC_num_samples); ++w;
		}
		if (probe.vector->srcs & 0x100){ // AIC3
			*probe_datafifo.buffer_w++ = (int)(PRB_AIC_data_sum[3]/PRB_AIC_num_samples); ++w;
		}
		if (probe.vector->srcs & 0x200){ // AIC4
			*probe_datafifo.buffer_w++ = (int)(PRB_AIC_data_sum[4]/PRB_AIC_num_samples); ++w;
		}
		if (probe.vector->srcs & 0x400){ // AIC6
			*probe_datafifo.buffer_w++ = (int)(PRB_AIC_data_sum[6]/PRB_AIC_num_samples); ++w;
		}
		if (probe.vector->srcs & 0x800){ // AIC7
			*probe_datafifo.buffer_w++ = (int)(PRB_AIC_data_sum[7]/PRB_AIC_num_samples); ++w;
		}
	} else {

#endif // ------------------------------------------------------------

		if (probe.vector->srcs & 0x01) // Zmonitor (AIC5 -->)
			*probe_datafifo.buffer_w++ = iobuf.mout5, ++w;
		if (probe.vector->srcs & 0x02) // Umonitor (AIC6 -->)
			*probe_datafifo.buffer_w++ = iobuf.mout6, ++w;
		if (probe.vector->srcs & 0x10) // AIC5 <-- I (current)
			*probe_datafifo.buffer_w++ = iobuf.min5, ++w;

		if (probe.vector->srcs & 0x20) // AIC0
			*probe_datafifo.buffer_w++ = iobuf.min0, ++w;

		if (probe.vector->srcs & 0x40) // AIC1
			*probe_datafifo.buffer_w++ = iobuf.min1, ++w;

		if (probe.vector->srcs & 0x80) // AIC2
			*probe_datafifo.buffer_w++ = iobuf.min2, ++w;

		if (probe.vector->srcs & 0x100) // AIC3
			*probe_datafifo.buffer_w++ = iobuf.min3, ++w;

		if (probe.vector->srcs & 0x200) // AIC4
			*probe_datafifo.buffer_w++ = iobuf.min4, ++w;

		if (probe.vector->srcs & 0x400) // AIC6
			*probe_datafifo.buffer_w++ = iobuf.min6, ++w;

		if (probe.vector->srcs & 0x800) // AIC7
			*probe_datafifo.buffer_w++ = iobuf.min7, ++w;

#ifdef DSP_PROBE_AIC_AVG
	}
#endif

	// adjust long ptr
	if (w&1) ++w, probe_datafifo.buffer_w++;

	probe_datafifo.w_position += w;

	w >>= 1;
	probe_datafifo.buffer_l += w;

	// LockIn data (long)
	if (probe.vector->srcs & 0x0F008) // "--" LockIn_0
		*probe_datafifo.buffer_l++ = probe.LockIn_0, probe_datafifo.buffer_w += 2,
			probe_datafifo.w_position += 2;
	    
	if (probe.vector->srcs & 0x01000) // "C1" LockIn_1st - phase A
		*probe_datafifo.buffer_l++ = probe.LockIn_1stA, probe_datafifo.buffer_w += 2,
			probe_datafifo.w_position += 2;

	if (probe.vector->srcs & 0x02000) // "D1" LockIn_1st - phase B
		*probe_datafifo.buffer_l++ = probe.LockIn_1stB, probe_datafifo.buffer_w += 2,
			probe_datafifo.w_position += 2;
	    
	if (probe.vector->srcs & 0x04000) // "--" LockIn_2nd - phase A
		*probe_datafifo.buffer_l++ = probe.LockIn_2ndA, probe_datafifo.buffer_w += 2,
			probe_datafifo.w_position += 2;

	if (probe.vector->srcs & 0x08000) // "--" LockIn_2nd - phase B
		*probe_datafifo.buffer_l++ = probe.LockIn_2ndB, probe_datafifo.buffer_w += 2,
			probe_datafifo.w_position += 2;
	    
	if (probe.vector->srcs & 0x00004){ // "--" last CR Counter count
		update_count ();
		*probe_datafifo.buffer_l++ = CR_generic_io.count, probe_datafifo.buffer_w += 2,
			probe_datafifo.w_position += 2;
	}
	    
	// check free buffer range!!
	if (probe_datafifo.w_position > (EXTERN_PROBEDATAFIFO_LENGTH - EXTERN_PROBEDATA_MAX_LEFT)){
		// loop-marking
		*probe_datafifo.buffer_l++ = 0;
		*probe_datafifo.buffer_l++ = -1;
		*probe_datafifo.buffer_l++ = 0;
		// loop to fifo start!
		probe_datafifo.buffer_w = probe_datafifo.buffer_base;
		probe_datafifo.buffer_l = (DSP_LONG*) probe_datafifo.buffer_base;
		probe_datafifo.w_position = 0;
	}
}

// run a probe section step:
// decrement and check counter for zero, then initiate next section (...add header info)
// decrement skip data counter, check for zero then
//  -acquire data of all srcs, store away
//  -update current count, restore skip data counter

void run_one_time_step(){
	integrate_probe_data_srcs ();

	if (! probe.iix--){
		if (probe.ix--){
			store_probe_data_srcs ();
			if (!probe.ix){
				next_section ();
				clear_probe_data_srcs ();
				probe_append_header_and_positionvector ();
			}
		}
		probe.iix = probe.vector->dnx; // load steps inbetween "data aq" points
		clear_probe_data_srcs ();
	}
}


#define LOCKIN_ON (probe.vector->srcs & 0x3000)
void run_probe (){

	if (LOCKIN_ON){
		if (PRB_lockin_restart_flg){
			probe.AC_ix = -PRB_SE_DELAY;
			PRB_lockin_restart_flg = 0;
		}

		// wait until starting procedure (korrelation integration time) finished
		if(probe.AC_ix >= 0){
			run_one_time_step ();
			add_probe_vector ();
		}

		run_lockin ();

	} else {
		PRB_lockin_restart_flg = 1;

		run_one_time_step ();
		add_probe_vector ();

		// manipulate U (bias)
		iobuf.mout6 = _lsshl (probe.Upos, -16) & 0xfffe;
	}

	// convert 32 bit data to DAC format and update DMA buffers:

	analog.x_offset = _lsshl (move.XPos, -16);
	analog.y_offset = _lsshl (move.YPos, -16);

	// used for phase probe now
	// analog.z_offset = _lsshl (move.ZPos, -16);

	analog.x_scan = _lsshl (_lsadd ( _lsmpy (_lsshl (scan.Xpos, -16), scan.rotxx),
					 _lsmpy (_lsshl (scan.Ypos, -16), scan.rotxy)
					), -16);
	analog.y_scan = _lsshl (_lsadd ( _lsmpy (_lsshl (scan.Xpos, -16), scan.rotyx),
					 _lsmpy (_lsshl (scan.Ypos, -16), scan.rotyy)
					), -16);

// update X,Y, X0,Y0,Z0 buffers
#ifdef DSP_OFFSET_ADDING
	if (state.mode & MD_OFFSETADDING){
		iobuf.mout2 = analog.z_offset & 0xfffe; /* Z Offset */
		iobuf.mout3 = _sadd (analog.x_scan, analog.x_offset) & 0xfffe;
		iobuf.mout4 = _sadd (analog.y_scan, analog.y_offset) & 0xfffe;
	} else {
		iobuf.mout0 = analog.x_offset & 0xfffe; /* X Offset */
		iobuf.mout1 = analog.y_offset & 0xfffe; /* Y Offset */
		iobuf.mout2 = analog.z_offset & 0xfffe; /* Z Offset */
		iobuf.mout3 = analog.x_scan & 0xfffe; /* X Scan */
		iobuf.mout4 = analog.y_scan & 0xfffe; /* Y Scan */
	}
#else
	iobuf.mout0 = analog.x_offset & 0xfffe; /* X Offset */
	iobuf.mout1 = analog.y_offset & 0xfffe; /* Y Offset */
	iobuf.mout2 = analog.z_offset & 0xfffe; /* Z Offset */
	iobuf.mout3 = analog.x_scan & 0xfffe; /* X Scan */
	iobuf.mout4 = analog.y_scan & 0xfffe; /* Y Scan */
#endif

// manipulate Z, relative to "current" Z
	iobuf.mout5 = _ssub (feedback.z, _lsshl (probe.Zpos, -16)) & 0xfffe;

// using AIC7-out as "z-offset" for auxillary stuff -- mirroring Z-Offset
//	iobuf.mout7 = analog.z_offset & 0xfffe;

// increment probe time
	++probe.time; 
}

// digital LockIn, computes 0., 1st, 2nd order Amplitudes, generates reference/bias sweep, phase correction for 1st and 2nd order
run_lockin (){
	int i;

        // LockIn Korrel func:  Amp = 2pi/nN Sum_{i=0..nN} I(i2pi/nN) sin(i2pi/nN + Delta)
        // tab used: PRB_sintab[i] = sin(i*2.*MATH_PI/SPECT_SIN_LEN);

#define Uin         iobuf.min5                // LockIn Data Signal 

        /* read DAC 0 (fixed to this (may be changed) channel for LockIn)
         * summ up avgerage
         * summ up korrelation product to appropriate korrel sum interleave buffer 
         */

// 		AvgData += Uin
	PRB_data_sum = _lsadd (PRB_data_sum, Uin);

//      PRB_korrel_sum1st[PRB_ki] += PRB_ref1st * Uin;

	PRB_korrel_sum1stA[PRB_ki] = _lsadd (PRB_korrel_sum1stA[PRB_ki], _lsshl ( _lsmpy (PRB_ref1stA, Uin), -16));
	PRB_korrel_sum1stB[PRB_ki] = _lsadd (PRB_korrel_sum1stB[PRB_ki], _lsshl ( _lsmpy (PRB_ref1stB, Uin), -16));

//	PRB_korrel_sum1st[PRB_ki] = _smac (PRB_korrel_sum1st[PRB_ki], PRB_ref1st, Uin);

	PRB_korrel_sum2ndA[PRB_ki] = _lsadd (PRB_korrel_sum2ndA[PRB_ki], _lsshl ( _lsmpy (PRB_ref2ndA, Uin), -16));
	PRB_korrel_sum2ndB[PRB_ki] = _lsadd (PRB_korrel_sum2ndB[PRB_ki], _lsshl ( _lsmpy (PRB_ref2ndB, Uin), -16));
                
	// average periods elapsed?
        if (PRB_AveCount >= probe.AC_nAve){
                PRB_AveCount = 0;
                probe.LockIn_1stA = PRB_korrel_sum1stA[0]; 
                probe.LockIn_1stB = PRB_korrel_sum1stB[0]; 
                probe.LockIn_2ndA = PRB_korrel_sum2ndA[0]; 
                probe.LockIn_2ndB = PRB_korrel_sum2ndB[0]; 

		// build korrelation sum
                for (i = 1; i < LOCKIN_ILN; ++i){
//                      LockInData += PRB_korrel_sum1st[i]
                        probe.LockIn_1stA = _lsadd (probe.LockIn_1stA, PRB_korrel_sum1stA[i]);
                        probe.LockIn_1stB = _lsadd (probe.LockIn_1stB, PRB_korrel_sum1stB[i]);
                        probe.LockIn_2ndA = _lsadd (probe.LockIn_2ndA, PRB_korrel_sum2ndA[i]);
                        probe.LockIn_2ndB = _lsadd (probe.LockIn_2ndB, PRB_korrel_sum2ndB[i]);
		}

		// data is ready now: probe.LockIn_1st, _2nd, _0
                if(probe.AC_ix >= 0)
			probe.LockIn_0 = PRB_data_sum; 

                ++probe.AC_ix;

                if (++PRB_ki >= LOCKIN_ILN)
                        PRB_ki = 0;

		// reset korrelation and average sums
                PRB_korrel_sum1stA[PRB_ki] = 0;
                PRB_korrel_sum1stB[PRB_ki] = 0;
                PRB_korrel_sum2ndA[PRB_ki] = 0;
                PRB_korrel_sum2ndB[PRB_ki] = 0;
		PRB_data_sum = 0;
        }

	PRB_modindex += probe.AC_frq;
        if (PRB_modindex >= SPECT_SIN_LEN){
                PRB_modindex=0;
                ++PRB_AveCount;
        }

        PRB_ref1stA = PRB_sintab[PRB_modindex];
//      write_dac (BASEBOARD, 3, PRB_XPos + PRB_ACAmp * PRB_ref);
	iobuf.mout6 = _lsshl (_smac (probe.Upos, PRB_ref1stA, probe.AC_amp), -16) & 0xfffe;

        // correct phases
	PRB_ref1stA = PRB_sinreftabA[(PRB_modindex + SPECT_SIN_LEN-PIPE_LEN*probe.AC_frq + PRB_ACPhIdxA) & SPECT_SIN_LEN_MSK];
	PRB_ref1stB = PRB_sinreftabB[(PRB_modindex + SPECT_SIN_LEN-PIPE_LEN*probe.AC_frq + PRB_ACPhIdxB) & SPECT_SIN_LEN_MSK];
	PRB_ref2ndA = PRB_sinreftabA[((PRB_modindex<<1) + SPECT_SIN_LEN-PIPE_LEN*probe.AC_frq + PRB_ACPhIdxA) & SPECT_SIN_LEN_MSK];
	PRB_ref2ndB = PRB_sinreftabB[((PRB_modindex<<1) + SPECT_SIN_LEN-PIPE_LEN*probe.AC_frq + PRB_ACPhIdxB) & SPECT_SIN_LEN_MSK];
}
