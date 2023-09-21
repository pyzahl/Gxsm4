/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001,2002,2003 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * additional features: Andreas Klust <klust@users.sf.net>
 * WWW Home: http://gxsm.sf.net
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


#ifndef __RPSPMC_PACPLL_HWI_STRUCTS_H
#define __RPSPMC_PACPLL_HWI_STRUCTS_H

#define UTF8_DEGREE    "\302\260"
#define UTF8_MU        "\302\265"
#define UTF8_ANGSTROEM "\303\205"

#define SERVO_SETPT 0
#define SERVO_CP    1
#define SERVO_CI    2

#define X_SOURCE_MSK 0x10000000 // select for X-mapping
#define P_SOURCE_MSK 0x20000000 // select for plotting
#define A_SOURCE_MSK 0x40000000 // select for Avg plotting
#define S_SOURCE_MSK 0x80000000 // select for Sec plotting

#define NUM_VECTOR_SIGNALS 8

// G_ARRAY EXPANDED PROBE DATA
#define NUM_PROBEDATA_ARRAYS 27

// HEADER (FIXED BLOCK)
#define PROBEDATA_ARRAY_INDEX 0 // Array [0] holds the probe index over all sections
#define PROBEDATA_ARRAY_TIME  1 // Array [1] holds the time
#define PROBEDATA_ARRAY_AA    2 // Array [2] holds X-Offset
#define PROBEDATA_ARRAY_BB    3 // Array [3] holds Y-Offset
#define PROBEDATA_ARRAY_PHI   4 // Array [4] holds Z-Offset
#define PROBEDATA_ARRAY_XS    5 // Array [5] holds X-Scan
#define PROBEDATA_ARRAY_YS    6 // Array [6] holds Y-Scan
#define PROBEDATA_ARRAY_ZS    7 // Array [7] holds Z-Scan
#define PROBEDATA_ARRAY_U     8 // Array [8] holds U (Bias)
#define PROBEDATA_ARRAY_SEC   9 // Array [9] holds Section Index

#define NUM_PV_HEADER_SIGNALS (PROBEDATA_ARRAY_SEC+1)

// ARB DATA SIGNALS
#define PROBEDATA_ARRAY_S1    10 // Array [10] holds ZMON (AIC5 out) [[AIC5OUT_ZMON]]
#define PROBEDATA_ARRAY_S2    11 // Array [11] holds UMON (AIC6 out)
#define PROBEDATA_ARRAY_S3    12 // Array [12] holds FBS (Feedback Source, i.e. I, df, force, ...)
#define PROBEDATA_ARRAY_S4    13 // Array [13] holds AIC0 in
#define PROBEDATA_ARRAY_S5    14 // Array [14] holds AIC1 in
#define PROBEDATA_ARRAY_S6    15 // Array [15] holds AIC2 in
#define PROBEDATA_ARRAY_S7    16 // Array [16] holds AIC3 in
#define PROBEDATA_ARRAY_S8    17 // Array [17] holds AIC4 in
#define PROBEDATA_ARRAY_S9    18 // Array [18] holds AIC6 in (not used yet)
#define PROBEDATA_ARRAY_S10   19 // Array [19] holds AIC7 in (not used yet)
#define PROBEDATA_ARRAY_S11   20 // Array [20] holds LockIn0st
#define PROBEDATA_ARRAY_S12   21 // Array [21] holds LockIn1st
#define PROBEDATA_ARRAY_S13   22 // Array [22] holds LockIn22st
#define PROBEDATA_ARRAY_S14   23 // Array [23] holds LockIn1st
#define PROBEDATA_ARRAY_S15   24 // Array [24] holds LockIn22st
// Block Management
#define PROBEDATA_ARRAY_COUNT 25 // Array [25] holds Count
#define PROBEDATA_ARRAY_BLOCK 26 // Array [26] holds Block start index (hold start index for every section) 

#define PROBEDATA_ARRAY_END   PROBEDATA_ARRAY_BLOCK // last element number

#define MAX_NUM_CHANNELS (PROBEDATA_ARRAY_END-1)  // 26

#define NUM_PV_DATA_SIGNALS (PROBEDATA_ARRAY_END-PROBEDATA_ARRAY_S1)


// NEW: generalized signals

typedef struct {
        guint32     mask;   // signal source mask, or signal id for swappable
        const gchar *label;  // label for signal | NULL for flex signal life swappable
        const gchar *description; // signal description
        const gchar *unit;  // gxsm signal unit symbolic id
        const gchar *unit_sym;  // gxsm signal unit symbol
        double scale_factor; // multiplier for raw value to unit conversion
        int garr_index; // expanded garray index lookup to store data
} SOURCE_SIGNAL_DEF;


// borrowed from MK3 DSP... TDB

#define NUM_SIGNALS 99
#define NUM_SIGNALS_UNIVERSAL 200

typedef struct { gint32 id; const gchar* name; guint32 sigptr; } MOD_INPUT;

/**
# * Signal Input ID's for safe DSP based signal pointer adjustment methods
#**/

#define DSP_SIGNAL_MONITOR_INPUT_BASE_ID   0x0000

#define DSP_SIGNAL_BASE_BLOCK_A_ID         0x1000
#define DSP_SIGNAL_Z_SERVO_INPUT_ID       (DSP_SIGNAL_BASE_BLOCK_A_ID+1)
#define DSP_SIGNAL_M_SERVO_INPUT_ID       (DSP_SIGNAL_Z_SERVO_INPUT_ID+1)
#define DSP_SIGNAL_MIXER0_INPUT_ID        (DSP_SIGNAL_M_SERVO_INPUT_ID+1)
#define DSP_SIGNAL_MIXER1_INPUT_ID        (DSP_SIGNAL_MIXER0_INPUT_ID+1)
#define DSP_SIGNAL_MIXER2_INPUT_ID        (DSP_SIGNAL_MIXER1_INPUT_ID+1)
#define DSP_SIGNAL_MIXER3_INPUT_ID        (DSP_SIGNAL_MIXER2_INPUT_ID+1)
#define DSP_SIGNAL_DIFF_IN0_ID            (DSP_SIGNAL_MIXER3_INPUT_ID+1)
#define DSP_SIGNAL_DIFF_IN1_ID            (DSP_SIGNAL_DIFF_IN0_ID+1)
#define DSP_SIGNAL_DIFF_IN2_ID            (DSP_SIGNAL_DIFF_IN1_ID+1)
#define DSP_SIGNAL_DIFF_IN3_ID            (DSP_SIGNAL_DIFF_IN2_ID+1)
#define DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID   (DSP_SIGNAL_DIFF_IN3_ID+1)
#define DSP_SIGNAL_SCAN_CHANNEL_MAP1_ID   (DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID+1)
#define DSP_SIGNAL_SCAN_CHANNEL_MAP2_ID   (DSP_SIGNAL_SCAN_CHANNEL_MAP1_ID+1)
#define DSP_SIGNAL_SCAN_CHANNEL_MAP3_ID   (DSP_SIGNAL_SCAN_CHANNEL_MAP2_ID+1)

#define DSP_SIGNAL_BASE_BLOCK_B_ID         0x2000
#define DSP_SIGNAL_LOCKIN_A_INPUT_ID      (DSP_SIGNAL_BASE_BLOCK_B_ID+1)
#define DSP_SIGNAL_LOCKIN_B_INPUT_ID      (DSP_SIGNAL_LOCKIN_A_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE0_INPUT_ID     (DSP_SIGNAL_LOCKIN_B_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE1_INPUT_ID     (DSP_SIGNAL_VECPROBE0_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE2_INPUT_ID     (DSP_SIGNAL_VECPROBE1_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE3_INPUT_ID     (DSP_SIGNAL_VECPROBE2_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE0_CONTROL_ID   (DSP_SIGNAL_VECPROBE3_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE1_CONTROL_ID   (DSP_SIGNAL_VECPROBE0_CONTROL_ID+1)
#define DSP_SIGNAL_VECPROBE2_CONTROL_ID   (DSP_SIGNAL_VECPROBE1_CONTROL_ID+1)
#define DSP_SIGNAL_VECPROBE3_CONTROL_ID   (DSP_SIGNAL_VECPROBE2_CONTROL_ID+1)
#define DSP_SIGNAL_VECPROBE_TRIGGER_INPUT_ID (DSP_SIGNAL_VECPROBE3_CONTROL_ID+1)
#define DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID (DSP_SIGNAL_VECPROBE_TRIGGER_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE_LIMITER_UP_INPUT_ID (DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE_LIMITER_DN_INPUT_ID (DSP_SIGNAL_VECPROBE_LIMITER_UP_INPUT_ID+1)
#define DSP_SIGNAL_VECPROBE_TRACKER_INPUT_ID (DSP_SIGNAL_VECPROBE_LIMITER_DN_INPUT_ID+1)

#define DSP_SIGNAL_BASE_BLOCK_C_ID             0x3000
#define DSP_SIGNAL_OUTMIX_CH0_INPUT_ID        (DSP_SIGNAL_BASE_BLOCK_C_ID+1)
#define DSP_SIGNAL_OUTMIX_CH0_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH0_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH0_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH0_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH0_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH0_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH0_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH0_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH1_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH0_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH1_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH1_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH1_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH1_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH1_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH1_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH1_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH1_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH2_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH1_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH2_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH2_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH2_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH2_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH2_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH2_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH2_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH2_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH3_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH2_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH3_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH3_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH3_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH3_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH3_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH3_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH3_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH3_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH4_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH3_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH4_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH4_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH4_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH4_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH4_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH4_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH4_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH4_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH5_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH4_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH5_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH5_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH5_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH5_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH5_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH5_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH5_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH5_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH6_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH5_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH6_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH6_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH6_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH6_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH6_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH6_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH6_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH6_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH7_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH6_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH7_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH7_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH7_SUB_B_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH7_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH7_SMAC_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH7_SUB_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH7_SMAC_B_INPUT_ID (DSP_SIGNAL_OUTMIX_CH7_SMAC_A_INPUT_ID+1)

#define DSP_SIGNAL_OUTMIX_CH8_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH7_SMAC_B_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH8_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH8_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH9_INPUT_ID        (DSP_SIGNAL_OUTMIX_CH8_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH9_ADD_A_INPUT_ID  (DSP_SIGNAL_OUTMIX_CH9_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH10_INPUT_ID       (DSP_SIGNAL_OUTMIX_CH9_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH10_ADD_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH10_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH11_INPUT_ID       (DSP_SIGNAL_OUTMIX_CH10_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH11_ADD_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH11_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH12_INPUT_ID       (DSP_SIGNAL_OUTMIX_CH11_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH12_ADD_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH12_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH13_INPUT_ID       (DSP_SIGNAL_OUTMIX_CH12_ADD_A_INPUT_ID+1)
#define DSP_SIGNAL_OUTMIX_CH13_ADD_A_INPUT_ID (DSP_SIGNAL_OUTMIX_CH13_INPUT_ID+1)

#define DSP_SIGNAL_BASE_BLOCK_D_ID             0x4000
#define DSP_SIGNAL_ANALOG_AVG_INPUT_ID        (DSP_SIGNAL_BASE_BLOCK_D_ID+1)
#define DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID     (DSP_SIGNAL_ANALOG_AVG_INPUT_ID+1)
#define DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID     (DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID+1)
#define DSP_SIGNAL_SCO1_INPUT_ID               (DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID+1)
#define DSP_SIGNAL_SCO1_AMPLITUDE_INPUT_ID     (DSP_SIGNAL_SCO1_INPUT_ID+1)
#define DSP_SIGNAL_SCO2_INPUT_ID               (DSP_SIGNAL_SCO1_AMPLITUDE_INPUT_ID+1)
#define DSP_SIGNAL_SCO2_AMPLITUDE_INPUT_ID     (DSP_SIGNAL_SCO2_INPUT_ID+1)

#define LAST_INPUT_ID                          DSP_SIGNAL_SCO2_AMPLITUDE_INPUT_ID

#define SIGNAL_INPUT_DISABLED -10



typedef struct{
	guint32 p;  // pointer in usigned int (32 bit)
	guint32 dim;
	const gchar *label;
	const gchar *unit;
	double scale; // factor to multipy with to get base unit value representation
	const gchar *module;
	int index; // actual vector index
} DSP_SIG_UNIVERSAL;

typedef struct{
	gint32    mindex;           /**< monitor signal index to be set to adress by signald_id -- 0...NUM_MONITOR_SIGNALS-1, or any valid MODULE_SIGNAL_ID.  -1: ready/no action =WO */
	gint32    signal_id;        /**< valid signal identification -- 0...#siganls configured in signal list OR set this to -1 to query signal at mindex  =WO */
	guint32   act_address_input_set;  /**RO (W=0 OK)  verification of actual action taken -- actual input addresss of pointer manipulated =WO */
	guint32   act_address_signal;     /**RO (W=0 OK)  verification of actual signal address configure for input =WO */
} SIGNAL_MANAGE;


typedef enum { PV_MODE_NONE, PV_MODE_IV, PV_MODE_FZ, PV_MODE_PL, PV_MODE_LP, PV_MODE_SP, PV_MODE_TS, PV_MODE_GVP, PV_MODE_AC, PV_MODE_AX, PV_MODE_TK, PV_MODE_ABORT } pv_mode;
typedef enum { MAKE_VEC_FLAG_NORMAL=0, MAKE_VEC_FLAG_VHOLD=1, MAKE_VEC_FLAG_RAMP=2, MAKE_VEC_FLAG_END=4 } make_vector_flags;

#define FLAG_FB_ON       0x01 // FB on
#define FLAG_DUAL        0x02 // Dual Data
#define FLAG_SHOW_RAMP   0x04 // show ramp data
#define FLAG_INTEGRATE   0x08 // integrate and normalize date of all AIC data point inbetween

#define FLAG_AUTO_SAVE   0x01 // auto save
#define FLAG_AUTO_PLOT   0x02 // auto plot
#define FLAG_AUTO_GLOCK  0x04 // auto graph lock
#define FLAG_AUTO_RUN_INITSCRIPT  0x08 // auto run init script


/** VP option masks ** MUST MATCH DSP DEFINITIONS ** **/
#define VP_FEEDBACK_HOLD 0x01
#define VP_AIC_INTEGRATE 0x02
#define VP_TRACK_REF     0x04
#define VP_TRACK_UP      0x08
#define VP_TRACK_DN      0x10
#define VP_TRACK_FIN     0x20
#define VP_TRACK_SRC     0xC0
#define VP_LIMITER       0x300 // Limiter ON/OFF flag mask
#define VP_LIMITER_UP    0x100 // Limit if > value
#define VP_LIMITER_DN    0x200 // Limit if < value
#define VP_LIMIT_SRC     0xC0  // Limiter "Value" source code bit mask 0x40+0x80  00: Z (IN0), 01: I (IN1), 10: (IN2), 11: (IN3) // MK2!, Signal Mk3
#define VP_GPIO_MSK        0x00ff0000 // GPIO 8bit mask (lower only)
#define VP_TRIGGER_P       0x01000000 // GPIO/signal trigger flag on pos edge -- release VP on "data & mask" or time end/out of section 
#define VP_TRIGGER_N       0x02000000 // GPIO/signal trigger flag on neg edge -- release VP on "data & mask" or time end/out of section 
#define VP_INITIAL_SET_VEC 0x04000000 // GPIO set/update data -- once per section via statemachine, using idle cycle time for slow IO!!
#define VP_GPIO_READ       0x08000000 // GPIO set/update data -- once per section via statemachine, using idle cycle time for slow IO!!
#define VP_RESET_COUNTER_0 0x10000000
#define VP_RESET_COUNTER_1 0x20000000
#define VP_NODATA_RESERVED 0x80000000


/**
 * Vector Probe Vector
 */
typedef struct{
	gint32    n;             // number of points to do
        double    slew;          // slew rate in points per second
	guint32   srcs;          // SRCS source channel coding
	guint32   options;       // Options, Dig IO, ... not yet all defined
	gint32    repetitions;   // numer of repetitions
        gint32    iloop;
        gint32    ptr_next;      // loop jump relative to vpc
	double    f_du;          // dU (bias) full vector delta in Volts
	double    f_dx;          // dX full vector delta in Volts
	double    f_dy;          // dY full vector delta in Volts
	double    f_dz;          // dZ full vector delta in Volts
	double    f_da;          // dA aux channel A full vector delta in Volts
	double    f_db;          // dB aux channel B full vector delta in Volts
} PROBE_VECTOR_GENERIC;

/**
 * Vector Probe Vector
 */
typedef struct{
        // ==== FPGA STREAM MATCHING DATA SECTION
        guint16   srcs_mask;
        guint16   srcs_mask_vector;
        guint16   index;
        gint32    chNs[16];      // full channel data set -- includes positions. X,Y,Z,U,IN1,IN2,x,x,dF,Exec,Phase,Ampl, LckA, LckB
        gint64    gvp_time;      // GVP time in 1/125MHz, 48bit
        int       number_channels;
        int       ch_lut[16];
        // ======================
        double    dataexpanded[NUM_PV_DATA_SIGNALS];
        gint32    i;             /* GVP i: N-1, ... 0 */
        gint32    ilast;
	gint32    n;             /* number data vectors following in section */
	guint32   srcs;          /* data source coding mask */
        guint32   time;          /* timestamp */
        double    scan_xyz[3];   /* Scan XY (rotated coords in global system, scan aligend), Z = F-feedback + Z-Probe */
        double    bias;          /* Bias */
        gint32    section;       /* VP section count */
        gint32    endmark;
} PROBE_HEADER_POSITIONVECTOR;


#define MM_OFF     0x00  // ------
#define MM_ON      0x01  // ON/OFF
#define MM_LOG     0x02  // LOG/LIN
#define MM_LV_FUZZY   0x04  // FUZZY-LV/NORMAL
#define MM_CZ_FUZZY   0x08  // FUZZY-CZ/NORMAL
#define MM_NEG     0x10  // NEGATE SOURCE (INPUT)

// GUI limit
#define N_GVP_VECTORS 25 //  vectors max total, need a few extra for controls and finish.

#endif
