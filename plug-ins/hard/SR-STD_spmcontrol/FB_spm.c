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

/*
 * code derived of the PCI32 TMS320C32 DSP code "xsm.c, ..." and the SRanger FB_Demo.c
 * 20030107-PZ: start of porting
 * CVS FLICK2
 */
                    
#include "FB_spm.h"  
#include "dataprocess.h"
#include "FB_spm_statemaschine.h"
#include "FB_spm_analog.h"

/*
 * SPM subsystem
 * ==================================================
 * This structures are read/write accessed by Gxsm
 * for SPM control. 
 * (all define via #include "FB_spm_dataexchange.h")
 */

// initializing of global structs in this way saves a lot of space!!

// IMPORTANT:
// KEEP THIS LISTS COMPATIBLE WITH STRUCT DEFINITIONS !!!!!

/* Init SPM state maschine */
SPM_STATEMACHINE state = {
		0,0,  // set, clear_mode
		MD_IDLE,   //	state.mode       = MD_IDLE;
		MD_PID | MD_LOG | MD_OFFSETADDING,    //	state.last_mode  = MD_READY | MD_PID | MD_OFFSETCOMP;
		0,    //	state.BLK_count  = 0;
		2000, //	state.BLK_Ncount = 2000;
		0,    //	state.DSP_time   = 0;
		0,    //	state.DSP_tens   = 0;
		0,    //	state.DataProcessTime = 0;
		0,    //	state.IdleTime   = 0;
		0, 0, //        2x-dito .._peak
		AIC_OFFSET_COMPENSATION    //	state.DataProcessMode = AIC_OFFSET_COMPENSATION;
};

/* Init Feedback */
SPM_PI_FEEDBACK  feedback = {
		327,   //	feedback.cp = 327;
		327,   //	feedback.ci = 327;
		0,     //	feedback.setpoint = 0;
		-6136, //	feedback.soll = -6136;
		0,     //	feedback.ist = 0;
		0,     //	feedback.delta = 0;
		0L,    //	feedback.i_pl_sum = 0;
		0L,    //	feedback.i_sum = 0;
		0      //	feedback.z = 0;
};

/* Init external control parameter */
EXT_CONTROL external = {
                1,     //   external.FB_control_channel = -1;
                5242,   //   external.FB_control_treshold = 5242;
                0,     //   external.watch_value = 0;
                0,     //   external.watch_min = 0;
                32767  //   external.watch_max = 32727;
};
                
/* Init DFM_FUZZYMIX values */
DFM_FUZZYMIX dfm_fuzzymix = {
	10000, // dfm_fuzzymix.level;
	-8000  // dfm_fuzzymix.gain;
};

/* Init all simple DA out: U and other simple steady state DAC channels */
ANALOG_VALUES    analog = {
		0,0,0,0, 0,0,0,0, // in0..7
		0,    //	analog.x_offset = 0;
		0,    //	analog.y_offset = 0;
		0,    //	analog.z_offset = 0;
		0,    //	analog.x_scan = 0;
		0,    //	analog.y_scan = 0;
		0,    //	analog.z_scan = 0;
		VOLT2DAC (0.5),    //	analog.bias = VOLT2DAC (2.0);
		0,    //	analog.aux7 = 0;
		0,0,0,0, 0,0,0,0,   // offset[8]
		{ 0L,0L,0L,0L, 0L,0L,0L,0L }, // AIC avg
		{ 0L,0L,0L,0L, 0L,0L,0L,0L }, // AIC slow
		{ 64,64,64,64, 64,64,0x4000,0x4000}, // length
		// pipes....
//		{ 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 }, // 64
		{ 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 }, // 32
		{ 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 }, // 32
		{ 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 }, // 32
		{ 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 }, // 32
		{ 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 }, // 32
		{ 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 }, // 32
		{ 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
		  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
		}, // 128
		{ 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
		  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
		}, // 128
		0,0,0,0, 0,0,0,0   // out_offset[8]
};

/* Init Move: Offset */
MOVE_OFFSET      move = {
		0,    //	move.start = 0;
		1,    //	move.nRegel= 1;
		0, 0, //	move.Xnew = move.Ynew = 0;
		0, 0, 0, //	move.f_dx = move.f_dy = move.f_dz = 0;
		0L,    //	move.num_steps = 0;
		0, 0, 0, //	move.XPos = move.YPos = move.ZPos = 0;
		0     //	move.pflg  = 0;
};

/* Init Scan: Scan Control Process, Speed, Rotation, Dimensions, Sizes */
AREA_SCAN        scan = {
		0,    //	scan.start= 0;
		0,    //	scan.stop = 0;
		1<<15,  //	scan.rotxx = 1<<15;
		0,    //	scan.rotxy = 0;
		0,    //	scan.rotyx = 0;
		1<<15,  //	scan.rotyy = 1<<15;
		0,      //	scan.nx_pre = 0;
		0,      //	scan.dnx_probe = 0;
		0, 0,   //      scan.raster_a, _b= 0;
		0, 0,   //	scan.srcs_xp,xm  = 0;
		0, 0,   //	scan.srcs_2nd_xp,2nd_xm  = 0;
		0,    //	scan.nx    = 0;
		0,    //	scan.ny    = 0;
		1L<<16,    //	scan.fs_dx  = 1L<<16;
		1L<<16,    //	scan.fs_dy  = 1L<<16;
		0L,        //   scan.num_steps_move_xy = 0L;
		1L<<16,    //	scan.fm_dx  = 1L<<16;
		1L<<16,    //	scan.fm_dy  = 1L<<16;
		1,    //	scan.dnx   = 1;
		1,    //	scan.dny   = 1;
		0, 0, //        scan.Zoff_2nd_xp = 0, scan.Zoff_2nd_xm = 0;
		0L, 0L, //      scan.fm_dzx, _dzy
		0L,    //	scan.Xpos  = 0;
		0L,    //	scan.Ypos  = 0;
		0L, 0L, //      scan.cfs_dx = cfs_dy = 0
		0,    //	scan.iix    = 0;
		0,    //	scan.iiy    = 0;
		0,    //	scan.ix    = 0;
		0,    //	scan.iy    = 0;
		0,    //	scan.sstate = 0;
		0     //	scan.pflg = 0;
};

SCAN_EVENT_TRIGGER scan_event_trigger = {
	-1,-1,-1,-1, // xp bias
	-1,-1,-1,-1, // xm
	-1,-1,-1,-1, // xp setpoint
	-1,-1,-1,-1, // xm
	0,0,0,0,     // biases
	0,0,0,0,
	0,0,0,0,     // setpoints
	0,0,0,0,
	0 // pflg
};

/* Init Probe: */
PROBE            probe = {
	0,    //	probe.start = 0;
	0,    //	probe.stop  = 0;
	0,    //	probe.AC_amp = 0;
	0,    //	probe.AC_frq = 0;
	0,    //	probe.AC_phaseA = 0;
	0,    //	probe.AC_phaseB = 0;
	0,    //	probe.AC_nAve = 0;
	0,    //        probe.AC_ix = 0;
	0,    //        probe.time = 0;
	0,    //        probe.Upos = 0;
	0,    //        probe.Zpos = 0;
	0,    //        probe.LockIn_0 = 0;
	0,    //        probe.LockIn_1stA = 0;
	0,    //        probe.LockIn_1stB = 0;
	0,    //        probe.LockIn_2ndA = 0;
	0,    //        probe.LockIn_2ndB = 0;
	0,    //        sinreftabA
	0,    //        sinreftabA length
	0,    //        sinreftabB
	0,    //        sinreftabB length
	EXTERN_PROBE_VECTOR_HEAD_DEFAULT_P, //      PROBE_VECTOR *vector_head; -- default vector head, may be changed later
	NULL, //        PROBE_VECTOR *vector;
	0, 0,    //	probe.ix = probe.iix = 0;
	0,    //	probe.state = 0;
	0     //	probe.pflg  = 0;
};

/* Init autoapproach */
AUTOAPPROACH     autoapp = {
		0,    //	autoapp.start = 0;
		0,    //	autoapp.stop  = 0;
		0,    //	autoapp.mover_mode  = 0;
		6,    //	autoapp.piezo_steps = 6;
		2,    //	autoapp.n_wait      = 2;
		8000,    //	autoapp.u_piezo_max = 8000;
		16000,   //	autoapp.u_piezo_amp = 16000;
		2,    //	autoapp.piezo_speed = 2;
		0,    //	autoapp.mv_count    = 0;
		0,    //	autoapp.mv_dir      = 0;
		0,    //	autoapp.mv_step_count = 0;
		0,    //	autoapp.u_piezo   = 0;
		0,    //	autoapp.step_next = 0;
		0,    //	autoapp.tip_mode  = 0;
		0,    //	autoapp.delay_cnt = 0;
		20,   //	autoapp.ci_mult   = 20;
		0,    //	autoapp.cp        = 0;
		0,    //	autoapp.ci        = 0;
		0     //	autoapp.pflg      = 0;
};

CR_OUT_PULSE      CR_out_pulse = {
	0, // DSP_INT     start;           /* Initiate =WO */
	0, // DSP_INT     stop;            /* Cancel   =WO */
	10, // DSP_INT     duration;        /* Puls duration count =WR */
	100, // DSP_INT     period;          /* Puls period count =WR */
	1, //DSP_INT     number;          /* number of pulses =WR */
	0, //DSP_INT     on_bcode;        /* IO value at "on", while "duration" =WR */
	0, //DSP_INT     off_bcode;       /* IO value at "off", while remaining time of period =WR */
	0, //DSP_INT     reset_bcode;     /* IO value at end/cancel =WR */
	0, //DSP_INT     io_address;      /* IO address to use =WR */
	0, //DSP_INT     i_per;            /* on counter =RO */
	0, //DSP_INT     i_rep;           /* repeat counter =RO */
	0  //DSP_INT     pflg;            /* process active flag =RO */
};

CR_GENERIC_IO    CR_generic_io = {
	0, // DSP_INT     start;           /* Initiate =WO */
	0, // DSP_INT     stop;            /* Cancel   =WO */
	0, 0, // DSP_INT io port, io_rw 
	0, 0, // DSP_UINT io_data_lo, io_data_hi
	0, 0, 13L, // mode, gatetime, count
	999L, 888L, 0, 100L, // peak, peak_cur, xtime, peak_holdtime
	0  //DSP_INT     pflg;            /* process active flag =RO */
};


/* Data Fifo Struct */
DATA_FIFO        datafifo = {
		0, 0, // datafifo.r_position, datafifo.w_position
		0, 0, // datafifo.fill, datafifo.stall
		DATAFIFO_LENGTH, // datafifo.length
		1,    // datafifo.mode
		1,2,3,4,5,6,7,8 // first data...
};

/* Probe Data Fifo Struct */
DATA_FIFO_EXTERN probe_datafifo = {
		0, 0, // datafifo.r_position, datafifo.w_position
		0, 0, // current_section_head_positon, section_index
		0, 0, // datafifo.fill, datafifo.stall
		EXTERN_PROBEDATAFIFO_LENGTH, // datafifo.length
		EXTERN_DATA_FIFO_ADDRESS_P,
		NULL,
		NULL
};

void main()
{
	/* Extern structure of AIC registers */
	extern	struct	iobufdef    iobuf;
	extern	struct aicregdef    aicreg;

	/* fixed location of magic struct, reserved by SR Memlayout (Vis.Linker) */
	SPM_MAGIC_DATA_LOCATIONS    *magic = (SPM_MAGIC_DATA_LOCATIONS*) FB_SPM_MAGIC_ADR;

	/* fill magic struct, contains version info and pointers to data structs */
	magic->magic        = 0x0000; /* not valid yet! */
	magic->version      = FB_SPM_VERSION;
	magic->year         = FB_SPM_DATE_YEAR;
	magic->mmdd         = FB_SPM_DATE_MMDD;
	magic->dsp_soft_id  = FB_SPM_SOFT_ID;
	magic->statemachine = (unsigned int)(&state);
	magic->AIC_in       = (unsigned int)(&iobuf.min0);
	magic->AIC_out      = (unsigned int)(&iobuf.mout0);
	magic->AIC_reg      = (unsigned int)(&aicreg);
	magic->feedback     = (unsigned int)(&feedback);
	magic->analog       = (unsigned int)(&analog);
	magic->scan         = (unsigned int)(&scan);
	magic->move         = (unsigned int)(&move);
	magic->probe        = (unsigned int)(&probe);
	magic->autoapproach = (unsigned int)(&autoapp);
	magic->datafifo     = (unsigned int)(&datafifo);
	magic->probedatafifo= (unsigned int)(&probe_datafifo);
	magic->scan_event_trigger= (unsigned int)(&scan_event_trigger);
	magic->dfm_fuzzymix = (unsigned int)(&dfm_fuzzymix);
	magic->CR_out_puls  = (unsigned int)(&CR_out_pulse);
	magic->external     = (unsigned int)(&external);
	magic->CR_generic_io= (unsigned int)(&CR_generic_io);
 	magic->dummy        = 0x0000; /* Null Termination */
	magic->magic        = FB_SPM_MAGIC; /* now valid! */

	/* Setup LockIn tabels */

	init_probe_module ();

	/* Init AIC registers : Defaut cfg (all gains = 0 db, except marked) */
	
	aicreg.valprd=0x08;
	
	aicreg.reg10=0x6250; /* AIC0 */
	aicreg.reg11=0x4250;
	aicreg.reg12=0x2250;
	aicreg.reg13=0x0250;
	aicreg.reg14=0x6250; /* AIC4 */
/*	aicreg.reg15=0x4250; */
	aicreg.reg15=0x4254; /* CtrlReg1, D2 enables FIR Bypass mode AIC5 */
	aicreg.reg16=0x2250;
/*	aicreg.reg17=0x0250; */
	aicreg.reg17=0x0254; /* CtrlReg1, D2 enables FIR Bypass mode AIC7 */
	
	aicreg.reg20=0x6401;
	aicreg.reg21=0x4401;
	aicreg.reg22=0x2401;
	aicreg.reg23=0x0401;
	aicreg.reg24=0x6401;
	aicreg.reg25=0x4401;
	aicreg.reg26=0x2401;
	aicreg.reg27=0x0401;
	
	aicreg.reg30=0x6601;
	aicreg.reg31=0x4601;
	aicreg.reg32=0x2601;
	aicreg.reg33=0x0601;
	aicreg.reg34=0x6601;
	aicreg.reg35=0x4601;
	aicreg.reg36=0x2601;
	aicreg.reg37=0x0601;
	
	aicreg.reg40=0x6800;
	aicreg.reg41=0x4800;
	aicreg.reg42=0x2800;
	aicreg.reg43=0x0800;
	aicreg.reg44=0x6800;
	aicreg.reg45=0x480A; // Output gain is set to +6db to compensate FIR Bypass
/*	aicreg.reg45=0x48AA;  Set Gain to +6dB */
	aicreg.reg46=0x2800;
	aicreg.reg47=0x080A; // Output gain is set to +6db to compensate FIR Bypass
		
	/* Init AIC */

	start_aic ();
//	stop_aic (); // Stop AICs now again -- testing only
	
	/*
	 * Wait for DMA interrupt (see dataprocess.c for interrupt service routine)
	 * while running the never ending state maschine's idle and control loop.
	 */

	/* Start State Maschine now */

	dsp_idle_loop ();
}

