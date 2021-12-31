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
                 
#include "FB_spm_dataexchange.h"   
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

extern PROBE_VECTOR prbvh;
extern DSP_INT      prbdf;

// IMPORTANT:
// KEEP THIS LISTS COMPATIBLE WITH STRUCT DEFINITIONS !!!!!


/*
 * DSP RT ENGINE (C) PyZ subsystem
 * ==================================================
 */


// IMPORTANT:
// KEEP THIS LISTS COMPATIBLE WITH STRUCT DEFINITIONS !!!!!

/* Init SPM state maschine */
SPM_STATEMACHINE state = {
		0,0,      // set, clear_mode
		MD_IDLE,  //	state.mode       = MD_IDLE;
		0,        // last_mode (dummy)
		0,     //	state.DSP_seconds  = 0;
		0,     //	state.DSP_minutes  = 0;
		0L,    //	state.DSP_count_seconds  = 0;
		0L,    //	state.DSP_time  = 0;
		0L,    //	state.DataProcessTime = 0;
		0L,    //	state.ProcessReentryTime   = 0;
		0x0000ffffL, //  state.ProcessReentryPeak   = 0;  HI|LO
		0x0000ffffL, //  state.IdleTime_Peak   = 0;       HI|LO
                { // DATA PROCESS Real Time Task Control [1+12]
                        // 0x00: Never, 0x10: Always, 0x20: Odd, 0x40: Even
                        { 0x10,0,0L,0L }, // RT000
                        { 0x20,0,0L,0L }, { 0x20,0,0L,0L }, { 0x10,0,0L,0L }, { 0x00,0,0L,0L }, // RT001..004
                        { 0x00,0,0L,0L }, { 0x00,0,0L,0L }, { 0x00,0,0L,0L }, { 0x10,0,0L,0L }, // RT005..008
                        { 0x00,0,0L,0L }, { 0x00,0,0L,0L }, { 0x00,0,0L,0L }, { 0x00,0,0L,0L }  // RT009..012
                },
                { // IDLE Task Control [32]
                        // 0x00: Never, 0x10: always normal, 0x20: timer controlled (150kHz clock), 0x40: clock mode
                        { 0x10,0,0L,0L }, { 0x20,0,0L,1L  }, { 0x10,0,0L,0L }, { 0x20,0,0L,15L }, // ID001..004
                        { 0x10,0,0L,0L }, { 0x10,0,0L,0L  }, { 0x10,0,0L,0L }, { 0x10,0,0L,0L }, // ID005..008
                        { 0x10,0,0L,0L }, { 0x10,0,0L,0L  }, { 0x10,0,0L,0L }, { 0x10,0,0L,0L }, // ID009..012
                        { 0x10,0,0L,0L }, { 0x10,0,0L,17L  }, { 0x00,0,0L,0L }, { 0x00,0,0L,0L }, // ID013..016
                        { 0x20,0,0L,111L }, { 0x00,0,0L,0L }, { 0x00,0,0L,0L }, { 0x00,0,0L,0L }, // ID017..020
                        { 0x00,0,0L,0L }, { 0x00,0,0L,0L }, { 0x00,0,0L,0L }, { 0x00,0,0L,0L }, // ID021..024
                        { 0x00,0,0L,0L }, { 0x00,0,0L,0L }, { 0x00,0,0L,0L }, { 0x10,0,0L,3L }, // ID025..028
                        { 0x00,0,0L,0L }, { 0x20,0,0L,5L  }, { 0x00,0,0L,7L }, { 0x40,0,0L,150000L }  // ID029..032
                },
                2000L
};


/* A810 Configuration -- may be altered and then A810 restarted via AIC_STO, START cycle */
/* A810_CONFIG      a810_config = { 5, 1, 1 }; // 150kHz, +/-5V, QEP=on -- for SD on, testing */
A810_CONFIG      a810_config = { 5, 0, 1 }; // 150kHz, +/-10V, QEP=on -- for SD on, testing
/* A810_CONFIG      a810_config = { 10, 0, 1 }; // 75kHz, +/-10V, QEP=on */

/* Init Feedback */
SPM_PI_FEEDBACK  feedback = {
		327,   //	feedback.cp = 327;
		327,   //	feedback.ci = 327;
		0,     //	feedback.setpoint = 0; --note: obsolete -- mixer only!
		13000, //	feedback.ca = 13000;
		4500L, //	feedback.cb_Ic = 4500;
		20,    //	feedback.I_cross = 20;
		5,     //	feedback.I_offset = 5;
		32700, //	feedback.q_factor = 32700;
		-6136, //	feedback.soll = -6136;
		0,     //	feedback.ist = 0;
		0,     //	feedback.delta = 0;
		0L,    //	feedback.i_sum = 0;
		0L,    //	feedback.I_iir = 0;
		0,0,   //	feedback.z,I_fbw = 0;
		0,0,   //       feedback.zxx,yy;
		0L,0L, //       feedback.Iavg,Irms;
		0L,0L, //       feedback.z32, z32neg
		0
};

/* Init external control parameter */
EXT_CONTROL external = {
                1,     //   external.FB_control_channel = -1;
                5242,  //   external.FB_control_treshold = 5242;
                0,     //   external.watch_value = 0;
                0,     //   external.watch_min = 0;
                32767, //   external.watch_max = 32727;
		0,0,0,0, // cp,ci,cd, set
		0,0,0, // set,bw,ref,out
		0,0, // del, del2
		0L // isum
};
                
/* Init FEEDBACK_MIXER values */
FEEDBACK_MIXER feedback_mixer = {
	0,0,0,0,         // level for fuzzy mix
	10000, 0, 0, 0,  // gain 0.3,0,0,0 (STM default on 0)
	3, 0,0,0,        // mode (log, iir -> STM default on 0)
	-6136, 0,0,0,    // setpoint
	0,0,0,
	0L, 
	0,0,0,0
};

/* Init all simple DA out: U and other simple steady state DAC channels */
ANALOG_VALUES    analog = {
		0,0,0,0, 0,0, // out0..5
		VOLT2DAC (0.5),    //	out6 bias = VOLT2DAC (2.0);
		0,    //	out7 analog.motor = 0;
		0,0,0,0, 0,0,0,0, // offsets out / dummy
		0,0,0,0, 0,0,0,0, // offsets in / dummy
		{ 0L,0L }, { 0L, 0L }
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
		1L<<31,  //	scan.rotm[xx] = 1<<15;
		0L,    //	scan.rotm[xy] = 0;
		0L,    //	scan.rotm[yx] = 0;
		1L<<31,  //	scan.rotm[yy] = 1<<15;
		0,      //	scan.nx_pre = 0;
		0,      //	scan.dnx_probe = 0;
		0, 0,   //      scan.raster_a, _b= 0;
		0L, 0L,   //	scan.srcs_xp,xm  = 0;
		0L, 0L,   //	scan.srcs_2nd_xp,2nd_xm  = 0;
		0,    //	scan.nx    = 0;
		0,    //	scan.ny    = 0;
		1L<<16,    //	scan.fs_dx  = 1L<<16;
		1L<<16,    //	scan.fs_dy  = 1L<<16;
		0L,        //   scan.num_steps_move_xy = 0L;
		1L<<16,    //	scan.fm_dx  = 1L<<16;
		1L<<16,    //	scan.fm_dy  = 1L<<16;
		1L<<16,    //	scan.fm_dzxy  = 1L<<16;
		1,    //	scan.dnx   = 1;
		1,    //	scan.dny   = 1;
		0, 0, //        scan.Zoff_2nd_xp = 0, scan.Zoff_2nd_xm = 0;
		0L, 0L, //      scan.fm_dz0x, _dz0y
		2L<<16, //      scan.z_slope_max
		1,1, //         fast_return, fast_return_2nd
		1,1, //         slow_down_factor, slow_down_factor_2nd
		1001,1002,1003,1004, //  bias_secion[4]
		0L,1L, //       xyz_gain, pad
		0L,    //	scan.Xpos  = 0;
		0L,    //	scan.Ypos  = 0;
		0L,    //	scan.Zpos  = 0;
		0L,    //	scan.XposR  = 0;
		0L,    //	scan.YposR  = 0;
		0L, 0L, //      scan.cfs_dx = cfs_dy = 0
		0,    //	scan.iix    = 0;
		0,    //	scan.iiy    = 0;
		0,    //	scan.ix    = 0;
		0,    //	scan.iy    = 0;
		0,   //         iiix;
		0,    //        scan.ifr;
		0,    //	scan.sstate = 0;
		0,    //	scan.section = 0;
		0     //	scan.pflg = 0;
};

DATA_SYNC_IO data_sync_io = {
	{0,0,0,0},
	{0,0,0,0},
	0,0,
	0,
	0,
	0 // pflg
};

/* Init Probe: */
PROBE            probe = {
	0,    //	probe.start = 0;
	0,    //	probe.stop  = 0;
	0,0,  //        probe.LIM_up, dn;
	0,    //	probe.AC_amp = 0;
	0,    //	probe.AC_frq = 0;
	0,    //	probe.AC_phaseA = 0;
	0,    //	probe.AC_phaseB = 0;
	0,    //	probe.AC_nAve = 0;
	0,    //        probe.AC_ix = 0;
	0L,    //        probe.time = 0;
	0L,    //        probe.Upos = 0;
	0L,    //        probe.Zpos = 0;
	0L,    //        probe.LockIn_0 = 0;
	0L,    //        probe.LockIn_1stA = 0;
	0L,    //        probe.LockIn_1stB = 0;
	0L,    //        probe.LockIn_2ndA = 0;
	0L,    //        probe.LockIn_2ndB = 0;
	0,    //        sinreftabA
	0,    //        sinreftabA length
	0,    //        sinreftabB
	0,    //        sinreftabB length
	&prbvh, //      PROBE_VECTOR *vector_head; -- default vector head, may be changed later
	NULL, //        PROBE_VECTOR *vector;
	0L, 0L, 0, 0,   //	probe.ix = probe.iix = 0L, probe.lix = 0, dum;
	0,    //	probe.state = 0;
	0,0   //	probe.pflg  = 0; probe.pflg_lockin = 0
};

/* Init autoapproach */
AUTOAPPROACH     autoapp = {
	0,     // start;           /**<0 Initiate =WO */
	0,     // stop;            /**<1 Cancel   =WO */
	0,     // mover_mode;      /**<2 Mover mode, see below */
	0,     // max_wave_cycles; /**<3 max number of repetitions */
	0,     // wave_length;     /**<4 total number of samples per wave cycle for all channels (must be multipe of n_waves_channels)  */
	0,     // n_wave_channels; /**<5 number of wave form channel to play simulatneously (1 to max 6)*/
	1,     // wave_speed;      /**<6 number of samples per wave cycle */
	1,     // n_wait;          /**<7 delay inbetween cycels */
	{3,4,0,0,0,0}, // channel_mapping[6]; /** 8,9,10,11,12,13 -- may include the flag "ch | AAP_MOVER_SIGNAL_ADD"  */
	0,     // axis;            /** axis id (0,1,2) for optional step count */
	0,     // dir;             /** direction for count +/-1 */
	0,0,   // ci_retract, dum
	10000L,     // NG   n_wait_fin;      /**<14 # cycles to wait and check (run FB) before finish auto app. */
	0L,     // NG   fin_cnt;         /**< tmp count for auto fin. */
	0,     // mv_count;        /**< "time axis" */
	0,     // mv_step_count;   /**< step counter */
	0,     // tip_mode;        /**< Tip mode, used by auto approach */
	0,     // delay_cnt;       /**< Delay count */
	0,0,   // cp, ci;          /**< temporary used */
        0,0,0, // count_axis[3];   /**< axis step counters */
	0     // pflg;            /**< process active flag =RO */
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
		(DSP_INT_P)(&prbdf),
		NULL,
		NULL
};

#ifdef WATCH_ENABLE
/* Watch Struct */
WATCH        watch = { 0,0,
		       0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
		       0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L,
		       0
};
#endif

extern long Signal1[];
extern long Signal2[];

RECORDER recorder = {
	(long)(&feedback.I_iir), (long)(&feedback.z32neg),
	-1L, -1L,
	0, 0
};
/* Signal Buffer for S1,S2, S1,S2 in SDRAM, 0x10000 word: 0x8000 */

void main()
{
#ifdef USE_ANALOG_16 // if special build for MK2-Analog_16
	/* Extern structure of AIC registers */
	extern struct aicreg_rec aic24reg;
#else // A810 config
	extern unsigned short FreqDiv;
	extern unsigned short ADCRange;
	extern unsigned short QEP_ON;
#endif


	/* fixed location of magic struct, reserved by SR Memlayout (Vis.Linker) */
	extern SPM_MAGIC_DATA_LOCATIONS magic;
	extern int     sigma_delta_hr_mask[];


	/* fill magic struct, contains version info and pointers to data structs */
	magic.magic        = 0x0000; /* not valid yet! */
	magic.version      = FB_SPM_VERSION;
	magic.year         = FB_SPM_DATE_YEAR;
	magic.mmdd         = FB_SPM_DATE_MMDD;
	magic.dsp_soft_id  = FB_SPM_SOFT_ID;
	magic.statemachine = (DSP_UINT)(&state);
	magic.AIC_in       = (DSP_UINT)(&AIC_IN(0));
	magic.AIC_out      = (DSP_UINT)(&AIC_OUT(0));
	magic.AIC_reg      = (DSP_UINT)(NULL);
	magic.feedback     = (DSP_UINT)(&feedback);
	magic.analog       = (DSP_UINT)(&analog);
	magic.scan         = (DSP_UINT)(&scan);
	magic.move         = (DSP_UINT)(&move);
	magic.probe        = (DSP_UINT)(&probe);
	magic.autoapproach = (DSP_UINT)(&autoapp);
	magic.datafifo     = (DSP_UINT)(&datafifo);
	magic.probedatafifo= (DSP_UINT)(&probe_datafifo);
	magic.data_sync_io  =(DSP_UINT)(&data_sync_io);
	magic.feedback_mixer = (DSP_UINT)(&feedback_mixer);
	magic.CR_out_puls  = (DSP_UINT)(&CR_out_pulse);
	magic.external     = (DSP_UINT)(&external);
	magic.CR_generic_io= (DSP_UINT)(&CR_generic_io);
	magic.a810_config  = (DSP_UINT)(&a810_config);
#ifdef WATCH_ENABLE
	magic.watch        = (DSP_UINT)(&watch);
#else
	magic.watch        = 0x0000;
#endif
        magic.hr_mask      = (DSP_UINT)(&sigma_delta_hr_mask);
        magic.recorder     = (DSP_UINT)(&recorder);
 	magic.dummy1       = 0x0000;   /* Null Termination */
 	magic.dummy2       = 0xBBAA;   /* Null Termination */
 	magic.dummyl       = 0x44332211; /* Byte Order Test Indicator LONG */


	/* Acknowledge the PC */

	*(ioport int *) HPIC = Acknowledge;

	/* Setup LockIn tabels */

	init_probe_module ();

	/* Init AIC registers : Defaut cfg (all gains = 0 db, except marked) */
	
// all AIC: running at 25475.54 Hz, anti-aliasing filter ON-FIR, x128 oversampling
//          VBias 1.35V, differential inputs, all input/output gains at 0dB

	/* Setup DSP timer/clock */

	TSCInit();

#ifdef USE_ANALOG_16 // if special build for MK2-Analog_16

/* AIC24:

For the AIC loop delay, I suggest to set the anti-aliasing filter to
IIR (see attached document fopr loopback response). Then, the loop
delay will be 16 samples instead of 42 samples (for the FIR case). To
set the AIC24 with the IIR filter, set the 

AICReg(192) to AICReg(207) wtih 0x2721 instead of 0x2701. 

This way, all AIC will be set with a IIR filter. With the AIC24, the
anti-aliasing filter can't be removed on one input only but the FIR or
IIR set-up can be adjusted separately on each AIC.

*/


// all AIC: running at 25475.54 Hz, anti-aliasing filter ON-FIR, x128 oversampling
//          VBias 1.35V, differential inputs, all input/output gains at 0dB
        aic24reg.pReg4_m[0]=0x8780;
        aic24reg.pReg4_m[1]=0x8780;
        aic24reg.pReg4_m[2]=0x8780;
        aic24reg.pReg4_m[3]=0x8780;
        aic24reg.pReg4_m[4]=0x8780;
        aic24reg.pReg4_m[5]=0x8780;
        aic24reg.pReg4_m[6]=0x8780;
        aic24reg.pReg4_m[7]=0x8780;
        aic24reg.pReg4_m[8]=0x8780;
        aic24reg.pReg4_m[9]=0x8780;
        aic24reg.pReg4_m[10]=0x8780;
        aic24reg.pReg4_m[11]=0x8780;
        aic24reg.pReg4_m[12]=0x8780;
        aic24reg.pReg4_m[13]=0x8780;
        aic24reg.pReg4_m[14]=0x8780;
        aic24reg.pReg4_m[15]=0x8780;
        aic24reg.pReg4_n[0]=0x8700;
        aic24reg.pReg4_n[1]=0x8700;
        aic24reg.pReg4_n[2]=0x8700;
        aic24reg.pReg4_n[3]=0x8700;
        aic24reg.pReg4_n[4]=0x8700;
        aic24reg.pReg4_n[5]=0x8700;
        aic24reg.pReg4_n[6]=0x8700;
        aic24reg.pReg4_n[7]=0x8700;
        aic24reg.pReg4_n[8]=0x8700;
        aic24reg.pReg4_n[9]=0x8700;
        aic24reg.pReg4_n[10]=0x8700;
        aic24reg.pReg4_n[11]=0x8700;
        aic24reg.pReg4_n[12]=0x8700;
        aic24reg.pReg4_n[13]=0x8700;
        aic24reg.pReg4_n[14]=0x8700;
        aic24reg.pReg4_n[15]=0x8700;
        aic24reg.Reg3A[0]=0x6701;
        aic24reg.Reg3A[1]=0x6701;
        aic24reg.Reg3A[2]=0x6701;
        aic24reg.Reg3A[3]=0x6701;
        aic24reg.Reg3A[4]=0x6701;
        aic24reg.Reg3A[5]=0x6701;
        aic24reg.Reg3A[6]=0x6701;
        aic24reg.Reg3A[7]=0x6701;
        aic24reg.Reg3A[8]=0x6701;
        aic24reg.Reg3A[9]=0x6701;
        aic24reg.Reg3A[10]=0x6701;
        aic24reg.Reg3A[11]=0x6701;
        aic24reg.Reg3A[12]=0x6701;
        aic24reg.Reg3A[13]=0x6701;
        aic24reg.Reg3A[14]=0x6701;
        aic24reg.Reg3A[15]=0x6701;
        aic24reg.Reg3B[0]=0x6743;
        aic24reg.Reg3B[1]=0x6743;
        aic24reg.Reg3B[2]=0x6743;
        aic24reg.Reg3B[3]=0x6743;
        aic24reg.Reg3B[4]=0x6743;
        aic24reg.Reg3B[5]=0x6743;
        aic24reg.Reg3B[6]=0x6743;
        aic24reg.Reg3B[7]=0x6743;
        aic24reg.Reg3B[8]=0x6743;
        aic24reg.Reg3B[9]=0x6743;
        aic24reg.Reg3B[10]=0x6743;
        aic24reg.Reg3B[11]=0x6743;
        aic24reg.Reg3B[12]=0x6743;
        aic24reg.Reg3B[13]=0x6743;
        aic24reg.Reg3B[14]=0x6743;
        aic24reg.Reg3B[15]=0x6743;
        aic24reg.Reg3C[0]=0x6780;
        aic24reg.Reg3C[1]=0x6780;
        aic24reg.Reg3C[2]=0x6780;
        aic24reg.Reg3C[3]=0x6780;
        aic24reg.Reg3C[4]=0x6780;
        aic24reg.Reg3C[5]=0x6780;
        aic24reg.Reg3C[6]=0x6780;
        aic24reg.Reg3C[7]=0x6780;
        aic24reg.Reg3C[8]=0x6780;
        aic24reg.Reg3C[9]=0x6780;
        aic24reg.Reg3C[10]=0x6780;
        aic24reg.Reg3C[11]=0x6780;
        aic24reg.Reg3C[12]=0x6780;
        aic24reg.Reg3C[13]=0x6780;
        aic24reg.Reg3C[14]=0x6780;
        aic24reg.Reg3C[15]=0x6780;
        aic24reg.Reg4_m[0]=0x8797;
        aic24reg.Reg4_m[1]=0x8797;
        aic24reg.Reg4_m[2]=0x8797;
        aic24reg.Reg4_m[3]=0x8797;
        aic24reg.Reg4_m[4]=0x8797;
        aic24reg.Reg4_m[5]=0x8797;
        aic24reg.Reg4_m[6]=0x8797;
        aic24reg.Reg4_m[7]=0x8797;
        aic24reg.Reg4_m[8]=0x8797;
        aic24reg.Reg4_m[9]=0x8797;
        aic24reg.Reg4_m[10]=0x8797;
        aic24reg.Reg4_m[11]=0x8797;
        aic24reg.Reg4_m[12]=0x8797;
        aic24reg.Reg4_m[13]=0x8797;
        aic24reg.Reg4_m[14]=0x8797;
        aic24reg.Reg4_m[15]=0x8797;
        aic24reg.Reg4_n[0]=0x8708;
        aic24reg.Reg4_n[1]=0x8708;
        aic24reg.Reg4_n[2]=0x8708;
        aic24reg.Reg4_n[3]=0x8708;
        aic24reg.Reg4_n[4]=0x8708;
        aic24reg.Reg4_n[5]=0x8708;
        aic24reg.Reg4_n[6]=0x8708;
        aic24reg.Reg4_n[7]=0x8708;
        aic24reg.Reg4_n[8]=0x8708;
        aic24reg.Reg4_n[9]=0x8708;
        aic24reg.Reg4_n[10]=0x8708;
        aic24reg.Reg4_n[11]=0x8708;
        aic24reg.Reg4_n[12]=0x8708;
        aic24reg.Reg4_n[13]=0x8708;
        aic24reg.Reg4_n[14]=0x8708;
        aic24reg.Reg4_n[15]=0x8708;
        aic24reg.Reg5A[0]=0xA700;
        aic24reg.Reg5A[1]=0xA700;
        aic24reg.Reg5A[2]=0xA700;
        aic24reg.Reg5A[3]=0xA700;
        aic24reg.Reg5A[4]=0xA700;
        aic24reg.Reg5A[5]=0xA700;
        aic24reg.Reg5A[6]=0xA700;
        aic24reg.Reg5A[7]=0xA700;
        aic24reg.Reg5A[8]=0xA700;
        aic24reg.Reg5A[9]=0xA700;
        aic24reg.Reg5A[10]=0xA700;
        aic24reg.Reg5A[11]=0xA700;
        aic24reg.Reg5A[12]=0xA700;
        aic24reg.Reg5A[13]=0xA700;
        aic24reg.Reg5A[14]=0xA700;
        aic24reg.Reg5A[15]=0xA700;
        aic24reg.Reg5B[0]=0xA740;
        aic24reg.Reg5B[1]=0xA740;
        aic24reg.Reg5B[2]=0xA740;
        aic24reg.Reg5B[3]=0xA740;
        aic24reg.Reg5B[4]=0xA740;
        aic24reg.Reg5B[5]=0xA740;
        aic24reg.Reg5B[6]=0xA740;
        aic24reg.Reg5B[7]=0xA740;
        aic24reg.Reg5B[8]=0xA740;
        aic24reg.Reg5B[9]=0xA740;
        aic24reg.Reg5B[10]=0xA740;
        aic24reg.Reg5B[11]=0xA740;
        aic24reg.Reg5B[12]=0xA740;
        aic24reg.Reg5B[13]=0xA740;
        aic24reg.Reg5B[14]=0xA740;
        aic24reg.Reg5B[15]=0xA740;
        aic24reg.Reg5C[0]=0xA7BF;
        aic24reg.Reg5C[1]=0xA7BF;
        aic24reg.Reg5C[2]=0xA7BF;
        aic24reg.Reg5C[3]=0xA7BF;
        aic24reg.Reg5C[4]=0xA7BF;
        aic24reg.Reg5C[5]=0xA7BF;
        aic24reg.Reg5C[6]=0xA7BF;
        aic24reg.Reg5C[7]=0xA7BF;
        aic24reg.Reg5C[8]=0xA7BF;
        aic24reg.Reg5C[9]=0xA7BF;
        aic24reg.Reg5C[10]=0xA7BF;
        aic24reg.Reg5C[11]=0xA7BF;
        aic24reg.Reg5C[12]=0xA7BF;
        aic24reg.Reg5C[13]=0xA7BF;
        aic24reg.Reg5C[14]=0xA7BF;
        aic24reg.Reg5C[15]=0xA7BF;
        aic24reg.Reg6A[0]=0xC702;
        aic24reg.Reg6A[1]=0xC701;
        aic24reg.Reg6A[2]=0xC702;
        aic24reg.Reg6A[3]=0xC701;
        aic24reg.Reg6A[4]=0xC702;
        aic24reg.Reg6A[5]=0xC701;
        aic24reg.Reg6A[6]=0xC702;
        aic24reg.Reg6A[7]=0xC701;
        aic24reg.Reg6A[8]=0xC702;
        aic24reg.Reg6A[9]=0xC701;
        aic24reg.Reg6A[10]=0xC702;
        aic24reg.Reg6A[11]=0xC701;
        aic24reg.Reg6A[12]=0xC702;
        aic24reg.Reg6A[13]=0xC701;
        aic24reg.Reg6A[14]=0xC702;
        aic24reg.Reg6A[15]=0xC701;
        aic24reg.Reg6B[0]=0xC782;
        aic24reg.Reg6B[1]=0xC781;
        aic24reg.Reg6B[2]=0xC782;
        aic24reg.Reg6B[3]=0xC781;
        aic24reg.Reg6B[4]=0xC782;
        aic24reg.Reg6B[5]=0xC781;
        aic24reg.Reg6B[6]=0xC782;
        aic24reg.Reg6B[7]=0xC781;
        aic24reg.Reg6B[8]=0xC782;
        aic24reg.Reg6B[9]=0xC781;
        aic24reg.Reg6B[10]=0xC782;
        aic24reg.Reg6B[11]=0xC781;
        aic24reg.Reg6B[12]=0xC782;
        aic24reg.Reg6B[13]=0xC781;
        aic24reg.Reg6B[14]=0xC782;
        aic24reg.Reg6B[15]=0xC781;
        aic24reg.Reg1[0]=0x2721; // 0x2701 FIR (42x), 0x2721 IIR (16x)
        aic24reg.Reg1[1]=0x2721; // ..
        aic24reg.Reg1[2]=0x2721;
        aic24reg.Reg1[3]=0x2721;
        aic24reg.Reg1[4]=0x2721;
        aic24reg.Reg1[5]=0x2721;
        aic24reg.Reg1[6]=0x2721;
        aic24reg.Reg1[7]=0x2721;
        aic24reg.Reg1[8]=0x2721;
        aic24reg.Reg1[9]=0x2721;
        aic24reg.Reg1[10]=0x2721;
        aic24reg.Reg1[11]=0x2721;
        aic24reg.Reg1[12]=0x2721;
        aic24reg.Reg1[13]=0x2721;
        aic24reg.Reg1[14]=0x2721; // ..
        aic24reg.Reg1[15]=0x2721; // FIR/IIR
        aic24reg.Reg2[0]=0x47A0;
        aic24reg.Reg2[1]=0x47A0;
        aic24reg.Reg2[2]=0x47A0;
        aic24reg.Reg2[3]=0x47A0;
        aic24reg.Reg2[4]=0x47A0;
        aic24reg.Reg2[5]=0x47A0;
        aic24reg.Reg2[6]=0x47A0;
        aic24reg.Reg2[7]=0x47A0;
        aic24reg.Reg2[8]=0x47A0;
        aic24reg.Reg2[9]=0x47A0;
        aic24reg.Reg2[10]=0x47A0;
        aic24reg.Reg2[11]=0x47A0;
        aic24reg.Reg2[12]=0x47A0;
        aic24reg.Reg2[13]=0x47A0;
        aic24reg.Reg2[14]=0x47A0;
        aic24reg.Reg2[15]=0x47A0;

	/* Init AIC */

	start_aic24 ();
//	stop_aic24 (); // Stop AICs now again -- testing only

#else // default: MK2-A810

	/* Init Analog810 */

	FreqDiv   = a810_config.freq_div;     // [default: =10 75kHz *** 5 ... 65536  150kHz at N=5   Fs = Clk_in/(N * 200), Clk_in=150MHz] / 2!!!!
	ADCRange  = a810_config.adc_range;    // default: =0 0: +/-10V, 1:+/-5V
	QEP_ON    = a810_config.qep_on;       // default: =1 (on) manage QEP counters

	start_Analog810 ();
//	stop_Analog810 (); // Stop AICs now again -- testing only

#endif

	/*
	 * Wait for DMA interrupt (see dataprocess.c for interrupt service routine)
	 * while running the never ending state maschine's idle and control loop.
	 */

	magic.magic        = FB_SPM_MAGIC; /* now valid! */

	/* Start State Maschine now */

	dsp_idle_loop ();
}

