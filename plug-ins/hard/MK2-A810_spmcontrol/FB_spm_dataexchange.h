/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* SRanger MK2 and Gxsm - Gnome X Scanning Microscopy Project
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Important Notice:
 * ===================================================================
 * THIS FILES HAS TO BE IDENTICAL WITH THE CORESPONDING
 * SRANGER-DSP-SOFT SOURCE FILE (same name) AND/OR OTHER WAY ROUND!!
 * It's included in both, the SRanger and Gxsm-2.0 CVS tree for easier
 * independend package usage.
 * ===================================================================
 *
 * Copyright (C) 1999-2008 Percy Zahl
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

/* MK2 Note: all addresses are 32bit (LONG) */


#ifndef __FB_SPM_DATAEXCHANGE_H
#define __FB_SPM_DATAEXCHANGE_H

/* Special Features Config */

/** define DSP_OFFSET_ADDING if the XY-Offset should be added to the XY-Scan-Position, 
 * no XY-Offset output if defined! 
 */
#define DSP_OFFSET_ADDING

/** define DSP_PROBE_AIC_AVG if AIC_in_0..5 should be integrated and normalized while Vector probe (inbetween points)
 */
#define DSP_PROBE_AIC_AVG

/** define DSP_AS_AIC_AVG if AIC_in_0..5 & Zout should be integrated and normalized while area scan (inbetween points)
 */
#define DSP_AS_AIC_AVG

/** define DSP_PROBE_VECTOR_PROGRAM if the Vector table shall be programmable with loops, enable vector micro kernel
 */
#define DSP_PROBE_VECTOR_PROGRAM

/** define AIC_STOPMODE_ENABLE if the (maybe needed only for testing) AIC-Stop mode shall be availabe
 */
/* #define AIC_STOPMODE_ENABLE -- trouble if restart attempt!!! -- */

/** define DSP_Z0_SLOPE_COMPENSATION -- enable "analog" slope compensation via Z' = Z+Z0, Z0 = Xs*scan_fm_dzx + Ys*scan_fm_dzy
    THIS CONFIG=ON WILL DISABLE THE internal feedback slope pre-correction at the same time!!
 */
#define DSP_Z0_SLOPE_COMPENSATION


/** DSP-Soft Identification */

#ifdef FB_SPM_MAGIC_ADR
#undef FB_SPM_MAGIC_ADR
#endif
#define FB_SPM_MAGIC_ADR 0x5000    /**< Magic struct is at this fixed addess in external SRAM */
#define FB_SPM_MAGIC     0xEE01

/* Features Description */

#ifdef DSP_OFFSET_ADDING
# define FB_SPM_FEATURES_OFFSET_ADDING "enabled"
#else
# define FB_SPM_FEATURES_OFFSET_ADDING "disabled"
#endif

#ifdef DSP_PROBE_AIC_AVG
# define FB_SPM_FEATURES_AIC_INT "Yes"
#else
# define FB_SPM_FEATURES_AIC_INT "No"
#endif

#ifdef DSP_AS_AIC_AVG
# define FB_SPM_FEATURES_AIC_AS_INT "Yes"
#else
# define FB_SPM_FEATURES_AIC_AS_INT "No"
#endif

#ifdef DSP_PROBE_VECTOR_PROGRAM
# define FB_SPM_FEATURES_VECTOR_PRG "Yes"
#else
# define FB_SPM_FEATURES_VECTOR_PRG "No"
#endif

#ifdef DSP_Z0_SLOPE_COMPENSATION
# define FB_SPM_FEATURES_DSP_Z0_SLOPE_COMPENSATION "Yes"
#else
# define FB_SPM_FEATURES_DSP_Z0_SLOPE_COMPENSATION "No"
#endif

/** Define to enable Watch Vars -- dbg purpose only */
#define WATCH_ENABLE


// these are checked/compared with the Gxsm-build and must match!
// -- otherwise you exactly need to know/be sure what you are doing --
// -- odd things like changed data structures, etc.., could break data transfer --
#define FB_SPM_SOFT_ID   0x1001 /* FB_SPM sofware id */
#define FB_SPM_VERSION   0x2041 /* FB_SPM main Version, BCD: 00.00 */
#define FB_SPM_DATE_YEAR 0x2020 /* Date: Year, BCD */
#define FB_SPM_DATE_MMDD 0x1123 /* Date: Month/Day, BCD */

#define FB_SPM_FEATURES     \
	"Version: Crash Hack RTEngine4Gxsm Mark2 2019\n"\
	"4 Source Feedback Mix: Yes\nFB-EXT: Option\nFB-watch\nZ-Setpoint/Fuzzy Control\n"\
	"dynamic current IIR: Yes, IIR/RMS comp. always if set, MIX_MODE negate: Yes, FUZZY/LOG: yes\n"\
	"SCAN: Yes, Pause/Resume: Yes, FastRet: Yes, 2nd-Zoff-scan AKTIVE, Z[16], AIC0-7[16], dIdV/ddIdV/LckI0/Count[32]! AIC_INT: " FB_SPM_FEATURES_AIC_AS_INT "\n" \
	"Z0-SLOPE-COMPENSATION: " FB_SPM_FEATURES_DSP_Z0_SLOPE_COMPENSATION "\n" \
	"Scan and Offset: vector moves\n Z0-Plan-Off: Yes\n" \
	"MOVER: GENERIC MULTI CHANNEL WAVE PLAY, AAP GPIO pulse: Yes and general GPIO\n"\
	"VPROBE: Yes, AIC0-7\nVPROBE-AICdnxINT: " FB_SPM_FEATURES_AIC_INT "\nACPROBE: Yes\nACPROBE2ND: Yes\nACphiQ19\n VP-TRACK: Yes\n VP_LIM: Yes\n"\
	"VPROBE-Program-Loops: " FB_SPM_FEATURES_VECTOR_PRG " with TRK, LIM feature\n"\
	"Universal Recorder16/32: Yes, GPIO: Yes\n"\
	"DSP-level XY-Offset-Adding:" FB_SPM_FEATURES_OFFSET_ADDING "\n"\
	"Diff Option AIC5-AIC7 -> AIC5: Yes\n"\
	"AICout0/1/2/6 Offset Adjust: Yes\n"\
	"HR MODE FOR ALL XYZ Offset and Scan: Yes, LDC option in offset move function: Yes\n"\
	"THIS IS SR-MK2 with Analog810 8Ch 16bity in/out + GPIO + 2 Counters\nAnalog810 ReConfig+-Start: Yes\nHR-mode: Yes RNG: Yes\n"\
	"Non blocking user KernelRead/Write support: Yes -- at ByteAddress 00000800 ReadMemNA, ReadProgNA, WriteMemNA, WriteProgNA\n"

/*
 * only used for initialisation, no floats needed on DSP, 
 * only the cc should compute this! 
 */

#define U_DA_MAX 10.0
#define U_AD_MAX 10.0
#define VOLT2DAC(U) (DSP_INT)((U)*32767./U_DA_MAX)
#define DAC2VOLT(R) (DSP_INT)((R)*U_AD_MAX/32767.)


/** Full Frame Area Scan Control Structure */
// start modes
#define AREA_SCAN_RUN            0x01
#define AREA_SCAN_RUN_FAST       0x02
#define AREA_SCAN_MOVE_TIP       0x10
// stop modes
#define AREA_SCAN_STOP           0x01
#define AREA_SCAN_PAUSE          0x02
#define AREA_SCAN_RESUME         0x04
// special control modes
#ifdef DSP_CC
# define APPLY_NEW_ROTATION           0x1378 /* priavte start commands for applying data to private copy */
#else
# define MK2_APPLY_NEW_ROTATION           0x1378 /* priavte start commands for applying data to private copy */
#endif

#define PROBE_NO_LOCKIN         0
#define PROBE_INIT_LOCKIN       1
#define PROBE_RUN_LOCKIN_PROBE  2
#define PROBE_RUN_LOCKIN_SCAN   3
#define PROBE_RUN_LOCKIN_FREE   4

/**
 * Here are all structs and constants needed for data exchange and
 * control from user frontend (Gxsm)
 * ===============================================================
 *
 * Read/Write permissions to be followed by Host are indicated by:
 * =RO -> Read Only
 * =WO -> Write Only (Read OK, but useless)
 * =RW -> Read and Write allowed
 *
 * The similar named macro "MAX_WRITE_structname" defines the max
 * allowed write length in DSP words. All host writeabe members are
 * located at the beginning of the struct.
 * 
 * Note:
 * DSP_(U)LONG type needs to be LONG-word aligned!!
 */

/**
 * definitions used for statemaschine controls 
 * ==================================================
 */

#define MD_IDLE         0x0000  /**< all zero means ready, no idle task (scan,move,probe,approch,...) is running */
#define MD_HR           0x0001  /**< High Res mode enable (Sigma-Delta resolution enhancement via bit0 for Z) */
#define MD_BLK          0x0002  /**< Blinking indicator - DSPs heartbeat */
#define MD_PID          0x0004  /**< PID-Regler activ */
#define MD_XXA          0x0008  /**< testing - A flag - normally 0 */
#define MD_OFFSETCOMP   0x0010  /**< Offset Compensation on */
#define MD_ZTRAN        0x0020  /**< Transform Z(AIC[7]) * (-1) -> AIC[6] */
#define MD_NOISE        0x0040  /**< add noise to Bias */
#define MD_OFFSETADDING 0x0080  /**< DSP-level Offset Adding */
#define MD_EXTFB        0x0100  /**< Switching external feedback control ON/OFF */
#define MD_XXB          0x0200  /**< testing - B flag - normally 0 */
#define MD_WATCH        0x0400  /**< Watching variables on OUT7 */
#define MD_DIFFIN0M4    0x0800  /**< Software differential inputs: IN0-IN4 -> IN0 */
#define MD_2_NOTUSED    0x1000  /**< reserved for future use */
#define MD_AIC_STOP     0x2000  /**< AICs can be placed in stop (no conversion will take place, dataprocess runs in dummy mode */
#define MD_AIC_RECONFIG 0x4000  /**< Request AIC Reconfig */
#define MD_3_NOTUSED    0x8000  /**< reserved for future use */



// use DSP_CC for DSP CC, not if used for GXSM SRanger_HwI include file!
// ==> now defined at compile time via Makefile CC option -DDSP_CC
// #define DSP_CC

#ifdef DSP_CC
typedef unsigned char DSP_UINT8;
typedef short DSP_INT;
typedef int   DSP_IINT;
typedef unsigned short DSP_UINT;
typedef long DSP_LONG;
typedef unsigned long DSP_ULONG;
typedef short           DSP_INT16; // data - MK2 native is "int", same as "short"
typedef unsigned short  DSP_UINT16;
typedef long            DSP_INT32;
typedef unsigned long   DSP_UINT32;
#else
typedef gint16  DSP_INT;
typedef guint16 DSP_IINT;
typedef guint16 DSP_UINT;
typedef gint32  DSP_LONG;
typedef guint32 DSP_ULONG;
typedef gint16   DSP_INT16;
typedef guint16  DSP_UINT16;
typedef gint32   DSP_INT32;
typedef guint32  DSP_UINT32;
#endif

#ifdef DSP_CC
# define PROBE_VECTOR_P  PROBE_VECTOR*  
# define DSP_INT_P       DSP_INT*
# define DSP_LONG_P      DSP_LONG*
# define DSP_INT16_P     DSP_INT16*
# define DSP_INT32_P     DSP_INT32*
#else
# define PROBE_VECTOR_P  DSP_UINT
# define DSP_INT_P       DSP_UINT
# define DSP_LONG_P      DSP_UINT
//# define DSP_INT16_P     DSP_UINT
//# define DSP_INT32_P     DSP_UINT
#endif


/**
 * Main DSP Statemachine Control Structure
 */

/** Real Time Task Control for Data-Processing (DP) Tasks **/
typedef struct {
        DSP_UINT   process_flag;
        DSP_UINT   missed_count; // RT-taks: missed count, IDeal-task: N executed count
        DSP_UINT32 process_time;
        DSP_UINT32 process_time_peak_now;
} DSPMK2_DP_TASK_CONTROL;

/** Idle Task Control for other less time criticl Idle (ID) Tasks **/
typedef struct {
        DSP_UINT   process_flag;
        DSP_UINT   exec_count; // RT-taks: missed count, IDeal-task: N executed count
        DSP_UINT32 timer_next;
        DSP_UINT32 interval;
} DSPMK2_ID_TASK_CONTROL;

#define NUM_DATA_PROCESSING_TASKS 12
#define NUM_IDLE_TASKS 32 // MUST BE 2^N

/**
 * DSP Magics Structure -- (C) 2008 by P.Zahl
 * MK2 all addresses have to be 32bit
 * Magics 0..4 -- all 32bit now, keeping order compatible.
 */
typedef struct {
	DSP_UINT magic;         /**< 0: Magic Number to validate entry =RO */
	DSP_UINT version;       /**< 1: SPM soft verion, BCD =RO */
	DSP_UINT year,mmdd;     /**< 2,3: year, date, BCD =RO */
	DSP_UINT dsp_soft_id;   /**< 4: Unique SRanger DSP software ID =RO */
	/* -------------------- Magic Symbol Table ------------------------------ */
	DSP_UINT statemachine;  /**< 5: Address of statemachine struct =RO */
	DSP_UINT AIC_in;        /**< 6: AIC in buffer =RO */
	DSP_UINT AIC_out;       /**< 7: AIC out buffer =RO */
	DSP_UINT AIC_reg;       /**< 8: AIC register (AIC control struct) =RO */
	/* -------------------- basic magic data set ends ----------------------- */
	DSP_UINT feedback;      /**<  9: Address of feedback struct =RO */
	DSP_UINT analog;        /**< 10: Address of analog struct =RO */
	DSP_UINT scan;          /**< 11: Address of ascan struct =RO */
	DSP_UINT move;          /**< 12: Address of move struct =RO */
	DSP_UINT probe;         /**< 13: Address of probe struct =RO */
	DSP_UINT autoapproach;  /**< 14: Address of autoapproch/wavegen struct =RO */
	DSP_UINT datafifo;      /**< 15: Address of datafifo struct =RO */
	DSP_UINT probedatafifo; /**< 16: Address of probe datafifo struct =RO */
	DSP_UINT data_sync_io;  /**< 17: Address of data_sync_io struct =RO */
	DSP_UINT feedback_mixer; /**< 18: Address of DFM FUZZY mix control parameter struct =RO */
	DSP_UINT CR_out_puls;   /**< 19: Address of CoolRunner IO Out Puls action struct =RO */
	DSP_UINT external;      /**< 20: Address of external control struct =RO */
	DSP_UINT CR_generic_io; /**< 21: Address of CR_generic_io struct =RO */
	DSP_UINT a810_config;   /**< 22: Address of a810_config struct =RO */
	DSP_UINT watch;         /**< 23: Address of watch struct if available =RO */
        DSP_UINT hr_mask;       /**< 24: Address of sigma_delta_hr_mask */
        DSP_UINT recorder;      /**< 25: Address of recorder -- used by PACscope tool */
	DSP_UINT dummy1, dummy2; /**< --: Address of xxx struct =RO */
	DSP_ULONG dummyl;         /**< --: Address of xxx struct =RO */
} SPM_MAGIC_DATA_LOCATIONS;
#define MAX_WRITE_SPM_MAGIC_DATA_LOCATIONS 0

/**
 * Main DSP Statemachine Control Structure
 */

typedef struct {
	DSP_UINT set_mode;      /**< mode change request: set bits =WO */
	DSP_UINT clr_mode;      /**< mode change request: clear bits =WO */
	DSP_UINT mode;          /**< current state =RO */
	DSP_UINT last_mode;     /**< last state =RO */

	DSP_UINT   DSP_seconds;   /**< 14 60 second down counter =RO */
	DSP_UINT   DSP_minutes;   /**< 10 1min counter  =RO */
        DSP_UINT32 DSP_count_seconds;    /**< 8  1s counter =RO */
	DSP_UINT32 DSP_time;      /**< 12 DSP time 150kHz ticks =RO */

	DSP_UINT32 DataProcessTime;/**< 16 time spend in dataprocess -- performance indicator =RO */
	DSP_UINT32 DataProcessReentryTime; /**< 18time spend not in dataprocess -- performance indicator =RO */
	DSP_UINT32 DataProcessReentryPeak; /**< 20 time spend in dataprocess, peak -- performance indicator =RO */
	DSP_UINT32 IdleTime_Peak;  /**< 22 time spend not in dataprocess, peak -- performance indicator =RO */

        DSPMK2_DP_TASK_CONTROL dp_task_control[NUM_DATA_PROCESSING_TASKS+1];
        DSPMK2_ID_TASK_CONTROL id_task_control[NUM_IDLE_TASKS];
        DSP_UINT32 DP_max_time_until_abort; 
} SPM_STATEMACHINE;
#define MAX_WRITE_SPM_STATEMACHINE 2

/** 
 * definitions used for Analog810 Reconfiguration
 * ==================================================
 */

typedef struct {
	unsigned short freq_div;  // = 10;  *** 75kHz *** 5 ... 65536  150kHz at N=5   Fs = Clk_in/(N * 200), Clk_in=150MHz
        unsigned short adc_range; // = 0;   *** 0: +/-10V, 1:+/-5V
        unsigned short qep_on;    // = 0x0001; *** enable managing QEP counters - disabling will free DSP io bandwidth
} A810_CONFIG;
#define MAX_WRITE_A810_CONFIG 3

/** 
 * definitions used for feedback control
 * ==================================================
 */

/** Feedback System Data/Control Block --- ACHTUNG!!! do not change without adjusting feedback*.asm code as well!!! */

typedef struct {
	DSP_INT cp;           /* 0  0*< Const Proportional in Q15 rep =RW */
	DSP_INT ci;           /* 1  1*< Const Integral in Q15 rep =RW */
	DSP_INT setpoint;     /* 2  2*< real world setpoint, if loaded gain, spm_logn will be calculated and "soll" is updated =WO -- obsolete now, use mixer!! */
	DSP_INT ca_q15;       /* 3  3*< IIR filter adapt: ca    = q_sat_limit cut-off (0:=FBW or f0_max) [Q15] */
	DSP_LONG cb_Ic;       /* 4  4*< IIR filter adapt: cb_Ic = f_0,min/f_0,max * I_cross [32] */
	DSP_INT I_cross;      /* 5  6*< IIR filter adapt: I_crossover */
	DSP_INT I_offset;     /* 6  7*< I_offset, log start/characteristic */
	DSP_INT q_factor15;   /* 7  8*< IIR filter: q --  (In=1/(1+q) (I+q*In)) in Q15 */
	DSP_INT soll;         /* 8  9*< "Soll Wert" Set Point (behind log trafo) =RO */
	DSP_INT ist;          /* 9 10*< "Ist Wert" Real System Value (behind log trafo) =RO */
	DSP_INT delta;        /*10 11*< "Regelabweichung" ist-soll =RO */
	DSP_LONG i_sum;       /*11 12*< xxx obolete -- CI summing only, mirror, 32bits! =RO */
	DSP_LONG I_iir;       /*12 14*< Current I smooth (IIR response)  =RO */
	DSP_INT z;            /*13 16*< "Stellwert" Feedback Control Value "Z" =RO */
	DSP_INT I_fbw;        /*14 17*< I full Band width */
	DSP_INT zxx;          /*15 18*< spacer */
	DSP_INT zyy;          /*16 19*< spacer */
	DSP_LONG I_avg;       /*17 20*< not normalized, 256x */
	DSP_LONG I_rms;       /*18 22*< not normalized, 256x */
	DSP_LONG z32neg;      /*19 24*< Z lower 16 "resolution" bits =RO */
	DSP_LONG z32;         /*20 26*< "Stellwert" Feedback Control Value "Z" in 32 bit full resolution =RO */
	DSP_INT watch;        /*21 28*< watch FB activity */
} SPM_PI_FEEDBACK;
#define MAX_WRITE_SPM_PI_FEEDBACK 9

/** 
 * definitions used for external control
 * ==================================================
 */

typedef struct {
	DSP_INT FB_control_channel;  /**< channel for external feedback control (-1 no ext FB control) =RW */
	DSP_INT FB_control_treshold; /**< treshold for external FB control =RW */ 
	DSP_INT watch_value;         /**< pass a variable out on channel 7 =RW */
	DSP_INT watch_min;           /**< minimum output =RW */
	DSP_INT watch_max;           /**< maximum output =RW */
	DSP_INT fb2_cp;              /**< feedback 2nd CP =RW */
	DSP_INT fb2_ci;              /**< feedback 2nd CI =RW */
	DSP_INT fb2_cd;              /**< feedback 2nd CD =RW */
	DSP_INT fb2_set;             /**< feedback 2nd setpoint =RW */
	DSP_INT fb2_bw;              /**< feedback 2nd bandwidth =RW */
	DSP_INT fb2_ref_ch;          /**< feedback 2nd reference (ref signal input) coding [0 = off] =RW */
	DSP_INT fb2_out_ch;          /**< feedback 2nd output channel =RW */
	DSP_INT fb2_delta;           /**< feedback 2nd delta =RW */
	DSP_INT fb2_delta2;          /**< feedback 2nd delta2 =RW */
	DSP_LONG fb2_i_sum;           /**< feedback 2nd intergrator =RW */
        DSP_INT output_M;            /** mapped to OUT(7) "Motor" */
} EXT_CONTROL;
#define MAX_WRITE_EXT_CONTROL 12


/** 
 * definitions used for FUZZY DFM signal mixing
 * ==================================================
 */

typedef struct {
	DSP_INT level[4]; /**< 0 if FUZZY MODE: Level for FUZZY interaction =RW */
	DSP_INT gain[4];  /**< 4 Gain for signal in Q15 rep =RW */
	DSP_INT mode[4];  /**< 8 mixer mode: Bit0: 0=OFF, 1=ON (Lin mode), Bit1: 1=Log/0=Lin, Bit2: reserved (IIR/FULL), Bit3: 1=FUZZY mode, Bit4: negate input value */
	DSP_INT setpoint[4]; /**< 12 individual setpoint for every signal */
        DSP_INT Z_setpoint; /** 16 Z setpoint for fuzzy Z control */
	DSP_INT exec;     /**< 17 log computation of x -> lnx =WO */
	DSP_INT x, lnx;   /** < 18,19 individual setpoint for every signal RW */
	DSP_INT y;        /** < 20 */
        DSP_INT pad;
	DSP_LONG delta;   /** < 22 */
	DSP_INT setpoint_log[4]; /**< 24 individual setpoint after log for every signal */
} FEEDBACK_MIXER;
#define MAX_WRITE_FEEDBACK_MIXER 17

/** 
 * definitions used for Move Offset and Scan control
 * ==================================================
 */

/** MoveTo X/Y: Offset Coordinates, Control Structure */
#define MODE_OFFSET_MOVE         1  /** < normal offset move */
#define MODE_LDC_MOVE            2  /** < Linear Drift Correction move */
#define MODE_ZOFFSET_MOVE        4  /** < Adjust Z offset */
#define MODE_ZOFFSET_AUTO_ADJUST 8  /** < Auto Adjust Z offset */

/* generic X/Y/Z vector indices */
#define i_X 0
#define i_Y 1
#define i_Z 2

typedef struct{
	DSP_INT     start;           /**< Initiate Offset Move -- see MODE_[OFFSET | LDC]_MOVE =WO */
	DSP_INT	    nRegel;	     /**< Anzahl Reglerdurchlaeufe je Schritt =WR */
	DSP_LONG    xy_new_vec[2];   /**< new/final offset coordinates =WR */
	DSP_LONG    f_d_xyz_vec[3];  /**< dx, dy of line to move along OR in LDC mode: single LDC step every num_steps cycle  =WR */
	DSP_LONG    num_steps;       /**< num steps needed to reach final point and counter OR in LDC mode: slow down factor  =WR */
	DSP_LONG    xyz_vec[3];      /**< current offset coordinates =RO */
	DSP_INT     pflg;            /**< process active flag -- set to 1 or 2 as copy of start mode via statemachine -- see MODE_[OFFSET | LDC]_MOVE =RO */
} MOVE_OFFSET;
#define MAX_WRITE_MOVE 14


/** Full Frame Area Scan Control Structure */

typedef struct{
	DSP_INT  start;             /**< 0,  #0 Initiate Area Scan =WO -- 1 normal start scan, > 1 start slow down scan with factor = start */
	DSP_INT  stop;              /**< 1,  #1 Cancel (=1) or Pause (=2) or Resume (=4) Area Scan =WO */
	DSP_LONG rotm[4];           /**< 2,  #2..5 scan rotation matrix Q31 xx,xy,yx,yy */
	DSP_INT	 nx_pre;            /**< 10, #6 number of pre-scan dummy points =WR */
	DSP_INT	 dnx_probe;         /**< 11, #7 number of positions inbetween probe points, -1==noprobe =WR */
	DSP_INT	 raster_a, raster_b;/**< 12, #8  alternate probe raster scheme -1==noprobe =WR */
	DSP_LONG srcs_xp, srcs_xm;  /**<14,  #10 source channel configuration =WR */
	DSP_LONG srcs_2nd_xp, srcs_2nd_xm;  /**<18  #12 source channel configuration =WR */
	DSP_INT	 nx, ny;            /**<22  #14 number of points to scan in x =WR */
	DSP_LONG fs_dx, fs_dy;      /**<24  #16 32bit stepsize "scan", High Word -> DAC, Low Word -> "decimals" */
	DSP_LONG num_steps_move_xy; /**<28  #18   number of move steps for vector (fm_dx,fm_dy) xy move */
	DSP_LONG fm_dx, fm_dy, fm_dzxy; /**<30 #19,20,21 32bit stepsize "move" and slope in xy vec dir, High Word -> DAC, Low Word -> "decimals" */
	DSP_INT	 dnx, dny;          /**<36  #22 delta "nx": number of "DAC positions" inbetween "data points" =WR */
	DSP_INT  Zoff_2nd_xp, Zoff_2nd_xm; /**<38 #24 Zoffset for 2nd scan in EFM/MFM/... quasi const height mode */
	DSP_LONG fm_dz0_xy_vec[2];  /**<40 #26 X/Y-Z0-slopes in X and Y in main XY coordinate system =WR */
	DSP_LONG z_slope_max;       /**<44 #28 fast return option */
	DSP_INT  fast_return, fast_return_2nd;/**<46 #29,30 fast return option */
	DSP_INT  slow_down_factor, slow_down_factor_2nd; /**< 48 #31,32  slow down options */
	DSP_INT  bias_section[4];    /** 50-53 #33,34,35,36 */
	DSP_LONG xyz_gain;          /**< 54,55 #37 bitcoded 8-/8z/8y/8x (0..255)x */
	DSP_LONG pad;               /**< 56,57 #38 bitcoded 8-/8z/8y/8x (0..255)x */
	DSP_LONG xyz_vec[3];        /**<58  #39,40,41 current X/Yposition, 32bit =RO, scan coord sys */
	DSP_LONG xy_r_vec[2];        /**<  #42 current X/Yposition, 32bit =RO, rotated final/offset coord sys */
	DSP_LONG cfs_dx, cfs_dy;    /**< copy of 32bit stepsize "scan", High Word -> DAC, Low Word -> "decimals" */
	DSP_INT  iix, iiy, ix, iy;  /**< current inter x/y, and x/y counters =RO */
	DSP_INT  iiix;              /**< current scan speed reduction beyond 2^15 dnx */
	DSP_INT  ifr;               /**< fast return count option */
	DSP_INT  sstate;            /**< current scan state =RO */
	DSP_INT  section;           /**< current scan state =RO */
	DSP_INT  pflg;              /**< process active flag =RO */
} AREA_SCAN;
#define MAX_WRITE_SCAN 56

/** Data Sync Control */

typedef struct{
	DSP_INT xyit[4];
	DSP_INT last_xyit[4];
        DSP_IINT gpiow_bits; // tmp data, mixed with generic gpio data
        DSP_IINT gpior_bits; // dummy
        DSP_INT config;
	DSP_INT tick;
	DSP_INT pflg;
} DATA_SYNC_IO;
#define MAX_WRITE_DATA_SYNC_IO (0)

/** Vector Probe Control structure and Probe actions definition vector, one Element of a Vector Program:
 *
 * The vector list is a Vector Program (VP), i.e. we have a Vector Program Counter (VPC) to control flow.
 *
 * The PROBE structure two below holds the VPC (probe.vector), i.e. the current Vector.
 * Program Entry is probe.vector_head, i.e first Vector instruction at probe start.
 * Loop branches (ptr_next) are relative to VPC, i.e. ptr_next = -1 to repeat last Vector.
 * Normal vector program flow is linear, ++VPC, as long as ptr_final != 0, ptr_final should be 1 for normal proceeding.
 */

/**
 * Vector Probe Vector
 */
typedef struct{
	DSP_LONG    n;             /**< 0: number of steps to do =WR */
	DSP_LONG    dnx;           /**< 2: distance of samples in steps =WR */
	DSP_LONG    srcs;          /**< 4: SRCS source channel coding =WR */
	DSP_LONG    options;       /**< 6: Options, Dig IO, ... not yet all defined =WR */
	DSP_UINT    ptr_fb;        /**< 8: optional pointer to new feedback data struct first 3 values of SPM_PI_FEEDBACK =WR */
	DSP_UINT    repetitions;   /**< 9: numer of repetitions =WR */
	DSP_UINT    i,j;           /**<10,11: loop counter(s) =RO/Zero */
	DSP_INT     ptr_next;      /**<12: next vector (relative to VPC) until --rep_index > 0 and ptr_next != 0 =WR */
        DSP_INT     ptr_final;     /**<13: next vector (relative to VPC), =1 for next Vector in VP, if 0, probe is done =WR */
	DSP_LONG    f_du;          /**<14: U (bias) stepwidth (32bit) =WR */
	DSP_LONG    f_dx;          /**<16: X stepwidth (32bit) =WR */
	DSP_LONG    f_dy;          /**<18: Y stepwidth (32bit) =WR */
	DSP_LONG    f_dz;          /**<20: Z stepwidth (32bit) =WR */
	DSP_LONG    f_dx0;         /**<22: X0 (offset) stepwidth (32bit) =WR */
	DSP_LONG    f_dy0;         /**<24: Y0 (offset) stepwidth (32bit) =WR */
	DSP_LONG    f_dphi;        /**<26: Phase stepwidth (32bit) +/-15.16Degree =WR */
} PROBE_VECTOR;
#define SIZE_OF_PROBE_VECTOR 28

/** VP option masks */
#define VP_FEEDBACK_HOLD 0x01 // Feedback Hold Flag
#define VP_AIC_INTEGRATE 0x02 // ADC integrate intermediate values
#define VP_TRACK_REF     0x04 // Get Track Ref Value
#define VP_TRACK_UP      0x08 // Track on gradient up (raw signal)
#define VP_TRACK_DN      0x10 // Track on gradient down (raw signal)
#define VP_TRACK_FIN     0x20 // Track Fin (make decision, finish cycle), repeat as set
#define VP_TRACK_SRC     0xC0 // Track "Value" source code bit mask 0x40+0x80  00: Z (OUT5), 01: I (IN0), 10: (IN1), 11: (IN2)
#define VP_LIMITER       0x300 // Limiter ON/OFF flag mask
#define VP_LIMITER_UP    0x100 // Limit if > value
#define VP_LIMITER_DN    0x200 // Limit if < value
#define VP_LIMITER_MODE  0xC0 // Limiter mode code bit mask 0x40+0x80  00: hold, 01: abort with vector forward branch (postive) PCJR
#define VP_LIMIT_SRC     0xC0 // Limiter "Value" source code bit mask 0x40+0x80  00: Z (IN0), 01: I (IN1), 10: (IN2), 11: (IN3)
#ifdef VP_GPIO_MSK
#undef VP_GPIO_MSK
#endif
#define VP_GPIO_MSK      0xff0000
#define VP_NODATA_RESERVED 0x80000000

#define VP_TRIGGER_P       0x01000000 // GPIO/signal trigger flag on pos edge -- release VP on "data & mask" or time end/out of section 
#define VP_TRIGGER_N       0x02000000 // GPIO/signal trigger flag on neg edge -- release VP on "data & mask" or time end/out of section 
#define VP_GPIO_SET        0x04000000 // GPIO set/update data -- once per section via statemachine, using idle cycle time for slow IO!!
#define VP_GPIO_READ       0x08000000 // GPIO set/update data -- once per section via statemachine, using idle cycle time for slow IO!!
#define VP_RESET_COUNTER_0 0x10000000
#define VP_RESET_COUNTER_1 0x20000000
#define VP_NODATA_RESERVED 0x80000000

#define PROBE_NO_LOCKIN         0
#define PROBE_INIT_LOCKIN       1
#define PROBE_RUN_LOCKIN_PROBE  2
#define PROBE_RUN_LOCKIN_SCAN   3
#define PROBE_RUN_LOCKIN_FREE   4

/**
 * Vector Probe control structure
 */
typedef struct{
	DSP_INT     start;           /**<0: Initiate Probe =WO */
	DSP_INT     stop;            /**<1: Cancel Probe =WO */
	DSP_INT     LIM_up, LIM_dn;  /**<2,3: limiter value upper, lower =WR */
	DSP_INT     AC_amp;          /**<4: digi LockIn: Amplitude -- digi LockIn used if amp>0 =WR */
	DSP_INT     AC_frq;          /**<5: digi LockIn: Frq. =WR */
	DSP_INT     AC_phaseA;       /**<6: digi LockIn: Phase A =WR */
	DSP_INT     AC_phaseB;       /**<7: digi LockIn: Phase B =WR */
	DSP_INT     AC_nAve;         /**<8: digi LockIn: #Averages =WR */
	DSP_INT     AC_ix;           /**<9: digi LockIn: index =RO/Zero */
	DSP_LONG    time;            /**<10: probe time in loops =RO */
	DSP_LONG    Upos;    	     /**<12: current "X" (probe) value =RO */
	DSP_LONG    Zpos;    	     /**<14: current "X" (probe) value =WR */
	DSP_LONG    LockIn_0;  	     /**<16: last LockIn 0 order result (avg. src) =RO */
	DSP_LONG    LockIn_1stA;     /**<18: last LockIn 1st order result =RO */
	DSP_LONG    LockIn_1stB;     /**<20: last LockIn 1st order result =RO */
	DSP_LONG    LockIn_2ndA;     /**<22: last LockIn 2nd order result =RO */
	DSP_LONG    LockIn_2ndB;     /**<24: last LockIn 2nd order result =RO */
	DSP_UINT    LockInRefSinTabA; /**< Address of phase shifted sin reference table =RW */
	DSP_INT     LockInRefSinLenA; /**< Length of RefTab =RO */
	DSP_UINT    LockInRefSinTabB; /**< Address of phase shifted sin reference table =RW */
	DSP_INT     LockInRefSinLenB; /**< Length of RefTab =RO */
	PROBE_VECTOR_P vector_head;  /**< pointer to head of list of probe vectors =RO/SET */
	PROBE_VECTOR_P vector;       /**< pointer to head of list of probe vectors =RO/SET */
	DSP_LONG    ix, iix;         /**< counters =RO */
	DSP_INT     lix, dum;        /**< flags for limiter  =RO */
	DSP_INT     state;           /**< current probe state =RO */
        DSP_INT     gpio_setting;    /**< GPIO data last set */
	DSP_INT     pflg;            /**< process active flag =RO */
	DSP_INT     pflg_lockin;     /**< process active flag =RO */
} PROBE;
#define MAX_WRITE_PROBE 14


/** Auto Approach and Slider/Mover Parameters */
typedef struct{
	DSP_INT     start;           /**<0 Initiate =WO */
	DSP_INT     stop;            /**<1 Cancel   =WO */
	DSP_INT     mover_mode;      /**<2 Mover mode, see below */
	DSP_INT     max_wave_cycles; /**<3 max number of repetitions */
	DSP_INT     wave_length;     /**<4 total number of samples per wave cycle for all channels (must be multipe of n_waves_channels)  */
	DSP_INT     n_wave_channels; /**<5 number of wave form channel to play simulatneously (1 to max 6)*/
	DSP_INT     wave_speed;      /**<6 number of samples per wave cycle */
	DSP_INT     n_wait;          /**<7 delay inbetween cycels */
	DSP_INT     channel_mapping[6]; /** 8,9,10,11,12,13 -- may include the flag "ch | AAP_MOVER_SIGNAL_ADD"  */
	DSP_INT     axis;            /**<14 axis id (0,1,2) for optional step count */
	DSP_INT     dir;             /**<15 direction for count +/-1 */
	DSP_INT     ci_retract;      /**<16 retract CI (inverted normal, may be bigger) */
	DSP_INT     dum;             /**<17 retract CI (inverted normal, may be bigger) */
	DSP_ULONG   n_wait_fin;      /**<18 # cycles to wait and check (run FB) before finish auto app. */
	DSP_ULONG   fin_cnt;         /**< tmp count for auto fin. */
	DSP_INT     mv_count;        /**< "time axis" */
	DSP_INT     mv_step_count;   /**< step counter */
	DSP_INT     tip_mode;        /**< Tip mode, used by auto approach */
	DSP_INT     delay_cnt;       /**< Delay count */
	DSP_INT     cp, ci;          /**< temporary used */
	DSP_INT     count_axis[3];   /**< axis step counter */
	DSP_INT     pflg;            /**< process active flag =RO */
} AUTOAPPROACH;
#define MAX_WRITE_AUTOAPPROACH 17

/** MOVER MODES **/
#define AAP_MOVER_STOP          0 /**< cancel wave play/auto -- all off */
#define AAP_MOVER_AUTO_APP      1 /**< set this flag to run wave play in auto approach mode */
#define AAP_MOVER_WAVE_PLAY     2 /**< run wave play */
#define AAP_MOVER_PULSE         4 /**< GPIO - -experimental */
#define AAP_MOVER_SIGNAL_ADD    0x100 /**< wave play, channel falg indicating "add to channel" mode */

/** CR_out_puls */
typedef struct{
	DSP_INT     start;           /**< Initiate =WO */
	DSP_INT     stop;            /**< Cancel   =WO */
	DSP_INT     duration;        /**< Puls duration count =WR */
	DSP_INT     period;          /**< Puls period count =WR */
	DSP_INT     number;          /**< number of pulses =WR */
	DSP_IINT    on_bcode;        /**< IO value at "on", while "duration" =WR */
	DSP_IINT    off_bcode;       /**< IO value at "off", while remaining time of period =WR */
	DSP_IINT    reset_bcode;     /**< IO value at end/cancel =WR */
	DSP_INT     io_address;      /**< IO address to use =WR */
	DSP_INT     i_per;           /**< on counter =RO */
	DSP_INT     i_rep;           /**< repeat counter =RO */
	DSP_INT     pflg;            /**< process active flag =RO */
} CR_OUT_PULSE;
#define MAX_WRITE_CR_OUT_PULS 8

/**
 * CR_generic_io (GPIO) and Counter (Rate-Meter mode) control for FPGA
 *
 */
typedef struct{
	DSP_INT     start;           /**<  0* Initiate =WO */
	DSP_INT     stop;            /**<  1* Cancel   =WO */
	DSP_IINT    gpio_direction_bits;  /**<  2* GPIO direct bits (0=input, 1=output) =WO */
	DSP_IINT    gpio_data_in;    /**<  3* GPIO Data (16 bit) care only about in-bits  =RO */
	DSP_IINT    gpio_data_out;   /**<  4* GPIO Data (16 bit) care only about out-bits =WO */
	DSP_UINT    gatetime_l_0;    /**<  5* Gatetime 0 lower 16 bits in dataprocess cycle units =RW */
	DSP_UINT    gatetime_h_0;    /**<  6* Gatetime 0 upper 16 bits */
	DSP_UINT    gatetime_1;      /**<  7* Gatetime 1 only 16 bits (<800ms max @ 75kHz data process) */
        DSP_ULONG   count_0;         /**<  8* most recent count 0 -- 32bit */
        DSP_ULONG   count_1;         /**< 10* most recent count 1 -- 32bit */
        DSP_ULONG   peak_count_0;    /**< 12* peak count 0 -- 32bit */
        DSP_ULONG   peak_count_1;    /**< 14* peak count 1 -- 32bit */
        DSP_ULONG   peak_holdtime;   /**< 16* *** not used yet ***=> reset via write 0 on peak counts -- 32bit */
	DSP_INT     pflg;            /**< 18* rate-meter mode process active flag =RO */
} CR_GENERIC_IO;
#define MAX_WRITE_CR_GENERIC_IO 8

#ifdef WATCH_ENABLE

/** WATCH -- Debugging Purpuse Only */
# define MAX_WRITE_WATCH 2
typedef struct{
	DSP_INT     start;           /**< 0 Initiate =WO */
	DSP_INT     stop;            /**< 1 Cancel   =WO */
	DSP_INT     v16[16];     /**< 2..17 Watch Vars 16bit */
	DSP_LONG    v32[8];      /**< 18..34 Watch Vars 32bit */
	DSP_INT     pflg;            /**< 35 process active flag =RO */
} WATCH;
# define WATCH16(N,X) {watch.v16[N] = X;}
# define WATCH32(N,X) {watch.v32[N] = X;}
#else
# define WATCH16(N,X) {;}
# define WATCH32(N,X) {;}
#endif

/** Recorder / PACscope tool */
#define MAX_WRITE_RECORDER 7
typedef struct{
	DSP_LONG          pSignal1;        /**< 0 pointer to Signal1 */
	DSP_LONG          pSignal2;        /**< 2 pointer to Signal1 */
	volatile DSP_LONG blcklen16;       /**< 4 Max: (Signalx size * 2) - 1  (8191) */
	volatile DSP_LONG blcklen32;       /**< 6 Max: Signalx size - 1 (4095) */
	DSP_INT           start;           /**< Control Recorder: 1 start 16bit, 2: start 32bit, 99: Stop*/
	DSP_INT           pflg;            /**< 8 recorder process flag -- can be externally set. 0=OFF. Bit 1=>16bit rec, 2=>32bit rec */
} RECORDER;

#if 0
/** DUMMY Control -- Template Only */
#define MAX_WRITE_DUMMYCTRL 3
typedef struct{
	DSP_INT     start;           /**< Initiate =WO */
	DSP_INT     stop;            /**< Cancel   =WO */
	DSP_INT     dummyparameter;  /**< ... =WR */
	DSP_INT     pflg;            /**< process active flag =RO */
} DUMMYCTRL;
#endif

/** Analog intermediate Data Buffer */

#define ANALOG_X0_AUX 0
#define ANALOG_Y0_AUX 1
#define ANALOG_Z0_AUX 2
#define ANALOG_XS_AUX 3
#define ANALOG_YS_AUX 4
#define ANALOG_ZS_AUX 5
#define ANALOG_BIAS   6
#define ANALOG_MOTOR  7

/** Analog and Counter processed data and offset corrections */
typedef struct{
	DSP_INT out[8];         /** data for out request, like bias/motor updates */
	DSP_INT in_offset[8];   /** input offsets */
	DSP_INT out_offset[8];  /** output offsets */
/**  ----------------------- FPGA extended and gated counter[2] -- auto gated on sampling speed/point interval */
	DSP_ULONG counter[2];    /** [] counts */
	DSP_ULONG gate_time[2];  /** [] number of summing cycles */
} ANALOG_VALUES;
#define START_WRITE_ANALOG 0
#define MAX_WRITE_ANALOG 8


#define DATAFIFO_LENGTH (1<<12)
#define DATAFIFO_MASK   ((DATAFIFO_LENGTH)-1)
#define DATAFIFO_MASKL  ((DATAFIFO_LENGTH>>1)-1)

/** DataFifo: data streaming control and buffer (Area Scan) */
typedef struct{
	DSP_INT	    r_position;	  /**< 0 read pos (Gxsm) (WO) by host =WO */
	DSP_INT     w_position;   /**< 1 write pos (DSP) (RO) by host =RO */
	DSP_INT	    fill;	  /**< 2 filled indicator = max num to read =RO */
	DSP_INT	    stall;	  /**< 3 number of fifo stalls =RO */
	DSP_INT	    length;	  /**< 4 constant, buffer size =RO */
	DSP_INT     mode;         /**< 5 data mode (short/long) =RO */
	union {
		DSP_INT	    w[DATAFIFO_LENGTH]; /**< fifo buffer (RO) by host =RO */
		DSP_UINT    uw[DATAFIFO_LENGTH]; /**< fifo buffer (RO) by host =RO */
		DSP_LONG    l[DATAFIFO_LENGTH>>1]; /**< fifo buffer (RO) by host =RO */
		DSP_ULONG   ul[DATAFIFO_LENGTH>>1]; /**< fifo buffer (RO) by host =RO */
	} buffer;
} DATA_FIFO;
#define MAX_WRITE_DATA_FIFO 1


// must match size declaration in FB_spm_magic.asm
#define	EXTERN_PROBEDATAFIFO_LENGTH 0x2800
#define EXTERN_PROBEDATA_MAX_LEFT   0x0040


/** DataFifo: data streaming control and buffer (Vector Probe) */
typedef struct{
	DSP_UINT    r_position;	  /**< read pos (Gxsm) (always in word size) (WO) by host =WO */
	DSP_UINT    w_position;   /**< write pos (DSP) (always in word size) (RO) by host =RO */
	DSP_UINT    current_section_head_position; /**< resync/verify and status info =RO */
	DSP_UINT    current_section_index; /**< index of current section =RO */
	DSP_UINT    fill;	      /**< filled indicator = max num to read =RO */
	DSP_UINT    stall;	      /**< number of fifo stalls =RO */
	DSP_UINT    length;	      /**< constant, buffer size =RO */
	DSP_INT_P   buffer_base; /**< pointer to external memory, buffer start =RO */
	DSP_INT_P   buffer_w;    /**< pointer to external memory =RO */
	DSP_LONG_P  buffer_l;
} DATA_FIFO_EXTERN;
#define MAX_WRITE_DATA_FIFO_EXTERN 1

#endif
