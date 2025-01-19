/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,..2023 Percy Zahl
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

#pragma once

// ************* RPSPMC + PACPLL FPGA GPIO CONFIGURATION MAPPINGS

#define PACPLL_CFG0_OFFSET 0
#define PACPLL_CFG1_OFFSET 32
#define PACPLL_CFG2_OFFSET 64
#define PACPLL_CFG3_OFFSET 128

// ===== PACPLL

// CONFIGURATION (CFG) DATA REGISTER 0 [1023:0] x 4 = 4k
// PAC-PLL Control Core

// ===== CFG0 =====
// PACPLL core controls
#define PACPLL_CFG_DDS_PHASEINC  0    // 64bit wide
#define PACPLL_CFG_VOLUME_SINE   2    // 32bit wide (default)
#define PACPLL_CFG_CONTROL_LOOPS 3

#define PACPLL_CFG_PACTAU        4 // (actual Q22 mu)
#define PACPLL_CFG_DC_OFFSET     5


#define PACPLL_CFG_TRANSPORT_CONTROL         6 // function bits masks in pacpll.h ...RESET/STRAT/SINGLE
#define PACPLL_CFG_TRANSPORT_SAMPLES         7
#define PACPLL_CFG_TRANSPORT_DECIMATION      8
#define PACPLL_CFG_TRANSPORT_CHANNEL_SELECT  9

// PACPLL Controller Core (Servos) Relative Block Offsets:
#define PACPLL_CFG_SET   0
#define PACPLL_CFG_CP    1
#define PACPLL_CFG_CI    2
#define PACPLL_CFG_UPPER 3 // 3,4 64bit
#define PACPLL_CFG_LOWER 5 // 5,6 64bit

// [CFG0]+10 AMPL Controller
// [CFG0]+20 PHASE Controller
// [CFG1]+00 dFREQ Controller   
// +0 Set, +2 CP, +4 CI, +6 UPPER, +8 LOWER
#define PACPLL_CFG_PHASE_CONTROLLER     10 //10:16 (10,11,12,13:14,15:16)
// 17,18,19 -
#define PACPLL_CFG_AMPLITUDE_CONTROLLER 20 //20:26 (20,21,22,23:24,25:26)

#define PACPLL_CFG_PACATAU      27
#define PACPLL_CFG_PAC_DCTAU    28

#define QCONTROL_CFG_GAIN_DELAY 29

// general control paging (future options)
//#define PACPLL_CFG_PAGE_CONTROL  30   // 32bit wide
//#define PACPLL_CFG_PAGE          31   // 32bit wide



// CFG DATA REGISTER 1 [1023:0]

// ===== CFG1 =====

#define PACPLL_CFG_DFREQ_CONTROLLER         (PACPLL_CFG1_OFFSET + 0) // 00:06
#define PACPLL_CFG_PHASE_CONTROL_THREASHOLD (PACPLL_CFG1_OFFSET + 8) // 08

// + 9 avilable

// +0 Set, +2 CP, +4 CI, +6 UPPER, +8 LOWER
// Controller Core (Servos) Relative Block Offsets:
#define SPMC_CFG_SET   0
#define SPMC_CFG_CP    1
#define SPMC_CFG_CI    2
#define SPMC_CFG_UPPER 3 // 3,4 64bit
#define SPMC_CFG_LOWER 5 // 5,6 64bit


/* MOVED TO MODULE CONFIG
// SPM Z-CONTROL SERVO
#define SPMC_CFG_Z_SERVO_CONTROLLER         (PACPLL_CFG1_OFFSET + 10) // 10:16
#define SPMC_CFG_Z_SERVO_ZSETPOINT          (PACPLL_CFG1_OFFSET + 17) // 17
#define SPMC_CFG_Z_SERVO_LEVEL              (PACPLL_CFG1_OFFSET + 18) // 18 -- kinda obsolete -> program limits

// CONTROL REGISTER 19:
// Bit0: SERVO CONTROL (enable)
// Bit1: LN (log mode) if set: Ln (ABS (INPUT 32bit)) in 8.24 Fractional (32bit) MUST CONVERT SETPOINT ACCORDINGLY!!!, else linear (32bit signed)
// Bit2: Fuzzy CZ mode
#define SPMC_CFG_Z_SERVO_MODE               (PACPLL_CFG1_OFFSET + 19) // 19: SERVO CONTROL REGISTER


#define SPMC_CFG_SC_LCK_DDS_PHASEINC        (PACPLL_CFG1_OFFSET + 20) // 20,21: Frequency 64bit LockIn (DDS Phase Inc)
#define SPMC_CFG_SC_LCK_VOLUME              (PACPLL_CFG1_OFFSET + 22) // 22: SC Volume 
#define SPMC_CFG_SC_LCK_TARGET              (PACPLL_CFG1_OFFSET + 23) // 23: Target for mixing 1..4 so far, 0=NONE / OFF
*/

// CFG1 26...31 available


// CFG DATA REGISTER 2 [1023:0]

// ===== CFG2 =====

// PACPLL-PulseFormer Control Core

#define PACPLL_CFG_PULSE_FORM_BASE      (PACPLL_CFG2_OFFSET + 0)
#define PACPLL_CFG_PULSE_FORM_DELAY_01  0 // [ 31..16 Delay P0, 15..0 Delay P1 ] 32bit
#define PACPLL_CFG_PULSE_FORM_WH01_ARR  1 // [ 31..16 Width_n P0, 15..0 Width_n P1; 31..16 Height_n P0, 15..0 Height_n P1, ... [n=0,1,2]] 14x32 bit
// pulse_12_width_height_array[ 1*32-1 : 0];    // 0W 1,2 =pWIF
// pulse_12_width_height_array[ 2*32-1 : 1*32]; // 0H 1,2 =pIHF
// pulse_12_width_height_array[ 3*32-1 : 2*32]; // 1W =pWIF
// pulse_12_width_height_array[ 4*32-1 : 3*32]; // 1H =pWIF
// pulse_12_width_height_array[ 5*32-1 : 4*32]; // 2W =WIF
// pulse_12_width_height_array[ 6*32-1 : 5*32]; // 2H =HIF
// pulse_12_width_height_array[ 7*32-1 : 6*32]; // 3W =pW
// pulse_12_width_height_array[ 8*32-1 : 7*32]; // 3H =pH
// pulse_12_width_height_array[ 9*32-1 : 8*32]; // 4W =pWIF
// pulse_12_width_height_array[10*32-1 : 9*32]; // 4H =pHIF
// pulse_12_width_height_array[11*32-1 :10*32]; // 5W =WIF
// pulse_12_width_height_array[12*32-1 :11*32]; // 5H =HIF
// pulse_12_width_height_array[13*32-1 :12*32]; // Bias pre
// pulse_12_width_height_array[14*32-1 :13*32]; // Bias post



#if 0
// ---------- NOT IMPLEMENTED/NO ROOM left / Z10  ---------------------
// using fixed length FIR filters 
// Configure transport tau: time const or high speed IIR filter stages
#define PACPLL_CFG_TRANSPORT_TAU_DFREQ   (PACPLL_CFG1_OFFSET + 0)
#define PACPLL_CFG_TRANSPORT_TAU_PHASE   (PACPLL_CFG1_OFFSET + 1)
#define PACPLL_CFG_TRANSPORT_TAU_EXEC    (PACPLL_CFG1_OFFSET + 2)
#define PACPLL_CFG_TRANSPORT_TAU_AMPL    (PACPLL_CFG1_OFFSET + 3)
#endif



// ===== SPMC

// CFG DATA REGISTER 3 [1023:0]

// ===== CFG3 =====

// SPMControl Core, GVP

#define SPMC_BASE                     PACPLL_CFG3_OFFSET

// SPMC CONGIGUARTION BUS SYSTEM at
#define SPMC_MODULE_CONFIG_ADDR    (SPMC_BASE + 0) // COMPLEX MODULE CONFIGURATION ADDRESS, (SLOW/OCCASIOANLLY). 0 := NON, 1: GVP, ...
#define SPMC_MODULE_CONFIG_DATA    (SPMC_BASE + 1) // CONFIGURATION DATA VECTOR 1..16 // 512 bits (16x32)

// NOTHING at CONFIG ADDR = 0 => standy/disabled

// GVP MODULE AT CONFIGURATION ADDRs  1,2,3,4
#define SPMC_GVP_CONTROL_REG          5001   // CONTROL CONFIG REG: B0: reset, B1: Pause
#define SPMC_GVP_RESET_OPTIONS_REG    5002   // RESET OPTIONS REG:  reset options
#define SPMC_GVP_VECTOR_DATA_REG      5003   // VECTOR PROGRAM REG: 1..16 // 512 bits (16x32)
#define SPMC_GVP_RESET_VECTOR_REG     5004   // [XYZ]UAB
#define SPMC_GVP_VECTORX_DATA_REG     5005   // VECTORX PROGRAM REG -- Vector Extension Components

// GVP VCETOR COMPONETS IN ARRAY AT OFFESTS in CONFIG_REG (512bits)
//                   decii      du        dz        dy        dx     Next       Nrep,   Options,     nii,      N,    [Vadr]
#define GVP_VEC_VADR   0
#define GVP_VEC_N      1
#define GVP_VEC_NII    2
#define GVP_VEC_OPT    3
//#define GVP_VEC_SRC    4
#define GVP_VEC_NREP   4
#define GVP_VEC_NEXT   5
#define GVP_VEC_DX     6
#define GVP_VEC_DY     7
#define GVP_VEC_DZ     8
#define GVP_VEC_DU     9
#define GVP_VEC_DA    10
#define GVP_VEC_DB    11
#define GVP_VEC_012   12
#define GVP_VEC_013   13
#define GVP_VEC_014   14
#define GVP_VEC_DECII 15
#define GVP_VECX_SRC   0


// Z_SERVO @ CONFIG ADDRESS
#define SPMC_Z_SERVO_CONTROL_REG              100
#define SPMC_Z_SERVO_MODE_CONTROL_REG         101
/*
        z_servo_control_reg_address:
        begin
            r_control_setpoint <= config_data[1*32-1 : 0*32];
            r_cp               <= config_data[2*32-1 : 1*32];
            r_ci               <= config_data[3*32-1 : 2*32];
            r_upper            <= config_data[4*32-1 : 3*32];
            r_lower            <= config_data[5*32-1 : 4*32];
        end   
          
        z_servo_modes_reg_address:
        begin
            r_signal_offset    <= config_data[1*32-1 : 0*32];
            r_control_signal_offset <= config_data[2*32-1 : 1*32]; // normally 0 
            r_z_setpoint       <= config_data[3*32-1 : 2*32];
            r_transfer_mode    <= config_data[4*32-1 : 3*32];
        end     
        endcase
*/
        
// LOCKIN MODULE @ CONFIG ADDRESS
#define SPMC_LOCKIN_F0_CONTROL_REG      1000 // [0] LockIn Config Reg (Gain Control, B0: enable gain AXIS, B1: progammed gain Q24)
                                             // [1] Fixed Gain, Q24
                                             // [2] PhaseInc [PH48, DDSN2_16]

// BIQUAD  MODULE @ CONFIG ADDRESS
#define SPMC_BIQUAD_F0_CONTROL_REG      1001 //  BiQuad Parameters b0, b1, b2, a0, a1

#define SPMC_MAIN_CONTROL_XYZU_OFFSET_REG  1100 //  SPMC MAIN CONTROL IP REGISTERS
#define SPMC_MAIN_CONTROL_ROTM_REG         1101 //  SPMC MAIN CONTROL IP REGISTERS
#define SPMC_MAIN_CONTROL_SLOPE_REG        1102 //  SPMC MAIN CONTROL IP REGISTERS
#define SPMC_MAIN_CONTROL_MODULATION_REG   1103 //  SPMC MAIN CONTROL IP REGISTERS
#define SPMC_MAIN_CONTROL_BIAS_REG         1104 //  SPMC MAIN CONTROL IP REGISTERS -- Gxsm set Bias onyl (U), included in XYZU as well
/*
        case (config_addr) // BQ configuration, and auto reset
        xyzu_offset_reg_address: // == 1100
        begin
            // SCAN OFFSET / POSITION COMPONENTS, ABSOLUTE COORDS
            x0 <= config_data[1*32-1 : 0*32]; // vector components
            y0 <= config_data[2*32-1 : 1*32]; // ..
            z0 <= config_data[3*32-1 : 2*32]; // ..
            u0 <= config_data[4*32-1 : 3*32]; // Bias Reference
            
            xy_offset_step <= config_data[5*32-1 : 4*32]; // @Q31 => Q31 / 120M => [18 sec full scale swin @ step 1 decii = 0]  x RDECI
            z_offset_step  <= config_data[6*32-1 : 5*32]; // @Q31 => Q31 / 120M => [18 sec full scale swin @ step 1 decii = 0]  x RDECI
        end

        rotm_reg_address: // == 1101
        begin
            // scan rotation (yx=-xy, yy=xx)
            rotmxx <= config_data[1*32-1 : 0*32]; // =cos(alpha)
            rotmxy <= config_data[2*32-1 : 1*32]; // =sin(alpha)
        end

        slope_reg_address: // == 1102
        begin
            // slope -- always applied in global XY plane ???
            slope_x <= config_data[1*32-1 : 0*32]; // SQSLOPE (31)
            slope_y <= config_data[2*32-1 : 1*32]; // SQSLOPE (31)
        end

        modulation_reg_address: // == 1103
        begin
            // modulation control
            modulation_volume <= config_data[11*32-1 : 10*32]; // volume for modulation Q31
            modulation_target <= config_data[12*32-1 : 11*32]; // target signal for mod (#XYZUAB)
        end
        endcase
*/

// 16 to 6 MUX to SRCS-STREAMING  MODULE @ CONFIG ADDRESS
#define SPMC_MUX16_6_SRCS_CONTROL_REG   2000 // MUX selection, test mode, test value.
                                             // GVP SRCS selections: 16->6 -|-|4|4|4|4|4|4


// MODULE READBACK REGISTER A,B ADRESS MAPPINGS
#define SPMC_READBACK_Z_REG            100001
#define SPMC_READBACK_BIAS_REG         100002 // Bias Sum, Gxsm Bias "U0" Set
#define SPMC_READBACK_GVPBIAS_REG      100003 // Bias GVP Comp., Bias MOD (DBG: currently = GVP-A)
#define SPMC_READBACK_MUX_REG          100004 // SRCS-MUX, --
#define SPMC_READBACK_XX_REG           100999 // DBG: SrcsMUX sel, GVP-B

#define SPMC_READBACK_TEST_RESET_REG           102000
#define SPMC_READBACK_TEST_VALUE_REG           101999

/*
    input wire [32-1:0] Z_GVP_mon,   ==> A
    input wire [32-1:0] Z_slope_mon, ==> B

    input wire [32-1:0] Bias_mon,    ==> A
    input wire [32-1:0] Bias_GVP_mon,==> B

    input wire [32-1:0] rbXa, ==> A
    input wire [32-1:0] rbXb  ==> B
*/

// ****** FIXED CONFIGURATIONS

#define SPMC_CFG_AD5791_DAC_AXIS_DATA (SPMC_BASE + 17) // 32bits
#define SPMC_CFG_AD5791_DAC_CONTROL   (SPMC_BASE + 18) // bits 0,1,2: axis; bit 3: config mode; bit 4: send config data, MUST reset "send bit in config mode to resend next, on hold between"


/* MOVED TO MODULE CONFIG BUS
// SPMC Transformations Core
#define SPMC_ROTM_XX             (SPMC_BASE + 20)  // cos(Alpha)
#define SPMC_ROTM_XY             (SPMC_BASE + 21)  // sin(Alpha)
//#define SPMC_ROTM_YX = -XY = -sin(Alpha) // calculated on FPGA
//#define SPMC_ROTM_YY =  XX =  cos(Alpha) // calculated on FPGA

#define SPMC_SLOPE_X             (SPMC_BASE + 22)
#define SPMC_SLOPE_Y             (SPMC_BASE + 23)
#define SPMC_OFFSET_X            (SPMC_BASE + 24)
#define SPMC_OFFSET_Y            (SPMC_BASE + 25)
#define SPMC_OFFSET_Z            (SPMC_BASE + 26)
#define SPMC_BIAS_REF            (SPMC_BASE + 27)

#define SPMC_XY_MOVE_STEP        (SPMC_BASE + 28)
#define SPMC_Z_MOVE_STEP         (SPMC_BASE + 29)
*/





// ************* BASIC DEFINITIONS AND CONSTANTS



// SPMC
#define SPMC_ACLK_MHZ   125 // RP Analog Clock Base in MHz
#define SPMC_RDECI      4
#define SPMC_CLK        ((double)SPMC_ACLK_MHZ*1e6/(1<<(SPMC_RDECI+1)))
#define SPMC_GVP_MIN_DECII 2           // fastest
#define SPMC_GVP_REGULAR_MIN_DECII 128 // about 1MHz max for ADCs
#define SPMC_GVP_CLK    ((double)SPMC_ACLK_MHZ*1e6) // /2 for decii i noutside block only (old)

#define MAX_NUM_PROGRAN_VECTORS 32
#define Q_XY_PRECISION Q28
#define Q_Z_SLOPE_PRECISION Q31

#define SPMC_IN01_REFV   1.13 // RP IN1,2 REF Volatge is 1.0V (+/-1V Range)
#define SPMC_AD5791_REFV 5.0 // DAC AD5791 Reference Volatge is 5.000000V (+/-5V Range)
#define QZSCOEF Q31 // Q Z-Servo Controller



#define QN(N) ((1<<(N))-1)
#define QN64(N) ((1LL<<(N))-1)

#define Q22 QN(22)
#define Q23 QN(23)
#define Q28 QN(28)
#define Q24 QN(24)
#define Q20 QN(20)
#define Q19 QN(19)
#define Q16 QN(16)
#define Q15 QN(15)
#define Q13 QN(13)
#define Q12 QN(12)
#define Q10 QN(10)
#define Q31 0x7FFFFFFF  // (1<<31)-1 ... ov in expression expansion
#define Q32 0xFFFFFFFF  // (1<<32)-1 ... ov in expression expansion
#define Q40 QN64(40)
#define Q36 QN64(36)
#define Q37 QN64(37)
#define Q47 QN64(47)
#define Q44 QN64(44)
#define Q48 QN64(48)

// for PhaseInc I need
#define QQ44 (1LL<<44)


typedef unsigned short guint16;
typedef short gint16;


#ifdef __cplusplus
extern "C" {
#endif

  uint8_t* cfg_reg_adr(int cfg_slot);
  void set_gpio_cfgreg_int32 (int cfg_slot, int value);
  void set_gpio_cfgreg_uint32 (int cfg_slot, unsigned int value);
  void set_gpio_cfgreg_int16_int16 (int cfg_slot, gint16 value_1, gint16 value_2);
  void set_gpio_cfgreg_uint16_uint16 (int cfg_slot, guint16 value_1, guint16 value_2);
  void set_gpio_cfgreg_int64 (int cfg_slot, long long value);
  void set_gpio_cfgreg_int48 (int cfg_slot, unsigned long long value);
  int32_t read_gpio_reg_int32_t (int gpio_block, int pos);
  int read_gpio_reg_int32 (int gpio_block, int pos);
  unsigned int read_gpio_reg_uint32 (int gpio_block, int pos);
  
#ifdef __cplusplus
}
#endif
