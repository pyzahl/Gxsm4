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

#################################################
####### NOTE BACKPORT FROM MK3 to MK2 IN PROGRESS
#################################################

version = "2.1.0"

import os		# use os because python IO is bugy
import time
import fcntl

#import GtkExtra
import struct
import array

import math
import re

from numpy import *


################################################################################
## python mapping (manual) for FB_spmcontrol/dataexchange.h defined
##        C structures "XXX" and struct elements "YYY":
################################################################################
## i_XXX      -- magic index of sub structure to locate structur base address
## fmt_XXX    -- data format mapping of structure elements incl. endianess definition "<" here
## ii_XXX_YYY -- structure XXX element YYY index after unpacking
################################################################################

## 00003fd3    _sigma_delta_hr_mask

### MK2

MK2_i_magic = 0
MK2_fmt_magic = "<HHHHHHHHHHHHHHHHHHHHHHHHHH"

MK2_i_magic_version = 1
MK2_i_magic_year    = 2
MK2_i_magic_date    = 3
MK2_i_magic_sid     = 4


MK2_i_statemachine = 5
MK2_fmt_statemachine = "<HHHHHHLLLLLL"
MK2_fmt_statemachine_w = "<H"
[
	MK2_ii_statemachine_set_mode,
	MK2_ii_statemachine_clr_mode,
	MK2_ii_statemachine_mode,
	MK2_ii_statemachine_last_mode,
	MK2_ii_statemachine_DSP_seconds,
	MK2_ii_statemachine_DSP_minutes,
	MK2_ii_statemachine_DSP_count_seconds,
	MK2_ii_statemachine_DSP_time,
	MK2_ii_statemachine_DataProcessTime,
	MK2_ii_statemachine_DataProcessReentryTime,
	MK2_ii_statemachine_DataProcessReentryPeak,
	MK2_ii_statemachine_IdleTime_Peak,
	MK2_ii_statemachine_DP_tasks, # ... DSPMK2_DP_TASK_CONTROL dp_task_control[NUM_DATA_PROCESSING_TASKS+1];
                                      # ... DSPMK2_ID_TASK_CONTROL id_task_control[NUM_IDLE_TASKS];
        # DP_max_time_until_abort
] = range (0,13)


MK2_i_AIC_in = 6
MK2_fmt_AIC_in = "<hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"

MK2_i_AIC_out = 7
MK2_fmt_AIC_out = "<hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"

MK2_i_feedback = 9
MK2_fmt_feedback = "<hhhhlhhhhhhlllhhhhllhhlh"
#               0123456789012345678901
#       fmt = "<hhhhlhhhhhhlllhhhhllll"

MK2_i_analog = 10
MK2_fmt_analog_out = "<hhhhhhhh"  #+0
MK2_fmt_analog_in_offset = "<hhhhhhhh" #+8
MK2_fmt_analog_out_offset = "<hhhhhhhh" #+16
MK2_fmt_analog_counter = "<LLLL" #+24

MK2_i_scan = 11
MK2_fmt_scan = "<hhllllhhhhllllhhllllllhhhhlllhhlllllllhhhhhhhhh"
#               ss rot  npab srcs nxy fs n fm dxny Zo fmz  XYZ
#       fmt = "<hh hhhh hhhh llll hh  ll l lll hh hh ll lll ll hhhh hh" [38]
#       fmt = "<hhhhhhhhhhllllhhllllllhhhhlllllllhhhhhh"
MK2_ii_scan_rotm = 2
MK2_ii_scan_xyz = 30
MK2_ii_scan_xyzr = 33

MK2_i_move = 12
MK2_fmt_move = "<hhlllllllllh"

MK2_i_probe = 13
MK2_fmt_probe = "hhhhhhhhllllllll"
MK2_ii_probe_ACamp = 2
MK2_ii_probe_ACfrq = 3
MK2_ii_probe_ACphaseA = 4
MK2_ii_probe_ACphaseB = 5
MK2_ii_probe_ACnAve = 6
MK2_ii_probe_ACix = 7
MK2_ii_probe_time = 8


MK2_i_feedback_mixer = 18
MK2_fmt_feedback_mixer = "<hhhhhhhhhhhhhhhhhhhhlhhhh"

MK2_i_IO_pulse = 19 #19 CR pulse
MK2_fmt_IO_pulse = "<HHHHHHHHHHHH"

MK2_i_external = 20
MK2_fmt_external = "<hhhhh"
MK2_fmt_external_w = "<h"

MK2_i_CR_generic_io = 21
MK2_fmt_CR_generic_io = "<hhhhHHHHLLLLLh"
MK2_fmt_CR_generic_io_write_a5 = "<hhhLLLLL" #+5

MK2_i_watch = 23
MK2_fmt_watch = "<hhhhhhhhhhhhhhhhllllllll"


MK2_i_hr_mask = 24
MK2_fmt_hr_mask = "<hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"


## Signal Recorder MK2 -- can recored 16 or 32bit as requested 
MK2_SDRAM_BUFFER = 0x8000  ## MK2 SDRAM START: word-adr 0x08000
MK2_i_recorder = 25
MK2_fmt_recorder = "<LLLLhh"
[
	MK2_ii_pSignal1,
	MK2_ii_pSignal2,
	MK2_ii_blcklen16,
	MK2_ii_blcklen32,
	MK2_ii_pad,
	MK2_ii_pflg
] = range(0, 6)


DSP32Qs15dot16TOV  =    (10.0/(32767.*(1<<16)))
DSP32Qs15dot16TO_Volt = (10.0/(32767.*(1<<16)))
DSP32Qs15dot0TO_Volt  = (10.0/32767.)
DSP32Qs23dot8TO_Volt  = (10.0/(32767.*(1<<8)))

MK2_NUM_SIGNALS = 30

MK2_SIGNAL_LOOKUP = [
	[0, 99, 1, "analog.in[0]", "In 0", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT1"], ## 	DSP_SIG IN[0..7] MUST BE FIRST IN THIS LIST!!
	[1, 99, 1, "analog.in[1]", "In 1", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT2"], ## 	DSP_SIG IN[0..7]
	[2, 99, 1, "analog.in[2]", "In 2", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT3"], ## 	DSP_SIG IN[0..7]
	[3, 99, 1, "analog.in[3]", "In 3", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT4"], ## 	DSP_SIG IN[0..7]
	[4, 99, 1, "analog.in[4]", "In 4", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT5"], ## 	DSP_SIG IN[0..7]
	[5, 99, 1, "analog.in[5]", "In 5", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT6"], ## 	DSP_SIG IN[0..7]
	[6, 99, 1, "analog.in[6]", "In 6", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT7"], ## 	DSP_SIG IN[0..7]
	[7, 99, 1, "analog.in[7]", "In 7", "V", DSP32Qs15dot16TO_Volt, "Analog_IN", "ADC INPUT8"], ## 	DSP_SIG IN[0..7] MUST BE at pos 8 IN THIS LIST!!
	[8, 99, 1, "analog.counter[0]", "Counter 0", "CNT", 1, "Counter", "FPGA based Counter Channel 1"],  ## 	DSP_SIG Counter[0]
	[9, 99, 1, "analog.counter[1]", "Counter 1", "CNT", 1, "Counter", "FPGA based Counter Channel 2"], ## 	DSP_SIG Counter[1]
	[10, 99, 1, "probe.LockIn_0A",     "LockIn A-0",   "*V",   DSP32Qs15dot16TO_Volt, "LockIn", "LockIn A 0 (average over full periods)"], ##  DSP_SIG LockInA[0]
	[11, 99, 1, "probe.LockIn_1stA",   "LockIn A-1st", "*dV",  DSP32Qs15dot16TO_Volt, "LockIn", "LockIn A 1st order"], ##  DSP_SIG LockInA[1]
	[12, 99, 1, "probe.LockIn_2ndA",   "LockIn A-2nd", "*ddV", DSP32Qs15dot16TO_Volt, "LockIn", "LockIn A 2nd oder"],  ##  DSP_SIG LockInA[2]
	[13, 99, 1, "probe.LockIn_0B",     "LockIn B-0",   "*V",   DSP32Qs15dot16TO_Volt, "LockIn", "LockIn B 0 (average over full periods)"], ##  DSP_SIG LockInB[0]
	[14, 99, 1, "probe.LockIn_1stB",   "LockIn B-1st", "*dV",  DSP32Qs15dot16TO_Volt, "LockIn", "LockIn B 1st order"], ##  DSP_SIG LockInB[1]
	[15, 99, 1, "probe.LockIn_2ndB",   "LockIn B-2nd", "*ddV", DSP32Qs15dot16TO_Volt, "LockIn", "LockIn B 2nd order"], ##  DSP_SIG LockInB[2]
	[16, 99, 1, "probe.LockIn_ref",    "LockIn Ref",    "V",   DSP32Qs15dot0TO_Volt,  "LockIn", "LockIn Reference Sinewave (Modulation) (Internal Reference Signal)"], ##  DSP_SIG LockInMod;
	[17, 99, 1, "probe.PRB_ACPhaseA32","LockIn Phase A","deg", 180./(2913*((1<<(16))-1.)),   "LockIn", "DSP internal LockIn PhaseA32 watch"],    ## DSP_SIG LockInPRB_ACPhaseA; PHASE_FACTOR_Q19   2913 = (1<<15)*SPECT_SIN_LEN/360/(1<<5)
	[-1, 0, 0, "no signal", "END OF SIGNALS", "NA", 0]  ## END MARKING
]

MK2_DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID = 100
MK2_DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID = 101

MK2_DSP_MODULE_INPUT_ID_CATEGORIZED = {
	## "MODULE": [[ MODULE_SIGNAL_INPUT_ID, name, actual hooked signal address ],..]
	"Scope":
	[[MK2_DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID, "SCOPE_SIGNAL1_INPUT", 0, 0, "gold" ],
	 [MK2_DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID, "SCOPE_SIGNAL2_INPUT", 0, 0, "gold" ]],
	"Process_Control":[]
	}


#    void set_blcklen (int len){
#         lseek (dsp, magic_data.recorder, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
#         dsp_recorder.blcklen32 = long_2_sranger_long (len);
# //      dsp_recorder.blcklen32 = swap_32 (len);
#         dsp_recorder.pflg = int_2_sranger_int (2);
#         sr_write (dsp, &dsp_recorder, sizeof (dsp_recorder));
#     }

#            case 8: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.feedback + 4); break; // cb_Ic
#             case 9: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.feedback + 24); break; // z32neg
#             case 10: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.feedback + 14); break; // I_iir
#             case 11: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.feedback + 20); break; // I_avg
#             case 12: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.feedback + 22); break; // I_rms
#             case 13: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.feedback_mixer + 20); break; // delta
#             case 14: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.scan + 48); break; // xyz_vec[0]
#             case 15: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.scan + 50); break; // xyz_vec[1]
#             case 16: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.scan + 52); break; // xyz_vec[2]
#             case 17: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.scan + 54); break; // xy_r_vec[0]
#             case 18: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.scan + 56); break; // xy_r_vec[1]
#             case 19: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_in + 0); break; // IN_0
#             case 20: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_out + 0); break; // OUT_0
#             case 21: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_in + 2); break; // IN_0
#             case 22: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_out + 2); break; // OUT_0
#             case 23: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_in + 4); break; // IN_0
#             case 24: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_out + 4); break; // OUT_0
#             case 25: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_in + 6); break; // IN_0
#             case 26: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_out + 6); break; // OUT_0
#             case 27: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_in + 8); break; // IN_0
#             case 28: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_out + 8); break; // OUT_0
#             case 29: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_in + 10); break; // IN_0
#             case 30: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_out + 10); break; // OUT_0
#             case 31: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_in + 12); break; // IN_0
#             case 32: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_out + 12); break; // OUT_0
#             case 33: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_in + 14); break; // IN_0
#             case 34: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.AIC_out + 14); break; // OUT_0
#             default: dsp_recorder.pSignal1 = long_2_sranger_long (magic_data.feedback + 14); break; // I_iir


#        dsp_recorder.blcklen16 = int_2_sranger_int (-1);
#         dsp_recorder.blcklen32 = int_2_sranger_int (-1);
#         dsp_recorder.pad  = int_2_sranger_int (0);
#         dsp_recorder.pflg = int_2_sranger_int (0);
# //
#     int read_pll_signal1 (PAC_control &pll, int n, double scale=1., gint flag=FALSE){
#         lseek (dsp, magic_data.recorder, SRANGER_MK23_SEEK_DATA_SPACE | SRANGER_MK23_SEEK_ATOMIC);
#         sr_read (dsp, &dsp_recorder, sizeof (dsp_recorder));
#         pll.blcklen = long_2_sranger_long (dsp_recorder.blcklen32);

# //      g_print ("MK2rec: pS1=%8x pS2=%8x n16=%d n32=%d p=%d\n", 
# //               long_2_sranger_long (dsp_recorder.pSignal1),
# //               long_2_sranger_long (dsp_recorder.pSignal2),
# //               long_2_sranger_long (dsp_recorder.blcklen16),
# //               long_2_sranger_long (dsp_recorder.blcklen32),
# //               int_2_sranger_int (dsp_recorder.pflg)
# //          );

#         if (pll.blcklen == -1){
#             if (n > 0 && n <= PAC_BLCKLEN){
#                 read_pll_array32 (MK2_SDRAM_BUFFER, n<<1, pll.tmp_array);
#                 for (int i=0; i<n; ++i)
#                     pll.Signal1[i] = (double)pll.tmp_array[i<<1] * scale;
#             }
#             if (flag)
#                 set_blcklen (n);
#             return 0;
#         }
#         return -1;
#     }
    
#     // must preceed a read of signal 1.
#     int read_pll_signal2 (PAC_control &pll, int n, double scale=1., gint flag=FALSE){ 
#         for (int i=0; i<n; ++i)
#             pll.Signal2[i] = (double)pll.tmp_array[(i<<1)+1] * scale;

#         return 0; 
#     };

# #define MK2_SDRAM_BUFFER 0x8000  // MK2 SDRAM START: word-adr 0x08000








## MK3 ##########################################################################################
##
##

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
DSP_SIGNAL_VECPROBE_TRIGGER_INPUT_ID = (DSP_SIGNAL_VECPROBE3_CONTROL_ID+1)
DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID = (DSP_SIGNAL_VECPROBE_TRIGGER_INPUT_ID+1)
DSP_SIGNAL_VECPROBE_TRACKER_INPUT_ID = (DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID+1)

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

DSP_SIGNAL_BASE_BLOCK_D_ID =             0x4000
DSP_SIGNAL_ANALOG_AVG_INPUT_ID =        (DSP_SIGNAL_BASE_BLOCK_D_ID+1)
DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID =     (DSP_SIGNAL_ANALOG_AVG_INPUT_ID+1)
DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID =     (DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID+1)

LAST_INPUT_ID =                          DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID

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


MK3_NUM_SIGNALS =  98

FB_SPM_FLASHBLOCK_XIDENTIFICATION_A =   0x10aa
FB_SPM_FLASHBLOCK_XIDENTIFICATION_B =   0x0001
FB_SPM_SIGNAL_LIST_VERSION =          0x0005
FB_SPM_SIGNAL_INPUT_VERSION =         0x0003

MAX_INPUT_NUMBER_LIMIT =              0x100  ## just a safey limit for FLASH storage
















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
        [0, 99, 1, "analog.wave[1]", "Wave Y", "V", DSP32Qs15dot0TO_Volt, "Coarse", "Wave generator: Wave-Y (coarse motions)"], ## 	DSP_SIG Wave Y (coarse wave form out "Y");
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
	[0, 99, 64, "VP_sec_end_buffer[0]", "VP SecV",  "xV", DSP32Qs15dot16TO_Volt, "VP", "VP section data tranfer buffer vector [64 = 8X Sec + 8CH matrix]"], ## DSP_SIG
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
	 [DSP_SIGNAL_VECPROBE_LIMITER_INPUT_ID, "VECPROBE_LIMITER", 0, 0, "cyan" ],
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
	 [DSP_SIGNAL_OUTMIX_CH9_ADD_A_INPUT_ID, "OUTMIX_CH9_ADD_A", 0, 0, "gold" ]],
	"RMS":
	[[DSP_SIGNAL_ANALOG_AVG_INPUT_ID, "ANALOG_AVG_INPUT", 0, 0, "gold" ]],
	"Scope":
	[[DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID, "SCOPE_SIGNAL1_INPUT", 0, 0, "gold" ],
	 [DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID, "SCOPE_SIGNAL2_INPUT", 0, 0, "gold" ]],
	"PAC":[[0xF000, "PLL_INPUT", 0, 0, "purple"]],
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
MK3_i_magic = 0
MK3_fmt_magic = "<LLLLLLLLLLLLLLLLLLLLLLLLLLLL"

i_magic_version = 1
i_magic_year    = 2
i_magic_date    = 3
i_magic_sid     = 4

i_statemachine = 5
fmt_statemachine = "<LLLLLLLLLLLLL"
fmt_statemachine_w = "<L"
fmt_statemachine_w2 = "<LL"
[
	ii_statemachine_set_mode,
	ii_statemachine_clr_mode,
	ii_statemachine_mode,
	ii_statemachine_last_mode,
	ii_statemachine_BLK_count,
	ii_statemachine_BLK_Ncount,
	ii_statemachine_DSP_time,
	ii_statemachine_DSP_tens,
	ii_statemachine_DataProcessTime,
	ii_statemachine_IdleTime,
	ii_statemachine_DataProcessTime_Peak,
	ii_statemachine_IdleTime_Peak,
	ii_statemachine_DataProcessMode
] = range (0,13)

i_AIC_in = 6
fmt_AIC_in = "<hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"

i_AIC_out = 7
fmt_AIC_out = "<hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"

i_z_servo = 12
i_m_servo = 13
fmt_servo = "<lllLlllll"
#               0123456789
[
	ii_servo_cp,
	ii_servo_ci,
	ii_servo_setpoint,
	ii_servo_input_p,
	ii_servo_delta,
	ii_servo_i_sum,
	ii_servo_control,
	ii_servo_neg_control,
	ii_servo_watch
] = range (0,9)

i_analog = 9
fmt_analog = "<llLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLllllllllllllllllLLLLllll"
[
	ii_analog_bias,
	ii_analog_motor,
	ii_analog_out00p,
	ii_analog_out00ap,
	ii_analog_out00bp,
	ii_analog_out00afp,
	ii_analog_out00bfp,
	ii_analog_out01p,
	ii_analog_out01ap,
	ii_analog_out01bp,
	ii_analog_out01afp,
	ii_analog_out01bfp,
	ii_analog_out02p,
	ii_analog_out02ap,
	ii_analog_out02bp,
	ii_analog_out02afp,
	ii_analog_out02bfp,
	ii_analog_out03p,
	ii_analog_out03ap,
	ii_analog_out03bp,
	ii_analog_out03afp,
	ii_analog_out03bfp,
	ii_analog_out04p,
	ii_analog_out04ap,
	ii_analog_out04bp,
	ii_analog_out04afp,
	ii_analog_out04bfp,
	ii_analog_out05p,
	ii_analog_out05ap,
	ii_analog_out05bp,
	ii_analog_out05afp,
	ii_analog_out05bfp,
	ii_analog_out06p,
	ii_analog_out06ap,
	ii_analog_out06bp,
	ii_analog_out06afp,
	ii_analog_out06bfp,
	ii_analog_out07p,
	ii_analog_out07ap,
	ii_analog_out07bp,
	ii_analog_out07afp,
	ii_analog_out07bfp,
	ii_analog_out08p,
	ii_analog_out08ap,
	ii_analog_out08bp,
	ii_analog_out08afp,
	ii_analog_out08bfp,
	ii_analog_out09p,
	ii_analog_out09ap,
	ii_analog_out09bp,
	ii_analog_out09afp,
	ii_analog_out09bfp,
	ii_analog_avg_input,
	ii_analog_avg_signal,
	ii_analog_rms_signal,
	ii_analog_bias_adjust,
	ii_analog_noise,
	ii_analog_vnull,
	ii_analog_wave0,
	ii_analog_wave1,
	ii_analog_in0,
	ii_analog_in1,
	ii_analog_in2,
	ii_analog_in3,
	ii_analog_in4,
	ii_analog_in5,
	ii_analog_in6,
	ii_analog_in7,
	ii_analog_diff_in_p0,
	ii_analog_diff_in_p1,
	ii_analog_diff_in_p2,
	ii_analog_diff_in_p3,
	ii_analog_counter0,
	ii_analog_counter1,
	ii_analog_gatetime0,
	ii_analog_gatetime1
] = range (0,76)

num_monitor_signals = NUM_MONITOR_SIGNALS
i_signal_monitor = 10
#######################miAAppppppppppppppppppppssssssssssssssssssssp
fmt_signal_monitor = "<llLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLlllllllllllllllllllllllllllllll"
ii_signal_monitor_p_first = 4
ii_signal_monitor_first = 4 + num_monitor_signals

i_scan = 14
fmt_scan = "<llllllllllllllllllllllllllllllllllllllllllLLLLll"
#               ss rot  npab srcs nxy fs n fm dxny Zo fmz  XYZ
#	fmt = "<hh hhhh hhhh llll hh  ll l lll hh hh ll lll ll hhhh hh" [38]
#	fmt = "<hhhhhhhhhhllllhhllllllhhhhlllllllhhhhhh"
[
	ii_scan_start,
	ii_scan_stop,
	ii_scan_rotmXX,
	ii_scan_rotmXY,
	ii_scan_rotmYX,
	ii_scan_rotmYY,
	ii_scan_nx_pre,
	ii_scan_dnx_probe,
	ii_scan_raster_a,
	ii_scan_raster_b,
	ii_scan_srcs_xp,
	ii_scan_srcs_xm,
	ii_scan_srcs_2nd_xp,
	ii_scan_srcs_2nd_xm,
	ii_scan_nx,
	ii_scan_ny,
	ii_scan_fs_dx,
	ii_scan_fs_dy,
	ii_scan_num_steps_move_xy,
	ii_scan_fm_dx,
	ii_scan_fm_dy,
	ii_scan_fm_dzxy,
	ii_scan_dnx,
	ii_scan_dny,
	ii_scan_Zoff_2nd_xp,
	ii_scan_Zoff_2nd_xm,
	ii_scan_fm_dz0_xy_vecX,
	ii_scan_fm_dz0_xy_vecY,
	ii_scan_z_slope_max,
	ii_scan_fast_return,
	ii_scan_xyz_vecX,
	ii_scan_xyz_vecY,
	ii_scan_xyz_vecZ,
	ii_scan_xy_r_vecX,
	ii_scan_xy_r_vecY,
	ii_z_offset_xyslope,
	ii_scan_cfs_dx,
	ii_scan_cfs_dy,
	ii_scan_iix,
	ii_scan_iiy,
	ii_scan_ix,
	ii_scan_iy,
	ii_scan_ifr,
	ii_scan_P_src_input0,
	ii_scan_P_src_input1,
	ii_scan_P_src_input2,
	ii_scan_P_src_input3,
	ii_scan_sstate,
	ii_scan_pflg
] = range (0,49)

i_move = 15
fmt_move = "<llllllllllll"
#	fmt = "<hhhhlllllllh"
[
	ii_move_start,
	ii_move_nRegel,
	ii_move_xy_new_vecX,
	ii_move_xy_new_vecY,
	ii_move_f_d_xyz_vecX,
	ii_move_f_d_xyz_vecY,
	ii_move_f_d_xyz_vecZ,
	ii_move_num_steps,
	ii_move_xyz_vecX,
	ii_move_xyz_vecY,
	ii_move_xyz_vecZ,
	ii_move_pflg
] = range (0,12)

i_probe = 16
fmt_probe = "<llllllllllllllllllllllllll"
ii_probe_LIM_up = 2
ii_probe_LIM_dn = 3
ii_probe_ACamp = 4
ii_probe_ACamp_aux = 5
ii_probe_ACfrq = 6
ii_probe_ACphaseA = 7
ii_probe_ACphaseB = 8
ii_probe_ACnAve = 9
ii_probe_lockin_shr_corrprod = 10
ii_probe_lockin_shr_corrsum = 11
ii_probe_src_lookup = 12  #..15
ii_probe_prb_lookup = 16 #..19
# ... incomplete py mapping

i_probe_vector_head = 0
fmt_probe_vector = "llLLLLLLllllllll"
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

i_autoapp = 17
fmt_autoapp = "<llllllllllLLllllllllllllll"
[
	ii_autoapp_start,
	ii_autoapp_stop,
	ii_autoapp_mover_mode,
	ii_autoapp_wave_outX,
	ii_autoapp_wave_outY,
	ii_autoapp_piezo_steps,
	ii_autoapp_n_wait,
	ii_autoapp_u_piezo_max,
	ii_autoapp_u_piezo_amp,
	ii_autoapp_piezo_speed,
	ii_autoapp_n_wait_fin,
	ii_autoapp_fin_cnt,
	ii_autoapp_mv_count,
	ii_autoapp_mv_dir,
	ii_autoapp_mv_step_count,
	ii_autoapp_u_piezo,
	ii_autoapp_step_next,
	ii_autoapp_tip_mode,
	ii_autoapp_delay_cnt,
	ii_autoapp_ci_mult,
	ii_autoapp_cp,
	ii_autoapp_ci,
	ii_autoapp_count_axis0,
	ii_autoapp_count_axis1,
	ii_autoapp_count_axis2,
	ii_autoapp_pflg
] = range (0,26)


i_pulse = 21
fmt_pulse = "<lllllHHHHllll"
#	fmt = "<hhhhlllllllh"
[
	ii_pulse_start,
	ii_pulse_stop,
	ii_pulse_duration,
	ii_pulse_period,
	ii_pulse_number,
	ii_pulse_on_bcode,
	ii_pulse_off_bcode,
	ii_pulse_reset_bcode,
	ii_pulse_dummy,
	ii_pulse_io_address,
	ii_pulse_i_per,
	ii_pulse_i_rep,
	ii_pulse_pflg
] = range (0,13)

i_generic_io = 22
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



i_datafifo = 18

fmt_datalong4 = "<LLLL"

fmt_datalong128 = "<LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"

fmt_datalong512 = "<LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"

fmt_datalong2048 = "<LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"

i_probedatafifo = 19

i_feedback_mixer = 11
fmt_feedback_mixer = "<llllllllllllllllllllllllLLllllllllllllllLLLLllllllll"

[
	ii_feedback_mixer_level0,
	ii_feedback_mixer_level1,
	ii_feedback_mixer_level2,
	ii_feedback_mixer_level3,
	ii_feedback_mixer_gain0,
	ii_feedback_mixer_gain1,
	ii_feedback_mixer_gain2,
	ii_feedback_mixer_gain3,
	ii_feedback_mixer_mode0,
	ii_feedback_mixer_mode1,
	ii_feedback_mixer_mode2,
	ii_feedback_mixer_mode3,
	ii_feedback_mixer_setpoint0,
	ii_feedback_mixer_setpoint1,
	ii_feedback_mixer_setpoint2,
	ii_feedback_mixer_setpoint3,
	ii_feedback_mixer_iir_ca_q15_0,
	ii_feedback_mixer_iir_ca_q15_1,
	ii_feedback_mixer_iir_ca_q15_2,
	ii_feedback_mixer_iir_ca_q15_3,
	ii_feedback_mixer_cb_Ic,
	ii_feedback_mixer_I_cross,
	ii_feedback_mixer_I_offset,
	ii_feedback_mixet_dum_align,
	ii_feedback_mixer_f_refLO,
	ii_feedback_mixer_f_refHI,
	ii_feedback_mixer_exec,
	ii_feedback_mixer_x,
	ii_feedback_mixer_lnx,
	ii_feedback_mixer_y,
	ii_feedback_mixer_iir_signal0,
	ii_feedback_mixer_iir_signal1,
	ii_feedback_mixer_iir_signal2,
	ii_feedback_mixer_iir_signal3,
	ii_feedback_mixer_q_factor15,
	ii_feedback_mixer_delta,
	ii_feedback_mixer_setpoint_log0,
	ii_feedback_mixer_setpoint_log1,
	ii_feedback_mixer_setpoint_log2,
	ii_feedback_mixer_setpoint_log3,
	ii_feedback_mixer_input_p0,
	ii_feedback_mixer_input_p1,
	ii_feedback_mixer_input_p2,
	ii_feedback_mixer_input_p3,
	ii_feedback_mixer_FB_IN_processed0,
	ii_feedback_mixer_FB_IN_processed1,
	ii_feedback_mixer_FB_IN_processed2,
	ii_feedback_mixer_FB_IN_processed3,
	ii_feedback_mixer_channel0,
	ii_feedback_mixer_channel1,
	ii_feedback_mixer_channel2,
	ii_feedback_mixer_channel3,
	ii_feedback_mixer_FB_IN_is_analog0,
	ii_feedback_mixer_FB_IN_is_analog1,
	ii_feedback_mixer_FB_IN_is_analog2,
	ii_feedback_mixer_FB_IN_is_analog3
] = range (0,56)

i_IO_pulse = 19 #19 CR pulse
fmt_IO_pulse = "<HHHHHHHHHHHH"

i_CR_generic_io = 22
fmt_CR_generic_io = "<llhhhhLLLLLLLLl"
fmt_CR_generic_io_write_a5 = "<hhhLLLLL" #+5
#	fmt = "<hhhhHHHHLLLLLh"

i_hr_mask = 24
fmt_hr_mask = "<llllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll"

## PLL header short map, recorder only
i_recorder = 25
fmt_recorder = "<lllLLLLL"
[
	ii_recorder_starmt,
	ii_recorder_stop,
	ii_recorder_pflg,
	ii_pSignal1,
	ii_pSignal2,
	ii_Signal1,
	ii_Signal2,
	ii_blcklen
] = range(0, 8)


# MK3Pro/SMPControl Magic is: 0x3202EE01
FB_SPM_MAGIC_ADR_MK3 = 0x10F13F00
FB_SPM_MAGIC_REF_MK3 = 0x3202EE01

# MK2 Magic
FB_SPM_MAGIC_ADR_MK2 = 0x5000
FB_SPM_MAGIC_REF_MK2 = 0xEE01


NUM_SIGNALS = 0


def dum_null ():
	return 0

class SPMcontrol():

	def __init__(self, mark="MK2", start_dev_id=0):
		self.sr_dev_index = start_dev_id
		self.sr_dev_base  = "/dev/sranger_mk2_"
		self.sr_dev_path  = "/dev/sranger_mk2_0"


		# auto locate Mk2 with SPMControl (for Gxsm) firmware
		
		self.spm_control_found = "NONE"
		# open SRanger, read magic offsets (look at FB_spm_dataexchange.h -> SPM_MAGIC_DATA_LOCATIONS)
		for i in range(self.sr_dev_index, 8-self.sr_dev_index):
			self.sr_dev_path = self.sr_dev_base+"%d" %i
			print "looking for "+mark+"/FB_SPMCONTROL/A810 at " + self.sr_dev_path
			try:
				with open (self.sr_dev_path):
					sr = open (self.sr_dev_path, "rw")
					# open SRanger (MK2) check for SPM_control software, read magic

					if mark == "MK2":
						os.lseek (sr.fileno(), FB_SPM_MAGIC_ADR_MK2, 1)
						self.magic = struct.unpack (MK2_fmt_magic, os.read (sr.fileno(), struct.calcsize (MK2_fmt_magic)))
						sr.close ()

						print ("FB_SPM_Control Magic ID (blind read):   %08X" %self.magic[0])
						if self.magic[0] == FB_SPM_MAGIC_REF_MK2:
							self.spm_control_found = "MK2"
							print "Identified MK2/FB_SPMCONTROL/A810 at " + self.sr_dev_path
							sr = open (self.sr_dev_path, "rw")
							os.lseek (sr.fileno(), self.magic[i_recorder], 0)
							self.RECORDER_VARS = struct.unpack (MK2_fmt_recorder, os.read (sr.fileno(), struct.calcsize (MK2_fmt_recorder)))
							sr.close ()
							break
					else:
						if mark == "MK3":
							os.lseek (sr.fileno(), FB_SPM_MAGIC_ADR_MK3, 1)
							self.magic = struct.unpack (MK3_fmt_magic, os.read (sr.fileno(), struct.calcsize (MK3_fmt_magic)))
							sr.close ()

							if self.magic[0] == FB_SPM_MAGIC_REF_MK3:
								self.spm_control_found = "MK3"
								print "Identified MK3-Pro/FB_SPMCONTROL/A810 at " + self.sr_dev_path
								sr = open (self.sr_dev_path, "rw")
								os.lseek (sr.fileno(), self.magic[i_recorder], 0)
								self.RECORDER_VARS = struct.unpack (fmt_recorder, os.read (sr.fileno(), struct.calcsize (fmt_recorder)))
								os.lseek (sr.fileno(), self.magic[i_pll_vars_lookup], 0)
								self.PLL_VARS = struct.unpack (fmt_pll_vars_lookup, os.read (sr.fileno(), struct.calcsize (fmt_pll_vars_lookup)))
								sr.close ()
								break
						else:
							print "unkown mark id requested"
							self.magic = [0,0,0,0,0,0]
							break
			except IOError:
				print 'Oh dear. No such device.'

			print "continue seeking for "+mark+" with FB_SPMCONTROL ..."

		if self.spm_control_found == "MK2":
			global NUM_SIGNALS
			NUM_SIGNALS = MK2_NUM_SIGNALS
			print
			print ("Mark2 on "+self.sr_dev_path)
			print ("*** SR-MK2-A810  FB-SPMCONTROL Magic Information ***")
			print ("Magic:   %08X" %self.magic[0])
			print ("Version: %08X" %self.magic[1])
			print ("Year:    %08X" %self.magic[2])
			print ("MM/DD:   %08X" %self.magic[3])
			print ("SoftID:  %08X" %self.magic[4])
			print ("Statema  %08X" %self.magic[5])
			print ("DataFIFO %08X" %self.magic[i_datafifo])
			print ("PrbFIFO  %08X" %self.magic[i_probedatafifo])
			print
		else:
			if self.spm_control_found == "MK3":
				global NUM_SIGNALS
				NUM_SIGNALS = MK3_NUM_SIGNALS
				print
				print ("Mark3 on "+self.sr_dev_path)
				print ("*** SR-MK3Pro-A810  FB-SPMCONTROL Magic Information ***")
				print ("Magic:   %08X" %self.magic[0])
				print ("Version: %08X" %self.magic[1])
				print ("Year:    %08X" %self.magic[2])
				print ("MM/DD:   %08X" %self.magic[3])
				print ("SoftID:  %08X" %self.magic[4])
				print ("Statema  %08X" %self.magic[5])
				print ("DataFIFO %08X" %self.magic[i_datafifo])
				print ("PrbFIFO  %08X" %self.magic[i_probedatafifo])
				print
			else:
				print
				print "*** Sorry, no valid SPM-CONTROL found running on any plugged in MK2/3 ***"
				self.sr_dev_path  = "none"

		## MK3 -- must be NUM_SIGNALS x L:
		self.fmt_signal_lookup = "<"
		for i in range (0,NUM_SIGNALS):
			self.fmt_signal_lookup = self.fmt_signal_lookup+"L"


	def status (self):
		return self.spm_control_found


	def get_spm_code_version_info (self):
		return "V%04X-Y%04x%04X"%(self.magic[1], self.magic[2], self.magic[3])

	def read_configurations (self):
		self.read_signal_lookup()
		self.read_actual_module_configuration ()
		self.get_status ()

## MK3 ---------------------------------------------------------------------------------
	def read_signal_lookup (self, update_pass=0):

		if self.spm_control_found == "MK2":
			self.SIGNAL_LOOKUP = MK2_SIGNAL_LOOKUP # make copy for self
			return 1
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
		if self.spm_control_found == "MK3":
			return DSP_MODULE_INPUT_ID_CATEGORIZED
		else:
			return MK2_DSP_MODULE_INPUT_ID_CATEGORIZED

        def read_actual_module_configuration (self):
                print "CURRENT FB_SPMCONTROL MODULE SIGNAL INPUT CONFIGURATION:"
		mod_inputs = self.get_module_configuration_list ()
                for mod in mod_inputs.keys():
                        for mod_inp in mod_inputs[mod]:
                                [signal, data, offset] = self.query_module_signal_input(mod_inp[0], mod_inp[3])
                                mod_inp[2] = signal [SIG_INDEX];
                                if mod_inp[2] >= 0:
                                        print signal[SIG_NAME]," ==> ",mod_inp[1],"     (", mod_inp, signal, data, ")"
                                else:
                                        if mod_inp[2] == 0 and mod_inp[3] == 1:
                                                print "DISABLED [p=0] ==> ", mod_inp[1],mod_inp[3],"     (", mod_inp, signal, data, ")"
                                        else:
                                                if	data[3] == 0 and data[0] == -1:
                                                        print "ERROR (can not associate signal adress): [p=0] ==> (", mod_inp, signal, data, ")"


	##### basic support -- universal

        def magic(self, i):
		return self.magic[i]

        def sr_path(self):
		return self.sr_dev_path

        def readf(self, srfileno,  magic_id, fmt, mode=0):
		os.lseek (srfileno, self.magic[magic_id], mode)
		return struct.unpack (fmt, os.read (srfileno, struct.calcsize (fmt)))

        def read(self, magic_id, fmt, mode=0):
		sr = open( self.sr_dev_path, "rb")
		data = self.readf( sr.fileno(), magic_id, fmt, mode)
		sr.close ()
		return data

        ## packed data example:    struct.pack ("<llLL", _input_id, _signal[SIG_INDEX],0,0)
        def write(self, magic_id, packeddata, mode=0):
		sr = open (self.sr_dev_path, "wb")
		os.lseek (sr.fileno(), self.magic[magic_id], mode)
		os.write (sr.fileno(), packeddata)
		sr.close ()

        def write_o(self, magic_id, offset, packeddata, mode=0):
		sr = open (self.sr_dev_path, "wb")
		os.lseek (sr.fileno(), self.magic[magic_id]+offset, mode)
		os.write (sr.fileno(), packeddata)
		sr.close ()

	def write_parameter(self, address,  packeddata, mode=0):
		sr = open (self.sr_dev_path, "wb")
		os.lseek (sr.fileno(), address, mode)
		os.write (sr.fileno(), packeddata)
		sr.close ()

        def get_status(self):
		sr = open (self.sr_dev_path)

		self.analog = self.readf( sr.fileno(), i_analog, fmt_analog)
		self.SPM_STATEMACHINE =  self.readf( sr.fileno(), i_statemachine,  fmt_statemachine)

		self.SPM_SCAN =  self.readf( sr.fileno(), i_scan,  fmt_scan)
		self.SPM_MOVE =  self.readf( sr.fileno(), i_move,  fmt_move)
		self.PROBE    =  self.readf( sr.fileno(), i_probe,  fmt_probe)
		self.AIC_in_buffer   =  self.readf( sr.fileno(), i_AIC_in,  fmt_AIC_in)
		self.AIC_out_buffer  =  self.readf( sr.fileno(), i_AIC_out, fmt_AIC_out)
		self.MIXER_FB =  self.readf( sr.fileno(), i_feedback_mixer,  fmt_feedback_mixer)

		self.AUTOAPP_WATCH  =  self.readf( sr.fileno(), i_autoapp,  fmt_autoapp)
		self.CR_COUNTER =  self.readf( sr.fileno(), i_CR_generic_io,  fmt_CR_generic_io)
		
		self.counts0  = self.CR_COUNTER[ii_generic_io_count_0];
		self.counts1  = self.CR_COUNTER[ii_generic_io_count_1];
		self.counts0h = self.CR_COUNTER[ii_generic_io_peak_count_0];
		self.counts1h = self.CR_COUNTER[ii_generic_io_peak_count_1];

		if self.spm_control_found == "MK3":
			self.Z_SERVO  =  self.readf( sr.fileno(), i_z_servo,  fmt_servo)
			self.M_SERVO  =  self.readf( sr.fileno(), i_m_servo,  fmt_servo)
			self.SIGNAL_MONITOR =  self.readf( sr.fileno(), i_signal_monitor,  fmt_signal_monitor)
#			print self.SIGNAL_MONITOR 

		sr.close ()
		return 1

	def read_signal_monitor_only(self):
		self.SIGNAL_MONITOR =  self.read(i_signal_monitor,  fmt_signal_monitor)

	def get_ADDA(self):
		return [self.AIC_in_buffer, self.AIC_out_buffer]

	def read_gpio(self,dum=0):
		GPIO_control = self.read (i_generic_io, fmt_generic_io)
		print "GENERIC-IO  (GPIO) control record:"
		print GPIO_control

	def write_gpio(self,dum=0):
		self.write (i_generic_io, struct.pack (fmt_generic_io, 0, 0, 255, 288, 32, 32, 0, 0, 0, 0, 0, 0, 22328, 0, 0))

	##### HR mode control

	def read_hr(self,dum=0):
		HR_MASK = self.read (i_hr_mask, fmt_hr_mask)
		print "HR_MASK is set to:"
		print HR_MASK

	def set_hr_0(self,dum=0):
####### 0                       1                       2                       3                       4                       5                       6                       7
####### 0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  
		self.write (i_hr_mask, struct.pack (fmt_hr_mask, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 \
							    ))
		self.read_hr()

	def set_hr_1(self,dum=0):
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

        ##### MK3 SIGNAL SUPPORT

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

                return [[-1,0,0,"n/a","N/A","0",0],0]

        def lookup_signal_by_name (self, signame):
                for signal in self.SIGNAL_LOOKUP:
                        if signal[SIG_NAME] == signame:
                                return signal
                return [-1,0,0,"n/a","N/A","0",0]

	# for sanity never direct change/write DSP pointers, but go via signal input pointer change request via signal lookup and signal id
	def change_signal_input(self, _signal, _input_id, voffset_func=lambda:0):
		voffset = voffset_func ()
		fmt = "<llLL"
		sr = open (self.sr_dev_path, "wb")
		os.lseek (sr.fileno(), self.magic[i_signal_monitor], 0)
		if voffset > 0 and voffset < _signal[SIG_DIM]:
			os.write (sr.fileno(), struct.pack (fmt, _input_id, _signal[SIG_INDEX]+(voffset<<16),0,0))
		else:
			os.write (sr.fileno(), struct.pack (fmt, _input_id, _signal[SIG_INDEX],0,0))
			voffset=0
		sr.close ()
		print "Signal Input ID (mindex) %d"%_input_id+" change request to signal id %d"%_signal[SIG_INDEX]+" Voff:[%d]"%voffset+"\n ==> "+str(_signal)
		# check on updates (units mixer)
		return 1

	# MK3 SIGNAL MONITOR SUPPORT
	def get_monitor_data (self, tap):
		return  self.SIGNAL_MONITOR[ii_signal_monitor_first+tap]
	
	def get_monitor_pointer (self, tap):
		return  self.SIGNAL_MONITOR[ii_signal_monitor_p_first+tap]
	
	def get_monitor_signal (self, tap):
		sigptr = self.SIGNAL_MONITOR[ii_signal_monitor_p_first+tap]
		if sigptr == 0:
			return [0.,0.,[-1,0,0,"n/a","N/A","0",0]]
		else:
			value  = self.SIGNAL_MONITOR[ii_signal_monitor_first+tap]
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

        # _signal_id ==  DSP_SIGNAL_TABLE_STORE_TO_FLASH_ID, DSP_SIGNAL_TABLE_RESTORE_FROM_FLASH_ID, DSP_SIGNAL_TABLE_ERASE_FLASH_ID, ... 
        def manage_signal_configuration(self, _button, _signal_id, data=dum_null):
		fmt = "<llLL" ## [ mindex, signal_id, act_address_input_set, act_address_signal [][] ]
		print "SIGNAL MONITOR: MANAGE SIGNAL CONFIGURATION REQUEST mi=0x%08X si=0x%08X data vector=[ "%(int (data()), _signal_id), fmt, int (data ()), _signal_id, 0,0, " ]"
		self.write (i_signal_monitor, struct.pack (fmt, int (data ()), _signal_id, 0,0))
		time.sleep(0.2)
		d = self.read(i_signal_monitor, fmt)
		print "Return = [ %x, %x, %x, %x ]"%(d[0], d[1], d[2], d[3])

	def flash_dump(self, _button, nb8=16):
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

        def disable_signal_input(self, dum, dumdum, _input_id): # _input_id == 0 reverts to power up defaults!!
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
		self.write (i_signal_monitor, struct.pack (fmt, _input_id, -1,0,0))
		#	print "Query Signal Input ID (mindex) %d:"%_input_id
		
		time.sleep(0.01)
		data = self.read(i_signal_monitor, fmt)
		#	print data
		[signal,offset] = self.lookup_signal_by_ptr (data[3], nullok)
		#	print signal
		return signal, data, offset
    
	def adjust_statemachine_flag(self, mask, flag):
		if flag:
			self.write_o (i_statemachine, 0, struct.pack (fmt_statemachine_w, mask))
		else:
			self.write_o (i_statemachine, 4, struct.pack (fmt_statemachine_w, mask))

	###define SRANGER_MK23_SEEK_ATOMIC      1 // Bit indicates ATOMIC mode to be used
	###define SRANGER_MK23_SEEK_DATA_SPACE  0 // DATA_SPACE, NON ATOMIC -- MK2: ONLY VIA USER PROVIDED NON ATOMIC KFUNC!!!!
	###define SRANGER_MK23_SEEK_PROG_SPACE  2 // default mode
	###define SRANGER_MK23_SEEK_IO_SPACE    4 // default mode

# MK2 recorder setup is special

	def MK2_recorder_assign_signals(self):
		return

	def MK2_set_recorder16(self, n, trigger=0, trigger_level=0):
		self.write_parameter(self.RECORDER_VARS[ii_blcklen16], struct.pack ("<l", n-1), 1)
		self.write_o(MK2_i_recorder, MK2_ii_pad, struct.pack ("<hh", 0,1))
		self.recorder_set=16;
		self.recorder_datamode  = dtype('<i2')
		self.recorder_blckindex = MK2_ii_blcklen16
		self.recorder_data_S1   = MK2_SDRAM_BUFFER
		self.recorder_data_S2   = MK2_SDRAM_BUFFER

	def MK2_set_recorder32(self, n, trigger=0, trigger_level=0):
		self.write_parameter(self.RECORDER_VARS[MK2_ii_blcklen32], struct.pack ("<l", n-1), 1)
		self.write_o(MK2_i_recorder, MK2_ii_pad, struct.pack ("<hh", 0,2))
		self.recorder_set=32;
		self.recorder_datamode = dtype('<i4')
		self.recorder_blckindex = MK2_ii_blcklen32
		self.recorder_data_S1   = MK2_SDRAM_BUFFER
		self.recorder_data_S2   = MK2_SDRAM_BUFFER

	def MK2_stop_recorder(self, n, trigger=0, trigger_level=0):
		self.recorder_set=0;
		self.write_o(MK2_i_recorder, MK2_ii_recorder_pad, struct.pack ("<hh", 0,0))
		self.write_parameter(self.RECORDER_VARS[MK2_ii_blcklen16], struct.pack ("<l", -1), 1)
		self.write_parameter(self.RECORDER_VARS[MK2_ii_blcklen32], struct.pack ("<l", -1), 1)


# MK2/MK3 recorder

	def MK3_set_recorder(self, n, trigger=0, trigger_level=0):
		self.write_o(i_recorder, ii_recorder_stop, struct.pack ("<ll", round(trigger_level), trigger))
		self.write_parameter(self.RECORDER_VARS[ii_blcklen], struct.pack ("<l", n-1), 1)
		self.recorder_datamode = dtype('<i4')
		self.recorder_blckindex = ii_blcklen
		self.recorder_data_S1 = self.RECORDER_VARS[ii_Signal1]
		self.recorder_data_S2 = self.RECORDER_VARS[ii_Signal2]

	def check_recorder_ready(self):
		sr = open (self.sr_dev_path, "rb")
		os.lseek (sr.fileno(), self.RECORDER_VARS[self.recorder_blckindex], 1)
		fmt = "<ll"
		[blck, dum] = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
		sr.close ()
		return blck


	def set_recorder(self, n, trigger=0, trigger_level=0):
		if self.spm_control_found == "MK3":
			self.MK3_set_recorder(n, trigger, trigger_level)	
		else:
			self.MK2_set_recorder32(n, trigger, trigger_level)	


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


	def read_recorder(self, n):
		aS1 = self.recorder_data_S1
		aS2 = self.recorder_data_S2
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
				xarray = append (xarray, fromfile(sr, self.recorder_datamode, num/4))
			else:
				xarray = fromfile(sr, self.recorder_datamode, num/4)
			sr.close ()

			sr = open (self.sr_dev_path, "rb")
			os.lseek (sr.fileno(), aS2+aoff, 0)
			if aoff > 0:
				yarray = append(yarray, fromfile(sr, self.recorder_datamode, num/4))
			else:
				yarray = fromfile(sr, self.recorder_datamode, num/4)
			sr.close ()

			aoff = aoff+BL
			num_left = num_left-BL 

		return [xarray.astype(float)[::-1], yarray.astype(float)[::-1]]

# generic recorder wrapper


# MK3 PAC/PLL SUPPORT

	def adjust_PLL_sine (self, volumeSine, FSineHz):
		# reconfigure sine generator
		vol = round (CPN(22)*volumeSine/10.)
		sr = open (self.sr_dev_path, "wb")	

		os.lseek (sr.fileno(), self.PLL_VARS[ii_PLL_volumeSine], 1)
		os.write (sr.fileno(), struct.pack ("<l", vol))

		theta = 2.*math.pi*FSineHz/150000.
		re = round (CPN(31) * math.cos (theta))
		im = round (CPN(31) * math.sin (theta))
		os.lseek (sr.fileno(), self.PLL_VARS[ii_PLL_deltaReT], 1)
		os.write (sr.fileno(), struct.pack ("<l", re))
		os.lseek (sr.fileno(), self.PLL_VARS[ii_PLL_deltaImT], 1)
		os.write (sr.fileno(), struct.pack ("<l", im))

		phase = round (CPN(29)*theta)
		os.lseek (sr.fileno(), self.PLL_VARS[ii_PLL_PI_Phase_Out], 1)
		os.write (sr.fileno(), struct.pack ("<l", phase))

		# Exec change Freq
		os.lseek (sr.fileno(), self.magic[i_pll_vars_lookup], 1)
		os.write (sr.fileno(), struct.pack ("<ll", 1,0)) #  => exec ChangeFreqHL () on DSP after adjused Re/Im.

		sr.close ()

	def set_PLL_phase_step (self, phase_step):
		# setup a step and exectute step response measurement
		sr = open (self.sr_dev_path, "wb")	

		d_re = int (round (CPN(29) * phase_step * math.pi / 180.))
		os.lseek (sr.fileno(), self.PLL_VARS[ii_PLL_deltaReT], 1)
		os.write (sr.fileno(), struct.pack ("<l", d_re))

		# Exec change TestPIRest
		os.lseek (sr.fileno(), self.magic[i_pll_vars_lookup], 3)
		os.write (sr.fileno(), struct.pack ("<ll", 1,0)) #  => exec 3 => TestPhasePIResp_HL

		sr.close ()

	def set_PLL_amplitude_step (self, amp_step):
		# setup a step and exectute step response measurement
		sr = open (self.sr_dev_path, "wb")	

		d_re = int (round (CPN(29) * amp_step / 10.))
		os.lseek (sr.fileno(), self.PLL_VARS[ii_PLL_deltaReT], 1)
		os.write (sr.fileno(), struct.pack ("<l", d_re))

		# Exec change TestPIRest
		os.lseek (sr.fileno(), self.magic[i_pll_vars_lookup], 4)
		os.write (sr.fileno(), struct.pack ("<ll", 1,0)) #  => exec 4 => TestAmpPIResp_HL

		sr.close ()

	def read_PLL_Filter64Out (self):
		sr = open (self.sr_dev_path, "rb")
		os.lseek (sr.fileno(), self.PLL_VARS[ii_PLL_Filter64Out], 1)
		Filter64Out = struct.unpack (fmt_Filter64Out, os.read (sr.fileno(), struct.calcsize (fmt_Filter64Out)))
		sr.close ()
		return Filter64Out

# END
