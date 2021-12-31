/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* SRanger MK3 and Gxsm - Gnome X Scanning Microscopy Project
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
 * Copyright (C) 1999-2011 Percy Zahl
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

/* MK3 Note: all addresses and data are 32bit (LONG) */

#ifndef __MK3_FB_SPM_DATAEXCHANGE_H
#define __MK3_FB_SPM_DATAEXCHANGE_H

#ifdef DSPEMU
#include "glib.h"
int map_dsp_memory (const gchar *sr_emu_dev_file);

#endif

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

/** define RECONFIGURE_DSP_SPEED to provide DSP speed reconfiguartion control
 */
#define RECONFIGURE_DSP_SPEED


/** DSP-Soft Identification */

#define FB_SPM_MAGIC_ADR  0x10F13F00 /**< Magic struct is at this fixed addess in external SRAM */
#define FB_SPM_MAGIC      0x3202EE01

/* GXSM SPM CONTROL FLASH SECTOR */
#define SPM_CONTROL_FLASH_SECTOR_ADDRESS  0x420A0000

/* Features Description */

#define USE_PLL_API
#ifdef USE_PLL_API
# define FB_SPM_FEATURES_PLL "enabled"
#else
# define FB_SPM_FEATURES_PLL "disabled"
#endif

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
#define FB_SPM_SOFT_ID   0x00001002 /* FB_SPM sofware id */
#define FB_SPM_VERSION   0x00003051 /* FB_SPM main Version, BCD: 00.00 */
#define FB_SPM_DATE_YEAR 0x00002016 /* Date: Year/MM/DD, BCD */
#define FB_SPM_DATE_MMDD 0x00000616 /* Date: Month/Day, BCD */

#define FB_SPM_FEATURES     \
	"Version: Signal Master Evolved GXSM3B\n"\
	"MK3Pro-A810/PLL+PAC Platform:\n\n" \
	"Signal Management/Routing, Signal Matrix support\n"\
	"dynamic current IIR channel + IIR mixer channels: Yes\n"\
	"PLL PAC1F2F AMPCTRL-FUZZY-NEG: " FB_SPM_FEATURES_PLL "\n"\
	"SCAN: Yes, Pause/Resume: Yes, Fast X-Sine mode: Yes, 2nd-Zoff-scan AKTIVE, Z[16], AIC0-7[16], 4x32bit-SIGNALS]! AIC_INT: " FB_SPM_FEATURES_AIC_AS_INT "\n" \
	"Scan and Offset: vector moves, scan xyz move with limiter option\n" \
	"MOVER,APP+ChanSelect: Yes, AAP GPIO pulse: Yes and general GPIO, IW/phase: Yes\n"\
	"VPROBE: Yes\nVPROBE-AICdnxINT: " FB_SPM_FEATURES_AIC_INT "\nACPROBE: Yes\n 4x32bit-SIGNALs: Yes\nACphiQ19\n GPIO-WRITE option per segment: Yes, Trigger-Opt: Yes"\
	"VPROBE-Program-Loops: " FB_SPM_FEATURES_VECTOR_PRG "\n"\
	"SIGNAL RECORDER: Yes (if PAC=on), GPIO: Yes\n"\
	"DSP-level XY-Offset-Adding:" FB_SPM_FEATURES_OFFSET_ADDING "\n"\
	"4x Diff Input Mixer: Yes\n"\
	"Bias/Z Re-Adjuster: Yes\n"\
	"HR MODE FOR ALL OUTPUT SIGNALS, HR MATRIX control: Yes, LDC option in offset move function: Yes\n"\
	"SCO block: Yes\n"\
	"THIS IS SR-MK3-Pro with Analog810 8Ch 16bit in/out + GPIO + 2 Counters\nAnalog810 ReConfig+-Start: Yes\n"


// use DSP_CC for DSP CC, not if used for GXSM SRanger_HwI include file!
// ==> now defined at compile time via Makefile CC option -DDSP_CC
// #define DSP_CC

#ifdef DSP_CC  // int on C6x is 32bit, long is 40bits, long long is 64bits
# ifdef DSP_EMU
typedef gint32   DSP_NINT;
typedef guint32  DSP_NUINT;
typedef guint32  DSP_UINTA;
typedef gint16   DSP_INT16;
typedef guint16  DSP_UINT16;
typedef gint32   DSP_INT32;
typedef guint32  DSP_UINT32;
typedef gint64   DSP_INT64;
typedef guint64  DSP_UINT64;
typedef size_t   DSP_MEMOFFSET;
# else
typedef int            DSP_NINT;  // index, emumerations, counting, sizes
typedef unsigned int   DSP_NUINT;
typedef unsigned int   DSP_UINTA; // pointers, adresses, references
typedef short          DSP_INT16; // data
typedef unsigned short DSP_UINT16;
typedef int            DSP_INT32;
typedef unsigned int   DSP_UINT32;
typedef long           DSP_INT40; // only on DSP for temp calc
typedef unsigned long  DSP_UINT40;
typedef long long      DSP_INT64;
typedef unsigned long long  DSP_UINT64;
typedef unsigned int   DSP_MEMOFFSET;
# endif
#else
typedef gint32   DSP_NINT;
typedef guint32  DSP_NUINT;
typedef guint32  DSP_UINTA;
typedef gint16   DSP_INT16;
typedef guint16  DSP_UINT16;
typedef gint32   DSP_INT32;
typedef guint32  DSP_UINT32;
typedef gint64   DSP_INT64;
typedef guint64  DSP_UINT64;
typedef guint32  DSP_MEMOFFSET;
#endif

# define PROBE_VECTOR_P  PROBE_VECTOR*  
# define DSP_INT16_P     DSP_INT16*
# define DSP_INT32_P     DSP_INT32*
# define DSP_INT64_P     DSP_INT64*

/*
 * only used for initialisation, no floats needed on DSP, 
 * only the cc should compute this! 
 */

#define U_DA_MAX 10.0
#define U_AD_MAX 10.0

#ifndef NULL
#define NULL ((void *) 0)
#endif

/* virtual 32bit -- actual 16.16 -- may use HR output mode */
#define VOLT2DAC(U) (DSP_INT32)((U)*32767./U_DA_MAX*(1<<16))
#define DAC2VOLT(R) (DSP_INT32)((R)*U_AD_MAX/(32767.*(1<<16)))

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

/* -- NOTE: modes mostly obsoleted now -- */

#define MD_IDLE          0x0000  /**< all zero means ready, no idle task (scan,move,probe,approch,...) is running */
#define MD_HR            0x0001  /**< High Res mode enable (Sigma-Delta resolution enhancement via bit0 for Z) */
#define MD_BLK           0x0002  /**< Blinking indicator - DSPs heartbeat */
#define MD_PID           0x0004  /**< PID-Regler activ */
#define MD_XYSROT        0x0008  /**< ENABLE VECTOR SCAN ROTATION */
#define MD_NOISE         0x0040  /**< add noise to Bias */
#define MD_OFFSETADDING  0x0080  /**< DSP-level Offset Adding */
#define MD_ZPOS_ADJUSTER 0x0200  /**< must be normally enabled: auto readjust ZPos to center after probe (or XYZ scan manupilations) */
#define MD_PLL           0x1000  /**< run PLL */
#define MD_AIC_STOP      0x2000  /**< AICs can be placed in stop (no conversion will take place, dataprocess runs in dummy mode */
#define MD_AIC_RECONFIG  0x4000  /**< Request AIC Reconfig */

#define MaxSizeSigBuf 1000000




/**
 * DSP Magics Structure -- (C) 2008 by P.Zahl
 * MK2 all addresses have to be 32bit
 * Magics 0..4 -- all 32bit now, keeping order compatible.
 */
typedef struct {
	DSP_UINT32 magic;         /**< 0: Magic Number to validate entry =RO */
	DSP_UINT32 version;       /**< 1: SPM soft verion, BCD =RO */
	DSP_UINT32 year,mmdd;     /**< 2,3: year, date, BCD =RO */
	DSP_UINT32 dsp_soft_id;   /**< 4: Unique SRanger DSP software ID =RO */
	/* -------------------- Magic Symbol Table ------------------------------ */
	DSP_MEMOFFSET statemachine;  /**<  5: Address of statemachine struct =RO */
	DSP_MEMOFFSET AIC_in;        /**<  6: AIC in buffer =RO */
	DSP_MEMOFFSET AIC_out;       /**<  7: AIC out buffer =RO */
	DSP_MEMOFFSET AIC_reg;       /**<  8: AIC register (AIC control struct) =RO */
	/* -------------------- basic magic data set ends ----------------------- */
	DSP_MEMOFFSET analog;        /**<  9: Address of analog struct =RO */
	DSP_MEMOFFSET signal_monitor;/**< 10: Address of watch struct if available =RO */
	DSP_MEMOFFSET feedback_mixer;/**< 11: Address of DFM FUZZY mix control parameter struct =RO */
	DSP_MEMOFFSET z_servo;       /**< 12: Address of z-servo (main feedback) struct =RO */
	DSP_MEMOFFSET m_servo;       /**< 13: Address of motor servo (auxillary feedback) control struct =RO */
	DSP_MEMOFFSET scan;          /**< 14: Address of ascan struct =RO */
	DSP_MEMOFFSET move;          /**< 15: Address of move struct =RO */
	DSP_MEMOFFSET probe;         /**< 16: Address of probe struct =RO */
	DSP_MEMOFFSET autoapproach;  /**< 17: Address of autoapproch struct =RO */
	DSP_MEMOFFSET datafifo;      /**< 18: Address of datafifo struct =RO */
	DSP_MEMOFFSET probedatafifo; /**< 19: Address of probe datafifo struct =RO */
	DSP_MEMOFFSET scan_event_trigger;/**< 20: Address of scan_event_trigger struct =RO */
	DSP_MEMOFFSET CR_out_puls;   /**< 21: Address of CoolRunner IO Out Puls action struct =RO */
	DSP_MEMOFFSET CR_generic_io; /**< 22: Address of CR_generic_io struct =RO */
	DSP_MEMOFFSET a810_config;   /**< 23: Address of a810_config struct =RO */
	DSP_MEMOFFSET hr_mask;       /**< 24: Address of sigma_delta_hr_mask */
	DSP_MEMOFFSET PLL_lookup;    /**< 25: Address of PLL_lookup magics  */
	DSP_MEMOFFSET signal_lookup; /**< 26: Address of PLL_lookup magics  */
	DSP_MEMOFFSET sco1; /**< 27: Address of sco1 struct =RO */
	DSP_INT32  life;         /**< --: life counter  =RO */
	DSP_UINT32 dummyBE;      /**< --: BE test pat =RO */
#ifdef DSP_CC
} SPM_MAGIC_DATA_LOCATIONS;
#else
} SPM_MAGIC_DATA_LOCATIONS_MK3;
#endif
#define MAX_WRITE_SPM_MAGIC_DATA_LOCATIONS 0

/**
 * Main DSP Statemachine Control Structure
 */

typedef struct {
	DSP_UINT32 set_mode;      /**< 0 mode change request: set bits =WO */
	DSP_UINT32 clr_mode;      /**< 2 mode change request: clear bits =WO */
	DSP_UINT32 mode;          /**< 4 current state =RO */
	DSP_UINT32 last_mode;     /**< 6 last state =RO */
	DSP_UINT32 BLK_count;    /**< 8 DSP counter, incremented in dataprocess =RO -- obsolete */
	DSP_UINT32 BLK_Ncount;   /**< 10 divider to get 1/10 sec =RO -- obsolete */
	DSP_INT32 DSP_time;     /**< 12 DSP time in 1/10sec =RO */
	DSP_UINT32 DSP_tens;       /**< 14 counter to derive 1Hz heart beat =RO */
	DSP_UINT32 DataProcessTime;/**< 16 time spend in dataprocess -- performance indicator =RO */
	DSP_UINT32 IdleTime;       /**< 18time spend not in dataprocess -- performance indicator =RO */
	DSP_UINT32 DataProcessTime_Peak; /**< 20 time spend in dataprocess, peak -- performance indicator =RO */
	DSP_UINT32 IdleTime_Peak;        /**< 22 time spend not in dataprocess, peak -- performance indicator =RO */
	DSP_UINT32 DSP_speed[2]; /**< 24, 26 DSP speed setting (MHz) actual, requested */
#ifdef DSP_CC
} SPM_STATEMACHINE;
#else
} SPM_STATEMACHINE_MK3;
#endif
#define MAX_WRITE_SPM_STATEMACHINE (2*2)

/** 
 * definitions used for Analog810 Reconfiguration
 * ==================================================
 */

typedef struct {
	DSP_UINT16 freq_div;  // = 10;  *** 75kHz *** 5 ... 65536  150kHz at N=5   Fs = Clk_in/(N * 200), Clk_in=150MHz
        DSP_UINT16 adc_range; // = 0;   *** 0: +/-10V, 1:+/-5V
        DSP_UINT16 qep_on;    // = 0x0001; *** enable managing QEP counters - disabling will free DSP io bandwidth
	DSP_UINT16 fill;
#ifdef DSP_CC
} A810_CONFIG;
#else
} A810_CONFIG_MK3;
#endif
#define MAX_WRITE_A810_CONFIG (3*2)

/** 
 * definitions used for Generic Servo (=Feedback) control module
 * =================================================================
 **/

typedef struct {
	DSP_INT32   cp;              /**< servo CP =RW */
	DSP_INT32   ci;              /**< servo CI =RW */
	DSP_INT32   setpoint;        /**< servo setpoint =RW ** optional for stand alone servo */
	DSP_INT32_P input;           /**< servo input =RW -- must be Q23 aligned */
	DSP_INT32   delta;           /**< servo delta =RO */
	DSP_INT32   i_sum;           /**< servo intergrator =RO */
	DSP_INT32   control;         /**< servo control output =RO */
	DSP_INT32   neg_control;     /**< servo -control output =RO */
	DSP_INT32   watch;           /**< servo activity watch flag */
#ifdef DSP_CC
} SERVO;
#else
} SERVO_MK3;
#endif
#define MAX_WRITE_SERVO (2*4)

/** 
 * definitions used for Generic SCO (Signal Controlled Oszillator)
 * =================================================================
 **/

typedef struct {
	DSP_INT32   center;          /**< sco f-center Input Signal Value =RW */
	DSP_INT32   fspanV;          /**< sco f span per input Signal, scaled 89953.584655 =RW */
	DSP_INT32   deltasRe;        /**< sco internal =RO */
	DSP_INT32   deltasIm;        /**< sco internal =RO */
	DSP_INT32   cr;              /**< sco internal =RO */
	DSP_INT32   ci;              /**< sco internal =RO */
	DSP_INT32   cr2;             /**< sco internal =RO */
	DSP_INT32   ci2;             /**< sco internal =RO */
	DSP_INT32_P in;              /**< sco Input Signal =RW */
	DSP_INT32_P amplitude;       /**< sco amplitude: input Signal =RW */
	DSP_INT32   out;             /**< sco Output Signal =RO */
        DSP_INT32   control;         /**< sco mode: 0 run auto in signal, 1: run manual on fixed freq */
#ifdef DSP_CC
} SCO;
#else
} SCO_MK3;
#endif
#define MAX_WRITE_SCO (2*4)
#define NUM_SCO_S 2


/** 
 * definitions used for FUZZY DFM signal mixing
 * ==================================================
 */

typedef struct {
	DSP_INT32 level[4];           /**< 0 if FUZZY MODE: Level for FUZZY interaction =RW */
	DSP_INT32 gain[4];            /**< 4 Gain for signal in Q15 rep =RW */
	DSP_INT32 mode[4];            /**< 8 mixer mode: Bit0: 0=OFF, 1=ON (Lin mode), Bit1: 1=Log/0=Lin, Bit2: reserved (IIR/FULL), Bit3: 1=FUZZY mode, Bit4: negate input value */
	DSP_INT32 setpoint[4];        /**< 12 individual setpoint for every signal */
	DSP_INT32 iir_ca_q15[4];      /**< 16 IIR to channel input, IIR ca value, 0=FBW -- MIX0: IIR filter adapt: ca    = q_sat_limit cut-off */
	DSP_INT32 cb_Ic;              /**< 20 MIX0 adapt IIR filter cb_Ic = f_0,min/f_0,max * I_cross [32] */
	DSP_INT32 I_cross;            /**< 21 MIX0 adapt IIR filter adapt: I_crossover */
	DSP_INT32 I_offset;           /**< 22 MIX0 adapt I_offset, log start/characteristic */
	DSP_INT32 align;
	DSP_INT64 f_reference;        /**< 24 ALIGN 64!!! PAC f-setpoint/reference to be subtracted from APC F-EXEC to get the shift the FB works on */
	DSP_INT32 exec;               /**  26 log computation of x -> lnx =WO */
	DSP_INT32 x, lnx;             /**  27,28 individual setpoint for every signal RW */
	DSP_INT32 y;                  /**  29 */
	DSP_INT32 iir_signal[4];      /**  30..33 IIR filtered or original signal, Q15.16 */
	DSP_INT32 q_factor15;         /**  34 MIX0 IIR filter: q --  (In=1/(1+q) (I+q*In)) in Q15 */
	DSP_INT32 delta;              /**  35 final mixer delta signal */
	DSP_INT32 setpoint_log[4];    /**  36..39 individual setpoint after log for every signal */
	DSP_INT32_P input_p[4];       /**< 40..43 MIN[0..3] input signal pointer -- may by a channel index 0..7 */
	DSP_INT32 FB_IN_processed[4]; /**  44..47 processed MIX0..3 inputs Q23.8 */
	DSP_INT32 channel[4];         /**  48..51 processed MIX0..3 channel after setpoint, etc. differences Q23.8 */
	DSP_INT32 FB_IN_is_analog_flg[4];
#ifdef DSP_CC
 } FEEDBACK_MIXER;
#else
} FEEDBACK_MIXER_MK3;
#endif
#define MAX_WRITE_FEEDBACK_MIXER (2*(5*4 + 4))

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
	DSP_INT32    start;           /**< Initiate Offset Move -- see MODE_[OFFSET | LDC]_MOVE =WO */
	DSP_INT32    nRegel;	      /**< Anzahl Reglerdurchlaeufe je Schritt =WR */
	DSP_INT32    xy_new_vec[2];   /**< new/final offset coordinates =WR */
	DSP_INT32    f_d_xyz_vec[3];      /**< dx, dy of line to move along OR in LDC mode: single LDC step every num_steps cycle  =WR */
	DSP_INT32    num_steps;       /**< num steps needed to reach final point and counter OR in LDC mode: slow down factor  =WR */
	DSP_INT32    xyz_gain;        /**< bitcoded 8-/8z/8y/8x (0..255)x */
	DSP_INT32    xyz_vec[3];      /**< current offset coordinates =RO */
	DSP_INT32    pflg;            /**< process active flag -- set to 1 or 2 as copy of start mode via statemachine -- see MODE_[OFFSET | LDC]_MOVE =RO */
#ifdef DSP_CC
} MOVE_OFFSET;
#else
} MOVE_OFFSET_MK3;
#endif
#define MAX_WRITE_MOVE (2*9)


/** Full Frame Area Scan Control Structure */
// start modes
#define AREA_SCAN_RUN            0x01
#define AREA_SCAN_START_NORMAL   0x04
#define AREA_SCAN_START_FASTSCAN 0x08
// stop modes
#define AREA_SCAN_STOP           0x01
#define AREA_SCAN_PAUSE          0x02
#define AREA_SCAN_RESUME         0x04

typedef struct{
	DSP_INT32  start;             /**< 0 Initiate Area Scan =WO */
	DSP_INT32  stop;              /**< 1 Cancel (=1) or Pause (=2) or Resume (=4) Area Scan =WO */
	DSP_INT32  rotm[4];           /**< 2,3,4,5 scan rotation matrix, Q15 =WR */
	DSP_INT32  nx_pre;            /**< 6 number of pre-scan dummy points =WR */
	DSP_INT32  dnx_probe;         /**< 7 number of positions inbetween probe points, -1==noprobe =WR */
	DSP_INT32  raster_a, raster_b;/**< 8,9 alternate probe raster scheme -1==noprobe =WR */
	DSP_INT32  srcs_xp, srcs_xm;  /**< 10,11 source channel configuration =WR */
	DSP_INT32  srcs_2nd_xp, srcs_2nd_xm;  /**< 12,13    source channel configuration =WR */
	DSP_INT32  nx, ny;            /**< 14,15  number of points to scan in x =WR */
	DSP_INT32  fs_dx, fs_dy;      /**< 16,17  32bit stepsize "scan", High Word -> DAC, Low Word -> "decimals" */
	DSP_INT32  num_steps_move_xy; /**< 18     number of move steps for vector (fm_dx,fm_dy) xy move */
	DSP_INT32  fm_dx, fm_dy, fm_dz; /**< 19,20,21  32bit stepsize "move" and slope in xy vec dir, High Word -> DAC, Low Word -> "decimals" */
	DSP_INT32  dnx, dny;          /**< 22,23  delta "nx": number of "DAC positions" inbetween "data points" =WR */
	DSP_INT32  Zoff_2nd_xp, Zoff_2nd_xm; /**< 24,25 Zoffset for 2nd scan in EFM/MFM/... quasi const height mode -- shared with deltasRe/Im for fast sine scan*/
        DSP_INT32  fm_dz0_xy_vec[2];  /**< 26,27 X/Y-Z0-slopes in X and Y in main XY coordinate system =WR */
        DSP_INT32  z_slope_max;       /**< 28,   fast return option */
        DSP_INT32  fast_return;       /**< 29,   fast return option */
	DSP_INT32  xyz_gain;          /**< 30 bitcoded 8-/8z/8y/8x (0..255)x */
        DSP_INT32  xyz_vec[3];        /**< 31,32,33  current X/Yposition, 32bit =RO, scan coord sys */
        DSP_INT32  xy_r_vec[2];       /**< 34,35     current X/Yposition, 32bit =RO, rotated final/offset coord sys */
	DSP_INT32  z_offset_xyslope;  /**< 36 Z offset calculated from XY Scan pos and slope coefficients, smooth follower */
        DSP_INT32  cfs_dx, cfs_dy;    /**< 37,38 copy of 32bit stepsize "scan", High Word -> DAC, Low Word -> "decimals" */
        DSP_INT32  iix, iiy, ix, iy;  /**< current inter x/y, and x/y counters =RO */
	DSP_INT32  slow_down_factor, iiix; /**< current scan speed reduction beyond 2^15 dnx */
        DSP_INT32  ifr;               /**< fast return count option */
	DSP_INT32_P src_input[4];     /**< signals mapped as channel source data. defaults: probe.LockIn_1stA, LockIn_2ndA, LockIn_0A, analog.counter[0]  */
	DSP_INT32  sstate;            /**< current scan state =RO */
	DSP_INT32  pflg;              /**< process active flag =RO */
#ifdef DSP_CC
} AREA_SCAN;
#else
} AREA_SCAN_MK3;
#endif
#define MAX_WRITE_SCAN (2*31)


/** Trigger for reoccurring x-pos auto events */

typedef struct{
	DSP_INT32 trig_i_xp[8]; // 4x for bias, 4x for setpoint, == -1: off
	DSP_INT32 trig_i_xm[8];
	DSP_INT32 xp_bias_setpoint[8]; // 4x bias, 4x setpoint
	DSP_INT32 xm_bias_setpoint[8];
	DSP_INT32 pflg;
#ifdef DSP_CC
} SCAN_EVENT_TRIGGER;
#else
} SCAN_EVENT_TRIGGER_MK3;
#endif
#define MAX_WRITE_SCAN_EVENT_TRIGGER (2*4*8)

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
	DSP_INT32    n;             /**< 0: number of steps to do =WR */
	DSP_INT32    dnx;           /**< 2: distance of samples in steps =WR */
	DSP_UINT32   srcs;          /**< 4: SRCS source channel coding =WR */
	DSP_UINT32   options;       /**< 6: Options, Dig IO, ... not yet all defined =WR */
	DSP_UINT32   ptr_fb;        /**< 8: optional pointer to new feedback data struct first 3 values of SPM_PI_FEEDBACK =WR */
	DSP_UINT32   repetitions;   /**< 9: numer of repetitions =WR */
	DSP_UINT32   i,j;           /**<10,11: loop counter(s) =RO/Zero */
	DSP_INT32    ptr_next;      /**<12: next vector (relative to VPC) until --rep_index > 0 and ptr_next != 0 =WR */
        DSP_INT32    ptr_final;     /**<13: next vector (relative to VPC), =1 for next Vector in VP, if 0, probe is done =WR */
	DSP_INT32    f_du;          /**<14: U (bias) stepwidth (32bit) =WR */
	DSP_INT32    f_dx;          /**<16: X stepwidth (32bit) =WR */
	DSP_INT32    f_dy;          /**<18: Y stepwidth (32bit) =WR */
	DSP_INT32    f_dz;          /**<20: Z stepwidth (32bit) =WR */
	DSP_INT32    f_dx0;         /**<22: X0 (offset) stepwidth (32bit) =WR */
	DSP_INT32    f_dy0;         /**<24: Y0 (offset) stepwidth (32bit) =WR */
	DSP_INT32    f_dphi;        /**<26: Phase stepwidth (32bit) +/-15.16Degree =WR */
#ifdef DSP_CC
} PROBE_VECTOR;
#else
} PROBE_VECTOR_MK3;
#endif
#define SIZE_OF_PROBE_VECTOR (2*17)

/** VP option masks */
#define VP_FEEDBACK_HOLD 0x01
#define VP_AIC_INTEGRATE 0x02
#define VP_TRACK_REF     0x04
#define VP_TRACK_UP      0x08
#define VP_TRACK_DN      0x10
#define VP_TRACK_FIN     0x20
#define VP_TRACK_SRC     0xC0  //** obsolete -- signal now
#define VP_LIMITER       0x300 // Limiter ON/OFF flag mask
#define VP_LIMITER_UP    0x100 // Limit if > value
#define VP_LIMITER_DN    0x200 // Limit if < value
#define VP_LIMITER_MODE  0xC0  // Limiter mode code bit mask 0x40+0x80  00: hold, 01: abort with vector forward branch (postive) PCJR
#define VP_GPIO_MSK        0x00ff0000 // GPIO 8bit mask (lower only)
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
	DSP_INT32     start;           /**<0: Initiate Probe =WO */
	DSP_INT32     stop;            /**<1: Cancel Probe =WO */
	DSP_INT32     AC_amp;          /**<2: digi LockIn: Amplitude -- for digi LockIn signal scaling (default use: Bias) =WR */
	DSP_INT32     AC_amp_aux;      /**<3: digi LockIn: aux. Amplitude -- digi LockIn signal scaling (default use: Z) =WR */
	DSP_INT32     AC_frq;          /**<4: digi LockIn: Frq. =WR */
	DSP_INT32     AC_phaseA;       /**<5: digi LockIn: Phase A =WR */
	DSP_INT32     AC_phaseB;       /**<6: digi LockIn: Phase B =WR */
	DSP_INT32     AC_nAve;         /**<7: digi LockIn: #Averages =WR */
	DSP_INT32     noise_amp;       /**<8: general purpose "noise_Amp" - may be used as scale for noise Q15 =WR */
	DSP_INT32     lockin_shr_corrprod; /**<9: LockIn data scaling of correlation product (shr 16) =WR */
	DSP_INT32     lockin_shr_corrsum;  /**<10: LockIn data scaling of correlation intergation results (shr 8) =WR */
	DSP_INT32_P   src_input[4];    /**<11:..  mapped data from lookup =0/1/2/3: defaults IN-CH0/1/2/3, IN-4..7. */
	DSP_INT32_P   prb_output[4];   /**<:   mapped data from lookup =0/1/2/3: defaults IN-CH0/1/2/3, IN-4..7. */
	DSP_INT32_P   LockIn_input[4]; /**<:   mapped data for LockIn A,B Inputs [MIX0...3, processed] and A,B Mod Out [U_Bias, Zpos, M] set via lookup */
	DSP_INT32_P   limiter_updn[2]; /**<: limiter value signal upper, lower =WR */
	DSP_INT32_P   limiter_input;   /**< mapping for Limiter (I) signal  */
	DSP_INT32_P   tracker_input;   /**< mapping for Tracker (Z) signal  */
	DSP_INT32     AC_ix;           /**<9: digi LockIn: index =RO/Zero */
	DSP_INT32    time;            /**<10: probe time in loops =RO */
	DSP_INT32    Upos;    	     /**<11: current "X" (probe) value =RO */
	DSP_INT32    Zpos;    	     /**<12: current "X" (probe) value =WR */
	DSP_INT32    LockIn_ref;     /**<14: internal reference (modulation) signal */
	DSP_INT32    LockIn_0A;	     /**<16: last LockIn 0 order result (avg. src) =RO */
	DSP_INT32    LockIn_0B;	     /**<22: last LockIn 0 order result (avg. src) =RO */
	DSP_INT32    LockIn_1stA;     /**<18: last LockIn 1st order result =RO */
	DSP_INT32    LockIn_1stB;     /**<20: last LockIn 1st order result =RO */
	DSP_INT32    LockIn_2ndA;     /**<24: last LockIn 2nd order result =RO */
	DSP_INT32    LockIn_2ndB;     /**<26: last LockIn 2nd order result =RO */
	DSP_INT16_P  LockInRefSinTabA; /**< Address of phase shifted sin reference table =RW */
	DSP_INT32    LockInRefSinLenA; /**< Length of RefTab =RO */
	DSP_INT16_P  LockInRefSinTabB; /**< Address of phase shifted sin reference table =RW */
	DSP_INT32    LockInRefSinLenB; /**< Length of RefTab =RO */
	PROBE_VECTOR_P vector_head;  /**< pointer to head of list of probe vectors =RO/SET */
	PROBE_VECTOR_P vector;       /**< pointer to head of list of probe vectors =RO/SET */
	DSP_INT32     ix, iix, lix;         /**< counters =RO */
	DSP_INT32     PRB_ACPhaseA32;
	DSP_INT32     gpio_setting;
	DSP_INT32     gpio_data;       /**< gpio data mirror 32bit */
	DSP_INT32_P   trigger_input;   /**< mapped to trigger signal */
	DSP_INT32     state;           /**< current probe state =RO */
	DSP_INT32     pflg;            /**< process active flag =RO */
#ifdef DSP_CC
} PROBE;
#else
} PROBE_MK3;
#endif
#define MAX_WRITE_PROBE (2*(13))


/** Auto Approach and Slider/Mover Parameters */
typedef struct{
	DSP_INT32     start;           /**< Initiate =WO */
	DSP_INT32     stop;            /**< Cancel   =WO */
	DSP_INT32     mover_mode;      /**< Mover mode, one off AAP_MOVER_..., see above */
	DSP_INT32     wave_out_channel[2];  /** wave[X/Y]: coarse motion signals -- output channel mapping INDEX 0..7, data is computed via out[8,9] mixing */
	DSP_INT32     piezo_steps;     /**< max number of repetitions */
	DSP_INT32     n_wait;          /**< delay inbetween cycels */
	DSP_INT32     u_piezo_max;     /**< Amplitude, Peak or length of waveform table */
	DSP_INT32     u_piezo_amp;     /**< Amplitude, Peak2Peak or phase offset for IW mode*/
	DSP_INT32     piezo_speed;     /**< Speed */
        DSP_UINT32    n_wait_fin;      /**< # cycles to wait and check (run FB) before finish auto app. */
        DSP_UINT32    fin_cnt;         /**< tmp count for auto fin. */
 	DSP_INT32     mv_count;        /**< "time axis" */
	DSP_INT32     mv_dir;          /**< "time direction" */
	DSP_INT32     mv_step_count;   /**< step counter */
	DSP_INT32     u_piezo;         /**< Current Piezo Voltage */
	DSP_INT32     step_next;       /**< used for motor (optional) */
	DSP_INT32     tip_mode;        /**< Tip mode, used by auto approach */
	DSP_INT32     delay_cnt;       /**< Delay count */
	DSP_INT32     ci_mult;         /**< retract speedup factor */
	DSP_INT32     cp, ci;          /**< temporary used */
	DSP_INT32     count_axis[3];   /**< */
	DSP_INT32     pflg;            /**< process active flag =RO */
#ifdef DSP_CC
} AUTOAPPROACH;
#else
} AUTOAPPROACH_MK3;
#endif
#define MAX_WRITE_AUTOAPPROACH (2*11)

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
#define AAP_MOVER_XXOFFSET 0x8000 /**< set this bit if Motor output (X only on X0=Y0=X-signal) is to be used */
#define AAP_MOVER_ZSCANADD 0x0800 /**< set this bit if signal should be added to Z */
#define AAP_MOVER_IWMODE   0x0100 /**< InchWorm drive mode (fwd/rev via _XP/XM on X0/Y0), 120deg phase */

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
	DSP_INT32    start;           /**< Initiate =WO */
	DSP_INT32    stop;            /**< Cancel   =WO */
	DSP_INT32    duration;        /**< Puls duration count =WR */
	DSP_INT32    period;          /**< Puls period count =WR */
	DSP_INT32    number;          /**< number of pulses =WR */
	DSP_UINT16   on_bcode;        /**< IO value at "on", while "duration" =WR */
	DSP_UINT16   off_bcode;       /**< IO value at "off", while remaining time of period =WR */
	DSP_UINT16   reset_bcode;     /**< IO value at end/cancel =WR */
	DSP_UINT16   dummy_align;     /**< IO value at end/cancel =WR */
	DSP_INT32    io_address;      /**< IO address to use =WR */
	DSP_INT32    i_per;           /**< on counter =RO */
	DSP_INT32    i_rep;           /**< repeat counter =RO */
	DSP_INT32    pflg;            /**< process active flag =RO */
#ifdef DSP_CC
} CR_OUT_PULSE;
#else
} CR_OUT_PULSE_MK3;
#endif
#define MAX_WRITE_CR_OUT_PULS 14

/**
 * CR_generic_io (GPIO) and Counter (Rate-Meter mode) control for FPGA
 *
 */
typedef struct{
	DSP_INT32    start;           /**<  0* Initiate =WO */
	DSP_INT32    stop;            /**<  1* Cancel   =WO */
	DSP_UINT16   gpio_direction_bits;  /**<  2* GPIO direct bits (0=input, 1=output) =WO */
	DSP_UINT16   gpio_data_in;    /**<  3* GPIO Data (16 bit) care only about in-bits  =RO */
	DSP_UINT16   gpio_data_out;   /**<  4* GPIO Data (16 bit) care only about out-bits =WO */
	DSP_UINT16   gpio_subsample;  /**<  4* sub sample number, need to slow down GPIO access to preven DMA timing problems */
	DSP_UINT32   gatetime_l_0;    /**<  5* Gatetime 0 lower 16 bits in dataprocess cycle units =RW */
	DSP_UINT32   gatetime_h_0;    /**<  6* Gatetime 0 upper 16 bits */
	DSP_UINT32   gatetime_1;      /**<  7* Gatetime 1 only 16 bits (<800ms max @ 75kHz data process) */
        DSP_UINT32   count_0;         /**<  8* most recent count 0 -- 32bit */
        DSP_UINT32   count_1;         /**< 10* most recent count 1 -- 32bit */
        DSP_UINT32   peak_count_0;    /**< 12* peak count 0 -- 32bit */
        DSP_UINT32   peak_count_1;    /**< 14* peak count 1 -- 32bit */
        DSP_UINT32   peak_holdtime;   /**< 16* *** not used yet ***=> reset via write 0 on peak counts -- 32bit */
	DSP_INT32    pflg;            /**< 18* rate-meter mode process active flag =RO */
#ifdef DSP_CC
} CR_GENERIC_IO;
#else
} CR_GENERIC_IO_MK3;
#endif
#define MAX_WRITE_CR_GENERIC_IO 14

/** signals monitor */
#define NUM_MONITOR_SIGNALS 30
#define MAX_WRITE_SIGNAL_MONITOR (2*(4+NUM_MONITOR_SIGNALS))
typedef struct{
	DSP_INT32     mindex;           /**< monitor signal index to be set to adress by signald_id -- 0...NUM_MONITOR_SIGNALS-1, or any valid MODULE_SIGNAL_ID.  -1: ready/no action =WO */
	DSP_INT32     signal_id;        /**< valid signal identification -- 0...#siganls configured in signal list OR set this to -1 to query signal at mindex  =WO */
	DSP_INT32_P   act_address_input_set;  /**RO (W=0 OK)  verification of actual action taken -- actual input addresss of pointer manipulated =WO */
	DSP_INT32_P   act_address_signal;     /**RO (W=0 OK)  verification of actual signal address configure for input =WO */
	DSP_INT32_P   signal_p[NUM_MONITOR_SIGNALS];  /**< Monitor Signal Pointers */
	DSP_INT32     signal[NUM_MONITOR_SIGNALS];    /**< Monitor Signal Data */
	DSP_INT32     pflg;            /**< process active flag =RO */
#ifdef DSP_CC
} SIGNAL_MONITOR;
#else
} SIGNAL_MONITOR_MK3;
/* define signal management header only */
typedef struct{
	DSP_INT32     mindex;           /**< monitor signal index to be set to adress by signald_id -- 0...NUM_MONITOR_SIGNALS-1, or any valid MODULE_SIGNAL_ID.  -1: ready/no action =WO */
	DSP_INT32     signal_id;        /**< valid signal identification -- 0...#siganls configured in signal list OR set this to -1 to query signal at mindex  =WO */
	DSP_INT32_P   act_address_input_set;  /**RO (W=0 OK)  verification of actual action taken -- actual input addresss of pointer manipulated =WO */
	DSP_INT32_P   act_address_signal;     /**RO (W=0 OK)  verification of actual signal address configure for input =WO */
} SIGNAL_MANAGE;	
#endif

#if 0
/** DUMMY Control -- Template Only */
#define MAX_WRITE_DUMMYCTRL (2*3)
typedef struct{
	DSP_INT32     start;           /**< Initiate =WO */
	DSP_INT32     stop;            /**< Cancel   =WO */
	DSP_INT32     dummyparameter;  /**< ... =WR */
	DSP_INT32     pflg;            /**< process active flag =RO */
#ifdef DSP_CC
} DUMMYCTRL;
#else
} DUMMYCTRL_MK3;
#endif
#endif

// generic signal output mixer module
typedef struct{
        DSP_INT32_P  p;                /** data for out request for OUT[0..7], like bias/motor updates */
        DSP_INT32_P  add_a_p;          /** add data A to OUT[0..7]  */
        DSP_INT32_P  sub_b_p;          /** sub data B from OUT[0..7] */
        DSP_INT32_P  smac_a_fac_q15_p; /** SMAC: multiply A fac q15 and sat add data on OUT[0..7] -- SMAC mode if this ptr NON-ZERO */
        DSP_INT32_P  smac_b_fac_q15_p; /** SMAC: multiply B fac q15 and sat add data on OUT[0..7] -- SMAC mode if this ptr NON-ZERO */
	DSP_INT32    s;                /** final out data in full 32bit before HR out */
} OUT_MIXER;

/** Control, Analog and Counter management Module  */
typedef struct{
/**  --- controls, generated, constant signals **/
	DSP_INT32    bias, motor;   /**< RW life mapped, these are commanded by Gxsm HwI **/
/**  --- DA out processing **/
	OUT_MIXER    out[10];        /** output module for each channel 0..7 and 8,9 special mapping for wave-X/Y output mode */
/**  --- IN signal processing **/
	DSP_INT32_P  avg_input;
	DSP_INT32    avg_signal;
	DSP_INT32    rms_signal;
/**  --- **/
	DSP_INT32    bias_adjust;   /** RO smoothly following bias **/
	DSP_INT32    noise, vnull;  /** RO noise generator, null value **/
	DSP_INT32    wave[2];       /** wave[X/Y]: coarse motion signals */
/**  --- AD input mirror 32bit **/
        DSP_INT32    in[8];         /** HR data IN[0..7] (Q15.16) after IIR processing, there used as Q15.0 */
        DSP_INT32_P  diff_in_p[4];  /** ==ptr to analog.vnull or pointer in[0..7] data to be subtracted from channel to create up to 4 digital differential inputs like [0-4], [1-5], [2-6], [3-7] **/
/**  ----------------------- FPGA extended and gated counter[2] -- auto gated on sampling speed/point interval */
        DSP_INT32    counter[2];    /** [] counts */
        DSP_INT32    gate_time[2];  /** [] number of summing cycles */
	DSP_INT32    debug[8];      /** scratch variabel space */
#ifdef DSP_CC
} ANALOG_VALUES;
#else
} ANALOG_VALUES_MK3;
#endif
#define MAX_WRITE_ANALOG_CONTROL (2*2)
#define MAX_WRITE_ANALOG_FULL (2*(2+8*5+1))


#define DATAFIFO_LENGTH (1<<12) // in 16 bit words
#define DATAFIFO_MASK   ((DATAFIFO_LENGTH)-1)
#define DATAFIFO_MASKL  ((DATAFIFO_LENGTH>>1)-1)

/** DataFifo: data streaming control and buffer (Area Scan) */
typedef struct{
	DSP_INT32       r_position;	  /**< 0 read pos (Gxsm) (WO) by host =WO */
	DSP_INT32       w_position;   /**< 1 write pos (DSP) (RO) by host =RO */
	DSP_INT32       fill;	  /**< 2 filled indicator = max num to read =RO */
	DSP_INT32	stall;	  /**< 3 number of fifo stalls =RO */
	DSP_INT32       length;	  /**< 4 constant, buffer size =RO */
	DSP_INT32       mode;         /**< 5 data mode (short/long) =RO */
	DSP_UINT32      buffer_ul[DATAFIFO_LENGTH>>1]; /**< fifo buffer (RO) by host =RO */
#ifdef DSP_CC
} DATA_FIFO;
#else
} DATA_FIFO_MK3;
#endif
#define MAX_WRITE_DATA_FIFO (2*1)
#define MAX_WRITE_DATA_FIFO_SETUP (2*4)


// must match size declaration in CMD_SR3.cmd linker file -- attention def in CMD file is in BYTES, this here is in WORD size!!!
#define	EXTERN_PROBEDATAFIFO_LENGTH 0x1780   // 0x2800 BYTES
#define EXTERN_PROBEDATA_MAX_LEFT   0x0040


/** DataFifo: data streaming control and buffer (Vector Probe) */
typedef struct{
	DSP_UINT32    r_position;	  /**< read pos (Gxsm) (always in word size) (WO) by host =WO */
	DSP_UINT32    w_position;   /**< write pos (DSP) (always in word size) (RO) by host =RO */
	DSP_UINT32    current_section_head_position; /**< resync/verify and status info =RO */
	DSP_UINT32    current_section_index; /**< index of current section =RO */
	DSP_UINT32    fill;	      /**< filled indicator = max num to read =RO */
	DSP_UINT32    stall;	      /**< number of fifo stalls =RO */
	DSP_UINT32    length;	      /**< constant, buffer size =RO */
	DSP_INT16_P   buffer_base; /**< pointer to external memory, buffer start =RO */
	DSP_INT16_P   buffer_w;    /**< pointer to external memory =RO */
	DSP_INT32_P   buffer_l;
#ifdef DSP_CC
} DATA_FIFO_EXTERN;
#else
} DATA_FIFO_EXTERN_MK3;
#endif
#define MAX_WRITE_DATA_FIFO_EXTERN (2*1)
#define MAX_WRITE_DATA_FIFO_EXTERN_SETUP (2*6)

/** PLL Data Magics */
typedef struct{
	DSP_INT32    start;           /**<  0* 1: Re-Initiat PLL Sine, ... =WO */
	DSP_INT32    stop;            /**<  1* -- =WO */
	DSP_INT32    pflg;            /**<  0* PLL process flag =WO */

// Pointers for the acsiquition
	DSP_INT32_P   *pSignal1; 
	DSP_INT32_P   *pSignal2; 
	DSP_INT32_P   Signal1; 
	DSP_INT32_P   Signal2; 
	
// Block length
	DSP_INT32_P   blcklen; // Max MaxSizeSigBuf-1 
	
// Sine variables
	DSP_INT32_P deltaReT; 
	DSP_INT32_P deltaImT; 
	DSP_INT32_P volumeSine; 
 
// Phase/Amp detector time constant 
	DSP_INT32_P Tau_PAC;

// Phase corrector variables
	DSP_INT32_P PI_Phase_Out;
	DSP_INT32_P pcoef_Phase;
	DSP_INT32_P icoef_Phase;
	DSP_INT32_P errorPI_Phase;
	DSP_INT64_P memI_Phase;
	DSP_INT32_P setpoint_Phase;
	DSP_INT32_P ctrlmode_Phase;
	DSP_INT64_P corrmin_Phase;
	DSP_INT64_P corrmax_Phase;
	
// Min/Max for the output signals
	DSP_INT32_P ClipMin;
	DSP_INT32_P ClipMax;

// Amp corrector variables
	DSP_INT32_P ctrlmode_Amp;
	DSP_INT64_P corrmin_Amp;
	DSP_INT64_P corrmax_Amp;
	DSP_INT32_P setpoint_Amp;
	DSP_INT32_P pcoef_Amp;
	DSP_INT32_P icoef_Amp;
	DSP_INT32_P errorPI_Amp;
	DSP_INT64_P memI_Amp;
	
// LP filter selections
	DSP_INT32_P LPSelect_Phase;
	DSP_INT32_P LPSelect_Amp; 

// pointers to live signals
	DSP_INT32_P InputFiltered;       // Resonator Output Signal
	DSP_INT32_P SineOut0;            // Excitation Signal
	DSP_INT32_P phase;               // Resonator Phase (no LP filter)
	DSP_INT32_P PI_Phase_OutMonitor; // Excitation Freq. (no LP filter)
	DSP_INT32_P amp_estimation;      // Resonator Amp. (no LP filter)
	DSP_INT32_P volumeSineMonitor;   // Excitation Amp. (no LP filter) (same as volumeSine)
	DSP_INT32_P Filter64Out;         // int[4] : 
	// [0] Excitation Freq. (with LP filter)
	// [1] Resonator Phase (with LP filter)
	// [2] Resonator Amp. (with LP filter)
	// [3] Excitation Amp. (with LP filter)

// short Mode2F 0: mode normal (F on analog out/F on the Phase detector) 1: mode 2F (F on analog out/2F on the Phase detector)
	DSP_INT16_P Mode2F; 

	DSP_INT32 pac_lib_status;
} PLL_LOOKUP;

// only used for DSP_EMU to align data in shared memory block
typedef struct{
        DSP_INT32 Signal1[MaxSizeSigBuf];
        DSP_INT32 Signal2[MaxSizeSigBuf];
	
        // Pointers for the acsiquition
        
        DSP_INT32 *pSignal1; 
        DSP_INT32 *pSignal2; 
	
        // Block length

        DSP_INT32 blcklen; // Max MaxSizeSigBuf-1 
	
        // Sine variables

        DSP_INT32 deltaReT; 
        DSP_INT32 deltaImT; 
        DSP_INT32 volumeSine; 
 
        // Phase/Amp detector time constant 

        DSP_INT32 Tau_PAC;

        // Phase corrector variables
	
        DSP_INT32 PI_Phase_Out; 
        DSP_INT32 pcoef_Phase;
        DSP_INT32 icoef_Phase;
        DSP_INT32 errorPI_Phase; 
        DSP_INT64 memI_Phase; 
        DSP_INT32 setpoint_Phase; 
        DSP_INT32 ctrlmode_Phase; 
        DSP_INT64 corrmin_Phase; 
        DSP_INT64 corrmax_Phase; 
	
        // Min/Max for the output signals
	
        DSP_INT32 ClipMin[4]; 
        DSP_INT32 ClipMax[4]; 

        // Amp corrector variables
	
        DSP_INT32 ctrlmode_Amp;
        DSP_INT64 corrmin_Amp; 
        DSP_INT64 corrmax_Amp; 
        DSP_INT32 setpoint_Amp;
        DSP_INT32 pcoef_Amp;
        DSP_INT32 icoef_Amp;
        DSP_INT32 errorPI_Amp;
        DSP_INT64 memI_Amp;
	
        // LP filter selections
	
        DSP_INT32 LPSelect_Phase;
        DSP_INT32 LPSelect_Amp; 

        // Other variables

        DSP_INT32 outmix_display[4]; // Must be set at 7 to bypass the PLL output generation
        DSP_INT32 InputFiltered;
        DSP_INT32 SineOut0; 
        DSP_INT32 phase;
        DSP_INT32 amp_estimation;
        DSP_INT32 Filter64Out[4];

        // Mode 2F variable

        DSP_INT16 Mode2F; // 0: mode normal (F on analog out/F on the Phase detector) 1: mode 2F (F on analog out/2F on the Phase detector)

        // Output of the sine generator (F and 2F)

        DSP_INT32 cr1; // Sine F (RE)
        DSP_INT32 ci1; // Sine F (IM)
        DSP_INT32 cr2; // Sine 2F (RE)
        DSP_INT32 ci2; // Sine 2F (IM)

} PAC_PLL_MEMORY;



/** SPM Signal/Data Magics **/
typedef struct{
	DSP_INT32_P p;  // pointer in usigned int (32 bit)
#ifndef DSP_CC
	guint dim;
	const gchar *label;
	const gchar *unit;
	double scale; // factor to multipy with to get base unit value representation
	const gchar *module;
#endif
} DSP_SIG;

#ifdef DSP_CC
# define MAKE_DSP_SIG_ENTRY(P,L,U,F,M,D) { P }
# define MAKE_DSP_SIG_ENTRY_VECTOR(DIM,P,L,U,F,M,D) { P }
# define MAKE_DSP_SIG_ENTRY_END(L,U) { (NULL) }
#else
# define MAKE_DSP_SIG_ENTRY(P,L,U,F,M,D)            { 99,   1, L, U, F, M }
# define MAKE_DSP_SIG_ENTRY_VECTOR(DIM,P,L,U,F,M,D) { 99, DIM, L, U, F, M }
# define MAKE_DSP_SIG_ENTRY_END(L,U)                {  0,   0, L, U, 0., "END" }
#endif


#ifdef DSPEMU
#include "SR3PRO_A810Driver.h"
#include "dsp_signals.h"

#define DSP_MEMORY(BLOCK) dspmem->BLOCK
#define DSP_MEMORY_OFF_P(MEMBER) (DSP_INT32*)offsetof(DSP_EMU_INTERNAL_MEMORY, MEMBER)
#define DSP_MEMORY_OFFSET(MEMBER) offsetof(DSP_EMU_INTERNAL_MEMORY, MEMBER)
#define DSP_MEMORY_PAC_OFF(MEMBER) (DSP_INT32*)offsetof(DSP_EMU_INTERNAL_MEMORY, pac_pll_memory.MEMBER)
#define DSP_MEMORY_P(BLOCK) &dspmem->BLOCK
#define DSP_MEMORY_PAC(BLOCK) dspmem->pac_pll_memory.BLOCK

// so far used only for DSP EMU for data alignment in mmapped buffer
typedef struct{
        SPM_MAGIC_DATA_LOCATIONS magic;
        SPM_STATEMACHINE state;
        A810_CONFIG a810_config;
        SERVO z_servo;
        SERVO m_servo;
        SCO sco_s[2];
        ANALOG_VALUES analog;
        FEEDBACK_MIXER feedback_mixer;
        MOVE_OFFSET move;
        AREA_SCAN scan;
        SCAN_EVENT_TRIGGER scan_event_trigger;
        PROBE probe;
        AUTOAPPROACH autoapp;
        CR_OUT_PULSE CR_out_pulse;
        CR_GENERIC_IO CR_generic_io;
        SIGNAL_MONITOR sig_mon;
        DATA_FIFO datafifo;
        DATA_FIFO_EXTERN probe_datafifo;
        PLL_LOOKUP PLL_lookup;
        DSP_SIG dsp_sig;
        DSP_INT32 VP_sec_end_buffer[10*8]; // 8x section + 8 values
        DSP_INT32 user_input_signal_array[32];
        DSP_INT32 sigma_delta_hr_mask[8*8]; // copy!
        struct iobuf_rec iobuf;
        DSP_INT16 QEP_cnt[2];
        DSP_INT32 GPIO_Data[4];
        DSP_INT32 GPIO_DataDir[4];
        PAC_PLL_MEMORY pac_pll_memory;
        DSP_SIG dsp_signal_lookup[NUM_SIGNALS];
        PROBE_VECTOR prbvh[1000];
        DSP_INT16 prbdf[100000];
        DSP_INT32 lock;
} DSP_EMU_INTERNAL_MEMORY;

extern DSP_EMU_INTERNAL_MEMORY *dspmem;

#else
#define DSP_MEMORY(BLOCK) BLOCK
#define DSP_MEMORY_OFF_P(MEMBER) &MEMBER
#define DSP_MEMORY_OFFSET(MEMBER) (DSP_MEMOFFSET)&MEMBER
#define DSP_MEMORY_PAC_OFF(MEMBER) &MEMBER
#define DSP_MEMORY_P(BLOCK) &BLOCK
#define DSP_MEMORY_PAC(BLOCK) BLOCK
#endif

#endif
