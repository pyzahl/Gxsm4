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

extern SPM_STATEMACHINE state;
extern ANALOG_VALUES    analog;
extern AUTOAPPROACH     autoapp;
extern CR_OUT_PULSE     CR_out_pulse;

ioport   unsigned int Port6;  /*CPLD port*/

// initialize for mover and autoapproach
void init_CR_out_pulse (){
	// now prepare for starting...
	CR_out_pulse.start = 0;
	CR_out_pulse.stop  = 0;

	// reset counters
	CR_out_pulse.i_per = 0;	
	CR_out_pulse.i_rep = 0;	

	// enable job
	CR_out_pulse.pflg = 1;
}

// stop all mover/approach actions, cleanup
void stop_CR_out_pulse (){
//	Port[CR_out_pulse.io_address] = CR_out_pulse.reset_bcode;
	Port6 = CR_out_pulse.reset_bcode;
	CR_out_pulse.stop  = 0;
	CR_out_pulse.pflg  = 0;
}

void run_CR_out_pulse (){
	if (CR_out_pulse.i_per == 0)
//		Port[CR_out_pulse.io_address] = CR_out_pulse.on_bcode;
		Port6 = CR_out_pulse.on_bcode;

	if (CR_out_pulse.i_per == CR_out_pulse.duration)
//		Port[CR_out_pulse.io_address] = CR_out_pulse.off_bcode;
		Port6 = CR_out_pulse.off_bcode;

	if (CR_out_pulse.i_per++ == CR_out_pulse.period){
		CR_out_pulse.i_per = 0;	
		if (CR_out_pulse.i_rep++ == CR_out_pulse.number)
			stop_CR_out_pulse ();
	}
}
