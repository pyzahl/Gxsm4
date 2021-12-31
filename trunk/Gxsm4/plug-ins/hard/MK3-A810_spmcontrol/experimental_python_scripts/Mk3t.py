#!/usr/bin/env python

## * Python User Control for
## * Configuration and SPM Approach Control tool for the FB_spm DSP program/GXSM2
## * for the Signal Ranger STD/SP2 DSP board
## * 
## * Copyright (C) 2003 Percy Zahl
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

import pygtk
pygtk.require('2.0')

import gobject, gtk
import os		# use os because python IO is bugy
import time
import fcntl

#import GtkExtra
import struct
import array

import math

# Set here the SRanger dev to be used:
sr_dev_path = "/dev/sranger_mk2_0"

i_magic = 0
fmt_magic = "<LLLLLLLLLLLLLLLLLLLLLLLLLL"

i_magic_version = 1
i_magic_year    = 2
i_magic_date    = 3
i_magic_sid     = 4

i_statemachine = 5
fmt_statemachine = "<LLLLLLLLLLLLL"
fmt_statemachine_w = "<l"
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

i_feedback = 9
fmt_feedback = "<llllllllllllllllllllll"
#               0123456789012345678901
#	fmt = "<hhhhlhhhhhhlllhhhhllll"
[
	ii_feedback_cp,
	ii_feedback_ci,
	ii_feedback_setpoint,
	ii_feedback_ca_q15,
	ii_feedback_cb_Ic,
	ii_feedback_I_cross,
	ii_feedback_I_offset,
	ii_feedback_q_factor15,
	ii_feedback_soll,
	ii_feedback_ist,
	ii_feedback_delta,
	ii_feedback_i_sum,
	ii_feedback_I_iir,
	ii_feedback_z,
	ii_feedback_I_fbw,
	ii_feedback_zxx,
	ii_feedback_zyy,
	ii_feedback_I_avg,
	ii_feedback_I_rms,
	ii_feedback_z32neg,
	ii_feedback_z32,
	ii_feedback_watch
] = range (0,22)

i_analog = 10
fmt_analog = "<hhhhhhhhhhhhhhhh"  #+0
fmt_analog_offset = "<hhhhhhhhllllllll" #+16
fmt_analog_out_offset = "<hhhhhhhh" #+ 16 +8+2*2*8+8+6*32+2*128

i_scan = 11
fmt_scan = "<lllllllllllllllllllllllllllllllllllllll"
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
	ii_scan_cfs_dx,
	ii_scan_cfs_dy,
	ii_scan_iix,
	ii_scan_iiy,
	ii_scan_ix,
	ii_scan_iy,
	ii_scan_ifr,
	ii_scan_sstate,
	ii_scan_pflg
] = range (0,44)

i_move = 12
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

i_probe = 13
fmt_probe = "<hhhhhhhhllllllll"
ii_probe_ACamp = 2
ii_probe_ACfrq = 3
ii_probe_ACphaseA = 4
ii_probe_ACphaseB = 5
ii_probe_ACnAve = 6
ii_probe_ACix = 7
ii_probe_time = 8

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

i_datafifo = 15
fmt_datafifo = "<llllllLL"
[
	ii_datafifo_r_position,
	ii_datafifo_w_position,
	ii_datafifo_fill,
	ii_datafifo_stall,
	ii_datafifo_length,
	ii_datafifo_mode,
	ii_datafifo_data_v0,
	ii_datafifo_data_v1
] = range (0,8)

fmt_datalong4 = "<LLLL"

fmt_datalong32 = "<LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"

fmt_datalong64 = "<LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"

fmt_datalong128 = "<LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"

fmt_datalong256 = "<LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"

fmt_datalong512 = "<LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"

fmt_datalong2048 = "<LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"

i_probedatafifo = 16

i_feedback_mixer = 18
fmt_feedback_mixer = "<llllllllllllllllllllllll"
#	fmt = "<hhhhhhhhhhhhhhhhhhhlhhhh"
[
	ii_feedabck_mixer_level0,
	ii_feedabck_mixer_level1,
	ii_feedabck_mixer_level2,
	ii_feedabck_mixer_level3,
	ii_feedabck_mixer_gain0,
	ii_feedabck_mixer_gain1,
	ii_feedabck_mixer_gain2,
	ii_feedabck_mixer_gain3,
	ii_feedabck_mixer_mode0,
	ii_feedabck_mixer_mode1,
	ii_feedabck_mixer_mode2,
	ii_feedabck_mixer_mode3,
	ii_feedabck_mixer_setpoint0,
	ii_feedabck_mixer_setpoint1,
	ii_feedabck_mixer_setpoint2,
	ii_feedabck_mixer_setpoint3,
	ii_feedabck_mixer_exec,
	ii_feedabck_mixer_x,
	ii_feedback_mixer_lnx,
	ii_feedabck_mixer_y,
	ii_feedabck_mixer_delta,
	ii_feedabck_mixer_setpoint_log0,
	ii_feedabck_mixer_setpoint_log1,
	ii_feedabck_mixer_setpoint_log2,
	ii_feedabck_mixer_setpoint_log3
] = range (0,25)

i_IO_pulse = 19 #19 CR pulse
fmt_IO_pulse = "<HHHHHHHHHHHH"

i_external = 20
fmt_external = "<lllll"
#	fmt = "<hhhhh"
fmt_external_w = "<l"
# "<h"

i_CR_generic_io = 21
fmt_CR_generic_io = "<llhhhhLLLLLLLL"
fmt_CR_generic_io_write_a5 = "<hhhLLLLL" #+5
#	fmt = "<hhhhHHHHLLLLLh"

i_watch = 23
fmt_watch = "<llllllllllllllllllllllll"
#	fmt = "<hhhhhhhhhhhhhhhhllllllll"

i_hr_mask = 24
fmt_hr_mask = "<llllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll"


FB_SPM_MAGIC_ADR_MK3 = 0x10F13F00
FB_SPM_MAGIC_REF_MK3 = 0x3202EE01

global SPM_STATEMACHINE  # at magic[5]
global CR_COUNTER        # at magic[21]
global SPM_FEEDBACK      # at magic[9]
global MIXER_FB          # at magic[18]
global analog            # at magic[10]
global analog_offset	 # at magic[10]+16
global analog_out_offset # at magic[10]+8+2*2*8+8+6*32+2*128
global AIC_in_buffer	 # at magic[6]
global AIC_out_buffer	 # at magic[7]
global EXT_CONTROL       # at magic[20]
global SPM_SCAN          # at magic[11]
global SPM_MOVE          # at magic[12]
global WATCH             # at magic[23]
global WATCHL16
global WATCHL17
global offset_mutex
global counts0
global counts1
global counts0h
global counts1h
global forceupdate	 # force update routine: bit 0-8 are for the offset adjustment
global xX
global app_cp
global app_ci
global app_mode
global app_pulse_cnt
global app_cnt

global stage_direction
global xyz_count
global xyz_steps

global fuzzy_adj
global user_action_mode
global user_action_y
global user_action_i

global AIC_slow
global lvdt_zero, lvdt_ist, lvdt_set
global goto_mode

global gatetime_ms1
global gatetime_ms2

gatetime_ms1 = 100.
gatetime_ms2 = 10.

goto_mode = 0
lvdt_xy = [0., 0.]

AIC_slow = [0.,0.,0.,0., 0.,0.,0.,0.]
lvdt_ist  = [0., 0.]
lvdt_zero = [0., 0.]
lvdt_set  = [0., 0.]

user_action_i = 0

stage_direction = 0
xyz_count = [0, 0, 0]
xyz_steps = [10, 10, 10]

xX = "*"
app_ci=int (-0.01*32767)
app_cp=int (-0.01*32767)
app_mode=-1
app_pulse_cnt=0
app_cnt=0

printrate = 5000
pv = 0
pc = 0


updaterate = 80	# update time watching the SRanger in ms

wins = {}

def delete_event(win, event=None):
	win.hide()
	# don't destroy window -- just leave it hidden
	return True


# open SRanger, read magic offsets (look at FB_spm_dataexchange.h -> SPM_MAGIC_DATA_LOCATIONS)
def open_sranger():
	sr = open (sr_dev_path, "rw")
	os.lseek (sr.fileno(), FB_SPM_MAGIC_ADR_MK3, 1)
	magic = struct.unpack (fmt_magic, os.read (sr.fileno(), struct.calcsize (fmt_magic)))
	sr.close ()
	return magic

# update DSP setting parameter
def int_param_update(_adj):
	param_address = _adj.get_data("int_param_address")
	param = int (_adj.get_data("int_param_multiplier")*_adj.value)
	sr = open (sr_dev_path, "wb")
	os.lseek (sr.fileno(), param_address, 1)
	os.write (sr.fileno(), struct.pack ("<h", param))
	sr.close ()
	
def AIC_in_offset_update(_adj):
	param_address = _adj.get_data("AIC_offset_address")
	param = int (_adj.value*3.2767)
	sr = open (sr_dev_path, "wb")
	os.lseek (sr.fileno(), param_address, 1)
	os.write (sr.fileno(), struct.pack ("<h", param))
	sr.close ()

def AIC_out_offset_update(_adj):
	param_address = _adj.get_data("AIC_offset_address")
	param = int (_adj.value*3.2767)
	sr = open (sr_dev_path, "wb")
	os.lseek (sr.fileno(), param_address, 1)
	os.write (sr.fileno(), struct.pack ("<h", param))
	sr.close ()

def ext_FB_control(_object, _value, _identifier):
	value = 0
	sr = open (sr_dev_path, "wb")
	if _identifier=="ext_channel":
		os.lseek (sr.fileno(), magic[i_external]+0, 1)
		value = _object.get_value()
	if _identifier=="ext_treshold":
		os.lseek (sr.fileno(), magic[i_external]+4, 1)
		value = _object.get_value()*3276.7
	if _identifier=="ext_value":
		os.lseek (sr.fileno(), magic[i_external]+8, 1)
		value = _value
	if _identifier=="ext_min":
		os.lseek (sr.fileno(), magic[i_external]+12, 1)
		value = _object.get_value()*16383
	if _identifier=="ext_max":
		os.lseek (sr.fileno(), magic[i_external]+16, 1)
		value = _object.get_value()*16383
	os.write (sr.fileno(), struct.pack (fmt_external_w, int(value)))
	sr.close ()
	get_status()
	return 1

def make_menu_item(name, _object, callback, value, identifier):
	    item = gtk.MenuItem(name)
	    item.connect("activate", callback, value, identifier)
	    item.show()
	    return item
 
def toggle_flag(w, mask):
	sr = open (sr_dev_path, "wb")
	if w.get_active():
		os.lseek (sr.fileno(), magic[i_statemachine], 1)
	else:
		os.lseek (sr.fileno(), magic[i_statemachine]+4, 1)
	os.write (sr.fileno(), struct.pack (fmt_statemachine_w, mask))
	sr.close ()
	get_status()

def enable_ratemeter(w, gtms1, gtms2, htms):
	global gatetime_ms1
	global gatetime_ms2
	DSP_GATEREF = 1./75000. # [s] 75 kHz DSP Gate-Window

	gc1 = float(gtms1())
	gc2 = float(gtms2())

	gatetime_ms1 = gc1
	gatetime_ms2 = gc2

	gcc1 = 0
	gc1 = gc1*1e-3
	gc2 = gc2*1e-3
	gcc1 = int (gc1/DSP_GATEREF)
	gcc2 = int (gc2/DSP_GATEREF)
	if (gcc1 < 0):
		gcc1 = 1;
	m = 0
	if (gcc2 < 0):
		gcc2 = 1;
	if (gcc2 > 65535):
		gcc2 = 65535;

	gatetime_ms1 = gcc1 * DSP_GATEREF * 1e3    # actual gate time 1
	gatetime_ms2 = gcc2 * DSP_GATEREF * 1e3    # actual gate time 2

	gcc32 = gcc1
	while (gcc1 > 65536):
		gcc1 -= 65536
		m += 1

	print "Gate-Length1 (cycles) g32bit gm g16, actual ms 1 2: ", gcc32, m, gcc1, gatetime_ms1, gatetime_ms2
#### to be fixed ######
	sr = open (sr_dev_path, "wb")
	os.lseek (sr.fileno(), magic[21]+5, 1) # mode, gatecount
	os.write (sr.fileno(), struct.pack ("<hhhLLLLL", gcc1, m, gcc2, 0,0,0,0,0))
	sr.close ()
	
	sr = open (sr_dev_path, "wb")
	os.lseek (sr.fileno(), magic[21]+18, 1)
	if w.get_active():
		os.write (sr.fileno(), struct.pack ("<h", 1))
	else:
		os.write (sr.fileno(), struct.pack ("<h", 0))
	sr.close ()

	sr = open (sr_dev_path, "wb")
	os.lseek (sr.fileno(), magic[21], 1)
	if w.get_active():
		os.write (sr.fileno(), struct.pack ("<h", 1))
	else:
		os.write (sr.fileno(), struct.pack ("<h", 0))
	sr.close ()


def CoolRunnerIOReadCb(lab0):
	data = CoolRunnerIORead ()
	lab0.set_text ("[GPIO-RD] 0x%04X" %data)

def CoolRunnerIORead():
	sr = open (sr_dev_path, "wb")
	os.lseek (sr.fileno(), magic[21], 1)
	os.write (sr.fileno(), struct.pack ("<h", 2)) # Start (code what 1: WR, 2: RD, 3: DirReconf), Stop, Dir-, In-, Out-data
	sr.close ()
	sr = open (sr_dev_path, "rb")
	os.lseek (sr.fileno(), magic[21], 1)
	fmt = "<hhHHH" # Start (code what 1: WR, 2: RD, 3: DirReconf), Stop, Dir-, In-, Out-data
	data = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
	sr.close ()
	return data[3]

def CoolRunnerIOWritePort(data):
	sr = open (sr_dev_path, "wb")
	os.lseek (sr.fileno(), magic[21]+4, 1)
	os.write (sr.fileno(), struct.pack ("<H", data)) # Start (code what 1: WR, 2: RD, 3: DirReconf), Stop, Dir-, In-, Out-data
	os.lseek (sr.fileno(), magic[21], 1)
	os.write (sr.fileno(), struct.pack ("<hh", 1,0)) # Start (code what 1: WR, 2: RD, 3: DirReconf), Stop, Dir-, In-, Out-data
	sr.close ()
	return 1

def CoolRunnerIOWriteDirection(w, entry, status):
	dirdata = int (entry.get_text(), 16) # Hex
	sr = open (sr_dev_path, "wb")
	os.lseek (sr.fileno(), magic[21], 1)
	os.write (sr.fileno(), struct.pack ("<hhH", 3,0, dirdata)) # Start (code what 1: WR, 2: RD, 3: DirReconf), Stop, Dir-, In-, Out-data
	sr.close ()
	status.set_text ("GPIO CONF DIRECTION: 0x%04X" %dirdata)
	return 1


def CoolRunnerIOReadE16(w, entry, status):
	data = CoolRunnerIORead()
	entry.set_text ("0x%04X" %data)
	status.set_text ("GPIO RD: 0x%04X" %data)

def CoolRunnerIOWrite(w, entry, status):
	data = int (entry.get_text(), 16) # Hex
	CoolRunnerIOWritePort (data)
	status.set_text ("GPIO WR: 0x%04X" %data)



def CoolRunnerIOpulse_mask(num, tau, mask, trig):
	Tp = 0.0454545454545
	sr = open (sr_dev_path, "wb")
	W = int (tau/10./Tp)
	T = int (tau/Tp)
	N = num - 1
	bOn = mask
	bOff = trig+mask
	bReset =  trig+mask
	os.lseek (sr.fileno(), magic[19], 1)
	os.write (sr.fileno(), struct.pack ("<HHHHHHHHHHHH", 1, 0, W, T, N, bOn, bOff, bReset, 6, 0, 0, 0))
	sr.close ()
	return 1

def CoolRunnerIOpulse(w, entry_W, entry_T, entry_N, entry_bOn, entry_bOff, entry_bR, status):
	Tp = 0.0454545454545
	sr = open (sr_dev_path, "wb")
	W = int (float (entry_W.get_text())/Tp)
	T = int (float (entry_T.get_text())/Tp)
	N = int (entry_N.get_text()) - 1
	bOn = int (entry_bOn.get_text(), 16)
	bOff = int (entry_bOff.get_text(), 16)
	bReset =  int (entry_bR.get_text(), 16)
	os.lseek (sr.fileno(), magic[19], 1)
	os.write (sr.fileno(), struct.pack ("<HHHHHHHHHHHH", 1, 0, W, T, N, bOn, bOff, bReset, 6, 0, 0, 0))
	sr.close ()
	status.set_text ("CoolRunner ExecPlus: W%d" %W + " T%d" %T + " #%d" %N)
	return 1

def CoolRunnerAppCtrl(w, flg):
	global app_mode
	app_mode = flg
	return 1

def CoolRunnerAppRun(w, entry_pSpn, entry_W, entry_T, entry_N, entry_bOn, entry_bOff, entry_bR, status, status2):

	global app_cp
	global app_ci
	global app_mode
	global app_pulse_cnt
	global app_cnt

	if app_mode == -2:
		sr = open (sr_dev_path, "wb")
		os.lseek (sr.fileno(), magic[9], 1)
		os.write (sr.fileno(), struct.pack ("<hh", app_cp, app_ci))
		sr.close ()
		app_mode = -1
		return 1;

	if app_mode == -1:
		return 1

	if app_mode == 0:
		sr = open (sr_dev_path, "rb")
		os.lseek (sr.fileno(), magic[9], 1)
		fmt = "<hh"
		data = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
		sr.close ()
		# app_cp = int (-0.01*32767)
		app_cp = data[0]
		# app_ci = int (-0.01*32767)
		app_ci = data[1]
		# check appoach status before starting!!
		app_mode = 4
		app_pulse_cnt = 0
		app_cnt = 0
		
	cpf = app_cp/32767.
	cif = app_ci/32767.

#	ci_retract = int (float(entry_negCI.get_text())*32767) 
	ci_retract = -50.*app_ci
	cirf = ci_retract/32767.

	Zspan = int (float(entry_pSpn.get_text())/100*32000)

	if Zspan > 0.0:
		Zsignum = 1.0
	else:
		Zsignum = -1.0
		Zspan = -Zspan

	if Zspan > 32000:
		Zspan = 32000
	if Zspan < 5000:
		Zspan = 5000

	
	status.set_text ("AppMd[%1d]" %app_mode + "  FB: CP: %+6.4f" %cpf + "  CI: %+6.4f" %cif + "  CIre: %+6.4f" %cirf + " # %4d " %app_pulse_cnt + ":%3d" %app_cnt)

	# voll weg > +32000 ==> +Zspan
	# voll ran < -32000 ==> -Zspan

	if app_mode == 1:
		# retract tip
		sr = open (sr_dev_path, "wb")
		os.lseek (sr.fileno(), magic[9], 1)
		os.write (sr.fileno(), struct.pack ("<hh", app_cp, ci_retract))
		sr.close ()
		app_mode = 2
		return 1

	if app_mode == 2:
		# monitor Z
		sr = open (sr_dev_path, "rb")
		os.lseek (sr.fileno(), magic[7]+5, 1)
		fmt = "<h"
		data = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
		sr.close ()
		status2.set_text ("Zr: %6d" %data[0])
		if Zsignum*data[0] > Zspan:
			app_mode = 3
		return 1

	if app_mode == 3:
		# IO Puls(es) now
		CoolRunnerIOpulse(w, entry_W, entry_T, entry_N, entry_bOn, entry_bOff, entry_bR, status2)
		app_pulse_cnt += 1
		app_mode = 4
		return 1

	if app_mode == 4:
		# approach tip, using feedback
		sr = open (sr_dev_path, "wb")
		os.lseek (sr.fileno(), magic[9], 1)
		os.write (sr.fileno(), struct.pack ("<hh", app_cp, app_ci))
		sr.close ()
		app_mode = 5
		return 1


	if app_mode == 5:
		# monitor Z
		sr = open (sr_dev_path, "rb")
		os.lseek (sr.fileno(), magic[7]+5, 1)
		fmt = "<h"
		data = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
		sr.close ()
		status2.set_text ("Za: %6d" %data[0])
		# voll ran?
		if Zsignum*data[0] < -Zspan:
		        app_cnt = 0
		        app_mode = 1
			return 1
		app_cnt += 1

		# Stop, done?
		if app_cnt > 200:
			app_mode = -2
			status2.set_text ("Approach OK. ??   Z = %6d" %data[0])
			print "App OK.? Z=", data[0]
	return 1

def force_offset_recalibration (w, mask):
	global forceupdate
	sr = open (sr_dev_path, "wb")
	os.lseek (sr.fileno(), magic[i_statemachine] + 2*24, 0)
	os.write (sr.fileno(), struct.pack (fmt_statemachine_w, 0))
	# calibration needs 8300 cycles for avg (0.38 s)
	time.sleep(.5)
	# activating offset compensation
	os.lseek (sr.fileno(), magic[i_statemachine], 0)
	os.write (sr.fileno(), struct.pack (fmt_statemachine_w, 0x0010))
	sr.close ()
	# refresh sliders and check buttons
	get_status()
	forceupdate = 255

# create SR DSP SPM settings dialog
def create_settings_edit(_button):
	name = "SR DSP SPM Settings"
	if not wins.has_key(name):
		win = gobject.new(gtk.Window,
				  type=gtk.WINDOW_TOPLEVEL,
				  title=name,
				  allow_grow=False,
				  allow_shrink=False,
				  border_width=10)
		wins[name] = win
		win.connect("delete_event", delete_event)
		

		box1 = gobject.new(gtk.VBox(spacing=5))
		win.add(box1)
		box1.show()

		box2 = gobject.new(gtk.VBox(spacing=5))
		box2.set_border_width(10)
		box1.pack_start(box2)
		box2.show()

		hbox = gobject.new(gtk.HBox())
		box2.pack_start(hbox)
		hbox.show()

		lab = gobject.new(gtk.Label, label="* * * State Flags [state.mode] * * *\n -->  watching every %d ms" %updaterate)
		hbox.pack_start(lab)
		lab.show()

		separator = gobject.new(gtk.HSeparator())
		box2.pack_start(separator, expand=False)
		separator.show()

		hbox = gobject.new(gtk.VBox())
		box2.pack_start(hbox)
		hbox.show()

		box2 = gobject.new(gtk.VBox(spacing=10))
		box2.set_border_width(10)
		box1.pack_start(box2, expand=False)
		box2.show()
		
		box3 = gobject.new(gtk.HBox())
		hbox.pack_start(box3)
		box3.show()		

#		adj = gtk.Adjustment(int(EXT_CONTROL[0]), 0, 7, 1, 1, 0)
		adj = gtk.Adjustment(int(EXT_CONTROL[0]), -32768, 32767, 1, 1, 0)
		spinner = gtk.SpinButton(adj, 0, 0)
		spinner.set_wrap(False)
		spinner.set_numeric(True)
		spinner.set_update_policy(gtk.UPDATE_ALWAYS)
		spinner.connect('value-changed', ext_FB_control, 0, "ext_channel")
		spinner.show()

		adj = gtk.Adjustment(EXT_CONTROL[1]/3276.7, -9.9, 9.9, .1, 1, 0)
		spinner2 = gtk.SpinButton(adj, 0, 0)
		spinner2.set_digits(1)
		spinner2.set_snap_to_ticks(True)
		spinner2.set_wrap(False)
		spinner2.set_numeric(True)
		spinner2.set_update_policy(gtk.UPDATE_IF_VALID)
		spinner2.connect('value-changed', ext_FB_control, 0, "ext_treshold")
		spinner2.show()

		label = gobject.new(gtk.Label, label="is above")
		label.show()

		label2 = gobject.new(gtk.Label, label="V")
		label2.show()

		check_button = gtk.CheckButton("Feedback OFF if channel")
		check_button.set_active(False)
		gobject.timeout_add (updaterate, check_update, check_button.set_active, 0x0100)
		check_button.connect('toggled', toggle_flag, 0x0100)
		check_button.show()

		box3.pack_start(check_button)
		box3.pack_start(spinner)
		box3.pack_start(label)
		box3.pack_start(spinner2)
		box3.pack_start(label2)


		check_button = gtk.CheckButton("Log. transformation STM/AFM  [0x0008]\n (enable for STM!, handled by GXSM!)")
		if SPM_STATEMACHINE[2] & 0x0008:
			check_button.set_active(True)
		gobject.timeout_add (updaterate, check_update, check_button.set_active, 0x0008)
		check_button.connect('toggled', toggle_flag, 0x0008)
		check_button.show()
		hbox.pack_start(check_button)

		check_button = gtk.CheckButton("Offset Compensation [0x0010]\n (automatic offset compensation, needs\n Zero volt or open inputs at DSP startup!)")
		if SPM_STATEMACHINE[2] & 0x0010:
			check_button.set_active(True)
		check_button.connect('toggled', toggle_flag, 0x0010)
		gobject.timeout_add (updaterate, check_update, check_button.set_active, 0x0010)
		check_button.show()
		hbox.pack_start(check_button)

		check_button = gtk.CheckButton("High Resolution Mode [0x0001]\n (Sigma Delta Style Resolution increase via bit0 for Z))")
		if SPM_STATEMACHINE[2] & 0x0001:
			check_button.set_active(True)
		check_button.connect('toggled', toggle_flag, 0x0001)
		gobject.timeout_add (updaterate, check_update, check_button.set_active, 0x0001)
		check_button.show()
		hbox.pack_start(check_button)

		check_button = gtk.CheckButton("Z-Transformation [0x0020]\n (AIC[6] * (-1) -> AIC[5])")
		if SPM_STATEMACHINE[2] & 0x0020:
			check_button.set_active(True)
		gobject.timeout_add (updaterate, check_update, check_button.set_active, 0x0020)
		check_button.connect('toggled', toggle_flag, 0x0020)
		check_button.show()
		hbox.pack_start(check_button)

		check_button = gtk.CheckButton("RANDOM NOISE GEN [0x0040] (add with AC_Amp on Bias!)\n")
		if SPM_STATEMACHINE[2] & 0x0040:
			check_button.set_active(True)
		check_button.connect('toggled', toggle_flag, 0x0040)
		gobject.timeout_add (updaterate, check_update, check_button.set_active, 0x0040)
		hbox.pack_start(check_button)
		check_button.show()
			
		check_button = gtk.CheckButton("Offset Adding [0x0080]\n (enable offset adding to scan signals: AIC[0/1/2] not used)")
		if SPM_STATEMACHINE[2] & 0x0080:
			check_button.set_active(True)
		gobject.timeout_add (updaterate, check_update, check_button.set_active, 0x0080)
		check_button.connect('toggled', toggle_flag, 0x0080)
		check_button.show()
		hbox.pack_start(check_button)

		check_button = gtk.CheckButton("AIC Slow [0x0200]\n (enable compuation of additional slow AIC6/7 statistical data)")
		if SPM_STATEMACHINE[2] & 0x0200:
			check_button.set_active(True)
		gobject.timeout_add (updaterate, check_update, check_button.set_active, 0x0200)
		check_button.connect('toggled', toggle_flag, 0x0200)
		check_button.show()
		hbox.pack_start(check_button)

		check_button = gtk.CheckButton("AIC Stop [0x2000]\n (Halt AICs -- stops data conversion and DMA transfer)")
		if SPM_STATEMACHINE[2] & 0x2000:
			check_button.set_active(True)
		gobject.timeout_add (updaterate, check_update, check_button.set_active, 0x2000)
		check_button.connect('toggled', toggle_flag, 0x2000)
		check_button.show()
		hbox.pack_start(check_button)

		check_button = gtk.CheckButton("Pass")
		if SPM_STATEMACHINE[2] & 0x0400:
			check_button.set_active(True)
		gobject.timeout_add (updaterate, check_update, check_button.set_active, 0x0400)
		check_button.connect('toggled', toggle_flag, 0x0400)
		check_button.show()

	        opt = gtk.OptionMenu()
	        menu = gtk.Menu()
	        item = make_menu_item("Feedback", make_menu_item, ext_FB_control, 0, "ext_value")
	        menu.append(item)
	        item = make_menu_item("X Offset", make_menu_item, ext_FB_control, 1, "ext_value")
	        menu.append(item)
	        item = make_menu_item("log[AIC5]", make_menu_item, ext_FB_control, 2, "ext_value")
	        menu.append(item)
	        item = make_menu_item("FB delta", make_menu_item, ext_FB_control, 3, "ext_value")
	        menu.append(item)
	        item = make_menu_item("PRB Sine", make_menu_item, ext_FB_control, 4, "ext_value")
	        menu.append(item)
	        item = make_menu_item("PRB Phase", make_menu_item, ext_FB_control, 5, "ext_value")
	        menu.append(item)
	        item = make_menu_item("Bias Add(7)", make_menu_item, ext_FB_control, 6, "ext_value")
	        menu.append(item)
	        item = make_menu_item("EN_MOTOR Off[7]->M", make_menu_item, ext_FB_control, 7, "ext_value")
	        menu.append(item)
	        item = make_menu_item("PRB Phase->M", make_menu_item, ext_FB_control, 8, "ext_value")
	        menu.append(item)
	        #item = make_menu_item("Source 3", make_menu_item, ext_FB_control, 2, "ext_value")
	        #menu.append(item)
		menu.set_active(EXT_CONTROL[2])
	        opt.set_menu(menu)
	        opt.show()

		label0 = gobject.new(gtk.Label, label="to OUT7. Output voltage will be   ")
		label0.show()

		adj = gtk.Adjustment(EXT_CONTROL[3]/16383., -2.0, 2.0, .1, 1, 0)
		spinner = gtk.SpinButton(adj, 0, 0)
		spinner.set_digits(1)
		spinner.set_snap_to_ticks(True)
		spinner.set_wrap(False)
		spinner.set_numeric(True)
		spinner.set_update_policy(gtk.UPDATE_IF_VALID)
		spinner.connect('value-changed', ext_FB_control, 0, "ext_min")
		spinner.show()

		label1 = gobject.new(gtk.Label, label="   from ")
		label1.show()

		label2 = gobject.new(gtk.Label, label="V (min/on)  to")
		label2.show()

		adj = gtk.Adjustment(EXT_CONTROL[4]/16383., -2.0, 2.0, .1, 1, 0)
		spinner2 = gtk.SpinButton(adj, 0, 0)
		spinner2.set_digits(1)
		spinner2.set_snap_to_ticks(True)
		spinner2.set_wrap(False)
		spinner2.set_numeric(True)
		spinner2.set_update_policy(gtk.UPDATE_IF_VALID)
		spinner2.connect('value-changed', ext_FB_control, 0, "ext_max")
		spinner2.show()

		label3 = gobject.new(gtk.Label, label="V (max/off)      ")
		label3.show()

		box3 = gobject.new(gtk.HBox())
		hbox.pack_start(box3)
		box3.show()		
		box3.pack_start(check_button)
	        box3.pack_start(opt)
		box3.pack_start(label0)

		box3 = gobject.new(gtk.HBox())
		hbox.pack_start(box3)
		box3.show()		
		box3.pack_start(label1)
		box3.pack_start(spinner)
		box3.pack_start(label2)
		box3.pack_start(spinner2)
		box3.pack_start(label3)

		separator = gobject.new(gtk.HSeparator())
		box1.pack_start(separator, expand=False)
		separator.show()

		status = gobject.new(gtk.Label, label="---")
		gobject.timeout_add (updaterate, check_dsp_status, status.set_text)
		box1.pack_start(status)
		status.show()

		status = gobject.new(gtk.Label, label="---")
		gobject.timeout_add (updaterate, check_dsp_feedback, status.set_text)
		box1.pack_start(status)
		status.show()

		separator = gobject.new(gtk.HSeparator())
		box1.pack_start(separator, expand=False)
		separator.show()

		status = gobject.new(gtk.Label, label="---")
		gobject.timeout_add (updaterate, check_dsp_scan, status.set_text)
		box1.pack_start(status)
		status.show()

		separator = gobject.new(gtk.HSeparator())
		box1.pack_start(separator, expand=False)
		separator.show()

		box2 = gobject.new(gtk.VBox(spacing=10))
		box2.set_border_width(10)
		box1.pack_start(box2, expand=False)
		box2.show()
		
		button = gtk.Button(stock='gtk-close')
		button.connect("clicked", lambda w: win.hide())
		box2.pack_start(button)
		button.set_flags(gtk.CAN_DEFAULT)
		button.grab_default()
		button.show()
	wins[name].show()

# create ratemeter dialog
def create_ratemeter_app(_button):
	name = "SR DSP CR-RateMeter"
	if not wins.has_key(name):
		win = gobject.new(gtk.Window,
				  type=gtk.WINDOW_TOPLEVEL,
				  title=name,
				  allow_grow=False,
				  allow_shrink=False,
				  border_width=10)
		wins[name] = win
		win.connect("delete_event", delete_event)
		

		box1 = gobject.new(gtk.VBox(spacing=5))
		win.add(box1)
		box1.show()

                tr = 0

		table = gtk.Table(4, 2)
		table.set_row_spacings(5)
		table.set_col_spacings(5)
		box1.pack_start (table)
		table.show()

		lab = gobject.new(gtk.Label, label="Gatetime C1 [ms]:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_GTm1 = gtk.Entry()
		entry_GTm1.set_text("100")
		table.attach(entry_GTm1, 1, 2, tr, tr+1)
		entry_GTm1.show()

		lab = gobject.new(gtk.Label, label="C2 [ms]:")
		lab.show ()
		table.attach(lab, 2, 3, tr, tr+1)
		lab = gobject.new(gtk.Label, label="gate C2 max:")
		lab.show ()
		table.attach(lab, 2, 3, tr+1, tr+2)
		lab = gobject.new(gtk.Label, label="873ms")
		lab.show ()
		table.attach(lab, 3, 4, tr+1, tr+2)

		entry_GTm2 = gtk.Entry()
		entry_GTm2.set_text("20")
		table.attach(entry_GTm2, 3, 4, tr, tr+1)
		entry_GTm2.show()

		tr = tr+1

		lab = gobject.new(gtk.Label, label="PeakHold [ms]:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_holdTm = gtk.Entry()
		entry_holdTm.set_text("0")
		table.attach(entry_holdTm, 1, 2, tr, tr+1)
		entry_holdTm.show()

		tr = tr+1


		check_button = gtk.CheckButton("Enable RateMeter")
		if CR_COUNTER[13] and CR_COUNTER[0]:
			check_button.set_active(True)
		check_button.connect('toggled', enable_ratemeter, entry_GTm1.get_text, entry_GTm2.get_text, entry_holdTm.get_text)
		box1.pack_start(check_button)
		check_button.show()


		separator = gobject.new(gtk.HSeparator())
		box1.pack_start(separator, expand=False)
		separator.show()

		c1 = gobject.new(gtk.Label, label="-CNT1- ---")
		c1.set_use_markup(True)
		box1.pack_start(c1)
		c1.show()

		c2 = gobject.new(gtk.Label, label="-CNT2- ---")
		c2.set_use_markup(True)
		box1.pack_start(c2)
		c2.show()

		gobject.timeout_add (updaterate, check_dsp_ratemeter, c1.set_markup, c2.set_markup)

		status = gobject.new(gtk.Label, label="-Status- ---")
		gobject.timeout_add (updaterate, check_dsp_status, status.set_text)
		box1.pack_start(status)
		status.show()

		separator = gobject.new(gtk.HSeparator())
		box1.pack_start(separator, expand=False)
		separator.show()

		box2 = gobject.new(gtk.VBox(spacing=10))
		box2.set_border_width(10)
		box1.pack_start(box2, expand=False)
		box2.show()
		
		button = gtk.Button(stock='gtk-close')
		button.connect("clicked", lambda w: win.hide())
		box2.pack_start(button)
		button.set_flags(gtk.CAN_DEFAULT)
		button.grab_default()
		button.show()
	wins[name].show()


# build offset editor dialog
def create_offset_edit(_button):
	global analog_offset
	global analog_out_offset
	global forceupdate
	name="A810 Offset Control" + "[" + sr_dev_path + "]"
	address=magic[i_analog]+2*16
	if not wins.has_key(name):
		win = gobject.new(gtk.Window,
				  type=gtk.WINDOW_TOPLEVEL,
				  title=name,
				  allow_grow=True,
				  allow_shrink=True,
				  border_width=20)
		wins[name] = win
		win.connect("delete_event", delete_event)
		
		box1 = gobject.new(gtk.VBox())
		win.add(box1)

		box2 = gobject.new(gtk.VBox(spacing=0))
		box2.set_border_width(10)
		box1.pack_start(box2)

		table = gtk.Table (6, 17)
                table.set_row_spacings(5)
                table.set_col_spacings(4)
                box2.pack_start(table, False, False, 0)
     
		lab = gobject.new(gtk.Label, label="AIC-in Offset Level [mV]")
		table.attach (lab, 1, 2, 0, 1)

		lab = gobject.new(gtk.Label, label="Reading")
		lab.set_alignment(1.0, 0.5)
		table.attach (lab, 2, 3, 0, 1, gtk.FILL | gtk.EXPAND )

		lab = gobject.new(gtk.Label, label="AIC-out Offset Level [mV]")
		table.attach (lab, 5, 6, 0, 1)

		lab = gobject.new(gtk.Label, label="Reading")
		lab.set_alignment(1.0, 0.5)
		table.attach (lab, 6, 7, 0, 1, gtk.FILL | gtk.EXPAND )

		level_max = 10000. # mV
		level_fac = 0.305185095 # mV/Bit
		forceupdate = 255

		# create scales for level and gain for all taps...
		for tap in range(0,8):
			offset=0
			if tap < 8:
				offset=analog_offset[tap]	
			adjustment = gtk.Adjustment (value=offset*level_fac, lower=-level_max, upper=level_max, step_incr=0.1, page_incr=2, page_size=50)
			adjustment.set_data("AIC_offset_address", address+2*tap) 
			adjustment.connect("value-changed", AIC_in_offset_update)

			scale = gobject.new(gtk.HScale, adjustment=adjustment)
			scale.set_size_request(250, 30)
			scale.set_update_policy(gtk.UPDATE_DELAYED)
			scale.set_digits(1)
			scale.set_draw_value(True)

			lab = gobject.new(gtk.Label, label="AIC_"+str(tap))
			table.attach (lab, 0, 1, tap+1, tap+2)
			table.attach (scale, 1, 2, tap+1, tap+2)
			
			labin = gobject.new (gtk.Label, label="+####.# mV")
			labin.set_alignment(1.0, 0.5)
			table.attach (labin, 2, 3, tap+1, tap+2, gtk.FILL | gtk.EXPAND )

			labout = gobject.new (gtk.Label, label="+####.# mV")
			labout.set_alignment(1.0, 0.5)
			table.attach (labout, 6, 7, tap+1, tap+2, gtk.FILL | gtk.EXPAND )

			gobject.timeout_add(updaterate, offset_update, labin.set_text, labout.set_text, adjustment.set_value, tap)
						
		# create scales for level and gain for all taps...
		for tap in range(0,8):
			if tap >= 3 and tap != 6 and tap != 7:
				continue
			adjustment = gtk.Adjustment (value=analog_out_offset[tap]*level_fac, lower=-level_max, upper=level_max, step_incr=0.1, page_incr=2, page_size=50)
			adjustment.set_data("AIC_offset_address", address+2*(8+2*2*8+8+6*32+2*128+tap))
			adjustment.connect("value-changed", AIC_out_offset_update)

			scale = gobject.new(gtk.HScale, adjustment=adjustment)
			scale.set_size_request(250, 30)
			scale.set_update_policy(gtk.UPDATE_DELAYED)
			scale.set_digits(1)
			scale.set_draw_value(True)

#			lab = gobject.new(gtk.Label, label="AICout"+str(tap))
#			table.attach (lab, 4, 5, tap+1, tap+2)
			table.attach (scale, 5, 6, tap+1, tap+2)
			
						
		# add save/restore to/from file options

		separator = gobject.new(gtk.HSeparator())
		box1.pack_start(separator, expand=False)
		separator.show()

		check_button = gtk.CheckButton("Offset Compensation (automatic offset compensation \nneeds Zero volt or open inputs at DSP startup!)")
		if SPM_STATEMACHINE[2] & 0x0010:
			check_button.set_active(True)
		check_button.connect('toggled', toggle_flag, 0x0010)
		gobject.timeout_add (updaterate, check_update, check_button.set_active, 0x0010)
		box1.pack_start(check_button)

		check_button = gtk.CheckButton("Differential Inputs AIC5 - AIC7 mapped to AIC5")
		if SPM_STATEMACHINE[2] & 0x0800:
			check_button.set_active(True)
		check_button.connect('toggled', toggle_flag, 0x0800)
		gobject.timeout_add (updaterate, check_update, check_button.set_active, 0x0800)
		box1.pack_start(check_button)


		box2 = gobject.new(gtk.HBox(spacing=0))
		box2.set_border_width(10)
		box1.pack_start(box2, expand=False)

		button = gtk.Button(label="Load Settings")
		button.connect("clicked", open_file_dialog)
		box2.pack_start(button)

		button = gtk.Button(label="Save Settings")
		button.connect("clicked", save_file_dialog)
		box2.pack_start(button)
		
		button = gtk.Button(label="AIC-in Auto Offset")
		button.connect("clicked", force_offset_recalibration, 0)
		box2.pack_start(button)
		
		separator = gobject.new(gtk.HSeparator())
		box1.pack_start(separator, expand=False)

		box2 = gobject.new(gtk.VBox(spacing=10))
		box2.set_border_width(10)
		box1.pack_start(box2, expand=False)
		
		button = gtk.Button(stock='gtk-close')
		button.connect("clicked", lambda w: win.hide())
		box2.pack_start(button)
		button.set_flags(gtk.CAN_DEFAULT)
		button.grab_default()

	wins[name].show_all()

def refresh(_button):
	global forceupdate
	forceupdate = 255
	return 1

# updates the right side of the offset dialog
def offset_update(_labin,_labout,_adj,_tap):
	global pc
	global offset_mutex
	global analog_offset
	global AIC_in_buffer
	global AIC_out_buffer
	global forceupdate
	global lvdt_xy
	sumlength = 16384.
	gainfac0db  = 1.0151 # -0.1038V Offset
	gainfac6db  = 2.0217 # -0.1970
	gainfac12db = 4.0424 # -0.3845
	gainfac18db = 8.0910 # -0.7620
	level_fac = 0.305185095 # mV/Bit
	diff=(AIC_in_buffer[_tap])*level_fac
	
	outmon=0.;
	if _tap < 8:
		diff=(AIC_in_buffer[_tap])*level_fac
		outmon=(AIC_out_buffer[_tap]+analog_out_offset[_tap])*level_fac
	else:
		diff=(AIC_in_buffer[_tap+8])*level_fac
		outmon=(AIC_out_buffer[_tap+8])*level_fac/5

#	slowdiff=(analog_offset[8+_tap])*level_fac/sumlength
#	slowdiffX=(analog_offset[8+6])*level_fac/sumlength
#	slowdiffY=(analog_offset[8+7])*level_fac/sumlength
# mirroring raw only:

	if _tap < 8:
		slowdiff=(AIC_in_buffer[_tap])*level_fac
		slowdiffX=(AIC_in_buffer[6])*level_fac
		slowdiffY=(AIC_in_buffer[7])*level_fac

# settling speedup
                if abs (AIC_slow[_tap] - slowdiff) > 3.5:
			AIC_slow[_tap] = slowdiff

		new = 0.01;	
		AIC_slow[_tap] *= 1.-new; 
		AIC_slow[_tap] += new*slowdiff; 

	_labin("%.1f mV" %diff)
	_labout("%.1f mV" %outmon)
# 4.988
        LVDT_V2umX = 1.63316*0.97841586
        LVDT_V2umY = 1.63316*0.79801383*1.006
	lvdt_xy[0] = AIC_slow[6]/LVDT_V2umX-26.5 # Calib 20050127
	lvdt_xy[1] = AIC_slow[7]/LVDT_V2umY-30.3 # Calib 20050127
#	_labin("%.1f mV %10.3f mV %10.3f mV %10.3f um" %(diff,slowdiff/gainfac0db,AIC_slow[_tap], (AIC_slow[_tap]/LVDT_V2umX)))
	if (_tap == 7):
		pc += 1
		if (pc > printrate/updaterate):
			pc = 0
			f = open ('Stencil_Stage_LVDT_XY_reading', 'w')
			f.write ('# Stencil Stage LVDT XY reading [um]:\n' + str('%+7.2f'%lvdt_xy[0]) + ' ' + str('%+7.2f'%lvdt_xy[1]) + '\n')
			f.close ()
		
	if (forceupdate >> _tap) & 1:
		_adj(analog_offset[_tap]*level_fac)
		forceupdate = forceupdate & (255 - (1 << _tap))
	return 1

def check_update(_set, _mask):
	if SPM_STATEMACHINE[2] & _mask:
		_set (True)
	else:
		_set (False)
	return 1

def check_dsp_status(_set):
	_set ("DataProcessTicks: %05d" %SPM_STATEMACHINE[8] + "  Idle: %05d" %SPM_STATEMACHINE[9] )
#	_set ("DataProcessTicks: %05d" %SPM_STATEMACHINE[8] + "  Idle: %05d" %SPM_STATEMACHINE[9] + "  DSP Load: %.1f" %(100.*SPM_STATEMACHINE[8]/(SPM_STATEMACHINE[8]+SPM_STATEMACHINE[9])) + "  Peak: %05d" %SPM_STATEMACHINE[10])
	_set ("DataProcessTicks: %05d" %SPM_STATEMACHINE[8] + "  Idle: %05d" %SPM_STATEMACHINE[9] + "  DSP Load: %.1f" %(100.*SPM_STATEMACHINE[8]/(SPM_STATEMACHINE[8]+SPM_STATEMACHINE[9])) + "  Peak: %.1f" %(100.*SPM_STATEMACHINE[10]/(SPM_STATEMACHINE[10]+SPM_STATEMACHINE[11])) + " S:%05d" %(SPM_STATEMACHINE[8]+SPM_STATEMACHINE[9]))
	return 1

def check_dsp_feedback(_set):
	global AIC_in_buffer
	if SPM_FEEDBACK[7] > 0:
		f0 = -math.log (SPM_FEEDBACK[7]/32767.) / (2.*math.pi/75000.)
	else:
		f0 = 75000.
	_set ("FB:%d" %(SPM_STATEMACHINE[2]&4) + " CP=%6.4f" %(SPM_FEEDBACK[0]/32767.) + " CI=%6.4f" %(SPM_FEEDBACK[1]/32767.) + " set=%05d" %SPM_FEEDBACK[2] + " soll=%05d" %SPM_FEEDBACK[9] + " ist=%05d" %SPM_FEEDBACK[10] + "\ndelta=%05d" %SPM_FEEDBACK[10] + " i_sum=%010d" %SPM_FEEDBACK[13] + " Z=%05d" %SPM_FEEDBACK[14] + "\nIIR_ca=%6.4f" %(SPM_FEEDBACK[3]/32767.) + " IIR_cb_Ic=%6.4f" %(SPM_FEEDBACK[4]/32767.) + " I_coss=%05d" %SPM_FEEDBACK[5] + " I_offset=%05d" %SPM_FEEDBACK[6]  + " q_factor_Sat=%6.4f" %(SPM_FEEDBACK[7]/32767.)  + " q*_factor=%6.4f" %(SPM_FEEDBACK[16]/32767.) + " f0=%8.1f" %f0 + " Hz" + "\nI_fbw=%05d" %SPM_FEEDBACK[15] + " I_iir=%010d" %AIC_in_buffer[5] + " zxx=%010d" %SPM_FEEDBACK[16] + " zyy=%010d" %SPM_FEEDBACK[17] )
	return 1

def check_dsp_scan(_set):
	global analog
	vx = SPM_SCAN[ii_scan_cfs_dx]*75000./(65536.*32767.)*10.0
	xs = SPM_SCAN[ii_scan_cfs_dx]/(65536.*32767.)*SPM_SCAN[ii_scan_dnx]*10.0
	_set ("Scan-Control:\n"
	      + " nx_pre:%03d" %(SPM_SCAN[ii_scan_nx_pre])+ ", fastreturn:%03d" %(SPM_SCAN[ii_scan_fast_return])
	      + " nx:%05d" %(SPM_SCAN[ii_scan_nx]) + ", ny:%05d" %(SPM_SCAN[ii_scan_ny]) + "\n"
	      + " fs_dx:%d" %(SPM_SCAN[ii_scan_fs_dx]) + ", y:%d" %(SPM_SCAN[ii_scan_fs_dy]) + "\n"
	      + " cfs_dx:%d" %(SPM_SCAN[ii_scan_cfs_dx]) + ",y:%d" %(SPM_SCAN[ii_scan_cfs_dy]) + "\n"
	      + " dnx:%d" %(SPM_SCAN[ii_scan_dnx]) + ", y:%d" %(SPM_SCAN[ii_scan_dny]) + "\n"
	      + " X speed calc [V/s]: %f" %vx + "  in A @ 533 A/V G1: %f\n" %(vx*533)
	      + " X step size [V]...: %f" %xs + "  in A @ 533 A/V G1: %f\n" %(xs*533)
	      + " num_steps_mmove_xy:%d" %(SPM_SCAN[ii_scan_num_steps_move_xy]) + "\n"
	      + " fm_dx:%d" %(SPM_SCAN[ii_scan_fm_dx]) + ",y:%d" %(SPM_SCAN[ii_scan_fm_dy]) + ",zxy:%d" %(SPM_SCAN[ii_scan_fm_dzxy])
	      + " fm_dzy:%d" %(SPM_SCAN[ii_scan_fm_dz0_xy_vecX]) + ",y:%d" %(SPM_SCAN[ii_scan_fm_dz0_xy_vecY]) + "\n"
	      + " SRCSXP: %08x" %(SPM_SCAN[ii_scan_srcs_xp]) + " XM: %08x" %(SPM_SCAN[ii_scan_srcs_xm]) + " XP2 %08x" %(SPM_SCAN[ii_scan_srcs_2nd_xp])
	      + " XM2 %08x" %(SPM_SCAN[ii_scan_srcs_2nd_xm]) + " 2ndOffxp %d" %(SPM_SCAN[ii_scan_Zoff_2nd_xp]) + ", xm %d\n" %(SPM_SCAN[ii_scan_Zoff_2nd_xm])
	      + " Sc0-Xpos:%d" %(SPM_SCAN[ii_scan_xyz_vecX]) + ",Y:%d" %(SPM_SCAN[ii_scan_xyz_vecY]) + ",Z:%d\n" %(SPM_SCAN[ii_scan_xyz_vecZ])
	      + " Rot-Xpos:%d" %(SPM_SCAN[ii_scan_xy_r_vecX]) + ",Y:%d" %(SPM_SCAN[ii_scan_xy_r_vecY])
	      + "\nMove-Control:\n"
	      + "M: Xnew:%d" %(SPM_MOVE[ii_move_xy_new_vecX]) + ",Ynew:%d" %(SPM_MOVE[ii_move_xy_new_vecY]) + ",fdx:%d" %(SPM_MOVE[ii_move_f_d_xyz_vecX]) + ",fdy:%d\n" %(SPM_MOVE[ii_move_f_d_xyz_vecY])
	      + "M: Xpos:%d" %(SPM_MOVE[ii_move_xyz_vecX]) + ",Ypos:%d" %(SPM_MOVE[ii_move_xyz_vecY]) + ",steps:%d" %(SPM_MOVE[ii_move_num_steps]) + ",pflg:%d\n" %(SPM_MOVE[ii_move_pflg])
	      + "\nAnalog-Watches:\n"
	      + "A: in0: %06d" %(analog[0]) + " in1: %06d" %(analog[1]) + " in2: %06d" %(analog[2]) + " in3: %06d\n" %(analog[3])
	      + "A: in4: %06d" %(analog[4]) + " FB5: %06d" %(analog[5]) + " in6: %06d" %(analog[6]) + " in7: %06d\n" %(analog[7])
	      + "A: Xo0: %06d" %(analog[8]) + " Yo0: %06d" %(analog[9]) + " Zo0: %06d\n" %(analog[10])
	      + "A: Xsc: %06d" %(analog[11]) + " Ysc: %06d" %(analog[12]) + " Zsc: %06d\n" %(analog[13])
	      + "A: Bias %06d" %(analog[14]) + " Out7 %06d\n" %(analog[15])
	      + "W16: %08d" %(WATCH[0]) + " %08d" %(WATCH[1]) + " %08d" %(WATCH[2]) + " %08d" %(WATCH[3]) + " %08d" %(WATCH[4]) + " %08d\n" %(WATCH[5])
	      + "\nMixer/Feedback-Control:\n"
	      + "LN_SET0: %06d" %(MIXER_FB[ii_feedabck_mixer_setpoint_log0]) + " SET0 %06d" %(MIXER_FB[ii_feedabck_mixer_setpoint0])
#	      + "W32: %08d" %(WATCHL16) + " %08d" %(WATCHL17) + " %08d" %(WATCH[16+2]) + " %08d" %(WATCH[16+3])
	      )
	return 1

def check_dsp_ratemeter(_c1set, _c2set):
	global gatetime_ms1
	global gatetime_ms2
	global counts0
	global counts1
	global counts0h
	global counts1h
	cps0 = float(counts0)/gatetime_ms1/1e-3
	cps1 = float(counts1)/gatetime_ms2/1e-3
	if cps0 > 2e6:
		spantag = "<span size=\"24000\" font_family=\"monospace\" color=\"#ff0000\">"
	else:
		spantag = "<span size=\"24000\" font_family=\"monospace\">"
		
	if gatetime_ms1 > 2000:
		_c1set (spantag + "<b>C1:%12.2f Hz" %cps0 + "  %010d Cnt" %(counts0) + "  Pk: %010d</b></span>" %(counts0h))
	else:
		_c1set (spantag + "<b>C1:%12.0f Hz" %cps0 + "  %010d Cnt" %(counts0) + "  Pk: %010d</b></span>" %(counts0h))

	if gatetime_ms1 > 2000:
		_c2set (spantag + "<b>C2:%12.2f Hz" %cps1 + "  %010d Cnt" %(counts1) + "  Pk: %010d</b></span>" %(counts1h))
	else:
		_c2set (spantag + "<b>C2:%12.0f Hz" %cps1 + "  %010d Cnt" %(counts1) + "  Pk: %010d</b></span>" %(counts1h))

	return 1

# create info dialog
def create_info(_button):
	address = magic[0]
	name = "General SRanger DSP code info"
	if not wins.has_key(name):
		win = gobject.new(gtk.Window,
				  type=gtk.WINDOW_TOPLEVEL,
				  title=name,
				  allow_grow=True,
				  allow_shrink=True,
				  border_width=10)
		wins[name] = win
		win.set_size_request(300, 220)
		win.connect("delete_event", delete_event)

		box1 = gobject.new(gtk.VBox())
		win.add(box1)
		box1.show()

		box2 = gobject.new(gtk.VBox(spacing=0))
		box2.set_border_width(10)
		box1.pack_start(box2)
		box2.show()

		lab = gobject.new(gtk.Label, label="Magic ID: %08X" %magic[0])
		box2.pack_start(lab)
		lab.show()

		lab = gobject.new(gtk.Label, label="SPM Version: %08X" %magic[1])
		box2.pack_start(lab)
		lab.show()

		lab = gobject.new(gtk.Label, label="Software ID: %08X" %magic[4])
		box2.pack_start(lab)
		lab.show()

		lab = gobject.new(gtk.Label, label="DSP Code Released: " + "%02X." %(magic[3] & 0xFF) + "%02X." %((magic[3] >> 8) & 0xFF) +"%X " %magic[2])
		box2.pack_start(lab)
		lab.show()

		separator = gobject.new(gtk.HSeparator())
		box1.pack_start(separator, expand=False)
		separator.show()

		separator = gobject.new(gtk.HSeparator())
		box1.pack_start(separator, expand=False)
		separator.show()

		box2 = gobject.new(gtk.VBox(spacing=10))
		box2.set_border_width(10)
		box1.pack_start(box2, expand=False)
		box2.show()
		
		button = gtk.Button(stock='gtk-close')
		button.connect("clicked", lambda w: win.hide())
		box2.pack_start(button)
		button.set_flags(gtk.CAN_DEFAULT)
		button.grab_default()
		button.show()
	wins[name].show()

# create Coolrunner IO approach dialog
def create_coolrunner_app(_button):
	address = magic[0]
	name = "MK2-A810 GPIO Control"

	if not wins.has_key(name):
		win = gobject.new(gtk.Window,
				  type=gtk.WINDOW_TOPLEVEL,
				  title=name,
				  allow_grow=True,
				  allow_shrink=True,
				  border_width=10)
		wins[name] = win
		win.set_size_request(480, 600)
		win.connect("delete_event", delete_event)

		box1 = gobject.new(gtk.VBox())
		win.add(box1)
		box1.show()

		box2 = gobject.new(gtk.VBox(spacing=0))
		box2.set_border_width(10)
		box1.pack_start(box2)
		box2.show()

		lab = gobject.new(gtk.Label, label="-- FPGA GPIO --\n")
		box2.pack_start(lab)
		lab.show()

                tr = 0

		table = gtk.Table(8, 4)
		table.set_row_spacings(5)
		table.set_col_spacings(5)
		box2.pack_start (table)
		table.show()

		statuslab = gobject.new(gtk.Label, label="---")
		box2.pack_start(statuslab)
		statuslab.show()
		statuslab2 = gobject.new(gtk.Label, label="---")
		box2.pack_start(statuslab2)
		statuslab2.show()

		lab = gobject.new(gtk.Label, label="GPIO_WRITE >XY<:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_Out0 = gtk.Entry()
		entry_Out0.set_text("0")
		table.attach(entry_Out0, 1, 2, tr, tr+1)
		entry_Out0.show()

		button = gtk.Button("W")
		button.connect("clicked", CoolRunnerIOWrite, entry_Out0, statuslab)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()

		tr = tr+1

		lab = gobject.new(gtk.Label, label="GPIO WRITE >Rot<:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_Out1 = gtk.Entry()
		entry_Out1.set_text("0")
		table.attach(entry_Out1, 1, 2, tr, tr+1)
		entry_Out1.show()

		button = gtk.Button("W")
		button.connect("clicked", CoolRunnerIOWrite, entry_Out1, statuslab)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()

		tr = tr+1

		lab = gobject.new(gtk.Label, label="GPIO WRITE >XX<:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_Out2 = gtk.Entry()
		entry_Out2.set_text("0")
		table.attach(entry_Out2, 1, 2, tr, tr+1)

		entry_Out2.show()

		button = gtk.Button("W")
		button.connect("clicked", CoolRunnerIOWrite, entry_Out2, statuslab)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()

		tr = tr+1

		lab = gobject.new(gtk.Label, label="GPIO Direction:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_OutC = gtk.Entry()
		entry_OutC.set_text("0")
		table.attach(entry_OutC, 1, 2, tr, tr+1)
		entry_OutC.show()

		button = gtk.Button("W")
		button.connect("clicked", CoolRunnerIOWriteDirection, entry_OutC, statuslab)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()
		tr = tr+1

		lab = gobject.new(gtk.Label, label="GPIO READING:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_In0 = gtk.Entry()
		entry_In0.set_text("XXXX")
		table.attach(entry_In0, 1, 2, tr, tr+1)
		entry_In0.show()

		button = gtk.Button("R")
		button.connect("clicked", CoolRunnerIOReadE16, entry_In0, statuslab)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()

		tr = tr+1

		lab = gobject.new(gtk.Label, label="Puls W[ms]/T[ms]/#/bOn/bOff/bR:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)


		hbox = gobject.new(gtk.HBox(spacing=1))
		hbox.show()
		table.attach(hbox, 1, 2, tr, tr+1)

		entry_W = gtk.Entry()
		entry_W.set_text("5")
		entry_W.set_size_request(30, -1)
		entry_W.show()
		hbox.pack_start(entry_W)

		entry_T = gtk.Entry()
		entry_T.set_text("10")
		entry_T.set_size_request(30, -1)
		entry_T.show()
		hbox.pack_start(entry_T)

		entry_N = gtk.Entry()
		entry_N.set_text("1")
		entry_N.set_size_request(30, -1)
		entry_N.show()
		hbox.pack_start(entry_N)

		entry_bOn = gtk.Entry()
		entry_bOn.set_text("0")
		entry_bOn.set_size_request(30, -1)
		entry_bOn.show()
		hbox.pack_start(entry_bOn)

		entry_bOff = gtk.Entry()
		entry_bOff.set_text("1")
		entry_bOff.set_size_request(30, -1)
		entry_bOff.show()
		hbox.pack_start(entry_bOff)

		entry_bR = gtk.Entry()
		entry_bR.set_text("80")
		entry_bR.set_size_request(30, -1)
		entry_bR.show()
		hbox.pack_start(entry_bR)


		button = gtk.Button("Exec")
		button.connect("clicked", CoolRunnerIOpulse, entry_W, entry_T, entry_N, entry_bOn, entry_bOff, entry_bR, statuslab)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()


		tr = tr+1

# Auto Approach Controller

		lab = gobject.new(gtk.Label, label="Py Auto Approach:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)


		hbox = gobject.new(gtk.HBox(spacing=1))
		hbox.show()
		table.attach(hbox, 1, 2, tr, tr+1)

##		lab = gobject.new(gtk.Label, label="-CI:")
##		lab.show ()
##		hbox.pack_start(lab)

##		entry_negCI = gtk.Entry()
##		entry_negCI.set_text("0.01")
##		entry_negCI.set_size_request(30, -1)
##		entry_negCI.show()
##		hbox.pack_start(entry_negCI)

		lab = gobject.new(gtk.Label, label="%Span:")
		lab.show ()
		hbox.pack_start(lab)

		entry_pSpn = gtk.Entry()
		entry_pSpn.set_text("75")
		entry_pSpn.set_size_request(30, -1)
		entry_pSpn.show()
		hbox.pack_start(entry_pSpn)


		button = gtk.Button("Start")
		gobject.timeout_add (50, CoolRunnerAppRun, 0, entry_pSpn, entry_W, entry_T, entry_N, entry_bOn, entry_bOff, entry_bR, statuslab, statuslab2)
		button.connect("clicked", CoolRunnerAppCtrl, 0)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()

		tr = tr+1

		button = gtk.Button("Stop")
		button.connect("clicked", CoolRunnerAppCtrl, -2)
		
		table.attach(button, 2, 3, tr, tr+1)
		button.show()




		separator = gobject.new(gtk.HSeparator())
		box1.pack_start(separator, expand=False)
		separator.show()

		box2 = gobject.new(gtk.VBox(spacing=10))
		box2.set_border_width(10)
		box1.pack_start(box2, expand=False)
		box2.show()
		
		button = gtk.Button(stock='gtk-close')
		button.connect("clicked", lambda w: win.hide())
		box2.pack_start(button)
		button.set_flags(gtk.CAN_DEFAULT)
		button.grab_default()
		button.show()
	wins[name].show()


# Stage Direction (Omicron Slider box) via Dig-IO
def set_direction(mask):
	global stage_direction
	TRIG = 0x80
	stage_direction = mask
	CoolRunnerIOWritePort (mask+TRIG)

	
# toggle button pressed, readjust direction and on/off
def toggle_direction(w, mask, alldir, indicators):
	global stage_direction
	global xyz_count

	TRIG = 0x80

	for c in alldir:
		if w != c:
			c.set_active (0)
	if w.get_active():
		stage_direction = mask
		CoolRunnerIOWritePort (mask+TRIG)
	else:
		stage_direction = 0
		CoolRunnerIOWritePort (TRIG)
#		time.sleep(0.05)
		CoolRunnerIOWritePort (0)

	i = 0
	for ind in indicators:
		ind.set_sensitive (stage_direction == 0)
		xyz_count [i] = int (ind.get_text ())
		ind.set_text ("%05d" %xyz_count[i])
		i += 1

# move stage tp absolut position with LVDT feedback, P-Controller
def stage_goto (w, flag, lut, indicators, set_pos):
	global stage_direction
	global xyz_count
	global goto_mode

	epsilon = 1.8

	if flag > 0:
		goto_mode = flag
		return 1

	if flag < 0:
		goto_mode = 0
		set_direction (0)
		print '--> Goto Stopped <--'
		return 1

	if goto_mode == 0:
		return 1

	print 'X>', set_pos[0].get_text (), '<, Y>', set_pos[0].get_text (), '<'
	lvdt_set[0] = float (set_pos[0].get_text ())
	lvdt_set[1] = float (set_pos[1].get_text ())
#	set_pos[0].set_text ("%08.3f" %lvdt_set[0])
#	set_pos[1].set_text ("%08.3f" %lvdt_set[1])
		
	dX = lvdt_set[0] - lvdt_xy[0]
	dY = lvdt_set[1] - lvdt_xy[1]
		
	if (abs (dX) < epsilon) and (abs (dY) < epsilon):
		set_direction (0)
		print '--> Goto OK <--'
		goto_mode = 0
		return 1
			
	if goto_mode == 1:

		XP = 0x02
		XM = 0x01
		YP = 0x08
		YM = 0x04
		
		print '-- Auto Goto --'
		print 'dX: %.2f'%dX
		print 'dY: %.2f'%dY
		
		goto_steps = [0.,0.]
		goto_steps[0] = abs(dX)*0.8
		goto_steps[1] = abs(dY)*1.2
		
		if dX > epsilon:
			set_direction (XP)
		else:
			if dX < -epsilon:
				set_direction (XM)
				
			else:
				if dY > epsilon:
					set_direction (YP)
					
				else:
					if dY < -epsilon:
						set_direction (YM)
					else:
						set_direction (0)
						print '--> Goto OK '
						return 1
						
		print '--> Set Dir: ', stage_direction
		time.sleep (0.1)

		print '--> Move Stage '
		move_stage_n (w, lut, goto_steps, indicators)
		
	return 1

# move stage in set directiom, num steps
def move_stage_n (w, lut, steps, indicators):
	global stage_direction
	global xyz_count

	if stage_direction == 0:
		i = 0
		for ind in indicators:
			xyz_count [i] = int (ind.get_text ())
			ind.set_text ("%05d" %xyz_count[i])
			i += 1
		return

	i = lut [stage_direction]/2 
	n = int (steps [i])
		 
	CoolRunnerIOpulse_mask(n, 1.0, stage_direction, 0x80)
		 
	if lut [stage_direction] & 1:
		xyz_count [i] -= n
	else:
		xyz_count [i] += n

	indicators [i].set_text ("%05d" %xyz_count[i])

# move stage in set direction, num steps from num steps entry
def move_stage (w, lut, steps, indicators):
	global stage_direction
	global xyz_count

	if stage_direction == 0:
		i = 0
		for ind in indicators:
			xyz_count [i] = int (ind.get_text ())
			ind.set_text ("%05d" %xyz_count[i])
			i += 1
		return

	i = lut [stage_direction]/2 
	n = int (steps [i].get_text ())
		 
	CoolRunnerIOpulse_mask(n, 1.0, stage_direction, 0x80)
		 
	if lut [stage_direction] & 1:
		xyz_count [i] -= n
	else:
		xyz_count [i] += n

	indicators [i].set_text ("%05d" %xyz_count[i])

# User Program -- Action Controller
def UserActions(lab, lut, steps, indicators):
	global user_action_mode
	global user_action_y
	global user_action_i

	if user_action_i == 0:
		user_action_i = 1

	if user_action_i > 20:
		lab.set_text ("-- User Program Aborted/Done @ #%d --" %user_action_i)
		user_action_i = 0
		user_action_mode = 0
		return 0

	sr = open (sr_dev_path, "rb")
	os.lseek (sr.fileno(), magic[i_AIC_out]+2*3, 1) # Y
	fmt = "<hhh"
	data = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
	sr.close ()
	x=data[0]
	y=data[1]
	z=data[2]

	zlimit = 32000
		
	if user_action_mode == 4:
		user_action_mode = 5
		
	if user_action_mode == 3:
		user_action_mode = 4

	# c) now wait until Z is retracted, then move stage by one "run" hit
	if user_action_mode == 2:
		if z < -zlimit:
			move_stage (0, lut, steps, indicators)
			user_action_mode = 3
			
	# b) Assumes Scaner returns to zero/scan start (Y Offset must be close 0!!!)

	# a) Hysteresis activated by Y-scan crossing Zero and return
	if y < -1000 and user_action_mode == 0:
		user_action_mode = 1


	lab.set_text ("Y: %6d" %y + " Z: %6d" %z + " M: %d" %user_action_mode + " #i: %d" %user_action_i)
	return 1

# User Program setup
def UserProgram(w, lab, lut, steps, indicators):
	global user_action_mode
	global user_action_i

	# cancel running
	if user_action_i > 0:
		user_action_i = 9999
		return 0
	
	user_action_mode = 0
	gobject.timeout_add (500, UserActions, lab, lut, steps, indicators)
	return 1

# Update LVDT position indicators
def lvdt_position_update(_xe, _ye, xrel, yrel, p_zero):
	global lvdt_xy
	global lvdt_zero
	global lvdt_ist
	global analog_offset
	sumlength = 0x1000
	gainfac0db  = 1.0151 # -0.1038V Offset
	gainfac6db  = 2.0217 # -0.1970
	gainfac12db = 4.0424 # -0.3845
	gainfac18db = 8.0910 # -0.7620
	level_fac = 0.305185095 # mV/Bit
	slowdiffX=(analog_offset[8+6])*level_fac/sumlength/gainfac0db
	slowdiffY=(analog_offset[8+7])*level_fac/sumlength/gainfac0db
	lvdt_ist[0] = Xpos = lvdt_xy[0]
	lvdt_ist[1] = Ypos = lvdt_xy[1]
	if xrel():
		Xpos -= lvdt_zero[0]
		p_zero[0].set_sensitive (0)
	else:
		p_zero[0].set_sensitive (1)

	if yrel():
		Ypos -= lvdt_zero[1]
		p_zero[1].set_sensitive (0)
	else:
		p_zero[1].set_sensitive (1)

	_xe("%08.3f um" %Xpos)
	_ye("%08.3f um" %Ypos)
	return 1

def LVDT_zero(button, index, _entry, _rel):
	global lvdt_zero
	global analog_offset
	if _rel ():
		lvdt_zero[index] = lvdt_ist[index]
	else:
		lvdt_zero[index] = float (_entry.get_text ())
	_entry.set_text ("%08.3f um" %lvdt_zero[index])

# Stencil stage control app: LVDT Stage w goto, Slider, Auto App
def create_coolrunner_lvdt_stage_control_app(_button):
	global stage_direction
	global xyz_count
	global xyz_steps

	name = "SR Coolrunner/LVDT Stage Control"

	# on CoolRunner Out2 via Optocoupler directionselect and triggerpuls are connected:
	XP = 0x02
	XM = 0x01
	YP = 0x08
	YM = 0x04
	ZM = 0x10
	ZP = 0x20
	TRIG = 0x80

	lut = { XP: 0, XM: 1, YP: 2, YM: 3, ZP: 4, ZM: 5 }

	position_ist = []
	position_set = []
	position_zero = []

	dir_chk = []
	indicators = []
	steps = []

	if not wins.has_key(name):
		win = gobject.new(gtk.Window,
				  type=gtk.WINDOW_TOPLEVEL,
				  title=name,
				  allow_grow=True,
				  allow_shrink=True,
				  border_width=10)
		wins[name] = win
		win.set_size_request(500, 470)
		win.connect("delete_event", delete_event)

		box1 = gobject.new(gtk.VBox())
		win.add(box1)
		box1.show()

		box2 = gobject.new(gtk.VBox(spacing=0))
		box2.set_border_width(10)
		box1.pack_start(box2)
		box2.show()



		lab = gobject.new(gtk.Label, label="LVDT Stage Stage Control\n")
		box2.pack_start(lab)
		lab.show()

## X/Y/Z Slider Control ##

		table = gtk.Table(4, 4)
		table.set_row_spacings(5)
		table.set_col_spacings(1)
		box2.pack_start (table)
		table.show()

		r = 0
		c = 0
		Xrel = check_button = gtk.CheckButton("rel. X")
		check_button.show()
		table.attach (check_button, c, c+1, r, r+1)
		r += 1
		Yrel = check_button = gtk.CheckButton("rel Y")
		check_button.show()
		table.attach (check_button, c, c+1, r, r+1)



		r = 0
		c = 1
		position_ist.append (gtk.Entry ())
		position_ist[0].set_text("---LVDT")
		position_ist[0].set_size_request(50, -1)
		position_ist[0].show ()
		table.attach (position_ist[0], c, c+1, r, r+1)
		r += 1
		position_ist.append (gtk.Entry ())
		position_ist[1].set_text("---LVDT")
		position_ist[1].set_size_request(50, -1)
		position_ist[1].show ()
		table.attach (position_ist[1], c, c+1, r, r+1)

		r = 0
		c = 2
		position_set.append (gtk.Entry ())
		position_set[0].set_text("0.0")
		position_set[0].set_size_request(50, -1)
		position_set[0].show ()
		table.attach (position_set[0], c, c+1, r, r+1)
		r += 1
		position_set.append (gtk.Entry ())
		position_set[1].set_text("0.0")
		position_set[1].set_size_request(50, -1)
		position_set[1].show ()
		table.attach (position_set[1], c, c+1, r, r+1)

		r = 0
		c = 3
		position_zero.append (gtk.Entry ())
		position_zero[0].set_text("---not set---")
		position_zero[0].set_size_request(50, -1)
		position_zero[0].show ()
		table.attach (position_zero[0], c, c+1, r, r+1)
		r += 1
		position_zero.append (gtk.Entry ())
		position_zero[1].set_text("---not set---")
		position_zero[1].set_size_request(50, -1)
		position_zero[1].show ()
		table.attach (position_zero[1], c, c+1, r, r+1)

		r = 0
		c = 4
		button = gtk.Button("Z")
		button.show ()
		table.attach (button, c, c+1, r, r+1)
		button.connect("clicked", LVDT_zero, 0, position_zero[0], Xrel.get_active)
		r += 1
		button = gtk.Button("Z")
		button.show ()
		table.attach (button, c, c+1, r, r+1)
		button.connect("clicked", LVDT_zero, 1, position_zero[1], Yrel.get_active)


		gobject.timeout_add (500, lvdt_position_update, position_ist[0].set_text, position_ist[1].set_text, Xrel.get_active, Yrel.get_active, position_zero)

		table = gtk.Table(4, 1)
		table.set_row_spacings(5)
		table.set_col_spacings(5)
		box2.pack_start (table)
		table.show()

		c = 0
		r = 0
		button = gtk.Button("Goto Set Position!")
		button.show ()
		table.attach (button, c, c+1, r, r+1)
		gobject.timeout_add (500, stage_goto, 0, 0, lut, indicators, position_set)
		button.connect("clicked", stage_goto, 1, lut, indicators, position_set)

		c += 1
		button = gtk.Button("STOP !!")
		button.show ()
		table.attach (button, c, c+1, r, r+1)
		button.connect("clicked", stage_goto, -1, lut, indicators, position_set)



## Manual Steps
		separator = gobject.new(gtk.HSeparator())
		box2.pack_start(separator, expand=False)
		separator.show()

		table = gtk.Table(4, 4)
		table.set_row_spacings(5)
		table.set_col_spacings(5)
		box2.pack_start (table)
		table.show()

		c = 0
		r = 1

		check_button = gtk.CheckButton("+X")
		dir_chk.append (check_button)
		check_button.show()
		table.attach (check_button, c, c+1, r, r+1)

		c += 2
		check_button = gtk.CheckButton("-X")
		dir_chk.append (check_button)
		check_button.show()
		table.attach (check_button, c, c+1, r, r+1)

		c = 1
		r = 0

		check_button = gtk.CheckButton("+Y")
		dir_chk.append (check_button)
		check_button.show()
		table.attach (check_button, c, c+1, r, r+1)

		r += 2
		check_button = gtk.CheckButton("-Y")
		dir_chk.append (check_button)
		check_button.show()
		table.attach (check_button, c, c+1, r, r+1)


		c = 3
		r = 0

		check_button = gtk.CheckButton("+Z")
		dir_chk.append (check_button)
		check_button.show()
		table.attach (check_button, c, c+1, r, r+1)

		r += 2
		check_button = gtk.CheckButton("-Z")
		dir_chk.append (check_button)
		check_button.show()
		table.attach (check_button, c, c+1, r, r+1)


		r = 0
		c += 1
		lab = gobject.new (gtk.Label, label="X:")
		lab.show ()
		table.attach (lab, c, c+1, r, r+1)
		r += 1
		lab = gobject.new (gtk.Label, label="Y:")
		lab.show ()
		table.attach (lab, c, c+1, r, r+1)
		r += 1
		lab = gobject.new (gtk.Label, label="Z:")
		lab.show ()
		table.attach (lab, c, c+1, r, r+1)
		r += 1

		r = 0
		c += 1
		indicators.append (gtk.Entry ())
		indicators[0].set_text("%4d" %xyz_count[0])
		indicators[0].set_size_request(20, -1)
		indicators[0].show ()
		table.attach (indicators[0], c, c+1, r, r+1)
		r += 1
		indicators.append (gtk.Entry ())
		indicators[1].set_text("%4d" %xyz_count[1])
		indicators[1].set_size_request(20, -1)
		indicators[1].show ()
		table.attach (indicators[1], c, c+1, r, r+1)
		r += 1
		indicators.append (gtk.Entry ())
		indicators[2].set_text("%4d" %xyz_count[2])
		indicators[2].set_size_request(20, -1)
		indicators[2].show ()
		table.attach (indicators[2], c, c+1, r, r+1)

		r = 0
		c += 1
		steps.append (gtk.Entry ())
		steps[0].set_text("%4d" %xyz_steps[0])
		steps[0].set_size_request(20, -1)
		steps[0].show ()
		table.attach (steps[0], c, c+1, r, r+1)
		r += 1
		steps.append (gtk.Entry ())
		steps[1].set_text("%4d" %xyz_steps[1])
		steps[1].set_size_request(20, -1)
		steps[1].show ()
		table.attach (steps[1], c, c+1, r, r+1)
		r += 1
		steps.append (gtk.Entry ())
		steps[2].set_text("%4d" %xyz_steps[2])
		steps[2].set_size_request(20, -1)
		steps[2].show ()
		table.attach (steps[2], c, c+1, r, r+1)
		r += 1



		table = gtk.Table(4, 1)
		table.set_row_spacings(5)
		table.set_col_spacings(5)
		box2.pack_start (table)
		table.show()

		button = gtk.Button("Run manual steps!")
		button.show ()
		table.attach (button, c, c+1, r, r+1)
		button.connect("clicked", move_stage, lut, steps, indicators)


		dir_chk[0].connect('toggled', toggle_direction, XP, dir_chk, indicators)
		dir_chk[1].connect('toggled', toggle_direction, XM, dir_chk, indicators)
		dir_chk[2].connect('toggled', toggle_direction, YP, dir_chk, indicators)
		dir_chk[3].connect('toggled', toggle_direction, YM, dir_chk, indicators)
		dir_chk[4].connect('toggled', toggle_direction, ZP, dir_chk, indicators)
		dir_chk[5].connect('toggled', toggle_direction, ZM, dir_chk, indicators)


 ## Aproach Control ##

		separator = gobject.new(gtk.HSeparator())
		box2.pack_start(separator, expand=False)
		separator.show()

		table = gtk.Table(4, 4)
		table.set_row_spacings(5)
		table.set_col_spacings(5)
		box2.pack_start (table)
		table.show()

		statuslab = gobject.new(gtk.Label, label="---")
		box2.pack_start(statuslab)
		statuslab.show()
		statuslab2 = gobject.new(gtk.Label, label="---")
		box2.pack_start(statuslab2)
		statuslab2.show()


		lab = gobject.new(gtk.Label, label="Puls W[ms]/T[ms]/#/bOn/bOff/bR:")
		lab.show ()
		table.attach(lab, 0, 1, 3, 4)


		hbox = gobject.new(gtk.HBox(spacing=1))
		hbox.show()
		table.attach(hbox, 1, 2, 3, 4)

		entry_W = gtk.Entry()
		entry_W.set_text("0.1")
		entry_W.set_size_request(30, -1)
		entry_W.show()
		hbox.pack_start(entry_W)

		entry_T = gtk.Entry()
		entry_T.set_text("1")
		entry_T.set_size_request(30, -1)
		entry_T.show()
		hbox.pack_start(entry_T)

		entry_N = gtk.Entry()
		entry_N.set_text("4")
		entry_N.set_size_request(30, -1)
		entry_N.show()
		hbox.pack_start(entry_N)

		entry_bOn = gtk.Entry()
		entry_bOn.set_text("%02X" %ZM)
		entry_bOn.set_size_request(30, -1)
		entry_bOn.show()
		hbox.pack_start(entry_bOn)

		entry_bOff = gtk.Entry()
		entry_bOff.set_text("%02X" %(TRIG+ZM))
		entry_bOff.set_size_request(30, -1)
		entry_bOff.show()
		hbox.pack_start(entry_bOff)

		entry_bR = gtk.Entry()
		entry_bR.set_text("%02X" %(TRIG+ZM))
		entry_bR.set_size_request(30, -1)
		entry_bR.show()
		hbox.pack_start(entry_bR)

		# Freeze -- enable only for experts...
		entry_W.set_sensitive (0)
		entry_T.set_sensitive (0)
		entry_bOn.set_sensitive (0)
		entry_bOff.set_sensitive (0)
		entry_bR.set_sensitive (0)


		button = gtk.Button("Exec")
		button.connect("clicked", CoolRunnerIOpulse, entry_W, entry_T, entry_N, entry_bOn, entry_bOff, entry_bR, statuslab)
		table.attach(button, 2, 3, 3, 4)
		button.show()


# Auto Approach Controller

		lab = gobject.new(gtk.Label, label="Auto App:")
		lab.show ()
		table.attach(lab, 0, 1, 4, 5)


		hbox = gobject.new(gtk.HBox(spacing=1))
		hbox.show()
		table.attach(hbox, 1, 2, 4, 5)

##		lab = gobject.new(gtk.Label, label="-CI:")
##		lab.show ()
##		hbox.pack_start(lab)

##		entry_negCI = gtk.Entry()
##		entry_negCI.set_text("0.1")
##		entry_negCI.set_size_request(30, -1)
##		entry_negCI.show()
##		hbox.pack_start(entry_negCI)

		lab = gobject.new(gtk.Label, label="%Span:")
		lab.show ()
		hbox.pack_start(lab)

		entry_pSpn = gtk.Entry()
		entry_pSpn.set_text("-80")
		entry_pSpn.set_size_request(30, -1)
		entry_pSpn.show()
		hbox.pack_start(entry_pSpn)


		button = gtk.Button("Start")
		gobject.timeout_add (50, CoolRunnerAppRun, 0, entry_pSpn, entry_W, entry_T, entry_N, entry_bOn, entry_bOff, entry_bR, statuslab, statuslab2)
		button.connect("clicked", CoolRunnerAppCtrl, 0)
		table.attach(button, 2, 3, 4, 5)
		button.show()

		button = gtk.Button("Stop")
		button.connect("clicked", CoolRunnerAppCtrl, -2)
		
		table.attach(button, 2, 3, 5, 6)
		button.show()

		button = gtk.Button("RunProgam")
		button.connect("clicked", UserProgram, statuslab, lut, steps, indicators)
		
		table.attach(button, 1, 2, 5, 6)
		button.show()

# Close ---

		button = gtk.Button(stock='gtk-close')
		button.connect("clicked", lambda w: win.hide())
		box2.pack_start(button)
		button.set_flags(gtk.CAN_DEFAULT)
		button.grab_default()
		button.show()
	wins[name].show()
	
		
# create Coolrunner IO approach dialog
def create_coolrunner_slider_control_app(_button):
	global stage_direction
	global xyz_count
	global xyz_steps

	name = "SR Coolrunner Slider and Approach Control"

	# on CoolRunner Out2 via Optocoupler directionselect and triggerpuls are connected:
	XP = 0x02
	XM = 0x01
	YP = 0x08
	YM = 0x04
	ZM = 0x10
	ZP = 0x20
	TRIG = 0x80

	lut = { XP: 0, XM: 1, YP: 2, YM: 3, ZP: 4, ZM: 5 }

	dir_chk = []
	indicators = []
	steps = []

	if not wins.has_key(name):
		win = gobject.new(gtk.Window,
				  type=gtk.WINDOW_TOPLEVEL,
				  title=name,
				  allow_grow=True,
				  allow_shrink=True,
				  border_width=10)
		wins[name] = win
		win.set_size_request(480, 370)
		win.connect("delete_event", delete_event)

		box1 = gobject.new(gtk.VBox())
		win.add(box1)
		box1.show()

		box2 = gobject.new(gtk.VBox(spacing=0))
		box2.set_border_width(10)
		box1.pack_start(box2)
		box2.show()



		lab = gobject.new(gtk.Label, label="Slider and Approach Control\n")
		box2.pack_start(lab)
		lab.show()

## X/Y/Z Slider Control ##

		separator = gobject.new(gtk.HSeparator())
		box2.pack_start(separator, expand=False)
		separator.show()

		table = gtk.Table(4, 4)
		table.set_row_spacings(5)
		table.set_col_spacings(5)
		box2.pack_start (table)
		table.show()

		c = 0
		r = 1

		check_button = gtk.CheckButton("+X")
		dir_chk.append (check_button)
		check_button.show()
		table.attach (check_button, c, c+1, r, r+1)

		c += 2
		check_button = gtk.CheckButton("-X")
		dir_chk.append (check_button)
		check_button.show()
		table.attach (check_button, c, c+1, r, r+1)

		c = 1
		r = 0

		check_button = gtk.CheckButton("+Y")
		dir_chk.append (check_button)
		check_button.show()
		table.attach (check_button, c, c+1, r, r+1)

		r += 2
		check_button = gtk.CheckButton("-Y")
		dir_chk.append (check_button)
		check_button.show()
		table.attach (check_button, c, c+1, r, r+1)


		c = 3
		r = 0

		check_button = gtk.CheckButton("+Z")
		dir_chk.append (check_button)
		check_button.show()
		table.attach (check_button, c, c+1, r, r+1)

		r += 2
		check_button = gtk.CheckButton("-Z")
		dir_chk.append (check_button)
		check_button.show()
		table.attach (check_button, c, c+1, r, r+1)


		r = 0
		c += 1
		lab = gobject.new (gtk.Label, label="X:")
		lab.show ()
		table.attach (lab, c, c+1, r, r+1)
		r += 1
		lab = gobject.new (gtk.Label, label="Y:")
		lab.show ()
		table.attach (lab, c, c+1, r, r+1)
		r += 1
		lab = gobject.new (gtk.Label, label="Z:")
		lab.show ()
		table.attach (lab, c, c+1, r, r+1)
		r += 1

		r = 0
		c += 1
		indicators.append (gtk.Entry ())
		indicators[0].set_text("%4d" %xyz_count[0])
		indicators[0].set_size_request(20, -1)
		indicators[0].show ()
		table.attach (indicators[0], c, c+1, r, r+1)
		r += 1
		indicators.append (gtk.Entry ())
		indicators[1].set_text("%4d" %xyz_count[1])
		indicators[1].set_size_request(20, -1)
		indicators[1].show ()
		table.attach (indicators[1], c, c+1, r, r+1)
		r += 1
		indicators.append (gtk.Entry ())
		indicators[2].set_text("%4d" %xyz_count[2])
		indicators[2].set_size_request(20, -1)
		indicators[2].show ()
		table.attach (indicators[2], c, c+1, r, r+1)

		r = 0
		c += 1
		steps.append (gtk.Entry ())
		steps[0].set_text("%4d" %xyz_steps[0])
		steps[0].set_size_request(20, -1)
		steps[0].show ()
		table.attach (steps[0], c, c+1, r, r+1)
		r += 1
		steps.append (gtk.Entry ())
		steps[1].set_text("%4d" %xyz_steps[1])
		steps[1].set_size_request(20, -1)
		steps[1].show ()
		table.attach (steps[1], c, c+1, r, r+1)
		r += 1
		steps.append (gtk.Entry ())
		steps[2].set_text("%4d" %xyz_steps[2])
		steps[2].set_size_request(20, -1)
		steps[2].show ()
		table.attach (steps[2], c, c+1, r, r+1)
		r += 1


		r += 1
		button = gtk.Button("Run")
		button.show ()
		table.attach (button, c, c+1, r, r+1)
		button.connect("clicked", move_stage, lut, steps, indicators)

		dir_chk[0].connect('toggled', toggle_direction, XP, dir_chk, indicators)
		dir_chk[1].connect('toggled', toggle_direction, XM, dir_chk, indicators)
		dir_chk[2].connect('toggled', toggle_direction, YP, dir_chk, indicators)
		dir_chk[3].connect('toggled', toggle_direction, YM, dir_chk, indicators)
		dir_chk[4].connect('toggled', toggle_direction, ZP, dir_chk, indicators)
		dir_chk[5].connect('toggled', toggle_direction, ZM, dir_chk, indicators)



 ## Aproach Control ##

		separator = gobject.new(gtk.HSeparator())
		box2.pack_start(separator, expand=False)
		separator.show()

		table = gtk.Table(4, 4)
		table.set_row_spacings(5)
		table.set_col_spacings(5)
		box2.pack_start (table)
		table.show()

		statuslab = gobject.new(gtk.Label, label="---")
		box2.pack_start(statuslab)
		statuslab.show()
		statuslab2 = gobject.new(gtk.Label, label="---")
		box2.pack_start(statuslab2)
		statuslab2.show()


		lab = gobject.new(gtk.Label, label="Puls W[ms]/T[ms]/#/bOn/bOff/bR:")
		lab.show ()
		table.attach(lab, 0, 1, 3, 4)


		hbox = gobject.new(gtk.HBox(spacing=1))
		hbox.show()
		table.attach(hbox, 1, 2, 3, 4)

		entry_W = gtk.Entry()
		entry_W.set_text("0.1")
		entry_W.set_size_request(30, -1)
		entry_W.show()
		hbox.pack_start(entry_W)

		entry_T = gtk.Entry()
		entry_T.set_text("1")
		entry_T.set_size_request(30, -1)
		entry_T.show()
		hbox.pack_start(entry_T)

		entry_N = gtk.Entry()
		entry_N.set_text("4")
		entry_N.set_size_request(30, -1)
		entry_N.show()
		hbox.pack_start(entry_N)

		entry_bOn = gtk.Entry()
		entry_bOn.set_text("%02X" %ZM)
		entry_bOn.set_size_request(30, -1)
		entry_bOn.show()
		hbox.pack_start(entry_bOn)

		entry_bOff = gtk.Entry()
		entry_bOff.set_text("%02X" %(TRIG+ZM))
		entry_bOff.set_size_request(30, -1)
		entry_bOff.show()
		hbox.pack_start(entry_bOff)

		entry_bR = gtk.Entry()
		entry_bR.set_text("%02X" %(TRIG+ZM))
		entry_bR.set_size_request(30, -1)
		entry_bR.show()
		hbox.pack_start(entry_bR)

		# Freeze -- enable only for experts...
		entry_W.set_sensitive (0)
		entry_T.set_sensitive (0)
		entry_bOn.set_sensitive (0)
		entry_bOff.set_sensitive (0)
		entry_bR.set_sensitive (0)


		button = gtk.Button("Exec")
		button.connect("clicked", CoolRunnerIOpulse, entry_W, entry_T, entry_N, entry_bOn, entry_bOff, entry_bR, statuslab)
		table.attach(button, 2, 3, 3, 4)
		button.show()


# Auto Approach Controller

		separator = gobject.new(gtk.HSeparator())
		box2.pack_start(separator, expand=False)
		separator.show()

		lab = gobject.new(gtk.Label, label="Auto App:")
		lab.show ()
		table.attach(lab, 0, 1, 4, 5)


		hbox = gobject.new(gtk.HBox(spacing=1))
		hbox.show()
		table.attach(hbox, 1, 2, 4, 5)

		lab = gobject.new(gtk.Label, label="-CI:")
		lab.show ()
		hbox.pack_start(lab)

##		entry_negCI = gtk.Entry()
##		entry_negCI.set_text("0.1")
##		entry_negCI.set_size_request(30, -1)
##		entry_negCI.show()
##		hbox.pack_start(entry_negCI)

		lab = gobject.new(gtk.Label, label="%Span:")
		lab.show ()
		hbox.pack_start(lab)

		entry_pSpn = gtk.Entry()
		entry_pSpn.set_text("-80")
		entry_pSpn.set_size_request(30, -1)
		entry_pSpn.show()
		hbox.pack_start(entry_pSpn)


		button = gtk.Button("Start")
		gobject.timeout_add (50, CoolRunnerAppRun, 0, entry_pSpn, entry_W, entry_T, entry_N, entry_bOn, entry_bOff, entry_bR, statuslab, statuslab2)
		button.connect("clicked", CoolRunnerAppCtrl, 0)
		table.attach(button, 2, 3, 4, 5)
		button.show()

		button = gtk.Button("Stop")
		button.connect("clicked", CoolRunnerAppCtrl, -2)
		
		table.attach(button, 2, 3, 5, 6)
		button.show()

		button = gtk.Button("RunProgam")
		button.connect("clicked", UserProgram, statuslab, lut, steps, indicators)
		
		table.attach(button, 1, 2, 5, 6)
		button.show()

		separator = gobject.new(gtk.HSeparator())
		box2.pack_start(separator, expand=False)
		separator.show()

		box2 = gobject.new(gtk.VBox(spacing=10))
		box2.set_border_width(10)
		box1.pack_start(box2, expand=False)
		box2.show()
		
		button = gtk.Button(stock='gtk-close')
		button.connect("clicked", lambda w: win.hide())
		box2.pack_start(button)
		button.set_flags(gtk.CAN_DEFAULT)
		button.grab_default()
		button.show()
	wins[name].show()



def read_hr(_button):
        sr = open (sr_dev_path, "rb")
        os.lseek (sr.fileno(), magic[i_hr_mask], 0)
        HR_MASK = struct.unpack (fmt_hr_mask, os.read (sr.fileno(), struct.calcsize (fmt_hr_mask)))
        print "HR_MASK is:"
        print HR_MASK
        sr.close ()

def set_hr_0(_button):
        sr = open (sr_dev_path, "wb")
        os.lseek (sr.fileno(), magic[i_hr_mask], 0)
####### 0                       1                       2                       3                       4                       5                       6                       7
####### 0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  
        os.write (sr.fileno(), struct.pack (fmt_hr_mask, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 \
                                            ))
        sr.close ()

        sr = open (sr_dev_path, "rb")
        os.lseek (sr.fileno(), magic[i_hr_mask], 0)
        HR_MASK = struct.unpack (fmt_hr_mask, os.read (sr.fileno(), struct.calcsize (fmt_hr_mask)))
        print "HR_MASK is now:"
        print HR_MASK
        sr.close ()

def set_hr_1(_button):
        sr = open (sr_dev_path, "wb")
        os.lseek (sr.fileno(), magic[i_hr_mask], 0)
####### 0                       1                       2                       3                       4                       5                       6                       7
####### 0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  
        os.write (sr.fileno(), struct.pack (fmt_hr_mask, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1 \
                                            ))
        sr.close ()

        sr = open (sr_dev_path, "rb")
        os.lseek (sr.fileno(), magic[i_hr_mask], 0)
        HR_MASK = struct.unpack (fmt_hr_mask, os.read (sr.fileno(), struct.calcsize (fmt_hr_mask)))
        print "HR_MASK is now:"
        print HR_MASK
        sr.close ()

def set_hr_1slow(_button):
        sr = open (sr_dev_path, "wb")
        os.lseek (sr.fileno(), magic[i_hr_mask], 0)
####### 0                       1                       2                       3                       4                       5                       6                       7
####### 0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  
        os.write (sr.fileno(), struct.pack (fmt_hr_mask, \
        0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0 \
                                            ))
        sr.close ()

        sr = open (sr_dev_path, "rb")
        os.lseek (sr.fileno(), magic[i_hr_mask], 0)
        HR_MASK = struct.unpack (fmt_hr_mask, os.read (sr.fileno(), struct.calcsize (fmt_hr_mask)))
        print "HR_MASK is now:"
        print HR_MASK
        sr.close ()

def set_hr_1slow2(_button):
        sr = open (sr_dev_path, "wb")
        os.lseek (sr.fileno(), magic[i_hr_mask], 0)
####### 0                       1                       2                       3                       4                       5                       6                       7
####### 0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  0  1  2  3  4  5  6  7  
        os.write (sr.fileno(), struct.pack (fmt_hr_mask, \
        0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0 \
                                            ))
        sr.close ()

        sr = open (sr_dev_path, "rb")
        os.lseek (sr.fileno(), magic[i_hr_mask], 0)
        HR_MASK = struct.unpack (fmt_hr_mask, os.read (sr.fileno(), struct.calcsize (fmt_hr_mask)))
        print "HR_MASK is now:"
        print HR_MASK
        sr.close ()

def set_hr_s2(_button):
        sr = open (sr_dev_path, "wb")
        os.lseek (sr.fileno(), magic[i_hr_mask], 0)
        os.write (sr.fileno(), struct.pack (fmt_hr_mask, \
                                            -1, -2,  0,  1,  2, -1,  0,  1,    0, -2,  1, -1, -1,  2,  0,  2,   -2,  1,  0, -1,  2,  0,  2,  0,   1, -1,  2,  0,  2,  0, -2,  1,    2,  0, -2,  1,  1, -1,  2,  1,    1,  2, -1,  2,  0, -2,  1,  2,    0, -2,  2,  2,  1,  2, -1,  2,   1, -2,  2,  2,  1,  2, -1,  2 \
                                            ))
        sr.close ()

        sr = open (sr_dev_path, "rb")
        os.lseek (sr.fileno(), magic[i_hr_mask], 0)
        HR_MASK=struct.unpack (fmt_hr_mask, os.read (sr.fileno(), struct.calcsize (fmt_hr_mask)))
        print "HR_MASK is now:"
        print HR_MASK
        sr.close ()

def readFIFO(_button):
# define SRANGER_MK3_IOCTL_ATOMIC_TRANSFER            95
# define SRANGER_MK3_IOCTL_NON_ATOMIC_TRANSFER        96

        sr = open (sr_dev_path, "rb")
#	fcntl.ioctl (sr, 96)
        os.lseek (sr.fileno(), magic[i_datafifo], 0)
        data=struct.unpack (fmt_datalong2048, os.read (sr.fileno(), struct.calcsize (fmt_datalong2048)))
        print "DataFIFO[%08X"%(magic[i_datafifo]) +"]="
	for i in range(0,2048, 8):
		print "%08X "%data[i]+ "%08X "%data[i+1]+ "%08X "%data[i+2]+ "%08X "%data[i+3]+ "%08X "%data[i+4]+ "%08X "%data[i+5]+ "%08X "%data[i+6]+ "%08X "%data[i+7]
        sr.close ()

def readFIFO128(_button):
        sr = open (sr_dev_path, "rb")
        os.lseek (sr.fileno(), magic[i_datafifo], 0)
        data=struct.unpack (fmt_datalong128, os.read (sr.fileno(), struct.calcsize (fmt_datalong128)))
        print "DataFIFO[%08X"%(magic[i_datafifo]) +"]="
	for i in range(0,128, 8):
		print "%08X "%data[i]+ "%08X "%data[i+1]+ "%08X "%data[i+2]+ "%08X "%data[i+3]+ "%08X "%data[i+4]+ "%08X "%data[i+5]+ "%08X "%data[i+6]+ "%08X "%data[i+7]
        sr.close ()

def readFIFO128c(_button):
        sr = open (sr_dev_path, "rb")
        os.lseek (sr.fileno(), magic[i_datafifo]+128*4, 0)
        data=struct.unpack (fmt_datalong128, os.read (sr.fileno(), struct.calcsize (fmt_datalong128)))
        print "DataFIFO[%08X"%(magic[i_datafifo]+128*4) +"]="
	for i in range(0,128, 8):
		print "%08X "%data[i]+ "%08X "%data[i+1]+ "%08X "%data[i+2]+ "%08X "%data[i+3]+ "%08X "%data[i+4]+ "%08X "%data[i+5]+ "%08X "%data[i+6]+ "%08X "%data[i+7]
        sr.close ()

def readFIFO128c2(_button):
        sr = open (sr_dev_path, "rb")
        os.lseek (sr.fileno(), magic[i_datafifo]+128*2, 0)
        data=struct.unpack (fmt_datalong128, os.read (sr.fileno(), struct.calcsize (fmt_datalong128)))
        print "DataFIFO[%08X"%(magic[i_datafifo]+128*2) +"]="
	for i in range(0,128, 8):
		print "%08X "%data[i]+ "%08X "%data[i+1]+ "%08X "%data[i+2]+ "%08X "%data[i+3]+ "%08X "%data[i+4]+ "%08X "%data[i+5]+ "%08X "%data[i+6]+ "%08X "%data[i+7]
        sr.close ()

def readFIFO128cc(_button):
        sr = open (sr_dev_path, "rb")
        os.lseek (sr.fileno(), magic[i_datafifo]+256*4, 0)
        data=struct.unpack (fmt_datalong128, os.read (sr.fileno(), struct.calcsize (fmt_datalong128)))
        print "DataFIFO[%08X"%(magic[i_datafifo]+256*4) +"]="
	for i in range(0,128, 8):
		print "%08X "%data[i]+ "%08X "%data[i+1]+ "%08X "%data[i+2]+ "%08X "%data[i+3]+ "%08X "%data[i+4]+ "%08X "%data[i+5]+ "%08X "%data[i+6]+ "%08X "%data[i+7]
        sr.close ()

def readFIFO64(_button):
        sr = open (sr_dev_path, "rb")
        os.lseek (sr.fileno(), magic[i_datafifo], 0)
        data=struct.unpack (fmt_datalong64, os.read (sr.fileno(), struct.calcsize (fmt_datalong64)))
        print "DataFIFO="
	for i in range(0,64, 8):
		print "%08X "%data[i]+ "%08X "%data[i+1]+ "%08X "%data[i+2]+ "%08X "%data[i+3]+ "%08X "%data[i+4]+ "%08X "%data[i+5]+ "%08X "%data[i+6]+ "%08X "%data[i+7]
        sr.close ()

def readFIFON(_button):
        sr = open (sr_dev_path, "rb")
        os.lseek (sr.fileno(), magic[i_datafifo], 0)
	N=136
	fmtN="<LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"
        data=struct.unpack (fmtN, os.read (sr.fileno(), struct.calcsize (fmtN)))
        print "DataFIFO="
	for i in range(0,N, 8):
		print "%08X "%data[i]+ "%08X "%data[i+1]+ "%08X "%data[i+2]+ "%08X "%data[i+3]+ "%08X "%data[i+4]+ "%08X "%data[i+5]+ "%08X "%data[i+6]+ "%08X "%data[i+7]

        sr.close ()

def readFIFO32(_button):
        sr = open (sr_dev_path, "rb")
        os.lseek (sr.fileno(), magic[i_datafifo], 0)
        data=struct.unpack (fmt_datalong32, os.read (sr.fileno(), struct.calcsize (fmt_datalong32)))
        print "DataFIFO="
	for i in range(0,32, 8):
		print "%08X "%data[i]+ "%08X "%data[i+1]+ "%08X "%data[i+2]+ "%08X "%data[i+3]+ "%08X "%data[i+4]+ "%08X "%data[i+5]+ "%08X "%data[i+6]+ "%08X "%data[i+7]
        sr.close ()

def testwriteFIFO(_button):
        sr = open (sr_dev_path, "wb")
        os.lseek (sr.fileno(), magic[i_datafifo], 0)
        os.write (sr.fileno(), struct.pack (fmt_datalong128, \
					    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, \
					    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, \
					    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, \
					    96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127 \
                                            ))
        sr.close ()

def clearFIFO(_button):
        sr = open (sr_dev_path, "wb")
        os.lseek (sr.fileno(), magic[i_datafifo], 0)
        os.write (sr.fileno(), struct.pack (fmt_datalong128, \
		  0, 0, 0, 0, 4096, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
		  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
		  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
		  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 \
					    ))
        os.lseek (sr.fileno(), magic[i_datafifo]+4*128, 0)
        os.write (sr.fileno(), struct.pack (fmt_datalong128, \
		  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
		  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
		  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
		  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 \
					    ))
        sr.close ()

def fillFIFO(_button):
        sr = open (sr_dev_path, "wb")
	for i in range (0,16):
		a0 = 128*4*i
		os.lseek (sr.fileno(), magic[i_datafifo]+a0, 0)
		a0 = 128*i
		os.write (sr.fileno(), struct.pack (fmt_datalong128, \
					    a0, a0+1, a0+2, a0+3, a0+4, a0+5, a0+6, a0+7, a0+8, a0+9, a0+10, a0+11, a0+12, a0+13, a0+14, a0+15, a0+16, a0+17, a0+18, a0+19, a0+20, a0+21, a0+22, a0+23, a0+24, a0+25, a0+26, a0+27, a0+28, a0+29, a0+30, a0+31, a0+32, a0+33, a0+34, a0+35, a0+36, a0+37, a0+38, a0+39, a0+40, a0+41, a0+42, a0+43, a0+44, a0+45, a0+46, a0+47, a0+48, a0+49, a0+50, a0+51, a0+52, a0+53, a0+54, a0+55, a0+56, a0+57, a0+58, a0+59, a0+60, a0+61, a0+62, a0+63, a0+64, a0+65, a0+66, a0+67, a0+68, a0+69, a0+70, a0+71, a0+72, a0+73, a0+74, a0+75, a0+76, a0+77, a0+78, a0+79, a0+80, a0+81, a0+82, a0+83, a0+84, a0+85, a0+86, a0+87, a0+88, a0+89, a0+90, a0+91, a0+92, a0+93, a0+94, a0+95, a0+96, a0+97, a0+98, a0+99, a0+100, a0+101, a0+102, a0+103, a0+104, a0+105, a0+106, a0+107, a0+108, a0+109, a0+110, a0+111, a0+112, a0+113, a0+114, a0+115, a0+116, a0+117, a0+118, a0+119, a0+120, a0+121, a0+122, a0+123, a0+124, a0+125, a0+126, a0+127 \
					    ))

        sr.close ()

def testwriteHFIFO(_button):
        sr = open (sr_dev_path, "wb")
        os.lseek (sr.fileno(), magic[i_datafifo], 0)
        os.write (sr.fileno(), struct.pack ("<HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH", \
					    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, \
					    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, \
					    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, \
					    96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127 \
                                            ))
        sr.close ()

def initFIFO(_button):
        sr = open (sr_dev_path, "wb")
        os.lseek (sr.fileno(), magic[i_datafifo], 0)
        os.write (sr.fileno(), struct.pack (fmt_datafifo, 0,0,0,0,4096,1,0x101,0x102))
        sr.close ()


def testreadsFIFO(_button):
        sr = open (sr_dev_path, "rb")
        os.lseek (sr.fileno(), magic[i_datafifo], 0)

	fmt5H = "<HHHHH"
        data=os.read (sr.fileno(), struct.calcsize (fmt5H))
	print data
#        data=struct.unpack (fmt5H, os.read (sr.fileno(), struct.calcsize (fmt5H)))
#        print "DataFIFO [5H]="
#	i=0
#	print "%08X "%data[i]+ "%08X "%data[i+1]+ "%08X "%data[i+2]+ "%08X "%data[i+3]+ "%08X "%data[i+4]

	fmt12L = "<LLLLLLLLLLLL"
        data=struct.unpack (fmt12L, os.read (sr.fileno(), struct.calcsize (fmt12L)))
        print "DataFIFO [12L]="
	for i in range(0,12):
		print "%08X "%data[i]

	fmt16L = "<LLLLLLLLLLLLLLLL"
        data=struct.unpack (fmt16L, os.read (sr.fileno(), struct.calcsize (fmt16L)))
        print "DataFIFO [16L]="
	for i in range(0,16, 8):
		print "%08X "%data[i]+ "%08X "%data[i+1]+ "%08X "%data[i+2]+ "%08X "%data[i+3]+ "%08X "%data[i+4]+ "%08X "%data[i+5]+ "%08X "%data[i+6]+ "%08X "%data[i+7]

        sr.close ()



def open_file_dialog(_button):
	global forceupdate
	filew = gtk.FileSelection("load offset file")
	filew.set_modal(True)
	#filew.connect("destroy", destroy)
	#filew.cancel_button.connect("clicked", lambda w: filew.hide())
	filew.ok_button.connect("clicked", lambda w: open_file(filew.get_filename()))
	filew.set_filename("offset")
	filew.run()
	forceupdate = 255
	filew.hide()

def open_file(filename):
	global globaltext
	array_fmt = "<hhhhhhhh"
	try:
		file = open (filename, "rb")
		array = file.read (struct.calcsize (array_fmt))
		file.close ()
	except IOError:
		errorbox = gtk.MessageDialog(flags=gtk.DIALOG_MODAL, type = gtk.MESSAGE_ERROR, buttons = gtk.BUTTONS_OK, message_format = "Could not open " + filename)
		errorbox.set_title("Error!")
		errorbox.connect("response", lambda dialog, response: dialog.destroy())
		errorbox.show()
		return
	sr = open (sr_dev_path, "wb")
	os.lseek (sr.fileno(), magic[i_analog]+2*16, 1)
	os.write (sr.fileno(), array)
	sr.close ()
	print (struct.unpack (array_fmt, array))

def save_file_dialog(_button):
	filew = gtk.FileSelection("save offset file")
	filew.set_modal(True)
	filew.ok_button.connect("clicked", lambda w: save_file(filew.get_filename()))
	filew.set_filename("offset")
	filew.run()
	filew.hide()

def save_file(filename):
	array_fmt = "<hhhhhhhh"
	sr = open (sr_dev_path, "rb")
	os.lseek (sr.fileno(), magic[i_analog]+2*16, 1)
	array = os.read (sr.fileno(), struct.calcsize (array_fmt))
	sr.close ()
	try:
		file = open (filename, "wb")
		file.write (array)
		file.close ()
	except IOError:
		errorbox = gtk.MessageDialog(flags=gtk.DIALOG_MODAL, type = gtk.MESSAGE_ERROR, buttons = gtk.BUTTONS_OK, message_format = "Could not save " + filename)
		errorbox.set_title("Error!")
		errorbox.connect("response", lambda dialog, response: dialog.destroy())
		errorbox.show()
		return
	print (struct.unpack (array_fmt, array))

def get_status():
	global offset_mutex
	global SPM_STATEMACHINE
	global SPM_FEEDBACK
	global MIXER_FB
	global SPM_SCAN
	global SPM_MOVE
	global CR_COUNTER
	global AIC_in_buffer
	global AIC_out_buffer
	global analog
	global analog_offset
	global analog_out_offset
	global EXT_CONTROL
	global counts0
	global counts1
	global counts0h
	global counts1h
	global WATCH
	global WATCHL16
	global WATCHL17

	sr = open (sr_dev_path)

	os.lseek (sr.fileno(), magic[i_analog], 0)
	analog = struct.unpack (fmt_analog, os.read (sr.fileno(), struct.calcsize (fmt_analog)))

	if offset_mutex == 0:
		os.lseek (sr.fileno(), magic[i_analog] + 2*16, 0)
		analog_offset = struct.unpack (fmt_analog_offset, os.read (sr.fileno(), struct.calcsize (fmt_analog_offset)))
		os.lseek (sr.fileno(), magic[i_analog] + 2*(16 + 8+2*2*8+8+6*32+2*128), 0)
		analog_out_offset = struct.unpack (fmt_analog_out_offset, os.read (sr.fileno(), struct.calcsize (fmt_analog_out_offset)))
	
	
	os.lseek (sr.fileno(), magic[i_statemachine], 0)
	SPM_STATEMACHINE = struct.unpack (fmt_statemachine, os.read (sr.fileno(), struct.calcsize (fmt_statemachine)))

	os.lseek (sr.fileno(), magic[i_scan], 0)
	SPM_SCAN = struct.unpack (fmt_scan, os.read (sr.fileno(), struct.calcsize (fmt_scan)))

	os.lseek (sr.fileno(), magic[i_move], 0)
	SPM_MOVE = struct.unpack (fmt_move, os.read (sr.fileno(), struct.calcsize (fmt_move)))

	os.lseek (sr.fileno(), magic[i_AIC_in], 0)
	AIC_in_buffer = struct.unpack (fmt_AIC_in, os.read (sr.fileno(), struct.calcsize (fmt_AIC_in)))

	os.lseek (sr.fileno(), magic[i_AIC_out], 0)
	AIC_out_buffer = struct.unpack (fmt_AIC_out, os.read (sr.fileno(), struct.calcsize (fmt_AIC_out)))
	
	os.lseek (sr.fileno(), magic[i_feedback], 0)
	SPM_FEEDBACK = struct.unpack (fmt_feedback, os.read (sr.fileno(), struct.calcsize (fmt_feedback)))

	os.lseek (sr.fileno(), magic[i_feedback_mixer], 0)
	MIXER_FB = struct.unpack (fmt_feedback_mixer, os.read (sr.fileno(), struct.calcsize (fmt_feedback_mixer)))
	
	os.lseek (sr.fileno(), magic[i_external], 0)
	EXT_CONTROL = struct.unpack (fmt_external, os.read (sr.fileno(), struct.calcsize (fmt_external)))
	
	os.lseek (sr.fileno(), magic[i_CR_generic_io], 0)
	CR_COUNTER = struct.unpack (fmt_CR_generic_io, os.read (sr.fileno(), struct.calcsize (fmt_CR_generic_io)))

	x = CR_COUNTER[8]
	l = (x & 0xffff0000) >> 16
	h = (x & 0x0000ffff) << 16
	counts0 = l | h

	x = CR_COUNTER[9]
	l = (x & 0xffff0000) >> 16
	h = (x & 0x0000ffff) << 16
	counts1 = l | h
	
	x = CR_COUNTER[10]
	l = (x & 0xffff0000) >> 16
	h = (x & 0x0000ffff) << 16
	counts0h = l | h
	
	x = CR_COUNTER[11]
	l = (x & 0xffff0000) >> 16
	h = (x & 0x0000ffff) << 16
	counts1h = l | h
	
	os.lseek (sr.fileno(), magic[i_watch]+2*2, 0)
	WATCH = struct.unpack (fmt_watch, os.read (sr.fileno(), struct.calcsize (fmt_watch)))
	x = WATCH[16]
	l = (x & 0xffff0000) >> 16
	h = (x & 0x0000ffff) << 16
	WATCHL16 = l | h
	x = WATCH[17]
	l = (x & 0xffff0000) >> 16
	h = (x & 0x0000ffff) << 16
	WATCHL17 = l | h
	
	sr.close ()
	return 1

def do_exit(button):
        gtk.main_quit()

def destroy(*args):
        gtk.main_quit()


def create_main_window():
	buttons = {
		   "A810 Offset Control": create_offset_edit,
		   "FPGA GPIO Control": create_coolrunner_app,
		   "FPGA Rate Meter": create_ratemeter_app,
		   "FPGA Slider Control": create_coolrunner_slider_control_app,
		   "FPGA LVDT Stage Control": create_coolrunner_lvdt_stage_control_app,
		   "SR MK3 DSP Info": create_info,
		   "SR MK3 DSP SPM Settings": create_settings_edit,
                   "SR MK3 read HR mask": read_hr,
                   "SR MK3 set HR0": set_hr_0,
                   "SR MK3 set HR1": set_hr_1,
                   "SR MK3 set HR1slow": set_hr_1slow,
                   "SR MK3 set HR1slow2": set_hr_1slow2,
                   "SR MK3 set HRs2": set_hr_s2,
                   "SR MK3 FIFO-INIT": initFIFO,
                   "SR MK3 FIFO-R128L": readFIFO128,
                   "SR MK3 FIFO-R128Lc": readFIFO128c,
                   "SR MK3 FIFO-R128Lc2": readFIFO128c2,
                   "SR MK3 FIFO-R128Lcc": readFIFO128cc,
                   "SR MK3 FIFO-R64L": readFIFO64,
                   "SR MK3 FIFO-RNL": readFIFON,
                   "SR MK3 FIFO-R32L": readFIFO32,
                   "SR MK3 FIFO": readFIFO,
                   "SR MK3 FIFO-TWR-L": testwriteFIFO,
                   "SR MK3 FIFO-TWR-H": testwriteHFIFO,
                   "SR MK3 FIFO-TRDs": testreadsFIFO,
                   "SR MK3 FIFO-clear": clearFIFO,
                   "SR MK3 FIFO-fill": fillFIFO,
	   	 }
	win = gobject.new(gtk.Window,
	             type=gtk.WINDOW_TOPLEVEL,
                     title='SR SPM Control '+version,
                     allow_grow=True,
                     allow_shrink=True,
                     border_width=5)
	win.set_name("main window")
	win.set_size_request(240, 440)
	win.connect("destroy", destroy)
	win.connect("delete_event", destroy)

	box1 = gobject.new(gtk.VBox(False, 0))
	win.add(box1)
	box1.show()
	scrolled_window = gobject.new(gtk.ScrolledWindow())
	scrolled_window.set_border_width(10)
	scrolled_window.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
	box1.pack_start(scrolled_window)
	scrolled_window.show()
	box2 = gobject.new(gtk.VBox())
	box2.set_border_width(5)
	scrolled_window.add_with_viewport(box2)
	box2.show()

	lab = gtk.Label("[" + sr_dev_path + "]")
	lab.show()
	box2.pack_start(lab)

	k = buttons.keys()
	k.sort()
	for i in k:
		button = gobject.new(gtk.Button, label=i)

		if buttons[i]:
			button.connect("clicked", buttons[i])
		else:
			button.set_sensitive(False)
 		box2.pack_start(button)
		button.show()

	separator = gobject.new(gtk.HSeparator())
	box1.pack_start(separator, expand=False)
	separator.show()
	box2 = gobject.new(gtk.VBox(spacing=10))
	box2.set_border_width(5)
	box1.pack_start(box2, expand=False)
	box2.show()
	button = gtk.Button(stock='gtk-close')
	button.connect("clicked", do_exit)
	button.set_flags(gtk.CAN_DEFAULT)
	box2.pack_start(button)
	button.grab_default()
	button.show()
	win.show()



offset_mutex = 0
magic = open_sranger()
#magic = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
#magic[i_datafifo] = 0x108026a0

print ("Magic:   %08X" %magic[0])
print ("Version: %08X" %magic[1])
print ("Year:    %08X" %magic[2])
print ("MM/DD:   %08X" %magic[3])
print ("SoftID:  %08X" %magic[4])
print ("Statema  %08X" %magic[5])
print ("DataFIFO %08X" %magic[i_datafifo])
print ("PrbFIFO  %08X" %magic[i_probedatafifo])

#sr = open (sr_dev_path, "wb")
#os.lseek (sr.fileno(), FB_SPM_MAGIC_ADR_MK3+4, 1)
##test write
#os.write (sr.fileno(), struct.pack ("<l", magic[1]+1))


get_status()
create_main_window()
#gobject.timeout_add(updaterate, get_status)	

gtk.main()
print ("Byby.")

