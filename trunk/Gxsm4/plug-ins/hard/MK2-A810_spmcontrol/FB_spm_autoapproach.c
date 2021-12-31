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
extern CR_OUT_PULSE     CR_out_pulse;

extern DSP_INT      prbdf;

int app_ci_signum=1;
int tmp_i=0;
int tmp_s=0;
short *waveptr;

// initialize for mover and autoapproach
void init_autoapp (){
	// now prepare for starting...

        // backup
        autoapp.cp = feedback.cp;
        autoapp.ci = feedback.ci;

	// init mover
	autoapp.mv_count = 0; 
	autoapp.mv_step_count = 0;
	waveptr = &prbdf; // using probe buffer!

	// set start state for autoapp
	autoapp.fin_cnt = autoapp.n_wait_fin;
	autoapp.tip_mode = TIP_ZPiezoMax;

	tmp_s = 0;

	// enable job
	autoapp.pflg = 1;
}

// stop all mover/approach actions, cleanup
void stop_autoapp (){

	if (app_ci_signum*feedback.ci < 0) {
		// ensure proper feedback on
		feedback.cp = autoapp.cp;
		feedback.ci = autoapp.ci;
	}

	// switch off after next analog (iobuf) update
	autoapp.pflg  = 2;
}

// Dummy Macro, no IO yet!
#define IOBIT_SET(N)
#define IOBIT_CLR(N)

// generate mover/coarse motion signals from wave forms -- analog.out[] are used to define mapping
// ==> final signal distribution to OUT-channels as requested later in data process.
// ==> overrides XYZ-0 and -Scan signals if engaged as configured!
void run_wave_play (){
        // play multi channel wave form?
	if (autoapp.mover_mode & AAP_MOVER_WAVE_PLAY){
                ++tmp_s;
		if (tmp_s == 1){ // set waves to next values
                        for (tmp_i=0; tmp_i<autoapp.n_wave_channels; ++tmp_i, autoapp.mv_count++)
                                analog.out[autoapp.channel_mapping[tmp_i] & 7] = waveptr[autoapp.mv_count];
                } else { // hold waves, check hold complete
                        if (tmp_s >= autoapp.wave_speed)
                                tmp_s = 0; // start over next
                        else // hold, do nothing
                                return;
		}
                
		if (autoapp.mv_count >= autoapp.wave_length) // check if wave len exeeded, done.
			autoapp.mv_count = 0; // wave cycle completed
	} // else ... may be GPIO, ... modes

	// final rep counter check
        if (!autoapp.mv_count){
                ++autoapp.mv_step_count;
		autoapp.count_axis[autoapp.axis] += autoapp.dir;

		// all steps done?
                if(autoapp.mv_step_count >= autoapp.max_wave_cycles){
			if (autoapp.mover_mode & AAP_MOVER_AUTO_APP)
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
                // restore feedback settings
                feedback.cp = autoapp.cp;
                feedback.ci = autoapp.ci;
                
		// check for tunneling conditions
		// check for limit
		if (app_ci_signum*feedback.z > 30000 ){ // almost extended? !!!Z-Signum dependent!!!
			autoapp.tip_mode = TIP_ZPiezoMin;
			autoapp.fin_cnt = autoapp.n_wait_fin;
		}

		// delta is always <0 if no or low current!
		if (feedback.delta > 0)
			--autoapp.fin_cnt;


		if (autoapp.fin_cnt == 0){
			autoapp.tip_mode = TIP_Ready; // dummy state
			autoapp.pflg  = 0; // done! Cancel autoapproach.
			autoapp.fin_cnt = autoapp.n_wait_fin;
		}

                break;
				
        case TIP_ZPiezoMin:
		// retract tip, using feedback reprogramming
		// reprogramm feedback ? -- feedback is and stays on!!!
		if (app_ci_signum*feedback.ci > 0) {
			// save feedback settings
			app_ci_signum = feedback.ci > 0 ? 1:-1;
                        // udpate (user may adjust while app)
			autoapp.cp = feedback.cp;
			autoapp.ci = feedback.ci;
						
			// reprogramm for reverse direction!!!
			feedback.cp = 0;
			feedback.ci = -app_ci_signum*autoapp.ci_retract;
		} else {
			// check progress
			if ( app_ci_signum*feedback.z < -30000 ){ // >= half extended?
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
			autoapp.mv_step_count = 0;
                        tmp_s = 0;
			// setup for TIP_Delay_2
			autoapp.delay_cnt = autoapp.n_wait;
			// next state
                        autoapp.tip_mode = TIP_ZSteps;
		}
                break;
        case TIP_ZSteps:
		// Run coarse approach steps using wave play -- must be pre-configured!!!
		// If mover steps are done, TIP_Delay_2 state is selected within run_wave_play ()
		if (autoapp.mover_mode & AAP_MOVER_PULSE)
			if (CR_out_pulse.pflg){
				run_CR_out_pulse (); // run pules job, will self terminate and rest it's pflg
				if (!CR_out_pulse.pflg) // check if pulse job done ?
					if (autoapp.mover_mode & AAP_MOVER_PULSE) // double check if we not got canceled
						autoapp.tip_mode = TIP_Delay_2; // proceed with next approach cycle
					else
						autoapp.pflg = 0; // done, max number steps exceeded
			} else
				init_CR_out_pulse (); // initiate pulse job
		else
			run_wave_play (); // run mover job, will self terminate and initiate next state
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
	if (autoapp.mover_mode & AAP_MOVER_AUTO_APP)
		run_tip_app (); 
	else
		run_wave_play ();
}
