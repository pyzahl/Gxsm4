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

#import GtkExtra
import struct
import array


# Set here the SRanger dev to be used:
sr_dev_path = "/dev/sranger0"


global SPM_STATEMACHINE  # at magic[5]
global CR_COUNTER        # at magic[21]
global SPM_FEEDBACK      # at magic[9]
global analog_offset	 # at magic[10]+16
global analog_out_offset # at magic[10]+8+2*2*8+8+6*32+2*128
global AIC_gain          # at magic[8]+1+3*8
global AIC_in_buffer	 # at magic[6]
global EXT_CONTROL       # at magic[20]
global SPM_SCAN          # at magic[11]
global offset_mutex
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

global gatetime_ms

gatetime_ms = 1.

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
	sr.seek (0x4000, 1)
	magic_fmt = ">HHHHHHHHHHHHHHHHHHHHHH"
	magic = struct.unpack (magic_fmt, sr.read (struct.calcsize (magic_fmt)))
	sr.close ()
	return magic

# update one FIR param at given address
def aic_dbgain_update(_adj):
	gainreg = { -42: 15,
				-36: 1, -30: 2, -24: 3, -18: 4,
				-12: 5,  -9: 6,  -6: 7,  -3: 8,
				0: 0,
				3: 9,   6: 10,  9: 11,  12: 12,
				18: 13, 24: 14,
				}
	param_address = _adj.get_data("aic_dbgain_address")
#	print ("Address:", param_address)
#	print ("Value:", _adj.value)
	sr = open (sr_dev_path, "wb")
	sr.seek (param_address, 1)
	db_in  = int(_adj.get_data("aic_dbgain_in_adj").value)
	db_out = int(_adj.get_data("aic_dbgain_out_adj").value)
#	print ("dB in:  ", db_in)
#	print ("dB out: ", db_out)

	# adjust
	if (db_in < -12 or db_in > 12):
		db_in = db_in/6*6
	else:
		db_in = db_in/3*3

	if (db_out < -12 or db_out > 12):
		db_out = db_out/6*6
	else:
		db_out = db_out/3*3

	_adj.get_data("aic_dbgain_in_adj").set_value (db_in);
	_adj.get_data("aic_dbgain_out_adj").set_value (db_out);
#	print ("dB in:  ", db_in, gainreg[db_in]<<4)
#	print ("dB out: ", db_out, gainreg[db_out])
#	print ("Reg: ", gainreg[db_in]<<4 | gainreg[db_out])

	sr.write (struct.pack (">h", gainreg[db_in]<<4 | gainreg[db_out]))
	sr.close ()

	# set mode MD_AIC_RECONFIG (==8) in statemachine
	sr = open (sr_dev_path, "wb")
	sr.seek (magic[5], 1)
	sr.write (struct.pack (">h", 0x4000))
	sr.close ()

def create_aic_gain_app(_button):
	create_aic_gain_edit(_button, magic[8]+1+3*8, "AIC In/Out Gain")

# create Gain edjustments
def create_aic_gain_edit(_button, dbgain_address, name):
	if not wins.has_key(name):
		win = gobject.new(gtk.Window,
		                  type=gtk.WINDOW_TOPLEVEL,
				  title=name,
				  allow_grow=True,
				  allow_shrink=True,
				  border_width=10)

		wins[name] = win
		win.connect("delete_event", delete_event)
		win.set_title(name)
		
		box1 = gtk.VBox()
		win.add(box1)
		box1.show()

		box2 = gtk.VBox(spacing=0)
		box2.set_border_width(10)
		box1.pack_start(box2)
		box2.show()

		# get current Gain parameters
		#		sr = open (sr_dev_path, "rw")
		#		sr.seek (dbgain_address, 1)
		#		fmt = ">hhhhhhhh"
		#		dbgain  = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
		#		sr.close ()

		hbox = gtk.HBox()
		box2.pack_start(hbox)
		hbox.show()

		lab = gtk.Label("In Gain [dB]")
		hbox.pack_start(lab)
		lab.show()

		lab = gtk.Label("Out Gain [dB]")
		hbox.pack_start(lab)
		lab.show()

		reg2gain = [ 0, -36, -30, -24, -18, -12, -9, -6, -3, 3, 6, 9, 12, 18, 24, -42 ]

		aic_in_name  = ["Xo", "Yo", "Zo", "Xs",  "Ys", "Zs", "U", "M"  ]
		aic_out_name = ["", "", "", "", "", "Fb", "", "" ]

		# create scales for delay and gain for all taps...
		for aic in range(0,8):
			hbox = gtk.HBox()
			box2.pack_start(hbox)
			hbox.show()
			
			lab = gtk.Label("AIC"+str(aic)+": "+aic_in_name[aic])
			lab.set_size_request (70,-1)
			hbox.pack_start(lab)
			lab.show()
			
			aic_in_gain = reg2gain[AIC_gain[aic]>>4 & 15]
			adjustment1 = gtk.Adjustment(aic_in_gain, -40, 24, 1, 3, 0)
			scale = gtk.HScale(adjustment1)
			scale.set_size_request (150, 30)
			scale.set_update_policy(gtk.UPDATE_DELAYED)
			scale.set_digits(0)
			scale.set_draw_value(True)
			hbox.pack_start(scale)
			scale.show()
			adjustment1.set_data("aic_dbgain_address", dbgain_address+aic) 
			adjustment1.connect("value-changed", aic_dbgain_update)

			aic_out_gain = reg2gain[AIC_gain[aic] & 15]
			adjustment2 = gtk.Adjustment(aic_out_gain, -40, 24, 1, 3, 0)
			scale = gtk.HScale(adjustment2)
			scale.set_size_request (150, 30)
			scale.set_update_policy(gtk.UPDATE_DELAYED)
			scale.set_digits(0)
			scale.set_draw_value(True)
			hbox.pack_start(scale)
			scale.show()
			adjustment2.set_data("aic_dbgain_address", dbgain_address+aic) 
			adjustment2.connect("value-changed", aic_dbgain_update)

			adjustment1.set_data("aic_dbgain_in_adj", adjustment1) 
			adjustment1.set_data("aic_dbgain_out_adj", adjustment2) 
			adjustment2.set_data("aic_dbgain_in_adj", adjustment1) 
			adjustment2.set_data("aic_dbgain_out_adj", adjustment2) 

			lab = gtk.Label(aic_out_name[aic])
			lab.set_size_request (70, -1)
			hbox.pack_start(lab)
			lab.show()


		separator = gtk.HSeparator()
		box1.pack_start(separator, expand=False)
		separator.show()

		box2 = gtk.VBox(spacing=10)
		box2.set_border_width(10)
		box1.pack_start(box2, expand=False)
		box2.show()
		
		button = gtk.Button("close")
		button.connect("clicked",  lambda w: win.hide())
		box2.pack_start(button)
		button.set_flags(gtk.CAN_DEFAULT)
		button.grab_default()
		button.show()
	wins[name].show()


# update FUZZY parameter
def fuzzy_param_update(_adj):
	param_address = _adj.get_data("fuzzy_param_address")
	param = _adj.get_data("fuzzy_param_multiplier")*_adj.value
	sr = open (sr_dev_path, "wb")
	sr.seek (param_address, 1)
	sr.write (struct.pack (">h", param))
	sr.close ()

# update DSP setting parameter
def int_param_update(_adj):
	param_address = _adj.get_data("int_param_address")
	param = int (_adj.get_data("int_param_multiplier")*_adj.value)
	sr = open (sr_dev_path, "wb")
	sr.seek (param_address, 1)
	sr.write (struct.pack (">h", param))
	sr.close ()
	
def AIC_in_offset_update(_adj):
	param_address = _adj.get_data("AIC_offset_address")
	param = int (_adj.value*3.2767)
	sr = open (sr_dev_path, "wb")
	sr.seek (param_address, 1)
	sr.write (struct.pack (">h", param))
	sr.close ()

def AIC_out_offset_update(_adj):
	param_address = _adj.get_data("AIC_offset_address")
	param = int (_adj.value*3.2767*5)
	sr = open (sr_dev_path, "wb")
	sr.seek (param_address, 1)
	sr.write (struct.pack (">h", param))
	sr.close ()

def ext_FB_control(_object, _value, _identifier):
	value = 0
	sr = open (sr_dev_path, "wb")
	if _identifier=="ext_channel":
		sr.seek (magic[20]+0, 1)
		value = _object.get_value()
	if _identifier=="ext_treshold":
		sr.seek (magic[20]+1, 1)
		value = _object.get_value()*3276.7
	if _identifier=="ext_value":
		sr.seek (magic[20]+2, 1)
		value = _value
	if _identifier=="ext_min":
		sr.seek (magic[20]+3, 1)
		value = _object.get_value()*16383
	if _identifier=="ext_max":
		sr.seek (magic[20]+4, 1)
		value = _object.get_value()*16383
	sr.write (struct.pack (">h", int(value)))
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
		sr.seek (magic[5], 1)
	else:
		sr.seek (magic[5]+1, 1)
	sr.write (struct.pack (">h", mask))
	sr.close ()
	get_status()

def enable_ratemeter(w, gtms, htms):
	global gatetime_ms
	CPLD_GATEREF=89.977748e-9
#	CPLD_GATEREF=90.000000e-9

	gc = float(gtms())
	hc = int (float(htms()) / gc)

	gatetime_ms = gc

	gc = gc*1e-3
	if gc < 5.9e-3:
		gcc = int (gc/CPLD_GATEREF)
		m = 3
	elif gc < 2.7:	
		gcc = int (gc/CPLD_GATEREF/500.)	
		m = 5
	else:
		gcc = int (gc/CPLD_GATEREF/50000.)
		m = 9

	print "Gate,m,hc: ", gcc, m, hc

	sr = open (sr_dev_path, "wb")
	sr.seek (magic[21]+6, 1) # mode, gatecount
	sr.write (struct.pack (">hh", m, gcc))
	sr.close ()
	
	sr = open (sr_dev_path, "wb")
	sr.seek (magic[21]+16, 1) # peak holdcount
	sr.write (struct.pack (">L", hc))
	sr.close ()
		
	sr = open (sr_dev_path, "wb")
	sr.seek (magic[21]+18, 1)
	if w.get_active():
		sr.write (struct.pack (">h", 1))
	else:
		sr.write (struct.pack (">h", 0))
	sr.close ()

	sr = open (sr_dev_path, "wb")
	sr.seek (magic[21], 1)
	if w.get_active():
		sr.write (struct.pack (">h", 1))
	else:
		sr.write (struct.pack (">h", 0))
	sr.close ()


def CoolRunnerIOReadCb(lab0, lab1, lab3, lab7):
	data = CoolRunnerIORead (0)
	lab0.set_text ("[InP0] 0x%04X" %data)
	data = CoolRunnerIORead (1)
	lab1.set_text ("[InP1] 0x%04X" %data)
	data = CoolRunnerIORead (3)
	lab3.set_text ("[Status] 0x%04X" %data)
	dataL = CoolRunnerIOReadL (7)
	lab7.set_text ("[Count] 0x%08X" %dataL)

def CoolRunnerIOReadMem(w, status):
	sr = open (sr_dev_path, "rb")
	sr.seek (magic[21], 1)
	fmt = ">hhhhHHhhHHHH"
	data = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
	sr.close ()
	(data[5]<<16) + data[4]
	status.set_text ("IOm: st%d" %data[0] + " sp%d" %data[1] + " po%d" %data[2] + " lo%04x" %data[4] + " hi%04x" %data[5] + " md%d" %data[6] + " gt%d" %data[7] + " CNT%04x" %data[8] + ":%04x" %data[9]) 

def CoolRunnerIORead(port):
	sr = open (sr_dev_path, "wb")
	sr.seek (magic[21], 1)
	sr.write (struct.pack (">hhHHH", 1,0, port,0,0)) # Start, Stop, Port, WR, data
	sr.close ()
	sr = open (sr_dev_path, "rb")
	sr.seek (magic[21]+4, 1)
	fmt = ">HH"
	data = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
	sr.close ()
	return data[0]

def CoolRunnerIOReadL(port):
	sr = open (sr_dev_path, "wb")
	sr.seek (magic[21], 1)
	sr.write (struct.pack (">hhH", 1,0, port)) # Start, Stop, Port
#	sr.write (struct.pack (">hhHHH", 1,0, port,0,0)) # Start, Stop, Port, WR, data
	sr.close ()
	sr = open (sr_dev_path, "rb")
	sr.seek (magic[21]+4, 1)
	fmt = ">HH"
	data = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
	sr.close ()
	return (data[1]<<16) + data[0]

def CoolRunnerIOReadE16(w, entry, port, status):
	data = CoolRunnerIORead(port)
	entry.set_text ("0x%04X" %data)
	status.set_text ("CoolRunner Read16: 0x%04X" %data + " at IO Port %02d" %port)

def CoolRunnerIOReadE32(w, entry, port, status):
	data = CoolRunnerIOReadL(port)
	entry.set_text ("0x%08X" %data)
	status.set_text ("CoolRunner Read32: 0x%08X" %data + " at IO Port %02d" %port)

def CoolRunnerIOWrite(w, entry, port, status):
	data = int (entry.get_text(), 16) # Hex
	sr = open (sr_dev_path, "wb")
	sr.seek (magic[21], 1)
	sr.write (struct.pack (">hhHHH", 1,0, port,0, data)) # Start, Stop, Port, WR, data
	sr.close ()
	status.set_text ("CoolRunner Write: 0x%04X" %data + " at IO Port %02d" %port)

def CoolRunnerIOWritePort(port, data):
	sr = open (sr_dev_path, "wb")
	sr.seek (magic[21], 1)
	sr.write (struct.pack (">hhHHH", 1,0, port,0, data)) # Start, Stop, Port, WR, data
	sr.close ()
	return 1

def CoolRunnerIOpulse_mask(num, tau, mask, trig):
	Tp = 0.0454545454545
	sr = open (sr_dev_path, "wb")
	W = int (tau/10./Tp)
	T = int (tau/Tp)
	N = num - 1
	bOn = mask
	bOff = trig+mask
	bReset =  trig+mask
	sr.seek (magic[19], 1)
	sr.write (struct.pack (">HHHHHHHHHHHH", 1, 0, W, T, N, bOn, bOff, bReset, 6, 0, 0, 0))
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
	sr.seek (magic[19], 1)
	sr.write (struct.pack (">HHHHHHHHHHHH", 1, 0, W, T, N, bOn, bOff, bReset, 6, 0, 0, 0))
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
		sr.seek (magic[9], 1)
		sr.write (struct.pack (">hh", app_cp, app_ci))
		sr.close ()
		app_mode = -1
		return 1;

	if app_mode == -1:
		return 1

	if app_mode == 0:
		sr = open (sr_dev_path, "rb")
		sr.seek (magic[9], 1)
		fmt = ">hh"
		data = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
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
		sr.seek (magic[9], 1)
		sr.write (struct.pack (">hh", app_cp, ci_retract))
		sr.close ()
		app_mode = 2
		return 1

	if app_mode == 2:
		# monitor Z
		sr = open (sr_dev_path, "rb")
		sr.seek (magic[7]+5, 1)
		fmt = ">h"
		data = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
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
		sr.seek (magic[9], 1)
		sr.write (struct.pack (">hh", app_cp, app_ci))
		sr.close ()
		app_mode = 5
		return 1


	if app_mode == 5:
		# monitor Z
		sr = open (sr_dev_path, "rb")
		sr.seek (magic[7]+5, 1)
		fmt = ">h"
		data = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
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
	os.lseek (sr.fileno(), magic[5] + 15, 0)
	os.write (sr.fileno(), struct.pack (">h", 0))
	# calibration needs 8300 cycles for avg (0.38 s)
	time.sleep(.5)
	# activating offset compensation
	os.lseek (sr.fileno(), magic[5], 0)
	os.write (sr.fileno(), struct.pack (">h", 0x0010))
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

		adj = gtk.Adjustment(int(EXT_CONTROL[0]), 0, 7, 1, 1, 0)
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

		check_button = gtk.CheckButton("Z-Transformation [0x0020]\n (AIC[6] * (-1) -> AIC[5])")
		if SPM_STATEMACHINE[2] & 0x0020:
			check_button.set_active(True)
		gobject.timeout_add (updaterate, check_update, check_button.set_active, 0x0020)
		check_button.connect('toggled', toggle_flag, 0x0020)
		check_button.show()
		hbox.pack_start(check_button)

		check_button = gtk.CheckButton("FUZZY [0x0040]\n (enable linear mixing for FB srcs AIC[1] & AIC[5])")
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

		lab = gobject.new(gtk.Label, label="Gatetime [ms]:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_GTm = gtk.Entry()
		entry_GTm.set_text("1")
		table.attach(entry_GTm, 1, 2, tr, tr+1)
		entry_GTm.show()

		tr = tr+1

		lab = gobject.new(gtk.Label, label="PeakHold [ms]:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_holdTm = gtk.Entry()
		entry_holdTm.set_text("500")
		table.attach(entry_holdTm, 1, 2, tr, tr+1)
		entry_holdTm.show()

		tr = tr+1


		check_button = gtk.CheckButton("Enable RateMeter")
		if CR_COUNTER[13] and CR_COUNTER[0]:
			check_button.set_active(True)
		check_button.connect('toggled', enable_ratemeter, entry_GTm.get_text, entry_holdTm.get_text)
		box1.pack_start(check_button)
		check_button.show()


		separator = gobject.new(gtk.HSeparator())
		box1.pack_start(separator, expand=False)
		separator.show()

		status = gobject.new(gtk.Label, label="-Rate- ---")
		status.set_use_markup(True)
		gobject.timeout_add (updaterate, check_dsp_ratemeter, status.set_markup)
		box1.pack_start(status)
		status.show()

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


# build FUZZY editor
def create_fuzzy_edit(_button):
	global fuzzy_adj
	fuzzy_address = magic[18]
	name = "DFM Fuzzy Control"
	if not wins.has_key(name):
		win = gobject.new(gtk.Window,
				  type=gtk.WINDOW_TOPLEVEL,
				  title=name,
				  allow_grow=True,
				  allow_shrink=True,
				  border_width=10)
		wins[name] = win
		win.connect("delete_event", delete_event)
		
		box1 = gobject.new(gtk.VBox())
		win.add(box1)
		box1.show()

		box2 = gobject.new(gtk.VBox(spacing=0))
		box2.set_border_width(10)
		box1.pack_start(box2)
		box2.show()

		# get current FUZZY parameters: FUZZY.level and FUZZY.gain
		sr = open (sr_dev_path, "rw")
		sr.seek (fuzzy_address, 1)
		fmt = ">h"
		level = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
		gain  = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
		sr.close ()

		hbox = gobject.new(gtk.HBox())
		box2.pack_start(hbox)
		hbox.show()

		lab = gobject.new(gtk.Label, label="Level [V]")
		hbox.pack_start(lab)
		lab.show()
		lab = gobject.new(gtk.Label, label="Tap Gain")
		hbox.pack_start(lab)
		lab.show()
		lab = gobject.new(gtk.Label, label="-- Flags --")
		hbox.pack_start(lab)
		lab.show()

		level_max = 10 # Volt
		level_fac = 32767/10
		gain_max  = 1.0 # 0...32767 / Q15
		gain_fac  = 32767.0

		# create scales for level and gain for all taps...
		for tap in range(0,1):
			print ("FUZZYadr:", fuzzy_address)
			print ("Level:", level[tap]/level_fac)
			print ("Gain:", gain[tap]/gain_fac)
		
			hbox = gobject.new(gtk.HBox())
			box2.pack_start(hbox)
			hbox.show()

			adjustment = gtk.Adjustment (value=level[tap]/level_fac, lower=-level_max, upper=level_max, step_incr=0.001, page_incr=0.05, page_size=0.1)
			scale = gobject.new(gtk.HScale, adjustment=adjustment)
			scale.set_size_request(150, 30)
			scale.set_update_policy(gtk.UPDATE_DELAYED)
			scale.set_digits(3)
			scale.set_draw_value(True)
			hbox.pack_start(scale)
			scale.show()
			adjustment.set_data("fuzzy_param_address", fuzzy_address) 
			adjustment.set_data("fuzzy_param_multiplier", level_fac) 
			adjustment.connect("value-changed", fuzzy_param_update)
			fuzzy_adj = adjustment
			
			adjustment = gtk.Adjustment (value=gain[tap]/gain_fac, lower=-gain_max, upper=gain_max, step_incr=0.001, page_incr=0.02, page_size=0.02)
			scale = gobject.new(gtk.HScale, adjustment=adjustment)
			scale.set_size_request(150, 30)
			scale.set_update_policy(gtk.UPDATE_DELAYED)
			scale.set_digits(3)
			scale.set_draw_value(True)
			hbox.pack_start(scale)
			scale.show()
			adjustment.set_data("fuzzy_param_address", fuzzy_address+1) 
			adjustment.set_data("fuzzy_param_multiplier", gain_fac) 
			adjustment.connect("value-changed", fuzzy_param_update)

			check_button = gtk.CheckButton("_FUZZY")
			if SPM_STATEMACHINE[2] & 0x0040:
				check_button.set_active(True)
			check_button.connect('toggled', toggle_flag, 0x0040)
			gobject.timeout_add (updaterate, check_update, check_button.set_active, 0x0040)
			hbox.pack_start(check_button)
			check_button.show()

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
	name="AIC Offset Control" + "[" + sr_dev_path + "]"
	address=magic[10]+16
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
			adjustment = gtk.Adjustment (value=analog_offset[tap]*level_fac, lower=-level_max, upper=level_max, step_incr=0.1, page_incr=2, page_size=50)
			adjustment.set_data("AIC_offset_address", address+tap) 
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
		for tap in range(0,7):
			if tap >= 3 and tap != 6:
				continue
			adjustment = gtk.Adjustment (value=analog_out_offset[tap]*level_fac/5, lower=-level_max, upper=level_max, step_incr=0.1, page_incr=2, page_size=50)
			adjustment.set_data("AIC_offset_address", address+8+2*2*8+8+6*32+2*128+tap)
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
	global forceupdate
	global lvdt_xy
	sumlength = 16384.
	gainfac0db  = 1.0151 # -0.1038V Offset
	gainfac6db  = 2.0217 # -0.1970
	gainfac12db = 4.0424 # -0.3845
	gainfac18db = 8.0910 # -0.7620
	level_fac = 0.305185095 # mV/Bit
	diff=(AIC_in_buffer[_tap])*level_fac
	outmon=(AIC_in_buffer[8+_tap]+analog_out_offset[_tap])*level_fac/5
#	slowdiff=(analog_offset[8+_tap])*level_fac/sumlength
#	slowdiffX=(analog_offset[8+6])*level_fac/sumlength
#	slowdiffY=(analog_offset[8+7])*level_fac/sumlength
# mirroring raw only:
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
	_set ("DataProcessTicks: %05d" %SPM_STATEMACHINE[8] + "  Idle: %05d" %SPM_STATEMACHINE[9] + "  DSP Load: %.1f" %(100.*SPM_STATEMACHINE[8]/(SPM_STATEMACHINE[8]+SPM_STATEMACHINE[9])) + "  Peak: %.1f" %(100.*SPM_STATEMACHINE[10]/(SPM_STATEMACHINE[10]+SPM_STATEMACHINE[11])))
	# + " S:%05d" %(SPM_STATEMACHINE[8]+SPM_STATEMACHINE[9]))
	return 1

def check_dsp_feedback(_set):
	_set ("FB:%d" %(SPM_STATEMACHINE[2]&4) + " CP=%6.4f" %(SPM_FEEDBACK[0]/32767.) + " CI=%6.4f" %(SPM_FEEDBACK[1]/32767.) + " set=%05d" %SPM_FEEDBACK[2] + " soll=%05d" %SPM_FEEDBACK[3] + " ist=%05d" %SPM_FEEDBACK[4] + "\ndelta=%05d" %SPM_FEEDBACK[5] + " i_sum=%010d" %SPM_FEEDBACK[6] + " Z=%05d" %SPM_FEEDBACK[7])
	return 1

def check_dsp_scan(_set):
	vx = SPM_SCAN[31]*22000./(65536.*32767.)*2.05
	xs = SPM_SCAN[31]/(65536.*32767.)*SPM_SCAN[22]*2.05
	_set ("SCAN-pre:%d" %(SPM_SCAN[6])
	      + " Nx:%d" %(SPM_SCAN[14]) + ",y:%d" %(SPM_SCAN[15]) + "\n"
	      + " fs_dx:%d" %(SPM_SCAN[16]) + ",y:%d" %(SPM_SCAN[17]) + "\n"
	      + " cfs_dx:%d" %(SPM_SCAN[31]) + ",y:%d" %(SPM_SCAN[32]) + "\n"
	      + " dnx:%d" %(SPM_SCAN[22]) + ",y:%d" %(SPM_SCAN[23]) + "\n"
	      + " X speed calc [V/s]: %f" %vx + "  in A @ 533 A/V G1: %f\n" %(vx*533)
	      + " X step size [V]...: %f" %xs + "  in A @ 533 A/V G1: %f\n" %(xs*533)
	      + " nsm_xm:%d" %(SPM_SCAN[18]) + "\n"
	      + " fm_dx:%d" %(SPM_SCAN[19]) + ",y:%d" %(SPM_SCAN[20]) + ",zxy:%d" %(SPM_SCAN[21])
	      + " fm_dzy:%d" %(SPM_SCAN[26]) + ",y:%d" %(SPM_SCAN[27]) + "\n"
	      + " Xpos:%d" %(SPM_SCAN[28]) + ",Y:%d" %(SPM_SCAN[29]) + ",Z:%d" %(SPM_SCAN[30])
	      )
	return 1

def check_dsp_ratemeter(_set):
	global gatetime_ms
	cps = float(CR_COUNTER[8])/gatetime_ms/1e-3
	if cps > 2e6:
		spantag = "<span size=\"32000\" color=\"#ff0000\">"
	else:
		spantag = "<span size=\"32000\">"
	if gatetime_ms > 2000:
		_set (spantag + "<b>%12.2f Hz" %cps + "  Cnt: %d" %(CR_COUNTER[8]) + "  Pk: %d</b></span>" %(CR_COUNTER[9]))
	else:
		_set (spantag + "<b>%12.0f Hz" %cps + "  Cnt: %d" %(CR_COUNTER[8]) + "  Pk: %d</b></span>" %(CR_COUNTER[9]))
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

		lab = gobject.new(gtk.Label, label="Magic ID: %04X" %magic[0])
		box2.pack_start(lab)
		lab.show()

		lab = gobject.new(gtk.Label, label="SPM Version: %04X" %magic[1])
		box2.pack_start(lab)
		lab.show()

		lab = gobject.new(gtk.Label, label="Software ID: %04X" %magic[4])
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
	name = "SRanger Coolrunner"

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

		lab = gobject.new(gtk.Label, label="Coolrunner IO\n")
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

		lab = gobject.new(gtk.Label, label="IO_Out_P0:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_Out0 = gtk.Entry()
		entry_Out0.set_text("0")
		table.attach(entry_Out0, 1, 2, tr, tr+1)
		entry_Out0.show()

		button = gtk.Button("W")
		button.connect("clicked", CoolRunnerIOWrite, entry_Out0, 4, statuslab)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()

		tr = tr+1

		lab = gobject.new(gtk.Label, label="IO_Out_P1:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_Out1 = gtk.Entry()
		entry_Out1.set_text("0")
		table.attach(entry_Out1, 1, 2, tr, tr+1)
		entry_Out1.show()

		button = gtk.Button("W")
		button.connect("clicked", CoolRunnerIOWrite, entry_Out1, 5, statuslab)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()

		tr = tr+1

		lab = gobject.new(gtk.Label, label="IO_Out_P2:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_Out2 = gtk.Entry()
		entry_Out2.set_text("0")
		table.attach(entry_Out2, 1, 2, tr, tr+1)

		entry_Out2.show()

		button = gtk.Button("W")
		button.connect("clicked", CoolRunnerIOWrite, entry_Out2, 6, statuslab)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()

		tr = tr+1

		lab = gobject.new(gtk.Label, label="GATECOUNT:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_OutG = gtk.Entry()
		entry_OutG.set_text("1")
		table.attach(entry_OutG, 1, 2, tr, tr+1)
		entry_OutG.show()

		button = gtk.Button("W")
		button.connect("clicked", CoolRunnerIOWrite, entry_OutG, 10, statuslab)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()

		tr = tr+1

		lab = gobject.new(gtk.Label, label="Control:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_OutC = gtk.Entry()
		entry_OutC.set_text("0")
		table.attach(entry_OutC, 1, 2, tr, tr+1)
		entry_OutC.show()

		button = gtk.Button("W")
		button.connect("clicked", CoolRunnerIOWrite, entry_OutC, 9, statuslab)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()
		tr = tr+1

		lab = gobject.new(gtk.Label, label="IO_In_P0:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_In0 = gtk.Entry()
		entry_In0.set_text("XXXX")
		table.attach(entry_In0, 1, 2, tr, tr+1)
		entry_In0.show()

		button = gtk.Button("R")
		button.connect("clicked", CoolRunnerIOReadE16, entry_In0, 0, statuslab)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()

		tr = tr+1

		lab = gobject.new(gtk.Label, label="IO_In_P1:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_In1 = gtk.Entry()
		entry_In1.set_text("XXXX")
		table.attach(entry_In1, 1, 2, tr, tr+1)
		entry_In1.show()

		button = gtk.Button("R")
		button.connect("clicked", CoolRunnerIOReadE16, entry_In1, 1, statuslab)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()

		tr = tr+1

		lab = gobject.new(gtk.Label, label="IO_In_P2:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_In2 = gtk.Entry()
		entry_In2.set_text("XXXX")
		table.attach(entry_In2, 1, 2, tr, tr+1)
		entry_In2.show()

		button = gtk.Button("R")
		button.connect("clicked", CoolRunnerIOReadE16, entry_In2, 2, statuslab)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()

		tr = tr+1

		lab = gobject.new(gtk.Label, label="IO_Status:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_In3 = gtk.Entry()
		entry_In3.set_text("----")
		table.attach(entry_In3, 1, 2, tr, tr+1)
		entry_In3.show()

		button = gtk.Button("R")
		button.connect("clicked", CoolRunnerIOReadE16, entry_In3, 3, statuslab)
		table.attach(button, 2, 3, tr, tr+1)
		button.show()

		tr = tr+1

		lab = gobject.new(gtk.Label, label="IO_Count:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_In78 = gtk.Entry()
		entry_In78.set_text("XXXXXXXX")
		table.attach(entry_In78, 1, 2, tr, tr+1)
		entry_In78.show()

		button = gtk.Button("R")
		button.connect("clicked", CoolRunnerIOReadE32, entry_In78, 7, statuslab)
		button.connect("clicked", CoolRunnerIOReadMem, statuslab)
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

		lab = gobject.new(gtk.Label, label="Auto App:")
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
	CoolRunnerIOWritePort (2, mask+TRIG)

	
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
		CoolRunnerIOWritePort (2, mask+TRIG)
	else:
		stage_direction = 0
		CoolRunnerIOWritePort (2, TRIG)
#		time.sleep(0.05)
		CoolRunnerIOWritePort (2, 0)

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
	global fuzzy_adj

	if user_action_i == 0:
		user_action_i = 1

	if user_action_i > 20:
		lab.set_text ("-- User Program Aborted/Done @ #%d --" %user_action_i)
		user_action_i = 0
		user_action_mode = 0
		return 0

	sr = open (sr_dev_path, "rb")
	sr.seek (magic[7]+3, 1) # Y
	fmt = ">hhh"
	data = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
	sr.close ()
	x=data[0]
	y=data[1]
	z=data[2]

	zlimit = 32000
	# d) Set Fuzzy to "normal", and proceed... for ever so far
	if user_action_mode == 5:
		fuzzy_adj.set_value (-1.5)
		user_action_mode = 0
		user_action_i += 1
		
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
	if y > 1000 and user_action_mode == 1:
		fuzzy_adj.set_value (-10.0)
		user_action_mode = 2

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
	array_fmt = ">hhhhhhhh"
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
	sr.seek (magic[10]+16, 1)
	sr.write (array)
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
	array_fmt = ">hhhhhhhh"
	sr = open (sr_dev_path, "rb")
	sr.seek (magic[10]+16, 1)
	array = sr.read (struct.calcsize (array_fmt))
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
	global SPM_SCAN
	global CR_COUNTER
	global AIC_in_buffer
	global analog_offset
	global analog_out_offset
	global AIC_gain
	global EXT_CONTROL

	sr = open (sr_dev_path)

	if offset_mutex == 0:
		fmt = ">hhhhhhhhllllllll"
		os.lseek (sr.fileno(), magic[10] + 16, 0)
		analog_offset = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
		fmt = ">hhhhhhhh"
		os.lseek (sr.fileno(), magic[10] + 16 + 8+2*2*8+8+6*32+2*128, 0)
		analog_out_offset = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
	
	
	fmt = ">hhhhlllhhhhhh"
	os.lseek (sr.fileno(), magic[5], 0)
	SPM_STATEMACHINE = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))

#               ss rot  npab srcs nxy fs n fm dxny Zo fmz  XYZ
#	fmt = ">hh hhhh hhhh llll hh  ll l lll hh hh ll lll ll hhhh hh" [38]
	fmt = ">hhhhhhhhhhllllhhllllllhhhhlllllllhhhhhh"
	os.lseek (sr.fileno(), magic[11], 0)
	SPM_SCAN = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))

	fmt = ">hhhhhhhhhhhhhhhh"
	os.lseek (sr.fileno(), magic[6], 0)
	AIC_in_buffer = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))

	fmt = ">hhhhhhhh"
	os.lseek (sr.fileno(), magic[8]+1+3*8, 0)
	AIC_gain = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
	
	fmt = ">hhhhhhlh"
	os.lseek (sr.fileno(), magic[9], 0)
	SPM_FEEDBACK = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))

	fmt = ">hhhhh"
	os.lseek (sr.fileno(), magic[20], 0)
	EXT_CONTROL = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
	
	fmt = ">hhhhHHHHLLLLLh"
	os.lseek (sr.fileno(), magic[21], 0)
	CR_COUNTER = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
	
	sr.close ()
	return 1

def do_exit(button):
        gtk.main_quit()

def destroy(*args):
        gtk.main_quit()


def create_main_window():
	buttons = {
		   "AIC Offset Control": create_offset_edit,
		   "AIC Gain/FIR Setup": create_aic_gain_app,
		   "CR IO Test": create_coolrunner_app,
		   "CR Rate Meter": create_ratemeter_app,
		   "CR Slider Control": create_coolrunner_slider_control_app,
		   "CR LVDT Stage Control": create_coolrunner_lvdt_stage_control_app,
		   "Fuzzy DFM Control": create_fuzzy_edit,
		   "SR DSP Info": create_info,
		   "SR DSP SPM Settings": create_settings_edit,
	   	 }
	win = gobject.new(gtk.Window,
	             type=gtk.WINDOW_TOPLEVEL,
                     title='SR SPM Control '+version,
                     allow_grow=True,
                     allow_shrink=True,
                     border_width=5)
	win.set_name("main window")
	win.set_size_request(220, 380)
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
print ("Magic:", magic)
get_status()
create_main_window()
gobject.timeout_add(updaterate, get_status)	

gtk.main()
print ("Byby.")

