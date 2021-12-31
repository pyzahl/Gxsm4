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
#include "FB_spm.h"
#include "spm_log.h"
#include "dataprocess.h"
#include "ReadWrite_GPIO.h"
#include "FB_spm_offsetmove.h"
#include "FB_spm_areascan.h"
#include "FB_spm_probe.h"
#include "FB_spm_autoapproach.h"
#include "FB_spm_CoolRunner_puls.h"

#ifdef USE_PLL_API
#include "PAC_pll.h"
#endif

/*
 *	DSP idle loop, runs for ever, returns never !!!!
 *  ============================================================
 *	Main of the DSP State Maschine
 *      - State Status "STMMode", heartbeat
 *      - manage process commands via state, 
 *        this may change the state
 *      - enable/disable of all tasks
 */

DSP_INT32 setpoint_old = 0;
DSP_INT32 mix_setpoint_old[4] = {0,0,0,0};

int AIC_stop_mode = 0;
int sleepcount = 0;

#ifndef DSPEMU
#pragma CODE_SECTION(dsp_idle_loop, ".text:slow")
#endif

void dsp_idle_loop (void){
        int i;
//	short io;

	/* configure GPIO all in for startup safety -- this is ans should be default power-up, just to make sure again */

//	io = 0x0000; WR_GPIO (GPIO_Dir_0, &io, 1);

#ifndef DSPEMU
	for(;;){	/* forever !!! */
#else
        {
#endif
		++DSP_MEMORY(magic).life;

		/* state change request? */
		if (DSP_MEMORY(state).set_mode){
                        DSP_MEMORY(state).mode |= DSP_MEMORY(state).set_mode;
                        DSP_MEMORY(state).set_mode = 0;
		}
		if (DSP_MEMORY(state).clr_mode){
                        DSP_MEMORY(state).mode &= ~DSP_MEMORY(state).clr_mode;
                        DSP_MEMORY(state).clr_mode = 0;
		}

		/* PID-feedback on/off only via flag MD_PID -- nothing to do here */

		/* (test) tool for log execution */
		if (DSP_MEMORY(feedback_mixer).exec){
			DSP_MEMORY(feedback_mixer).lnx = calc_mix_log (DSP_MEMORY(feedback_mixer).x, DSP_MEMORY(feedback_mixer).I_offset);
			DSP_MEMORY(feedback_mixer).exec = 0;
		}

		/* DSP level setpoint calculation for log mode */
		for (i=0; i<4; ++i){
			if (DSP_MEMORY(feedback_mixer).setpoint[i] != mix_setpoint_old[i]){ // any change?
				DSP_MEMORY(feedback_mixer).setpoint_log[i] = calc_mix_log (DSP_MEMORY(feedback_mixer).setpoint[i], DSP_MEMORY(feedback_mixer).I_offset);
				mix_setpoint_old[i] = DSP_MEMORY(feedback_mixer).setpoint[i];
			}
		}

		/* module signal configuration management */
		if (DSP_MEMORY(sig_mon).mindex >= 0){
			if (DSP_MEMORY(sig_mon).signal_id < 0)
				query_signal_input ();  // read back a input signal configuration
			else
				adjust_signal_input ();  // adjust a input signal configuration
		}		

		/* Start Offset Moveto ? */
		if (DSP_MEMORY(move).start && !DSP_MEMORY(autoapp).pflg)
			init_offset_move ();

		/* Start/Stop/Pause/Resume Area Scan ? */
		if (DSP_MEMORY(scan).start && !DSP_MEMORY(autoapp).pflg)
			init_area_scan ();

		switch (DSP_MEMORY(scan).stop){
		case 0: break;
		case AREA_SCAN_STOP:	 DSP_MEMORY(scan).stop = 0; DSP_MEMORY(scan).pflg = 0; stop_lockin (PROBE_RUN_LOCKIN_SCAN); break; // Stop/Cancel/Abort scan
		case AREA_SCAN_PAUSE:	 DSP_MEMORY(scan).stop = 0; DSP_MEMORY(scan).pflg &= (AREA_SCAN_START_NORMAL|AREA_SCAN_START_FASTSCAN); break; // Pause Scan
		case AREA_SCAN_RESUME: 
			if (DSP_MEMORY(scan).pflg & (AREA_SCAN_START_NORMAL|AREA_SCAN_START_FASTSCAN)){
				DSP_MEMORY(scan).stop = 0; DSP_MEMORY(scan).pflg |= AREA_SCAN_RUN; 
			} break; // Resume Scan from Pause, else ignore
		default: break;
		}

		/* Start Probe ? */
		if (DSP_MEMORY(probe).start && !DSP_MEMORY(probe).pflg && !DSP_MEMORY(autoapp).pflg){
			if (DSP_MEMORY(probe).start == PROBE_RUN_LOCKIN_FREE)
				init_lockin (PROBE_RUN_LOCKIN_FREE);
			else {
				init_probe_fifo (); // reset probe fifo!
				init_probe ();
			}
		}
		if (DSP_MEMORY(probe).stop){
			if (DSP_MEMORY(probe).stop == PROBE_RUN_LOCKIN_FREE)
				stop_lockin (PROBE_RUN_LOCKIN_FREE);
			else
				stop_probe ();
		}
		if (DSP_MEMORY(probe).pflg){
			if (DSP_MEMORY(probe).vector->options & VP_GPIO_SET){
				i =  (DSP_MEMORY(probe).vector->options & VP_GPIO_MSK) >> 16;
				if (DSP_MEMORY(probe).gpio_setting != i){ // update GPIO!
					DSP_MEMORY(CR_generic_io).gpio_data_out = (DSP_UINT16)i;
					WR_GPIO (GPIO_Data_0, &DSP_MEMORY(CR_generic_io).gpio_data_out, 1);
					DSP_MEMORY(probe).gpio_setting = i;
				}
			}
		}

		/* Start Autoapproach/run Mover ? */
		if (DSP_MEMORY(autoapp).start && !DSP_MEMORY(probe).pflg && !DSP_MEMORY(scan).pflg)
				init_autoapp ();
		if (DSP_MEMORY(autoapp).stop)
				stop_autoapp ();

		/* Start CoolRunner IO pulse ? */
		if (DSP_MEMORY(CR_out_pulse).start)
			init_CR_out_pulse ();
                if (DSP_MEMORY(CR_out_pulse).stop)
			stop_CR_out_pulse ();

		/* Do CoolRunner generic IO ? */
		if (DSP_MEMORY(CR_generic_io).start){
			switch (DSP_MEMORY(CR_generic_io).start){
			case 1: WR_GPIO (GPIO_Data_0, &DSP_MEMORY(CR_generic_io).gpio_data_out, 1); break; // write port
			case 2: WR_GPIO (GPIO_Data_0, &DSP_MEMORY(CR_generic_io).gpio_data_in, 0); break; // read port
			case 3: WR_GPIO (GPIO_Dir_0, &DSP_MEMORY(CR_generic_io).gpio_direction_bits, 1); break; // reconfigure port
			default: break;
			}
			DSP_MEMORY(CR_generic_io).start=0;
		}

#ifdef USE_PLL_API
		/* PAC/PLL controls */
		if (DSP_MEMORY(PLL_lookup).start){
			switch (DSP_MEMORY(PLL_lookup).start){
			case 1: ChangeFreqHL(); break;
			case 2: OutputSignalSetUp_HL(); break;
			case 3: TestPhasePIResp_HL(); break;
			case 4: TestAmpPIResp_HL(); break;
			case 99: StartPLL(5,7); break;
			}
			DSP_MEMORY(PLL_lookup).start = 0;
		}
#endif

		
#ifndef DSPEMU
#ifdef RECONFIGURE_DSP_SPEED
		if (DSP_MEMORY(state).DSP_speed[0] != DSP_MEMORY(state).DSP_speed[1]){
			switch (DSP_MEMORY(state).DSP_speed[1]){
			case 688:
				stop_Analog810 (); // Stop AICs now again -- testing only
				dsp_688MHz();
				start_Analog810 ();
				DSP_MEMORY(state).DSP_speed[0]=688;
				break;
			default:
				stop_Analog810 (); // Stop AICs now again -- testing only
				dsp_590MHz();
				start_Analog810 ();
				DSP_MEMORY(state).DSP_speed[0]=590;
				break;
			}
			DSP_MEMORY(state).DSP_speed[1]=DSP_MEMORY(state).DSP_speed[0];
		}
#endif
#endif
		
#ifdef AIC_STOPMODE_ENABLE

		/* DANGER!!! Analog reconfiguration may cause freeze issues if used wrong --> ISR */
		if (DSP_MEMORY(state).mode & MD_AIC_STOP){
			if (!AIC_stop_mode){
				AIC_stop_mode = 1;
				sleepcount = 0;

				/* Stop Analog810 -- Out/In may be undefined while stopped */
				stop_Analog810 ();
			}
			if (++sleepcount > 50){
				sleepcount = 0;
				dataprocess();
			}
		} else {
			if (AIC_stop_mode){
				AIC_stop_mode = 0;

				/* ReInit and Start Analog810 */
				FreqDiv   = a810_config.freq_div;     // default: =10 75kHz *** 5 ... 65536  150kHz at N=5   Fs = Clk_in/(N * 200), Clk_in=150MHz
				ADCRange  = a810_config.adc_range;    // default: =0 0: +/-10V, 1:+/-5V
				QEP_ON    = a810_config.qep_on;       // default: =1 (on) manage QEP counters
				start_Analog810 ();
			}
		}
#endif
		
	} /* repeat idle loop forever... */
}
