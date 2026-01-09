/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: rpspmc_pacpll.C
 * ========================================
 * 
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Percy Zahl <zahl@fkp.uni-hannover.de>
 * additional features: Andreas Klust <klust@fkp.uni-hannover.de>
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

/* Please do not change the Begin/End lines of this comment section!
 * this is a LaTeX style section used for auto generation of the PlugIn Manual 
 * Chapter. Add a complete PlugIn documentation inbetween the Begin/End marks!
 * All "% PlugInXXX" commentary tags are mandatory
 * All "% OptPlugInXXX" tags are optional
 * --------------------------------------------------------------------------------
% BeginPlugInDocuSection
% PlugInDocuCaption: RPSPMC PACPLL
% PlugInName: rpspmc_pacpll
% PlugInAuthor: Percy Zahl
% PlugInAuthorEmail: zahl@users.sf.net
% PlugInMenuPath: windows-section RPSPMC PACPLL
RP data streaming

% PlugInDescription

% PlugInUsage

% OptPlugInRefs

% OptPlugInNotes

% OptPlugInHints

% EndPlugInDocuSection
 * -------------------------------------------------------------------------------- 
 */


#include <gtk/gtk.h>
#include <gio/gio.h>
#include <libsoup/soup.h>
#include <zlib.h>
#include <gsl/gsl_rstat.h>

#include "config.h"
#include "plugin.h"

#include "unit.h"
#include "pcs.h"
#include "xsmtypes.h"
#include "glbvars.h"
#include "action_id.h"

#include "gxsm_app.h"
#include "gxsm_window.h"
#include "surface.h"

#include "../common/pyremote.h"

#include "rpspmc_pacpll.h"
#include "rpspmc_gvpmover.h"

#include "plug-ins/control/resonance_fit.h"


// define to enable SHARED MEMOY BLOCK ACTIONS and GXSM.SET/GET functionality
#define ENABLE_SHM_ACTIONS

// Define HwI PlugIn reference name here, this is what is listed later within "Preferenced Dialog"
// i.e. the string selected for "Hardware/Card"!
#define THIS_HWI_PLUGIN_NAME "RPSPMC:SPM"
#define THIS_HWI_PREFIX      "RPSPMC_HwI"

#define GVP_SHIFT_UP  1
#define GVP_SHIFT_DN -1


typedef union {
        struct { unsigned char ch, x, y, z; } s;
        unsigned long   l;
} AmpIndex;

/*
  // DATA SIGNAL MAPPING DEFINED in FPGA STREAM_SRCS RTL module:
  // ========================================================================
  // from RPSPMC/rtl/axis_bram_stream_srcs.v
    input a2_clk, // double a_clk used for BRAM (125MHz)
    input wire [32-1:0] S_AXIS_ch1s_tdata, // XS      0x0001  X in Scan coords
    input wire [32-1:0] S_AXIS_ch2s_tdata, // YS      0x0002  Y in Scan coords
    input wire [32-1:0] S_AXIS_ch3s_tdata, // ZS      0x0004  Z
    input wire [32-1:0] S_AXIS_ch4s_tdata, // U       0x0008  Bias
    input wire [32-1:0] S_AXIS_ch5s_tdata, // IN1     0x0010  IN1 RP (Signal)
    input wire [32-1:0] S_AXIS_ch6s_tdata, // IN2     0x0020  IN2 RP (Current)
    input wire [32-1:0] S_AXIS_ch7s_tdata, // IN3     0x0040  reserved, N/A at this time
    input wire [32-1:0] S_AXIS_ch8s_tdata, // IN4     0x0080  reserved, N/A at this time
    input wire [32-1:0] S_AXIS_ch9s_tdata, // DFREQ   0x0100  via PACPLL FIR1 ** via transport / decimation selector
    input wire [32-1:0] S_AXIS_chAs_tdata, // EXEC    0x0200  via PACPLL FIR2
    input wire [32-1:0] S_AXIS_chBs_tdata, // PHASE   0x0400  via PACPLL FIR3
    input wire [32-1:0] S_AXIS_chCs_tdata, // AMPL    0x0800  via PACPLL FIR4
    input wire [32-1:0] S_AXIS_chDs_tdata, // LockInA 0x1000  LockIn X (ToDo)
    input wire [32-1:0] S_AXIS_chEs_tdata, // LockInB 0x2000  LocKin R (ToDo)
    // from below
    // gvp_time[32-1: 0]      // TIME  0x4000 // lower 32
    // gvp_time[48-1:32]      // TIME  0x8000 // upper 32 (16 lower only)
    input wire [48-1:0] S_AXIS_gvp_time_tdata,  // time since GVP start in 1/125MHz units
*/

// Masks MUST BE unique **** max # signal: 32  (graphs_matrix[][32] fix size! Unused=uninitialized.)
SOURCE_SIGNAL_DEF rpspmc_source_signals[] = {
        // -- 8 vector generated signals (outputs/mapping) ==> must match: #define NUM_VECTOR_SIGNALS 8 ** OBSOLET???
        //  xxxxSRCS
        // mask,       name/label,  descr, unit, sym, scale, garrindex, scanchpos
        //  ****SRCS lower 32 bits, upper GVP internal/generated
        { 0x01000000, "Index",    " ",  "#",            "#",                  1.0, PROBEDATA_ARRAY_INDEX, 0 },
        { 0x02000000, "Time",     " ", "ms",           "ms",                  1.0, PROBEDATA_ARRAY_TIME,  0 }, // time in ms
        { 0x04000000, "SEC",      " ", "#",             "#",                  1.0, PROBEDATA_ARRAY_SEC,   0 },
        { 0x00100000, "X-Scan",   " ", "AA", UTF8_ANGSTROEM, SPMC_AD5791_to_volts, PROBEDATA_ARRAY_XS, 0 }, // see  RPSPMC_Control::vp_scale_lookup() Life Mapping!!
        { 0x00200000, "Y-Scan",   " ", "AA", UTF8_ANGSTROEM, SPMC_AD5791_to_volts, PROBEDATA_ARRAY_YS, 0 }, // see  RPSPMC_Control::vp_scale_lookup() Life Mapping!!
        { 0x00400000, "Z-Scan",   " ", "AA", UTF8_ANGSTROEM,-SPMC_AD5791_to_volts, PROBEDATA_ARRAY_ZS, 0 }, // see  RPSPMC_Control::vp_scale_lookup() Life Mapping!!
        { 0x00800000, "Bias",     " ", "V",             "V", SPMC_AD5791_to_volts, PROBEDATA_ARRAY_U,  0 }, // see  RPSPMC_Control::vp_scale_lookup() Life Mapping!!
        { 0x08000000, "A",        " ", "V",             "V", SPMC_AD5791_to_volts, PROBEDATA_ARRAY_A,  0 },
        { 0x10000000, "B",        " ", "V",             "V", SPMC_AD5791_to_volts, PROBEDATA_ARRAY_B,  0 },
        { 0x20000000, "AM",       " ", "V",             "V", SPMC_AD5791_to_volts, PROBEDATA_ARRAY_AM, 0 },
        { 0x40000000, "FM",       " ", "Hz",           "Hz", SPMC_AD5791_to_volts, PROBEDATA_ARRAY_FM, 0 },
        // -- GVP measured signals starting from here        <=== to Volt conversion here -- unit sym and scale are custom auto adjusted in .._eventhandling lookup functions as of this mask
        // IF SHIFTING/ADDING SIGNALS, ADJUST in rpspmc_hwi_structs.h: ---->>>   #define SIGNAL_INDEX_ICH0 11   <<<------
        // ===> XS-Mon,... *** SIGNAL_INDEX_ICH0=11 --> rpspmc_source_signals[SIGNAL_INDEX_ICH0] => PROBEDATA_ARRAY_S1 **
        { 0x00000001, "XS-Mon",       " ", "AA", UTF8_ANGSTROEM,                   SPMC_AD5791_to_volts, PROBEDATA_ARRAY_S1,  1 }, // ich= 0 see  RPSPMC_Control::vp_scale_lookup() Life Mapping!!
        { 0x00000002, "YS-Mon",       " ", "AA", UTF8_ANGSTROEM,                   SPMC_AD5791_to_volts, PROBEDATA_ARRAY_S2,  2 }, // see  RPSPMC_Control::vp_scale_lookup() Life Mapping!!
        { 0x00000004, "ZS-Topo",      " ", "AA", UTF8_ANGSTROEM,                  -SPMC_AD5791_to_volts, PROBEDATA_ARRAY_S3,  3 }, // see  RPSPMC_Control::vp_scale_lookup() Life Mapping!!
        { 0x00000008, "Bias-Mon",     " ", "V",             "V",                   SPMC_AD5791_to_volts, PROBEDATA_ARRAY_S4,  4 }, // BiasFac, see  RPSPMC_Control::vp_scale_lookup() Life Mapping!!
        { 0x00000010, "Current",      " ", "nA",           "nA",                   SPMC_RPIN34_to_volts, PROBEDATA_ARRAY_S5,  5 }, // processed tunnel current signal "Volts" here, to nA/pA later!!! ADJUST W MUX!!!
        { 0x00000020, "IN2-RF-FIR",   " ", "V",             "V",                   SPMC_RPIN12_to_volts, PROBEDATA_ARRAY_S6,  6 }, // IN2 RP 125MSPS (RF input, FIR, routable to Z-Servo Control as tunnel current signal)!
        { 0x00000040, "IN3-AD463-24-CHA", " ", "V",         "V",                   SPMC_RPIN34_to_volts, PROBEDATA_ARRAY_S7,  7 }, // IN3 ADC4630-24-A 2MSPS (routable to Z-Servo Control as tunnel current signal)
        { 0x00000080, "IN4-AD463-24-CHB", " ", "V",         "V",                   SPMC_RPIN34_to_volts, PROBEDATA_ARRAY_S8,  8 }, // IN4 ADC4630-24-B 2MSPS
        { 0x00000100, "SWP*00",       " ",  "V",            "V",                                    1.0, PROBEDATA_ARRAY_S9,  9 },  // ich= 8 ** swappable via GVP-SRC-MUX1 ** -- been replaced as set from rpspmc_swappable_signals[]
        { 0x00000200, "SWP*01",        " ", "V",            "V",                                    1.0, PROBEDATA_ARRAY_S10, 10 }, // ich= 9 ** swappable via GVP-SRC-MUX2 ** -- been replaced as set from rpspmc_swappable_signals[]
        { 0x00000400, "SWP*02",       " ",  "V",            "V",                                    1.0, PROBEDATA_ARRAY_S11, 11 }, // ich=10 ** swappable via GVP-SRC-MUX3 ** -- been replaced as set from rpspmc_swappable_signals[]
        { 0x00000800, "SWP*03",       " ",  "V",            "V",                                    1.0, PROBEDATA_ARRAY_S12, 12 }, // ich=11 ** swappable via GVP-SRC-MUX4 ** -- been replaced as set from rpspmc_swappable_signals[]
        { 0x00001000, "SWP*04",       " ",  "V",            "V",                                    1.0, PROBEDATA_ARRAY_S13, 13 }, // ich=12 ** swappable via GVP-SRC-MUX5 ** -- been replaced as set from rpspmc_swappable_signals[]
        { 0x00002000, "SWP*05",       " ",  "V",            "V",                                    1.0, PROBEDATA_ARRAY_S14, 14 }, // ich=13 ** swappable via GVP-SRC-MUX6 ** -- been replaced as set from rpspmc_swappable_signals[]
        { 0x0000C000, "Time-Mon",     " ", "ms",           "ms",                                    1.0, PROBEDATA_ARRAY_S15, 15 }, // time in ms [11]
        //{ 0x00000000, "--",           " ", "V",             "V",                                    1.0, PROBEDATA_ARRAY_S15,   -1 }, // -- DUMMY SO FAR
        //{ 0x00000000, "--",           " ", "V",             "V",                                    1.0, PROBEDATA_ARRAY_COUNT, -1 }, // -- DUMMY SO FAR
        { 0x80000000, "BlockI",       " ", "i#",           "i#",                                    1.0, PROBEDATA_ARRAY_BLOCK, -1 }, // MUST BE ALWAYS LAST AND IN HERE!! END MARK.
        { 0x00000000, NULL, NULL, NULL, NULL, 0.0, 0 }
};

// so far fixed to swappable 4 signals as of GUI design!
SOURCE_SIGNAL_DEF rpspmc_swappable_signals[] = {                                                                 // DEFAULT MUX MAP, 16 signals max 
        //  SIGNAL #  Name               Units.... Scale                                         DEFAULT ASSIGN
        { 0x00000000, "dFrequency",  " ", "Hz",  "Hz", (125e6/((1L<<RP_FPGA_QFREQ)-1)),               0, -1 },   // dFREQ via PACPLL FIR_CH2 ** via transport / decimation selector 
        { 0x00000001, "Excitation",  " ", "mV1",   "mV", (1000.0/((1L<<RP_FPGA_QEXEC)-1)),             1, -1 },   // EXEC  via PACPLL FIR_CH4 ** via transport / decimation selector 
        { 0x00000002, "Phase",       " ", "deg", UTF8_DEGREE, (180.0/(M_PI*((1L<<RP_FPGA_QATAN)-1))), 2, -1 },   // PHASE via PACPLL FIR_CH1 ** via transport / decimation selector 
        { 0x00000003, "Amplitude",   " ", "mV1",   "mV", (1000.0/((1L<<RP_FPGA_QSQRT)-1)),             3, -1 },   // AMPL  via PACPLL FIR_CH3 ** via transport / decimation selector 
        { 0x00000004, "dFreq-Control", " ", "mV1", "mV", (1000.0*SPMC_AD5791_to_volts),                4, -1 },   // IR_CH2_DFREQ_CTRL_VAL : can be added to Z-control for true Z AFM mode in freq regulation, or addded to Bias for SQDM mode
                                                                                                                 // *** still assuming +/-10V range in PAC Control mappted to 5V here
        { 0x00000005, "05-IN1-RF-FBW",     " ", "mV", "mV", 1000*SPMC_RPIN12_to_volts,               -1, -1 },   // IN1 FBW **** IN1 RP 125MSPS (Signal) -- PLL Signal (FBW)
        { 0x00000006, "06-IN1-RF-FIR",     " ", "V", "V", SPMC_RPIN12_to_volts,                      -1, -1 },   // IN1 FIR **** IN1 RP 125MSPS (Signal) -- PLL Signal (FIR)
        { 0x00000007, "07-IN2-RF-FBW",     " ", "V", "V", SPMC_RPIN12_to_volts,                      -1, -1 },   // IN2 FBW
        { 0x00000008, "08-LockIn-Mag",     " ", "mV", "mV", SPMC_RPIN34_to_volts,                       5, -1 },   // LCK-Mag after BiQuad Section 2
        { 0x00000009, "09-LockIn-Phase",   " ", "deg", UTF8_DEGREE, (180.0/(M_PI*((1L<<(RP_FPGA_QATAN+(32-24)))))), 6, -1 },   // LCK-Phase after BiQuand Section2 
        { 0x0000000A, "10-LockIn-Y",       " ", "V", "V", SPMC_RPIN34_to_volts,                      -1, -1 },   // (TEST)
        { 0x0000000B, "11-IN4-FIR",        " ", "V", "V", SPMC_RPIN34_to_volts,                      -1, -1 },   // IN4 FIR
        { 0x0000000C, "12-LockIn-X",       " ", "V", "V", SPMC_RPIN34_to_volts,                      -1, -1 },   // (TEST)
        { 0x0000000D, "13-LockIn-CosRef",  " ", "V", "V", SPMC_RPIN34_to_volts,                      -1, -1 },   // ** SD-Ref ** dbg
        { 0x0000000E, "14-LockIn-Mag-pass"," ", "V", "V", SPMC_RPIN34_to_volts,                      -1, -1 },   // LCK-Mag (sqrt(x^2+y^2)) raw (no filter)
        { 0x0000000F, "15-LockIn-S-AC",    " ", "V", "V", SPMC_RPIN34_to_volts,                      -1, -1 },   // LCK-Mag after BiQuad Stage 1
        { 0x00000010, "16-GVP-A",          " ", "V", "V", SPMC_AD5791_to_volts,                      -1, -1 },   // GVP-A
        { 0x00000011, "17-GVP-B",          " ", "V", "V", SPMC_AD5791_to_volts,                      -1, -1 },   // GVP-B
        { 0x00000012, "18-GVP-AM",         " ", "V", "V", SPMC_AD5791_to_volts,                      -1, -1 },   // GVP-AM
        { 0x00000013, "19-GVP-FM",         " ", "V", "V", SPMC_AD5791_to_volts,                      -1, -1 },   // GVP-FM
        { 0x00000014, "20-Z-OUT",          " ", "B", "B", 1.0/(1<<12),                               -1, -1 },   // Z-DAC out raw value signed 20bit coming top aligned in " 32 >> 12 "
        { 0x00000015, "21-ZS-BQ-Out",      " ", "nA", "nA", SPMC_RPIN34_to_volts,                    -1, -1 },   // Auto OverSampled -- processed tunnel current signal "Volts" here, to nA/pA later!!! ADJUST W MUX!!!
        { 0x00000016, "22-LockIn-S-DC",    " ", "V",  "V",  SPMC_RPIN34_to_volts,                    -1, -1 },   // Auto OverSampled -- IN4 ADC4630-24-B 2MSPS
        //{ 0x00000010, "X-TestSignal = 0", " ", "V",   "V", (1.0),                         -1, -1 },
        //{ 0x00000011, "X-TestSignal = 1", " ", "V",   "V", (1.0),                         -1, -1 },
        //{ 0x00000012, "X-TestSignal = -1", " ", "V",   "V", (1.0),                        -1, -1 },
        //{ 0x00000013, "X-TestSignal = 99", " ", "V",   "V", (1.0),                        -1, -1 },
        //{ 0x00000014, "X-Test-Signal = -99", " ", "V",   "V", (1.0),                                 -1, -1 },   // TEST (Debug only, disabled on FPGA for production)
        { 0x00000017,  NULL, NULL, NULL, NULL, 0.0, 0, 0 },
        { 0x00000018,  NULL, NULL, NULL, NULL, 0.0, 0, 0 },
        { 0x00000019,  NULL, NULL, NULL, NULL, 0.0, 0, 0 },
        { 0x0000001a,  NULL, NULL, NULL, NULL, 0.0, 0, 0 },
        { 0x0000001b,  NULL, NULL, NULL, NULL, 0.0, 0, 0 },
        { 0x0000001c,  NULL, NULL, NULL, NULL, 0.0, 0, 0 },
        { 0x0000001d,  NULL, NULL, NULL, NULL, 0.0, 0, 0 },
        { 0x0000001e,  NULL, NULL, NULL, NULL, 0.0, 0, 0 },
        { 0x0000001f,  NULL, NULL, NULL, NULL, 0.0, 0, 0 }, // #32 MAX
        { 0x000000ff,  NULL, NULL, NULL, NULL, 0.0, 0, 0 }  // END
};

SOURCE_SIGNAL_DEF modulation_targets[] = {
        //  SIGNAL #  Name               Units.... Scale
        { 0x00000000, "OFF",         " ",  "-",  "-", 0.0,                          0, 0 },
        { 0x00000001, "X-Scan",      " ", "AA", UTF8_ANGSTROEM, 1., 0, 0 }, // scale_factor to get "Volts" or RP base unit for signal
        { 0x00000002, "Y-Scan",      " ", "AA", UTF8_ANGSTROEM, 1., 0, 0 }, //
        { 0x00000003, "Z-Scan",      " ", "AA", UTF8_ANGSTROEM, 1., 0, 0 }, //
        { 0x00000004, "Bias",        " ", "mV",           "mV", 1e-3/BiasFac, 0, 0 },
        { 0x00000005, "A",           " ", "V",             "V", 1., 0, 0 },
        { 0x00000006, "B",           " ", "V",             "V", 1., 0, 0 },
        { 0x00000007, "Bias-Aref",   " ", "mV",           "mV", 1e-3/BiasFac, 0, 0 },
        { 0x00000008, "Filter-Test", " ", "mV",           "mV", 1., 0, 0 },
        { 0x00000016,  NULL, NULL, NULL, NULL, 0.0, 0 }
};


SOURCE_SIGNAL_DEF z_servo_current_source[] = {
        //  SIGNAL #  Name               Units.... Scale (not needed or used from here)
        { 0x00000000, "IN2-RF-FIR",    " ",  "nA",  "nA", SPMC_RPIN12_to_volts, 0, 0 }, // 24.8 Z-Servo internal signal '(32-8) -> x 256
        { 0x00000001, "IN3-1MSPS",     " ",  "nA",  "nA", SPMC_RPIN34_to_volts, 0, 0 }, // 24.8 Z-Servo internal signal '(32-8) -> x 256  ** IN3-AD463-24-CHA-1MSPS
        { 0x00000002, "IN3-1M@FIR1024"," ",  "nA",  "nA", SPMC_RPIN34_to_volts, 0, 0 }, // 24.8 Z-Servo internal signal '(32-8) -> x 256 ** IN3-AD463-24-CHA-1MSPS FIR 1024x dec (SINC) (~1.58 ms / 632 Hz)
        { 0x00000003, "IN3-1M@FIR128" ," ",  "nA",  "nA", SPMC_RPIN34_to_volts, 0, 0 }, // 24.8 Z-Servo internal signal '(32-8) -> x 256 ** IN3-AD463-24-CHA-1MSPS FIRX 128x dec (SINC) (~0.217 ms / 4.6 kHz)
        { 0x00000016,  NULL, NULL, NULL, NULL, 0.0, 0 }
};

SOURCE_SIGNAL_DEF rf_gen_out_dest[] = {
        //  SIGNAL #  Name               Units.... Scale
        { 0x00000000, "PForm-Out1",  " ",  "V",  "V", SPMC_RPIN34_to_volts, 0, 0 },
        { 0x00000001, "RFgen-Out1",  " ",  "V",  "V", SPMC_RPIN12_to_volts, 0, 0 },
        { 0x00000002, "PF+RF-Out1",  " ",  "V",  "V", SPMC_RPIN12_to_volts, 0, 0 },
        { 0x00000003, "PF/2+RF/2-Out1",  " ",  "V",  "V", SPMC_RPIN12_to_volts, 0, 0 },
        { 0x00000016,  NULL, NULL, NULL, NULL, 0.0, 0 }
};



// helper func to assemble mux value from selctions
guint64 __GVP_selection_muxval (int selection[6]) {
        guint64 mux=0;
        for (int i=0; i<6; i++)
                if (selection[i] < 32)
                        mux |= (guint64)(selection[i] & 0x1f)<<(8*i); // regular MUX control
        //g_message ("_GVP_selection_muxval: %016lx", mux);
        return mux;
}

// inverse mux helper, mux val to selection
int __GVP_muxval_selection (guint64 mux, int k) {
        return  (mux >> (8*k)) & 0x1f;
}

// make MUX HVAL for TEST
int __GVP_selection_muxHval (int selection[6]) {
        int muxh=0;
        int axis_test=0;
        int axis_value=0;
        for (int i=0; i<6; i++)
                if (selection[i] >= 32)
                        switch (selection[i]-32){
                        case 0: axis_test = i+1; axis_value = 0; break;
                        case 1: axis_test = i+1; axis_value = 1; break;
                        case 2: axis_test = i+1; axis_value = -1; break;
                        case 3: axis_test = i+1; axis_value = 99; break;
                        case 4: axis_test = i+1; axis_value = -99; break;
                        default: continue;
                        }
        return ((axis_value & 0xfffffff) << 4) | (axis_test&0xf);
}

extern int debug_level;
extern int force_gxsm_defaults;


extern "C++" {
        extern RPspmc_pacpll *rpspmc_pacpll;
        extern GxsmPlugin rpspmc_pacpll_hwi_pi;
}


// Plugin Prototypes - default PlugIn functions
static void rpspmc_pacpll_hwi_init (void); // PlugIn init
static void rpspmc_pacpll_hwi_query (void); // PlugIn "self-install"
static void rpspmc_pacpll_hwi_about (void); // About
static void rpspmc_pacpll_hwi_configure (void); // Configure plugIn, called via PlugIn-Configurator
static void rpspmc_pacpll_hwi_cleanup (void); // called on PlugIn unload, should cleanup PlugIn rescources

// other PlugIn Functions and Callbacks (connected to Buttons, Toolbar, Menu)
static void rpspmc_pacpll_hwi_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void rpspmc_pacpll_hwi_SaveValues_callback ( gpointer );

// custom menu callback expansion to arbitrary long list
static PIMenuCallback rpspmc_pacpll_hwi_mcallback_list[] = {
		rpspmc_pacpll_hwi_show_callback,             // direct menu entry callback1
		NULL  // NULL terminated list
};

static GxsmPluginMenuCallbackList rpspmc_pacpll_hwi_menu_callback_list = {
	1,
	rpspmc_pacpll_hwi_mcallback_list
};

GxsmPluginMenuCallbackList *get_gxsm_plugin_menu_callback_list ( void ) {
	return &rpspmc_pacpll_hwi_menu_callback_list;
}

// Fill in the GxsmPlugin Description here -- see also: Gxsm/src/plugin.h
GxsmPlugin rpspmc_pacpll_hwi_pi = {
	NULL,                   // filled in and used by Gxsm, don't touch !
	NULL,                   // filled in and used by Gxsm, don't touch !
	0,                      // filled in and used by Gxsm, don't touch !
	NULL,                   // The Gxsm-App Class Ref.pointer (called "gapp" in Gxsm) is 
	// filled in here by Gxsm on Plugin load, 
	// just after init() is called !!!
	// ----------------------------------------------------------------------
	// Plugins Name, CodeStly is like: Name-M1S|M2S-BG|F1D|F2D|ST|TR|Misc
	"rpspmc_pacpll_hwi-"
	"HW-INT-NS-FLOAT",
	// Plugin's Category - used to autodecide on Pluginloading or ignoring
	// In this case of Hardware-Interface-Plugin here is the interface-name required
	// this is the string selected for "Hardware/Card"!
	THIS_HWI_PLUGIN_NAME,
	// Description, is shown by PluginViewer (Plugin: listplugin, Tools->Plugin Details)
	g_strdup ("Red Pitaya SPM Control and PACPLL hardware interface."),
	// Author(s)
	"Percy Zahl",
	// Menupath to position where it is appendet to -- not used by HwI PIs
	"windows-section,windows-section,windows-section,windows-section",
	// Menuentry -- not used by HwI PIs
	N_("RPSPM Control"),
	// help text shown on menu
	N_("This is the " THIS_HWI_PLUGIN_NAME " - GXSM Hardware Interface."),
	// more info...
	"N/A",
	NULL,          // error msg, plugin may put error status msg here later
	NULL,          // Plugin Status, managed by Gxsm, plugin may manipulate it too
	// init-function pointer, can be "NULL", 
	// called if present at plugin load
	rpspmc_pacpll_hwi_init,  
	// query-function pointer, can be "NULL", 
	// called if present after plugin init to let plugin manage it install itself
	rpspmc_pacpll_hwi_query, // query can be used (otherwise set to NULL) to install
	// additional control dialog in the GXSM menu
	// about-function, can be "NULL"
	// can be called by "Plugin Details"
	rpspmc_pacpll_hwi_about,
	// configure-function, can be "NULL"
	// can be called by "Plugin Details"
	rpspmc_pacpll_hwi_configure,
	// run-function, can be "NULL", if non-Zero and no query defined, 
	// it is called on menupath->"plugin"
	NULL, // run should be "NULL" for Gxsm-Math-Plugin !!!
	// cleanup-function, can be "NULL"
	// called if present at plugin removal
	NULL,   // direct menu entry callback1 or NULL
	NULL,   // direct menu entry callback2 or NULL
	rpspmc_pacpll_hwi_cleanup // plugin cleanup callback or NULL
};

// Text used in Aboutbox, please update!!
static const char *about_text = N_("RPSPMC PACPLL Control Plugin\n\n"
                                   "Complete FPGA SPM Control using Red Pitaya + Analog Modules.\n"
	);

/* Here we go... */

RPSPMC_Control *RPSPMC_ControlClass = NULL;
GVPMoverControl *rpspmc_gvpmover = NULL;
        
// event hooks
static void RPSPMC_Control_StartScan_callback ( gpointer );
static void RPSPMC_Control_SaveValues_callback ( gpointer );
static void RPSPMC_Control_LoadValues_callback ( gpointer );

gchar *rpspmc_pacpll_hwi_configure_string = NULL;   // name of the currently in GXSM configured HwI (Hardware/Card)
RPspmc_pacpll *rpspmc_pacpll = NULL;
rpspmc_hwi_dev *rpspmc_hwi = NULL;


// Symbol "get_gxsm_hwi_hardware_class" is resolved by dlsym from Gxsm for all HwI type PIs, 
// Essential Plugin Function!!
XSM_Hardware *get_gxsm_hwi_hardware_class ( void *data ) {
        gchar *tmp;
        main_get_gapp()->monitorcontrol->LogEvent (THIS_HWI_PREFIX " XSM_Hardware *get_gxsm_hwi_hardware_class", "Init 1");

	rpspmc_pacpll_hwi_configure_string = g_strdup ((gchar*)data);

        
	// probe for harware here...
	rpspmc_hwi = new rpspmc_hwi_dev ();
	
        main_get_gapp()->monitorcontrol->LogEvent ("HwI: probing succeeded.", "RP SPM Control System Ready.");
	return rpspmc_hwi;
}

// init-Function
void rpspmc_pacpll_hwi_init(void)
{
	PI_DEBUG (DBG_L2, "rspmc_pacpll_hwi Plugin Init");
	rpspmc_pacpll = NULL;
}


// Symbol "get_gxsm_plugin_info" is resolved by dlsym from Gxsm, used to get Plugin's info!! 
// Essential Plugin Function!!
GxsmPlugin *get_gxsm_plugin_info ( void ){ 
	rpspmc_pacpll_hwi_pi.description = g_strdup_printf(N_("Gxsm rpspmc_pacpll plugin %s"), VERSION);
	return &rpspmc_pacpll_hwi_pi; 
}



// data passed to "idle" function call, used to refresh/draw while waiting for data
typedef struct {
	GSList *scan_list; // scans to update
	GFunc  UpdateFunc; // function to call for background updating
	gpointer data; // additional data (here: reference to the current rpspmc_pacpll object)
} IdleRefreshFuncData;


// Query Function, installs Plugin's in File/Import and Export Menupaths!

static void rpspmc_pacpll_hwi_query(void)
{
	if(rpspmc_pacpll_hwi_pi.status) g_free(rpspmc_pacpll_hwi_pi.status); 
	rpspmc_pacpll_hwi_pi.status = g_strconcat (
                                                 N_("Plugin query has attached "),
                                                 rpspmc_pacpll_hwi_pi.name, 
                                                 N_(": File IO Filters are ready to use"),
                                                 NULL);

//      Setup Control Windows
// ==================================================
	PI_DEBUG (DBG_L2, "rpspmc_pacpll_query:new" );

        // first
        rpspmc_pacpll = new RPspmc_pacpll (main_get_gapp() -> get_app ());

        // second
	RPSPMC_ControlClass = new RPSPMC_Control (main_get_gapp() -> get_app ());
        
        // third
        rpspmc_gvpmover = new GVPMoverControl (main_get_gapp() -> get_app ());

	rpspmc_pacpll_hwi_pi.status = g_strconcat(N_("Plugin query has attached "),
                                                  rpspmc_pacpll_hwi_pi.name, 
                                                  N_(": " THIS_HWI_PREFIX "-Control is created."),
                                                  NULL);
        
	PI_DEBUG (DBG_L2, "rpspmc_pacpll_query:res" );
	
	rpspmc_pacpll_hwi_pi.app->ConnectPluginToCDFSaveEvent (rpspmc_pacpll_hwi_SaveValues_callback);
	//rpspmc_pacpll_hwi_pi.app->ConnectPluginToCDFLoadEvent (XXX_LoadValues_callback);
}

static void rpspmc_pacpll_hwi_SaveValues_callback ( gpointer gp_ncf ){
	NcFile *ncf = (NcFile *) gp_ncf;
        if (rpspmc_pacpll)
                rpspmc_pacpll->save_values (*ncf);
        if (RPSPMC_ControlClass)
                RPSPMC_ControlClass->save_values (*ncf);
}


// 5.) Start here with the plugins code, vars def., etc.... here.
// ----------------------------------------------------------------------
//



// about-Function 
static void rpspmc_pacpll_hwi_about(void)
{
        const gchar *authors[] = { rpspmc_pacpll_hwi_pi.authors, NULL};
        gtk_show_about_dialog (NULL,
                               "program-name",  rpspmc_pacpll_hwi_pi.name,
                               "version", VERSION,
                               "license", GTK_LICENSE_GPL_3_0,
                               "comments", about_text,
                               "authors", authors,
                               NULL
                               );
}

// configure-Function
static void rpspmc_pacpll_hwi_configure(void)
{
	if(rpspmc_pacpll_hwi_pi.app)
		rpspmc_pacpll_hwi_pi.app->message("rpspmc_pacpll Plugin Configuration");
}

// cleanup-Function, make sure the Menustrings are matching those above!!!
static void rpspmc_pacpll_hwi_cleanup(void)
{
	// delete ...
        if (rpspmc_gvpmover)
                delete rpspmc_gvpmover ;
        
	if( rpspmc_pacpll )
		delete rpspmc_pacpll ;

	if(rpspmc_pacpll_hwi_pi.status) g_free(rpspmc_pacpll_hwi_pi.status); 

	PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::rpspmc_hwi_cleanup -- Plugin Cleanup, -- Menu [disabled]\n");

        // remove GUI
	if( RPSPMC_ControlClass ){
		delete RPSPMC_ControlClass ;
        }
	RPSPMC_ControlClass = NULL;

	if (rpspmc_hwi){
                PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::rpspmc_hwi_cleanup -- delete DSP Common-HwI-Class.");
		delete rpspmc_hwi;
        }
	rpspmc_hwi = NULL;

        PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::rpspmc_hwi_cleanup -- g_free rpspmc_pacpll_hwi_configure_string Info.");
	g_free (rpspmc_pacpll_hwi_configure_string);
	rpspmc_pacpll_hwi_configure_string = NULL;
	
        PI_DEBUG (DBG_L4, THIS_HWI_PREFIX "::rpspmc_pacpll_hwi_cleanup -- Done.");
}


static void rpspmc_pacpll_hwi_show_callback(GSimpleAction *simple, GVariant *parameter, gpointer user_data){
	PI_DEBUG (DBG_L2, "rpspmc_pacpll Plugin : show" );
	if( rpspmc_pacpll )
		rpspmc_pacpll->show();
}

/* **************************************** SPM Control GUI **************************************** */

// GUI Section -- custom to HwI
// ================================================================================

// advanced GUI building support and building automatizations

GtkWidget*  GUI_Builder::grid_add_mixer_options (gint channel, gint preset, gpointer ref){
        gchar *id;
        GtkWidget *cbtxt = gtk_combo_box_text_new ();

        g_message ("RPSPMC:: GUI_Builder::grid_add_mixer_options");
        
        g_object_set_data (G_OBJECT (cbtxt), "mix_channel", GINT_TO_POINTER (channel)); 
                                                                        
        id = g_strdup_printf ("%d", MM_OFF);        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "OFF"); g_free (id); 
        id = g_strdup_printf ("%d", MM_ON);         gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "LIN"); g_free (id); 
        id = g_strdup_printf ("%d", MM_ON | MM_LOG);          gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "LOG"); g_free (id); 
        //id = g_strdup_printf ("%d", MM_ON | MM_LOG | MM_FCZ); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "FCZ-LOG"); g_free (id); 
        //id = g_strdup_printf ("%d", MM_ON | MM_FCZ); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, "FCZ-LIN"); g_free (id); 

        gchar *preset_id = g_strdup_printf ("%d", preset); 
        gtk_combo_box_set_active_id (GTK_COMBO_BOX (cbtxt), preset_id);
        //g_print ("SetActive MIX%d p=%d <%s>\n", channel, preset, preset_id);
        g_free (preset_id);
                
        g_signal_connect (G_OBJECT (cbtxt),"changed",	
                          G_CALLBACK (RPSPMC_Control::choice_mixmode_callback), 
                          ref);				
                
        grid_add_widget (cbtxt);
        return cbtxt;
};

GtkWidget* GUI_Builder::grid_add_scan_input_signal_options (gint channel, gint preset, gpointer ref){ // preset=scan_source[CHANNEL]
        GtkWidget *cbtxt = gtk_combo_box_text_new (); 
        g_object_set_data(G_OBJECT (cbtxt), "scan_channel_source", GINT_TO_POINTER (channel)); 

        int jj=0;

        for (int jj=0; rpspmc_swappable_signals[jj].label; ++jj){
                gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, rpspmc_swappable_signals[jj].label); g_free (id);
        }
        
        gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), preset); 
        g_signal_connect (G_OBJECT (cbtxt), "changed",	
                          G_CALLBACK (RPSPMC_Control::choice_scansource_callback), 
                          ref);
        grid_add_widget (cbtxt);
        return cbtxt;
};


GtkWidget* GUI_Builder::grid_add_probe_source_signal_options (gint channel, gint preset, gpointer ref){ // preset=probe_source[CHANNEL])
        GtkWidget *cbtxt = gtk_combo_box_text_new (); 
        gtk_widget_set_size_request (cbtxt, 50, -1); 
        g_object_set_data(G_OBJECT (cbtxt), "prb_channel_source", GINT_TO_POINTER (channel)); 


        for (int jj=0; rpspmc_swappable_signals[jj].label; ++jj){
                gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, rpspmc_swappable_signals[jj].label); g_free (id);
        }

        if (preset >= 0)
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), preset); 
        else
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 4); // NULL SIGNAL [TESTING FALLBACK for -1/error]
                
        g_signal_connect (G_OBJECT (cbtxt), "changed",	
                          G_CALLBACK (RPSPMC_Control::choice_prbsource_callback), 
                          ref);				
        grid_add_widget (cbtxt);
        return cbtxt;
};

GtkWidget* GUI_Builder::grid_add_modulation_target_options (gint channel, gint preset, gpointer ref){
        GtkWidget *cbtxt = gtk_combo_box_text_new (); 
        gtk_widget_set_size_request (cbtxt, 50, -1); 
        g_object_set_data(G_OBJECT (cbtxt), "mod_channel", GINT_TO_POINTER (channel)); 


        for (int jj=0;  modulation_targets[jj].label; ++jj){
                gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, modulation_targets[jj].label); g_free (id);
        }

        if (preset >= 0)
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), preset); 
        else
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 4); // NULL SIGNAL [TESTING FALLBACK for -1/error]
                
        g_signal_connect (G_OBJECT (cbtxt), "changed",	
                          G_CALLBACK (RPSPMC_Control::choice_mod_target_callback), 
                          ref);				
        grid_add_widget (cbtxt);
        return cbtxt;
};

GtkWidget* GUI_Builder::grid_add_z_servo_current_source_options (gint channel, gint preset, gpointer ref){
        GtkWidget *cbtxt = gtk_combo_box_text_new (); 
        gtk_widget_set_size_request (cbtxt, 50, -1); 
        g_object_set_data(G_OBJECT (cbtxt), "z_servo_current_source_id", GINT_TO_POINTER (channel)); 


        for (int jj=0; z_servo_current_source[jj].label; ++jj){
                gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id, z_servo_current_source[jj].label); g_free (id);
        }

        if (preset >= 0)
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), preset); 
        else
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 4); // NULL SIGNAL [TESTING FALLBACK for -1/error]
                
        g_signal_connect (G_OBJECT (cbtxt), "changed",	
                          G_CALLBACK (RPSPMC_Control::choice_z_servo_current_source_callback), 
                          ref);				
        grid_add_widget (cbtxt);
        return cbtxt;
};

GtkWidget* GUI_Builder::grid_add_rf_gen_out_options (gint channel, gint preset, gpointer ref){
        GtkWidget *cbtxt = gtk_combo_box_text_new (); 
        gtk_widget_set_size_request (cbtxt, 50, -1); 
        g_object_set_data(G_OBJECT (cbtxt), "rf_gen_out_dest_id", GINT_TO_POINTER (channel)); 


        for (int jj=0; rf_gen_out_dest[jj].label; ++jj){
                gchar *id = g_strdup_printf ("%d", jj); gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbtxt), id,  rf_gen_out_dest[jj].label); g_free (id);
        }

        if (preset >= 0)
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), preset); 
        else
                gtk_combo_box_set_active (GTK_COMBO_BOX (cbtxt), 4); // NULL SIGNAL [TESTING FALLBACK for -1/error]
                
        g_signal_connect (G_OBJECT (cbtxt), "changed",	
                          G_CALLBACK (RPSPMC_Control::choice_rf_gen_out_callback), 
                          ref);				
        grid_add_widget (cbtxt);
        return cbtxt;
};




GtkWidget* GUI_Builder::grid_add_probe_status (const gchar *status_label) {
        GtkWidget *inp = grid_add_input (status_label);
        gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY(inp))), " --- ", -1);
        gtk_editable_set_editable (GTK_EDITABLE (inp), FALSE);
        return inp;
};

void GUI_Builder::grid_add_probe_controls (gboolean have_dual,
                              guint64 option_flags, GCallback option_cb,
                              guint64 auto_flags, GCallback auto_cb,
                              GCallback exec_cb, GCallback write_cb, GCallback graph_cb, GCallback x_cb,
                              gpointer cb_data,
                              const gchar *control_id) {                                                            

        new_grid_with_frame ("Probe Controller");
        new_grid ();
        gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);
        gtk_widget_set_hexpand (grid, TRUE);
                
        // ==================================================
        set_configure_list_mode_on ();
        // OPTIONS
        grid_add_check_button_guint64 ("Feedback On",
                                       "Check to keep Z feedback on while probe.", 1,
                                       GCallback (option_cb), cb_data, option_flags, FLAG_FB_ON); 
        if (have_dual) {                                         
                grid_add_check_button_guint64 ("Dual Mode",
                                               "Check to run symmetric probe:\n"
                                               "take reverse data to start.", 1,
                                               GCallback (option_cb), cb_data, option_flags, FLAG_DUAL); 
        }                                                       
        //grid_add_check_button_guint64 ("Oversampling", "Check do enable DSP data oversampling:\n"
        //                               "integrates data at intermediate points (averaging).\n"
        //                               "(recommended)", 1,
        //                               GCallback (option_cb), cb_data, option_flags, FLAG_INTEGRATE); 
        grid_add_check_button_guint64 ("Full Ramp", "Check do enable data acquisition on all probe segments:\n"
                                       "including start/end ramps and delays.", 1,
                                       GCallback (option_cb), cb_data, option_flags, FLAG_SHOW_RAMP); 
        // AUTO MODES
        grid_add_check_button_guint64 ("Auto Plot", "Enable life data plotting:\n"
                                       "disable to save resources and CPU time for huge data sets\n"
                                       "and use manual plot update if needed.", 1,
                                       GCallback (auto_cb), cb_data, auto_flags, FLAG_AUTO_PLOT); 
        grid_add_check_button_guint64 ("Auto Save", "Enable save data automatically at competion.\n"
                                       "(recommended)", 1,
                                       GCallback (auto_cb), cb_data, auto_flags, FLAG_AUTO_SAVE); 
        grid_add_check_button_guint64 ("GLock", "Lock Data/Graphs Configuration.\n"
                                       "(recommended to check after setup and one test run)", 1,
                                       GCallback (auto_cb), cb_data, auto_flags, FLAG_AUTO_GLOCK); 
        grid_add_check_button_guint64 ("Run Init Script", "Run Init Script before VP execution.\n"
                                       "(use only if init script (see python console, Actions Scripts) for VP-mode is created and reviewed, you can configure any initial condtions like bias, setpoint for example.)", 1,
                                       GCallback (auto_cb), cb_data, auto_flags, FLAG_AUTO_RUN_INITSCRIPT);
        set_configure_list_mode_off ();
        // ==================================================
        GtkWidget *tmp = grid;
        pop_grid ();
        grid_add_widget (tmp);
                                                                        
        new_line ();

        new_grid ();
        gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);
        gtk_widget_set_hexpand (grid, TRUE);
                
        remote_action_cb *ra = g_new( remote_action_cb, 1);     
        ra -> cmd = g_strdup_printf("DSP_VP_%s_EXECUTE", control_id); 
        gchar *help = g_strconcat ("Remote example: action (", ra->cmd, ")", NULL); 
        grid_add_button ("Execute", help, 2,
                         GCallback (exec_cb), cb_data);
        g_free (help);
        ra -> RemoteCb = (void (*)(GtkWidget*, void*))exec_cb;  
        ra -> widget = button;                                  
        ra -> data = cb_data;                                      

                
        main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra ); 
        PI_DEBUG (DBG_L2, "Adding new Remote Cmd: " << ra->cmd ); 
                                                                        
        // ==================================================
        set_configure_list_mode_on ();
        grid_add_button ("Write Vectors",
                         "Update Vector Program on DSP.\n"
                         "USE WITH CAUTION: possibel any time, but has real time consequences,\n"
                         "may be used to accellerate/abort VP.", 1,
                         GCallback (write_cb), cb_data);
        set_configure_list_mode_off ();
        // ==================================================
		                                                        
        grid_add_button ("Plot",
                         "Manually update all graphs.", 1,
                         GCallback (graph_cb), cb_data);         
                                                                        
        grid_add_button ("Save",
                         "Save all VP data, file name is auto generated.", 1,
                         G_CALLBACK (RPSPMC_Control::Probing_save_callback), cb_data);         

        // ==================================================
        set_configure_list_mode_on ();
        grid_add_button ("ABORT", "Cancel VP Program (writes and executes Zero Vector Program)\n"
                         "USE WITH CAUTION: pending data will get corrupted.", 1,
                         GCallback (x_cb), cb_data);
        set_configure_list_mode_off ();
        // ==================================================

        tmp = grid;
        pop_grid ();
        grid_add_widget (tmp);
        pop_grid ();
};


static GActionEntry win_RPSPMC_popup_entries[] = {
        { "dsp-mover-configure", RPSPMC_Control::configure_callback, NULL, "true", NULL },
};

void RPSPMC_Control_show_callback (GSimpleAction *simple, GVariant *parameter, gpointer user_data){
        ; // show window... if dummy closed it...
}



void RPSPMC_Control::configure_callback (GSimpleAction *action, GVariant *parameter, 
                                          gpointer user_data){
        RPSPMC_Control *mc = (RPSPMC_Control *) user_data;
        GVariant *old_state, *new_state;

        if (action){
                old_state = g_action_get_state (G_ACTION (action));
                new_state = g_variant_new_boolean (!g_variant_get_boolean (old_state));
                
                g_simple_action_set_state (action, new_state);

                PI_DEBUG_GP (DBG_L4, "Toggle action %s activated, state changes from %d to %d\n",
                             g_action_get_name (G_ACTION (action)),
                             g_variant_get_boolean (old_state),
                             g_variant_get_boolean (new_state));
                g_simple_action_set_state (action, new_state);
                g_variant_unref (old_state);

                g_settings_set_boolean (mc->hwi_settings, "configure-mode", g_variant_get_boolean (new_state));
        } else {
                // new_state = g_variant_new_boolean (true);
                new_state = g_variant_new_boolean (g_settings_get_boolean (mc->hwi_settings, "configure-mode"));
        }

	if (g_variant_get_boolean (new_state)){
                g_slist_foreach
                        ( mc->bp->get_configure_list_head (),
                          (GFunc) App::show_w, NULL
                          );
                g_slist_foreach
                        ( mc->bp->get_config_checkbutton_list_head (),
                          (GFunc) RPSPMC_Control::show_tab_to_configure, NULL
                          );
        } else  {
                g_slist_foreach
                        ( mc->bp->get_configure_list_head (),
                          (GFunc) App::hide_w, NULL
                          );
                g_slist_foreach
                        ( mc->bp->get_config_checkbutton_list_head (),
                          (GFunc) RPSPMC_Control::show_tab_as_configured, NULL
                          );
        }
        if (!action){
                g_variant_unref (new_state);
        }
}


//
// dconf "settings" managements for custom non PCS managed parameters
//



void RPSPMC_Control::store_graphs_values (){
        g_settings_set_int (hwi_settings, "probe-sources", Source);
        g_settings_set_int (hwi_settings, "probe-sources-x", XSource);
        g_settings_set_boolean (hwi_settings, "probe-x-join", XJoin);
        g_settings_set_boolean (hwi_settings, "probe-graph-matrix-window", GrMatWin);
        g_settings_set_int (hwi_settings, "probe-p-sources", PSource);
        g_settings_set_int (hwi_settings, "probe-pavg-sources", PlotAvg);
        g_settings_set_int (hwi_settings, "probe-psec-sources", PlotSec);

        //set_tab_settings ("AC", AC_option_flags, AC_auto_flags, AC_glock_data);
        set_tab_settings ("IV", IV_option_flags, IV_auto_flags, IV_glock_data);
        //set_tab_settings ("FZ", FZ_option_flags, FZ_auto_flags, FZ_glock_data);
        //set_tab_settings ("PL", PL_option_flags, PL_auto_flags, PL_glock_data);
        //set_tab_settings ("LP", LP_option_flags, LP_auto_flags, LP_glock_data);
        //set_tab_settings ("SP", SP_option_flags, SP_auto_flags, SP_glock_data);
        //set_tab_settings ("TS", TS_option_flags, TS_auto_flags, TS_glock_data);
        set_tab_settings ("VP", GVP_option_flags, GVP_auto_flags, GVP_glock_data);
        //set_tab_settings ("TK", TK_option_flags, TK_auto_flags, TK_glock_data);
        //set_tab_settings ("AX", AX_option_flags, AX_auto_flags, AX_glock_data);
        //set_tab_settings ("AB", ABORT_option_flags, ABORT_auto_flags, ABORT_glock_data);

        GVP_store_vp ("VP_set_last"); // last in view
        PI_DEBUG_GM (DBG_L3, "RPSPMC_Control::store_graphs_values matrix complete.");
}

void RPSPMC_Control::restore_graphs_values (){
        PI_DEBUG_GM (DBG_L3, "RPSPMC_Control::restore_graphs_values matrix settings.");
        Source = g_settings_get_int (hwi_settings, "probe-sources");
        XSource = g_settings_get_int (hwi_settings, "probe-sources-x");
        XJoin = g_settings_get_boolean (hwi_settings, "probe-x-join");
        GrMatWin = g_settings_get_boolean (hwi_settings, "probe-graph-matrix-window");
        PSource = g_settings_get_int (hwi_settings, "probe-p-sources");
        PlotAvg = g_settings_get_int (hwi_settings, "probe-pavg-sources");
        PlotSec = g_settings_get_int (hwi_settings, "probe-psec-sources");

        for (int i=0; graphs_matrix[0][i]; ++i)
                if (graphs_matrix[0][i]){
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (graphs_matrix[0][i]),  Source & rpspmc_source_signals[i].mask);
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (graphs_matrix[1][i]), XSource & rpspmc_source_signals[i].mask);
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (graphs_matrix[2][i]), PSource & rpspmc_source_signals[i].mask);
                        // ..
                 }
}

void RPSPMC_Control::GVP_store_vp (const gchar *key){
	PI_DEBUG_GM (DBG_L3, "GVP-VP store to memo %s", key);
	g_message ("GVP-VP store to memo %s", key);
        GVariant *v = g_settings_get_value (hwi_settings, "probe-gvp-vector-program-matrix");
        //GVariant *v = g_settings_get_value (hwi_settings, "probe-lm-vector-program-matrix");
        GVariantDict *dict = g_variant_dict_new (v);
        if (!dict){
                PI_DEBUG_GW (DBG_L1, "ERROR: RPSPMC_Control::GVP_store_vp -- can't read dictionary 'probe-gvp-vector-program-matrix' a{sv}.");
                return;
        }

        gint32 vp_program_length;
        for (vp_program_length=0; GVP_points[vp_program_length] > 0; ++vp_program_length);
                
        gsize    n = MIN (vp_program_length+1, N_GVP_VECTORS);
        GVariant *pc_array_du = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_du, n, sizeof (double));
        GVariant *pc_array_dx = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_dx, n, sizeof (double));
        GVariant *pc_array_dy = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_dy, n, sizeof (double));
        GVariant *pc_array_dz = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_dz, n, sizeof (double));
        GVariant *pc_array_da = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_da, n, sizeof (double));
        GVariant *pc_array_db = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_db, n, sizeof (double));
        GVariant *pc_array_dam = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_dam, n, sizeof (double));
        GVariant *pc_array_dfm = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_dfm, n, sizeof (double));
        GVariant *pc_array_ts = g_variant_new_fixed_array (g_variant_type_new ("d"), GVP_ts, n, sizeof (double));
        GVariant *pc_array_pn = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_points, n, sizeof (gint32));
        GVariant *pc_array_op = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_opt, n, sizeof (gint32));
        GVariant *pc_array_vn = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_vnrep, n, sizeof (gint32));
        GVariant *pc_array_vp = g_variant_new_fixed_array (g_variant_type_new ("i"), GVP_vpcjr, n, sizeof (gint32));

        GVP_glock_data[0] = Source; GVP_glock_data[1] = XSource; GVP_glock_data[2] = PSource; GVP_glock_data[3] = XJoin; GVP_glock_data[4] = PlotAvg; GVP_glock_data[5] = PlotSec;
        GVariant *pc_array_grm = g_variant_new_fixed_array (g_variant_type_new ("t"), GVP_glock_data, 6, sizeof (guint64));
        
        GVariant *pc_array[] = { pc_array_du, pc_array_dx, pc_array_dy, pc_array_dz, pc_array_da, pc_array_db, pc_array_dam, pc_array_dfm,
                                 pc_array_ts,
                                 pc_array_pn, pc_array_op, pc_array_vn, pc_array_vp,
                                 pc_array_grm,
                                 NULL };
        const gchar *vckey[] = { "du", "dx", "dy", "dz", "da", "db", "dam", "dfm", "ts", "pn", "op", "vn", "vp", "grm", NULL };

        for (int i=0; vckey[i] && pc_array[i]; ++i){
                gchar *m_vckey = g_strdup_printf ("%s-%s", vckey[i], key);

                g_print ("GVP store: %s = %s\n", m_vckey, g_variant_print (pc_array[i], true));

                if (g_variant_dict_contains (dict, m_vckey)){
                        if (!g_variant_dict_remove (dict, m_vckey)){
                                PI_DEBUG_GW (DBG_L1, "ERROR: RPSPMC_Control::GVP_store_vp -- key '%s' found, but removal failed.", m_vckey);
                                g_free (m_vckey);
                                return;
                        }
                }
                g_variant_dict_insert_value (dict, m_vckey, pc_array[i]);
                g_free (m_vckey);
        }

        GVariant *probe_vector_program_matrix = g_variant_dict_end (dict);
        g_settings_set_value (hwi_settings, "probe-gvp-vector-program-matrix", probe_vector_program_matrix);
        //g_settings_set_value (hwi_settings, "probe-lm-vector-program-matrix", probe_vector_program_matrix);

        // all g_variants created here are "consumed" by the "set" calls, if I try to unref, it cause random crashes.
        //g_variant_unref (probe_vector_program_matrix);
        //g_variant_dict_unref (dict);
        // Can't do, don't need??? -- getting this: GLib-CRITICAL **: g_variant_unref: assertion 'value->ref_count > 0' failed
        //        for (int i=0; pc_array[i]; ++i)
        //                g_variant_unref (pc_array[i]);
}

void RPSPMC_Control::GVP_restore_vp (const gchar *key){
	// g_message ("GVP-VP restore memo key=%s", key);
	PI_DEBUG_GP (DBG_L2, "GVP-VP restore memo %s\n", key);
	g_message ( "GVP-VP restore memo %s\n", key);
        GVariant *v = g_settings_get_value (hwi_settings, "probe-gvp-vector-program-matrix");
        //GVariant *v = g_settings_get_value (hwi_settings, "probe-lm-vector-program-matrix");
        GVariantDict *dict = g_variant_dict_new (v);
        if (!dict){
                PI_DEBUG_GP (DBG_L2, "ERROR: RPSPMC_Control::GVP_restore_vp -- can't read dictionary 'probe-gvp-vector-program-matrix' a{sv}.\n");
                return;
        }
        gsize  n; // == N_GVP_VECTORS;
        GVariant *vd[9];
        GVariant *vi[4];
        double *pc_array_d[9];
        gint32 *pc_array_i[5];
        const gchar *vckey_d[] = { "du", "dx", "dy", "dz", "da", "db", "dam", "dfm", "ts", NULL };
        const gchar *vckey_i[] = { "pn", "op", "vn", "vp", NULL };
        double *GVPd[] = { GVP_du, GVP_dx, GVP_dy, GVP_dz, GVP_da, GVP_db, GVP_dam, GVP_dfm, GVP_ts, NULL };
        gint32 *GVPi[] = { GVP_points, GVP_opt, GVP_vnrep, GVP_vpcjr, NULL };
        gint32 vp_program_length=0;
        
        for (int i=0; vckey_i[i]; ++i){
                gchar *m_vckey = g_strdup_printf ("%s-%s", vckey_i[i], key);
                g_print ( "GVP-VP restore %s\n", m_vckey);

                for (int k=0; k<N_GVP_VECTORS; ++k) GVPi[i][k]=0; // zero init vector
                if ((vi[i] = g_variant_dict_lookup_value (dict, m_vckey, ((const GVariantType *) "ai"))) == NULL){
                        PI_DEBUG_GP (DBG_L2, "GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_i '%s' memo not found. Setting to Zero.\n", m_vckey);
                        // g_warning ("GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_i '%s' memo not found. Setting to Zero.\n", m_vckey);
                        g_free (m_vckey);
                        continue;
                }
                g_print ("GVP restore: %s = %s\n", m_vckey, g_variant_print (vi[i], true));

                pc_array_i[i] = (gint32*) g_variant_get_fixed_array (vi[i], &n, sizeof (gint32));
                if (i==0) // actual length of this vector should fit all others -- verify
                        vp_program_length=n;
                else
                        if (n != vp_program_length)
                                g_warning ("GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_i '%s' vector length %ld not matching program n=%d.\n", m_vckey, n, vp_program_length);
                // g_assert_cmpint (n, ==, N_GVP_VECTORS);
                for (int k=0; k<n && k<N_GVP_VECTORS; ++k)
                        GVPi[i][k]=pc_array_i[i][k];
                g_free (m_vckey);
                g_variant_unref (vi[i]);
        }                        

        {
                gchar *m_vckey = g_strdup_printf ("%s-%s", "grm", key);
                g_print ( "GVP-VP restore %s\n", m_vckey);
                GVariant *viG;
                if ((viG = g_variant_dict_lookup_value (dict, m_vckey, ((const GVariantType *) "at"))) == NULL){
                        PI_DEBUG_GP (DBG_L2, "GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_i '%s' memo not found. Setting to Zero.\n", m_vckey);
                        // g_warning ("GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_i '%s' memo not found. Setting to Zero.\n", m_vckey);
                        g_free (m_vckey);
                } else {
                        g_print ("GVP restore: %s = %s\n", m_vckey, g_variant_print (viG, true));
                        guint64 *gvp_grm_array_i =  (guint64*) g_variant_get_fixed_array (viG, &n, sizeof (gint64));
                        for (int k=0; k<n && k<6; ++k)
                                GVP_glock_data[k]=gvp_grm_array_i[k];
                }
        }
        
        for (int i=0; vckey_d[i]; ++i){
                gchar *m_vckey = g_strdup_printf ("%s-%s", vckey_d[i], key);
                for (int k=0; k<N_GVP_VECTORS; ++k) GVPd[i][k]=0.; // zero init vector
                if ((vd[i] = g_variant_dict_lookup_value (dict, m_vckey, ((const GVariantType *) "ad"))) == NULL){
                        PI_DEBUG_GP (DBG_L2, "GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_d '%s' memo not found. Setting to Zero.", m_vckey);
                        // g_warning ("GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_d '%s' memo not found. Setting to Zero.", m_vckey);
                        g_free (m_vckey);
                        continue;
                }
                g_print ("GVP restore: %s = %s\n", m_vckey, g_variant_print (vd[i], true));

                pc_array_d[i] = (double*) g_variant_get_fixed_array (vd[i], &n, sizeof (double));
                //g_assert_cmpint (n, ==, N_GVP_VECTORS);
                if (n != vp_program_length)
                        g_warning ("GXSM4 DCONF: RPSPMC_Control::GVP_restore_vp -- key_d '%s' vector length %ld not matching program n=%d.\n", m_vckey, n, vp_program_length);
                for (int k=0; k<vp_program_length && k<N_GVP_VECTORS; ++k)
                        GVPd[i][k]=pc_array_d[i][k];
                g_free (m_vckey);
                g_variant_unref (vd[i]);
        }                        
        
        g_variant_dict_unref (dict);
        g_variant_unref (v);

        Source = GVP_glock_data[0]; XSource = GVP_glock_data[1]; PSource = GVP_glock_data[2]; XJoin = GVP_glock_data[3]; PlotAvg = GVP_glock_data[4];  PlotSec = GVP_glock_data[5];
        // update Graphs
        for (int i=0; graphs_matrix[0][i]; ++i)
                if (graphs_matrix[0][i]){
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (graphs_matrix[0][i]),  Source & rpspmc_source_signals[i].mask?true:false);
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (graphs_matrix[1][i]), XSource & rpspmc_source_signals[i].mask?true:false);
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (graphs_matrix[2][i]), PSource & rpspmc_source_signals[i].mask?true:false);
                        // ..
                 }


        
	update_GUI ();
}

int RPSPMC_Control::callback_edit_GVP (GtkWidget *widget, RPSPMC_Control *self){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
        int x=1, y=10;
	int ki = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "VPC"));
	int  a = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "ARROW"));

	if (ki < 0){
		const int VPT_YPAD=0;
		int c = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(self->VPprogram[0]), "CF"));
		int cw = 1;
		if (self->VPprogram[1]){
			for (int j=1; j<10; ++j)
				if (self->VPprogram[j]){
                                        // FIX-ME GTK4 ???
					gtk_window_destroy (GTK_WINDOW (self->VPprogram[j]));
					self->VPprogram[j] = NULL;
				}
		} else
			for (int k=0; k<N_GVP_VECTORS && cw < 10; ++k){
				if (self->GVP_vpcjr[k] < 0){
					int kf=k;
					int ki=k+self->GVP_vpcjr[k];
					if (kf >= 0){
                                                // fix!! todo
						//** ADD_BUTTON_GRID ("arrow-up-symbolic", "Loop",   self->VPprogram[0], x+0, y+ki+1, 1, kf-ki+2, -2, NULL, NULL, self->VPprogram[cw]);
                                                //ADD_BUTTON_TAB(GTK_ARROW_UP, "Loop",   self->VPprogram[0], c+0, c+1, ki+1, kf+2, GTK_FILL, GTK_FILL, 0, VPT_YPAD, -1, -2, NULL, NULL, self->VPprogram[cw]);
						++c; ++cw;
					}
				}
			}
		return 0;
	}

	if (a == GVP_SHIFT_UP && ki >= 1 && ki < N_GVP_VECTORS)
		for (int k=ki-1; k < N_GVP_VECTORS-1; ++k){
			int ks = k+1;
			self->GVP_du[k] = self->GVP_du[ks];
			self->GVP_dx[k] = self->GVP_dx[ks];
			self->GVP_dy[k] = self->GVP_dy[ks];
			self->GVP_dz[k] = self->GVP_dz[ks];
			self->GVP_da[k] = self->GVP_da[ks];
			self->GVP_db[k] = self->GVP_db[ks];
			self->GVP_dam[k] = self->GVP_dam[ks];
			self->GVP_dfm[k] = self->GVP_dfm[ks];
			self->GVP_ts[k]  = self->GVP_ts[ks];
			self->GVP_points[k] = self->GVP_points[ks];
			self->GVP_opt[k] = self->GVP_opt[ks];
			self->GVP_vnrep[k] = self->GVP_vnrep[ks];
			self->GVP_vpcjr[k] = self->GVP_vpcjr[ks];
		} 
	else if (a == GVP_SHIFT_DN && ki >= 0 && ki < N_GVP_VECTORS-2)
		for (int k=N_GVP_VECTORS-1; k > ki; --k){
			int ks = k-1;
			self->GVP_du[k] = self->GVP_du[ks];
			self->GVP_dx[k] = self->GVP_dx[ks];
			self->GVP_dy[k] = self->GVP_dy[ks];
			self->GVP_dz[k] = self->GVP_dz[ks];
			self->GVP_da[k] = self->GVP_da[ks];
			self->GVP_db[k] = self->GVP_db[ks];
			self->GVP_dam[k] = self->GVP_dam[ks];
			self->GVP_dfm[k] = self->GVP_dfm[ks];
			self->GVP_ts[k]  = self->GVP_ts[ks];
			self->GVP_points[k] = self->GVP_points[ks];
			self->GVP_opt[k] = self->GVP_opt[ks];
			self->GVP_vnrep[k] = self->GVP_vnrep[ks];
			self->GVP_vpcjr[k] = self->GVP_vpcjr[ks];
		}
	self->update_GUI ();
        return 0;
}


// NetCDF support for parameter storage to file

// helper func
void rpspmc_pacpll_hwi_ncaddvar (NcFile &ncf, const gchar *varname, const gchar *varunit, const gchar *longname, const gchar *shortname, const gchar *label, double value, NcVar &ncv){
	ncv = ncf.addVar (varname, ncDouble);
	ncv.putAtt ("long_name", longname);
	ncv.putAtt ("short_name", shortname);
	ncv.putAtt ("var_unit", varunit);
	if (label) ncv.putAtt ("label", label);
	ncv.putVar (&value);
}

void* rpspmc_pacpll_hwi_ncaddvar (NcFile &ncf, const gchar *varname, const gchar *varunit, const gchar *longname, const gchar *shortname, const gchar *label, int value, NcVar &ncv){
	ncv = ncf.addVar (varname, ncInt);
	ncv.putAtt ("long_name", longname);
	ncv.putAtt ("short_name", shortname);
	ncv.putAtt ("var_unit", varunit);
	if (label) ncv.putAtt ("label", label);
	ncv.putVar (&value);
}

#define SPMHWI_ID "rpspmc_hwi_"

void RPSPMC_Control::save_values (NcFile &ncf){
	NcVar ncv;

	PI_DEBUG (DBG_L4, "RPSPMC_Control::save_values");
	gchar *i=NULL;

        i = g_strconcat ("RP SPM Control HwI ** Hardware-Info:\n", rpspmc_hwi->get_info (), NULL);

	NcDim infod  = ncf.addDim("rpspmc_info_dim", strlen(i));
	NcVar info   = ncf.addVar("rpspmc_info", ncChar, infod);
	info.putAtt ("long_name", "RPSPMC_Control plugin information");
	info.putVar ({0}, {strlen(i)}, i);
	g_free (i);

// Feedback/Scan Parameters from GUI ============================================================

	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"Bias", "V", "RPSPMC: (Sampel or Tip) Bias Voltage", "Bias", "Bias", bias, ncv);
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"Z_Servo_CZ_SetPoint", "A", "RPSPMC: Z-Servo CZ Setpoint", "CZ Set Point", "Z Setpoint", zpos_ref, ncv);
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"Z_Servo_CP_in_dB", "dB", "RPSPMC: Z-Servo CP in dB", "Z SERVO CP in dB", "Z Servo CP (dB)", spmc_parameters.z_servo_cp_db, ncv);
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"Z_Servo_CI_in_dB", "dB", "RPSPMC: Z-Servo CI in dB", "Z SERVO CI in dB", "Z Servo CI (dB)", spmc_parameters.z_servo_ci_db, ncv);
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"Z_Servo_SetPoint", "nA", "RPSPMC: Z-Servo STM/Current/.. Set Point", "Current Setpt.", "Current", mix_set_point[0], ncv);
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"Z_Servo_FLevel", "1", "Z-Servo RPSPMC: FLevel", "Current/.. level", "Level", mix_level[0], ncv);
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"Z_Servo_Transfer_Mode", "BC", "RPSPMC: Z-Servo Transfer Mode", "Z-Servo Transfer Mode", NULL, (double)mix_transform_mode[0], ncv);
	ncv.putAtt ("mode_bcoding", "0:Off, 1:On, 2:Log");

	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"scan_speed_x", "A/s", "RPSPMC: Scan speed X", "Xs Velocity", "Scan Speed", scan_speed_x, ncv);
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"move_speed_x", "A/s", "RPSPMC: Move speed X", "Xm Velocity", "Move Speed", move_speed_x, ncv);

// LockIn Parameters from GUI ============================================================
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"LCK_Mod", "Hz", "RPSPMC: LockIn Mod Freq", "LockIn Mod Freq", "Lck Mod Freq", spmc_parameters.lck_frequency, ncv);

        int jj = spmc_parameters.lck_target;
        //for (int jj=1; modulation_targets[jj].label && jj < LCK_NUM_TARGETS; ++jj)
        {
                gchar *lab = g_strdup_printf ("Modulation Volume for %s", modulation_targets[jj].label);
                gchar *id = g_strdup_printf ("%sLCK_ModVolume_%s", SPMHWI_ID, modulation_targets[jj].label);
                for (gchar *c=id; *c; c++) if (*c == '-') *c='_'; // transcode '-' to '_'
                rpspmc_pacpll_hwi_ncaddvar (ncf, id, modulation_targets[jj].unit, lab, lab, lab, LCK_Volume[jj], ncv);
                g_free (lab); g_free(id);
        }
	rpspmc_pacpll_hwi_ncaddvar (ncf, SPMHWI_ID"LCK_Sens", "mV", "RPSPMC: LockIn Sensitivity", "LockIn Sens", "Lck Sens", spmc_parameters.lck_sens, ncv);

        
// Filter Parameters from GUI ============================================================

        
        
// Vector Probe ============================================================

}


#define NC_GET_VARIABLE(VNAME, VAR) if(!ncf.getVar(VNAME).isNull ()) ncf.getVar(VNAME).getVar(VAR)

void RPSPMC_Control::load_values (NcFile &ncf){
	PI_DEBUG (DBG_L4, "RPSPMC_Control::load_values");
	// Values will also be written in old style DSP Control window for the reason of backwards compatibility
	// OK -- but will be obsoleted and removed at any later point -- PZ
	NC_GET_VARIABLE ("rpspmc_pacpll_hwi_bias", &bias);
	NC_GET_VARIABLE ("rpspmc_pacpll_hwi_bias", &main_get_gapp()->xsm->data.s.Bias);
        NC_GET_VARIABLE ("rpspmc_pacpll_hwi_set_point0", &mix_set_point[0]);

	update_GUI ();
}




void RPSPMC_Control::get_tab_settings (const gchar *tab_key, guint64 &option_flags, guint64 &auto_flags, guint64 glock_data[6]) {
        PI_DEBUG_GP (DBG_L4, "RPSPMC_Control::get_tab_settings for tab '%s'\n", tab_key);

        gint dbg_level = 0;

#define PI_DEBUG_GPX(X, ARGS...) g_print (ARGS);
        
        GVariant *v = g_settings_get_value (hwi_settings, "probe-tab-options");

        // PI_DEBUG_GPX (dbg_level, "probe-tab-options type is '%s'\n", g_variant_get_type (v));
        //        PI_DEBUG_GPX (dbg_level, "probe-tab-options=%s\n", g_variant_print (v,true));
        
        GVariantDict *dict = g_variant_dict_new (v);

        if (!dict){
                PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::get_tab_settings -- can't read dictionary 'probe-tab-options' a{sv}.\n");
                return;
        }
#if 0
        if (!g_variant_dict_contains (dict, tab_key)){
                PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::get_tab_settings -- can't find key '%s' in dict 'probe-tab-options'.\n", tab_key);
        }
        PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::get_tab_settings -- key '%s' found in dict 'probe-tab-options' :)\n", tab_key);
#endif
        
        GVariant *value = g_variant_dict_lookup_value (dict, tab_key, ((const GVariantType *) "at")); // array uint64
        if (!value){
                g_warning ("WARNING (Normal at first start) -- Note: only at FIRST START: Building Settings. RPSPMC_Control::get_tab_settings:\n --> can't get find array 'at' data for key '%s' in 'probe-tab-options' dictionary.\n Storing default now.", tab_key);
                g_print ("WARNING (Normal at first start) -- Note: only at FIRST START: Building Settings. RPSPMC_Control::get_tab_settings:\n --> can't get find array 'at' data for key '%s' in 'probe-tab-options' dictionary.\n Storing default now.", tab_key);

                if (dict)
                        g_variant_dict_unref (dict);
                if (value)
                        g_variant_unref (value);
                
                // make default -- at first start, it is missing as can not be define in schema.
                // uint64 @at [18446744073709551615,..]
                GVariant *v_default = g_variant_new_parsed ("@a{sv} {"
                                                            "'AC': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'IV': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'FZ': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'PL': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'LP': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'SP': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'TS': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'VP': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            //"'GVP': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'TK': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'AX': <@at [0,0,0,0,0,0,0,0,0]>, "
                                                            "'AB': <@at [0,0,0,0,0,0,0,0,0]>"
                                                            "}");
                dict = g_variant_dict_new (v_default);
                value = g_variant_dict_lookup_value (dict, tab_key, ((const GVariantType *) "at"));
                g_variant_unref (v_default);
        }
        if (value){
                guint64  *ai = NULL;
                gsize     ni;
                
                ai = (guint64*) g_variant_get_fixed_array (value, &ni, sizeof (guint64));
                g_assert_cmpint (ni, ==, 9);

#if 0
                PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::get_tab_settings -- reading '%s' tab options [", tab_key);
                for (int i=0; i<ni; ++i)
                        PI_DEBUG_GPX (DBG_L4, "0x%08x, ", ai[i]);
                PI_DEBUG_GPX (dbg_level, "]\n");
#endif
                option_flags = ai[1];
                auto_flags = ai[2];
      
                for (int i=0; i<6; ++i)
                        glock_data [i] = ai[1+2+i];

                // must free data for ourselves
                g_variant_unref (value);
        } else {
                PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::get_tab_settings -- cant get array 'at' from key '%s' in 'probe-tab-options' dictionary. And default rewrite failed also.\n", tab_key);
        }

        GVariant *vto = g_variant_dict_end (dict);

        // testing storing
#if 0
        if (vto){
                PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::get_tab_settings -- stored tab-setings dict to variant OK.\n");
        } else {
                PI_DEBUG_GPX (dbg_level, "ERROR RPSPMC_Control::get_tab_settings -- store tab-setings dict to variant failed.\n");
        }
        if (g_settings_is_writable (hwi_settings, "probe-tab-options")){
                PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::get_tab_settings -- is writable\n");
        } else {
                PI_DEBUG_GPX (dbg_level, "WARNING/ERROR RPSPMC_Control::get_tab_settings -- is NOT writable\n");
        }
#endif
        if (!g_settings_set_value (hwi_settings, "probe-tab-options", vto)){
                PI_DEBUG_GPX (dbg_level, "ERROR RPSPMC_Control::get_tab_settings -- update probe-tab-options dict failed.\n");
        }
        g_variant_dict_unref (dict);
        //        g_variant_unref (vto);
        // PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::get_tab_settings -- done.\n");
}

//                PI_DEBUG (DBG_L1, "key '" << tab_key << "' not found in probe-tab-options.");


void RPSPMC_Control::set_tab_settings (const gchar *tab_key, guint64 option_flags, guint64 auto_flags, guint64 glock_data[6]) {
        GVariant *v = g_settings_get_value (hwi_settings, "probe-tab-options");

        gint dbg_level = 0;

#define PI_DEBUG_GPX(X, ARGS...) g_print (ARGS);

        // PI_DEBUG_GPX (dbg_level, "probe-tab-options type is '%s'\n", g_variant_get_type (v));
        //        PI_DEBUG_GPX (dbg_level, "probe-tab-options old=%s\n", g_variant_print (v,true));

        GVariantDict *dict = g_variant_dict_new (v);

        if (!dict){
                PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::set_tab_settings -- can't read dictionary 'probe-tab-options' a{sai}.\n");
                return;
        }
#if 0
        if (!g_variant_dict_contains (dict, tab_key)){
                PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::set_tab_settings -- can't find key '%s' in dict 'probe-tab-options'.\n", tab_key);
                return;
        }
        PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings -- key '%s' found in dict 'probe-tab-options' :)\n", tab_key);
#endif

        GVariant *value = g_variant_dict_lookup_value (dict, tab_key, ((const GVariantType *) "at"));
        if (!value){
                PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::set_tab_settings -- can't find array 'at' data from key '%s' in 'probe-tab-options' dictionary for update.\n", tab_key);
                g_variant_dict_unref (dict);
                return;
        }
        if (value){
                gsize    ni = 9;
                guint64 *ai = g_new (guint64, ni);

                //                PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings ** key '%s' old v=%s\n", tab_key, g_variant_print (value, true));
#if 0
                // ---- debug read
                guint64  *aix = NULL;
                gsize     nix;
                
                aix = (guint64*) g_variant_get_fixed_array (value, &nix, sizeof (guint64));
                g_assert_cmpint (nix, ==, 9);

                PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings ** key '%s' old x=[", tab_key);
                for (int i=0; i<nix; ++i)
                        PI_DEBUG_GPX (DBG_L4, "0x%08llx, ", aix[i]);
                PI_DEBUG_GPX (dbg_level, "] hex\n");
                PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings ** key '%s' old x=[", tab_key);
                for (int i=0; i<nix; ++i)
                        PI_DEBUG_GPX (DBG_L4, "%lld, ", aix[i]);
                PI_DEBUG_GPX (dbg_level, "] decimal\n");
                // -----end debug read back test
#endif
                // option_flags = option_flags & 0xfff; // mask
                ai[0] = 0;
                ai[1] = option_flags;
                ai[2] = auto_flags;

                // test -- set all the same
                //for (int i=0; i<9; ++i)
                //        ai[i] = auto_flags;

                for (int i=0; i<6; ++i)
                        ai[1+2+i] = glock_data [i];

#if 0
                PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings ** key '%s' new x=[", tab_key);
                for (int i=0; i<ni; ++i)
                        PI_DEBUG_GPX (DBG_L4, "0x%08llx, ", ai[i]);
                PI_DEBUG_GPX (dbg_level, "] hex\n");
#endif
                
                g_variant_unref (value); // free old variant
                value = g_variant_new_fixed_array (g_variant_type_new ("t"), ai, ni, sizeof (guint64));

                // PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings ** key '%s' new v=%s\n", tab_key, g_variant_print (value, true));

                // remove old
                if (!g_variant_dict_remove (dict, tab_key)){
                        PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::set_tab_settings -- cant find and remove key '%s' from 'probe-tab-options' dictionary.\n", tab_key);
                }
                // insert new
                // PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings -- inserting new updated key '%s' into 'probe-tab-options' dictionary.\n", tab_key);
                g_variant_dict_insert_value (dict, tab_key, value);

                g_free (ai); // free array
                // must free data for ourselves
        } else {
                PI_DEBUG_GPX (dbg_level, "ERROR: RPSPMC_Control::set_tab_settings -- cant get array 'au' from key '%s' in 'probe-tab-options' dictionary.\n", tab_key);
        }

        GVariant *vto = g_variant_dict_end (dict);

        // PI_DEBUG_GPX (dbg_level, "probe-tab-options new=%s\n", g_variant_print (vto, true));

        // testing storing
        if (!vto){
                PI_DEBUG_GPX (dbg_level, "ERROR RPSPMC_Control::set_tab_settings -- store tab-setings dict to variant failed.\n");
        }
        if (!g_settings_set_value (hwi_settings, "probe-tab-options", vto)){
                PI_DEBUG_GPX (dbg_level, "ERROR RPSPMC_Control::set_tab_settings -- update probe-tab-options dict failed.\n");
        }
        g_variant_dict_unref (dict);
        //        g_variant_unref (vto);

        //        PI_DEBUG_GPX (dbg_level, "RPSPMC_Control::set_tab_settings -- done.\n");
        return;
}


// MAIN HwI GUI window construction


void RPSPMC_Control::AppWindowInit(const gchar *title){
        if (title) { // stage 1
                PI_DEBUG (DBG_L2, "RPSPMC_Control::AppWindowInit -- header bar");

                app_window = gxsm4_app_window_new (GXSM4_APP (main_get_gapp()->get_application ()));
                window = GTK_WINDOW (app_window);

                header_bar = gtk_header_bar_new ();
                gtk_widget_show (header_bar);
                // hide close, min, max window decorations
                //gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header_bar), false);

                gtk_window_set_title (GTK_WINDOW (window), title);
                // gtk_header_bar_set_title ( GTK_HEADER_BAR (header_bar), title);
                // gtk_header_bar_set_subtitle (GTK_HEADER_BAR  (header_bar), title);
                gtk_window_set_titlebar (GTK_WINDOW (window), header_bar);

                v_grid = gtk_grid_new ();

                gtk_window_set_child (GTK_WINDOW (window), v_grid);
                g_object_set_data (G_OBJECT (window), "v_grid", v_grid); // was "vbox"

                gtk_widget_show (GTK_WIDGET (window));               
        } else {
                PI_DEBUG (DBG_L2, "RPSPMC_Control::AppWindowInit -- header bar -- stage two, hook configure menu");

                win_RPSPMC_popup_entries[0].state = g_settings_get_boolean (hwi_settings, "configure-mode") ? "true":"false"; // get last state
                g_action_map_add_action_entries (G_ACTION_MAP (app_window),
                                                 win_RPSPMC_popup_entries, G_N_ELEMENTS (win_RPSPMC_popup_entries),
                                                 this);

                // create window PopUp menu  ---------------------------------------------------------------------
                GtkWidget *mc_popup_menu = gtk_popover_menu_new_from_model (G_MENU_MODEL (main_get_gapp()->get_hwi_mover_popup_menu ())); // fix me -- reusuing same menu def (from DSP MOVER)

                // attach popup menu configuration to tool button --------------------------------
                GtkWidget *header_menu_button = gtk_menu_button_new ();
                gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (header_menu_button), "applications-utilities-symbolic");
                gtk_menu_button_set_popover (GTK_MENU_BUTTON (header_menu_button), mc_popup_menu);
                gtk_header_bar_pack_end (GTK_HEADER_BAR (header_bar), header_menu_button);
                gtk_widget_show (header_menu_button);
        }
}

int RPSPMC_Control::choice_Ampl_callback (GtkWidget *widget, RPSPMC_Control *spmsc){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	gint i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	gint j = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "chindex"));
	switch(j){
	case 0: rpspmc_pacpll_hwi_pi.app->xsm->Inst->VX(i);
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_DSP_OFFSET_ADDING)
			rpspmc_pacpll_hwi_pi.app->xsm->Inst->VX0(i);
		break;
	case 1: rpspmc_pacpll_hwi_pi.app->xsm->Inst->VY(i);
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_DSP_OFFSET_ADDING)
			rpspmc_pacpll_hwi_pi.app->xsm->Inst->VY0(i);
		break;
	case 2: rpspmc_pacpll_hwi_pi.app->xsm->Inst->VZ(i); 
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_DSP_OFFSET_ADDING)
			rpspmc_pacpll_hwi_pi.app->xsm->Inst->VZ0(i); 
		break;
	case 3:
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING)
			rpspmc_pacpll_hwi_pi.app->xsm->Inst->VX0(i);
		break;
	case 4:
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING)
			rpspmc_pacpll_hwi_pi.app->xsm->Inst->VY0(i);
		break;
	case 5:
		if (main_get_gapp()->xsm->Inst->OffsetMode() == OFM_ANALOG_OFFSET_ADDING)
			rpspmc_pacpll_hwi_pi.app->xsm->Inst->VZ0(i);
		break;
	}

	PI_DEBUG (DBG_L2, "Ampl: Ch=" << j << " i=" << i );
	rpspmc_pacpll_hwi_pi.app->spm_range_check(NULL, rpspmc_pacpll_hwi_pi.app);

        rpspmc_hwi->update_hardware_mapping_to_rpspmc_source_signals ();
        
	return 0;
}

// No-op to prevent w from propagating "scroll" events it receives.

void disable_scroll_cb( GtkWidget *w ) {}

void RPSPMC_Control::create_folder (){
        GtkWidget *notebook;
 	GSList *zpos_control_list=NULL;
	GSList *multi_IVsec_list=NULL;

        GSList *FPGA_readback_update_list=NULL;
        GSList *EC_FPGA_SPMC_server_settings_list=NULL;
        GSList *EC_MONITORS_list = NULL;
        
        AppWindowInit ("RP-SPM Control Window");

        // ========================================
        notebook = gtk_notebook_new ();
        gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
        gtk_notebook_popup_enable (GTK_NOTEBOOK (notebook));

        gtk_widget_show (notebook);
        gtk_grid_attach (GTK_GRID (v_grid), notebook, 1,1, 1,1);


        bp = new GUI_Builder (v_grid);
        bp->set_error_text ("Invalid Value.");
        bp->set_input_width_chars (10);
        bp->set_no_spin ();

        bp->set_pcs_remote_prefix ("dsp-"); // for script compatibility reason keeping this the same for all HwI.

// ==== Folder: Feedback & Scan ========================================
        bp->start_notebook_tab (notebook, "Feedback & Scan", "rpspmc-tab-feedback-scan", hwi_settings);

	bp->new_grid_with_frame ("SPM Controls");
        
        // ------- FB Characteristics
        bp->start (); // start on grid top and use widget grid attach nx=4
        bp->set_scale_nx (4); // set scale width to 4
        bp->set_input_width_chars (12);
        bp->set_label_width_chars (10);

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::BiasChanged, this);
        bp->grid_add_ec_with_scale ("Bias", Volt, &bias, -10., 10., "4g", 0.001, 0.01, "fbs-bias");
        FPGA_readback_update_list = g_slist_prepend (FPGA_readback_update_list, bp->ec); // add to FPGA reconnect readback parameter list
        //        bp->ec->set_adjustment_mode (PARAM_CONTROL_ADJUSTMENT_LOG | PARAM_CONTROL_ADJUSTMENT_LOG_SYM | PARAM_CONTROL_ADJUSTMENT_DUAL_RANGE | PARAM_CONTROL_ADJUSTMENT_ADD_MARKS ); // LIN:0, LOG:1, SYM:2, DR: 4, TICS:8
        bp->ec->set_logscale_min (1e-3);

        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);
        bp->ec->SetScaleWidget (bp->scale, 0);
        bp->new_line ();

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ZPosSetChanged, this);
        //bp->set_input_width_chars (20);
        //bp->set_input_nx (2);
        bp->grid_add_ec_with_scale ("Z-Pos/Setpoint", Angstroem, &zpos_ref, -1000., 1000., "6g", 0.01, 0.1, "adv-dsp-zpos-ref");
        FPGA_readback_update_list = g_slist_prepend (FPGA_readback_update_list, bp->ec); // add to FPGA reconnect readback parameter list
        bp->ec->SetScaleWidget (bp->scale, 0);
        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);
        //ZPos_ec = bp->ec;
        bp->grid_add_ec ("", Angstroem, &zpos_mon, -1000., 1000., "6g", 0.01, 0.1, "adv-dsp-zpos-mon");
        ZPos_ec = bp->ec;
        zpos_control_list = g_slist_prepend (zpos_control_list, bp->ec);

        bp->set_input_width_chars (12);
        bp->set_input_nx ();

        #if 0
	bp->grid_add_check_button ("Z-Pos Monitor",
                                   "Z Position Monitor. Disable to set Z-Position Setpoint for const height mode. "
                                   "Switch Transfer to CZ-FUZZY-LOG for Z-CONTROL Constant Heigth Mode Operation with current compliance given by Fuzzy-Level. "
                                   "Slow down feedback to minimize instabilities. "
                                   "Set Current setpoint a notch below compliance level.",
                                   1,
                                   G_CALLBACK (RPSPMC_Control::zpos_monitor_callback), this,
                                   0, 0);
        GtkWidget *zposmon_checkbutton = bp->button;
        #endif
        bp->new_line ();

        bp->set_configure_list_mode_off ();

        // MIXER headers

        bp->set_input_width_chars (12);
        bp->set_label_width_chars (10);
        bp->set_configure_list_mode_on ();
	bp->grid_add_label ("Z-Servo");
        z_servo_current_source_options_selector = bp->grid_add_z_servo_current_source_options (0, (int)spmc_parameters.z_servo_src_mux, this);
        bp->set_configure_list_mode_off ();

        bp->grid_add_label ("Setpoint", NULL, 4);

        bp->set_configure_list_mode_on ();
        bp->set_input_width_chars (6);
        bp->set_label_width_chars (6);
        //bp->grid_add_label ("Gain");

        bp->set_configure_list_mode_on ();
        bp->grid_add_label ("In-Offset");
        bp->set_configure_list_mode_off ();
        bp->grid_add_label ("Fuzzy-Level");
        bp->set_configure_list_mode_on ();
        bp->grid_add_label ("Transfer");
        bp->set_configure_list_mode_off ();

        bp->new_line ();

        bp->set_scale_nx (4);

        // Build MIXER CHANNELs
        UnitObj *mixer_unit[4] = { Current, Frq, Volt, Volt };
        const gchar *mixer_channel_label[4] = { "STM Current", "AFM/Force", "Mix2", "Mix3" };
        const gchar *mixer_remote_id_set[4] = { "fbs-mx0-current-set",  "fbs-mx1-freq-set",   "fbs-mx2-set",   "fbs-mx3-set" };
        const gchar *mixer_remote_id_gn[4]  = { "fbs-mx0-current-gain" ,"fbs-mx1-freq-gain",  "fbs-mx2-gain",  "fbs-mx3-gain" };
        const gchar *mixer_remote_id_oc[4]  = { "fbs-mx0-current-oc" ,"fbs-mx1-freq-oc",  "fbs-mx2-oc",  "fbs-mx3-oc" };
        const gchar *mixer_remote_id_fl[4]  = { "fbs-mx0-current-level","fbs-mx1-freq-level", "fbs-mx2-level", "fbs-mx3-level" };

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ZServoParamChanged, this);
        // Note: transform mode is always default [LOG,OFF,OFF,OFF] -- NOT READ BACK FROM DSP -- !!!
        for (gint ch=0; ch<1; ++ch){

                GtkWidget *signal_select_widget = NULL;
                UnitObj *tmp = NULL;
#if 0
                // add feedback mixer source signal selection
                signal_select_widget = bp->grid_add_mixer_input_options (ch, mix_fbsource[ch], this);
                if (ch > 1){
                        const gchar *u =  sranger_common_hwi->lookup_dsp_signal_managed (mix_fbsource[ch])->unit;

                        switch (u[0]){
                        case 'V': tmp = new UnitObj("V","V"); break;
                        case 'A' : tmp = new UnitObj(UTF8_ANGSTROEM,"A"); break;
                        case 'H': tmp = new UnitObj("Hz","Hz"); break;
                        case 'd': tmp = new UnitObj(UTF8_DEGREE,"Deg"); break;
                        case 's': tmp = new UnitObj("s","sec"); break;
                        case 'C': tmp = new UnitObj("#","CNT"); break;
                        case 'm': tmp = new UnitObj("V","V"); break;
                        case '1': tmp = new UnitObj("x1","x1"); break;
                        case 'X': tmp = new UnitObj("X","Flag"); break;
                        case 'x': tmp = new UnitObj("xV","xV"); break;
                        case '*': tmp = new UnitObj("*V","*V"); break;
                        default: tmp = new UnitObj("V","V"); break;
                        }
                        mixer_unit[ch] = tmp;
                }                
#else
                // predef label
                bp->grid_add_label (mixer_channel_label[ch]);
#endif

                bp->grid_add_ec_with_scale (NULL, mixer_unit[ch], &mix_set_point[ch], ch==0? 0.0:-100.0, 100., "4g", 0.001, 0.01, mixer_remote_id_set[ch]);
                FPGA_readback_update_list = g_slist_prepend (FPGA_readback_update_list, bp->ec); // add to FPGA reconnect readback parameter list
                // bp->ec->set_adjustment_mode (PARAM_CONTROL_ADJUSTMENT_LOG | PARAM_CONTROL_ADJUSTMENT_ADD_MARKS );
                bp->ec->set_logscale_min (1e-4);
                bp->ec->set_logscale_magshift (-3);
                bp->ec->SetScaleWidget (bp->scale, 0);
                gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);

                if (signal_select_widget)
                        g_object_set_data (G_OBJECT (signal_select_widget), "related_ec_setpoint", bp->ec);

                
                // bp->add_to_configure_hide_list (bp->scale);
                bp->set_input_width_chars (6);
                bp->set_configure_list_mode_on ();
                //bp->grid_add_ec (NULL, Unity, &mix_gain[ch], -1.0, 1.0, "5g", 0.001, 0.01, mixer_remote_id_gn[ch]);
                //bp->set_configure_list_mode_on ();
                bp->grid_add_ec (NULL, mixer_unit[ch], &mix_in_offsetcomp[ch], -1.0, 1.0, "5g", 0.001, 0.01, mixer_remote_id_oc[ch]);
                bp->set_configure_list_mode_off ();
                bp->grid_add_ec (NULL, mixer_unit[ch], &mix_level[ch], -100.0, 100.0, "5g", 0.001, 0.01, mixer_remote_id_fl[ch]);
                bp->set_configure_list_mode_on ();
                if (tmp) delete (tmp); // done setting unit -- if custom
                
                if (signal_select_widget)
                        g_object_set_data (G_OBJECT (signal_select_widget), "related_ec_level", bp->ec);
                
                z_servo_options_selector[ch] = bp->grid_add_mixer_options (ch, mix_transform_mode[ch], this);
                bp->set_configure_list_mode_off ();
                bp->new_line ();
        }
        //bp->set_configure_list_mode_off ();

        bp->set_input_width_chars ();
        
        // ========== Z-Servo
        // bp->pop_grid ();
        //bp->new_line ();
        //bp->new_grid_with_frame ("Z-Servo");

        bp->set_label_width_chars (7);
        bp->set_input_width_chars (12);

        bp->set_configure_list_mode_on ();
	bp->grid_add_ec_with_scale ("CP", dB, &spmc_parameters.z_servo_cp_db, -100., 20., "5g", 1.0, 0.1, "fbs-cp"); // z_servo[SERVO_CP]
        GtkWidget *ZServoCP = bp->input;
        FPGA_readback_update_list = g_slist_prepend (FPGA_readback_update_list, bp->ec); // add to FPGA reconnect readback parameter list
        bp->set_configure_list_mode_off ();

        mix_level[2] = 5.;
        mix_level[3] = -5.;
        // *** spmc_parameters.z_servo_upper *** readback also, alt control via Z-Position, so must unlink here
        bp->set_configure_list_mode_on ();
        bp->grid_add_ec ("Z Upper", Volt, &z_limit_upper_v, -5.0, 5.0, "5g", 0.001, 0.01,"fbs-upper");
        bp->set_configure_list_mode_off ();
        FPGA_readback_update_list = g_slist_prepend (FPGA_readback_update_list, bp->ec); // add to FPGA reconnect readback parameter list
        ec_z_upper = bp->ec;
        
        bp->new_line ();
        bp->grid_add_ec_with_scale ("CI", dB, &spmc_parameters.z_servo_ci_db, -100., 20., "5g", 1.0, 0.1, "fbs-ci"); // z_servo[SERVO_CI
        GtkWidget *ZServoCI = bp->input;
        FPGA_readback_update_list = g_slist_prepend (FPGA_readback_update_list, bp->ec); // add to FPGA reconnect readback parameter list

        g_object_set_data( G_OBJECT (ZServoCI), "HasClient", ZServoCP);
        g_object_set_data( G_OBJECT (ZServoCP), "HasMaster", ZServoCI);
        g_object_set_data( G_OBJECT (ZServoCI), "HasRatio", GINT_TO_POINTER((guint)round(1000.*spmc_parameters.z_servo_cp_db/spmc_parameters.z_servo_ci_db)));
        
        bp->set_configure_list_mode_on ();
        bp->grid_add_ec ("Z Lower", Volt, &spmc_parameters.z_servo_lower, -5.0, 5.0, "5g", 0.001, 0.01,"fbs-lower");
        bp->set_configure_list_mode_off ();
        FPGA_readback_update_list = g_slist_prepend (FPGA_readback_update_list, bp->ec); // add to FPGA reconnect readback parameter list
        
        bp->new_line ();
        bp->set_label_width_chars ();

//#define ENABLE_ZSERVO_POLARITY_BUTTON  // only for development and testing
        // Z OUPUT POLARITY:
        spmc_parameters.z_polarity      = xsmres.ScannerZPolarity ? 1 : -1;    // 1: pos, 0: neg (bool) -- adjust zpos_ref accordingly!
        spmc_parameters.gxsm_z_polarity = -(xsmres.ScannerZPolarity ? 1 : -1); // Gxsm: negative internally required to have native Z positive or up = tip up or higher surface

#ifdef  ENABLE_ZSERVO_POLARITY_BUTTON
        bp->grid_add_check_button ("Enable", "enable Z servo feedback controller." PYREMOTE_CHECK_HOOK_KEY("MainZservo"), 1,
                                   G_CALLBACK(RPSPMC_Control::ZServoControl), this, ((int)spmc_parameters.gvp_status)&1, 0);


        bp->grid_add_check_button ( N_("Invert (Neg Polarity)"), "Z-Output Polarity. ** startup setting, does NOT for sure reflect actual setting unless toggled!", 1,
                                    G_CALLBACK (RPSPMC_Control::ZServoControlInv), this, spmc_parameters.z_polarity > 1 ? 0:1 );

#endif


	// ========================================

        PI_DEBUG (DBG_L4, "SPMC----SCAN ------------------------------- ");
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Scan Characteristics");

        bp->start (4); // wx=4
        bp->set_label_width_chars (7);

        bp->set_configure_list_mode_on ();

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ChangedNotifyMoveSpeed, this);
	bp->grid_add_ec_with_scale ("MoveSpd", Speed, &move_speed_x, 0.1, 10000., "5g", 1., 10., "fbs-scan-speed-move");
        bp->ec->set_logscale_min (1);
        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);
        bp->ec->SetScaleWidget (bp->scale, 0);
        bp->new_line ();
        bp->set_configure_list_mode_off ();

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ChangedNotifyScanSpeed, this);
	bp->grid_add_ec_with_scale ("ScanSpd", Speed, &scan_speed_x_requested, 0.1, 100000., "5g", 1., 10., "fbs-scan-speed-scan");
        bp->ec->set_logscale_min (1);
        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);
        bp->ec->SetScaleWidget (bp->scale, 0);
        scan_speed_ec = bp->ec;

        bp->new_line ();
	bp->grid_add_ec ("Fast Return", Unity, &fast_return, 1., 1000., "5g", 1., 10.,  "adv-scan-fast-return");

#if 0
        bp->set_configure_list_mode_on ();
	bp->grid_add_ec ("XS 2nd ZOff", Angstroem, &x2nd_Zoff, -10000., 10000., ".2f", 1., 1., "adv-scan-xs2nd-z-offset");
        bp->set_configure_list_mode_off ();
#endif

        bp->new_line ();
        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::Slope_dZX_Changed, this);
        bp->grid_add_ec_with_scale ("Slope X", Unity, &area_slope_x, -0.2, 0.2, ".5f", 0.0001, 0.0001,  "adv-scan-slope-x"); slope_x_ec = bp->ec;
        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);
        bp->new_line ();

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::Slope_dZY_Changed, this);
        bp->grid_add_ec_with_scale ("Slope Y", Unity, &area_slope_y, -0.2, 0.2, ".5f", 0.0001, 0.0001,  "adv-scan-slope-y"); slope_y_ec = bp->ec;
        gtk_scale_set_digits (GTK_SCALE (bp->scale), 5);

        bp->set_scale_nx ();
        bp->new_line (0,2);

        //bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ChangedNotify, this); // set default handler

        //bp->grid_add_check_button ("Enable automatic Tip return to center", "enable auto tip return to center after scan", 1,
        //                               G_CALLBACK(RPSPMC_Control::DSP_cret_callback), this, center_return_flag, 0);
        //g_settings_bind (hwi_settings, "adv-enable-tip-return-to-center",
        //                 G_OBJECT (GTK_CHECK_BUTTON (bp->button)), "active",
        //                 G_SETTINGS_BIND_DEFAULT);

        PI_DEBUG (DBG_L4, "SPMC----FB-CONTROL -- INPUT-SRCS ----------------------------- ");

	// ========== SCAN CHANNEL INPUT SOURCE CONFIGURATION MENUS
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Configure Scan Source GVP MUX Selectors (Swappable Signals)");

        bp->set_configure_list_mode_on ();
        bp->add_to_configure_list (bp->frame); // manage en block
        bp->set_configure_list_mode_off ();
        
        bp->new_line ();

        for (int i=0; i<6; ++i){
                bp->grid_add_scan_input_signal_options (i, scan_source[i], this);
                VPScanSrcVPitem[i] =  bp->wid;
        }

#if 0
        // ========== LDC -- Enable Linear Drift Correction -- Controls
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Enable Linear Drift Correction (LDC) -- N/A on RPSPMC at this time");
        bp->set_configure_list_mode_on ();

	LDC_status = bp->grid_add_check_button ("Enable Linear Drift Correction","Enable Linear Drift Correction" PYREMOTE_CHECK_HOOK_KEY("MainLDC"), 3);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (LDC_status), 0);
	ldc_flag = 0;
	g_signal_connect (G_OBJECT (LDC_status), "toggled",
			    G_CALLBACK (RPSPMC_Control::ldc_callback), this);


        // LDC settings
	bp->grid_add_ec ("LDCdX", Speed, &dxdt, -100., 100., "5g", 0.001, 0.01, "fbs-scan-ldc-dx");
        bp->grid_add_ec ("dY", Speed, &dydt, -100., 100., "5g", 0.001, 0.01, "fbs-scan-ldc-dy");
        bp->grid_add_ec ("dZ", Speed, &dzdt, -100., 100., "5g", 0.001, 0.01, "fbs-scan-ldc-dz");

        bp->set_configure_list_mode_off ();

#endif

	// ========== Piezo Drive / Amplifier Settings
        bp->set_input_width_chars (8);
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Piezo Drive Settings");

        const gchar *PDR_gain_label[6] = { "VX", "VY", "VZ", "VX0", "VY0", "VZ0" };
        const gchar *PDR_gain_key[6] = { "vx", "vy", "vz", "vx0", "vy0", "vz0" };
        for(int j=0; j<6; j++) {
                if (j == 3 && main_get_gapp()->xsm->Inst->OffsetMode () == OFM_DSP_OFFSET_ADDING)
                        break;

                gtk_label_set_width_chars (GTK_LABEL (bp->grid_add_label (PDR_gain_label[j])), 6);
        
                GtkWidget *wid = gtk_combo_box_text_new ();
                g_object_set_data  (G_OBJECT (wid), "chindex", GINT_TO_POINTER (j));

                // Init gain-choicelist
                for(int ig=0; ig<GAIN_POSITIONS; ig++){
                        AmpIndex  AmpI;
                        AmpI.l = 0L;
                        AmpI.s.ch = j;
                        AmpI.s.x  = ig;
                        gchar *Vxyz = g_strdup_printf("%g",xsmres.V[ig]);
                        gchar *id = g_strdup_printf ("%ld",AmpI.l);
                        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), id, Vxyz);
                        g_free (id);
                        g_free (Vxyz);
                }

                g_signal_connect (G_OBJECT (wid), "changed",
                                  G_CALLBACK (RPSPMC_Control::choice_Ampl_callback),
                                  this);

                gchar *tmpkey = g_strconcat ("spm-rpspmc-h", PDR_gain_key[j], NULL); 
                // get last setting, will call callback connected above to update gains!
                g_settings_bind (hwi_settings, tmpkey,
                                 G_OBJECT (GTK_COMBO_BOX (wid)), "active",
                                 G_SETTINGS_BIND_DEFAULT);
                g_free (tmpkey);

                //VXYZS0Gain[j] = wid; // store for remote access/manipulation
                bp->grid_add_widget (wid);
                bp->add_to_scan_freeze_widget_list (wid);
        }
        bp->pop_grid ();
        bp->new_line ();

        bp->notebook_tab_show_all ();
        bp->pop_grid ();

        // ==== Folder: LockIn  ========================================
        bp->new_grid (); // x=y=1  set_xy (x,y)
        bp->start_notebook_tab (notebook, "Lock-In", "rpspmc-tab-lockin", hwi_settings);

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::lockin_adjust_callback, this);
 	bp->new_grid_with_frame ("Lock-In Control and Routing");

        LCK_unit = new UnitAutoMag ("V","V");
        LCK_unit->add_ref ();
        bp->grid_add_ec ("Magnitude Reading", LCK_unit, &spmc_parameters.lck1_bq2_mag_monitor, -10.0, 10.0, ".03g", 0.1, 1., "LCK-MAG-MONITOR");
        LCK_Reading = bp->ec;
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();

        bp->grid_add_ec ("Phase Reading", Deg, &spmc_parameters.lck1_bq2_ph_monitor, -180.0, 180.0, ".03g", 0.1, 1., "LCK-PH-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();

        bp->grid_add_ec ("aCLKs/S", Unity, &spmc_parameters.lck_aclocks_per_sample_monitor, 0.0, 1e6, "6.0f", 0.1, 1., "LCK-ACLK-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();

        bp->grid_add_ec ("LCK-Decii", Unity, &spmc_parameters.lck_decii_monitor, 0.0, 1e6, "6.0f", 0.1, 1., "LCK-DECII-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();

        bp->grid_add_ec ("LCK-ilen", Unity, &spmc_parameters.lck_ilen_monitor, 0.0, 1e6, "6.0f", 0.1, 1., "LCK-ILEN-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();

        bp->grid_add_ec ("BQDec#", Unity, &spmc_parameters.lck_bq_dec_monitor, 0.0, 1e6, "6.0f", 0.1, 1., "LCK-BQDEC-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();

	bp->grid_add_ec ("Modulation Frequency", new UnitObj("Hz","Hz"), &spmc_parameters.lck_frequency, 0.0, 10e6, "5g", 1.0, 100.0, "SPMC-LCK-FREQ");
        LCK_ModFrq = bp->ec;
        
        bp->new_line ();
        bp->grid_add_label ("Modulation on");
        bp->grid_add_modulation_target_options (0, (int)spmc_parameters.lck_target, this);

        LCK_Volume[0]=0.0; // #0 is not used
        for (int jj=1; modulation_targets[jj].label && jj < LCK_NUM_TARGETS; ++jj){
                bp->new_line ();
                gchar *lab = g_strdup_printf ("Volume for %s", modulation_targets[jj].label);
                gchar *id = g_strdup_printf ("SPMC-LCK-ModVolume-%s", modulation_targets[jj].label);
                UnitObj *u = new UnitObj(modulation_targets[jj].unit_sym, modulation_targets[jj].unit);
                bp->grid_add_ec (lab, u, &LCK_Volume[jj], 0., 1e6, "5g", 0.001, 0.01, id);
                g_free (lab);
                g_free (id);
                LCK_VolumeEntry[jj]=bp->input;
                gtk_widget_set_sensitive (bp->input, false);
        }

        bp->new_line ();
        lck_gain=1.;
	bp->grid_add_ec ("Lck Sens.", new UnitObj("mV","mV"), &spmc_parameters.lck_sens, 0.001, 5000., "5g", 1.0, 10.0, "SPMC-LCK-SENS");
        LCK_Sens = bp->ec;
        bp->new_line ();
        bp->grid_add_check_button ("CorrS PH Aligned", bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Lock-In Corr Phase Aligned","dsp-lck-phaligned"), 1,
                                   GCallback (callback_change_LCK_mode), this,
                                   &spmc_parameters.lck_mode, 4);

        // ***
        bp->pop_grid (); bp->set_xy (2,2);
 	bp->new_grid_with_frame ("Filter Section 1");

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::bq_filter_adjust_callback, this);

        bp->grid_add_label ("Filter type");

        const gchar *filter_types[] = { "None/Pass", "--", "--", "From AB", "Stop", "By-Pass", "Disable", NULL };

        GtkWidget *combo_bqfilter_type = gtk_combo_box_text_new ();
        for (int jj=0; filter_types[jj]; ++jj){
                gchar *id = g_strdup_printf ("%d", jj);
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo_bqfilter_type), id, filter_types[jj]);
                g_free (id);
        }
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo_bqfilter_type), 3);
        update_bq_filterS1_widget = combo_bqfilter_type;

        g_object_set_data (G_OBJECT (combo_bqfilter_type), "section", GINT_TO_POINTER (1)); 
        g_signal_connect (G_OBJECT (combo_bqfilter_type), "changed",
                          G_CALLBACK (RPSPMC_Control::choice_BQfilter_type_callback),
                          this);
        bp->grid_add_widget (combo_bqfilter_type);
        bp->new_line ();
        
        UnitObj *unity_unit = new UnitObj(" "," ");
        for(int jj=0; jj<6; ++jj){
                spmc_parameters.sc_bq_coef[jj]=0.;
                spmc_parameters.sc_bq_coef[0]=1.;
                gchar *l = g_strdup_printf ("BQ1 %s%d", jj<3?"b":"a", jj<3?jj:jj-3);
                gchar *id = g_strdup_printf ("SPMC-LCK-BQ1-COEF-BA");
                bp->grid_add_ec (l, unity_unit, &spmc_parameters.sc_bq1_coef[jj], -1e10, 1e10, "g", 1., 10., id, jj);
                if (jj==5) bp->init_ec_array ();
                g_object_set_data (G_OBJECT (combo_bqfilter_type), id, bp->ec);
                g_free (l);
                g_free (id);
                bp->new_line ();
        }	

        //
        bp->pop_grid (); bp->set_xy (3,2);
 	bp->new_grid_with_frame ("Filter Section 2");

        bp->grid_add_label ("Filter type");
        combo_bqfilter_type = gtk_combo_box_text_new ();
        for (int jj=0; filter_types[jj]; ++jj){
                gchar *id = g_strdup_printf ("%d", jj);
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo_bqfilter_type), id, filter_types[jj]);
                g_free (id);
        }
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo_bqfilter_type), 3);
        update_bq_filterS2_widget = combo_bqfilter_type;
        g_object_set_data (G_OBJECT (combo_bqfilter_type), "section", GINT_TO_POINTER (2)); 
        g_signal_connect (G_OBJECT (combo_bqfilter_type), "changed",
                          G_CALLBACK (RPSPMC_Control::choice_BQfilter_type_callback),
                          this);
        bp->grid_add_widget (combo_bqfilter_type);
        bp->new_line ();

        for(int jj=0; jj<6; ++jj){
                spmc_parameters.sc_bq_coef[jj]=0.;
                spmc_parameters.sc_bq_coef[0]=1.;
                gchar *l = g_strdup_printf ("BQ2 %s%d", jj<3?"b":"a", jj<3?jj:jj-3);
                gchar *id = g_strdup_printf ("SPMC-LCK-BQ2-COEF-BA");
                bp->grid_add_ec (l, unity_unit, &spmc_parameters.sc_bq2_coef[jj], -1e10, 1e10, "g", 1., 10., id, jj);
                if (jj==5) bp->init_ec_array ();
                g_object_set_data (G_OBJECT (combo_bqfilter_type), id, bp->ec);
                g_free (l);
                g_free (id);
                bp->new_line ();
        }	
        
        //
        bp->pop_grid (); bp->set_xy (4,2);
 	bp->new_grid_with_frame ("Z Servo Input BiQuad Filter");

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::zs_input_filter_adjust_callback, this);
       
        bp->grid_add_label ("Filter type");
        combo_bqfilter_type = gtk_combo_box_text_new ();
        for (int jj=0; filter_types[jj]; ++jj){
                gchar *id = g_strdup_printf ("%d", jj);
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo_bqfilter_type), id, filter_types[jj]);
                g_free (id);
        }
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo_bqfilter_type), 5);
        update_bq_filterZS_widget = combo_bqfilter_type;
        g_object_set_data (G_OBJECT (combo_bqfilter_type), "section", GINT_TO_POINTER (3)); 
        g_signal_connect (G_OBJECT (combo_bqfilter_type), "changed",
                          G_CALLBACK (RPSPMC_Control::choice_BQfilter_type_callback),
                          this);
        bp->grid_add_widget (combo_bqfilter_type);
        bp->new_line ();
        for(int jj=0; jj<6; ++jj){
                spmc_parameters.sc_zs_bq_coef[jj]=0.;
                spmc_parameters.sc_zs_bq_coef[0]=1.;
                gchar *l = g_strdup_printf ("ZSBQ %s%d", jj<3?"b":"a", jj<3?jj:jj-3);
                gchar *id = g_strdup_printf ("SPMC-ZS-BQ-COEF-BA");
                bp->grid_add_ec (l, unity_unit, &spmc_parameters.sc_zs_bq_coef[jj], -1e10, 1e10, "g", 1., 10., id, jj);
                if (jj==5) bp->init_ec_array ();
                g_object_set_data (G_OBJECT (combo_bqfilter_type), id, bp->ec);
                g_free (l);
                g_free (id);
                bp->new_line ();
        }	
        
        // ***
        bp->pop_grid (); bp->set_xy (5,2);
 	bp->new_grid_with_frame ("RF Generator and AM/FM Control Setup");

        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::rfgen_adjust_callback, this);

	bp->grid_add_ec ("RF Center", new UnitObj("Hz","Hz"), &spmc_parameters.rf_gen_frequency, 0.0, 30e6, "5g", 1.0, 100.0, "SPMC-RF-GEN-FREQ");
        bp->new_line ();
	bp->grid_add_label ("AM Modulation");
        bp->grid_add_check_button ("En", bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Enable AM","dsp-gvp-RFAM"), 1,
                                   GCallback (callback_change_RF_mode), this,
                                   &spmc_parameters.rf_gen_mode, 1);
        bp->new_line ();
	bp->grid_add_ec ("FM Mod Scale", new UnitObj("Hz/V","Hz/V"), &spmc_parameters.rf_gen_fmscale, -1e9, 1e9, "6g", 0.1, 5.0, "SPMC-RF-GEN-FMSCALE");
        bp->grid_add_check_button ("En", bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Enable FM","dsp-gvp-RFFM"), 1,
                                   GCallback (callback_change_RF_mode), this,
                                   &spmc_parameters.rf_gen_mode, 2);

        bp->new_line ();
        bp->set_label_width_chars (10);
	bp->grid_add_label ("RF Gen Output on");
        bp->grid_add_rf_gen_out_options (0, (int)spmc_parameters.rf_gen_out_mux, this);
        
        //***
        
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("... for convenience: Execute GVP, IV buttons");

	bp->grid_add_button ("Execute GVP");
	g_signal_connect (G_OBJECT (bp->button), "clicked",
                          GCallback (RPSPMC_Control::Probing_exec_GVP_callback), this);
        
	bp->grid_add_button ("Execute STS");
	g_signal_connect (G_OBJECT (bp->button), "clicked",
                          GCallback (RPSPMC_Control::Probing_exec_IV_callback), this);
        
        bp->notebook_tab_show_all ();
        bp->pop_grid ();

        
// ==== Folder Set for Vector Probe ========================================
// ==== Folder: I-V STS setup ========================================
        PI_DEBUG (DBG_L4, "SPMC----TAB-IV ------------------------------- ");

        bp->new_grid ();
        bp->start_notebook_tab (notebook, "STS", "rpspmc-tab-sts", hwi_settings);
	// ==================================================
        bp->new_grid_with_frame ("I-V Type Spectroscopy");
	// ==================================================
        GtkWidget *multiIV_checkbutton = NULL;
#if 1 // SIMPLE IV
        IV_sections  = 0;  // simple mode
        multiIV_mode = -1; // simple mode
        
	bp->grid_add_label ("Start"); bp->grid_add_label ("End");  bp->grid_add_label ("Points"); bp->grid_add_label ("Slope"); bp->grid_add_label ("dZ");
        bp->new_line ();

        bp->grid_add_ec (NULL, Volt, &IV_start[0], -10.0, 10., "5.3g", 0.1, 0.025, "IV-Start");
        bp->grid_add_ec (NULL, Volt, &IV_end[0], -10.0, 10.0, "5.3g", 0.1, 0.025, "IV-End");
        bp->grid_add_ec (NULL, Unity, &IV_points[0], 1, 1000, "5g", "IV-Points");
	bp->grid_add_ec (NULL, Vslope, &IV_slope,0.1,1000.0, "5.3g", 1., 10., "IV-Slope");
	bp->grid_add_ec (NULL, Angstroem, &IV_dz, -1000.0, 1000.0, "5.4g", 1., 2., "IV-dz");
        
        bp->new_line ();
	bp->grid_add_ec ("Slope Ramp", Vslope, &IV_slope_ramp,0.1,1000.0, "5.3g", 1., 10., "IV-Slope-Ramp");
        bp->grid_add_check_button ("FB off", "disable Z Servo Feedback for going to IV Start Bias @ Slope Ramp rate", 1,
                                   G_CALLBACK(RPSPMC_Control::Probing_RampFBoff_callback), this, 0);
        bp->set_configure_list_mode_on ();
        bp->new_line ();
	bp->grid_add_ec ("#IV sets", Unity, &IV_repetitions, 1, 100, "3g", "IV-rep");
        bp->new_line ();
	bp->grid_add_ec ("IV-Recover-Delay", Time, &IV_recover_delay, 0., 10., "5.3g", 0.001, 0.01,  "IV-Recover-Delay");
        bp->set_configure_list_mode_off ();
#else
      
	bp->grid_add_ec ("Sections", Unity, &IV_sections, 1,6, "2g", "IV-Sections");
        multiIV_checkbutton = bp->grid_add_check_button ("Multi-IV mode", "enable muli section IV curve mode", 1,
                                                         G_CALLBACK(RPSPMC_Control::Probing_multiIV_callback), this, IV_sections-1);

        bp->new_line ();
	bp->grid_add_label ("IV Probe"); bp->grid_add_label ("Start"); bp->grid_add_label ("End");  bp->grid_add_label ("Points");
	for (int i=0; i<6; ++i) {
                bp->new_line ();

                gchar *tmp = g_strdup_printf("IV[%d]", i+1);
                bp->grid_add_label (tmp);
                if (i>0) multi_IVsec_list = g_slist_prepend (multi_IVsec_list, bp->label);
                g_free(tmp);

                if (i > 0){
                        tmp = g_strdup_printf("dsp-", i+1);
                        bp->set_pcs_remote_prefix (tmp);
                        g_free(tmp);
                }
                
                bp->grid_add_ec (NULL, Volt, &IV_start[i], -10.0, 10., "5.3g", 0.1, 0.025, "IV-Start", i);
                if (i == 5) bp->init_ec_array ();
                if (i>0) multi_IVsec_list = g_slist_prepend (multi_IVsec_list, bp->input);

                bp->grid_add_ec (NULL, Volt, &IV_end[i], -10.0, 10.0, "5.3g", 0.1, 0.025, "IV-End", i);
                if (i == 5) bp->init_ec_array ();
                if (i>0) multi_IVsec_list = g_slist_prepend (multi_IVsec_list, bp->input);

                bp->grid_add_ec (NULL, Unity, &IV_points[i], 1, 1000, "5g", "IV-Points", i);
                if (i == 5) bp->init_ec_array ();
                if (i>0) multi_IVsec_list = g_slist_prepend (multi_IVsec_list, bp->input);
        }
        bp->set_pcs_remote_prefix ("dsp-");
        
        bp->new_line ();

        bp->set_configure_list_mode_on ();
	bp->grid_add_ec ("dz", Angstroem, &IV_dz, -1000.0, 1000.0, "5.4g", 1., 2., "IV-dz");
	bp->grid_add_ec ("#dZ probes", Unity, &IVdz_repetitions, 0, 100, "3g", "IV-dz-rep");
        bp->set_configure_list_mode_off ();

        bp->new_line ();
	bp->grid_add_ec ("Slope", Vslope, &IV_slope,0.1,1000.0, "5.3g", 1., 10., "IV-Slope");
	bp->grid_add_ec ("Slope Ramp", Vslope, &IV_slope_ramp,0.1,1000.0, "5.3g", 1., 10., "IV-Slope-Ramp");
        bp->new_line ();
        bp->set_configure_list_mode_on ();
	bp->grid_add_ec ("#IV sets", Unity, &IV_repetitions, 1, 100, "3g", "IV-rep");

        bp->new_line ();
	bp->grid_add_ec ("Line-dX", Angstroem, &IV_dx, -1e6, 1e6, "5.3g", 1., 10., "IV-Line-dX");
        bp->grid_add_ec ("dY",  Angstroem, &IV_dy, -1e6, 1e6, "5.3g", 1., 10.,  "IV-Line-dY");
#ifdef MOTOR_CONTROL
	bp->grid_add_ec ("Line-dM",  Volt, &IV_dM, -1e6, 1e6, "5.3g", 1., 10.,  "IV-Line-dM");
#else
	IV_dM = 0.;
#endif
        bp->new_line ();
        bp->grid_add_ec ("Points", Unity, &IV_dxy_points, 1, 1000, "3g" "IV-Line-Pts");
	bp->grid_add_ec ("Slope", Speed, &IV_dxy_slope, -1e6, 1e6, "5.3g", 1., 10.,  "IV-Line-slope");
        bp->new_line ();
        bp->grid_add_ec ("Delay", Time, &IV_dxy_delay, 0., 10., "3g", 0.01, 0.1,  "IV-Line-Final-Delay");

        bp->new_line ();
	bp->grid_add_ec ("Final Delay", Time, &IV_final_delay, 0., 1., "5.3g", 0.001, 0.01,  "IV-Final-Delay");
	bp->grid_add_ec ("IV-Recover-Delay", Time, &IV_recover_delay, 0., 1., "5.3g", 0.001, 0.01,  "IV-Recover-Delay");
        bp->set_configure_list_mode_off ();
        
#endif

        // *************** IV controls
        bp->new_line ();

	IV_status = bp->grid_add_probe_status ("Status");
        bp->new_line ();

        bp->pop_grid ();
        bp->new_line ();
        bp->grid_add_probe_controls (TRUE,
                                         IV_option_flags, GCallback (callback_change_IV_option_flags),
                                         IV_auto_flags,  GCallback (callback_change_IV_auto_flags),
                                         GCallback (RPSPMC_Control::Probing_exec_IV_callback),
                                         GCallback (RPSPMC_Control::Probing_write_IV_callback),
                                         GCallback (RPSPMC_Control::Probing_graph_callback),
                                         GCallback (RPSPMC_Control::Probing_abort_callback),
                                         this,
                                         "IV");
        bp->notebook_tab_show_all ();
        bp->pop_grid ();

       
        // ==== Folder: GVP  ========================================
        bp->new_grid ();
        bp->start_notebook_tab (notebook, "GVP", "rpspmc-tab-gvp", hwi_settings);

        PI_DEBUG (DBG_L4, "SPMC----TAB-VP ------------------------------- ");

 	bp->new_grid_with_frame ("Generic Vector Program (VP) Probe and Manipulation");
 	// g_print ("================== TAB 'GVP' ============= Generic Vector Program (VP) Probe and Manipulation\n");
        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::ChangedNotifyVP, this);

        bp->set_input_width_chars (6);

	// ----- VP Program Vectors Headings
	// ------------------------------------- divided view

	// ------------------------------------- VP program table, scrolled
 	GtkWidget *vp = gtk_scrolled_window_new ();
        gtk_widget_set_vexpand (vp, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (vp), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_min_content_height  (GTK_SCROLLED_WINDOW (vp), 100);
        gtk_widget_set_vexpand (bp->grid, TRUE);
        bp->grid_add_widget (vp); // only widget in bp->grid

        // catch and disable scroll on wheel
        GtkEventController *gec;
        gec = gtk_event_controller_scroll_new( GTK_EVENT_CONTROLLER_SCROLL_VERTICAL );
        gtk_event_controller_set_propagation_phase( gec, GTK_PHASE_CAPTURE );
        g_signal_connect( gec, "scroll", G_CALLBACK( disable_scroll_cb ), vp );
        gtk_widget_add_controller( vp, gec );
        
        
        bp->new_grid ();
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (vp), bp->grid);
        
        // ----- VP Program Vectors Listing -- headers
	VPprogram[0] = bp->grid;

        bp->set_no_spin ();
        
        bp->set_input_width_chars (3);
        bp->set_label_width_chars (3);

        bp->grid_add_ec ("Mon", Volt, &spmc_parameters.bias_monitor, -10.0, 10.0, ".03g", 0.1, 1., "GVP-U-MONITOR");
        //bp->grid_add_ec ("Mon:", Volt, &spmc_parameters.gvpu_monitor, -10.0, 10.0, ".03g", 0.1, 1., "GVP-U-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->grid_add_ec (NULL, Angstroem, &GVP_XYZ_mon_AA[0], -1e10, 1e10, ".03g", 0.1, 1., "GVP-XS-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->grid_add_ec (NULL, Angstroem, &GVP_XYZ_mon_AA[1], -1e10, 1e10, ".03g", 0.1, 1., "GVP-YS-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->grid_add_ec (NULL, Angstroem, &GVP_XYZ_mon_AA[2], -1e10, 1e10, ".03g", 0.1, 1., "GVP-ZS-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->set_configure_list_mode_on ();
        bp->grid_add_ec (NULL, Volt, &spmc_parameters.gvpa_monitor, -10.0, 10.0, ".03g", 0.1, 1., "GVP-A-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->grid_add_ec (NULL, Volt, &spmc_parameters.gvpb_monitor, -10.0, 10.0, ".03g", 0.1, 1., "GVP-B-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->grid_add_ec (NULL, Volt, &spmc_parameters.gvpamc_monitor, -10.0, 10.0, ".03g", 0.1, 1., "GVP-AMC-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->grid_add_ec (NULL, Volt, &spmc_parameters.gvpfmc_monitor, -10.0, 10.0, ".03g", 0.1, 1., "GVP-FMC-MONITOR");
        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->set_configure_list_mode_off ();
        mon_FB = 0;
        bp->set_input_nx (5);
        bp->grid_add_ec (" FB: ", Unity, &mon_FB, -9999, 9999, ".0f", "GVP-ZSERVO-MONITOR");
        gvp_monitor_fb_label = bp->label;
        gvp_monitor_fb_info_ec = bp->ec;

        EC_MONITORS_list = g_slist_prepend( EC_MONITORS_list, bp->ec);
        bp->ec->Freeze ();
        bp->set_input_nx ();
        
	g_object_set_data (G_OBJECT (window), "SPMC_MONITORS_list", EC_MONITORS_list);

        bp->new_line ();
        bp->grid_add_label ("VPC", "Vector Program Counter");

        bp->set_input_width_chars (7);
        bp->set_label_width_chars (7);

        GVP_preview_on[0]=0; // not used
        
        //bp->grid_add_label ("VP-dU", "vec-du (Bias, DAC CH4)");
        bp->grid_add_button ("VP-dU", "vec-du (Bias, DAC CH4)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "gvpcolor4"); GVP_preview_on[4]=1;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (4));
        
        //bp->grid_add_label ("VP-dX", "vec-dx (X-Scan *Mrot + X0 => DAC CH1)");
        bp->grid_add_button ("VP-dX", "vec-dx (X-Scan *Mrot + X0 => DAC CH1)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "normal"); GVP_preview_on[1]=0;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (1));

        //bp->grid_add_label ("VP-dY", "vec-dy (Y-Scan, *Mrot + Y0, DAC CH2)");
        bp->grid_add_button ("VP-dY", "vec-dy (Y-Scan *Mrot + Y0 => DAC CH2)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "normal"); GVP_preview_on[2]=0;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (2));

        //bp->grid_add_label ("VP-dZ", "vec-dz (Z-Probe + Z-Servo + Z0, DAC CH3)");
        bp->grid_add_button ("VP-dZ", "vec-dz (Z-Probe + Z-Servo + Z0, DAC CH3)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "gvpcolor3"); GVP_preview_on[3]=1;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (3));

        bp->set_configure_list_mode_on ();
        //bp->grid_add_label ("VP-dA", "vec-dA (**DAC CH5)");
        bp->grid_add_button ("VP-dA", "vec-dA (**DAC CH5)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "normal"); GVP_preview_on[5]=0;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (5));

        //bp->grid_add_label ("VP-dB", "vec-dB (**DAC CH6)");
        bp->grid_add_button ("VP-dB", "vec-dB (**DAC CH6)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "normal"); GVP_preview_on[6]=0;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (6));

        //bp->grid_add_label ("VP-dAM", "vec-dAM (**RF-AM control)");
        bp->grid_add_button ("VP-dAM", "vec-dAM (**RF-AMP control)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "normal"); GVP_preview_on[7]=0;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (7));

        //bp->grid_add_label ("VP-dFM", "vec-dFM (**RF-FM control)");
        bp->grid_add_button ("VP-dFM", "vec-dFM (**RF-FM control)", 1, G_CALLBACK (callback_GVP_preview_me), this);
        gtk_widget_set_name (bp->button, "normal"); GVP_preview_on[8]=0;
        g_object_set_data (G_OBJECT(bp->button), "AXIS", GINT_TO_POINTER (8));

        bp->set_configure_list_mode_off ();
        bp->grid_add_label ("time", "total time for VP section");
        bp->grid_add_label ("points", "points (# vectors to add)");

        bp->set_input_width_chars (3);
        bp->set_label_width_chars (3);
        bp->grid_add_label ("FB", "Feedback (Option bit 0), CHECKED=FB-OFF (for now)");
        bp->grid_add_label ("VSet", "Treat this as a initial set position, vector differential from current position are computed!\nAttention: Does NOT apply to XYZ.");
        bp->set_configure_list_mode_on ();

        GSList *EC_vpc_exthdr_list=NULL;
        bp->grid_add_label ("OP", "X-OPCD"); EC_vpc_exthdr_list = g_slist_prepend( EC_vpc_exthdr_list, bp->label); gtk_widget_hide (bp->label);
        bp->grid_add_label ("CI", "X-RCHI"); EC_vpc_exthdr_list = g_slist_prepend( EC_vpc_exthdr_list, bp->label); gtk_widget_hide (bp->label);
        bp->grid_add_label ("VAL", "X-CMPV"); EC_vpc_exthdr_list = g_slist_prepend( EC_vpc_exthdr_list, bp->label); gtk_widget_hide (bp->label);
        bp->grid_add_label ("JR", "X-JMPR"); EC_vpc_exthdr_list = g_slist_prepend( EC_vpc_exthdr_list, bp->label); gtk_widget_hide (bp->label);

        bp->grid_add_label ("VX", "Enable VecEXtension OPCodes. Option bit 7");
        bp->grid_add_label ("HS", "Enable Max Sampling. Option bit 6");
        bp->grid_add_label ("5", "Option bit 5");
        bp->grid_add_label ("4", "Option bit 4");
        bp->grid_add_label ("3", "Option bit 3");
        bp->grid_add_label ("2", "Option bit 2");
        bp->grid_add_label ("1", "Option bit 1");

        bp->set_input_width_chars (4);
        bp->set_label_width_chars (4);
        bp->grid_add_label ("Nrep", "VP # repetition");
        bp->grid_add_label ("PCJR", "Vector-PC jump relative\n (example: set to -2 to repeat previous two vectors.)");
        bp->set_configure_list_mode_off ();
        bp->grid_add_label("Shift", "Edit: Shift VP Block up/down");

        bp->new_line ();
        
	GSList *EC_vpc_opt_list=NULL;
        Gtk_EntryControl *ec_iter[12];
	for (int k=0; k < N_GVP_VECTORS; ++k) {
		gchar *tmpl = g_strdup_printf ("%02d", k); 

                // g_print ("GVP-DEBUG:: %s -[%d]------> GVPdu=%g, ... points=%d,.. opt=%8x\n", tmpl, k, GVP_du[k], GVP_points[k],  GVP_opt[k]);

                bp->set_input_width_chars (3);
                bp->set_label_width_chars (3);
		bp->grid_add_label (tmpl, "Vector PC");
                
                bp->set_input_width_chars (7);
                bp->set_label_width_chars (7);
		bp->grid_add_ec (NULL,      Volt, &GVP_du[k], -10.0,   10.0,   "6.4g", 1., 10., "gvp-du", k);
                if (k == 0) GVP_V0Set_ec[3] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

		bp->grid_add_ec (NULL, Angstroem, &GVP_dx[k], -1000000.0, 1000000.0, "6.4g", 1., 10., "gvp-dx", k); 
                if (k == 0) GVP_V0Set_ec[0] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

		bp->grid_add_ec (NULL, Angstroem, &GVP_dy[k], -1000000.0, 1000000.0, "6.4g", 1., 10., "gvp-dy", k); 
                if (k == 0) GVP_V0Set_ec[1] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

		bp->grid_add_ec (NULL, Angstroem, &GVP_dz[k], -1000000.0, 1000000.0, "6.4g", 1., 10., "gvp-dz", k); 
                if (k == 0) GVP_V0Set_ec[2] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

                bp->set_configure_list_mode_on (); // === advanced ===========================================
		bp->grid_add_ec (NULL,    Volt, &GVP_da[k], -10.0, 10.0, "6.4g",1., 10., "gvp-da", k); 
                if (k == 0) GVP_V0Set_ec[4] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
		bp->grid_add_ec (NULL,    Volt, &GVP_db[k], -10.0, 10.0, "6.4g",1., 10., "gvp-db", k); 
                if (k == 0) GVP_V0Set_ec[5] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
		bp->grid_add_ec (NULL,    Volt, &GVP_dam[k], -10.0, 10.0, "6.4g",1., 10., "gvp-dam", k); 
                if (k == 0) GVP_V0Set_ec[6] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
		bp->grid_add_ec (NULL,    Volt, &GVP_dfm[k], -10.0, 10.0, "6.4g",1., 10., "gvp-dfm", k); 
                if (k ==0 ) GVP_V0Set_ec[7] = bp->ec;
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
                bp->set_configure_list_mode_off (); // ========================================================

		bp->grid_add_ec (NULL,      Time, &GVP_ts[k], 0., 147573952580.0,     "5.4g", 1., 10., "gvp-dt", k); // 1<<64 / 125e6 s max
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

		bp->grid_add_ec (NULL,      Unity, &GVP_points[k], 0, (1<<16)-1,  "5g", "gvp-n", k); // FPGA GVP can do 1<<32-1, currently limited by 16bit GVP stream index.
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();


                bp->set_input_width_chars (2);
                bp->set_label_width_chars (2);
		//note:  VP_FEEDBACK_HOLD is only the mask, this bit in GVP_opt is set to ONE for FEEBACK=ON !! Ot is inveretd while vector generation ONLY.
		bp->grid_add_check_button ("", bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Enable Zservo","dsp-gvp-FB",k), 1,
                                               GCallback (callback_change_GVP_vpc_option_flags), this,
                                               GVP_opt[k], VP_FEEDBACK_HOLD);
                EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, bp->button);
                g_object_set_data (G_OBJECT (bp->button), "VPC", GINT_TO_POINTER (k));

                bp->set_configure_list_mode_on (); // ================ advanced section

                bp->grid_add_check_button ("", bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Vector is a absolute reference (goto)","dsp-gvp-VS",k), 1,
                                           GCallback (callback_change_GVP_vpc_option_flags), this,
                                           GVP_opt[k], VP_INITIAL_SET_VEC);
                EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, bp->button);
                g_object_set_data (G_OBJECT (bp->button), "VPC", GINT_TO_POINTER (k));


                bp->set_input_width_chars (4);
                bp->set_label_width_chars (4);
                GSList *EC_vpc_ext_list=NULL;
		bp->grid_add_ec (NULL, Unity, &GVPX_opcd[k],   0.,  5.,  ".0f", "gvp-xopcd", k); //
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
                EC_vpc_ext_list = g_slist_prepend( EC_vpc_ext_list, bp->input);
		bp->grid_add_ec (NULL, Unity, &GVPX_rchi[k],   0., 14.,  ".0f", "gvp-xrchi", k); //
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
                EC_vpc_ext_list = g_slist_prepend( EC_vpc_ext_list, bp->input);
                bp->set_input_width_chars (7);
                bp->set_label_width_chars (7);
                bp->grid_add_ec (NULL, Unity, &GVPX_cmpv[k], -1e6, 1e6,  "6.4g", 1., 10., "gvp-xcmpv", k); //
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
                EC_vpc_ext_list = g_slist_prepend( EC_vpc_ext_list, bp->input);
                bp->set_input_width_chars (4);
                bp->set_label_width_chars (4);
		bp->grid_add_ec (NULL, Unity, &GVPX_jmpr[k], -32., 32.,  ".0f", "gvp-xjmpr", k); //
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();
                EC_vpc_ext_list = g_slist_prepend( EC_vpc_ext_list, bp->input);

                
                bp->set_input_width_chars (2);
                bp->set_label_width_chars (2);
                GtkWidget *VX_b7=NULL;
                for (int bit=7; bit >= 1; --bit){
                        bp->set_label_width_chars (1);
                        bp->grid_add_check_button ("", NULL, 1,
                                                   GCallback (callback_change_GVP_vpc_option_flags), this,
                                                   GVP_opt[k], (1<<bit));
                        if (bit == 7) VX_b7 = bp->button;
                        EC_vpc_opt_list = g_slist_prepend( EC_vpc_opt_list, bp->button);
                        g_object_set_data (G_OBJECT (bp->button), "VPC", GINT_TO_POINTER (k));
                }
                g_object_set_data( G_OBJECT (VX_b7), "DSP_VPC_EXTOPC_list", EC_vpc_ext_list);
                g_object_set_data( G_OBJECT (VX_b7), "DSP_VPC_EXTHDR_list", EC_vpc_exthdr_list);

                bp->set_input_width_chars (4);
                bp->set_label_width_chars (4);
		bp->grid_add_ec (NULL, Unity, &GVP_vnrep[k], 0., 65536.,  ".0f", "gvp-nrep", k); // limit to 1<<16
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

		bp->grid_add_ec (NULL, Unity, &GVP_vpcjr[k], -16.,   0.,  ".0f", "gvp-pcjr", k); // -MAX NUM VECTORS at longest
                if (k == (N_GVP_VECTORS-1)) bp->init_ec_array ();

                bp->set_configure_list_mode_off (); // ==================================

                bp->set_input_width_chars (4);
                GtkWidget *button;
		if (k > 0){
                        bp->grid_add_widget (button=gtk_button_new_from_icon_name ("arrow-up-symbolic"));
                        gtk_widget_set_tooltip_text (button, "Shift VP up here");
                        g_object_set_data (G_OBJECT(button), "VPC", GINT_TO_POINTER (k));
                        g_object_set_data (G_OBJECT(button), "ARROW", GINT_TO_POINTER (GVP_SHIFT_UP));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (callback_edit_GVP),
                                           this);
		} else {
                        bp->grid_add_widget (button=gtk_button_new_from_icon_name ("view-more-symbolic"));
                        gtk_widget_set_tooltip_text (button, "Toggle VP Flow Chart");
                        g_object_set_data (G_OBJECT(button), "VPC", GINT_TO_POINTER (k));
                        g_object_set_data (G_OBJECT(button), "ARROW", GINT_TO_POINTER (GVP_SHIFT_UP));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (callback_edit_GVP),
                                           this);
                }
		if (k < N_GVP_VECTORS-1){
                        bp->grid_add_widget (button=gtk_button_new_from_icon_name ("arrow-down-symbolic"));
                        gtk_widget_set_tooltip_text (button, "Shift VP down here");
                        g_object_set_data (G_OBJECT(button), "VPC", GINT_TO_POINTER (k));
                        g_object_set_data (G_OBJECT(button), "ARROW", GINT_TO_POINTER (GVP_SHIFT_DN));
			g_signal_connect ( G_OBJECT (button), "clicked",
                                           G_CALLBACK (callback_edit_GVP),
                                           this);
		}
		g_free (tmpl);
		if (k==0)
			g_object_set_data (G_OBJECT (bp->grid), "CF", GINT_TO_POINTER (bp->x));

                bp->new_line ();
                
	}
	g_object_set_data( G_OBJECT (window), "DSP_VPC_OPTIONS_list", EC_vpc_opt_list);

        bp->set_no_spin (false);

        bp->set_input_width_chars (6);

        // lower part: controls
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("VP Memos");

	// MEMO BUTTONs
        gtk_widget_set_hexpand (bp->grid, TRUE);
	const gchar *keys[] = { "VPA", "VPB", "VPC", "VPD", "VPE", "VPF", "VPG", "VPH", "VPI", "VPJ", "V0", NULL };

	for (int i=0; keys[i]; ++i) {
                GdkRGBA rgba;
		gchar *gckey  = g_strdup_printf ("GVP_set_%s", keys[i]);
		gchar *stolab = g_strdup_printf ("STO %s", keys[i]);
		gchar *rcllab = g_strdup_printf ("RCL %s", keys[i]);
		gchar *memolab = g_strdup_printf ("M %s", keys[i]);             
		gchar *memoid  = g_strdup_printf ("memo-vp%c", 'a'+i);             
                remote_action_cb *ra = NULL;
                gchar *help = NULL;

                bp->set_xy (i+1, 10);
                // add button with remote support for program recall
                ra = g_new( remote_action_cb, 1);
                ra -> cmd = g_strdup_printf("DSP_VP_STO_%s", keys[i]);
                help = g_strconcat ("Remote example: action (", ra->cmd, ")", NULL); 
                bp->grid_add_button (N_(stolab), help, 1,
                                         G_CALLBACK (callback_GVP_store_vp), this,
                                         "key", gckey);
                g_free (help);
                ra -> RemoteCb = (void (*)(GtkWidget*, void*))callback_GVP_store_vp;
                ra -> widget = bp->button;
                ra -> data = this;
                main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
                PI_DEBUG (DBG_L2, "Adding new Remote Cmd: " << ra->cmd ); 

                // CSS
                //                if (gdk_rgba_parse (&rgba, "tomato"))
                //                        gtk_widget_override_background_color ( GTK_WIDGET (bp->button), GTK_STATE_FLAG_PRELIGHT, &rgba);

                bp->set_xy (i+1, 11);

                // add button with remote support for program recall
                ra = g_new( remote_action_cb, 1);
                ra -> cmd = g_strdup_printf("DSP_VP_RCL_%s", keys[i]);
                help = g_strconcat ("Remote example: action (", ra->cmd, ")", NULL); 
                bp->grid_add_button (N_(rcllab), help, 1,
                                         G_CALLBACK (callback_GVP_restore_vp), this,
                                         "key", gckey);
                g_free (help);
                ra -> RemoteCb = (void (*)(GtkWidget*, void*))callback_GVP_restore_vp;
                ra -> widget = bp->button;
                ra -> data = this;
                main_get_gapp()->RemoteActionList = g_slist_prepend ( main_get_gapp()->RemoteActionList, ra );
                PI_DEBUG (DBG_L2, "Adding new Remote Cmd: " << ra->cmd ); 
                
                // CSS
                //                if (gdk_rgba_parse (&rgba, "SeaGreen3"))
                //                        gtk_widget_override_background_color ( GTK_WIDGET (bp->button), GTK_STATE_FLAG_PRELIGHT, &rgba);
#if 0 // may adda memo/info button
                bp->set_xy (i+1, 12);
                bp->grid_add_button (N_(memolab), memolab, 1,
                                         G_CALLBACK (callback_GVP_memo_vp), this,
                                         "key", gckey);
#endif
                bp->set_xy (i+1, 12);
                bp->grid_add_input (NULL);
                bp->set_input_width_chars (10);
                gtk_widget_set_hexpand (bp->input, TRUE);

                g_settings_bind (hwi_settings, memoid,
                                 G_OBJECT (bp->input), "text",
                                 G_SETTINGS_BIND_DEFAULT);

                //g_free (gckey);
                //g_free (stolab);
                //g_free (rcllab);
                g_free (memolab);
                g_free (memoid);
	}

        // ===================== done with panned
        
        bp->pop_grid ();
        bp->new_line ();

	bp->new_grid_with_frame ("GVP Wave Preview");
        WavePreview=bp->frame;
	//bp->new_grid_with_frame ("VP Status");
	//GVP_status = bp->grid_add_probe_status ("Status");

        gvp_preview_area = gtk_drawing_area_new ();
        //gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (gvp_preview_area), 512);
        gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (gvp_preview_area), 128);
        //gtk_widget_set_size_request (gvp_preview_area, -1, 128);
        //gtk_widget_set_hexpand (gvp_preview_area, true);
        //gtk_widget_compute_expand (gvp_preview_area, GTK_ORIENTATION_HORIZONTAL);

	GtkWidget *scrollarea = gtk_scrolled_window_new ();

        gtk_widget_set_hexpand (scrollarea, TRUE);
        gtk_widget_set_vexpand (scrollarea, TRUE);
        
	/* the policy is one of GTK_POLICY AUTOMATIC, or GTK_POLICY_ALWAYS.
	 * GTK_POLICY_AUTOMATIC will automatically decide whether you need
	 * scrollbars, whereas GTK_POLICY_ALWAYS will always leave the scrollbars
	 * there.  The first one is the horizontal scrollbar, the second, 
	 * the vertical. */
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollarea),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrollarea), gvp_preview_area);
        
        bp->grid_add_widget (scrollarea);
        //bp->grid_add_widget (gvp_preview_area);

        gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (gvp_preview_area),
                                        GtkDrawingAreaDrawFunc (RPSPMC_Control::gvp_preview_draw_function),
                                        this, NULL);
        
        bp->pop_grid ();
        bp->new_line ();

        bp->grid_add_probe_controls (FALSE,
                                         GVP_option_flags, GCallback (callback_change_GVP_option_flags),
                                         GVP_auto_flags,  GCallback (callback_change_GVP_auto_flags),
                                         GCallback (RPSPMC_Control::Probing_exec_GVP_callback),
                                         GCallback (RPSPMC_Control::Probing_write_GVP_callback),
                                         GCallback (RPSPMC_Control::Probing_graph_callback),
                                         GCallback (RPSPMC_Control::Probing_abort_callback),
                                         this,
                                         "VP");
        bp->notebook_tab_show_all ();
        bp->pop_grid ();



// ==== Folder: Graphs -- Vector Probe Data Visualisation setup ========================================
        bp->new_grid ();
        bp->start_notebook_tab (notebook, "Graphs", "rpspmc-tab-graphs", hwi_settings);

        PI_DEBUG (DBG_L4, "SPMC----TAB-GRAPHS ------------------------------- ");

 	bp->new_grid_with_frame ("Probe Sources & Graph Setup");

        bp->grid_add_label ("Ref-Source", "Check column to activate channel", 2, 0.);
        bp->set_input_width_chars (1);
        bp->grid_add_label ("X", "Check column to plot channel on X axis.", 1);
        bp->grid_add_label ("Y", "Check column to plot channel on Y axis.", 1);
        bp->grid_add_label ("Avg", "Check column to plot average of all spectra", 1);
        bp->grid_add_label ("Sec", "Check column to show all sections.", 1);
        GtkWidget *sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
        gtk_widget_set_size_request (sep, 5, -1);
        bp->grid_add_widget (sep, 5);
        //bp->grid_add_label (" --- ", NULL, 5);

        bp->grid_add_label ("Source", "Check column to activate channel", 2, 0.);
        bp->set_input_width_chars (1);
        bp->grid_add_label ("X", "Check column to plot channel on X axis.", 1);
        bp->grid_add_label ("Y", "Check column to plot channel on Y axis.", 1);
        bp->grid_add_label ("Avg", "Check column to plot average of all spectra", 1);
        bp->grid_add_label ("Sec", "Check column to show all sections.", 1);
        sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
        gtk_widget_set_size_request (sep, 5, -1);
        bp->grid_add_widget (sep, 5);
        //bp->grid_add_label (" --- ", NULL, 5);

        bp->grid_add_label ("Source", "Check column to activate channel", 2, 0.);
        bp->set_input_width_chars (1);
        bp->grid_add_label ("X", "Check column to plot channel on X axis.", 1);
        bp->grid_add_label ("Y", "Check column to plot channel on Y axis.", 1);
        bp->grid_add_label ("Avg", "Check column to plot average of all spectra", 1);
        bp->grid_add_label ("Sec", "Check column to show all sections.", 1);
        sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
        gtk_widget_set_size_request (sep, 5, -1);
        bp->grid_add_widget (sep, 5);
        //bp->grid_add_label (" --- ", NULL, 5);

#if 0 // if need more
        bp->grid_add_label ("Source", "Check column to activate channel", 2, 0.);
        bp->set_input_width_chars (1);
        bp->grid_add_label ("X", "Check column to plot channel on X axis.", 1);
        bp->grid_add_label ("Y", "Check column to plot channel on Y axis.", 1);
        bp->grid_add_label ("Avg", "Check column to plot average of all spectra", 1);
        bp->grid_add_label ("Sec", "Check column to show all sections.", 1);
        sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
        gtk_widget_set_size_request (sep, 5, -1);
        bp->grid_add_widget (sep, 5);
#endif
        bp->new_line ();

        PI_DEBUG (DBG_L4, "SPMC----TAB-GRAPHS TOGGELS  ------------------------------- ");

        gint y = bp->y;
        gint mm=0;

        for (int i=0; i<32; ++i) for (int j=0; j<5; ++j) graphs_matrix[j][i]=NULL;
        restore_graphs_values ();

        int ii=0;
        for (int i=0; rpspmc_source_signals[i].mask; ++i) {
                const int rows=11;
                int k=-1;
		int c=ii/rows; 
                int r = y+ii%rows+1; // row
                ii++;
		c*=11; // col
                c++;

                if (rpspmc_source_signals[i].scan_source_pos < 0) continue; // skip
                PI_DEBUG (DBG_L4, "GRAPHS*** i=" << i << " " << rpspmc_source_signals[i].label);

                switch (rpspmc_source_signals[i].mask){
                case 0x0100: k=0; break;
                case 0x0200: k=1; break;
                case 0x0400: k=2; break;
                case 0x0800: k=3; break;
                case 0x1000: k=4; break;
                case 0x2000: k=5; break;
                }

                if (k >= 0 && k < 6){
                        c=23; r=y+k+1;
                        if (!rpspmc_swappable_signals[k].label) { g_warning ("GVP SOURCE MUX/SWPS INIT ** i=%d k=%d SWPS invalid/NULL", i,k); break; }
                      //rpspmc_source_signals[i].name         = rpspmc_swappable_signals[k].name;
                        rpspmc_source_signals[i].label        = rpspmc_swappable_signals[k].label;
                        rpspmc_source_signals[i].unit         = rpspmc_swappable_signals[k].unit;
                        rpspmc_source_signals[i].unit_sym     = rpspmc_swappable_signals[k].unit_sym;
                        rpspmc_source_signals[i].scale_factor = rpspmc_swappable_signals[k].scale_factor;
                        PI_DEBUG (DBG_L4, "GRAPHS*** SWPS init i=" << i << " k=" << k << " " << rpspmc_source_signals[i].label << " sfac=" << rpspmc_source_signals[i].scale_factor);
                        g_message ("GRAPHS*** SWPS init i=%d k=%d {%s} sfac=%g", i, k, rpspmc_source_signals[i].label,rpspmc_source_signals[i].scale_factor);
                }
                if (rpspmc_source_signals[i].mask == 0xc000){ // Time-Mon
                        c=23; r=y+7+1;
                }

                bp->set_xy (c, r);

                g_message ("GRAPHS MATRIX SELECTOR BUILD: %d => 0x%08x",i,rpspmc_source_signals[i].mask);
                
                // Source
                bp->set_input_width_chars (1);
                graphs_matrix[0][i] = bp->grid_add_check_button ("", NULL, 1,
                                                                 GCallback (change_source_callback), this,
                                                                 Source, rpspmc_source_signals[i].mask
                                                                 );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER (rpspmc_source_signals[i].mask)); 
                g_object_set_data (G_OBJECT(bp->button), "SourceMapping",  GINT_TO_POINTER (MAP_SOURCE_REC)); 
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 

                // source selection for SWPS?:
                if (k >= 0 && k < 6){ // swappable flex source
                        //g_message("bp->grid_add_probe_source_signal_options m=%d  %s => %d %s", k, rpspmc_source_signals[i].label,  probe_source[k], rpspmc_swappable_signals[k].label);
                        probe_source_signal_selector[k] = bp->grid_add_probe_source_signal_options (k, probe_source[k], this);
                }else { // or fixed assignment
                        bp->grid_add_label (rpspmc_source_signals[i].label, NULL, 1, 0.);
                }
                //fixed assignment:
                // bp->grid_add_label (lablookup[i], NULL, 1, 0.);
                
                // use as X-Source
                graphs_matrix[1][i] = bp->grid_add_check_button ("", NULL, 1,
                                                                 GCallback (change_source_callback), this,
                                                                 XSource, X_SOURCE_MSK | rpspmc_source_signals[i].mask
                                                                 );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER (rpspmc_source_signals[i].mask)); 
                g_object_set_data (G_OBJECT(bp->button), "Source_Mapping", GINT_TO_POINTER (MAP_SOURCE_X)); 
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 

                // use as Plot (Y)-Source
                graphs_matrix[2][i] = bp->grid_add_check_button ("", NULL, 1,
                                                                 G_CALLBACK (change_source_callback), this,
                                                                 PSource, P_SOURCE_MSK | rpspmc_source_signals[i].mask
                                                                 );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER (rpspmc_source_signals[i].mask)); 
                g_object_set_data (G_OBJECT(bp->button), "Source_Mapping", GINT_TO_POINTER (MAP_SOURCE_PLOTY));
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // use as A-Source (Average)
                graphs_matrix[3][i] = bp->grid_add_check_button ("", NULL, 1,
                                                                 G_CALLBACK (change_source_callback), this,
                                                                 PlotAvg, A_SOURCE_MSK | rpspmc_source_signals[i].mask
                                                                 );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER (rpspmc_source_signals[i].mask)); 
                g_object_set_data (G_OBJECT(bp->button), "Source_Mapping", GINT_TO_POINTER (MAP_SOURCE_AVG));
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // use as S-Source (Section)
                graphs_matrix[4][i] = bp->grid_add_check_button ("", NULL, 1,
                                                                 G_CALLBACK (change_source_callback), this,
                                                                 PlotSec, S_SOURCE_MSK | rpspmc_source_signals[i].mask
                                                                 );
                g_object_set_data (G_OBJECT(bp->button), "Source_Channel", GINT_TO_POINTER (rpspmc_source_signals[i].mask)); 
                g_object_set_data (G_OBJECT(bp->button), "Source_Mapping", GINT_TO_POINTER (MAP_SOURCE_SEC));
                g_object_set_data (G_OBJECT(bp->button), "VPC", GINT_TO_POINTER (i)); 
                
                // bp->grid_add_check_button_graph_matrix(lablookup[i], (int) rpspmc_source_signals[i].mask, m, probe_source[m], i, this);
                // bp->grid_add_check_button_graph_matrix(" ", (int) (X_SOURCE_MSK | rpspmc_source_signals[i].mask), -1, i, this);
                // bp->grid_add_check_button_graph_matrix(" ", (int) (P_SOURCE_MSK | rpspmc_source_signals[i].mask), -1, i, this);
                // bp->grid_add_check_button_graph_matrix(" ", (int) (A_SOURCE_MSK | rpspmc_source_signals[i].mask), -1, i, this);
                // bp->grid_add_check_button_graph_matrix(" ", (int) (S_SOURCE_MSK | rpspmc_source_signals[i].mask), -1, i, this);

                sep = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
                gtk_widget_set_size_request (sep, 5, -1);
                bp->grid_add_widget (sep);
	}
        g_message ("GRAPHS MATRIX SELECTOR BUILD complete.");

        
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Plot Mode Configuration");

        g_message ("GRAPHS PLOT CFG");
	graphs_matrix[0][31] = bp->grid_add_check_button ("Join all graphs for same X", "Join all plots with same X.\n"
                                                          "Note: if auto scale (default) Y-scale\n"
                                                          "will only apply to 1st graph - use hold/exp. only for asolute scale.)", 1,
                                                          GCallback (callback_XJoin), this,
                                                          XJoin, 1
                                                          );
	graphs_matrix[1][31] = bp->grid_add_check_button ("Use single window", "Place all probe graphs in single window.",
                                                          1,
                                                          GCallback (callback_GrMatWindow), this,
                                                          GrMatWin, 1
                                                          );
        bp->pop_grid ();
        bp->new_line ();
        bp->new_grid_with_frame ("Plot / Save current data in buffer -- for convenience: extra Execute GVP button");

        g_message ("GRAPHS PLOT ACTIONS");
	bp->grid_add_button ("Plot");
	g_signal_connect (G_OBJECT (bp->button), "clicked",
                          G_CALLBACK (RPSPMC_Control::Probing_graph_callback), this);
	
        save_button = bp->grid_add_button ("Save");
	g_signal_connect (G_OBJECT (bp->button), "clicked",
                          G_CALLBACK (RPSPMC_Control::Probing_save_callback), this);


	bp->grid_add_button ("Execute GVP");
	g_signal_connect (G_OBJECT (bp->button), "clicked",
                          GCallback (RPSPMC_Control::Probing_exec_GVP_callback), this);
        
        bp->notebook_tab_show_all ();
        bp->pop_grid ();



// ==== Folder: RPSPMC System Connection ========================================
        g_message ("Folder: RPSPMC System Connection");
        bp->set_pcs_remote_prefix ("");
        bp->new_grid ();
        bp->start_notebook_tab (notebook, "RedPitaya Web Socket", "rpspmc-tab-system", hwi_settings);

        PI_DEBUG (DBG_L4, "SPMC----TAB-SYSTEM ------------------------------- ");

        bp->new_grid_with_frame ("RedPitaya Web Socket Address for JSON talk", 10);

        bp->set_input_width_chars (25);
        rpspmc_pacpll->input_rpaddress = bp->grid_add_input ("RedPitaya Address");

        g_settings_bind (rpspmc_pacpll->inet_json_settings, "redpitay-address",
                         G_OBJECT (bp->input), "text",
                         G_SETTINGS_BIND_DEFAULT);
        gtk_widget_set_tooltip_text (rpspmc_pacpll->input_rpaddress, "RedPitaya IP Address like rp-f05603.local or 130.199.123.123");
        //  "ws://rp-f05603.local:9002/"
        //gtk_entry_set_text (GTK_ENTRY (input_rpaddress), "http://rp-f05603.local/pacpll/?type=run");
        //gtk_entry_set_text (GTK_ENTRY (input_rpaddress), "130.199.243.200");
        //gtk_entry_set_text (GTK_ENTRY (input_rpaddress), "192.168.1.10");

        g_message ("Folder: RPSPMC CHECK CONNECT");

        bp->grid_add_check_button ( N_("Restart"), "Check to reload FPGA and initiate connection, uncheck to close connection.", 1,
                                    G_CALLBACK (RPspmc_pacpll::connect_cb), rpspmc_pacpll);
	g_object_set_data( G_OBJECT (bp->button), "RESTART", GINT_TO_POINTER (1));

        bp->grid_add_check_button ( N_("Re-Connect"), "Check to re-initiate connection, uncheck to close connection.", 1,
                                    G_CALLBACK (RPspmc_pacpll::connect_cb), rpspmc_pacpll);
	g_object_set_data( G_OBJECT (bp->button), "RESTART", GINT_TO_POINTER (0));

        bp->grid_add_check_button ( N_("Connect Stream"), "Check to initiate stream connection, uncheck to close connection.", 1,
                                    G_CALLBACK (rpspmc_hwi_dev::spmc_stream_connect_cb), rpspmc_hwi);
        stream_connect_button=bp->button;

        spmc_parameters.rpspmc_dma_pull_interval = 10.0;
        bp->set_input_width_chars (2);
        bp->set_default_ec_change_notice_fkt (RPSPMC_Control::spmc_server_control_callback, this);
  	bp->grid_add_ec ("DMA rate limit", msTime, &spmc_parameters.rpspmc_dma_pull_interval, 10., 10., ".0f", 5., 250., "RP-DMA-PULL-INTERVAL");        
        EC_FPGA_SPMC_server_settings_list = g_slist_prepend(EC_FPGA_SPMC_server_settings_list, bp->ec);
        
        bp->grid_add_widget (GVP_stop_all_zero_button=gtk_button_new_from_icon_name ("process-stopall-symbolic"));
        g_signal_connect (G_OBJECT (GVP_stop_all_zero_button), "clicked", G_CALLBACK (RPSPMC_Control::GVP_AllZero), this);
        gtk_widget_set_tooltip_text (GVP_stop_all_zero_button, "Click to force reset GVP (WARNING: XYZ JUMP possible)!");

        g_message ("Folder: RPSPMC DBG");

        bp->new_line ();
        bp->grid_add_check_button ( N_("Debug"), "Enable debugging LV1.", 1,
                                    G_CALLBACK (RPspmc_pacpll::dbg_l1), rpspmc_pacpll);
        bp->grid_add_check_button ( N_("+"), "Debug LV2", 1,
                                    G_CALLBACK (RPspmc_pacpll::dbg_l2), rpspmc_pacpll);
        bp->grid_add_check_button ( N_("++"), "Debug LV4", 1,
                                    G_CALLBACK (RPspmc_pacpll::dbg_l4), rpspmc_pacpll);
        bp->grid_add_check_button ( N_("GVP6"), "Set GVP Option Bit 6 for scan", 1,
                                    G_CALLBACK (RPspmc_pacpll::scan_gvp_opt6), rpspmc_pacpll);
        bp->grid_add_check_button ( N_("GVP7"), "Set GVP Option Bit 7 for scan", 1,
                                    G_CALLBACK (RPspmc_pacpll::scan_gvp_opt7), rpspmc_pacpll);
        rp_verbose_level = 0.0;
        bp->set_input_width_chars (2);
  	bp->grid_add_ec ("Verbosity", Unity, &rp_verbose_level, 0., 10., ".0f", 1., 1., "RP-VERBOSE-LEVEL");        
        EC_FPGA_SPMC_server_settings_list = g_slist_prepend(EC_FPGA_SPMC_server_settings_list, bp->ec);
        //bp->new_line ();
        //tmp=bp->grid_add_button ( N_("Read"), "TEST READ", 1,
        //                          G_CALLBACK (RPspmc_pacpll::read_cb), this);
        //tmp=bp->grid_add_button ( N_("Write"), "TEST WRITE", 1,
        //                          G_CALLBACK (RPspmc_pacpll::write_cb), this);

        bp->new_line ();
        bp->set_input_width_chars (80);
        rpspmc_pacpll->red_pitaya_health = bp->grid_add_input ("RedPitaya Health",10);
        gtk_widget_set_name (bp->input, "entry-mono-text-start");
        
        PangoFontDescription *fontDesc = pango_font_description_from_string ("monospace 10");
        //gtk_widget_modify_font (red_pitaya_health, fontDesc);
        // ### GTK4 ??? CSS ??? ###  gtk_widget_override_font (red_pitaya_health, fontDesc); // use CSS, thx, annyoing garbage... ??!?!?!?
        pango_font_description_free (fontDesc);

        gtk_widget_set_sensitive (bp->input, FALSE);
        gtk_editable_set_editable (GTK_EDITABLE (bp->input), FALSE); 
        rpspmc_pacpll->update_health ("Not connected.");
        bp->new_line ();

        g_message ("Folder: RPSPMC LOG");
        
        rpspmc_pacpll->text_status = gtk_text_view_new ();
 	gtk_text_view_set_editable (GTK_TEXT_VIEW (rpspmc_pacpll->text_status), FALSE);
        //gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_status), GTK_WRAP_WORD_CHAR);
        GtkWidget *scrolled_window = gtk_scrolled_window_new ();
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        //gtk_widget_set_size_request (scrolled_window, 200, 60);
        gtk_widget_set_hexpand (scrolled_window, TRUE);
        gtk_widget_set_vexpand (scrolled_window, TRUE);
        gtk_scrolled_window_set_child ( GTK_SCROLLED_WINDOW (scrolled_window), rpspmc_pacpll->text_status);
        bp->grid_add_widget (scrolled_window, 10);
        gtk_widget_show (scrolled_window);
        
        // ========================================
        bp->pop_grid ();
        bp->show_all ();

        g_message ("*** STORE LISTS ***");

        
	// ============================================================
	// save List away...
        rpspmc_pacpll_hwi_pi.app->RemoteEntryList = g_slist_concat (rpspmc_pacpll_hwi_pi.app->RemoteEntryList, bp->get_remote_list_head ());
        configure_callback (NULL, NULL, this); // configure "false"

	g_object_set_data( G_OBJECT (window), "DSP_EC_list", bp->get_ec_list_head ());
        if (multiIV_checkbutton)
                g_object_set_data( G_OBJECT (multiIV_checkbutton), "DSP_multiIV_list", multi_IVsec_list);
	//g_object_set_data( G_OBJECT (zposmon_checkbutton), "DSP_zpos_control_list", zpos_control_list);


	g_object_set_data( G_OBJECT (window), "FPGA_readback_update_list", FPGA_readback_update_list);
        g_object_set_data( G_OBJECT (window), "EC_FPGA_SPMC_server_settings_list", EC_FPGA_SPMC_server_settings_list);
        
	GUI_ready = TRUE;
        
        AppWindowInit (NULL); // stage two
        set_window_geometry ("rpspmc-main-control"); // must add key to xml file: core-sources/org.gnome.gxsm4.window-geometry.gschema.xml

        g_message ("RPSPMC CONTROL READY -- updating");
       
#ifdef ENABLE_SHM_ACTIONS
        g_message ("RPSPMC CONTROL STARTING SHM ACTION IDLE-CALLBACK");
        check_shm_action_idle_id = 0;
        //check_shm_action_idle_id = g_idle_add (check_shm_action_idle_callback, this);
        check_shm_action_idle_id = g_timeout_add (50, check_shm_action_idle_callback, this);
#else
        check_shm_action_idle_id = 0;
#endif
        //Probing_RampFBoff_callback (multiIV_checkbutton, this); // update
        //Probing_multiIV_callback (multiIV_checkbutton, this); // update

        g_message ("RPSPMC BUILD TABS DONE.");
}

void RPSPMC_Control::Init_SPMC_on_connect (){
        // fix-me -- need life readback once life re-connect works!
        // update: mission critical readbacks done
        if (rpspmc_pacpll && rpspmc_hwi){
                rpspmc_pacpll->write_parameter ("SPMC_GVP_RESET_OPTIONS", 0x0000); // default/reset GVP OPTIONS: Feedback ON! *** WATCH THIS ***
        
                BiasChanged(NULL, this);
                Slope_dZX_Changed(NULL, this);
                Slope_dZY_Changed(NULL, this);
                ZPosSetChanged(NULL, this);
                ZServoParamChanged(NULL, this);

                // reset and zero GVP
                GVP_zero_all_smooth ();
        }
}

void RPSPMC_Control::Init_SPMC_after_cold_start (){
        // init filter sections
        if (rpspmc_pacpll && rpspmc_hwi){
                gtk_combo_box_set_active (GTK_COMBO_BOX (update_bq_filterS1_widget), 0);
                rpspmc_hwi->status_append (" * INIT-BQS1...\n"); for (int i=0; i<10; ++i){ while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE); usleep(20000); }
                gtk_combo_box_set_active (GTK_COMBO_BOX (update_bq_filterS1_widget), 3);
                rpspmc_hwi->status_append (" * INIT-BQS1 to AB.\n"); for (int i=0; i<10; ++i){ while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE); usleep(20000); }
                gtk_combo_box_set_active (GTK_COMBO_BOX (update_bq_filterS2_widget), 0);
                rpspmc_hwi->status_append (" * INIT-BQS2...\n"); for (int i=0; i<10; ++i){ while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE); usleep(20000); }
                gtk_combo_box_set_active (GTK_COMBO_BOX (update_bq_filterS2_widget), 3);
                rpspmc_hwi->status_append (" * INIT-BQS2 to AB.\n"); for (int i=0; i<10; ++i){ while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE); usleep(20000); }
                gtk_combo_box_set_active (GTK_COMBO_BOX (update_bq_filterZS_widget), 0);
                rpspmc_hwi->status_append (" * INIT-BQZS...\n"); for (int i=0; i<10; ++i){ while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE); usleep(20000); }
                gtk_combo_box_set_active (GTK_COMBO_BOX (update_bq_filterZS_widget), 5);
                rpspmc_hwi->status_append (" * INIT-BQZS to ByPass.\n"); for (int i=0; i<10; ++i){ while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE); usleep(20000); }
        }
}

void  RPSPMC_Control::spmc_server_control_callback (GtkWidget *widget, RPSPMC_Control *self){
        g_message ("RPSPMC+Control::spmc_server_control_callback DMA: %d ms, VL: %d",(int)spmc_parameters.rpspmc_dma_pull_interval, (int)self->rp_verbose_level);
        //RPSPMC_Control::GVP_AllZero (self);
        if (rpspmc_pacpll){
                rpspmc_pacpll->write_parameter ("RPSPMC_DMA_PULL_INTERVAL", (int)spmc_parameters.rpspmc_dma_pull_interval);
                rpspmc_pacpll->write_parameter ("PACVERBOSE", (int)self->rp_verbose_level);
        }
}

void RPSPMC_Control::GVP_zero_all_smooth (){
        // reset GVP
        int vector_index=0;
        int gvp_options=0;

        rpspmc_hwi->resetVPCconfirmed ();
        make_dUZXYAB_vector (vector_index++,
                             0., 0., // GVP_du[k], GVP_dz[k],
                             0., 0., 0., 0., 0., 0.,  // GVP_dx[k], GVP_dy[k], GVP_da[k], GVP_db[k], AM, FM
                             100, 0, 0, 0.5, // GVP_points[k], GVP_vnrep[k], GVP_vpcjr[k], GVP_ts[k],
                             0, VP_INITIAL_SET_VEC | gvp_options);
        append_null_vector (vector_index, gvp_options);

        g_message ("last vector confirmed: %d, need %d", rpspmc_hwi->getVPCconfirmed (), rpspmc_hwi->last_vector_index);

        int timeout = 100;
        while (rpspmc_hwi->getVPCconfirmed () < rpspmc_hwi->last_vector_index && timeout--){
#if GVP_DEBUG_VERBOSE > 4
                g_message ("GVP-ZERO: Waiting for GVP been written and confirmed. [Vector %d]", rpspmc_hwi->getVPCconfirmed ());
#endif
                usleep(20000);
        }
        if (timeout > 0)
                g_message ("GVP-ZERO been written and confirmed for vector #%d. Init GVP. Executing GVP now.", rpspmc_hwi->getVPCconfirmed ());
        else {
                g_message ("GVP-ZERO program write and confirm failed. Aborting Scan.");
                rpspmc_hwi->EndScan2D();
                return NULL;
        }

        usleep(200000);
        rpspmc_hwi->GVP_execute_vector_program(); // non blocking
}

int RPSPMC_Control::GVP_AllZero (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
        g_message ("RPSPMC_Control::GVP_AllZero: GVP STOP and set ALL ZERO   ** DANGER IF IN OPERATION ** -- No Action here in productioin version or confirm dialog.");

        if (gapp->question_yes_no ("WARNING WARNING WARNING\n"
                                   "RPSPMC will force all GVP componet to zero (smooth).\n"
                                   "This does NOT effect the Z control/feedback Z value, only any GVP offsets.\n"
                                   "This aborts any GVP (scan/probe) in progress.\n"
                                   "Confirm to proceed with Set All GVP to Zero -- No: no action.", self->window)){
                rpspmc_hwi->GVP_abort_vector_program ();
                usleep(200000);
                self->GVP_zero_all_smooth ();
                usleep(200000);
                rpspmc_hwi->GVP_reset_UAB ();
        }
        return 0;
}

int RPSPMC_Control::DSP_cret_callback (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	self->center_return_flag = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));	
        return 0;
}


int RPSPMC_Control::ldc_callback(GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
	//self->update_controller();
	return 0;
}

void RPSPMC_Control::BiasChanged(Param_Control* pcs, RPSPMC_Control* self){
        int j=0;
        spmc_parameters.bias_monitor = self->bias;
        if (rpspmc_pacpll)
                rpspmc_pacpll->write_parameter ("SPMC_BIAS", self->bias);

        // add user event, mirror basic parameters to GXSM4 main
        if (rpspmc_hwi)
                rpspmc_hwi->add_user_event_now ("Bias_adjust", "[V] (pre,now)", main_get_gapp()->xsm->data.s.Bias, self->bias);
 	main_get_gapp()->xsm->data.s.Bias = self->bias;
}

void RPSPMC_Control::Slope_dZX_Changed(Param_Control* pcs, RPSPMC_Control* self){
        if (rpspmc_pacpll){
                double zx_ratio = main_get_gapp()->xsm->Inst->XResolution () / main_get_gapp()->xsm->Inst->ZResolution (); 
                //rpspmc_pacpll->write_parameter ("SPMC_SLOPE_SLEW", 100.0);
                //usleep (100000);
                rpspmc_pacpll->write_parameter ("SPMC_SLOPE_DZX", zx_ratio * self->area_slope_x);
        }
}

void RPSPMC_Control::Slope_dZY_Changed(Param_Control* pcs, RPSPMC_Control* self){
        if (rpspmc_pacpll){
                double zy_ratio = main_get_gapp()->xsm->Inst->YResolution () / main_get_gapp()->xsm->Inst->ZResolution ();
                //rpspmc_pacpll->write_parameter ("SPMC_SLOPE_SLEW", 100.0); // MIN 10
                //usleep (100000);
                rpspmc_pacpll->write_parameter ("SPMC_SLOPE_DZY", zy_ratio * self->area_slope_y);
        }
}

void RPSPMC_Control::ZPosSetChanged(Param_Control* pcs, RPSPMC_Control *self){
        if (rpspmc_pacpll){
                const gchar *SPMC_SET_ZPOS_SERVO_COMPONENTS[] = {
                        "SPMC_Z_SERVO_SETPOINT_CZ",
                        "SPMC_Z_SERVO_SETPOINT",
                        "SPMC_Z_SERVO_UPPER", 
                        NULL };
                double jdata[3];
                jdata[0] = main_get_gapp()->xsm->Inst->ZA2Volt(-self->zpos_ref); // FPGA  internal has pos Z feedback behavior, i.e. Z pos = tip down
                jdata[1] = self->mix_level[0] > 0. ? main_get_gapp()->xsm->Inst->nAmpere2V(self->mix_level[0])
                                                   : main_get_gapp()->xsm->Inst->nAmpere2V(self->mix_set_point[0]);
                jdata[2] = self->mix_level[0] > 0.  // Manual CZ-Control of Upper limit via Z-Position?
                        ? main_get_gapp()->xsm->Inst->ZA2Volt(-self->zpos_ref)  // FPGA  internal has pos Z feedback behavior, i.e. Z pos = tip down
                        : self->z_limit_upper_v; //5.; // UPPER
                if (self->mix_level[0] > 0.)
                        self->ec_z_upper->Freeze();
                else
                        self->ec_z_upper->Thaw();

                
                rpspmc_pacpll->write_array (SPMC_SET_ZPOS_SERVO_COMPONENTS, 0, NULL,  3, jdata);
        }

        if (rpspmc_hwi){
                rpspmc_hwi->add_user_event_now ("Z_Set_Point_adjust", "[A] (pre,now)", main_get_gapp()->xsm->data.s.ZSetPoint, self->zpos_ref);
                rpspmc_hwi->add_user_event_now ("I_Set_Point_adjust", "[nA] (setpt, level)", self->mix_set_point[0], self->mix_level[0], true);
        }
        
        // mirror basic parameters to GXSM4 main
        main_get_gapp()->xsm->data.s.SetPoint  = self->zpos_ref;
        main_get_gapp()->xsm->data.s.ZSetPoint = self->zpos_ref;
}

void RPSPMC_Control::ZServoParamChanged(Param_Control* pcs, RPSPMC_Control *self){
        if (rpspmc_pacpll){

                const gchar *SPMC_SET_Z_SERVO_COMPONENTS[] = {
                        "SPMC_Z_SERVO_MODE", 
                        "SPMC_Z_SERVO_SETPOINT",
                        "SPMC_Z_SERVO_IN_OFFSETCOMP", 
                        "SPMC_Z_SERVO_CP", 
                        "SPMC_Z_SERVO_CI", 
                        "SPMC_Z_SERVO_UPPER", 
                        "SPMC_Z_SERVO_LOWER", 
                        NULL };
                double jdata[6];
                int jdata_i[1];

                // obsolete invert, critical/no good here. Add final Z-polarity at very end
                self->z_servo[SERVO_CP] = pow (10., spmc_parameters.z_servo_cp_db/20.); 
                self->z_servo[SERVO_CI] = pow (10., spmc_parameters.z_servo_ci_db/20.);

                jdata_i[0] = (self->mix_transform_mode[0] & 0xff) | ((self->I_fir & 0xff) << 8); // TRANSFORM MDOE | FIR SELECTION
                jdata[0]   = self->mix_level[0] > 0. ? main_get_gapp()->xsm->Inst->nAmpere2V(self->mix_level[0])
                                                     : main_get_gapp()->xsm->Inst->nAmpere2V(self->mix_set_point[0]);
                jdata[1]   = main_get_gapp()->xsm->Inst->nAmpere2V(self->mix_in_offsetcomp[0]);
                jdata[2]   = self->z_servo[SERVO_CP];
                jdata[3]   = self->z_servo[SERVO_CI];
                jdata[4]   = self->mix_level[0] > 0.  // Manual CZ-Control of Upper limit via Z-Position?
                        ? main_get_gapp()->xsm->Inst->ZA2Volt(-self->zpos_ref)  // FPGA  internal has pos Z feedback behavior, i.e. Z pos = tip down
                        : self->z_limit_upper_v; //5.; // UPPER
                jdata[5]   = spmc_parameters.z_servo_lower; // -5.; // LOWER

                if (self->mix_level[0] > 0.)
                        self->ec_z_upper->Freeze();
                else
                        self->ec_z_upper->Thaw();
                
                rpspmc_pacpll->write_array (SPMC_SET_Z_SERVO_COMPONENTS, 1, jdata_i,  6, jdata);
        }

        if (rpspmc_hwi){
                rpspmc_hwi->add_user_event_now ("Z_Servo_adjust", "[dB] (CP,CI)", spmc_parameters.z_servo_cp_db, spmc_parameters.z_servo_ci_db );
                rpspmc_hwi->add_user_event_now ("Z_Set_Point_adjust", "[A] (pre,now)", main_get_gapp()->xsm->data.s.ZSetPoint, self->zpos_ref, true);
                rpspmc_hwi->add_user_event_now ("I_Set_Point_adjust", "[nA] (setpt, level)", self->mix_set_point[0], self->mix_level[0], true);
        }

        // mirror basic parameters to GXSM4 main
        main_get_gapp()->xsm->data.s.Current = self->mix_set_point[0];
}


void RPSPMC_Control::ZServoControl (GtkWidget *widget, RPSPMC_Control *self){
        /*
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))){
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_MODE", self->mix_transform_mode[0]);
                // set MixMode to grey/inconsistent
        } else {
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_MODE", MM_OFF);
                // update MixMode
        }
        */
}

// ** THIS INVERTS ONYL THE Z OUTPUT, does NOT effect any internal behaviors!
void RPSPMC_Control::ZServoControlInv (GtkWidget *widget, RPSPMC_Control* self){
        int flg = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));

        // CONFIRMATION DLG
        if (flg)
                if ( gapp->question_yes_no ("WARNING: Please confirm Z-Polarity set to NEGATIVE."))
                        rpspmc_pacpll->write_parameter ("SPMC_Z_POLARITY", -1);
                else
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (widget), true);
        else
                if ( gapp->question_yes_no ("WARNING: Please confirm Z-Polarity set to POSITIV."))
                        rpspmc_pacpll->write_parameter ("SPMC_Z_POLARITY", 1);
                else
                        gtk_check_button_set_active (GTK_CHECK_BUTTON (widget), false);
}


void RPSPMC_Control::ChangedNotifyVP(Param_Control* pcs, RPSPMC_Control* self){
        g_message ("**ChangedNotifyVP**");

	//gchar *id=pcs->get_refname();

        
        for (int j=0; j<8; ++j)
                if (pcs == self->GVP_V0Set_ec[j]){
                        const gchar *info=self->GVP_V0Set_ec[j]->get_info();
                        if (info)
                                switch (info[0]){
                                case 'X': self->GVP_V0Set_ec[j]->set_css_name("bgrey"); break;
                                case 'S': self->GVP_V0Set_ec[j]->set_css_name("orange"); break;
                                default: self->GVP_V0Set_ec[j]->set_css_name("normal"); break;
                                }
                        else
                                self->GVP_V0Set_ec[j]->set_css_name("normal");
                        self->GVP_V0Set_ec[j]->Put_Value (); // update
                }
}

int RPSPMC_Control::choice_mixmode_callback (GtkWidget *widget, RPSPMC_Control *self){
	gint channel=0;
        gint selection=0;

        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

        channel = GPOINTER_TO_INT (g_object_get_data(G_OBJECT (widget), "mix_channel"));
        selection = atoi (gtk_combo_box_get_active_id (GTK_COMBO_BOX (widget)));


	PI_DEBUG_GP (DBG_L3, "MixMode[%d]=0x%x\n",channel,selection);

	self->mix_transform_mode[channel] = selection;
        if (channel == 0){
                g_print ("Choice MIX%d MT=%d\n", channel, selection);
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_MODE", (selection & 0xff) | ((self->I_fir&0xff) << 8)); // mapped to MM_LIN/LOG (0,1) | FIR SELECTION
        }
        PI_DEBUG_GP (DBG_L4, "%s ** 2\n",__FUNCTION__);

        self->ZServoParamChanged (NULL, self);

        PI_DEBUG_GP (DBG_L4, "%s **3 done\n",__FUNCTION__);
        return 0;
}

void RPspmc_pacpll::set_stream_mux(int *mux_source_selections){
        guint64 mux = __GVP_selection_muxval (mux_source_selections);
        const gchar *SPMC_SET_SMUX_COMPONENTS[] = {
                "SPMC_GVP_STREAM_MUX_0",
                "SPMC_GVP_STREAM_MUX_1",
                "SPMC_GVP_STREAM_MUXH",
                NULL };
        int jdata_i[3];
        jdata_i[0] = (guint32)(mux&0xffffffff);
        jdata_i[1] = (guint32)(mux>>32);
        jdata_i[2] = __GVP_selection_muxHval (mux_source_selections);
        write_array (SPMC_SET_SMUX_COMPONENTS, 3, jdata_i,  0, NULL);
}

int RPSPMC_Control::choice_scansource_callback (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) == -1 || g_object_get_data( G_OBJECT (widget), "updating_in_progress")){
                PI_DEBUG_GP (DBG_L4, "%s bail out for label refresh only\n",__FUNCTION__);
                return 0;
        }
            
	gint selection = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        gint channel   = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "scan_channel_source"));

	self->scan_source[channel] = selection;

        g_message ("RPSPMC_Control::choice_scansource_callback: scan_source[%d] = %d [%s]", channel, selection, rpspmc_swappable_signals[selection].label);

	PI_DEBUG_GP (DBG_L3, "GVP STREAM MUX SELECTOR:  [%d] %s ==> [%d]\n",signal, rpspmc_swappable_signals[selection].label, channel);

        rpspmc_hwi->set_spmc_signal_mux (RPSPMC_ControlClass->scan_source);

        if (rpspmc_pacpll)
                rpspmc_pacpll->set_stream_mux (self->scan_source);

        main_get_gapp()->channelselector->SetModeChannelSignal(13+channel,
                                                               rpspmc_swappable_signals[selection].label,
                                                               rpspmc_swappable_signals[selection].label,
                                                               rpspmc_swappable_signals[selection].unit,
                                                               1.0 //rpspmc_swappable_signals[selection].scale_factor // ** data is scaled to unit at transfer time
                                                               );

        
        return 0;
}

void RPSPMC_Control::update_GUI(){
        if (!GUI_ready) return;

	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "DSP_EC_list"),
			(GFunc) App::update_ec, NULL);
	g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "DSP_VPC_OPTIONS_list"),
			(GFunc) callback_update_GVP_vpc_option_checkbox, this);
}


void RPSPMC_Control::update_FPGA_from_GUI (){ // after cold start
        //zpos_ref = 0.; // ** via limits

        z_limit_upper_v = spmc_parameters.z_servo_upper; //5.; // READBACK UPPER

        g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (window), "FPGA_readback_update_list"), (GFunc) App::update_parameter, NULL); // UPDATE PARAMETRE form GUI!

        bias = spmc_parameters.bias_monitor; // Volts direct
        mix_set_point[0] = spmc_parameters.z_servo_setpoint / main_get_gapp()->xsm->Inst->nAmpere2V(1.0); // convert nA!!

        if (spmc_parameters.z_servo_cp > 0.0)
                spmc_parameters.z_servo_cp_db = 20.* log10(spmc_parameters.z_servo_cp); // convert to dB *** pow (10., spmc_parameters.z_servo_cp_db/20.);
        if (spmc_parameters.z_servo_ci > 0.0)
                spmc_parameters.z_servo_ci_db = 20.* log10(spmc_parameters.z_servo_ci); // convert to dB

        // push to FPGA
        BiasChanged(NULL, this);
        ZServoParamChanged (NULL, this);
        Slope_dZX_Changed(NULL, this);
        Slope_dZY_Changed(NULL, this);
        ZPosSetChanged(NULL, this);
        ZServoControl (NULL, this);
}

void RPSPMC_Control::update_GUI_from_FPGA (){ // after warm start or re-connect
        //zpos_ref = 0.; // ** via limits
        bias = spmc_parameters.bias_monitor; // Volts direct
        mix_set_point[0] = spmc_parameters.z_servo_setpoint / main_get_gapp()->xsm->Inst->nAmpere2V(1.0); // convert nA!!

        if (spmc_parameters.z_servo_cp > 0.0)
                spmc_parameters.z_servo_cp_db = 20.* log10(spmc_parameters.z_servo_cp); // convert to dB *** pow (10., spmc_parameters.z_servo_cp_db/20.);
        if (spmc_parameters.z_servo_ci > 0.0)
                spmc_parameters.z_servo_ci_db = 20.* log10(spmc_parameters.z_servo_ci); // convert to dB
        
        g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (window), "FPGA_readback_update_list"), (GFunc) App::update_ec, NULL); // UPDATE GUI!
}

void RPSPMC_Control::update_zpos_readings(){
        //double zp,a,b;
        //main_get_gapp()->xsm->hardware->RTQuery ("z", zp, a, b);
        zpos_mon = main_get_gapp()->xsm->Inst->Volt2ZA(spmc_parameters.gxsm_z_polarity*spmc_parameters.z_monitor + spmc_parameters.zs_monitor); // remove slope component
        gchar *info;
        if (-spmc_parameters.zs_monitor >= 0.)
                info = g_strdup_printf (" + %g "UTF8_ANGSTROEM" Slp",  main_get_gapp()->xsm->Inst->Volt2ZA(-spmc_parameters.zs_monitor));
        else
                info = g_strdup_printf (" %g "UTF8_ANGSTROEM" Slp",  main_get_gapp()->xsm->Inst->Volt2ZA(-spmc_parameters.zs_monitor));
        ZPos_ec->set_info (info);
        ZPos_ec->Put_Value ();
        g_free (info);
}

#if 0
guint RPSPMC_Control::refresh_zpos_readings(RPSPMC_Control *self){ 
	if (main_get_gapp()->xsm->hardware->IsSuspendWatches ())
		return TRUE;

	self->update_zpos_readings ();
	return TRUE;
}

int RPSPMC_Control::zpos_monitor_callback( GtkWidget *widget, RPSPMC_Control *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))){
                //g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (widget), "DSP_zpos_control_list"), (GFunc) App::thaw_ec, NULL);
                self->zpos_refresh_timer_id = g_timeout_add (200, (GSourceFunc)RPSPMC_Control::refresh_zpos_readings, self);
        }else{
                //g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (widget), "DSP_zpos_control_list"), (GFunc) App::freeze_ec, NULL);

                if (self->zpos_refresh_timer_id){
                        g_source_remove (self->zpos_refresh_timer_id);
                        self->zpos_refresh_timer_id = 0;
                }
        }
        return 0;
}
#endif

int RPSPMC_Control::choice_prbsource_callback (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
        
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) == -1)
                return 0;

        gint selection = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	gint channel   = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "prb_channel_source"));

	self->probe_source[channel] = selection;

        if (RPSPMC_ControlClass && rpspmc_pacpll){
                rpspmc_hwi->set_spmc_signal_mux (RPSPMC_ControlClass->probe_source);
                rpspmc_pacpll->set_stream_mux (RPSPMC_ControlClass->probe_source);
        }
        g_message ("RPSPMC_Control::choice_prbsource_callback: probe_source[%d] = %d [%s] MUX=%016lx", channel, selection,
                   rpspmc_swappable_signals[selection].label,
                   __GVP_selection_muxval (self->probe_source));

        return 0;
}



int RPSPMC_Control::callback_change_IV_option_flags (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		self->IV_option_flags = (self->IV_option_flags & (~msk)) | msk;
	else
		self->IV_option_flags &= ~msk;

        self->set_tab_settings ("IV", self->IV_option_flags, self->IV_auto_flags, self->IV_glock_data);
        return 0;
}

int RPSPMC_Control::callback_change_IV_auto_flags (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		self->IV_auto_flags = (self->IV_auto_flags & (~msk)) | msk;
	else
		self->IV_auto_flags &= ~msk;

	//if (self->write_vector_mode == PV_MODE_IV)
	//	self->raster_auto_flags = self->IV_auto_flags;

        self->set_tab_settings ("IV", self->IV_option_flags, self->IV_auto_flags, self->IV_glock_data);
        return 0;
}

int RPSPMC_Control::Probing_RampFBoff_callback(GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
        self->RampFBoff_mode = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
        return 0;
}

int RPSPMC_Control::Probing_multiIV_callback(GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "DSP_multiIV_list"),
				(GFunc) gtk_widget_show, NULL);
	else
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "DSP_multiIV_list"),
				(GFunc) gtk_widget_hide, NULL);

        self->multiIV_mode = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
        return 0;
}


int RPSPMC_Control::Probing_exec_IV_callback( GtkWidget *widget, RPSPMC_Control *self){

        if (rpspmc_hwi->is_scanning()){
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is busy scanning. Please stop scanning for any GVP actions.");
                //gapp->warning ("RPSPMC is busy scanning.\nPlease stop scanning and any GVP actions.", window);
                return -1;
        }

	if (self->check_vp_in_progress ()){
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is streaming GVP data -- ABORT FIRST");
                return -1;
        }

        
        self->current_auto_flags = self->IV_auto_flags;


        rpspmc_hwi->set_spmc_signal_mux (RPSPMC_ControlClass->probe_source);
        if (rpspmc_pacpll)
                rpspmc_pacpll->set_stream_mux (self->probe_source);

        self->init_vp_signal_info_lookup_cache(); // update signal mapping cache

        if (self->IV_auto_flags & FLAG_AUTO_RUN_INITSCRIPT){
                gchar *tmp = g_strdup ("vp-sts-initial");
                main_get_gapp()->SignalRemoteActionToPlugins (&tmp);
                g_free (tmp);
        }

	if (self->IV_auto_flags & FLAG_AUTO_GLOCK){
		self->vis_Source  = self->IV_glock_data[0];
		self->vis_XSource = self->IV_glock_data[1];
		self->vis_PSource = self->IV_glock_data[2];
		self->vis_XJoin   = self->IV_glock_data[3];
		self->vis_PlotAvg = self->IV_glock_data[4];
		self->vis_PlotSec = self->IV_glock_data[5];
	} else {
		self->vis_Source = self->IV_glock_data[0] = self->Source ;
		self->vis_XSource= self->IV_glock_data[1] = self->XSource;
		self->vis_PSource= self->IV_glock_data[2] = self->PSource;
		self->vis_XJoin  = self->IV_glock_data[3] = self->XJoin  ;
		self->vis_PlotAvg= self->IV_glock_data[4] = self->PlotAvg;
		self->vis_PlotSec= self->IV_glock_data[5] = self->PlotSec;
	}

	self->current_auto_flags = self->IV_auto_flags;

        // write and exec GVP code on controller and initiate data streaming

	self->write_spm_vector_program (1, PV_MODE_IV); // Write and Exec GVP program

	return 0;
}

int RPSPMC_Control::Probing_write_IV_callback( GtkWidget *widget, RPSPMC_Control *self){
         if (rpspmc_hwi->is_scanning()){
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is busy scanning. Please stop scanning for any GVP actions.");
                //gapp->warning ("RPSPMC is busy scanning.\nPlease stop scanning and any GVP actions.", window);
                return -1;
        }

        if (rpspmc_hwi->probe_fifo_thread_active>0){
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is streaming GVP data -- FORCE UPDATE REQUESTED");
        }

        // write GVP code to controller
        self->write_spm_vector_program (0, PV_MODE_IV);
        return 0;
}

// GVP vector program editor 

int RPSPMC_Control::callback_update_GVP_vpc_option_checkbox (GtkWidget *widget, RPSPMC_Control *self){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	int  k   = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "VPC"));
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	gtk_check_button_set_active (GTK_CHECK_BUTTON(widget), (self->GVP_opt[k] & msk) ? 1:0);

	if( gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))){
		g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTOPC_list"),
				(GFunc) gtk_widget_show, NULL);
		g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTHDR_list"),
				(GFunc) gtk_widget_show, NULL);
	} else {
		g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTOPC_list"),
				(GFunc) gtk_widget_hide, NULL);
		g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTHDR_list"),
				(GFunc) gtk_widget_hide, NULL);
        }


        
        self->set_tab_settings ("VP", self->GVP_option_flags, self->GVP_auto_flags, self->GVP_glock_data);
        return 0;
}

int RPSPMC_Control::callback_change_LCK_mode (GtkWidget *widget, RPSPMC_Control *self){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(widget), "Bit_Mask"));
	if( gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                spmc_parameters.lck_mode |= msk;
        else
                spmc_parameters.lck_mode &= ~msk;

        g_message ("RPSPMC_Control::callback_change_LCK_mode %d", spmc_parameters.lck_mode);
        
        return 0;
}

int RPSPMC_Control::callback_change_RF_mode (GtkWidget *widget, RPSPMC_Control *self){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(widget), "Bit_Mask"));
	if( gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                spmc_parameters.rf_gen_mode |= msk;
        else
                spmc_parameters.rf_gen_mode &= ~msk;

        g_message ("RPSPMC_Control::callback_change_RF_mode %d", spmc_parameters.rf_gen_mode);
        
        return 0;
}

int RPSPMC_Control::callback_change_GVP_vpc_option_flags (GtkWidget *widget, RPSPMC_Control *self){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	int  k = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "VPC"));
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(widget), "Bit_Mask"));

        int x=self->GVP_opt[k];
        //g_message (" RPSPMC_Control::callback_change_GVP_vpc_option_flags :: changed [%d] %04x ^ %04x",k,x,msk);
	if( gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))){
		self->GVP_opt[k] = (self->GVP_opt[k] & (~msk)) | msk;
                if (msk & (1<<7)){
                        g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTOPC_list"),
                                        (GFunc) gtk_widget_show, NULL);
                        g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTHDR_list"),
                                        (GFunc) gtk_widget_show, NULL);
                }
	} else {
		self->GVP_opt[k] &= ~msk;
                if (msk & (1<<7) && x & (1<<7)){
                        //g_message (" RPSPMC_Control::callback_change_GVP_vpc_option_flags :: uncheck VX [%d]",k);
                        g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTOPC_list"),
                                        (GFunc) gtk_widget_hide, NULL);
                        int vx=0;
                        for (int j=0; j < N_GVP_VECTORS; ++j) 
                                if (self->GVP_opt[j]&(1<<7)) { vx++; break; }
                        if (!vx)
                                g_slist_foreach((GSList*)g_object_get_data (G_OBJECT(widget), "DSP_VPC_EXTHDR_list"),
                                                (GFunc) gtk_widget_hide, NULL);
                }
        }
        
        self->set_tab_settings ("VP", self->GVP_option_flags, self->GVP_auto_flags, self->GVP_glock_data);

        if (k==0)
                if ((msk & VP_INITIAL_SET_VEC) && (x & VP_INITIAL_SET_VEC)) // un-checked
                        for (int j=0; j<8; ++j){
                                self->GVP_V0Set_ec[j]->set_info(NULL);
                                self->GVP_V0Set_ec[j]->set_css_name("normal");
                                self->GVP_V0Set_ec[j]->Put_Value (); // update
                        }
                else if ((msk & VP_INITIAL_SET_VEC) && !(x & VP_INITIAL_SET_VEC)) // checked
                        for (int j=0; j<8; ++j){
                                switch (j){
                                case 0: case 1:
                                        self->GVP_V0Set_ec[j]->set_info("X");
                                        self->GVP_V0Set_ec[j]->set_css_name("bgrey");
                                        break;
                                default:
                                        self->GVP_V0Set_ec[j]->set_info("Set");
                                        self->GVP_V0Set_ec[j]->set_css_name("orange");
                                        break;
                                }
                                self->GVP_V0Set_ec[j]->Put_Value (); // update
                        }

        return 0;
}

int RPSPMC_Control::callback_change_GVP_option_flags (GtkWidget *widget, RPSPMC_Control *self){
        //        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		self->GVP_option_flags = (self->GVP_option_flags & (~msk)) | msk;
	else
		self->GVP_option_flags &= ~msk;

	if (self->write_vector_mode == PV_MODE_GVP)
		self->raster_auto_flags = self->GVP_auto_flags;

        self->set_tab_settings ("VP", self->GVP_option_flags, self->GVP_auto_flags, self->GVP_glock_data);
        self->GVP_store_vp ("VP_set_last"); // last in view
        return 0;
}

int RPSPMC_Control::callback_GVP_store_vp (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	self->GVP_store_vp ((const gchar*)g_object_get_data(G_OBJECT(widget), "key"));
        return 0;
}
int RPSPMC_Control::callback_GVP_restore_vp (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	self->GVP_restore_vp ((const gchar*)g_object_get_data(G_OBJECT(widget), "key"));
        return 0;
}


int RPSPMC_Control::callback_GVP_preview_me (GtkWidget *widget, RPSPMC_Control *self){
        int i = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "AXIS"));
        if (i>0 && i <= 8){
                self->GVP_preview_on[i] = self->GVP_preview_on[i] ? 0:1;

                if (self->GVP_preview_on[i]){
                        const gchar *gvpcolor = g_strdup_printf("gvpcolor%d",i);
                        gtk_widget_set_name (widget, gvpcolor);
                        g_free (gvpcolor);
                } else
                        gtk_widget_set_name (widget, "normal");
        }

        // auto hide/show
        int num_previews=0;
        for(int k=0; k < 8; ++k)
                num_previews += self->GVP_preview_on[k];

        if (num_previews > 0)
                gtk_widget_show (self->WavePreview);
        else
                gtk_widget_hide (self->WavePreview);
        
        return 0;
}


int RPSPMC_Control::callback_change_GVP_auto_flags (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint64 msk = (guint64) GPOINTER_TO_UINT (g_object_get_data(G_OBJECT(widget), "Bit_Mask"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
		self->GVP_auto_flags = (self->GVP_auto_flags & (~msk)) | msk;
	else
		self->GVP_auto_flags &= ~msk;

        self->set_tab_settings ("VP", self->GVP_option_flags, self->GVP_auto_flags, self->GVP_glock_data);
        return 0;
}

int RPSPMC_Control::Probing_exec_GVP_callback( GtkWidget *widget, RPSPMC_Control *self){
	self->current_auto_flags = self->GVP_auto_flags;

        if (!self->check_GVP())
                return -1;
        
        if (rpspmc_hwi->is_scanning()){
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is busy scanning. Please stop scanning for any GVP actions.");
                if (gapp->question_yes_no ("WARNING: Scan in progress.\n"
                                           "RPSPMC is currently streaming GVP data.\n"
                                           "Abort current activity/scanning and start GVP?", self->window)){
                        rpspmc_hwi->GVP_abort_vector_program ();
                        usleep(200000);
                } else
                        return -1;
        }

	if (self->check_vp_in_progress ()){ 
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is streaming GVP data -- ABORT FIRST");
                if (gapp->question_yes_no ("WARNING: Scan/Probe in progress.\n"
                                           "RPSPMC is currently streaming GVP data.\n"
                                           "Abort current activity and start GVP?",  self->window)){
                        rpspmc_hwi->GVP_abort_vector_program ();
                        usleep(200000);
                } else
                        return -1;
        }

        // make sure all is sane and cleaned up
        rpspmc_hwi->GVP_abort_vector_program ();
        usleep(100000);

        rpspmc_hwi->set_spmc_signal_mux (RPSPMC_ControlClass->probe_source);
        if (rpspmc_pacpll)
                rpspmc_pacpll->set_stream_mux (self->probe_source);

        self->init_vp_signal_info_lookup_cache(); // update signal mapping cache


        if (self->GVP_auto_flags & FLAG_AUTO_RUN_INITSCRIPT){
                gchar *tmp = g_strdup ("vp-gvp-initial");
                main_get_gapp()->SignalRemoteActionToPlugins (&tmp);
                g_free (tmp);
        }
      
	if (self->GVP_auto_flags & FLAG_AUTO_GLOCK){
		self->vis_Source  = self->GVP_glock_data[0];
		self->vis_XSource = self->GVP_glock_data[1];
		self->vis_PSource = self->GVP_glock_data[2];
		self->vis_XJoin   = self->GVP_glock_data[3];
		self->vis_PlotAvg = self->GVP_glock_data[4];
		self->vis_PlotSec = self->GVP_glock_data[5];
	} else {
		self->vis_Source = self->GVP_glock_data[0] = self->Source ;
		self->vis_XSource= self->GVP_glock_data[1] = self->XSource;
		self->vis_PSource= self->GVP_glock_data[2] = self->PSource;
		self->vis_XJoin  = self->GVP_glock_data[3] = self->XJoin  ;
		self->vis_PlotAvg= self->GVP_glock_data[4] = self->PlotAvg;
		self->vis_PlotSec= self->GVP_glock_data[5] = self->PlotSec;
	}

	self->current_auto_flags = self->GVP_auto_flags;

        // write and exec GVP code on controller and initiate data streaming
        
	self->write_spm_vector_program (1, PV_MODE_GVP); // Write and Exec GVP program

	return 0;
}


int RPSPMC_Control::Probing_write_GVP_callback( GtkWidget *widget, RPSPMC_Control *self){

        self->check_GVP();
        
        if (rpspmc_hwi->is_scanning()){
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is busy scanning. Please stop scanning for any GVP actions.");
                //gapp->warning ("RPSPMC is busy scanning.\nPlease stop scanning and any GVP actions.", window);
                return -1;
        }

        if (rpspmc_hwi->probe_fifo_thread_active>0){
                g_message (" RPSCPM_Control::Probing_abort_callback ** RPSPMC is streaming GVP data -- FORCE UPDATE REQUESTED");
        }

        // write GVP code to controller
        self->write_spm_vector_program (0, PV_MODE_GVP);

        gtk_widget_queue_draw (self->gvp_preview_area); // update wave

        return 0;
}

// Graphs Callbacks
int RPSPMC_Control::change_source_callback (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	guint32 channel = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "Source_Channel"));
	guint32 mapping = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(widget), "Source_Mapping"));
	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                switch (mapping){
                case MAP_SOURCE_REC:   self->Source  |= channel; break;
                case MAP_SOURCE_X:     self->XSource |= channel; break;
                case MAP_SOURCE_PLOTY: self->PSource |= channel; break;
                case MAP_SOURCE_AVG:   self->PlotAvg |= channel; break;
                case MAP_SOURCE_SEC:   self->PlotSec |= channel; break;
                default: break;
                }
        else
                switch (mapping){
                case MAP_SOURCE_REC:   self->Source  &= ~channel; break;
                case MAP_SOURCE_X:     self->XSource &= ~channel; break;
                case MAP_SOURCE_PLOTY: self->PSource &= ~channel; break;
                case MAP_SOURCE_AVG:   self->PlotAvg &= ~channel; break;
                case MAP_SOURCE_SEC:   self->PlotSec &= ~channel; break;
                default: break;
                }

        g_message ("CH=%08x MAP:%d *** S%08x X%08x Y%08x",channel, mapping, self->Source,self->XSource,self->PSource );
        
        //g_message ("change_source_callback: Ch: %08x => Srcs: %08x", channel, self->Source);
        
	self->vis_Source  = self->Source;
	self->vis_XSource = self->XSource;
	self->vis_PSource = self->PSource;
	self->vis_XJoin   = self->XJoin;
	self->vis_PlotAvg = self->PlotAvg;
	self->vis_PlotSec = self->PlotSec;

        // store
        self->store_graphs_values ();
        
	return 0;
}

int RPSPMC_Control::callback_XJoin (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	self->XJoin = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? TRUE : FALSE;
	self->vis_XJoin = self->XJoin;
        g_settings_set_boolean (self->hwi_settings, "probe-x-join", self->XJoin);
        return 0;
}

int RPSPMC_Control::callback_GrMatWindow (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L3, "%s \n",__FUNCTION__);
	self->GrMatWin = (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget))) ? TRUE : FALSE;
        g_settings_set_boolean (self->hwi_settings, "probe-graph-matrix-window", self->GrMatWin);
        return 0;
}

#define QQ44 (1LL<<44)

// Q44: 32766.0000 Hz -> phase_inc=4611404543  0000000112dc72ff
double dds_phaseinc (double freq){
        double fclk = 125e6; //ADC_SAMPLING_RATE;
        return QQ44*freq/fclk;
}


void RPSPMC_Control::configure_filter(int id, int mode, double sos[6], int decimation=0){
        const gchar *SPMC_SET_BQ_COMPONENTS[] = {
                "SPMC_SC_FILTER_MODE",   // INT
                "SPMC_SC_FILTER_SELECT", // INT ** SECTION / FILTER BLOCK SELECTION
                "SPMC_SC_BQ_COEF_B0", //spmc_parameters.sc_lck_bq_coef[0,1,2]
                "SPMC_SC_BQ_COEF_B1",
                "SPMC_SC_BQ_COEF_B2",
                "SPMC_SC_BQ_COEF_A0", //spmc_parameters.sc_lck_bq_coef[3,4,5] ** A0 == 1 per def, been ignored.
                "SPMC_SC_BQ_COEF_A1",
                "SPMC_SC_BQ_COEF_A2",
                NULL };

        int jdata_i[2] = { mode & 0xffff | (decimation << 16), id };
                
        g_message ("Config BQ Filter #%d (SOS AB coef)", id);
        rpspmc_pacpll->write_array (SPMC_SET_BQ_COMPONENTS, 2, jdata_i,  6, sos);
}

void RPSPMC_Control::delayed_filter_update (){
        configure_filter (1, spmc_parameters.sc_bq1mode, spmc_parameters.sc_bq1_coef, BQ_decimation);
        usleep (200000);
        configure_filter (2, spmc_parameters.sc_bq2mode, spmc_parameters.sc_bq2_coef, BQ_decimation);
        delayed_filter_update_timer_id = 0; // done.
        delayed_filter_update_ref = 0;
}

static guint RPSPMC_Control::delayed_filter_update_callback (RPSPMC_Control *self){
        self->delayed_filter_update ();
	return FALSE;
}

void RPSPMC_Control::bq_filter_adjust_callback(Param_Control* pcs, RPSPMC_Control *self){
        self->BQ_decimation = 128; //1 + (int)round(8 * 5000. / spmc_parameters.lck_frequency);
        spmc_parameters.lck_bq_dec_monitor = self->BQ_decimation;
        g_message ("BQ Filter Deciamtion: #%d",  self->BQ_decimation);
        //self->configure_filter (1, spmc_parameters.sc_bq1mode, spmc_parameters.sc_bq1_coef, decimation);
        //self->configure_filter (2, spmc_parameters.sc_bq2mode, spmc_parameters.sc_bq2_coef, decimation);

        // if not already scheduled, schedule delayed

        if (self->delayed_filter_update_timer_id) // if scheduled, remove and reset timeout next
                g_source_remove (self->delayed_filter_update_timer_id);

        self->delayed_filter_update_ref++;
        
        //if (!self->delayed_filter_update_timer_id) // if scheduled, remove and reset timeout next
        self->delayed_filter_update_timer_id = g_timeout_add (1000, (GSourceFunc)RPSPMC_Control::delayed_filter_update_callback, self);
        
}

void RPSPMC_Control::delayed_zsfilter_update (){
        configure_filter (10, spmc_parameters.sc_zs_bqmode, spmc_parameters.sc_zs_bq_coef, 50); // ZS / Notch DECIMATION
        delayed_zsfilter_update_timer_id = 0; // done.
}

static guint RPSPMC_Control::delayed_zsfilter_update_callback (RPSPMC_Control *self){
        self->delayed_zsfilter_update ();
	return FALSE;
}
void RPSPMC_Control::zs_input_filter_adjust_callback(Param_Control* pcs, RPSPMC_Control *self){
        // Update BQ SECTION ZS =10 (Z SERVO INPUT BiQuad) [can be used to program a NOTCH]
        // Test-DECII=32 for NOTCH intended IIR @ ~2MSPS / 32
        //self->configure_filter (10, spmc_parameters.sc_zs_bqmode, spmc_parameters.sc_zs_bq_coef, 128);
        // if not already scheduled, schedule delayed update

        if (self->delayed_zsfilter_update_timer_id)
                g_source_remove (self->delayed_zsfilter_update_timer_id);

        //if (!self->delayed_zsfilter_update_timer_id)
        self->delayed_zsfilter_update_timer_id = g_timeout_add (1000, (GSourceFunc)RPSPMC_Control::delayed_zsfilter_update_callback, self);
}

void RPSPMC_Control::lockin_adjust_callback(Param_Control* pcs, RPSPMC_Control *self){
        if (rpspmc_pacpll){

                double rp_fs = 125e6; // actual sampling freq
                int naclks   = (int)(spmc_parameters.lck_aclocks_per_sample_monitor) % 1000;
                int decii_mon= (int)(spmc_parameters.lck_decii_monitor);
                int decii2   = 0;
                if (naclks < 1 || naclks > 512) naclks = 66; // backup, 59 nominally
                double fn    = rp_fs / naclks / spmc_parameters.lck_frequency;

                int decii2_max = 12;

                // calculate decii2
                while (fn > 20. && decii2 < decii2_max){
                        decii2++;
                        fn /= 2.0;
                }

                g_message ("LCK Adjust Calc: RP-Fs: %g Hz, AD-NACLKs: %d, FLCK/BQ: %g Hz, Fnorm: %g @ decii2: %d", rp_fs, naclks, rp_fs/naclks/(1<<decii2), fn, decii2);

                
                const gchar *SPMC_LCK_COMPONENTS[] = {
                        "SPMC_LCK_MODE",      // INT
                        "SPMC_LCK_GAIN",      // INT (SHIFT) IN, OUT
                        "SPMC_LCK_FREQUENCY",
                        "SPMC_LCK_VOLUME",
                        NULL };
                int jdata_i[2];
                double jdata[2];
                jdata_i[0] = spmc_parameters.lck_mode;
                if (spmc_parameters.lck_sens <= 0.){
                        g_message ("LCK Gain defaulting to 1x (0)");
                        jdata_i[1] = 0;
                } else {
                        /* on RP:
                          int decii2_max   = 12;
                          int max_gain_out = 10;
                          int gain_in  =  gain      & 0xff; // 1x : == decii2
                          int gain_out = (gain>>8)  & 0xff; // 1x : == 
                          
                          gain_in  = gain_in  > decii2 ? 0 : decii2-gain_in;
                          gain_out = gain_out > max_gain_out ? max_gain_out : gain_out;

                          on FPGA:
                          ampl2_norm_shr <= 2*LCK_CORRSUM_Q_WIDTH - AM2_DATA_WIDTH - gain_out[10-1:0];
                          ... signal     <= signal_dec >>> gain_in[DECII2_MAX-1:0]; // custom reduced norm, MAX, full norm: decii2  ***** correleation producst/sums will overrun for large siganls if not at FULL NORM!!
                          ... ampl2 <= ab2 >>> ampl2_norm_shr; // Q48 for SQRT:   Norm Square, reduce to AM2 WIDTH


                        */
                        
                        double s = 1e-3*spmc_parameters.lck_sens; // mV to V
                        double g = 5.0/s; // to gain. I.e. gain 1x for 5V (5000mV)
                        if (g < 1.) g=1.;
                        int  gl2 = int(log2(g));
                        int  gain_in = gl2 <= decii2_max ? gl2 : decii2_max;
                        g_message ("LCK Gain request: %g -> shift#: %d ", g, gain_in);
                        int gain_out=0;

        
                        if (gain_out == 0 && gain_in > decii2)
                                gain_out = gain_in - decii2;

                        self->lck_gain = (1<<gain_in) * (1<<gain_out);
                        self->LCK_unit->set_gain (1./(double)((1<<gain_in) * (1<<gain_out)));
                        gchar *tmp = g_strdup_printf ("[%d x %d] %d", 1<<gain_in, 1<<gain_out, (1<<gain_in) * (1<<gain_out));
                        self->LCK_Sens->set_info (tmp);
                        self->LCK_Sens->Put_Value ();
                        g_free (tmp);
                        
                        jdata_i[1] = (decii2<<16) | (gain_out<<8) | gain_in;
                }
                jdata[0] =  spmc_parameters.lck_frequency;

                if  (self->LCK_Target > 0 && self->LCK_Target < LCK_NUM_TARGETS)
                        jdata[1] = modulation_targets[self->LCK_Target].scale_factor * self->LCK_Volume[self->LCK_Target]; // => Volts
                else
                        jdata[1] = 0.;

                g_message ("LCK Adjust SENDING UPDATE T#%d M:%d G:<<%x F:%g Hz V: %g {%g} Decii2 requested: %d => fs=%g Hz, fn=%g Hz ", self->LCK_Target, jdata_i[0], jdata_i[1], jdata[0], self->LCK_Volume[self->LCK_Target], jdata[1], decii2, rp_fs/naclks/(1<<decii2), fn);
                rpspmc_pacpll->write_array (SPMC_LCK_COMPONENTS, 2, jdata_i,  2, jdata);
        }
}


void RPSPMC_Control::rfgen_adjust_callback(Param_Control* pcs, RPSPMC_Control *self){
        if (rpspmc_pacpll){
                const gchar *SPMC_RFGEN_COMPONENTS[] = {
                        "SPMC_RF_GEN_MODE",      // INT
                        "SPMC_RF_GEN_FREQUENCY",
                        "SPMC_RF_GEN_FMSCALE",
                        NULL };
                int jdata_i[1];
                double jdata[2];
                jdata_i[0] = spmc_parameters.rf_gen_mode;
                jdata[0]   = spmc_parameters.rf_gen_frequency;
                jdata[1]   = spmc_parameters.rf_gen_fmscale;
                g_message ("RF-GEN Adjust SENDING MODE, RF-FREQ, FM-SCALE");
                rpspmc_pacpll->write_array (SPMC_RFGEN_COMPONENTS, 1, jdata_i,  2, jdata);
        }
}

int RPSPMC_Control::choice_mod_target_callback (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

	self->LCK_Target = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        
        for (int jj=1; modulation_targets[jj].label && jj < LCK_NUM_TARGETS; ++jj) //  ** omit last!
                gtk_widget_set_sensitive (self->LCK_VolumeEntry[jj], jj == self->LCK_Target);
        
        if (rpspmc_pacpll){
                g_message ("Setting LCK TARGET: %d", self->LCK_Target);
                rpspmc_pacpll->write_parameter ("SPMC_LCK_TARGET", self->LCK_Target);
                self->lockin_adjust_callback (NULL, self); // update correct volume
        }
        return 0;
}

int RPSPMC_Control::choice_BQfilter_type_callback (GtkWidget *widget, RPSPMC_Control *self){
	int id = gtk_combo_box_get_active (GTK_COMBO_BOX (widget)); // filter type/config mode PASS, IIR, BQIIR, AB, STOP

        int BQsec = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "section"))-1;

        switch (BQsec){
        case 0: spmc_parameters.sc_bq1mode = id;
                bq_filter_adjust_callback (NULL, self);
                break;
        case 1: spmc_parameters.sc_bq2mode = id;
                bq_filter_adjust_callback (NULL, self);
                break;
        case 2: spmc_parameters.sc_zs_bqmode = id;
                zs_input_filter_adjust_callback (NULL, self);
                break;
        }

        if (BQsec == 0 || BQsec == 1 || BQsec == 2){

                gchar *bqid[] = {"LCK-SEC1", "LCK-SEC2", "ZS-Input" };
                gchar *wids[3][3] = {{ "IIR1-TC", "BQ1-TC", "BQ1-Q" }, { "IIR2-TC", "BQ2-TC", "BQ2-Q" }, { "IIRZS-TC", "BQZS-TC", "BQZS-Q" }};
                switch (id){
                case 0:
                        g_message ("Set %s BQ to ONE, PASS", bqid[BQsec]);
                        break;
                case 1:
                        g_message ("BQ: N/A --- **IIR 1st");
                        break;
                case 2:
                        g_message ("BQ: N/A --- **BiQuad 2nd");
                        break;
                case 3:
                        g_message ("Set %s BiQuad SOS from AB-Coef", bqid[BQsec]);
                        break;
                case 4:
                        g_message ("Set %s BiQuad SOS to NULL (STOP)", bqid[BQsec]); break;
                        break;
                case 5:
                        g_message ("Set %s BiQuad SOS to By-Pass Mode", bqid[BQsec]); break;
                        break;
                case 6:
                        g_message ("Set %s BiQuad SOS to STOP", bqid[BQsec]);
                        break;
                }
        }
        
        return 0;
}


int RPSPMC_Control::choice_z_servo_current_source_callback (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

	int id = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

        switch (id){
        case 2: // AD3-FIR 512
                id=1;
                self->I_fir = 1;
                break;
        case 3: // AD3-FIRX 128
                id=1;
                self->I_fir = 2;
                break;
        default:
                self->I_fir = 0;
                break;
        }
        if (rpspmc_pacpll){
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_MODE", self->mix_transform_mode[0] | (self->I_fir << 8)); // mapped to MM_LIN/LOG/FCZLOG (0,1,3) | FIR SELECTION
                rpspmc_pacpll->write_parameter ("SPMC_Z_SERVO_SRC_MUX", id);
        }
        
        for (int k=0; rpspmc_source_signals[k].mask; ++k)
                if (rpspmc_source_signals[k].mask == 0x00000010){ // update as of 1V/5V "Current Input" Volt Scale Range Change from signal list
                        rpspmc_source_signals[k].scale_factor = z_servo_current_source[id].scale_factor; break;
                }

        // rpspmc_hwi->update_hardware_mapping_to_rpspmc_source_signals (); // not required as mapping to volts is done with the above

        return 0;
}

int RPSPMC_Control::choice_rf_gen_out_callback (GtkWidget *widget, RPSPMC_Control *self){
        PI_DEBUG_GP (DBG_L4, "%s \n",__FUNCTION__);

	int id = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        
        if (rpspmc_pacpll)
                rpspmc_pacpll->write_parameter ("SPMC_RF_GEN_OUT_MUX", id);
        
        return 0;
}


void RPSPMC_Control::on_new_data (){
        int s=(int)round(spmc_parameters.gvp_status);
        int Sgvp = (s>>8) & 0xf;  // assign GVP_status = { sec[32-4:0], setvec, reset, pause, ~finished };
        int Sspm = s & 0xff;      // assign SPM status = { ...SPM Z Servo Hold, GVP-FB-Hold, GVP-Finished, Z Servo EN }

        int fb=0;
        mon_FB =  (fb=((Sspm & 0x01) && !(Sspm & 0x04) && !(Sspm & 0x04) && !(Sspm & 0x08)) ? 1:0)
                + ((Sgvp & 0x01 ? 1:0) << 1);

        gchar *gvp_status = g_strdup_printf (" VPC: %d B: %d I:%d",
                                             current_probe_section,
                                             current_probe_block_index,
                                             current_probe_data_index);

        if (fb)
                gtk_widget_set_name (gvp_monitor_fb_label, "green");
        else
                gtk_widget_set_name (gvp_monitor_fb_label, "red");

        gvp_monitor_fb_info_ec->set_info(gvp_status);
        g_free (gvp_status);

        GVP_XYZ_mon_AA[0] = main_get_gapp()->xsm->Inst->Volt2XA (spmc_parameters.xs_monitor);
        GVP_XYZ_mon_AA[1] = main_get_gapp()->xsm->Inst->Volt2YA (spmc_parameters.ys_monitor);
        GVP_XYZ_mon_AA[2] = main_get_gapp()->xsm->Inst->Volt2ZA (spmc_parameters.z0_monitor);
                
        // RPSPM-Monitors: Lck, GVP
        if (G_IS_OBJECT (window))
                g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "SPMC_MONITORS_list"),
                                (GFunc) App::update_ec, NULL);

        update_zpos_readings();
}



void RPSPMC_Control::delayed_vector_update (){
        // SCAN SPEED COMPUTATIONS
        double slew[2];
        slew[0] = scan_speed_x = scan_speed_x_requested; // FIX ME -- recalc actual scan speed from vectors -> scan_speed_x!
        slew[1] = fast_return * scan_speed_x_requested;
        double scanpixelrate = main_get_gapp()->xsm->data.s.dx/slew[0];
        
        gchar *info = g_strdup_printf (" (%g ms/pix, %g Hz)", scanpixelrate*1e3, 1./scanpixelrate);
        scan_speed_ec->set_info (info);
        g_message ("Delayed Scan Speed Update: rewriting GVP Scan code for %s", info);
        g_free (info);

        if (rpspmc_hwi->is_scanning()) // only if scanning!
                write_spm_scan_vector_program (main_get_gapp()->xsm->data.s.rx, main_get_gapp()->xsm->data.s.ry,
                                               main_get_gapp()->xsm->data.s.nx, main_get_gapp()->xsm->data.s.ny,
                                               slew, NULL, NULL);
        delayed_vector_update_timer_id = 0; // done.

        if (rpspmc_hwi)
                rpspmc_hwi->add_user_event_now ("ScanSpeed_adjust", "[A/s] (speed,fast_ret_fac)", scan_speed_x, fast_return );
        
}

guint RPSPMC_Control::delayed_vector_update_callback (RPSPMC_Control *self){
        self->delayed_vector_update ();
	return FALSE;
}

void RPSPMC_Control::ChangedNotifyScanSpeed(Param_Control* pcs, RPSPMC_Control *self){
        if (!rpspmc_hwi->is_scanning()) // only if scanning!
                return;

        // if not already scheduled, schedule delayed scan speed vector update to avoid messageover load via many slider events
        if (!self->delayed_vector_update_timer_id)
                self->delayed_vector_update_timer_id = g_timeout_add (500, (GSourceFunc)RPSPMC_Control::delayed_vector_update_callback, self);

        //if (self->delayed_vector_update_timer_id){
        //        g_source_remove (self->delayed_vector_update_timer_id);
        //        self->delayed_vector_update_timer_id = 0;
        // }
}

void RPSPMC_Control::ChangedNotifyMoveSpeed(Param_Control* pcs, RPSPMC_Control *self){
        // obsolete, always done together with XY Offset adjustments
}


int RPSPMC_Control::check_vp_in_progress (const gchar *extra_info=NULL) {
        double a,b,c;
        rpspmc_hwi->RTQuery ("s", a,b,c);

        int g = (int)c & 1; // GVP
        int p = (int)c & 2; // PAUSE/STOPPED
        int r = (int)c & 4; // RESET
        
        g_message ("RPSPMC_Control::check_vp_in_progres ** GVP status: %02x == %s %s %s", (int)c, g? "GVP":"--", p? "PAUSE":"--", r? "RESET":"--");
        
        return (g && !p && !r) ? true : false;
        //return rpspmc_hwi->probe_fifo_thread_active>0 ? true:false;
} // GVP active?



/* **************************************** END SPM Control GUI **************************************** */

/* **************************************** PAC-PLL GUI **************************************** */

#define dB_min_from_Q(Q) (20.*log10(1./((1L<<(Q))-1)))
#define dB_max_from_Q(Q) (20.*log10(1L<<((32-(Q))-1)))
#define SETUP_dB_RANGE_from_Q(PCS, Q) { PCS->setMin(dB_min_from_Q(Q)); PCS->setMax(dB_max_from_Q(Q)); }

RPspmc_pacpll::RPspmc_pacpll (Gxsm4app *app):AppBase(app),RP_JSON_talk(){
        GtkWidget *tmp;
        GtkWidget *wid;
	
	GSList *EC_R_list=NULL;
	GSList *EC_QC_list=NULL;
	GSList *EC_FP_list=NULL;

        scan_gvp_options = 0;
        debug_level = 0;
        input_rpaddress = NULL;
        text_status = NULL;
        streaming = 0;

        resonator_frequency_fitted = -1.;
        resonator_phase_fitted = -1.;
        
        ch_freq = -1;
        ch_ampl = -1;
        deg_extend = 1;

        for (int i=0; i<5; ++i){
                scope_ac[i]=false;
                scope_dc_level[i]=0.;
                gain_scale[i] = 0.001; // 1000mV full scale
        }
        unwrap_phase_plot = true;
        
        
	PI_DEBUG (DBG_L2, "RPspmc_pacpll Plugin : building interface" );

	Unity    = new UnitObj(" "," ");
	Hz       = new UnitObj("Hz","Hz");
	Deg      = new UnitObj(UTF8_DEGREE,"deg");
	VoltDeg  = new UnitObj("V/" UTF8_DEGREE, "V/deg");
	Volt     = new UnitObj("V","V");
	mVolt     = new UnitObj("mV","mV");
	VoltHz   = new UnitObj("mV/Hz","mV/Hz");
	dB       = new UnitObj("dB","dB");
	Time     = new UnitObj("s","s");
	mTime    = new LinUnit("ms", "ms", "Time");
	uTime    = new LinUnit(UTF8_MU "s", "us", "Time");

        // Window Title
	AppWindowInit("RPSPMC PACPLL Control for RedPitaya");

        gchar *gs_path = g_strdup_printf ("%s.hwi.rpspmc-control", GXSM_RES_BASE_PATH_DOT);
        inet_json_settings = g_settings_new (gs_path);

        bp = new BuildParam (v_grid);
        bp->set_no_spin (true);

        bp->set_pcs_remote_prefix ("rp-pacpll-");

        bp->set_input_width_chars (16);
        bp->set_default_ec_change_notice_fkt (NULL, NULL);

        bp->new_grid_with_frame ("RedPitaya PAC Setup");
        bp->set_input_nx (2);
        bp->grid_add_ec ("In1 Offset", mVolt, &parameters.dc_offset, -1000.0, 1000.0, "g", 0.1, 1., "DC-OFFSET");
        EC_R_list = g_slist_prepend( EC_R_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();

        parameters.transport_tau[0] = -1.; // us, negative = disabled [bit 32 set in FPGA tau)
        parameters.transport_tau[1] = -1.; // us, negative = disabled [bit 32 set in FPGA tau)
        parameters.transport_tau[2] = -1.; // us, negative = disabled [bit 32 set in FPGA tau)
        parameters.transport_tau[3] = -1.; // us, negative = disabled [bit 32 set in FPGA tau)

        parameters.pac_dctau = 0.1; // ms
        parameters.pactau  = 50.0; // us
        parameters.pacatau = 30.0; // us
        parameters.frequency_manual = 32768.0; // Hz
        parameters.frequency_center = 32768.0; // Hz
        parameters.volume_manual = 300.0; // mV
        parameters.qc_gain=0.0; // gain +/-1.0
        parameters.qc_phase=0.0; // deg
        bp->set_input_width_chars (8);
        bp->set_input_nx (1);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::pac_tau_parameter_changed, this);
  	bp->grid_add_ec ("Tau DC", mTime, &parameters.pac_dctau, -1.0, 63e6, "6g", 10., 1., "PAC-DCTAU");
        bp->new_line ();
  	bp->grid_add_ec ("Tau PAC", uTime, &parameters.pactau, 0.0, 63e6, "6g", 10., 1., "PACTAU");
  	bp->grid_add_ec (NULL, uTime, &parameters.pacatau, 0.0, 63e6, "6g", 10., 1., "PACATAU");
        bp->new_line ();
        bp->set_no_spin (false);
        bp->set_input_nx (2);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::qc_parameter_changed, this);
  	bp->grid_add_ec ("QControl",Deg, &parameters.qc_phase, 0.0, 360.0, "5g", 10., 1., "QC-PHASE");
        EC_QC_list = g_slist_prepend( EC_QC_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();
  	bp->grid_add_ec ("QC Gain", Unity, &parameters.qc_gain, -1.0, 1.0, "4g", 10., 1., "QC-GAIN");
        EC_QC_list = g_slist_prepend( EC_QC_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();
        bp->set_no_spin (false);
        bp->set_input_width_chars (12);
        bp->set_input_nx (2);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::pac_frequency_parameter_changed, this);
  	bp->grid_add_ec ("Frequency", Hz, &parameters.frequency_manual, 0.0, 20e6, ".3lf", 0.1, 10., "FREQUENCY-MANUAL");
        EC_FP_list = g_slist_prepend( EC_FP_list, bp->ec);
        bp->new_line ();
        bp->set_input_width_chars (8);
        bp->set_input_nx (1);
  	bp->grid_add_ec ("Center", Hz, &parameters.frequency_center, 0.0, 20e6, ".3lf", 0.1, 10., "FREQUENCY-CENTER");
        EC_FP_list = g_slist_prepend( EC_FP_list, bp->ec);
  	bp->grid_add_button (N_("Copy"), "Copy F-center result from tune.", 1,
                             G_CALLBACK (RPspmc_pacpll::copy_f0_callback), this);
        GtkWidget *CpyButton = bp->button;

        bp->new_line ();
        bp->set_no_spin (false);
        bp->set_input_nx (2);
        bp->set_input_width_chars (12);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::pac_volume_parameter_changed, this);
  	bp->grid_add_ec ("Volume", mVolt, &parameters.volume_manual, 0.0, 1000.0, "5g", 0.1, 1.0, "VOLUME-MANUAL");
        bp->new_line ();
        bp->set_no_spin (true);
        bp->set_input_width_chars (10);
        parameters.tune_dfreq = 0.1;
        parameters.tune_span = 50.0;
        bp->set_input_nx (1);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::tune_parameter_changed, this);
  	bp->grid_add_ec ("Tune dF,Span", Hz, &parameters.tune_dfreq, 1e-4, 1e3, "g", 0.01, 0.1, "TUNE-DFREQ");
  	bp->grid_add_ec (NULL, Hz, &parameters.tune_span, 0.0, 1e6, "g", 0.1, 10., "TUNE-SPAN");

        bp->pop_grid ();
        bp->set_default_ec_change_notice_fkt (NULL, NULL);

        bp->new_grid_with_frame ("Amplitude Controller");
        bp->set_input_nx (3);
        bp->grid_add_ec ("Reading", mVolt, &parameters.volume_monitor, -1.0, 1.0, "g", 0.1, 1., "VOLUME-MONITOR");
        EC_R_list = g_slist_prepend( EC_R_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();
        parameters.amplitude_fb_setpoint = 12.0; // mV
        parameters.amplitude_fb_invert = 1.;
        parameters.amplitude_fb_cp_db = -25.;
        parameters.amplitude_fb_ci_db = -35.;
        parameters.exec_fb_upper = 500.0;
        parameters.exec_fb_lower = -500.0;
        bp->set_no_spin (false);
        bp->set_input_width_chars (10);

        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::amp_ctrl_parameter_changed, this);
        bp->grid_add_ec ("Setpoint", mVolt, &parameters.amplitude_fb_setpoint, 0.0, 1000.0, "5g", 0.1, 10.0, "AMPLITUDE-FB-SETPOINT");
        bp->new_line ();
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::amplitude_gain_changed, this);
        bp->grid_add_ec ("CP gain", dB, &parameters.amplitude_fb_cp_db, -200.0, 200.0, "g", 0.1, 1.0, "AMPLITUDE-FB-CP");
        SETUP_dB_RANGE_from_Q(bp->ec, 31); // Amplitude QAMCOEF = Q31
        bp->new_line ();
        bp->grid_add_ec ("CI gain", dB, &parameters.amplitude_fb_ci_db, -200.0, 200.0, "g", 0.1, 1.0, "AMPLITUDE-FB-CI");
        SETUP_dB_RANGE_from_Q(bp->ec, 31); // Amplitude QAMCOEF = Q31
        bp->new_line ();
        bp->set_no_spin (true);
        bp->set_input_width_chars (16);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::amp_ctrl_parameter_changed, this);
        bp->set_input_nx (1);
        bp->set_input_width_chars (10);
        bp->grid_add_ec ("Limits", mVolt, &parameters.exec_fb_lower, -1000.0, 1000.0, "g", 1.0, 10.0, "EXEC-FB-LOWER");
        bp->grid_add_ec ("...", mVolt, &parameters.exec_fb_upper, 0.0, 1000.0, "g", 1.0, 10.0, "EXEC-FB-UPPER");
        bp->new_line ();
        bp->set_input_width_chars (16);
        bp->set_input_nx (3);
        bp->set_default_ec_change_notice_fkt (NULL, NULL);
        bp->grid_add_ec ("Exec Amp", mVolt, &parameters.exec_amplitude_monitor, -1000.0, 1000.0, "g", 0.1, 1., "EXEC-AMPLITUDE-MONITOR");
        EC_R_list = g_slist_prepend( EC_R_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();
        bp->set_input_nx (1);
        bp->grid_add_check_button ( N_("Enable"), bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Enable Amplitude Controller","rp-pacpll-AMcontroller"), 2,
                                    G_CALLBACK (RPspmc_pacpll::amplitude_controller), this);
        bp->grid_add_check_button ( N_("Invert"), "Invert Amplitude Controller Gain. Normally positive.", 2,
                                    G_CALLBACK (RPspmc_pacpll::amplitude_controller_invert), this);

        bp->new_line ();
        bp->grid_add_check_button ( N_("Set Auto Trigger SS"), "Set Auto Trigger SS", 2,
                                    G_CALLBACK (RPspmc_pacpll::set_ss_auto_trigger), this);
        bp->grid_add_check_button ( N_("QControl"), "QControl", 2,
                                    G_CALLBACK (RPspmc_pacpll::qcontrol), this);
	g_object_set_data( G_OBJECT (bp->button), "QC_SETTINGS_list", EC_QC_list);
        bp->new_line ();
        bp->grid_add_check_button ( N_("Use LockIn Mode*"), "Use LockIn Mode.\n*: Must use PAC/LockIn FPGA bit code\n instead of Dual PAC FPGA bit code.", 2,
                                    G_CALLBACK (RPspmc_pacpll::select_pac_lck_amplitude), this);
        bp->new_line ();
        bp->grid_add_check_button ( N_("Pulse Control"), "Show Pulse Controller", 2,
                                    G_CALLBACK (RPspmc_pacpll::show_pulse_control), this);

        bp->pop_grid ();

        bp->new_grid_with_frame ("Phase Controller");
        bp->set_input_nx (3);
        bp->grid_add_ec ("Reading", Deg, &parameters.phase_monitor, -180.0, 180.0, "g", 1., 10., "PHASE-MONITOR");
        EC_R_list = g_slist_prepend( EC_R_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();
        parameters.phase_fb_setpoint = 60.;
        parameters.phase_fb_invert = 1.;
        parameters.phase_fb_cp_db = -123.5;
        parameters.phase_fb_ci_db = -184.;
        parameters.freq_fb_upper = 34000.;
        parameters.freq_fb_lower = 29000.;
        bp->set_no_spin (false);
        bp->set_input_width_chars (8);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::phase_ctrl_parameter_changed, this);
        bp->grid_add_ec ("Setpoint", Deg, &parameters.phase_fb_setpoint, -180.0, 180.0, "5g", 0.1, 1.0, "PHASE-FB-SETPOINT");
        bp->new_line ();
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::phase_gain_changed, this);
        bp->grid_add_ec ("CP gain", dB, &parameters.phase_fb_cp_db, -200.0, 200.0, "g", 0.1, 1.0, "PHASE-FB-CP");
        SETUP_dB_RANGE_from_Q(bp->ec, 31); // PHASE  QPHCOEF = Q31 
        bp->new_line ();
        bp->grid_add_ec ("CI gain", dB, &parameters.phase_fb_ci_db, -200.0, 200.0, "g", 0.1, 1.0, "PHASE-FB-CI");
        SETUP_dB_RANGE_from_Q(bp->ec, 31); // PHASE  QPHCOEF = Q31 
        bp->new_line ();
        bp->set_no_spin (true);
        bp->set_input_width_chars (16);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::phase_ctrl_parameter_changed, this);
        bp->set_input_width_chars (10);
        bp->set_input_nx (1);
        bp->grid_add_ec ("Limits", Hz, &parameters.freq_fb_lower, 0.0, 25e6, "g", 0.1, 1.0, "FREQ-FB-LOWER");
        bp->grid_add_ec ("...", Hz, &parameters.freq_fb_upper, 0.0, 25e6, "g", 0.1, 1.0, "FREQ-FB-UPPER");
        bp->new_line ();
        bp->set_input_width_chars (16);
        bp->set_input_nx (3);
        bp->set_default_ec_change_notice_fkt (NULL, NULL);
        bp->grid_add_ec ("DDS Freq", Hz, &parameters.dds_frequency_monitor, 0.0, 25e6, ".4lf", 0.1, 1., "DDS-FREQ-MONITOR");
        EC_R_list = g_slist_prepend( EC_R_list, bp->ec);
        input_ddsfreq=bp->ec;
        bp->ec->Freeze ();

        bp->new_line ();
        bp->grid_add_ec ("AM Lim", mVolt, &parameters.phase_hold_am_noise_limit, 0.0, 100.0, "g", 0.1, 1.0, "PHASE-HOLD-AM-NOISE-LIMIT");

        bp->new_line ();
        bp->set_input_nx (1);
        bp->grid_add_check_button ( N_("Enable"), bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Enable Phase Controller","rp-pacpll-PHcontroller"), 2,
                                    G_CALLBACK (RPspmc_pacpll::phase_controller), this);
        bp->grid_add_check_button ( N_("Invert"), "Invert Phase Controller Gain. Normally positive.", 2,
                                    G_CALLBACK (RPspmc_pacpll::phase_controller_invert), this);
        bp->new_line ();
        bp->grid_add_check_button ( N_("Unwrapping"), "Always unwrap phase/auto unwrap only if controller is enabled", 2,
                                    G_CALLBACK (RPspmc_pacpll::phase_unwrapping_always), this);
        //bp->grid_add_check_button ( N_("Unwap Plot"), "Unwrap plot at high level", 2,
        //                            G_CALLBACK (RPspmc_pacpll::phase_unwrap_plot), this);
        GtkWidget *cbrotab = gtk_combo_box_text_new ();
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbrotab), "0", "r0");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbrotab), "1", "r45");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbrotab), "2", "r-45");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbrotab), "3", "r90");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbrotab), "4", "r-90");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbrotab), "5", "r180");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cbrotab), "6", "r-180");
        g_signal_connect (G_OBJECT (cbrotab), "changed",
                          G_CALLBACK (RPspmc_pacpll::phase_rot_ab), 
                          this);				
        gtk_combo_box_set_active_id (GTK_COMBO_BOX (cbrotab), "0");
        bp->grid_add_widget (cbrotab);

        bp->new_line ();
        bp->grid_add_check_button ( N_("Use LockIn Mode"), "Use LockIn Mode", 2,
                                    G_CALLBACK (RPspmc_pacpll::select_pac_lck_phase), this);
        bp->grid_add_check_button ( N_("dFreq Control"), "Show delta Frequency Controller", 2,
                                    G_CALLBACK (RPspmc_pacpll::show_dF_control), this);

        bp->pop_grid ();

        // =======================================
        bp->new_grid_with_frame ("delta Frequency Controller");
        dF_control_frame = bp->frame;
        bp->set_input_nx (3);
        bp->grid_add_ec ("Reading", Hz, &parameters.dfreq_monitor, -200.0, 200.0, "g", 1., 10., "DFREQ-MONITOR");
        EC_R_list = g_slist_prepend( EC_R_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();
        parameters.dfreq_fb_setpoint = -3.0;
        parameters.dfreq_fb_invert = 1.;
        parameters.dfreq_fb_cp_db = -70.;
        parameters.dfreq_fb_ci_db = -120.;
        parameters.control_dfreq_fb_upper = 300.;
        parameters.control_dfreq_fb_lower = -300.;
        bp->set_no_spin (false);
        bp->set_input_width_chars (8);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::dfreq_ctrl_parameter_changed, this);
        bp->grid_add_ec ("Setpoint", Hz, &parameters.dfreq_fb_setpoint, -200.0, 200.0, "5g", 0.1, 1.0, "DFREQ-FB-SETPOINT");
        bp->new_line ();
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::dfreq_gain_changed, this);
        bp->grid_add_ec ("CP gain", dB, &parameters.dfreq_fb_cp_db, -200.0, 200.0, "g", 0.1, 1.0, "DFREQ-FB-CP");
        bp->new_line ();
        bp->grid_add_ec ("CI gain", dB, &parameters.dfreq_fb_ci_db, -200.0, 200.0, "g", 0.1, 1.0, "DFREQ-FB-CI");
        bp->new_line ();
        bp->set_no_spin (true);
        bp->set_input_width_chars (16);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::dfreq_ctrl_parameter_changed, this);
        bp->set_input_width_chars (10);
        bp->set_input_nx (1);
        bp->grid_add_ec ("Limits", mVolt, &parameters.control_dfreq_fb_lower, -10000.0, 10000.0, "g", 0.1, 1.0, "CONTROL-DFREQ-FB-LOWER");
        bp->grid_add_ec ("...", mVolt, &parameters.control_dfreq_fb_upper, -10000.0, 10000.0, "g", 0.1, 1.0, "CONTROL-DFREQ-FB-UPPER");
        bp->new_line ();
        bp->set_input_width_chars (16);
        bp->set_input_nx (3);
        bp->set_default_ec_change_notice_fkt (NULL, NULL);
        bp->grid_add_ec ("Control", mVolt, &parameters.control_dfreq_monitor, 0.0, 25e6, ".4lf", 0.1, 1., "DFREQ-CONTROL-MONITOR");
        EC_R_list = g_slist_prepend( EC_R_list, bp->ec);
        bp->ec->Freeze ();
        bp->new_line ();
        bp->set_input_nx (1);
        bp->grid_add_check_button ( N_("Enable"), bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Enable Dfreq Controller","rp-pacpll-DFcontroller"), 2,
                                    G_CALLBACK (RPspmc_pacpll::dfreq_controller), this);
        bp->grid_add_check_button ( N_("Invert"), "Invert Dfreq Controller Gain. Normally positive.", 2,
                                    G_CALLBACK (RPspmc_pacpll::dfreq_controller_invert), this);

        bp->new_line ();
        bp->grid_add_check_button ( N_("Control Z"), bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Engage Dfreq Controller on Z","rp-pacpll-ZDFcontrol"), 2,
                                    G_CALLBACK (RPspmc_pacpll::EnZdfreq_control), this);
        bp->new_line ();
        bp->grid_add_check_button ( N_("Control Bias"), bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Engage Dfreq Controller on Bias","rp-pacpll-UDFcontrol"), 2,
                                    G_CALLBACK (RPspmc_pacpll::EnUdfreq_control), this);

        // =======================================
        bp->pop_grid ();
        
        bp->new_grid_with_frame ("Pulse Former");
        pulse_control_frame = bp->frame;
        parameters.pulse_form_bias0 = 0;
        parameters.pulse_form_bias1 = 0;
        parameters.pulse_form_phase0 = 0;
        parameters.pulse_form_phase1 = 0;
        parameters.pulse_form_width0 = 0.1;
        parameters.pulse_form_width1 = 0.1;
        parameters.pulse_form_width0if = 0.5;
        parameters.pulse_form_width1if = 0.5;
        parameters.pulse_form_height0 = 200.;
        parameters.pulse_form_height1 = -200.;
        parameters.pulse_form_height0if = -40.;
        parameters.pulse_form_height1if = 40.;
        parameters.pulse_form_shapex = 1.;
        parameters.pulse_form_shapexif = 1.;
        parameters.pulse_form_shapexw = 0.;
        parameters.pulse_form_shapexwif = 0.;
        parameters.pulse_form_enable = false;
        bp->set_no_spin (false);
        bp->set_input_width_chars (6);
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::pulse_form_parameter_changed, this);
        GtkWidget *inputMB = bp->grid_add_ec ("Bias", mVolt, &parameters.pulse_form_bias0, 0.0, 180.0, "5g", 0.1, 1.0, "PULSE-FORM-BIAS0");
        GtkWidget *inputCB = bp->grid_add_ec (NULL,     mVolt, &parameters.pulse_form_bias1, 0.0, 180.0, "5g", 0.1, 1.0, "PULSE-FORM-BIAS1");
        g_object_set_data( G_OBJECT (inputMB), "HasClient", inputCB);
        bp->new_line ();
        GtkWidget *inputMP = bp->grid_add_ec ("Phase", Deg, &parameters.pulse_form_phase0, 0.0, 180.0, "5g", 1.0, 10.0, "PULSE-FORM-PHASE0");
        GtkWidget *inputCP = bp->grid_add_ec (NULL,      Deg, &parameters.pulse_form_phase1, 0.0, 180.0, "5g", 1.0, 10.0, "PULSE-FORM-PHASE1");
        g_object_set_data( G_OBJECT (inputMP), "HasClient", inputCP);

        bp->new_line ();
        GtkWidget *inputMW = bp->grid_add_ec ("Width", uTime, &parameters.pulse_form_width0, 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-WIDTH0");
        GtkWidget *inputCW = bp->grid_add_ec (NULL,      uTime, &parameters.pulse_form_width1, 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-WIDTH1");
        g_object_set_data( G_OBJECT (inputMW), "HasClient", inputCW);
        bp->new_line ();
        GtkWidget *inputMWI = bp->grid_add_ec ("WidthIF", uTime, &parameters.pulse_form_width0if, 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-WIDTH0IF");
        GtkWidget *inputCWI = bp->grid_add_ec (NULL,        uTime, &parameters.pulse_form_width1if, 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-WIDTH1IF");
        g_object_set_data( G_OBJECT (inputMWI), "HasClient", inputCWI);
        bp->new_line ();
        
        GtkWidget *inputMH = bp->grid_add_ec ("Height", mVolt, &parameters.pulse_form_height0, -1000.0, 1000.0, "5g", 1.0, 10.0, "PULSE-FORM-HEIGHT0");
        GtkWidget *inputCH = bp->grid_add_ec (NULL,       mVolt, &parameters.pulse_form_height1, -1000.0, 1000.0, "5g", 1.0, 10.0, "PULSE-FORM-HEIGTH1");
        bp->new_line ();
        GtkWidget *inputMHI = bp->grid_add_ec ("HeightIF", mVolt, &parameters.pulse_form_height0if, -1000.0, 1000.0, "5g", 1.0, 10.0, "PULSE-FORM-HEIGHT0IF");
        GtkWidget *inputCHI = bp->grid_add_ec (NULL,         mVolt, &parameters.pulse_form_height1if, -1000.0, 1000.0, "5g", 1.0, 10.0, "PULSE-FORM-HEIGTH1IF");
        bp->new_line ();
        
        GtkWidget *inputX  = bp->grid_add_ec ("Shape", Unity, &parameters.pulse_form_shapex, -10.0, 10.0, "4g", 0.1, 1.0, "PULSE-FORM-SHAPEX");
        GtkWidget *inputXI = bp->grid_add_ec (NULL,    Unity, &parameters.pulse_form_shapexif, -10.0, 10.0, "4g", 0.1, 1.0, "PULSE-FORM-SHAPEXIF");
        g_object_set_data( G_OBJECT (inputX), "HasClient", inputXI);
        bp->new_line ();
        GtkWidget *inputXW  = bp->grid_add_ec ("ShapeW", Unity, &parameters.pulse_form_shapexw, 0.0, 1.0, "4g", 0.1, 1.0, "PULSE-FORM-SHAPEXW");
        GtkWidget *inputXWI = bp->grid_add_ec (NULL,    Unity, &parameters.pulse_form_shapexwif, 0.0, 1.0, "4g", 0.1, 1.0, "PULSE-FORM-SHAPEXWIF");
        g_object_set_data( G_OBJECT (inputXW), "HasClient", inputXWI);

#if 1
        bp->new_line ();
        GtkWidget *inputDPt0 = bp->grid_add_ec ("DPt0v0", uTime, &parameters.pulse_form_dpt[0], 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-DPT0");
        GtkWidget *inputDPv0 = bp->grid_add_ec (NULL,     Unity, &parameters.pulse_form_dpv[0], 0.0, 3.0, "5g", 1.0, 1.0, "PULSE-FORM-DPV0");
        bp->new_line ();
        GtkWidget *inputDPt1 = bp->grid_add_ec ("DPt1v1", uTime, &parameters.pulse_form_dpt[1], 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-DPT1");
        GtkWidget *inputDPv1 = bp->grid_add_ec (NULL,     Unity, &parameters.pulse_form_dpv[1], 0.0, 3.0, "5g", 1.0, 1.0, "PULSE-FORM-DPV1");
        bp->new_line ();
        GtkWidget *inputDPt2 = bp->grid_add_ec ("DPt2v2", uTime, &parameters.pulse_form_dpt[2], 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-DPT2");
        GtkWidget *inputDPv2 = bp->grid_add_ec (NULL,     Unity, &parameters.pulse_form_dpv[2], 0.0, 3.0, "5g", 1.0, 1.0, "PULSE-FORM-DPV2");
        bp->new_line ();
        GtkWidget *inputDPt3 = bp->grid_add_ec ("DPt3v3", uTime, &parameters.pulse_form_dpt[3], 0.0, 10000.0, "5g", 0.1, 1.0, "PULSE-FORM-DPT3");
        GtkWidget *inputDPv3 = bp->grid_add_ec (NULL,     Unity, &parameters.pulse_form_dpv[3], 0.0, 3.0, "5g", 1.0, 1.0, "PULSE-FORM-DPV3");
#endif
        
        bp->new_line ();
        bp->set_input_nx (1);
        bp->grid_add_check_button ( N_("Enable"),  bp->PYREMOTE_CHECK_HOOK_KEY_FUNC("Enable Pulse Forming","rp-pacpll-PF"), 1,
                                    G_CALLBACK (RPspmc_pacpll::pulse_form_enable), this);

        bp->grid_add_exec_button ( N_("Single Shot"),
                                   G_CALLBACK (RPspmc_pacpll::pulse_form_fire), this, "FirePulse",
                                   1);

        GtkWidget *pf_ts = gtk_combo_box_text_new (); // Pulse Form Trigger Source
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (pf_ts), "0", "QP");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (pf_ts), "1", "LCK");
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (pf_ts), "2", "GVP");
        g_signal_connect (G_OBJECT (pf_ts), "changed",
                          G_CALLBACK (RPspmc_pacpll::pulse_form_pf_ts), 
                          this);				
        gtk_combo_box_set_active_id (GTK_COMBO_BOX (pf_ts), "0");
        bp->grid_add_widget (pf_ts);
        

        // =======================================

        bp->pop_grid ();
        bp->new_line ();

        // ========================================
        
        bp->new_grid_with_frame ("Oscilloscope and Data Transfer Control", 10);
        // OPERATION MODE
	wid = gtk_combo_box_text_new ();

        bp->set_input_width_chars (8);
        bp->set_input_nx (1);
        /*
        bp->set_default_ec_change_notice_fkt (RPspmc_pacpll::pac_tau_transport_changed, this);
  	bp->grid_add_ec ("LP dFreq,Phase,Exec,Ampl", mTime, &parameters.transport_tau[0], -1.0, 63e6, "6g", 10., 1., "PAC-TRANSPORT-TAU-DFREQ");
  	bp->grid_add_ec (NULL, mTime, &parameters.transport_tau[1], -1.0, 63e6, "6g", 10., 1., "PAC-TRANSPORT-TAU-PHASE");
  	bp->grid_add_ec (NULL, mTime, &parameters.transport_tau[2], -1.0, 63e6, "6g", 10., 1., "PAC-TRANSPORT-TAU-EXEC");
  	bp->grid_add_ec (NULL, mTime, &parameters.transport_tau[3], -1.0, 63e6, "6g", 10., 1., "PAC-TRANSPORT-TAU-AMPL");
        bp->new_line ();
        */
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_operation_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

        const gchar *operation_modes[] = {
                "MANUAL",
                "MEASURE DC_OFFSET",
                "RUN SCOPE",
                "INIT BRAM TRANSPORT", // 3
                "SINGLE SHOT",
                "STREAMING OPERATION", // 5 "START BRAM LOOP",
                "RUN TUNE",
                "RUN TUNE F",
                "RUN TUNE FF",
                "RUN TUNE RS",
                NULL };

        // Init choicelist
	for(int i=0; operation_modes[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), operation_modes[i], operation_modes[i]);

        update_op_widget = wid;
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), operation_mode=5); // "START BRAM LOOP" mode need to run for data decimation and transfer analog + McBSP

        // FPGA Update Period
	wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_update_ts_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);
        update_ts_widget = wid;

        /* for(i=0; i< 30; i=i+2) {printf ("\" %2d/%.2fms\"\n",i,1024*(1<<i)*2/125e6*1e3);}
 for(i=0; i<=30; i=i+1) {printf ("\" %2d/%.2fms\"\n",i,1024*(1<<i)*2/125e6*1e3);}
"  0/~16.38us"
"  1/~32.77us"
"  2/~65.54us"
"  3/~131.07us"
"  4/~262.14us"
"  5/~524.29us"
"  6/~1048.58us"
"  6/~1.05ms"
"  7/~2.10ms"
"  8/~4.19ms"
"  9/~8.39ms"
" 10/~16.78ms"
" 11/~33.55ms"
" 12/~67.11ms"
" 13/~134.22ms"
" 14/~268.44ms"
" 15/~536.87ms"
" 16/~1073.74ms"
" 16/~1.07s"
" 17/~2.15s"
" 18/~4.29s"
" 19/~8.59s"
" 20/~17.18s"
" 21/~34.36s"
" 22/~68.72s"
" 23/~137.44s"
" 24/~274.88s"
" 25/~549.76s"
" 26/~1099.51s"
" 27/~2199.02s"
" 28/~4398.05s"
" 29/~8796.09s"

        */
        
	const gchar *update_ts_list[] = {
                "* 1/ 32.8us  ( 32ns)",
                "  2/ 65.5us  ( 64ns)",
                "  3/131us    (128ns)",
                "  4/262.14us (256ns)",
                "  5/524.29us (512ns)",
                "  6/  1.05ms ( 1.024us)",
                "  7/  2.10ms ( 2.048us)",
                "  8/  4.19ms ( 4.096us)",
                "  9/  8.39ms ( 8.192us)",
                " 10/ 16.78ms (16.4us",
                " 11/ 33.55ms (32.8us)",
                " 12/ 67.11ms (65.5us)",
                " 13/134.22ms (131us)",
                " 14/268.44ms (262us)",
                " 15/536.87ms (524us)",
                " 16/  1.07s  ( 1.05ms)",
                " 17/  2.15s  ( 2.10ms)",
                " 18/  4.29s  ( 4.19ms)",
                " 19/  8.59s  ( 8.39ms)",
                " 20/ 17.18s  (16.8ms)",
                " 21/ 34.36s  (33.6ms)",
                " 22/ 68.72s  (67.1ms)",
                "DEC/SCOPE-LEN(BOXCARR)",
                NULL };
   
	// Init choicelist
	for(int i=0; update_ts_list[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), update_ts_list[i], update_ts_list[i]);

	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 18); // select 19

        // Scope Trigger Options
	wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_trigger_mode_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

	const gchar *trigger_modes[] = {
                "Trigger: None",
                "Auto CH1 Pos",
                "Auto CH1 Neg",
                "Auto CH2 Pos",
                "Auto CH2 Neg",
                "Logic B0 H",
                "Logic B0 L",
                "Logic B1 H",
                "Logic B1 L",
                "Logic B2 H",
                "Logic B2 L",
                "Logic B3 H",
                "Logic B3 L",
                "Logic B4 H",
                "Logic B4 L",
                "Logic B5 H",
                "Logic B5 L",
                "Logic B6 H",
                "Logic B6 L",
                "Logic B7 H",
                "Logic B7 L",
                NULL };
   
	// Init choicelist
	for(int i=0; trigger_modes[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), trigger_modes[i], trigger_modes[i]);

	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 1);
                
        // Scope Auto Set Modes
	wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_auto_set_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

	const gchar *auto_set_modes[] = {
                "Auto Set CH1",
                "Auto Set CH2",
                "Auto Set CH3",
                "Auto Set CH4",
                "Auto Set CH5",
                "Default All=1000mV",
                "Default All=100mV",
                "Default All=10mV",
                "Manual",
                NULL };
   
	// Init choicelist
	for(int i=0; auto_set_modes[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), auto_set_modes[i], auto_set_modes[i]);

	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 6);
        
        bp->new_line ();

        // BRAM TRANSPORT MODE BLOCK S1,S2
	wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_transport_ch12_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

	const gchar *transport_modes[] = {
                "OFF: no plot",
                "IN1, IN2",             // [0] SCOPE
                "IN1: AC, DC",          // [1] MON
                "AMC: Ampl, Exec",      // [2] AMC Adjust
                "PHC: dPhase, dFreq",   // [3] PHC Adjust
                "Phase, Ampl",          // [4] TUNE
                "Phase, dFreq,[Am,Ex]", // [5] SCAN
                "DHC: dFreq, dFControl", // [6] DFC Adjust
                "DDR IN1/IN2",          // [7] SCOPE with DEC=1,SHR=0 Double(max) Data Rate config
                "AXIS7/8",             // [8]
                NULL };
   
	// Init choicelist
	for(int i=0; transport_modes[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), transport_modes[i], transport_modes[i]);

        channel_selections[5] = 0;
        channel_selections[6] = 0;
        channel_selections[0] = 1;
        channel_selections[1] = 1;
        transport=5; // 5: Phase, dFreq,[Ampl,Exec] (default for streaming operation, scanning, etc.)
        update_tr_widget = wid;
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), transport+1); // normal operation for PLL, transfer: Phase, Freq,[Am,Ex] (analog: Freq, McBSP: 4ch transfer Ph, Frq, Am, Ex)

// *** PAC_PLL::GPIO MONITORS ***                                                                    readings are via void *thread_gpio_reading_FIR(g), read_gpio_reg_int32 (n,m)
// *** DBG ***                                                                                                //        gpio_reading_FIRV_vector[GPIO_READING_LMS_A]        (1,0); // GPIO X1 : LMS A
// *** DBG ***                                                                                                //        gpio_reading_FIRV_vector[GPIO_READING_LMS_A]        (1,1); // GPIO X2 : LMS B
// *** DBG ***                                                                                                //        -----------------------                             (2,0); // GPIO X3 : DBG M
//oubleParameter VOLUME_MONITOR("VOLUME_MONITOR", CBaseParameter::RW, 0, 0, -1000.0, 1000.0);                 // mV  ** gpio_reading_FIRV_vector[GPIO_READING_AMPL]         (2,1); // GPIO X4 : CORDIC SQRT (AM2=A^2+B^2)
// via  DC_OFFSET rp_PAC_auto_dc_offset_correct ()                                                                                                                          (3,0); // GPIO X5 : DC_OFFSET (M-DC)
// *** DBG ***                                                                                                //        -----------------------                             (3,1); // GPIO X6 : --- SPMC STATUS [FB, GVP, AD5791, --]
//oubleParameter EXEC_MONITOR("EXEC_MONITOR", CBaseParameter::RW, 0, 0, -1000.0, 1000.0);                     //  mV ** gpio_reading_FIRV_vector[GPIO_READING_EXEC]         (4,0); // GPIO X7 : Exec Ampl Control Signal (signed)
//oubleParameter DDS_FREQ_MONITOR("DDS_FREQ_MONITOR", CBaseParameter::RW, 0, 0, 0.0, 25e6);                   //  Hz ** gpio_reading_FIRV_vector[GPIO_READING_DDS_FREQ]     (4,1); // GPIO X8 : DDS Phase Inc (Freq.) upper 32 bits of 44 (unsigned)
//                                                                                                              //                                                            (5,0); // GPIO X9 : DDS Phase Inc (Freq.) lower 32 bits of 44 (unsigned)
//oubleParameter PHASE_MONITOR("PHASE_MONITOR", CBaseParameter::RW, 0, 0, -180.0, 180.0);                     // deg ** gpio_reading_FIRV_vector[GPIO_READING_PHASE]        (5,1); // GPIO X10: CORDIC ATAN(X/Y)
//oubleParameter DFREQ_MONITOR("DFREQ_MONITOR", CBaseParameter::RW, 0, 0, -1000.0, 1000.0);                   // Hz  ** gpio_reading_FIRV_vector[GPIO_READING_DDS_FREQ]     (6,0); // GPIO X11: dFreq
// *** DBG ***                                                                                                //        -----------------------                             (6,1); // GPIO X12: ---
//oubleParameter CONTROL_DFREQ_MONITOR("CONTROL_DFREQ_MONITOR", CBaseParameter::RW, 0, 0, -10000.0, 10000.0); // mV  **  gpio_re..._FIRV_vector[GPIO_READING_CONTROL_DFREQ] (7,0); // GPIO X13: control dFreq value
// *** DBG ***                                                                                                //        -----------------------                             (7,1); // GPIO X14: --- SIGNAL PASS [IN2] (Current, FB SRC)
// *** DBG ***                                                                                                //        -----------------------                             (8,0); // GPIO X15: --- UMON
// *** DBG ***                                                                                                //        -----------------------                             (8,1); // GPIO X16: --- XMON
// *** DBG ***                                                                                                //        -----------------------                             (9,0); // GPIO X17: --- YMON
// *** DBG ***                                                                                                //        -----------------------                             (9,1); // GPIO X18: --- ZMON
// *** DBG ***                                                                                                //        -----------------------                            (10,0); // GPIO X19: ---
// *** DBG ***                                                                                                //        -----------------------                            (10,1); // GPIO X20: ---
        
        // GPIO monitor selections -- full set, experimental
	const gchar *monitor_modes_gpio[] = {
                "OFF: no plot",
                "X1 LMS A",
                "X2 LMS B",
                "X3 DBG M",
                "X4 SQRT AM2",
                "X5 DC OFFSET",
                "X6 SPMC STATUS FB,GVP, AD",
                "X7 EXEC AM",
                "X8 DDS PH INC upper",
                "X9 DDS PH INC lower",
                "X10 ATAN(X/Y)",
                "X11 dFreq",
                "X12 DBG --",
                "X13 Ctrl dFreq",
                "X14 Signal Pass IN2",
                "X15 UMON",
                "X16 XMON",
                "X17 YMON",
                "X18 ZMON",
                "X19 ===",
                "X20 ===",
                NULL };

        // CH3 from GPIO MONITOR</p>
	wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_transport_ch3_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

	// Init choicelist
	for(int i=0; monitor_modes_gpio[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), monitor_modes_gpio[i], monitor_modes_gpio[i]);

        channel_selections[2] = 0;
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 0);

        // CH4 from GPIO MONITOR</p>
	wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_transport_ch4_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

	// Init choicelist
	for(int i=0; monitor_modes_gpio[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), monitor_modes_gpio[i], monitor_modes_gpio[i]);

        channel_selections[3] = 0;
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 0);

        // CH5 from GPIO MONITOR</p>
	wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::choice_transport_ch5_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

	// Init choicelist
	for(int i=0; monitor_modes_gpio[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid), monitor_modes_gpio[i], monitor_modes_gpio[i]);

        channel_selections[4] = 0;
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 0);


        // Scope Display
        bp->new_line ();

	GtkWidget* scrollarea = gtk_scrolled_window_new ();
        gtk_widget_set_hexpand (scrollarea, TRUE);
        gtk_widget_set_vexpand (scrollarea, TRUE);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollarea),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        bp->grid_add_widget (scrollarea, 10);
        signal_graph_area = gtk_drawing_area_new ();
        
        gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (signal_graph_area), 128);
        gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (signal_graph_area), 4);
        gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (signal_graph_area), graph_draw_function, this, NULL);
        //        bp->grid_add_widget (signal_graph_area, 10);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrollarea), signal_graph_area);
        
        bp->new_line ();

        // Scope Cotnrols
        bp->grid_add_check_button ( N_("Ch1: AC"), "Remove Offset from Ch1", 1,
                                    G_CALLBACK (RPspmc_pacpll::scope_ac_ch1_callback), this);
        wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::scope_z_ch1_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);
        const gchar *zoom_levels[] = {
                                      "0.001",
                                      "0.003",
                                      "0.01",
                                      "0.03",
                                      "0.1",
                                      "0.3",
                                      "1",
                                      "3",
                                      "10",
                                      "30",
                                      "100",
                                      "300",
                                      "1000",
                                      "3000",
                                      "10k",
                                      "30k",
                                      "100k",
                                      NULL };
	for(int i=0; zoom_levels[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid),  zoom_levels[i], zoom_levels[i]);
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 6);


        bp->grid_add_check_button ( N_("Ch2: AC"), "Remove Offset from Ch2", 1,
                                    G_CALLBACK (RPspmc_pacpll::scope_ac_ch2_callback), this);

        wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::scope_z_ch2_callback),
                          this);
        bp->grid_add_widget (wid);
        gtk_widget_show (wid);

	for(int i=0; zoom_levels[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid),  zoom_levels[i], zoom_levels[i]);
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 3);


        bp->grid_add_check_button ( N_("Ch3 AC"), "Remove Offset from Ch3", 1,
                                    G_CALLBACK (RPspmc_pacpll::scope_ac_ch3_callback), this);
        bp->grid_add_check_button ( N_("XY"), "display XY plot for In1, In2", 1,
                                    G_CALLBACK (RPspmc_pacpll::scope_xy_on_callback), this);
        bp->grid_add_check_button ( N_("FFT"), "display FFT plot for In1, In2", 1,
                                    G_CALLBACK (RPspmc_pacpll::scope_fft_on_callback), this);
        
        wid = gtk_combo_box_text_new ();
        g_signal_connect (G_OBJECT (wid), "changed",
                          G_CALLBACK (RPspmc_pacpll::scope_fft_time_zoom_callback),
                          this);
        bp->grid_add_widget (wid);
        const gchar *time_zoom_levels[] = {
                                      "1",
                                      "2",
                                      "5",
                                      "10",
                                      "20",
                                      "50",
                                      "100",
                                      NULL };
	for(int i=0; time_zoom_levels[i]; i++)
                gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (wid),  time_zoom_levels[i], time_zoom_levels[i]);
	gtk_combo_box_set_active (GTK_COMBO_BOX (wid), 0);

        bp->new_line ();

        bp->grid_add_exec_button ( N_("Save"), G_CALLBACK (RPspmc_pacpll::scope_save_data_callback), this, "ScopeSaveData");

        bram_shift = 0;
        bp->grid_add_widget (gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, 4096, 10), 10);
        g_signal_connect (G_OBJECT (bp->any_widget), "value-changed",
                          G_CALLBACK (RPspmc_pacpll::scope_buffer_position_callback),
                          this);

        bp->new_line ();
        run_scope=0;
        bp->grid_add_check_button ( N_("Scope"), "Enable Scope", 1,
                                    G_CALLBACK (RPspmc_pacpll::enable_scope), this);
        scope_width_points  = 512.;
        scope_height_points = 256.;
        bp->set_input_width_chars (5);
  	bp->grid_add_ec ("SW", Unity, &scope_width_points, 256.0, 4096.0, ".0f", 128, 256., "SCOPE-WIDTH");
        bp->set_input_width_chars (4);
  	bp->grid_add_ec ("SH", Unity, &scope_height_points, 128.0, 1024.0, ".0f", 128, 256., "SCOPE-HEIGHT");

        gtk_widget_hide (dF_control_frame);
        gtk_widget_hide (pulse_control_frame);

        // ========================================
        
        bp->pop_grid ();
        bp->new_line ();

        // save List away...
	//g_object_set_data( G_OBJECT (window), "RPSPMCONTROL_EC_list", EC_list);

	g_object_set_data( G_OBJECT (window), "RPSPMCONTROL_EC_READINGS_list", EC_R_list);
	g_object_set_data( G_OBJECT (CpyButton), "PAC_FP_list", EC_FP_list);

        set_window_geometry ("rpspmc-pacpll-control"); // needs rescoure entry and defines window menu entry as geometry is managed

	rpspmc_pacpll_hwi_pi.app->RemoteEntryList = g_slist_concat (rpspmc_pacpll_hwi_pi.app->RemoteEntryList, bp->remote_list_ec);

        
        // hookup to scan start and stop
        rpspmc_pacpll_hwi_pi.app->ConnectPluginToStartScanEvent (RPspmc_pacpll::scan_start_callback);
        rpspmc_pacpll_hwi_pi.app->ConnectPluginToStopScanEvent (RPspmc_pacpll::scan_stop_callback);

        
        
}

RPspmc_pacpll::~RPspmc_pacpll (){

        update_shm_monitors (0,1); // close SHM

        delete mTime;
	delete uTime;
	delete Time;
	delete dB;
	delete VoltHz;
	delete Volt;
	delete mVolt;
	delete VoltDeg;
	delete Deg;
	delete Hz;
	delete Unity;
}

void RPspmc_pacpll::connect_cb (GtkWidget *widget, RPspmc_pacpll *self){
        // get connect type request (restart or reconnect button was clicked)
        self->reconnect_mode = GPOINTER_TO_INT (g_object_get_data( G_OBJECT (widget), "RESTART"));
        // connect (checked) or dissconnect    
        self->json_talk_connect_cb (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)), self->reconnect_mode);
}


void RPspmc_pacpll::scan_start_callback (gpointer user_data){
        g_message ("RPspmc_pacpll::scan_start_callback");

        if (rpspmc_hwi->is_scanning()){
                g_message ("RPspmc_pacpll::scan_start_callback ** RPSPMC scan in progress -- STOP FIRST TO RE-START SCAN.");
                return -1;
        }
        if (RPSPMC_ControlClass->check_vp_in_progress ()){
                g_message ("RPspmc_pacpll::scan_start_callback ** RPSPMC is streaming GVP data -- ABORT/STOP FIRST TO START SCAN.");
                return -1;
        }

        rpspmc_pacpll->ch_freq = -1;
        rpspmc_pacpll->ch_ampl = -1;
        rpspmc_pacpll->streaming = 1;


        rpspmc_hwi->set_spmc_signal_mux (RPSPMC_ControlClass->scan_source);
        if (RPSPMC_ControlClass)
                rpspmc_pacpll->set_stream_mux (RPSPMC_ControlClass->scan_source);

}

void RPspmc_pacpll::scan_stop_callback (gpointer user_data){
         if (! rpspmc_hwi->is_scanning()){
                g_message ("RPspmc_pacpll::scan_stopt_callback ** RPSPMC is no scanning.");
                return -1;
        }

        rpspmc_pacpll->ch_freq = -1;
        rpspmc_pacpll->ch_ampl = -1;
        rpspmc_pacpll->streaming = 0;

        g_message ("RPspmc_pacpll::scan_stop_callback");
}

int RPspmc_pacpll::setup_scan (int ch, 
				 const gchar *titleprefix, 
				 const gchar *name,
				 const gchar *unit,
				 const gchar *label,
				 double d2u
	){

        // extra setup -- not needed
	return 0;
}

void RPspmc_pacpll::pac_tau_transport_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("TRANSPORT_TAU_DFREQ", self->parameters.transport_tau[0]); // negative = disabled
        self->write_parameter ("TRANSPORT_TAU_PHASE", self->parameters.transport_tau[1]);
        self->write_parameter ("TRANSPORT_TAU_EXEC",  self->parameters.transport_tau[2]);
        self->write_parameter ("TRANSPORT_TAU_AMPL",  self->parameters.transport_tau[3]);
}

void RPspmc_pacpll::pac_tau_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("PAC_DCTAU", self->parameters.pac_dctau);
        self->write_parameter ("PACTAU", self->parameters.pactau);
        self->write_parameter ("PACATAU", self->parameters.pacatau);
}

void RPspmc_pacpll::pac_frequency_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("FREQUENCY_MANUAL", self->parameters.frequency_manual, "%.4f"); //, TRUE);
        self->write_parameter ("FREQUENCY_CENTER", self->parameters.frequency_center, "%.4f"); //, TRUE);
}

void RPspmc_pacpll::pac_volume_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("VOLUME_MANUAL", self->parameters.volume_manual);
}

static void freeze_ec(Gtk_EntryControl* ec, gpointer data){ ec->Freeze (); };
static void thaw_ec(Gtk_EntryControl* ec, gpointer data){ ec->Thaw (); };

void RPspmc_pacpll::show_dF_control (GtkWidget *widget, RPspmc_pacpll *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                gtk_widget_show (self->dF_control_frame);
        else
                gtk_widget_hide (self->dF_control_frame);
}

void RPspmc_pacpll::show_pulse_control (GtkWidget *widget, RPspmc_pacpll *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                gtk_widget_show (self->pulse_control_frame);
        else
                gtk_widget_hide (self->pulse_control_frame);
}

void RPspmc_pacpll::select_pac_lck_amplitude (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("LCK_AMPLITUDE", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
}
void RPspmc_pacpll::select_pac_lck_phase (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("LCK_PHASE", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
}

void RPspmc_pacpll::qcontrol (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("QCONTROL", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.qcontrol = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));

        if (self->parameters.qcontrol)
                g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "QC_SETTINGS_list"),
				(GFunc) thaw_ec, NULL);
        else
                g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "QC_SETTINGS_list"),
				(GFunc) freeze_ec, NULL);
}

void RPspmc_pacpll::qc_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("QC_GAIN", self->parameters.qc_gain);
        self->write_parameter ("QC_PHASE", self->parameters.qc_phase);
}

void RPspmc_pacpll::tune_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("TUNE_DFREQ", self->parameters.tune_dfreq);
        self->write_parameter ("TUNE_SPAN", self->parameters.tune_span);
}

void RPspmc_pacpll::amp_ctrl_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("AMPLITUDE_FB_SETPOINT", self->parameters.amplitude_fb_setpoint);
        self->write_parameter ("EXEC_FB_UPPER", self->parameters.exec_fb_upper);
        self->write_parameter ("EXEC_FB_LOWER", self->parameters.exec_fb_lower);
}

void RPspmc_pacpll::phase_ctrl_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("PHASE_FB_SETPOINT", self->parameters.phase_fb_setpoint);
        self->write_parameter ("FREQ_FB_UPPER", self->parameters.freq_fb_upper);
        self->write_parameter ("FREQ_FB_LOWER", self->parameters.freq_fb_lower);
        self->write_parameter ("PHASE_HOLD_AM_NOISE_LIMIT", self->parameters.phase_hold_am_noise_limit);
}

void RPspmc_pacpll::dfreq_ctrl_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("DFREQ_FB_SETPOINT", self->parameters.dfreq_fb_setpoint);
        self->write_parameter ("DFREQ_FB_UPPER", self->parameters.control_dfreq_fb_upper);
        self->write_parameter ("DFREQ_FB_LOWER", self->parameters.control_dfreq_fb_lower);
}

void RPspmc_pacpll::pulse_form_parameter_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->write_parameter ("PULSE_FORM_BIAS0", self->parameters.pulse_form_bias0);
        self->write_parameter ("PULSE_FORM_BIAS1", self->parameters.pulse_form_bias1);
        self->write_parameter ("PULSE_FORM_PHASE0", self->parameters.pulse_form_phase0);
        self->write_parameter ("PULSE_FORM_PHASE1", self->parameters.pulse_form_phase1);
        self->write_parameter ("PULSE_FORM_WIDTH0", self->parameters.pulse_form_width0);
        self->write_parameter ("PULSE_FORM_WIDTH1", self->parameters.pulse_form_width1);
        self->write_parameter ("PULSE_FORM_WIDTH0IF", self->parameters.pulse_form_width0if);
        self->write_parameter ("PULSE_FORM_WIDTH1IF", self->parameters.pulse_form_width1if);
        self->write_parameter ("PULSE_FORM_SHAPEXW", self->parameters.pulse_form_shapexw);
        self->write_parameter ("PULSE_FORM_SHAPEXWIF", self->parameters.pulse_form_shapexwif);
        self->write_parameter ("PULSE_FORM_SHAPEX", self->parameters.pulse_form_shapex);
        self->write_parameter ("PULSE_FORM_SHAPEXIF", self->parameters.pulse_form_shapexif);

        self->write_parameter ("PULSE_FORM_DPOS0", self->parameters.pulse_form_dpt[0]);
        self->write_parameter ("PULSE_FORM_DPOS1", self->parameters.pulse_form_dpt[1]);
        self->write_parameter ("PULSE_FORM_DPOS2", self->parameters.pulse_form_dpt[2]);
        self->write_parameter ("PULSE_FORM_DPOS3", self->parameters.pulse_form_dpt[3]);
        
        if (self->parameters.pulse_form_enable){
                self->write_parameter ("PULSE_FORM_HEIGHT0", self->parameters.pulse_form_height0);
                self->write_parameter ("PULSE_FORM_HEIGHT1", self->parameters.pulse_form_height1);
                self->write_parameter ("PULSE_FORM_HEIGHT0IF", self->parameters.pulse_form_height0if);
                self->write_parameter ("PULSE_FORM_HEIGHT1IF", self->parameters.pulse_form_height1if);
                self->write_parameter ("PULSE_FORM_DPVAL0", self->parameters.pulse_form_dpv[0]);
                self->write_parameter ("PULSE_FORM_DPVAL1", self->parameters.pulse_form_dpv[1]);
                self->write_parameter ("PULSE_FORM_DPVAL2", self->parameters.pulse_form_dpv[2]);
                self->write_parameter ("PULSE_FORM_DPVAL3", self->parameters.pulse_form_dpv[3]);
        } else {
                self->write_parameter ("PULSE_FORM_HEIGHT0", 0.0);
                self->write_parameter ("PULSE_FORM_HEIGHT1", 0.0);
                self->write_parameter ("PULSE_FORM_HEIGHT0IF", 0.0);
                self->write_parameter ("PULSE_FORM_HEIGHT1IF", 0.0);
                self->write_parameter ("PULSE_FORM_DPVAL0", 0.0);
                self->write_parameter ("PULSE_FORM_DPVAL1", 0.0);
                self->write_parameter ("PULSE_FORM_DPVAL2", 0.0);
                self->write_parameter ("PULSE_FORM_DPVAL3", 0.0);
        }
}

void RPspmc_pacpll::pulse_form_enable (GtkWidget *widget, RPspmc_pacpll *self){
        self->parameters.pulse_form_enable = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
        self->pulse_form_parameter_changed (NULL, self);
}

void RPspmc_pacpll::pulse_form_fire (GtkWidget *widget, RPspmc_pacpll *self){
        gtk_combo_box_set_active (GTK_COMBO_BOX (self->update_op_widget), 3); // RESET (INIT BRAM TRANSPORT AND CLEAR FIR RING BUFFERS)
        while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, false);
        usleep(300000);
        gtk_combo_box_set_active (GTK_COMBO_BOX (self->update_op_widget), 4); // SINGLE SHOT
}

void RPspmc_pacpll::pulse_form_pf_ts (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("PULSE_FORM_TRIGGER_SELECT", gtk_combo_box_get_active (GTK_COMBO_BOX (widget)));
}

void RPspmc_pacpll::save_scope_data (){
        static int count=0;
        static int base_count=-1;
	std::ofstream f;
        int i;
	const gchar *separator = "\t";
	time_t t;
	time(&t);

        if (base_count != gapp->xsm->file_counter) // auto reset for new image
                count=0;
                
        base_count = gapp->xsm->file_counter;
        gchar *fntmp = g_strdup_printf ("%s/%s%03d-%s%04d.rpdata",
					g_settings_get_string (gapp->get_as_settings (), "auto-save-folder-probe"), 
					gapp->xsm->data.ui.basename, base_count, "RP", count++);
        
	f.open (fntmp);

        int ix=-999999, iy=-999999;

	if (gapp->xsm->MasterScan){
		gapp->xsm->MasterScan->World2Pixel (gapp->xsm->data.s.x0, gapp->xsm->data.s.y0, ix, iy, SCAN_COORD_ABSOLUTE);
	}

	const gchar *transport_modes[] = {
                "IN1, IN2",             // [0] SCOPE
                "IN1: AC, DC",          // [1] MON
                "AMC: Ampl, Exec",      // [2] AMC Adjust
                "PHC: dPhase, dFreq",   // [3] PHC Adjust
                "Phase, Ampl",          // [4] TUNE
                "Phase, dFreq,[Am,Ex]", // [5] SCAN
                "DHC: dFreq, dFControl", // [6] DFC Adjust
                "DDR IN1/IN2",          // [7] SCOPE with DEC=1,SHR=0 Double(max) Data Rate config
                "DEBUG McBSP",          // [8]
                NULL };

        
        // self->bram_window_length = 1024.*(double)decimation*1./125e6; // sec
        int decimation = 1 << data_shr_max;
        double tw1024 = bram_window_length;

        f.precision (12);

	f << "# view via: xmgrace -graph 0 -pexec 'title \"GXSM RP Data: " << fntmp << "\"' -block " << fntmp  << " -bxy 2:4 ..." << std::endl;
	f << "# GXSM RP Data :: RPVersion=00.01 vdate=20241114" << std::endl;
	f << "# Date                   :: date=" << ctime(&t) << "#" << std::endl;
	f << "# FileName               :: name=" << fntmp << std::endl;
	f << "# GXSM-Main-Offset       :: X0=" << gapp->xsm->data.s.x0 << " Ang" <<  "  Y0=" << gapp->xsm->data.s.y0 << " Ang" 
	  << ", iX0=" << ix << " Pix iX0=" << iy << " Pix"
	  << std::endl;

        f << "#C RP SAMPLING and DECIMATION SETTINGS:" << std::endl;
        f << "#C BRAM transfer window length is 1024 points, duration is tw=" << tw1024 << " s" << std::endl;
        f << "#C BRAM data length is 4096 points=" << (4*tw1024) << " s" << std::endl;
        f << "#C                    SHR_DEC_DATA=" << data_shr_max << std::endl;
        f << "#C            TRANSPORT_DECIMATION=" << decimation << std::endl;
        f << "#C         BRAM_SCOPE_TRIGGER_MODE=" << trigger_mode << std::endl;
        f << "#C SET_SINGLESHOT_TRGGER_POST_TIME=" << trigger_post_time << " us" << std::endl;
        f << "#C              BRAM_WINDOW_LENGTH=" << (1e3*bram_window_length)       << " ms" << std::endl;
        f << "#C                        BRAM_DEC=" << decimation << std::endl;
        f << "#C                   BRAM_DATA_SHR=" << data_shr_max << std::endl;
	f << "#C       CH1 CH2 TRANSPORT MAPPING=" << transport << " CH1,CH2 <= " << transport_modes[transport] << std::endl;
	f << "#C Data CH1, CH2 is raw data as of RP subsystem. For In1/In2: voltage in mV" << std::endl;
	f << "#C " << std::endl;

        double *signal[] = { pacpll_signals.signal_ch1, pacpll_signals.signal_ch2, pacpll_signals.signal_ch3, pacpll_signals.signal_ch4, pacpll_signals.signal_ch5, // 0...4 CH1..5
                             pacpll_signals.signal_phase, pacpll_signals.signal_ampl  }; // 5,6 PHASE, AMPL in Tune Mode, averaged from burst

        int uwait=600000;
        
        tw1024 /= 1024.; // per px in sec
        tw1024 *= 1e6;   // effective sample intervall in us between points
        f << "#C index time[us] " << transport_modes[transport] << std::endl;
        for (int k=0; k<4; ++k){
                write_parameter ("BRAM_SCOPE_SHIFT_POINTS", k*1024);
                usleep(uwait); // wait for data to update
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, false);

                for (i=0; i<1024; ++i){
                        bram_saved_buffer[0][i+k*1024] = ((i+k*1024)*tw1024);                   // time; buffer for later access via pyremote
                        bram_saved_buffer[1][i+k*1024] = pacpll_signals.signal_ch1[(i+1)%1024]; // CH1; buffer for later access via pyremote
                        bram_saved_buffer[2][i+k*1024] = pacpll_signals.signal_ch2[(i+1)%1024]; // CH2; buffer for later access via pyremote
                        f << (i+k*1024) << " \t"
                          << ((i+k*1024)*tw1024) << " \t"
                          << pacpll_signals.signal_ch1[(i+1)%1024] << " \t"
                          << pacpll_signals.signal_ch2[i] << "\n";
                }
        }
        f.close();

        write_parameter ("BRAM_SCOPE_SHIFT_POINTS", 0);
        
}

void RPspmc_pacpll::save_values (NcFile &ncf){
        // store all rpspmc_pacpll's control parameters for the RP PAC-PLL
        // if socket connection is up
        if (client){
                const gchar* pll_prefix = "rpspmc_hwi";
                // JSON READ BACK PARAMETERS! Loosing precison to float some where!! (PHASE_CI < -100dB => 0!!!)
                for (JSON_parameter *p=PACPLL_JSON_parameters; p->js_varname; ++p){
                        gchar *vn = g_strdup_printf ("%s_JSON_%s", pll_prefix, p->js_varname);
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", p->js_varname);
                        ncv.putAtt ("var_unit", p->unit_id);
                        ncv.putVar (p->value);
                }
                // ACTUAL PARAMETERS in "dB" DOUBLE PRECISION VALUES, getting OK to the FPGA...
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_PHASE_FB_CP");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_PHASE_FB_CP");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.phase_fb_cp); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_PHASE_FB_CI");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_PHASE_FB_CI");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.phase_fb_ci); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_AMPLITUDE_FB_CP");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_AMPLITUDE_FB_CP");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.amplitude_fb_cp); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_AMPLITUDE_FB_CI");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_AMPLITUDE_FB_CI");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.amplitude_fb_ci); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_PHASE_FB_CP_dB");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_PHASE_FB_CP_dB");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.phase_fb_cp_db); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_PHASE_FB_CI_dB");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_PHASE_FB_CI_dB");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.phase_fb_ci_db); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_AMPLITUDE_FB_CP_dB");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_AMPLITUDE_FB_CP_dB");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.amplitude_fb_cp_db); // OK!
                }
                {
                        gchar *vn = g_strdup_printf ("%s_%s", pll_prefix, "_AMPLITUDE_FB_CI_dB");
                        NcVar ncv = ncf.addVar ( vn, ncDouble);
                        ncv.putAtt ("long_name", vn);
                        ncv.putAtt ("short_name", "_AMPLITUDE_FB_CI_dB");
                        ncv.putAtt ("var_unit", "dB");
                        ncv.putVar (&parameters.amplitude_fb_ci_db); // OK!
                }
        }
}

void RPspmc_pacpll::send_all_parameters (){
        write_parameter ("GAIN1", 1.);
        write_parameter ("GAIN2", 1.);
        write_parameter ("GAIN3", 1.);
        write_parameter ("GAIN4", 1.);
        write_parameter ("GAIN5", 1.);
        write_parameter ("SHR_DEC_DATA", 4.);
        write_parameter ("SCOPE_TRIGGER_MODE", 1);
        write_parameter ("PACVERBOSE", 0);
        write_parameter ("TRANSPORT_DECIMATION", 19);
        write_parameter ("TRANSPORT_MODE", transport);
        write_parameter ("OPERATION", operation_mode);
        write_parameter ("BRAM_SCOPE_SHIFT_POINTS", bram_shift = 0);
        pac_tau_parameter_changed (NULL, this);
        pac_frequency_parameter_changed (NULL, this);
        pac_volume_parameter_changed (NULL, this);
        qc_parameter_changed (NULL, this);
        tune_parameter_changed (NULL, this);
        amp_ctrl_parameter_changed (NULL, this);
        amplitude_gain_changed (NULL, this);
        phase_ctrl_parameter_changed (NULL, this);
        phase_gain_changed (NULL, this);
        dfreq_ctrl_parameter_changed (NULL, this);
        dfreq_gain_changed (NULL, this);
}

// AT COLD START AND RECONNECT
void RPspmc_pacpll::update_SPMC_parameters (){
        RPSPMC_ControlClass->Init_SPMC_on_connect ();
}


// ONLY at COLD START
void RPspmc_pacpll::SPMC_cold_start_init (){
        RPSPMC_ControlClass->Init_SPMC_after_cold_start ();
}


void RPspmc_pacpll::choice_operation_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->operation_mode = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->write_parameter ("OPERATION", self->operation_mode);
}

void RPspmc_pacpll::choice_update_ts_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->data_shr_max = 0;
        self->bram_window_length = 1.;

        int ts = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (ts < 11){
                self->write_parameter ("SIGNAL_PERIOD", 100);
                self->write_parameter ("PARAMETER_PERIOD",  100);
        } else {
                self->write_parameter ("SIGNAL_PERIOD", 200);
                self->write_parameter ("PARAMETER_PERIOD",  200);
        }
        
        self->data_shr_max = ts;

        int decimation = 1 << self->data_shr_max;
        self->write_parameter ("SHR_DEC_DATA", self->data_shr_max);
        self->write_parameter ("TRANSPORT_DECIMATION", decimation);

        self->bram_window_length = 1024.*(double)decimation*1./125e6; // sec
        //g_print ("SET_SINGLESHOT_TRGGER_POST_TIME = %g ms\n", 0.1 * 1e3*self->bram_window_length);
        //g_print ("BRAM_WINDOW_LENGTH ............ = %g ms\n", 1e3*self->bram_window_length);
        //g_print ("BRAM_DEC ...................... = %d\n", decimation);
        //g_print ("BRAM_DATA_SHR ................. = %d\n", self->data_shr_max);
        self->write_parameter ("SET_SINGLESHOT_TRIGGER_POST_TIME", 0.1 * 1e6*self->bram_window_length); // is us --  10% pre trigger
}

void RPspmc_pacpll::choice_transport_ch12_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->channel_selections[0] = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->channel_selections[1] = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) > 0){
                int i=gtk_combo_box_get_active (GTK_COMBO_BOX (widget))-1;
                if (i<0) i=0;
                self->write_parameter ("TRANSPORT_MODE", self->transport=i);
        }
}

void RPspmc_pacpll::choice_trigger_mode_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("BRAM_SCOPE_TRIGGER_MODE", self->trigger_mode = gtk_combo_box_get_active (GTK_COMBO_BOX (widget)));
        self->write_parameter ("BRAM_SCOPE_TRIGGER_POS", (int)(0.1*1024));
}

void RPspmc_pacpll::choice_auto_set_callback (GtkWidget *widget, RPspmc_pacpll *self){
        int m=gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (m>=0 && m<5)
                self->gain_scale[m] = -1.; // recalculate
        else {
                m -= 5;
                double s[] = { 1.0, 10., 100. };
                if (m < 3) 
                        for (int i=0; i<5; ++i)
                                self->gain_scale[i] = s[m]*1e-3; // Fixed
        }
}

void RPspmc_pacpll::scope_buffer_position_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("BRAM_SCOPE_SHIFT_POINTS", self->bram_shift = (int)gtk_range_get_value (GTK_RANGE (widget)));
}

void RPspmc_pacpll::scope_save_data_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->save_scope_data ();
}


void RPspmc_pacpll::choice_transport_ch3_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->channel_selections[2] = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) > 0)
                self->write_parameter ("TRANSPORT_CH3", gtk_combo_box_get_active (GTK_COMBO_BOX (widget))-1);
}

void RPspmc_pacpll::choice_transport_ch4_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->channel_selections[3] = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) > 0)
                self->write_parameter ("TRANSPORT_CH4", gtk_combo_box_get_active (GTK_COMBO_BOX (widget))-1);
}

void RPspmc_pacpll::choice_transport_ch5_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->channel_selections[4] = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        if (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) > 0)
                self->write_parameter ("TRANSPORT_CH5", gtk_combo_box_get_active (GTK_COMBO_BOX (widget))-1);
}

void RPspmc_pacpll::set_ss_auto_trigger (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("SET_SINGLESHOT_TRANSPORT_TRIGGER", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
}

void RPspmc_pacpll::amplitude_gain_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->parameters.amplitude_fb_cp = self->parameters.amplitude_fb_invert * pow (10., self->parameters.amplitude_fb_cp_db/20.);
        self->parameters.amplitude_fb_ci = self->parameters.amplitude_fb_invert * pow (10., self->parameters.amplitude_fb_ci_db/20.);
        self->write_parameter ("AMPLITUDE_FB_CP", self->parameters.amplitude_fb_cp);
        self->write_parameter ("AMPLITUDE_FB_CI", self->parameters.amplitude_fb_ci);
}

void RPspmc_pacpll::amplitude_controller_invert (GtkWidget *widget, RPspmc_pacpll *self){
        self->parameters.amplitude_fb_invert = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)) ? -1.:1.;
        self->amplitude_gain_changed (NULL, self);
}

void RPspmc_pacpll::amplitude_controller (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("AMPLITUDE_CONTROLLER", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.amplitude_controller = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::phase_gain_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->parameters.phase_fb_cp = self->parameters.phase_fb_invert * pow (10., self->parameters.phase_fb_cp_db/20.);
        self->parameters.phase_fb_ci = self->parameters.phase_fb_invert * pow (10., self->parameters.phase_fb_ci_db/20.);
        // g_message("PH_CICP=%g, %g", self->parameters.phase_fb_ci, self->parameters.phase_fb_cp );
        self->write_parameter ("PHASE_FB_CP", self->parameters.phase_fb_cp);
        self->write_parameter ("PHASE_FB_CI", self->parameters.phase_fb_ci);
}

void RPspmc_pacpll::phase_controller_invert (GtkWidget *widget, RPspmc_pacpll *self){
        self->parameters.phase_fb_invert = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)) ? -1.:1.;
        self->phase_gain_changed (NULL, self);
}

void RPspmc_pacpll::phase_controller (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("PHASE_CONTROLLER", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.phase_controller = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::phase_unwrapping_always (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("PHASE_UNWRAPPING_ALWAYS", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.phase_unwrapping_always = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::phase_rot_ab (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("PAC_ROT_AB", gtk_combo_box_get_active (GTK_COMBO_BOX (widget)));
}

void RPspmc_pacpll::phase_unwrap_plot (GtkWidget *widget, RPspmc_pacpll *self){
        self->unwrap_phase_plot = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}


void RPspmc_pacpll::dfreq_gain_changed (Param_Control* pcs, gpointer user_data){
        RPspmc_pacpll *self = (RPspmc_pacpll *)user_data;
        self->parameters.dfreq_fb_cp = self->parameters.dfreq_fb_invert * pow (10., self->parameters.dfreq_fb_cp_db/20.);
        self->parameters.dfreq_fb_ci = self->parameters.dfreq_fb_invert * pow (10., self->parameters.dfreq_fb_ci_db/20.);
        // g_message("DF_CICP=%g, %g", self->parameters.dfreq_fb_ci, self->parameters.dfreq_fb_ci );
        self->write_parameter ("DFREQ_FB_CP", self->parameters.dfreq_fb_cp);
        self->write_parameter ("DFREQ_FB_CI", self->parameters.dfreq_fb_ci);
}

void RPspmc_pacpll::dfreq_controller_invert (GtkWidget *widget, RPspmc_pacpll *self){
        self->parameters.dfreq_fb_invert = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)) ? -1.:1.;
        self->dfreq_gain_changed (NULL, self);
}

void RPspmc_pacpll::dfreq_controller (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("DFREQ_CONTROLLER", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.dfreq_controller = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::EnZdfreq_control (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("DFREQ_CONTROL_Z", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.Zdfreq_control = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::EnUdfreq_control (GtkWidget *widget, RPspmc_pacpll *self){
        self->write_parameter ("DFREQ_CONTROL_U", gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)));
        self->parameters.Udfreq_control = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::update(){
	if (G_IS_OBJECT (window))
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "RPSPMCONTROL_EC_list"),
				(GFunc) App::update_ec, NULL);
}

void RPspmc_pacpll::update_monitoring_parameters(){

        // RPSPMC-PACPLL
        
        // mirror global parameters to private
        parameters.dc_offset = pacpll_parameters.dc_offset;
        parameters.exec_amplitude_monitor = pacpll_parameters.exec_amplitude_monitor;
        parameters.dds_frequency_monitor = pacpll_parameters.dds_frequency_monitor;
        parameters.volume_monitor = pacpll_parameters.volume_monitor;
        parameters.phase_monitor = pacpll_parameters.phase_monitor;
        parameters.control_dfreq_monitor = pacpll_parameters.control_dfreq_monitor;
        parameters.dfreq_monitor = pacpll_parameters.dfreq_monitor;
        
        parameters.cpu_load = pacpll_parameters.cpu_load;
        parameters.free_ram = pacpll_parameters.free_ram;
        parameters.counter = pacpll_parameters.counter;

        pacpll_parameters.dds_dfreq_computed = parameters.dds_frequency_monitor - parameters.frequency_center;
        gchar *delta_freq_info = g_strdup_printf ("[%g]", pacpll_parameters.dds_dfreq_computed);

        
        input_ddsfreq->set_info (delta_freq_info);
        g_free (delta_freq_info);
        if (G_IS_OBJECT (window))
		g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (window), "RPSPMCONTROL_EC_READINGS_list"),
				(GFunc) App::update_ec, NULL);

}

void RPspmc_pacpll::copy_f0_callback (GtkWidget *widget, RPspmc_pacpll *self){
        if (self->resonator_frequency_fitted > 0){
                self->parameters.frequency_center = self->parameters.frequency_manual = self->resonator_frequency_fitted;
                self->parameters.phase_fb_setpoint = self->resonator_phase_fitted;
                g_slist_foreach((GSList*)g_object_get_data( G_OBJECT (widget), "PAC_FP_list"),
				(GFunc) App::update_ec, NULL);
        }
}

void RPspmc_pacpll::enable_scope (GtkWidget *widget, RPspmc_pacpll *self){
        self->run_scope = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::dbg_l1 (GtkWidget *widget, RPspmc_pacpll *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                self->debug_level |= 1;
        else
                self->debug_level &= ~1;
}
void RPspmc_pacpll::dbg_l2 (GtkWidget *widget, RPspmc_pacpll *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                self->debug_level |= 2;
        else
                self->debug_level &= ~2;
}
void RPspmc_pacpll::dbg_l4 (GtkWidget *widget, RPspmc_pacpll *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                self->debug_level |= 4;
        else
                self->debug_level &= ~4;
}

void RPspmc_pacpll::scan_gvp_opt6 (GtkWidget *widget, RPspmc_pacpll *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                self->scan_gvp_options |= (1<<6);
        else
                self->scan_gvp_options &= ~(1<<6);
}

void RPspmc_pacpll::scan_gvp_opt7 (GtkWidget *widget, RPspmc_pacpll *self){
        if (gtk_check_button_get_active (GTK_CHECK_BUTTON (widget)))
                self->scan_gvp_options |= (1<<7);
        else
                self->scan_gvp_options &= ~(1<<7);
}


void RPspmc_pacpll::scope_ac_ch1_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->scope_ac[0] = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}
void RPspmc_pacpll::scope_ac_ch2_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->scope_ac[1] = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}
void RPspmc_pacpll::scope_ac_ch3_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->scope_ac[2] = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::scope_xy_on_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->scope_xy_on = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}

void RPspmc_pacpll::scope_fft_on_callback (GtkWidget *widget, RPspmc_pacpll *self){
        self->scope_fft_on = gtk_check_button_get_active (GTK_CHECK_BUTTON (widget));
}
void RPspmc_pacpll::scope_z_ch1_callback (GtkWidget *widget, RPspmc_pacpll *self){
        double z[] = { 0.001, 0.003, 0.01, 0.03, 0.1, 0.3, 1.0, 3.0, 10., 30.0, 100., 300.0, 1000., 3000.0, 10e3, 30e3, 100e3, 1., 0., 0. };
        int i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->scope_z[0] = z[i];
}
void RPspmc_pacpll::scope_z_ch2_callback (GtkWidget *widget, RPspmc_pacpll *self){
        double z[] = { 0.001, 0.003, 0.01, 0.03, 0.1, 0.3, 1.0, 3.0, 10., 30.0, 100., 300.0, 1000., 3000.0, 10e3, 30e3, 100e3, 1., 0., 0. };
        int i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->scope_z[1] = z[i];
}
void RPspmc_pacpll::scope_fft_time_zoom_callback (GtkWidget *widget, RPspmc_pacpll *self){
        double z[] = { 1.0, 2.0, 5.0, 10.0, 20.0, 50.0, 100., 1., 0., 0. };
        int i = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
        self->scope_fft_time_zoom = z[i];
}



void RPspmc_pacpll::update_health (const gchar *msg){

        static GDateTime *now = NULL;
        static double t0_utc;
        static double t0 = -1.;
        static double t0_avg = 0.;
        static int init_count = 2500;
        double deviation=0.;
        int valid=0;
        static double dev_med1k=0.;
        static gsl_rstat_workspace *rstat_p = gsl_rstat_alloc();

        /*
        gsl_rstat_add(data[i], rstat_p);
        mean = gsl_rstat_mean(rstat_p);
        variance = gsl_rstat_variance(rstat_p);
        largest  = gsl_rstat_max(rstat_p);
        smallest = gsl_rstat_min(rstat_p);
        median   = gsl_rstat_median(rstat_p);
        sd       = gsl_rstat_sd(rstat_p);
        sd_mean  = gsl_rstat_sd_mean(rstat_p);
        skew     = gsl_rstat_skew(rstat_p);
        rms      = gsl_rstat_rms(rstat_p);
        kurtosis = gsl_rstat_kurtosis(rstat_p);
        n        = gsl_rstat_n(rstat_p);
        gsl_rstat_reset(rstat_p);
        n = gsl_rstat_n(rstat_p);
        gsl_rstat_free(rstat_p);
        */
        
        if (msg){
                gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY((red_pitaya_health)))), msg, -1);
                g_slist_foreach ((GSList*)g_object_get_data (G_OBJECT (window), "EC_FPGA_SPMC_server_settings_list"), (GFunc) App::update_ec, NULL); // UPDATE GUI!
                gtk_widget_set_name (GTK_WIDGET (red_pitaya_health), "entry-mono-text-msg");
        } else {
                double sec;
                double sec_dec = modf(spmc_parameters.uptime_seconds, &sec);
                int S = (int)sec;
                int tmp=3600*24;
                int d = S/tmp;
                int h = (tmp=(S-d*tmp))/3600;
                int m = (tmp=(tmp-h*3600))/60;
                double s = (tmp-m*60) + sec_dec;

                if (t0 < 0.){
                        GDateTime *now = g_date_time_new_now_utc();
                        t0_avg = spmc_parameters.uptime_seconds; // init to first reading
                        if (t0_avg > 10.)
                                t0 = t0_avg; // accept
                        t0_utc = g_date_time_to_unix(now) + (double)g_date_time_get_microsecond(now) * 1e-6;
                        g_date_time_unref(now);
                        g_message ("t0_avg: %g,  t0_utc: %g", t0_avg, t0_utc);
                } else {
                        if (init_count){
                                GDateTime *now = g_date_time_new_now_utc();
                                double utc = g_date_time_to_unix(now) + (double)g_date_time_get_microsecond(now) * 1e-6;
                                g_date_time_unref(now);
                                double estimate = spmc_parameters.uptime_seconds - (utc - t0_utc);
                                t0_avg = 0.99 * t0_avg + 0.01 * estimate;

                                gsl_rstat_add(estimate, rstat_p);
#if 0
                                g_message ("[%d] t0_avg: %g,  d_utc: %g   gsl mean: %g med: %g rms: %g  var: %g  skew: %g #%d",
                                           init_count, t0_avg, utc-t0_utc,
                                           gsl_rstat_mean(rstat_p),
                                           gsl_rstat_median(rstat_p),
                                           gsl_rstat_rms(rstat_p),
                                           gsl_rstat_variance(rstat_p),
                                           gsl_rstat_skew(rstat_p),
                                           gsl_rstat_n(rstat_p));
#endif
                                --init_count;
                                if (init_count == 0){
                                        t0_avg = gsl_rstat_median(rstat_p);
                                        gsl_rstat_reset(rstat_p);
                                }
                        }
                }

                
                //gsl_rstat_n(rstat_p));

                
                // Get the current date and time with microsecond precision.
                GDateTime *now = g_date_time_new_now_utc();
                double utc = g_date_time_to_unix(now) + (double)g_date_time_get_microsecond(now) * 1e-6;
                g_date_time_unref(now);
                double time_since_t0 = utc - t0_utc;
                double deviation = spmc_parameters.uptime_seconds - t0_avg - time_since_t0;

                if (init_count == 0){
                        if (gsl_rstat_n(rstat_p) > 3000){
                                dev_med1k=gsl_rstat_median(rstat_p); valid++;
                                gsl_rstat_reset(rstat_p);
                                gsl_rstat_add (dev_med1k, rstat_p);
                        }
                
                        gsl_rstat_add (deviation, rstat_p);
#if 0
                        g_message ("[%d] %g ** mean: %g med: %g rms: %g  var: %g  skew: %g",
                                   gsl_rstat_n(rstat_p), deviation,
                                   gsl_rstat_mean(rstat_p),
                                   gsl_rstat_median(rstat_p),
                                   gsl_rstat_rms(rstat_p),
                                   gsl_rstat_variance(rstat_p),
                                   gsl_rstat_skew(rstat_p));
#endif
                }
                
                // Extract the timestamp as a 64-bit integer representing microseconds since the epoch.
                //gint64 timestamp_us = g_date_time_to_unix(now) * 1000000 + g_date_time_get_microsecond(now);
                //gint64 t0_us = (gint64)round(t0*1e6); // sec to us
                        
                // Print the timestamp.
                //g_print("Timestamp (microseconds since epoch): %" G_GINT64_FORMAT "\n", timestamp_us);
                        
                // Format the time as a string.
                //gchar *formatted_time = g_date_time_format(now, "%Y-%m-%d %H:%M:%S.%f %z");
                //g_print("Formatted time: %s\n", formatted_time);
                        
                // Clean up.
                //g_free(formatted_time);
                //g_date_time_unref(now);


#if 0
                gchar *gpiox_string = NULL;
                if (scope_width_points > 1000){
                        gpiox_string = g_strdup_printf ("1[%08x %08x, %08x %08x] 5[%08x %08x, %08x %08x] 9[%08x %08x, %08x %08x] 13[%08x %08x, %08x %08x]",
                                                        (int)pacpll_signals.signal_gpiox[0], (int)pacpll_signals.signal_gpiox[1],
                                                        (int)pacpll_signals.signal_gpiox[2], (int)pacpll_signals.signal_gpiox[3],
                                                        (int)pacpll_signals.signal_gpiox[4], (int)pacpll_signals.signal_gpiox[5],
                                                        (int)pacpll_signals.signal_gpiox[8], (int)pacpll_signals.signal_gpiox[9],
                                                        (int)pacpll_signals.signal_gpiox[10], (int)pacpll_signals.signal_gpiox[11],
                                                        (int)pacpll_signals.signal_gpiox[12], (int)pacpll_signals.signal_gpiox[13],
                                                        (int)pacpll_signals.signal_gpiox[14], (int)pacpll_signals.signal_gpiox[15]
                                                        );
                }
                gchar *health_string = g_strdup_printf ("CPU: %03.0f%% Free: %6.1f MB %s #%06.1f Up:%d d %02d:%02d:%04.1f FPGA clock tics: %.2f",
                                                        pacpll_parameters.cpu_load,
                                                        pacpll_parameters.free_ram/1024/1024,
                                                        gpiox_string?gpiox_string:"[]", pacpll_parameters.counter,
                                                        d,h,m,s, spmc_parameters.uptime_seconds
                                                        );
                g_free (gpiox_string);
#else
                gchar *health_string = g_strdup_printf ("CPU: %03.0f%% Free: %6.1f MB #%06.1f Up:%d d %02d:%02d:%04.1f   %s: %g ppm  [%.4f s as of %d]",
                                                        pacpll_parameters.cpu_load,
                                                        pacpll_parameters.free_ram/1024/1024,
                                                        pacpll_parameters.counter,
                                                        d,h,m,s, init_count ? "estimating precise t0":"delta UTC", 1e6*dev_med1k/time_since_t0,
                                                        valid ? dev_med1k : gsl_rstat_median(rstat_p), valid ? 1000 : gsl_rstat_n(rstat_p)
                                                        );
                if (valid){
                        main_get_gapp()->monitorcontrol->LogEvent ("RPSPMC-HS-UpTime-PPM", health_string, 2);
                        valid = 0;
                }
#endif
                gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (gtk_entry_get_buffer (GTK_ENTRY((red_pitaya_health)))), health_string, -1);
                gtk_widget_set_name (GTK_WIDGET (red_pitaya_health), "entry-mono-text-health");
                g_free (health_string);
        }
}

void RPspmc_pacpll::status_append (const gchar *msg){
	GtkTextBuffer *console_buf;
	GString *output;
	GtkTextMark *end_mark;
        GtkTextIter iter, start_iter, end_trim_iter, end_iter;
        gint lines, max_lines=400;

	if (msg == NULL) {
                //status_append ("** Zero Data/Message **");
                /* Change default font and color throughout the widget */
                GtkCssProvider *provider = gtk_css_provider_new ();
                gtk_css_provider_load_from_data (provider,
                                                 "textview {"
                                                 " font: 12px monospace;"
                                                 // "  color: green;"
                                                 "}",
                                                 -1);
                GtkStyleContext *context = gtk_widget_get_style_context (GTK_WIDGET (text_status));
                gtk_style_context_add_provider (context,
                                                GTK_STYLE_PROVIDER (provider),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
                

                // clear text buffer
                // read string which contain last command output
                console_buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW(text_status));
                gtk_text_buffer_get_bounds (console_buf, &start_iter, &end_iter);
                
                gtk_text_buffer_get_start_iter (console_buf, &start_iter);
                lines = gtk_text_buffer_get_line_count (console_buf);
                if (lines > 80){
                        g_message ("Clear socket log %d lines. <%s>", lines, msg);
                        gtk_text_buffer_get_iter_at_line_index (console_buf, &end_trim_iter, lines-10, 0);
                        gtk_text_buffer_delete (console_buf,  &start_iter,  &end_trim_iter);
                }

                gtk_text_buffer_get_end_iter (console_buf, &iter);
                gtk_text_buffer_create_mark (console_buf, "scroll", &iter, TRUE);
                
                return;
	}

	// read string which contain last command output
	console_buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW(text_status));
	gtk_text_buffer_get_bounds (console_buf, &start_iter, &end_iter);

	// get output widget content
	output = g_string_new (gtk_text_buffer_get_text (console_buf,
                                                         &start_iter, &end_iter,
                                                         FALSE));

        // insert text and keep view at end
        gtk_text_buffer_get_end_iter (console_buf, &iter); // get end
        gtk_text_buffer_insert (console_buf, &iter, msg, -1); // insert at end
        gtk_text_iter_set_line_offset (&iter, 0); // do not scroll horizontal
        end_mark = gtk_text_buffer_get_mark (console_buf, "scroll");
        gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (text_status), end_mark);

        // purge top
        gtk_text_buffer_get_start_iter (console_buf, &start_iter);
        lines = gtk_text_buffer_get_line_count (console_buf);
        if (lines > (max_lines)){
                gtk_text_buffer_get_iter_at_line_index (console_buf, &end_trim_iter, lines-max_lines, 0);
                gtk_text_buffer_delete (console_buf,  &start_iter,  &end_trim_iter);
        }

        gtk_text_iter_set_line_offset (&iter, 0); // do not scroll horizontal
        end_mark = gtk_text_buffer_get_mark (console_buf, "scroll");
        gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (text_status), end_mark);

#if 1
        // scroll to end
        gtk_text_buffer_get_end_iter (console_buf, &end_iter);
        end_mark = gtk_text_buffer_create_mark (console_buf, "cursor", &end_iter,
                                                FALSE);
        g_object_ref (end_mark);
        gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (text_status),
                                      end_mark, 0.0, FALSE, 0.0, 0.0);
        g_object_unref (end_mark);
#endif
        main_get_gapp()->monitorcontrol->LogEvent ("RPSPMC Stream Info", msg, 1); // add to main logfile with level 1       

}

void RPspmc_pacpll::on_connect_actions(){
        status_append ("3. RedPitaya SPM Control, PAC-PLL loading configuration:\n");

        // reconnect_mode: 0 = RECONNECT attempt to runnign FPGA, may fail if not running!
        //                 1 = COLD START/FPGA RELOAD=FULL RESET

        if (reconnect_mode){ // only of cold start
                send_all_parameters (); // PAC-PLL
        } else {
                send_all_parameters (); // PAC-PLL -- always update
        }
        
        while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);

        status_append (" * RedPitaya SPM Control, PAC-PLL init, DEC FAST (12)...\n");
        gtk_combo_box_set_active (GTK_COMBO_BOX (update_tr_widget), 6);  // select Ph,dF,Am,Ex
        gtk_combo_box_set_active (GTK_COMBO_BOX (update_ts_widget), 12); // select 12, fast
        for (int i=0; i<25; ++i){
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
                usleep(20000);
        }
        
        gtk_combo_box_set_active (GTK_COMBO_BOX (update_op_widget), 3); // INIT BRAM TRANSPORT AND CLEAR FIR RING BUFFERS, give me a second...
        status_append (" * RedPitaya SPM Control, PAC-PLL init, INIT-FIR... [2s Zzzz]\n");
        for (int i=0; i<100; ++i){
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
                usleep(20000);
        }
        
        status_append (" * RedPitaya SPM Control, PAC-PLL init, INIT-FIR completed.\n");
        for (int i=0; i<50; ++i){
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
                usleep(20000);
        }
        
        status_append (" * RedPitaya SPM Control, PAC-PLL normal operation, set to data streaming mode.\n");
        for (int i=0; i<25; ++i){
                while(g_main_context_pending (NULL)) g_main_context_iteration (NULL, FALSE);
                usleep(20000);
        }
        
        gtk_combo_box_set_active (GTK_COMBO_BOX (update_op_widget), 5); // STREAMING OPERATION
        gtk_combo_box_set_active (GTK_COMBO_BOX (update_ts_widget), 18); // select 19 (typical scan decimation/time scale filter)
        
        status_append (" * RedPitaya SPM Control: PAC-PLL is ready.\n");
        status_append (" * RedPitaya SPM Control, SPMC init...\n");

        rpspmc_hwi->info_append (NULL); // clear
        rpspmc_hwi->info_append ("RPSPMC+PACPALL is connected.");
 
        int i=0;
        
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Version.....: 0x%08x\n", (int)spmc_parameters.rpspmc_version); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC VDate.......: 0x%08x\n", (int)spmc_parameters.rpspmc_date); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC FPGAIMPL....: 0x%08x\n", (int)spmc_parameters.rpspmc_fpgaimpl); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC FPGAIMPL_D..: 0x%08x\n", (int)spmc_parameters.rpspmc_fpgaimpl_date); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC FPGA_STAUP..: 0x%08x\n", (int)spmc_parameters.rpspmc_fpgastartup); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC FPGA_RSC#...: 0x%08x\n", (int)spmc_parameters.rpspmc_fpgastartupcnt); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }

        if ((int)spmc_parameters.rpspmc_fpgaimpl != 0xec010099 ||
            (int)spmc_parameters.rpspmc_version < 0x00160000){
                g_warning ("INVALID RPSPMC server or wrong FPGA implementaion loaded. -- trying to continue, may fail/crash at any point from here.");
                { gchar *tmp = g_strdup_printf (" EE ERROR: RedPitaya SPMC Server or RPSPMC FPGA implementation invalid.\n *\n"); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        } else {
                { gchar *tmp = g_strdup_printf (" * RedPitaya SPMC Server and RPSPMC FPGA implementation accepted.\n *\n"); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        }
        
        /*
         * RedPitaya SPM RPSPMC Version.....: 0x00160000
         * RedPitaya SPM RPSPMC VDate.......: 0x20250406
         * RedPitaya SPM RPSPMC FPGAIMPL....: 0xec010099
         * RedPitaya SPM RPSPMC FPGAIMPL_D..: 0x20250328
         * RedPitaya SPM RPSPMC FPGA_STAUP..: 0x00000001
         * RedPitaya SPM RPSPMC FPGA_RSC#...: 0x00000001
         * RedPitaya SPM RPSPMC Z_SERVO_MODE: 0x00000000
         */

        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_SERVO_MODE: 0x%08x\n", i=(int)spmc_parameters.z_servo_mode); status_append (tmp); g_free (tmp); }        
        i &= MM_ON | MM_LOG | MM_FCZ | MM_RESET;
        RPSPMC_ControlClass->mix_transform_mode[0] = i;
        { gchar *tmp = g_strdup_printf ("%d", i & (MM_ON | MM_LOG | MM_FCZ)); gtk_combo_box_set_active_id (GTK_COMBO_BOX (RPSPMC_ControlClass->z_servo_options_selector[0]), tmp); g_free (tmp); }
        ;
        { gchar *tmp = g_strdup_printf (" *                                 ==> %s%s [%s]\n",
                                        i&MM_ON ? i&MM_LOG  ?"LOG":"LIN":"OFF",
                                        i&MM_FCZ && i&MM_ON ? "-FCZ":"",
                                        i&MM_RESET          ? "RESET":"NORMAL"); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }        

        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_POLARITY..: %s "
                                        "** Positive Polarity := Z piezo aka tip extends towards the surface with positive voltage.\n"
                                        " *                                        "
                                        "** But Gxsm and we want logically Z been positive for higher up, thus inverting internally all Z values.\n",
                                        ((int)spmc_parameters.gvp_status)&(1<<7) ? "NEG":"POS"); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }        

        // ((int)spmc_parameters.gvp_status)&(1<<7) BIT=1 (true) := invert Z on FPGA after slope to DAC-Z, this effects Z-monitor. FPGA internal Z is inverted vs GXSM convetion of pos Z = tip up
        spmc_parameters.gxsm_z_polarity = ((int)spmc_parameters.gvp_status)&(1<<7) ? 1:-1;
        int gxsm_preferences_polarity = xsmres.ScannerZPolarity ? 1 : -1; // 1: pos, 0: neg (bool) -- adjust zpos_ref accordingly!
                 
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_SERVO_SET.: %g Veq\n", spmc_parameters.z_servo_setpoint); status_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_SERVO_CP..: %g\n", spmc_parameters.z_servo_cp); status_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_SERVO_CI..: %g\n", spmc_parameters.z_servo_ci); status_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_SERVO_UPR.: %g V\n", spmc_parameters.z_servo_upper); status_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_SERVO_LOR.: %g V\n", spmc_parameters.z_servo_lower); status_append (tmp); g_free (tmp); }

        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z_SERVO IN..: %08x MUX selection\n", i=(int)spmc_parameters.z_servo_src_mux); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }
        //i &= 3; // only bit 0,1,2
        { gchar *tmp = g_strdup_printf (" *                                 ==> %s\n", i==0? "IN2-RF": i==1?"IN3-AD4630-24A":i==2?"IN3-AD4630-24A-FIR":"EEE"); status_append (tmp); g_free (tmp); }        

        if (i >= 0 && i < 3)
                gtk_combo_box_set_active (GTK_COMBO_BOX (RPSPMC_ControlClass->z_servo_current_source_options_selector), i);
        
        guint64 mux=0;
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC GVP SRCS MUX: %016lx MUX selection code\n", mux=(((guint64)spmc_parameters.gvp_stream_mux_1) << 32) | (guint64)spmc_parameters.gvp_stream_mux_0); status_append (tmp); g_free (tmp); }
        for (int k=0; k<6; ++k){

                RPSPMC_ControlClass->probe_source[k] = __GVP_muxval_selection (mux, k);

                int pass=0;
                if (RPSPMC_ControlClass->probe_source[k] >= 0 && RPSPMC_ControlClass->probe_source[k] < 32){
                        if (rpspmc_swappable_signals[RPSPMC_ControlClass->probe_source[k]].label){
                                { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC GVP SRCS MUX[%d]: %02d <=> %s\n", k, RPSPMC_ControlClass->probe_source[k], rpspmc_swappable_signals[RPSPMC_ControlClass->probe_source[k]].label); status_append (tmp); g_free (tmp); }
                                gtk_combo_box_set_active (GTK_COMBO_BOX (RPSPMC_ControlClass->probe_source_signal_selector[k]), RPSPMC_ControlClass->probe_source[k]);
                                pass=1;
                        }
                }
                if (!pass) {
                        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC GVP SRCS MUX[%d]: %02d <=> FPGA DATA ERROR: INVALID SIGNAL\n", k, RPSPMC_ControlClass->probe_source[k]); status_append (tmp); g_free (tmp); }
                        RPSPMC_ControlClass->probe_source[k] = 0; // default to safety
                        gtk_combo_box_set_active (GTK_COMBO_BOX (RPSPMC_ControlClass->probe_source_signal_selector[k]), RPSPMC_ControlClass->probe_source[0]);
                }
        }

        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC X,Y,Z.......: %g %g %g V\n", spmc_parameters.x_monitor, spmc_parameters.y_monitor, spmc_parameters.z_monitor); status_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC Z0[z-slope].: %g V\n", spmc_parameters.z0_monitor); status_append (tmp); g_free (tmp); }
        { gchar *tmp = g_strdup_printf (" * RedPitaya SPM RPSPMC BIAS........: %g V\n", spmc_parameters.bias_monitor); status_append (tmp); g_free (tmp); }

        if (reconnect_mode){ // only if cold start
                status_append (" * RedPitaya SPM RPSPMC: Init completed and ackowldeged. Releasing normal control. Updating all from GXSM parameters.\n");
                write_parameter ("RPSPMC_INITITAL_TRANSFER_ACK", 99); // Acknoledge inital parameters received, release server parameter updates
                RPSPMC_ControlClass->update_FPGA_from_GUI ();
                update_SPMC_parameters ();
                SPMC_cold_start_init (); // ONLY at COLD START
        } else {
                // update GUI! (mission critical: Z-SERVO mainly)
                RPSPMC_ControlClass->update_GUI_from_FPGA ();
                status_append (" * RedPitaya SPM Control: RECONNECTING/READBACK Z-SERVO STATUS...\n");
                // ...
                update_SPMC_parameters (); // then send all other less critical parameters to make sure all is in sync

                //
                status_append (" * RedPitaya SPM RPSPMC: Readback completed and ackowldeged. Releasing normal control. No FPGA parameter update from GXSM!\n");
                write_parameter ("RPSPMC_INITITAL_TRANSFER_ACK", 99); // Acknoledge inital parameters received, release server parameter updates
        }

        
        //status_append (" * RedPitaya SPM Control ready. NEXT: Please Check Connect Stream.\n");
        status_append (" * RedPitaya SPM Control ready. Connecting SPMC Data Stream...\n");

        rpspmc_hwi->info_append (" * RPSPMC is READY *");
 
        gtk_check_button_set_active (GTK_CHECK_BUTTON (RPSPMC_ControlClass->stream_connect_button), true);

        gtk_button_set_child (GTK_BUTTON (RPSPMC_ControlClass->GVP_stop_all_zero_button), gtk_image_new_from_icon_name ("gxsm4-rp-icon"));

        gapp->check_events ();

        // VERIFY IF CORRECT, ASK TO ADJUST IF MISMATCH
        if ((-spmc_parameters.gxsm_z_polarity) != gxsm_preferences_polarity &&  gxsm_preferences_polarity < 0)
                if ( gapp->question_yes_no ("WARNING: Gxsm Preferences indicate NEGATIVE Z Polarity.\nPlease confirm to set Z-Polarity set to NEGATIVE.")){
                        rpspmc_pacpll->write_parameter ("SPMC_Z_POLARITY", -1); // inverted "pos" on hardware internal FPGA processing as of pos feedback behavior default
                        spmc_parameters.gxsm_z_polarity = 1; // inverted to flip for Gxsm default Z-up = pos
                        for (int i=0; i<10; ++i) { usleep (100000); gapp->check_events (); }
                        { gchar *tmp = g_strdup_printf (" *** Adjusted (+=>-) SPM RPSPMC Z_POLARITY..: %s\n", ((int)spmc_parameters.gvp_status)&(1<<7) ? "NEG":"POS"); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }        
                }

        gapp->check_events ();

        // most usual case w single Z piezo:
        if ((-spmc_parameters.gxsm_z_polarity) != gxsm_preferences_polarity &&  gxsm_preferences_polarity > 0)
                if ( gapp->question_yes_no ("WARNING: Gxsm Preferences indicate POSITIVE Z Polarity.\nPlease confirm to set Z-Polarity set to POSITIVE.")){
                        rpspmc_pacpll->write_parameter ("SPMC_Z_POLARITY", 1); // inverted "pos" on hardware internal FPGA processing as of pos feedback behavior default
                        spmc_parameters.gxsm_z_polarity = -1; // inverted to flip for Gxsm default Z-up = pos
                        for (int i=0; i<10; ++i) { usleep (100000); gapp->check_events (); }
                        { gchar *tmp = g_strdup_printf (" *** Adjusted (-=>+) SPM RPSPMC Z_POLARITY..: %s\n", ((int)spmc_parameters.gvp_status)&(1<<7) ? "NEG":"POS"); status_append (tmp);  rpspmc_hwi->info_append (tmp); g_free (tmp); }        
                }

        gapp->check_events ();
        
        if ((-spmc_parameters.gxsm_z_polarity) != gxsm_preferences_polarity){
                g_warning (" Z-Polarity was not changed. ");
        }
        
}



double AutoSkl(double dl){
  double dp=floor(log10(dl));
  if(dp!=0.)
    dl/=pow(10.,dp);
  if(dl>4) dl=5;
  else if(dl>1) dl=2;
  else dl=1;
  if(dp!=0.)
    dl*=pow(10.,dp);
  return dl;
}

double AutoNext(double x0, double dl){
  return(ceil(x0/dl)*dl);
}

void RPspmc_pacpll::stream_data (){
#if 0
        int deci=16;
        int n=4;
        if (data_shr_max > 2) { //if (ch_freq >= 0 || ch_ampl >= 0){
                double decimation = 125e6 * main_get_gapp()->xsm->hardware->GetScanrate ();
                deci = (gint64)decimation;
                if (deci > 2){
                        for (n=0; deci; ++n) deci >>= 1;
                        --n;
                }
                if (n>data_shr_max) n=data_shr_max; // limit to 24. Note: (32 bits - 8 control, may shorten control, only 3 needed)
                deci = 1<<n;
                //g_print ("Scan Pixel rate is %g s/pix -> Decimation %g -> %d n=%d\n", main_get_gapp()->xsm->hardware->GetScanrate (), decimation, deci, n);
        }
        if (deci != data_decimation){
                data_decimation = deci;
                data_shr = n;
                write_parameter ("SHR_DEC_DATA", data_shr);
                write_parameter ("TRANSPORT_DECIMATION", data_decimation);
        }

        if (ch_freq >= 0 || ch_ampl >= 0){
                if (x < main_get_gapp()->xsm->scan[ch_freq]->mem2d->GetNx () &&
                    y < main_get_gapp()->xsm->scan[ch_freq]->mem2d->GetNy ()){
                        for (int i=0; i<1000; ++i) {
                                if (ch_freq >= 0)
                                        main_get_gapp()->xsm->scan[ch_freq]->mem2d->PutDataPkt (parameters.freq_fb_lower + pacpll_signals.signal_ch2[i], x,y);
                                if (ch_ampl >= 0)
                                        main_get_gapp()->xsm->scan[ch_ampl]->mem2d->PutDataPkt (pacpll_signals.signal_ch1[i], x,y);
                                ++x;
                                if (x >= main_get_gapp()->xsm->scan[ch_freq]->mem2d->GetNx ()) {x=0; ++y; };
                                if (y >= main_get_gapp()->xsm->scan[ch_freq]->mem2d->GetNy ()) break;
                        }
                        streampos += 1024;
                        main_get_gapp()->xsm->scan[ch_freq]->draw ();
                }
        }
#endif
}

#define FFT_LEN 1024
double blackman_nuttall_window (int i, double y){
        static gboolean init=true;
        static double w[FFT_LEN];
        if (init){
                for (int i=0; i<FFT_LEN; ++i){
                        double ipl = i*M_PI/FFT_LEN;
                        double x1 = 2*ipl;
                        double x2 = 4*ipl;
                        double x3 = 6*ipl;
                        w[i] = 0.3635819 - 0.4891775 * cos (x1) + 0.1365995 * cos (x2) - 0.0106411 * cos (x3);
                }
                init=false;
        }
        return w[i]*y;
}

gint compute_fft (gint len, double *data, double *psd, double mu=1.){
        static gint n=0;
        static double *in=NULL;
        //static double *out=NULL;
        static fftw_complex *out=NULL;
        static fftw_plan plan=NULL;

        if (n != len || !in || !out || !plan){
                if (plan){
                        fftw_destroy_plan (plan); plan=NULL;
                }
                // free temp data memory
                if (in) { delete[] in; in=NULL; }
                if (out) { delete[] out; out=NULL; }
                n=0;
                if (len < 2) // clenaup only, exit
                        return 0;

                n = len;
                // get memory for complex data
                in  = new double [n];
                out = new fftw_complex [n];
                //out = new double [n];

                // create plan for fft
                plan = fftw_plan_dft_r2c_1d (n, in, out, FFTW_ESTIMATE);
                //plan = fftw_plan_r2r_1d (n, in, out, FFTW_REDFT00, FFTW_ESTIMATE);
                if (plan == NULL)
                        return -1;
        }

                
        // prepare data for fftw
#if 0
        double s=2.*M_PI/(n-1);
        double a0=0.54;
        double a1=1.-a0;
        int n2=n/2;
        for (int i = 0; i < n; ++i){
                double w=a0+a1*cos((i-n2)*s);
                in[i] = w*data[i];
        }
#else
        //double mx=0., mi=0.;
        for (int i = 0; i < n; ++i){
                in[i] = blackman_nuttall_window (i, data[i]);
                //if (i==0) mi=mx=in[i];
                //if (in[i] > mx) mx=in[i];
                //if (in[i] < mi) mi=in[i];
        }
        //g_print("FFT in Min: %g Max: %g\n",mi,mx);
#endif
        //g_print("FFTin %g",in[0]);
                
        // compute transform
        fftw_execute (plan);

        //double N=2*(n-1);
        //double scale = 1./(max*2*(n-1));
        double scale = M_PI*2/n;
        double mag=0.;
        for (int i=0; i<n/2; ++i){
                mag = scale * sqrt(c_re(out[i])*c_re(out[i]) + c_im(out[i])*c_im(out[i]));
                //psd_db[i] = gfloat((1.-mu)*(double)psd_db[i] + mu*20.*log(mag));
                psd[i] = gfloat((1.-mu)*(double)psd[i] + mu*mag);
                //if (i==0) mi=mx=mag;
                //if (mag > mx) mx=mag;
                //if (mag < mi) mi=mag;
        }
        //g_print("FFT out Min: %g Max: %g\n",mi,mx);
        return 0;
}

double RPspmc_pacpll::unwrap (int k, double phi){
        static int pk=0;
        static int side=1;
        static double pphi;
        
#if 0
        if (!unwrap_phase_plot) return phi;

        if (k==0){ // choose start
                if (phi > 0.) side = 1; else side = -1;
                pk=k; pphi=phi;
                return (phi);
        }
        if (fabs(phi-pphi) > 180){
                switch (side){
                case 1: phi += 360;
                        pk=k; pphi=phi;
                        return (phi);
                case -1: phi -= 360;
                        pk=k; pphi=phi;
                        return (phi);
                }
        } else {
                if (phi > 0.) side = 1; else side = -1;
        }
        pk=k; pphi=phi;
#endif
        return (phi);
}

// to force udpate call:   gtk_widget_queue_draw (self->signal_graph_area);
void RPspmc_pacpll::graph_draw_function (GtkDrawingArea *area, cairo_t *cr,
                                                       int             width,
                                                       int             height,
                                                       RPspmc_pacpll *self){
        self->dynamic_graph_draw_function (area, cr, width, height);
}

void RPspmc_pacpll::dynamic_graph_draw_function (GtkDrawingArea *area, cairo_t *cr, int width, int height){
        int n=1023;
        int h=(int)height; //scope_height_points;
        static int hist=0;
        static int current_width=0;
        if (!run_scope)
                h=10;
        scope_width_points=current_width = width;
        double xs = scope_width_points/1024.;
        double xcenter = scope_width_points/2;
        double xwidcenter = scope_width_points/2;
        double scope_width = scope_width_points;
        double x0 = xs*n/2;
        double yr = h/2;
        double y_hi  = yr*0.95;
        double dB_hi   =  0.0;
        double dB_mags =  4.0;
        static int rs_mode=0;
        const int binary_BITS = 16;
        
        if (!run_scope && hist == h)
                return;
        //if (current_width != (int)scope_width_points || h != hist){
                //current_width = (int)scope_width_points;
                //gtk_drawing_area_set_content_width (area, current_width);
                //gtk_drawing_area_set_content_height (area, h);
        //}
        hist=h;
        if (run_scope){
                cairo_translate (cr, 0., yr);
                cairo_scale (cr, 1., 1.);
                cairo_save (cr);
                cairo_item_rectangle *paper = new cairo_item_rectangle (0., -yr, scope_width_points, yr);
                paper->set_line_width (0.2);
                paper->set_stroke_rgba (CAIRO_COLOR_GREY1);
                paper->set_fill_rgba (CAIRO_COLOR_BLACK);
                paper->draw (cr);
                delete paper;
                //cairo_item_segments *grid = new cairo_item_segments (44);
                
                double avg=0.;
                double avg10=0.;
                char *valuestring;
                cairo_item_text *reading = new cairo_item_text ();
                reading->set_font_face_size ("Ununtu", 10.);
                reading->set_anchor (CAIRO_ANCHOR_W);
                cairo_item_path *wave = new cairo_item_path (n);
                cairo_item_path *binwave8bit[binary_BITS] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                wave->set_line_width (1.0);
                const gchar *ch_id[] = { "Ch1", "Ch2", "Ch3", "Ch4", "Ch5", "Phase", "Ampl" };
                CAIRO_BASIC_COLOR_IDS color[] = { CAIRO_COLOR_RED_ID, CAIRO_COLOR_GREEN_ID, CAIRO_COLOR_CYAN_ID, CAIRO_COLOR_YELLOW_ID,
                                                  CAIRO_COLOR_BLUE_ID, CAIRO_COLOR_RED_ID, CAIRO_COLOR_GREEN_ID, CAIRO_COLOR_FORESTGREEN_ID  };
                double *signal[] = { pacpll_signals.signal_ch1, pacpll_signals.signal_ch2, pacpll_signals.signal_ch3, pacpll_signals.signal_ch4, pacpll_signals.signal_ch5, // 0...4 CH1..5
                                     pacpll_signals.signal_phase, pacpll_signals.signal_ampl  }; // 5,6 PHASE, AMPL in Tune Mode, averaged from burst
                double x,xf,min,max,s,ydb,yph;

                const gchar *ch01_lab[2][10] = { {"IN1", "IN1-AC", "Ampl", "dPhase", "Phase", "Phase", "-", "-", "AX7", NULL },
                                                 {"IN2", "IN1-DC", "Exec", "dFreq ", "Ampl ", "dFreq", "-", "-", "AX8", NULL } }; 
                
                if (operation_mode >= 6 && operation_mode <= 9){
                        if (operation_mode == 9)
                                rs_mode=3;
                        operation_mode = 6; // TUNE
                        channel_selections[5]=5;
                        channel_selections[6]=6;
                } else {
                        rs_mode = 0;
                        channel_selections[5]=0;
                        channel_selections[6]=0;
                }
                // operation_mode = 6 : in TUNING CONFIGURATION, 7 total signals
               
                int ch_last=(operation_mode == 6) ? 7 : 5;
                int tic_db=0;
                int tic_deg=0;
                int tic_hz=0;
                int tic_lin=0;
                for (int ch=0; ch<ch_last; ++ch){
                        int part_i0=0;
                        int part_pos=1;

                        if (channel_selections[ch] == 0)
                                continue;

                        // Setup binary mode traces (DBG Ch)
                        if(channel_selections[0]==9){
                                if (!binwave8bit[0])
                                        for (int bit=0; bit<binary_BITS; ++bit)
                                                binwave8bit[bit] = new cairo_item_path (n);
                                for (int bit=0; bit<binary_BITS; ++bit){
                                        if (ch == 0)
                                                        binwave8bit[bit]->set_stroke_rgba (color[bit%4]);
                                        else
                                                        binwave8bit[bit]->set_stroke_rgba (color[4+(bit%4)]);
                                }
                        }
                        
                        if (operation_mode == 6 && (ch == 0 || ch == 1))
                                if (ch == 0)
                                        wave->set_stroke_rgba (1.,0.,0.,0.4);
                                else
                                        wave->set_stroke_rgba (0.,1.,0.,0.4);
                        else
                                wave->set_stroke_rgba (BasicColors[color[ch]]);

                        if (operation_mode == 6 && ch > 1)
                                wave->set_linemode (rs_mode);
                        else
                                wave->set_linemode (0);

                        min=max=signal[ch][512];
                        for (int k=0; k<n; ++k){
                                s=signal[ch][k];
                                if (s>max) max=s;
                                if (s<min) min=s;

                                if (scope_ac[ch])
                                        s -= scope_dc_level[ch]; // AC
                                
                                if (ch<2)
                                        if (scope_z[ch]>0.)
                                                s *= scope_z[ch]; // ZOOM

                                xf = xcenter + scope_width*pacpll_signals.signal_frq[k]/parameters.tune_span; // tune plot, freq x
                                x  = (operation_mode == 6 && ch > 1) ? xf : xs*k; // over freq or time plot

                                // MAPPING TREE ---------------------

                                // GPIO monitoring channels ch >= 2
                                if (operation_mode == 6) { // TUNING
                                        if ((ch == 6) || (ch == 1) || ( (ch > 1) && (ch < 5) && ((channel_selections[ch]==1) || (channel_selections[ch]==5)))){ // 1,5: GPIO Ampl
                                                // 0..-60dB range, 1mV:-60dB (center), 0dB:1000mV (top)
                                                wave->set_xy_fast (k, ch == 1 ? x:xf, ydb=db_to_y (dB_from_mV (s), dB_hi, y_hi, dB_mags)), tic_db=1;
                                        } else if ((ch == 5) || (ch == 0) || ( (ch > 1) && (ch < 5) && ((channel_selections[ch]==2) || (channel_selections[ch]==6)))){ // 2,6: GPIO Phase
                                                // [-180 .. +180 deg] x expand
                                                wave->set_xy_fast (k, ch == 0 ? x:xf, yph=deg_to_y (ch == 0 ? s:unwrap(k,s), y_hi)), tic_deg=1; // Phase for Tuning: with hi level unwrapping for plot
                                        } else if ((ch > 1) && (ch < 5) && channel_selections[ch] > 0){ // Linear, other GPIO
                                                wave->set_xy_fast (k, xf, -yr*(gain_scale[ch]>0.?gain_scale[ch]:1.)*s), tic_lin=1; // else: linear scale with gain
                                        } // else  --
                                } else { // SCOPE, OPERATION: over time / index
                                        if ((ch > 1) && (ch < 5)){ // GPIO mon...
                                                if (channel_selections[ch]==2 || channel_selections[ch]==6) // Phase
                                                        wave->set_xy_fast (k, unwrap (k,x), deg_to_y (s, y_hi)), tic_deg=1;
                                                else if (channel_selections[ch]==1 || channel_selections[ch]==5 || channel_selections[ch]==9) // Amplitude mode for GPIO monitoring
                                                        wave->set_xy_fast (k, x, db_to_y (dB_from_mV (s), dB_hi, y_hi, dB_mags)), tic_db=1;
                                                else
                                                        wave->set_xy_fast (k, x,-yr*(gain_scale[ch]>0.?gain_scale[ch]:1.)*s), tic_lin=1; // else: linear scale with gain

                                        } else if (   (ch == 0 && channel_selections[0]==3) // AMC:  Ampl,  Exec
                                                   || (ch == 1 && channel_selections[0]==3) // AMC:  Ampl,  Exec
                                                   || (ch == 1 && channel_selections[0]==5) // TUNE: Phase, Ampl
                                                      ){
                                                wave->set_xy_fast (k, x, db_to_y (dB_from_mV (s), dB_hi, y_hi, dB_mags)), tic_db=1; // Ampl, dB
                                                if (k>2 && s<0. && part_pos){  // dB for negative ampl -- change color!!
                                                        wave->set_stroke_rgba (CAIRO_COLOR_MAGENTA_ID);
                                                        wave->draw_partial (cr, part_i0, k-1); part_i0=k; part_pos=0;
                                                }
                                                if (k>2 && s>0. && !part_pos){
                                                        wave->set_stroke_rgba (BasicColors[color[ch]]);
                                                        wave->draw_partial (cr, part_i0, k-1); part_i0=k; part_pos=1;
                                                }

                                        } else if (   (ch == 0 && channel_selections[0]==5) // TUNE: Phase, Ampl -- with unwrapping
                                                   || (ch == 0 && channel_selections[0]==4) // PHC: dPhase, dFreq
                                                   || (ch == 0 && channel_selections[0]==6) // OP:   Phase, dFreq
                                                )
                                                wave->set_xy_fast (k, x, deg_to_y (s, y_hi)), tic_deg=1; // Phase direct

                                        else if (   (ch == 1 && channel_selections[0]==4) // PHC:dPhase, dFreq
                                                 || (ch == 1 && channel_selections[0]==6) // OP:  Phase, dFreq
                                                )
                                                    wave->set_xy_fast (k, x, freq_to_y (s, y_hi)), tic_hz=1; // Hz
                                        else if ( channel_selections[0]==9 && ch < 2){ // DBG AXIS7/8
                                                if (fabs (signal[ch][k]) > 10)
                                                        g_print("%04d C%01d: %g, %g * %g\n",k,ch,x,gain_scale[ch],signal[ch][k]);
                                                wave->set_xy_fast_clip_y (k, x,-yr*(gain_scale[ch]>0.?gain_scale[ch]:1.)*signal[ch][k], yr), tic_lin=1; // else: linear scale with gain
#if 0
                                                for (int bit=0; bit<binary_BITS; ++bit)
                                                        binwave8bit[bit]->set_xy_fast (k, x, binary_to_y (signal[ch][k], bit, ch, y_hi, binary_BITS));
                                                wave->set_xy_fast (k, x, -yr*((int)(s)%256)/256.);
#endif
                                        } else // SCOPE IN1,IN2, IN1 AC,DC
                                                wave->set_xy_fast (k, x,-yr*(gain_scale[ch]>0.?gain_scale[ch]:1.)*s), tic_lin=1; // else: linear scale with gain
                                }
                        }
                        if (s>0. && part_i0>0){
                                wave->set_stroke_rgba (BasicColors[color[ch]]);
                                wave->draw_partial (cr, part_i0, n-2);
                        }
                        if (s<0. && part_i0>0){
                                wave->set_stroke_rgba (CAIRO_COLOR_MAGENTA_ID);
                                wave->draw_partial (cr, part_i0, n-2);
                        }
                        if (channel_selections[ch] && part_i0==0){

                                wave->draw (cr);
                                
                                if (binwave8bit[0])
                                        for (int bit=0; bit<binary_BITS; ++bit)
                                                binwave8bit[bit]->draw (cr);
                        }

                        if (ch<6){
                                scope_dc_level[ch] = 0.5*(min+max);

                                if (gain_scale[ch] < 0.)
                                        if (scope_ac[ch]){
                                                min -= scope_dc_level[ch];
                                                max -= scope_dc_level[ch];
                                                gain_scale[ch] = 0.7 / (0.0001 + (fabs(max) > fabs(min) ? fabs(max) : fabs(min)));
                                        } else
                                                gain_scale[ch] = 0.7 / (0.0001 + (fabs(max) > fabs(min) ? fabs(max) : fabs(min)));
                        }
                                           
                        if (operation_mode != 6 && channel_selections[ch]){
                                avg=avg10=0.;
                                for (int i=1023-100; i<1023; ++i) avg+=signal[ch][i]; avg/=100.;
                                for (int i=1023-10; i<1023; ++i) avg10+=signal[ch][i]; avg10/=10.;
                                valuestring = g_strdup_printf ("%s %g %12.5f [x %g] %g %g {%g}",
                                                               ch < 2 ? ch01_lab[ch][channel_selections[ch]-1] : ch_id[ch],
                                                               avg10, avg, gain_scale[ch], min, max, max-min);
                                reading->set_stroke_rgba (BasicColors[color[ch]]);
                                reading->set_text (10, -(110-14*ch), valuestring);
                                g_free (valuestring);
                                reading->draw (cr);
                        }
                        
                }
                if (pacpll_parameters.bram_write_adr >= 0 && pacpll_parameters.bram_write_adr < 0x4000){
                        cairo_item_segments *cursors = new cairo_item_segments (2);
                        double icx = scope_width * (0.5*pacpll_parameters.bram_write_adr - bram_shift)/1024.;
                        cursors->set_line_width (2.5);
                        if (icx >= 0. && icx <= scope_width){
                                cursors->set_line_width (0.5);
                                cursors->set_stroke_rgba (CAIRO_COLOR_WHITE);
                        } else if (icx < 0)  cursors->set_stroke_rgba (CAIRO_COLOR_RED), icx=2;
                        else cursors->set_stroke_rgba (CAIRO_COLOR_RED), icx=scope_width-2;
                        cursors->set_xy_fast (0, icx, -80.);
                        cursors->set_xy_fast (1, icx,  80.);
                        cursors->draw (cr);
                        g_free (cursors);
                }

                if (debug_level&1){
                        valuestring = g_strdup_printf ("BRAM{A:0x%04X, #:%d, F:%d}",
                                                       (int)pacpll_parameters.bram_write_adr,
                                                       (int)pacpll_parameters.bram_sample_pos,
                                                       (int)pacpll_parameters.bram_finished);
                        reading->set_stroke_rgba (CAIRO_COLOR_WHITE);
                        reading->set_text (10, -(110-14*6), valuestring);
                        g_free (valuestring);
                        reading->draw (cr);
                }

                cairo_item_segments *cursors = new cairo_item_segments (2);
                cursors->set_line_width (0.3);

                // tick marks dB
                if (tic_db){
                        cursors->set_stroke_rgba (0.2,1.,0.2,0.5);
                        for (int db=(int)dB_hi; db >= -20*dB_mags; db -= 10){
                                double y;
                                valuestring = g_strdup_printf ("%4d dB", db);
                                reading->set_stroke_rgba (CAIRO_COLOR_GREEN);
                                reading->set_text (scope_width - 40,  y=db_to_y ((double)db, dB_hi, y_hi, dB_mags), valuestring);
                                g_free (valuestring);
                                reading->draw (cr);
                                cursors->set_xy_fast (0,xcenter+(scope_width)*(-0.5),y);
                                cursors->set_xy_fast (1,xcenter+(scope_width*0.7)*(0.5),y);
                                cursors->draw (cr);
                        }
                }
                // ticks deg
                if(tic_deg){
                        cursors->set_stroke_rgba (1.,0.2,0.2,0.5);
                        for (int deg=-180*deg_extend; deg <= 180*deg_extend; deg += 30*deg_extend){
                                double y;
                                valuestring = g_strdup_printf ("%d" UTF8_DEGREE, deg);
                                reading->set_stroke_rgba (CAIRO_COLOR_RED);
                                reading->set_text (scope_width- 80, y=deg_to_y (deg, y_hi), valuestring);
                                g_free (valuestring);
                                reading->draw (cr);
                                cursors->set_xy_fast (0,xcenter+(scope_width)*(-0.5),y);
                                cursors->set_xy_fast (1,xcenter+(scope_width*0.7)*(0.5),y);
                                cursors->draw (cr);
                        }
                }
                // ticks lin
                if(tic_lin){
                        cursors->set_stroke_rgba (0.8,0.8,0.8,0.5);
                        for (int yi=-100; yi <= 100; yi += 10){
                                double y; //-yr*(gain_scale[ch]>0.?gain_scale[ch]:1.)*s
                                valuestring = g_strdup_printf ("%d", yi);
                                reading->set_stroke_rgba (0.8,0.8,0.8,0.5);
                                reading->set_text (scope_width- 120, y=yi*0.01*y_hi, valuestring);
                                g_free (valuestring);
                                reading->draw (cr);
                                cursors->set_xy_fast (0,xcenter+(scope_width)*(-0.5),y);
                                cursors->set_xy_fast (1,xcenter+(scope_width*0.7)*(0.5),y);
                                cursors->draw (cr);
                        }
                }
                
                if(tic_hz){
                        cursors->set_stroke_rgba (0.8,0.8,0.0,0.5);
                        for (int yi=-10; yi <= 10; yi += 1){
                                double y; //-yr*(gain_scale[ch]>0.?gain_scale[ch]:1.)*s
                                valuestring = g_strdup_printf ("%d Hz", yi);
                                reading->set_stroke_rgba (0.8,0.8,0.0,0.5);
                                reading->set_text (scope_width- 120, y=yi*0.1*y_hi, valuestring);
                                g_free (valuestring);
                                reading->draw (cr);
                                cursors->set_xy_fast (0,xcenter+(scope_width)*(-0.5),y);
                                cursors->set_xy_fast (1,xcenter+(scope_width*0.7)*(0.5),y);
                                cursors->draw (cr);
                        }
                }
                
                if (operation_mode != 6){ // add time scale
                        double tm = bram_window_length;
                        double ts = tm < 10. ? 1. : 1e-3;
                        tm *= ts;
                        double dt = AutoSkl(tm/10.);
                        double t0 = -0.1*tm + tm*bram_shift/1024.;
                        double t1 = t0+tm;
                        cursors->set_stroke_rgba (0.8,0.8,0.8,0.4);
                        reading->set_anchor (CAIRO_ANCHOR_CENTER);
                        for (double t=AutoNext(t0, dt); t <= t1; t += dt){
                                double x=scope_width*(t-t0)/tm; // time position tic pos
                                valuestring = g_strdup_printf ("%g %s", t*1000, ts > 0.9 ? "ms":"s");
                                reading->set_stroke_rgba (CAIRO_COLOR_WHITE);
                                reading->set_text (x, yr-8, valuestring);
                                g_free (valuestring);
                                reading->draw (cr);
                                cursors->set_xy_fast (0,x,-yr);
                                cursors->set_xy_fast (1,x,yr);
                                cursors->draw (cr);
                        }
                        cursors->set_stroke_rgba (0.4,0.1,0.4,0.4);
                        double x=scope_width*0.1; // time position of post trigger (excl. soft delays)
                        cursors->set_xy_fast (0,x,-yr);
                        cursors->set_xy_fast (1,x,yr);
                        cursors->draw (cr);
                }

                cursors->set_line_width (0.5);
                cursors->set_stroke_rgba (CAIRO_COLOR_WHITE);
                // coord cross
                cursors->set_xy_fast (0,xcenter+(scope_width*0.8)*(-0.5),0.);
                cursors->set_xy_fast (1,xcenter+(scope_width*0.8)*(0.5),0.);
                cursors->draw (cr);
                cursors->set_xy_fast (0,xcenter,y_hi);
                cursors->set_xy_fast (1,xcenter,-y_hi);
                cursors->draw (cr);

                // phase setpoint
                cursors->set_stroke_rgba (1.,0.,0.,0.5);
                cursors->set_xy_fast (0,xcenter+scope_width*(-0.5), deg_to_y (parameters.phase_fb_setpoint, y_hi));
                cursors->set_xy_fast (1,xcenter+scope_width*(0.5), deg_to_y (parameters.phase_fb_setpoint, y_hi));
                cursors->draw (cr);

                // phase setpoint
                cursors->set_stroke_rgba (0.,1.,0.,0.5);
                cursors->set_xy_fast (0,xcenter+scope_width*(-0.5), db_to_y (dB_from_mV (parameters.amplitude_fb_setpoint), dB_hi, y_hi, dB_mags));
                cursors->set_xy_fast (1,xcenter+scope_width*(0.5), db_to_y (dB_from_mV (parameters.amplitude_fb_setpoint), dB_hi, y_hi, dB_mags));
                cursors->draw (cr);

               
                if (operation_mode == 6){ // tune info
                        cursors->set_stroke_rgba (0.8,0.8,0.8,0.4);
                        reading->set_anchor (CAIRO_ANCHOR_CENTER);
                        for (double f=-0.9*parameters.tune_span/2.; f < 0.8*parameters.tune_span/2.; f += 0.1*parameters.tune_span/2.){
                                double x=xcenter+scope_width*f/parameters.tune_span; // tune plot, freq x transform to canvas
                                valuestring = g_strdup_printf ("%g", f);
                                reading->set_stroke_rgba (CAIRO_COLOR_WHITE);
                                reading->set_text (x, yr-8, valuestring);
                                g_free (valuestring);
                                reading->draw (cr);
                                cursors->set_xy_fast (0,x,-yr);
                                cursors->set_xy_fast (1,x,yr);
                                cursors->draw (cr);
                        }
                        reading->set_anchor (CAIRO_ANCHOR_W);
                        
                        if (debug_level > 0)
                                g_print ("Tune: %g Hz,  %g mV,  %g dB, %g deg\n",
                                         pacpll_signals.signal_frq[n-1],
                                         s,
                                         -20.*log (fabs(s)),
                                         pacpll_signals.signal_ch1[n-1]
                                         );

                        // current pos marks
                        cursors->set_stroke_rgba (CAIRO_COLOR_YELLOW);
                        x = xcenter+scope_width*pacpll_signals.signal_frq[n-1]/parameters.tune_span;
                        cursors->set_xy_fast (0,x,ydb-20.);
                        cursors->set_xy_fast (1,x,ydb+20.);
                        cursors->draw (cr);
                        cursors->set_xy_fast (0,x,yph-20);
                        cursors->set_xy_fast (1,x,yph+20);
                        cursors->draw (cr);

                        cursors->set_stroke_rgba (CAIRO_COLOR_GREEN);
                        x = xcenter+scope_width*(pacpll_parameters.center_frequency-pacpll_parameters.frequency_manual)/parameters.tune_span;
                        ydb=-y_hi*(20.*log10 (fabs (pacpll_parameters.center_amplitude)))/60.;
                        cursors->set_xy_fast (0,x,ydb-50.);
                        cursors->set_xy_fast (1,x,ydb+50.);
                        cursors->draw (cr);

                        cursors->set_stroke_rgba (CAIRO_COLOR_RED);
                        x = xcenter+scope_width*(pacpll_parameters.center_frequency-pacpll_parameters.frequency_manual)/parameters.tune_span;
                        ydb=-y_hi*pacpll_parameters.center_phase/180.;
                        cursors->set_xy_fast (0,x-50,ydb);
                        cursors->set_xy_fast (1,x+50,ydb);
                        cursors->draw (cr);

                        // Set Point Phase / F-Center Mark
                        cursors->set_stroke_rgba (CAIRO_COLOR_YELLOW);
                        x = xcenter+scope_width*(pacpll_parameters.center_frequency-pacpll_parameters.frequency_center)/parameters.tune_span;
                        ydb=-y_hi*pacpll_parameters.phase_fb_setpoint/180.;
                        cursors->set_xy_fast (0,x-50,ydb);
                        cursors->set_xy_fast (1,x+50,ydb);
                        cursors->draw (cr);
                        cursors->set_xy_fast (0,x,ydb-20);
                        cursors->set_xy_fast (1,x,ydb+20);
                        cursors->draw (cr);

                        g_free (cursors);

                        valuestring = g_strdup_printf ("Phase: %g deg", pacpll_signals.signal_phase[n-1]);
                        reading->set_stroke_rgba (CAIRO_COLOR_RED);
                        reading->set_text (10, -(110), valuestring);
                        g_free (valuestring);
                        reading->draw (cr);
                        valuestring = g_strdup_printf ("Amplitude: %g dB (%g mV)", dB_from_mV (pacpll_signals.signal_ampl[n-1]), pacpll_signals.signal_ampl[n-1] );
                        reading->set_stroke_rgba (CAIRO_COLOR_GREEN);
                        reading->set_text (10, -(110-14), valuestring);
                        g_free (valuestring);
                        reading->draw (cr);
                        
                        valuestring = g_strdup_printf ("Tuning: last peak @ %g mV  %.4f Hz  %g deg",
                                                       pacpll_parameters.center_amplitude,
                                                       pacpll_parameters.center_frequency,
                                                       pacpll_parameters.center_phase
                                                       );
                        reading->set_stroke_rgba (CAIRO_COLOR_WHITE);
                        reading->set_text (10, (110+14*0), valuestring);
                        g_free (valuestring);
                        reading->draw (cr);

                        // do FIT
                        if (!rs_mode){ // don't in RS tune mode, X is bad
                                double f[1024];
                                double a[1024];
                                double p[1024];
                                double ma[1024];
                                double mp[1024];
                                int fn=0;
                                for (int i=0; i<n; ++i){
                                        double fi = pacpll_signals.signal_frq[i];
                                        double ai = pacpll_signals.signal_ampl[i];
                                        double pi = pacpll_signals.signal_phase[i];
                                        if (fi != 0. && ai > 0.){
                                                f[fn] = fi+pacpll_parameters.frequency_manual;
                                                a[fn] = ai;
                                                p[fn] = pi;
                                                fn++;
                                        }
                                }
                                if (fn > 15){
                                        resonance_fit lmgeofit_res (f,a,ma, fn); // using the Levenberg-Marquardt method with geodesic acceleration. Using GSL here.
                                        // initial guess
                                        if (pacpll_parameters.center_frequency > 1000.){
                                                lmgeofit_res.set_F0 (pacpll_parameters.center_frequency);
                                                lmgeofit_res.set_A (1000.0/pacpll_parameters.center_amplitude);
                                        } else {
                                                lmgeofit_res.set_F0 (pacpll_parameters.frequency_manual);
                                                lmgeofit_res.set_A (1000.0/20.0);
                                        }
                                        lmgeofit_res.set_Q (5000.0);
                                        lmgeofit_res.execute_fit ();
                                        cairo_item_path *resfit = new cairo_item_path (fn);
                                        resfit->set_line_width (1.0);
                                        resfit->set_stroke_rgba (CAIRO_COLOR_MAGENTA);

                                        phase_fit lmgeofit_ph (f,p,mp, fn); // using the Levenberg-Marquardt method with geodesic acceleration. Using GSL here.
                                        // initial guess
                                        if (pacpll_parameters.center_frequency > 1000.){
                                                lmgeofit_ph.set_B (pacpll_parameters.center_frequency);
                                                lmgeofit_ph.set_A (1.0);
                                        } else {
                                                lmgeofit_ph.set_B (pacpll_parameters.frequency_manual);
                                                lmgeofit_ph.set_A (1.0);
                                        }
                                        lmgeofit_ph.set_C (60.0);
                                        lmgeofit_ph.execute_fit ();
                                        cairo_item_path *phfit = new cairo_item_path (fn);
                                        phfit->set_line_width (1.0);
                                        phfit->set_stroke_rgba (CAIRO_COLOR_MAGENTA);
                                        
                                        for (int i=0; i<fn; ++i){
                                                resfit->set_xy_fast (i,
                                                                     xcenter+scope_width*(f[i]-pacpll_parameters.frequency_manual)/parameters.tune_span, // tune plot, freq x transform to canvas
                                                                     ydb=db_to_y (dB_from_mV (ma[i]), dB_hi, y_hi, dB_mags)
                                                                     );

                                                phfit->set_xy_fast (i,
                                                                    xcenter+scope_width*(f[i]-pacpll_parameters.frequency_manual)/parameters.tune_span, // tune plot, freq x transform to canvas,
                                                                    yph=deg_to_y (mp[i], y_hi)
                                                                    );

                                                // g_print ("%05d \t %10.3f \t %8.3f %8.3f\n", i, f[i], a[i], m[i]);
                                        }
                                        resfit->draw (cr);
                                        phfit->draw (cr);
                                        delete resfit;
                                        delete phfit;

                                        resonator_frequency_fitted = lmgeofit_res.get_F0 ();
                                        resonator_phase_fitted = lmgeofit_ph.get_C ();
                                        
                                        valuestring = g_strdup_printf ("Model Q: %g  A: %g mV  F0: %g Hz  Phase: %g [%g] @ %g Hz",
                                                                       lmgeofit_res.get_Q (),
                                                                       1000./lmgeofit_res.get_A (),
                                                                       lmgeofit_res.get_F0 (),
                                                                       lmgeofit_ph.get_C (),
                                                                       lmgeofit_ph.get_A (),
                                                                       lmgeofit_ph.get_B ()
                                                                       );
                                        reading->set_stroke_rgba (CAIRO_COLOR_MAGENTA);
                                        reading->set_text (10, (110-14*1), valuestring);
                                        g_free (valuestring);
                                        reading->draw (cr);
                                }else {
                                        resonator_frequency_fitted = -1.; // invalid, no fit
                                        resonator_phase_fitted = -1.;
                                }
                        }
                        
                } else {
                        if (transport == 0 and scope_xy_on){ // add polar plot for CH1,2 as XY
                                wave->set_stroke_rgba (CAIRO_COLOR_MAGENTA);
                                for (int k=0; k<n; ++k)
                                        wave->set_xy_fast (k,xcenter-yr*scope_z[0]*gain_scale[0]*signal[0][k],-yr*scope_z[1]*gain_scale[1]*signal[1][k]);
                                wave->draw (cr);
                        } if (transport == 0 and scope_fft_on){ // add FFT plot for CH1,2
                                const unsigned int fftlen = FFT_LEN; // use first N samples from buffer
                                double tm = FFT_LEN/1024*bram_window_length;
                                double fm = FFT_LEN/tm/2.;
                                static double power_spectrum[2][FFT_LEN/2];
                                cairo_item_path *wave_psd = new cairo_item_path (fftlen/2);
                                wave_psd->set_line_width (1.0);
                                        
                                compute_fft (fftlen, &signal[0][0], &power_spectrum[0][0]);

                                wave_psd->set_stroke_rgba (CAIRO_COLOR_MAGENTA);
                                double xk0 = xcenter-0.4*scope_width;
                                double xkf = scope_width/(fftlen/2)*scope_fft_time_zoom;
                                int km=0;
                                double psdmx=0.;
                                for (int k=0; k<fftlen/2; ++k){
                                        wave_psd->set_xy_fast (k, xk0+xkf*(k-bram_shift)*scope_fft_time_zoom, db_to_y (dB_from_mV (scope_z[0]*power_spectrum[0][k]), dB_hi, y_hi, dB_mags));
                                        if (k==2){ // skip nyquist
                                                psdmx = power_spectrum[0][k];
                                                km = k;
                                        } else
                                                if (psdmx < power_spectrum[0][k]){
                                                        psdmx = power_spectrum[0][k];
                                                        km = k;
                                                }
                                }
                                wave_psd->draw (cr);
                                {
                                        double f=fm/scope_fft_time_zoom * km/(fftlen/2.0);
                                        double x=xk0+xkf*(km-bram_shift)*scope_fft_time_zoom;
                                        valuestring = g_strdup_printf ("%g mV @ %g %s", psdmx, f*1e-3, "kHz");
                                        reading->set_stroke_rgba (CAIRO_COLOR_MAGENTA);
                                        reading->set_text (x, db_to_y (dB_from_mV (scope_z[0]*psdmx), dB_hi, y_hi, dB_mags), valuestring);
                                        g_free (valuestring);
                                        reading->draw (cr);
                                }
                                compute_fft (fftlen, &signal[1][0], &power_spectrum[1][0]);
                                
                                wave_psd->set_stroke_rgba (CAIRO_COLOR_YELLOW);
                                for (int k=0; k<fftlen/2; ++k){
                                        wave_psd->set_xy_fast (k, xk0+xkf*(k-bram_shift)*scope_fft_time_zoom, db_to_y (dB_from_mV (scope_z[1]*power_spectrum[1][k]), dB_hi, y_hi, dB_mags));
                                        if (k==2){ // skip nyquist
                                                psdmx = power_spectrum[1][k];
                                                km = k;
                                        } else
                                                if (psdmx < power_spectrum[1][k]){
                                                        psdmx = power_spectrum[1][k];
                                                        km = k;
                                                }
                                }
                                wave_psd->draw (cr);
                                delete wave_psd;
                                {
                                        double f=fm/scope_fft_time_zoom * km/(fftlen/2.0);
                                        double x=xk0+xkf*(km-bram_shift)*scope_fft_time_zoom;
                                        valuestring = g_strdup_printf ("%g mV @ %g %s", psdmx, f*1e-3, "kHz");
                                        reading->set_stroke_rgba (CAIRO_COLOR_YELLOW);
                                        reading->set_text (x, db_to_y (dB_from_mV (scope_z[1]*psdmx), dB_hi, y_hi, dB_mags), valuestring);
                                        g_free (valuestring);
                                        reading->draw (cr);
                                }                                
                                // compute FFT frq scale

                                // Freq tics!
                                double df = AutoSkl(fm/10)/scope_fft_time_zoom;
                                double f0 = 0.0;
                                double fr = fm/scope_fft_time_zoom;
                                double x0=xk0-xkf*bram_shift*scope_fft_time_zoom;
                                cursors->set_stroke_rgba (CAIRO_COLOR_YELLOW_ID, 0.5);
                                reading->set_anchor (CAIRO_ANCHOR_CENTER);
                                for (double f=AutoNext(f0, df); f <= fr; f += df){
                                        double x=x0+scope_width*(f-f0)/fr; // freq position tic pos
                                        valuestring = g_strdup_printf ("%g %s", f*1e-3, "kHz");
                                        reading->set_stroke_rgba (CAIRO_COLOR_YELLOW);
                                        reading->set_text (x, yr-18, valuestring);
                                        g_free (valuestring);
                                        reading->draw (cr);
                                        cursors->set_xy_fast (0,x,-yr);
                                        cursors->set_xy_fast (1,x,yr);
                                        cursors->draw (cr);
                                }

                                // dB/Hz ticks
                                cursors->set_stroke_rgba (CAIRO_COLOR_MAGENTA_ID, 0.5);
                                for (int db=(int)dB_hi; db >= -20*dB_mags; db -= 10){
                                        double y;
                                        valuestring = g_strdup_printf ("%4d dB/Hz", db);
                                        reading->set_stroke_rgba (CAIRO_COLOR_MAGENTA);
                                        reading->set_text (scope_width - 70,  y=db_to_y ((double)db, dB_hi, y_hi, dB_mags), valuestring);
                                        g_free (valuestring);
                                        reading->draw (cr);
                                        cursors->set_xy_fast (0,xcenter+(scope_width)*(-0.5),y);
                                        cursors->set_xy_fast (1,xcenter+(scope_width*0.7)*(0.5),y);
                                        cursors->draw (cr);
                                }
                        }
                }
                delete wave;
                delete reading;
                if (binwave8bit[0])
                        for (int bit=0; bit<binary_BITS; ++bit)
                                delete binwave8bit[bit];
        } else {
                deg_extend = 1;
                // minimize
                gtk_drawing_area_set_content_width (area, 128);
                gtk_drawing_area_set_content_height (area, 4);
                current_width=0;
        }
}

/*  SHM memory block for external apps providing all monitors in binary double
 *
### Python snippet to read XYZMaMi monitors:

import requests
import numpy as np
from multiprocessing import shared_memory
from multiprocessing.resource_tracker import unregister

xyz_shm = shared_memory.SharedMemory(name='gxsm4rpspmc_monitors')
unregister(xyz_shm._name, 'shared_memory') ## necessary to prevent python to destroy this shm block at exit :( :(

print (xyz_shm)
xyz=np.ndarray((9,), dtype=np.double, buffer=xyz_shm.buf).reshape((3,3)).T  # X Mi Ma, Y Mi Ma, Z Mi Ma
print (xyz)

 *  
 */



/* stolen from app_remote.C */
static void Check_ec(Gtk_EntryControl* ec, remote_args* ra){
	ec->CheckRemoteCmd (ra); // only reading PCS is thread safe!
};

static void Check_conf(GnomeResPreferences* grp, remote_args* ra){
        if (grp && ra)
                gnome_res_check_remote_command (grp, ra);
};


void remote_set_ra (const gchar* id, const gchar* value)
{
        remote_args ra; 
	gchar *list[] = { (char *)"set", id, value, NULL };
	ra.arglist = list;

        // check PCS entries
	g_slist_foreach (main_get_gapp()->RemoteEntryList, (GFunc) Check_ec, (gpointer)&ra);

        // check current active/open CONFIGURE elements
	g_slist_foreach (main_get_gapp()->RemoteConfigureList, (GFunc) Check_conf, (gpointer)&ra);
}

double remote_get_ra (const gchar *id){
	remote_args ra;
	ra.qvalue = 0.;
	gchar *list[] = {(gchar *)"get", id, NULL};
	ra.arglist = list;

	g_slist_foreach(main_get_gapp()->RemoteEntryList, (GFunc) Check_ec, (gpointer)&ra);
	PI_DEBUG(DBG_L2, " query result: " << ra.qvalue << ra.qstr);

        if (ra.qstr)
                return atof (ra.qstr);
        else
                return ra.qvalue;
}


gboolean RPSPMC_Control::check_shm_action_idle_callback(gpointer self){
        if (rpspmc_pacpll->update_shm_monitors (1)) // manage SHM ACTIONS only
                return G_SOURCE_CONTINUE;
        else
                return G_SOURCE_REMOVE;
}

gboolean RPspmc_pacpll::update_shm_monitors (int ctrl, int close_shm){
        const char *rpspmc_monitors = "/gxsm4rpspmc_monitors";
        static int shm_fd = -1;
        static void *shm_ptr = NULL;
        // Set the size of the shared memory region
        static size_t shm_size = sizeof(double)*512;
        static gboolean keep_alife=true;
        
        if (!keep_alife)
                return true;
        
        if (shm_fd == -1){
                shm_fd = shm_open(rpspmc_monitors, O_CREAT | O_RDWR, 0666);
                if (shm_fd == -1) {
                        g_error ("Error shm_open of %s.", rpspmc_monitors);
                        return false;
                }
                
                // Resize the shared memory object to the desired size
                if (ftruncate (shm_fd, shm_size) == -1) {
                        g_error ("Error ftruncate of %s.", rpspmc_monitors);
                        return false;
                }

                // Map the shared memory object into the process address space
                shm_ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
                if (shm_ptr == MAP_FAILED) {
                        g_error("Error mmap");
                        return false;
                }
        }

        if (close_shm){
                keep_alife=false;

                g_message ("Closing SHM. End of Serving actions.");
                
                // Unmap the shared memory object
                if (munmap(shm_ptr, shm_size) == -1) {
                        g_error("Error munmap");
                        return;
                }
                // Close the shared memory file descriptor
                if (close(shm_fd) == -1) {
                        g_error("Error close");
                        return 1;
                }
                
                // Unlink the shared memory object
                if (shm_unlink(rpspmc_monitors) == -1) {
                        g_error("Error shm_unlink");
                        return 1;
                }
                shm_ptr = NULL;
                shm_fd = -1;
                return false;
        }

        
        if (ctrl == 0){
                spmc_signals.xyz_meter[9] = (double)g_get_real_time ();
                
                // Write data to the shared memory

                // RPSPMC XYZ MAX MIN (3x3)
                memcpy  (shm_ptr, spmc_signals.xyz_meter, sizeof(gint64)+sizeof(spmc_signals.xyz_meter));

                // RPSPMC Monitors: Bias, reg, set,   GPVU,A,B,AM,FM, MUX, Signal (Current), AD463x[2], XYZ, XYZ0, XYZS
                memcpy  (shm_ptr+10*sizeof(double), &spmc_parameters.bias_monitor, 21*sizeof(double));

                // PAC-PLL Monitors: dc-offset, exec_ampl_mon, dds_freq_mon, volume_mon, phase_mon, control_dfreq_mon
                memcpy  (shm_ptr+40*sizeof(double), &pacpll_parameters.dc_offset, 7*sizeof(double));

                // FPGA RPSPMC uptime in sec, 8ns resolution -- i.e. exact time of last reading
                memcpy  (shm_ptr+100*sizeof(double), &spmc_parameters.uptime_seconds, sizeof(double));

                // push history
                rpspmc_hwi->push_history_vector (shm_ptr, 48); // currently used: 0..8, 10..31, 40..47 as size of double

                g_message ("SHM MONITOR UPDATE @RPSPMC TIME: %.9fs", spmc_parameters.uptime_seconds);
                
                /*
                sprintf (shm_ptr+512, "XYZ=[[%g %g %g] [%g %g %g] [%g %g %g]]\n",
                         spmc_signals.xyz_meter[0],spmc_signals.xyz_meter[1],spmc_signals.xyz_meter[2],
                         spmc_signals.xyz_meter[3],spmc_signals.xyz_meter[4],spmc_signals.xyz_meter[5],
                         spmc_signals.xyz_meter[6],spmc_signals.xyz_meter[7],spmc_signals.xyz_meter[8]
                         );
                */
        } else {
        
#ifdef ENABLE_SHM_ACTIONS
                // SHM GET/SET hack tests
                // TEST READ / ACTION

                // ACTION FUZZY LEVEL Z CONTROL:
                // SHM[128] := 1 ---> Level=0 (Auto/Regular Feedback); completed when SHM[128] := 0
                // SHM[128] := 2 ---> Level=Current-Setpoint (Adjust Z to Z-Setpoint if Current Set Point is not exceeded, i.e. const Z mode); completed when SHM[128] := 0

                // ACTIONS
#define SHM_ACTION_IDLE        0x0000000
#define SHM_ACTION_CTRL_Z_SET  0x0000010
#define SHM_ACTION_CTRL_Z_FB   0x0000011
#define SHM_ACTION_GET         0x0000020
#define SHM_ACTION_SET         0x0000021

                // SHM MEMORY LOCATION ASIGNMENTS
#define SHM_ACTION_OFFSET           128
#define SHM_ACTION_VPAR_64_START    130
#define SHM_ACTION_FPN(N)           (SHM_ACTION_VPAR_64_START+N) // max 10
#define SHM_ACTION_FRET_64_START    140
#define SHM_ACTION_FRETN(N)         (SHM_ACTION_FRET_64_START+N) // max 10

#define SHM_ACTION_FP_ID_STRING_64  150 // max 64

#define SHM_ADR(OFFSET) (shm_ptr+(OFFSET)*sizeof(double))
        
                guint64 action_request = *(guint64*)(shm_ptr+SHM_ACTION_OFFSET*sizeof(guint64));

                gchar *sval=NULL;
                switch ((int)action_request){
                case SHM_ACTION_IDLE:
                        break;  // NOP

                case SHM_ACTION_CTRL_Z_FB:    // void CTRL_Z_FB(void)
                        sval = g_strdup_printf("0");
                        *(double*)SHM_ADR(SHM_ACTION_FRETN(9)) = 0.0; // OK
                        g_message ("SHM-ACTION-CTRL-Z-FB {%08x}", (int)action_request);
                        // ... this continues below intentional no "break;" here!
                case SHM_ACTION_CTRL_Z_SET: { // void CTRL_Z_SET(void)
                        gchar *id_cl="dsp-fbs-mx0-current-level";
                        gchar *id_cs="dsp-fbs-mx0-current-set";
                        if (!sval){
                                sval=g_strdup_printf("%g", remote_get_ra (id_cs));
                                g_message ("SHM-ACTION-CTRL-Z-SET {%08x}", (int)action_request);
                        }
                        remote_set_ra (id_cl, sval); // set level to 0 or current setpoint as of action request
                        g_free (sval);
                        *(double*)SHM_ADR(SHM_ACTION_FRETN(9)) = 0.0; // OK
                        break;
                }
                case SHM_ACTION_SET: {       // void SET (SHM_ACTION_FP_ID_STRING_64 ID, SHM_ACTION_FPN(0) Value)
                        // GXSM.SET function
                        gchar id_set[64]; memset (id_set, 0, sizeof(id_set));
                        memcpy (id_set, SHM_ADR(SHM_ACTION_FP_ID_STRING_64), sizeof(id_set));
                        id_set[63]=0; // safety termination
                        double v = *(double*)SHM_ADR(SHM_ACTION_FPN(0));
                        g_message ("SHM-ACTION-SET VALUE {%08x} ** id=%s := %g", (int)action_request, id_set, v);
                        gchar *sval_cs=g_strdup_printf("%g", v);
                        remote_set_ra (id_set, sval_cs);
                        g_free (sval_cs);
                        *(double*)SHM_ADR(SHM_ACTION_FRETN(9)) = 0.0; // OK
                        break;
                }
                case SHM_ACTION_GET: {       // (SHM_ADR(SHM_ACTION_FRETN(0))) GET (SHM_ACTION_FP_ID_STRING_64 ID)
                        // GXSM.GET function
                        gchar id_get[64]; memset (id_get, 0, sizeof(id_get));
                        memcpy (id_get, SHM_ADR(SHM_ACTION_FP_ID_STRING_64), sizeof(id_get));
                        id_get[63]=0; // safety termination
                        double v=remote_get_ra (id_get);
                        g_message ("SHM-ACTION-GET VALUE {%08x} ** id=%s => %g", (int)action_request, id_get, v);
                        *(double*)SHM_ADR(SHM_ACTION_FRETN(0)) = v; // return value
                        *(double*)SHM_ADR(SHM_ACTION_FRETN(9)) = 0.0; // OK
                        break;
                }

                default:
                        *(double*)SHM_ADR(SHM_ACTION_FRETN(9)) = -1; // return ERROR in RET9
                        g_message ("SHM-ACTION* INVALID REQUEST {%08x}", (int)action_request);
                        break;
                }

                *(guint64*)(shm_ptr+SHM_ACTION_OFFSET*sizeof(guint64)) = 0L; // clear action control
        
#endif
        }        
        
        return true;
}

// END
