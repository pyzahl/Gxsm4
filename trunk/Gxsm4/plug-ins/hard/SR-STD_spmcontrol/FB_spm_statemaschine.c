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

#include "FB_spm_statemaschine.h"
#include "FB_spm_analog.h"

extern SPM_STATEMACHINE state;
extern SPM_PI_FEEDBACK  feedback;
extern ANALOG_VALUES    analog;
extern MOVE_OFFSET      move;
extern AREA_SCAN        scan;
extern PROBE            probe;
extern AUTOAPPROACH     autoapp;
extern CR_OUT_PULSE     CR_out_pulse;
extern CR_GENERIC_IO    CR_generic_io;

extern	void asm_calc_setpoint_log ();
extern	void asm_calc_setpoint_lin ();

/*
 *	DSP idle loop, runs for ever, returns never !!!!
 *  ============================================================
 *	Main of the DSP State Maschine
 *      - State Status "STMMode", heartbeat
 *      - manage process commands via state, 
 *        this may change the state
 *      - enable/disable of all tasks
 */

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

//int *extmem = (int*)0xc000;

int setpoint_old = 0;

int AIC_stop_mode = 0;
int sleepcount = 0;

void dsp_idle_loop (void){

	for(;;){	/* forever !!! */

		/* Sate Machines heartbeat... */
		if (state.BLK_count >= state.BLK_Ncount){
			state.BLK_count=0L;
			++state.DSP_time;
			if (++state.DSP_tens == 10){
				state.mode ^= MD_BLK;
				state.DSP_tens = 0;
			}
		}

		/* state change request? */
		if (state.set_mode){
				state.mode |= state.set_mode;
				state.set_mode = 0;
		}
		if (state.clr_mode){
				state.mode &= ~state.clr_mode;
				state.clr_mode = 0;
		}

		/* need to recalculate "soll" via log transform? */
		if (feedback.setpoint != setpoint_old){
			setpoint_old = feedback.setpoint;
			if (state.mode & MD_LOG)
				asm_calc_setpoint_log ();  // calculate "soll" from new setpoint via log trafo
			else
				asm_calc_setpoint_lin ();  // set new "soll" from new setpoint (no trafo)
		}

		/* PID-feedback on/off only via flag MD_PID -- nothing to do here */

		/* Start Offset Moveto ? */
		if (move.start && !autoapp.pflg)
			init_offset_move ();

		/* Start Area Scan ? */
		if (scan.start && !autoapp.pflg)
			init_area_scan ();
		if (scan.stop){
			scan.stop  = 0;
			scan.pflg = 0;
		}

		/* Start Probe ? */
		if (probe.start && !probe.pflg && !autoapp.pflg){
			init_probe_fifo (); // reset probe fifo!
			init_probe ();
		}
		if (probe.stop)
			stop_probe ();

		/* Start Autoapproach/run Mover ? */
		if (autoapp.start && !probe.pflg && !scan.pflg)
				init_autoapp ();
		if (autoapp.stop)
				stop_autoapp ();

		/* Start CoolRunner IO pulse ? */
		if (CR_out_pulse.start)
			init_CR_out_pulse ();
		if (CR_out_pulse.stop)
			stop_CR_out_pulse ();

		/* Do CoolRunner generic IO ? */
		if (CR_generic_io.start){
			if (CR_generic_io.pflg){ // RateMeter Mode
				switch (CR_generic_io.pflg){
				case 1: setup_counter(); restart_counter(); CR_generic_io.pflg=2; /* extmem[1]=0; */ break;
				case 2: update_count(); 
					if (CR_generic_io.rw & 1){
						if (CR_generic_io.xtime > CR_generic_io.peak_holdtime){
							CR_generic_io.peak_count = CR_generic_io.peak_count_cur;
							CR_generic_io.peak_count_cur = CR_generic_io.count;
							CR_generic_io.xtime = 0L;
//							++extmem[1];
						}
						if (CR_generic_io.count > CR_generic_io.peak_count_cur) 
							CR_generic_io.peak_count_cur = CR_generic_io.count;
					}
/*
					extmem[0] = CR_generic_io.rw;
					extmem[2] = 0xEEEE;
					extmem[3] = 0xffff;
					extmem[4] = 0x1234;
*/
					break;
				}
			} else {
				switch (CR_generic_io.port){
				case 0: CR_generic_io.data_lo = Port0; break;
				case 1: CR_generic_io.data_lo = Port1; break;
				case 2: CR_generic_io.data_lo = Port2; break;
				case 3: CR_generic_io.data_lo = Port3; break;
				case 4: Port4 = CR_generic_io.data_lo; break;
				case 5: Port5 = CR_generic_io.data_lo; break;
				case 6: Port6 = CR_generic_io.data_lo; break;
				case 7: CR_generic_io.data_lo = Port7;
					CR_generic_io.data_hi = Port8; break;
				case 9: Port9 = CR_generic_io.data_lo; break;
				case 10: PortA = CR_generic_io.data_lo; break;
				}
				CR_generic_io.start=0;
			}
		}

#ifdef AIC_STOPMODE_ENABLE
		if (state.mode & MD_AIC_STOP){
			if (!AIC_stop_mode){
				AIC_stop_mode = 1;
				sleepcount = 0;
				stop_aic ();
			}
			if (++sleepcount > 50){
				sleepcount = 0;
				dataprocess();
			}
		} else {
			if (AIC_stop_mode){
				AIC_stop_mode = 0;
				start_aic ();
			}
		}
#endif
				
	} /* repeat idle loop forever... */
}
