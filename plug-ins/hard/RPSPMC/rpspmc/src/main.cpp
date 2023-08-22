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

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <vector>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>

#include "main.h"
  
// INSTALL:
// cp ~/SVN/RedPitaya/RedPACPLL4mdc-SPI/RedPACPLL4mdc-SPI.runs/impl_1/system_wrapper.bit fpga.bit
// scp -r pacpll root@rp-f05603.local:/opt/redpitaya/www/apps/
// make clean; make INSTALL_DIR=/opt/redpitaya

// CHROME BROWSER NOTES: USER SCHIT-F5 to force reload of all data, else caches are fooling....

/*
 * RedPitaya A9 JSON Interface PARAMETERS and SIGNALS
 * ------------------------------------------------------------
 */

#define REDPACPLL_DATE    0x20200408
#define REDPACPLL_VERSION 0x00160000

// FPGA MEMORY MAPPING for "RP-SPMC-RedPACPLL" 202308V00

// FPGA page size is 0x1000

#define FPGA_BRAM_BASE    0x40000000 //    2M (0x4000_0000 ... 0x401F_FFFF)

#define FPGA_CFG_REG      0x42000000 //    4k (0x4200_0000 ... _1000)
#define FPGA_CFG_PAGES    2          // currently only 1st page assigned/used

#define FPGA_GPIO_BASE    0x42002000 // 256 each (=0x100) //** NOTE: NEW, changed from excessiv 4k to 256 as only adresses 0..128 are needed
#define FPGA_GPIO_SIZE    0x0100     // 0x1000 previously
#define FPGA_GPIO_BLOCKS  9          // 9x256 (0x4200_2000 ... _2800)
#define FPGA_GPIO_PAGES   1          // 1 page

#define PACPLL_CFG0_OFFSET 0
#define PACPLL_CFG1_OFFSET 32
#define PACPLL_CFG2_OFFSET 64
#define PACPLL_CFG3_OFFSET 96


// CONFIGURATION (CFG) DATA REGISTER 0 [1023:0] x 4 = 4k
// PAC-PLL Control Core

// general control paging (future options)
//#define PACPLL_CFG_PAGE_CONTROL  30   // 32bit wide
//#define PACPLL_CFG_PAGE          31   // 32bit wide

// PACPLL core controls
#define PACPLL_CFG_DDS_PHASEINC  0    // 64bit wide
#define PACPLL_CFG_VOLUME_SINE   2    // 32bit wide (default)
#define PACPLL_CFG_CONTROL_LOOPS 3

#define PACPLL_CFG_PACTAU        4 // (actual Q22 mu)
#define PACPLL_CFG_DC_OFFSET     5

#define PACPLL_CFG_TRANSPORT_CONTROL         6
#define PACPLL_CFG_TRANSPORT_SAMPLES         7
#define PACPLL_CFG_TRANSPORT_DECIMATION      8
#define PACPLL_CFG_TRANSPORT_CHANNEL_SELECT  9

#define PACPLL_CFG_PACATAU      27
#define PACPLL_CFG_PAC_DCTAU    28

// Controller Core (Servos) Relative Block Offsets:
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
#define PACPLL_CFG_AMPLITUDE_CONTROLLER 20 //20:26 (20,21,22,23:24,25:26)


// CFG DATA REGISTER 1 [1023:0]
#define PACPLL_CFG_DFREQ_CONTROLLER         (PACPLL_CFG1_OFFSET + 0) // 00:06
#define PACPLL_CFG_PHASE_CONTROL_THREASHOLD (PACPLL_CFG1_OFFSET + 8) // 08

// SPM Z-CONTROL SERVO
#define SPMC_CFG_Z_SERVO_CONTROLLER         (PACPLL_CFG1_OFFSET + 10) // 10:16
#define SPMC_CFG_Z_SERVO_ZSETPOINT          (PACPLL_CFG1_OFFSET + 17) // 17
#define SPMC_CFG_Z_SERVO_LEVEL              (PACPLL_CFG1_OFFSET + 18) // 18
#define SPMC_CFG_Z_SERVO_MODE               (PACPLL_CFG1_OFFSET + 19) // 19: SERVO CONTROL (enable) Bit0, ...

// CFG DATA REGISTER 2 [1023:0]
// PACPLL-PulseFormer Control Core

#define PACPLL_CFG_PULSE_FORM_BASE      (PACPLL_CFG2_OFFSET + 0)
#define PACPLL_CFG_PULSE_FORM_DELAY_01  0  // [ 31..16 Delay P0, 15..0 Delay P1 ] 32bit
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




// not used any more
#define PACPLL_CFG_TRANSPORT_AUX_SCALE       17
#define PACPLL_CFG_TRANSPORT_AUX_CENTER      18  // 18,19



#if 0
// ---------- NOT IMPLEMENTED/NO ROOM ---------------------
// using fixed length FIR filters 
// Configure transport tau: time const or high speed IIR filter stages
#define PACPLL_CFG_TRANSPORT_TAU_DFREQ   (PACPLL_CFG1_OFFSET + 0)
#define PACPLL_CFG_TRANSPORT_TAU_PHASE   (PACPLL_CFG1_OFFSET + 1)
#define PACPLL_CFG_TRANSPORT_TAU_EXEC    (PACPLL_CFG1_OFFSET + 2)
#define PACPLL_CFG_TRANSPORT_TAU_AMPL    (PACPLL_CFG1_OFFSET + 3)
#endif


// CFG DATA REGISTER 3 [1023:0]
// SPMControl Core

#define SPMC_BASE                    (PACPLL_CFG3_OFFSET + 0)
#define SPMC_GVP_CONTROL              0 // 0: reset 1: setvec
#define SPMC_GVP_VECTOR_DATA          1..16 // 512 bits (16x32)

#define SPMC_CFG_AD5791_AXIS_DAC     17
#define SPMC_CFG_AD5791_DAC_CONTROL  18 // 0: config mode 1,2,3: axis 4: send

// SPMC Transformations Core
#define SPMC_ROTM_XX             20  // cos(Alpha)
#define SPMC_ROTM_XY             21  // sin(Alpha)
//#define SPMC_ROTM_YX = -XY = -sin(Alpha)
//#define SPMC_ROTM_YY =  XX =  cos(Alpha)
#define SPMC_SLOPE_X             22
#define SPMC_SLOPE_Y             23
#define SPMC_OFFSET_X            24
#define SPMC_OFFSET_Y            25
#define SPMC_OFFSET_Z            26


// ****************************************


#define ADC_DECIMATING     1
#define ADC_SAMPLING_RATE (125e6/ADC_DECIMATING)

//Signal size
#define SIGNAL_SIZE_DEFAULT       1024
#define TUNE_SIGNAL_SIZE_DEFAULT  1024
#define PARAMETER_UPDATE_INTERVAL 200 // ms
#define SIGNAL_UPDATE_INTERVAL    200 // ms

#define SIGNAL_SIZE_GPIOX 16

//Signal
// Block mode
CFloatSignal SIGNAL_CH1("SIGNAL_CH1", SIGNAL_SIZE_DEFAULT, 0.0f);
CFloatSignal SIGNAL_CH2("SIGNAL_CH2", SIGNAL_SIZE_DEFAULT, 0.0f);

// Slow from GPIO, stripe plotter mode

CFloatSignal SIGNAL_CH3("SIGNAL_CH3", SIGNAL_SIZE_DEFAULT, 0.0f);
std::vector<float> g_data_signal_ch3(SIGNAL_SIZE_DEFAULT);

CFloatSignal SIGNAL_CH4("SIGNAL_CH4", SIGNAL_SIZE_DEFAULT, 0.0f);
std::vector<float> g_data_signal_ch4(SIGNAL_SIZE_DEFAULT);

CFloatSignal SIGNAL_CH5("SIGNAL_CH5", SIGNAL_SIZE_DEFAULT, 0.0f);
std::vector<float> g_data_signal_ch5(SIGNAL_SIZE_DEFAULT);

CFloatSignal SIGNAL_FRQ("SIGNAL_FRQ", TUNE_SIGNAL_SIZE_DEFAULT, 0.0f);
std::vector<float> g_data_signal_frq(TUNE_SIGNAL_SIZE_DEFAULT);

CFloatSignal SIGNAL_TUNE_PHASE("SIGNAL_TUNE_PHASE", TUNE_SIGNAL_SIZE_DEFAULT, 0.0f);
std::vector<float> g_data_signal_ch1pa(TUNE_SIGNAL_SIZE_DEFAULT); // only used in tune mode

CFloatSignal SIGNAL_TUNE_AMPL("SIGNAL_TUNE_AMPL", TUNE_SIGNAL_SIZE_DEFAULT, 0.0f);
std::vector<float> g_data_signal_ch2aa(TUNE_SIGNAL_SIZE_DEFAULT); // only used in tune mode

CFloatSignal SIGNAL_TIME("SIGNAL_TIME", SIGNAL_SIZE_DEFAULT, 0.0f);
std::vector<float> g_data_signal_time(SIGNAL_SIZE_DEFAULT);

CIntSignal SIGNAL_GPIOX("SIGNAL_GPIOX",  SIGNAL_SIZE_GPIOX, 0);
std::vector<int> g_data_signal_gpiox(SIGNAL_SIZE_GPIOX);

double signal_dc_measured = 0.0;

//Parameter
CIntParameter GAIN1("GAIN1", CBaseParameter::RW, 100, 0, 1, 10000);
CIntParameter GAIN2("GAIN2", CBaseParameter::RW, 100, 0, 1, 10000);
CIntParameter GAIN3("GAIN3", CBaseParameter::RW, 1000, 0, 1, 10000);
CIntParameter GAIN4("GAIN4", CBaseParameter::RW, 100, 0, 1, 10000);
CIntParameter GAIN5("GAIN5", CBaseParameter::RW, 100, 0, 1, 10000);

CIntParameter OPERATION("OPERATION", CBaseParameter::RW, 1, 0, 1, 10);
CIntParameter PACVERBOSE("PACVERBOSE", CBaseParameter::RW, 0, 0, 0, 10);


/*
 *  TRANSPORT FPGA MODULE to BRAM:
 *  S_AXIS1: M-AXIS-SIGNAL from LMS CH0 (input), CH1 (not used) -- pass through from ADC 14 bit
 *  S_AXIS2: M-AXIS-CONTROL Amplitude: output/monitor from Ampl. Controller (32 bit)
 *  S_AXIS3: M-AXIS-CONTROL Phase: output/monitor from Phase. Controller (48 bit)
 *  S_AXIS4: M-AXIS-CONTROL Phase pass, monitor Phase. from CORDIC ATAN X/Y (24 bit)
 *  S_AXIS5: M-AXIS-CONTROL Amplitude pass, monitor Amplitude from CORDIC SQRT (24 bit)
 *
 *  TRANSPORT MODE:
 *  BLOCK modes singel shot, fill after start, stop when full:
 *  0: S_AXIS1 CH1, CH2 decimated, aver aged (sum)
 *  1: S_AXIS4,5  decimated, averaged (sum)
 *  2: A_AXIS1 CH1 decimated, averaged (sum), ADDRreg
 *  3: TEST -- ch1n <= +/-1, ch2n <= +/-2;
 *  4: S_AXIS2,3 decimated64bit ==> Ampl 32bit , Frq-Flower 32bit;
 *  CONTINEOUS STEAM FIFO operation (loop, overwrite)
 *  ... modes to be added for FIFO contineous operation mode
 */

CIntParameter TRANSPORT_CH3("TRANSPORT_CH3", CBaseParameter::RW, 0, 0, 0, 13);
CIntParameter TRANSPORT_CH4("TRANSPORT_CH4", CBaseParameter::RW, 1, 0, 0, 13);
CIntParameter TRANSPORT_CH5("TRANSPORT_CH5", CBaseParameter::RW, 1, 0, 0, 13);
CIntParameter TRANSPORT_MODE("TRANSPORT_MODE", CBaseParameter::RW, 0, 0, 0, 32768);
CIntParameter TRANSPORT_DECIMATION("TRANSPORT_DECIMATION", CBaseParameter::RW, 2, 0, 1, 1<<24);
CIntParameter SHR_DEC_DATA("SHR_DEC_DATA", CBaseParameter::RW, 0, 0, 0, 24);

CIntParameter BRAM_SCOPE_TRIGGER_POS("BRAM_SCOPE_TRIGGER_POS", CBaseParameter::RW, 0, 0, 0, 4096); // Trigger Position Index
CDoubleParameter BRAM_SCOPE_TRIGGER_LEVEL("BRAM_SCOPE_TRIGGER_LEVEL", CBaseParameter::RW, 0, 0, -20000., 20000.); // Trigger Level (in mV, Hz, deg)
CIntParameter BRAM_SCOPE_TRIGGER_MODE("BRAM_SCOPE_TRIGGER_MODE", CBaseParameter::RW, 0, 0, 0, 20);  // 0: None, 1-4 CH1/2+/- 5-20 B0-8HL
CIntParameter BRAM_SCOPE_SHIFT_POINTS("BRAM_SCOPE_SHIFT_POINTS", CBaseParameter::RW, 0, 0, 0, 4096); // Scope Data BRAM Shift/Offset Position
CIntParameter BRAM_WRITE_ADR("BRAM_WRITE_ADR", CBaseParameter::RW, 0, 0, 0, 1<<16);
CIntParameter BRAM_SAMPLE_POS("BRAM_SAMPLE_POS", CBaseParameter::RW, 0, 0, 0, 1<<16);
CIntParameter BRAM_FINISHED("BRAM_FINISHED", CBaseParameter::RW, 0, 0, 0, 1);
CIntParameter BRAM_DEC_COUNT("BRAM_DEC_COUNT", CBaseParameter::RW, 0, 0, 0, 0xffffffff);

CFloatParameter DC_OFFSET("DC_OFFSET", CBaseParameter::RW, 0, 0, -1000.0, 1000.0); // mV

CDoubleParameter EXEC_MONITOR("EXEC_MONITOR", CBaseParameter::RW, 0, 0, -1000.0, 1000.0); // mV
CDoubleParameter DDS_FREQ_MONITOR("DDS_FREQ_MONITOR", CBaseParameter::RW, 0, 0, 0.0, 25e6); // Hz
CDoubleParameter PHASE_MONITOR("PHASE_MONITOR", CBaseParameter::RW, 0, 0, -180.0, 180.0); // deg
CDoubleParameter VOLUME_MONITOR("VOLUME_MONITOR", CBaseParameter::RW, 0, 0, -1000.0, 1000.0); // mV
CDoubleParameter DFREQ_MONITOR("DFREQ_MONITOR", CBaseParameter::RW, 0, 0, -1000.0, 1000.0); // Hz
CDoubleParameter CONTROL_DFREQ_MONITOR("CONTROL_DFREQ_MONITOR", CBaseParameter::RW, 0, 0, -10000.0, 10000.0); // mV

CDoubleParameter CENTER_AMPLITUDE("CENTER_AMPLITUDE", CBaseParameter::RW, 0, 0, 0.0, 1000.0); // mV
CDoubleParameter CENTER_FREQUENCY("CENTER_FREQUENCY", CBaseParameter::RW, 0, 0, 0.0, 25e6); // Hz
CDoubleParameter CENTER_PHASE("CENTER_PHASE", CBaseParameter::RW, 0, 0, -180.0, 180.0); // deg

// PAC CONFIGURATION
//./pacpll -m s -f 32766.0 -v .5 -t 0.0004 -V 3
//./pacpll -m t -f 32766.0 -v .5 -t 0.0004 -M 1 -s 2.0 -d 0.05 -u 150000  > data-tune
CDoubleParameter FREQUENCY_TUNE("FREQUENCY_TUNE", CBaseParameter::RW, 32766.0, 0, 1, 25e6); // Hz
CDoubleParameter FREQUENCY_MANUAL("FREQUENCY_MANUAL", CBaseParameter::RW, 32766.0, 0, 1, 25e6); // Hz
CDoubleParameter FREQUENCY_CENTER("FREQUENCY_CENTER", CBaseParameter::RW, 32766.0, 0, 1, 25e6); // Hz -- used for BRam and AUX data to remove offset, and scale
CDoubleParameter AUX_SCALE("AUX_SCALE", CBaseParameter::RW, 1.0, 0, -1e6, 1e6); // 1
CDoubleParameter VOLUME_MANUAL("VOLUME_MANUAL", CBaseParameter::RW, 300.0, 0, 0.0, 1000.0); // mV
CDoubleParameter PAC_DCTAU("PAC_DCTAU", CBaseParameter::RW, 10.0, 0, -1.0, 1e6); // ms ,negative value disables DC LMS and used manual DC 
CDoubleParameter PACTAU("PACTAU", CBaseParameter::RW, 10.0, 0, 0.0, 60e6); // in periods now -- 350us good value @ 30kHz
CDoubleParameter PACATAU("PACATAU", CBaseParameter::RW, 1.5, 0, 0.0, 60e6); // in periods now -- 50us good value @ 30kHz

CDoubleParameter QC_GAIN("QC_GAIN", CBaseParameter::RW, 0, 0, -1024.0, 1024.0); // gain factor
CDoubleParameter QC_PHASE("QC_PHASE", CBaseParameter::RW, 0, 0, 0.0, 360.0); // deg

CDoubleParameter TUNE_SPAN("TUNE_SPAN", CBaseParameter::RW, 5.0, 0, 0.1, 1e6); // Hz
CDoubleParameter TUNE_DFREQ("TUNE_DFREQ", CBaseParameter::RW, 0.05, 0, 0.0001, 1000.); // Hz

// BRAM LOAD CONTROL
CBooleanParameter SET_SINGLESHOT_TRANSPORT_TRIGGER("SET_SINGLESHOT_TRANSPORT_TRIGGER", CBaseParameter::RW, false, 0);
CDoubleParameter  SET_SINGLESHOT_TRIGGER_POST_TIME("SET_SINGLESHOT_TRIGGER_POST_TIME", CBaseParameter::RW, 0.0, 0, 0.0, 10e6); // us

// PLL CONFIGURATION
CBooleanParameter AMPLITUDE_CONTROLLER("AMPLITUDE_CONTROLLER", CBaseParameter::RW, false, 0);
CBooleanParameter PHASE_CONTROLLER("PHASE_CONTROLLER", CBaseParameter::RW, false, 0);
CBooleanParameter PHASE_UNWRAPPING_ALWAYS("PHASE_UNWRAPPING_ALWAYS", CBaseParameter::RW, false, 0);
CBooleanParameter QCONTROL("QCONTROL", CBaseParameter::RW, false, 0);
CBooleanParameter LCK_AMPLITUDE("LCK_AMPLITUDE", CBaseParameter::RW, false, 0);
CBooleanParameter LCK_PHASE("LCK_PHASE", CBaseParameter::RW, false, 0);
CBooleanParameter DFREQ_CONTROLLER("DFREQ_CONTROLLER", CBaseParameter::RW, false, 0);

//void rp_PAC_set_phase_controller64 (double setpoint, double cp, double ci, double upper, double lower)
CDoubleParameter AMPLITUDE_FB_SETPOINT("AMPLITUDE_FB_SETPOINT", CBaseParameter::RW, 20, 0, 0, 1000); // mV
CDoubleParameter AMPLITUDE_FB_CP("AMPLITUDE_FB_CP", CBaseParameter::RW, 0, 0, -1000, 1000);
CDoubleParameter AMPLITUDE_FB_CI("AMPLITUDE_FB_CI", CBaseParameter::RW, 0, 0, -1000, 1000);
CDoubleParameter EXEC_FB_UPPER("EXEC_FB_UPPER", CBaseParameter::RW, 500, 0, 0, 1000); // mV
CDoubleParameter EXEC_FB_LOWER("EXEC_FB_LOWER", CBaseParameter::RW, 0, 0, -1000, 1000); // mV
CDoubleParameter PHASE_HOLD_AM_NOISE_LIMIT("PHASE_HOLD_AM_NOISE_LIMIT", CBaseParameter::RW, 5, 0, 0, 1000); // mV

CDoubleParameter PHASE_FB_SETPOINT("PHASE_FB_SETPOINT", CBaseParameter::RW, 0, 0, -180, 180); // deg
CDoubleParameter PHASE_FB_CP("PHASE_FB_CP", CBaseParameter::RW, 0, 0, -1000, 1000); 
CDoubleParameter PHASE_FB_CI("PHASE_FB_CI", CBaseParameter::RW, 0, 0, -1000, 1000);
CDoubleParameter FREQ_FB_UPPER("FREQ_FB_UPPER", CBaseParameter::RW, 32768.0, 0, 0, 25e6); // Hz
CDoubleParameter FREQ_FB_LOWER("FREQ_FB_LOWER", CBaseParameter::RW, 0.1, 0, 0, 25e6); // Hz

// dFREQ Controller (Hz -> Control for Bias / Z / ...)
CDoubleParameter DFREQ_FB_SETPOINT("DFREQ_FB_SETPOINT", CBaseParameter::RW, 0, 0, -100, 100); // Hz
CDoubleParameter DFREQ_FB_CP("DFREQ_FB_CP", CBaseParameter::RW, 0, 0, -1000, 1000); 
CDoubleParameter DFREQ_FB_CI("DFREQ_FB_CI", CBaseParameter::RW, 0, 0, -1000, 1000);
CDoubleParameter DFREQ_FB_UPPER("DFREQ_FB_UPPER", CBaseParameter::RW,  100.0, 0, -10000, 10000); // Control
CDoubleParameter DFREQ_FB_LOWER("DFREQ_FB_LOWER", CBaseParameter::RW, -100.0, 0, -10000, 10000); // Control
        /*
PULSE_FORM_BIAS0=0
PULSE_FORM_BIAS1=0
PULSE_FORM_PHASE0=73
PULSE_FORM_PHASE1=73
PULSE_FORM_WIDTH0=0.1
PULSE_FORM_WIDTH0IF=0.5
PULSE_FORM_WIDTH1=0.1
PULSE_FORM_WIDTH1IF=0.5
PULSE_FORM_HEIGHT0=333
PULSE_FORM_HEIGHT0IF=-37
PULSE_FORM_HEIGHT1=-200
PULSE_FORM_HEIGHT1IF=20
PULSE_FORM_SHAPEXW=0.04
PULSE_FORM_SHAPEXWIF=0.04
PULSE_FORM_SHAPEX=0
PULSE_FORM_SHAPEXIF=1.3
        */

CDoubleParameter PULSE_FORM_BIAS0("PULSE_FORM_BIAS0", CBaseParameter::RW, 0.0, 0, -1000, 1000); // mV
CDoubleParameter PULSE_FORM_BIAS1("PULSE_FORM_BIAS1", CBaseParameter::RW, 0.0, 0, -1000, 1000); // mV
CDoubleParameter PULSE_FORM_PHASE0("PULSE_FORM_PHASE0", CBaseParameter::RW, 0.0, 0, 0, 180); // deg
CDoubleParameter PULSE_FORM_PHASE1("PULSE_FORM_PHASE1", CBaseParameter::RW, 0.0, 0, 0, 180); // deg
CDoubleParameter PULSE_FORM_WIDTH0("PULSE_FORM_WIDTH0", CBaseParameter::RW, 0.0, 0, 0, 10000); // us
CDoubleParameter PULSE_FORM_WIDTH1("PULSE_FORM_WIDTH1", CBaseParameter::RW, 0.0, 0, 0, 10000); // us

CDoubleParameter PULSE_FORM_HEIGHT0("PULSE_FORM_HEIGHT0", CBaseParameter::RW, 0.0, 0, -1000, 1000); // mV
CDoubleParameter PULSE_FORM_HEIGHT1("PULSE_FORM_HEIGHT1", CBaseParameter::RW, 0.0, 0, -1000, 1000); // mV
CDoubleParameter PULSE_FORM_WIDTH0IF("PULSE_FORM_WIDTH0IF", CBaseParameter::RW, 0.0, 0, 0, 10000); // us
CDoubleParameter PULSE_FORM_WIDTH1IF("PULSE_FORM_WIDTH1IF", CBaseParameter::RW, 0.0, 0, 0, 10000); // us
CDoubleParameter PULSE_FORM_HEIGHT0IF("PULSE_FORM_HEIGHT0IF", CBaseParameter::RW, 0.0, 0, -1000, 1000); // mV
CDoubleParameter PULSE_FORM_HEIGHT1IF("PULSE_FORM_HEIGHT1IF", CBaseParameter::RW, 0.0, 0, -1000, 1000); // mV

CDoubleParameter PULSE_FORM_SHAPEX("PULSE_FORM_SHAPEX", CBaseParameter::RW, 0.0, 0, -10, 10); // 1
CDoubleParameter PULSE_FORM_SHAPEXIF("PULSE_FORM_SHAPEXIF", CBaseParameter::RW, 0.0, 0, -10, 10); // 1
CDoubleParameter PULSE_FORM_SHAPEXW("PULSE_FORM_SHAPEXW", CBaseParameter::RW, 0.0, 0, 0, 1); // 1
CDoubleParameter PULSE_FORM_SHAPEXWIF("PULSE_FORM_SHAPEXWIF", CBaseParameter::RW, 0.0, 0, 0, 1); // 1

/*
// Transport LP TAU parameters
CDoubleParameter TRANSPORT_TAU_DFREQ("TRANSPORT_TAU_DFREQ", CBaseParameter::RW, 0.01, 0, -1, 10e3); 
CDoubleParameter TRANSPORT_TAU_PHASE("TRANSPORT_TAU_PHASE", CBaseParameter::RW, 0.01, 0, -1, 10e3); 
CDoubleParameter TRANSPORT_TAU_EXEC("TRANSPORT_TAU_EXEC", CBaseParameter::RW, 0.01, 0, -1, 10e3); 
CDoubleParameter TRANSPORT_TAU_AMPL("TRANSPORT_TAU_AMPL", CBaseParameter::RW, 0.01, 0, -1, 10e3); 
*/

// PHASE Valid for PAC time constant set to 15us:
// Cp = 20*log10 ( 1.6575e-5*Fc )
// Ci = 20*log10 ( 1.7357e-10*Fc^2 )
// Where Fc is the desired bandwidth of the controller in Hz (the suggested range is between 1.5 Hz to 4.5kHz).

// AMPL
// Cp = 20*log10 (0.08045*Q Fc / Gain_res F0 )
// Ci = 20*log10 (8.4243e-7*Q Fc^2 /Gain_res F0 )
// Where :
//Gain res is the gain of the resonator at the resonance
//Q is the Q factor of the resonator
//F0 is the frequency at the resonance in Hz
//Fc is the desired bandwidth of the controller in Hz (the suggested range is between 1.5 Hz to 10Hz).

CStringParameter pacpll_text("PAC_TEXT", CBaseParameter::RW, "N/A                                    ", 40);

CIntParameter parameter_updatePeriod("PARAMETER_PERIOD", CBaseParameter::RW, PARAMETER_UPDATE_INTERVAL, 0, 0, 50000);
CIntParameter signal_updatePeriod("SIGNAL_PERIOD", CBaseParameter::RW, SIGNAL_UPDATE_INTERVAL, 0, 0, 50000);
CIntParameter timeDelay("time_delay", CBaseParameter::RW, 50000, 0, 0, 100000000);
CFloatParameter cpuLoad("CPU_LOAD", CBaseParameter::RW, 0, 0, 0, 100);
CFloatParameter memoryFree ("FREE_RAM", CBaseParameter::RW, 0, 0, 0, 1e15);
CDoubleParameter counter("COUNTER", CBaseParameter::RW, 1, 0, 1e-12, 1e+12);

// global thread control parameter
int thread_data__tune_control=0;


/*
 * RedPitaya A9 FPGA Link
 * ------------------------------------------------------------
 */

const char *FPGA_PACPLL_A9_name = "/dev/mem";
void *FPGA_PACPLL_cfg  = NULL;
void *FPGA_PACPLL_gpio = NULL;
void *FPGA_PACPLL_bram = NULL;
size_t FPGA_PACPLL_CFG_block_size  = 0; // CFG + GPIO space 
size_t FPGA_PACPLL_BRAM_block_size = 0;

//#define DEVELOPMENT_PACPLL_OP
int verbose = 0;

//fprintf(stderr, "");



// FIR filter for GPIO beased readings
#define MAX_FIR_VALUES 9
#define GPIO_FIR_LEN   1024
static pthread_t gpio_reading_thread;
double gpio_reading_FIRV_vector[MAX_FIR_VALUES];
int gpio_reading_FIRV_buffer[MAX_FIR_VALUES][GPIO_FIR_LEN];
unsigned long gpio_reading_FIRV_buffer_DDS[2][GPIO_FIR_LEN];
#define GPIO_READING_DDS_X8 0
#define GPIO_READING_DDS_X9 1

// GPIO READING VECTOR ASSIGNMENTS
#define GPIO_READING_LMS_A  0          // gpio_reading_FIRV_vector[GPIO_READING_LMS_A] / GPIO_FIR_LEN / QLMS
#define GPIO_READING_LMS_B  1          // gpio_reading_FIRV_vector[GPIO_READING_LMS_B] / GPIO_FIR_LEN / QLMS
#define GPIO_READING_OFFSET 2          // / QLMS
#define GPIO_READING_AMPL   3          // gpio_reading_FIRV_vector[GPIO_READING_AMPL] / GPIO_FIR_LEN * 1000 / QCORDICSQRT => mV
#define GPIO_READING_PHASE  4          // ... 180 / QCORDICATAN / M_PI => deg
#define GPIO_READING_EXEC   5          // ... 1000 / QEXEC => mV
#define GPIO_READING_DDS_FREQ       6  // ... * dds_phaseinc_to_freq[[4,1]<<(44-32) | [5,0]>>(64-44)]
#define GPIO_READING_DFREQ          7  // ... * dds_phaseinc_to_freq()
#define GPIO_READING_CONTROL_DFREQ  8  // ... 10000/Q31
pthread_attr_t gpio_reading_attr;
pthread_mutex_t gpio_reading_mutexsum;
int gpio_reading_control = -1;
void *thread_gpio_reading_FIR(void *arg);



/*
 * RedPitaya A9 FPGA Memory Mapping Init
 * ------------------------------------------------------------
 */

int rp_PAC_App_Init(){
        int fd;
        FPGA_PACPLL_CFG_block_size  = (FPGA_CFG_PAGES + FPGA_GPIO_PAGES)*sysconf (_SC_PAGESIZE);   // sysconf (_SC_PAGESIZE) is 0x1000; map CFG + GPIO pages
        FPGA_PACPLL_BRAM_block_size = 2048*sysconf(_SC_PAGESIZE); // Dual Ported FPGA BRAM

        //>INIT RP FPGA PACPLL. --- FPGA MEMORY MAPPING ---
        //>> RP FPGA_PACPLL PAGESIZE:    00001000.
        //>> RP FPGA_PACPLL BRAM: mapped 40000000 - 40800000   block length: 00800000.
        //>> RP FPGA_PACPLL  CFG: mapped 42000000 - 42009000   block length: 00009000.

        // RP-FPGA ADRESS MAP
        // 32bit
        // PS/axi_bram_reader0    0x4000_0000  2M  0x401F_FFFF
        // PS/axi_cfg_register_0  0x4200_0000  2K  0x4200_07FF  SLICES: CFG0: 0...1023 (#0-31)@32bit   CFG1: 1024-2047 (#32-63)@32bit
        // PS/axi_cfg_register_1  0x4200_0800  2K  0x4200_0FFF  SLICES: CFG2: 0...1023 (#0-31)@32bit [ CFG3: 1024-2047 (#32-63)@32bit ]
        // PS/axi_gpio_0          0x4200_1000  4K  0x4200_1FFF
        // PS/axi_gpio_1          0x4200_2000  4K  0x4200_2FFF
        // PS/axi_gpio_2          0x4200_3000  4K  0x4200_3FFF
        // PS/axi_gpio_3          0x4200_4000  4K  0x4200_4FFF
        // PS/axi_gpio_4          0x4200_5000  4K  0x4200_5FFF
        // PS/axi_gpio_5          0x4200_6000  4K  0x4200_6FFF
        // PS/axi_gpio_6          0x4200_7000  4K  0x4200_7FFF
        //### PS/axi_gpio_7          0x4200_8000  4K  0x4200_8FFF // removed mapping
        
        if ((fd = open (FPGA_PACPLL_A9_name, O_RDWR)) < 0) {
                perror ("open");
                return RP_EOOR;
        }

        FPGA_PACPLL_bram = mmap (NULL, FPGA_PACPLL_BRAM_block_size,
                                 PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);

        if (FPGA_PACPLL_bram == MAP_FAILED)
                return RP_EOOR;
  
#ifdef DEVELOPMENT_PACPLL_OP
        fprintf(stderr, "INIT RP FPGA PACPLL. --- FPGA MEMORY MAPPING ---\n");
        fprintf(stderr, "RP FPGA_PACPLL PAGESIZE:    0x%08lx.\n", (unsigned long)(sysconf (_SC_PAGESIZE)));
        fprintf(stderr, "RP FPGA_PACPLL BRAM: mapped 0x%08lx - 0x%08lx   block length: 0x%08lx.\n", (unsigned long)(0x40000000), (unsigned long)(0x40000000 + FPGA_PACPLL_BRAM_block_size), (unsigned long)(FPGA_PACPLL_BRAM_block_size));
#else
        if (verbose > 1) fprintf(stderr, "RP FPGA_PACPLL BRAM: mapped 0x%08lx - 0x%08lx.\n", (unsigned long)(0x40000000), (unsigned long)(0x40000000 + FPGA_PACPLL_BRAM_block_size));
#endif

        
        FPGA_PACPLL_cfg = mmap (NULL, FPGA_PACPLL_CFG_block_size,
                                PROT_READ|PROT_WRITE,  MAP_SHARED, fd, 0x42000000);

        if (FPGA_PACPLL_cfg == MAP_FAILED)
                return RP_EOOR;

#ifdef DEVELOPMENT_PACPLL_OP
        fprintf(stderr, "RP FPGA_PACPLL  CFG: mapped 0x%08lx - 0x%08lx   block length: 0x%08lx.\n", (unsigned long)(0x42000000), (unsigned long)(0x42000000 + FPGA_PACPLL_CFG_block_size),  (unsigned long)(FPGA_PACPLL_CFG_block_size));
#else
        if (verbose > 1) fprintf(stderr, "RP FPGA_PACPLL  CFG: mapped 0x%08lx - 0x%08lx.\n", (unsigned long)(0x42000000), (unsigned long)(0x42000000 + FPGA_PACPLL_CFG_block_size));
#endif
        
        srand(time(NULL));   // init random

        pthread_mutex_init (&gpio_reading_mutexsum, NULL);
        pthread_attr_init (&gpio_reading_attr);
        pthread_attr_setdetachstate (&gpio_reading_attr, PTHREAD_CREATE_JOINABLE);
        pthread_create ( &gpio_reading_thread, &gpio_reading_attr, thread_gpio_reading_FIR, NULL); // start GPIO reading thread FIRs
        pthread_attr_destroy (&gpio_reading_attr);
        return RP_OK;
}

void rp_PAC_App_Release(){
        void *status;
        gpio_reading_control = 0;
        pthread_join (gpio_reading_thread, &status);
        pthread_mutex_destroy (&gpio_reading_mutexsum);
        
        munmap (FPGA_PACPLL_cfg, FPGA_PACPLL_CFG_block_size);
        munmap (FPGA_PACPLL_bram, FPGA_PACPLL_BRAM_block_size);

        pthread_exit (NULL);
}


/*
 * RedPitaya A9 FPGA Control and Data Transfer
 * ------------------------------------------------------------
 */

#define QN(N) ((1<<(N))-1)
#define QN64(N) ((1LL<<(N))-1)

#define Q22 QN(22)
#define Q23 QN(23)
#define Q24 QN(24)
#define Q16 QN(16)
#define Q15 QN(15)
#define Q13 QN(13)
#define Q12 QN(12)
#define Q10 QN(10)
#define QLMS QN(22)
#define BITS_CORDICSQRT 24
#define BITS_CORDICATAN 24
#define QCORDICSQRT QN(23) // W24: 1Q23
#define QCORDICATAN QN(21) // W24: 3Q21
#define QCORDICSQRTFIR QN64(32-2)
// #define QCORDICATANFIR (QN(24-3)*1024)
#define QCORDICATANFIR QN(32-3)

#define BITS_AMPL_CONTROL   32
#define BITS_PLHASE_CONTROL 48

#define Q31 0x7FFFFFFF  // (1<<31)-1 ... ov in expression expansion
#define Q32 0xFFFFFFFF  // (1<<32)-1 ... ov in expression expansion
#define Q40 QN64(40)
#define Q36 QN64(36)
#define Q37 QN64(37)
#define Q47 QN64(47)
#define Q44 QN64(44)
#define Q48 QN64(48)

#define QEXEC   Q31
#define QAMCOEF Q31
#define QFREQ   Q47
#define QPHCOEF Q31
#define QDFCOEF Q31


typedef unsigned short guint16;
typedef short gint16;

long double cpu_values[4] = {0, 0, 0, 0}; /* reading only user, nice, system, idle */

int phase_setpoint_qcordicatan = 0;

inline void set_gpio_cfgreg_int32 (int cfg_slot, int value){
        size_t off = cfg_slot * 4;

        *((int32_t *)((uint8_t*)FPGA_PACPLL_cfg + off)) = value;

        if (verbose > 2) fprintf(stderr, "set_gpio32[CFG%d] int32 %04x = %08x %d\n", cfg_slot, off, value, value);
}

inline void set_gpio_cfgreg_uint32 (int cfg_slot, unsigned int value){
        size_t off = cfg_slot * 4;

        *((int32_t *)((uint8_t*)FPGA_PACPLL_cfg + off)) = value;

        if (verbose > 2) fprintf(stderr, "set_gpio32[CFG%d] uint32 %04x = %08x %d\n", cfg_slot, off, value, value);
}

inline void set_gpio_cfgreg_int16_int16 (int cfg_slot, gint16 value_1, gint16 value_2){
        size_t off = 0x8000 + cfg_slot * 4;
        union { struct { gint16 hi, lo; } hl; int ww; } mem;
        mem.hl.hi = value_1;
        mem.hl.lo = value_2;

        *((int32_t *)((uint8_t*)FPGA_PACPLL_cfg + off)) = mem.ww;

        if (verbose > 2) fprintf(stderr, "set_gpio32[CFG%d] int16|16 %04x = %04x | %04x\n", cfg_slot, off, value_1, value_2);
}

inline void set_gpio_cfgreg_uint16_uint16 (int cfg_slot, guint16 value_1, guint16 value_2){
        size_t off = 0x8000 + cfg_slot * 4;
        union { struct { guint16 hi, lo; } hl; int ww; } mem;
        mem.hl.hi = value_1;
        mem.hl.lo = value_2;

        *((int32_t *)((uint8_t*)FPGA_PACPLL_cfg + off)) = mem.ww;

        if (verbose > 2) fprintf(stderr, "set_gpio32[CFG%d] uint16|16 %04x = %04x | %04x\n", cfg_slot, off, value_1, value_2);
}

inline void set_gpio_cfgreg_int64 (int cfg_slot, long long value){
        size_t off_lo = cfg_slot * 4;
        size_t off_hi = off_lo + 4;
        unsigned long long uv = (unsigned long long)value;
        if (verbose > 2) fprintf(stderr, "set_gpio64[CFG%d] int64 %04x = %08x %d\n", cfg_slot, off_lo, value, value);
        *((int32_t *)((uint8_t*)FPGA_PACPLL_cfg + off_lo)) = uv & 0xffffffff;
        *((int32_t *)((uint8_t*)FPGA_PACPLL_cfg + off_hi)) = uv >> 32;
}

 
// 48bit in 64cfg reg from top -- eventual precision bits=0
inline void set_gpio_cfgreg_int48 (int cfg_slot, unsigned long long value){
        set_gpio_cfgreg_int64 (cfg_slot, value << 16);
}
 

inline int32_t read_gpio_reg_int32_t (int gpio_block, int pos){
        size_t offset = gpio_block * FPGA_GPIO_SIZE + pos * 8;

        return *((int32_t *)((uint8_t*)FPGA_PACPLL_cfg + offset)); 
}

inline int read_gpio_reg_int32 (int gpio_block, int pos){
        size_t offset = gpio_block * FPGA_GPIO_SIZE + pos * 8;
        int x = *((int32_t *)((uint8_t*)FPGA_PACPLL_cfg + offset));
        return x;
}

inline unsigned int read_gpio_reg_uint32 (int gpio_block, int pos){
        size_t offset = gpio_block * FPGA_GPIO_SIZE + pos * 8;
        unsigned int x = *((int32_t *)((uint8_t*)FPGA_PACPLL_cfg + offset));
        return x;
}

 

// Q44: 32766.0000 Hz -> phase_inc=4611404543  0000000112dc72ff
double dds_phaseinc (double freq){
        double fclk = ADC_SAMPLING_RATE;
        return Q44*freq/fclk;
}

double dds_phaseinc_to_freq (unsigned long long ddsphaseincQ44){
        double fclk = ADC_SAMPLING_RATE;
#ifdef DEVELOPMENT_PACPLL_OP_V
        double df = fclk*(double)ddsphaseincQ44/(double)(Q44);
        fprintf(stderr, "##DDS Phase Inc abs to freq: df= %12.4f Hz <- Q44 phase_inc=%lld  %016llx\n", df, ddsphaseincQ44, ddsphaseincQ44phase_inc);
        return df;
#else
        return fclk*(double)ddsphaseincQ44/(double)(Q44);
#endif
}

double dds_phaseinc_rel_to_freq (long long ddsphaseincQ44){
        double fclk = ADC_SAMPLING_RATE;
#ifdef DEVELOPMENT_PACPLL_OP_V
        double df = fclk*(double)ddsphaseincQ44/(double)(Q44);
        fprintf(stderr, "##DDS Phase Inc rel to freq: df= %12.4f Hz <- Q44 phase_inc=%lld  %016llx\n", df, ddsphaseincQ44, ddsphaseincQ44phase_inc);
        return df;
#else
        return fclk*(double)ddsphaseincQ44/(double)(Q44);
#endif
}

void rp_PAC_adjust_dds (double freq){
        // 44 Bit Phase, using 48bit tdata
        unsigned long long phase_inc = (unsigned long long)round (dds_phaseinc (freq));
        //        unsigned int lo32, hi32;
#ifdef DEVELOPMENT_PACPLL_OP
        fprintf(stderr, "##Adjust DDS: f= %12.4f Hz -> Q44 phase_inc=%lld  %016llx\n", freq, phase_inc, phase_inc);
#else
        if (verbose > 2) fprintf(stderr, "##Adjust: f= %12.4f Hz -> Q44 phase_inc=%lld  %016llx\n", freq, phase_inc, phase_inc);
#endif
        
        set_gpio_cfgreg_int48 (PACPLL_CFG_DDS_PHASEINC, phase_inc);

#ifdef DEVELOPMENT_PACPLL_OP
        // Verify
        unsigned long xx8 = read_gpio_reg_uint32 (4,1); // GPIO X7 : Exec Ampl Control Signal (signed)
        unsigned long xx9 = read_gpio_reg_uint32 (5,0); // GPIO X8 : DDS Phase Inc (Freq.) upper 32 bits of 44 (signed)
        unsigned long long ddsphi = ((unsigned long long)xx8<<(44-32)) + ((unsigned long long)xx9>>(64-44)); // DDS Phase Inc (Freq.) upper 32 bits of 44 (signed)
        double vfreq = dds_phaseinc_to_freq(ddsphi);
        fprintf(stderr, "##Verify DDS: f= %12.4f Hz -> Q44 phase_inc=%lld  %016llx  ERROR: %12.4f Hz\n", vfreq, ddsphi, ddsphi, (vfreq-freq));
#endif

}

void rp_PAC_set_volume (double volume){
        if (verbose > 2) fprintf(stderr, "##Configure: volume= %g V\n", volume); 
        set_gpio_cfgreg_int32 (PACPLL_CFG_VOLUME_SINE, (int)(Q31 * volume));
}

// Configure Control Switched: Loops Ampl and Phase On/Off, Unwrapping, QControl
void rp_PAC_configure_switches (int phase_ctrl, int am_ctrl, int phase_unwrap_always, int qcontrol, int lck_amp, int lck_phase, int dfreq_ctrl){
        if (verbose > 2) fprintf(stderr, "##Configure loop controls: %x",  phase_ctrl ? 1:0 | am_ctrl ? 2:0); 
        set_gpio_cfgreg_int32 (PACPLL_CFG_CONTROL_LOOPS,
                               (phase_ctrl ? 1:0) | (am_ctrl ? 2:0) | (phase_unwrap_always ? 4:0) |   // Bits 1,2,3
                               (qcontrol ? 8:0) |                                                     // Bit  4
                               (lck_amp ? 0x10:0)  | (lck_phase ? 0x20:0) |                           // Bits 5,6
                               //(b7 ? 0x40:0) | (b8 ? 0x80:0)
                               (dfreq_ctrl ? 0x100:0)                                                   // Bit 9
                               );
}

#define QCONTROL_CFG_GAIN_DELAY 29
// Configure Q-Control Logic build into Volume Adjuster
void rp_PAC_set_qcontrol (double gain, double phase){
        double samples_per_period = ADC_SAMPLING_RATE / FREQUENCY_MANUAL.Value ();
        int ndelay = int (samples_per_period * phase/360. + 0.5);

        if (ndelay < 0 || ndelay > 8192 || phase < 0.)
                ndelay = 0; // Q-Control disabled when delay == 0

        if (verbose > 2) fprintf(stderr, "##Configure: qcontrol= %d, %d\n", (int)(Q15*gain), ndelay); 

        ndelay = 8192-ndelay; // pre compute offset in ring buffer
        set_gpio_cfgreg_int32 (QCONTROL_CFG_GAIN_DELAY, ((int)(Q10 * gain)<<16) | ndelay );
}

#if 0
// tau from mu
double time_const(double fref, double mu){
        return -(1.0/fref) / log(1.0-mu);
}
// mu from tau
double mu_const(double fref, double tau){
        return 1.0-exp(-1.0/(fref*tau));
}
// -3dB cut off freq
double cut_off_freq_3db(double fref, double mu){
        return -(1.0/(fref*2.*M_PI)) * log(1.0-mu);
}

double mu_opt (double periods){
        double mu = 11.9464 / (6.46178 + periods);
        return mu > 1.0 ? 1.0 : mu;
}
#endif

// tau in s for dual PAC and auto DC offset
void rp_PAC_set_pactau (double tau, double atau, double dc_tau){
        if (verbose > 2) fprintf(stderr, "##Configure: tau= %g  Q22: %d\n", tau, (int)(Q22 * tau)); 

#if 1
        // in tau s (us) -> mu
        set_gpio_cfgreg_int32 (PACPLL_CFG_PACTAU, (int)(Q22/ADC_SAMPLING_RATE/(1e-6*tau))); // Q22 significant from top - tau for phase
        set_gpio_cfgreg_int32 (PACPLL_CFG_PACATAU, (int)(Q22/ADC_SAMPLING_RATE/(1e-6*atau))); // Q22 significant from top -- atau is tau for amplitude
#else
        // in peridos relative to reference base frequency. Silently limit mu to opt mu.
        // mu_opt = 11.9464 / (6.46178 + ADC_SAMPLE_FRQ/F_REF)
        double spp = ADC_SAMPLING_RATE / FREQUENCY_MANUAL.Value (); // samples per period
        double mu_fastest = mu_opt (spp);
        double mu = mu_const (ADC_SAMPLING_RATE, tau/FREQUENCY_MANUAL.Value ()); // mu from user tau measured in periods of f-reference
        if (verbose > 2) fprintf(stderr, "##Configure: pac PHtau   mu= %g, fastest=%g\n", mu, mu_fstest);
        if (mu > mu_fastest) mu=mu_fastest;
        set_gpio_cfgreg_int32 (PACPLL_CFG_PACTAU, (int)(Q22*mu)); // Q22 significant from top - tau for phase

        double mu = mu_const (ADC_SAMPLING_RATE, atau/FREQUENCY_MANUAL.Value ()); // mu from user tau measured in periods of f-reference
        if (verbose > 2) fprintf(stderr, "##Configure: pac AMPtau   mu= %g, fastest=%g\n", mu, mu_fstest);
        if (mu > mu_fastest) mu=mu_fastest;
        set_gpio_cfgreg_int32 (PACPLL_CFG_PACATAU, (int)(Q22*mu)); // Q22 significant from top -- atau is tau for amplitude
#endif
        // Q22 significant from top -- dc_tau is tau for DC FIR-IIR Filter on phase aligned decimated data:
        // at 4x Freq Ref sampling rate. Moving averaging FIR sampling at past 4 zero crossing of Sin Cos ref passed on to IIR with tau
        if (dc_tau > 0.)
                set_gpio_cfgreg_int32 (PACPLL_CFG_PAC_DCTAU, (int)(Q31/(4.*FREQUENCY_MANUAL.Value ())/dc_tau));
        else if (dc_tau < 0.)
                set_gpio_cfgreg_uint32 (PACPLL_CFG_PAC_DCTAU, 0xffffffff); // disable -- use manaul DC, set below
        else
                set_gpio_cfgreg_uint32 (PACPLL_CFG_PAC_DCTAU, 0); // freeze
}

// Set "manual" DC offset used if dc_tau (see above) signum bit is set (neg).
void rp_PAC_set_dcoff (double dc){
        if (verbose > 2) fprintf(stderr, "##Configure: dc= %g  Q22: %d\n", dc, (int)(Q22 * dc)); 
        set_gpio_cfgreg_int32 (PACPLL_CFG_DC_OFFSET, (int)(Q22 * dc));
}

// measure DC, update manual offset
void rp_PAC_auto_dc_offset_adjust (){
        double dc = 0.0;
        int x,i,k;
        double x1;

        rp_PAC_set_dcoff (0.0);
        usleep(10000);
        
        for (k=i=0; i<300000; ++i){
                if (i%777 == 0) usleep(1000);
                x = read_gpio_reg_int32 (3,0); // 2,0
                x1=(double)x / QLMS;
                if (fabs(x1) < 0.5){
                        dc += x1;
                        ++k;
                }
        }
        dc /= k;
        if (verbose > 1) fprintf(stderr, "RP PACPLL DC-Offset = %10.6f  {%d}\n", dc, k);
        rp_PAC_set_dcoff (dc);

        signal_dc_measured = dc;
}

// update/follow (slow IIR) DC offset
void rp_PAC_auto_dc_offset_correct (){
        int i,x;
        double x1;
        double q=0.0000001;
        double q1=1.0-q;
        for (i=0; i<30000; ++i){
                if (i%777 == 0) usleep(1000);
                x = read_gpio_reg_int32 (3,0);
                x1=(double)x / QLMS;
                if (fabs(x1) < 0.1)
                        signal_dc_measured = q1*signal_dc_measured + q*x1;
        }
        rp_PAC_set_dcoff (signal_dc_measured);
}

// Controller Topology:
/*
  // IP Configuration
  //                                  DEFAULTS   PHASE   AMPL         
    parameter AXIS_TDATA_WIDTH =            32,  24      24    // INPUT AXIS DATA WIDTH
    parameter M_AXIS_CONTROL_TDATA_WIDTH =  32,  48      16    // SERVO CONTROL DATA WIDTH OF AXIS
    parameter CONTROL_WIDTH =               32,  44      16    // SERVO CONTROL DATA WIDTH
    parameter M_AXIS_CONTROL2_TDATA_WIDTH = 32,  48      32    // INTERNAL CONTROl DATA WIDTH MAPPED TO AXIS FOR READOUT not including extend
    parameter CONTROL2_WIDTH =              50,  75      56    // INTERNAL CONTROl DATA WIDTH not including extend **** COEFQ+AXIS_TDATA_WIDTH == CONTROL2_WIDTH
    parameter CONTROL2_OUT_WIDTH =          32,  44      32    // max passed outside control width, must be <= CONTROL2_WIDTH
    parameter COEF_WIDTH =                  32,  32      32    // CP, CI WIDTH
    parameter QIN =                         22,  22      22    // Q In Signal
    parameter QCOEF =                       31,  31      31    // Q CP, CI's
    parameter QCONTROL =                    31,  31      31    // Q Controlvalue
    parameter CEXTEND =                      4,   1       4    // room for saturation check
    parameter DEXTEND =                      1,   1       1    // data, erorr extend
    parameter AMCONTROL_ALLOW_NEG_SPECIAL =  1    0       1    // Special Ampl. Control Mode
  // ********************

  if (AMCONTROL_ALLOW_NEG_SPECIAL)
     if (error_next > $signed(0) && control_next < $signed(0)) // auto reset condition for amplitude control to preven negative phase, but allow active "damping"
          control      <= 0;
          controlint   <= 0;

  ... check limits and limit to upper and lower, set limiter status indicators;

  m = AXI-axi_input;   // [AXIS_TDATA_WIDTH-1:0]
  error = setpoint - m;

  if (enable)
    controlint_next <= controlint + ci*error; // saturation via extended range and limiter // Q64.. += Q31 x Q22 ==== AXIS_TDATA_WIDTH + COEF_WIDTH
    control_next    <= controlint + cp*error; // 
  else
    controlint_next <= reset;
    control_next    <= reset;

  *************************
  assign M_AXIS_CONTROL_tdata   = {control[CONTROL2_WIDTH+CEXTEND-1], control[CONTROL2_WIDTH-2:CONTROL2_WIDTH-CONTROL_WIDTH]}; // strip extension
  assign M_AXIS_CONTROL2_tdata  = {control[CONTROL2_WIDTH+CEXTEND-1], control[CONTROL2_WIDTH-2:CONTROL2_WIDTH-CONTROL2_OUT_WIDTH]};

  assign mon_signal  = {m[AXIS_TDATA_WIDTH+DEXTEND-1], m[AXIS_TDATA_WIDTH-2:0]};
  assign mon_error   = {error[AXIS_TDATA_WIDTH+DEXTEND-1], error[AXIS_TDATA_WIDTH-2:0]};
  assign mon_control = {control[CONTROL2_WIDTH+CEXTEND-1], control[CONTROL2_WIDTH-2:CONTROL2_WIDTH-32]};
  assign mon_control_lower32 = {{control[CONTROL2_WIDTH-32-1 : (CONTROL2_WIDTH>=64? CONTROL2_WIDTH-32-1-31:0)]}, {(CONTROL2_WIDTH>=64?0:(64-CONTROL2_WIDTH)){1'b0}}}; // signed, lower 31
  *************************
 */


// Configure Controllers


// Amplitude Controller
// AMPL from CORDIC: 24bit Q23 -- QCORDICSQRT
void rp_PAC_set_amplitude_controller (double setpoint, double cp, double ci, double upper, double lower){
        if (verbose > 2) fprintf(stderr, "##Configure Controller: set= %g  Q22: %d    cp=%g ci=%g upper=%g lower=%g\n", setpoint, (int)(Q22 * setpoint), cp, ci, upper, lower); 
        /*
        double Q = pll.Qfactor;     // Q-factor
        double F0 = pll.FSineHz; // Res. Freq
        double Fc = pll.auto_set_BW_Amp; // 1.5 .. 10Hz
        double gainres = pll.GainRes;
        double cp = 20. * log10 (0.08045   * Q*Fc / (gainres*F0));
        double ci = 20. * log10 (8.4243e-7 * Q*Fc*Fc / (gainres*F0));

        write_pll_variable32 (dsp_pll.icoef_Amp, pll.signum_ci_Amp * CPN(29)*pow (10.,pll.ci_gain_Amp/20.));
        // = ISign * CPN(29)*pow(10.,Igain/20.);
		
        write_pll_variable32 (dsp_pll.pcoef_Amp, pll.signum_cp_Amp * CPN(29)*pow (10.,pll.cp_gain_Amp/20.));
        // = PSign * CPN(29)*pow(10.,Pgain/20.);
        */
        set_gpio_cfgreg_int32 (PACPLL_CFG_AMPLITUDE_CONTROLLER + PACPLL_CFG_SET,   ((int)(QCORDICSQRT * setpoint))<<(32-BITS_CORDICSQRT));
        set_gpio_cfgreg_int32 (PACPLL_CFG_AMPLITUDE_CONTROLLER + PACPLL_CFG_CP,    ((int)(QAMCOEF * cp)));
        set_gpio_cfgreg_int32 (PACPLL_CFG_AMPLITUDE_CONTROLLER + PACPLL_CFG_CI,    ((int)(QAMCOEF * ci)));
        set_gpio_cfgreg_int32 (PACPLL_CFG_AMPLITUDE_CONTROLLER + PACPLL_CFG_UPPER, ((int)(QEXEC * upper)));
        set_gpio_cfgreg_int32 (PACPLL_CFG_AMPLITUDE_CONTROLLER + PACPLL_CFG_LOWER, ((int)(QEXEC * lower)));
}

// Phase Controller
// CONTROL[75] OUT[44] : [75-1-1:75-44]=43+1   m[24]  x  c[32]  = 56 M: 24{Q32},  P: 44{Q14}
void rp_PAC_set_phase_controller (double setpoint, double cp, double ci, double upper, double lower, double am_threashold){
        if (verbose > 2) fprintf(stderr, "##Configure Controller: set= %g  Q22: %d    cp=%g ci=%g upper=%g lower=%g\n", setpoint, (int)(Q22 * setpoint), cp, ci, upper, lower); 

        /*
        double cp = 20. * log10 (1.6575e-5  * pll.auto_set_BW_Phase);
        double ci = 20. * log10 (1.7357e-10 * pll.auto_set_BW_Phase*pll.auto_set_BW_Phase);

        write_pll_variable32 (dsp_pll.icoef_Phase, pll.signum_ci_Phase * CPN(29)*pow (10.,pll.ci_gain_Phase/20.));
        // = ISign * CPN(29)*pow(10.,Igain/20.);
	
        write_pll_variable32 (dsp_pll.pcoef_Phase, pll.signum_cp_Phase * CPN(29)*pow (10.,pll.cp_gain_Phase/20.));
        // = PSign * CPN(29)*pow(10.,Pgain/20.);
        */

        set_gpio_cfgreg_int32 (PACPLL_CFG_PHASE_CONTROLLER + PACPLL_CFG_SET,   (phase_setpoint_qcordicatan = (int)(QCORDICATAN * setpoint))); // <<(32-BITS_CORDICATAN));
        set_gpio_cfgreg_int32 (PACPLL_CFG_PHASE_CONTROLLER + PACPLL_CFG_CP,    (long long)(QPHCOEF * cp)); // {32}   22+1 bit error, 32bit CP,CI << 31 --  m[24]  x  cp|ci[32]  = 56 M: 24{Q32},  P: 44{Q14}, top S43
        set_gpio_cfgreg_int32 (PACPLL_CFG_PHASE_CONTROLLER + PACPLL_CFG_CI,    (long long)(QPHCOEF * ci));
        set_gpio_cfgreg_int48 (PACPLL_CFG_PHASE_CONTROLLER + PACPLL_CFG_UPPER, (unsigned long long)round (dds_phaseinc (upper))); // => 44bit phase
        set_gpio_cfgreg_int48 (PACPLL_CFG_PHASE_CONTROLLER + PACPLL_CFG_LOWER, (unsigned long long)round (dds_phaseinc (lower)));

// PHASE_CONTROLLER_THREASHOLD for hold control if amplituide drops into noise regime to avoid run away
// AMPL from CORDIC: 24bit Q23 -- QCORDICSQRT
        set_gpio_cfgreg_int32 (PACPLL_CFG_PHASE_CONTROL_THREASHOLD,   ((int)(QCORDICSQRT * am_threashold))<<(32-BITS_CORDICSQRT));
}


void rp_PAC_set_pulse_form (double bias0, double bias1,
                            double phase0, double phase1,
                            double width0, double width0if,
                            double width1, double width1if,
                            double height0, double height0if,
                            double height1, double height1if,
                            double shapexw, double shapexwif,
                            double shapex, double shapexif){ // deg, us, mV, [Hz-ref]
        // T = 1/freq_ref, N = T/ADC_SAMPLING_RATE, N = freq_ref / ADC_SAMPLING_RATE
        // m = phase/360 * N
        
        const int RDECI = 2;
        double freq_ref = DDS_FREQ_MONITOR.Value ();
        double mfactor  = ADC_SAMPLING_RATE / RDECI / freq_ref / 360.; // phase in deg to # samples
        double usfactor = 1e-6 * ADC_SAMPLING_RATE;     // us to samples
        double mVfactor = 1e-3*32768.; // mV to DAC[16bit]

        double p0phws2 =  0.5*(width0 + 2*width0if)*usfactor; // complete pulse 1/2 width in steps
        double p1phws2 =  0.5*(width1 + 2*width1if)*usfactor;

        phase0 -= p0phws2/mfactor; if (phase0 < 0.) phase0 = 0.;
        phase1 -= p1phws2/mfactor; if (phase1 < 0.) phase1 = 0.;

        // phase delay
        set_gpio_cfgreg_uint16_uint16 (PACPLL_CFG_PULSE_FORM_BASE + PACPLL_CFG_PULSE_FORM_DELAY_01,   (guint16)(phase0*mfactor+0.5),    (guint16)(phase1*mfactor+0.5));     // do conversion phase angle in deg to phase steps

        double rstif = shapexwif; // relative shaping pulse length time
        double pstif = 1.0 - rstif; // remaining pulse length time
        double rst = shapexw; // relative shaping pulse length time
        double pst = 1.0 - rst; // remaining pulse length time
        
        // pre pulse shaping pre pulse, then remaining pulse
        set_gpio_cfgreg_uint16_uint16 (PACPLL_CFG_PULSE_FORM_BASE + PACPLL_CFG_PULSE_FORM_WH01_ARR+0, (guint16)(width0if*rstif*usfactor+0.5),     (guint16)(width1if*rstif*usfactor+0.5));  // 0,1: width in us to steps
        set_gpio_cfgreg_int16_int16   (PACPLL_CFG_PULSE_FORM_BASE + PACPLL_CFG_PULSE_FORM_WH01_ARR+1, ( gint16)(height0if*shapexif*mVfactor),     ( gint16)(height1if*shapexif*mVfactor));     // 2,3: height in mV to 16bit (later 14bit on FPGA)
        set_gpio_cfgreg_uint16_uint16 (PACPLL_CFG_PULSE_FORM_BASE + PACPLL_CFG_PULSE_FORM_WH01_ARR+2, (guint16)(width0if*pstif*usfactor+0.5),     (guint16)(width1if*pstif*usfactor+0.5));  // 0,1: width in us to steps
        set_gpio_cfgreg_int16_int16   (PACPLL_CFG_PULSE_FORM_BASE + PACPLL_CFG_PULSE_FORM_WH01_ARR+3, ( gint16)(height0if*mVfactor),              ( gint16)(height1if*mVfactor));     // 2,3: height in mV to 16bit (later 14bit on FPGA)

        // pulse shaping pre pulse, then remaining pulse
        set_gpio_cfgreg_uint16_uint16 (PACPLL_CFG_PULSE_FORM_BASE + PACPLL_CFG_PULSE_FORM_WH01_ARR+4, (guint16)(width0*rst*usfactor+0.5),         (guint16)(width1*rst*usfactor+0.5));    // 4,5: width in us to steps
        set_gpio_cfgreg_int16_int16   (PACPLL_CFG_PULSE_FORM_BASE + PACPLL_CFG_PULSE_FORM_WH01_ARR+5, ( gint16)(height0*shapex*mVfactor),         ( gint16)(height1*shapex*mVfactor));       // 6,7: height in mV to 16bit (later 14bit on FPGA)
        set_gpio_cfgreg_uint16_uint16 (PACPLL_CFG_PULSE_FORM_BASE + PACPLL_CFG_PULSE_FORM_WH01_ARR+6, (guint16)(width0*pst*usfactor+0.5),         (guint16)(width1*pst*usfactor+0.5));    // 4,5: width in us to steps
        set_gpio_cfgreg_int16_int16   (PACPLL_CFG_PULSE_FORM_BASE + PACPLL_CFG_PULSE_FORM_WH01_ARR+7, ( gint16)(height0*mVfactor),                ( gint16)(height1*mVfactor));       // 6,7: height in mV to 16bit (later 14bit on FPGA)

        // post pulse shaping pre pulse, then remaining pulse
        set_gpio_cfgreg_uint16_uint16 (PACPLL_CFG_PULSE_FORM_BASE + PACPLL_CFG_PULSE_FORM_WH01_ARR+8, (guint16)(width0if*rstif*usfactor+0.5),     (guint16)(width1if*rstif*usfactor+0.5));  // 8,9: width in us to steps
        set_gpio_cfgreg_int16_int16   (PACPLL_CFG_PULSE_FORM_BASE + PACPLL_CFG_PULSE_FORM_WH01_ARR+9, ( gint16)(height0if*shapexif*mVfactor),     ( gint16)(height1if*shapexif*mVfactor));   // 10,11: height in mV to 16bit (later 14bit on FPGA)
        set_gpio_cfgreg_uint16_uint16 (PACPLL_CFG_PULSE_FORM_BASE + PACPLL_CFG_PULSE_FORM_WH01_ARR+10,(guint16)(width0if*pstif*usfactor+0.5),     (guint16)(width1if*pstif*usfactor+0.5));  // 8,9: width in us to steps
        set_gpio_cfgreg_int16_int16   (PACPLL_CFG_PULSE_FORM_BASE + PACPLL_CFG_PULSE_FORM_WH01_ARR+11,( gint16)(height0if*mVfactor),              ( gint16)(height1if*mVfactor));   // 10,11: height in mV to 16bit (later 14bit on FPGA)

        // offset/bias base lines pre/post
        set_gpio_cfgreg_int16_int16   (PACPLL_CFG_PULSE_FORM_BASE + PACPLL_CFG_PULSE_FORM_WH01_ARR+12,( gint16)(bias0*mVfactor),        ( gint16)(bias1*mVfactor));       // 12,13: B height in mV to 16bit (later 14bit on FPGA)
        set_gpio_cfgreg_int16_int16   (PACPLL_CFG_PULSE_FORM_BASE + PACPLL_CFG_PULSE_FORM_WH01_ARR+13,( gint16)(bias0*mVfactor),        ( gint16)(bias1*mVfactor));       // 14,15: B height in mV to 16bit (later 14bit on FPGA)
}


/*
        OPERATION::
        start   <= operation[0:0]; // Trigger start for single shot
        init    <= operation[7:4]; // reset/reinit
        shr_dec_data <= operation[31:8]; // DEC data shr for ch1..4

 */

int __rp_pactransport_shr_dec_hi=0;

// Configure BRAM Data Transport Mode
// Bit Masks for TRANSPORT CONTROL REGISTER
#define PACPLL_CFG_TRANSPORT_RESET           16  // Bit 4=1
#define PACPLL_CFG_TRANSPORT_SINGLE          2   // Bit 1=1
#define PACPLL_CFG_TRANSPORT_LOOP            0   // Bit 1=0
#define PACPLL_CFG_TRANSPORT_START           1   // Bit 0=1
void rp_PAC_transport_set_control (int control){
        static int control_reg=0;

        if (control >= 0){
                control_reg = control;
                if (verbose == 1) fprintf(stderr, "TR: RESET: %s, START: %s, MODE: %s\n",
                                          control_reg & PACPLL_CFG_TRANSPORT_RESET ? "SET  " : "READY",
                                          control_reg & PACPLL_CFG_TRANSPORT_START ? "GO" : "ARMED",
                                          control_reg & PACPLL_CFG_TRANSPORT_SINGLE ? "SINGLE":"LOOP");
        }
        set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_CONTROL, (control_reg & 0xff) | __rp_pactransport_shr_dec_hi);
}

void rp_PAC_configure_transport (int control, int shr_dec_data, int nsamples, int decimation, int channel_select, double scale, double center){
        __rp_pactransport_shr_dec_hi = channel_select > 7 ? 0 : ((shr_dec_data >= 0 && shr_dec_data <= 24) ? shr_dec_data : 0) << 8; // keep memory, do not shift DBG data
        
        if (verbose == 1) fprintf(stderr, "Configure transport: dec=%d, shr=%d, CHM=%d N=%d ", decimation, shr_dec_data, channel_select, nsamples);
        
        if (nsamples > 0){
                set_gpio_cfgreg_int32 (PACPLL_CFG_TRANSPORT_SAMPLES, nsamples);
        }
        
        set_gpio_cfgreg_int32 (PACPLL_CFG_TRANSPORT_DECIMATION, decimation);

#ifdef REMAP_TO_OLD_FPGA_VERSION
        //           old mapping  IN12 PA 1A 1P AF AE PF MA DBG -
        //                 from   { 0, 1, 2, 3, 4, 5, 6, 7, 8  }
        //             remap to   { 0, 4, 2, 3, 3, 2, 5, 1, 8, 0 }; // best match
        static int remap_ch[10] = { 0, 7, 2, 3, 1, 6, 5, 1, 8, 0 };
        channel_select = remap_ch[channel_select];
#endif
        set_gpio_cfgreg_int32 (PACPLL_CFG_TRANSPORT_CHANNEL_SELECT, channel_select);
        // AUX scale, center
        set_gpio_cfgreg_int32 (PACPLL_CFG_TRANSPORT_AUX_SCALE, (int)round(Q15*scale));
        set_gpio_cfgreg_int48 (PACPLL_CFG_TRANSPORT_AUX_CENTER, (unsigned long long)round (dds_phaseinc (center))); // => 44bit phase f0 center ref for delta_freq calculation on FPGA

        rp_PAC_transport_set_control (control);
}

void rp_PAC_start_transport (int control_mode, int nsamples, int tr_mode){
        // RESET TRANSPORT
        rp_PAC_configure_transport (PACPLL_CFG_TRANSPORT_RESET,
                                    SHR_DEC_DATA.Value (),
                                    nsamples,
                                    TRANSPORT_DECIMATION.Value (),
                                    tr_mode, // CHANNEL COMBINATION MODE
                                    AUX_SCALE.Value (),
                                    FREQUENCY_CENTER.Value()
                                    );
        
        if (control_mode == PACPLL_CFG_TRANSPORT_RESET)
                return;
        
        // START TRANSPORT IN SINGLE or LOOP MODE
        rp_PAC_transport_set_control (PACPLL_CFG_TRANSPORT_START | control_mode);
}

// BRAM write completed (single shot mode)?
int bram_ready(){
        return read_gpio_reg_int32 (6,1) & 0x8000 ? 1:0; // BRAM write completed (finshed flag set)
}

int bram_status(int bram_status[3]){
        int x12 = read_gpio_reg_int32 (6,1); // GPIO X12 : assign writeposition = { decimate_count[15:0], finished, BR_wpos[14:0] };
        bram_status[0] = x12 & 0x7fff;       // GPIO X12 : Transport WPos [15bit]
        bram_status[1] = (x12>>16) & 0xffff; // GPIO X12 : Transport Sample Count
        bram_status[2] = x12 & 0x8000 ? 1:0;      // GPIO X12 : Transport Finished
        return bram_status[2]; // finished (finished and run set)
}


/*
// ---------- NOT IMPLEMENTED/NO ROOM ---------------------
// Configure transport tau: time const or high speed IIR filter stages

void rp_PAC_set_tau_transport (double tau_dfreq, double tau_phase, double tau_exec, double tau_ampl){

#ifdef REMAP_TO_OLD_FPGA_VERSION
        return;
#endif
        //if (verbose == 1) fprintf(stderr, "PAC TAU TRANSPORT DBG: dFreq tau = %g ms ->  %d\n", tau_dfreq, (int)(Q32/ADC_SAMPLING_RATE/(1e-3*tau_dfreq)));
        if (tau_dfreq > 0.)
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_DFREQ, (int)(Q32/ADC_SAMPLING_RATE/(1e-3*tau_dfreq)));
        else
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_DFREQ, 0xffffffff);
        if (tau_phase > 0.)
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_PHASE, (int)(Q32/ADC_SAMPLING_RATE/(1e-3*tau_phase)));
        else
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_PHASE, 0xffffffff);
        if (tau_exec > 0.)
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_EXEC,  (int)(Q32/ADC_SAMPLING_RATE/(1e-3*tau_exec)));
        else
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_EXEC, 0xffffffff);
        if (tau_ampl > 0.)
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_AMPL,  (int)(Q32/ADC_SAMPLING_RATE/(1e-3*tau_ampl)));
        else
                set_gpio_cfgreg_uint32 (PACPLL_CFG_TRANSPORT_TAU_AMPL, 0xffffffff);
}
*/

// Phase Controller
// CONTROL[75] OUT[44] : [75-1-1:75-44]=43+1   m[24]  x  c[32]  = 56 M: 24{Q32},  P: 44{Q14}
void rp_PAC_set_dfreq_controller (double setpoint, double cp, double ci, double upper, double lower){
        if (verbose > 2) fprintf(stderr, "##Configure Controller: set= %g  Q22: %d    cp=%g ci=%g upper=%g lower=%g\n", setpoint, (int)(Q22 * setpoint), cp, ci, upper, lower); 

        /*
        double cp = 20. * log10 (1.6575e-5  * pll.auto_set_BW_Phase);
        double ci = 20. * log10 (1.7357e-10 * pll.auto_set_BW_Phase*pll.auto_set_BW_Phase);

        write_pll_variable32 (dsp_pll.icoef_Phase, pll.signum_ci_Phase * CPN(29)*pow (10.,pll.ci_gain_Phase/20.));
        // = ISign * CPN(29)*pow(10.,Igain/20.);
	
        write_pll_variable32 (dsp_pll.pcoef_Phase, pll.signum_cp_Phase * CPN(29)*pow (10.,pll.cp_gain_Phase/20.));
        // = PSign * CPN(29)*pow(10.,Pgain/20.);
        */

        set_gpio_cfgreg_int32 (PACPLL_CFG_DFREQ_CONTROLLER + PACPLL_CFG_SET,   (long long)(round(dds_phaseinc (setpoint)))); // 32bit dFreq (Full range is Q44)
        set_gpio_cfgreg_int32 (PACPLL_CFG_DFREQ_CONTROLLER + PACPLL_CFG_CP,    (long long)(QDFCOEF * cp)); // {32}
        set_gpio_cfgreg_int32 (PACPLL_CFG_DFREQ_CONTROLLER + PACPLL_CFG_CI,    (long long)(QDFCOEF * ci));
        set_gpio_cfgreg_int32 (PACPLL_CFG_DFREQ_CONTROLLER + PACPLL_CFG_UPPER, (long long)round (Q31*upper/10.0)); // => 15.16 32Q16 Control (Bias, Z, ...) @ +/-10V range in Q31
        set_gpio_cfgreg_int32 (PACPLL_CFG_DFREQ_CONTROLLER + PACPLL_CFG_LOWER, (long long)round (Q31*lower/10.0));
}



// new GPIO READING THREAD with FIR
void *thread_gpio_reading_FIR(void *arg) {
        unsigned long x8,x9;
        int i,j,x,y;
        for (j=0; j<GPIO_FIR_LEN; j++){
                gpio_reading_FIRV_buffer[GPIO_READING_DDS_X8][j] = 0;
                gpio_reading_FIRV_buffer[GPIO_READING_DDS_X9][j] = 0;
        }
        for (i=0; i<MAX_FIR_VALUES; i++){
                gpio_reading_FIRV_vector[i] = 0.0;
                for (j=0; j<GPIO_FIR_LEN; j++)
                        gpio_reading_FIRV_buffer[i][j] = 0;
        }
        for(j=0; gpio_reading_control; ){

                pthread_mutex_lock (&gpio_reading_mutexsum);


                x = read_gpio_reg_int32 (1,0); // GPIO X1 : LMS A (cfg + 0x1000)
                gpio_reading_FIRV_vector[GPIO_READING_LMS_A] -= (double)gpio_reading_FIRV_buffer[GPIO_READING_LMS_A][j];
                gpio_reading_FIRV_vector[GPIO_READING_LMS_A] += (double)x;
                gpio_reading_FIRV_buffer[GPIO_READING_LMS_A][j] = x;

                x = read_gpio_reg_int32 (1,1); // GPIO X2 : LMS B (cfg + 0x1008)
                gpio_reading_FIRV_vector[GPIO_READING_LMS_B] -= (double)gpio_reading_FIRV_buffer[GPIO_READING_LMS_B][j];
                gpio_reading_FIRV_vector[GPIO_READING_LMS_B] += (double)x;
                gpio_reading_FIRV_buffer[GPIO_READING_LMS_B][j] = x;

                //x = read_gpio_reg_int32 (3,0); // GPIO X5 : MDC
                //gpio_reading_FIRV_vector[GPIO_READING_OFSET] -= (double)gpio_reading_FIRV_buffer[GPIO_READING_OFSET][j];
                //gpio_reading_FIRV_vector[GPIO_READING_OFSET] += (double)x;
                //gpio_reading_FIRV_buffer[GPIO_READING_OFSET][j] = x;
                
                x = read_gpio_reg_int32 (2,1); // GPIO X4 : CORDIC SQRT (AM2=A^2+B^2) = Amplitude Monitor
                gpio_reading_FIRV_vector[GPIO_READING_AMPL] -= (double)gpio_reading_FIRV_buffer[GPIO_READING_AMPL][j];
                gpio_reading_FIRV_vector[GPIO_READING_AMPL] += (double)x;
                gpio_reading_FIRV_buffer[GPIO_READING_AMPL][j] = x;

                x = read_gpio_reg_int32 (5,1); // GPIO X10: CORDIC ATAN(X/Y) = Phase Monitor
                gpio_reading_FIRV_vector[GPIO_READING_PHASE] -= (double)gpio_reading_FIRV_buffer[GPIO_READING_PHASE][j];
                gpio_reading_FIRV_vector[GPIO_READING_PHASE] += (double)x;
                gpio_reading_FIRV_buffer[GPIO_READING_PHASE][j] = x;

                x = read_gpio_reg_int32 (4,0); // GPIO X7 : Exec Ampl Control Signal (signed)
                gpio_reading_FIRV_vector[GPIO_READING_EXEC] -= (double)gpio_reading_FIRV_buffer[GPIO_READING_EXEC][j];
                gpio_reading_FIRV_vector[GPIO_READING_EXEC] += (double)x;
                gpio_reading_FIRV_buffer[GPIO_READING_EXEC][j] = x;
                
                x8 = read_gpio_reg_uint32 (4,1); // GPIO X8 : DDS Phase Inc (Freq.) upper 32 bits of 44 (unsigned)
                x9 = read_gpio_reg_uint32 (5,0); // GPIO X9 : DDS Phase Inc (Freq.) lower 32 bits of 44 (unsigned)
                gpio_reading_FIRV_vector[GPIO_READING_DDS_FREQ] -= dds_phaseinc_to_freq(  ((unsigned long long)gpio_reading_FIRV_buffer_DDS[GPIO_READING_DDS_X8][j]<<(44-32))
                                                                                        + ((unsigned long long)gpio_reading_FIRV_buffer_DDS[GPIO_READING_DDS_X9][j]>>(64-44)));
                gpio_reading_FIRV_vector[GPIO_READING_DDS_FREQ] += dds_phaseinc_to_freq(  ((unsigned long long)x8<<(44-32))
                                                                                        + ((unsigned long long)x9>>(64-44)));
                gpio_reading_FIRV_buffer_DDS[GPIO_READING_DDS_X8][j] = x8;
                gpio_reading_FIRV_buffer_DDS[GPIO_READING_DDS_X9][j] = x9;

                x = read_gpio_reg_int32 (6,0); // GPIO X11 : dFreq
                gpio_reading_FIRV_vector[GPIO_READING_DFREQ] -= dds_phaseinc_rel_to_freq((long long)(gpio_reading_FIRV_buffer[GPIO_READING_DFREQ][j]));
                gpio_reading_FIRV_vector[GPIO_READING_DFREQ] += dds_phaseinc_rel_to_freq((long long)(x));
                gpio_reading_FIRV_buffer[GPIO_READING_DFREQ][j] = x;

                x = read_gpio_reg_int32 (7,0); // GPIO X13: control dFreq value
                gpio_reading_FIRV_vector[GPIO_READING_CONTROL_DFREQ] -= (double)gpio_reading_FIRV_buffer[GPIO_READING_CONTROL_DFREQ][j];
                gpio_reading_FIRV_vector[GPIO_READING_CONTROL_DFREQ] += (double)x;
                gpio_reading_FIRV_buffer[GPIO_READING_CONTROL_DFREQ][j] = x;

                pthread_mutex_unlock (&gpio_reading_mutexsum);

                ++j;
                j &= 0x3ff; // 1024
                usleep (1000); // updated every 1ms
        }
        return NULL;
}


/*
 * Get Single Direct FPGA Reading via GPIO
 * ========================================
 * reading_vector[0] := LMS Amplitude estimation (from A,B)
 * reading_vector[1] := LMS Phase estimation (from A,B)
 * reading_vector[2] := LMS A
 * reading_vector[3] := LMS B
 * reading_vector[4] := FPGA CORDIC Amplitude Monitor
 * reading_vector[5] := FPGA CORDIC Phase Monitor
 * reading_vector[6] := x5 M-DC_LMS
 * reading_vector[7] := x6
 * reading_vector[8] := x7 Exec Amp Mon
 * reading_vector[9] := DDS Freq
 * reading_vector[10]:= x3 Monitor (In0 Signal, LMS inpu)
 * reading_vector[11]:= x3
 * reading_vector[12]:= x11
 * reading_vector[13]:= x12
 */

#define READING_MAX_VALUES 14

// compat func
void rp_PAC_get_single_reading_FIR (double reading_vector[READING_MAX_VALUES]){
        double a,b,v,p;
        // LMS Detector Readings and double precision conversions
        a = gpio_reading_FIRV_vector[GPIO_READING_LMS_A]/GPIO_FIR_LEN / QLMS;
        b = gpio_reading_FIRV_vector[GPIO_READING_LMS_B]/GPIO_FIR_LEN / QLMS;
        // hi level atan2 and sqrt
        v=sqrt (a*a+b*b);
        p=atan2 ((a-b),(a+b));
        reading_vector[0] = v*1000.; // LMS Amplitude (Volume) in mVolts (from A,B)
        reading_vector[1] = p/M_PI*180.; // LMS Phase (from A,B)
        reading_vector[2] = a; // LMS A (Real)
        reading_vector[3] = b; // LMS B (Imag)

        // FPGA CORDIC based convertions
        reading_vector[4] = gpio_reading_FIRV_vector[GPIO_READING_AMPL]/GPIO_FIR_LEN * 1000./QCORDICSQRT; //reading_vector[4]; // LMS Amplitude (Volume) in mVolts (from A,B)
        reading_vector[5] = gpio_reading_FIRV_vector[GPIO_READING_PHASE]/GPIO_FIR_LEN * 180./QCORDICATAN/M_PI;   //reading_vector[5]; // LMS Phase (from A,B) in deg

        // N/A
        //reading_vector[6] = x5*1000.;  // X5
        //reading_vector[7] = x6;  // X4
        //reading_vector[10] = x3; // M (LMS input Signal)
        //reading_vector[11] = x3; // M1 (LMS input Signal-DC), tests...

        // Controllers
        reading_vector[8] = gpio_reading_FIRV_vector[GPIO_READING_EXEC]/GPIO_FIR_LEN  * 1000/QEXEC;
        reading_vector[9] = gpio_reading_FIRV_vector[GPIO_READING_DDS_FREQ]/GPIO_FIR_LEN;
        reading_vector[12] = gpio_reading_FIRV_vector[GPIO_READING_DFREQ]/GPIO_FIR_LEN;
        reading_vector[13] = gpio_reading_FIRV_vector[GPIO_READING_CONTROL_DFREQ]/GPIO_FIR_LEN * 10000/Q31;


        SIGNAL_GPIOX[0] = read_gpio_reg_int32 (1,0); // GPIO X1 : LMS A (cfg + 0x1000)
        SIGNAL_GPIOX[1] = read_gpio_reg_int32 (1,1); // GPIO X2 : LMS B (cfg + 0x1008)
        SIGNAL_GPIOX[2] = read_gpio_reg_int32 (2,0); // GPIO X3 : LMS DBG1 :: M (LMS input Signal) (cfg + 0x2000) ===> used for DC OFFSET calculation
        SIGNAL_GPIOX[3] = read_gpio_reg_int32 (2,1); // GPIO X4 : CORDIC SQRT (AM2=A^2+B^2) = Amplitude Monitor
        SIGNAL_GPIOX[4] = read_gpio_reg_int32 (3,0); // GPIO X5 : MDC
        SIGNAL_GPIOX[5] = read_gpio_reg_int32 (3,1); // GPIO X6 : XXX
        SIGNAL_GPIOX[6] = read_gpio_reg_int32 (4,0); // GPIO X7 : Exec Ampl Control Signal (signed)
        SIGNAL_GPIOX[7] = read_gpio_reg_uint32 (4,1); // GPIO X8 : DDS Phase Inc (Freq.) upper 32 bits of 44 (signed)
        SIGNAL_GPIOX[8] = read_gpio_reg_uint32 (5,0); // GPIO X9 : DDS Phase Inc (Freq.) lower 32 bits of 44 (signed)
        SIGNAL_GPIOX[9] = read_gpio_reg_int32 (5,1); // GPIO X10: CORDIC ATAN(X/Y) = Phase Monitor
        SIGNAL_GPIOX[10]= read_gpio_reg_int32 (6,0); // GPIO X11 : dFreq
        SIGNAL_GPIOX[11]= read_gpio_reg_int32 (6,1); // GPIO X12 BRAM write position
        SIGNAL_GPIOX[12]= read_gpio_reg_int32 (7,0); // GPIO X13: control dFreq
        SIGNAL_GPIOX[13]= read_gpio_reg_int32 (7,1); // GPIO X14
        //SIGNAL_GPIOX[14] = read_gpio_reg_int32 (8,0); // GPIO X15
        //SIGNAL_GPIOX[15] = read_gpio_reg_int32 (8,1); // GPIO X16

}


// Get all GPIO mapped data / system state snapshot
void rp_PAC_get_single_reading (double reading_vector[READING_MAX_VALUES]){
        int x,y,xx7;
        unsigned long xx8, xx9, uix;
        double a,b,v,p,x3,x4,x5,x6,x7,x8,x9,x10,qca,pfpga,x11,x12;
        
        SIGNAL_GPIOX[0] = x = read_gpio_reg_int32 (1,0); // GPIO X1 : LMS A (cfg + 0x1000)
        a=(double)x / QLMS;
        SIGNAL_GPIOX[1] = x = read_gpio_reg_int32 (1,1); // GPIO X2 : LMS B (cfg + 0x1008)
        b=(double)x / QLMS;
        v=sqrt (a*a+b*b);
        p=atan2 ((a-b),(a+b));
        
        SIGNAL_GPIOX[2] = x = read_gpio_reg_int32 (2,0); // GPIO X3 : LMS DBG1 :: M (LMS input Signal) (cfg + 0x2000) ===> used for DC OFFSET calculation
        x3=(double)x / QLMS;
        SIGNAL_GPIOX[3] = x = read_gpio_reg_int32 (2,1); // GPIO X4 : CORDIC SQRT (AM2=A^2+B^2) = Amplitude Monitor
        x4=(double)x / QCORDICSQRT;
        //x4=(double)x / QEXEC;
        SIGNAL_GPIOX[4] = x = read_gpio_reg_int32 (3,0); // GPIO X5 : MDC
        x5=(double)x / QLMS; // MDC    /QCORDICSQRTFIR;
        SIGNAL_GPIOX[5] = x = read_gpio_reg_int32 (3,1); // GPIO X6 : XXX
        x6=(double)x / QCORDICATANFIR;
        SIGNAL_GPIOX[6] = xx7 = x = read_gpio_reg_int32 (4,0); // GPIO X7 : Exec Ampl Control Signal (signed)
        x7=(double)x / QEXEC;

        SIGNAL_GPIOX[7] = xx8 = x = read_gpio_reg_uint32 (4,1); // GPIO X8 : DDS Phase Inc (Freq.) upper 32 bits of 44 (signed)
        SIGNAL_GPIOX[8] = xx9 = x = read_gpio_reg_uint32 (5,0); // GPIO X9 : DDS Phase Inc (Freq.) lower 32 bits of 44 (signed)
        x9=(double)x / QLMS;

        SIGNAL_GPIOX[9] = x = read_gpio_reg_int32 (5,1); // GPIO X10: CORDIC ATAN(X/Y) = Phase Monitor
        x10 = (double)x / QCORDICATAN; // ATAN 24bit 3Q21 

        SIGNAL_GPIOX[10] = x = read_gpio_reg_int32 (6,0); // GPIO X11 : dFreq
        x11=(double)(x);

        SIGNAL_GPIOX[11] = x = read_gpio_reg_int32 (6,1); // GPIO X12 BRAM write position
        x12=(double)(x);

        SIGNAL_GPIOX[12] = read_gpio_reg_int32 (7,0); // GPIO X13: control dFreq
        SIGNAL_GPIOX[13] = read_gpio_reg_int32 (7,1); // GPIO X14
        //SIGNAL_GPIOX[14] = read_gpio_reg_int32 (8,0); // GPIO X15
        //SIGNAL_GPIOX[15] = read_gpio_reg_int32 (8,1); // GPIO X16

        
        // LMS Detector Readings and double precision conversions
        reading_vector[0] = v*1000.; // LMS Amplitude (Volume) in mVolts (from A,B)
        reading_vector[1] = p/M_PI*180.; // LMS Phase (from A,B)
        reading_vector[2] = a; // LMS A (Real)
        reading_vector[3] = b; // LMS B (Imag)

        // FPGA CORDIC based convertions
        reading_vector[4] = x4*1000.;  // FPGA CORDIC (SQRT) Amplitude Monitor in mVolt
        reading_vector[5] = x10/M_PI*180.; // FPGA CORDIC (ATAN) Phase Monitor

        reading_vector[6] = x5*1000.;  // X5
        reading_vector[7] = x6;  // X4

        reading_vector[8] = x7*1000.0;  // Exec Ampl Control Signal (signed) in mV
        reading_vector[9] = dds_phaseinc_to_freq(((unsigned long long)xx8<<(44-32)) + ((unsigned long long)xx9>>(64-44)));  // DDS Phase Inc (Freq.) upper 32 bits of 44 (signed)

        reading_vector[10] = x3; // M (LMS input Signal)
        reading_vector[11] = x3; // M1 (LMS input Signal-DC), tests...

        reading_vector[12] = dds_phaseinc_rel_to_freq((long long)(x11)); // delta Freq
        reading_vector[13] = 10000.*(double)SIGNAL_GPIOX[12]/Q31; // control of dFreq controller

        if (verbose > 2){
                if (verbose == 1) fprintf(stderr, "PAC TRANSPORT DBG: BRAM WritePos=%04X Sample#=%04x Finished=%d\n", x&0x7fff, x>>16, x&0x8000 ? 1:0);
                else if (verbose > 4) fprintf(stderr, "PAC DBG: A:%12f  B:%12f X1:%12f X2:%12f X3:%12f X4:%12f X5:%12f X6:%12f X7:%12f X8:%12f X9:%12f X10:%12f X11:%12f X12:%12f\n", a,b,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12);
                else if (verbose > 3) fprintf(stderr, "PAC READING: Ampl=%10.4f V Phase=%10.4f deg a=%10.4f b=%10.4f  FPGA: %10.4f %10.4f FIR: %10.4f %10.4f  M %10.4f V  pfpga=%10.4f\n", v,p, a,b, x4, x10, x5, x6, x3, pfpga);
        }
}
        

/*
 * RedPitaya A9 JSON Interface
 * ------------------------------------------------------------
 */



// manage PAC configuration and trigger for tune single shot operations
void set_PAC_config()
{
        //double reading_vector[READING_MAX_VALUES];
        //int status[3];

        static double VOL_set_prev = 0.0;
        static double AM_set_prev = 0.0;
        static double PH_set_prev = 0.0;
        static int AM_sw_prev = 0;
        static int PH_sw_prev = 0;
        //static int DF_sw_prev = 0;
        static int uw_sw_prev = 0;
        
        useconds_t code_delay = 70; // approx. measured post time we have regardless
        useconds_t trigger_delay = (useconds_t)SET_SINGLESHOT_TRIGGER_POST_TIME.Value ();
        if (trigger_delay >= code_delay)
                trigger_delay -= code_delay;
        else
                trigger_delay = 0;
        

        // prepare for trigger
        /*
        if ( OPERATION.Value () == 2 || OPERATION.Value () == 4 ){ // auto SINGLE SHOT only on OP Param change or always in SCOPE mode
                if ( AM_set_prev != AMPLITUDE_FB_SETPOINT.Value () || VOL_set_prev != VOLUME_MANUAL.Value() || PH_set_prev != PHASE_FB_SETPOINT.Value () || OPERATION.Value () == 4 ){
                        if (SET_SINGLESHOT_TRANSPORT_TRIGGER.Value () && bram_ready ()){
                                // Reset and rearm -- Auto Re-Trigger SCOPE in SS Mode if ready
                                rp_PAC_configure_transport (PACPLL_CFG_TRANSPORT_RESET,
                                                            SHR_DEC_DATA.Value (), 4096, TRANSPORT_DECIMATION.Value (), TRANSPORT_MODE.Value (), AUX_SCALE.Value (), FREQUENCY_CENTER.Value());
                        }
                }
        } else { // update transport settings on the fly
                rp_PAC_configure_transport (-1,
                                            SHR_DEC_DATA.Value (),
                                            -1,
                                            TRANSPORT_DECIMATION.Value (),
                                            TRANSPORT_MODE.Value (),
                                            AUX_SCALE.Value (),
                                            FREQUENCY_CENTER.Value()
                                            );
        }
        */
        if ( OPERATION.Value () != 4  )
                rp_PAC_configure_transport (-1,
                                            SHR_DEC_DATA.Value (),
                                            -1,
                                            TRANSPORT_DECIMATION.Value (),
                                            TRANSPORT_MODE.Value (),
                                            AUX_SCALE.Value (),
                                            FREQUENCY_CENTER.Value()
                                            );


        if (AM_set_prev != AMPLITUDE_FB_SETPOINT.Value () && OPERATION.Value () == 4 && SET_SINGLESHOT_TRANSPORT_TRIGGER.Value ()){
                rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_SINGLE, 4096, TRANSPORT_MODE.Value ());
                usleep(trigger_delay);
        }
        rp_PAC_set_amplitude_controller (
                                         AMPLITUDE_FB_SETPOINT.Value ()/1000., // mv to V
                                         AMPLITUDE_FB_CP.Value (),
                                         AMPLITUDE_FB_CI.Value (),
                                         EXEC_FB_UPPER.Value ()/1000., // mV -> +/-1V
                                         EXEC_FB_LOWER.Value ()/1000.
                                         );
        AM_set_prev = AMPLITUDE_FB_SETPOINT.Value ();
        
        if (PH_set_prev != PHASE_FB_SETPOINT.Value () && OPERATION.Value () == 4 && SET_SINGLESHOT_TRANSPORT_TRIGGER.Value ()){
                rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_SINGLE, 4096, TRANSPORT_MODE.Value ());
                usleep(trigger_delay);
        }
        rp_PAC_set_phase_controller (
                                     PHASE_FB_SETPOINT.Value ()/180.*M_PI, // Phase
                                     PHASE_FB_CP.Value (),
                                     PHASE_FB_CI.Value (),
                                     FREQ_FB_UPPER.Value (), // Hz
                                     FREQ_FB_LOWER.Value (),
                                     PHASE_HOLD_AM_NOISE_LIMIT.Value ()/1000. // mV to V
                                     );
        PH_set_prev = PHASE_FB_SETPOINT.Value ();
        
        if (VOL_set_prev != VOLUME_MANUAL.Value() && OPERATION.Value () == 4 && SET_SINGLESHOT_TRANSPORT_TRIGGER.Value ()){
                rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_SINGLE, 4096, TRANSPORT_MODE.Value ());
                usleep(trigger_delay);
        }
        rp_PAC_set_volume (VOLUME_MANUAL.Value() / 1000.); // mV -> V
        VOL_set_prev = VOLUME_MANUAL.Value();

        rp_PAC_set_pactau (PACTAU.Value(), PACATAU.Value(), PAC_DCTAU.Value() * 1e-3); // periods, periods, ms -> s

        rp_PAC_set_qcontrol (QC_GAIN.Value (), QC_PHASE.Value ());

        if ( (AM_sw_prev != AMPLITUDE_CONTROLLER.Value() || PH_sw_prev!= PHASE_CONTROLLER.Value () || uw_sw_prev != PHASE_UNWRAPPING_ALWAYS.Value ())
             && OPERATION.Value () == 4 && SET_SINGLESHOT_TRANSPORT_TRIGGER.Value ()){
                AM_sw_prev = AMPLITUDE_CONTROLLER.Value ();
                PH_sw_prev = PHASE_CONTROLLER.Value ();
                uw_sw_prev = PHASE_UNWRAPPING_ALWAYS.Value ();
                rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_SINGLE, 4096, TRANSPORT_MODE.Value ());
                usleep(trigger_delay);
        }
        
        // in tune mode disable controllers autmatically and do not touch DDS settings, but allow Q Control
        if (OPERATION.Value() < 6){
                rp_PAC_adjust_dds (FREQUENCY_MANUAL.Value());
                rp_PAC_configure_switches (PHASE_CONTROLLER.Value ()?1:0,
                                           AMPLITUDE_CONTROLLER.Value ()?1:0,
                                           PHASE_UNWRAPPING_ALWAYS.Value ()?1:0,
                                           QCONTROL.Value ()?1:0,
                                           LCK_AMPLITUDE.Value ()?1:0,
                                           LCK_PHASE.Value ()?1:0,
                                           DFREQ_CONTROLLER.Value ()?1:0);
        } else {
                rp_PAC_configure_switches (0, 0, PHASE_UNWRAPPING_ALWAYS.Value ()?1:0, QCONTROL.Value ()?1:0, LCK_AMPLITUDE.Value ()?1:0, LCK_PHASE.Value ()?1:0, 0);
        }

        /*
        rp_PAC_set_tau_transport (
                                  TRANSPORT_TAU_DFREQ.Value (), // tau given in ms (1-e3)
                                  TRANSPORT_TAU_PHASE.Value (),
                                  TRANSPORT_TAU_EXEC.Value (),
                                  TRANSPORT_TAU_AMPL.Value ()
                                  );
        */                        

        rp_PAC_set_dfreq_controller (
                                     DFREQ_FB_SETPOINT.Value (), // dHz -> ...
                                     DFREQ_FB_CP.Value (),
                                     DFREQ_FB_CI.Value (),
                                     DFREQ_FB_UPPER.Value ()/1000., // mV -> V
                                     DFREQ_FB_LOWER.Value ()/1000.
                                     );

        rp_PAC_set_pulse_form (
                               PULSE_FORM_BIAS0.Value (),
                               PULSE_FORM_BIAS1.Value (),
                               PULSE_FORM_PHASE0.Value (),
                               PULSE_FORM_PHASE1.Value (),
                               PULSE_FORM_WIDTH0.Value (),
                               PULSE_FORM_WIDTH0IF.Value (),
                               PULSE_FORM_WIDTH1.Value (),
                               PULSE_FORM_WIDTH1IF.Value (),
                               PULSE_FORM_HEIGHT0.Value (),
                               PULSE_FORM_HEIGHT0IF.Value (),
                               PULSE_FORM_HEIGHT1.Value (),
                               PULSE_FORM_HEIGHT1IF.Value (),
                               PULSE_FORM_SHAPEXW.Value (),
                               PULSE_FORM_SHAPEXWIF.Value (),
                               PULSE_FORM_SHAPEX.Value (),
                               PULSE_FORM_SHAPEXIF.Value ()
                               );
}


const char *rp_app_desc(void)
{
        return (const char *)"Red Pitaya PACPLL.\n";
}


int rp_app_init(void)
{
        double reading_vector[READING_MAX_VALUES];

        fprintf(stderr, "Loading PACPLL application\n");

        // Initialization of PAC api
        if (rp_PAC_App_Init() != RP_OK) 
                {
                        fprintf(stderr, "Red Pitaya PACPLL API init failed!\n");
                        return EXIT_FAILURE;
                }
        else fprintf(stderr, "Red Pitaya PACPLL API init success!\n");

        rp_PAC_auto_dc_offset_adjust ();

        //Set signal update interval
        CDataManager::GetInstance()->SetSignalInterval(SIGNAL_UPDATE_INTERVAL);
        CDataManager::GetInstance()->SetParamInterval(PARAMETER_UPDATE_INTERVAL);

        // Init PAC
        set_PAC_config();

        rp_PAC_get_single_reading (reading_vector);

        // init block transport for scope
        rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_LOOP, 4096, TRANSPORT_MODE.Value ());
        
        return 0;
}


int rp_app_exit(void)
{
        fprintf(stderr, "Unloading Red Pitaya PACPLL application\n");

        rp_PAC_App_Release();
        
        return 0;
}


int rp_set_params(rp_app_params_t *p, int len)
{
        return 0;
}


int rp_get_params(rp_app_params_t **p)
{
        return 0;
}


int rp_get_signals(float ***s, int *sig_num, int *sig_len)
{
        return 0;
}


//READ
//return *((volatile uint32_t *) ( ((uint8_t*)map) + offset ));

//WRITE
//*((volatile uint32_t *) ( ((uint8_t*)map)+ offset )) = value;

int check_trigger(int t, int slope, int y){
        int threashold = 65;
        // fprintf(stderr, "CHECK TRIGGER t:%d  slope %d  y=%d \n",  t, slope, y);
        if (slope > 0){ // Pos Zero X
                if (y < -threashold) // neg theashold to start checking (5mV)
                        return -1;
                if (t < 0 && y > 0)
                        return 1;
        } else { // Neg Zero X
                if (y > threashold) // pos theashold to start checking
                        return -1;
                if (t < 0 && y < 0)
                        return 1;
        }
        return t;
}

int check_trigger_bit(int t, int tm, unsigned int y){
        unsigned int b = y & (1 << ( tm >> 1 ));
        // TRIGGER simple on rising edge zero x
        if (tm & 1){ // Pos Edge
                if (b == 0)
                        return -1;
                if (t == -1 && b)
                        return 1;
        } else {
                if (b)
                        return -1;
                if (t == -1 && b == 0)
                        return 1;
        }
        return t;
}

void read_bram (int n, int dec, int t_mode, double gain1, double gain2){
        int t,k;
        int pos;
        size_t i = 12;
        // size_t N = 8*n;
        double trigger_level = BRAM_SCOPE_TRIGGER_LEVEL.Value ();
        int index_shift  = 8*BRAM_SCOPE_SHIFT_POINTS.Value ();
        int trigger_mode = BRAM_SCOPE_TRIGGER_MODE.Value ();
        int trigger_pos  = trigger_mode ? BRAM_SCOPE_TRIGGER_POS.Value () : 0;
        size_t trigger_adr =  (size_t)(8*trigger_pos);
        // if (verbose == 1) fprintf(stderr, "SCOPE TRIGGER: m=%d pos=%d\n",  trigger_mode, trigger_pos );

        switch (t_mode){
        case 0: // AXIS1:  IN1, IN2
                if (trigger_mode >= 1 && trigger_mode <= 4){
                        int tlv = (int)round(trigger_level/1000.*Q13);
                        i += trigger_adr; // start
                        // advance i to trigger position
                        for (t=0, pos=0; pos < 1000; ++pos){
                                int32_t ix32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // IN1 (14)
                                int32_t iy32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // IN2 (14)
                                if (pos < 100)
                                        continue;
                                if (trigger_mode > 2)
                                        t=check_trigger (t, trigger_mode&1 ? 1:0, (int16_t)(iy32&0xffff)-tlv);
                                else
                                        t=check_trigger (t, trigger_mode&1 ? 1:0, (int16_t)(ix32&0xffff)-tlv);
                                if (t == 1 && verbose == 1) fprintf(stderr, "SCOPE TRIGGER F adr:%d  at %d  @x= %d  @y= %d t=%d\n",  trigger_adr, i, ix32, iy32, t);
                                if (t == 1) break;
                        }
                        i -= trigger_adr; // go back by pre-trigger time
                        if (verbose == 1) fprintf(stderr, "SCOPE TRIGGER AT: adr:%d  at %d t=%d\n",  trigger_adr, i, t);
                }
                i += index_shift;
                for (k=0; k < SIGNAL_SIZE_DEFAULT; ++k){
                        uint32_t ix32 = *((uint32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // IN1 (14)
                        uint32_t iy32 = *((uint32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // IN2 (14)
                        SIGNAL_CH1[k] = (float)((int16_t)(ix32&0xffff))*gain1/Q13*1000.;
                        SIGNAL_CH2[k] = (float)((int16_t)(iy32&0xffff))*gain2/Q13*1000.;
                }
                break;
        case 7: // DDR mode: IN1, IN2 -- only for dec=1
                if (trigger_mode >= 1 && trigger_mode <= 4){
                        int tlv = (int)round(trigger_level/1000.*Q13);
                        i += trigger_adr/2; // start
                        // advance i to trigger position
                        for (t=0, pos=0; pos < 1000; ++pos){
                                int32_t ix32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // IN1 (14)
                                int32_t iy32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // IN2 (14)
                                if (trigger_mode > 2)
                                        t=check_trigger (t, trigger_mode&1 ? 1:0, (int16_t)(iy32&0xffff)-tlv);
                                else
                                        t=check_trigger (t, trigger_mode&1 ? 1:0, (int16_t)(ix32&0xffff)-tlv);
                                if (t == 1 && verbose == 1) fprintf(stderr, "SCOPE TRIGGER F adr:%d  at %d  @x= %d  @y= %d t=%d\n",  trigger_adr, i, ix32, iy32, t);
                                if (t == 1) break;
                        }
                        i -= trigger_adr/2; // go back by pre-trigger time
                        if (verbose == 1) fprintf(stderr, "SCOPE TRIGGER AT: adr:%d  at %d t=%d\n",  trigger_adr, i, t);
                }
                i += index_shift;
                for (k=0; k < SIGNAL_SIZE_DEFAULT; ++k){
                        uint32_t ix32 = *((uint32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // IN1 (14)
                        uint32_t iy32 = *((uint32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // IN2 (14)
                        SIGNAL_CH1[k] = (float)((int16_t)(ix32&0xffff))*gain1/Q13*1000.;
                        SIGNAL_CH2[k] = (float)((int16_t)(iy32&0xffff))*gain2/Q13*1000.;
                        ++k;
                        SIGNAL_CH1[k] = (float)((int16_t)((ix32>>16)&0xffff))*gain1/Q13*1000.;
                        SIGNAL_CH2[k] = (float)((int16_t)((iy32>>16)&0xffff))*gain2/Q13*1000.;
                }
                break;
        case 1: // AXIS6: DC Filter: In1(AC), In1(DC)
                i += index_shift;
                for (k=0; k < SIGNAL_SIZE_DEFAULT; ++k){
                        int32_t ix32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // IN1 - AC (IN1-MDC AC dec + filtered) (16)
                        int32_t iy32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // IN1 - DC (IN1 IIR LP dec + filtered) (16)
                        SIGNAL_CH1[k] = (float)ix32*gain1/Q15*1000.;
                        SIGNAL_CH2[k] = (float)iy32*gain2/Q15*1000. * 0.5; // where is the 1/2 ???
                }
                break;
        case 2: // AXIS5/2: AMC Ampl, Exec
                i += index_shift;
                for (k=0; k < SIGNAL_SIZE_DEFAULT; ++k){
                        int32_t ix32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // AMC: Ampl (24)   [[AMPLITUDE_FB_SETPOINT.Value ()]]
                        int32_t iy32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // AMC: Ampl Exec (32)
                        SIGNAL_CH1[k]  = (float)ix32 / QCORDICSQRT*1000.;
                        SIGNAL_CH2[k]  = (float)iy32 / QEXEC*1000.;
                }
                break;
        case 3: // AXIS4/3delta: PHC Phase Error, delta Freq
                i += index_shift;
                for (k=0; k < SIGNAL_SIZE_DEFAULT; ++k){
                        int32_t ix32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // PHC: (Phase (24) - PHASE_FB_SETPOINT.Value ())
                        int32_t iy32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // PHC: Freq (48)-Lower(48)
                        SIGNAL_CH1[k]  = (float)(ix32-phase_setpoint_qcordicatan) / QCORDICATAN*180./M_PI;
                        SIGNAL_CH2[k]  = (float)dds_phaseinc_rel_to_freq ((long long)iy32); // correct to 44
                }
                break;
        case 4: // AXIS4/5: Phase, Ampl (Tuning configuration)
                i += index_shift;
                for (k=0; k < SIGNAL_SIZE_DEFAULT; ++k){
                        int32_t ix32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // PHC: Phase (24)
                        int32_t iy32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // AMC: Ampl (24)
                        SIGNAL_CH1[k]  = (float)ix32 / QCORDICATAN*180./M_PI;
                        SIGNAL_CH2[k]  = (float)iy32 / QCORDICSQRT*1000.;
                }
                break;
        case 5: // Operation Transfer: Phase, delta Freq, [**McBSP3,4: Ampl, Exec]
                i += index_shift;
                for (k=0; k < SIGNAL_SIZE_DEFAULT; ++k){
                        int32_t ix32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // Phase (24)
                        int32_t iy32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // Freq (48)-Lower(48)
                        SIGNAL_CH1[k]  = (float)ix32 / QCORDICATAN*180./M_PI;
                        SIGNAL_CH2[k]  = (float)dds_phaseinc_rel_to_freq ((long long)iy32); // correct to 44
                }
                break;
        case 6: // Operation Transfer: delta Freq, DfControl Value
                i += index_shift;
                for (k=0; k < SIGNAL_SIZE_DEFAULT; ++k){
                        int32_t ix32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // dFreq (48) in (32)
                        int32_t iy32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // dFControl (32) SQ31 "x 10V"
                        SIGNAL_CH1[k]  = (float)dds_phaseinc_rel_to_freq ((long long)ix32); // correct to 44
                        SIGNAL_CH2[k]  = (float)iy32/Q31*10.0; // correct to Q31 * 10V
                }
                break;
        case 8: // Debug and Testing Channels [McBSP bits]
                i += index_shift;
                for (k=0; k < SIGNAL_SIZE_DEFAULT; ++k){
                        int32_t ix32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // IN1 - AC (IN1-MDC AC dec + filtered) (16)
                        int32_t iy32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // IN1 - DC (IN1 IIR LP dec + filtered) (16)
                        SIGNAL_CH1[k] = (float)ix32*gain1/Q22*1000.;
                        SIGNAL_CH2[k] = (float)iy32*gain2/Q22*1000.; // where is the 1/2 ???
                }
#ifdef GPIODEBUG_ON
                i += index_shift;
                if (trigger_mode >= 5){
                        i += trigger_adr;
                        // advance i to trigger position
                        for (t=0, pos=0; pos < 1000; ++pos){
                                int32_t ix32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // DBG0
                                int32_t iy32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // DBG1
                                if (verbose == 1)
                                        if ((ix32>>5) & 0xff == 0xff)
                                                t=1;
                                else
                                        t=check_trigger_bit (t, trigger_mode-4, (unsigned int)ix32);
                                //assign dbgC = {data_read[16-5-8-1:0], frame_bit_counter[7:0], rtrigger, tx, reg_data_rx, frame_start, clkr};
                                if (t == 1 && verbose == 1) fprintf(stderr, "SCOPE TRIGGER BIN adr:%d  at %d  @x= %d  @y= %d t=%d\n",  trigger_adr, i, ix32, iy32, t);

                                if (t == 1) break;
                                
                        }
                        i -= trigger_adr;
                }
                for (k=0; k < SIGNAL_SIZE_DEFAULT; ++k){
                        uint32_t ix32 = *((uint32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4;
                        uint32_t iy32 = *((uint32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4;
                        SIGNAL_CH1[k] = (float)((uint16_t)(ix32&0xffff));
                        SIGNAL_CH2[k] = (float)((int16_t)(iy32&0xffff))*gain2/Q13*1000.; // get 1st in lo16
                        if (dec == 1){ // DDR in hi/low 16
                                ++k;
                                SIGNAL_CH1[k] = (float)((uint16_t)((ix32>>16)&0xffff));
                                SIGNAL_CH2[k] = (float)((int16_t)((iy32>>16)&0xffff))*gain2/Q13*1000.; // get 2nd in hi16
                        }
                }
#endif
                break;
        default : // ***** OTHER -- PLAIN DATA *****
                i += index_shift;
                for (k=0; k < SIGNAL_SIZE_DEFAULT; ++k){
                        int32_t ix32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // direct data
                        int32_t iy32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // direct data
                        SIGNAL_CH1[k]  = (float)ix32;
                        SIGNAL_CH2[k]  = (float)iy32;
                }
                break;
        }
}

int wait_for_data(int max_wait_ms){
        // wait for buffer full
        int status[3];
        int max_try=max_wait_ms; while ( !bram_status(status) && --max_try>0) usleep (1000);

        BRAM_WRITE_ADR.Value ()  = status[0];
        BRAM_SAMPLE_POS.Value () = status[1];
        BRAM_FINISHED.Value ()   = status[2];

        if (verbose > 1) fprintf(stderr, "wait_for_data. tm=%d S[%d %d %d]\n", max_wait_ms - max_try, status[0], status[1], status[2] );
        
        return status[2];
}

void measure_and_read_phase_ampl_buffer_avg (double &ampl, double &phase){
        int k=0;
        
        if (verbose > 1) fprintf(stderr, "initiate_bram_write_measurements. RESET and start SINGLE.\n");
        rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_SINGLE, 4096, 4); // TransferMode=4: configure for PHASE, AMPL
        usleep(5000);

        if (wait_for_data (500)){
                size_t i = 12+8*512; // skip
                int count=0;
                if (verbose > 1) fprintf(stderr, " measure_and_read_phase_ampl_buffer_avg. reading BRAM...\n");
                // read data
                ampl=0.;
                phase=0.;
                for (k=0; k < SIGNAL_SIZE_DEFAULT; ++k){
                        int32_t ix32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // Phase (24)
                        int32_t iy32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; // Ampl (24)
                        phase += (SIGNAL_CH1[k] = (double)ix32/QCORDICATAN/M_PI*180.); // PLL Phase deg
                        ampl  += (SIGNAL_CH2[k] = (double)iy32/QCORDICSQRT*1000.); // // Resonator Amplitude Signal Monitor in mV
                        count++;
                }
                phase /= count;
                ampl  /= count;
        } else {
                //double reading_vector[READING_MAX_VALUES];
                if (verbose > 1) fprintf(stderr, " measure_and_read_phase_ampl_buffer_avg. Failed. Using GPIO fallback snapshot!\n");
                // try reinitialize transport
                rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_SINGLE, 4096, 4); // configure for PHASE, AMPL
                usleep(2000);
                // get GPIO reading
                //rp_PAC_get_single_reading (reading_vector);
                // use GPIO FIR reading thread data
                ampl  = gpio_reading_FIRV_vector[GPIO_READING_AMPL]/GPIO_FIR_LEN * 1000./QCORDICSQRT; //reading_vector[4]; // LMS Amplitude (Volume) in mVolts (from A,B)
                phase = gpio_reading_FIRV_vector[GPIO_READING_PHASE]/GPIO_FIR_LEN * 180./QCORDICATAN/M_PI;   //reading_vector[5]; // LMS Phase (from A,B) in deg
         }

        // clear what is not complete
        for (; k < SIGNAL_SIZE_DEFAULT; ++k){
                SIGNAL_CH1[k] = 0.; //phase;
                SIGNAL_CH2[k] = 0.; //ampl;
        }
}

double get_tune_df(double f){
        if (OPERATION.Value () == 6) // tune simple, linear
                return TUNE_DFREQ.Value ();

        else if (OPERATION.Value () >= 7){ // tune fast, bigger steps away from center
                if (fabs (f) >  TUNE_SPAN.Value ()/2)
                        return 16*TUNE_DFREQ.Value ();
                else if (fabs (f) >  TUNE_SPAN.Value ()/4)
                        return 8*TUNE_DFREQ.Value ();
                else if (fabs (f) >  TUNE_SPAN.Value ()/8)
                        return 4*TUNE_DFREQ.Value ();
                else if (fabs (f) >  TUNE_SPAN.Value ()/16)
                        return 2*TUNE_DFREQ.Value ();
                else if (fabs (f) >  TUNE_SPAN.Value ()/32)
                        return 1.3*TUNE_DFREQ.Value ();
                else
                        return TUNE_DFREQ.Value ();
        }
        
        return TUNE_DFREQ.Value ();
}

void *thread_tuning(void *arg) {
        double epsilon = 0.10; // 0.02 // 2%
        //double reading_vector[READING_MAX_VALUES];
        //int status[3];
        //int ch;
        int dir=1;
        double f=0.;
        double tune_amp_max=0.;
        double tune_phase=0.;
        double tune_fcenter=0.;
        double ampl, phase;
        int    i_prev = TUNE_SIGNAL_SIZE_DEFAULT/2;
        double f_prev = 0.;
        
        static int i0 = TUNE_SIGNAL_SIZE_DEFAULT/2;
        double df = TUNE_SPAN.Value ()/(TUNE_SIGNAL_SIZE_DEFAULT-1);
        double ampl_prev=0.;
        //double phase_prev=0.;

        // zero buffers, reset state at tune start
        // init tune -------------------------------------------------
        for (int i = 0; i < TUNE_SIGNAL_SIZE_DEFAULT; i++){
                SIGNAL_TUNE_PHASE[i] = 0.;
                SIGNAL_TUNE_AMPL[i]  = 0.;
                SIGNAL_FRQ[i] = -TUNE_SPAN.Value ()/2 + df*i;
                
                g_data_signal_ch1pa.erase (g_data_signal_ch1pa.begin());
                g_data_signal_ch1pa.push_back (0.0);
                
                g_data_signal_ch2aa.erase (g_data_signal_ch2aa.begin());
                g_data_signal_ch2aa.push_back (0.0);
                
                g_data_signal_frq.erase (g_data_signal_frq.begin());
                g_data_signal_frq.push_back (0.0);
        }
        f = 0.; // f = -TUNE_SPAN.Value ()/4.;
        dir = 1;
        i0 = TUNE_SIGNAL_SIZE_DEFAULT/2;
        i_prev = TUNE_SIGNAL_SIZE_DEFAULT/2;
        f_prev = 0.;

        // get intial reading
        rp_PAC_adjust_dds (FREQUENCY_MANUAL.Value() + f);
        FREQUENCY_TUNE.Value() = f;
        usleep (25000); // min recover time
        measure_and_read_phase_ampl_buffer_avg (ampl, phase);
        ampl_prev  = ampl;
        //phase_prev = phase;
        
        CENTER_FREQUENCY.Value () = tune_fcenter;
        CENTER_PHASE.Value () = tune_phase;
        CENTER_AMPLITUDE.Value () = tune_amp_max;
        tune_amp_max=0.;

        // ---------------------------------------------------------------

        while (thread_data__tune_control){
                double s = i0 > TUNE_SIGNAL_SIZE_DEFAULT/2 ? 1:-1.;
                double x = (double)(i0-TUNE_SIGNAL_SIZE_DEFAULT/2); x *= x; x /= TUNE_SIGNAL_SIZE_DEFAULT/2; x *= s;
                int k = 0;
              
                if (OPERATION.Value () == 9){
                        // TUNE mode: RS (Random Search)
                        // if stable or last rep, continue to near by point
                        k = (int)(128.*(double)rand () / RAND_MAX) - 64;
                        int i = TUNE_SIGNAL_SIZE_DEFAULT/2 + (int)x + k;
                        if (i < 0) i=0;
                        if (i >= TUNE_SIGNAL_SIZE_DEFAULT) i=TUNE_SIGNAL_SIZE_DEFAULT-1;

                        f = -TUNE_SPAN.Value ()/2 + df*i;

                        if (SIGNAL_TUNE_PHASE[i_prev] != 0.0){
                                SIGNAL_TUNE_PHASE[i_prev] += phase;
                                SIGNAL_TUNE_PHASE[i_prev] *= 0.5;
                        } else
                                SIGNAL_TUNE_PHASE[i_prev] = phase;
                        if (SIGNAL_TUNE_AMPL[i_prev]  != 0.0){
                                SIGNAL_TUNE_AMPL[i_prev] += ampl;
                                SIGNAL_TUNE_AMPL[i_prev] *= 0.5;
                        } else
                                SIGNAL_TUNE_AMPL[i_prev] = ampl;
                        SIGNAL_FRQ[i_prev] = f_prev;
                        i_prev = i;
                        f_prev = f;
                } else {
                        // TUNE mode linear rocking sweep loop

                        g_data_signal_ch1pa.erase (g_data_signal_ch1pa.begin());
                        g_data_signal_ch1pa.push_back (phase);
                
                        g_data_signal_ch2aa.erase (g_data_signal_ch2aa.begin());
                        g_data_signal_ch2aa.push_back (ampl);
                
                        g_data_signal_frq.erase (g_data_signal_frq.begin());
                        g_data_signal_frq.push_back (f);

                        if (f < TUNE_SPAN.Value ()/2 && dir == 1){
                                f += get_tune_df (f);
                        } else if (dir == 1){
                                dir = -1;
                                CENTER_FREQUENCY.Value () = tune_fcenter;
                                CENTER_PHASE.Value () = tune_phase;
                                CENTER_AMPLITUDE.Value () = tune_amp_max;
                                tune_amp_max=0.;
                        }
                        if (f > -TUNE_SPAN.Value ()/2 && dir == -1){
                                f -= get_tune_df (f);
                        } else if (dir == -1){
                                dir = 1;
                                CENTER_FREQUENCY.Value () = tune_fcenter;
                                CENTER_PHASE.Value () = tune_phase;
                                CENTER_AMPLITUDE.Value () = tune_amp_max;
                                tune_amp_max=0.;
                        }
                }
                // next
                if (verbose > 1) fprintf(stderr, "Tuning at: %g \n", FREQUENCY_MANUAL.Value() + f);

                // adjust DDS
                rp_PAC_adjust_dds (FREQUENCY_MANUAL.Value() + f);
                FREQUENCY_TUNE.Value() = f;

                // min recover time after adjusting tune
                if (OPERATION.Value () == 6) // tune
                        usleep (5000);
                else if (OPERATION.Value () == 7) // tune F
                        usleep (8000);
                else if (OPERATION.Value () == 8) // tune FF
                        usleep (16000);
                else // tune RS
                        usleep (32000 + (unsigned long) (100000 * (1.-fabs (f)/TUNE_SPAN.Value ())  ));

                // get a reading
                measure_and_read_phase_ampl_buffer_avg (ampl, phase);

                // check for stable reading
                int max_try = 30;
                while (fabs(ampl-ampl_prev) > epsilon*ampl && --max_try>0){
                        if (verbose > 1) fprintf(stderr, "wait for tune to stabilize, ampl error: %g \n", fabs(ampl-ampl_prev));
                        // try again
                        measure_and_read_phase_ampl_buffer_avg (ampl, phase);
                        ampl_prev  = ampl;
                        //phase_prev = phase;
                }
                ampl_prev  = ampl;
                //phase_prev = phase;

                // update
                if (ampl > tune_amp_max){
                        tune_amp_max = ampl;
                        tune_phase = phase;
                        tune_fcenter = FREQUENCY_MANUAL.Value() + f;
                }

                if (OPERATION.Value () == 9){
                        ; // set directly at index above in RS mode
                } else {
                        for (int i = 0; i < TUNE_SIGNAL_SIZE_DEFAULT; i++){
                                SIGNAL_TUNE_PHASE[i] = g_data_signal_ch1pa[i];
                                SIGNAL_TUNE_AMPL[i]  = g_data_signal_ch2aa[i];
                                SIGNAL_FRQ[i] = g_data_signal_frq[i];
                        }
                }
                i0 = (TUNE_SIGNAL_SIZE_DEFAULT-1)*(double)rand () / RAND_MAX;
        }

        return NULL;
}

void UpdateSignals(void)
{
        //static int clear_tune_data=1;
        static pthread_t tune_thread;
        double reading_vector[READING_MAX_VALUES];
        int ch;
        int status[3];
        static int last_op=0;
        
        if (verbose > 3) fprintf(stderr, "UpdateSignals()\n");

        rp_PAC_get_single_reading_FIR (reading_vector);
        bram_status(status);
        BRAM_WRITE_ADR.Value ()  = status[0];
        BRAM_SAMPLE_POS.Value () = status[1];
        BRAM_FINISHED.Value ()   = status[2];

        switch ( OPERATION.Value () ){
        case 2:
                if (verbose == 1) fprintf(stderr, "UpdateSignals SCOPE OPERATION=2\n");
                // Scope mode, wait for trace to completed, then auto retrigger start
                if (last_op != OPERATION.Value ()){ // initiate
                        rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_SINGLE, 4096, TRANSPORT_MODE.Value ());
                        usleep(10000);
                }
                if (status[2]){ // finished?
                        // read
                        read_bram (SIGNAL_SIZE_DEFAULT, TRANSPORT_DECIMATION.Value (),  TRANSPORT_MODE.Value (), GAIN1.Value (), GAIN2.Value ());
                        // trigger next
                        rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_SINGLE, 4096, TRANSPORT_MODE.Value ());
                } else { // try re-initiate
                        if (SET_SINGLESHOT_TRANSPORT_TRIGGER.Value ())
                                rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_SINGLE, 4096, TRANSPORT_MODE.Value ());
                }
                break;
        case 4:
                // Scope in single shot mode -- just read what we got and update signals
        case 5:
                // Streaming / Loop Mode -- just read what ever we got and update signals
                if (verbose == 1){
                        unsigned int x = read_gpio_reg_int32 (6,1); // GPIO X12 : Transport Dbg
                        fprintf(stderr, "PAC TRANSPORT: BRAM WritePos=%04X Sample#=%04x Finished=%d\n", x&0x7fff, x>>16, x&0x8000 ? 1:0);
                }
                read_bram (SIGNAL_SIZE_DEFAULT, TRANSPORT_DECIMATION.Value (),  TRANSPORT_MODE.Value (), GAIN1.Value (), GAIN2.Value ());
                break;
        case 6:
        case 7:
        case 8:
        case 9:
        // all TUNE MODEs
                if (!thread_data__tune_control){
                        thread_data__tune_control=1;
                        if ( pthread_create( &tune_thread, NULL, thread_tuning, NULL) ) {
                                fprintf(stderr, "Error creating tune thread.\n");
                        } 
                        //clear_tune_data = 0; // after completed / mode switch zero tune data buffers
                }
                break;
        }        

        if (OPERATION.Value () < 6){
                if (thread_data__tune_control){
                    thread_data__tune_control=0;
                    if ( pthread_join ( tune_thread, NULL ) ) {
                            fprintf(stderr, "Error joining tune thread.\n");
                    }
                }
                //clear_tune_data = 1;
                if (verbose > 3) fprintf(stderr, "UpdateSignals get GPIO reading:\n");

                // Slow GPIO MONITOR in strip plotter mode
                // Push it to vector
                ch = TRANSPORT_CH3.Value ();
                if (verbose > 3) fprintf(stderr, "UpdateSignals: CH3=%d \n", ch);
                g_data_signal_ch3.erase (g_data_signal_ch3.begin());
                g_data_signal_ch3.push_back (reading_vector[ch >=0 && ch < READING_MAX_VALUES ? ch : 0] * GAIN3.Value ());

                ch = TRANSPORT_CH4.Value ();
                if (verbose > 3) fprintf(stderr, "UpdateSignals: CH4=%d \n", ch);
                g_data_signal_ch4.erase (g_data_signal_ch4.begin());
                g_data_signal_ch4.push_back (reading_vector[ch >=0 && ch < READING_MAX_VALUES ? ch : 1] * GAIN4.Value ());

                ch = TRANSPORT_CH5.Value ();
                if (verbose > 3) fprintf(stderr, "UpdateSignals: CH5=%d \n", ch);
                g_data_signal_ch5.erase (g_data_signal_ch5.begin());
                g_data_signal_ch5.push_back (reading_vector[ch >=0 && ch < READING_MAX_VALUES ? ch : 1] * GAIN5.Value ());

                // Copy data to signals
                if (verbose > 3) fprintf(stderr, "UpdateSignals copy signals\n");
                for (int i = 0; i < SIGNAL_SIZE_DEFAULT; i++){
                        SIGNAL_CH3[i] = g_data_signal_ch3[i];
                        SIGNAL_CH4[i] = g_data_signal_ch4[i];
                        SIGNAL_CH5[i] = g_data_signal_ch5[i];
                }
        }
        
        if (verbose > 3) fprintf(stderr, "UpdateSignal complete.\n");

        // direct GPIO snapshot (noisy)
        //rp_PAC_get_single_reading (reading_vector);
        //VOLUME_MONITOR.Value ()   = reading_vector[4]; // Resonator Amplitude Signal Monitor in mV
        //PHASE_MONITOR.Value ()    = reading_vector[5]; // PLL Phase in deg
        //EXEC_MONITOR.Value ()     = reading_vector[8]; // Exec Amplitude Monitor in mV
        //DDS_FREQ_MONITOR.Value () = reading_vector[9]; // DDS Freq Monitor in Hz
        //DFREQ_MONITOR.Value ()    = reading_vector[12]; // delta Frequency Monitor in Hz (after decimation)
        //CONTROL_DFREQ_MONITOR.Value () = reading_vector[13]; // Control value of delta Frequency Controller

        // GPIO via FIR thread
        VOLUME_MONITOR.Value ()   = gpio_reading_FIRV_vector[GPIO_READING_AMPL]/GPIO_FIR_LEN * 1000./QCORDICSQRT; //reading_vector[4]; // LMS Amplitude (Volume) in mVolts (from A,B)
        PHASE_MONITOR.Value ()    = gpio_reading_FIRV_vector[GPIO_READING_PHASE]/GPIO_FIR_LEN * 180./QCORDICATAN/M_PI;   //reading_vector[5]; // LMS Phase (from A,B) in deg
        EXEC_MONITOR.Value ()     = gpio_reading_FIRV_vector[GPIO_READING_EXEC]/GPIO_FIR_LEN  * 1000/QEXEC;
        DDS_FREQ_MONITOR.Value () = gpio_reading_FIRV_vector[GPIO_READING_DDS_FREQ]/GPIO_FIR_LEN;
        DFREQ_MONITOR.Value ()    = gpio_reading_FIRV_vector[GPIO_READING_DFREQ]/GPIO_FIR_LEN;
        CONTROL_DFREQ_MONITOR.Value () = gpio_reading_FIRV_vector[GPIO_READING_CONTROL_DFREQ]/GPIO_FIR_LEN * 10000/Q31;
        
        rp_PAC_auto_dc_offset_correct ();

        last_op = OPERATION.Value (); 
}


void UpdateParams(void){
        if (verbose > 3) fprintf(stderr, "UpdateParams()\n");
	CDataManager::GetInstance()->SetParamInterval (parameter_updatePeriod.Value());
	CDataManager::GetInstance()->SetSignalInterval (signal_updatePeriod.Value());

        DC_OFFSET.Value() = (float)(1000.*signal_dc_measured); // mV
   	
        FILE *fp = fopen("/proc/stat","r");
        if(fp) {
                long double a[4];
                int ret=fscanf(fp,"%*s %Lf %Lf %Lf %Lf",&a[0],&a[1],&a[2],&a[3]);
                fclose(fp);

                if (ret == 4){
                        long double divider = ((a[0]+a[1]+a[2]+a[3]) - (cpu_values[0]+cpu_values[1]+cpu_values[2]+cpu_values[3]));
                        long double loadavg = 100;
                        if(divider > 0.01) {
                                loadavg = ((a[0]+a[1]+a[2]) - (cpu_values[0]+cpu_values[1]+cpu_values[2])) / divider;
                        }
                        cpuLoad.Value() = (float)(loadavg * 100);
                        cpu_values[0]=a[0];cpu_values[1]=a[1];cpu_values[2]=a[2];cpu_values[3]=a[3];
                } else {
                       cpuLoad.Value() = (float)(-1); // ERROR EVALUATING LOAD
                }
                
        }
    
        struct sysinfo memInfo;
        sysinfo (&memInfo);
        memoryFree.Value() = (float)memInfo.freeram;

        counter.Value()=counter.Value()+(double)parameter_updatePeriod.Value()/1000.0;

        if (verbose > 3) fprintf(stderr, "UpdateParams: text update\n");
        pacpll_text.Value() = "Hello this is the RP PACPLL Server.    ";

        if (counter.Value()>30000) {
                counter.Value()=0;
        }
        if (verbose > 3) fprintf(stderr, "UpdateParams complete.\n");
}


void OnNewParams(void)
{
        //int x=0;
        //static int ppv=0;
        //static int spv=0;
        static int operation=0;
        //double reading_vector[READING_MAX_VALUES];

#if 0
        if (ppv == 0) { ppv=parameter_updatePeriod.Value(); parameter_updatePeriod.Update (); }
        if (spv == 0) { spv=signal_updatePeriod.Value(); signal_updatePeriod.Update (); }
        
        if (verbose > 3) fprintf(stderr, "OnNewParams()\n");

        if (ppv != parameter_updatePeriod.Value ()){
                CDataManager::GetInstance()->SetParamInterval (parameter_updatePeriod.Value());
                ppv = parameter_updatePeriod.Value ();
        }
        if (spv != signal_updatePeriod.Value ()){
                CDataManager::GetInstance()->SetSignalInterval (signal_updatePeriod.Value());
                spv = signal_updatePeriod.Value ();
        }
#endif
        
        PACVERBOSE.Update ();
        OPERATION.Update ();

        GAIN1.Update ();
        GAIN2.Update ();
        GAIN3.Update ();
        GAIN4.Update ();
        GAIN5.Update ();
        SHR_DEC_DATA.Update ();
        TUNE_SPAN.Update ();
        TUNE_DFREQ.Update ();

        FREQUENCY_MANUAL.Update ();
        FREQUENCY_CENTER.Update ();
        AUX_SCALE.Update ();
        VOLUME_MANUAL.Update ();
        PACTAU.Update ();
        PACATAU.Update ();
        PAC_DCTAU.Update ();

        QC_GAIN.Update ();
        QC_PHASE.Update ();
        QCONTROL.Update ();
        
        PHASE_CONTROLLER.Update ();
        AMPLITUDE_CONTROLLER.Update ();
        SET_SINGLESHOT_TRANSPORT_TRIGGER.Update ();
        SET_SINGLESHOT_TRIGGER_POST_TIME.Update ();
        PHASE_UNWRAPPING_ALWAYS.Update ();
        LCK_AMPLITUDE.Update ();
        LCK_PHASE.Update ();
        DFREQ_CONTROLLER.Update ();
        
        AMPLITUDE_FB_SETPOINT.Update ();
        AMPLITUDE_FB_CP.Update ();
        AMPLITUDE_FB_CI.Update ();
        EXEC_FB_UPPER.Update ();
        EXEC_FB_LOWER.Update ();

        PHASE_FB_SETPOINT.Update ();
        PHASE_FB_CP.Update ();
        PHASE_FB_CI.Update ();
        FREQ_FB_UPPER.Update ();
        FREQ_FB_LOWER.Update ();
        PHASE_HOLD_AM_NOISE_LIMIT.Update ();
        
        if (verbose > 3) fprintf(stderr, "OnNewParams: verbose=%d\n", PACVERBOSE.Value ());
        verbose = PACVERBOSE.Value ();

        BRAM_SCOPE_TRIGGER_MODE.Update ();
        BRAM_SCOPE_TRIGGER_POS.Update ();
        BRAM_SCOPE_TRIGGER_LEVEL.Update ();
        BRAM_SCOPE_SHIFT_POINTS.Update ();
        
        TRANSPORT_DECIMATION.Update ();
        TRANSPORT_MODE.Update ();
        TRANSPORT_CH3.Update ();
        TRANSPORT_CH4.Update ();
        TRANSPORT_CH5.Update ();

        /*
        TRANSPORT_TAU_DFREQ.Update ();
        TRANSPORT_TAU_PHASE.Update ();
        TRANSPORT_TAU_EXEC.Update ();
        TRANSPORT_TAU_AMPL.Update ();
        */
        
        DFREQ_FB_SETPOINT.Update ();
        DFREQ_FB_CP.Update ();
        DFREQ_FB_CI.Update ();
        DFREQ_FB_UPPER.Update ();
        DFREQ_FB_LOWER.Update ();

        PULSE_FORM_BIAS0.Update ();
        PULSE_FORM_BIAS1.Update ();
        PULSE_FORM_PHASE0.Update ();
        PULSE_FORM_PHASE1.Update ();
        PULSE_FORM_WIDTH0.Update ();
        PULSE_FORM_WIDTH1.Update ();
        PULSE_FORM_HEIGHT0.Update ();
        PULSE_FORM_HEIGHT1.Update ();
        PULSE_FORM_WIDTH0IF.Update ();
        PULSE_FORM_WIDTH1IF.Update ();
        PULSE_FORM_HEIGHT0IF.Update ();
        PULSE_FORM_HEIGHT1IF.Update ();
        PULSE_FORM_SHAPEXW.Update ();
        PULSE_FORM_SHAPEXWIF.Update ();
        PULSE_FORM_SHAPEX.Update ();
        PULSE_FORM_SHAPEXIF.Update ();

        
        if ( OPERATION.Value () > 0 && OPERATION.Value () != operation ){
                operation = OPERATION.Value ();
                switch (operation){
                case 1:
                        rp_PAC_auto_dc_offset_adjust ();
                        if (verbose > 0) fprintf(stderr, "OnNewParams: OP=1 Auto Offset Run, DC=%f mV\n", 1000.*signal_dc_measured);
                        pacpll_text.Value() = "Auto Offset completed.                 ";
                        break;
                case 2: // Scope
                        break;
                case 3: // Reset
                        rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_RESET, 2048, TRANSPORT_MODE.Value ());
                        break;
                case 4: // Single Shot
                        rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_SINGLE, 4096, TRANSPORT_MODE.Value ());
                        break;
                case 5: // Operation Contineous FIFO Transport Mode (scanning)
                        rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_LOOP, 4096, TRANSPORT_MODE.Value ());
                        break;
                case 6: // Tune
                        break;
                case 7: // Tune fast
                        break;
                case 8: // Tune very fast
                        break;
                case 9: // Tune RS
                        break;
                }
        }
        
        if (verbose > 3) fprintf(stderr, "OnNewParams: set_PAC_config\n");
        set_PAC_config();
        if (verbose > 3) fprintf(stderr, "OnNewParams done.\n");
}


void OnNewSignals(void){
        if (verbose > 3) fprintf(stderr, "OnNewSignals()\n");
	// do something
	CDataManager::GetInstance()->UpdateAllSignals();
        if (verbose > 3) fprintf(stderr, "OnNewSignals done.\n");
}


void PostUpdateSignals(void){
        if (verbose > 3) fprintf(stderr, "PostUpdateSignals()\n");
        if (verbose > 3) fprintf(stderr, "PostUpdateSignals done.\n");
}

// END.
