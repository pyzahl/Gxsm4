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
/* 20040820 CVS fix2 */

/*
 *
 *	dataprocess.c
 *
 *	This is the ISR of the DMA. This ISR is called at each new sample.
 */

#include "g_intrinsics.h"
#include "FB_spm_dataexchange.h"
#include "dataprocess.h"  
#include "PAC_pll.h"
#include "spm_log.h"
#include "FB_spm_offsetmove.h"
#include "FB_spm_areascan.h"
#include "FB_spm_probe.h"
#include "FB_spm_autoapproach.h"
#include "FB_spm_CoolRunner_puls.h"

#ifdef DSPEMU
#include "assert.h"
#include "time.h"
#endif

/* local used variables for automatic offset compensation */
int       IdleTimeTmp;
long      tmpsum  = 0;
DSP_INT64 rmssum2 = 0L;
DSP_INT64 rmssum  = 0L;

// RMS buffer
#define RMS_N2           8
#define RMS_N            (1 << RMS_N2)
int     rms_pipi = 0;       /* Pipe index */
int     rms_I_pipe[RMS_N] = 
{ 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0
};
#if 0
DSP_INT32    rms_I2_pipe[RMS_N] = 
{ 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0
};
#endif
DSP_INT64    rms_I2_pipe64[RMS_N] = 
{ 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0
};

// 8x digital sigma-delta over sampling using bit0 of 16 -> gain of 2 bits resoultion (16+2=18)
#define SIGMA_DELTA_LEN   8
int     sigma_delta_index = 0;
int	max_out_ch = 8; // it's down to 7 when PLL is active -- else PLL output signal is overwritten

DSP_INT32  s_xymult = 0;
// DSP_INT32  zpos_xymult = 0;
DSP_INT32  d_tmp=0;
DSP_INT32  tmp_hrbits = 0;

//#define SIGMA_DELTA_HR_MASK_FAST
#ifdef SIGMA_DELTA_HR_MASK_FAST  // Fast HR Matrix -- good idea, but performance/distorsion issues with the A810 due to the high dynamic demands
DSP_INT32 sigma_delta_hr_mask[SIGMA_DELTA_LEN * SIGMA_DELTA_LEN] = { 
        0, 0, 0, 0,  0, 0, 0, 0,
        0, 0, 0, 0,  1, 0, 0, 0,
        0, 0, 0, 1,  0, 0, 0, 1,
        0, 0, 1, 0,  0, 1, 0, 1,

        0, 1, 0, 1,  0, 1, 0, 1,
        0, 1, 1, 0,  1, 1, 0, 1,
        1, 0, 1, 1,  1, 0, 1, 1,
        1, 1, 1, 1,  0, 1, 1, 1
};
#else // Slow HR Matrix -- actually better analog performance
DSP_INT32 sigma_delta_hr_mask[SIGMA_DELTA_LEN * SIGMA_DELTA_LEN] = { 
        0, 0, 0, 0,  0, 0, 0, 0,
        1, 0, 0, 0,  0, 0, 0, 0,
        1, 1, 0, 0,  0, 0, 0, 0,
        1, 1, 1, 0,  0, 0, 0, 0,

        1, 1, 1, 1,  0, 0, 0, 0,
        1, 1, 1, 1,  1, 0, 0, 0,
        1, 1, 1, 1,  1, 1, 0, 0,
        1, 1, 1, 1,  1, 1, 1, 0
};
#endif

// for(hrb=0; hrb<8; ++hrb) {for(i=0; i<8; ++i) printf ("%d, ", hrb>i?1:0 ); printf("\n");

#define HR_OUT(CH, VALUE32) tmp_hrbits = _SSHL32 (VALUE32, -13) & 7; AIC_OUT(CH) = _SADD16 (_SSHL32 (VALUE32, -16), sigma_delta_hr_mask[(tmp_hrbits<<3) + sigma_delta_index])
#define DC_OUT(CH, VALUE32) AIC_OUT(CH) = _SSHL32 (VALUE32, -16)

int     sdt=0;
int     sdt_16=0;

DSP_UINT16      QEP_cnt_old[2];
DSP_UINT32      SUM_QEP_cnt_Diff[2];

int       gate_cnt = 0;
int       gate_cnt_multiplier = 0;
int       gate_cnt_1 = 0;
int       feedback_hold = 0; /* Probe controlled Feedback-Hold flag */
int       pipi = 0;       /* Pipe index */
int       scnt = 0;       /* Slow summing count */

/* IIR tmp */

DSP_INT32   Q30   = 1073741823L;
DSP_INT32   Q15L   = 32767L;
DSP_INT32   Q15    = 32767;
DSP_INT32   AbsIn  = 0;

/* RANDOM GEN */

DSP_INT32 randomnum = 1L;

DSP_INT32 blcklen_mem = 0;

/* externals of SPM control */

#ifndef DSPEMU
extern SPM_STATEMACHINE state;
extern SERVO            z_servo;
extern SERVO            m_servo;
extern SCO              sco_s[2];
extern FEEDBACK_MIXER   feedback_mixer;
extern ANALOG_VALUES    analog;
extern MOVE_OFFSET      move;
extern AREA_SCAN        scan;
extern AUTOAPPROACH     autoapp;
extern PROBE            probe;
extern CR_OUT_PULSE     CR_out_pulse;
extern CR_GENERIC_IO    CR_generic_io;
extern SIGNAL_MONITOR   sig_mon;
extern PLL_LOOKUP       PLL_lookup;
#endif

extern int AS_ch2nd_constheight_enabled; /* const H mode flg of areascan process */
extern struct aicregdef    aicreg;

extern DSP_INT32 PRB_ACPhaseA32;

DSP_INT32 xy_vec[4];
DSP_INT32 mm_vec[4];
DSP_INT32 result_vec[4];


#define BIAS_ADJUST_STEP (4*(1<<16))
#define Z_ADJUST_STEP    (0x200)

/* Auxillary Random Number Generator */

#define RNG_a 16807         /* multiplier */
#define RNG_m 2147483647L   /* 2**31 - 1 */
#define RNG_q 127773L       /* m div a */
#define RNG_r 2836          /* m mod a */

#define USE_MUL32C
#ifdef USE_MUL32C 
// multiplication of 2x 32bit Q31 elements

// The overhead is now 38 cycles and you can use this formula to compute the total number of cycle:
// 38+nx*1.5 cycles. The code size if now 26 instructions instead of 20 for the version I sent to you this morning.

unsigned int mul32(int *restrict x, int *restrict y,  int *restrict r, unsigned short nx)
{
#ifndef DSPEMU
   int i;
   _nassert((int) nx % 2 == 0);
   _nassert((int) nx >= 2);
  
   for (i=0; i<nx; i += 2)
   {
       r[i] =(int)(((long long)(x[i]) * (long long)(y[i])) >> 31);
       r[i+1] =(int)(((long long)(x[i+1]) * (long long)(y[i+1])) >> 31);
   }
#else
   int i;
   assert((int) nx % 2 == 0);
   assert((int) nx >= 2);
  
   for (i=0; i<nx; i += 2)
   {
       r[i] =(int)(((long long)(x[i]) * (long long)(y[i])) >> 31);
       r[i+1] =(int)(((long long)(x[i+1]) * (long long)(y[i+1])) >> 31);
   }
#endif
   return 0;
}

#if 0
// The overhead of the function (when linked in L1PRAM) is 42 cpu cycles and you have to compute 2 CPU cycles by nx if x, y and r are in L1DRAM. 
// So, it is 42+nx*2 cycles.
unsigned int mul32(int *restrict x, int *restrict y,  int *restrict r, unsigned short nx){
	int i;
	for (i=0; i<nx; i++)
		r[i] =(int)(((long long)(x[i]) * (long long)(y[i])) >> 31);
	return 0;
}
#endif


#else
# include "mul32.h"
#endif

#ifndef DSPEMU
#pragma CODE_SECTION(generate_nextlongrand, ".text:slow")
#endif

void generate_nextlongrand (){
      DSP_INT32 lo, hi;

      lo = _SMPY32 (RNG_a, randomnum & 0xFFFF);
      hi = _SMPY32 (RNG_a, (DSP_UINT32)randomnum >> 16);
      lo += (hi & 0x7FFF) << 16;
      if (lo > RNG_m){
            lo &= RNG_m;
            ++lo;
      }
      lo += hi >> 15;
      if (lo > RNG_m){
            lo &= RNG_m;
            ++lo;
      }
      randomnum = (DSP_INT32)lo;
}

/* #define AIC_OUT(N) iobuf.mout[N] */
/* smoothly adjust bias - make sure |analog_bias| < 32766-BIAS_ADJUST_STEP !!  */
void run_bias_adjust ();
inline void run_bias_adjust (){
	if (DSP_MEMORY(analog).bias - BIAS_ADJUST_STEP > DSP_MEMORY(analog).bias_adjust){
		DSP_MEMORY(analog).bias_adjust += BIAS_ADJUST_STEP;
	}else{	
		if (DSP_MEMORY(analog).bias + BIAS_ADJUST_STEP < DSP_MEMORY(analog).bias_adjust)
			DSP_MEMORY(analog).bias_adjust -= BIAS_ADJUST_STEP;
		else	
			DSP_MEMORY(analog).bias_adjust = DSP_MEMORY(analog).bias;
	}
}

/* smoothly brings Zpos back to zero in case VP left it non zero at finish */
void run_Zpos_adjust ();
inline void run_Zpos_adjust (){
	if (DSP_MEMORY(probe).Zpos > Z_ADJUST_STEP)
		DSP_MEMORY(probe).Zpos -= Z_ADJUST_STEP;
	else
		if (DSP_MEMORY(probe).Zpos < -Z_ADJUST_STEP)
			DSP_MEMORY(probe).Zpos += Z_ADJUST_STEP;
		else
			DSP_MEMORY(probe).Zpos = 0;
}

/* run SCO on signal */
#define CPN31 2147483647
void run_sco (SCO *sco);
inline void run_sco (SCO *sco){ // "VCO" or SCO = Signal Controlled Oszillator
        long long tmp;
  // mapping VCO_Vcenter, Vspan  => VCO_fcenter, fspan -> VCO_fout = (VCO_Vin-VCO_Vcenter) * fspan / Vspan
  // 100 .. 1000Hz -> 2pi / 1500, 150 ->  approx: deltaIm = CPN * sin (2pi f / 150000) ~= 2147483647 * 2 * pi / 150000 f = 89953.584655 f
  // with y^2:=deltasRe^2 + x^2:=deltasIm^2 = "1" := CPN^2  -> "y=1-1/2*x*x-1/8*x*x*x*x"... -> CPN-deltasIm*deltasIm/(2*CPN)...

  // #define CPN(N) ((double)(1LL<<(N))-1.)
  // deltasRe = round (CPN(PREC) * cos (2.*M_PI*freq/150000.)); -- here 2147483647 * cos (2pi/N), N=n*dnx*2+dny
  // deltasIm = round (CPN(PREC) * sin (2.*M_PI*freq/150000.)); -- ... sin
  // VCO_cr = -2147483647; // staring sine at 6 O'Clock or at -1.00 or -(1<<31) int equiv.
  // VCO_ci = 0;
  // VCO_amplitude = (long long)((scan.dnx * scan.nx * scan.cfs_dx)>>1);
        if (sco->control == 0){
                sco->deltasIm = tmp = ((long long)_SSUB32(*sco->in, sco->center) * (long long)sco->fspanV) >> 21; // Q15.16 + QR89953.584655/1024 ; fspanV in f [Hz] * 89953.584655 / Vspan [V15.16] ??
                sco->deltasRe = CPN31 - ((tmp*tmp) >> 32);
        }
	SineSDB (&sco->cr, &sco->ci, &sco->cr2, &sco->ci2, sco->deltasRe, sco->deltasIm);
	sco->out = (int)(((long long)(*sco->amplitude) * (long long)sco->cr)>>31); // adjust "volume"
}

/* generic servo controller, run one time step */
/*
 * --------------------------------------------------------------------------------
	;; compute Proportional-Integral-Feedback signal, SATuration A:
	;; delta = ist-soll
	;; i_pl_sum += CI*delta
	;; z = CP*delta + i_pl_sum 
* --------------------------------------------------------------------------------
*/

void run_servo_timestep (SERVO *servo);
inline void run_servo_timestep (SERVO *servo){
	long long tmp;
	servo->i_sum = _SAT32 ((long)servo->i_sum + (long)( ((long long)servo->delta * (long long)servo->ci) >> (15+23)) );
	tmp = (long)((long)servo->i_sum + (((long long)servo->delta * (long long) servo->cp) >> (15+23)));
	// make both output polarities available
	servo->control = _SAT32 (tmp);
	servo->neg_control = _SAT32 (-tmp);
	servo->watch = 1;
}


/* generic analog out mixer used for most channels */
void compute_analog_out (int ch, OUT_MIXER *out);
inline void compute_analog_out (int ch, OUT_MIXER *out){
	// sub or smac B operation
	if (out->smac_b_fac_q15_p != NULL){
		out->s = *(out->p);
		out->s = _SMAC (out->s, *(out->sub_b_p),*(out->smac_b_fac_q15_p));
	} else {
		out->s = _SSUB32 (*(out->p), *(out->sub_b_p)); 
	}

	// add or smac A operation
	if (out->smac_a_fac_q15_p != NULL){
		out->s = _SMAC (out->s, *(out->add_a_p),*(out->smac_a_fac_q15_p));
	} else {
		out->s = _SADD32 (out->s, *(out->add_a_p)); 
	}
	// HR output
	HR_OUT (ch, out->s);
}

/* simple out mixer for coarse mode wave output to channel mixing -- 16bit aligned for wave !! */
void compute_analog_wave_out (int ch, OUT_MIXER *out);
inline void compute_analog_wave_out (int ch, OUT_MIXER *out){
	out->s = _SADD16 (*(out->add_a_p), *(out->p));
	AIC_OUT(ch) = out->s;
}

/* This is ISR is called on each new sample.  The "mode"/statevariable
 * should be initialized with AIC_OFFSET_COMPENSATION before starting
 * the AIC/DMA isr, this assures the correct offset initialization and
 * runs the automatic offset compensation on startup.
 */

void dataprocess();
void dataprocess()
{
        #ifdef DSPEMU
        static clock_t ccti, cctf, ccdt;
        #endif

	/* FEEDBACK */
	long long tmp40 = 0; // 40bit
	long long FB_mixer_delta = 0; // 40bit
	int i;

	/* Update Idle Time */
#ifndef DSPEMU
	TSCReadSecond ();
	DSP_MEMORY(state).IdleTime = MeasuredTime;
	TSCReadFirst ();
#else
        cctf = clock ();
        ccdt = cctf - ccti;
	DSP_MEMORY(state).IdleTime = ccdt;
        ccti = clock ();
#endif
        
// ============================================================
// PROCESS MODULE: DSP PROCESSING CLOCK TICKS -- looping
// ============================================================
	++DSP_MEMORY(state).DSP_time; // 150kHz ticks

// ============================================================
// PROCESS MODULE: PAC (Phase Amplitude Controller) 
//             and PLL (Phase Locked Loop)
// and recorder data/trigger handling if not via PLL
// ============================================================
#ifdef USE_PLL_API
        if (DSP_MEMORY(state).mode & MD_PLL){
		max_out_ch = 7;
		DataprocessPLL();

		// Amplitude Controller Patch to handle negative amplitude big step responses
		// most efficient and stable.
		// setpoint_Amp;   //   (2^29-1)/10 x AmpSet[V]
		// amp_estimation; //== "PLL Res Amp"  (2^22-1)/10 x Vol[V]
		// volumeSine;     //== "PLL Exci Amp" (2^22-1)/10 x Vol[V]
		if (DSP_MEMORY_PAC(volumeSine) < 0){
		        if (DSP_MEMORY_PAC(amp_estimation) <= (DSP_MEMORY_PAC(setpoint_Amp) >> 7)){
		                DSP_MEMORY_PAC(memI_Amp) = 0L;
			}
		}
		
	} else {
		max_out_ch = 8; // it's down to 7 when PLL is active -- else PLL output signal is overwritten
		if (DSP_MEMORY_PAC(blcklen) != -1){
			if (DSP_MEMORY(PLL_lookup).pflg == 0){ // ==0 : no trigger or go after trigger event
				DSP_MEMORY_PAC(Signal1[DSP_MEMORY_PAC(blcklen)]) = DSP_MEMORY_PAC(pSignal1[0]);
				DSP_MEMORY_PAC(Signal2[DSP_MEMORY_PAC(blcklen)]) = DSP_MEMORY_PAC(pSignal2[0]);
				DSP_MEMORY_PAC(blcklen)--;
			} else { // manage trigger -- ONLY IF PLL OFF AVAILABLE
				if (DSP_MEMORY(PLL_lookup).pflg > 0){ // pos trigger on level ".stop"
					if (DSP_MEMORY_PAC(pSignal1[0]) < (DSP_MEMORY(PLL_lookup).stop-2))
						DSP_MEMORY(PLL_lookup).pflg = 2;
					else if (DSP_MEMORY(PLL_lookup).pflg > 1)
						if (DSP_MEMORY_PAC(pSignal1[0]) > DSP_MEMORY(PLL_lookup).stop)
							DSP_MEMORY(PLL_lookup).pflg = 0; // go!
				} else { // else neg trigger on level ".stop"
					if (DSP_MEMORY_PAC(pSignal1[0]) > (DSP_MEMORY(PLL_lookup).stop+2))
						DSP_MEMORY(PLL_lookup).pflg = -2;
					else if (DSP_MEMORY(PLL_lookup).pflg < -1)
						if (DSP_MEMORY_PAC(pSignal1[0]) < DSP_MEMORY(PLL_lookup).stop)
							DSP_MEMORY(PLL_lookup).pflg = 0; // go!
				}
			}

		} 
	}
#endif

// ============================================================
// PROCESS MODULE: NOISE GENERATOR (RANDOM NUMBER)
// ============================================================
	// update noise source
	generate_nextlongrand ();
	DSP_MEMORY(analog).noise = randomnum;

// ============================================================
// PROCESS MODULE: ANALOG_IN
// ============================================================
	// copy AD inputs -- scale to 15.16
	for (i=0; i<8; ++i){
		DSP_MEMORY(analog).in[i] = AIC_IN(i) << 16;
	}
	// any differentials?
	for (i=0; i<4; ++i){
		DSP_MEMORY(analog).in[i] = _SSUB32 (DSP_MEMORY(analog).in[i], *DSP_MEMORY(analog).diff_in_p[i]);
	}

// =============================================================
// PROCESS MODULE: MIXER INPUT PROCESSING, IIR, adaptive on MIX0
// =============================================================
	// map data and run real time 4-channel IIR filter
	for (i=0; i<4; ++i){ // assign data / Q23 transfromations for ADCs
		if (DSP_MEMORY(feedback_mixer).FB_IN_is_analog_flg[i]){
			tmp40 = (*(DSP_MEMORY(feedback_mixer).input_p[i])) >> 16;    // make 16bit input (original analog in level)
			DSP_MEMORY(feedback_mixer).q_factor15 = DSP_MEMORY(feedback_mixer).iir_ca_q15[i]; // get q
			if (i==0){ // special adaptive IIR for MIX0 only
				if (DSP_MEMORY(feedback_mixer).I_cross > 0){
					AbsIn = _abs (tmp40);
					DSP_MEMORY(feedback_mixer).q_factor15 = _SSUB32 (Q15L, _SMAC (DSP_MEMORY(feedback_mixer).cb_Ic, AbsIn, Q15) / _SADD16 (AbsIn, DSP_MEMORY(feedback_mixer).I_cross));
					if (DSP_MEMORY(feedback_mixer).q_factor15 < DSP_MEMORY(feedback_mixer).iir_ca_q15[0])
						DSP_MEMORY(feedback_mixer).q_factor15 = DSP_MEMORY(feedback_mixer).iir_ca_q15[0];
				}
			}
			// compute IIR
			DSP_MEMORY(feedback_mixer).iir_signal[i] = _SMAC ( _SMPY32 ( _SSUB16 (Q15, DSP_MEMORY(feedback_mixer).q_factor15), tmp40), DSP_MEMORY(feedback_mixer).q_factor15, _SSHL32 (DSP_MEMORY(feedback_mixer).iir_signal[i], -16));
			DSP_MEMORY(feedback_mixer).FB_IN_processed[i] = DSP_MEMORY(feedback_mixer).iir_signal[i] >> 8;  // Q23.8 for MIXER
		} else
			DSP_MEMORY(feedback_mixer).FB_IN_processed[i] = *(DSP_MEMORY(feedback_mixer).input_p[i]); // about Q23.8 range -- assume equiv ~~~ AIC_IN(i) << 8;

		if (DSP_MEMORY(feedback_mixer).mode[i] & 0x10) // negate feedback source?
		        DSP_MEMORY(feedback_mixer).FB_IN_processed[i] *= -1;
	}

// ============================================================
// PROCESS MODULE: RMS -- average and rms computations via FIR
// ============================================================
	// Average and RMS computations
	d_tmp = *DSP_MEMORY(analog).avg_input >> 16; // 32bit (QS15.16 input) -- scale to fit sum 
	DSP_MEMORY(analog).avg_signal = _SADD32 (DSP_MEMORY(analog).avg_signal, d_tmp);
	DSP_MEMORY(analog).avg_signal = _SSUB32 (DSP_MEMORY(analog).avg_signal, rms_I_pipe[rms_pipi]);
	rms_I_pipe[rms_pipi] = d_tmp;
						
	tmpsum = _SSUB32 (d_tmp, _SSHL32 (DSP_MEMORY(analog).avg_signal, -RMS_N2));
	rmssum2 = tmpsum;
	rmssum2 *= rmssum2;
	rmssum  += rmssum2; 
	rmssum  -= rms_I2_pipe64[rms_pipi];
	DSP_MEMORY(analog).rms_signal = rmssum >> 32;
	rms_I2_pipe64[rms_pipi] = rmssum;

	// now tricky -- taking chances of mult OVR if RMS is too big, designed for read small noise with offset eliminated.
	// tmpsum = _SMPY32 (tmpsum, tmpsum);
	// DSP_MEMORY(analog).rms_signal = _SADD32 (DSP_MEMORY(analog).rms_signal, tmpsum);
	// DSP_MEMORY(analog).rms_signal = _SSUB32 (DSP_MEMORY(analog).rms_signal, rms_I2_pipe[rms_pipi]);
	// rms_I2_pipe[rms_pipi]  = tmpsum;
				
	if (++rms_pipi == RMS_N) 
		rms_pipi = 0;
	// RMS, AVG results normalizsation on host level!

	if (sigma_delta_index & 1){

// ============================================================
// PROCESS MODULE: COUNTER
//  QEP, GATE TIME CONTROL, COUNTER expansion
// ============================================================

#ifdef DSPEMU                
                DSP_MEMORY(analog).counter[0] = 0; // Counter/Timer support not yet available via FPGA
                DSP_MEMORY(analog).counter[1] = 0; // Counter/Timer support not yet available via FPGA
#else
#ifdef USE_ANALOG_16 // if special build for MK2-Analog_16
                DSP_MEMORY(analog).counter[0] = 0; // Counter/Timer support not yet available via FPGA
                DSP_MEMORY(analog).counter[1] = 0; // Counter/Timer support not yet available via FPGA
#else // default: MK2-A810

		/* Automatic Aligned Gateing by scan or probe process */
//		SUM_QEP_cnt_Diff[0]=SUM_QEP_cnt_Diff[0]+(int)(QEP_cnt[0]-QEP_cnt_old[0]);
		DSP_MEMORY(analog).counter[0]=DSP_MEMORY(analog).counter[0] + (unsigned short)(QEP_cnt[0]-QEP_cnt_old[0]); 

		QEP_cnt_old[0]=QEP_cnt[0];

//		SUM_QEP_cnt_Diff[1]=SUM_QEP_cnt_Diff[1]+(int)(QEP_cnt[1]-QEP_cnt_old[1]);
		DSP_MEMORY(analog).counter[1]=DSP_MEMORY(analog).counter[1] + (unsigned short)(QEP_cnt[1]-QEP_cnt_old[1]); 
		QEP_cnt_old[1]=QEP_cnt[1];      
			
		/* always handle Counter_1 -- 16it gatetime only (fast only, < 800ms @ 75 kHz) */
		if (++gate_cnt_1 >= CR_generic_io.gatetime_1){
			gate_cnt_1 = 0;
			CR_generic_io.count_1 = DSP_MEMORY(analog).counter[1];
			DSP_MEMORY(analog).counter[1] = 0;
			
			if (CR_generic_io.count_1 > CR_generic_io.peak_count_1) 
				CR_generic_io.peak_count_1 = CR_generic_io.count_1;
		}
			
		if (!(scan.pflg || DSP_MEMORY(probe).pflg) && CR_generic_io.pflg) {
			/* stand alone Rate Meter Mode else gating managed via spm_areascan or _probe module */
				
			/* handle Counter_0 -- 32it gatetime (very wide range) */
			if (gate_cnt_multiplier >= CR_generic_io.gatetime_h_0){
				if (++gate_cnt >= CR_generic_io.gatetime_l_0){
					gate_cnt = 0;
					gate_cnt_multiplier = 0;
						
					CR_generic_io.count_0 = DSP_MEMORY(analog).counter[0];
					DSP_MEMORY(analog).counter[0] = 0;
					
					if (CR_generic_io.count_0 > CR_generic_io.peak_count_0) 
						CR_generic_io.peak_count_0 = CR_generic_io.count_0;
				}
			} else { // this makes up a 32bit counter...
				if (++gate_cnt == 0)
					++gate_cnt_multiplier;
			}
		}
#endif
#endif
                
// ============================================================
// PROCESS MODULE: OFFSET MOVE
// ============================================================
		/* Offset Move task ? */
		if (DSP_MEMORY(move).pflg)
			run_offset_move ();

// ============================================================
// PROCESS MODULE: AREA SCAN
// ============================================================
		/* Area Scan task - normal mode ?
		 * the feedback task needs to be enabled to see the effect
                 * --> can set CI/CP to small values to "contineously" disable it!
                 */
                if (DSP_MEMORY(scan).pflg == (AREA_SCAN_RUN | AREA_SCAN_START_NORMAL))
                        if (!DSP_MEMORY(scan).raster_b || !DSP_MEMORY(probe).pflg) // pause scan if raster_b!=0 and probe is going.
                                run_area_scan ();

                // reset FB watch -- see below
                DSP_MEMORY(z_servo).watch = 0; // for actual watch fb activity

	} else {
                
// ============================================================
// PROCESS MODULE: VECTOR PROBE (VP)
// or if idle VP BIAS and Z-Adjusters to follow controls
// ============================================================
                /* Vector Probe task ?  (Bias may be changed here) */
                feedback_hold = 0;
                if (DSP_MEMORY(probe).pflg){
                        run_probe ();
                        if (DSP_MEMORY(probe).vector)
                                if (DSP_MEMORY(probe).vector->options & VP_FEEDBACK_HOLD)
                                        feedback_hold = 1;
                } else {
                        /* (re) Adjust Bias, Zpos ? */
                        if (DSP_MEMORY(analog).bias_adjust != DSP_MEMORY(analog).bias)
                                run_bias_adjust ();

			if ((DSP_MEMORY(state).mode & MD_ZPOS_ADJUSTER) && DSP_MEMORY(probe).Zpos != 0) // enable Zpos adjusting normally!
				run_Zpos_adjust ();

                        DSP_MEMORY(probe).Upos = DSP_MEMORY(analog).bias_adjust; // DSP_MEMORY(probe).Upos is default master for BIAS at all times, foolowing user control or VP comand
                }
        }

// ============================================================
// PROCESS MODULE: FULL BW: LockIn task
// ============================================================
	if (DSP_MEMORY(probe).state >= PROBE_RUN_LOCKIN_PROBE)
		run_lockin ();
	else
		DSP_MEMORY(probe).LockIn_ref = 0; // reset modulation

// ============================================================
// PROCESS MODULE: FAST SCAN
// ============================================================
	/* run FAST AREA SCAN (sinusodial) ? */
	if (DSP_MEMORY(scan).pflg == (AREA_SCAN_RUN | AREA_SCAN_START_FASTSCAN))
		run_area_scan_fast ();

	
// ============================================================
// PROCESS MODULES: 
//  MIXER DATA TRANSFORMATIONS and DELTA COMPUTATIONS
//  DELTA ACCUMULATION
// ============================================================
	/* FeedBack (FB) task ?  (evaluates several conditions and runs in various modes) */

	/** FB_IN_processed[0..3] ==> 4-CHANNEL MIXER MODULE ==> MIXER.delta **/
	/** FOLLOWED BY Z-SERVO -- normally main Z feedback **/

	if (!feedback_hold && sigma_delta_index & 1){ /* may be freezed by probing process */
		/* Let's check out, if the feedback loop is really running. */
		feedback_hold = 1; 

		/* run main Z-servo (feedback) control loop? */
		if (DSP_MEMORY(state).mode & MD_PID){
			if ( !(AS_ch2nd_constheight_enabled && DSP_MEMORY(scan).pflg)){ // skip in 2nd const height mode!
				feedback_hold = 0; 

						
				// Feedback Mixer -- data transform and delta computation, summing
				FB_mixer_delta = 0L; // Q23 x 16 (long long) -- can grow to 40+2bits total!

				for (i=0; i<4; ++i){
					tmp40 = 0L;
 					// process MIXER CHANNEL i
					switch (DSP_MEMORY(feedback_mixer).mode[i]&0x0f){
					case 3: // LOG
						// log result is Q23 (1 and 2^23-1) from Q23 input:
						DSP_MEMORY(feedback_mixer).lnx = calc_mix_log (DSP_MEMORY(feedback_mixer).FB_IN_processed[i], DSP_MEMORY(feedback_mixer).I_offset);
						tmp40 = (long long)(DSP_MEMORY(feedback_mixer).lnx - DSP_MEMORY(feedback_mixer).setpoint_log[i]);
						break;
					case 1: // LIN
						tmp40 = (long long)(DSP_MEMORY(feedback_mixer).FB_IN_processed[i] - DSP_MEMORY(feedback_mixer).setpoint[i]);
						break;
					case 9: // FUZZY
						if (DSP_MEMORY(feedback_mixer).FB_IN_processed[i] > DSP_MEMORY(feedback_mixer).level[i])
							tmp40 = (long long)(DSP_MEMORY(feedback_mixer).FB_IN_processed[i] - DSP_MEMORY(feedback_mixer).level[i] - DSP_MEMORY(feedback_mixer).setpoint[i]);
						break;
					case 11: // FUZZY LOG
					        if (DSP_MEMORY(feedback_mixer).FB_IN_processed[i] > DSP_MEMORY(feedback_mixer).level[i]){
						        DSP_MEMORY(feedback_mixer).lnx = calc_mix_log (DSP_MEMORY(feedback_mixer).FB_IN_processed[i] - DSP_MEMORY(feedback_mixer).level[i], 0);
							tmp40 = (long long)(DSP_MEMORY(feedback_mixer).lnx - DSP_MEMORY(feedback_mixer).setpoint[i]);
						}
						break;
					default: break; // OFF
					}
					DSP_MEMORY(feedback_mixer).channel[i] = _SAT32 (tmp40);
					FB_mixer_delta += tmp40 * (long long)DSP_MEMORY(feedback_mixer).gain[i];
				}

				DSP_MEMORY(feedback_mixer).delta = _SAT32 (FB_mixer_delta>>8); // Q23, SAT
	
				// **SIGNAL** :: default link is *z_servo.input == DSP_MEMORY(feedback_mixer).delta 

// ============================================================
// PROCESS MODULE: MAIN Z SERVO
// ============================================================
				DSP_MEMORY(z_servo).delta = _SAT32 ((long)*DSP_MEMORY(z_servo).input); // Q23 input and setpoint, SAT difference
				run_servo_timestep (&DSP_MEMORY(z_servo));
			}
		}
	}

// NOW OUTPUT HR SIGNALS ON XYZ-Offset and XYZ-Scan -- do not touch Bias OUT(6) and Motor OUT(7) here -- handled directly previously.
// note: OUT(0-5) get overridden below by coarse/mover actions if requeste!!!

/* HR sigma-delta data processing (if enabled) -- turn off via adjusting sigma_delta_hr_mask to all 0 */

// do scan coordinate rotation transformation:
        if ( !(DSP_MEMORY(state).mode & MD_XYSROT))
        {
                xy_vec[2] = xy_vec[0] = DSP_MEMORY(scan).xyz_vec[i_X];
                xy_vec[3] = xy_vec[1] = DSP_MEMORY(scan).xyz_vec[i_Y];
                mul32 (xy_vec, DSP_MEMORY(scan).rotm, result_vec, 4);
                DSP_MEMORY(scan).xy_r_vec[i_X] = _SADD32 (result_vec[0], result_vec[1]);
                DSP_MEMORY(scan).xy_r_vec[i_Y] = _SADD32 (result_vec[2], result_vec[3]);
        } else {
                DSP_MEMORY(scan).xy_r_vec[i_X] = DSP_MEMORY(scan).xyz_vec[i_X];
                DSP_MEMORY(scan).xy_r_vec[i_Y] = DSP_MEMORY(scan).xyz_vec[i_Y];
        }

// XY-Offset and XY-Scan output -- calculates Scan XY output and added offset as configured
// default: HR_OUT[3,4] = DSP_MEMORY(scan).xy_r_vec + move.xyz_vec
//** done below in one shot loop **
//**	compute_analog_out (3, &DSP_MEMORY(analog).out[3]);
//**	compute_analog_out (4, &DSP_MEMORY(analog).out[4]);

// PROCESS MODULE: SLOPE-COMPENSATION
// ==================================================
// Z-Offset -- slope compensation output
//---- slope add X*mx + Y*my
// limit dz add from xy-mult to say 10x DSP_MEMORY(scan).fm_dz0x+y, feedback like adjust if "diff" to far off from sudden slope change 
//      zpos_xymult = move.ZPos + DSP_MEMORY(scan).XposR * DSP_MEMORY(scan).fm_dz0x +  DSP_MEMORY(scan).YposR * DSP_MEMORY(scan).fm_dz0y ;
// make sure a smooth adjust -- if slope parameters get changed, need to prevent a jump.

	mul32 (DSP_MEMORY(scan).xy_r_vec, DSP_MEMORY(scan).fm_dz0_xy_vec, result_vec, 2);
	s_xymult = _SADD32 (result_vec[i_X], result_vec[i_Y]);
        
	d_tmp = _SSUB32 (s_xymult, DSP_MEMORY(scan).z_offset_xyslope);
	if (d_tmp > DSP_MEMORY(scan).z_slope_max) // limit up
		DSP_MEMORY(scan).z_offset_xyslope = _SADD32 (DSP_MEMORY(scan).z_offset_xyslope, DSP_MEMORY(scan).z_slope_max);
	else if (d_tmp < -DSP_MEMORY(scan).z_slope_max) // limit dn
		DSP_MEMORY(scan).z_offset_xyslope = _SADD32 (DSP_MEMORY(scan).z_offset_xyslope, -DSP_MEMORY(scan).z_slope_max);
	else DSP_MEMORY(scan).z_offset_xyslope = s_xymult; // normally this should do it


// ============================================================
// PROCESS MODULE: MOTOR SERVO
// ============================================================

	DSP_MEMORY(m_servo).delta = _SAT32 ((long)DSP_MEMORY(m_servo).setpoint - (long)*DSP_MEMORY(m_servo).input); // Q23 input and setpoint, SAT difference
        run_servo_timestep (&DSP_MEMORY(m_servo));

// ============================================================
// PROCESS MODULE: SCO (Signal Controller Oszillator)
// ============================================================
	if (DSP_MEMORY(sco_s[0]).in)
                run_sco (&DSP_MEMORY(sco_s[0]));

        if (DSP_MEMORY(sco_s[1]).in)
                run_sco (&DSP_MEMORY(sco_s[1]));

        
// PROCESS MODULE: ANALOG OUTPUT MIXER
// ==================================================

// simply process all output channels now in same manner
// ** max_out_ch is 8 for all, must be 7 to not override PAC/PLL output set to CH7 fixed if PLL processing is on.
	for (i=0; i<max_out_ch; ++i)
		compute_analog_out (i, &DSP_MEMORY(analog).out[i]);

//** --- here is what the default means in expanded form: ---

// DEFAULT: Z SERVO (FEEDBACK) OUT -- calculate Z-Scan output:
// **SIGNAL** :: default link is *DSP_MEMORY(analog).out_p[5] == z_servo.neg_control   SUB: -DSP_MEMORY(probe).Zpos  ADD: +NULL

//**	compute_analog_out (5, &DSP_MEMORY(analog).out[5]);

// DEFAULT: BIAS_ADJUST OUT -- calculate Bias output:
// **SIGNAL** :: default link is *DSP_MEMORY(analog).out_p[6] == DSP_MEMORY(probe).Upos    SUB: -NULL  ADD: +NULL
// ** probe and lockin mod/ref is hooking in here!

//**	compute_analog_out (6, &DSP_MEMORY(analog).out[6]);

// DEFAULT: MOTOR OUT -- calculate Motor output:
// **SIGNAL** :: default link is *DSP_MEMORY(analog).out_p[7] == DSP_MEMORY(analog).motor    SUB: -NULL  ADD: +NULL

//**	compute_analog_out (7, &DSP_MEMORY(analog).out[7]);

// ========== END OUTPUT ANALOG DATA [except coase mover signal overrides later] ==========

        ++sigma_delta_index;
        sigma_delta_index &= 7;

// ========== END HR processing ================



// PROCESS MODULE: COARSE --- WAVE and PULSE out OVERRIDES if active!!
// ======================================================================
	/* Run Autoapproch/Movercontrol task ? */
        if (DSP_MEMORY(autoapp).pflg){
                if (DSP_MEMORY(autoapp).pflg==2)
                        DSP_MEMORY(autoapp).pflg = 0;
                else
                        run_autoapp ();
		
		if (DSP_MEMORY(autoapp).mover_mode & (AAP_MOVER_XYOFFSET | AAP_MOVER_XYSCAN |  AAP_MOVER_XXOFFSET | AAP_MOVER_XYMOTOR | AAP_MOVER_ZSCANADD)){
			// wave output computation
			compute_analog_wave_out (DSP_MEMORY(autoapp).wave_out_channel[0], &DSP_MEMORY(analog).out[8]);
			compute_analog_wave_out (DSP_MEMORY(autoapp).wave_out_channel[1], &DSP_MEMORY(analog).out[9]);
		}
        } else 
                /* Run CoolRunner Out puls(es) task ? */
                if (DSP_MEMORY(CR_out_pulse).pflg)
                        run_CR_out_pulse ();

	
// ============================================================
// PROCESS MODULE: SIGNAL MONITOR data copy processing
// ============================================================

	for (i=0; i<NUM_MONITOR_SIGNALS; ++i)
		DSP_MEMORY(sig_mon).signal[i] = *DSP_MEMORY(sig_mon).signal_p[i];

 
// ************************************************************
// END OF DATA PROCESSING TASKS -- final time keeping
// ************************************************************

	/* -- end of all data processing, preformance statistics update now -- */
	
//-****	asm_read_time ();

	/* Update Dataprocess Time */
#ifndef DSPEMU
	TSCReadSecond();
	DSP_MEMORY(state).DataProcessTime = MeasuredTime;
	TSCReadFirst();
#else
        cctf = clock ();
        ccdt = cctf - ccti;
	DSP_MEMORY(state).DataProcessTime = ccdt;
        ccti = clock ();
#endif

	/* Load Peak Detection */
	if (_abs(DSP_MEMORY(state).DataProcessTime) > _abs(DSP_MEMORY(state).DataProcessTime_Peak)){
                DSP_MEMORY(state).DataProcessTime_Peak = DSP_MEMORY(state).DataProcessTime;
		DSP_MEMORY(state).IdleTime_Peak = DSP_MEMORY(state).IdleTime;
	}
}


/* END */
