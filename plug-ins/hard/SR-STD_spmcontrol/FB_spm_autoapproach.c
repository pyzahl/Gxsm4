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

#define TIP_Ready     0
#define TIP_ZPiezoMax 1
#define TIP_ZPiezoMin 2
#define TIP_Delay_1   3
#define TIP_ZSteps    4
#define TIP_Delay_2   5

extern SPM_STATEMACHINE state;
extern SPM_PI_FEEDBACK  feedback;
extern ANALOG_VALUES    analog;
extern AUTOAPPROACH     autoapp;
extern MOVE_OFFSET      move;

ioport   unsigned int Port4;  /*CPLD port*/

int wave_hold=0;
int app_ci_signum=1;
short *waveptr;

// initialize for mover and autoapproach
void init_autoapp (){
	// now prepare for starting...
	autoapp.start = 0;
	autoapp.stop  = 0;

	// init mover
	autoapp.mv_count = 0; 
	autoapp.mv_dir   = 0; 
	autoapp.mv_step_count = 0;
	wave_hold = 0;
	waveptr = EXTERN_DATA_FIFO_ADDRESS_P;

	// set start state for autoapp
	autoapp.tip_mode = TIP_ZPiezoMax;

	// enable job
	autoapp.pflg = 1;
}

// stop all mover/approach actions, cleanup
void stop_autoapp (){
	autoapp.start = 0;
	autoapp.stop  = 0;

	if (app_ci_signum*feedback.ci < 0) {
		// ensure proper feedback on
		feedback.cp = autoapp.cp;
		feedback.ci = autoapp.ci;
	}

	if (autoapp.mover_mode & AAP_MOVER_PULSE)
		Port4 = waveptr[autoapp.u_piezo_max-1] & 0xff;

	// enable FB
	if(!(state.mode & MD_PID))
		state.mode |= MD_PID;

	// reset offset
	analog.x_offset = _lsshl (move.XPos, -16); // move.XPos>>16;
	analog.y_offset = _lsshl (move.YPos, -16); // move.YPos>>16;

	// switch off after next analog (iobuf) update
	autoapp.pflg  = 2;
}

// Dummy Macro, no IO yet!
#define IOBIT_SET(N)
#define IOBIT_CLR(N)


// generate mover ramp signal (parabel sections)
void run_mover (){
	// waveform mode or default simple parabulum mode?
	if (autoapp.mover_mode & AAP_MOVER_WAVE){
		if (autoapp.mover_mode & AAP_MOVER_PULSE)
			Port4 = waveptr[autoapp.mv_count] & 0xff;

		switch (autoapp.mover_mode & AAP_MOVER_DIRMMSK) {
		case AAP_MOVER_XP: analog.x_offset = waveptr[autoapp.mv_count]; break;
		case AAP_MOVER_XM: analog.x_offset = -waveptr[autoapp.mv_count]; break;
		case AAP_MOVER_YP: analog.y_offset = waveptr[autoapp.mv_count]; break;
		case AAP_MOVER_YM: analog.y_offset = -waveptr[autoapp.mv_count]; break;
		case AAP_MOVER_OFF: analog.x_offset = 0; analog.y_offset = 0;
			if (autoapp.mover_mode & AAP_MOVER_PULSE)
				Port4 = waveptr[autoapp.u_piezo_max-1] & 0xff;
			break;
		default: break;
		}
		
		if (autoapp.piezo_speed > 1){
			if (++wave_hold == autoapp.piezo_speed){
				wave_hold = 0;
				++autoapp.mv_count;
			}
		} else
			++autoapp.mv_count;

		if (autoapp.mv_count >= autoapp.u_piezo_max)
			autoapp.mv_count = 0;
	} else {
		// may need to use SATuration here!!
		// autoapp.u_piezo = _smpy (autoapp.mv_count, autoapp.mv_count);
		autoapp.u_piezo = autoapp.mv_count*autoapp.mv_count;

		if(autoapp.u_piezo >= autoapp.u_piezo_max){
			autoapp.mv_count -= autoapp.piezo_speed;
			autoapp.u_piezo = autoapp.mv_count*autoapp.mv_count;
			autoapp.mv_dir = 1;
		}
		
		if(autoapp.mv_dir){
			autoapp.mv_count -= autoapp.piezo_speed;
			autoapp.u_piezo   = autoapp.u_piezo_amp - autoapp.u_piezo;
		} else
			autoapp.mv_count += autoapp.piezo_speed;
		
		switch (autoapp.mover_mode & AAP_MOVER_DIRMMSK) {
		case AAP_MOVER_XP: analog.x_offset =  autoapp.u_piezo-autoapp.u_piezo_max; break;
		case AAP_MOVER_XM: analog.x_offset = -autoapp.u_piezo+autoapp.u_piezo_max; break;
		case AAP_MOVER_YP: analog.y_offset =  autoapp.u_piezo-autoapp.u_piezo_max; break;
		case AAP_MOVER_YM: analog.y_offset = -autoapp.u_piezo+autoapp.u_piezo_max; break;
		case AAP_MOVER_OFF: analog.x_offset = 0; analog.y_offset = 0; break;
		default: break;
		} 
	}

	// final rep counter check
        if (!autoapp.mv_count && !wave_hold){
		autoapp.mv_dir=0;
                ++autoapp.mv_step_count;

		// all steps done?
                if(autoapp.mv_step_count >= autoapp.piezo_steps){
			analog.x_offset = 0;
			analog.y_offset = 0;

			if (autoapp.mover_mode & AAP_MOVER_XP_AUTO_APP)
				autoapp.tip_mode = TIP_Delay_2; // proceed with next approach cycle
			else
				autoapp.pflg = 0; // done, max number steps exceeded
			return;
                }
        }
}

// statemaschine: coordinates automatic tip approach, start with state
// (statevar is autoapp.tip_mode) "TIP_ZPiezoMax"
void run_tip_app (){
        switch(autoapp.tip_mode){
        case TIP_ZPiezoMax:
		// enable feedback, Z-approach via CI,CP
                if(!(state.mode & MD_PID))     // need to start feedback?
                        state.mode |= MD_PID;     // enable feedback

		// check for tunneling conditions
		// check for limit
		if (app_ci_signum*feedback.z > 30000 ) // almost extended? !!!Z-Signum dependent!!!
			autoapp.tip_mode = TIP_ZPiezoMin;

		// delta is always <0 if no or low current!
		if (feedback.delta > 0){
			autoapp.tip_mode = TIP_Ready; // dummy state
			autoapp.pflg  = 0; // done! Cancel autoapproach.
		}

                break;
				
        case TIP_ZPiezoMin:
		// retract tip, using feedback reprogramming
		// reprogramm feedback ? -- feedback is and stays on!!!
		if (app_ci_signum*feedback.ci > 0) {
			// save feedback settings
			app_ci_signum = feedback.ci > 0 ? 1:-1;
			autoapp.cp = feedback.cp;
			autoapp.ci = feedback.ci;
						
			// reprogramm for reverse direction!!!
			feedback.cp = 0;
			feedback.ci *= -autoapp.ci_mult;
		} else {
			// check progress
			if ( app_ci_signum*feedback.z < -30000 ){ // >= half extended?
				state.mode &= ~MD_PID;     // disable feedback
				// restore feedback settings
				feedback.cp = autoapp.cp;
				feedback.ci = autoapp.ci;

				autoapp.delay_cnt = autoapp.n_wait;
				// next state
				autoapp.tip_mode = TIP_Delay_1;
			}
		}

                break;
        case TIP_Delay_1:
		// small delay
                if( ! autoapp.delay_cnt-- ){
			// mover parameter init
			autoapp.mv_count = 0; 
			autoapp.mv_dir   = 0; 
			autoapp.mv_step_count = 0;
			// setup for TIP_Delay_2
			autoapp.delay_cnt = autoapp.n_wait;
			// next state
                        autoapp.tip_mode = TIP_ZSteps;
		}
                break;
        case TIP_ZSteps:
		// Run coarse approach steps with mover
		// If mover steps are done, TIP_Delay_2 state is selected within run_mover ()
		run_mover ();
		break;
        case TIP_Delay_2:
		// small delay
		// next state if delay is elapsed, repeat...
                if( ! autoapp.delay_cnt-- )
                        autoapp.tip_mode = TIP_ZPiezoMax;
                break;
	}
}

// run the job, select if auto approach or (otherwise) manual mover action
void run_autoapp (){
	if (autoapp.mover_mode & AAP_MOVER_XP_AUTO_APP)
		run_tip_app (); 
	else
		run_mover ();
}
