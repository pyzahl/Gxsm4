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
#include "dataprocess.h"

/* compensate for X AIC pipe delay */
#define NULL ((void *) 0)
#define PIPE_LEN (5)

extern SPM_STATEMACHINE state;
extern SPM_PI_FEEDBACK  feedback;
extern ANALOG_VALUES    analog;
extern PROBE            probe;
extern DATA_FIFO_EXTERN probe_datafifo;
extern MOVE_OFFSET      move;
extern AREA_SCAN        scan;
extern CR_GENERIC_IO    CR_generic_io;
extern DATA_SYNC_IO     data_sync_io;

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


#define SINE_GEN


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




#define SPECT_SIN_LEN_MSK  (SPECT_SIN_LEN-1)     /* "modulo" mask */
#define LOCKIN_ILN         8                     /* LockIn Interval interleave number */
#define PRB_SE_DELAY       (LOCKIN_ILN + 2)    /* Start/End Delay */

extern DSP_INT      prbdf;

int PRB_store_nx_init[3];
int PRB_store_nx_finish[3];
int PRB_store_nx;

int AC_tab_inc = 1;
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

int  PRB_Trk_state;
int  PRB_Trk_i;
int  PRB_Trk_ie;
long PRB_Trk_ref;

#define HRVALUE_TO_16(VALUE32)  _lsshl (VALUE32, -16)

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

	if (scan.dnx_probe){ // probe header for DSP LEVEL RASTER PROBE
                *probe_datafifo.buffer_w++ = scan.ix;
                *probe_datafifo.buffer_w++ = scan.iy;
                *probe_datafifo.buffer_w++ = 0;
                *probe_datafifo.buffer_w++ = HRVALUE_TO_16 (scan.xyz_vec[i_X]);
                *probe_datafifo.buffer_w++ = HRVALUE_TO_16 (scan.xyz_vec[i_Y]);
                *probe_datafifo.buffer_w++ = AIC_OUT(5);  // Z: feedback.z + Z_pos>>16; *****!
                *probe_datafifo.buffer_w++ = AIC_OUT(6);  // Bias: U_pos>>16; *******!
        } else { // probe header normal
		// Section start situation vector [XY[Z or Phase]_Offset, XYZ_Scan, Upos(bias), 0xffff]_16 :: 8
		*probe_datafifo.buffer_w++ = HRVALUE_TO_16 (move.xyz_vec[i_X]);
		*probe_datafifo.buffer_w++ = HRVALUE_TO_16 (move.xyz_vec[i_Y]);
		*probe_datafifo.buffer_w++ = _lsshl (PRB_ACPhaseA32, -12); // Phase    //  HRVALUE_TO_16 (move.xyz_vec[i_Z])

		*probe_datafifo.buffer_w++ = HRVALUE_TO_16 (scan.xy_r_vec[i_X]);
		*probe_datafifo.buffer_w++ = HRVALUE_TO_16 (scan.xy_r_vec[i_Y]);
		*probe_datafifo.buffer_w++ = AIC_OUT(5);  // Z: feedback.z + Z_pos>>16;
		*probe_datafifo.buffer_w++ = AIC_OUT(6);  // Bias: U_pos>>16;
	}
	
	*probe_datafifo.buffer_w++ = PRB_section_count;
	probe_datafifo.buffer_l += 4; // skip 4 long == 8 words

	probe_datafifo.w_position += 6 + 6 + 2;
}


void init_lockin (){
	int j;
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
		AC_tab_inc = 8*AC_TAB_INC;     // LockIn-Ref on 1375Hz
	else if (probe.AC_frq > 500)
		AC_tab_inc = 4*AC_TAB_INC;     // LockIn-Ref on 687.5Hz
	else if (probe.AC_frq > 250)
		AC_tab_inc = 2*AC_TAB_INC;     // LockIn-Ref on 343.75Hz
	else AC_tab_inc = 1*AC_TAB_INC;     // LockIn-Ref on 171.875Hz

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
	probe.vector = NULL;
	probe.time = 0; 
	probe.lix = 0; 

	PRB_Trk_state = -1;
	PRB_Trk_ref   = 0L;

	PRB_lockin_restart_flg = 1;

//	probe.f_dx = _lsshl ( probe.f_dx, -8+AC_tab_inc); // smooth bias ramp compu for LockIn

	// calc data normalization factor
//	PRB_ks_normfac  = PRB_ACMult*2.*MATH_PI/(PRB_nAve*SPECT_SIN_LEN*LOCKIN_ILN);
//	PRB_avg_normfac = PRB_ACMult/(PRB_nAve*SPECT_SIN_LEN*LOCKIN_ILN);

	// Probe data preample with full start position vector

	next_section ();
	clear_probe_data_srcs ();
	probe_append_header_and_positionvector ();

        probe.start = 0;
        probe.stop  = 0;
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
	// check for valid vector and for limiter active (freeze)
	if (!probe.vector || probe.lix) return;

// _lsadd full differential situation vector to current situation, using saturation mode
	probe.Upos = _lsadd (probe.Upos, probe.vector->f_du);

	scan.xyz_vec[i_X] = _lsadd (scan.xyz_vec[i_X], probe.vector->f_dx);
	scan.xyz_vec[i_Y] = _lsadd (scan.xyz_vec[i_Y], probe.vector->f_dy);

	probe.Zpos = _lsadd (probe.Zpos, probe.vector->f_dz);

	move.xyz_vec[i_X] = _lsadd (move.xyz_vec[i_X], probe.vector->f_dx0);
	move.xyz_vec[i_Y] = _lsadd (move.xyz_vec[i_Y], probe.vector->f_dy0);
	//	move.xyz_vec[i_Z] = _lsadd (move.xyz_vec[i_Z], probe.vector->f_dz0);

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
	PRB_AIC_num_samples = 0;
}

void integrate_probe_data_srcs (){
#ifdef DSP_PROBE_AIC_AVG
	if (probe.vector->srcs & 0x01) // Zmonitor (AIC5 out)
		PRB_AIC_data_sum[8] += AIC_OUT(5);

	if (probe.vector->srcs & 0x10) // AIC0 <-- FBMix 0: I (current)
		PRB_AIC_data_sum[0] += AIC_IN(0);

	if (probe.vector->srcs & 0x20) // AIC1 <-- FBMix 1: dF (aux1)
		PRB_AIC_data_sum[1] += AIC_IN(1);

	if (probe.vector->srcs & 0x40) // AIC2 <-- FBMix 2: (aux2)
		PRB_AIC_data_sum[2] += AIC_IN(2);

	if (probe.vector->srcs & 0x80) // AIC3 <-- FBMix 3: (aux3)
		PRB_AIC_data_sum[3] += AIC_IN(3);

	if (probe.vector->srcs & 0x100) // AIC4
		PRB_AIC_data_sum[4] += AIC_IN(4);

	if (probe.vector->srcs & 0x200) // AIC5
		PRB_AIC_data_sum[5] += AIC_IN(5);

	if (probe.vector->srcs & 0x400) // AIC6
		PRB_AIC_data_sum[6] += AIC_IN(6);

	if (probe.vector->srcs & 0x800) // AIC7
		PRB_AIC_data_sum[7] += AIC_IN(7);

        PRB_AIC_num_samples++;
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
			*probe_datafifo.buffer_w++ = AIC_OUT(6), ++w;
		if (probe.vector->srcs & 0x10){ // AIC0 <-- FBMix 0: I (current)
			*probe_datafifo.buffer_w++ = (int)(PRB_AIC_data_sum[0]/PRB_AIC_num_samples); ++w;
		}
		if (probe.vector->srcs & 0x20){ // AIC1 <-- FBMix 1: dF
			*probe_datafifo.buffer_w++ = (int)(PRB_AIC_data_sum[1]/PRB_AIC_num_samples); ++w;
		}
		if (probe.vector->srcs & 0x40){ // AIC2 <-- FBMix 2
			*probe_datafifo.buffer_w++ = (int)(PRB_AIC_data_sum[2]/PRB_AIC_num_samples); ++w;
		}
		if (probe.vector->srcs & 0x80){ // AIC3 <-- FBMix 3
			*probe_datafifo.buffer_w++ = (int)(PRB_AIC_data_sum[3]/PRB_AIC_num_samples); ++w;
		}
		if (probe.vector->srcs & 0x100){ // AIC4
			*probe_datafifo.buffer_w++ = (int)(PRB_AIC_data_sum[4]/PRB_AIC_num_samples); ++w;
		}
		if (probe.vector->srcs & 0x200){ // AIC5
			*probe_datafifo.buffer_w++ = (int)(PRB_AIC_data_sum[5]/PRB_AIC_num_samples); ++w;
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
			*probe_datafifo.buffer_w++ = AIC_OUT(5), ++w;
		if (probe.vector->srcs & 0x02) // Umonitor (AIC6 -->)
			*probe_datafifo.buffer_w++ = AIC_OUT(6), ++w;
		if (probe.vector->srcs & 0x10) // AIC0
			*probe_datafifo.buffer_w++ = AIC_IN(0), ++w;

		if (probe.vector->srcs & 0x20) // AIC1
			*probe_datafifo.buffer_w++ = AIC_IN(1), ++w;

		if (probe.vector->srcs & 0x40) // AIC2
			*probe_datafifo.buffer_w++ = AIC_IN(2), ++w;

		if (probe.vector->srcs & 0x80) // AIC3
			*probe_datafifo.buffer_w++ = AIC_IN(3), ++w;

		if (probe.vector->srcs & 0x100) // AIC4
			*probe_datafifo.buffer_w++ = AIC_IN(4), ++w;

		if (probe.vector->srcs & 0x200) // AIC5
			*probe_datafifo.buffer_w++ = AIC_IN(5), ++w;

		if (probe.vector->srcs & 0x400) // AIC6
			*probe_datafifo.buffer_w++ = AIC_IN(6), ++w;

		if (probe.vector->srcs & 0x800) // AIC7
			*probe_datafifo.buffer_w++ = AIC_IN(7), ++w;
//			*probe_datafifo.buffer_w++ = feedback.q_factor15, ++w;
#ifdef DSP_PROBE_AIC_AVG
	}
#endif

	// adjust long ptr
	if (w&1) ++w, probe_datafifo.buffer_w++;

	probe_datafifo.w_position += w;

	w >>= 1;
	probe_datafifo.buffer_l += w;

	// read and buffer (for Rate Meter, gatetime not observed)
	CR_generic_io.count_0 = analog.counter[0];

	// LockIn data (long)
	if (probe.vector->srcs & 0x00008) // "--" LockIn_0
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
		*probe_datafifo.buffer_l++ = analog.counter[0], probe_datafifo.buffer_w += 2,
			probe_datafifo.w_position += 2;
		analog.counter[0] = 0L;
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

	// update sync info
        data_sync_io.xyit[2]=probe.ix;
        //data_sync_io.xyti[3]=state.DSP_time;
        data_sync_io.tick=1;
}



// mini probe program flow interpreter kernel
void next_section (){
	if (! probe.vector){ // initialize ?
		probe.vector = probe.vector_head;
                clear_probe_data_srcs ();
		PRB_section_count = 0; // init PVC
#if 0
                if (probe.vector < (DSP_INT)(&prbdf)){// error, cancel probe now.
                        probe.stop  = 1;
                        probe.pflg = 0;
                }
#endif
	}
	else{
		if (!probe.vector->ptr_final){ // end Vector program?
                        probe.stop  = 1;
                        probe.pflg = 0;
			return;
		}
#ifdef DSP_PROBE_VECTOR_PROGRAM
		if (probe.vector->i > 0){ // do next loop?
			--probe.vector->i; // decrement loop counter
			if (probe.vector->ptr_next){ // loop or branch to next
				PRB_section_count += probe.vector->ptr_next; // adjust PVC (section "count" -- better section-index)
				probe.vector += probe.vector->ptr_next; // next vector to loop
			}else{
                                probe.stop  = 1;
                                probe.pflg = 0;
			} // error, no valid next vector: stop_probe now
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
		analog.counter[0] = 0L;
		analog.counter[1] = 0L;
	}
}


// #define VP_TRACK_SRC     0xC0 // Track "Value" source code bit mask 0x40+0x80  00: Z (OUT5), 01: I (IN0), 10: (IN1), 11: (IN2)
inline int track_reference_source (){
	switch (probe.vector->options & VP_TRACK_SRC){
	case 0x00: return AIC_OUT(5); // Z
	case 0x40: return AIC_IN(0);
	case 0x80: return AIC_IN(1);
	case 0xC0: return AIC_IN(2);
	}
	return  AIC_OUT(5); // fall back to Z
}

// manage conditional vector tracking mode -- atom/feature tracking
void next_track_vector(){
	int value;
	if (probe.vector->options & VP_TRACK_REF){ // take reference?
		PRB_Trk_state = 0;
		PRB_Trk_ie    = 0;
		PRB_Trk_i     = 999;
		PRB_Trk_ref   = track_reference_source ();
	} else if ((probe.vector->options & VP_TRACK_FIN) || (PRB_Trk_i <= PRB_Trk_ie )){ // follow to max/min
		if (PRB_Trk_i == 999){ // done with mov to fin now
			++PRB_Trk_state;
			if (PRB_Trk_ie > 0){ // start move to condition match pos
				PRB_Trk_i = 0;
				probe.vector -= PRB_Trk_state; // skip back to v1 of TRACK vectors
				PRB_section_count -= PRB_Trk_state; // skip back to v1 of TRACK vectors
			} else { // done, no move, keep watching!
//				PRB_Trk_i     = 999; // it's still this!
				PRB_Trk_state = -1;
				PRB_Trk_ref   = 0L;
				clear_probe_data_srcs ();
				probe_append_header_and_positionvector ();
			}
		} else if (PRB_Trk_i == PRB_Trk_ie){ // done!
			probe.vector += PRB_Trk_state - PRB_Trk_ie; // skip forward to end of track sequence
			PRB_section_count += PRB_Trk_state - PRB_Trk_ie; // skip forward to end of track sequence
			PRB_Trk_i     = 999;
			PRB_Trk_state = -1;
			PRB_Trk_ref   = 0L;
			clear_probe_data_srcs ();
			probe_append_header_and_positionvector ();
		}
		++PRB_Trk_i;
	} else if (probe.vector->options & (VP_TRACK_UP | VP_TRACK_DN)){
		++PRB_Trk_state;
		value = track_reference_source ();
		if (probe.vector->options & VP_TRACK_UP){
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
	++probe.vector; // increment programm (vector) counter to go to next TRACK check position
	++PRB_section_count; // next
	probe.ix = probe.vector->n; // load total steps per section = # vec to add
	probe.iix = probe.vector->dnx; // do dnx steps to take data until next point!
}

inline int limiter_reference_source (){
	switch (probe.vector->options & VP_LIMIT_SRC){
	case 0x00: return AIC_IN(0);
	case 0x40: return AIC_IN(1);
	case 0x80: return AIC_IN(2);
	case 0xC0: return AIC_IN(3);
	}
	return  AIC_IN(0); // fall back
}

// ===== FIX ME and REVIEW !!! ======= PORT IN PROGRESS
inline int trigger_reference_source (){
	switch (probe.vector->options & VP_LIMIT_SRC){
	case 0x00: return AIC_IN(0);
	case 0x40: return AIC_IN(1);
	case 0x80: return AIC_IN(2);
	case 0xC0: return AIC_IN(3);
	}
	return  AIC_IN(0); // fall back
}

// ===== FIX ME and REVIEW !!! ======= PORT IN PROGRESS
// #pragma CODE_SECTION(probe_signal_limiter_test, ".text:slow")
void probe_signal_limiter_test(){	
        if (probe.lix > 0) { ++probe.lix;  goto run_one_time_step_1; }
        if (probe.vector->options & VP_LIMITER_UP)
                if (limiter_reference_source () > probe.LIM_up){
                        ++probe.lix;
                        goto run_one_time_step_1;
                }
        if (probe.vector->options & VP_LIMITER_DN)
                if (limiter_reference_source () < probe.LIM_dn){
                        ++probe.lix;
                        goto run_one_time_step_1;
                }
        
        goto limiter_continue;

 run_one_time_step_1:
        if ((probe.vector->options & VP_LIMITER_MODE) != 0) {
                probe.lix = 1; // cancel any "undo" mode, use final vector in next section
                probe.vector->i = 0; // exit any loop
        }
 limiter_continue:
        ;
}



// trigger condition evaluation
// #pragma CODE_SECTION(wait_for_trigger, ".text:slow")
int wait_for_trigger (){
	if (probe.vector->options == (VP_TRIGGER_P | VP_TRIGGER_N)) // logic
		return ((trigger_reference_source ()) & ((probe.vector->options & VP_GPIO_MSK) >> 16)) ? 0:1; // usually tigger_input would point to probe.gpio_data for this GPIO/logic trigger
	else if (probe.vector->options & VP_TRIGGER_P){ // positive
		return (trigger_reference_source () > 0) ? 0:1;
	} else { // VP_TRIGGER_N -> negative 
		return (trigger_reference_source () < 0) ? 0:1;
	}
}


// prepare for next section or tracking vector -- called from idle processing regime!
// #pragma CODE_SECTION(process_next_section, ".text:slow")
void process_next_section (){
        // Special VP Track Mode Vector Set?
        if (probe.vector->options & (VP_TRACK_UP | VP_TRACK_DN | VP_TRACK_REF | VP_TRACK_FIN)){
                next_track_vector ();
                
        } else { // run regular VP program mode
                next_section ();
                //                clear_probe_data_srcs ();
                probe_append_header_and_positionvector ();
        }
}

#if 0
#pragma CODE_SECTION(GPIO_check, ".text:slow")
void GPIO_check(){
        if (!GPIO_subsample--){
                GPIO_subsample = CR_generic_io.gpio_subsample;
                WR_GPIO (GPIO_Data_0, &CR_generic_io.gpio_data_in, 0);
                probe.gpio_data = CR_generic_io.gpio_data_in; // mirror data in 32bit signal
        }
}
#endif

// run a probe section step:
// decrement and check counter for zero, then initiate next section (...add header info)
// decrement skip data counter, check for zero then
//  -acquire data of all srcs, store away
//  -update current count, restore skip data counter

void run_probe (){
        // next time step in GVP
        // ** run_one_time_step ();
        // ** add_probe_vector ();

        // next time step in GVP
        if (probe.ix){ // idle task is working on completion of data management, wait!
#if 0
                // initiate external data SPI request and transfer
                if (probe.iix == probe.vector->dnx && !(state.dp_task_control[9].process_flag&0xffff))
                        initiate_McBSP_transfer (probe.ix);

                // read GPIO if requested (CPU time costy!!!)
                if (probe.vector->options & VP_GPIO_READ)
                        GPIO_check();
#endif
                // unsatisfied trigger condition will also HOLD VP as of now!
                if (probe.vector->options & (VP_TRIGGER_P|VP_TRIGGER_N))
                        if (wait_for_trigger ())
                                return;

                integrate_probe_data_srcs ();

                if (! probe.iix-- || probe.lix){
                        if (probe.ix--){
#ifdef ENABLE_SCAN_GP55_CLOCK
                                // generate external probe point data clock on GP55
                                if (probe.ix&1)
                                        SET_DSP_GP55;
                                else
                                        CLR_DSP_GP55;
#endif
                                store_probe_data_srcs ();
                        
#if 0 // ===> moved to idle tasks processing space
                                if (!probe.ix){
                                        // Special VP Track Mode Vector Set?
                                        if (probe.vector->options & (VP_TRACK_UP | VP_TRACK_DN | VP_TRACK_REF | VP_TRACK_FIN)){
                                                next_track_vector ();

                                        } else { // run regular VP program mode
                                                next_section ();
                                                probe_append_header_and_positionvector ();
                                        }
                                }
#endif
                        }
                        // Limiter active?
                        if (probe.vector->options & VP_LIMITER){
                                probe_signal_limiter_test();
                        } else if (probe.lix > 0) --probe.lix;
		 
                        probe.iix = probe.vector->dnx; // load steps inbetween "data aq" points
                        clear_probe_data_srcs ();
                }
       
                add_probe_vector ();
        }

        // increment probe time
	++probe.time; 
}


void sine_sdb32(int *cr, int *ci, int deltasRe, int deltasIm){
	long RealP,ImaP,Error_e;
	long RealP_Hi,ImaP_Hi,Error_int;
	
	// *********** Sinus calculation -- reverted to 32bit from 64bit precision

	//    Real part calculation : crdr - cidi
	//  Q31*Q31 - Q31*Q31=Q62
	//  For cr=2147483647 and deltasRe=2129111627
	//      ci=0 and  deltasIm=280302863
	//  crdr - cidi = 2129111626 Q31 (RealP_Hi)
	//  RealP=4572232401620063669

//	RealP=((long)cr[0] * (long)deltasRe) - ((long)ci[0] * (long)deltasIm);
//	RealP_Hi=(long)(RealP>>15); //Q32 to Q15

	RealP    = _lssub (_lsmpy (cr[0], deltasRe), _lsmpy (ci[0], deltasIm));
	RealP_Hi = _lsshl (RealP, -15);

	//    Imaginary part calculation : drci + crdi
	//  Q15*Q15 - Q15*Q15=Q32
	//  drci - crdi= -280302863 Q31 (ImaP_Hi)
	//  ImaP=-601945814499781361

//	ImaP=((long)ci[0] * (long)deltasRe) + ((long)cr[0] * (long)deltasIm);
//	ImaP_Hi=(long)(ImaP>>15); //Q62 to Q31

	ImaP    = _lsadd (_lsmpy (ci[0], deltasRe), _lsmpy (cr[0], deltasIm));
	ImaP_Hi = _lsshl (ImaP, -15);

	//    e=Error/2 = (1 - (cr^2+ci^2))/2
	//  e=(Q62 - Q62 - Q62))>>32 --> Q30 (so Q31/2)
	//  4611686018427387903 - (-280302863*-280302863) - (2129111626*2129111626)
	//  Error_e=4611686018427387903 - 78569695005996769 - 4533116315968363876
	//  Error_e = 7453027258
	//  Error_int = 1

	Error_e=0x3FFFFFFF; //1073741823
//	Error_e=Error_e - ((long)ImaP_Hi*(long)ImaP_Hi) - ((long)RealP_Hi*(long)RealP_Hi);
//	Error_int=(long)(Error_e>>16); //Q32 to Q15 plus /2
	Error_e   = _lssub (Error_e, _lssub (_lsmpy (ImaP_Hi, ImaP_Hi), _lsmpy (RealP_Hi, RealP_Hi)));
	Error_int = _lsshl (Error_e, -16);

	//    Update Cre and Cim
	//  Cr=Cr(1-e)=Cr-Cr*e // Q62 - Q31*Q31
	//  Ci=Ci(1-e)=Ci-Ci*e
	//  ImaP=-601945814499781361 - 1*-280302863 = -601945814219478498
	//  ci[0]=-280302862
	//  RealP=4572232401620063669 - 1*2129111626 = 4572232399490952043
	//  cr[0]=2129111625

//	ImaP=ImaP + ((long)ImaP_Hi*(long)Error_int);
//	ImaP_Hi=(long)(ImaP>>15); //Q62 to Q31
	ImaP    = _lsadd (ImaP, _lsmpy (ImaP_Hi, Error_int));
	ImaP_Hi = _lsshl (ImaP, -15);

	if (ImaP_Hi>32767)ImaP_Hi=32767;
	if (ImaP_Hi<-32767)ImaP_Hi=-32767;
	ci[0]=(int)ImaP_Hi;

//	RealP=RealP + ((long)RealP_Hi*(long)Error_int);
//	RealP_Hi=(long)(RealP>>15); //Q62 to Q31
	RealP    = _lsadd (RealP, _lsmpy (RealP_Hi, Error_int));
	RealP_Hi = _lsshl (RealP, -15);

	if (RealP_Hi>32767)RealP_Hi=32767;
	if (RealP_Hi<-32767)RealP_Hi=-32767;
	cr[0]=(int)RealP_Hi;
}


// digital LockIn, computes 0., 1st, 2nd order Amplitudes, generates reference/bias sweep, phase correction for 1st and 2nd order
run_lockin (){
	int i;

        // LockIn Korrel func:  Amp = 2pi/nN Sum_{i=0..nN} I(i2pi/nN) sin(i2pi/nN + Delta)
        // tab used: PRB_sintab[i] = sin(i*2.*MATH_PI/SPECT_SIN_LEN);

#define Uin         AIC_IN(0)                // LockIn Data Signal 

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

	PRB_modindex += AC_tab_inc;
        if (PRB_modindex >= SPECT_SIN_LEN){
                PRB_modindex=0;
                ++PRB_AveCount;
        }

        PRB_ref1stA = PRB_sintab[PRB_modindex];
//      write_dac (BASEBOARD, 3, PRB_XPos + PRB_ACAmp * PRB_ref);
	AIC_OUT(6) = _lsshl (_smac (probe.Upos, PRB_ref1stA, probe.AC_amp), -16);

        // correct phases
	PRB_ref1stA = PRB_sinreftabA[(PRB_modindex + SPECT_SIN_LEN-PIPE_LEN*AC_tab_inc + PRB_ACPhIdxA) & SPECT_SIN_LEN_MSK];
	PRB_ref1stB = PRB_sinreftabB[(PRB_modindex + SPECT_SIN_LEN-PIPE_LEN*AC_tab_inc + PRB_ACPhIdxB) & SPECT_SIN_LEN_MSK];
	PRB_ref2ndA = PRB_sinreftabA[((PRB_modindex<<1) + SPECT_SIN_LEN-PIPE_LEN*AC_tab_inc + PRB_ACPhIdxA) & SPECT_SIN_LEN_MSK];
	PRB_ref2ndB = PRB_sinreftabB[((PRB_modindex<<1) + SPECT_SIN_LEN-PIPE_LEN*AC_tab_inc + PRB_ACPhIdxB) & SPECT_SIN_LEN_MSK];
}
