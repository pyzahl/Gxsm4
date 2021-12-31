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


#include "FB_spm_statemaschine.h"
#include "FB_spm_analog.h"
#include "dataprocess.h"
#include "ReadWrite_GPIO.h"
#include "FB_spm_task_names.h"

#ifdef WATCH_ENABLE
extern WATCH            watch;
#endif

extern SPM_STATEMACHINE state;
extern SPM_PI_FEEDBACK  feedback;
extern FEEDBACK_MIXER   feedback_mixer;
extern SPM_PI_FEEDBACK  feedback;
extern EXT_CONTROL      external;
extern ANALOG_VALUES    analog;
extern MOVE_OFFSET      move;
extern AREA_SCAN        scan;
extern PROBE            probe;
extern AUTOAPPROACH     autoapp;
extern CR_OUT_PULSE     CR_out_pulse;
extern CR_GENERIC_IO    CR_generic_io;
extern A810_CONFIG      a810_config;
extern DATA_SYNC_IO     data_sync_io;
extern RECORDER         recorder;

extern long randomnum;

extern int feedback_hold; /* Probe controlled Feedback-Hold flag */


extern	void asm_calc_mix_log ();

extern int bz_push_area_scan_data_out (void);

/*
 *	DSP idle loop, runs for ever, returns never !!!!
 *  ============================================================
 *	Main of the DSP State Maschine
 *      - State Status "STMMode", heartbeat
 *      - manage process commands via state, 
 *        this may change the state
 *      - enable/disable of all tasks
 */

int setpoint_old = 0;
int mix_setpoint_old[4] = {0,0,0,0};

int AIC_stop_mode = 0;
int sleepcount = 0;


#define STOP_RT_TASK(N)         state.dp_task_control[N].process_flag  = 0x00
#define START_RT_TASK(N)        state.dp_task_control[N].process_flag |= 0x10
#define START_RT_TASK_ALWAYS(N) state.dp_task_control[N].process_flag |= 0x10
#define START_RT_TASK_EVEN(N)   state.dp_task_control[N].process_flag |= 0x20
#define START_RT_TASK_ODD(N)    state.dp_task_control[N].process_flag |= 0x40



#define BIAS_ADJUST_STEP 4
#define Z_ADJUST_STEP   0x200L



#if 0
/* smoothly adjust bias - make sure |analog_bias| < 32766-BIAS_ADJUST_STEP !!  */
inline void run_bias_adjust (){
	if (analog.out[ANALOG_BIAS]-BIAS_ADJUST_STEP > AIC_OUT(6)){
		AIC_OUT(6) += BIAS_ADJUST_STEP;
	}else{	
		if (analog.out[ANALOG_BIAS]+BIAS_ADJUST_STEP < AIC_OUT(6))
			AIC_OUT(6) -= BIAS_ADJUST_STEP;
		else	
			AIC_OUT(6) = analog.out[ANALOG_BIAS];
	}
}
#endif

/* smoothly brings Zpos back to zero in case VP left it non zero at finish */
inline void run_Zpos_adjust (){
#if 0
	probe.Zpos = 0;
#else
	if (probe.Zpos > 0)
//		probe.Zpos -= Z_ADJUST_STEP;
		probe.Zpos = _lsadd (probe.Zpos, -Z_ADJUST_STEP);
	else
		if (probe.Zpos < 0)
//			probe.Zpos += Z_ADJUST_STEP;
			probe.Zpos = _lsadd (probe.Zpos, Z_ADJUST_STEP);
		else
			probe.Zpos = 0L;
#endif
}


// ========================================
// IDLE TASKS
// ========================================


#pragma CODE_SECTION(idle_task_001, ".text:slow")
int idle_task_001(void){
        return bz_push_area_scan_data_out ();
}

#pragma CODE_SECTION(idle_task_002, ".text:slow")
int idle_task_002(void){
        // ============================================================
        // PROCESS MODULE: OFFSET MOVE
        // ============================================================
        /* Offset Move task ? */
        if (move.pflg){
                run_offset_move ();
                return 1;
        }
        return 0;
}

// Generic Vector Probe Program between Section Processing Function calls moved out of real time regime:
extern void process_next_section ();

#pragma CODE_SECTION(idle_task_003, ".text:slow")
int idle_task_003(void){
        // Vector Probe Manager
        if (probe.pflg){
                if (!probe.ix) // idle task needs to work on section completion out of RT regime
                        process_next_section ();
                
                if (state.mode & MD_PID){
                        if (probe.vector)
                                if (probe.vector->options & VP_FEEDBACK_HOLD){
                                        STOP_RT_TASK (RT_TASK_FEEDBACK);
                                        feedback.watch = 0; // must reset flag here, if task is not running, it is not managed
                                        return 1;
                                } else {
                                        START_RT_TASK_EVEN (RT_TASK_FEEDBACK);
                                        return 1;
                                }
                }
	}
        return 0;
}

#pragma CODE_SECTION(idle_task_004, ".text:slow")
int idle_task_004(void){
	// any special external task/watch/etc. ? -- pass variable out on channel 7 -- no FIR */
	if (state.mode & MD_WATCH)
	{
		switch(external.watch_value){
			//default: external.output_M = 0;
#ifdef EN_CODE_EXTERNAL_FEEDBACK_CTRL
		case EXTERNAL_FEEDBACK: // NOTE by Py: multiplikation here is to be removed: inefficient, expensive.
			external.output_M = _sadd(feedback_hold * _ssub(external.watch_max, external.watch_min), external.watch_min);
			break;
			// external.output_M = feedback_hold * (external.watch_max - external.watch_min) + external.watch_min;
#endif
		case EXTERNAL_OFFSET_X:
			external.output_M = analog.out[ANALOG_X0_AUX];
			break;
#if 0
		case EXTERNAL_LOG:
			external.output_M = feedback_mixer.lnx;
			break;
#endif
#if 0
		case EXTERNAL_DELTA:
			external.output_M = feedback.delta;
			break;
#endif
		case EXTERNAL_PRB_SINE:
			external.output_M = PRB_ref1stA;
			break;
		case EXTERNAL_PRB_PHASE:
			external.output_M = PRB_modindex > (probe.LockInRefSinLenA>>1) ? 16384 : 0;
			break;
		case EXTERNAL_BIAS_ADD:
			AIC_OUT(6) = _lsshl (_smac (probe.Upos, AIC_IN(7), probe.AC_amp), -16);
			break;
		case EXTERNAL_FEEDBACK_ADD:
			external.output_M = _sadd (AIC_OUT(5), AIC_IN(7));
			break;
		case EXTERNAL_EN_MOTOR:
			external.output_M = analog.out[ANALOG_MOTOR];
			break;
//#define EN_CODE_EXTERNAL_FB2
#ifdef EN_CODE_EXTERNAL_FB2
// note: may break things here... max code size exceeded eventually depending on other code sections enabled
		case EXTERNAL_EN_PHASE:
			external.output_M = _sadd (_lsshl (PRB_ACPhaseA32, -16), analog.out[ANALOG_MOTOR]);
			break;
		case EXTERNAL_PID:
			// run 2nd PID feedback in AIC 7 in/out
			tmpsum = AIC_IN(7);
			tmpsum = _ssub (external.fb2_set, tmpsum);
			external.fb2_delta2 = _ssub (external.fb2_delta, tmpsum);
			external.fb2_delta  = tmpsum;
			tmpsum = external.fb2_i_sum = _smac (external.fb2_i_sum, external.fb2_ci, external.fb2_delta);
			tmpsum = _smac (tmpsum, external.fb2_cp, external.fb2_delta);
			tmpsum = _smac (tmpsum, external.fb2_cd, external.fb2_delta2);
                        tmp_hrbits = _lsshl (tmpsum, -13) & 7;
                        external.output_M = _sadd (_lsshl (tmpsum, -16), sigma_delta_hr_mask[(tmp_hrbits<<3) + sigma_delta_index]);
			break;
#endif
		}
	}
	else external.output_M = 0;
	
// Auxillary outputs, option, coarse moves overrides, etc.
#if 0
	if (state.mode & MD_NOISE){
		AIC_OUT(6) = _lsshl (_smac (probe.Upos, randomnum, probe.AC_amp), -16);
	}
#endif
        return 0;
}

/* PID-feedback on/off only via flag MD_PID -- nothing to do here */

#pragma CODE_SECTION(process_mode_set, ".text:slow")
void process_mode_set (int m){
        if (m & MD_PID){
                START_RT_TASK_EVEN (RT_TASK_FEEDBACK);
        }
}

#pragma CODE_SECTION(process_mode_clr, ".text:slow")
void process_mode_clr (int m){
        if (m & MD_PID){
                STOP_RT_TASK (RT_TASK_FEEDBACK);
                feedback.watch = 0; // must reset flag here, if task is not running, it is not managed
        }
}

#pragma CODE_SECTION(idle_task_005, ".text:slow")
int idle_task_005(void){
        /* state change request? */
        if (state.set_mode){
                state.mode |= state.set_mode;
		process_mode_set (state.set_mode);
                state.set_mode = 0;
                return 1;
        }
        if (state.clr_mode){
                state.mode &= ~state.clr_mode;
		process_mode_clr (state.clr_mode);
                state.clr_mode = 0;
                return 1;
        }
        /* (test) tool for log execution */
	if (feedback_mixer.exec){
		asm_calc_mix_log ();
		feedback_mixer.exec = 0;
                return 1;
	}
        return 0;
}

#pragma CODE_SECTION(idle_task_006, ".text:slow")
int idle_task_006(void){
        /* DSP level setpoint calculation for log mode */
        int i;
	for (i=0; i<4; ++i)
		if (feedback_mixer.setpoint[i] != mix_setpoint_old[i]){
			feedback_mixer.x = feedback_mixer.setpoint[i];
			asm_calc_mix_log ();
			feedback_mixer.setpoint_log[i] = feedback_mixer.lnx;
			mix_setpoint_old[i] = feedback_mixer.setpoint[i];
			WATCH16(i,feedback_mixer.setpoint_log[i]);
			return 1; // one by one less blocking!
		}
        return 0;
}

#pragma CODE_SECTION(idle_task_007, ".text:slow")
int idle_task_007(void){
	// Signal Management N/A
        return 0;
}

#pragma CODE_SECTION(idle_task_008, ".text:slow")
int idle_task_008(void){
        /* Start Offset Moveto ? */
        if (move.start && !autoapp.pflg){
                init_offset_move ();
                return 1;
        }
        return 0;
}

void switch_rt_task_areascan_to_probe (void){
        if (!probe.pflg){
		STOP_RT_TASK (RT_TASK_AREA_SCAN);
		probe.start=1;
        }
}

#pragma CODE_SECTION(idle_task_009, ".text:slow")
int idle_task_009(void){
        /* Start/Stop/Pause/Resume Area Scan ? */
        // if (scan.start == APPLY_NEW_ROTATION) return;

        if (scan.start){
		if (autoapp.pflg){
			scan.start = 0;
			return 1;
		}
                init_area_scan (scan.start);
                if (scan.pflg)
                        START_RT_TASK_ODD (RT_TASK_AREA_SCAN);
                return 1; // keep checking until FIFO unstalled and host is ready to go
        }

        switch (scan.stop){
        case 0: return 0;
        case AREA_SCAN_STOP:
                scan.stop = 0;
                finish_area_scan ();
		STOP_RT_TASK (RT_TASK_AREA_SCAN);
                return 1; // Finsihed or Stop/Cancel/Abort scan
        case AREA_SCAN_PAUSE:
                scan.stop = 0;
                if (scan.pflg & AREA_SCAN_RUN){
			STOP_RT_TASK (RT_TASK_AREA_SCAN);
                }
                return 1; // Pause Scan
        case AREA_SCAN_RESUME: 
                scan.stop = 0;
                if (scan.pflg & AREA_SCAN_RUN){
			START_RT_TASK_ODD (RT_TASK_AREA_SCAN);
                }
                return 1; // Resume Scan from Pause, else ignore
        }

        return 0;
}

#pragma CODE_SECTION(idle_task_010, ".text:slow")
int idle_task_010(void){
        /* LockIn run free and Vector Probe control manager */
        if (probe.start == PROBE_RUN_LOCKIN_FREE){
                init_lockin ();
		probe.start = 0;
		START_RT_TASK_ODD (RT_TASK_LOCKIN);
                return 1;
        }
        // ELSE:
        if (probe.start){
		if (probe.pflg || autoapp.pflg){
			probe.start = 0; // ignore and abort
			return 1;
		}
                if (scan.pflg)
			STOP_RT_TASK (RT_TASK_AREA_SCAN);
                init_probe_fifo (); // reset probe fifo!
                init_probe ();
		START_RT_TASK_ODD (RT_TASK_VECTOR_PROBE);
                return 1;
        }
        
        if (probe.stop == PROBE_RUN_LOCKIN_FREE){
		STOP_RT_TASK (RT_TASK_LOCKIN);
                return 1;
        }
        // ELSE:
        if (probe.stop){
		STOP_RT_TASK (RT_TASK_VECTOR_PROBE);
                stop_probe ();
                // make sure it's back on if enabled
		if (state.mode & MD_PID)
			START_RT_TASK_EVEN (RT_TASK_FEEDBACK);
                if (scan.pflg)
			START_RT_TASK_ODD (RT_TASK_AREA_SCAN);
                
                return 1;
        }
#if 1 // may backport -- testing --
        // else if probe running, check on FB manipulations
        if (probe.pflg){
                if (probe.vector)
			if (probe.vector->options & VP_GPIO_SET){
				unsigned int i =  (probe.vector->options & VP_GPIO_MSK) >> 16;
				if (probe.gpio_setting != i){ // update GPIO!
					CR_generic_io.gpio_data_out = (DSP_UINT16)i;
					//WRITE_GPIO_0 (CR_generic_io.gpio_data_out);
					WR_GPIO (GPIO_Data_0, &CR_generic_io.gpio_data_out, 1);
					probe.gpio_setting = i;
				}
				return 1;
			}
	}
#endif
        return 0;
}

#pragma CODE_SECTION(idle_task_011, ".text:slow")
int idle_task_011(void){
        /* Start Autoapproach/run Mover ? */
        if (autoapp.start){
		if (probe.pflg || scan.pflg){
			autoapp.start=0; // abort
			return 1;
		}
                init_autoapp ();
                autoapp.start = 0;
                autoapp.stop  = 0;
		START_RT_TASK (RT_TASK_WAVEPLAY);
                return 1;
        }
        if (autoapp.stop){
                stop_autoapp ();
               	autoapp.start = 0;
                autoapp.stop  = 0;
                return 1;
        }
	if (autoapp.pflg < 0){
                STOP_RT_TASK (RT_TASK_WAVEPLAY);
                autoapp.pflg = 0 ;
                return 1;
        }
        
	return 0;
}

#pragma CODE_SECTION(idle_task_012, ".text:slow")
int idle_task_012(void){
        /* Start CoolRunner IO pulse ? */
        if (CR_out_pulse.start){
                init_CR_out_pulse ();
		START_RT_TASK (RT_TASK_WAVEPLAY);
                return 1;
        }
        if (CR_out_pulse.stop){
		STOP_RT_TASK (RT_TASK_WAVEPLAY);
                stop_CR_out_pulse ();
                return 1;
        }
        return 0;
}

#pragma CODE_SECTION(idle_task_013, ".text:slow")
int idle_task_013(void){
	int ret=0;
        /* Control GPIO pins ? */
        if (CR_generic_io.start){
		switch (CR_generic_io.start){
		case 1: WR_GPIO (GPIO_Data_0, &CR_generic_io.gpio_data_out, 1); break; // write port
		case 2: WR_GPIO (GPIO_Data_0, &CR_generic_io.gpio_data_in, 0); break; // read port
		case 3: WR_GPIO (GPIO_Dir_0, &CR_generic_io.gpio_direction_bits, 1); break; // reconfigure port
		default: break;
		}
		CR_generic_io.start=0;
		ret=1;
        }

	if (data_sync_io.pflg) //  || (CR_generic_io.gpio_direction_bits & 0x0100))
		if (data_sync_io.tick){
			data_sync_io.gpiow_bits =
				(CR_generic_io.gpio_data_out & 0xff)
				| ((data_sync_io.xyit[0] & 1)<<8)
				| ((data_sync_io.xyit[1] & 1)<<9)
				| ((data_sync_io.xyit[2] & 1)<<10);
			WR_GPIO (GPIO_Data_0, &data_sync_io.gpiow_bits, 1); // write port
			data_sync_io.tick=0;
			//data_sync_io.last_xyt[0] = data_sync_io.xyt[0];
			ret=1;
		}
	
        return ret;
}

#pragma CODE_SECTION(idle_task_014, ".text:slow")
int idle_task_014(void){
        // Recorder Manager
        switch (recorder.start){
        case 0: return 0;
        case 1: // Record mode ASM16
                recorder.pflg = 1;
                START_RT_TASK (RT_TASK_RECORDER);
                recorder.start = 0;
                return 1;
        case 2: // Record mode ASM32
                recorder.pflg = 2;
                START_RT_TASK (RT_TASK_RECORDER);
                recorder.start = 0;
                return 1;
        case 99: // Stop Recorder
                STOP_RT_TASK (RT_TASK_RECORDER);
                recorder.pflg = 0;
                recorder.start = 0;
                return 1;
        }
        return 0;
}

#pragma CODE_SECTION(idle_task_015, ".text:slow")
int idle_task_015(void){
        // DSP Core Manager -- reserved
        return 0;
}

#pragma CODE_SECTION(idle_task_016, ".text:slow")
int idle_task_016(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_017, ".text:slow")
int idle_task_017(void){
#if 0
        int i;
// ============================================================
// PROCESS MODULE: SIGNAL MONITOR data copy processing
// ============================================================
        for (i=0; i<NUM_MONITOR_SIGNALS; ++i)
                sig_mon.signal[i] = *sig_mon.signal_p[i];
#endif
        return 0;
}

#pragma CODE_SECTION(idle_task_018, ".text:slow")
int idle_task_018(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_019, ".text:slow")
int idle_task_019(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_020, ".text:slow")
int idle_task_020(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_021, ".text:slow")
int idle_task_021(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_022, ".text:slow")
int idle_task_022(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_023, ".text:slow")
int idle_task_023(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_024, ".text:slow")
int idle_task_024(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_025, ".text:slow")
int idle_task_025(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_026, ".text:slow")
int idle_task_026(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_027, ".text:slow")
int idle_task_027(void){
        return 0;
}

#pragma CODE_SECTION(idle_task_028, ".text:slow")
int idle_task_028(void){
        // Bias and ZPos adjuster
        if (!probe.pflg){
                feedback_hold = 0; // make sure it's reset to normal

		// Bias Adjust:
                // probe.Upos = _lsshl (analog.out[ANALOG_BIAS], 16);
                probe.Upos = (long)analog.out[ANALOG_BIAS] << 16; // BIAS from manual or scan

		// Z Adjust: if no probe Zpos should return to zero
                if (probe.Zpos){
                        run_Zpos_adjust ();
                        return 1;
                }
        }
        return 0;
}

#pragma CODE_SECTION(idle_task_029, ".text:slow")
int idle_task_029(void){
        return 0;
}

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
long    rms_I2_pipe[RMS_N] = 
{ 
	0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L, 0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L, 
	0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L, 0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L, 
	0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L, 0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L, 
	0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L, 0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0L,  0L,0L,0L,0L, 0L,0L,0L,0
};

#pragma CODE_SECTION(idle_task_030, ".text:slow")
int idle_task_030(void){
	long    tmpsum;
	// Average and RMS computations
	feedback.I_avg = _lsadd (feedback.I_avg, AIC_IN(0));
	feedback.I_avg = _lssub (feedback.I_avg, rms_I_pipe[rms_pipi]);
	rms_I_pipe[rms_pipi] = AIC_IN(0);
	
	tmpsum = _lssub (AIC_IN(0), _lsshl (feedback.I_avg, -RMS_N2));
	tmpsum = _abss(tmpsum);
	if (tmpsum > 2047) // need to limit to avoid overflow of sum
	        tmpsum = 2047;
	tmpsum *= tmpsum;
	feedback.I_rms = _lsadd (feedback.I_rms, tmpsum);
	feedback.I_rms = _lssub (feedback.I_rms, rms_I2_pipe[rms_pipi]);
	rms_I2_pipe[rms_pipi]  = tmpsum;
	
	if (++rms_pipi == RMS_N) 
	        rms_pipi = 0;
						
        return 1;
}

/* Auxillary Random Number Generator */

#define RNG_a 16807         /* multiplier */
#define RNG_m 2147483647L   /* 2**31 - 1 */
#define RNG_q 127773L       /* m div a */
#define RNG_r 2836          /* m mod a */

long randomnum = 1L;

#pragma CODE_SECTION(idle_task_031, ".text:slow")
int idle_task_031(void){
        // ============================================================
        // PROCESS MODULE: NOISE GENERATOR (RANDOM NUMBER)
        // ============================================================
	unsigned long lo, hi;

	lo = _lsmpy (RNG_a, randomnum & 0xFFFF);
	hi = _lsmpy (RNG_a, (unsigned long)randomnum >> 16);
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
	randomnum = (long)lo;
      
        return 1;
}

#pragma CODE_SECTION(idle_task_032, ".text:slow")
int idle_task_032(void){
        // 1s Timer
        // 1s Timer
        state.DSP_count_seconds++;
        if (state.DSP_seconds > 0)
                state.DSP_seconds--;
        else{
                state.DSP_seconds = 59;
                state.DSP_minutes++;
        }
        return 1;
}


typedef int (*func_ptr)(void);

func_ptr idle_task_list[NUM_IDLE_TASKS] = {
        idle_task_001, idle_task_002, idle_task_003, idle_task_004,
        idle_task_005, idle_task_006, idle_task_007, idle_task_008,
        idle_task_009, idle_task_010, idle_task_011, idle_task_012,
        idle_task_013, idle_task_014, idle_task_015, idle_task_016,
        idle_task_017, idle_task_018, idle_task_019, idle_task_020,
        idle_task_021, idle_task_022, idle_task_023, idle_task_024,
        idle_task_025, idle_task_026, idle_task_027, idle_task_028,
        idle_task_029, idle_task_030, idle_task_031, idle_task_032
};



int run_next_priority_job(void){
        static int iip=0;
        int i;
        for (i=iip; i<NUM_IDLE_TASKS; i++){
                if (state.id_task_control[i].process_flag & 0x10){
                        state.id_task_control[i].process_flag |= 1;
                        if (idle_task_list [i] ()){
                                state.id_task_control[i].timer_next = state.DSP_time; // since last data process end
                                state.id_task_control[i].exec_count++; // was actually processing, count n calls in flag of idle tasks
                                iip = (i+1) & (NUM_IDLE_TASKS-1);
                                return 1;
                        }
                        state.id_task_control[i].process_flag &= 0xf0;
                }
        }
        iip=0;
        return 0;
}

// NOTE: CHECK FOR state.DSP_time looping conditon "rep timer" mode
int run_next_sync_job(void){
        static int iip=0;
        int i;
        long long delta;
        for (i=iip; i<NUM_IDLE_TASKS; i++){
                if (state.id_task_control[i].process_flag & 0x20){
                        delta = (long long)state.id_task_control[i].timer_next - (long long)state.DSP_time;
                        if (delta < 0 || delta > 0x7FFFFFFFLL){
                                state.id_task_control[i].process_flag |= 1;
                                state.id_task_control[i].timer_next = state.DSP_time + state.id_task_control[i].interval; // next time due, too close to now
                                if (idle_task_list [i] ()){
                                        state.id_task_control[i].exec_count++; // was actually processing, count n calls in flag of idle tasks
                                        iip = (i+1) & (NUM_IDLE_TASKS-1);
                                        return 1;
                                }
                                state.id_task_control[i].process_flag &= 0xf0;
                        }
                }
        }
        iip=0;
        return 0;
}

// NOTE: CHECK FOR state.DSP_time looping conditon "clock" mode
#pragma CODE_SECTION(run_next_sync_job_precision, ".text:slow")
int run_next_sync_job_precision(void){
        static int iip=0;
        int i;
        long long delta;
        for (i=iip; i<NUM_IDLE_TASKS; i++){
                if (state.id_task_control[i].process_flag & 0x40){
                        delta = (long long)state.id_task_control[i].timer_next - (long long)state.DSP_time;
                        if (delta < 0 || delta > 0x7FFFFFFFLL){
                                state.id_task_control[i].process_flag |= 1;
                                if ( -delta < state.id_task_control[i].interval && !(delta > 0))
                                        state.id_task_control[i].timer_next += state.id_task_control[i].interval; // next time due -- exact
                                else
                                        state.id_task_control[i].timer_next = state.DSP_time + state.id_task_control[i].interval; // next time due, too close to now
                                if (idle_task_list [i] ()){
                                        state.id_task_control[i].exec_count++; // was actually processing, count n calls in flag of idle tasks
                                        iip = (i+1) & (NUM_IDLE_TASKS-1);
                                        return 1;
                                }
                                state.id_task_control[i].process_flag &= 0xf0;
                        }
                }
        }
        iip=0;
        return 0;
}


void dsp_idle_loop (void){
	/* configure GPIO all in for startup safety -- this is ans should be default power-up, just to make sure again */
	int i = 0x0000; WR_GPIO (GPIO_Dir_0, &i, 1);

	for(;;){	/* forever !!! */

                /* LOW PRIORITY: Process Priority Idle Tasks */
                if (run_next_priority_job())
                        continue;
                
                /* LOW PRIORITY: Process Timed Idle Tasks */
                if (run_next_sync_job())
                        continue;
                
                /* LOW PRIORITY: Precision Process Timed Idle Tasks -- intervall >> 100 please */
                if (run_next_sync_job_precision())
                        continue;

		// state.id_task_control[25].exec_count++; // TEST FOR IDLE LOOPS FULL
	} /* repeat idle loop forever... */
}

/* END */
