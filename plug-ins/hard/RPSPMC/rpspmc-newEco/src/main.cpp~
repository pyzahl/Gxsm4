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

// VIVADO 2023.1.1 NEEDS TCL HACK:>> set_param placedata.goqFix yes

int verbose = 2;
int stream_debug_flags = 0;

// global thread control parameter
int thread_data__tune_control=0;


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

#define REDPACPLL_DATE    0x20250101
#define REDPACPLL_VERSION 0x00160000
#define RPSPMC_VERSION    0x00010015
#define RPSPMC_VNAME      "Evaluation Regime SG20250101"
#define RPSPMC_SRCS_INFO  "Bit0..3: XYZU | Bit4-7: IN1,2,3,4 | Bit8-11: dFREQ,EXEC,PHASE,AMPL | Bit 12..15: LckA,B, TIME64"

#define PARAMETER_UPDATE_INTERVAL 200 // ms
#define SIGNAL_UPDATE_INTERVAL    200 // ms

#include "main.h"
#include "fpga_cfg.h"
#include "pacpll.h"
#include "spmc.h"
#include "spmc_dma.h"
#include "spmc_stream_server.h"

spmc_stream_server spmc_stream_server_instance;
spmc_dma_support *spm_dma_instance = NULL;


// Some thing is off while Linking, application does NOT (terminated with unkown error a tload time) work with compliled separate and linked to lib
// In Makefile: CXXSOURCES=main.cpp fpga_cfg.cpp pacpll.cpp spmc.cpp
// Thus this primitive alternative assembly here
#include "fpga_cfg.cpp"

#include "spmc_module_config_utils.h"
#include "spmc_module_config_utils.cpp"

#include "pacpll.cpp"
#include "spmc_stream_server.cpp"



#include "spmc.cpp"

// INSTALL:
// =============================================================
// copy complete infrastructure:
// scp -r rpspmc root@rp-f05603.local:/opt/redpitaya/www/apps/
// build:
// make clean; make INSTALL_DIR=/opt/redpitaya

// update fpga.bit only:
// Gxsm4/plug-ins/hard/RPSPMC/rpspmc$ 
// scp ../project_RP-SPMC-PACPLL-Gxsm4/project_RP-SPMC-PACPLL-Gxsm4.runs/impl_1/system_wrapper.bit  root@rp-f09296.local:/opt/redpitaya/www/apps/rpspmc/fpga.bit

// copy sources:
// pzahl@phenom:~/SVN/Gxsm4/plug-ins/hard/RPSPMC/rpspmc$ scp src/*.h src/*.cpp root@rp-f09296.local:/opt/redpitaya/www/apps/rpspmc/src

// On rev RP OS 2.x >>> need to install: root@rp-f09296:/opt/redpitaya/www/apps/rpspmc# apt-get install libwebsocketpp-dev

// CHROME BROWSER NOTES: USER SCHIT-F5 to force reload of all data, else caches are fooling....

/*
 * RedPitaya A9 JSON Interface PARAMETERS and SIGNALS
 * ------------------------------------------------------------
 */


// FPGA MEMORY MAPPING for "RP-SPMC-RedPACPLL" 202308V00

/*
Network 1					
/PS/axi_bram_reader_scope/s00_axi	S_AXI	reg0	0x4000_0000	2M	0x401F_FFFF
/PS/axi_dma_spmc/S_AXI_LITE	S_AXI_LITE	Reg	0x4040_0000	64K	0x4040_FFFF
/PS/Configurations/axi_cfg_register_0/s_axi	S_AXI	reg0	0x4200_0000	4K	0x4200_0FFF
/PS/Configurations/axi_cfg_register_1/s_axi	S_AXI	reg0	0x4300_0000	4K	0x4300_0FFF
/PS/GPIOs/axi_gpio_0/S_AXI	S_AXI	Reg	0x4200_2000	4K	0x4200_2FFF
/PS/GPIOs/axi_gpio_1/S_AXI	S_AXI	Reg	0x4200_3000	4K	0x4200_3FFF
/PS/GPIOs/axi_gpio_2/S_AXI	S_AXI	Reg	0x4200_4000	4K	0x4200_4FFF
/PS/GPIOs/axi_gpio_3/S_AXI	S_AXI	Reg	0x4200_5000	4K	0x4200_5FFF
/PS/GPIOs/axi_gpio_4/S_AXI	S_AXI	Reg	0x4200_6000	4K	0x4200_6FFF
/PS/GPIOs/axi_gpio_5/S_AXI	S_AXI	Reg	0x4200_7000	4K	0x4200_7FFF
/PS/GPIOs/axi_gpio_6/S_AXI	S_AXI	Reg	0x4200_8000	4K	0x4200_8FFF
/PS/GPIOs/axi_gpio_7/S_AXI	S_AXI	Reg	0x4200_9000	4K	0x4200_9FFF
/PS/GPIOs/axi_gpio_8/S_AXI	S_AXI	Reg	0x4200_A000	4K	0x4200_AFFF
/PS/GPIOs/axi_gpio_9/S_AXI	S_AXI	Reg	0x4200_B000	4K	0x4200_BFFF
/PS/GPIOs/axi_gpio_10/S_AXI	S_AXI	Reg	0x4200_C000	4K	0x4200_CFFF
                   11                                  D000    
*/

// FPGA page size is 0x1000

#define FPGA_BRAM_PACPLL_BASE    0x40000000 // 2M (0x4000_0000 ... 0x401F_FFFF)
#define FPGA_BRAM_PACPLL_PAGES   2048       // 2M


#define FPGA_CFG_REG      0x43000000 // 4k (0x4200_0000 ... _0FFF)
#define FPGA_CFG_PAGES    1          // 1 pages assigned, 4k

#define FPGA_GPIO_BASE    0x42002000 // 11 pages @4k each (=0x1000) 0x4200_2000 .. 0x4200_BFFF
#define FPGA_GPIO_PAGES   12         // 11 pages @4k each for each GPIO block containg 2x32bit channels @read address +0 and +8 (0x4200_2000-_2FFF, _3000-_3FFF, .. _B000-_BFFF)


//Signal size
#define SIGNAL_SIZE_DEFAULT       1024
#define TUNE_SIGNAL_SIZE_DEFAULT  1024

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

CIntParameter TRANSPORT_CH3("TRANSPORT_CH3", CBaseParameter::RW, 0, 0, 0, 19);
CIntParameter TRANSPORT_CH4("TRANSPORT_CH4", CBaseParameter::RW, 1, 0, 0, 19);
CIntParameter TRANSPORT_CH5("TRANSPORT_CH5", CBaseParameter::RW, 1, 0, 0, 19);
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
CIntParameter BRAM_DEC_COUNT("BRAM_DEC_COUNT", CBaseParameter::RW, 0, 0, -2147483648,2147483647);

CFloatParameter DC_OFFSET("DC_OFFSET", CBaseParameter::RW, 0, 0, -1000.0, 1000.0); // mV


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

// *** PAC_PLL::GPIO MONITORS ***                                                                    readings are via void *thread_gpio_reading_FIR(g), read_gpio_reg_int32 (n,m)
// *** DBG ***                                                                                                //        gpio_reading_FIRV_vector[GPIO_READING_LMS_A]        (1,0); // GPIO X1 : XS-GVP
// *** DBG ***                                                                                                //        gpio_reading_FIRV_vector[GPIO_READING_LMS_A]        (1,1); // GPIO X2 : YS-GVP
// *** DBG ***                                                                                                //        -----------------------                             (2,0); // GPIO X3 : DBG M
CDoubleParameter VOLUME_MONITOR("VOLUME_MONITOR", CBaseParameter::RW, 0, 0, -1000.0, 1000.0);                 // mV  ** gpio_reading_FIRV_vector[GPIO_READING_AMPL]         (2,1); // GPIO X4 : CORDIC SQRT (AM2=A^2+B^2)
// via  DC_OFFSET rp_PAC_auto_dc_offset_correct ()                                                                                                                          (3,0); // GPIO X5 : DC_OFFSET (M-DC)
// *** DBG ***                                                                                                //        -----------------------                             (3,1); // GPIO X6 : --- SPMC STATUS [FB, GVP, AD5791, --]
CDoubleParameter EXEC_MONITOR("EXEC_MONITOR", CBaseParameter::RW, 0, 0, -1000.0, 1000.0);                     //  mV ** gpio_reading_FIRV_vector[GPIO_READING_EXEC]         (4,0); // GPIO X7 : Exec Ampl Control Signal (signed)
CDoubleParameter DDS_FREQ_MONITOR("DDS_FREQ_MONITOR", CBaseParameter::RW, 0, 0, 0.0, 25e6);                   //  Hz ** gpio_reading_FIRV_vector[GPIO_READING_DDS_FREQ]     (4,1); // GPIO X8 : DDS Phase Inc (Freq.) upper 32 bits of 44 (unsigned)
                                                                                                              //                                                            (5,0); // GPIO X9 : DDS Phase Inc (Freq.) lower 32 bits of 44 (unsigned)
CDoubleParameter PHASE_MONITOR("PHASE_MONITOR", CBaseParameter::RW, 0, 0, -180.0, 180.0);                     // deg ** gpio_reading_FIRV_vector[GPIO_READING_PHASE]        (5,1); // GPIO X10: CORDIC ATAN(X/Y)
CDoubleParameter DFREQ_MONITOR("DFREQ_MONITOR", CBaseParameter::RW, 0, 0, -1000.0, 1000.0);                   // Hz  ** gpio_reading_FIRV_vector[GPIO_READING_DDS_FREQ]     (6,0); // GPIO X11: dFreq
// *** DBG ***                                                                                                //        -----------------------                             (6,1); // GPIO X12: --- Transport Status: writeposition BRAM
CDoubleParameter CONTROL_DFREQ_MONITOR("CONTROL_DFREQ_MONITOR", CBaseParameter::RW, 0, 0, -10000.0, 10000.0); // mV  **  gpio_re..._FIRV_vector[GPIO_READING_CONTROL_DFREQ] (7,0); // GPIO X13: control dFreq value
// *** DBG ***                                                                                                //        -----------------------                             (7,1); // GPIO X14: --- SIGNAL PASS [IN2] (Current, FB SRC)
// *** DBG ***                                                                                                //        -----------------------                             (8,0); // GPIO X15: --- UMON Bias
// *** DBG ***                                                                                                //        -----------------------                             (8,1); // GPIO X16: --- XMON
// *** DBG ***                                                                                                //        -----------------------                             (9,0); // GPIO X17: --- YMON
// *** DBG ***                                                                                                //        -----------------------                             (9,1); // GPIO X18: --- ZMON   (Z total at DAC3)
// *** DBG ***                                                                                                //        -----------------------                            (10,0); // GPIO X19: --- CONFIG READ-BACK REG A --- ***//0-MON (Z-Offset) *** ZS-MON (Z-GVP)
// *** DBG ***                                                                                                //        -----------------------                            (10,1); // GPIO X20: --- CONFIG READ-BACK REG B --- ***//Z0-MON (Z-Offset)
// *** DBG ***                                                                                                //        -----------------------                            (11,0); // GPIO X21: --- SPMC BRAM LAST WRITE ADDRESS (0..16383)
// *** DBG ***                                                                                                //        -----------------------                            (11,1); // GPIO X22: --- Z-Sum to DAC MON
// *** DBG ***                                                                                                //        -----------------------                            (12,0); // GPIO X23: --- AMON
// *** DBG ***                                                                                                //        -----------------------                            (12,1); // GPIO X24: --- BMON



/* SPMC Parameters */

CDoubleParameter  SPMC_BIAS("SPMC_BIAS", CBaseParameter::RW, 0.0, 0, -5.0, 5.0); // Volts

CIntParameter     SPMC_Z_SERVO_MODE("SPMC_Z_SERVO_MODE", CBaseParameter::RW, 0, 0, 0, 0xffff);
CDoubleParameter  SPMC_Z_SERVO_SETPOINT("SPMC_Z_SERVO_SETPOINT", CBaseParameter::RW, 0.0, 0, -5.0, 5.0); // Volts
CDoubleParameter  SPMC_Z_SERVO_CP("SPMC_Z_SERVO_CP", CBaseParameter::RW, 0.0, 0, -1000.0, 1000.0); // XXX
CDoubleParameter  SPMC_Z_SERVO_CI("SPMC_Z_SERVO_CI", CBaseParameter::RW, 0.0, 0, -1000.0, 1000.0); // XXX
CDoubleParameter  SPMC_Z_SERVO_UPPER("SPMC_Z_SERVO_UPPER", CBaseParameter::RW, 0.0, 0, -5.0, 5.0); // Volts
CDoubleParameter  SPMC_Z_SERVO_LOWER("SPMC_Z_SERVO_LOWER", CBaseParameter::RW, 0.0, 0, -5.0, 5.0); // Volts
CDoubleParameter  SPMC_Z_SERVO_SETPOINT_CZ("SPMC_Z_SERVO_SETPOINT_CZ", CBaseParameter::RW, 0.0, 0, -5.0, 5.0); // Volts
CDoubleParameter  SPMC_Z_SERVO_IN_OFFSETCOMP("SPMC_Z_SERVO_IN_OFFSETCOMP", CBaseParameter::RW, 0.0, 0, -5.0, 5.0); // Volts

#define SPMC_GVP_CONTROL_RESET     0
#define SPMC_GVP_CONTROL_EXECUTE   1   
#define SPMC_GVP_CONTROL_PAUSE     2
#define SPMC_GVP_CONTROL_RESUME    3
#define SPMC_GVP_CONTROL_RESET_UAB 4
#define SPMC_GVP_CONTROL_GVP_EXECUTE_NO_DMA 5   
#define SPMC_GVP_CONTROL_GVP_RESET_ONLY     6   

CIntParameter    SPMC_GVP_STREAM_MUX("SPMC_GVP_STREAM_MUX",   CBaseParameter::RW, 0, 0, -2147483648,2147483647); //
CIntParameter    SPMC_GVP_STREAM_MUXH("SPMC_GVP_STREAM_MUXH", CBaseParameter::RW, 0, 0, -2147483648,2147483647); //

CIntParameter     SPMC_GVP_CONTROL_MODE("SPMC_GVP_CONTROL", CBaseParameter::RW, 0, 0, 0, 0xffff);
CBooleanParameter SPMC_GVP_LIVE_VECTOR_UPDATE("SPMC_GVP_LIVE_VECTOR_UPDATE", CBaseParameter::RW, true, 0);
CIntParameter     SPMC_GVP_RESET_OPTIONS("SPMC_GVP_RESET_OPTIONS",   CBaseParameter::RW, 0, 0, 0, 0xffff);
CIntParameter     SPMC_GVP_STATUS("SPMC_GVP_STATUS",   CBaseParameter::RW, 0, 0, 0, 0xffff);
#define GVP_VECTOR_SIZE  16 // 10 components used (1st is index, then: N, nii, Options, Nrep, Next, dx, dy, dz, du, fill w zero to 16)
CFloatSignal SPMC_GVP_VECTOR("SPMC_GVP_VECTOR", GVP_VECTOR_SIZE, 0.0f); // vector components in mV, else "converted to int"

CIntParameter     SPMC_GVP_VECTOR_PC("SPMC_GVP_VECTOR_PC", CBaseParameter::RW, 0, 0, -2147483648,2147483647); // Vector[PC] to set
CIntParameter     SPMC_GVP_VECTOR__N("SPMC_GVP_VECTOR__N", CBaseParameter::RW, 0, 0, -2147483648,2147483647); // # points
CIntParameter     SPMC_GVP_VECTOR__O("SPMC_GVP_VECTOR__O", CBaseParameter::RW, 0, 0, -2147483648,2147483647); // options [Z-Servo Hold, ... , SRCS bits]
CIntParameter     SPMC_GVP_VECTORSRC("SPMC_GVP_VECTORSRC", CBaseParameter::RW, 0, 0, -2147483648,2147483647); // SRCS bits (new, preparing to separate from opt)
CIntParameter     SPMC_GVP_VECTORNRP("SPMC_GVP_VECTORNRP", CBaseParameter::RW, 0, 0, -2147483648,2147483647); // # repetitions
CIntParameter     SPMC_GVP_VECTORNXT("SPMC_GVP_VECTORNXT", CBaseParameter::RW, 0, 0, -2147483648,2147483647); // loop jump rel to PC to next vector
CDoubleParameter  SPMC_GVP_VECTOR_DX("SPMC_GVP_VECTOR_DX", CBaseParameter::RW, 0, 0, -10.0, 10.0); // DX in Volts total length of vector component
CDoubleParameter  SPMC_GVP_VECTOR_DY("SPMC_GVP_VECTOR_DY", CBaseParameter::RW, 0, 0, -10.0, 10.0); // DY in Volts total length of vector component
CDoubleParameter  SPMC_GVP_VECTOR_DZ("SPMC_GVP_VECTOR_DZ", CBaseParameter::RW, 0, 0, -10.0, 10.0); // DZ in Volts total length of vector component
CDoubleParameter  SPMC_GVP_VECTOR_DU("SPMC_GVP_VECTOR_DU", CBaseParameter::RW, 0, 0, -10.0, 10.0); // DU (=Bias) adjust rel to Bias Ref in Volts total length of vector component
CDoubleParameter  SPMC_GVP_VECTOR_AA("SPMC_GVP_VECTOR_AA", CBaseParameter::RW, 0, 0, -10.0, 10.0); // AA (Aux Channel ADC #5) -- reserved
CDoubleParameter  SPMC_GVP_VECTOR_BB("SPMC_GVP_VECTOR_BB", CBaseParameter::RW, 0, 0, -10.0, 10.0); // BB (Aux Channel ADC #6) -- reserved
CDoubleParameter  SPMC_GVP_VECTORSLW("SPMC_GVP_VECTORSLW", CBaseParameter::RW, 0, 0,   0.0, 1e6);  // slew rate in #points / sec -- max: 1 MSPS

CDoubleParameter  SPMC_ALPHA("SPMC_ALPHA", CBaseParameter::RW, 0.0, 0, -360, +360); // deg
CDoubleParameter  SPMC_SLOPE_DZX("SPMC_SLOPE_DZX", CBaseParameter::RW, 0.0, 0, -1.0, +1.0); // slope in Volts Z / Volt X
CDoubleParameter  SPMC_SLOPE_DZY("SPMC_SLOPE_DZY", CBaseParameter::RW, 0.0, 0, -1.0, +1.0); // slope in Volts Z / Volt X
CDoubleParameter  SPMC_SLOPE_SLEW("SPMC_SLOPE_SLEW", CBaseParameter::RW, 0.0, 0, 0.0, +1e6); // slope in 1 / sec

CDoubleParameter  SPMC_SET_SCANPOS_X("SPMC_SET_SCANPOS_X", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_SET_SCANPOS_Y("SPMC_SET_SCANPOS_Y", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_SET_SCANPOS_SLEW("SPMC_SET_SCANPOS_SLEW", CBaseParameter::RW, 0.0, 0, 0.0, 1e6); // Volts/s
CIntParameter     SPMC_SET_SCANPOS_OPTS("SPMC_SET_SCANPOS_OPTS", CBaseParameter::RW, 0, 0,  -2147483648,2147483647); // options [Z-Servo Hold, ..., SRCS bits]

CDoubleParameter  SPMC_SET_OFFSET_X("SPMC_SET_OFFSET_X", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_SET_OFFSET_Y("SPMC_SET_OFFSET_Y", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_SET_OFFSET_Z("SPMC_SET_OFFSET_Z", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts

CDoubleParameter  SPMC_SET_OFFSET_XY_SLEW("SPMC_SET_OFFSET_XY_SLEW", CBaseParameter::RW, 0.0, 0, 0.0, 1000.0); // V/s slew rate
CDoubleParameter  SPMC_SET_OFFSET_Z_SLEW("SPMC_SET_OFFSET_Z_SLEW", CBaseParameter::RW, 0.0, 0, 0.0, 1000.0); // V/s slew rate

// MODULATION, LOCK-IN

CDoubleParameter  SPMC_SC_LCK_FREQUENCY("SPMC_SC_LCK_FREQUENCY", CBaseParameter::RW, 0.0, 0, 0.0, 30e6); // Hz
CDoubleParameter  SPMC_SC_LCK_VOLUME(   "SPMC_SC_LCK_VOLUME", CBaseParameter::RW, 0.0, 0, 0.0, 5000.0); //  mV
CIntParameter     SPMC_SC_LCK_TARGET(   "SPMC_SC_LCK_TARGET", CBaseParameter::RW, 0, 0, 0, 16); // # 0=NONE, 1:X, 2:Y, 3:Z, 4:U
CDoubleParameter  SPMC_SC_LCK_GAIN(     "SPMC_SC_LCK_GAIN", CBaseParameter::RW, 0.0, 0, -1e10, 1e10); // Q

CDoubleParameter  SPMC_SC_LCK_F0BQ_TAU("SPMC_SC_LCK_F0BQ_TAU", CBaseParameter::RW, 0.0, 0, 0.0, 1e10); // ms
CDoubleParameter  SPMC_SC_LCK_F0BQ_Q(  "SPMC_SC_LCK_F0BQ_Q", CBaseParameter::RW, 0.0, 0, -1e10, 1e10); // BQ Q
CDoubleParameter  SPMC_SC_LCK_F0BQ_IIR("SPMC_SC_LCK_F0BQ_IIR", CBaseParameter::RW, 0.0, 0, 0.0, 1e10); // ms 0: pass mode


// *** RP SPMC::GPIO MONITORS ***
CDoubleParameter  SPMC_BIAS_MONITOR("SPMC_BIAS_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_BIAS_REG_MONITOR("SPMC_BIAS_REG_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_BIAS_SET_MONITOR("SPMC_BIAS_SET_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_GVPU_MONITOR("SPMC_GVPU_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_GVPA_MONITOR("SPMC_GVPA_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_GVPB_MONITOR("SPMC_GVPB_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CIntParameter     SPMC_MUX_MONITOR("SPMC_MUX_MONITOR", CBaseParameter::RW, 0, 0, -2147483648,2147483647); // MUX code
CDoubleParameter  SPMC_SIGNAL_MONITOR("SPMC_SIGNAL_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_X_MONITOR("SPMC_X_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_Y_MONITOR("SPMC_Y_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_Z_MONITOR("SPMC_Z_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_X0_MONITOR("SPMC_X0_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_Y0_MONITOR("SPMC_Y0_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_Z0_MONITOR("SPMC_Z0_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_XS_MONITOR("SPMC_XS_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_YS_MONITOR("SPMC_YS_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts
CDoubleParameter  SPMC_ZS_MONITOR("SPMC_ZS_MONITOR", CBaseParameter::RW, 0.0, 0, -5.0, +5.0); // Volts




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


/*
 * RedPitaya A9 FPGA LinksR
 * ------------------------------------------------------------
 */

const char *FPGA_PACPLL_A9_name = "/dev/mem";
void *FPGA_PACPLL_bram = NULL;
//volatile uint8_t *FPGA_SPMC_bram = NULL;
void *FPGA_PACPLL_cfg_reg = NULL;
void *FPGA_PACPLL_gpio = NULL;
void *FPGA_PACPLL_gpio2 = NULL;
size_t FPGA_PACPLL_CFG_block_size  = 0; // CFG space 
size_t FPGA_PACPLL_GPIO_block_size  = 0; // GPIO space 
size_t FPGA_PACPLL_BRAM_block_size = 0; // BRAM space PACPLL
size_t FPGA_SPMC_BRAM_block_size = 0; // BRAM space SPMC

#define DEVELOPMENT_PACPLL_OP

//fprintf(stderr, "");

extern pthread_attr_t gpio_reading_attr;
extern pthread_mutex_t gpio_reading_mutexsum;
extern int gpio_reading_control;
extern double gpio_reading_FIRV_vector[MAX_FIR_VALUES];
extern double gpio_reading_FIRV_vector_CH3_mapping;
extern double gpio_reading_FIRV_vector_CH4_mapping;
extern double gpio_reading_FIRV_vector_CH5_mapping;
extern pthread_t gpio_reading_thread;

extern pthread_attr_t stream_server_attr;
extern pthread_mutex_t stream_server_mutexsum;
extern int stream_server_control;
extern pthread_t stream_server_thread;

/*
 * RedPitaya A9 FPGA Memory Mapping Init
 * ------------------------------------------------------------
 */

int rp_PAC_App_Init(){
        int fd;

        fprintf(stderr, "*** INIT RP FPGA RPSPMC PACPLL ***\n");

        FPGA_PACPLL_BRAM_block_size  = 2048*sysconf(_SC_PAGESIZE); // Dual Ported FPGA BRAM
        FPGA_SPMC_BRAM_block_size    = 2048*sysconf(_SC_PAGESIZE); // Dual Ported FPGA BRAM
        FPGA_PACPLL_CFG_block_size   = FPGA_CFG_PAGES*sysconf (_SC_PAGESIZE);   // sysconf (_SC_PAGESIZE) is 0x1000; map CFG + GPIO pages
        FPGA_PACPLL_GPIO_block_size  = FPGA_GPIO_PAGES*sysconf (_SC_PAGESIZE);   // sysconf (_SC_PAGESIZE) is 0x1000; map CFG + GPIO pages
       
        if ((fd = open (FPGA_PACPLL_A9_name, O_RDWR|O_SYNC)) < 0) { // O_SYNC, O_DIRECT
                fprintf(stderr, "EE *** FPGA A9 /dev/mem FD OPEN FAILED ***\n");
                perror ("open");
                return RP_EOOR;
        }

        FPGA_PACPLL_bram = mmap (NULL, FPGA_PACPLL_BRAM_block_size,
                                 PROT_READ|PROT_WRITE, MAP_SHARED, fd, FPGA_BRAM_PACPLL_BASE);
        if (FPGA_PACPLL_bram == MAP_FAILED){
                fprintf(stderr, "EE ** FPGA MMAP PACPLL_BRAM block failed ***\n");
                return RP_EOOR;
        }

        FPGA_PACPLL_cfg_reg = mmap (NULL, FPGA_PACPLL_CFG_block_size,
                                    PROT_READ|PROT_WRITE,  MAP_SHARED, fd, FPGA_CFG_REG);
        if (FPGA_PACPLL_cfg_reg == MAP_FAILED){
                fprintf(stderr, "EE *** FPGA MMAP PACPLL_CFG2 block failed ***\n");
                return RP_EOOR;
        }
        
        FPGA_PACPLL_gpio = mmap (NULL, FPGA_PACPLL_GPIO_block_size,
                                PROT_READ|PROT_WRITE,  MAP_SHARED, fd, FPGA_GPIO_BASE);
        if (FPGA_PACPLL_gpio == MAP_FAILED){
                fprintf(stderr, "EE *** FPGA MMAP PACPLL_GPIO block failed ***\n");
                return RP_EOOR;
        }

        fprintf(stderr, "INIT RP FPGA RPSPMC PACPLL: BRAM, CONTROL and GPIO MEMORY MAPPINGs completed.\n");
        
#ifdef DEVELOPMENT_PACPLL_OP
        fprintf(stderr, "INIT RP FPGA RPSPMC PACPLL. --- FPGA MEMORY MAPPING ---\n");
        fprintf(stderr, "RP FPGA RPSPMC PACPLL PAGESIZE:        0x%08lx\n", (unsigned long)(sysconf (_SC_PAGESIZE)));
        fprintf(stderr, "RP FPGA RPSPMC PACPLL BRAM PLL  mapped 0x%08lx - 0x%08lx   block length: 0x%08lx\n", (unsigned long)FPGA_BRAM_PACPLL_BASE, (unsigned long)(FPGA_BRAM_PACPLL_BASE + FPGA_PACPLL_BRAM_block_size-1), (unsigned long)(FPGA_PACPLL_BRAM_block_size));
        fprintf(stderr, "RP FPGA RPSPMC PACPLL CFG REG   mapped 0x%08lx - 0x%08lx   block length: 0x%08lx\n", (unsigned long)FPGA_CFG_REG,   (unsigned long)(FPGA_CFG_REG + FPGA_PACPLL_CFG_block_size-1),  (unsigned long)(FPGA_PACPLL_CFG_block_size));
        fprintf(stderr, "RP FPGA RPSPMC PACPLL GPIO REGs mapped 0x%08lx - 0x%08lx   block length: 0x%08lx\n", (unsigned long)FPGA_GPIO_BASE, (unsigned long)(FPGA_GPIO_BASE + FPGA_PACPLL_GPIO_block_size-1), (unsigned long)(FPGA_PACPLL_GPIO_block_size));
#endif

        rp_spmc_gvp_config (); // assure GVP is in reset mode
        
        fprintf(stderr, "INIT RP FPGA RPSPMC PACPLL: initializing DMA support.\n");
        spm_dma_instance = new spmc_dma_support (fd); // create and init DMA support module

        fprintf(stderr, "INIT RP FPGA RPSPMC PACPLL: initializing worker threads.\n");

        srand(time(NULL));   // init random

        pthread_mutex_init (&gpio_reading_mutexsum, NULL);
        pthread_attr_init (&gpio_reading_attr);
        pthread_attr_setdetachstate (&gpio_reading_attr, PTHREAD_CREATE_JOINABLE);
        pthread_create ( &gpio_reading_thread, &gpio_reading_attr, thread_gpio_reading_FIR, NULL); // start GPIO reading thread FIRs
        pthread_attr_destroy (&gpio_reading_attr);

        pthread_mutex_init (&stream_server_mutexsum, NULL);
        pthread_attr_init (&stream_server_attr);
        pthread_attr_setdetachstate (&stream_server_attr, PTHREAD_CREATE_JOINABLE);
        pthread_create ( &stream_server_thread, &stream_server_attr, thread_stream_server, NULL); // start stream server
        pthread_attr_destroy (&stream_server_attr);
        
        fprintf(stderr, "INIT RP FPGA RPSPMC PACPLL completed.\n");

        return RP_OK;
}

void rp_PAC_App_Release(){
        void *status;
        gpio_reading_control = 0;
        pthread_join (gpio_reading_thread, &status);
        pthread_mutex_destroy (&gpio_reading_mutexsum);

        stream_server_control = 0;
        pthread_join (stream_server_thread, &status);
        pthread_mutex_destroy (&stream_server_mutexsum);

        munmap (FPGA_PACPLL_gpio, FPGA_PACPLL_GPIO_block_size);
        munmap (FPGA_PACPLL_cfg_reg, FPGA_PACPLL_CFG_block_size);
        munmap (FPGA_PACPLL_bram, FPGA_PACPLL_BRAM_block_size);
#ifdef DEVELOPMENT_PACPLL_OP
        fprintf(stderr, "RP FPGA maps unmapped.\n");
#endif
        pthread_exit (NULL);

        if (spm_dma_instance)
                delete spm_dma_instance;
        spm_dma_instance = NULL;
}



long double cpu_values[4] = {0, 0, 0, 0}; /* reading only user, nice, system, idle */

int phase_setpoint_qcordicatan = 0;

 

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
        
        OPERATION.Update ();
        
        if ( OPERATION.Value () != 4  ){
                if( SHR_DEC_DATA.IsNewValue ()
                    || TRANSPORT_DECIMATION.IsNewValue ()
                    || TRANSPORT_MODE.IsNewValue ()
                    || FREQUENCY_CENTER.IsNewValue ()
                    ){
                        SHR_DEC_DATA.Update ();
                        TRANSPORT_DECIMATION.Update ();
                        TRANSPORT_MODE.Update ();
                        FREQUENCY_CENTER.Update ();

                        rp_PAC_configure_transport (-1,
                                                    SHR_DEC_DATA.Value (),
                                                    -1,
                                                    TRANSPORT_DECIMATION.Value (),
                                                    TRANSPORT_MODE.Value (),
                                                    FREQUENCY_CENTER.Value()
                                                    );
                }
        }

        if (AM_set_prev != AMPLITUDE_FB_SETPOINT.Value () && OPERATION.Value () == 4 && SET_SINGLESHOT_TRANSPORT_TRIGGER.Value ()){
                rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_SINGLE, 4096, TRANSPORT_MODE.Value ());
                usleep (trigger_delay);
        }
        if (AMPLITUDE_FB_SETPOINT.IsNewValue ()
            || AMPLITUDE_FB_CP.IsNewValue ()
            || AMPLITUDE_FB_CI.IsNewValue ()
            || EXEC_FB_UPPER.IsNewValue ()
            || EXEC_FB_LOWER.IsNewValue ()
            || VOLUME_MANUAL.IsNewValue ()
            || AMPLITUDE_CONTROLLER.IsNewValue () ){

                AMPLITUDE_FB_SETPOINT.Update ();
                AMPLITUDE_FB_CP.Update ();
                AMPLITUDE_FB_CI.Update ();
                EXEC_FB_UPPER.Update ();
                EXEC_FB_LOWER.Update ();
                VOLUME_MANUAL.Update ();
                AMPLITUDE_CONTROLLER.Update ();
                
                rp_PAC_set_amplitude_controller (
                                                 AMPLITUDE_FB_SETPOINT.Value ()/1000., // mv to V
                                                 AMPLITUDE_FB_CP.Value (),
                                                 AMPLITUDE_FB_CI.Value (),
                                                 EXEC_FB_UPPER.Value ()/1000., // mV -> +/-1V
                                                 EXEC_FB_LOWER.Value ()/1000.,
                                                 VOLUME_MANUAL.Value() / 1000., // mV -> V in reset mode
                                                 OPERATION.Value() < 6 ? AMPLITUDE_CONTROLLER.Value () : 0 // enable? (in tune mode auto off)
                                                 );
        }
        
        VOL_set_prev = VOLUME_MANUAL.Value();
        AM_set_prev  = AMPLITUDE_FB_SETPOINT.Value ();
        
        if (PH_set_prev != PHASE_FB_SETPOINT.Value () && OPERATION.Value () == 4 && SET_SINGLESHOT_TRANSPORT_TRIGGER.Value ()){
                rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_SINGLE, 4096, TRANSPORT_MODE.Value ());
                usleep(trigger_delay);
        }

        // in tune mode disable controllers autmatically and do not touch DDS settings, but allow Q Control
        if (OPERATION.Value() < 6) // do not update here in tune mode!
                if (PHASE_FB_SETPOINT.IsNewValue ()
                    || PHASE_FB_CP.IsNewValue ()
                    || PHASE_FB_CI.IsNewValue ()
                    || FREQ_FB_UPPER.IsNewValue ()
                    || FREQ_FB_LOWER.IsNewValue ()
                    || PHASE_HOLD_AM_NOISE_LIMIT.IsNewValue ()
                    || FREQUENCY_MANUAL.IsNewValue ()
                    || PHASE_CONTROLLER.IsNewValue ()
                    || PHASE_UNWRAPPING_ALWAYS.IsNewValue () ){

                        PHASE_FB_SETPOINT.Update ();
                        PHASE_FB_CP.Update ();
                        PHASE_FB_CI.Update ();
                        FREQ_FB_UPPER.Update ();
                        FREQ_FB_LOWER.Update ();
                        PHASE_HOLD_AM_NOISE_LIMIT.Update ();
                        FREQUENCY_MANUAL.Update ();
                        PHASE_CONTROLLER.Update ();
                        PHASE_UNWRAPPING_ALWAYS.Update ();
                        rp_PAC_set_phase_controller (
                                             PHASE_FB_SETPOINT.Value ()/180.*M_PI, // Phase
                                             PHASE_FB_CP.Value (),
                                             PHASE_FB_CI.Value (),
                                             FREQ_FB_UPPER.Value (), // Hz
                                             FREQ_FB_LOWER.Value (),
                                             PHASE_HOLD_AM_NOISE_LIMIT.Value ()/1000., // mV to V
                                             FREQUENCY_MANUAL.Value (), // manul freq in reset mode
                                             PHASE_CONTROLLER.Value () ? 1 : 0 // enable ?
                                             | PHASE_UNWRAPPING_ALWAYS.Value ()?4:0
                                             );
                        PH_set_prev = PHASE_FB_SETPOINT.Value ();
                }
        
        if (VOL_set_prev != VOLUME_MANUAL.Value() && OPERATION.Value () == 4 && SET_SINGLESHOT_TRANSPORT_TRIGGER.Value ()){
                rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_SINGLE, 4096, TRANSPORT_MODE.Value ());
                usleep(trigger_delay);
        }

        if (PACTAU.IsNewValue() || PACATAU.IsNewValue() || LCK_PHASE.IsNewValue () || LCK_AMPLITUDE.IsNewValue ()){
                PACTAU.Update ();
                PACATAU.Update ();
                LCK_AMPLITUDE.Update ();
                LCK_PHASE.Update ();
                rp_PAC_set_pactau (PACTAU.Value(), PACATAU.Value(), LCK_AMPLITUDE.Value ()? 1:0 | LCK_PHASE.Value ()? 2:0); // periods, periods, ms -> s
        }

        if (PAC_DCTAU.IsNewValue ()){
                PAC_DCTAU.Update ();
                rp_PAC_set_dc_filter (signal_dc_measured, PAC_DCTAU.Value() * 1e-3);
        }

        if (QC_GAIN.IsNewValue () || QC_PHASE.IsNewValue () || QCONTROL.IsNewValue ()){
                QC_GAIN.Update ();
                QC_PHASE.Update ();
                QCONTROL.Update ();
                rp_PAC_set_qcontrol (QC_GAIN.Value (), QC_PHASE.Value (), QCONTROL.Value ());
        }

        if ( (AM_sw_prev != AMPLITUDE_CONTROLLER.Value() || PH_sw_prev!= PHASE_CONTROLLER.Value () || uw_sw_prev != PHASE_UNWRAPPING_ALWAYS.Value ())
             && OPERATION.Value () == 4 && SET_SINGLESHOT_TRANSPORT_TRIGGER.Value ()){
                AM_sw_prev = AMPLITUDE_CONTROLLER.Value ();
                PH_sw_prev = PHASE_CONTROLLER.Value ();
                uw_sw_prev = PHASE_UNWRAPPING_ALWAYS.Value ();
                rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_SINGLE, 4096, TRANSPORT_MODE.Value ());
                usleep(trigger_delay);
        }

        if (   DFREQ_FB_SETPOINT.IsNewValue ()
            || DFREQ_FB_CP.IsNewValue ()
            || DFREQ_FB_CI.IsNewValue ()
            || DFREQ_FB_UPPER.IsNewValue ()
            || DFREQ_FB_LOWER.IsNewValue ()
            || DFREQ_CONTROLLER.IsNewValue ()
               ){
                DFREQ_FB_SETPOINT.Update ();
                DFREQ_FB_CP.Update ();
                DFREQ_FB_CI.Update ();
                DFREQ_FB_UPPER.Update ();
                DFREQ_FB_LOWER.Update ();
                DFREQ_CONTROLLER.Update ();
                
                rp_PAC_set_dfreq_controller (
                                             DFREQ_FB_SETPOINT.Value (), // dHz -> ...
                                             DFREQ_FB_CP.Value (),
                                             DFREQ_FB_CI.Value (),
                                             DFREQ_FB_UPPER.Value ()/1000., // mV -> V
                                             DFREQ_FB_LOWER.Value ()/1000.,
                                             0.,
                                             DFREQ_CONTROLLER.Value ()
                                             );
        }
        
        if (   PULSE_FORM_BIAS0.IsNewValue ()
            || PULSE_FORM_BIAS1.IsNewValue ()
            || PULSE_FORM_PHASE0.IsNewValue ()
            || PULSE_FORM_PHASE1.IsNewValue ()
            || PULSE_FORM_WIDTH0.IsNewValue ()
            || PULSE_FORM_WIDTH1.IsNewValue ()
            || PULSE_FORM_HEIGHT0.IsNewValue ()
            || PULSE_FORM_HEIGHT1.IsNewValue ()
            || PULSE_FORM_WIDTH0IF.IsNewValue ()
            || PULSE_FORM_WIDTH1IF.IsNewValue ()
            || PULSE_FORM_HEIGHT0IF.IsNewValue ()
            || PULSE_FORM_HEIGHT1IF.IsNewValue ()
            || PULSE_FORM_SHAPEXW.IsNewValue ()
            || PULSE_FORM_SHAPEXWIF.IsNewValue ()
            || PULSE_FORM_SHAPEX.IsNewValue ()
            || PULSE_FORM_SHAPEXIF.IsNewValue ()
            ){
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
}


const char *rp_app_desc(void)
{
        return (const char *)"Red Pitaya PACPLL.\n";
}


int rp_app_init(void)
{
        fprintf(stderr, "Loading RPSPMC PACPLL application\n");

        // Initialization of PAC api
        if (rp_PAC_App_Init() != RP_OK) 
                {
                        fprintf(stderr, "Red Pitaya RPSPMC PACPLL API init failed!\n");
                        return EXIT_FAILURE;
                }
        else fprintf(stderr, "Red Pitaya RPSPMC PACPLL API init memory mappings success!\n");

        rp_PAC_auto_dc_offset_adjust ();

        //Set signal update interval
        CDataManager::GetInstance()->SetSignalInterval(SIGNAL_UPDATE_INTERVAL);
        CDataManager::GetInstance()->SetParamInterval(PARAMETER_UPDATE_INTERVAL);

        // Init PAC
        set_PAC_config();

        // init block transport for scope
        rp_PAC_start_transport (PACPLL_CFG_TRANSPORT_LOOP, 4096, TRANSPORT_MODE.Value ());

        // Init SPMC
        rp_spmc_AD5791_init ();

        rp_spmc_gvp_init ();

        fprintf(stderr, "Red Pitaya RPSPMC PACPLL API init completed!\n");
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
        case 8: // Debug and Testing *** Channels [McBSP bits]
                i += index_shift;
                for (k=0; k < SIGNAL_SIZE_DEFAULT; ++k){
                        int32_t ix32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; //DBG AXIS7 -- IN1 - AC (IN1-MDC AC dec + filtered) (16)
                        int32_t iy32 = *((int32_t *)((uint8_t*)FPGA_PACPLL_bram+i)); i+=4; //DBG AXIS8 -- IN1 - DC (IN1 IIR LP dec + filtered) (16)
                        SIGNAL_CH1[k] = (float)ix32; //*gain1/Q22*1000.;
                        SIGNAL_CH2[k] = (float)iy32; //*gain2/Q22*1000.; // where is the 1/2 ???
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
        // adjust DDS via controller reset value ManualFreq + f and enable = 0
        rp_PAC_set_phase_controller (
                                     PHASE_FB_SETPOINT.Value ()/180.*M_PI, // Phase
                                     PHASE_FB_CP.Value (),
                                     PHASE_FB_CI.Value (),
                                     FREQ_FB_UPPER.Value (), // Hz
                                     FREQ_FB_LOWER.Value (),
                                     PHASE_HOLD_AM_NOISE_LIMIT.Value ()/1000., // mV to V
                                     FREQUENCY_MANUAL.Value () + f, // manul freq in reset mode
                                     0 // disable, use manual freq
                                     | PHASE_UNWRAPPING_ALWAYS.Value ()?4:0
                                     );



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
                //rp_PAC_adjust_dds (FREQUENCY_MANUAL.Value() + f);
                // adjust DDS via controller reset value ManualFreq + f and enable = 0
                rp_PAC_set_phase_controller (
                                             PHASE_FB_SETPOINT.Value ()/180.*M_PI, // Phase
                                             PHASE_FB_CP.Value (),
                                             PHASE_FB_CI.Value (),
                                             FREQ_FB_UPPER.Value (), // Hz
                                             FREQ_FB_LOWER.Value (),
                                             PHASE_HOLD_AM_NOISE_LIMIT.Value ()/1000., // mV to V
                                             FREQUENCY_MANUAL.Value () + f, // manul freq in reset mode
                                             0 // disable, use manual freq
                                             | PHASE_UNWRAPPING_ALWAYS.Value ()?4:0
                                             );
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
        //int ch;
        int status[3];
        static int last_op=0;
        
        if (verbose > 2) fprintf(stderr, "** Update Signals **\n");

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
                //if (verbose > 3) fprintf(stderr, "UpdateSignals get GPIO reading:\n");

                // Slow GPIO MONITOR in strip plotter mode
                // Push it to vector
                //ch = TRANSPORT_CH3.Value ();
                //if (verbose > 3) fprintf(stderr, "UpdateSignals: CH3=%d \n", ch);
                g_data_signal_ch3.erase (g_data_signal_ch3.begin());
                //read_gpio_reg_int32 (n,m)
                g_data_signal_ch3.push_back (gpio_reading_FIRV_vector_CH3_mapping/GPIO_FIR_LEN * GAIN3.Value ());

                //ch = TRANSPORT_CH4.Value ();
                //if (verbose > 3) fprintf(stderr, "UpdateSignals: CH4=%d \n", ch);
                g_data_signal_ch4.erase (g_data_signal_ch4.begin());
                g_data_signal_ch4.push_back (gpio_reading_FIRV_vector_CH4_mapping/GPIO_FIR_LEN * GAIN4.Value ());

                //ch = TRANSPORT_CH5.Value ();
                //if (verbose > 3) fprintf(stderr, "UpdateSignals: CH5=%d \n", ch);
                g_data_signal_ch5.erase (g_data_signal_ch5.begin());
                g_data_signal_ch5.push_back (gpio_reading_FIRV_vector_CH5_mapping/GPIO_FIR_LEN * GAIN5.Value ());

                // Copy data to signals
                // if (verbose > 3) fprintf(stderr, "UpdateSignals copy signals\n");
                for (int i = 0; i < SIGNAL_SIZE_DEFAULT; i++){
                        SIGNAL_CH3[i] = g_data_signal_ch3[i];
                        SIGNAL_CH4[i] = g_data_signal_ch4[i];
                        SIGNAL_CH5[i] = g_data_signal_ch5[i];
                }
        }
        
        //if (verbose > 3) fprintf(stderr, "UpdateSignal complete.\n");

        // GPIO via FIR thread
        VOLUME_MONITOR.Value ()   = gpio_reading_FIRV_vector[GPIO_READING_AMPL]/GPIO_FIR_LEN * 1000./QCORDICSQRT;
        PHASE_MONITOR.Value ()    = gpio_reading_FIRV_vector[GPIO_READING_PHASE]/GPIO_FIR_LEN * 180./QCORDICATAN/M_PI;
        EXEC_MONITOR.Value ()     = gpio_reading_FIRV_vector[GPIO_READING_EXEC]/GPIO_FIR_LEN  * 1000/QEXEC;
        DDS_FREQ_MONITOR.Value () = gpio_reading_FIRV_vector[GPIO_READING_DDS_FREQ]/GPIO_FIR_LEN;
        DFREQ_MONITOR.Value ()    = gpio_reading_FIRV_vector[GPIO_READING_DFREQ]/GPIO_FIR_LEN;
        CONTROL_DFREQ_MONITOR.Value () = gpio_reading_FIRV_vector[GPIO_READING_CONTROL_DFREQ]/GPIO_FIR_LEN * 10000/Q31;
        
        rp_PAC_auto_dc_offset_correct ();

        last_op = OPERATION.Value ();

        // RPSPMC Update Monitors
        rp_spmc_update_readings ();
}


void UpdateParams(void){
        //if (verbose > 2) fprintf(stderr, "** Update Params **\n");
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

        //if (verbose > 3) fprintf(stderr, "UpdateParams: text update\n");
        pacpll_text.Value() = "Hello this is the RP PACPLL Server.    ";

        if (counter.Value()>30000) {
                counter.Value()=0;
        }
        //if (verbose > 3) fprintf(stderr, "UpdateParams complete.\n");
}


        
// RPSPMC Check New Parameters
// ****************************************
void OnNewParams_RPSPMC(void){
        static int do_rotate=0;
        static double lck_f0_frq=0;
        
        //SPMC_GVP_STATUS.Update ();

        if (SPMC_Z_SERVO_SETPOINT.IsNewValue()
            || SPMC_Z_SERVO_CP.IsNewValue()
            || SPMC_Z_SERVO_CI.IsNewValue()
            || SPMC_Z_SERVO_UPPER.IsNewValue()
            || SPMC_Z_SERVO_LOWER.IsNewValue()
            || SPMC_Z_SERVO_SETPOINT_CZ.IsNewValue()
            || SPMC_Z_SERVO_IN_OFFSETCOMP.IsNewValue()
            || SPMC_Z_SERVO_MODE.IsNewValue()
            ){
                SPMC_Z_SERVO_SETPOINT.Update ();
                SPMC_Z_SERVO_CP.Update ();
                SPMC_Z_SERVO_CI.Update ();
                SPMC_Z_SERVO_UPPER.Update ();
                SPMC_Z_SERVO_LOWER.Update ();
                SPMC_Z_SERVO_SETPOINT_CZ.Update ();
                SPMC_Z_SERVO_IN_OFFSETCOMP.Update ();
                SPMC_Z_SERVO_MODE.Update ();

                fprintf(stderr, "Z Servo Mode = %d\n", SPMC_Z_SERVO_MODE.Value());

                rp_spmc_set_zservo_controller (SPMC_Z_SERVO_SETPOINT.Value(), SPMC_Z_SERVO_CP.Value(), SPMC_Z_SERVO_CI.Value(), SPMC_Z_SERVO_UPPER.Value(), SPMC_Z_SERVO_LOWER.Value());
                rp_spmc_set_zservo_gxsm_speciality_setting (SPMC_Z_SERVO_MODE.Value(), SPMC_Z_SERVO_SETPOINT_CZ.Value(), SPMC_Z_SERVO_IN_OFFSETCOMP.Value());
        }
        

        if (SPMC_ALPHA.IsNewValue () || do_rotate){
                SPMC_ALPHA.Update ();
                do_rotate = rp_spmc_set_rotation (SPMC_ALPHA.Value (), SPMC_SET_OFFSET_XY_SLEW.Value ());
        }
        
        if (SPMC_SLOPE_DZX.IsNewValue () || SPMC_SLOPE_DZY.IsNewValue () || SPMC_SLOPE_SLEW.IsNewValue ()){
                SPMC_SLOPE_SLEW.Update ();
                SPMC_SLOPE_DZX.Update ();
                SPMC_SLOPE_DZY.Update ();
                rp_spmc_set_slope (SPMC_SLOPE_DZX.Value (), SPMC_SLOPE_DZY.Value (), SPMC_SLOPE_SLEW.Value ());
        }
        
        if (SPMC_SET_OFFSET_X.IsNewValue ()
            || SPMC_SET_OFFSET_Y.IsNewValue ()
            || SPMC_SET_OFFSET_Z.IsNewValue ()
            ){
                SPMC_BIAS.Update ();
                SPMC_SET_OFFSET_X.Update ();
                SPMC_SET_OFFSET_Y.Update ();
                SPMC_SET_OFFSET_Z.Update ();
                SPMC_SET_OFFSET_XY_SLEW.Update ();
                SPMC_SET_OFFSET_Z_SLEW.Update ();
                rp_spmc_set_offsets (SPMC_SET_OFFSET_X.Value (),
                                     SPMC_SET_OFFSET_Y.Value (),
                                     SPMC_SET_OFFSET_Z.Value (),
                                     SPMC_BIAS.Value(),
                                     SPMC_SET_OFFSET_XY_SLEW.Value (),
                                     SPMC_SET_OFFSET_Z_SLEW.Value ()
                                     );
        } else if (SPMC_BIAS.IsNewValue()){
                SPMC_BIAS.Update ();
                rp_spmc_set_bias (SPMC_BIAS.Value());
        }
        

        if (SPMC_SET_SCANPOS_X.IsNewValue () || SPMC_SET_SCANPOS_Y.IsNewValue ()){
                SPMC_SET_SCANPOS_SLEW.Update ();
                SPMC_SET_SCANPOS_OPTS.Update ();
                SPMC_SET_SCANPOS_X.Update ();
                SPMC_SET_SCANPOS_Y.Update ();
                rp_spmc_set_scanpos (SPMC_SET_SCANPOS_X.Value (),
                                     SPMC_SET_SCANPOS_Y.Value (),
                                     SPMC_SET_SCANPOS_SLEW.Value (),
                                     SPMC_SET_SCANPOS_OPTS.Value ());
        }

        // {"parameters":{"SPMC_GVP_VECTOR_PC":{"value":2},"SPMC_GVP_VECTOR__N":{"value":100},"SPMC_GVP_VECTOR__O":{"value":0},"SPMC_GVP_VECTORNRP":{"value":0},"SPMC_GVP_VECTORNXT":{"value":0},"SPMC_GVP_VECTOR_DX":{"value":0},"SPMC_GVP_VECTOR_DY":{"value":0},"SPMC_GVP_VECTOR_DZ":{"value":0},"SPMC_GVP_VECTOR_DU":{"value":-1},"SPMC_GVP_VECTOR_AA":{"value":0},"SPMC_GVP_VECTOR_BB":{"value":0},"SPMC_GVP_VECTORSLW":{"value":10000}}}

        int dirty=0;
        if (SPMC_GVP_VECTOR_PC.IsNewValue ()){ SPMC_GVP_VECTOR_PC.Update (); ++dirty; }
        if (SPMC_GVP_VECTOR__N.IsNewValue ()){ SPMC_GVP_VECTOR__N.Update (); ++dirty; }
        if (SPMC_GVP_VECTOR__O.IsNewValue ()){ SPMC_GVP_VECTOR__O.Update (); ++dirty; }
        if (SPMC_GVP_VECTORSRC.IsNewValue ()){ SPMC_GVP_VECTORSRC.Update (); ++dirty; }
        if (SPMC_GVP_VECTORNRP.IsNewValue ()){ SPMC_GVP_VECTORNRP.Update (); ++dirty; }
        if (SPMC_GVP_VECTORNXT.IsNewValue ()){ SPMC_GVP_VECTORNXT.Update (); ++dirty; }
        if (SPMC_GVP_VECTOR_DX.IsNewValue ()){ SPMC_GVP_VECTOR_DX.Update (); ++dirty; }
        if (SPMC_GVP_VECTOR_DY.IsNewValue ()){ SPMC_GVP_VECTOR_DY.Update (); ++dirty; }
        if (SPMC_GVP_VECTOR_DZ.IsNewValue ()){ SPMC_GVP_VECTOR_DZ.Update (); ++dirty; }
        if (SPMC_GVP_VECTOR_DU.IsNewValue ()){ SPMC_GVP_VECTOR_DU.Update (); ++dirty; }
        if (SPMC_GVP_VECTOR_AA.IsNewValue ()){ SPMC_GVP_VECTOR_AA.Update (); ++dirty; }
        if (SPMC_GVP_VECTOR_BB.IsNewValue ()){ SPMC_GVP_VECTOR_BB.Update (); ++dirty; }
        if (SPMC_GVP_VECTORSLW.IsNewValue ()){ SPMC_GVP_VECTORSLW.Update (); ++dirty; }

        if (dirty>0){
                SPMC_GVP_LIVE_VECTOR_UPDATE.Update ();
                rp_spmc_set_gvp_vector (SPMC_GVP_VECTOR_PC.Value (),
                                        SPMC_GVP_VECTOR__N.Value (),
                                        (unsigned int)SPMC_GVP_VECTOR__O.Value (),
                                        (unsigned int)SPMC_GVP_VECTORSRC.Value (),
                                        SPMC_GVP_VECTORNRP.Value (),
                                        SPMC_GVP_VECTORNXT.Value (),
                                        SPMC_GVP_VECTOR_DX.Value (),
                                        SPMC_GVP_VECTOR_DY.Value (),
                                        SPMC_GVP_VECTOR_DZ.Value (),
                                        SPMC_GVP_VECTOR_DU.Value (),
                                        SPMC_GVP_VECTOR_AA.Value (),
                                        SPMC_GVP_VECTOR_BB.Value (),
                                        SPMC_GVP_VECTORSLW.Value (),
                                        SPMC_GVP_LIVE_VECTOR_UPDATE.Value ()
                                        );
        }

        if (SPMC_GVP_STREAM_MUX.IsNewValue () || SPMC_GVP_STREAM_MUXH.IsNewValue ()){
                SPMC_GVP_STREAM_MUX.Update ();
                SPMC_GVP_STREAM_MUXH.Update ();
                int mux  = SPMC_GVP_STREAM_MUX.Value ();
                int muxh = SPMC_GVP_STREAM_MUXH.Value ();
                int axis_test=muxh&0xf;
                int axis_value=muxh>>4;
                fprintf(stderr, "*** SPMC_GVP_STREAM_MUX: %08x-%08x (AXIS[%d] <= %d)\n", muxh, mux, axis_test, axis_value);
                rp_set_gvp_stream_mux_selector (mux, axis_test, axis_value);
        }

        if (SPMC_GVP_RESET_OPTIONS.IsNewValue ()){
                SPMC_GVP_RESET_OPTIONS.Update ();
                rp_spmc_gvp_config (true, false, SPMC_GVP_RESET_OPTIONS.Value ());
        }
        
        if (SPMC_GVP_CONTROL_MODE.IsNewValue ()){
                std::stringstream info;
                info << "SPMC_GVP_CONTROL_MODE: {";
                // rp_spmc_gvp_config (reset, program, pause, [resetoptions]);
                SPMC_GVP_CONTROL_MODE.Update ();
                switch (SPMC_GVP_CONTROL_MODE.Value ()){

                case SPMC_GVP_CONTROL_RESET:
                        rp_spmc_gvp_config (true, false);
                        fprintf(stderr, "*** GVP Control Mode RESET, force stop stream server.\n");
                        info << "set RESET, force stop stream server";
                        stream_server_control = (stream_server_control & 0x01) | 4; // set stop bit
                        break;
                        
                case SPMC_GVP_CONTROL_EXECUTE:
                        rp_spmc_gvp_config (true, false);
                        fprintf(stderr, "*** GVP Control Mode EXEC: set GVP to RESET\n");
                        usleep(10000);
                        fprintf(stderr, "*** GVP Control Mode EXEC: reset stream server, clear buffer)\n");
                        stream_server_control = (stream_server_control & 0x01) | 4; // set stop bit
                        usleep(100000);
                        spm_dma_instance->clear_buffer (); // clean buffer now!

                        //rp_spmc_gvp_config (true, false); // make sure GVP is in reset state before
                        fprintf(stderr, "*** GVP Control Mode EXEC: START DMA+stream server\n");
                        //usleep(100000);

                        spm_dma_instance->start_dma (); // get DMA ready to take data
                        stream_server_control = (stream_server_control & 0x01) | 2; // set start bit
                        usleep(10000);

                        rp_spmc_gvp_config (false, false); // take GVP out of reset -> release GVP to start executing VPC now
                        fprintf(stderr, "*** GVP Control Mode EXEC: START 3 (taking out of RESET)\n");

                        info << "EXECUTE: RESET, start stream server, out of RESET";
                        break;
                case SPMC_GVP_CONTROL_GVP_RESET_ONLY:
                        rp_spmc_gvp_config (true, false);
                        fprintf(stderr, "*** GVP Control Mode RESET ONLY -- NO DMA RESET\n");
                case SPMC_GVP_CONTROL_GVP_EXECUTE_NO_DMA:
                        rp_spmc_gvp_config (false, false); // take GVP out of reset -> release GVP to start executing VPC now
                        fprintf(stderr, "*** GVP Control Mode EXECUTE -- NO DMA START\n");
                        break;
                case SPMC_GVP_CONTROL_PAUSE:
                        rp_spmc_gvp_config (false, true );
                        fprintf(stderr, "*** GVP Control Mode PAUSE\n"); info << "set PAUSE"; // SET PAUSE active
                        break;
                case SPMC_GVP_CONTROL_RESUME:
                        rp_spmc_gvp_config (false, false);
                        fprintf(stderr, "*** GVP Control Mode RESUME EXECUTE\n"); info << "unset PAUSE"; // UNSET PAUSE
                        break;
                case SPMC_GVP_CONTROL_RESET_UAB:
                        reset_gvp_positions_uab();
                        break;
                default: info << "INVALID MODE"; break;
                }

                info << " * ROpt=" << SPMC_GVP_RESET_OPTIONS.Value () << "}";
                spmc_stream_server_instance.add_info (info.str());
        }

        // RPSPMC Lock-In...
        if (SPMC_SC_LCK_FREQUENCY.IsNewValue () || SPMC_SC_LCK_GAIN.IsNewValue ()){
                SPMC_SC_LCK_FREQUENCY.Update ();
                SPMC_SC_LCK_GAIN.Update ();
                double gain = SPMC_SC_LCK_GAIN.Value ();
                int mode = 0;
                if (gain > 0) // parameter gain
                        mode = 1;
                else if (gain < 0) // via signal A
                        mode = 2;
                lck_f0_frq = rp_spmc_configure_lockin (SPMC_SC_LCK_FREQUENCY.Value (), gain, mode, SPMC_LOCKIN_F0_CONTROL_REG);
        }

        if (SPMC_SC_LCK_TARGET.IsNewValue () || SPMC_SC_LCK_VOLUME.IsNewValue ()){
                SPMC_SC_LCK_TARGET.Update ();
                SPMC_SC_LCK_VOLUME.Update ();
                rp_spmc_set_modulation (SPMC_SC_LCK_VOLUME.Value (), SPMC_SC_LCK_TARGET.Value ());

                if ((int)SPMC_SC_LCK_TARGET.Value () == 7)
                        rp_spmc_module_read_config_data_timing_test ();
        }

        // LockIn Signal Out BiQuad Filter
        if (SPMC_SC_LCK_F0BQ_Q.IsNewValue () || SPMC_SC_LCK_F0BQ_TAU.IsNewValue () || SPMC_SC_LCK_F0BQ_IIR.IsNewValue ()){
                SPMC_SC_LCK_F0BQ_Q.Update ();
                SPMC_SC_LCK_F0BQ_TAU.Update ();
                SPMC_SC_LCK_F0BQ_IIR.Update ();
                if (SPMC_SC_LCK_F0BQ_Q.Value () > 0.0 && SPMC_SC_LCK_F0BQ_TAU.Value () > 0.0) // BiQuad mode ?
                        rp_spmc_set_biqad_Lck_F0 (1000./SPMC_SC_LCK_F0BQ_TAU.Value (), SPMC_SC_LCK_F0BQ_Q.Value (), lck_f0_frq, SPMC_BIQUAD_F0_CONTROL_REG);
                else
                        if (SPMC_SC_LCK_F0BQ_IIR.Value () > 0.0) // IIR mode ?
                                rp_spmc_set_biqad_Lck_F0_IIR (1000./SPMC_SC_LCK_F0BQ_IIR.Value (), lck_f0_frq, SPMC_BIQUAD_F0_CONTROL_REG);
                        else // pass mode
                                rp_spmc_set_biqad_Lck_F0_pass (SPMC_BIQUAD_F0_CONTROL_REG);
        }
        
}

// PACPLL Check New Parameters
// ****************************************
void OnNewParams_PACPLL(void){
        if (verbose > 2) fprintf(stderr, "** New Params **\n");
        //int x=0;
        //static int ppv=0;
        //static int spv=0;
        static int operation=0;
        //double reading_vector[READING_MAX_VALUES];

        PACVERBOSE.Update ();

        // keep fixed for debugging
        //verbose = PACVERBOSE.Value ();

        
#if 0
        if (ppv == 0) { ppv=parameter_updatePeriod.Value(); parameter_updatePeriod.Update (); }
        if (spv == 0) { spv=signal_updatePeriod.Value(); signal_updatePeriod.Update (); }
        
        if (ppv != parameter_updatePeriod.Value ()){
                CDataManager::GetInstance()->SetParamInterval (parameter_updatePeriod.Value());
                ppv = parameter_updatePeriod.Value ();
        }
        if (spv != signal_updatePeriod.Value ()){
                CDataManager::GetInstance()->SetSignalInterval (signal_updatePeriod.Value());
                spv = signal_updatePeriod.Value ();
        }
#endif
        
        OPERATION.Update ();

        GAIN1.Update ();
        GAIN2.Update ();
        GAIN3.Update ();
        GAIN4.Update ();
        GAIN5.Update ();
        TUNE_SPAN.Update ();
        TUNE_DFREQ.Update ();

        SET_SINGLESHOT_TRANSPORT_TRIGGER.Update ();
        SET_SINGLESHOT_TRIGGER_POST_TIME.Update ();

        BRAM_SCOPE_TRIGGER_MODE.Update ();
        BRAM_SCOPE_TRIGGER_POS.Update ();
        BRAM_SCOPE_TRIGGER_LEVEL.Update ();
        BRAM_SCOPE_SHIFT_POINTS.Update ();
        
        TRANSPORT_CH3.Update ();
        TRANSPORT_CH4.Update ();
        TRANSPORT_CH5.Update ();



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

void OnNewParams(void)
{
        OnNewParams_PACPLL();
        OnNewParams_RPSPMC();
}


void OnNewSignals(void){
        if (verbose > 3) fprintf(stderr, "OnNewSignals()\n");
	// do something
	CDataManager::GetInstance()->UpdateAllSignals();
        if (verbose > 3) fprintf(stderr, "OnNewSignals done.\n");

#if 0
        // do not get anything here :(
        if (SPMC_GVP_VECTOR.IsNewValue()){
                SPMC_GVP_VECTOR.Update();
                fprintf(stderr, "GVP Vector[0]: [");
                for (int i=0; i<16; ++i)
                        fprintf(stderr, "%g ", SPMC_GVP_VECTOR[i]);
                fprintf(stderr, " ]\n");
        }
#endif
}


void PostUpdateSignals(void){
        if (verbose > 3) fprintf(stderr, "PostUpdateSignals()\n");
        if (verbose > 3) fprintf(stderr, "PostUpdateSignals done.\n");
}

// END.
