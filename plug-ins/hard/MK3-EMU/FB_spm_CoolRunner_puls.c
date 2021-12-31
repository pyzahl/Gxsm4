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
#include "ReadWrite_GPIO.h"
#include "FB_spm_CoolRunner_puls.h"

extern SPM_STATEMACHINE state;
extern ANALOG_VALUES    analog;
extern AUTOAPPROACH     autoapp;
extern CR_OUT_PULSE     CR_out_pulse;

#ifndef DSPEMU
#pragma CODE_SECTION(init_CR_out_pulse, ".text:slow")
#pragma CODE_SECTION(stop_CR_out_pulse, ".text:slow")
#pragma CODE_SECTION(run_CR_out_pulse, ".text:slow")
#endif

// CoolRunner "name" is historic only, now using FPGA GPIO port, one 16 bit port, 
// must be configured previously for direction bits!

// initialize for mover and autoapproach
#ifndef DSPEMU
#pragma CODE_SECTION(init_CR_out_pulse, ".text:slow")
#endif

void init_CR_out_pulse (){
	// now prepare for starting...
	DSP_MEMORY(CR_out_pulse).start = 0;
	DSP_MEMORY(CR_out_pulse).stop  = 0;

	// reset counters
	DSP_MEMORY(CR_out_pulse).i_per = 0;	
	DSP_MEMORY(CR_out_pulse).i_rep = 0;	

	// enable job
	DSP_MEMORY(CR_out_pulse).pflg = 1;
}

// stop all mover/approach actions, cleanup
#ifndef DSPEMU
#pragma CODE_SECTION(stop_CR_out_pulse, ".text:slow")
#endif

void stop_CR_out_pulse (){
	WR_GPIO (GPIO_Data_0, &DSP_MEMORY(CR_out_pulse).reset_bcode, 1);
	DSP_MEMORY(CR_out_pulse).stop  = 0;
	DSP_MEMORY(CR_out_pulse).pflg  = 0;
}

#ifndef DSPEMU
#pragma CODE_SECTION(run_CR_out_pulse, ".text:slow")
#endif

void run_CR_out_pulse (){
	if (DSP_MEMORY(CR_out_pulse).i_per == 0)
//		Port[DSP_MEMORY(CR_out_pulse).io_address] = DSP_MEMORY(CR_out_pulse).on_bcode;
		WR_GPIO (GPIO_Data_0, &DSP_MEMORY(CR_out_pulse).on_bcode, 1);

	if (DSP_MEMORY(CR_out_pulse).i_per == DSP_MEMORY(CR_out_pulse).duration)
//		Port[DSP_MEMORY(CR_out_pulse).io_address] = DSP_MEMORY(CR_out_pulse).off_bcode;
		WR_GPIO (GPIO_Data_0, &DSP_MEMORY(CR_out_pulse).off_bcode, 1);

	if (DSP_MEMORY(CR_out_pulse).i_per++ == DSP_MEMORY(CR_out_pulse).period){
		DSP_MEMORY(CR_out_pulse).i_per = 0;	
		if (DSP_MEMORY(CR_out_pulse).i_rep++ == DSP_MEMORY(CR_out_pulse).number)
			stop_CR_out_pulse ();
	}
}
