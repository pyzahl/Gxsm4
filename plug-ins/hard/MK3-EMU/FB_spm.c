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

/*
 * code derived of the PCI32 TMS320C32 DSP code "xsm.c, ..." and the SRanger FB_Demo.c
 * 20030107-PZ: start of porting SR-STD/SP2 -- MK2 -- MK3 -- DSPEMU
 * CVS FLICK2
 */
                    
#include "FB_spm_dataexchange.h"
#include "FB_spm.h"  
#include "dataprocess.h"
#include "FB_spm_statemaschine.h"
#include "FB_spm_analog.h"
#include "FB_spm_probe.h"
#include "SR3_flash_support.h"

#ifdef USE_PLL_API
#include "PAC_pll.h"
#endif

#ifdef DSPEMU

#include <gtk/gtk.h>
#include "dspemu_app.h"

#else

#include "SR3_Reg.h"

#define FB_INT6MASK		       0x00000040
extern cregister volatile unsigned int IER;
extern ANALOG_VALUES    analog;
#endif



/*
 * SPM subsystem
 * ==================================================
 * This structures are read/write accessed by Gxsm
 * for SPM control. 
 * (all define via #include "FB_spm_dataexchange.h")
 */

// initializing of global structs in this way saves a lot of space!!

#ifndef DSPEMU
#pragma DATA_SECTION(prbvh,"PRBVH") // Probe Vector Head
PROBE_VECTOR prbvh;
#pragma DATA_SECTION(prbdf,"PRBDF") // Probe Data Fifo
DSP_INT16    prbdf[];
#endif

#ifndef DSPEMU
// IMPORTANT:
// KEEP THIS LISTS COMPATIBLE WITH STRUCT DEFINITIONS !!!!!

/* Init SPM state maschine */
SPM_STATEMACHINE state = {
        0,0,  // set, clear_mode
        MD_OFFSETADDING | MD_ZPOS_ADJUSTER,   //	state.mode to start with: PLL on, OFFSETADDING is default -- dummy now.
        MD_IDLE | MD_ZPOS_ADJUSTER,    //	state.last_mode  = MF_IDLE
        0,    //	state.BLK_count  = 0;
        2000, //	state.BLK_Ncount = 2000;
        0,    //	state.DSP_time   = 0;
        0,    //	state.DSP_tens   = 0;
        0,    //	state.DataProcessTime = 0;
        0,    //	state.IdleTime   = 0;
        0, 0, //        2x-dito .._peak
        {590, 590} // state.DSP_speed = 590 MHz
};

/* A810 Configuration -- may be altered and then A810 restarted via AIC_STO, START cycle */
A810_CONFIG      a810_config = { 5, 0, 1 }; // 150kHz, +/-10V, QEP=on -- for SD on, testing
/* A810_CONFIG      a810_config = { 10, 0, 1 }; // 75kHz, +/-10V, QEP=on */

// few constants Q32s15.16 :: 1/10V * 32768*(1<<16)
#define C1V   214748365
#define C2V   429496730
#define C5V  1073741824
#define C10V 2147483647

// ---> move into DSPMEM
int  VP_sec_end_buffer[10*8]; // 8x section + 8 values
int  user_input_signal_array[32] = {
	C1V,-C1V,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0
};    // general purpuse control values, can be mapped to singnals

/* Init FEEDBACK_MIXER values */
FEEDBACK_MIXER feedback_mixer = {
	{ 0,0,0,0 },         // level for fuzzy mix
	{ 10000, 0, 0, 0 },  // gain 0.3,0,0,0 (STM default on 0)
	{ 3, 0,0,0 },        // mode (log, iir -> STM default on 0)
	{ -6136, 0,0,0 },    // setpoint
	{ 0,0,0,0 },         // IIR fmax ca value, 0=FBW
	0, 0, 0, 0,          // MIX0 adapt IIR settings cb_Ic, I_cross, I_offset, align
	0L,                  // PAC f-ref 64bit
	0,                   // exec flag -- tmp
	0,0,0,               // x, lnx, y -- tmp
	{ 0,0,0,0 },         // IIR signal
	0,                   // MIX0 adapt actual f0: q_factor15
	0,                   // delta signal
	{ 0,0,0,0 },         // setpoints log
	{ NULL, NULL, NULL, NULL }, // signal pointer input_p[4] -- init NULL, must be configure!
	{ 0,0,0,0 },         // FB_IN_processed[4]
	{ 0,0,0,0 },         // channel[4] -- FB_OUT processed
	{ 0,0,0,0 }          // is_analog_flag
};


#define OUT_MOD_INIT { NULL, NULL, NULL, NULL, NULL }

/* Control, Analog and Counter signal management Module */
ANALOG_VALUES    analog = {
	VOLT2DAC (0.5),  // analog.bias = VOLT2DAC (0.5);
	VOLT2DAC (0.0),  // motor = 0
	//-----------------
	{ OUT_MOD_INIT,
	  OUT_MOD_INIT,
	  OUT_MOD_INIT,
	  OUT_MOD_INIT,

	  OUT_MOD_INIT,
	  OUT_MOD_INIT,
	  OUT_MOD_INIT,
	  OUT_MOD_INIT,

	  OUT_MOD_INIT,
	  OUT_MOD_INIT },
	//-----------------
	0,0,0, // input for avg and rms
	//-----------------
	0, 0, 0, // biasadj, noise, null
	{ 0,0 }, // wave
	//-----------------
	{ 0,0,0,0,  0,0,0,0 }, // in 0..7
	{ NULL,NULL,NULL,NULL }, // to be initialized
	//-----------------
	{ 0,0 }, { 0, 0 },   // Counter, Gate
	{ 0,0,0,0,  0,0,0,0 } // scratch
};


/* Init Main Z Servo parameters */
SERVO z_servo = {
	0,0,0,
	&DSP_MEMORY(feedback_mixer).delta, // default input "DSP_MEMORY(feedback_mixer).delta"
	0,0,0,0
};
                
/* Init Motor Servo parameters */
SERVO m_servo = {
	0,0,0,
	&analog.vnull, // default input "null"
	0,0,0,0
};

/* Signal Controller Oscillator */
SCO sco_s[2] = {{
                0,
                1372,
                0,0,
                2147483647,0,0,0,
                NULL,
                &user_input_signal_array[16],
                0,
                0
        },{
                0,
                2372,
                0,0,
                2147483647,0,0,0,
                NULL,
                &user_input_signal_array[17],
                0,
                0
        }};

/* Init Move: Offset */
MOVE_OFFSET      move = {
        0,    //	move.start = 0;
        1,    //	move.nRegel= 1;
        { 0, 0 }, //	move.Xnew = move.Ynew = 0;
        { 0, 0, 0 }, //	move.f_dx = move.f_dy = move.f_dz = 0;
        0,    //	move.num_steps = 0;
        0x000A0A0A,  //	move.gains XYZ = 0x000A0A0A; - / 10 (z) / 10 (y) / 10 (x)
        { 0, 0, 0 }, //	move.XPos = move.YPos = move.ZPos = 0;
        0     //	move.pflg  = 0;
};

/* Init Scan: Scan Control Process, Speed, Rotation, Dimensions, Sizes */
AREA_SCAN        scan = {
        0,    //	scan.start= 0;
        0,    //	scan.stop = 0;
        { 1<<31 ,0,    //	scan.rotxx, xy,
          0, 1<<31 }, //	scan.rotyx, yy
        0,      //	scan.nx_pre = 0;
        0,      //	scan.dnx_probe = 0;
        0, 0,   //      scan.raster_a, _b= 0;
        0, 0,   //	scan.srcs_xp,xm  = 0;
        0, 0,   //	scan.srcs_2nd_xp,2nd_xm  = 0;
        0,    //	scan.nx    = 0;
        0,    //	scan.ny    = 0;
        1<<16,    //	scan.fs_dx  = 1L<<16;
        1<<16,    //	scan.fs_dy  = 1L<<16;
        0,        //   scan.num_steps_move_xy = 0L;
        1<<16,    //	scan.fm_dx  = 1L<<16;
        1<<16,    //	scan.fm_dy  = 1L<<16;
        0,    //	scan.fm_dz  = 0L<<16;
        1,    //	scan.dnx   = 1;
        1,    //	scan.dny   = 1;
        0, 0, //        scan.Zoff_2nd_xp = 0, scan.Zoff_2nd_xm = 0;
        { 0, 0 },  //      scan.fm_dzx, _dzy
        2<<16,    //	scan.z_slope_max;
        1,       //     scan.fast_return;
        0x000A0A0A,  //	scan.gains XYZ = 0x000A0A0A; - / 10 (z) / 10 (y) / 10 (x)
        { 0,0,0 },    //	scan.XYZpos[]  = 0;
        { 0,0 },    //	scan.XYpos_r[]  = 0;
        0,    // z_offset_xyslope
        0, 0, //      scan.cfs_dx = cfs_dy = 0
        0,    //	scan.iix    = 0;
        0,    //	scan.iiy    = 0;
        0,    //	scan.ix    = 0;
        0,    //	scan.iy    = 0;
        1,0,  //        scan.slow_down_factor, iiix;
        0,    //        scan.ifr
        { 0, 0, 0, 0 }, // src_input[4]; 
        0,    //	scan.sstate = 0;
        0     //	scan.pflg = 0;
};

SCAN_EVENT_TRIGGER scan_event_trigger = {
	{ -1,-1,-1,-1, // xp bias
	  -1,-1,-1,-1 }, // set
	{ -1,-1,-1,-1, // xm bias
	  -1,-1,-1,-1 }, // set
	{ 0,0,0,0,     // biases
	  0,0,0,0 },
	{ 0,0,0,0,     // setpoints
	  0,0,0,0 },
	0 // pflg
};

/* Init Probe: */
PROBE            probe = {
	0,    //	probe.start = 0;
	0,    //	probe.stop  = 0;
	0,    //	probe.AC_amp = 0;
	0,    //	probe.AC_amp_aux = 0;
	0,    //	probe.AC_frq = 0;
	0,    //	probe.AC_phaseA = 0;
	0,    //	probe.AC_phaseB = 0;
	0,    //	probe.AC_nAve = 0;
	0,    //	probe.noise_amp = 0;
	16, 8, //       probe.corrsumprod, sum shl
	{ 0,0,0,0 }, //     *probe.src_input[4]
	{ 0,0,0,0 }, //     *probe.prb_output[4]
	{ 0,0,0,0 }, //     *probe.LockIn input[4]
	{ 0,0 },  //     *probe.limiter_up, dn;
	0,0,  //     *probe.limiter_input, *probe.Tracker_input
	0,    //        probe.AC_ix = 0;
	0,    //        probe.time = 0;
	0,    //        probe.Upos = 0;
	0,    //        probe.Zpos = 0;
	0,    //        probe.LockIn_ref = 0;
	0,    //        probe.LockIn_0A = 0;
	0,    //        probe.LockIn_0B = 0;
	0,    //        probe.LockIn_1stA = 0;
	0,    //        probe.LockIn_1stB = 0;
	0,    //        probe.LockIn_2ndA = 0;
	0,    //        probe.LockIn_2ndB = 0;
	0,    //        sinreftabA
	0,    //        sinreftabA length
	0,    //        sinreftabB
	0,    //        sinreftabB length
	NULL, // &prbvh, //      PROBE_VECTOR *vector_head; -- default vector head, may be changed later
	NULL, //        PROBE_VECTOR *vector;
	0, 0, 0,    //	probe.ix = probe.iix = probe.lix = 0;
	0,    //        probe.PRBACPhaseA32
	0,    //        probe.gpio_setting
	0,    //        probe.gpio_data
	0,    //        *probe.trigger input
	0,    //	probe.state = 0;
	0     //	probe.pflg  = 0;
};

/* Init autoapproach */
AUTOAPPROACH     autoapp = {
        0,    //	autoapp.start = 0;
        0,    //	autoapp.stop  = 0;
        0,    //	autoapp.mover_mode  = 0;
        { 3,4 }, // default out on XY-SCAN !!
        6,    //	autoapp.piezo_steps = 6;
        2,    //	autoapp.n_wait      = 2;
        8000,    //	autoapp.u_piezo_max = 8000;
        16000,   //	autoapp.u_piezo_amp = 16000;
        2,    //	autoapp.piezo_speed = 2;
        10000, //       autoapp.n_wait_fin
        0,    //        autoapp.fin_cnt
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
        { 0,0,0 }, //       autoapp.count_axis[3]
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
	0, 0, 0, 4, // DSP_UINT16 dir, in, out, subs 
	0, 0, 0, // DSP_UINT32 gate lo, hi, 1
	0, 0, 0, 0, // count 0, 1, p0, p1
	0, // peak_holdtime
	0  //DSP_INT     pflg;            /* process active flag =RO */
};


/* Data Fifo Struct */
#ifndef DSPEMU
#pragma DATA_SECTION(datafifo,"FIFO_DATA") // Data Fifo
#endif

DATA_FIFO        datafifo = {
        0, 0, // datafifo.r_position, datafifo.w_position
        0, 0, // datafifo.fill, datafifo.stall
        DATAFIFO_LENGTH, // datafifo.length
        1,    // datafifo.mode
        { 1,2,3,4,5,6,7,8 } // first data...
        };

/* Probe Data Fifo Struct */
#ifndef DSPEMU
#pragma DATA_SECTION(probe_datafifo,"FIFO_DATA") // Data Fifo
#endif

DATA_FIFO_EXTERN probe_datafifo = {
        0, 0, // datafifo.r_position, datafifo.w_position
        0, 0, // current_section_head_positon, section_index
        0, 0, // datafifo.fill, datafifo.stall
        EXTERN_PROBEDATAFIFO_LENGTH, // datafifo.length
        NULL, //(DSP_INT16_P)(&prbdf),
        NULL,
        NULL
        };

/* Signal Monitor: Watch Data Pointers Defaults */
SIGNAL_MONITOR sig_mon;

#ifdef USE_PLL_API

#ifndef DSPEMU
#pragma DATA_SECTION(PLL_lookup, "SMAGIC")
#endif

PLL_LOOKUP PLL_lookup;
#endif

#endif

/** ---> separate into includabe file
 * Signal pointers and description table for RT signal handling, stored in ext. DRAM
 **/

// LIMITER FUCTION LOGIC:  if (*probe.limiter_input < *probe.limiter_updn[0] && *probe.limiter_input > *probe.limiter_updn[1]){..}
// user_input_signal_array[0,1] used as default control for limiter_updn[]

#define CREATE_DSP_SIGNAL_LOOKUP
#include "dsp_signals.h"

/**
 * Initialize final cross linked pointers for default signal setup
 **/
#define Q15 32767

#ifndef DSPEMU
#pragma CODE_SECTION(setup_default_signal_configuration, ".text:slow")
#endif

void setup_default_signal_configuration(){
#ifdef DSPEMU
        // copy signal lookup from gob def array via include file
        for (int i=0; i<NUM_SIGNALS; ++i)
                DSP_MEMORY(dsp_signal_lookup[i]).p = dsp_signal_lookup[i].p;
                
        // Pointers for the acquisition
        DSP_MEMORY(PLL_lookup).pSignal1 = &DSP_MEMORY_PAC(pSignal1);
        DSP_MEMORY(PLL_lookup).pSignal2 = &DSP_MEMORY_PAC(pSignal2);
        DSP_MEMORY(PLL_lookup).Signal1 = &DSP_MEMORY_PAC(Signal1[0]);
        DSP_MEMORY(PLL_lookup).Signal2 = &DSP_MEMORY_PAC(Signal2[0]);
	
        // Block length
        DSP_MEMORY(PLL_lookup).blcklen = &DSP_MEMORY_PAC(blcklen);
        // Max MaxSizeSigBuf-1 (DSP_UINTA)(&DSP_MEMORY_PAC(blcklen), // Max MaxSizeSigBuf-1 
	
        // Sine variables
        DSP_MEMORY(PLL_lookup).deltaReT = &DSP_MEMORY_PAC(deltaReT);
        DSP_MEMORY(PLL_lookup).deltaImT = &DSP_MEMORY_PAC(deltaImT);
        DSP_MEMORY(PLL_lookup).volumeSine = &DSP_MEMORY_PAC(volumeSine);
 
        // Phase/Amp detector time constant 
        DSP_MEMORY(PLL_lookup).Tau_PAC = &DSP_MEMORY_PAC(Tau_PAC);

        // Phase corrector variables
        DSP_MEMORY(PLL_lookup).PI_Phase_Out   = &DSP_MEMORY_PAC(PI_Phase_Out);
        DSP_MEMORY(PLL_lookup).pcoef_Phase    = &DSP_MEMORY_PAC(pcoef_Phase);
        DSP_MEMORY(PLL_lookup).icoef_Phase    = &DSP_MEMORY_PAC(icoef_Phase);
        DSP_MEMORY(PLL_lookup).errorPI_Phase  = &DSP_MEMORY_PAC(errorPI_Phase);
        DSP_MEMORY(PLL_lookup).memI_Phase     = &DSP_MEMORY_PAC(memI_Phase);
        DSP_MEMORY(PLL_lookup).setpoint_Phase = &DSP_MEMORY_PAC(setpoint_Phase);
        DSP_MEMORY(PLL_lookup).ctrlmode_Phase = &DSP_MEMORY_PAC(ctrlmode_Phase);
        DSP_MEMORY(PLL_lookup).corrmin_Phase  = &DSP_MEMORY_PAC(corrmin_Phase);
        DSP_MEMORY(PLL_lookup).corrmax_Phase  = &DSP_MEMORY_PAC(corrmax_Phase);
        
        // Min/Max for the output signals
        DSP_MEMORY(PLL_lookup).ClipMin = &DSP_MEMORY_PAC(ClipMin[0]);
        DSP_MEMORY(PLL_lookup).ClipMax = &DSP_MEMORY_PAC(ClipMax[0]);

        // Amp corrector variables
        DSP_MEMORY(PLL_lookup).ctrlmode_Amp = &DSP_MEMORY_PAC(ctrlmode_Amp);
        DSP_MEMORY(PLL_lookup).corrmin_Amp  = &DSP_MEMORY_PAC(corrmin_Amp);
        DSP_MEMORY(PLL_lookup).corrmax_Amp  = &DSP_MEMORY_PAC(corrmax_Amp);
        DSP_MEMORY(PLL_lookup).setpoint_Amp = &DSP_MEMORY_PAC(setpoint_Amp);
        DSP_MEMORY(PLL_lookup).pcoef_Amp    = &DSP_MEMORY_PAC(pcoef_Amp); 
        DSP_MEMORY(PLL_lookup).icoef_Amp    = &DSP_MEMORY_PAC(icoef_Amp);
        DSP_MEMORY(PLL_lookup).errorPI_Amp  = &DSP_MEMORY_PAC(errorPI_Amp);
        DSP_MEMORY(PLL_lookup).memI_Amp     = &DSP_MEMORY_PAC(memI_Amp);
	
        // LP filter selections
        DSP_MEMORY(PLL_lookup).LPSelect_Phase = &DSP_MEMORY_PAC(LPSelect_Phase);
        DSP_MEMORY(PLL_lookup).LPSelect_Amp   = &DSP_MEMORY_PAC(LPSelect_Amp);

        // pointers to live signals
        DSP_MEMORY(PLL_lookup).InputFiltered  = &DSP_MEMORY_PAC(InputFiltered);
        // Resonator Output Signal
        DSP_MEMORY(PLL_lookup).SineOut0       = &DSP_MEMORY_PAC(SineOut0);
        // Excitation Signal
        DSP_MEMORY(PLL_lookup).phase          = &DSP_MEMORY_PAC(phase);
        // Resonator Phase (no LP filter)
        DSP_MEMORY(PLL_lookup).PI_Phase_Out   = &DSP_MEMORY_PAC(PI_Phase_Out);
        // Excitation Freq. (no LP filter)(no LP filter)
        DSP_MEMORY(PLL_lookup).amp_estimation = &DSP_MEMORY_PAC(amp_estimation);
        // Resonator Amp. (no LP filter)
        DSP_MEMORY(PLL_lookup).volumeSine     = &DSP_MEMORY_PAC(volumeSine);
        // Excitation Amp. (no LP filter)
        DSP_MEMORY(PLL_lookup).Filter64Out    = &DSP_MEMORY_PAC(Filter64Out[0]);
        // Excitation Freq. (with LP filter; ... [0..3]

        //  Mode2F
        DSP_MEMORY(PLL_lookup).Mode2F = &DSP_MEMORY_PAC(Mode2F);

                    
        DSP_MEMORY(sig_mon).mindex = -1;
        int ii=0;
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(feedback_mixer).FB_IN_processed[0];
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(feedback_mixer).FB_IN_processed[1];
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(feedback_mixer).FB_IN_processed[2];
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(feedback_mixer).FB_IN_processed[3];

        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(move).xyz_vec[i_X];
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(move).xyz_vec[i_Y];
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(move).xyz_vec[i_Z];
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(state).DSP_time;

        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(scan).xy_r_vec[i_X];
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(scan).xy_r_vec[i_Y];
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(feedback_mixer).delta;
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(z_servo).neg_control;

        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(feedback_mixer).q_factor15;
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(analog).avg_signal;
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(analog).rms_signal;
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(probe).Zpos; 

        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(z_servo).watch;
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(analog).vnull;
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(analog).vnull;
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(analog).vnull;

        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(analog).vnull;
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(analog).vnull;

        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(analog).debug[0];
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(analog).debug[1];
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(analog).debug[2];
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(analog).debug[3];
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(analog).debug[4];
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(analog).debug[5];
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(analog).debug[6];
        DSP_MEMORY(sig_mon).signal_p[ii++] = DSP_MEMORY_P(analog).debug[7];

        // init DSPEMU MEMORY
        DSP_MEMORY(state).mode = MD_OFFSETADDING | MD_ZPOS_ADJUSTER;
        DSP_MEMORY(state).BLK_Ncount = 2000;
        DSP_MEMORY(state).DSP_speed[0] = 590;
        DSP_MEMORY(state).DSP_speed[1] = 590;

        DSP_MEMORY(user_input_signal_array[0]) = 214748365; // 1
        DSP_MEMORY(user_input_signal_array[1]) = -214748365;

        DSP_MEMORY(feedback_mixer).gain[0] = 10000;
        DSP_MEMORY(feedback_mixer).mode[0] = 3;
        DSP_MEMORY(feedback_mixer).setpoint[0] = -6136;

        DSP_MEMORY(z_servo).input = &DSP_MEMORY(feedback_mixer).delta; // configure default signal input
        DSP_MEMORY(m_servo).input = &DSP_MEMORY(analog).vnull; // configure default signal input

        DSP_MEMORY(sco_s[0]).center = 0;
        DSP_MEMORY(sco_s[0]).fspanV = 1372;
        DSP_MEMORY(sco_s[0]).cr = 2147483647;
        DSP_MEMORY(sco_s[0]).in = NULL;
        DSP_MEMORY(sco_s[0]).amplitude = &DSP_MEMORY(user_input_signal_array[16]);

        DSP_MEMORY(sco_s[1]).center = 0;
        DSP_MEMORY(sco_s[1]).fspanV = 2372;
        DSP_MEMORY(sco_s[1]).cr = 2147483647;
        DSP_MEMORY(sco_s[1]).in = NULL;
        DSP_MEMORY(sco_s[1]).amplitude = &DSP_MEMORY(user_input_signal_array[17]);

        DSP_MEMORY(move).nRegel = 1;
        DSP_MEMORY(move).xyz_gain  = 0x000A0A0A;

        DSP_MEMORY(scan).rotm[0] = 1<<31;
        DSP_MEMORY(scan).rotm[1] = 0;
        DSP_MEMORY(scan).rotm[2] = 0;
        DSP_MEMORY(scan).rotm[3] = 1<<31;
        DSP_MEMORY(scan).fs_dx = 1<<16;
        DSP_MEMORY(scan).fs_dy = 1<<16;
        DSP_MEMORY(scan).fm_dx = 1<<16;
        DSP_MEMORY(scan).fm_dy = 1<<16;
        DSP_MEMORY(scan).dnx = 1;
        DSP_MEMORY(scan).dny = 1;
        DSP_MEMORY(scan).z_slope_max = 2<<16;
        DSP_MEMORY(scan).fast_return = 1;
        DSP_MEMORY(scan).slow_down_factor = 1;
        DSP_MEMORY(scan).xyz_gain  = 0x000A0A0A;

        for (int i=0; i<8; ++i){
                DSP_MEMORY(scan_event_trigger).xp_bias_setpoint[i] = -1;
                DSP_MEMORY(scan_event_trigger).xm_bias_setpoint[i] = -1;
        }

        DSP_MEMORY(probe).lockin_shr_corrprod = 16;
        DSP_MEMORY(probe).lockin_shr_corrsum  = 8;

        DSP_MEMORY(autoapp).wave_out_channel[0]  = 3;
        DSP_MEMORY(autoapp).wave_out_channel[1]  = 4;
        DSP_MEMORY(autoapp).n_wait_fin = 10000;
        DSP_MEMORY(autoapp).ci_mult = 20;

        DSP_MEMORY(datafifo).length = DATAFIFO_LENGTH;
        DSP_MEMORY(datafifo).mode   = 1;
        
        DSP_MEMORY(probe_datafifo).length = EXTERN_PROBEDATAFIFO_LENGTH;

#endif
        DSP_MEMORY(probe).vector_head = &DSP_MEMORY(prbvh[0]);
        DSP_MEMORY(probe_datafifo).buffer_base = &DSP_MEMORY(prbdf[0]);
        
        int i;
	for (i=0; i<10; ++i){ // make default zero value pointer sane state
		DSP_MEMORY(analog).out[i].p       = &DSP_MEMORY(analog).vnull; // default "null"
		DSP_MEMORY(analog).out[i].add_a_p = &DSP_MEMORY(analog).vnull; // default "null"
		DSP_MEMORY(analog).out[i].sub_b_p = &DSP_MEMORY(analog).vnull; // default "null"
		DSP_MEMORY(analog).out[i].smac_a_fac_q15_p = NULL; // NULL PTR
		DSP_MEMORY(analog).out[i].smac_b_fac_q15_p = NULL; // NULL PTR
	}
	for (i=0; i<4; ++i){ // make default zero value pointer sane state
		DSP_MEMORY(analog).diff_in_p[i] = &DSP_MEMORY(analog).vnull;
	}

	// setup Z-Offset output with XY-Scan-slope compensation added
	DSP_MEMORY(analog).out[2].p       = &DSP_MEMORY(move).xyz_vec[i_Z];     // default move Z
	DSP_MEMORY(analog).out[2].add_a_p = &DSP_MEMORY(scan).z_offset_xyslope; // default add scan Z offset by XY slope

	// setup XY Scan out with offset
	DSP_MEMORY(analog).out[3].p       = &DSP_MEMORY(scan).xy_r_vec[i_X];  // default scan X, rot
	DSP_MEMORY(analog).out[3].add_a_p = &DSP_MEMORY(move).xyz_vec[i_X];   // default add move offset X --- replace with DSP_MEMORY(analog).vnull signal if any external offset adding is used!
	DSP_MEMORY(analog).out[4].p       = &DSP_MEMORY(scan).xy_r_vec[i_Y];  // default scan Y, rot
	DSP_MEMORY(analog).out[4].add_a_p = &DSP_MEMORY(move).xyz_vec[i_Y];   // default add move offset X --- replace with DSP_MEMORY(analog).vnull signal if any external offset adding is used!

	// setup Z Scan out with Z probe control offset
	DSP_MEMORY(analog).out[5].p       = &DSP_MEMORY(z_servo).neg_control; // default Z-servo
	DSP_MEMORY(analog).out[5].sub_b_p = &DSP_MEMORY(probe).Zpos;          // default Z-Pos Probe
	DSP_MEMORY(analog).out[5].add_a_p = &DSP_MEMORY(probe).LockIn_ref;    // default LockIn hook -- Z mod
	DSP_MEMORY(analog).out[5].smac_a_fac_q15_p = &DSP_MEMORY(probe).AC_amp_aux; // default lockIn hook -- Z mod amp. [set Z amp to zero for bais mod, or vs]

	// setup Bias and Motor outputs
	DSP_MEMORY(analog).out[6].p       = &DSP_MEMORY(probe).Upos;          // default Bias via Probe U Position
	DSP_MEMORY(analog).out[6].sub_b_p = &DSP_MEMORY(probe).LockIn_ref;    // default LockIn hook -- Bias mod.
	DSP_MEMORY(analog).out[6].smac_b_fac_q15_p = &DSP_MEMORY(probe).AC_amp; // default lockIn hook -- Bias mod. amp

	// Motor out -- NOTE: OUT7 is overrided if PLL is not active or not updated if PLL processing is emabled and outputs the exci sigbal here!
	DSP_MEMORY(analog).out[7].p       = &DSP_MEMORY(analog).motor;        // default Motor control output

	// setup wave channels -- redirected via probe out index
	DSP_MEMORY(analog).out[8].p       = &DSP_MEMORY(analog).wave[0];      // wave[0] out destination
	DSP_MEMORY(analog).out[9].p       = &DSP_MEMORY(analog).wave[1];      // wave[1] out destination

	// setup RMS module input
	DSP_MEMORY(analog).avg_input      = &DSP_MEMORY(analog).in[0];  // old choice: &DSP_MEMORY(feedback_mixer).FB_IN_processed[0];

	// setup mixer defaults
	DSP_MEMORY(feedback_mixer).input_p[0] = &DSP_MEMORY(analog).in[0]; // *** IN0..3 MIXER defaults
	DSP_MEMORY(feedback_mixer).FB_IN_is_analog_flg[0] = 1;
	DSP_MEMORY(feedback_mixer).iir_ca_q15[0] = (Q15)>>2; // slight low pass IIR

	DSP_MEMORY(feedback_mixer).input_p[1] = &DSP_MEMORY(analog).in[1];
	DSP_MEMORY(feedback_mixer).FB_IN_is_analog_flg[1] = 1;
	DSP_MEMORY(feedback_mixer).iir_ca_q15[1] = Q15-1; // FBW

	DSP_MEMORY(feedback_mixer).input_p[2] = &DSP_MEMORY(analog).in[2];
	DSP_MEMORY(feedback_mixer).FB_IN_is_analog_flg[2] = 1;
	DSP_MEMORY(feedback_mixer).iir_ca_q15[2] = Q15-1; // FBW

	DSP_MEMORY(feedback_mixer).input_p[3] = &DSP_MEMORY(analog).in[3];
	DSP_MEMORY(feedback_mixer).FB_IN_is_analog_flg[3] = 1;
	DSP_MEMORY(feedback_mixer).iir_ca_q15[3] = Q15-1; // FBW

	// setup LockIn defaults
	DSP_MEMORY(probe).LockIn_input[0] = &DSP_MEMORY(feedback_mixer).FB_IN_processed[0]; // defaults for LockIn Inputs
	DSP_MEMORY(probe).LockIn_input[1] = &DSP_MEMORY(feedback_mixer).FB_IN_processed[0];
	DSP_MEMORY(probe).src_input[0] = &DSP_MEMORY(probe).LockIn_1stB; // defaults for VP input mapping
	DSP_MEMORY(probe).src_input[1] = &DSP_MEMORY(probe).LockIn_2ndA;
	DSP_MEMORY(probe).src_input[2] = &DSP_MEMORY(probe).LockIn_2ndA;
	DSP_MEMORY(probe).src_input[3] = &DSP_MEMORY(analog).counter[0];
	DSP_MEMORY(probe).prb_output[0] = &DSP_MEMORY(scan).xyz_vec[i_X]; // defaults for VP output mapping
	DSP_MEMORY(probe).prb_output[1] = &DSP_MEMORY(scan).xyz_vec[i_Y];
	DSP_MEMORY(probe).prb_output[2] = &DSP_MEMORY(move).xyz_vec[i_X];
	DSP_MEMORY(probe).prb_output[3] = &DSP_MEMORY(move).xyz_vec[i_Y];
	DSP_MEMORY(probe).limiter_updn[0] = &DSP_MEMORY(user_input_signal_array[0]);
	DSP_MEMORY(probe).limiter_updn[1] = &DSP_MEMORY(user_input_signal_array[1]);
	DSP_MEMORY(probe).limiter_input = &DSP_MEMORY(analog).in[0];
	DSP_MEMORY(probe).tracker_input = &DSP_MEMORY(z_servo).neg_control;
	DSP_MEMORY(probe).trigger_input = &DSP_MEMORY(probe).gpio_data; // default to GPIO in buffer

	// setup default area scan flex input signal mappings:
	DSP_MEMORY(scan).src_input[0] = &DSP_MEMORY(probe).LockIn_1stA;
	DSP_MEMORY(scan).src_input[1] = &DSP_MEMORY(probe).LockIn_2ndA;
	DSP_MEMORY(scan).src_input[2] = &DSP_MEMORY(probe).LockIn_0A;
	DSP_MEMORY(scan).src_input[3] = &DSP_MEMORY(analog).counter[0];
}

#ifndef DSPEMU
#pragma CODE_SECTION(adjust_signal_input, ".text:slow")
#endif

void adjust_signal_input (){
	int is_analog;
	is_analog = 0;
	// full sanity check for signal id here

	// a few FLASH table managemnt and generic FLASH support tools
	if (DSP_MEMORY(sig_mon).signal_id == DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_ID){
		DSP_MEMORY(sig_mon).signal_id = restore_configuration_from_flash();
		DSP_MEMORY(sig_mon).mindex = -10;
		return;
	}
	if (DSP_MEMORY(sig_mon).signal_id == DSP_SIGNAL_TABLE_STORE_TO_FLASH_ID){
		DSP_MEMORY(sig_mon).signal_id = store_configuration_to_flash();
		DSP_MEMORY(sig_mon).mindex = -11;
		return;
	}
	if (DSP_MEMORY(sig_mon).signal_id == DSP_SIGNAL_TABLE_ERASE_FLASH_ID){
		flash_erase(); // erase sector
		DSP_MEMORY(sig_mon).mindex = -12;
		return;
	}
	if (DSP_MEMORY(sig_mon).signal_id == DSP_SIGNAL_TABLE_SEEKRW_FLASH_ID){
		flash_seek(DSP_MEMORY(sig_mon).mindex, 'r'); // seek
		flash_seek(DSP_MEMORY(sig_mon).mindex, 'w'); // seek
		DSP_MEMORY(sig_mon).mindex = -13;
		return;
	}
	if (DSP_MEMORY(sig_mon).signal_id == DSP_SIGNAL_TABLE_READ_FLASH_ID){
		DSP_MEMORY(sig_mon).signal_id = flash_read16(); // read
		DSP_MEMORY(sig_mon).mindex = -14;
		return;
	}
	if (DSP_MEMORY(sig_mon).signal_id == DSP_SIGNAL_TABLE_WRITE_FLASH_ID){
		flash_write16(DSP_MEMORY(sig_mon).mindex); // write -- must erase before!
		DSP_MEMORY(sig_mon).mindex = -15;
		return;
	}
	if (DSP_MEMORY(sig_mon).signal_id == DSP_SIGNAL_TABLE_TEST0_FLASH_ID){
		TestFlash_HL ();
		DSP_MEMORY(sig_mon).mindex = -20;
		return;
	}
	if (DSP_MEMORY(sig_mon).signal_id == DSP_SIGNAL_TABLE_TEST1_FLASH_ID){
		flash_seek (0, 'w'); // seek to GXSM flash sector/address
		flash_erase(); // erase sector

		// store FLASH HEADER informations:
		flash_write16 (FB_SPM_FLASHBLOCK_IDENTIFICATION_A);
		flash_write16 (FB_SPM_FLASHBLOCK_IDENTIFICATION_B);
		DSP_MEMORY(sig_mon).mindex = -21;
		return;
	}

	if (DSP_MEMORY(sig_mon).signal_id == DSP_SIGNAL_NULL_POINTER_REQUEST_ID){ // NULL pointer requested, allowed for some signals adjusted here
		switch (DSP_MEMORY(sig_mon).mindex){
		case 0: setup_default_signal_configuration(); break; // revert to power up default
		case DSP_SIGNAL_OUTMIX_CH0_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[0].smac_a_fac_q15_p = NULL; break;
		case DSP_SIGNAL_OUTMIX_CH0_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[0].smac_b_fac_q15_p = NULL; break;
		case DSP_SIGNAL_OUTMIX_CH1_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[1].smac_a_fac_q15_p = NULL; break;
		case DSP_SIGNAL_OUTMIX_CH1_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[1].smac_b_fac_q15_p = NULL; break;
		case DSP_SIGNAL_OUTMIX_CH2_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[2].smac_a_fac_q15_p = NULL; break;
		case DSP_SIGNAL_OUTMIX_CH2_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[2].smac_b_fac_q15_p = NULL; break;
		case DSP_SIGNAL_OUTMIX_CH3_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[3].smac_a_fac_q15_p = NULL; break;
		case DSP_SIGNAL_OUTMIX_CH3_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[3].smac_b_fac_q15_p = NULL; break;
		case DSP_SIGNAL_OUTMIX_CH4_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[4].smac_a_fac_q15_p = NULL; break;
		case DSP_SIGNAL_OUTMIX_CH4_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[4].smac_b_fac_q15_p = NULL; break;
		case DSP_SIGNAL_OUTMIX_CH5_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[5].smac_a_fac_q15_p = NULL; break;
		case DSP_SIGNAL_OUTMIX_CH5_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[5].smac_b_fac_q15_p = NULL; break;
		case DSP_SIGNAL_OUTMIX_CH6_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[6].smac_a_fac_q15_p = NULL; break;
		case DSP_SIGNAL_OUTMIX_CH6_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[6].smac_b_fac_q15_p = NULL; break;
		case DSP_SIGNAL_OUTMIX_CH7_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[7].smac_a_fac_q15_p = NULL; break;
		case DSP_SIGNAL_OUTMIX_CH7_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[7].smac_b_fac_q15_p = NULL; break;
		case DSP_SIGNAL_SCO1_INPUT_ID: DSP_MEMORY(sco_s[0]).in = NULL; break;
                case DSP_SIGNAL_SCO2_INPUT_ID: DSP_MEMORY(sco_s[1]).in = NULL; break;
		default:
			DSP_MEMORY(sig_mon).mindex = DSP_SIGNAL_ADJUST_INVALID_SIGNAL_INPUT_ERROR; 
			return; // invalid signal input id.
		}
		DSP_MEMORY(sig_mon).mindex = -1; // all good, done.
		return;
	}
	if ((DSP_MEMORY(sig_mon).signal_id & 0xffff) < 0){ // no good, abort
		DSP_MEMORY(sig_mon).mindex = DSP_SIGNAL_ADJUST_NEGID_ERROR;
		return;
	}
	if ((DSP_MEMORY(sig_mon).signal_id & 0xffff) >= NUM_SIGNALS){ // check if valid (in list) -- if no good, abort
		DSP_MEMORY(sig_mon).mindex = DSP_SIGNAL_ADJUST_INVALIDID_ERROR;
		return;
	}

	DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(dsp_signal_lookup[DSP_MEMORY(sig_mon).signal_id & 0xffff]).p + ((DSP_MEMORY(sig_mon).signal_id & DSP_SIGNAL_VECTOR_ELEMENT_INDEX_REQUEST_MSK)>>16);

	if (DSP_MEMORY(sig_mon).signal_id < 8){ // is analog signal?
		is_analog = 1;
	}
	// monitor input?
	if (DSP_MEMORY(sig_mon).mindex >= 0 && DSP_MEMORY(sig_mon).mindex < NUM_MONITOR_SIGNALS){
		DSP_MEMORY(sig_mon).signal_p[DSP_MEMORY(sig_mon).mindex] = DSP_MEMORY(sig_mon).act_address_signal;
		DSP_MEMORY(sig_mon).act_address_input_set = DSP_MEMORY(sig_mon).signal_p[DSP_MEMORY(sig_mon).mindex];
	} else {
                // too much code overhead, just keep ID here -- intentional integer to pointer here.
		DSP_MEMORY(sig_mon).act_address_input_set = (DSP_INT32_P)(DSP_MEMORY(sig_mon).mindex);
                
		switch (DSP_MEMORY(sig_mon).mindex){
		case DSP_SIGNAL_Z_SERVO_INPUT_ID: DSP_MEMORY(z_servo).input = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_M_SERVO_INPUT_ID: DSP_MEMORY(m_servo).input = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_MIXER0_INPUT_ID: DSP_MEMORY(feedback_mixer).input_p[0] = DSP_MEMORY(sig_mon).act_address_signal; DSP_MEMORY(feedback_mixer).FB_IN_is_analog_flg[0] = is_analog; break;
		case DSP_SIGNAL_MIXER1_INPUT_ID: DSP_MEMORY(feedback_mixer).input_p[1] = DSP_MEMORY(sig_mon).act_address_signal; DSP_MEMORY(feedback_mixer).FB_IN_is_analog_flg[1] = is_analog; break;
		case DSP_SIGNAL_MIXER2_INPUT_ID: DSP_MEMORY(feedback_mixer).input_p[2] = DSP_MEMORY(sig_mon).act_address_signal; DSP_MEMORY(feedback_mixer).FB_IN_is_analog_flg[2] = is_analog; break;
		case DSP_SIGNAL_MIXER3_INPUT_ID: DSP_MEMORY(feedback_mixer).input_p[3] = DSP_MEMORY(sig_mon).act_address_signal; DSP_MEMORY(feedback_mixer).FB_IN_is_analog_flg[3] = is_analog; break;

		case DSP_SIGNAL_DIFF_IN0_ID: DSP_MEMORY(analog).diff_in_p[0] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_DIFF_IN1_ID: DSP_MEMORY(analog).diff_in_p[1] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_DIFF_IN2_ID: DSP_MEMORY(analog).diff_in_p[2] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_DIFF_IN3_ID: DSP_MEMORY(analog).diff_in_p[3] = DSP_MEMORY(sig_mon).act_address_signal; break;

		case DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID: DSP_MEMORY(scan).src_input[0] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_SCAN_CHANNEL_MAP1_ID: DSP_MEMORY(scan).src_input[1] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_SCAN_CHANNEL_MAP2_ID: DSP_MEMORY(scan).src_input[2] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_SCAN_CHANNEL_MAP3_ID: DSP_MEMORY(scan).src_input[3] = DSP_MEMORY(sig_mon).act_address_signal; break;

		case DSP_SIGNAL_LOCKIN_A_INPUT_ID: DSP_MEMORY(probe).LockIn_input[0] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_LOCKIN_B_INPUT_ID: DSP_MEMORY(probe).LockIn_input[1] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_VECPROBE0_INPUT_ID: DSP_MEMORY(probe).src_input[0] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_VECPROBE1_INPUT_ID: DSP_MEMORY(probe).src_input[1] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_VECPROBE2_INPUT_ID: DSP_MEMORY(probe).src_input[2] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_VECPROBE3_INPUT_ID: DSP_MEMORY(probe).src_input[3] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_VECPROBE0_CONTROL_ID: DSP_MEMORY(probe).prb_output[0] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_VECPROBE1_CONTROL_ID: DSP_MEMORY(probe).prb_output[1] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_VECPROBE2_CONTROL_ID: DSP_MEMORY(probe).prb_output[2] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_VECPROBE3_CONTROL_ID: DSP_MEMORY(probe).prb_output[3] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_VECPROBE_TRIGGER_INPUT_ID: DSP_MEMORY(probe).trigger_input = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID: DSP_MEMORY(probe).limiter_input = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_VECPROBE_LIMITER_UP_INPUT_ID: DSP_MEMORY(probe).limiter_updn[0] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_VECPROBE_LIMITER_DN_INPUT_ID: DSP_MEMORY(probe).limiter_updn[1] = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_VECPROBE_TRACKER_INPUT_ID: DSP_MEMORY(probe).tracker_input = DSP_MEMORY(sig_mon).act_address_signal; break;

		case DSP_SIGNAL_OUTMIX_CH0_INPUT_ID:        DSP_MEMORY(analog).out[0].p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH0_ADD_A_INPUT_ID:  DSP_MEMORY(analog).out[0].add_a_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH0_SUB_B_INPUT_ID:  DSP_MEMORY(analog).out[0].sub_b_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH0_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[0].smac_a_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH0_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[0].smac_b_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;

		case DSP_SIGNAL_OUTMIX_CH1_INPUT_ID:        DSP_MEMORY(analog).out[1].p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH1_ADD_A_INPUT_ID:  DSP_MEMORY(analog).out[1].add_a_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH1_SUB_B_INPUT_ID:  DSP_MEMORY(analog).out[1].sub_b_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH1_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[1].smac_a_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH1_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[1].smac_b_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;

		case DSP_SIGNAL_OUTMIX_CH2_INPUT_ID:        DSP_MEMORY(analog).out[2].p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH2_ADD_A_INPUT_ID:  DSP_MEMORY(analog).out[2].add_a_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH2_SUB_B_INPUT_ID:  DSP_MEMORY(analog).out[2].sub_b_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH2_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[2].smac_a_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH2_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[2].smac_b_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;

		case DSP_SIGNAL_OUTMIX_CH3_INPUT_ID:        DSP_MEMORY(analog).out[3].p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH3_ADD_A_INPUT_ID:  DSP_MEMORY(analog).out[3].add_a_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH3_SUB_B_INPUT_ID:  DSP_MEMORY(analog).out[3].sub_b_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH3_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[3].smac_a_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH3_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[3].smac_b_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;

		case DSP_SIGNAL_OUTMIX_CH4_INPUT_ID:        DSP_MEMORY(analog).out[4].p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH4_ADD_A_INPUT_ID:  DSP_MEMORY(analog).out[4].add_a_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH4_SUB_B_INPUT_ID:  DSP_MEMORY(analog).out[4].sub_b_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH4_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[4].smac_a_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH4_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[4].smac_b_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;

		case DSP_SIGNAL_OUTMIX_CH5_INPUT_ID:        DSP_MEMORY(analog).out[5].p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH5_ADD_A_INPUT_ID:  DSP_MEMORY(analog).out[5].add_a_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH5_SUB_B_INPUT_ID:  DSP_MEMORY(analog).out[5].sub_b_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH5_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[5].smac_a_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH5_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[5].smac_b_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;

		case DSP_SIGNAL_OUTMIX_CH6_INPUT_ID:        DSP_MEMORY(analog).out[6].p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH6_ADD_A_INPUT_ID:  DSP_MEMORY(analog).out[6].add_a_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH6_SUB_B_INPUT_ID:  DSP_MEMORY(analog).out[6].sub_b_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH6_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[6].smac_a_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH6_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[6].smac_b_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;

		case DSP_SIGNAL_OUTMIX_CH7_INPUT_ID:        DSP_MEMORY(analog).out[7].p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH7_ADD_A_INPUT_ID:  DSP_MEMORY(analog).out[7].add_a_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH7_SUB_B_INPUT_ID:  DSP_MEMORY(analog).out[7].sub_b_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH7_SMAC_A_INPUT_ID: DSP_MEMORY(analog).out[7].smac_a_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH7_SMAC_B_INPUT_ID: DSP_MEMORY(analog).out[7].smac_b_fac_q15_p = DSP_MEMORY(sig_mon).act_address_signal; break;

		case DSP_SIGNAL_OUTMIX_CH8_INPUT_ID:        DSP_MEMORY(analog).out[8].p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH8_ADD_A_INPUT_ID:  DSP_MEMORY(analog).out[8].add_a_p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH9_INPUT_ID:        DSP_MEMORY(analog).out[9].p = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_OUTMIX_CH9_ADD_A_INPUT_ID:  DSP_MEMORY(analog).out[9].add_a_p = DSP_MEMORY(sig_mon).act_address_signal; break;

		case DSP_SIGNAL_ANALOG_AVG_INPUT_ID:        DSP_MEMORY(analog).avg_input = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID:     DSP_MEMORY_PAC(pSignal1) = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID:     DSP_MEMORY_PAC(pSignal2) = DSP_MEMORY(sig_mon).act_address_signal; break;

		case DSP_SIGNAL_SCO1_INPUT_ID:              DSP_MEMORY(sco_s[0]).in = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_SCO1_AMPLITUDE_INPUT_ID:    DSP_MEMORY(sco_s[0]).amplitude = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_SCO2_INPUT_ID:              DSP_MEMORY(sco_s[1]).in = DSP_MEMORY(sig_mon).act_address_signal; break;
		case DSP_SIGNAL_SCO2_AMPLITUDE_INPUT_ID:    DSP_MEMORY(sco_s[1]).amplitude = DSP_MEMORY(sig_mon).act_address_signal; break;
 
		default:
			DSP_MEMORY(sig_mon).mindex = DSP_SIGNAL_ADJUST_INVALID_SIGNAL_ERROR; 
			return; // invalid signal input id.
		}
	}
	DSP_MEMORY(sig_mon).mindex = -1; // all good, done.
}

#ifndef DSPEMU
#pragma CODE_SECTION(query_signal_input, ".text:slow")
#endif

void query_signal_input (){
	// monitor input?
	if (DSP_MEMORY(sig_mon).mindex >= 0 && DSP_MEMORY(sig_mon).mindex < NUM_MONITOR_SIGNALS){
		DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(sig_mon).signal_p[DSP_MEMORY(sig_mon).mindex];
	} else {
		switch (DSP_MEMORY(sig_mon).mindex){
		case DSP_SIGNAL_Z_SERVO_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(z_servo).input; break;
		case DSP_SIGNAL_M_SERVO_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(m_servo).input; break;
		case DSP_SIGNAL_MIXER0_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(feedback_mixer).input_p[0]; break;
		case DSP_SIGNAL_MIXER1_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(feedback_mixer).input_p[1]; break;
		case DSP_SIGNAL_MIXER2_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(feedback_mixer).input_p[2]; break;
		case DSP_SIGNAL_MIXER3_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(feedback_mixer).input_p[3]; break;
			
		case DSP_SIGNAL_DIFF_IN0_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).diff_in_p[0]; break;
		case DSP_SIGNAL_DIFF_IN1_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).diff_in_p[1]; break;
		case DSP_SIGNAL_DIFF_IN2_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).diff_in_p[2]; break;
		case DSP_SIGNAL_DIFF_IN3_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).diff_in_p[3]; break;

		case DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(scan).src_input[0]; break;
		case DSP_SIGNAL_SCAN_CHANNEL_MAP1_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(scan).src_input[1]; break;
		case DSP_SIGNAL_SCAN_CHANNEL_MAP2_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(scan).src_input[2]; break;
		case DSP_SIGNAL_SCAN_CHANNEL_MAP3_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(scan).src_input[3]; break;

		case DSP_SIGNAL_LOCKIN_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(probe).LockIn_input[0]; break;
		case DSP_SIGNAL_LOCKIN_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(probe).LockIn_input[1]; break;
		case DSP_SIGNAL_VECPROBE0_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(probe).src_input[0]; break;
		case DSP_SIGNAL_VECPROBE1_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(probe).src_input[1]; break;
		case DSP_SIGNAL_VECPROBE2_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(probe).src_input[2]; break;
		case DSP_SIGNAL_VECPROBE3_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(probe).src_input[3]; break;
		case DSP_SIGNAL_VECPROBE0_CONTROL_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(probe).prb_output[0]; break;
		case DSP_SIGNAL_VECPROBE1_CONTROL_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(probe).prb_output[1]; break;
		case DSP_SIGNAL_VECPROBE2_CONTROL_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(probe).prb_output[2]; break;
		case DSP_SIGNAL_VECPROBE3_CONTROL_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(probe).prb_output[3]; break;
		case DSP_SIGNAL_VECPROBE_TRIGGER_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(probe).trigger_input; break;
		case DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(probe).limiter_input; break;
		case DSP_SIGNAL_VECPROBE_LIMITER_UP_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(probe).limiter_updn[0]; break;
		case DSP_SIGNAL_VECPROBE_LIMITER_DN_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(probe).limiter_updn[1]; break;
		case DSP_SIGNAL_VECPROBE_TRACKER_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(probe).tracker_input; break;
		
		case DSP_SIGNAL_OUTMIX_CH0_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =        DSP_MEMORY(analog).out[0].p; break;
		case DSP_SIGNAL_OUTMIX_CH0_ADD_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[0].add_a_p; break;
		case DSP_SIGNAL_OUTMIX_CH0_SUB_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[0].sub_b_p; break;
		case DSP_SIGNAL_OUTMIX_CH0_SMAC_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[0].smac_a_fac_q15_p; break;
		case DSP_SIGNAL_OUTMIX_CH0_SMAC_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[0].smac_b_fac_q15_p; break;
		
		case DSP_SIGNAL_OUTMIX_CH1_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =        DSP_MEMORY(analog).out[1].p; break;
		case DSP_SIGNAL_OUTMIX_CH1_ADD_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[1].add_a_p; break;
		case DSP_SIGNAL_OUTMIX_CH1_SUB_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[1].sub_b_p; break;
		case DSP_SIGNAL_OUTMIX_CH1_SMAC_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[1].smac_a_fac_q15_p; break;
		case DSP_SIGNAL_OUTMIX_CH1_SMAC_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[1].smac_b_fac_q15_p; break;
		
		case DSP_SIGNAL_OUTMIX_CH2_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =        DSP_MEMORY(analog).out[2].p; break;
		case DSP_SIGNAL_OUTMIX_CH2_ADD_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[2].add_a_p; break;
		case DSP_SIGNAL_OUTMIX_CH2_SUB_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[2].sub_b_p; break;
		case DSP_SIGNAL_OUTMIX_CH2_SMAC_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[2].smac_a_fac_q15_p; break;
		case DSP_SIGNAL_OUTMIX_CH2_SMAC_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[2].smac_b_fac_q15_p; break;
		
		case DSP_SIGNAL_OUTMIX_CH3_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =        DSP_MEMORY(analog).out[3].p; break;
		case DSP_SIGNAL_OUTMIX_CH3_ADD_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[3].add_a_p; break;
		case DSP_SIGNAL_OUTMIX_CH3_SUB_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[3].sub_b_p; break;
		case DSP_SIGNAL_OUTMIX_CH3_SMAC_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[3].smac_a_fac_q15_p; break;
		case DSP_SIGNAL_OUTMIX_CH3_SMAC_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[3].smac_b_fac_q15_p; break;
		
		case DSP_SIGNAL_OUTMIX_CH4_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =        DSP_MEMORY(analog).out[4].p; break;
		case DSP_SIGNAL_OUTMIX_CH4_ADD_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[4].add_a_p; break;
		case DSP_SIGNAL_OUTMIX_CH4_SUB_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[4].sub_b_p; break;
		case DSP_SIGNAL_OUTMIX_CH4_SMAC_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[4].smac_a_fac_q15_p; break;
		case DSP_SIGNAL_OUTMIX_CH4_SMAC_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[4].smac_b_fac_q15_p; break;
		
		case DSP_SIGNAL_OUTMIX_CH5_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =        DSP_MEMORY(analog).out[5].p; break;
		case DSP_SIGNAL_OUTMIX_CH5_ADD_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[5].add_a_p; break;
		case DSP_SIGNAL_OUTMIX_CH5_SUB_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[5].sub_b_p; break;
		case DSP_SIGNAL_OUTMIX_CH5_SMAC_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[5].smac_a_fac_q15_p; break;
		case DSP_SIGNAL_OUTMIX_CH5_SMAC_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[5].smac_b_fac_q15_p; break;
		
		case DSP_SIGNAL_OUTMIX_CH6_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =        DSP_MEMORY(analog).out[6].p; break;
		case DSP_SIGNAL_OUTMIX_CH6_ADD_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[6].add_a_p; break;
		case DSP_SIGNAL_OUTMIX_CH6_SUB_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[6].sub_b_p; break;
		case DSP_SIGNAL_OUTMIX_CH6_SMAC_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[6].smac_a_fac_q15_p; break;
		case DSP_SIGNAL_OUTMIX_CH6_SMAC_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[6].smac_b_fac_q15_p; break;

		case DSP_SIGNAL_OUTMIX_CH7_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =        DSP_MEMORY(analog).out[7].p; break;
		case DSP_SIGNAL_OUTMIX_CH7_ADD_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[7].add_a_p; break;
		case DSP_SIGNAL_OUTMIX_CH7_SUB_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[7].sub_b_p; break;
		case DSP_SIGNAL_OUTMIX_CH7_SMAC_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[7].smac_a_fac_q15_p; break;
		case DSP_SIGNAL_OUTMIX_CH7_SMAC_B_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).out[7].smac_b_fac_q15_p; break;
		
		case DSP_SIGNAL_OUTMIX_CH8_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =        DSP_MEMORY(analog).out[8].p; break;
		case DSP_SIGNAL_OUTMIX_CH8_ADD_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[8].add_a_p; break;
		case DSP_SIGNAL_OUTMIX_CH9_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =        DSP_MEMORY(analog).out[9].p; break;
		case DSP_SIGNAL_OUTMIX_CH9_ADD_A_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal =  DSP_MEMORY(analog).out[9].add_a_p; break;

		case DSP_SIGNAL_ANALOG_AVG_INPUT_ID:    DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(analog).avg_input; break;
		case DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY_PAC(pSignal1); break;
		case DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY_PAC(pSignal2); break;

		case DSP_SIGNAL_SCO1_INPUT_ID:           DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(sco_s[0]).in; break;
		case DSP_SIGNAL_SCO1_AMPLITUDE_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(sco_s[0]).amplitude; break;
		case DSP_SIGNAL_SCO2_INPUT_ID:           DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(sco_s[1]).in; break;
		case DSP_SIGNAL_SCO2_AMPLITUDE_INPUT_ID: DSP_MEMORY(sig_mon).act_address_signal = DSP_MEMORY(sco_s[1]).amplitude; break;
 
		default:
			DSP_MEMORY(sig_mon).mindex = DSP_SIGNAL_ADJUST_INVALID_SIGNAL_ERROR; 
			return; // invalid signal input id.
		}
	}
	DSP_MEMORY(sig_mon).mindex = -1; // all good, done.
}

#ifndef DSPEMU
#pragma CODE_SECTION(store_configuration_to_flash, ".text:slow")
#endif

int store_configuration_to_flash(){
	int i, id, id_block;

	flash_seek (0, 'w'); // seek to GXSM flash sector/address
	flash_erase(); // erase sector

	// store FLASH HEADER informations:
	flash_write16 (FB_SPM_FLASHBLOCK_IDENTIFICATION_A);
	flash_write16 (FB_SPM_FLASHBLOCK_IDENTIFICATION_B);

	flash_write16 (FB_SPM_SOFT_ID);
	flash_write16 (FB_SPM_VERSION);

	flash_write16 (FB_SPM_DATE_YEAR);
	flash_write16 (FB_SPM_DATE_MMDD);

	flash_write16 (FB_SPM_SIGNAL_LIST_VERSION);
	flash_write16 (FB_SPM_SIGNAL_INPUT_VERSION);


	flash_write16 (NUM_SIGNALS);
	flash_write16 (LAST_INPUT_ID);

	flash_write16 (MAX_INPUT_NUMBER_LIMIT);
	flash_write16 (0xeffe); // INPUT_ID used as TABLE END MARK

	flash_write16 (0);
	flash_write16 (0);

	flash_write16 (0);
	flash_write16 (0);

	// query and iterate over all inputs, seek signal id, store found id combinations to flash
	i=0;
	for (id_block=0x0000; id_block < 0x8000; id_block += 0x1000){
		id = id_block;
		if (id >= 0x1000) // starting at 0xN001 except monitor signals.
			++id;
		do {
			DSP_MEMORY(sig_mon).mindex = id;
			query_signal_input (); // get actual address

			if (DSP_MEMORY(sig_mon).act_address_signal == NULL){
				flash_write16 (id);  // NULL POINTER: store DSP_MEMORY(sig_mon).mindex (INPUT ID) and DSP_SIGNAL_NULL_POINTER_REQUEST_ID for signal id
				flash_write16 (DSP_SIGNAL_NULL_POINTER_REQUEST_ID);
			}

			for (DSP_MEMORY(sig_mon).signal_id = 0; DSP_MEMORY(sig_mon).signal_id < NUM_SIGNALS; ++DSP_MEMORY(sig_mon).signal_id){
				if (DSP_MEMORY(sig_mon).act_address_signal == DSP_MEMORY(dsp_signal_lookup[DSP_MEMORY(sig_mon).signal_id]).p){ // match with signal list to find signal id
					flash_write16 (id);    // store  DSP_MEMORY(sig_mon).mindex (INPUT ID) and DSP_MEMORY(sig_mon).signal_id (SIGNAL ID) pairs
					flash_write16 (DSP_MEMORY(sig_mon).signal_id);
					break;
				}
			}
			++id;
		} while (DSP_MEMORY(sig_mon).mindex == -1 && ++i < MAX_INPUT_NUMBER_LIMIT);
	}

	// store end table mark
	flash_write16 (0xeffe);
	flash_write16 (0xfeef);
	flash_write16 (0x0000);
	flash_write16 (0x0000);
	
	return i;
}

#ifndef DSPEMU
#pragma CODE_SECTION(restore_configuration_from_flash, ".text:slow")
#endif

int restore_configuration_from_flash(){
	int i;
	int id1, id2;
	int ret = -1;

	flash_seek (0, 'r');
	// check INPUT TABLE ID / DATE / VERSION.... for identification
	id1 = flash_read16();
	id2 = flash_read16();
	if (id1 != FB_SPM_FLASHBLOCK_IDENTIFICATION_A || id2 != FB_SPM_FLASHBLOCK_IDENTIFICATION_B)
		return DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_FLASH_BLOCKID_ERROR;

	id1=flash_read16();
	id2=flash_read16();
	if (id1 != FB_SPM_SOFT_ID || id2 != FB_SPM_VERSION)
		return DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_VERSION_MISMATCH_ERROR;

	id1=flash_read16();
	id2=flash_read16();
	if (id1 != FB_SPM_DATE_YEAR || id2 != FB_SPM_DATE_MMDD)  
		ret = DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_FLASH_DATE_WARNING;

	// most critical matches
	id1=flash_read16();
	id2=flash_read16();
	if (id1 != FB_SPM_SIGNAL_LIST_VERSION)
		return DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_SIGNAL_LIST_VERSION_MISMATCH;
	if (id2 != FB_SPM_SIGNAL_INPUT_VERSION)
		return DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_INPUT_LIST_VERSION_MISMATCH;
	
	// skip forward only
	id1=flash_read16();
	id2=flash_read16();
	id1=flash_read16();
	id2=flash_read16();

	id1=flash_read16();
	id2=flash_read16();
	id1=flash_read16();
	id2=flash_read16();

	i = 0;
	do {
		// read  DSP_MEMORY(sig_mon).mindex (INPUT ID) and DSP_MEMORY(sig_mon).signal_id (SIGNAL ID) pairs
		DSP_MEMORY(sig_mon).mindex    = flash_read16();
		DSP_MEMORY(sig_mon).signal_id = flash_read16();
		if (DSP_MEMORY(sig_mon).mindex == 0xeffe)
			break; // end mark
		adjust_signal_input (); // adjust signal
		// abort on 1st error
		//	} while (DSP_MEMORY(sig_mon).mindex == -1 && ++i < MAX_INPUT_NUMBER_LIMIT); // abort on 1st/any error -- max inputs set: 100
	} while (++i < MAX_INPUT_NUMBER_LIMIT); // try all

	return ret;
}

/**
 * Magic Structure at locked DSP memory address "MAGIC"
 **/

#ifndef DSPEMU
#pragma DATA_SECTION(magic,"MAGIC")
#endif

SPM_MAGIC_DATA_LOCATIONS magic;

extern int     sigma_delta_hr_mask[];

#ifndef DSPEMU
#pragma CODE_SECTION(fill_magic, ".text:slow")
#endif

void fill_magic()
{
	/* fill magic struct, contains version info and pointers to data structs */
	DSP_MEMORY(magic).magic        = 0x00000000; /* not valid yet! */
	DSP_MEMORY(magic).version      = FB_SPM_VERSION;
	DSP_MEMORY(magic).year         = FB_SPM_DATE_YEAR;
	DSP_MEMORY(magic).mmdd         = FB_SPM_DATE_MMDD;
	DSP_MEMORY(magic).dsp_soft_id  = FB_SPM_SOFT_ID;
	DSP_MEMORY(magic).statemachine = DSP_MEMORY_OFFSET(state);
	DSP_MEMORY(magic).AIC_in       = DSP_MEMORY_OFFSET(iobuf.min[0]);
	DSP_MEMORY(magic).AIC_out      = DSP_MEMORY_OFFSET(iobuf.mout[0]);
	DSP_MEMORY(magic).AIC_reg      = 0;
	DSP_MEMORY(magic).analog       = DSP_MEMORY_OFFSET(analog);
	DSP_MEMORY(magic).signal_monitor= DSP_MEMORY_OFFSET(sig_mon);
	DSP_MEMORY(magic).feedback_mixer = DSP_MEMORY_OFFSET(feedback_mixer);
	DSP_MEMORY(magic).z_servo      = DSP_MEMORY_OFFSET(z_servo);
	DSP_MEMORY(magic).m_servo      = DSP_MEMORY_OFFSET(m_servo);
	DSP_MEMORY(magic).scan         = DSP_MEMORY_OFFSET(scan);
	DSP_MEMORY(magic).move         = DSP_MEMORY_OFFSET(move);
	DSP_MEMORY(magic).probe        = DSP_MEMORY_OFFSET(probe);
	DSP_MEMORY(magic).autoapproach = DSP_MEMORY_OFFSET(autoapp);
	DSP_MEMORY(magic).datafifo     = DSP_MEMORY_OFFSET(datafifo);
	DSP_MEMORY(magic).probedatafifo= DSP_MEMORY_OFFSET(probe_datafifo);
	DSP_MEMORY(magic).scan_event_trigger= DSP_MEMORY_OFFSET(scan_event_trigger);
	DSP_MEMORY(magic).CR_out_puls  = DSP_MEMORY_OFFSET(CR_out_pulse);
	DSP_MEMORY(magic).CR_generic_io= DSP_MEMORY_OFFSET(CR_generic_io);
	DSP_MEMORY(magic).hr_mask      = DSP_MEMORY_OFFSET(sigma_delta_hr_mask);

#ifdef USE_PLL_API
 	DSP_MEMORY(magic).PLL_lookup   = DSP_MEMORY_OFFSET(PLL_lookup);
#else
 	DSP_MEMORY(magic).PLL_lookup   = 0x00000000;
#endif
	DSP_MEMORY(magic).signal_lookup = DSP_MEMORY_OFFSET(dsp_signal_lookup);
 	DSP_MEMORY(magic).sco1        = DSP_MEMORY_OFFSET(sco_s[0]);
 	DSP_MEMORY(magic).life        = 0; /* Byte Order Test Indicator LONG */
 	DSP_MEMORY(magic).dummyBE     = 0xEEEEDDDD; /* Byte Order Test Indicator LONG */


	DSP_MEMORY(magic).magic        = 0x00000001; /* DBG MAGIC STEP 1 */

	/* Acknowledge the PC */
}

#ifndef DSPEMU
#pragma CODE_SECTION(start_Analog810_HL, ".text:slow")
#endif

void start_Analog810_HL()
{

#ifndef DSPEMU
        // Call start_Analog810
        start_Analog810();
                
        // Acknowledge the PC
        HPI_HPIC = Acknowledge; 
#endif
}

#ifndef DSPEMU
#pragma CODE_SECTION(stop_Analog810_HL, ".text:slow")
#endif

void stop_Analog810_HL()
{
#ifndef DSPEMU
        // Call stop_Analog810
        stop_Analog810();
                
        // Acknowledge the PC
        HPI_HPIC = Acknowledge;         
#endif
}

/* ------------------------------------------------------------------------  *
 *                                                                          *
 *  from Alex @ SoftdB for DSP clock reconfigure                            *
 *                                                                          *
 *  _enablePll1( pll_mult ) *
 *                                                                          *
 *                                                                          *
 *  pll_mult        <- 27: 28x Multiplier * 24.576MHz Clk = 688.128 MHz     *
 *                                                                          *
 * ------------------------------------------------------------------------  */
#ifndef DSPEMU
#pragma CODE_SECTION(main, ".text:slow")
#endif

void enablePll1( Uint16 pll_mult )
{
        volatile Uint32* pll_ctl    = ( volatile Uint32* )&PLL1_PLLCTL;
        volatile Uint32* pll_pllm   = ( volatile Uint32* )&PLL1_PLLM;
        volatile Uint32* pll_cmd    = ( volatile Uint32* )&PLL1_PLLCMD;
        volatile Uint32* pll_stat   = ( volatile Uint32* )&PLL1_PLLSTAT;
        volatile Uint32* pll_div1   = ( volatile Uint32* )&PLL1_PLLDIV1;
        volatile Uint32* pll_div2   = ( volatile Uint32* )&PLL1_PLLDIV2;
        volatile Uint32* pll_div3   = ( volatile Uint32* )&PLL1_PLLDIV3;

        /*
         *  Step 1 - Set clock mode
         */

        *pll_ctl &= ~0x0100;    // Onchip Oscillator

        /*
         *  Step 2 - Set PLL to bypass
         *         - Wait for PLL to stabilize
         */

        *pll_ctl &= ~0x0021;

        wait( 2 );        // 4 MXI/CLKIN cycles (7 cycles since wait's delay is (delay*7))
        // 1 * 7 = 7 cycles at 24.576MHz : 285 ns
        // double to be safe : 2

        /*
         *  Step 3 - Reset PLL
         */

        *pll_ctl &= ~0x0008;

        /*
         *  Step 4 - Disable PLL
         *  Step 5 - Powerup PLL
         *  Step 6 - Enable PLL
         *  Step 7 - Wait for PLL to stabilize
         */

        *pll_ctl |= 0x0010;        // Disable PLL
        *pll_ctl &= ~0x0002;       // Power up PLL
        *pll_ctl &= ~0x0010;       // Enable PLL

        wait( 1054 );             // Wait for PLL to stabilize (from 6424 datasheet : 150 us : 3687 cycles at 24.576 MHz)
        // 527 * 7 = 3689 at 25.576MHz = 150.1 us
        // Double to be safe : 1054
        /*
         *  Step 8 - Load PLL multiplier
         */

        *pll_pllm = pll_mult ;

        /*
         *  Step 9 - Load PLL dividers ( must be in a 1/3/6 ratio )
         *           1:DSP, 2:SCR,EMDA, 3:Peripherals
         */

        while( ( *pll_stat & 1 ) != 0 );// Wait for phase alignment
        *pll_div1 = 0x8000 | 0;         // Divide-by-1
        *pll_div2 = 0x8000 | 2;         // Divide-by-3
        *pll_div3 = 0x8000 | 5;         // Divide-by-6
        *pll_cmd |= 0x0001;             // Set phase alignment

        wait( 18 );                        // (2x6) + 50 cycles overhead = 62 source clock cycles
        // 9 * 7 : 63 cycles at 24.576MHz = 2.563 us
        // Double to be safe : 18

        while( ( *pll_stat & 1 ) != 0 );// Wait for phase alignment

        /*
         *  Step 10 - Wait for PLL to reset
         *  Step 11 - Release from reset
         */

        wait( 38 );                    // Must be 128xclkin
        // 19 * 7 : 133 cycles at 24.576MHz = 5.41 us
        // Double to be safe : 38

        *pll_ctl |= 0x0008;

        /*
         *  Step 12 - Wait for PLL to re-lock
         *  Step 13 - Switch out of BYPASS mode
         */

        wait( 572 );                    // Must be 2000Xclkin
        // 286 * 7 : 2002 at 24.576MHz = 81.46 us
        // Double to be safe : 572

        *pll_ctl |= 0x0001;

}

#ifndef DSPEMU
#pragma CODE_SECTION(main, ".text:slow")
#endif

void dsp_688MHz(){ // turbo speed
	enablePll1( 27 );    // [DSP @ 24.576 * 28 :  688.128 MHz][Core @ 1.20V]

	// Adjust the Flash interface for a main clock at 688.128 MHz (sysclk3: 114.688 MHz)

	AEMIF_A1CR = 0
		| ( 0 << 31 )           // selectStrobe
		| ( 0 << 30 )           // extWait
		| ( 5 << 26 )           // writeSetup      //  52 ns
		| ( 6 << 20 )           // writeStrobe     //  61 ns
		| ( 1 << 17 )           // writeHold       //  17.4 ns
		| ( 6 << 13 )           // readSetup       //  61 ns
		| ( 6 << 7 )               // readStrobe      //  61 ns
		| ( 1 << 4 )            // readHold        //  17.4 ns
		| ( 2 << 2 )            // turnAround      //  26 ns
		| ( 1 << 0 )            // asyncSize       //  16-bit bus
		;
	
	AEMIF_NANDFCR &= 0x00000F0E;  // NAND Flash [ OFF ]
	
	FLASH_BASE_PTR8 = FLASH_RESET;
}

#ifndef DSPEMU
#pragma CODE_SECTION(main, ".text:slow")
#endif

void dsp_590MHz(){ // normal speed -- as set at boot
	enablePll1( 23 );       // [DSP @ 24.576 * 24 :  589.824MHz][Core @ 1.20V]

	AEMIF_A1CR = 0
		| ( 0 << 31 )           // selectStrobe
		| ( 0 << 30 )           // extWait
		| ( 4 << 26 )           // writeSetup      //  50 ns
		| ( 5 << 20 )           // writeStrobe     //  60 ns
		| ( 0 << 17 )           // writeHold       //  10 ns
		| ( 5 << 13 )           // readSetup       //  60 ns
		| ( 5 << 7 )               // readStrobe      //  60 ns
		| ( 0 << 4 )            // readHold        //  10 ns
		| ( 1 << 2 )            // turnAround      //  20 ns
		| ( 1 << 0 )            // asyncSize       //  16-bit bus
		;
	
	AEMIF_NANDFCR &= 0x00000F0E;  // NAND Flash [ OFF ]
	
	FLASH_BASE_PTR8 = FLASH_RESET;
}

#ifndef DSPEMU
#pragma CODE_SECTION(main, ".text:slow")
void main ()
#else
int main (int argc, char *argv[])
#endif
{
#ifdef DSPEMU
        map_dsp_memory ("sranger_mk3emu");
#endif
#ifdef USE_PLL_API
	/* setup PLL */
	DSP_MEMORY_PAC(pSignal1) = &DSP_MEMORY_PAC(InputFiltered);
	DSP_MEMORY_PAC(pSignal2) = &DSP_MEMORY_PAC(Filter64Out[0]);
	DSP_MEMORY_PAC(blcklen) = 2096-1;

	DSP_MEMORY(PLL_lookup).pac_lib_status = StartPLL(4,7);
#endif

	fill_magic ();
	setup_default_signal_configuration ();

	/* check for valid saved flash signal configuration and auto-restore */
	restore_configuration_from_flash ();

	/* Setup LockIn tabels */

	init_probe_module ();

	/* Init AIC registers : Defaut cfg (all gains = 0 db, except marked) */
	
        // all AIC: running at 25475.54 Hz, anti-aliasing filter ON-FIR, x128 oversampling
        //          VBias 1.35V, differential inputs, all input/output gains at 0dB

#ifndef DSPEMU

	/* Setup DSP timer/clock */
        //	TSCReadFirst();
        //	TSCReadSecond();
	TSCInit();

	/* Init Analog810 */

	FreqDiv   = a810_config.freq_div;     // [default: =10 75kHz *** 5 ... 65536  150kHz at N=5   Fs = Clk_in/(N * 200), Clk_in=150MHz] / 2!!!!
	ADCRange  = a810_config.adc_range;    // default: =0 0: +/-10V, 1:+/-5V
	QEP_ON    = a810_config.qep_on;       // default: =1 (on) manage QEP counters


	/* Configure DSP "over drive" if selected -- regular safe and recommended default is 590 MHz */
	// #define RECONFIGURE_DSP_688MHZ  -- see Makefile for configuring this option via CFLAGS -- NOT HERE.
#ifdef RECONFIGURE_DSP_688MHZ
	// ************************* Change to 688.128
	dsp_688MHz();
#endif
	
	start_Analog810 ();
        //	stop_Analog810 (); // Stop AICs now again -- testing only


	// (re) configure hardware to allow FLASH support for read/write/erase:
	// Disable INT6

	IER = IER & ~FB_INT6MASK;        //  Mask the INT6 bit of IER

#endif
        
	// Set the GPIO0 to obtain a falling trig for interrupt generation from flash
#ifndef DSPEMU
	GPIO_BINTEN=1;                // Enable interrupt for the bank 0 (GPIO 0 to 15)
	GPIO_CLR_RIS_TRIG01=1;        // Clear rise trig on GPIO0
	GPIO_SET_FAL_TRIG01=1;        // Set falling trig on GPIO0

	// Link the GPIO0 event on the INT6 (the INT4 is for the HPIINT (47:2Fh))
	// The GPIO0 is at 64:40h
	// EDMA3CC_INT0 event Region 0 (35:23h) on INT5

	INTC_INTMUX1=0x0740232F;
#endif


	/*
	 * Wait for DMA interrupt (see dataprocess.c for interrupt service routine)
	 * while running the never ending state maschine's idle and control loop.
	 */

	DSP_MEMORY(magic).magic        = FB_SPM_MAGIC; /* now valid! */

#ifndef DSPEMU

	HPI_HPIC = Acknowledge;
        
	TSCReadFirst();
#endif
        
	/* Start State Maschine now */
#ifndef DSPEMU
	dsp_idle_loop ();
#else
        return g_application_run (G_APPLICATION (dspemu_app_new ()), argc, argv);
#endif
}
