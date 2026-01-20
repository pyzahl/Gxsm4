/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gnome gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * Gxsm Plugin Name: rpspmc_signals.cpp
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

// This is included into rpspmc_control.cpp ONLY.

// Here defined are.
//=======================================================
//extern SOURCE_SIGNAL_DEF rpspmc_source_signals[];
//extern SOURCE_SIGNAL_DEF rpspmc_swappable_signals[];
//extern SOURCE_SIGNAL_DEF modulation_targets[];
//extern SOURCE_SIGNAL_DEF z_servo_current_source[];
//extern SOURCE_SIGNAL_DEF rf_gen_out_dest[];
//=======================================================

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

// END
