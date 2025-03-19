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


// CONFIGURATION (CFG) DATA REGISTER 0 [1023:0] x 4 = 4k

// ===== SPMC
// ===== PACPLL

// SPMControl Core, GVP

#define SPMC_BASE   0   // CFG_REG_OFFSET   [2048-1:0]

// SPMC CONGIGUARTION BUS SYSTEM at
#define SPMC_MODULE_CONFIG_ADDR    (SPMC_BASE + 0) // COMPLEX MODULE CONFIGURATION ADDRESS, (SLOW/OCCASIOANLLY). 0 := NON, 1: GVP, ...
#define SPMC_MODULE_CONFIG_DATA    (SPMC_BASE + 1) // CONFIGURATION DATA VECTOR 1..16 // 512 bits (16x32)

// NOTHING at CONFIG ADDR = 0 => standy/disabled

// MODULE CONFIGURATION ** NEW
// MODULE PHASE_AMPLITUDE_DETECTOR
#define RESET_LMS_PHASE_AMPLITUDE_DETECTOR_ADDRESS  19999
#define LMS_PHASE_AMPLITUDE_DETECTOR_ADDRESS        20000
#define LMS_PACPLL_CFG_PACTAU        0 // (actual Q22 mu) Phase mu
#define LMS_PACPLL_CFG_PACATAU       1 // (actual Q22 mu) Amplitude mu
#define LMS_PACPLL_CFG_LCK_AM_PH     2 // Bit0: Ampl Bit1: Phase (if enabled in build, option to use LockIn)

// MODULE AMPLITUDE CONTROLLER
#define AMPLITUDE_CONTROLLER_ADDRESS   20002 // C32/L16/S24
#define AMPLITUDE_CONTROLLER_M_ADDRESS 20001 // L16

// MODULE PHASE CONTROLLER
#define PHASE_CONTROLLER_ADDRESS       20004 // C32/L48/S32
#define PHASE_CONTROLLER_M_ADDRESS     20003 // L48/T24

// MODULE DFREQ CONTROLLER
#define DFREQ_CONTROLLER_ADDRESS       20006 // C32/L32/S32
#define DFREQ_CONTROLLER_M_ADDRESS     20005 // L32

// CONTROLLER CONFIG MODULE POSITIONS
// REG ADDRESS
#define CONTROLLER_SETPOINT     0  // [width_setpoint]
#define CONTROLLER_CP           1  // [width_const]
#define CONTROLLER_CI           2  // [width_const]
#define CONTROLLER_UPPER        3  // [width_limits] ** limit regs are 64bit
#define CONTROLLER_LOWER        5  // [width_limits]
// REG_M ADDRESS
#define CONTROLLER_M_RESET_VAL  0  // [width_limits]
#define CONTROLLER_M_MODE       2  // [32]
#define CONTROLLER_M_THREASHOLD 4  // [threshold_limits]

// MODULE PULSE FORMER
#define PULSE_FORMER_DL_ADDRESS  20010
#define PULSE_FORMER_WH_ADDRESS  20011

// MODULE TRANSPORT 4S COMBINE
#define TRANSPORT_4S_COMBINE_ADDRESS 20020
#define TRANSPORT_CHANNEL_SELECTOR  0  // 4-1:0
#define TRANSPORT_NDECIMATE         1
#define TRANSPORT_NSAMPLES          2
#define TRANSPORT_OPERATION         3
#define TRANSPORT_SHIFT             4
#define TRANSPORT_FREQ_CENTER       5  // 48-1:0

// MODULE Q-CONTROL
#define QCONTROL_ADDRESS         20030
#define QCONTROL_ENABLE              0
#define QCONTROL_GAIN                1
#define QCONTROL_DELAY               2


// MODULE DC_FILTER
#define DC_FILTER_ADDRESS        20040
#define DC_FILTER_TAU                0
#define DC_FILTER_OFFSET             1


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
#define GVP_VEC_DAM   12
#define GVP_VEC_DFM   13
#define GVP_VEC_014   14
#define GVP_VEC_DECII 15
#define GVP_VECX_SRC   0


// Z_SERVO @ CONFIG ADDRESS
#define SPMC_Z_SERVO_CONTROL_REG              100
#define SPMC_Z_SERVO_MODE_CONTROL_REG         101
#define SPMC_Z_SERVO_SELECT_RB_SETPOINT_MODES_REG   110 // readback selection
#define SPMC_Z_SERVO_SELECT_RB_CPI_REG              111 // readback selection
#define SPMC_Z_SERVO_SELECT_RB_LIMITS_REG           112 // readback selection
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
#define SPMC_LOCKIN_F0_CONTROL_REG      1000 // [0] LockIn Config Reg (Gain Control, B0: enable AM control via (1V=1x GVP-A), B2: enable FM control)
                                             // [1] Fixed Gain, Q24
                                             // [2] DDSN2 (16)
                                             // [3] PHASE INC (48)
                                             // [4] FM Scale (Q24) (control via GVP-B, Frq shift in V/Hz)
                                             // [5] RF GEN PHASE INC (32)


// BIQUAD  MODULE @ CONFIG ADDRESS
#define SPMC_BIQUAD_F0_CONTROL_REG      1001 //  BiQuad Parameters b0, b1, b2, a0, a1

#define SPMC_MAIN_CONTROL_XYZU_OFFSET_REG  1100 //  SPMC MAIN CONTROL IP REGISTERS
#define SPMC_MAIN_CONTROL_ROTM_REG         1101 //  SPMC MAIN CONTROL IP REGISTERS
#define SPMC_MAIN_CONTROL_SLOPE_REG        1102 //  SPMC MAIN CONTROL IP REGISTERS
#define SPMC_MAIN_CONTROL_MODULATION_REG   1103 //  SPMC MAIN CONTROL IP REGISTERS
#define SPMC_MAIN_CONTROL_BIAS_REG         1104 //  SPMC MAIN CONTROL IP REGISTERS -- Gxsm set Bias onyl (U), included in XYZU as well
#define SPMC_MAIN_CONTROL_Z_POLARITY_REG   1105 //  SPMC MAIN CONTROL IP REGISTERS
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

#define SPMC_MUX2_Z_SERVO_CONTROL_REG   2001 // MUX selection, test mode, test value.
                                             // Z-SERVO INPUT RP_CH1 | ADC463x_CH0


#define SPMC_MUX2_RF_OUT_CONTROL_REG    2002 // MUX selection, test mode, test value.
                                             // RF (OUT1) AUX select: Pulse, RF sweep gen

// AD463x SPI control/configuration
#define SPMC_AD463X_CONTROL_REG            50000
#define SPMC_AD463X_CONFIG_MODE            0 // Bit0: activate config mode, Bit1: read config, Bit2: write config, Bit3: manual convert, Bit4: enable streaming to AXI, Bit7: SPI AD reset
#define SPMC_AD463X_CONFIG_MODE_CONFIG  0x01
#define SPMC_AD463X_CONFIG_MODE_RW      0x02
#define SPMC_AD463X_CONFIG_MODE_STREAM  0x04
#define SPMC_AD463X_CONFIG_MODE_CNV     0x08
#define SPMC_AD463X_CONFIG_MODE_AXI     0x10
#define SPMC_AD463X_CONFIG_MODE_FPGA_CNV 0x20
#define SPMC_AD463X_CONFIG_MODE_B6      0x40
#define SPMC_AD463X_CONFIG_MODE_RESET   0x80

#define SPMC_AD463X_CONFIG_WR_DATA         1 // address to write and data ** Config Write Address: Bit 23 is "A15": W=0/R=1 (send first), Bit 22:8: are A14..A0, Bit 7:0 are data D7:0 (24 BIT total Addr + Data)
#define SPMC_AD463X_CONFIG_N_BITS          2 // total num bits to read/write in config mode, cnv and AXI stream more
#define SPMC_AD463X_CONFIG_N_DECII         3 // SPI clock divider from 125 MHz
                                             
// MODULE READBACK REGISTER A,B ADRESS MAPPINGS
#define SPMC_READBACK_Z_REG            100001
#define SPMC_READBACK_BIAS_REG         100002 // Bias Sum, Gxsm Bias "U0" Set
#define SPMC_READBACK_GVPBIAS_REG      100003 // Bias GVP Comp., Bias MOD (DBG: currently = GVP-A)
#define SPMC_READBACK_PMD_DA56_REG     100004 // PMD AD MODULE 5,6 "A,B" 
#define SPMC_READBACK_Z_SERVO_REG      100005 // Z_SERVO readback configuration -- setup >what< in Z_SERVO_CONFIG first!
#define SPMC_READBACK_GVP_AMC_FMC_REG  100006 // GVP AMC, FMC
#define SPMC_READBACK_SRCS_MUX_REG     100010 // SRCS stream MUX selection
#define SPMC_READBACK_IN_MUX_REG       100011 // IN AXIS AD data stream MUX selection

#define SPMC_READBACK_AD463X_REG       100100 // AD463x read back config data/value

#define SPMC_READBACK_TIMINGTEST_REG   101999
#define SPMC_READBACK_TIMINGRESET_REG  102000

#define SPMC_READBACK_RPSPMC_PACPLL_VERSION_REG 199997  // READBACK: FPGA Version/Date
#define SPMC_READBACK_RPSPMC_SYSTEM_STATE       199999

#define SPMC_READBACK_XX_REG                    100999 // Debugging use, temp assignments

#define SPMC_READBACK_TEST_RESET_REG            102000
#define SPMC_READBACK_TEST_VALUE_REG             101999

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
#define Q_XY_PRECISION Q24
#define Q_Z_SLOPE_PRECISION Q24

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
