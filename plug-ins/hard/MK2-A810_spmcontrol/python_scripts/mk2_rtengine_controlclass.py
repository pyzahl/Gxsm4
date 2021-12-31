#!/usr/bin/env python

## * Python User Control for
## * Configuration and SPM Approach Control tool for the FB_spm DSP program/GXSM2 Mk3-Pro/A810 based
## * for the Signal Ranger STD/SP2 DSP board
## * 
## * Copyright (C) 2010-13 Percy Zahl
## *
## * Author: Percy Zahl <zahl@users.sf.net>
## * WWW Home: http://sranger.sf.net
## *
## * This program is free software; you can redistribute it and/or modify
## * it under the terms of the GNU General Public License as published by
## * the Free Software Foundation; either version 2 of the License, or
## * (at your option) any later version.
## *
## * This program is distributed in the hope that it will be useful,
## * but WITHOUT ANY WARRANTY; without even the implied warranty of
## * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## * GNU General Public License for more details.
## *
## * You should have received a copy of the GNU General Public License
## * along with this program; if not, write to the Free Software
## * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.

version = "2.1.0"

import os		# use os because python IO is bugy
import time
import fcntl
#import sys

#import GtkExtra
import struct
import array

import math
import re

from numpy import *

import pickle

import datetime

SRANGER_MK23_IOCTL_SET_EXCLUSIVE_AUTO       = 102
SRANGER_MK23_IOCTL_SET_EXCLUSIVE_UNLIMITED  = 103


NUM_MONITOR_SIGNALS = 30
PHASE_FACTOR_Q19 = 2913   # /* (1<<15)*SPECT_SIN_LEN/360/(1<<5) */

#################################################################################################
## insert/include/update dsp_signals.py file here -- may insert code her directly for portability
## update:
## sed -f dspsig2py.sed ../dsp_signals.h > dsp_signals.py
## put converted C structures here -----
##

#/* SRanger and Gxsm - Gnome X Scanning Microscopy Project
#* universal STM/AFM/SARLS/SPALEED/... controlling and
#* data analysis software
#*
#* DSP tools for Linux
#*
#* Copyright (C) 2014 Percy Zahl
#*
#* Authors: Percy Zahl <zahl@users.sf.net>
#* WWW Home:
#* DSP part:  http:##sranger.sf.net
#* Gxsm part: http:##gxsm.sf.net
#*
#* This program is free software; you can redistribute it and/or modify
#* it under the terms of the GNU General Public License as published by
#* the Free Software Foundation; either version 2 of the License", or
#* (at your option) any later version.
#*
#* This program is distributed in the hope that it will be useful",
#* but WITHOUT ANY WARRANTY; without even the implied warranty of
#* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#* GNU General Public License for more details.
#*
#* You should have received a copy of the GNU General Public License
#* along with this program; if not", write to the Free Software
#* Foundation", Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
#*/

#/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */




#/**
# * CAUTION: DO NOT CHANGE FORMAT CONVENTIONS -- MUST MATCH SED SCRIPT TO CONVERT INTO PYTHON
#**/


#/**
# * Signal Input management ID's for safe DSP based signal pointer adjustment methods.
# * And special command ID's for FLASH store/restore/erase (no defaults", only fixed power up fallback):
#**/

DSP_SIGNAL_VECTOR_ELEMENT_INDEX_REQUEST_MSK =    0xffff0000

DSP_SIGNAL_NULL_POINTER_REQUEST_ID =      0xfe01

DSP_SIGNAL_TABLE_STORE_TO_FLASH_ID =      0xf001
DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_ID =  0xf002
DSP_SIGNAL_TABLE_ERASE_FLASH_ID =         0xf0ee
DSP_SIGNAL_TABLE_SEEKRW_FLASH_ID =        0xf0e0
DSP_SIGNAL_TABLE_READ_FLASH_ID =          0xf0e1
DSP_SIGNAL_TABLE_WRITE_FLASH_ID =         0xf0e2
DSP_SIGNAL_TABLE_TEST0_FLASH_ID =         0xf0e7
DSP_SIGNAL_TABLE_TEST1_FLASH_ID =         0xf0e8

#/**
# * Signal Input ID's for safe DSP based signal pointer adjustment methods
#**/

DSP_SIGNAL_MONITOR_INPUT_BASE_ID =   0x0000

DSP_SIGNAL_BASE_BLOCK_A_ID =         0x1000
DSP_SIGNAL_Z_SERVO_INPUT_ID =       (DSP_SIGNAL_BASE_BLOCK_A_ID+1)
DSP_SIGNAL_M_SERVO_INPUT_ID =       (DSP_SIGNAL_Z_SERVO_INPUT_ID+1)
DSP_SIGNAL_MIXER0_INPUT_ID =        (DSP_SIGNAL_M_SERVO_INPUT_ID+1)
DSP_SIGNAL_MIXER1_INPUT_ID =        (DSP_SIGNAL_MIXER0_INPUT_ID+1)
DSP_SIGNAL_MIXER2_INPUT_ID =        (DSP_SIGNAL_MIXER1_INPUT_ID+1)
DSP_SIGNAL_MIXER3_INPUT_ID =        (DSP_SIGNAL_MIXER2_INPUT_ID+1)
DSP_SIGNAL_DIFF_IN0_ID =            (DSP_SIGNAL_MIXER3_INPUT_ID+1)
DSP_SIGNAL_DIFF_IN1_ID =            (DSP_SIGNAL_DIFF_IN0_ID+1)
DSP_SIGNAL_DIFF_IN2_ID =            (DSP_SIGNAL_DIFF_IN1_ID+1)
DSP_SIGNAL_DIFF_IN3_ID =            (DSP_SIGNAL_DIFF_IN2_ID+1)
DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID =   (DSP_SIGNAL_DIFF_IN3_ID+1)
DSP_SIGNAL_SCAN_CHANNEL_MAP1_ID =   (DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID+1)
DSP_SIGNAL_SCAN_CHANNEL_MAP2_ID =   (DSP_SIGNAL_SCAN_CHANNEL_MAP1_ID+1)
DSP_SIGNAL_SCAN_CHANNEL_MAP3_ID =   (DSP_SIGNAL_SCAN_CHANNEL_MAP2_ID+1)

DSP_SIGNAL_BASE_BLOCK_B_ID =         0x2000
DSP_SIGNAL_LOCKIN_A_INPUT_ID =      (DSP_SIGNAL_BASE_BLOCK_B_ID+1)
DSP_SIGNAL_LOCKIN_B_INPUT_ID =      (DSP_SIGNAL_LOCKIN_A_INPUT_ID+1)
DSP_SIGNAL_VECPROBE0_INPUT_ID =     (DSP_SIGNAL_LOCKIN_B_INPUT_ID+1)
DSP_SIGNAL_VECPROBE1_INPUT_ID =     (DSP_SIGNAL_VECPROBE0_INPUT_ID+1)
DSP_SIGNAL_VECPROBE2_INPUT_ID =     (DSP_SIGNAL_VECPROBE1_INPUT_ID+1)
DSP_SIGNAL_VECPROBE3_INPUT_ID =     (DSP_SIGNAL_VECPROBE2_INPUT_ID+1)
DSP_SIGNAL_VECPROBE0_CONTROL_ID =   (DSP_SIGNAL_VECPROBE3_INPUT_ID+1)
DSP_SIGNAL_VECPROBE1_CONTROL_ID =   (DSP_SIGNAL_VECPROBE0_CONTROL_ID+1)
DSP_SIGNAL_VECPROBE2_CONTROL_ID =   (DSP_SIGNAL_VECPROBE1_CONTROL_ID+1)
DSP_SIGNAL_VECPROBE3_CONTROL_ID =   (DSP_SIGNAL_VECPROBE2_CONTROL_ID+1)
DSP_SIGNAL_VECPROBE_TRIGGER_INPUT_ID    = (DSP_SIGNAL_VECPROBE3_CONTROL_ID+1)
DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID    = (DSP_SIGNAL_VECPROBE_TRIGGER_INPUT_ID+1)
DSP_SIGNAL_VECPROBE_LIMITER_UP_INPUT_ID = (DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID+1)
DSP_SIGNAL_VECPROBE_LIMITER_DN_INPUT_ID = (DSP_SIGNAL_VECPROBE_LIMITER_UP_INPUT_ID+1)
DSP_SIGNAL_VECPROBE_TRACKER_INPUT_ID    = (DSP_SIGNAL_VECPROBE_LIMITER_DN_INPUT_ID+1)

DSP_SIGNAL_BASE_BLOCK_C_ID =             0x3000
DSP_SIGNAL_OUTMIX_CH0_INPUT_ID =        (DSP_SIGNAL_BASE_BLOCK_C_ID+1)
DSP_SIGNAL_OUTMIX_CH0_ADD_A_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH0_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH0_SUB_B_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH0_ADD_A_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH0_SMAC_A_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH0_SUB_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH0_SMAC_B_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH0_SMAC_A_INPUT_ID+1)

DSP_SIGNAL_OUTMIX_CH1_INPUT_ID =        (DSP_SIGNAL_OUTMIX_CH0_SMAC_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH1_ADD_A_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH1_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH1_SUB_B_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH1_ADD_A_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH1_SMAC_A_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH1_SUB_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH1_SMAC_B_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH1_SMAC_A_INPUT_ID+1)

DSP_SIGNAL_OUTMIX_CH2_INPUT_ID =        (DSP_SIGNAL_OUTMIX_CH1_SMAC_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH2_ADD_A_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH2_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH2_SUB_B_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH2_ADD_A_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH2_SMAC_A_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH2_SUB_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH2_SMAC_B_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH2_SMAC_A_INPUT_ID+1)

DSP_SIGNAL_OUTMIX_CH3_INPUT_ID =        (DSP_SIGNAL_OUTMIX_CH2_SMAC_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH3_ADD_A_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH3_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH3_SUB_B_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH3_ADD_A_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH3_SMAC_A_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH3_SUB_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH3_SMAC_B_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH3_SMAC_A_INPUT_ID+1)

DSP_SIGNAL_OUTMIX_CH4_INPUT_ID =        (DSP_SIGNAL_OUTMIX_CH3_SMAC_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH4_ADD_A_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH4_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH4_SUB_B_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH4_ADD_A_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH4_SMAC_A_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH4_SUB_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH4_SMAC_B_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH4_SMAC_A_INPUT_ID+1)

DSP_SIGNAL_OUTMIX_CH5_INPUT_ID =        (DSP_SIGNAL_OUTMIX_CH4_SMAC_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH5_ADD_A_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH5_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH5_SUB_B_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH5_ADD_A_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH5_SMAC_A_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH5_SUB_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH5_SMAC_B_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH5_SMAC_A_INPUT_ID+1)

DSP_SIGNAL_OUTMIX_CH6_INPUT_ID =        (DSP_SIGNAL_OUTMIX_CH5_SMAC_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH6_ADD_A_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH6_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH6_SUB_B_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH6_ADD_A_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH6_SMAC_A_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH6_SUB_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH6_SMAC_B_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH6_SMAC_A_INPUT_ID+1)

DSP_SIGNAL_OUTMIX_CH7_INPUT_ID =        (DSP_SIGNAL_OUTMIX_CH6_SMAC_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH7_ADD_A_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH7_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH7_SUB_B_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH7_ADD_A_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH7_SMAC_A_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH7_SUB_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH7_SMAC_B_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH7_SMAC_A_INPUT_ID+1)

DSP_SIGNAL_OUTMIX_CH8_INPUT_ID =        (DSP_SIGNAL_OUTMIX_CH7_SMAC_B_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH8_ADD_A_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH8_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH9_INPUT_ID =        (DSP_SIGNAL_OUTMIX_CH8_ADD_A_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH9_ADD_A_INPUT_ID =  (DSP_SIGNAL_OUTMIX_CH9_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH10_INPUT_ID =       (DSP_SIGNAL_OUTMIX_CH9_ADD_A_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH10_ADD_A_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH10_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH11_INPUT_ID =       (DSP_SIGNAL_OUTMIX_CH10_ADD_A_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH11_ADD_A_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH11_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH12_INPUT_ID =       (DSP_SIGNAL_OUTMIX_CH11_ADD_A_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH12_ADD_A_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH12_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH13_INPUT_ID =       (DSP_SIGNAL_OUTMIX_CH12_ADD_A_INPUT_ID+1)
DSP_SIGNAL_OUTMIX_CH13_ADD_A_INPUT_ID = (DSP_SIGNAL_OUTMIX_CH13_INPUT_ID+1)


DSP_SIGNAL_BASE_BLOCK_D_ID =             0x4000
DSP_SIGNAL_ANALOG_AVG_INPUT_ID =        (DSP_SIGNAL_BASE_BLOCK_D_ID+1)
DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID =     (DSP_SIGNAL_ANALOG_AVG_INPUT_ID+1)
DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID =     (DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID+1)
DSP_SIGNAL_SCO1_INPUT_ID =               (DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID+1)
DSP_SIGNAL_SCO1_AMPLITUDE_INPUT_ID =     (DSP_SIGNAL_SCO1_INPUT_ID+1)
DSP_SIGNAL_SCO2_INPUT_ID =               (DSP_SIGNAL_SCO1_AMPLITUDE_INPUT_ID+1)
DSP_SIGNAL_SCO2_AMPLITUDE_INPUT_ID =     (DSP_SIGNAL_SCO2_INPUT_ID+1)

LAST_INPUT_ID =                          DSP_SIGNAL_SCO2_AMPLITUDE_INPUT_ID

#/**
#* ERROR/WARNING RETURN CODES
#**/

DSP_SIGNAL_ADJUST_NEGID_ERROR =                 -2
DSP_SIGNAL_ADJUST_INVALIDID_ERROR =             -3
DSP_SIGNAL_ADJUST_INVALID_SIGNAL_ERROR =        -5
DSP_SIGNAL_ADJUST_INVALID_SIGNAL_INPUT_ERROR =  -6

DSP_SIGNAL_TABLE_STORE_TO_FLASH_WRITE_ERROR =                    -2
DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_READ_ERROR =                 -3
DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_FLASH_BLOCKID_ERROR =        -10
DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_VERSION_MISMATCH_ERROR =     -11

DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_SIGNAL_LIST_VERSION_MISMATCH =   -12
DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_INPUT_LIST_VERSION_MISMATCH =    -13

DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_FLASH_DATE_WARNING =        -20


#/**
#* Signal pointers and description table for RT signal handling", stored in ext. DRAM on DSP in stripped version
#**/

DSP32Qs15dot16TOV  =    (10.0/(32767.*(1<<16)))
DSP32Qs15dot16TO_Volt = (10.0/(32767.*(1<<16)))
DSP32Qs15dot0TO_Volt  = (10.0/32767.)
DSP32Qs23dot8TO_Volt  = (10.0/(32767.*(1<<8)))

def CPN(N):
        return ((1<<(N))-1.)


NUM_SIGNALS =  110

FB_SPM_FLASHBLOCK_XIDENTIFICATION_A =   0x10aa
FB_SPM_FLASHBLOCK_XIDENTIFICATION_B =   0x0001
FB_SPM_SIGNAL_LIST_VERSION =          0x0005
FB_SPM_SIGNAL_INPUT_VERSION =         0x0003

MAX_INPUT_NUMBER_LIMIT =              0x100  ## just a safey limit for FLASH storage


RP_FPGA_QEXEC = 31 ## Q EXEC READING Controller        -- 1V/(2^RP_FPGA_QEXEC-1)
RP_FPGA_QSQRT = 23 ## Q CORDIC SQRT Amplitude Reading  -- 1V/(2^RP_FPGA_QSQRT-1)
RP_FPGA_QATAN = 21 ## Q CORDIC ATAN Phase Reading      -- 180deg/(PI*(2^RP_FPGA_QATAN-1))
RP_FPGA_QFREQ = 44 ## Q DIFF FREQ READING              -- 125MHz/(2^RP_FPGA_QFREQ-1) well number should not exceed 32bit 















SIGNAL_LOOKUP = [
	[0, 99, 1, "analog.in[0]", "In 0", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT1"], ## 	DSP_SIG IN[0..7] MUST BE FIRST IN THIS LIST!!
	[0, 99, 1, "analog.in[1]", "In 1", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT2"], ## 	DSP_SIG IN[0..7]
	[0, 99, 1, "analog.in[2]", "In 2", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT3"], ## 	DSP_SIG IN[0..7]
	[0, 99, 1, "analog.in[3]", "In 3", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT4"], ## 	DSP_SIG IN[0..7]
	[0, 99, 1, "analog.in[4]", "In 4", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT5"], ## 	DSP_SIG IN[0..7]
	[0, 99, 1, "analog.in[5]", "In 5", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT6"], ## 	DSP_SIG IN[0..7]
	[0, 99, 1, "analog.in[6]", "In 6", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT7"], ## 	DSP_SIG IN[0..7]
	[0, 99, 1, "analog.in[7]", "In 7", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT8"], ## 	DSP_SIG IN[0..7] MUST BE at pos 8 IN THIS LIST!!
	[0, 99, 1, "analog.counter[0]", "Counter 0", "CNT", 1, "Counter", "FPGA based Counter Channel 1"],  ## 	DSP_SIG Counter[0]
	[0, 99, 1, "analog.counter[1]", "Counter 1", "CNT", 1, "Counter", "FPGA based Counter Channel 2"], ## 	DSP_SIG Counter[1]
	[0, 99, 1, "probe.LockIn_0A",     "LockIn A-0",   "*V",   DSP32Qs15dot16TO_Volt, "LockIn", "LockIn A 0 (average over full periods)"], ##  DSP_SIG LockInA[0]
	[0, 99, 1, "probe.LockIn_1stA",   "LockIn A-1st", "*dV",  DSP32Qs15dot16TO_Volt, "LockIn", "LockIn A 1st order"], ##  DSP_SIG LockInA[1]
	[0, 99, 1, "probe.LockIn_2ndA",   "LockIn A-2nd", "*ddV", DSP32Qs15dot16TO_Volt, "LockIn", "LockIn A 2nd oder"],  ##  DSP_SIG LockInA[2]
	[0, 99, 1, "probe.LockIn_0B",     "LockIn B-0",   "*V",   DSP32Qs15dot16TO_Volt, "LockIn", "LockIn B 0 (average over full periods)"], ##  DSP_SIG LockInB[0]
	[0, 99, 1, "probe.LockIn_1stB",   "LockIn B-1st", "*dV",  DSP32Qs15dot16TO_Volt, "LockIn", "LockIn B 1st order"], ##  DSP_SIG LockInB[1]
	[0, 99, 1, "probe.LockIn_2ndB",   "LockIn B-2nd", "*ddV", DSP32Qs15dot16TO_Volt, "LockIn", "LockIn B 2nd order"], ##  DSP_SIG LockInB[2]
	[0, 99, 1, "probe.LockIn_ref",    "LockIn Ref",    "V",   DSP32Qs15dot0TO_Volt,  "LockIn", "LockIn Reference Sinewave (Modulation) (Internal Reference Signal)"], ##  DSP_SIG LockInMod;
	[0, 99, 1, "probe.PRB_ACPhaseA32","LockIn Phase A","deg", 180./(2913*CPN(16)),   "LockIn", "DSP internal LockIn PhaseA32 watch"],    ## DSP_SIG LockInPRB_ACPhaseA; PHASE_FACTOR_Q19   2913 = (1<<15)*SPECT_SIN_LEN/360/(1<<5)
	[0, 99, 1, "InputFiltered",  "PLL Res Out",     "V",                  10./CPN(22), "PAC", "Resonator Output Signal"],           ##   Resonator Output Signal
	[0, 99, 1, "SineOut0",       "PLL Exci Signal", "V",                  10./CPN(22), "PAC", "Excitation Signal"],                 ##   Excitation Signal
	[0, 99, 1, "phase",          "PLL Res Ph",     "deg",         180./(CPN(22)*math.pi), "PAC", "Resonator Phase (no LP filter)"],    ##   Resonator Phase (no LP filter)
	[0, 99, 1, "PI_Phase_Out",   "PLL Exci Frq",    "Hz", (150000./(CPN(29)*2.*math.pi)), "PAC", "Excitation Freq. (no LP filter)"],   ##   Excitation Freq. (no LP filter)
	[0, 99, 1, "amp_estimation", "PLL Res Amp",     "V",                  10./CPN(22), "PAC", "Resonator Amp. (no LP filter)"],     ##   Resonator Amp. (no LP filter)
	[0, 99, 1, "volumeSine",     "PLL Exci Amp",    "V",                  10./CPN(22), "PAC", "Excitation Amp. (no LP filter)"],    ##   Excitation Amp. (no LP filter)
	[0, 99, 1, "Filter64Out[0]", "PLL Exci Frq LP", "Hz", (150000./(CPN(29)*2.*math.pi)), "PAC", "Excitation Freq. (with LP filter)"], ##   Excitation Freq. (with LP filter)
	[0, 99, 1, "Filter64Out[1]", "PLL Res Ph LP",  "deg",       (180./(CPN(29)*math.pi)), "PAC", "Resonator Phase (with LP filter)"],  ##   Resonator Phase (with LP filter)
	[0, 99, 1, "Filter64Out[2]", "PLL Res Amp LP",  "V",              (10./(CPN(29))), "PAC", "Resonator Ampl. (with LP filter)"],  ##   Resonator Ampl. (with LP filter)
	[0, 99, 1, "Filter64Out[3]", "PLL Exci Amp LP", "V",              (10./(CPN(29))), "PAC", "Excitation Ampl. (with LP filter)"], ##   Excitation Ampl. (with LP filter)
	[0, 99, 1, "feedback_mixer.FB_IN_processed[0]", "MIX IN 0", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 0 processed input signal"], ## DSP_SIG MIX[0..3]
	[0, 99, 1, "feedback_mixer.FB_IN_processed[1]", "MIX IN 1", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 1 processed input signal"], ## DSP_SIG MIX[0..3]
	[0, 99, 1, "feedback_mixer.FB_IN_processed[2]", "MIX IN 2", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 2 processed input signal"], ## DSP_SIG MIX[0..3]
	[0, 99, 1, "feedback_mixer.FB_IN_processed[3]", "MIX IN 3", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 3 processed input signal"], ## DSP_SIG MIX[0..3]
	[0, 99, 1, "feedback_mixer.channel[0]", "MIX OUT 0", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 0 output signal"], ## DSP_SIG MIX CH[0..3]
	[0, 99, 1, "feedback_mixer.channel[1]", "MIX OUT 1", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 1 output signal"], ## DSP_SIG MIX CH[0..3]
	[0, 99, 1, "feedback_mixer.channel[2]", "MIX OUT 2", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 2 output signal"], ## DSP_SIG MIX CH[0..3]
	[0, 99, 1, "feedback_mixer.channel[3]", "MIX OUT 3", "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Channel 3 output signal"], ## DSP_SIG MIX CH[0..3]
	[0, 99, 1, "feedback_mixer.delta", "MIX out delta",      "V", DSP32Qs23dot8TO_Volt, "Mixer", "Mixer Processed Summed Error Signal (Delta) (Z-Servo Input normally)"], ## DSP_SIG MIX_delta
	[0, 99, 1, "feedback_mixer.q_factor15","MIX0 qfac15 LP",  "Q",       (1/(CPN(15))), "Mixer", "Mixer Channel 0 actuall life IIR cutoff watch: q LP fg; f in Hz via: (-log (qf15 / 32767.) / (2.*math.pi/75000.))"], ## DSP_SIG qfac15: q LP fg; f in Hz via: (-log (qf15 / 32767.) / (2.*M_PI/75000.))
	[0, 99, 1, "analog.avg_signal",  "signal AVG-256", "V",   DSP32Qs15dot0TO_Volt/256.,                        "RMS", "Averaged signal from Analog AVG module"], ## DSP_SIG MIX_0 AVG
        [0, 99, 1, "analog.rms_signal",  "signal RMS-256", "V^2", (DSP32Qs15dot0TO_Volt*DSP32Qs15dot0TO_Volt/256.), "RMS", "RMS signal from Analog AVG module"], ## DSP_SIG MIX_0 RMS
	[0, 99, 1, "z_servo.control",    "Z Servo",      "V", DSP32Qs15dot16TO_Volt, "Z_Servo", "Z-Servo output"], ## 	DSP_SIG Z_servo
	[0, 99, 1, "z_servo.neg_control","Z Servo Neg",  "V", DSP32Qs15dot16TO_Volt, "Z_Servo", "-Z-Servo output"], ## 	DSP_SIG Z_servo
	[0, 99, 1, "z_servo.watch",      "Z Servo Watch", "B",                  1, "Z_Servo", "Z-Servo status (boolean)"], ## 	DSP_SIG Z_servo watch activity
	[0, 99, 1, "m_servo.control",    "M Servo",      "V", DSP32Qs15dot16TO_Volt, "M_Servo", "M-Servo output"], ## 	DSP_SIG M_servo
	[0, 99, 1, "m_servo.neg_control","M Servo Neg",  "V", DSP32Qs15dot16TO_Volt, "M_Servo", "-M-Servo output"], ## 	DSP_SIG M_servo
	[0, 99, 1, "m_servo.watch",      "M Servo Watch", "B",                  1, "M_Servo", "M-Servo statuis (boolean)"], ## 	DSP_SIG M_servo watch activity
	[0, 99, 1, "probe.Upos", "VP Bias",  "V", DSP32Qs15dot16TO_Volt, "VP", "Bias after VP manipulations"], ##	DSP_SIG VP Bias
	[0, 99, 1, "probe.Zpos", "VP Z pos", "V", DSP32Qs15dot16TO_Volt, "VP", "temp Z offset generated by VP program"],   ## VP Z manipulation signal (normally subtracted from <Z Servo Neg> signal)")
	[0, 99, 1, "scan.xyz_vec[i_X]", "X Scan",           "V", DSP32Qs15dot16TO_Volt, "Scan", "Scan generator: X-Scan signal"], ## 	DSP_SIG vec_XY_scan[0]
	[0, 99, 1, "scan.xyz_vec[i_Y]", "Y Scan",           "V", DSP32Qs15dot16TO_Volt, "Scan", "Scan generator: Y-Scan signal"], ## 	DSP_SIG vec_XY_scan[1]
	[0, 99, 1, "scan.xyz_vec[i_Z]", "Z Scan",           "V", DSP32Qs15dot16TO_Volt, "Scan", "Scan generator: Z-Scan signal (**unused)"], ## 	DSP_SIG vec_XY_scan[2]
	[0, 99, 1, "scan.xy_r_vec[i_X]", "X Scan Rot",      "V", DSP32Qs15dot16TO_Volt, "Scan", "final X-Scan signal in rotated coordinates"], ## 	DSP_SIG vec_XY_scan[0]
        [0, 99, 1, "scan.xy_r_vec[i_Y]", "Y Scan Rot",      "V", DSP32Qs15dot16TO_Volt, "Scan", "final Y-Scan signal in rotated coordinates"], ## 	DSP_SIG vec_XY_scan[1]
        [0, 99, 1, "scan.z_offset_xyslope", "Z Offset from XY Slope",      "V", DSP32Qs15dot16TO_Volt, "Scan", "Scan generator: Z-offset generated by slop compensation calculation (integrative)"], ## 	DSP_SIG scan.z_offset_xyslope
	[0, 99, 1, "scan.xyz_gain",     "XYZ Scan Gain",    "X",                     1, "Scan", "XYZ Scan Gains: bitcoded -/8/8/8 (0..255)x -- 10x all: 0x000a0a0a"], ## 	DSP_SIG Scan XYZ_gain
	[0, 99, 1, "move.xyz_vec[i_X]", "X Offset",         "V", DSP32Qs15dot16TO_Volt, "Scan", "Offset Move generator: X-Offset signal"], ## 	DSP_SIG vec_XYZ_move[0];
	[0, 99, 1, "move.xyz_vec[i_Y]", "Y Offset",         "V", DSP32Qs15dot16TO_Volt, "Scan", "Offset Move generator: Y-Offset signal"], ## 	DSP_SIG vec_XYZ_move[1];
	[0, 99, 1, "move.xyz_vec[i_Z]", "Z Offset",         "V", DSP32Qs15dot16TO_Volt, "Scan", "Offset Move generator: Z-Offset signal"], ## 	DSP_SIG vec_XYZ_move[2];
	[0, 99, 1, "move.xyz_gain",     "XYZ Offset Gains", "X",                     1, "Scan", "XYZ Offset Gains: bitcoded -/8/8/8 (0..255)x -- not yet used and fixed set to 10x (0x000a0a0a)"], ## 	DSP_SIG Offset XYZ_gain
        [0, 99, 1, "analog.wave[0]", "Wave X", "V", DSP32Qs15dot0TO_Volt, "Coarse", "Wave generator: Wave-X (coarse motions)"], ## 	DSP_SIG Wave X (coarse wave form out "X");
        [0, 99, 6, "analog.wavech[0]", "WaveCh", "V", DSP32Qs15dot0TO_Volt, "Coarse", "Wave generator: Wave-Y (coarse motions)"], ## 	DSP_SIG Wave Y (coarse wave form out "Y");
        [0, 99, 1, "autoapp.count_axis[0]", "Count Axis 0", "1",     1, "Coarse", "Coarse Step Counter Axis 0 (X)"], ## 	DSP_SIG Count Axis [0]
        [0, 99, 1, "autoapp.count_axis[1]", "Count Axis 1", "1",     1, "Coarse", "Coarse Step Counter Axis 1 (Y)"], ## 	DSP_SIG Count Axis [1]
        [0, 99, 1, "autoapp.count_axis[2]", "Count Axis 2", "1",     1, "Coarse", "Coarse Step Counter Axis 2 (Z)"], ## 	DSP_SIG Count Axis [2]
	[0, 99, 1, "analog.bias",        "Bias",        "V", DSP32Qs15dot16TO_Volt, "Control", "DSP Bias Voltage reference following smoothly the Bias Adjuster"],  ##   DSP_SIG control_Bias;
	[0, 99, 1, "analog.bias_adjust", "Bias Adjust", "V", DSP32Qs15dot16TO_Volt, "Control", "Bias Adjuster (Bias Voltage) setpoint given by user interface"],  ##   DSP_SIG adjusted_Bias;
	[0, 99, 1, "analog.motor",       "Motor",       "V", DSP32Qs15dot16TO_Volt, "Control", "Motor Voltage (auxillary output shared with PLL excitiation if PAC processing is enabled!)"], ##	DSP_SIG control_Motor;
	[0, 99, 1, "analog.noise",       "Noise",        "1",                  1, "Control", "White Noise Generator"], ## 	DSP_SIG value_NOISE;
	[0, 99, 1, "analog.vnull",       "Null-Signal",  "0",                  1, "Control", "Null Signal used to fix any module input at Zero"], ## 	DSP_SIG value_NULL;
	[0, 99, 1, "probe.AC_amp",       "AC Ampl",     "V",  DSP32Qs15dot0TO_Volt, "Control", "AC Amplitude Control for Bias modulation"], ## 	DSP_SIG Ac Ampl (Bias);
	[0, 99, 1, "probe.AC_amp_aux",   "AC Ampl Aux", "V",  DSP32Qs15dot0TO_Volt, "Control", "AC Amplitude Control (auxillary channel or as default used for Z modulation)"], ## 	DSP_SIG Ac Ampl (Z);
	[0, 99, 1, "probe.noise_amp",    "Noise Ampl",  "V",  DSP32Qs15dot0TO_Volt, "Control", "Noise Amplitiude Control"], ## 	DSP_SIG Noise Ampl;
	[0, 99, 1, "probe.state",        "LockIn State",  "X",                 1, "Control", "LockIn Status watch"], ## 	DSP_SIG Probe / LockIn State;
	[0, 99, 1, "state.DSP_time", "TIME TICKS", "s", 1./150000., "DSP", "DSP TIME TICKS: real time DSP time based on 150kHz data processing loop for one tick. 32bit free running"], ## DSP TIME -- DSP clock
	[0, 99, 1, "feedback_mixer.iir_signal[0]", "IIR32 0", "V", DSP32Qs15dot16TO_Volt, "DBGX_Mixer", "Mixer processed input tap 0 32bit"], ## DSP_SIG IIR_MIX[0..3];
	[0, 99, 1, "feedback_mixer.iir_signal[1]", "IIR32 1", "V", DSP32Qs15dot16TO_Volt, "DBGX_Mixer", "Mixer processed input tap 1 32bit"], ## DSP_SIG IIR_MIX[0..3];
	[0, 99, 1, "feedback_mixer.iir_signal[2]", "IIR32 2", "V", DSP32Qs15dot16TO_Volt, "DBGX_Mixer", "Mixer processed input tap 2 32bit"], ## DSP_SIG IIR_MIX[0..3];
	[0, 99, 1, "feedback_mixer.iir_signal[3]", "IIR32 3", "V", DSP32Qs15dot16TO_Volt, "DBGX_Mixer", "Mixer processed input tap 3 32bit"], ## DSP_SIG IIR_MIX[0..3];
	[0, 99, 1, "analog.out[0].s", "Out 0", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 1"], ## 	DSP_SIG OUT[0..7]; 
	[0, 99, 1, "analog.out[1].s", "Out 1", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 2"], ## 	DSP_SIG OUT[0..7]; 
	[0, 99, 1, "analog.out[2].s", "Out 2", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 3"], ## 	DSP_SIG OUT[0..7]; 
	[0, 99, 1, "analog.out[3].s", "Out 3", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 4"], ## 	DSP_SIG OUT[0..7]; 
	[0, 99, 1, "analog.out[4].s", "Out 4", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 5"], ## 	DSP_SIG OUT[0..7]; 
	[0, 99, 1, "analog.out[5].s", "Out 5", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 6"], ## 	DSP_SIG OUT[0..7]; 
	[0, 99, 1, "analog.out[6].s", "Out 6", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 7"], ## 	DSP_SIG OUT[0..7]; 
	[0, 99, 1, "analog.out[7].s", "Out 7", "V", DSP32Qs15dot16TO_Volt, "Analog_OUT", "DAC OUTPUT 8"], ## 	DSP_SIG OUT[0..7]; 
	[0, 99, 1, "analog.out[8].s", "Wave Out 8", "V", DSP32Qs15dot0TO_Volt, "Analog_OUT", "VIRTUAL OUTPUT 8 (Wave X default)"], ## 	DSP_SIG OUT[8]; 
	[0, 99, 1, "analog.out[9].s", "Wave Out 9", "V", DSP32Qs15dot0TO_Volt, "Analog_OUT", "VIRTUAL OUTPUT 9 (Wave Y default)"], ## 	DSP_SIG OUT[9]; 
	[0, 99, 1, "state.mode",         "State mode",      "X", 1, "Process_Control", "DSP statmachine status"], ## DSP_SIG;
	[0, 99, 1, "move.pflg",          "Move pflag",      "X", 1, "Process_Control", "Offset Move generator process flag"], ## DSP_SIG;
	[0, 99, 1, "scan.pflg",          "Scan pflag",      "X", 1, "Process_Control", "Scan generator process flag"], ## DSP_SIG;
	[0, 99, 1, "probe.pflg",         "Probe pflag",     "X", 1, "Process_Control", "Vector Probe (VP) process flag"], ## DSP_SIG;
	[0, 99, 1, "autoapp.pflg",       "AutoApp pflag",   "X", 1, "Process_Control", "Auto Approach process flag"], ## DSP_SIG;
	[0, 99, 1, "CR_generic_io.pflg", "GenericIO pflag", "X", 1, "Process_Control", "Generic IO process flag"], ## DSP_SIG;
	[0, 99, 1, "CR_out_pulse.pflg",  "IO Pulse pflag",  "X", 1, "Process_Control", "IO pulse generator process flag"], ## DSP_SIG;
	[0, 99, 1, "probe.gpio_data",    "GPIO data",       "X", 1, "Process_Control", "GPIO data-in is read via VP if GPIO READ option is enabled"], ## DSP_SIG;
#	[0, 99, 10*8, "VP_sec_end_buffer[0]", "VP SecV",  "xV", DSP32Qs15dot16TO_Volt, "VP", "VP section data tranfer buffer vector [80 = 10X Sec + 8CH matrix]"], ## DSP_SIG
	[0, 99, 10*8, "VP_sec_end_buffer[0]", "VP SecV",  "V MIX", DSP32Qs23dot8TO_Volt, "VP", "VP section data tranfer buffer vector [80 = 10X Sec + 8CH matrix]"], ## DSP_SIG
#        [0, 99, 32, "user_input_signal_array[0]", "user signal array",  "xV", DSP32Qs15dot16TO_Volt, "Control", "user signal input value array [32]"], ## DSP_SIG
        [0, 99, 32, "user_input_signal_array[0]", "user signal array",  "xV", DSP32Qs23dot8TO_Volt, "Control", "user signal input value array [32]"], ## DSP_SIG
	[0, 99, 1, "sco_s[0].out",    "SCO1 Out",      "V", DSP32Qs15dot16TO_Volt, "SCO", "SCO1 output"], ## 	DSP_SIG SCO output
	[0, 99, 1, "sco_s[1].out",    "SCO2 Out",      "V", DSP32Qs15dot16TO_Volt, "SCO", "SCO2 output"], ## 	DSP_SIG SCO output
	[0, 99, 1, "analog.McBSP_FPGA[0]", "McBSP Phase", "deg", (180.0/(math.pi*((1L<<RP_FPGA_QATAN)-1))), "MCBSP_LINK", "McBSPCH0 DATA Phase"], ## DSP McBSP SPI LINK[0]
	[0, 99, 1, "analog.McBSP_FPGA[1]", "McBSP Freq", "Hz",   (125000000.0/((1L<<RP_FPGA_QFREQ)-1)),  "MCBSP_LINK", "McBSPCH1 DATA Freq"], ## DSP McBSP SPI LINK[1]
	[0, 99, 1, "analog.McBSP_FPGA[2]", "McBSP Ampl", "mV", (1.0/((1L<<RP_FPGA_QSQRT)-1)),            "MCBSP_LINK", "McBSPCH2 DATA Ampl"], ## DSP McBSP SPI LINK[2]
	[0, 99, 1, "analog.McBSP_FPGA[3]", "McBSP Exec", "mV", (1.0/((1L<<RP_FPGA_QEXEC)-1)),            "MCBSP_LINK", "McBSPCH3 DATA Exec"], ## DSP McBSP SPI LINK[3]
	[0, 99, 1, "analog.McBSP_FPGA[4]", "McBSP XAm",  "mV", (1.0/((1L<<RP_FPGA_QSQRT)-1)),            "MCBSP_LINK", "McBSPCH4 DATA Ampl raw"], ## DSP McBSP SPI LINK[4]
	[0, 99, 1, "analog.McBSP_FPGA[5]", "McBSP XPh", "deg", (180.0/(math.pi*((1L<<RP_FPGA_QATAN)-1))),   "MCBSP_LINK", "McBSPCH5 DATA Phase raw"], ## DSP McBSP SPI LINK[5]
	[0, 99, 1, "analog.McBSP_FPGA[6]", "McBSP XFH32", "Hz", ((1L<<32)*125000000.0/((1L<<RP_FPGA_QFREQ)-1)), "MCBSP_LINK", "McBSPCH6 DATA FreqHI Bit 63-32 raw"], ## DSP McBSP SPI LINK[6]
	[0, 99, 1, "analog.McBSP_FPGA[7]", "McBSP XFL32", "Hz", (125000000.0/((1L<<RP_FPGA_QFREQ)-1)), "MCBSP_LINK", "McBSPCH7 DATA FreqLO Bit 31-0 raw"], ## DSP McBSP SPI LINK[7]
        [0, 99, 8, "analog.McBSP_FPGA_VEC", "McBSP VECTOR", "X",  1, "MCBSP_LINK", "McBSP VECTOR"], ## McBSP SPI STATUS VECTOR
	[-1, 0, 0, "no signal", "END OF SIGNALS", "NA", 0]  ## END MARKING
]





#/* END DSP_SIGNALS */

##
## END INSERT file dsp_signals.py
###############################################################

## SIGNAL_LOOKUP_COLUM_DEF
###############################################################

SIG_INDEX = 0
SIG_ADR   = 1
SIG_DIM   = 2
SIG_VAR   = 3
SIG_NAME  = 4
SIG_UNIT  = 5
SIG_D2U   = 6
SIG_GRP   = 7


## MODUL CONFIGURATIONS ##

## DSP_MODULE_INPUT_ID_CATEGORIZED -- COLUMNS ##
MODIN_ID     = 0
MODIN_NAME   = 1
MODIN_SIGADR = 2
MODIN_OPT    = 3
MODIN_COLOR  = 4

DSP_MODULE_INPUT_ID_CATEGORIZED = {
	## "MODULE": [[ MODULE_SIGNAL_INPUT_ID, name, actual hooked signal address ],..]
	"Z_Servo":
	[[DSP_SIGNAL_Z_SERVO_INPUT_ID, "Z_SERVO", 0, 0, "gold" ]],

	"M_Servo":
	[[DSP_SIGNAL_M_SERVO_INPUT_ID, "M_SERVO", 0, 0, "gold" ]],

	"Mixer":
	[[DSP_SIGNAL_MIXER0_INPUT_ID, "MIXER0", 0, 0, "gold" ],
	 [DSP_SIGNAL_MIXER1_INPUT_ID, "MIXER1", 0, 0, "gold" ],
	 [DSP_SIGNAL_MIXER2_INPUT_ID, "MIXER2", 0, 0, "gold" ],
	 [DSP_SIGNAL_MIXER3_INPUT_ID, "MIXER3", 0, 0, "gold" ]],

	"Analog_IN":
	[[DSP_SIGNAL_DIFF_IN0_ID, "DIFF_IN0", 0, 0, "gold" ],
	 [DSP_SIGNAL_DIFF_IN1_ID, "DIFF_IN1", 0, 0, "gold" ],
	 [DSP_SIGNAL_DIFF_IN2_ID, "DIFF_IN2", 0, 0, "gold" ],
	 [DSP_SIGNAL_DIFF_IN3_ID, "DIFF_IN3", 0, 0, "gold" ]],

	"LockIn":
	[[DSP_SIGNAL_LOCKIN_A_INPUT_ID, "LOCKIN_A", 0, 0, "gold" ],
	 [DSP_SIGNAL_LOCKIN_B_INPUT_ID, "LOCKIN_B", 0, 0, "gold" ]],

	"VP":
	[[DSP_SIGNAL_VECPROBE0_INPUT_ID, "VECPROBE0", 0, 0, "gold" ],
	 [DSP_SIGNAL_VECPROBE1_INPUT_ID, "VECPROBE1", 0, 0, "gold" ],
	 [DSP_SIGNAL_VECPROBE2_INPUT_ID, "VECPROBE2", 0, 0, "gold" ],
	 [DSP_SIGNAL_VECPROBE3_INPUT_ID, "VECPROBE3", 0, 0, "gold" ],
	 [DSP_SIGNAL_VECPROBE0_CONTROL_ID, "VECPROBE0_C", 0, 0, "cyan" ],
	 [DSP_SIGNAL_VECPROBE1_CONTROL_ID, "VECPROBE1_C", 0, 0, "cyan" ],
	 [DSP_SIGNAL_VECPROBE2_CONTROL_ID, "VECPROBE2_C", 0, 0, "cyan" ],
	 [DSP_SIGNAL_VECPROBE3_CONTROL_ID, "VECPROBE3_C", 0, 0, "cyan" ],

	 [DSP_SIGNAL_VECPROBE_TRIGGER_INPUT_ID, "VECPROBE_TRIGGER", 0, 0, "cyan" ],

	 [DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID,    "VECPROBE_LIMITER_REF", 0, 0, "cyan" ],
         [DSP_SIGNAL_VECPROBE_LIMITER_UP_INPUT_ID, "VECPROBE_LIMITER_UP",  0, 0, "cyan" ],
         [DSP_SIGNAL_VECPROBE_LIMITER_DN_INPUT_ID, "VECPROBE_LIMITER_DN",  0, 0, "cyan" ],

	 [DSP_SIGNAL_VECPROBE_TRACKER_INPUT_ID, "VECPROBE_TRACKER", 0, 0, "cyan" ]],

	"Scan":
	[[DSP_SIGNAL_SCAN_CHANNEL_MAP0_ID, "SCAN_CHMAP0", 0, 0, "gold" ],
	 [DSP_SIGNAL_SCAN_CHANNEL_MAP1_ID, "SCAN_CHMAP1", 0, 0, "gold" ],
	 [DSP_SIGNAL_SCAN_CHANNEL_MAP2_ID, "SCAN_CHMAP2", 0, 0, "gold" ],
	 [DSP_SIGNAL_SCAN_CHANNEL_MAP3_ID, "SCAN_CHMAP3", 0, 0, "gold" ]],

	"Analog_OUT":
	[[DSP_SIGNAL_OUTMIX_CH0_INPUT_ID, "OUTMIX_CH0", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH0_ADD_A_INPUT_ID, "OUTMIX_CH0_ADD_A", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH0_SUB_B_INPUT_ID, "OUTMIX_CH0_SUB_B", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH0_SMAC_A_INPUT_ID, "OUTMIX_CH0_SMAC_A", 0, 1, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH0_SMAC_B_INPUT_ID, "OUTMIX_CH0_SMAC_B", 0, 1, "gold" ],

	 [DSP_SIGNAL_OUTMIX_CH1_INPUT_ID, "OUTMIX_CH1", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH1_ADD_A_INPUT_ID, "OUTMIX_CH1_ADD_A", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH1_SUB_B_INPUT_ID, "OUTMIX_CH1_SUB_B", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH1_SMAC_A_INPUT_ID, "OUTMIX_CH1_SMAC_A", 0, 1, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH1_SMAC_B_INPUT_ID, "OUTMIX_CH1_SMAC_B", 0, 1, "gold" ],

	 [DSP_SIGNAL_OUTMIX_CH2_INPUT_ID, "OUTMIX_CH2", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH2_ADD_A_INPUT_ID, "OUTMIX_CH2_ADD_A", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH2_SUB_B_INPUT_ID, "OUTMIX_CH2_SUB_B", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH2_SMAC_A_INPUT_ID, "OUTMIX_CH2_SMAC_A", 0, 1, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH2_SMAC_B_INPUT_ID, "OUTMIX_CH2_SMAC_B", 0, 1, "gold" ],

	 [DSP_SIGNAL_OUTMIX_CH3_INPUT_ID, "OUTMIX_CH3", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH3_ADD_A_INPUT_ID, "OUTMIX_CH3_ADD_A", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH3_SUB_B_INPUT_ID, "OUTMIX_CH3_SUB_B", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH3_SMAC_A_INPUT_ID, "OUTMIX_CH3_SMAC_A", 0, 1, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH3_SMAC_B_INPUT_ID, "OUTMIX_CH3_SMAC_B", 0, 1, "gold" ],

	 [DSP_SIGNAL_OUTMIX_CH4_INPUT_ID, "OUTMIX_CH4", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH4_ADD_A_INPUT_ID, "OUTMIX_CH4_ADD_A", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH4_SUB_B_INPUT_ID, "OUTMIX_CH4_SUB_B", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH4_SMAC_A_INPUT_ID, "OUTMIX_CH4_SMAC_A", 0, 1, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH4_SMAC_B_INPUT_ID, "OUTMIX_CH4_SMAC_B", 0, 1, "gold" ],

	 [DSP_SIGNAL_OUTMIX_CH5_INPUT_ID, "OUTMIX_CH5", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH5_ADD_A_INPUT_ID, "OUTMIX_CH5_ADD_A", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH5_SUB_B_INPUT_ID, "OUTMIX_CH5_SUB_B", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH5_SMAC_A_INPUT_ID, "OUTMIX_CH5_SMAC_A", 0, 1, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH5_SMAC_B_INPUT_ID, "OUTMIX_CH5_SMAC_B", 0, 1, "gold" ],
	 
	 [DSP_SIGNAL_OUTMIX_CH6_INPUT_ID, "OUTMIX_CH6", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH6_ADD_A_INPUT_ID, "OUTMIX_CH6_ADD_A", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH6_SUB_B_INPUT_ID, "OUTMIX_CH6_SUB_B", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH6_SMAC_A_INPUT_ID, "OUTMIX_CH6_SMAC_A", 0, 1, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH6_SMAC_B_INPUT_ID, "OUTMIX_CH6_SMAC_B", 0, 1, "gold" ],
	 
	 [DSP_SIGNAL_OUTMIX_CH7_INPUT_ID, "OUTMIX_CH7", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH7_ADD_A_INPUT_ID, "OUTMIX_CH7_ADD_A", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH7_SUB_B_INPUT_ID, "OUTMIX_CH7_SUB_B", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH7_SMAC_A_INPUT_ID, "OUTMIX_CH7_SMAC_A", 0, 1, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH7_SMAC_B_INPUT_ID, "OUTMIX_CH7_SMAC_B", 0, 1, "gold" ],
	 
	 [DSP_SIGNAL_OUTMIX_CH8_INPUT_ID, "OUTMIX_CH8", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH8_ADD_A_INPUT_ID, "OUTMIX_CH8_ADD_A", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH9_INPUT_ID, "OUTMIX_CH9", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH9_ADD_A_INPUT_ID, "OUTMIX_CH9_ADD_A", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH10_INPUT_ID, "OUTMIX_CH10", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH10_ADD_A_INPUT_ID, "OUTMIX_CH10_ADD_A", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH11_INPUT_ID, "OUTMIX_CH11", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH11_ADD_A_INPUT_ID, "OUTMIX_CH11_ADD_A", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH12_INPUT_ID, "OUTMIX_CH12", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH12_ADD_A_INPUT_ID, "OUTMIX_CH12_ADD_A", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH13_INPUT_ID, "OUTMIX_CH13", 0, 0, "gold" ],
	 [DSP_SIGNAL_OUTMIX_CH13_ADD_A_INPUT_ID, "OUTMIX_CH13_ADD_A", 0, 0, "gold" ]],

	"RMS":
	[[DSP_SIGNAL_ANALOG_AVG_INPUT_ID, "ANALOG_AVG_INPUT", 0, 0, "gold" ]],

	"Scope":
	[[DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID, "SCOPE_SIGNAL1_INPUT", 0, 0, "gold" ],
	 [DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID, "SCOPE_SIGNAL2_INPUT", 0, 0, "gold" ]],

	"PAC":[[0xF000, "PLL_INPUT", 0, 0, "purple"]],
        "SCO":
        [[DSP_SIGNAL_SCO1_INPUT_ID, "SCO1_INPUT", 0, 1, "gold" ],
         [DSP_SIGNAL_SCO1_AMPLITUDE_INPUT_ID, "SCO1_AMPLITUDE_INPUT", 0, 0, "gold" ],
         [DSP_SIGNAL_SCO2_INPUT_ID, "SCO2_INPUT", 0, 1, "gold" ],
         [DSP_SIGNAL_SCO2_AMPLITUDE_INPUT_ID, "SCO2_AMPLITUDE_INPUT", 0, 0, "gold" ]],
	"DSP":[],
	"Counter":[],
	"Coarse":[],
	"Control":[],
	"DBGX_Mixer":[],
	"Process_Control":[]
	}

################################################################################
## python mapping (manual) for FB_spmcontrol/dataexchange.h defined
##        C structures "XXX" and struct elements "YYY":
################################################################################
## i_XXX      -- magic index of sub structure to locate structur base address
## fmt_XXX    -- data format mapping of structure elements incl. endianess definition "<" here
## ii_XXX_YYY -- structure XXX element YYY index after unpacking
################################################################################

# magic table index
i_magic = 0
fmt_magic = "<HHHHHHHHHHHHHHHHHHHHHHHHHHHL"

i_magic_version = 1
i_magic_year    = 2
i_magic_date    = 3
i_magic_sid     = 4

i_statemachine = 5
#fmt_statemachine = "<LLLLLLlLLLLLLL"  "<LLLLLLLLLLLLLL LLL[8+1] LLL[16] ..."
####################################  0000   11112222333344445555666677778888   +4x4 9,10,11,12   +8x4                                +8x4
fmt_statemachine = "<HHHHHHLLLLLL"+"HHLL"+"HHLLHHLLHHLLHHLLHHLLHHLLHHLLHHLL"+"HHLLHHLLHHLLHHLL"+"HHLLHHLLHHLLHHLLHHLLHHLLHHLLHHLL"+"HHLLHHLLHHLLHHLLHHLLHHLLHHLLHHLL"+"HHLLHHLLHHLLHHLLHHLLHHLLHHLLHHLL"+"HHLLHHLLHHLLHHLLHHLLHHLLHHLLHHLL"+"L"
fmt_statemachine_w = "<H"
fmt_statemachine_w2 = "<HH"
NUM_RT_TASKS=1+12
NUM_ID_TASKS=32

[
	ii_statemachine_set_mode,  # 16
	ii_statemachine_clr_mode,  # 16 .
	ii_statemachine_mode,      # 16
	ii_statemachine_last_mode, # 16 .
	ii_statemachine_DSP_seconds, # 16
	ii_statemachine_DSP_minutes, # 16 .
	ii_statemachine_DSP_count_seconds,          # 32
	ii_statemachine_DSP_time,       # 32 
	ii_statemachine_DataProcessTime,   # 32
	ii_statemachine_DataProcessReentryTime, # 32
	ii_statemachine_DataProcessReentryPeak, # 32
	ii_statemachine_IdleTime_Peak,     # 32
        ii_statemachine_rt_task_control    # REATTIME-TASKS: STRUCT 12x [16 16 32 32], IDLE-TASKS: STRUCT 32x [16 16 32 32]
] = range (0,13)

adr_statemachine_DSP_seconds = 2
adr_statemachine_DSP_minutes = 2.5
adr_statemachine_DSP_count_second  = 3 
adr_statemachine_DSP_time          = 4
adr_statemachine_rt_task_control = ii_statemachine_rt_task_control-3 # offset in 32bit words

[
        ii_statemachine_rt_task_control_flags,
        ii_statemachine_rt_task_control_missed,
        ii_statemachine_rt_task_control_time,
        ii_statemachine_rt_task_control_time_peak_now, # in 16 bits each in  hi , lo
        ii_statemachine_rt_task_control_len
] = range (0,5)
adr_statemachine_rt_task_control_flags = 0
adr_statemachine_rt_task_control_missed = 0.5
adr_statemachine_rt_task_control_time = 1
adr_statemachine_rt_task_control_time_peak_now = 2 # in 16 bits each in  hi , lo
adr_statemachine_rt_task_control_len = 3 # len in 32 bit words

adr_statemachine_id_task_control = adr_statemachine_rt_task_control+NUM_RT_TASKS*adr_statemachine_rt_task_control_len 
[
        ii_statemachine_id_task_control_flags,
        ii_statemachine_id_task_control_n_exec,
        ii_statemachine_id_task_control_time_next,
        ii_statemachine_id_task_control_interval,
        ii_statemachine_id_task_control_len
] = range (0,5)
adr_statemachine_id_task_control_flags = 0
adr_statemachine_id_task_control_n_exec = 0.5
adr_statemachine_id_task_control_time_next = 1
adr_statemachine_id_task_control_interval = 2
adr_statemachine_id_task_control_len = 3

ii_statemachine_id_task_control=ii_statemachine_rt_task_control+ii_statemachine_rt_task_control_len*NUM_RT_TASKS
ii_statemachine_DP_max_time_until_abort = ii_statemachine_id_task_control + ii_statemachine_id_task_control_len*NUM_ID_TASKS

dsp_rt_process_name = [
        "System", # RT000

        ## RT Tasks
        "QEP, GATEING, COUNTER Expansion", ## RT001
        "Ratemeter",                       ## RT002
        "Adaptive IIR and Mixer",          ## RT003
        "Z-Servo (Feedback)",              ## RT004

        "Vector Scan",                     ## RT005
        "Vector Probe",                    ## RT006
        "LockIn",                          ## RT007
        "Analog OUT Mapping with HR",      ## RT008

        "AutoApp and Wave Generator",      ## RT009
        "M-servo",                         ## RT010
        "Recorder",                        ## RT011
        "--",
]                

dsp_id_process_name = [
        ## Ilde Tasks
        ## ID001
        "BZ push data from RT_FIFO",
        "Offset Move",
        "Vector Probe Manager",
        "",
        
        ## ID005
        "Statemachine Control, exec lnx",
        "Feedback Mixer log update",
        "Signal Management",
        "Offset Move init",

        ## ID009
        "Scan Controls",
        "Probe Controls",
        "Auto Approach Control",
        "GPIO Pulse Control",

        ## ID013
        "GPIO Services",
        "PACPLL Control",
        "DSP Core Management",
        "AIC Control**",

        ##ID0017
        "Signal Monitor Update",
        "",
        "",
        "",
        
        ##ID0021
        "",
        "",
        "",
        "",

        ##ID0025
        "",
        "",
        "",
        "Bias and Zpos Adjusters",
        
        ##ID0029
        "",
        "RMS Signal Processing",
        "Noise (random) Signal Generator",
        "1s Timer",

]


i_AIC_in = 6
fmt_AIC_in = "<hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"

i_AIC_out = 7
fmt_AIC_out = "<hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"

MK2_i_feedback = 9
MK2_fmt_feedback = "<hhhhlhhhhhhlllhhhhllhhlh"
#               0123456789012345678901
#       fmt = "<hhhhlhhhhhhlllhhhhllll"

MK2_i_analog = 10
MK2_fmt_analog = "<hhhhhhhh"  #+0
MK2_fmt_analog_out = "<hhhhhhhh"  #+0
MK2_fmt_analog_in_offset = "<hhhhhhhh" #+8
MK2_fmt_analog_out_offset = "<hhhhhhhh" #+16
MK2_fmt_analog_counter = "<LLLL" #+24

MK2_i_scan = 11
MK2_fmt_scan = "<hhllllhhhhllllhhllllllhhhhlllhhhhhhhhlllllllllhhhhhhhhh"
#               ss rot  npab srcs nxy fs n fmzfrsdbiasGnxyzXYCDiixyIiSsp
#       fmt = "<hh hhhh hhhh llll hh  ll l lll hh hh ll lll ll hhhh hh" [38]
#       fmt = "<hhhhhhhhhhllllhhllllllhhhhlllllllhhhhhh"
MK2_ii_scan_rotm = 2
MK2_ii_scan_xyz = 58
MK2_ii_scan_xyzr = 65

MK2_i_move = 12
MK2_fmt_move = "<hhlllllllllh"

MK2_i_probe = 13
MK2_fmt_probe = "<hhhhhhhhhhllllllllllllLLllhhhhh"
MK2_ii_probe_ACamp = 4
MK2_ii_probe_ACfrq = 5
MK2_ii_probe_ACphaseA = 6
MK2_ii_probe_ACphaseB = 7
MK2_ii_probe_ACnAve = 8
MK2_ii_probe_ACix = 9
MK2_ii_probe_time = 10


MK2_i_feedback_mixer = 18
MK2_fmt_feedback_mixer = "<hhhhhhhhhhhhhhhhhhhhlhhhh"

MK2_i_IO_pulse = 19 #19 CR pulse
MK2_fmt_IO_pulse = "<HHHHHHHHHHHH"

MK2_i_external = 20
MK2_fmt_external = "<hhhhhhhhhhhhhhhl"
MK2_fmt_external_w = "<h"

MK2_i_CR_generic_io = 21
MK2_fmt_CR_generic_io = "<hhhhHHHHLLLLLh"
MK2_fmt_CR_generic_io_write_a5 = "<hhhLLLLL" #+5

MK2_i_watch = 23
MK2_fmt_watch = "<hhhhhhhhhhhhhhhhllllllll"


MK2_i_hr_mask = 24
MK2_fmt_hr_mask = "<hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"



#xxxx---i_probe_vector_head = 0 read address via i_probe
fmt_probe_vector = "<llLLLLLLlllllllll"
[
	ii_probe_n,
	ii_probe_dnx,
	ii_probe_srcs,
	ii_probe_options,
	ii_probe_ptr_fb,
	ii_probe_repetitions,
	ii_probe_i,
	ii_probe_j,
	ii_probe_ptr_next,
	ii_probe_ptr_final,
	ii_probe_f_du,
	ii_probe_f_dx,
	ii_probe_f_dy,
	ii_probe_f_dz,
	ii_probe_f_dx0,
	ii_probe_f_dy0,
	ii_probe_f_dphi
] = range (0,17)

i_wavegen = 14
MK2_i_wavegen = 14
i_waveform = 16 # == probdatafifo
fmt_wavegen =       "<hhLhhLhhhhhhhhhhLlllh"
fmt_wavegen_words = "<HHHHHHHHHHHHHHHHHHHHHHHHHHH"
[
	ii_wavegen_start,
	ii_wavegen_stop,
	ii_wavegen_max_wave_cycles, #U32
	ii_wavegen_wave_length,
	ii_wavegen_n_wave_channels,
	ii_wavegen_wave_speed,      #U32
	ii_wavegen_channel_mapping0,
	ii_wavegen_channel_mapping1,
	ii_wavegen_channel_mapping2,
	ii_wavegen_channel_mapping3,
	ii_wavegen_channel_mapping4,
	ii_wavegen_channel_mapping5,
	ii_wavegen_axis,
	ii_wavegen_dir,
	ii_wavegen_sample_count,
        ii_dumudm,
	ii_wavegen_cycle_count, #U32
	ii_wavegen_count_axis0, #I32
	ii_wavegen_count_axis1, #I32
	ii_wavegen_count_axis2, #I32
	ii_wavegen_pflg
] = range (0,21)


i_datafifo = 15
fmt_datafifo_write = "<ll"
fmt_datafifo_read  = "<llllllL"
# r_pos, w_pos, fill, stall, length, mode, buffer_ul

i_probedatafifo = 16
fmt_probedatafifo_write = "<l"
fmt_probedatafifo_write_setup = "<ll"
fmt_probedatafifo_read  = "<lllllllLLL"
# r_pos, w_pos, cshp, csi, fill, stall, length, bufferbase, pbw, pbl

# i_data_sync_io = 17

num_monitor_signals = NUM_MONITOR_SIGNALS
i_signal_monitor = 18
#######################miAAppppppppppppppppppppssssssssssssssssssssp
fmt_signal_monitor = "<llLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLlllllllllllllllllllllllllllllll"
ii_signal_monitor_p_first = 4
ii_signal_monitor_first = 4 + num_monitor_signals

i_pulse = 19
fmt_pulse = "<lllllHHHHllll"
#	fmt = "<hhhhlllllllh"
[
	ii_pulse_start,
	ii_pulse_stop,
	ii_pulse_duration,
	ii_pulse_period,
	ii_pulse_number,
	ii_pulse_on_bcode,    # short
	ii_pulse_off_bcode,   # short
	ii_pulse_reset_bcode, # short
 	ii_pulse_dummy,       # short
	ii_pulse_io_address,
	ii_pulse_i_per,
	ii_pulse_i_rep,
	ii_pulse_pflg
] = range (0,13)

# i_external = 20

i_generic_io = 21
fmt_generic_io = "<llHHHhLLLLLLLLl"
#	fmt = "<hhhhlllllllh"
[
	ii_generic_io_start,
	ii_generic_io_stop,
	ii_generic_io_gpio_direction_bits,
	ii_generic_io_gpio_data_in,
	ii_generic_io_gpio_data_out,
	ii_generic_io_subsample,
	ii_generic_io_gatetime_l_0,
	ii_generic_io_gatetime_h_0,
	ii_generic_io_gatetime_1,
	ii_generic_io_count_0,
	ii_generic_io_count_1,
	ii_generic_io_peak_count_0,
	ii_generic_io_peak_count_1,
	ii_generic_io_peakholdtime,
	ii_generic_io_pflg
] = range (0,15)



fmt_datalong4 = "<LLLL"

fmt_datalong128 = "<LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"

fmt_datalong512 = "<LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"

fmt_datalong2048 = "<LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"


i_CR_generic_io = 21
fmt_CR_generic_io = "<llhhhhLLLLLLLLl"
fmt_CR_generic_io_write_a5 = "<hhhLLLLL" #+5
#	fmt = "<hhhhHHHHLLLLLh"

# i_a810_config = 22
i_watch = 23

i_hr_mask = 24
fmt_hr_mask = "<llllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll"

## PLL header short map, recorder only
i_recorder = 25
fmt_recorder = "<hhLLLLhhHHllhh"
[
        ii_recorder_start,
        ii_recorder_stop,
	ii_recorder_dec_sum0,
	ii_recorder_dec_sum1,
	ii_recorder_dec_sum2,
	ii_recorder_dec_sum3,
	ii_recorder_deci,
	ii_recorder_pad1,
	ii_recorder_pSignal1,
	ii_recorder_pSignal2,
	ii_recorder_blcklen16,
	ii_recorder_blcklen32,
	ii_recorder_pad2,
	ii_recorder_recorder_pflg,
] = range(0, 14)


i_signal_lookup = 26

# MK2 Magic info
MAGIC_ADR_MK2RTE      = 0x5000
MAGIC_REF_MK2RTE_A810 = 0xEE01
MAGIC_REF_MK2RTE_A16  = 0xEEFF


def dum_null ():
	return 0

class RTEcontrol():

	def __init__(self, start_dev_id=0):
                self.start=1
		self.sr_dev_index = start_dev_id
		self.sr_dev_base  = "/dev/sranger_mk2_"
		self.sr_dev_path  = "/dev/sranger_mk2_0"

		## must be NUM_SIGNALS x L:
		self.fmt_signal_lookup = "<"
		for i in range (0,NUM_SIGNALS):
			self.fmt_signal_lookup = self.fmt_signal_lookup+"L"

		# auto locate Mk2 with RTEngine4GXSM* firmware
		
		self.spm_control_found = 0
                self.analog_channels = 0
		# open SRanger, read magic offsets (look at FB_spm_dataexchange.h -> SPM_MAGIC_DATA_LOCATIONS)
		for i in range(self.sr_dev_index, 8-self.sr_dev_index):
			self.sr_dev_path = self.sr_dev_base+"%d" %i
			print "looking for MK2/A810 or A16 RTE at " + self.sr_dev_path
			try:
				with open (self.sr_dev_path):
					sr = open (self.sr_dev_path, "rw")
					# open SRanger (MK2-NG) check for RTE, read magic

					print ("Seeking to magic address of identification...")
					os.lseek (sr.fileno(), MAGIC_ADR_MK2RTE, 1)
					print ("Reading Magic...")
					self.magic = struct.unpack (fmt_magic, os.read (sr.fileno(), struct.calcsize (fmt_magic)))
					sr.close ()

					print ("Magic ID (blind read):   %08X" %self.magic[0])
					if self.magic[0] == MAGIC_REF_MK2RTE_A810:
						self.spm_control_found = 1
                                                self.analog_channels = 8
						print "Identified MK2/A810-RTE at " + self.sr_dev_path
						sr = open (self.sr_dev_path, "rw")
						os.lseek (sr.fileno(), self.magic[i_recorder], 0)
						self.RECORDER_VARS = struct.unpack (fmt_recorder, os.read (sr.fileno(), struct.calcsize (fmt_recorder)))
						sr.close ()
						break
					if self.magic[0] == MAGIC_REF_MK2RTE_A16:
						self.spm_control_found = 2
                                                self.analog_channels = 16

						print "Identified MK2/A16-RTE at " + self.sr_dev_path
						sr = open (self.sr_dev_path, "rw")
						os.lseek (sr.fileno(), self.magic[i_recorder], 0)
						self.RECORDER_VARS = struct.unpack (fmt_recorder, os.read (sr.fileno(), struct.calcsize (fmt_recorder)))
						sr.close ()
						break

			except IOError:
				print 'Oh dear. No such device.'

			print "continue seeking for MK2/A810|A16-RTE ..."

		if self.spm_control_found > 0:
			print
			print ("*** SR-MK2/A810|A16-RTE  Magic Information ***")
                        if self.spm_control_found == 1:
			        print ("*** SR-MK2/A810 RTE identified. # analog channels: %d"%self.analog_channels)
                        elif self.spm_control_found == 2:
			        print ("*** SR-MK2/A16 RTE identified. # analog channels: %d"%self.analog_channels)
			else:
                                print ("*** SR-MK2 ??? / unkonw magic id.")
			print ("00 Magic:   %08X" %self.magic[0])
			print ("   Version: %08X" %self.magic[1])
			print ("   Year:    %08X" %self.magic[2])
			print ("   MM/DD:   %08X" %self.magic[3])
			print ("   SoftID:  %08X" %self.magic[4])
			print ("05 Statema  %08X" %self.magic[5])
			print ("06 AIC-in   %08X" %self.magic[6])
			print ("07 AIC-out  %08X" %self.magic[7])
			print ("09 feedback %08X" %self.magic[MK2_i_feedback])
			print ("10 analog   %08X" %self.magic[MK2_i_analog])
			print ("11 scan     %08X" %self.magic[MK2_i_scan])
			print ("12 move     %08X" %self.magic[MK2_i_move])
			print ("13 probe    %08X" %self.magic[MK2_i_probe])
			print ("14 wavegen  %08X" %self.magic[MK2_i_wavegen])
			print ("20 external %08X" %self.magic[MK2_i_external])
			print ("   DataFIFO %08X" %self.magic[i_datafifo])
			print ("   PrbFIFO  %08X" %self.magic[i_probedatafifo])
			print ("   SigMonCt %08X" %self.magic[i_signal_monitor])
			print ("   Recorder %08X" %self.magic[i_recorder])
			print
		else:
			print
			print "*** Sorry, no valid MK2 with RTEngine found running on any plugged in MK2 ***"
			self.sr_dev_path  = "none"

	def status (self):
		return self.spm_control_found


	def get_spm_code_version_info (self):
                if self.status():
		        return "V%04X-Y%04x%04X"%(self.magic[1], self.magic[2], self.magic[3])
                else:
                        return "No MK2 with valid RTE application found."

	def read_configurations (self):
                if self.status():
			self.read_signal_lookup()
			# self.read_actual_module_configuration_verbose ()
			self.read_actual_module_configuration ()
			self.get_status ()
		else:
			print "Sorry, no RTE with Signal Management hooked up."

# Signal Management Support
                        
	def read_signal_lookup (self, update_pass=0):
                return
        
                if not self.status():
                        return
                
                if update_pass == 0:
                        print ("*** SR-MK3Pro-A810  FB-SPMCONTROL Reading Signal Lookup List ***")
                sr = open (self.sr_dev_path, "rw")
                os.lseek (sr.fileno(), self.magic[i_signal_lookup], 0)
                self.SIGNAL_LOOKUP_LIST = struct.unpack (self.fmt_signal_lookup, os.read (sr.fileno(), struct.calcsize (self.fmt_signal_lookup)))
                sr.close ()

        #	print SIGNAL_LOOKUP_LIST

                i=0
                j=0
		self.SIGNAL_LOOKUP = SIGNAL_LOOKUP # make copy for self
                for sigptr in self.SIGNAL_LOOKUP_LIST:
                        if sigptr > 0:
                                self.SIGNAL_LOOKUP[i][SIG_INDEX] = i
                                self.SIGNAL_LOOKUP[i][SIG_ADR]   = sigptr
                                if update_pass:
                                        if re.search ("feedback_mixer.FB_IN_processed\[[0-3]\]", self.SIGNAL_LOOKUP[i][SIG_VAR]):
#        					print "checking for MIXER[%d"%j+"] input type on " + self.SIGNAL_LOOKUP[i][SIG_VAR]
                                                [signal, data, offset] = self.query_module_signal_input(DSP_SIGNAL_MIXER0_INPUT_ID+j)
                                                j=j+1
#       					print signal
                                                if re.search("analog.in*", signal[SIG_VAR]):
                                                        self.SIGNAL_LOOKUP[i][SIG_D2U]=DSP32Qs23dot8TO_Volt
#       						print "Analog Signal.: Qs23.8 ** Ufactor adjust: DSP32Qs23dot8TO_Volt"
                                                else:
                                                        self.SIGNAL_LOOKUP[i][SIG_D2U]=DSP32Qs15dot16TO_Volt
#       						print "non analog input: Qs15.16 ** Ufactor adjust: DSP32Qs15dot16TO_Volt"
#       					print "=====> Sig%03d: "%i+"[%08X]"%sigptr, self.SIGNAL_LOOKUP[i]
                                else:
                                        print "Sig%03d: "%i+"[%08X]"%sigptr, self.SIGNAL_LOOKUP[i]
                                i=i+1
                return 1

	def get_module_configuration_list(self):
		return DSP_MODULE_INPUT_ID_CATEGORIZED

        def read_actual_module_configuration_verbose (self):
                print "MK3_SPMDSPC**CURRENT FB_SPMCONTROL MODULE SIGNAL INPUT CONFIGURATION (verbose):"
                print "FB_SPM_SIGNAL_LIST_VERSION  = ", hex(FB_SPM_SIGNAL_LIST_VERSION)
                print "FB_SPM_SIGNAL_INPUT_VERSION = ", hex(FB_SPM_SIGNAL_INPUT_VERSION)
                print "INPUT_ID:                       Signal Name ==> Input Name"
                for mod in DSP_MODULE_INPUT_ID_CATEGORIZED.keys():
                        for mod_inp in DSP_MODULE_INPUT_ID_CATEGORIZED[mod]:
                                [signal, data, offset] = self.query_module_signal_input(mod_inp[0], mod_inp[3])
                                mod_inp[2] = signal [SIG_INDEX];
                                if mod_inp[2] >= 0:
                                        print hex(mod_inp[0]), " : %32s"%signal[SIG_NAME]," ==> ",mod_inp[1], "[",offset,"] (",signal[SIG_VAR],"[",offset,"])"," \t# ", mod_inp, signal, data, ")"
                                else:
                                        if mod_inp[2] == 0 and mod_inp[3] == 1:
                                                print hex(mod_inp[0]), " : ", "DISABLED [p=0]\t ==> ", mod_inp[1],mod_inp[3],"     (", mod_inp, signal, data, ")"
                                        else:
                                                if	data[3] == 0 and data[0] == -1:
                                                        print hex(mod_inp[0]), " : ", "ERROR (can not associate signal adress): [p=0] ==> (", mod_inp, signal, data, ")"
                                                else:
                                                        print hex(mod_inp[0]), " : ", "OTHER ERROR # (can not associate signal adress): [p=0] ==> (", mod_inp, signal, data, ")"

        def read_actual_module_configuration (self):
                print "MK3_SPMDSPC**CURRENT FB_SPMCONTROL MODULE SIGNAL INPUT CONFIGURATION:"
                print "FB_SPM_SIGNAL_LIST_VERSION  = ", hex(FB_SPM_SIGNAL_LIST_VERSION)
                print "FB_SPM_SIGNAL_INPUT_VERSION = ", hex(FB_SPM_SIGNAL_INPUT_VERSION)
                print "INPUT_ID:                       Signal Name ==> Input Name"
                for mod in DSP_MODULE_INPUT_ID_CATEGORIZED.keys():
                        for mod_inp in DSP_MODULE_INPUT_ID_CATEGORIZED[mod]:
                                [signal, data, offset] = self.query_module_signal_input(mod_inp[0], mod_inp[3])
                                mod_inp[2] = signal [SIG_INDEX];
                                if mod_inp[2] >= 0:
                                        print hex(mod_inp[0]), " : %32s"%signal[SIG_NAME]," ==> ",mod_inp[1], "[",offset,"] (",signal[SIG_VAR],"[",offset,"])"
                                else:
                                        if mod_inp[2] == 0 and mod_inp[3] == 1:
                                                print hex(mod_inp[0]), " : ", "DISABLED*", " [p=0] ==> ", mod_inp[1],mod_inp[3]
                                        else:
                                                if	data[3] == 0 and data[0] == -1:
                                                        print hex(mod_inp[0]), " : ", "ERROR # (can not associate signal adress): [p=0] ==> (", mod_inp, signal, data, ")"
                                                else:
                                                        print hex(mod_inp[0]), " : ", "OTHER ERROR # (can not associate signal adress): [p=0] ==> (", mod_inp, signal, data, ")"
                                                        if mod_inp[0] < 0xf000:
                                                                print ("Trying to recover, resetting to In 0 signal.")
			                                        s = self.lookup_signal_by_name ("In 0")
			                                        #self.change_signal_input (0, s, mod_inp[0])
                                                                print ("Retry signal read back.")
                                                                [signal, data, offset] = self.query_module_signal_input(mod_inp[0], mod_inp[3])
                                                                mod_inp[2] = signal [SIG_INDEX];
                                                                if mod_inp[2] >= 0:
                                                                        print hex(mod_inp[0]), " : %32s"%signal[SIG_NAME]," ==> ",mod_inp[1], "[",offset,"] (",signal[SIG_VAR],"[",offset,"])"
                                                                else:
                                                                        if mod_inp[2] == 0 and mod_inp[3] == 1:
                                                                                print hex(mod_inp[0]), " : ", "DISABLED*", " [p=0] ==> ", mod_inp[1],mod_inp[3]
                                                                        else:
                                                                                print hex(mod_inp[0]), " : ", "CAN NO RECOVER -- PERMANENT ERROR, PLEASE TRY POWER CYCLE DSP # (can not associate signal adress): [p=0] ==> (", mod_inp, signal, data, ")"

	# def change_signal_input(self, _signal, _input_id, voffset_func=lambda:0):

#import pickle
#dict = {'one': 1, 'two': 2}
#file = open('dump.txt', 'w')
#pickle.dump(dict, file)
#
#file = open('dump.txt', 'r')
#dict = pickle.load(file)

        def read_and_save_actual_module_configuration (self, __button, file_name):
                print "DSP CONFIG SAVE TO FILE: ", file_name
                print "MK3_SPMDSPC**CURRENT FB_SPMCONTROL MODULE SIGNAL INPUT CONFIGURATION:"
                print "FB_SPM_SIGNAL_LIST_VERSION  = ", hex(FB_SPM_SIGNAL_LIST_VERSION)
                print "FB_SPM_SIGNAL_INPUT_VERSION = ", hex(FB_SPM_SIGNAL_INPUT_VERSION)
                print "INPUT_ID:                       Signal Name ==> Input Name"
                mod_input_config = []
                for mod in DSP_MODULE_INPUT_ID_CATEGORIZED.keys():
                        for mod_inp in DSP_MODULE_INPUT_ID_CATEGORIZED[mod]:
                                [signal, data, offset] = self.query_module_signal_input(mod_inp[0], mod_inp[3])
                                mod_inp[2] = signal [SIG_INDEX];
                                mod_input_config.append ({ 'mod_inp': mod_inp, 'signal': signal, 'data': data, 'offset': offset })
                                if mod_inp[2] >= 0:
                                        s="'"+signal[SIG_NAME]+"'"
                                        print hex(mod_inp[0]), " : %32s"%s," ==> ",mod_inp[1], "[",offset,"] (",signal[SIG_VAR],"[",offset,"])"
                                else:
                                        if mod_inp[2] == 0 and mod_inp[3] == 1:
                                                print hex(mod_inp[0]), " : ", "DISABLED*", " [p=0] ==> ", mod_inp[1],mod_inp[3]
                                        else:
                                                if	data[3] == 0 and data[0] == -1:
                                                        print hex(mod_inp[0]), " : ", "ERROR #  (can not associate signal adress): [p=0] ==> (", mod_inp, signal, data, ")"
                print "MK3_SPMDSPC**CURRENT USER_SIGNAL_ARRAY VALUES:"
                user_array_data = []
                for j in range (0,2):
                        for i in range (0,16):
				ij = 16*j+i
                                [u,uv, sig] = self.read_signal_by_name ("user signal array", ij)
                                user_array_data.append ([ij, uv])
                                print "user_signal_array[",ij,"] = ", u, ", ", uv, signal[SIG_UNIT]

                print "---- SAVING ----"
                config = { 'module_inputs' : mod_input_config, 'user_array_data' : user_array_data }
                print config
                file = open(file_name, 'w')
                pickle.dump (config, file)
                
        def load_and_write_actual_module_configuration (self, __button, file_name):
                print "DSP CONFIG RESTORE FROM FILE: ", file_name
                print "---- LOADING ----"
                file = open(file_name, 'r')
                config = pickle.load (file)
                mod_input_config = config['module_inputs']
                user_array_data  = config['user_array_data']
                print "RESTORING**MODULE_INPUT_CONFIGURATION:"
                # print mod_input_config
                for mod in mod_input_config:
                        print mod
                        if mod['signal'][0] == 0 and mod['signal'][1] == 0 and mod['signal'][2] == 0 and mod['signal'][3] == 'DISABLED':
                                print hex(mod['signal'][0]), " : ", "DISABLED*", " [p=0] ==> ", mod['signal'][1],mod['signal'][3]
                                self.disable_signal_input(0,  mod['signal'], mod['mod_inp'][0], lambda:mod['offset'])
                        else:
                                self.change_signal_input (0,  mod['signal'], mod['mod_inp'][0], lambda:mod['offset'])
                        print ("\n---------\n")
                
                print "RESTORING**USER_SIGNAL_ARRAY VALUES:"
                # print user_array_data
                for ud in user_array_data:
                        print ud
                        self.write_signal_by_name ("user signal array", ud[1], ud[0])
                
	##### basic support

        def magic(self, i):
		return self.magic[i]

        def sr_path(self):
		return self.sr_dev_path

        def readf(self, srfileno,  magic_id, fmt, mode=0, offset=0):
                #print ("READ from magic=",magic_id, fmt)
                ## Take care of stupid middle little endianness..... :( o[
                fmt_extended = ""
                for x in fmt:
                        if x == 'l' or x == 'L':
                                fmt_extended = fmt_extended + "HH"
                        else:
                                fmt_extended = fmt_extended + x
                
		os.lseek (srfileno, self.magic[magic_id]+offset, mode)
                data_x = struct.unpack (fmt_extended, os.read (srfileno, struct.calcsize (fmt_extended)))

                #print ("Read ext:", fmt_extended, data_x)
                
                i=0
                data = ()
                for x in fmt[1::]:
                        #print (i, x)
                        if x == 'l' or x == 'L':
                                #print (data_x[i+1], data_x[i])
                                data = data + struct.unpack("<"+x, struct.pack("<HH", data_x[i+1], data_x[i]))
                                i = i+1
                        else:
                                data = data + (data_x[i],)
                        #print (data)
                        i=i+1
                #print (data)
                return data
        
                #return struct.unpack (fmt, os.read (srfileno, struct.calcsize (fmt)))

        def read(self, magic_id, fmt, mode=0):
		sr = open( self.sr_dev_path, "rb")
		data = self.readf( sr.fileno(), magic_id, fmt, mode)
                sr.close ()
		return data

        def read_o(self, magic_id, offset, fmt, mode=0):
		sr = open( self.sr_dev_path, "rb")
		data = self.readf( sr.fileno(), magic_id, fmt, mode, offset)
		sr.close ()
		return data

        # EXCLUSICE ACCESS CONTROLS via ioctl
        #####################################
        # define SRANGER_MK23_IOCTL_QUERY_EXCLUSIVE_MODE      100
        # define SRANGER_MK23_IOCTL_CLR_EXCLUSIVE_MODE        101
        # define SRANGER_MK23_IOCTL_SET_EXCLUSIVE_AUTO        102
        # define SRANGER_MK23_IOCTL_SET_EXCLUSIVE_UNLIMITED   103

        def query_exclusive_mode (self, fd):
                return struct.unpack ('i', fcntl.ioctl (fd, 100,  # SRANGER_MK23_IOCTL_QUERY_EXCLUSIVE_MODE
                                                        struct.pack('i', 0)))[0]

        def clr_exclusive_mode (self, fd):
                return struct.unpack ('i', fcntl.ioctl (fd, 101,  # SRANGER_MK23_IOCTL_CLR_EXCLUSIVE_MODE
                                                        struct.pack('i', 0)))[0]

        def set_exclusive_auto (self, fd):
                #C  #define SRANGER_MK23_IOCTL_SET_EXCLUSIVE_AUTO        102
                #C  ret =  ioctl (dsp, SRANGER_MK23_IOCTL_SET_EXCLUSIVE_AUTO, (unsigned long)&mode);
                return struct.unpack ('i', fcntl.ioctl (fd, 102,  # SRANGER_MK23_IOCTL_SET_EXCLUSIVE_AUTO
                                                        struct.pack('i', 0)))[0]

        def set_exclusive_unlimited (self, fd):
                return struct.unpack ('i', fcntl.ioctl (fd, 103,  # SRANGER_MK23_IOCTL_SET_EXCLUSIVE_UNLIMITED
                                                        struct.pack('i', 0)))[0]

        def set_exclusive_mode (self, fd, mode):
                if mode == SRANGER_MK23_IOCTL_SET_EXCLUSIVE_AUTO:
                        self.set_exclusive_auto (fd)
                elif mode == SRANGER_MK23_IOCTL_SET_EXCLUSIVE_UNLIMITED:
                        self.set_exclusive_unlimited (fd)

        
        ## packed data example:    struct.pack ("<llLL", _input_id, _signal[SIG_INDEX],0,0)
        def writef(self, srfileno, magic_id, packeddata, mode=0, exclusive=0):
		os.lseek (srfileno, self.magic[magic_id], mode)
                self.set_exclusive_mode (srfileno, exclusive)
		os.write (srfileno, packeddata)

        def write(self, magic_id, packeddata, mode=0):
		sr = open (self.sr_dev_path, "wb")
		os.lseek (sr.fileno(), self.magic[magic_id], mode)
		os.write (sr.fileno(), packeddata)
		sr.close ()

        def write_o(self, magic_id, offset, packeddata, mode=0, exclusive=0): # offset in bytes, must be even!
		sr = open (self.sr_dev_path, "wb")
		os.lseek (sr.fileno(), self.magic[magic_id]+(offset>>1), mode)
                self.set_exclusive_mode (sr, exclusive)
		os.write (sr.fileno(), packeddata)
		sr.close ()

	def write_parameter(self, address,  packeddata, mode=0):
		sr = open (self.sr_dev_path, "wb")
		os.lseek (sr.fileno(), address, mode)
		os.write (sr.fileno(), packeddata)
		sr.close ()

	def read_parameter(self, address, fmt, mode=0):
		sr = open (self.sr_dev_path, "rb")
		os.lseek (sr.fileno(), address, mode)
		data = os.read (sr.fileno(), struct.calcsize (fmt))
		sr.close ()
                return struct.unpack (fmt, data)
                
        def get_status(self):
                if self.status():
		        sr = open (self.sr_dev_path)

		        self.analog = self.readf( sr.fileno(), MK2_i_analog, MK2_fmt_analog)
		        self.scan = self.readf( sr.fileno(), MK2_i_scan, MK2_fmt_scan)
		        self.SPM_STATEMACHINE =  self.readf( sr.fileno(), i_statemachine,  fmt_statemachine)
		        self.SIGNAL_MONITOR =  self.readf( sr.fileno(), i_signal_monitor,  fmt_signal_monitor)
		        self.AIC_in_buffer   =  self.readf( sr.fileno(), i_AIC_in,  fmt_AIC_in)
		        self.AIC_out_buffer  =  self.readf( sr.fileno(), i_AIC_out, fmt_AIC_out)

                        print (self.scan)
                        
		        sr.close ()
		return 1

        def get_rtos_parameter(self, ii_param):
                return self.SPM_STATEMACHINE[ii_param]
        def get_task_control_entry(self, ii_control, ii_entry, pid):
                return self.SPM_STATEMACHINE[ii_control+ii_statemachine_rt_task_control_len*pid+ii_entry]
        def get_task_control_entry_task_time(self, ii_control, pid):
                return self.SPM_STATEMACHINE[ii_control+ii_statemachine_rt_task_control_len*pid+ii_statemachine_rt_task_control_time_peak_now] >> 16
        def get_task_control_entry_peak_time(self, ii_control, pid):
                return self.SPM_STATEMACHINE[ii_control+ii_statemachine_rt_task_control_len*pid+ii_statemachine_rt_task_control_time_peak_now] & 0xffff

        def configure_rt_task(self, pid, flag):
                if flag == "sleep":
                        flags = 0x00
                elif flag == "active":
                        flags = 0x10
                elif flag == "odd":
                        flags = 0x20
                elif flag == "even":
                        flags = 0x40
                else:
                        return
                if pid > 0 and pid < NUM_RT_TASKS:
			self.write_o (i_statemachine, 4*(adr_statemachine_rt_task_control+adr_statemachine_rt_task_control_len*pid+adr_statemachine_rt_task_control_flags), struct.pack ("<H", flags))
                        
        def configure_id_task(self, pid, flag, interval=0):
                if flag == "sleep":
                        flags = 0x00
                elif flag == "always":
                        flags = 0x10
                elif flag == "timer":
                        flags = 0x20
                elif flag == "clock":
                        flags = 0x40
                else:
                        return
                if pid >= 0 and pid < NUM_ID_TASKS:
			self.write_o (i_statemachine, 4*(adr_statemachine_id_task_control+adr_statemachine_id_task_control_len*pid+ii_statemachine_id_task_control_flags), struct.pack ("<H", flags))
                        if interval > 0:
			        self.write_o (i_statemachine, 4*(adr_statemachine_id_task_control+adr_statemachine_id_task_control_len*pid+ii_statemachine_id_task_control_interval), struct.pack ("<L", interval))
                        
        def reset_task_control_peak(self, dum=0):
                for pid in range(0,NUM_RT_TASKS):
			self.write_o (i_statemachine, 4*(adr_statemachine_rt_task_control+adr_statemachine_rt_task_control_len*pid+adr_statemachine_rt_task_control_time_peak_now), struct.pack ("<L", 0))
			self.write_o (i_statemachine, 4*(adr_statemachine_rt_task_control+adr_statemachine_rt_task_control_len*pid+adr_statemachine_rt_task_control_missed), struct.pack ("<H", 0))
		#self.write_o (i_statemachine, 4*(adr_statemachine_rt_task_control + adr_statemachine_rt_task_control_time_peak_now), struct.pack ("<L", 0x0000ffff)) ## RT[0]
		self.write_o (i_statemachine, 4*adr_statemachine_DataProcessReentryPeak, struct.pack ("<L", 0x0000ffff))
		self.write_o (i_statemachine, 4*adr_statemachine_IdleTime_Peak, struct.pack ("<L", 0x0000ffff))
        def reset_task_control_nexec(self, dum=0):
                for pid in range(0,NUM_ID_TASKS):
			self.write_o (i_statemachine, 4*(adr_statemachine_id_task_control+adr_statemachine_rt_task_control_len*pid+adr_statemachine_id_task_control_n_exec), struct.pack ("<H", 0))

        def reset_dsp_clock(self, dum=0):
                ################## TESTS
                new_seconds  = 0 #0x7e000000
                new_minutes  = 0 #33*24*60+13*60+37
                new_dsp_time = 0 # 0x7e000000
		self.write_o (i_statemachine, 4*(adr_statemachine_DSP_seconds), struct.pack ("<HHLL", new_seconds,new_minutes,new_dsp_time,0))
                # need to reset TIMER schedules also, alse they freeze to that time
                for pid in range(0,NUM_ID_TASKS):
			self.write_o (i_statemachine, 4*(adr_statemachine_id_task_control+adr_statemachine_rt_task_control_len*pid+adr_statemachine_id_task_control_time_next), struct.pack ("<L", new_dsp_time))
                
        def set_dsp_rt_DP_max(self, value): ### WARNING keep below ~4000, default is 3500
                if value > 2000 and value <= 4400:
		        self.write_o (i_statemachine, 4*(adr_statemachine_DP_max_time_until_abort), struct.pack ("<L", value))
        def set_dsp_rt_DP_max_default(self, dum=0):
                self.set_dsp_rt_DP_max(3500)
        def set_dsp_rt_DP_max_hi1(self, dum=0):
                self.set_dsp_rt_DP_max(3800)
        def set_dsp_rt_DP_max_hi2(self, dum=0):
                self.set_dsp_rt_DP_max(4000)
        def set_dsp_rt_DP_max_hi3(self, dum=0):
                self.set_dsp_rt_DP_max(4200)
        def set_dsp_rt_DP_max_hi4(self, dum=0):
                self.set_dsp_rt_DP_max(4400)

                        
# VP support
                        
        def dump_VP_fifobuffer (self):
                self.PROBEFIFO = self.read (i_probedatafifo, fmt_probedatafifo_read)
                # fmt_probedatafifo_read  = "<lllllllLLL"
                # r_pos, w_pos, cshp, csi, fill, stall, length, bufferbase, pbw, pbl
                sr = open (self.sr_dev_path, "rb")
                aoff = 0
                num  = self.PROBEFIFO[6]
                print "FIFO at %08x" %(self.PROBEFIFO[7]+aoff) + " len=%d" %num
                os.lseek (sr.fileno(), self.PROBEFIFO[7]+aoff, 0) # non atomic reads for big data!
                if aoff > 0:
                        pd_array = append (pd_array, fromfile(sr, dtype('<i4'), num))
                else:
                        pd_array = fromfile(sr, dtype('<i4'), num)
                sr.close ()
                return pd_array
                
        def dump_VP_vector (self, vector_range):
                self.PROBE = self.read (i_probe, fmt_probe_read)
                vec_size = 0x44
                print "VP VECTOR PROGRAM="
                for index in vector_range:
                        print "[", index, "]", self.read_parameter(self.PROBE[ii_probe_vector_head] + index*vec_size, fmt_probe_vector)
                return 0
                
        def write_VP_vector (self, index, n, ts, nrep, ptr_next, sources, options, d_vector, duration, make_end = 0):
                VP_LIMITER     =  0x300
                VP_LIMITER_UP  =  0x100
                VP_LIMITER_DN  =  0x200
                self.PROBE = self.read (i_probe, fmt_probe_read)

                vec_size = 0x44
                
                q = 214748.3648   # (1<<(16+15)) / (10. * 1000.)    ----- mV to Q15.16
                frq_ref = 75000.
                if ts <= (0.01333e-3 * n):
                        dnx = 0
                else:
                        dnx = round ( fabs (ts*frq_ref/n) )
                steps = n * (dnx+1)
                duration += steps
                if make_end:
                        ptr_final = 0
                else:
                        if (options & VP_LIMITER):
                                ptr_final = ptr_next;
                        else:
                                ptr_final = 1
                                
                if n > 1 and steps > 0.:                
                        self.write_parameter(self.PROBE[ii_probe_vector_head] + index*vec_size,
                                             struct.pack (fmt_probe_vector,
                                                          int (n), int (dnx), sources, options, 0, nrep, nrep, 0, ptr_next, ptr_final,
                                                          int (round (d_vector[0]*q / steps)), # dU 
                                                          int (round (d_vector[1]*q / steps)), # dX
                                                          int (round (d_vector[2]*q / steps)), # dY
                                                          int (round (d_vector[3]*q / steps)), # dZ
                                                          int (round (d_vector[4]*q / steps)), # dX0
                                                          int (round (d_vector[5]*q / steps)), # dY0
                                                          int (round (d_vector[6]*q / steps))  # dPhi
                                                  ))
                else:
                        self.write_parameter(self.PROBE[ii_probe_vector_head] + index*vec_size,
                                             struct.pack (fmt_probe_vector,
                                                          int (n), int (dnx), sources, options, 0, nrep, nrep, 0, ptr_next, ptr_final,
                                                          0, 0,0,0, 0,0, 0
                                                  ))
                
                if steps > 0.:
                        print "0x%08x" %(self.PROBE[ii_probe_vector_head] + index*vec_size) + ": VP Vec[" + str(index) + "]=(" + str([n,  dnx, sources, options, 0, nrep, 0,0, ptr_next, ptr_final,
                               round (d_vector[0]*q / steps), #dU
                               round (d_vector[1]*q / steps), # dX
                               round (d_vector[2]*q / steps), # dY
                               round (d_vector[3]*q / steps), # dZ
                               round (d_vector[4]*q / steps), # dX0
                               round (d_vector[5]*q / steps), # dY0
                               round (d_vector[6]*q / steps)  # dPhi
                       ])+"), duration=" + str(duration) + "ms"
                else:
                        print "0x%08x" %(self.PROBE[ii_probe_vector_head] + index*vec_size) + ": VP Vec[" + str(index) + "]=([NULLVECTOR]), duration=" + str(duration) + "ms"

                return duration

        def write_stop_probe (self):
                # stop any eventual running probe
                self.write(i_probe, struct.pack (fmt_probe_write_process, 0, 1))

        def write_start_probe (self):
                # just start probe -- using what ever vp vectors in place
                self.write(i_probe, struct.pack (fmt_probe_write_process, 1, 0))

        def write_execute_probe (self, bias_i, motor_i, AC_amp, AC_frq, AC_phA, AC_phB, AC_avg_cycels, noise_amp=0., start=1):
                # write initial bias, motor [volt]
                print "Setting initial bias, motor:", bias_i, motor_i
                self.write(MK2_i_analog, struct.pack (MK2_fmt_analog_write_bm, int (round (bias_i/10.*(1<<(16+15)))), int (round (motor_i/10.*(1<<(16+15))))))

		time.sleep(0.2)

                self.PROBE = self.read (i_probe, fmt_probe_read)

                print "PROBE="
                print self.PROBE
                
#                self.PROBE[ii_probe_start]  = start

                # ...                
                # stop any eventual running probe
                self.write(i_probe, struct.pack (fmt_probe_write_process, 0, 1))
		time.sleep(0.1)
                # config probe

                self.write(i_probe, struct.pack (fmt_probe_write,
                                                 start, 0,
                                                 self.PROBE[ii_probe_ACamp], self.PROBE[ii_probe_ACamp_aux], self.PROBE[ii_probe_ACfrq],
                                                 self.PROBE[ii_probe_ACphaseA], self.PROBE[ii_probe_ACphaseB], self.PROBE[ii_probe_ACnAve],
                                                 self.PROBE[ii_probe_noise_amp]))
                # clear FIFO now -- not reading back here, ignoring data for now
                self.write(i_probedatafifo, struct.pack (fmt_probedatafifo_write_setup, 0,0))
                
#	double lvs=1.;
#	// check for signal scale Q32s16.15, or Q32s24.8

#	std::cout << "VECPROBE_LIMITER on     : " << dsp_signal_lookup_managed[query_module_signal_input(DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID)].label << std::endl;
#	std::cout << "VECPROBE_LIMITER unit   : (" << VP_lim_val[0] << ", " << VP_lim_val[1] << ") " << dsp_signal_lookup_managed[query_module_signal_input(DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID)].unit << std::endl;
#	std::cout << "VECPROBE_LIMITER 1/scale: " << (lvs = 1./dsp_signal_lookup_managed[query_module_signal_input(DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID)].scale) << std::endl;
#	std::cout << "VECPROBE_LIMITER up,dn  : (" << (VP_lim_val[0]*lvs) << ", " << (VP_lim_val[1]*lvs) << ")" << std::endl; 
	
#	dsp_probe.LIM_up = (gint32)round (VP_lim_val[0]*lvs); // CONST_DSP_F16*gapp->xsm->Inst->VoltOut2Dig (VP_lim_val[0]));
#	dsp_probe.LIM_dn = (gint32)round (VP_lim_val[1]*lvs); // CONST_DSP_F16*gapp->xsm->Inst->VoltOut2Dig (VP_lim_val[1]));
#	dsp_probe.AC_amp = DVOLT2AIC (AC_amp[0]);
#	dsp_probe.AC_amp_aux = gapp->xsm->Inst->ZA2Dig (AC_amp[1]);
#	dsp_probe.AC_frq = (int) (AC_frq); // DSP selects dicrete AC Freq. out of 75000/(128/N) N=1,2,4,8
#	dsp_probe.AC_phaseA = (int)round(AC_phaseA*16.);
#	dsp_probe.AC_phaseB = (int)round(AC_phaseB*16.);
#	dsp_probe.AC_nAve = AC_lockin_avg_cycels;
#	if (AC_amp[2] >= 0. && AC_amp[2] <= 32. && AC_amp[3] >= -32. && AC_amp[3] <= 32.){
#	        dsp_probe.lockin_shr_corrprod = (int)AC_amp[2];
#		dsp_probe.lockin_shr_corrsum  = (int)AC_amp[3];
#		std::cout << "LockIn SHR Settings: corrprod>>:" << dsp_probe.lockin_shr_corrprod << " corrsum>>:" << dsp_probe.lockin_shr_corrsum << std::endl;
#	} else { // safe defaults
#	        dsp_probe.lockin_shr_corrprod = 14;
#		dsp_probe.lockin_shr_corrsum  = 0;
#		std::cout << "LockIn SHR DEFAULT Settings: corrprod>>:" << dsp_probe.lockin_shr_corrprod << " corrsum>>:" << dsp_probe.lockin_shr_corrsum << std::endl;
#	}

#	// update AC_frq to actual
#	// AIC samples at 150000Hz, full Sintab length is 512, so need increments of
#	// 1,2,4,8 for the following discrete frequencies
#	const int AC_TAB_INC = 4;
#	int AC_tab_inc=0;
#	if (AC_frq >= 8000)
#		AC_tab_inc = AC_TAB_INC<<3;     // LockIn-Ref on 9375.00Hz
#	else if (AC_frq >= 4000)
#		AC_tab_inc = AC_TAB_INC<<2;     // LockIn-Ref on 4687.50Hz
#	else if (AC_frq >= 2000)
#		AC_tab_inc = AC_TAB_INC<<1;     // LockIn-Ref on 2343.75Hz
#	else if (AC_frq >= 1000)
#		AC_tab_inc = AC_TAB_INC;        // LockIn-Ref on 1171.88Hz
#	else if (AC_frq >= 500)
#		AC_tab_inc = AC_TAB_INC>>1;     // LockIn-Ref on  585.94Hz
#	else    AC_tab_inc = AC_TAB_INC>>2;     // LockIn-Ref on  292.97Hz
#	
#	AC_frq = 150000./512.*AC_tab_inc;
#
#	dsp_probe.noise_amp = DVOLT2AIC (noise_amp);


                
	def read_signal_monitor_only(self):
		self.SIGNAL_MONITOR =  self.read(i_signal_monitor,  fmt_signal_monitor)

	def get_ADDA(self):
		return [self.AIC_in_buffer, self.AIC_out_buffer]

	def read_gpio(self,dum=0):
                self.write (i_generic_io, struct.pack ("<ll", 2,0) , mode=1)
		time.sleep(0.2)
		GPIO_control = self.read (i_generic_io, fmt_generic_io)
		print "GENERIC-IO  (GPIO) control record:"
		print GPIO_control
                return GPIO_control[3]

        def configure_gpio(self, GPIO_direction_bits = 0xFFFF):
                self.write (i_generic_io, struct.pack ("<llHHH", 3,0, GPIO_direction_bits, 0,0) , mode=1)

	def write_gpio(self, data = 0x0000, GPIO_direction_bits = 0xFFFF):
                self.write (i_generic_io, struct.pack ("<llHHH", 3,0, 0x000F, 0, 0x03) , mode=1)
		time.sleep(0.2)
                for i in range (0,16):
                        self.write (i_generic_io, struct.pack ("<llHHH", 1,0, 0x0f, 0, i) , mode=1)
		        time.sleep(1)
                        self.write (i_generic_io, struct.pack ("<ll", 2,0) , mode=1)
		        time.sleep(0.2)
		        GPIO_control = self.read (i_generic_io, fmt_generic_io)
		        print i, GPIO_control
                self.write (i_generic_io, struct.pack ("<llHHH", 1,0, 0x000F, 0, 0x03) , mode=1)
		time.sleep(1)
                #self.write (i_generic_io, struct.pack ("<llHHH", 1,0, GPIO_direction_bits, 0, data) , mode=1)
                # self.write (i_generic_io, struct.pack (fmt_generic_io, 0, 0, 255, 288, 32, 32, 0, 0, 0, 0, 0, 0, 22328, 0, 0))

	def clr_dsp_gpio(self, junk, data = 0x00E0):
                print ("CLR DSP GP=%x"%data)
                self.write (i_generic_io, struct.pack ("<llHHH", 10,0, data,0,0) , mode=1)

	def set_dsp_gpio(self, junk, data = 0x00E0):
                print ("SET DSP GP=%x"%data)
                self.write (i_generic_io, struct.pack ("<llHHH", 11,0, data,0,0) , mode=1)

                
        ##### duration, period given in ms
        ##### must configure GPIO direction bits mask before!
        def execute_pulse_cb(self, junk, duration=100, period=200, number=100, on_bcode=3, off_bcode=2, reset_bcode=0, gpio_port=0):
                self.execute_pulse (duration, period, number, on_bcode, off_bcode, reset_bcode, gpio_port)
                
        def execute_pulse(self, duration, period, number=1, on_bcode=3, off_bcode=2, reset_bcode=0, gpio_port=0):
                GPIO_Data_0 = 0x46000006
                ms2dpc = 150.
                print ("GPIO PULSE:")
                print ([duration, period, number, on_bcode, off_bcode, reset_bcode])
                self.write (i_pulse, struct.pack (fmt_pulse, 1,0, int(round(duration*ms2dpc)), int(round(period*ms2dpc)), number, on_bcode, off_bcode, reset_bcode, 0, GPIO_Data_0, 0, 0, 0), mode=1)

##### HR mode control
	def read_hr(self, dum=0):
		HR_MASK = self.read (i_hr_mask, fmt_hr_mask)
		print "HR_MASK is set to:"
		print HR_MASK

	def set_hr_0(self, dum=0):
####### 0                       1                       2                       3                       4                       5                       6                       7
####### 0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  
		self.write (i_hr_mask, struct.pack (fmt_hr_mask, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 \
							    ))
		self.read_hr()

	def set_hr_1(self, dum=0):
####### 0                       1                       2                       3                       4                       5                       6                       7
####### 0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  
		self.write (i_hr_mask, struct.pack (fmt_hr_mask, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1 \
							    ))
		self.read_hr()


	def set_hr_1slow(self,dum=0):
####### 0                       1                       2                       3                       4                       5                       6                       7
####### 0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  
		self.write (i_hr_mask, struct.pack (fmt_hr_mask, \
        0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0 \
								    ))
		self.read_hr()

	def set_hr_1slow2(self,dum=0):
####### 0                       1                       2                       3                       4                       5                       6                       7
####### 0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  
		self.write (i_hr_mask, struct.pack (fmt_hr_mask, \
        0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0 \
								    ))
		self.read_hr()

	def set_hr_s2(self,dum=0):
####### 0                       1                       2                       3                       4                       5                       6                       7
####### 0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  
		self.write (i_hr_mask, struct.pack (fmt_hr_mask, \
       -1, -2,  0,  1,  2, -1,  0,  1,    0, -2,  1, -1, -1,  2,  0,  2,   -2,  1,  0, -1,  2,  0,  2,  0,   1, -1,  2,  0,  2,  0, -2,  1,    2,  0, -2,  1,  1, -1,  2,  1,    1,  2, -1,  2,  0, -2,  1,  2,    0, -2,  2,  2,  1,  2, -1,  2,   1, -2,  2,  2,  1,  2, -1,  2 \
							    ))
		self.read_hr()


            
        def print_statemachine_status(self, dummy=0):
                print "DSP Statemachine struct="
                print self.SPM_STATEMACHINE
                return 1
        
                
        ##### SIGNAL SUPPORT

	def get_signal_lookup_list (self):
		return self.SIGNAL_LOOKUP

	def get_signal (self, id):
		return self.SIGNAL_LOOKUP[i]

        def lookup_signal_by_ptr (self, sigptr, nullok=0):
                if sigptr == 0 and nullok:
                        return [[0,0,0,"DISABLED","DISABLED","0",0], 0]

                for signal in self.SIGNAL_LOOKUP:
			offset = (sigptr-signal[SIG_ADR])/4
			# if vector, offset is >=0 and < dimension (signal[SIG_DIM])
                        if sigptr >= signal[SIG_ADR] and sigptr < (signal[SIG_ADR]+4*signal[SIG_DIM]):
                                return [signal, offset]

                return [[-1,0,0,"0x%x" %sigptr,"N/A","0",0],0]

        def lookup_signal_by_name (self, signame):
                for signal in self.SIGNAL_LOOKUP:
                        if signal[SIG_NAME] == signame:
                                return signal
                return [-1,0,0,"n/a","N/A","0",0]

        def read_signal_by_name (self, signame, i=0):
                for signal in self.SIGNAL_LOOKUP:
                        if signal[SIG_NAME] == signame:
                                [x]=self.read_parameter(signal[SIG_ADR]+4*i, "<l")
                                return [x, x*signal[SIG_D2U], signal]
                return [0, [-1,0,0, signame, "???","0",0]]

        def write_signal_by_name (self, signame, x, i=0):
                for signal in self.SIGNAL_LOOKUP:
                        if signal[SIG_NAME] == signame:
                                self.write_parameter(signal[SIG_ADR]+4*i, struct.pack("<l", int(round(x/signal[SIG_D2U]))))
                                return 0
                return -1
        
	# for sanity never direct change/write DSP pointers, but go via signal input pointer change request via signal lookup and signal id
	def change_signal_input(self, _signal, _input_id, voffset_func=lambda:0):
		voffset = voffset_func ()
		fmt = "<llLL"
		sr = open (self.sr_dev_path, "wb")
		os.lseek (sr.fileno(), self.magic[i_signal_monitor], 0)
                set_exclusive_auto (sr.fileno())
		if voffset > 0 and voffset < _signal[SIG_DIM]:
			os.write (sr.fileno(), struct.pack (fmt, _input_id, _signal[SIG_INDEX]+(voffset<<16),0,0))
		else:
			os.write (sr.fileno(), struct.pack (fmt, _input_id, _signal[SIG_INDEX],0,0))
			voffset=0
                clr_exclusive_auto (sr.fileno())
		sr.close ()
		print "Signal Input ID (mindex) %d"%_input_id+" change request to signal id %d"%_signal[SIG_INDEX]+" Voff:[%d]"%voffset+"\n ==> "+str(_signal)
		# check on updates (units mixer)
		return 1

	# MONITOR SIGNALS
	def get_monitor_data (self, tap):
		return  self.SIGNAL_MONITOR[ii_signal_monitor_first+tap]
	
	def get_monitor_data_med (self, tap):              
                return median ([
                        self.SIGNAL_MONITOR_HIST[0][ii_signal_monitor_first+tap],
                        self.SIGNAL_MONITOR_HIST[1][ii_signal_monitor_first+tap],
                        self.SIGNAL_MONITOR_HIST[2][ii_signal_monitor_first+tap],
                        self.SIGNAL_MONITOR_HIST[3][ii_signal_monitor_first+tap],
                        self.SIGNAL_MONITOR_HIST[4][ii_signal_monitor_first+tap],
                        self.SIGNAL_MONITOR_HIST[5][ii_signal_monitor_first+tap],
                        self.SIGNAL_MONITOR_HIST[6][ii_signal_monitor_first+tap],
                        self.SIGNAL_MONITOR_HIST[7][ii_signal_monitor_first+tap]
                ])
	
	def get_monitor_pointer (self, tap):
		return  self.SIGNAL_MONITOR[ii_signal_monitor_p_first+tap]
	
	def get_monitor_signal (self, tap):
		sigptr = self.SIGNAL_MONITOR[ii_signal_monitor_p_first+tap]
		value  = self.SIGNAL_MONITOR[ii_signal_monitor_first+tap]
		if sigptr == 0:
			return [value, value,[-1,0,0,"debug","N/A","0",0]]
		else:
			[signal,off] = self.lookup_signal_by_ptr (sigptr)
	
		return [value, value*signal[SIG_D2U], signal]

        def get_monitor_signal_median (self, tap):
		sigptr = self.SIGNAL_MONITOR[ii_signal_monitor_p_first+tap]
		value  = self.get_monitor_data_med (tap)
		if sigptr == 0:
			return [value, value,[-1,0,0,"debug","N/A","0",0]]
		else:
			[signal,off] = self.lookup_signal_by_ptr (sigptr)
	
		return [value, value*signal[SIG_D2U], signal]

        
        # for sanity never direct change/write DSP pointers, but go via signal input pointer change request via signal lookup and signal id
        def change_signal_input(self, dum, _signal, _input_id, voffset_func=lambda:0):
		voffset = voffset_func ()
		if voffset > 0 and voffset < _signal[SIG_DIM]:
			self.write (i_signal_monitor, struct.pack ("<llLL", _input_id, _signal[SIG_INDEX]+(voffset<<16),0,0))
		else:
			self.write (i_signal_monitor, struct.pack ("<llLL", _input_id, _signal[SIG_INDEX],0,0))
			voffset=0
                print "Signal Input-ID (mindex) %d"%_input_id+" change request to signal id %d"%_signal[SIG_INDEX]+" Voff:[%d]"%voffset+"\n ==> "+str(_signal)
                return 1

        ## DEBUG ONLY ## direct address manipulation
        def change_signal_monitor_debug_pointer(self, dum, _signal, _input_id, dsp_variable_address_func=lambda:0x10f05240):
                dsp_variable_address = dsp_variable_address_func ()
                dbg_index = _input_id-22
                print "Signal Monitor Debug Address requested for MON[%d] "%_input_id + ":= p0x%x"%dsp_variable_address
                if dbg_index >= 0 and dbg_index < 8:
                        self.write_o (i_signal_monitor, 4*(ii_signal_monitor_p_first+22+dbg_index), struct.pack ("<L", dsp_variable_address))

                print "Reading back Signal Lookups."
                self.read_configurations()       
                return 1
        
        
        # _signal_id ==  DSP_SIGNAL_TABLE_STORE_TO_FLASH_ID, DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_ID, DSP_SIGNAL_TABLE_ERASE_FLASH_ID, ... 
        def manage_signal_configuration(self, _button, _signal_id, data=dum_null):
		fmt = "<llLL" ## [ mindex, signal_id, act_address_input_set, act_address_signal [][] ]
		print "SIGNAL MONITOR: MANAGE SIGNAL CONFIGURATION REQUEST mi=0x%08X si=0x%08X data vector=[ "%(int (data()), _signal_id), fmt, int (data ()), _signal_id, 0,0, " ]"
		self.write (i_signal_monitor, struct.pack (fmt, int (data ()), _signal_id, 0,0))
		time.sleep(0.2)
		d = self.read(i_signal_monitor, fmt)
		print "Return = [ %x, %x, %x, %x ]"%(d[0], d[1], d[2], d[3])

	def flash_dump(self, _button, nb8=32):
		fmt = "<llLL" ## [ mindex, signal_id, act_address_input_set, act_address_signal [][] ]
		print "SPM Flash Buffer Dump:"
		self.write (i_signal_monitor, struct.pack (fmt, 0, DSP_SIGNAL_TABLE_SEEKRW_FLASH_ID, 0,0))
		time.sleep(0.1)
		ret = "GXSM SIGNAL FLASH CONFIG HEADER:\n"
		for i in range(0, nb8):
			s = "[0x%04x] = "%(i*8)
			for k in range(0,8):
				self.write (i_signal_monitor, struct.pack (fmt, 0, DSP_SIGNAL_TABLE_READ_FLASH_ID, 0,0))
				d = self.read(i_signal_monitor, fmt)
				s = s + " 0x%04X"%d[1]
			print s
			ret = ret + s + "\n"
		return ret

        def disable_signal_input(self, _signal, _input_id, voffset_func=lambda:0): # _input_id == 0 reverts to power up defaults!!
		fmt = "<llLL"
		self.write (i_signal_monitor, struct.pack (fmt, _input_id, DSP_SIGNAL_NULL_POINTER_REQUEST_ID,0,0))
		print "Signal Input Disabled (mindex) %d set to NULL-POINTER."%_input_id
		time.sleep(0.01)
		print "Return:"
		print self.read (i_signal_monitor, fmt)
		print "Verify:"
		[s,d,o] = self.query_module_signal_input (_input_id, 1)
		print s,d,o
		return 1

        def disable_signal_input(self, dum, _signal, _input_id, voffset_func=lambda:0): # _input_id == 0 reverts to power up defaults!!
		fmt = "<llLL"
		self.write (i_signal_monitor, struct.pack (fmt, _input_id, DSP_SIGNAL_NULL_POINTER_REQUEST_ID,0,0))
		print "Signal Input Disabled (mindex) %d set to NULL-POINTER."%_input_id
		time.sleep(0.01)
		print "Return:"
		print self.read (i_signal_monitor, fmt)
		print "Verify:"
		[s,d,o] = self.query_module_signal_input (_input_id, 1)
		print s,d,o
		return 1
                
        def query_module_signal_input(self, _input_id, nullok=0):
                fmt = "<llLL"
		sr = open (self.sr_dev_path, "wb+")
                fd = sr.fileno ()
                
                self.writef (fd, i_signal_monitor, struct.pack (fmt, _input_id, -1,0,0), 0, SRANGER_MK23_IOCTL_SET_EXCLUSIVE_AUTO)
                #       print "Query Signal Input ID (mindex) %d:"%_input_id
                
                time.sleep(0.01)

                data = self.readf (fd, i_signal_monitor, fmt, 0)

                sr.close ()
                [signal,offset] = self.lookup_signal_by_ptr (data[3], nullok)
                #       print signal
                return signal, data, offset
      
	def adjust_statemachine_flag(self, mask, flag):
		if flag:
			self.write_o (i_statemachine, 0, struct.pack (fmt_statemachine_w, mask))
		else:
			self.write_o (i_statemachine, 4, struct.pack (fmt_statemachine_w, mask))

                
        # Experimental, use only on your risc -- 688 MHz (all older MK3 models are 600 MHz spec., only newer revision are 700 MHz approved)
        def dsp_adjust_speed(self, dummy, speed=590):
                clk=self.read_o (i_statemachine, ii_statemachine_DSP_speed_act*4, fmt_statemachine_w)
                print "\nDSP clock is " + str(clk[0]) + " MHz"
                if speed == 688:
                        print "\nRequesting 688 MHz. \n\n *** WARNING WARNING:   OVERCLOCKING ON YOUR OWN RISC *** \n\n"
			self.write_o (i_statemachine, ii_statemachine_DSP_speed_req*4, struct.pack (fmt_statemachine_w, 688),1)
                else:
                        print "\nRequesting default 590 MHz.\n"
			self.write_o (i_statemachine, ii_statemachine_DSP_speed_req*4, struct.pack (fmt_statemachine_w, 590),1)

		time.sleep(0.5)
                clk=self.read_o (i_statemachine, ii_statemachine_DSP_speed_act*4, fmt_statemachine_w)
                print "\nDSP clock is " + str(clk[0]) + " MHz"

                        
	###define SRANGER_MK23_SEEK_ATOMIC      1 // Bit indicates ATOMIC mode to be used
	###define SRANGER_MK23_SEEK_DATA_SPACE  0 // DATA_SPACE, NON ATOMIC -- MK2: ONLY VIA USER PROVIDED NON ATOMIC KFUNC!!!!
	###define SRANGER_MK23_SEEK_PROG_SPACE  2 // default mode
	###define SRANGER_MK23_SEEK_IO_SPACE    4 // default mode

    
        
        def write_wave_setup (self, start, stop, wavelen, channels, wavespeed, cycles):
                # start wave generator
                tmp =  struct.unpack("<hhHHhhHH", struct.pack("<hhLhhL", start, stop, cycles, wavelen, channels, wavespeed))
                self.write(i_wavegen, struct.pack("<hhHHhhHH",tmp[0], tmp[1], tmp[3], tmp[2], tmp[4], tmp[5], tmp[7], tmp[6] ))
                
        def write_wave(self, wave, channels, wavespeed, cycles):
                # send wave
		sr = open (self.sr_dev_path, "wb")
                print("WaveAddress: 0x%X"%self.magic[i_waveform] )
                print("WaveLen: %d"%wave.size)
		os.lseek (sr.fileno(), self.magic[i_waveform], 0) # non atomic reads for big data!
		wave.tofile(sr)
		sr.close ()
                self.write_wave_setup (1,0, wave.size, channels, wavespeed, cycles)
                
        
	def set_recorder(self, n, trigger=0, trigger_level=0):
                #print "set_recorder:",  n, trigger, trigger_level
		self.write_o(i_recorder, ii_recorder_stop, struct.pack ("<ll", int(round(trigger_level)), int(trigger)))
		self.write_parameter(self.RECORDER_VARS[ii_blcklen], struct.pack ("<l", n-1), 1)

		sr = open (self.sr_dev_path, "wb")
	        os.lseek (sr.fileno(), self.magic[i_recorder], 0)
		os.write (sr.fileno(), struct.pack ("<lll", 0, int(round(trigger_level)), int(trigger)))
                sr.close ()
	
		sr = open (self.sr_dev_path, "rw")
		os.lseek (sr.fileno(), self.magic[i_recorder], 0)
		tmp = struct.unpack (fmt_recorder, os.read (sr.fileno(), struct.calcsize (fmt_recorder)))
                #print "verify:", tmp

                sr.close ()
                
                
	def check_recorder_ready(self):
		sr = open (self.sr_dev_path, "rb")
		os.lseek (sr.fileno(), self.RECORDER_VARS[ii_blcklen], 1)
		fmt = "<ll"
		[blck, dum] = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
		sr.close ()
		return blck

	##    void read_pll_array32 (gint64 address, int n, gint32 *arr){
	##	gint64 nleft = n * sizeof (gint32);
	##	int i_block_start = 0;
	##	#define BL 32768
	##	for (gint64 a = address; nleft > 0; a += BL/2){
	##	    lseek (dsp, a, SRANGER_MK23_SEEK_DATA_SPACE);
	##	    sr_read (dsp, &arr[i_block_start], (size_t)(nleft > BL ? BL : nleft)); 
	##	    i_block_start += BL/4;
	##	    nleft -= BL;
	##	}
	##	    ...
	##    }

	## still some issue with block size >= 32768
	def read_recorder(self, n):
		aS1 = self.RECORDER_VARS[ii_Signal1]
		aS2 = self.RECORDER_VARS[ii_Signal2]
		num_left = n*4  # n x sizeof (gint32)
		BL = 32768
		aoff = 0

		while num_left > 0:
			if num_left > BL:
				num=BL
			else:
				num=num_left

			sr = open (self.sr_dev_path, "rb")
			os.lseek (sr.fileno(), aS1+aoff, 0) # non atomic reads for big data!
			if aoff > 0:
				xarray = append (xarray, fromfile(sr, dtype('<i4'), num/4))
			else:
				xarray = fromfile(sr, dtype('<i4'), num/4)
			sr.close ()

			sr = open (self.sr_dev_path, "rb")
			os.lseek (sr.fileno(), aS2+aoff, 0)
			if aoff > 0:
				yarray = append(yarray, fromfile(sr, dtype('<i4'), num/4))
			else:
				yarray = fromfile(sr, dtype('<i4'), num/4)
			sr.close ()

			aoff = aoff+BL
			num_left = num_left-BL 

		return [xarray.astype(float)[::-1], yarray.astype(float)[::-1]]


        # must call first with init=True
        def read_recorder_deci(self, n=4096, recorder_file="", init=False):
                if n < 0 or n > 0x1000:
                        return 0
                if init:
                        self.max_age = 60 # 60s
                        self.time_of_last_stamp = 0
                        self.ring_buffer_position_last = -1
                        self.ring_buffer_size = 0x1000
                        
                aS1=0x10000 # SDRAM DeciRecorder Buffer base address (word) for buffer[0] 
                
		sr = open (self.sr_dev_path, "rb")
		os.lseek (sr.fileno(), self.magic[i_recorder], 0)
		self.RECORDER_VARS = struct.unpack (fmt_recorder, os.read (sr.fileno(), struct.calcsize (fmt_recorder)))
		sr.close ()
                deci = self.RECORDER_VARS[ii_recorder_deci]
                if deci > 4096 or deci < 0:
                        print ("Error Deci Index: " + str(deci))
                        print (self.RECORDER_VARS)
                        return
                        
                start = deci-n
                
                if start > 0:
                        sr = open (self.sr_dev_path, "rb")
		        os.lseek (sr.fileno(), aS1+((start)<<1), 0) # non atomic reads for big data!
		        tmparray = fromfile(sr, dtype('<i4'), n)
		        sr.close ()
                else:
                        n1 = -start
                        n2 = n-n1
                        sr = open (self.sr_dev_path, "rb")
		        os.lseek (sr.fileno(), aS1+((self.ring_buffer_size-n1)<<1), 0) # non atomic reads for big data!
		        tmparray1 = fromfile(sr, dtype('<i4'), n1)
		        os.lseek (sr.fileno(), aS1+((n2)<<1), 0) # non atomic reads for big data!
		        tmparray2 = fromfile(sr, dtype('<i4'), n2)
		        sr.close ()
                        tmparray = concatenate((tmparray1, tmparray2), axis=0)

                if recorder_file != "" and self.ring_buffer_position_last>=0:
                        with open(recorder_file, "a") as recorder:
                                now=time.mktime(datetime.datetime.now().timetuple())
                                if self.time_of_last_stamp+self.max_age < now:
                                        self.time_of_last_stamp = now
                                        recorder.write("#T " + str(self.time_of_last_stamp )+ "\n")

                                recorder.write("# "+str(self.ring_buffer_position_last) + " ... " + str(deci) + "\n")
                                if self.ring_buffer_position_last < deci:
                                        for i in range(n-(deci-self.ring_buffer_position_last), n):
                                                recorder.write(str(tmparray[i]) + "\n")
                                else:
                                        for i in range(n-(deci+self.ring_buffer_size-self.ring_buffer_position_last), n):
                                                recorder.write(str(tmparray[i]) + " L\n")
                self.ring_buffer_position_last = deci
                return tmparray.astype(float)


        

# END
