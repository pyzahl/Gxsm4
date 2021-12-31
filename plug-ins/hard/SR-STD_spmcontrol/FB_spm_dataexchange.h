/* SRanger and Gxsm - Gnome X Scanning Microscopy Project
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
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
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

#ifndef __FB_SPM_DATAEXCHANGE_H
#define __FB_SPM_DATAEXCHANGE_H

/* Special Features Config */

/** define DSP_OFFSET_ADDING if the XY-Offset should be added to the XY-Scan-Position, 
 * no XY-Offset output if defined! 
 */
#define DSP_OFFSET_ADDING

/** define DSP_LIN_FUZZYMIX_OPTION if fuzzy mixing of Daemping and dFrq is desired 
 */
#define DSP_LIN_DFM_FUZZYMIX_OPTION

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
#define AIC_STOPMODE_ENABLE


/** DSP-Soft Identification */

#define FB_SPM_MAGIC_ADR 0x4000 /**< Magic struct is at this fixed addess in external SRAM */
#define FB_SPM_MAGIC     0xEE01

/* Features Description */

#ifdef DSP_OFFSET_ADDING
# define FB_SPM_FEATURES_OFFSET_ADDING "enabled"
#else
# define FB_SPM_FEATURES_OFFSET_ADDING "disabled"
#endif

#ifdef DSP_LIN_DFM_FUZZYMIX_OPTION
# define FB_SPM_FEATURES_DFM_FUZZY "available"
#else
# define FB_SPM_FEATURES_DFM_FUZZY "not available"
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



// these are checked/compared with the Gxsm-build and must match!
// -- otherwise you exactly need to know/be sure what you are doing --
// -- odd things like changed data structures, etc.., could break data transfer --
#define FB_SPM_SOFT_ID   0x1001 /* FB_SPM sofware id */
#define FB_SPM_VERSION   0x1058 /* FB_SPM main Version, BCD: 00.00 */
#define FB_SPM_DATE_YEAR 0x2008 /* Date: Year, BCD */
#define FB_SPM_DATE_MMDD 0x0504 /* Date: Month/Day, BCD */

#define FB_SPM_FEATURES     \
	"Log/Lin. Feedback: Yes\nFB-EXT: Option\n"\
	"SCAN: Yes & 2nd-Zoff-scan Disabled, Z[16], AIC0-7[16], dIdV/ddIdV/LckI0/Count[32]! AIC_INT: " FB_SPM_FEATURES_AIC_AS_INT "\n" \
	"Scan and Offset: vector moves\n" \
	"MOVER,APP+ChanSelect+CR-Wave@Port4: Yes\n"\
	"VPROBE: Yes, AIC0-7\nVPROBE-AICdnxINT: " FB_SPM_FEATURES_AIC_INT "\nACPROBE: Yes\nACPROBE2ND: Yes\nACphiQ19\n"\
	"VPROBE-Program-Loops: " FB_SPM_FEATURES_VECTOR_PRG "\n"\
	"DSOSZI: Yes\n"\
	"DSP-level XY-Offset-Adding:" FB_SPM_FEATURES_OFFSET_ADDING "\n"\
	"DFM fuzzymix of Damping+dfrq:" FB_SPM_FEATURES_DFM_FUZZY "\n"\
	"Diff Option AIC5-AIC7 -> AIC5: Yes\n"\
	"CoolRunner: Out Puls, Counter/Timer, Ratemeter mode\n"\
	"AICout0/1/2/6 Offset Adjust: Yes\n"\
	"Last Change: fixed scan line start, added fm_dz - !not compatible!\n"



// use DSP_CC for DSP CC, not if used for GXSM SRanger_HwI include file!
// now via CC flag -DDSP_CC
// #define DSP_CC

#ifdef DSP_CC
typedef short DSP_INT;
typedef unsigned short DSP_UINT;
typedef long DSP_LONG;
typedef unsigned long DSP_ULONG;
#else
typedef gint16  DSP_INT;
typedef guint16 DSP_UINT;
typedef gint32  DSP_LONG;
typedef guint32 DSP_ULONG;
#endif

#ifdef DSP_CC
# define PROBE_VECTOR_P  PROBE_VECTOR*  
# define DSP_INT_P       DSP_INT*
# define DSP_LONG_P      DSP_LONG*
#else
# define PROBE_VECTOR_P  DSP_UINT
# define DSP_INT_P       DSP_UINT
# define DSP_LONG_P      DSP_UINT
#endif


/*
 * only used for initialisation, no floats needed on DSP, 
 * only the cc should compute this! 
 */

#define U_DA_MAX  2.06
#define U_AD_MAX 10.0
#define VOLT2DAC(U) (DSP_INT)((U)*32767./U_DA_MAX)
#define DAC2VOLT(R) (DSP_INT)((R)*U_AD_MAX/32767.)

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
#define MD_1_NOTUSED    0x0001  /**< reserved for future use */
#define MD_BLK          0x0002  /**< Blinking indicator - DSPs heartbeat */
#define MD_PID          0x0004  /**< PID-Regler activ */
#define MD_LOG          0x0008  /**< Log enable flag */
#define MD_OFFSETCOMP   0x0010  /**< Offset Compensation on */
#define MD_ZTRAN        0x0020  /**< Transform Z(AIC[7]) * (-1) -> AIC[6] */
#define MD_DFM_FUZZY    0x0040  /**< DFM Fuzzymixmode: AIC[1] (Daemping) & AIC[6] (dF) are mixed */
#define MD_OFFSETADDING 0x0080  /**< DSP-level Offset Adding */
#define MD_EXTFB        0x0100  /**< Switching external feedback control ON/OFF */
#define MD_AIC_SLOW     0x0200  /**< Enable AIC averaging to get slow and smooth readings */
#define MD_WATCH        0x0400  /**< Watching variables on OUT7 */
#define MD_AIC5M7       0x0800  /**< Software differential inputs: AIC5&7 -> mix to virtual AIC5 */
#define MD_2_NOTUSED    0x1000  /**< reserved for future use */
#define MD_AIC_STOP     0x2000  /**< AICs can be placed in stop (no conversion will take place, dataprocess runs in dummy mode */
#define MD_AIC_RECONFIG 0x4000  /**< Request AIC Reconfig */
#define MD_3_NOTUSED    0x8000  /**< reserved for future use */

/**
 * DSP Magics Structure -- (C) 2002-2006 by P.Zahl
 */
typedef struct {
	DSP_UINT magic;         /**< 0: Magic Number to validate entry =RO */
	DSP_UINT version;       /**< 1: SPM soft verion, BCD =RO */
	DSP_UINT year,mmdd;     /**< 2: year, date, BCD =RO */
	DSP_UINT dsp_soft_id;   /**< 4: Unique SRanger DSP software ID =RO */
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
	DSP_UINT autoapproach;  /**< 14: Address of autoapproch struct =RO */
	DSP_UINT datafifo;      /**< 15: Address of datafifo struct =RO */
	DSP_UINT probedatafifo; /**< 16: Address of probe datafifo struct =RO */
	DSP_UINT scan_event_trigger;/**< 17: Address of scan_event_trigger struct =RO */
	DSP_UINT dfm_fuzzymix;  /**< 18: Address of DFM FUZZY mix control parameter struct =RO */
	DSP_UINT CR_out_puls;   /**< 19: Address of CoolRunner IO Out Puls action struct =RO */
	DSP_UINT external;      /**< 20: Address of external control struct =RO */
	DSP_UINT CR_generic_io; /**< 21: Address of CR_generic_io struct =RO */
	DSP_UINT dummy;         /**< --: Address of xxx struct =RO */
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
	DSP_ULONG BLK_count;    /**< DSP counter, incremented in dataprocess =RO */
	DSP_ULONG BLK_Ncount;   /**< divider to get 1/10 sec =RO */
	DSP_ULONG DSP_time;     /**< DSP time in 1/10sec =RO */
	DSP_INT DSP_tens;       /**< counter to derive 1Hz heart beat =RO */
	DSP_INT DataProcessTime;/**< time spend in dataprocess -- performance indicator =RO */
	DSP_INT IdleTime;       /**< time spend not in dataprocess -- performance indicator =RO */
	DSP_INT DataProcessTime_Peak; /**< time spend in dataprocess, peak -- performance indicator =RO */
	DSP_INT IdleTime_Peak;        /**< time spend not in dataprocess, peak -- performance indicator =RO */
	DSP_INT DataProcessMode;/**< Mode of data processing =RO */
} SPM_STATEMACHINE;
#define MAX_WRITE_SPM_STATEMACHINE 2

/** 
 * definitions used for feedback control
 * ==================================================
 */

/** Feedback System Data/Control Block */

typedef struct {
	DSP_INT cp;           /**< Const Proportional in Q15 rep =RW */
	DSP_INT ci;           /**< Const Integral in Q15 rep =RW */
	DSP_INT setpoint;     /**< real world setpoint, if loaded gain, spm_logn will be calculated and "soll" is updated =WO */
	DSP_INT soll;         /**< "Soll Wert" Set Point (behind log trafo) =RO */
	DSP_INT ist;          /**< "Ist Wert" Real System Value (behind log trafo) =RO */
	DSP_INT delta;        /**< "Regelabweichung" ist-soll =RO */
	DSP_LONG i_pl_sum;    /**< CI - summing, includes area-scan plane-correction summing, 32bits! =RO */
	DSP_LONG i_sum;       /**< CI summing only, mirror, 32bits! =RO */
	DSP_INT z;            /**< "Stellwert" Feedback Control Value "Z" =RO */
} SPM_PI_FEEDBACK;
#define MAX_WRITE_SPM_PI_FEEDBACK 3

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
} EXT_CONTROL;
#define MAX_WRITE_EXT_CONTROL 5


/** 
 * definitions used for FUZZY DFM signal mixing
 * ==================================================
 */

typedef struct {
	DSP_INT level; /**< Level for FUZZY interaction =RW */
	DSP_INT gain;  /**< Gain for FUZZY signal in Q15 rep =RW */
} DFM_FUZZYMIX;
#define MAX_WRITE_DFM_FUZZYMIX 2

/** 
 * definitions used for Move Offset and Scan control
 * ==================================================
 */

/** MoveTo X/Y: Offset Coordinates, Control Structure */

typedef struct{
	DSP_INT     start;           /**< Initiate Offset Move =WO */
	DSP_INT	    nRegel;	     /**< Anzahl Reglerdurchl�fe je Schritt =WR */
	DSP_INT     Xnew, Ynew;	     /**< new/final offset coordinates =WR */
	DSP_LONG    f_dx, f_dy, f_dz;/**< dx, dy of line to move along  =WR */
	DSP_LONG    num_steps;       /**< num steps needed to reach final point and counter =WR */
	DSP_LONG    XPos, YPos, ZPos;/**< current offset coordinates =RO */
	DSP_INT     pflg;            /**< process active flag =RO */
} MOVE_OFFSET;
#define MAX_WRITE_MOVE 12


/** Full Frame Area Scan Control Structure */

typedef struct{
	DSP_INT  start;             /**< 0 Initiate Area Scan =WO */
	DSP_INT  stop;              /**< 1 Cancel Area Scan =WO */
	DSP_INT  rotxx, rotxy, rotyx, rotyy; /**< 2,3,4,5 scan rotation matrix, Q15 =WR */
	DSP_INT	 nx_pre;            /**< 6 number of pre-scan dummy points =WR */
	DSP_INT	 dnx_probe;         /**< 7 number of positions inbetween probe points, -1==noprobe =WR */
	DSP_INT	 raster_a, raster_b;/**< 8,9 alternate probe raster scheme -1==noprobe =WR */
	DSP_LONG srcs_xp, srcs_xm;  /**< 10,12 source channel configuration =WR */
	DSP_LONG srcs_2nd_xp, srcs_2nd_xm;  /**< 14,16    source channel configuration =WR */
	DSP_INT	 nx, ny;            /**< 18,19  number of points to scan in x =WR */
	DSP_LONG fs_dx, fs_dy;      /**< 20,22  32bit stepsize "scan", High Word -> DAC, Low Word -> "decimals" */
	DSP_LONG num_steps_move_xy; /**< 24     number of move steps for vector (fm_dx,fm_dy) xy move */
	DSP_LONG fm_dx, fm_dy, fm_dzxy; /**< 26,28,30  32bit stepsize "move" and slope in xy vec dir, High Word -> DAC, Low Word -> "decimals" */
	DSP_INT	 dnx, dny;          /**< 32,33  delta "nx": number of "DAC positions" inbetween "data points" =WR */
	DSP_INT  Zoff_2nd_xp, Zoff_2nd_xm; /**< 34,35 Zoffset for 2nd scan in EFM/MFM/... quasi const height mode */
	DSP_LONG fm_dzx, fm_dzy;    /**< 36,38 Z-slope in X and Y -- given in possibly rotated area scan coordinate system =WR */
	DSP_LONG Xpos, Ypos, Zpos;  /**< 40,42  current X/Yposition, 32bit =RO */
	DSP_LONG cfs_dx, cfs_dy;    /**< copy of 32bit stepsize "scan", High Word -> DAC, Low Word -> "decimals" */
	DSP_INT  iix, iiy, ix, iy;  /**< current inter x/y, and x/y counters =RO */
	DSP_INT  sstate;            /**< current scan state =RO */
	DSP_INT  pflg;              /**< process active flag =RO */
} AREA_SCAN;
#define MAX_WRITE_SCAN 40


/** Trigger for reoccurring x-pos auto events */

typedef struct{
	DSP_INT trig_i_xp[8]; // 4x for bias, 4x for setpoint, == -1: off
	DSP_INT trig_i_xm[8];
	DSP_INT xp_bias_setpoint[8]; // 4x bias, 4x setpoint
	DSP_INT xm_bias_setpoint[8];
	DSP_INT pflg;
} SCAN_EVENT_TRIGGER;
#define MAX_WRITE_SCAN_EVENT_TRIGGER 33

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
#define VP_FEEDBACK_HOLD 1
#define VP_AIC_INTEGRATE 2



/**
 * Vector Probe control structure
 */
typedef struct{
	DSP_INT     start;           /**<0: Initiate Probe =WO */
	DSP_INT     stop;            /**<1: Cancel Probe =WO */
	DSP_INT     AC_amp;          /**<2: digi LockIn: Amplitude -- digi LockIn used if amp>0 =WR */
	DSP_INT     AC_frq;          /**<3: digi LockIn: Frq. =WR */
	DSP_INT     AC_phaseA;       /**<4: digi LockIn: Phase A =WR */
	DSP_INT     AC_phaseB;       /**<5: digi LockIn: Phase B =WR */
	DSP_INT     AC_nAve;         /**<6: digi LockIn: #Averages =WR */
	DSP_INT     AC_ix;           /**<7: digi LockIn: index =RO/Zero */
	DSP_LONG    time;            /**<8: probe time in loops =RO */
	DSP_LONG    Upos;    	     /**<10: current "X" (probe) value =RO */
	DSP_LONG    Zpos;    	     /**<12: current "X" (probe) value =WR */
	DSP_LONG    LockIn_0;  	     /**<14: last LockIn 0 order result (avg. src) =RO */
	DSP_LONG    LockIn_1stA;     /**<16: last LockIn 1st order result =RO */
	DSP_LONG    LockIn_1stB;     /**<18: last LockIn 1st order result =RO */
	DSP_LONG    LockIn_2ndA;     /**<20: last LockIn 2nd order result =RO */
	DSP_LONG    LockIn_2ndB;     /**<22: last LockIn 2nd order result =RO */
	DSP_UINT    LockInRefSinTabA; /**< Address of phase shifted sin reference table =RW */
	DSP_INT     LockInRefSinLenA; /**< Length of RefTab =RO */
	DSP_UINT    LockInRefSinTabB; /**< Address of phase shifted sin reference table =RW */
	DSP_INT     LockInRefSinLenB; /**< Length of RefTab =RO */
	PROBE_VECTOR_P vector_head;  /**< pointer to head of list of probe vectors =RO/SET */
	PROBE_VECTOR_P vector;       /**< pointer to head of list of probe vectors =RO/SET */
	DSP_INT     ix, iix;         /**< counters =RO */
	DSP_INT     state;           /**< current probe state =RO */
	DSP_INT     pflg;            /**< process active flag =RO */
} PROBE;
#define MAX_WRITE_PROBE 14


/** Auto Approach and Slider/Mover Parameters */
typedef struct{
	DSP_INT     start;           /**< Initiate =WO */
	DSP_INT     stop;            /**< Cancel   =WO */
	DSP_INT     mover_mode;      /**< Mover mode, one off AAP_MOVER_..., see above */
	DSP_INT     piezo_steps;     /**< max number of repetitions */
	DSP_INT     n_wait;          /**< delay inbetween cycels */
	DSP_INT     u_piezo_max;     /**< Amplitude, Peak or length of waveform table */
	DSP_INT     u_piezo_amp;     /**< Amplitude, Peak2Peak */
	DSP_INT     piezo_speed;     /**< Speedcontrol */
	DSP_INT     mv_count;        /**< "time axis" */
	DSP_INT     mv_dir;          /**< "time direction" */
	DSP_INT     mv_step_count;   /**< step counter */
	DSP_INT     u_piezo;         /**< Current Piezo Voltage */
	DSP_INT     step_next;       /**< used for motor (optional) */
	DSP_INT     tip_mode;        /**< Tip mode, used by auto approach */
	DSP_INT     delay_cnt;       /**< Delay count */
	DSP_INT     ci_mult;         /**< retract speedup factor */
	DSP_INT     cp, ci;          /**< temporary used */
	DSP_INT     pflg;            /**< process active flag =RO */
} AUTOAPPROACH;
#define MAX_WRITE_AUTOAPPROACH 8

#define AAP_MOVER_OFF           0 /**< all off */
#define AAP_MOVER_XP_AUTO_APP   1 /**< run in auto approach, uses "XP" for approach or other dirs if set (set mask like AAP_MOVER_XP_AUTO_APP |  AAP_MOVER_[[XY][PM]|WAVE]) */
#define AAP_MOVER_XP            2 /**< manuell XP (+, positive dir, ramp on X offset) steps */
#define AAP_MOVER_XM            4 /**< manuell XM (-, negative dir, ramp on X offset) steps */
#define AAP_MOVER_YP            6 /**< manuell YP (+, positive dir, ramp on Y offset) steps */
#define AAP_MOVER_YM            8 /**< manuell YM (-, negative dir, ramp on Y offset) steps */
#define AAP_MOVER_DIRMMSK  0x000e /**< mask for direction and wave mode */
#define AAP_MOVER_WAVE     0x0010 /**< run waveform in buffer @ 0x5000 (sharing EXTERN_DATA_FIFO_ADDRESS) */
#define AAP_MOVER_PULSE    0x0020 /**< run CR puls -- future mode, !!!not yet implemented!!! */
#define AAP_MOVER_XYOFFSET 0x1000 /**< set this bit if XY offset outputs to be used */
#define AAP_MOVER_XYSCAN   0x2000 /**< set this bit if XY scan outputs to be used */
#define AAP_MOVER_XYMOTOR  0x4000 /**< set this bit if Motor output (X only) is to be used */
#define AAP_MOVER_ZSCANADD 0x0800 /**< set this bit if signal should be added to Z */
#define AAP_MOVER_XXOFFSET 0x8000 /**< back wards compat -- MODE NOT AVAILABLE for SR-STD **/

#define DSOSZI_RUN          1
#define DSOSZI_ONESHOT      2
#define DSOSZI_STOP         3
#define DSOSZI_AIC_RECONFIG 10

#define DSOSZI_TRG_OFF      0
#define DSOSZI_TRG_AUTO_P   1
#define DSOSZI_TRG_AUTO_M   2
#define DSOSZI_TRG_NORMAL_P 3
#define DSOSZI_TRG_NORMAL_M 4
#define DSOSZI_TRG_SINGLE_P 5
#define DSOSZI_TRG_SINGLE_M 6


/** CR_out_puls */
typedef struct{
	DSP_INT     start;           /**< Initiate =WO */
	DSP_INT     stop;            /**< Cancel   =WO */
	DSP_INT     duration;        /**< Puls duration count =WR */
	DSP_INT     period;          /**< Puls period count =WR */
	DSP_INT     number;          /**< number of pulses =WR */
	DSP_INT     on_bcode;        /**< IO value at "on", while "duration" =WR */
	DSP_INT     off_bcode;       /**< IO value at "off", while remaining time of period =WR */
	DSP_INT     reset_bcode;     /**< IO value at end/cancel =WR */
	DSP_INT     io_address;      /**< IO address to use =WR */
	DSP_INT     i_per;           /**< on counter =RO */
	DSP_INT     i_rep;           /**< repeat counter =RO */
	DSP_INT     pflg;            /**< process active flag =RO */
} CR_OUT_PULSE;
#define MAX_WRITE_CR_OUT_PULS 8

/**
 * CR_generic_io and counter control for SR-CoolRunner with Parallel IO & Counter Timer module programmed
 * Port/CR-Register Mapping:
 *
 DSP_INTERFACE: sr_dsp_bus_interface port map (DATA => DATA, ADDRESS => ADDRESS, 
   ISQ => ISQ, RW => RW, IOSTRB => IOSTRB,
  REG0 => D16_0, -- IN_P0 (16bit)
  REG1 => D16_1, -- IN_P1 (16bit)
  REG2 => D16_2, -- IN_P2/STRB_H/L (8bit lo, 8bit hi strb)
  REG3 => STATUS, -- ID & Status -- Bit0: Poll StopBit
  REG4 => D16_4, -- OUT_P0 (16bit)
  REG5 => D16_5, -- OUT_P1 (16bit)
  REG6 => D16_6, -- OUT_P2 (lower 9bit)
  REG7 => COUNT (15 downto 0), -- COUNT L
  REG8 => COUNT (31 downto 16), -- COUNT H
  REG9 => CTRL, -- COUNTER CONTROL REGISTER -- Bit0: Reset, 1,2,3 Restart, 4: INT_EN
  REGA => GATECOUNT -- GATE COUNT i.e. TIME, in TOUT0 ticks
  );
 *
 */
typedef struct{
	DSP_INT     start;           /**<  0* Initiate =WO */
	DSP_INT     stop;            /**<  1* Cancel   =WO */
	DSP_INT     port;            /**<  2* IO port address to use (0..15 single port action, >15 special actions) =WR */
	DSP_INT     rw;              /**<  3* Data direction: Bit0 ==> 0: Read, 1: Write, Bit1 ==> Int3En =WO */
	DSP_UINT    data_lo;         /**<  4* Data (16 bit) =RW */
	DSP_UINT    data_hi;         /**<  5* Data (upper 16 bit for special long reads) =RW */
	DSP_UINT    mode;            /**<  6* Gate Timer Mode with reset (FF=3 (100ns), Fast=5, Slow=9) */
	DSP_UINT    gatetime;        /**<  7* 16bit downcounter for automatic gate timer */
        DSP_ULONG   count;           /**<  8* most recent count -- 32bit */
        DSP_ULONG   peak_count;      /**< 10* peak count -- 32bit */
        DSP_ULONG   peak_count_cur;  /**< 12* 2nd peak count -- 32bit */
        DSP_ULONG   xtime;           /**< 14* number of measurements -- 32bit */
        DSP_ULONG   peak_holdtime;   /**< 16* number of measurements until peak reset -- 32bit */
	DSP_INT     pflg;            /**< 18* process active flag =RO */
} CR_GENERIC_IO;
#define MAX_WRITE_CR_GENERIC_IO 8


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

/** Named AIC buffer struct */

/** Analog Output Configuration */
typedef struct{
/**  <---------------------- In <---*/
	DSP_INT in0;       /**< <- AIC0 [+0] */
	DSP_INT in1;       /**< <- AIC1 [+1] */
	DSP_INT in2;       /**< <- AIC2 [+2] */
	DSP_INT in3;       /**< <- AIC3 [+3] */
	DSP_INT in4;       /**< <- AIC4 [+4] */
	DSP_INT feedback;  /**< <- AIC5 [+5] */
	DSP_INT in6;       /**< <- AIC6 [+6] */
	DSP_INT in7;       /**< <- AIC7 [+7] */
/**  -----------------------> Out ->*/
	DSP_INT x_offset;  /**< -> AIC0 [+8]*/  
	DSP_INT y_offset;  /**< -> AIC1 [+9] */  
	DSP_INT z_offset;  /**< -> AIC2 [+10] */  
	DSP_INT x_scan;    /**< -> AIC3 [+11] */  
	DSP_INT y_scan;    /**< -> AIC4 [+12] */  
	DSP_INT z_scan;    /**< -> AIC5 [+13] */
	DSP_INT bias;      /**< -> AIC6 [+14] */
	DSP_INT aux7;      /**< -> AIC7 [+15] */
/**  ----------------------- AIC 0..7 input Offsets */
	DSP_INT offset[8];
/**  ----------------------- N times (length) averaged AIC data */
	DSP_LONG aic_slow[8];
	DSP_LONG aic_avg[8];
	DSP_INT  length[8];
	DSP_INT  aic0_pipe[32];
	DSP_INT  aic1_pipe[32];
	DSP_INT  aic2_pipe[32];
	DSP_INT  aic3_pipe[32];
	DSP_INT  aic4_pipe[32];
	DSP_INT  aic5_pipe[32];
	DSP_INT  aic6_pipe[128];
	DSP_INT  aic7_pipe[128];
/**  ----------------------- AIC 0..7 output Offsets */
	DSP_INT out_offset[8];
} ANALOG_VALUES;
#define START_WRITE_ANALOG 8
#define MAX_WRITE_ANALOG 8


#define DATAFIFO_LENGTH (1<<12)
#define DATAFIFO_MASK   ((DATAFIFO_LENGTH)-1)

/** DataFifo: data streaming control and buffer (Area Scan) */
typedef struct{
	DSP_INT	    r_position;	  /**< read pos (Gxsm) (WO) by host =WO */
	DSP_INT     w_position;   /**< write pos (DSP) (RO) by host =RO */
	DSP_INT	    fill;	      /**< filled indicator = max num to read =RO */
	DSP_INT	    stall;	      /**< number of fifo stalls =RO */
	DSP_INT	    length;	      /**< constant, buffer size =RO */
	DSP_INT     mode;         /**< data mode (short/long) =RO */
	union {
		DSP_INT	    w[DATAFIFO_LENGTH]; /**< fifo buffer (RO) by host =RO */
		DSP_LONG    l[DATAFIFO_LENGTH>>1]; /**< fifo buffer (RO) by host =RO */
	} buffer;
} DATA_FIFO;
#define MAX_WRITE_DATA_FIFO 1




#define EXTERN_PROBE_VECTOR_HEAD_DEFAULT 0x4100
#define EXTERN_PROBE_VECTOR_HEAD_DEFAULT_P ((PROBE_VECTOR*) EXTERN_PROBE_VECTOR_HEAD_DEFAULT)
#define	EXTERN_PROBEDATAFIFO_LENGTH 0xb000
#define EXTERN_PROBEDATA_MAX_LEFT   0x0040
#define EXTERN_DATA_FIFO_ADDRESS    0x5000
#define EXTERN_DATA_FIFO_ADDRESS_P ((void*) EXTERN_DATA_FIFO_ADDRESS)
#define EXTERN_DATA_FIFO_LIMIT (EXTERN_DATA_FIFO_ADDRESS+EXTERN_PROBEDATAFIFO_LENGTH-EXTERN_PROBEDATA_MAX_LEFT)

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
