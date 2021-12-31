/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* SRanger and Gxsm - Gnome X Scanning Microscopy Project
#* universal STM/AFM/SARLS/SPALEED/... controlling and
#* data analysis software
#*
#* DSP tools for Linux
#*
#* Copyright (C) 2014 Percy Zahl
#*
#* Authors: Percy Zahl <zahl@users.sf.net>
#* WWW Home:
#* DSP part:  http://sranger.sf.net
#* Gxsm part: http://gxsm.sf.net
#*
#* This program is free software; you can redistribute it and/or modify
#* it under the terms of the GNU General Public License as published by
#* the Free Software Foundation; either version 2 of the License, or
#* (at your option) any later version.
#*
#* This program is distributed in the hope that it will be useful,
#* but WITHOUT ANY WARRANTY; without even the implied warranty of
#* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#* GNU General Public License for more details.
#*
#* You should have received a copy of the GNU General Public License
#* along with this program; if not, write to the Free Software
#* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
#*/

#ifndef __SRANGER_MK3_DSP_SIGNALS_H
#define __SRANGER_MK3_DSP_SIGNALS_H

/**
# * CAUTION: DO NOT CHANGE FORMAT CONVENTIONS -- MUST MATCH SED SCRIPT TO CONVERT INTO PYTHON
#**/


/**
# * Signal Input management ID's for safe DSP based signal pointer adjustment methods.
# * And special command ID's for FLASH store/restore/erase (no defaults, only fixed power up fallback):
#**/

#define DSP_SIGNAL_VECTOR_ELEMENT_INDEX_REQUEST_MSK   0xffff0000

#define DSP_SIGNAL_NULL_POINTER_REQUEST_ID      0xfe01

#define DSP_SIGNAL_TABLE_STORE_TO_FLASH_ID      0xf001
#define DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_ID  0xf002
#define DSP_SIGNAL_TABLE_ERASE_FLASH_ID         0xf0ee
#define DSP_SIGNAL_TABLE_SEEKRW_FLASH_ID        0xf0e0
#define DSP_SIGNAL_TABLE_READ_FLASH_ID          0xf0e1
#define DSP_SIGNAL_TABLE_WRITE_FLASH_ID         0xf0e2
#define DSP_SIGNAL_TABLE_TEST0_FLASH_ID         0xf0e7
#define DSP_SIGNAL_TABLE_TEST1_FLASH_ID         0xf0e8

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

#define DSP_SIGNAL_BASE_BLOCK_D_ID             0x4000
#define DSP_SIGNAL_ANALOG_AVG_INPUT_ID        (DSP_SIGNAL_BASE_BLOCK_D_ID+1)
#define DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID     (DSP_SIGNAL_ANALOG_AVG_INPUT_ID+1)
#define DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID     (DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID+1)
#define DSP_SIGNAL_SCO1_INPUT_ID               (DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID+1)
#define DSP_SIGNAL_SCO1_AMPLITUDE_INPUT_ID     (DSP_SIGNAL_SCO1_INPUT_ID+1)
#define DSP_SIGNAL_SCO2_INPUT_ID               (DSP_SIGNAL_SCO1_AMPLITUDE_INPUT_ID+1)
#define DSP_SIGNAL_SCO2_AMPLITUDE_INPUT_ID     (DSP_SIGNAL_SCO2_INPUT_ID+1)

#define LAST_INPUT_ID                          DSP_SIGNAL_SCO2_AMPLITUDE_INPUT_ID

/**
#* ERROR/WARNING RETURN CODES
#**/

#define DSP_SIGNAL_ADJUST_NEGID_ERROR                -2
#define DSP_SIGNAL_ADJUST_INVALIDID_ERROR            -3
#define DSP_SIGNAL_ADJUST_INVALID_SIGNAL_ERROR       -5
#define DSP_SIGNAL_ADJUST_INVALID_SIGNAL_INPUT_ERROR -6

#define DSP_SIGNAL_TABLE_STORE_TO_FLASH_WRITE_ERROR                   -2
#define DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_READ_ERROR                -3
#define DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_FLASH_BLOCKID_ERROR       -10
#define DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_VERSION_MISMATCH_ERROR    -11

#define DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_SIGNAL_LIST_VERSION_MISMATCH   -12
#define DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_INPUT_LIST_VERSION_MISMATCH    -13

#define DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_FLASH_DATE_WARNING        -20


/**
#* Signal pointers and description table for RT signal handling, stored in ext. DRAM on DSP in stripped version
#**/

#define DSP32Qs15dot16TOV     (10.0/(32767.*(1<<16)))
#define DSP32Qs15dot16TO_Volt (10.0/(32767.*(1<<16)))
#define DSP32Qs15dot0TO_Volt  (10.0/32767.)
#define DSP32Qs23dot8TO_Volt  (10.0/(32767.*(1<<8)))
#define CPN(N) ((double)(1LL<<(N))-1.)

#define NUM_SIGNALS 101

#define FB_SPM_FLASHBLOCK_IDENTIFICATION_A  0x10aa
#define FB_SPM_FLASHBLOCK_IDENTIFICATION_B  0x0001
#define FB_SPM_SIGNAL_LIST_VERSION          0x0006
#define FB_SPM_SIGNAL_INPUT_VERSION         0x0004

#define MAX_INPUT_NUMBER_LIMIT              0x100  // just a safey limit for FLASH storage

#ifndef DSPEMU
#pragma DATA_SECTION(dsp_signal_lookup, "SMAGIC")
#endif

#endif

/* END DSP_SIGNALS HEADER PART */

/* CREATE TABLE */

#ifdef CREATE_DSP_SIGNAL_LOOKUP



DSP_SIG dsp_signal_lookup[NUM_SIGNALS] = {
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.in[0]), "In 0", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT1"), // 	DSP_SIG IN[0..7] MUST BE FIRST IN THIS LIST!!
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.in[1]), "In 1", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT2"), // 	DSP_SIG IN[0..7]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.in[2]), "In 2", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT3"), // 	DSP_SIG IN[0..7]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.in[3]), "In 3", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT4"), // 	DSP_SIG IN[0..7]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.in[4]), "In 4", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT5"), // 	DSP_SIG IN[0..7]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.in[5]), "In 5", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT6"), // 	DSP_SIG IN[0..7]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.in[6]), "In 6", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT7"), // 	DSP_SIG IN[0..7]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.in[7]), "In 7", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT8"), // 	DSP_SIG IN[0..7] MUST BE at pos 8 IN THIS LIST!!
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.counter[0]), "Counter 0", "CNT", 1, "Counter", "FPGA based Counter Channel 1"),  // 	DSP_SIG Counter[0]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.counter[1]), "Counter 1", "CNT", 1, "Counter", "FPGA based Counter Channel 2"), // 	DSP_SIG Counter[1]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.LockIn_0A),     "LockIn A-0",   "*V",   DSP32Qs15dot16TO_Volt, "LockIn", "LockIn A 0 (average over full periods)"), //  DSP_SIG LockInA[0]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.LockIn_1stA),   "LockIn A-1st", "*dV",  DSP32Qs15dot16TO_Volt, "LockIn", "LockIn A 1st order"), //  DSP_SIG LockInA[1]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.LockIn_2ndA),   "LockIn A-2nd", "*ddV", DSP32Qs15dot16TO_Volt, "LockIn", "LockIn A 2nd oder"),  //  DSP_SIG LockInA[2]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.LockIn_0B),     "LockIn B-0",   "*V",   DSP32Qs15dot16TO_Volt, "LockIn", "LockIn B 0 (average over full periods)"), //  DSP_SIG LockInB[0]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.LockIn_1stB),   "LockIn B-1st", "*dV",  DSP32Qs15dot16TO_Volt, "LockIn", "LockIn B 1st order"), //  DSP_SIG LockInB[1]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.LockIn_2ndB),   "LockIn B-2nd", "*ddV", DSP32Qs15dot16TO_Volt, "LockIn", "LockIn B 2nd order"), //  DSP_SIG LockInB[2]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.LockIn_ref),    "LockIn Ref",    "V",   DSP32Qs15dot0TO_Volt,  "LockIn", "LockIn Reference Sinewave (Modulation) (Internal Reference Signal)"), //  DSP_SIG LockInMod;
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.PRB_ACPhaseA32),"LockIn Phase A","deg", 180./(2913*CPN(16)),   "LockIn", "DSP internal LockIn PhaseA32 watch"),    // DSP_SIG LockInPRB_ACPhaseA; #define PHASE_FACTOR_Q19   2913 = (1<<15)*SPECT_SIN_LEN/360/(1<<5)
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_PAC_OFF(InputFiltered),  "PLL Res Out",     "V",                  10./CPN(22), "PAC", "Resonator Output Signal"),           //   Resonator Output Signal
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_PAC_OFF(SineOut0),       "PLL Exci Signal", "V",                  10./CPN(22), "PAC", "Excitation Signal"),                 //   Excitation Signal
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_PAC_OFF(phase),          "PLL Res Ph",     "deg",         180./(CPN(22)*M_PI), "PAC", "Resonator Phase (no LP filter)"),    //   Resonator Phase (no LP filter)
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_PAC_OFF(PI_Phase_Out),   "PLL Exci Frq",    "Hz", (150000./(CPN(29)*2.*M_PI)), "PAC", "Excitation Freq. (no LP filter)"),   //   Excitation Freq. (no LP filter)
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_PAC_OFF(amp_estimation), "PLL Res Amp",     "V",                  10./CPN(22), "PAC", "Resonator Amp. (no LP filter)"),     //   Resonator Amp. (no LP filter)
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_PAC_OFF(volumeSine),     "PLL Exci Amp",    "V",                  10./CPN(22), "PAC", "Excitation Amp. (no LP filter)"),    //   Excitation Amp. (no LP filter)
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_PAC_OFF(Filter64Out[0]), "PLL Exci Frq LP", "Hz", (150000./(CPN(29)*2.*M_PI)), "PAC", "Excitation Freq. (with LP filter)"), //   Excitation Freq. (with LP filter)
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_PAC_OFF(Filter64Out[1]), "PLL Res Ph LP",  "deg",       (180./(CPN(29)*M_PI)), "PAC", "Resonator Phase (with LP filter)"),  //   Resonator Phase (with LP filter)
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_PAC_OFF(Filter64Out[2]), "PLL Res Amp LP",  "V",              (10./(CPN(29))), "PAC", "Resonator Ampl. (with LP filter)"),  //   Resonator Ampl. (with LP filter)
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_PAC_OFF(Filter64Out[3]), "PLL Exci Amp LP", "V",              (10./(CPN(29))), "PAC", "Excitation Ampl. (with LP filter)"), //   Excitation Ampl. (with LP filter)
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(feedback_mixer.FB_IN_processed[0]), "MIX IN 0", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 0 processed input signal"), // DSP_SIG MIX[0..3]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(feedback_mixer.FB_IN_processed[1]), "MIX IN 1", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 1 processed input signal"), // DSP_SIG MIX[0..3]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(feedback_mixer.FB_IN_processed[2]), "MIX IN 2", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 2 processed input signal"), // DSP_SIG MIX[0..3]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(feedback_mixer.FB_IN_processed[3]), "MIX IN 3", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 3 processed input signal"), // DSP_SIG MIX[0..3]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(feedback_mixer.channel[0]), "MIX OUT 0", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 0 output signal"), // DSP_SIG MIX CH[0..3]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(feedback_mixer.channel[1]), "MIX OUT 1", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 1 output signal"), // DSP_SIG MIX CH[0..3]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(feedback_mixer.channel[2]), "MIX OUT 2", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 2 output signal"), // DSP_SIG MIX CH[0..3]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(feedback_mixer.channel[3]), "MIX OUT 3", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 3 output signal"), // DSP_SIG MIX CH[0..3]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(feedback_mixer.delta), "MIX out delta",      "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Processed Summed Error Signal (Delta) (Z-Servo Input normally)"), // DSP_SIG MIX_delta
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(feedback_mixer.q_factor15),"MIX0 qfac15 LP",  "Q",       (1/(CPN(15))), "Mixer", "Mixer Channel 0 actuall life IIR cutoff watch: q LP fg; f in Hz via: (-log (qf15 / 32767.) / (2.*M_PI/75000.))"), // DSP_SIG qfac15: q LP fg; f in Hz via: (-log (qf15 / 32767.) / (2.*M_PI/75000.))
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.avg_signal),  "signal AVG-256", "V",   DSP32Qs15dot0TO_Volt/256.,                        "RMS", "Averaged signal from Analog AVG module"), // DSP_SIG MIX_0 AVG
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.rms_signal),  "signal RMS-256", "V^2", (DSP32Qs15dot0TO_Volt*DSP32Qs15dot0TO_Volt/256.), "RMS", "RMS signal from Analog AVG module"), // DSP_SIG MIX_0 RMS
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(z_servo.control),    "Z Servo",      "V", DSP32Qs15dot16TO_Volt, "Z_Servo", "Z-Servo output"), // 	DSP_SIG Z_servo
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(z_servo.neg_control),"Z Servo Neg",  "V", DSP32Qs15dot16TO_Volt, "Z_Servo", "-Z-Servo output"), // 	DSP_SIG Z_servo
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(z_servo.watch),      "Z Servo Watch", "B",                  1, "Z_Servo", "Z-Servo status (boolean)"), // 	DSP_SIG Z_servo watch activity
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(m_servo.control),    "M Servo",      "V", DSP32Qs15dot16TO_Volt, "M_Servo", "M-Servo output"), // 	DSP_SIG M_servo
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(m_servo.neg_control),"M Servo Neg",  "V", DSP32Qs15dot16TO_Volt, "M_Servo", "-M-Servo output"), // 	DSP_SIG M_servo
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(m_servo.watch),      "M Servo Watch", "B",                  1, "M_Servo", "M-Servo statuis (boolean)"), // 	DSP_SIG M_servo watch activity
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.Upos), "VP Bias",  "V", DSP32Qs15dot16TO_Volt, "VP", "Bias after VP manipulations"), //	DSP_SIG VP Bias
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.Zpos), "VP Z pos", "V", DSP32Qs15dot16TO_Volt, "VP", "temp Z offset generated by VP program"),   // VP Z manipulation signal (normally subtracted from <Z Servo Neg> signal)")
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(scan.xyz_vec[i_X]), "X Scan",           "V", DSP32Qs15dot16TO_Volt, "Scan", "Scan generator: X-Scan signal"), // 	DSP_SIG vec_XY_scan[0]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(scan.xyz_vec[i_Y]), "Y Scan",           "V", DSP32Qs15dot16TO_Volt, "Scan", "Scan generator: Y-Scan signal"), // 	DSP_SIG vec_XY_scan[1]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(scan.xyz_vec[i_Z]), "Z Scan",           "V", DSP32Qs15dot16TO_Volt, "Scan", "Scan generator: Z-Scan signal (**unused)"), // 	DSP_SIG vec_XY_scan[2]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(scan.xy_r_vec[i_X]), "X Scan Rot",      "V", DSP32Qs15dot16TO_Volt, "Scan", "final X-Scan signal in rotated coordinates"), // 	DSP_SIG vec_XY_scan[0]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(scan.xy_r_vec[i_Y]), "Y Scan Rot",      "V", DSP32Qs15dot16TO_Volt, "Scan", "final Y-Scan signal in rotated coordinates"), // 	DSP_SIG vec_XY_scan[1]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(scan.z_offset_xyslope), "Z Offset from XY Slope",      "V", DSP32Qs15dot16TO_Volt, "Scan", "Scan generator: Z-offset generated by slop compensation calculation (integrative)"), // 	DSP_SIG scan.z_offset_xyslope
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(scan.xyz_gain),     "XYZ Scan Gain",    "X",                     1, "Scan", "XYZ Scan Gains: bitcoded -/8/8/8 (0..255)x -- 10x all: 0x000a0a0a"), // 	DSP_SIG Scan XYZ_gain
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(move.xyz_vec[i_X]), "X Offset",         "V", DSP32Qs15dot16TO_Volt, "Scan", "Offset Move generator: X-Offset signal"), // 	DSP_SIG vec_XYZ_move[0];
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(move.xyz_vec[i_Y]), "Y Offset",         "V", DSP32Qs15dot16TO_Volt, "Scan", "Offset Move generator: Y-Offset signal"), // 	DSP_SIG vec_XYZ_move[1];
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(move.xyz_vec[i_Z]), "Z Offset",         "V", DSP32Qs15dot16TO_Volt, "Scan", "Offset Move generator: Z-Offset signal"), // 	DSP_SIG vec_XYZ_move[2];
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(move.xyz_gain),     "XYZ Offset Gains", "X",                     1, "Scan", "XYZ Offset Gains: bitcoded -/8/8/8 (0..255)x -- not yet used and fixed set to 10x (0x000a0a0a)"), // 	DSP_SIG Offset XYZ_gain
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.wave[0]), "Wave X", "V", DSP32Qs15dot0TO_Volt, "Coarse", "Wave generator: Wave-X (coarse motions)"), // 	DSP_SIG Wave X (coarse wave form out "X");
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.wave[1]), "Wave Y", "V", DSP32Qs15dot0TO_Volt, "Coarse", "Wave generator: Wave-Y (coarse motions)"), // 	DSP_SIG Wave Y (coarse wave form out "Y");
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(autoapp.count_axis[0]), "Count Axis 0", "1",     1, "Coarse", "Coarse Step Counter Axis 0 (X)"), // 	DSP_SIG Count Axis [0]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(autoapp.count_axis[1]), "Count Axis 1", "1",     1, "Coarse", "Coarse Step Counter Axis 1 (Y)"), // 	DSP_SIG Count Axis [1]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(autoapp.count_axis[2]), "Count Axis 2", "1",     1, "Coarse", "Coarse Step Counter Axis 2 (Z)"), // 	DSP_SIG Count Axis [2]
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.bias),        "Bias",        "V", DSP32Qs15dot16TO_Volt, "Control", "DSP Bias Voltage reference following smoothly the Bias Adjuster"),  //   DSP_SIG control_Bias;
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.bias_adjust), "Bias Adjust", "V", DSP32Qs15dot16TO_Volt, "Control", "Bias Adjuster (Bias Voltage) setpoint given by user interface"),  //   DSP_SIG adjusted_Bias;
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.motor),       "Motor",       "V", DSP32Qs15dot16TO_Volt, "Control", "Motor Voltage (auxillary output shared with PLL excitiation if PAC processing is enabled!)"), //	DSP_SIG control_Motor;
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.noise),       "Noise",        "1",                  1, "Control", "White Noise Generator"), // 	DSP_SIG value_NOISE;
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.vnull),       "Null-Signal",  "0",                  1, "Control", "Null Signal used to fix any module input at Zero"), // 	DSP_SIG value_NULL;
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.AC_amp),       "AC Ampl",     "V",  DSP32Qs15dot0TO_Volt, "Control", "AC Amplitude Control for Bias modulation"), // 	DSP_SIG Ac Ampl (Bias);
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.AC_amp_aux),   "AC Ampl Aux", "V",  DSP32Qs15dot0TO_Volt, "Control", "AC Amplitude Control (auxillary channel or as default used for Z modulation)"), // 	DSP_SIG Ac Ampl (Z);
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.noise_amp),    "Noise Ampl",  "V",  DSP32Qs15dot0TO_Volt, "Control", "Noise Amplitiude Control"), // 	DSP_SIG Noise Ampl;
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.state),        "LockIn State",  "X",                 1, "Control", "LockIn Status watch"), // 	DSP_SIG Probe / LockIn State;
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(state.DSP_time), "TIME TICKS", "s", 1./150000., "DSP", "DSP TIME TICKS: real time DSP time based on 150kHz data processing loop for one tick. 32bit free running"), // DSP TIME -- DSP clock
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(feedback_mixer.iir_signal[0]), "IIR32 0", "V", DSP32Qs15dot16TO_Volt, "DBGX_Mixer", "Mixer processed input tap 0 32bit"), // DSP_SIG IIR_MIX[0..3];
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(feedback_mixer.iir_signal[1]), "IIR32 1", "V", DSP32Qs15dot16TO_Volt, "DBGX_Mixer", "Mixer processed input tap 1 32bit"), // DSP_SIG IIR_MIX[0..3];
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(feedback_mixer.iir_signal[2]), "IIR32 2", "V", DSP32Qs15dot16TO_Volt, "DBGX_Mixer", "Mixer processed input tap 2 32bit"), // DSP_SIG IIR_MIX[0..3];
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(feedback_mixer.iir_signal[3]), "IIR32 3", "V", DSP32Qs15dot16TO_Volt, "DBGX_Mixer", "Mixer processed input tap 3 32bit"), // DSP_SIG IIR_MIX[0..3];
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.out[0].s), "Out 0", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 1"), // 	DSP_SIG OUT[0..7]; 
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.out[1].s), "Out 1", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 2"), // 	DSP_SIG OUT[0..7]; 
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.out[2].s), "Out 2", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 3"), // 	DSP_SIG OUT[0..7]; 
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.out[3].s), "Out 3", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 4"), // 	DSP_SIG OUT[0..7]; 
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.out[4].s), "Out 4", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 5"), // 	DSP_SIG OUT[0..7]; 
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.out[5].s), "Out 5", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 6"), // 	DSP_SIG OUT[0..7]; 
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.out[6].s), "Out 6", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 7"), // 	DSP_SIG OUT[0..7]; 
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.out[7].s), "Out 7", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 8"), // 	DSP_SIG OUT[0..7]; 
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.out[8].s), "Wave Out 8", "V", DSP32Qs15dot0TO_Volt, "Analog_OUT", "VIRTUAL OUTPUT 8 (Wave X default)"), // 	DSP_SIG OUT[8]; 
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(analog.out[9].s), "Wave Out 9", "V", DSP32Qs15dot0TO_Volt, "Analog_OUT", "VIRTUAL OUTPUT 9 (Wave Y default)"), // 	DSP_SIG OUT[9]; 
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(state.mode),         "State mode",      "X", 1, "Process_Control", "DSP statmachine status"), // DSP_SIG;
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(move.pflg),          "Move pflag",      "X", 1, "Process_Control", "Offset Move generator process flag"), // DSP_SIG;
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(scan.pflg),          "Scan pflag",      "X", 1, "Process_Control", "Scan generator process flag"), // DSP_SIG;
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.pflg),         "Probe pflag",     "X", 1, "Process_Control", "Vector Probe (VP) process flag"), // DSP_SIG;
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(autoapp.pflg),       "AutoApp pflag",   "X", 1, "Process_Control", "Auto Approach process flag"), // DSP_SIG;
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(CR_generic_io.pflg), "GenericIO pflag", "X", 1, "Process_Control", "Generic IO process flag"), // DSP_SIG;
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(CR_out_pulse.pflg),  "IO Pulse pflag",  "X", 1, "Process_Control", "IO pulse generator process flag"), // DSP_SIG;
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(probe.gpio_data),    "GPIO data",       "X", 1, "Process_Control", "GPIO data-in is read via VP if GPIO READ option is enabled"), // DSP_SIG;
        MAKE_DSP_SIG_ENTRY_VECTOR (10*8, DSP_MEMORY_OFF_P(VP_sec_end_buffer[0]), "VP SecV",  "xV", DSP32Qs15dot16TO_Volt, "VP", "VP section data tranfer buffer vector [10*8 = 10X Sec + 8CH matrix]"), // DSP_SIG
        MAKE_DSP_SIG_ENTRY_VECTOR (32, DSP_MEMORY_OFF_P(user_input_signal_array[0]), "user signal array",  "xV", DSP32Qs15dot16TO_Volt, "Control", "user signal input value array [32]"), // DSP_SIG
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(sco_s[0].out),    "SCO1 Out",      "V", DSP32Qs15dot16TO_Volt, "SCO1", "SCO1 output"), // 	DSP_SIG SCO output
        MAKE_DSP_SIG_ENTRY (DSP_MEMORY_OFF_P(sco_s[1].out),    "SCO2 Out",      "V", DSP32Qs15dot16TO_Volt, "SCO2", "SCO2 output"), // 	DSP_SIG SCO output
	MAKE_DSP_SIG_ENTRY_END ("END OF SIGNALS", "NA")  // END MARKING
};

#endif

