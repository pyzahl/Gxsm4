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


/* ============================================================
    ** Recorder / Scope functionality **
    from Alex:
    I made the record function for SR2. The execution time depends on the buffers position. 
    For instance, if both buffers are in the SDRAM, it means 44 DSP cycles by write in SDRAM 
    plus the overhead (23 DSP cycles): 88+23=111 cycles.
 
    Here is the definition that you have to add in your C file:
 
   Definition of the record signal function defined in the assembler file
   This function records in Signal1() and Signal2() if blcklen is not equal to -1
   pSignal1 and pSignal2 pointers are used to determine the recorded signals
  ==============================================================
 */


// Definition of the record signal functions defined in the assembler file
// This functions record in Signalx if blcklen16/32 is not equal to -1
// pSignal1 and pSignal2 pointers are used to determine the recorded signals
 
extern void RecSignalsASM16();
extern void RecSignalsASM32();
 

#include "FB_spm_dataexchange.h"
#include "dataprocess.h"  
#include "mul32.h"
#include "FB_spm_task_names.h"

/* local used variables for automatic offset compensation */
long    memb[8];        /* 32 bit variable used to calculate the input0 offset*/
int     blcklen;	/* Block length, used for the offset measurement*/
int     IdleTimeTmp;


// 8x digital sigma-delta over sampling using bit0 of 16 -> gain of 2 bits resoultion (16+2=18)
#define SIGMA_DELTA_LEN   8
int     sigma_delta_index = 0;
long    zpos_xymult = 0L;
long    s_xymult = 0L;
long    s_xymult_prev = 0L;
long    d_tmp=0L;
int     tmp_hrbits = 0;

//#define SIGMA_DELTA_HR_MAKS_FAST
#ifdef SIGMA_DELTA_HR_MAKS_FAST  // Fast HR Matrix -- good idea, but performance/distorsion issues with the A810 due to the high dynamic demands
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

#define HR_OUT(CH, VALUE32) tmp_hrbits = _lsshl (VALUE32, -13) & 7; AIC_OUT(CH) = _sadd (_lsshl (VALUE32, -16), sigma_delta_hr_mask[(tmp_hrbits<<3) + sigma_delta_index])



int     sdt=0;
int     sdt_16=0;

unsigned short  QEP_cnt_old[2];
unsigned long   SUM_QEP_cnt_Diff[2];
long   DSP_time;

int     gate_cnt = 0;
int     gate_cnt_multiplier = 0;
int     gate_cnt_1 = 0;
int     feedback_hold = 0; /* Probe controlled Feedback-Hold flag */
int     pipi = 0;       /* Pipe index */
int     scnt = 0;       /* Slow summing count */
int     mout6_bias   = 0; /* Holds original bias value, while mout6 gets offset adjusted */
int     mout_orig[3] = {0,0,0}; /* Holds original value, while mout0..2 gets offset adjusted */

/* IIR tmp */

long   Q30L   = 1073741823L;
long   Q15L   = 32767L;
int    Q15    = 32767;
int    AbsIn  = 0;

/* externals of SPM control */

extern SPM_STATEMACHINE state;
extern SPM_PI_FEEDBACK  feedback;
extern EXT_CONTROL      external;
extern FEEDBACK_MIXER   feedback_mixer;
extern ANALOG_VALUES    analog;
extern MOVE_OFFSET      move;
extern AREA_SCAN        scan;
extern AUTOAPPROACH     autoapp;
extern PROBE            probe;
extern CR_OUT_PULSE     CR_out_pulse;
extern CR_GENERIC_IO    CR_generic_io;
extern RECORDER         recorder;

extern int AS_ch2nd_constheight_enabled; /* const H mode flg of areascan process */
extern struct aicregdef    aicreg;

extern long PRB_ACPhaseA32;

long xy_vec[4];
long mm_vec[4];
long result_vec[4];

int last_dp_i=0;
inline void read_dp_process_time (int i){
	unsigned long tmp1, tmp2;
	TSCReadSecond();
	state.dp_task_control[i].process_time = MeasuredTime;
        tmp1 = state.dp_task_control[i].process_time_peak_now & 0xffff; // last peak of process time (LO)
        tmp2 = MeasuredTime - state.dp_task_control[last_dp_i].process_time;  // actual process time from difference to previouly [ip] executed task
        state.dp_task_control[i].process_time_peak_now  = tmp2 << 16;  // actual process time from difference -> upper 16 (HI)
        state.dp_task_control[i].process_time_peak_now |= tmp2 > tmp1 ? tmp2 : tmp1; // peak in lower 16 (LO)
        //state.dp_task_control[i].process_flag &= 0xfffffff0; // completed, remove flag bit 0 (lowest nibble=0)
        last_dp_i=i;
}
	
/* This is ISR is called on each new sample.  The "mode"/statevariable
 * should be initialized with AIC_OFFSET_COMPENSATION before starting
 * the AIC/DMA isr, this assures the correct offset initialization and
 * runs the automatic offset compensation on startup.
 */


interrupt void dataprocess()
{
        unsigned long tmp1, tmp2;
	int mi, i, k;
// ============================================================
// PROCESS MODULE: DSP PROCESSING CLOCK TICKS and TIME KEEPING
// ============================================================
        // now done via SR3PRO_A810Driver[-interruptible-no-kernel-int].lib
        //IER &= ~0x0010; // Disable INT4 (kernel) -- protect data integrity (atomic read/write protect not necessary for data manipulated inside here)

        TSCReadSecond();
        state.dp_task_control[0].process_time = MeasuredTime; // reference time
        last_dp_i=0; // initialize reference to last reading
        
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

// ============================================================
// PROCESS MODULE: PRIMITIVE pflg controlled RT TASK PROCESSING
// ============================================================

// PROCESS FIXED RTE SCHEDULE...
	
// ============================================================
// RTE TASKS -- ALWAYS
// ============================================================
	if ( state.dp_task_control[RT_TASK_ADAPTIIR].process_flag & 0x10){
	// always compute IIR/filters/RM, applies also to STS sources with FB OFF now!
	// *** if (feedback.I_cross > 0){
	        // run IIR self adaptive filter on DAC0
	        feedback.I_fbw = AIC_IN(0);
		AbsIn = _abss(AIC_IN(0));
		feedback.q_factor15 = _lssub (Q15L, _smac (feedback.cb_Ic, AbsIn, Q15) / _sadd (AbsIn, feedback.I_cross));
		feedback.zxx = feedback.q_factor15;
		if (feedback.q_factor15 < feedback.ca_q15)
		        feedback.q_factor15 = feedback.ca_q15;
		feedback.I_iir = _smac ( _lsmpy ( _ssub (Q15, feedback.q_factor15), AIC_IN(0)),  feedback.q_factor15, _lsshl (feedback.I_iir, -16));
		AIC_IN(0) = _lsshl (feedback.I_iir, -16);

		read_dp_process_time (RT_TASK_ADAPTIIR);
	}

	// ALWAYS TASKS!!!
	for (mi=0; mi<4; ++mi){
	        // MIXER CHANNEL i
	        if (feedback_mixer.mode[mi] & 0x10) // negate flag set?
		        AIC_IN(mi) = -AIC_IN(mi);
	}
	/* "virtual" DSP based differential input for IN0 and IN4 as reference remapped on IN0 */
	if (state.mode & MD_DIFFIN0M4){
		AIC_IN(0) = _ssub (AIC_IN(0), AIC_IN(4));
	}
		
// ============================================================
// RTE TASKS -- EVEN 0x20
// ============================================================
	if (state.DSP_time & 1){
#ifdef USE_ANALOG_16 // if special build for MK2-Analog_16
		analog.counter[0] = 0L; // Counter/Timer support not yet available via FPGA
		analog.counter[1] = 0L; // Counter/Timer support not yet available via FPGA
#else // default: MK2-A810
		// QEP TASK ================================================
		if (state.dp_task_control[RT_TASK_QEP].process_flag & 0x20){

			/* Automatic Aligned Gateing by scan or probe process */
			asm_counter_accumulate (); /* accumulate events in counters */
			
			/* always handle Counter_1 -- 16it gatetime only (fast only, < 800ms @ 75 kHz) */
			if (++gate_cnt_1 >= CR_generic_io.gatetime_1){
				gate_cnt_1 = 0;
				CR_generic_io.count_1 = analog.counter[1];
				analog.counter[1] = 0L;
				
				if (CR_generic_io.count_1 > CR_generic_io.peak_count_1) 
					CR_generic_io.peak_count_1 = CR_generic_io.count_1;
			}
			
			read_dp_process_time (RT_TASK_QEP);
		}
		
		// RATEMETER TASK ================================================
		if (state.dp_task_control[RT_TASK_RATEMETER].process_flag & 0x20){
			//** if (!(scan.pflg || probe.pflg) && CR_generic_io.pflg) {
			/* stand alone Rate Meter Mode else gating managed via spm_areascan or _probe module */
				
			/* handle Counter_0 -- 32it gatetime (very wide range) */
			if (gate_cnt_multiplier >= CR_generic_io.gatetime_h_0){
				if (++gate_cnt >= CR_generic_io.gatetime_l_0){
					gate_cnt = 0;
					gate_cnt_multiplier = 0;
						
					CR_generic_io.count_0 = analog.counter[0];
					analog.counter[0] = 0L;
						
					if (CR_generic_io.count_0 > CR_generic_io.peak_count_0) 
						CR_generic_io.peak_count_0 = CR_generic_io.count_0;
				}
			} else { // this makes up a 32bit counter...
				if (++gate_cnt == 0)
					++gate_cnt_multiplier;
			}
			
			read_dp_process_time (RT_TASK_RATEMETER);
		}
#endif		
		if (state.dp_task_control[RT_TASK_FEEDBACK].process_flag & 0x20){
			feedback.watch = 0;
			if (!feedback_hold){ /* may be freezed by probing process */
				if ( !(AS_ch2nd_constheight_enabled && scan.pflg)){ // skip in 2nd const height mode!

					// Feedback Mixer -- data transform and delta computation, summing
					feedback_mixer.delta = 0L;

					for (mi=0; mi<4; ++mi){
						// MIXER CHANNEL mi
						switch (feedback_mixer.mode[mi]){
						case 3: // LOG
							feedback_mixer.x = AIC_IN(mi);
							asm_calc_mix_log ();
							feedback_mixer.delta = _smac(feedback_mixer.delta, _ssub (feedback_mixer.lnx, feedback_mixer.setpoint_log[mi]), feedback_mixer.gain[mi]);
							break;
						case 1: // LIN
							feedback_mixer.x = AIC_IN(mi);
							feedback_mixer.delta = _smac(feedback_mixer.delta, _ssub (feedback_mixer.x, feedback_mixer.setpoint[mi]), feedback_mixer.gain[mi]);
							break;
#ifdef OLDFUZZYLOGIC
						case 9: // FUZZY
							if (AIC_IN(mi) > feedback_mixer.level[mi]){
								feedback_mixer.x = _ssub (AIC_IN(mi), feedback_mixer.level[mi]);
								feedback_mixer.delta = _smac(feedback_mixer.delta, _ssub (feedback_mixer.x, feedback_mixer.setpoint[mi]), feedback_mixer.gain[mi]);
							}
							break;
						case 11: // FUZZY LOG
							if (AIC_IN(mi) > feedback_mixer.level[mi]){
								feedback_mixer.x = _ssub (AIC_IN(mi), feedback_mixer.level[mi]);
								asm_calc_mix_log ();
								feedback_mixer.delta = _smac(feedback_mixer.delta, _ssub (feedback_mixer.lnx, feedback_mixer.setpoint[mi]), feedback_mixer.gain[mi]);
							}
							break;
#else // new FUZZY Z-Pos Control
                                                case 9: // CZ FUZZY LIN
                                                        if (AIC_IN(mi) > feedback_mixer.level[mi]){
                                                                feedback_mixer.x = AIC_IN(mi);
                                                                feedback_mixer.delta = _smac(feedback_mixer.delta, _ssub (feedback_mixer.x, feedback_mixer.setpoint[mi]), feedback_mixer.gain[mi]);
                                                        } else {
                                                                feedback_mixer.delta = _smac(feedback_mixer.delta, _ssub (feedback.z32neg>>16, feedback_mixer.Z_setpoint), feedback_mixer.gain[mi]);
                                                        }
                                                        break;
                                                case 11: // CZ FUZZY LOG
                                                        if (abs (AIC_IN(mi)) > feedback_mixer.level[mi]){
                                                                feedback_mixer.x = AIC_IN(mi);
                                                                asm_calc_mix_log ();
                                                                feedback_mixer.delta = _smac(feedback_mixer.delta, _ssub (feedback_mixer.lnx, feedback_mixer.setpoint_log[mi]), feedback_mixer.gain[mi]);
                                                        } else {
                                                                feedback_mixer.delta = _smac(feedback_mixer.delta, _ssub (feedback.z32neg>>16, feedback_mixer.Z_setpoint), feedback_mixer.gain[mi]);
                                                        }
                                                        break;
#endif                                                        
						default: break; // OFF
						}
					}

					// run plain feedback from delta directly
					feedback.delta =  _lsshl (feedback_mixer.delta, -15);
					asm_feedback_from_delta ();
					feedback.watch = 1; // OK, we did it! -- for actual watch fb activity
				}
			}

			read_dp_process_time (RT_TASK_FEEDBACK);
		}

	} else {
// ============================================================
// RTE TASKS -- ODD 0x40
// ============================================================
		// ALWAYS set default bias, overriden below as required by scan_section, vetor_probe Upos, and may be modulated by locking
		analog.out[ANALOG_BIAS] = scan.bias_section[0];
		
		// VECTOR AREA SCAN TASK ====================================
		if (state.dp_task_control[RT_TASK_AREA_SCAN].process_flag & 0x40){
			if (scan.pflg & (AREA_SCAN_RUN | AREA_SCAN_MOVE_TIP))
                                run_area_scan ();
			
			// use section bias!
			analog.out[ANALOG_BIAS] = scan.bias_section[scan.section];

			read_dp_process_time (RT_TASK_AREA_SCAN);
		}
		
		// VECTRO PROBE TASK =========================================
		if (state.dp_task_control[RT_TASK_VECTOR_PROBE].process_flag & 0x40){
			/* NOTE: Bias "probe.Upos" may be changed here. */
			feedback_hold = 0;
                        if (probe.pflg){
                                run_probe (); // BIAS from probe relative to last setting

                                if (probe.vector)
                                        if (probe.vector->options & VP_FEEDBACK_HOLD)
                                                feedback_hold = 1;
			}
			read_dp_process_time (RT_TASK_VECTOR_PROBE);

		}
		// copy to Upos
		AIC_OUT(6) = _lsshl (probe.Upos, -16);

                // mirror from ext control
                AIC_OUT(7) = external.output_M;


		// VECTRO PROBE TASK =========================================
		if (state.dp_task_control[RT_TASK_LOCKIN].process_flag & 0x40){
			run_lockin ();
			read_dp_process_time (RT_TASK_LOCKIN);
		}
	}

	// ALWAYS TASKS!!! ===================================================

        // do scan coordinate rotation transformation:
        if ( !(state.mode & MD_XXA))
                {
                        xy_vec[2] = xy_vec[0] = scan.xyz_vec[i_X];
                        xy_vec[3] = xy_vec[1] = scan.xyz_vec[i_Y];
                        mul32 (xy_vec, scan.rotm, result_vec, 4);
                        scan.xy_r_vec[i_X] = _lsadd (result_vec[0], result_vec[1]);
                        scan.xy_r_vec[i_Y] = _lsadd (result_vec[2], result_vec[3]);
                } else {
                scan.xy_r_vec[i_X] = scan.xyz_vec[i_X];
                scan.xy_r_vec[i_Y] = scan.xyz_vec[i_Y];
        }

        // Calculate Slope for Z-Offset Output/Analog gain 1 adding option
        //---- slope add X*mx + Y*my
        //#ifdef DSP_Z0_SLOPE_COMPENSATION
        // limit dz add from xy-mult to say 10x scan.fm_dz0x+y, feedback like adjust if "diff" to far off from sudden slope change 
        //	zpos_xymult = move.ZPos + scan.XposR * scan.fm_dz0x +  scan.YposR * scan.fm_dz0y ;
        // make sure a smooth adjust -- if slope parameters get changed, need to prevent a jump.

        // Z AREA SCAN SLOPE
        if ( !(state.mode & MD_XXB)){
                mul32 (scan.xy_r_vec, scan.fm_dz0_xy_vec, result_vec, 2);
                s_xymult = _lsadd (result_vec[i_X], result_vec[i_Y]);
		
                d_tmp = _lssub (s_xymult, s_xymult_prev);
                if (d_tmp > scan.z_slope_max) // limit up
                        s_xymult_prev = _lsadd (s_xymult_prev, scan.z_slope_max);
                else if (d_tmp < -scan.z_slope_max) // limit dn
                        s_xymult_prev = _lsadd (s_xymult_prev, -scan.z_slope_max);
                else s_xymult_prev = s_xymult; // normally this should do it
        }


        
	// NOW OUTPUT HR SIGNALS ON XYZ-Offset and XYZ-Scan -- do not touch Bias OUT(6) and Motor OUT(7) here -- handled directly previously.
	// note: OUT(0-5) get overridden below by coarse/mover actions if requeste!!!

	/* HR sigma-delta data processing (if enabled) -- turn off via adjusting sigma_delta_hr_mask to all 0 */

	// XY-Offset and XY-Scan output -- separated or combined as life configuraion:
	if (state.mode & MD_OFFSETADDING){
		for (i=0; i<=i_Y; ++i)
			result_vec[i] = _lsadd (move.xyz_vec[i], scan.xy_r_vec[i]);
		
		HR_OUT (3, result_vec[i_X]);
		HR_OUT (4, result_vec[i_Y]);
	} else {
		HR_OUT (0, move.xyz_vec[i_X]);
		HR_OUT (1, move.xyz_vec[i_Y]);
		HR_OUT (3, scan.xy_r_vec[i_X]);
		HR_OUT (4, scan.xy_r_vec[i_Y]);
	}


	// Z-Offset (and slope if enabled) output
	if ( !(state.mode & MD_XXB)){
		zpos_xymult = _lsadd (move.xyz_vec[i_Z], s_xymult_prev);
		HR_OUT (2, zpos_xymult);
	} else {
		HR_OUT (2, move.xyz_vec[i_Z]);
	}


	// FEEDBACK OUT -- calculate Z-Scan output:
	d_tmp = _lssub (feedback.z32neg, probe.Zpos);
	//#define INTERNAL_Z_SLOPE_ADD
#ifdef INTERNAL_Z_SLOPE_ADD
	// internal slope adding -- experimental for Yoichi
	d_tmp = _lsadd (d_tmp, s_xymult_prev); // add this line here to add z-slope
#endif
	HR_OUT (5, d_tmp);

	sigma_delta_index = (++sigma_delta_index) & 7;

	read_dp_process_time (RT_TASK_ANALOG_OUTHR);
	
	// ========== END ALWAYS TASK -- HR processing ================

	//if ( state.dp_task_control[RT_TASK_MSERVO].process_flag & 0x10){
	//}
	
	// AUTO APP and COARSE MOTION/WAVEPLAY TASK ======================
	if ( state.dp_task_control[RT_TASK_WAVEPLAY].process_flag & 0x10){
		/* Run Autoapproch/Movercontrol task ? */
		if (autoapp.pflg > 0){
			if (autoapp.pflg==2)
                                autoapp.pflg = -1; // terminate next
			else
				run_autoapp ();
		
			for (i=0; i<autoapp.n_wave_channels; ++i){
				k = autoapp.channel_mapping[i] & 7;
				if (autoapp.channel_mapping[i] & AAP_MOVER_SIGNAL_ADD)
					AIC_OUT (k) = _sadd (AIC_OUT (k), analog.out[k]);
				else
					AIC_OUT (k) = analog.out[k];
			}
		} else 
			/* Run CoolRunner Out puls(es) task ? */
			if (CR_out_pulse.pflg)
				run_CR_out_pulse ();

		read_dp_process_time (RT_TASK_WAVEPLAY);
	}
	
	// RECORDER TASK =================================================
	if ( state.dp_task_control[RT_TASK_RECORDER].process_flag & 0x10){
		/* hooks to universal recorder */
		if (recorder.pflg){
			switch (recorder.pflg){
			case 1: RecSignalsASM16(); break;
			case 2: RecSignalsASM32(); break;
			}
		}
		read_dp_process_time (RT_TASK_RECORDER);
	}

// ============================================================
// PROCESS MODULE END: TIME KEEPING
// ============================================================

        /* Update Dataprocess Time Total */
        TSCReadSecond();
        state.DataProcessTime = MeasuredTime;

        /* Total Processing System Time DP[0] and Peak */
        tmp1 = state.dp_task_control[0].process_time_peak_now & 0xffff;
        state.dp_task_control[0].process_time_peak_now  = MeasuredTime << 16; // actual total data process time -> upper 16 (HI)
        state.dp_task_control[0].process_time_peak_now |= MeasuredTime > tmp1 ? MeasuredTime : tmp1; // peak in lower 16 (LO)

        /* END of DP */
        //state.dp_task_control[0].process_flag &= 0xf0; // self completed

        // now done via SR3PRO_A810Driver[-interruptible-no-kernel-int].lib
        //IER |= 0x0010; // Enable INT4 (kernel)  
}



