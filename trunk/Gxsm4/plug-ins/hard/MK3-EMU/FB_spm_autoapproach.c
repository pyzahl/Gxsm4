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
#include "FB_spm_autoapproach.h"
#include "FB_spm_CoolRunner_puls.h"
#include "dataprocess.h"

#define TIP_Ready     0
#define TIP_ZPiezoMax 1
#define TIP_ZPiezoMin 2
#define TIP_Delay_1   3
#define TIP_ZSteps    4
#define TIP_Delay_2   5

extern SPM_STATEMACHINE state;
extern SERVO            z_servo;
extern FEEDBACK_MIXER   feedback_mixer;
extern ANALOG_VALUES    analog;
extern AUTOAPPROACH     autoapp;
extern MOVE_OFFSET      move;
extern CR_OUT_PULSE     CR_out_pulse;

extern DSP_INT16      prbdf[];

int app_ci_signum=1;
int tmp_i=0;
int tmp_s=0;
DSP_INT16 *waveptr;

// initialize for mover and autoapproach
#ifndef DSPEMU
#pragma CODE_SECTION(init_autoapp, ".text:slow")
#endif

void init_autoapp (){
	// now prepare for starting...
	DSP_MEMORY(autoapp).start = 0;
	DSP_MEMORY(autoapp).stop  = 0;

	// init mover
	DSP_MEMORY(autoapp).mv_count = 0; 
	DSP_MEMORY(autoapp).mv_dir   = 0; 
	DSP_MEMORY(autoapp).mv_step_count = 0;
	waveptr = &DSP_MEMORY(prbdf[0]); // using probe buffer!

	// set start state for autoapp
	DSP_MEMORY(autoapp).fin_cnt = DSP_MEMORY(autoapp).n_wait_fin;
	DSP_MEMORY(autoapp).tip_mode = TIP_ZPiezoMax;

	tmp_s = DSP_MEMORY(autoapp).piezo_speed;

	// enable job
	DSP_MEMORY(autoapp).pflg = 1;
}

// stop all mover/approach actions, cleanup
#ifndef DSPEMU
#pragma CODE_SECTION(stop_autoapp, ".text:slow")
#endif

void stop_autoapp (){
	DSP_MEMORY(autoapp).start = 0;
	DSP_MEMORY(autoapp).stop  = 0;

	if (app_ci_signum*DSP_MEMORY(z_servo).ci < 0) {
		// ensure proper feedback on
		DSP_MEMORY(z_servo).cp = DSP_MEMORY(autoapp).cp;
		DSP_MEMORY(z_servo).ci = DSP_MEMORY(autoapp).ci;
	}

	// enable FB
	if(!(DSP_MEMORY(state).mode & MD_PID))
		DSP_MEMORY(state).mode |= MD_PID;

	// switch off after next analog (iobuf) update
	DSP_MEMORY(autoapp).pflg  = 2;
}

// Dummy Macro, no IO yet!
#define IOBIT_SET(N)
#define IOBIT_CLR(N)



// generate mover ramp signal (parabel sections)
#ifndef DSPEMU
#pragma CODE_SECTION(run_mover, ".text:slow")
#endif

void run_mover (){
	int axis=0;
	int dir=0;
	tmp_i = DSP_MEMORY(autoapp).mv_count;
	// waveform mode or default simple parabulum mode?
	if (DSP_MEMORY(autoapp).mover_mode & AAP_MOVER_WAVE){
		switch (DSP_MEMORY(autoapp).mover_mode & AAP_MOVER_DIRMMSK) {
		case AAP_MOVER_XP:
			axis=0; dir=1;
			if (DSP_MEMORY(autoapp).mover_mode & AAP_MOVER_IWMODE){ // IW mode fwd
				DSP_MEMORY(analog).wave[0] = waveptr[tmp_i];
				tmp_i += DSP_MEMORY(autoapp).u_piezo_amp;  // this is the phase offset in IW mode!
				if (tmp_i >= DSP_MEMORY(autoapp).u_piezo_max) // u_piezo_max is wave len in all wave modes
					tmp_i -= DSP_MEMORY(autoapp).u_piezo_max;
				DSP_MEMORY(analog).wave[1] = waveptr[tmp_i];
			} else
				DSP_MEMORY(analog).wave[0] = waveptr[tmp_i]; 
			break;
		case AAP_MOVER_XM:
			axis=0; dir=-1;
			if (DSP_MEMORY(autoapp).mover_mode & AAP_MOVER_IWMODE){ // IW mode rev
				DSP_MEMORY(analog).wave[0] = waveptr[DSP_MEMORY(autoapp).u_piezo_max-tmp_i-1];
				tmp_i += DSP_MEMORY(autoapp).u_piezo_amp;  // this is the phase offset in IW mode!
				if (tmp_i >= DSP_MEMORY(autoapp).u_piezo_max)
					tmp_i -= DSP_MEMORY(autoapp).u_piezo_max;
				DSP_MEMORY(analog).wave[1] = waveptr[DSP_MEMORY(autoapp).u_piezo_max-tmp_i-1];
			} else
				DSP_MEMORY(analog).wave[0] = -waveptr[tmp_i]; 
			break;
		case AAP_MOVER_YP: 
			axis=1; dir=1;
			DSP_MEMORY(analog).wave[1] = waveptr[tmp_i]; break;
		case AAP_MOVER_YM: 
			axis=1; dir=-1;
			DSP_MEMORY(analog).wave[1] = -waveptr[tmp_i]; break;
		case AAP_MOVER_OFF: 
			DSP_MEMORY(analog).wave[0] = 0; DSP_MEMORY(analog).wave[1] = 0; break;
		default: break;
		}
		--tmp_s;
		if (tmp_s <= 0){ // slow down factor
			DSP_MEMORY(autoapp).mv_count++;
			tmp_s = DSP_MEMORY(autoapp).piezo_speed;
		} else return;
		if (DSP_MEMORY(autoapp).mv_count >= DSP_MEMORY(autoapp).u_piezo_max) // check if wave len exeeded, done.
			DSP_MEMORY(autoapp).mv_count = 0;
	}

	// final rep counter check
        if (!DSP_MEMORY(autoapp).mv_count){
		DSP_MEMORY(autoapp).mv_dir=0;
                ++DSP_MEMORY(autoapp).mv_step_count;

		if (DSP_MEMORY(autoapp).mover_mode & AAP_MOVER_XP_AUTO_APP) 
			axis=2;
		DSP_MEMORY(autoapp).count_axis[axis] += dir;

		// all steps done?
                if(DSP_MEMORY(autoapp).mv_step_count >= DSP_MEMORY(autoapp).piezo_steps){
			DSP_MEMORY(analog).wave[0] = 0;
			DSP_MEMORY(analog).wave[1] = 0;

			if (DSP_MEMORY(autoapp).mover_mode & AAP_MOVER_XP_AUTO_APP)
				DSP_MEMORY(autoapp).tip_mode = TIP_Delay_2; // proceed with next approach cycle
			else
				DSP_MEMORY(autoapp).pflg = 0; // done, max number steps exceeded
			return;
                }
        }
}

// statemaschine: coordinates automatic tip approach, start with state
// (statevar is DSP_MEMORY(autoapp).tip_mode) "TIP_ZPiezoMax"
#ifndef DSPEMU
#pragma CODE_SECTION(run_tip_app, ".text:slow")
#endif

#define VAL_EXTEND (30000*(1<<16))   // Q15.16 d30000.16
void run_tip_app (){
        switch(DSP_MEMORY(autoapp).tip_mode){
        case TIP_ZPiezoMax:
		// enable feedback, Z-approach via CI,CP
                if(!(DSP_MEMORY(state).mode & MD_PID))     // need to start feedback?
                        DSP_MEMORY(state).mode |= MD_PID;     // enable feedback

		// check for tunneling conditions
		// check for limit
		if (app_ci_signum*DSP_MEMORY(z_servo).neg_control > VAL_EXTEND){ // almost extended? !!!Z-Signum dependent!!!
			DSP_MEMORY(autoapp).tip_mode = TIP_ZPiezoMin;
			DSP_MEMORY(autoapp).fin_cnt = DSP_MEMORY(autoapp).n_wait_fin;
		}

		// delta is always <0 if no or low current!
		if (DSP_MEMORY(z_servo).delta > 0)
			--DSP_MEMORY(autoapp).fin_cnt;


		if (DSP_MEMORY(autoapp).fin_cnt == 0){
			DSP_MEMORY(autoapp).tip_mode = TIP_Ready; // dummy state
			DSP_MEMORY(autoapp).pflg  = 0; // done! Cancel autoapproach.
			DSP_MEMORY(autoapp).fin_cnt = DSP_MEMORY(autoapp).n_wait_fin;
		}

                break;
				
        case TIP_ZPiezoMin:
		// retract tip, using feedback reprogramming
		// reprogramm feedback ? -- feedback is and stays on!!!
		if (app_ci_signum*DSP_MEMORY(z_servo).ci > 0) {
			// save feedback settings
			app_ci_signum = DSP_MEMORY(z_servo).ci > 0 ? 1:-1;
			DSP_MEMORY(autoapp).cp = DSP_MEMORY(z_servo).cp;
			DSP_MEMORY(autoapp).ci = DSP_MEMORY(z_servo).ci;
						
			// reprogramm for reverse direction!!!
			DSP_MEMORY(z_servo).cp = 0;
			DSP_MEMORY(z_servo).ci *= -DSP_MEMORY(autoapp).ci_mult;
		} else {
			// check progress
			if ( app_ci_signum*DSP_MEMORY(z_servo).neg_control < -VAL_EXTEND ){ // retracted?
				DSP_MEMORY(state).mode &= ~MD_PID;     // disable feedback
				// restore feedback settings
				DSP_MEMORY(z_servo).cp = DSP_MEMORY(autoapp).cp;
				DSP_MEMORY(z_servo).ci = DSP_MEMORY(autoapp).ci;

				DSP_MEMORY(autoapp).delay_cnt = DSP_MEMORY(autoapp).n_wait;
				// next state
				DSP_MEMORY(autoapp).tip_mode = TIP_Delay_1;
			}
		}

                break;
        case TIP_Delay_1:
		// small delay
                if( ! DSP_MEMORY(autoapp).delay_cnt-- ){
			// mover parameter init
			DSP_MEMORY(autoapp).mv_count = 0; 
			DSP_MEMORY(autoapp).mv_dir   = 0; 
			DSP_MEMORY(autoapp).mv_step_count = 0;
			// setup for TIP_Delay_2
			DSP_MEMORY(autoapp).delay_cnt = DSP_MEMORY(autoapp).n_wait;
			// next state
                        DSP_MEMORY(autoapp).tip_mode = TIP_ZSteps;
		}
                break;
        case TIP_ZSteps:
		// Run coarse approach steps with mover or using GPIO pulse -- must be pre-configured!!!
		// If mover steps are done, TIP_Delay_2 state is selected within run_mover ()
		if (DSP_MEMORY(autoapp).mover_mode & AAP_MOVER_PULSE)
			if (DSP_MEMORY(CR_out_pulse).pflg){
				run_CR_out_pulse (); // run pules job, will self terminate and rest it's pflg
				if (!DSP_MEMORY(CR_out_pulse).pflg){ // check if pulse job done ?
					if (DSP_MEMORY(autoapp).mover_mode & AAP_MOVER_XP_AUTO_APP) // double check if we not got canceled
						DSP_MEMORY(autoapp).tip_mode = TIP_Delay_2; // proceed with next approach cycle
					else
						DSP_MEMORY(autoapp).pflg = 0; // done, max number steps exceeded
				}
			} else
				init_CR_out_pulse (); // initiate pulse job
		else
			run_mover (); // run mover job, will self terminate and initiate next state
		break;
        case TIP_Delay_2:
		// small delay
		// next state if delay is elapsed, repeat...
                if( ! DSP_MEMORY(autoapp).delay_cnt-- )
                        DSP_MEMORY(autoapp).tip_mode = TIP_ZPiezoMax;
                break;
	}
}

// run the job, select if auto approach or (otherwise) manual mover action

#ifndef DSPEMU
#pragma CODE_SECTION(run_autoapp, ".text:slow")
#endif

void run_autoapp (){
	if (DSP_MEMORY(autoapp).mover_mode & AAP_MOVER_XP_AUTO_APP)
		run_tip_app (); 
	else
		run_mover ();
}
