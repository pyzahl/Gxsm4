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
#include "mcbsp_support.h"


// "old" mixer channel mode on level -- in a FUZZZY way adding to control error signal
#define FUZZYLOGIC_MIXER_MODE


// 8x digital sigma-delta over sampling using bit0 of 16 -> gain of 2 bits resoultion (16+2=18)
#define SIGMA_DELTA_LEN   8
int     sigma_delta_index = 0;

#ifdef MIN_PAC_BUILD
int	max_out_ch = 7; // it's down to 7 when PLL is active -- else PLL output signal is overwritten
#else
int	max_out_ch = 8; // it's down to 7 when PLL is active -- else PLL output signal is overwritten
#endif

DSP_INT32  s_xymult = 0;
// DSP_INT32  zpos_xymult = 0;
DSP_INT32  d_tmp=0;
DSP_INT32  tmp_hrbits = 0;

DSP_UINT8  recorder_decc = 0; // decimation count
DSP_UINT32 recorder_deci = 0; // decimation data index
DSP_INT40  recorder_dec_sum = 0L;
DSP_INT32  recorder_s1_buffer[0x100] = {
        //   16       32                 64                                   128                                                                        256
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0
};

//#define SIGMA_DELTA_HR_MASK_FAST
#ifdef SIGMA_DELTA_HR_MASK_FAST  // Fast HR Matrix -- good idea, but performance/distorsion issues with the A810 due to the high dynamic demands
int     sigma_delta_hr_mask[SIGMA_DELTA_LEN * SIGMA_DELTA_LEN] = { 
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
int     sigma_delta_hr_mask[SIGMA_DELTA_LEN * SIGMA_DELTA_LEN] = { 
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
int       pipi = 0;       /* Pipe index */
int       scnt = 0;       /* Slow summing count */

/* IIR tmp */

DSP_INT32   Q30   = 1073741823L;
DSP_INT32   Q15L   = 32767L;
DSP_INT32   Q15    = 32767;
DSP_INT32   AbsIn  = 0;

DSP_INT32 blcklen_mem = 0;

/* externals of SPM control */

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
extern DATA_SYNC_IO     data_sync_io;

extern int AS_ch2nd_constheight_enabled; /* const H mode flg of areascan process */
extern struct aicregdef    aicreg;

extern DSP_INT32 PRB_ACPhaseA32;

DSP_INT32 xy_vec[4];
DSP_INT32 mm_vec[4];
DSP_INT32 result_vec[4];


#define USE_MUL32C
#ifdef USE_MUL32C 
// multiplication of 2x 32bit Q31 elements

// The overhead is now 38 cycles and you can use this formula to compute the total number of cycle:
// 38+nx*1.5 cycles. The code size if now 26 instructions instead of 20 for the version I sent to you this morning.

void copy32(int *restrict x, int *restrict r, unsigned short nx)
{
   int i;
   _nassert((int) nx % 2 == 0);
   _nassert((int) nx >= 2);
  
   for (i=0; i<nx; i += 2)
   {
           r[i] = x[i];
           r[i+1] = x[i+1];
   }  
}

unsigned int mul32(int *restrict x, int *restrict y,  int *restrict r, unsigned short nx)
{
   int i;
   _nassert((int) nx % 2 == 0);
   _nassert((int) nx >= 2);
  
   for (i=0; i<nx; i += 2)
   {
       r[i] =(int)(((long long)(x[i]) * (long long)(y[i])) >> 31);
       r[i+1] =(int)(((long long)(x[i+1]) * (long long)(y[i+1])) >> 31);
   }  
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

/* run SCO on signal */
#define CPN31 2147483647
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

inline void run_servo_timestep (SERVO *servo){
	long long tmp;
	servo->i_sum = _SAT32 ((long)servo->i_sum + (long)( ((long long)servo->delta * (long long)servo->ci) >> (15+23)) );
	tmp = (long)((long)servo->i_sum + (((long long)servo->delta * (long long) servo->cp) >> (15+23)));
	// make both output polarities available
	servo->control = _SAT32 (tmp);
	servo->neg_control = _SAT32 (-tmp);
	servo->watch = 1;
}

/* --> moved to areascan
inline void run_servo_manual (SERVO *servo, DSP_INT32 set){
	long long tmp;
        servo->i_sum = set;
	tmp = set;
	// make both output polarities available
	servo->control = _SAT32 (tmp);
	servo->neg_control = _SAT32 (-tmp);
}
*/

/* generic analog out mixer used for most channels */
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
inline void compute_analog_wave_out (int ch, OUT_MIXER *out){
	out->s = _SADD16 (*(out->add_a_p)>>16, *(out->p));
	AIC_OUT(ch) = out->s;
}

inline void check_trigger_levels(){
#if 0 // running very low on DSP FAST RAM space
        if (PLL_lookup.pflg > 0){ // pos trigger on level ".stop"
                if (pSignal1[0] < (PLL_lookup.stop-2))
                        PLL_lookup.pflg = 2;
                else if (PLL_lookup.pflg > 1)
                        if (pSignal1[0] > PLL_lookup.stop)
                                PLL_lookup.pflg = 0; // go!
        } else { // else neg trigger on level ".stop"
                if (pSignal1[0] > (PLL_lookup.stop+2))
                        PLL_lookup.pflg = -2;
                else if (PLL_lookup.pflg < -1)
                        if (pSignal1[0] < PLL_lookup.stop)
                                PLL_lookup.pflg = 0; // go!
        }
#else // only pos trigger
        if (pSignal1[0] < (PLL_lookup.stop-2))
                PLL_lookup.pflg = 2;
        else if (PLL_lookup.pflg > 1)
                if (pSignal1[0] > PLL_lookup.stop)
                                PLL_lookup.pflg = 0; // go!
#endif
        if (!PLL_lookup.pflg && PLL_lookup.blcklen_trigger != -1)
                blcklen = PLL_lookup.blcklen_trigger; // now activate recording!
}

// Data Processing Tasks  dp_task_001..008(void)
//===========================================================
// scheduled in data process
// executed in idle loop with 01 = highest priority down to 08

int dp_task_001(void){
// ============================================================
// PROCESS MODULE: PAC (Phase Amplitude Controller) 
//             and PLL (Phase Locked Loop)
// and recorder data/trigger handling if not via PLL
// ============================================================
#ifdef MIN_PAC_BUILD
        int i;
	long long tmp40 = 0; // 40bit

        max_out_ch = 7;

        DataprocessPLL();
        if (volumeSine < 0){
                if (amp_estimation <= (setpoint_Amp >> 7)){
                        memI_Amp = 0L;
                }
        }

        analog.in[0] = AIC_IN(0) << 16;
        analog.in[1] = AIC_IN(1) << 16;
        analog.in[2] = AIC_IN(2) << 16;
        analog.in[3] = AIC_IN(3) << 16;
        analog.in[4] = AIC_IN(4) << 16;
        analog.in[5] = AIC_IN(5) << 16;
        analog.in[6] = AIC_IN(6) << 16;
        analog.in[7] = AIC_IN(7) << 16;

// =============================================================
// PROCESS MODULE: MIXER INPUT PROCESSING, IIR, adaptive on MIX0
// =============================================================
	// map data channels and run real time 4-channel IIR filter
	for (i=0; i<4; ++i){ // assign data / Q23 transfromations for ADCs
		if (feedback_mixer.FB_IN_is_analog_flg[i]){
			tmp40 = (*(feedback_mixer.input_p[i])) >> 16;    // make 16bit input (original analog in level)
			feedback_mixer.q_factor15 = feedback_mixer.iir_ca_q15[i]; // get q
			// compute IIR
			feedback_mixer.iir_signal[i] = _SMAC ( _SMPY32 ( _SSUB16 (Q15, feedback_mixer.q_factor15), tmp40), feedback_mixer.q_factor15, _SSHL32 (feedback_mixer.iir_signal[i], -16));
			feedback_mixer.FB_IN_processed[i] = feedback_mixer.iir_signal[i] >> 8;  // Q23.8 for MIXER
		} else
			feedback_mixer.FB_IN_processed[i] = *(feedback_mixer.input_p[i]); // about Q23.8 range -- assume equiv ~~~ AIC_IN(i) << 8;

		if (feedback_mixer.mode[i] & 0x10) // negate feedback source?
		        feedback_mixer.FB_IN_processed[i] *= -1;
	}

#else
#ifdef USE_PLL_API
        // RUN ALWAYS
        int i;
	long long tmp40 = 0; // 40bit
        if (state.mode & MD_PLL){
		max_out_ch = 7;
		if (PLL_lookup.blcklen_trigger != -1){
                        check_trigger_levels();
                }
		DataprocessPLL();

		// Amplitude Controller Patch to handle negative amplitude big step responses
		// most efficient and stable.
		// setpoint_Amp;   //   (2^29-1)/10 x AmpSet[V]
		// amp_estimation; //== "PLL Res Amp"  (2^22-1)/10 x Vol[V]
		// volumeSine;     //== "PLL Exci Amp" (2^22-1)/10 x Vol[V]
		if (volumeSine < 0){
		        if (amp_estimation <= (setpoint_Amp >> 7)){
		                memI_Amp = 0L;
			}
		}
		
	} else {
		max_out_ch = 8; // it's down to 7 when PLL is active -- else PLL output signal is overwritten
		if (blcklen != -1 || PLL_lookup.blcklen_trigger != -1){
			if (PLL_lookup.pflg == 0){ // ==0 : no trigger or go after trigger event
				Signal1[blcklen] = pSignal1[0];
				Signal2[blcklen] = pSignal2[0];
				blcklen--;
			} else { // manage trigger -- ONLY IF PLL OFF AVAILABLE
                                check_trigger_levels();
                        }
                }
                // decimating data ring buffer: recorder_decc is 8 bit uchar and thus loops at 255++ to 0
                recorder_s1_buffer[recorder_decc] = (DSP_INT32)pSignal1[0];
                recorder_decc++;
                recorder_dec_sum += (DSP_INT32)pSignal1[0];
                recorder_dec_sum -= recorder_s1_buffer[recorder_decc];
                if (recorder_decc==0){
                        Signal1[0x80000+recorder_deci] = recorder_dec_sum >> 8; // CIC, normalise by 256 (decc buffer sum len)
                        Signal2[0x80000+recorder_deci] = pSignal2[0];
                        Signal1[0x7ffff]=recorder_deci;
                        recorder_deci++;
                        recorder_deci &= 0x3ffff;
                }
	}
#endif

// ============================================================
// PROCESS MODULE: ANALOG_IN
// ============================================================
	// copy AD inputs -- scale to 15.16
	for (i=0; i<8; ++i){
		analog.in[i] = AIC_IN(i) << 16;
	}
	// any differentials?
	for (i=0; i<4; ++i){
		analog.in[i] = _SSUB32 (analog.in[i], *analog.diff_in_p[i]);
	}

// =============================================================
// PROCESS MODULE: MIXER INPUT PROCESSING, IIR, adaptive on MIX0
// =============================================================
	// map data channels and run real time 4-channel IIR filter
	for (i=0; i<4; ++i){ // assign data / Q23 transfromations for ADCs
		if (feedback_mixer.FB_IN_is_analog_flg[i]){
			tmp40 = (*(feedback_mixer.input_p[i])) >> 16;    // make 16bit input (original analog in level)
			feedback_mixer.q_factor15 = feedback_mixer.iir_ca_q15[i]; // get q
			if (i==0){ // special adaptive IIR for MIX0 only
				if (feedback_mixer.I_cross > 0){
					AbsIn = abs (tmp40);
					feedback_mixer.q_factor15 = _SSUB32 (Q15L, _SMAC (feedback_mixer.cb_Ic, AbsIn, Q15) / _SADD16 (AbsIn, feedback_mixer.I_cross));
					if (feedback_mixer.q_factor15 < feedback_mixer.iir_ca_q15[0])
						feedback_mixer.q_factor15 = feedback_mixer.iir_ca_q15[0];
				}
			}
			// compute IIR
			feedback_mixer.iir_signal[i] = _SMAC ( _SMPY32 ( _SSUB16 (Q15, feedback_mixer.q_factor15), tmp40), feedback_mixer.q_factor15, _SSHL32 (feedback_mixer.iir_signal[i], -16));
			feedback_mixer.FB_IN_processed[i] = feedback_mixer.iir_signal[i] >> 8;  // Q23.8 for MIXER
		} else
			feedback_mixer.FB_IN_processed[i] = *(feedback_mixer.input_p[i]); // about Q23.8 range -- assume equiv ~~~ AIC_IN(i) << 8;

		if (feedback_mixer.mode[i] & 0x10) // negate feedback source?
		        feedback_mixer.FB_IN_processed[i] *= -1;
	}
#endif
        return 0;
}

int dp_task_002(void){
// ============================================================
// PROCESS MODULE: COUNTER
// QEP, GATE TIME CONTROL, COUNTER expansion
// ============================================================

#ifdef USE_ANALOG_16 // if special build for MK2-Analog_16
        analog.counter[0] = 0; // Counter/Timer support not yet available via FPGA
        analog.counter[1] = 0; // Counter/Timer support not yet available via FPGA
#else // default: MK2-A810

        /* Automatic Aligned Gateing by scan or probe process */
        //		SUM_QEP_cnt_Diff[0]=SUM_QEP_cnt_Diff[0]+(int)(QEP_cnt[0]-QEP_cnt_old[0]);
        analog.counter[0]=analog.counter[0] + (unsigned short)(QEP_cnt[0]-QEP_cnt_old[0]); 

        QEP_cnt_old[0]=QEP_cnt[0];

        //		SUM_QEP_cnt_Diff[1]=SUM_QEP_cnt_Diff[1]+(int)(QEP_cnt[1]-QEP_cnt_old[1]);
        analog.counter[1]=analog.counter[1] + (unsigned short)(QEP_cnt[1]-QEP_cnt_old[1]); 
        QEP_cnt_old[1]=QEP_cnt[1];      
			
        /* always handle Counter_1 -- 16it gatetime only (fast only, < 800ms @ 75 kHz) */
        if (++gate_cnt_1 >= CR_generic_io.gatetime_1){
                gate_cnt_1 = 0;
                CR_generic_io.count_1 = analog.counter[1];
                analog.counter[1] = 0;
			
                if (CR_generic_io.count_1 > CR_generic_io.peak_count_1) 
                        CR_generic_io.peak_count_1 = CR_generic_io.count_1;
        }
			
        if (!(scan.pflg || probe.pflg) && CR_generic_io.pflg) {
                /* stand alone Rate Meter Mode else gating managed via spm_areascan or _probe module */
				
                /* handle Counter_0 -- 32it gatetime (very wide range) */
                if (gate_cnt_multiplier >= CR_generic_io.gatetime_h_0){
                        if (++gate_cnt >= CR_generic_io.gatetime_l_0){
                                gate_cnt = 0;
                                gate_cnt_multiplier = 0;
						
                                CR_generic_io.count_0 = analog.counter[0];
                                analog.counter[0] = 0;
					
                                if (CR_generic_io.count_0 > CR_generic_io.peak_count_0) 
                                        CR_generic_io.peak_count_0 = CR_generic_io.count_0;
                        }
                } else { // this makes up a 32bit counter...
                        if (++gate_cnt == 0)
                                ++gate_cnt_multiplier;
                }
        }
#endif
        return 0;
}

int dp_task_003(void){
        /* FEEDBACK */
	long long tmp40 = 0; // 40bit
	long long FB_mixer_delta = 0; // 40bit
	int i;
        
        // ============================================================
        // feedback and area scan in
        // ODD cycle
        // ============================================================
                        
        // ============================================================
        // if (sigma_delta_index & 1){}
        // PROCESS MODULES: 
        //  MIXER DATA TRANSFORMATIONS and DELTA COMPUTATIONS
        //  DELTA ACCUMULATION
        // ============================================================
        /* FeedBack (FB) task ?  (evaluates several conditions and runs in various modes) */

        /** FB_IN_processed[0..3] ==> 4-CHANNEL MIXER MODULE ==> MIXER.delta **/
        /** FOLLOWED BY Z-SERVO -- normally main Z feedback **/

        // reset FB watch -- see below
        z_servo.watch = 0; // for actual watch fb activity

        if ( !(AS_ch2nd_constheight_enabled && scan.pflg)){ // skip in 2nd const height mode!
						
                // Feedback Mixer -- data transform and delta computation, summing
                FB_mixer_delta = 0L; // Q23 x 16 (long long) -- can grow to 40+2bits total!

                for (i=0; i<4; ++i){
                        tmp40 = 0L;
                        // process MIXER CHANNEL i
                        /* +++ NOTE +++ these FLAGS are a critical subject to be reorganized in near future
                           //                  MASK     0   1
                          #define MM_ON        0x01  // OFF/ON
                          #define MM_LOG       0x02  // LIN/LOG
                          #define MM_LV_FUZZY  0x04  // NORMAL/FUZZY-LEVEL ZERO <-> NORMAL
                          #define MM_CZ_FUZCZY 0x08  // NORMAL/FUZZY-LEVEL CZ
                          #define MM_NEG       0x10  // NEGATE SOURCE (INPUT)
                          // OLD               ID
                          //#define MM_OFF     0x00  // OFF
                          //#define MM_ON      0x01  // ON/OFF
                          //#define MM_LOG     0x02  // LIN/LOG
                          //#define MM_IIR     0x04  // IIR/FULL
                          //#define MM_FUZZY   0x08  // FUZZY/NORMAL
                          //#define MM_NEG     0x10  // NEGATE SOURCE (INPUT)
                        */
                        switch (feedback_mixer.mode[i]&0x0f){
                        case 3: // LOG
                                // log result is Q23 (1 and 2^23-1) from Q23 input:
                                feedback_mixer.lnx = calc_mix_log (feedback_mixer.FB_IN_processed[i], feedback_mixer.I_offset);
                                tmp40 = (long long)(feedback_mixer.lnx - feedback_mixer.setpoint_log[i]);
                                break;
                        case 1: // LIN
                                tmp40 = (long long)(feedback_mixer.FB_IN_processed[i] - feedback_mixer.setpoint[i]);
                                break;
//#ifdef FUZZYLOGIC_MIXER_MODE
                        case 5: // FUZZY [9*]
                                if (feedback_mixer.FB_IN_processed[i] > feedback_mixer.level[i]){
                                        tmp40 = (long long)(feedback_mixer.FB_IN_processed[i] - feedback_mixer.setpoint[i]);
                                }
                                break;
#if 0                                
                        case 7: // FUZZY LOG [11*]
                                if (abs (feedback_mixer.FB_IN_processed[i]) > feedback_mixer.level[i]){
                                        feedback_mixer.lnx = calc_mix_log (feedback_mixer.FB_IN_processed[i], feedback_mixer.I_offset);
                                        tmp40 = (long long)(feedback_mixer.lnx - feedback_mixer.setpoint_log[i]);
                                }
                                break;
#endif
//#else // new default FUZZY Z-Pos Control

                        case 9: // CZ FUZZY LIN [9]
                                if (feedback_mixer.FB_IN_processed[i] > feedback_mixer.level[i]){
                                        //tmp40 = (long long)(feedback_mixer.FB_IN_processed[i] - feedback_mixer.level[i] - feedback_mixer.setpoint[i]);
                                        tmp40 = (long long)(feedback_mixer.FB_IN_processed[i] - feedback_mixer.setpoint[i]);
                                } else {
                                        tmp40 = (long long)(z_servo.neg_control) - (long long)(feedback_mixer.Z_setpoint);
                                }
                                break;
                        case 11: // CZ FUZZY LOG [11]
                                if (abs (feedback_mixer.FB_IN_processed[i]) > feedback_mixer.level[i]){
                                        feedback_mixer.lnx = calc_mix_log (feedback_mixer.FB_IN_processed[i], feedback_mixer.I_offset);
                                        tmp40 = (long long)(feedback_mixer.lnx - feedback_mixer.setpoint_log[i]);
                                } else {
                                        tmp40 = (long long)(z_servo.neg_control) - (long long)(feedback_mixer.Z_setpoint);
                                }
                                break;
//#endif
                        default: break; // OFF
                        }
                        feedback_mixer.channel[i] = _SAT32 (tmp40);
                        FB_mixer_delta += tmp40 * (long long)feedback_mixer.gain[i];
                }

                feedback_mixer.delta = _SAT32 (FB_mixer_delta>>8); // Q23, SAT
	
                // **SIGNAL** :: default link is *z_servo.input == feedback_mixer.delta 

                // ============================================================
                // PROCESS MODULE: MAIN Z SERVO
                // ============================================================
                z_servo.delta = _SAT32 ((long)*z_servo.input); // Q23 input and setpoint, SAT difference
                run_servo_timestep (&z_servo);
        }
        
        return 0;
}

int dp_task_004(void){
#ifdef MIN_PAC_BUILD
        /* ==> IDLE TASK 001 along with OFFSET MOVE */
#else
        // ============================================================
        // PROCESS MODULE: AREA SCAN
        // ============================================================
        /* Area Scan task - normal mode ?
         * the feedback task needs to be enabled to see the effect
         * --> can set CI/CP to small values to "contineously" disable it!
         */
        if (scan.pflg & (AREA_SCAN_RUN | AREA_SCAN_MOVE_TIP))
                if (!probe.pflg || probe.start) // pause scan if raster_b!=0 and probe is going.
                        run_area_scan ();

        // ============================================================
        // PROCESS MODULE: FAST SCAN
        // ============================================================

        /* run FAST AREA SCAN (sinusodial) ? */
        if (scan.pflg & AREA_SCAN_RUN_FAST)
                run_area_scan_fast ();

        // ============================================================
        // do expensive 32bit precision (tmp64/40) rotation in
        // EVEN cycle
        // ============================================================
                
        // NOW OUTPUT HR SIGNALS ON XYZ-Offset and XYZ-Scan -- do not touch Bias OUT(6) and Motor OUT(7) here -- handled directly previously.
        // note: OUT(0-5) get overridden below by coarse/mover actions if requeste!!!

        /* HR sigma-delta data processing (if enabled) -- turn off via adjusting sigma_delta_hr_mask to all 0 */

        // do scan coordinate rotation transformation:
        if ( !(state.mode & MD_XYSROT)){
                xy_vec[2] = xy_vec[0] = scan.xyz_vec[i_X];
                xy_vec[3] = xy_vec[1] = scan.xyz_vec[i_Y];
                mul32 (xy_vec, scan.rotmatrix, result_vec, 4);
                scan.xy_r_vec[i_X] = _SADD32 (result_vec[0], result_vec[1]);
                scan.xy_r_vec[i_Y] = _SADD32 (result_vec[2], result_vec[3]);
        } else {
                scan.xy_r_vec[i_X] = scan.xyz_vec[i_X];
                scan.xy_r_vec[i_Y] = scan.xyz_vec[i_Y];
        }

        // XY-Offset and XY-Scan output -- calculates Scan XY output and added offset as configured
        // default: HR_OUT[3,4] = scan.xy_r_vec + move.xyz_vec
        //** done below in one shot loop **
        //**	compute_analog_out (3, &analog.out[3]);
        //**	compute_analog_out (4, &analog.out[4]);

        // PROCESS MODULE: SLOPE-COMPENSATION
        // ==================================================
        // Z-Offset -- slope compensation output
        //---- slope add X*mx + Y*my
        // limit dz add from xy-mult to say 10x scan.fm_dz0x+y, feedback like adjust if "diff" to far off from sudden slope change 
        //      zpos_xymult = move.ZPos + scan.XposR * scan.fm_dz0x +  scan.YposR * scan.fm_dz0y ;
        // make sure a smooth adjust -- if slope parameters get changed, need to prevent a jump.

        mul32 (scan.xy_r_vec, scan.fm_dz0_xy_vec, result_vec, 2);
        s_xymult = _SADD32 (result_vec[i_X], result_vec[i_Y]);
        
        d_tmp = _SSUB32 (s_xymult, scan.z_offset_xyslope);
        if (d_tmp > scan.z_slope_max) // limit up
                scan.z_offset_xyslope = _SADD32 (scan.z_offset_xyslope, scan.z_slope_max);
        else if (d_tmp < -scan.z_slope_max) // limit dn
                scan.z_offset_xyslope = _SADD32 (scan.z_offset_xyslope, -scan.z_slope_max);
        else scan.z_offset_xyslope = s_xymult; // normally this should do it
#endif
        return 0;
}

int dp_task_005(void){
// ============================================================
// PROCESS MODULE: VECTOR PROBE (VP)
// or if idle VP BIAS and Z-Adjusters to follow controls
// ============================================================
        /* Vector Probe task ?  (Bias may be changed here) */
        if (probe.pflg)
                run_probe ();
        return 0;
}

int dp_task_006(void){
// ============================================================
// PROCESS MODULE: FULL BW: LockIn task
// ============================================================
        run_lockin ();
#if 0
	if (probe.state >= PROBE_RUN_LOCKIN_PROBE)
		run_lockin ();
	else
		probe.LockIn_ref = 0; // reset modulation
#endif
        return 0;
}

int dp_task_007(void){
        int i;
// ==================================================
// NO MORE TIME OR PHASE CRITICAL ANALOG OUTPUT
// ADJUSTMENTS PAST dp_task_006
// ==================================================

// PROCESS MODULE: ANALOG OUTPUT MIXER
// ==================================================

// simply process all output channels now in same manner
// ** max_out_ch is 8 for all, must be 7 to not override PAC/PLL output set to CH7 fixed if PLL processing is on.
	for (i=0; i<max_out_ch; ++i)
		compute_analog_out (i, &analog.out[i]);

// ========== END OUTPUT ANALOG DATA [except coase mover signal overrides later] ==========

        sigma_delta_index = (++sigma_delta_index) & 7;

// ========== END HR processing ================
        return 0;
}

int dp_task_008(void){
        int i,k;

// PROCESS MODULE: COARSE --- WAVE and PULSE out OVERRIDES if active!!
// ======================================================================
	/* Run Autoapproch/Movercontrol task ? */
        if (autoapp.pflg){
                run_autoapp ();
		
		if (autoapp.mover_mode & AAP_MOVER_WAVE_PLAY){
                        for (i=0; i<autoapp.n_wave_channels; ++i){
                                k = autoapp.channel_mapping[i] & 7;

                                // wave output computation
                                if (autoapp.channel_mapping[i] & AAP_MOVER_SIGNAL_ADD)
                                        AIC_OUT (k) = _sadd (AIC_OUT (k), *(analog.out[8+k].p));
                                else
                                        compute_analog_wave_out (k, &analog.out[8+k]);

                                // --- adding is to be configure via signals for MK3!
                                //if (autoapp.channel_mapping[i] & AAP_MOVER_SIGNAL_ADD)
                                //        AIC_OUT (k) = _sadd (AIC_OUT (k), analog.out[k]);
                                //else
                                //        AIC_OUT (k) = analog.out[k];
                        }

			// wave output computation
			//compute_analog_wave_out (autoapp.wave_out_channel[0], &analog.out[8]);
			//compute_analog_wave_out (autoapp.wave_out_channel[1], &analog.out[9]);
		}
        } else 
                /* Run CoolRunner Out puls(es) task ? */
                if (CR_out_pulse.pflg)
                        run_CR_out_pulse ();

        return 0;
}

int dp_task_009(void){
        static int dec=8;
        if (--dec) return 0;
        dec = 8;
        initiate_McBSP_transfer (0);
        return 0;
}

int dp_task_010(void){
// ============================================================
// PROCESS MODULE: MOTOR SERVO
// ============================================================

	m_servo.delta = _SAT32 ((long)m_servo.setpoint - (long)*m_servo.input); // Q23 input and setpoint, SAT difference
	run_servo_timestep (&m_servo);
        return 0;
}

int dp_task_011(void){
// ============================================================
// PROCESS MODULE: SCO (Signal Controller Oszillator)
// ============================================================
	if (sco_s[0].in)
	  run_sco (&sco_s[0]);

        return 0;
}

int dp_task_012(void){
	if (sco_s[1].in)
	  run_sco (&sco_s[1]);

        return 0;
}

typedef int (*func_ptr)(void);

func_ptr dp_task_list[NUM_DATA_PROCESSING_TASKS+1] = {
        NULL,
        dp_task_001, dp_task_002, dp_task_003, dp_task_004,
        dp_task_005, dp_task_006, dp_task_007, dp_task_008,
        dp_task_009, dp_task_010, dp_task_011, dp_task_012
};


/* This is ISR is called on each new sample.  The "mode"/statevariable
 * should be initialized with AIC_OFFSET_COMPENSATION before starting
 * the AIC/DMA isr, this assures the correct offset initialization and
 * runs the automatic offset compensation on startup.
 */

void dataprocess()
{
        int i, ip;
        int tmp1, tmp2;
        unsigned int ier_protect_mask;
// ============================================================
// PROCESS MODULE: DSP PROCESSING CLOCK TICKS and TIME KEEPING
// ============================================================
        // now done via SR3PRO_A810Driver[-interruptible-no-kernel-int].lib
        //IER &= ~0x0010; // Disable INT4 (kernel) -- protect data integrity (atomic read/write protect not necessary for data manipulated inside here)

        // protect initial checks from recursion -- disable DMA/A810 int
        ier_protect_mask = state.dp_task_control[0].process_flag >> 16;
        IER &= ~ier_protect_mask; // Disable MASK
        
        TSCReadSecond();
        state.DataProcessReentryTime = MeasuredTime;
        tmp1 = state.DataProcessReentryPeak & 0xffff; // LO (min time)
        tmp2 = state.DataProcessReentryPeak >> 16;    // HI (max time)
        state.DataProcessReentryPeak  = tmp1 < MeasuredTime ?  tmp1 : MeasuredTime; // LO (min time)
        state.DataProcessReentryPeak |= (tmp2 > MeasuredTime ?  tmp2 : MeasuredTime) << 16; // HI (max time)
        
	TSCReadFirst(); // Reference time start now

        MeasuredTime -= state.DataProcessTime; // time outside dataprocess (but incl. little overhead DMA, ...)
        tmp1 = state.IdleTime_Peak & 0xffff; // LO (min time)
        tmp2 = state.IdleTime_Peak >> 16;    // HI (max time)
        state.IdleTime_Peak  = tmp1 < MeasuredTime ?  tmp1 : MeasuredTime; // LO (min time)
        state.IdleTime_Peak |= (tmp2 > MeasuredTime ?  tmp2 : MeasuredTime) << 16; // HI (max time)
        
	++state.DSP_time; // 150kHz dataprocessing synchronous time ticks

        // inspect and return ASAP on missed complete processing (Emergency: only if not aborted correctly in time)
        for (i=1; i<=NUM_DATA_PROCESSING_TASKS; i++)
                if (state.dp_task_control[i].process_flag&1)
                        state.dp_task_control[i].missed_count++; // inclrement missed count

        if (state.dp_task_control[0].process_flag&1){ // self running?
                state.dp_task_control[0].missed_count++; // increment missed (aborted) system call count
                // enable (likley good idea DMA/A810 int...)
                // remove proection (inverse to above)
                IER |= ier_protect_mask;
                return;  // abort now and return ASAP
        }

        state.dp_task_control[0].process_flag |= 1; // set self to running now
        // enable (likley good idea DMA/A810 int...)
        // remove proection (inverse to above)
        IER |= ier_protect_mask;

// ============================================================
// REAL TIME PROCESS CONTROL
// ============================================================

        /* Initialize active processes for this cycle 
           -- flag mask 0x10 for active always
           -- flag mask 0x20 for active on ODD only
           -- flag mask 0x40 for active on EVEN only
        */
        for (i=1; i<=NUM_DATA_PROCESSING_TASKS; i++){
                /*
                if (state.dp_task_control[i].process_flag & 0x10)
                        state.dp_task_control[i].process_flag |= 1;
                else if (state.dp_task_control[i].process_flag & 0x20 && state.DSP_time&1)
                        state.dp_task_control[i].process_flag |= 1;
                else if (state.dp_task_control[i].process_flag & 0x40 && !(state.DSP_time&1))
                        state.dp_task_control[i].process_flag |= 1;
                */
                // OR
                state.dp_task_control[i].process_flag |= 
                           (state.dp_task_control[i].process_flag & 0x10)
                        || (state.dp_task_control[i].process_flag & 0x20 && state.DSP_time&1)
                        || (state.dp_task_control[i].process_flag & 0x40 && !(state.DSP_time&1))
                        ? 1 : 0;
        }
        
	TSCReadSecond();
        state.dp_task_control[0].process_time = MeasuredTime;

        /* HI PRIORITY: Data Processing Tasks */
        ip=0;
        for (i=1; i<=NUM_DATA_PROCESSING_TASKS; i++){
                if (state.dp_task_control[i].process_flag&1){

                        // Protect (disable IER) acordign to flag (normally NN=00 -- in process flag= 0x00NN 00PP -- means no protection, NN is IER MASK, PP are process mode and enable flags)
                        ier_protect_mask = state.dp_task_control[i].process_flag >> 16;
                        IER &= ~ier_protect_mask;
                        dp_task_list [i] (); // Execute RT Task
                        TSCReadSecond ();
                        state.dp_task_control[i].process_time = MeasuredTime; // incremental!
                        // remove proection (inverse to above)
                        IER |= ier_protect_mask;

                        tmp1 = state.dp_task_control[i].process_time_peak_now & 0xffff; // last peak of process time (LO)
                        tmp2 = MeasuredTime - state.dp_task_control[ip].process_time;  // actual process time from difference to previouly [ip] executed task
                        state.dp_task_control[i].process_time_peak_now  = tmp2 << 16;  // actual process time from difference -> upper 16 (HI)
                        state.dp_task_control[i].process_time_peak_now |= tmp2 > tmp1 ? tmp2 : tmp1; // peak in lower 16 (LO)
                        state.dp_task_control[i].process_flag &= 0xfffffff0; // completed, remove flag bit 0 (lowest nibble=0)
                        ip=i;
                }
                if (MeasuredTime > state.DP_max_time_until_abort) // time since start processing -- may selfadjust via 4000-max of next ???
                        break; // abort, running out of time! approx max 4500 - 100 - 60 - N x 120??? for possible McBSP interrupt -- ticks available to reentry
        }

        /* Update Dataprocess Time Total */
        TSCReadSecond();
        state.DataProcessTime = MeasuredTime;

        /* Total Processing System Time DP[0] and Peak */
        tmp1 = state.dp_task_control[0].process_time_peak_now & 0xffff;
        state.dp_task_control[0].process_time_peak_now  = MeasuredTime << 16; // actual total data process time -> upper 16 (HI)
        state.dp_task_control[0].process_time_peak_now |= MeasuredTime > tmp1 ? MeasuredTime : tmp1; // peak in lower 16 (LO)

        /* END of DP */
        state.dp_task_control[0].process_flag &= 0xfffffff0; // self completed

        // now done via SR3PRO_A810Driver[-interruptible-no-kernel-int].lib
        //IER |= 0x0010; // Enable INT4 (kernel)  
}


/* END */

/*
IER &= ~0x0010; // Disable INT4 (kernel)

... Critical code

IER |= 0x0010; // Enable INT4 (kernel)  
*/
