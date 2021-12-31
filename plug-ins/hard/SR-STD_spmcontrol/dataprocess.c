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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/*
 *
 *	dataprocess.c
 *
 *	This is the ISR of the DMA. This ISR is called at each new sample.
 */

#include "dataprocess.h"  
#include "FB_spm_dataexchange.h"

/* local used variables for automatic offset compensation */
long    memb[8];        /* 32 bit variable used to calculate the input0 offset*/
int     blcklen;	/* Block length, used for the offset measurement*/
int     IdleTimeTmp;
long    tmpsum;

int     feedback_hold = 0; /* Probe controlled Feedback-Hold flag */
int     pipi = 0;       /* Pipe index */
int     scnt = 0;       /* Slow summing count */
int     mout6_bias   = 0; /* Holds original bias value, while mout6 gets offset adjusted */
int     mout_orig[3] = {0,0,0}; /* Holds original value, while mout0..2 gets offset adjusted */

/* externals of SPM control */

extern SPM_STATEMACHINE state;
extern SPM_PI_FEEDBACK  feedback;
extern EXT_CONTROL      external;
extern DFM_FUZZYMIX     dfm_fuzzymix;
extern ANALOG_VALUES    analog;
extern MOVE_OFFSET      move;
extern AREA_SCAN        scan;
extern AUTOAPPROACH     autoapp;
extern PROBE            probe;
extern CR_OUT_PULSE     CR_out_pulse;

extern int AS_ch2nd_constheight_enabled; /* const H mode flg of areascan process */
extern struct aicregdef    aicreg;

#define BIAS_ADJUST_STEP 4

/* smoothly adjust bias - make sure |analog_bias| < 32766-BIAS_ADJUST_STEP !!  */
inline void run_bias_adjust (){
	analog.bias &= 0xfffe;
	if (analog.bias-BIAS_ADJUST_STEP > iobuf.mout6){
		iobuf.mout6 += BIAS_ADJUST_STEP;
	}else{	
		if (analog.bias+BIAS_ADJUST_STEP < iobuf.mout6)
			iobuf.mout6 -= BIAS_ADJUST_STEP;
		else	
			iobuf.mout6 = analog.bias;
	}
}


/* This is ISR is called on each new sample.  The "mode"/statevariable
 * should be initialized with AIC_OFFSET_COMPENSATION before starting
 * the AIC/DMA isr, this assures the correct offset initialization and
 * runs the automatic offset compensation on startup.
 */

interrupt void dataprocess()
{
	/* Compute IdleTime */
	IdleTimeTmp -= *(int*)TIM1;
	state.IdleTime = IdleTimeTmp;

	/* Load DataTime */
	state.DataProcessTime = *(int*)TIM1;

	/* To compensate FIR Filter OFF at AIC 5 and 7 */
	iobuf.min5 =  _sshl(iobuf.min5,1);
	iobuf.min7 =  _sshl(iobuf.min7,1);
	
	switch (state.DataProcessMode){

	case AIC_DATA_PROCESS_NORMAL:
		/* Update iobuf.min0 and iobuf.min1 with offset compensation ? */
		if (state.mode & MD_OFFSETCOMP){
			// iobuf.min0 -= analog.offset[0]; //----> use SAT instructions:
			iobuf.min0 = _ssub (iobuf.min0, analog.offset[0]);
			iobuf.min1 = _ssub (iobuf.min1, analog.offset[1]);
			iobuf.min2 = _ssub (iobuf.min2, analog.offset[2]);
			iobuf.min3 = _ssub (iobuf.min3, analog.offset[3]);
			iobuf.min4 = _ssub (iobuf.min4, analog.offset[4]);
			iobuf.min5 = _ssub (iobuf.min5, analog.offset[5]);
			iobuf.min6 = _ssub (iobuf.min6, analog.offset[6]);
			iobuf.min7 = _ssub (iobuf.min7, analog.offset[7]);

			// restore Bias; without Offset Compensation now
			iobuf.mout0 = mout_orig[0];
			iobuf.mout1 = mout_orig[1];
			iobuf.mout2 = mout_orig[2];
			iobuf.mout6 = mout6_bias;
		}

		/* compute software differential input for AIC5 & AIC7 */
		if (state.mode & MD_AIC5M7){
			iobuf.min5 = _ssub (iobuf.min5, iobuf.min7);
		}


		/* Offset Move task ? */
		if (move.pflg){
			run_offset_move ();
#ifdef DSP_OFFSET_ADDING
			if (state.mode & MD_OFFSETADDING){
				iobuf.mout3 = _sadd (analog.x_scan, analog.x_offset) & 0xfffe;
				iobuf.mout4 = _sadd (analog.y_scan, analog.y_offset) & 0xfffe;
			} else {
				iobuf.mout0 = analog.x_offset & 0xfffe; /* X Offset */
				iobuf.mout1 = analog.y_offset & 0xfffe; /* Y Offset */
				iobuf.mout2 = analog.z_offset & 0xfffe; /* Z Offset */
			}
#else
			iobuf.mout0 = analog.x_offset & 0xfffe; /* X Offset */
			iobuf.mout1 = analog.y_offset & 0xfffe; /* Y Offset */
			iobuf.mout2 = analog.z_offset & 0xfffe; /* Z Offset */
#endif
		}

		/* Area Scan task ?
		 * --> this may modify >> feedback.i_pl_sum << for fored slope compensation
		 * and thus it has to be executed before the feedback task,
		 * the feedback task needs to be enabled to see the effect
		 * --> can set CI/CP to small values to "contineously" disable it!
		 */
		if (scan.pflg){
			run_area_scan ();
#ifdef DSP_OFFSET_ADDING
			if (state.mode & MD_OFFSETADDING){
				iobuf.mout3 = _sadd (analog.x_scan, analog.x_offset) & 0xfffe;
				iobuf.mout4 = _sadd (analog.y_scan, analog.y_offset) & 0xfffe;
			} else {
				iobuf.mout3 = analog.x_scan & 0xfffe; /* X Scan */
				iobuf.mout4 = analog.y_scan & 0xfffe; /* Y Scan */
			}
#else
			iobuf.mout3 = analog.x_scan & 0xfffe; /* X Scan */
			iobuf.mout4 = analog.y_scan & 0xfffe; /* Y Scan */
#endif
		}

		
		/* Vector Probe task ?  (Bias may be changed here) */
		feedback_hold = 0;
		if (probe.pflg){
			run_probe ();
			if (probe.vector)
				if (probe.vector->options & VP_FEEDBACK_HOLD)
					feedback_hold = 1;
		} else {
			/* (re) Adjust Bias ? */
//			probe.Upos = _lsshl (analog.bias, 16);
			probe.Upos = (long)analog.bias << 16;

			if (iobuf.mout6 != analog.bias)
				run_bias_adjust ();
		}
		

		/* FeedBack (FB) task ?  (evaluates several conditions and runs in various modes) */

		if (!feedback_hold){ /* may be freezed by probing process */
			/* Let's check out, if the feedback loop is really running. */
			feedback_hold = 1; 

		/* check if PID flag, EXTFB flag (+ treshold check...) are set */
		if ((state.mode & (MD_EXTFB | MD_PID)) == (MD_EXTFB | MD_PID) ?
		    (*(((int*)&iobuf)+external.FB_control_channel) <= external.FB_control_treshold) : state.mode & MD_PID)
			{
				if ( !(AS_ch2nd_constheight_enabled && scan.pflg)){ // skip in 2nd const height mode!
					feedback_hold = 0; 
					if (state.mode & MD_LOG)
						asm_feedback_log (); // logarith mode (STM)
					else{
#ifdef DSP_LIN_DFM_FUZZYMIX_OPTION
						if (state.mode & MD_DFM_FUZZY){ // enable DFM-FUZZY control
							iobuf.min1 = _ssub (iobuf.min1, dfm_fuzzymix.level);
							if (iobuf.min1 > 0){
								tmpsum = iobuf.min5;
								tmpsum = _smac (tmpsum, iobuf.min1, dfm_fuzzymix.gain);
								iobuf.min5 = _lsshl (tmpsum, -15);
							}
						}
#endif
						asm_feedback_lin (); // linear mode (AFM)
					}
				}
			} else {
				if (state.mode & MD_ZTRAN) // use external Z, inverted
					feedback.z = iobuf.mout6 = (-iobuf.min7) & 0xfffe;
			}

		}


		/* external watch task ? -- pass variable out on channel 7 -- no FIR */
		if (state.mode & MD_WATCH)
		{
			switch(external.watch_value){
				//default: iobuf.mout7 = 0;
				case EXTERNAL_FEEDBACK:
					iobuf.mout7 = _sadd(feedback_hold * _ssub(external.watch_max, external.watch_min), external.watch_min);
					break;
					// iobuf.mout7 = feedback_hold * (external.watch_max - external.watch_min) + external.watch_min;
				case EXTERNAL_OFFSET_X:
					iobuf.mout7 = analog.x_offset & 0xfffe;
					break;
				case EXTERNAL_LOG:
					iobuf.mout7 = feedback.ist;
					break;
				case EXTERNAL_DELTA:
					iobuf.mout7 = feedback.delta;
					break;
			}
		}
		else iobuf.mout7 = 0;




		/* Run Autoapproch/Movercontrol task ? */
		if (autoapp.pflg){
			if (autoapp.pflg==2)
				autoapp.pflg = 0;
			else
				run_autoapp ();

			if (autoapp.mover_mode & AAP_MOVER_XYOFFSET){
				iobuf.mout0 = analog.x_offset & 0xfffe; /* X Offset */
				iobuf.mout1 = analog.y_offset & 0xfffe; /* Y Offset */
			} else if (autoapp.mover_mode & AAP_MOVER_XYSCAN){
				iobuf.mout3 = analog.x_offset & 0xfffe; /* X Scan */
				iobuf.mout4 = analog.y_offset & 0xfffe; /* Y Scan */
			} else if (autoapp.mover_mode & AAP_MOVER_XYMOTOR){
				iobuf.mout7 = analog.x_offset & 0xfffe; /* X on Motor */
			} else if (autoapp.mover_mode & AAP_MOVER_ZSCANADD){
				/* add to Z -- waveform needs t obe shifeted correctly to take care of offset and Z-polarity */
				iobuf.mout5 = _sadd (iobuf.mout5, analog.x_offset) & 0xfffe;
			} // else no output !!
		}

		/* Run CoolRunner Out puls(es) task ? */
		if (CR_out_pulse.pflg)
			run_CR_out_pulse ();

		/* AIC data processing for slow smooth response */
		if (state.mode & MD_AIC_SLOW){
			analog.aic_avg[6] = _lsadd (analog.aic_avg[6], iobuf.min6);
			analog.aic_avg[6] = _lssub (analog.aic_avg[6], analog.aic6_pipe[pipi]);
			analog.aic6_pipe[pipi] = iobuf.min6;
			analog.aic_avg[7] = _lsadd (analog.aic_avg[7], iobuf.min7);
			analog.aic_avg[7] = _lssub (analog.aic_avg[7], analog.aic7_pipe[pipi]);
			analog.aic7_pipe[pipi] = iobuf.min7;

			memb[0] = _lsadd (memb[0], iobuf.min0);
			memb[1] = _lsadd (memb[1], iobuf.min1);
			memb[2] = _lsadd (memb[2], iobuf.min2);
			memb[3] = _lsadd (memb[3], iobuf.min3);
			memb[4] = _lsadd (memb[4], iobuf.min4);
			memb[5] = _lsadd (memb[5], iobuf.min5);
			memb[6] = _lsadd (memb[6], iobuf.min6);
			memb[7] = _lsadd (memb[7], iobuf.min7);

			// only one length and only AIC6,7 contineously by 128 yet.
			if (++scnt == analog.length[6]){
				analog.aic_slow[0] = memb[0];	
				analog.aic_slow[1] = memb[1];	
				analog.aic_slow[2] = memb[2];	
				analog.aic_slow[3] = memb[3];	
				analog.aic_slow[4] = memb[4];	
				analog.aic_slow[5] = memb[5];	
				analog.aic_slow[6] = memb[6];	
				analog.aic_slow[7] = memb[7];	
				memb[0] = memb[1] = memb[2] = memb[3] = memb[4] 
					= memb[5] = memb[6] = memb[7] = 0;
				scnt = 0;
			}
 
			if (++pipi == 128) 
				pipi = 0;
		}

		/* Bias AIC-out offset compensation ? */
		if (state.mode & MD_OFFSETCOMP){
			// store Bias; apply Offset Compensation now
			mout6_bias = iobuf.mout6;
			iobuf.mout6 = _ssub(iobuf.mout6, analog.out_offset[6]);	iobuf.mout6 &= 0xfffe;

			// same procedure for AICout0,1,2 -- mirror and adjust
			mout_orig[0] = iobuf.mout0;
			mout_orig[1] = iobuf.mout1;
			mout_orig[2] = iobuf.mout2;
			iobuf.mout0 = _ssub(iobuf.mout0, analog.out_offset[0]); iobuf.mout0 &= 0xfffe;
			iobuf.mout1 = _ssub(iobuf.mout1, analog.out_offset[1]); iobuf.mout1 &= 0xfffe;
			iobuf.mout2 = _ssub(iobuf.mout2, analog.out_offset[2]); iobuf.mout2 &= 0xfffe;
		}

		/* Allow AIC gain reconfiguration ? */
		if (state.mode & MD_AIC_RECONFIG){
			// mute ???

			state.mode &= ~MD_AIC_RECONFIG;
			// set 2nd data request bit
			iobuf.mout0 |= 1;	
			iobuf.mout1 |= 1;	
			iobuf.mout2 |= 1;	
			iobuf.mout3 |= 1;	
			iobuf.mout4 |= 1;	
			iobuf.mout5 |= 1;		
			iobuf.mout6 |= 1;	
			iobuf.mout7 |= 1;	
			// switch mode temporary
			state.DataProcessMode = AIC_REQUEST_RECONFIG;
		}
		
		break;

	case AIC_REQUEST_RECONFIG:
		iobuf.mout0 = 0x6800 | (aicreg.reg40)&0xff;	
		iobuf.mout1 = 0x4800 | (aicreg.reg41)&0xff;	
		iobuf.mout2 = 0x2800 | (aicreg.reg42)&0xff;	
		iobuf.mout3 = 0x0800 | (aicreg.reg43)&0xff;	
		iobuf.mout4 = 0x6800 | (aicreg.reg44)&0xff;	
		iobuf.mout5 = 0x4800 | (aicreg.reg45)&0xff;	
		iobuf.mout6 = 0x2800 | (aicreg.reg46)&0xff;	
		iobuf.mout7 = 0x0800 | (aicreg.reg47)&0xff;	
		state.DataProcessMode = AIC_DATA_PROCESS_NORMAL;
		break;

	case AIC_OFFSET_COMPENSATION:
		state.DataProcessMode = AIC_OFFSET_CALCULATION;
		blcklen = 8300;
		analog.offset[0] = analog.offset[1] = analog.offset[2] = analog.offset[3] = analog.offset[4] 
			= analog.offset[5] = analog.offset[6] = analog.offset[7] = 0;
		memb[0] = memb[1] = memb[2] = memb[3] = memb[4] 
			= memb[5] = memb[6] = memb[7] = 0;
		/* no break here, go on to Offset Calculation now! */

	case AIC_OFFSET_CALCULATION:
		/* some initial delay */
		if (--blcklen < 8192){
			memb[0] += iobuf.min0;
			memb[1] += iobuf.min1;
			memb[2] += iobuf.min2;
			memb[3] += iobuf.min3;
			memb[4] += iobuf.min4;
			memb[5] += iobuf.min5;
			memb[6] += iobuf.min6;
			memb[7] += iobuf.min7;
		   	
			if (blcklen == 0){
				memb[0] >>= 13;
				analog.offset[0] = (int)memb[0];
				memb[1] >>= 13;
				analog.offset[1] = (int)memb[1];
				memb[2] >>= 13;
				analog.offset[2] = (int)memb[2];
				memb[3] >>= 13;
				analog.offset[3] = (int)memb[3];
				memb[4] >>= 13;
				analog.offset[4] = (int)memb[4];
				memb[5] >>= 13;
				analog.offset[5] = (int)memb[5];
				memb[6] >>= 13;
				analog.offset[6] = (int)memb[6];
				memb[7] >>= 13;
				analog.offset[7] = (int)memb[7];
				state.mode = state.last_mode;   // now ready for control!
				state.DataProcessMode = AIC_DATA_PROCESS_NORMAL; // switch to normal dataprocessing mode
			}
		}
		/* Set all outputs to zero */ 
		iobuf.mout0 = 0;
		iobuf.mout1 = 0;
		iobuf.mout2 = 0;
		iobuf.mout3 = 0;
		iobuf.mout4 = 0;
		iobuf.mout5 = 0;
		iobuf.mout6 = 0;
		iobuf.mout7 = 0;
		break;
	}
	
	/* increment DSP blink statemaschine's counter and time reference */
	++state.BLK_count;
	
	/* Compute DataTime */
	state.DataProcessTime -= *(int*)TIM1;

	/* Load IdleTime */
	IdleTimeTmp = *(int*)TIM1;

	/* Load Peak Detection */
	if (state.DataProcessTime > state.DataProcessTime_Peak){
		 state.DataProcessTime_Peak = state.DataProcessTime;
		 state.IdleTime_Peak = state.IdleTime;
	}
}



