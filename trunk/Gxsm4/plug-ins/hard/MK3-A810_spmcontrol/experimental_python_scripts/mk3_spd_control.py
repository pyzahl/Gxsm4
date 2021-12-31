#!/usr/bin/env python2.7

### #!/usr/bin/env python

## * Python User Control for
## * MK3-Pro-HV1 Piezo Drive
## * for the Signal Ranger STD/SP2 DSP board
## * 
## * Copyright (C) 2013 Percy Zahl
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

version = "1.0.0"

import pygtk
pygtk.require('2.0')

import gobject, gtk
import cairo
import os		# use os because python IO is bugy
import time
import fcntl
from threading import Timer

#import GtkExtra
import struct
import array

import math

from meterwidget import *
from scopewidget import *
from mk3_spmcontol_class import *

# Set here the SRanger dev to be used -- or first to start looking for it:
sr_spd_dev_index = 0
sr_spd_dev_base = "/dev/sranger_mk2_"
sr_spd_dev_path = "/dev/sranger_mk2_0"

sr_spm_dev_index = 0
sr_spm_dev_base = "/dev/sranger_mk2_"
sr_spm_dev_path = "/dev/sranger_mk2_0"

# SoftdB  Smart Piezo Drive MK3-HV1

# "HT_Magic_code" vector is at
CONFIGURATION_VECTOR_ADDRESS = 0x10F04A20
fmt_config = "<HHHHHHhhhhhhhhhhhhhh"
[
	ii_config_softwareid,
	ii_config_hardware_version,
	ii_config_firmware_version,
	ii_config_MAC0,
	ii_config_MAC1,
	ii_config_MAC2,
	ii_config_preset_X0,
	ii_config_preset_Y0,
	ii_config_preset_Z0,
	ii_config_gain_X,
	ii_config_gain_Y,
	ii_config_gain_Z,
	ii_config_bw_X,
	ii_config_bw_Y,
	ii_config_bw_Z,
	ii_config_slewadjust_steps,
	ii_config_target_X0,
	ii_config_target_Y0,
	ii_config_target_Z0,
	ii_config_dum
] = range (0, 20)


# "_HT_Output_Chx" vector is at
MONITOR_VECTOR_ADDRESS        = 0x10F04A48
fmt_monitor = "<hhhhhhhhhh"
[
	ii_monitor_X,
	ii_monitor_Y,
	ii_monitor_Z,
	ii_monitor_Xmin,
	ii_monitor_Ymin,
	ii_monitor_Zmin,
	ii_monitor_Xmax,
	ii_monitor_Ymax,
	ii_monitor_Zmax,
	ii_monitor_dum
] = range (0, 10)

# HV1 with SR3-Pro identification code
HV1_magic_softwareid = 0x4326

global HV1_configuration
global HV1_driftcomp
global HV1_monitor
HV1_driftcomp = [ 0., 0., 0. ]

DIGVfacM = 200./32767.
DIGVfacO = 181.81818/32767.

scaleM = [ DIGVfacM, DIGVfacM, DIGVfacM ]
scaleO = [ DIGVfacO, DIGVfacO, DIGVfacO ]
unit  = [ "V", "V", "V" ]

updaterate = 88	# update time watching the SRanger in ms

GXSM_Link_status = FALSE
Z0_invert_status  = FALSE

wins = {}

setup_list = []


class gxsm_link():
	def __init__(self, oc):
		NUM_MONITOR_SIGNALS = 20
		self.count = -1
		self.offset_ctrl = oc
		self.mk3spm = SPMcontrol ()
		self.gain_code_last=0
		self.gains = [ 0,0,0 ]
		self.gxsm_gain_control_link = False
		if self.mk3spm.status ():
			self.mk3spm.read_configurations ()
		# setup monitor for link:
			print ("Setting up Links in Signal Monitor. WARNING: DO NOT READJUST WHILE LINK IS ACTIVE.")
			x0 = self.mk3spm.lookup_signal_by_name ("X Offset")
			self.x0tap = NUM_MONITOR_SIGNALS-4
			y0 = self.mk3spm.lookup_signal_by_name ("Y Offset")
			self.y0tap = NUM_MONITOR_SIGNALS-3
			z0 = self.mk3spm.lookup_signal_by_name ("Z Offset")
			self.z0tap = NUM_MONITOR_SIGNALS-2
			gains = self.mk3spm.lookup_signal_by_name ("XYZ Scan Gain")
			self.gaintap = 10
			self.mk3spm.change_signal_input (0, x0, DSP_SIGNAL_MONITOR_INPUT_BASE_ID+self.x0tap)
			self.mk3spm.change_signal_input (0, y0, DSP_SIGNAL_MONITOR_INPUT_BASE_ID+self.y0tap)
			self.mk3spm.change_signal_input (0, z0, DSP_SIGNAL_MONITOR_INPUT_BASE_ID+self.z0tap)
			self.mk3spm.change_signal_input (0, gains, DSP_SIGNAL_MONITOR_INPUT_BASE_ID+self.gaintap)
		else:
			print ("NO SPM_CONTROL HOOKED UP.")

	def offset_adjust (self, start, count):
		global Z0_invert_status
		if self.count >= 0 and self.mk3spm.status ():
			self.mk3spm.read_signal_monitor_only ()
			self.count = count
			ticks = time.time()
			t = Timer(self.interval - (ticks-start-count*self.interval), self.offset_adjust, [start, count+1])
			t.start()
			v = [0.,0.,0.]
			for i in range (0,3):
				v[i] = 	20. * DSP32Qs15dot16TO_Volt * self.mk3spm.get_monitor_data (self.x0tap+i)
			if Z0_invert_status:
				v[2] = -v[2];
			self.offset_ctrl.write_t_vector_var(v)
			if self.gxsm_gain_control_link:
                                gc = self.mk3spm.get_monitor_data (self.gaintap)
                                if gc != self.gain_code_last:
                                        self.gain_code_last = gc
                                        self.gains = [ (gc&0xff0000)>>16, (gc&0xff00)>>8, gc&0xff ]
                                        print ("Adjust Gains XYZ: ", self.gains)
                                        self.offset_ctrl.adjust_gains (self.gains)
                                        
	#   print ("Number of ticks since 12:00am, January 1, 1970:", ticks, " #", count, v)
	    
	def start (self):
		self.count = 0
		self.interval = 0.100 # interval in sec
		self.t = Timer(self.interval, self.offset_adjust, (time.time(), 0)) # start over at full second, for testing only
		self.t.start()
        
	def stop (self):
		self.count = -1

	def enable_gain_control (self):
                self.gxsm_gain_control_link = True;

	def disable_gain_control (self):
                self.gxsm_gain_control_link = False;

                
	def status (self):
		return self.mk3spm.status ()



def delete_event(win, event=None):
	global GXSM_Link_status
	print ("Link: ", GXSM_Link_status)
	if GXSM_Link_status:
		print ("GXSM Link is active, please disable Link to quit")
		idlg = gtk.MessageDialog(None, 
				       gtk.DIALOG_DESTROY_WITH_PARENT,
				       gtk.MESSAGE_INFO, 
				       gtk.BUTTONS_OK, 
				       "Warning: GXSM Link is active!\nLink will go done on SPD Control termination.")
		idlg.run()
		idlg.destroy()
		return False
	else:
		win.hide()
		gtk.main_quit()
	return True

def make_menu_item(name, _object, callback, value, identifier):
	item = gtk.MenuItem(name)
	item.connect("activate", callback, value, identifier)
	item.show()
	return item

def read_back():
	global HV1_configuration
	sr = open (sr_spd_dev_path, "rw")
	
	os.lseek (sr.fileno(), CONFIGURATION_VECTOR_ADDRESS, 1)
	HV1_configuration = struct.unpack (fmt_config, os.read (sr.fileno(), struct.calcsize (fmt_config)))
	
	sr.close ()
	
def _X_write_back_():
	global HV1_configuration
	sr = open (sr_spd_dev_path, "wb")
	os.write (sr.fileno(), struct.pack (fmt_config,
					    HV1_configuration[ii_config_softwareid],
					    HV1_configuration[ii_config_hardware_version],
					    HV1_configuration[ii_config_firmware_version],
					    HV1_configuration[ii_config_MAC0],
					    HV1_configuration[ii_config_MAC1],
					    HV1_configuration[ii_config_MAC2],
					    HV1_configuration[ii_config_preset_X0],
					    HV1_configuration[ii_config_preset_Y0],
					    HV1_configuration[ii_config_preset_Z0],
					    HV1_configuration[ii_config_gain_X],
					    HV1_configuration[ii_config_gain_Y],
					    HV1_configuration[ii_config_gain_Z],
					    HV1_configuration[ii_config_bw_X],
					    HV1_configuration[ii_config_bw_Y],
					    HV1_configuration[ii_config_bw_Z],
					    HV1_configuration[ii_config_slewadjust_steps],
					    HV1_configuration[ii_config_target_X0],
					    HV1_configuration[ii_config_target_Y0],
					    HV1_configuration[ii_config_target_Z0],
					    HV1_configuration[ii_config_dum]))
	sr.close ()

## show only current
def update_HV1_monitor(_c1set, _c2set, _c3set):
	global HV1_configuration
	global HV1_monitor
	
#	_c1set (scaleM[0] * HV1_monitor[ii_monitor_X])
#	_c2set (scaleM[1] * HV1_monitor[ii_monitor_Y])
#	_c3set (scaleM[2] * HV1_monitor[ii_monitor_Z])
	_c1set (scaleM[0] * HV1_monitor[ii_monitor_X], scaleM[0] * HV1_monitor[ii_monitor_Xmin], scaleM[0] * HV1_monitor[ii_monitor_Xmax])
	_c2set (scaleM[1] * HV1_monitor[ii_monitor_Y], scaleM[1] * HV1_monitor[ii_monitor_Ymin], scaleM[1] * HV1_monitor[ii_monitor_Ymax])
	_c3set (scaleM[2] * HV1_monitor[ii_monitor_Z], scaleM[2] * HV1_monitor[ii_monitor_Zmin], scaleM[2] * HV1_monitor[ii_monitor_Zmax])
	
	return 1

## show min/max at 1Hz
def update_HV1_monitor_full(_c1set, _c2set, _c3set):
	global HV1_configuration
	global HV1_monitor

	spantag  = "<span size=\"24000\" font_family=\"monospace\">"   # .. color=\"#ff0000\"
	spantag2 = "<span size=\"10000\" font_family=\"monospace\">"   # .. color=\"#ff0000\"
		
	_c1set (spantag + "<b>%8.3f " %(scaleM[0]*HV1_monitor[ii_monitor_X]) + unit[0] + "</b></span>\n"
		+ spantag2
		+ "%8.3f " %(scaleM[0]*HV1_monitor[ii_monitor_Xmin]) + unit[0]
		+ "%8.3f " %(scaleM[0]*HV1_monitor[ii_monitor_Xmax]) + unit[0]
		+" </span>")
	_c2set (spantag + "<b>%8.3f " %(scaleM[1]*HV1_monitor[ii_monitor_Y]) + unit[1] + "</b></span>\n"
		+ spantag2
		+ "%8.3f " %(scaleM[0]*HV1_monitor[ii_monitor_Ymin]) + unit[0]
		+ "%8.3f " %(scaleM[0]*HV1_monitor[ii_monitor_Ymax]) + unit[0]
		+" </span>")
	_c3set (spantag + "<b>%8.3f " %(scaleM[2]*HV1_monitor[ii_monitor_Z]) + unit[2] + "</b></span>\n"
		+ spantag2
		+ "%8.3f " %(scaleM[0]*HV1_monitor[ii_monitor_Zmin]) + unit[0]
		+ "%8.3f " %(scaleM[0]*HV1_monitor[ii_monitor_Zmax]) + unit[0]
		+" </span>")
	
	return 1

def write_p_vector(button, epX0, epY0, epZ0):
	global HV1_configuration

	pX0 = int (float (epX0.get_text ()) / scaleO[0]);
	pY0 = int (float (epY0.get_text ()) / scaleO[1]);
	pZ0 = int (float (epZ0.get_text ()) / scaleO[2]);

	sr = open (sr_spd_dev_path, "wb")
	os.lseek (sr.fileno(), CONFIGURATION_VECTOR_ADDRESS + 2*ii_config_preset_X0, 1)
	os.write (sr.fileno(), struct.pack ("<hhh", pX0, pY0, pZ0))
	sr.close ()
	read_back ()

	epX0.set_text("%12.3f" %(scaleO[0]*HV1_configuration[ii_config_preset_X0]))
	epY0.set_text("%12.3f" %(scaleO[1]*HV1_configuration[ii_config_preset_Y0]))
	epZ0.set_text("%12.3f" %(scaleO[2]*HV1_configuration[ii_config_preset_Z0]))

def hv1_adjust(_object, _value, _identifier):
	value = _value
	index = _identifier
	if index >= ii_config_preset_X0 and index < ii_config_dum:
		sr = open (sr_spd_dev_path, "wb")
		os.lseek (sr.fileno(), CONFIGURATION_VECTOR_ADDRESS + 2*index, 1)
		os.write (sr.fileno(), struct.pack ("<h", value))
		sr.close ()

	read_back ()
	return 1


def goto_presets():
	global HV1_configuration

	tX0 = HV1_configuration[ii_config_preset_X0]
	tY0 = HV1_configuration[ii_config_preset_Y0]
	tZ0 = HV1_configuration[ii_config_preset_Z0]

	print ("goto presets:")
	print (tX0, tY0, tZ0)

	sr = open (sr_spd_dev_path, "wb")
	os.lseek (sr.fileno(), CONFIGURATION_VECTOR_ADDRESS + 2*ii_config_target_X0, 1)
	os.write (sr.fileno(), struct.pack ("<hhh", tX0, tY0, tZ0))
	sr.close ()

	read_back ()

#	etX0.set_text("%12.3f" %(scaleO[0]*HV1_configuration[ii_config_target_X0]))
#	etY0.set_text("%12.3f" %(scaleO[1]*HV1_configuration[ii_config_target_Y0]))
#	etZ0.set_text("%12.3f" %(scaleO[2]*HV1_configuration[ii_config_target_Z0]))



        
class offset_vector():
    def __init__(self, etv, gsv):
        self.etvec = etv
        self.gainselectmenuvec = gsv
        
    def write_t_vector(self, button):
        global HV1_configuration
        tX = [0,0,0]
        for i in range (0,3):
            tX[i] = int (float (self.etvec[i].get_text ()) / scaleO[i]);
            if tX[i] > 32766:
                    tX[i] = 32766
            if tX[i] < -32766:
                    tX[i] = -32766

#        print (tX)

        sr = open (sr_spd_dev_path, "wb")
        os.lseek (sr.fileno(), CONFIGURATION_VECTOR_ADDRESS + 2*ii_config_target_X0, 1)
        os.write (sr.fileno(), struct.pack ("<hhh", tX[0], tX[1], tX[2]))
        sr.close ()
        
        read_back ()
            
        for i in range (0,3):
            self.etvec[i].set_text("%12.3f" %(scaleO[i]*HV1_configuration[ii_config_target_X0+i]))

    def write_t_vector_var(self, tvec):
        global HV1_configuration
        tX = [0,0,0]
        for i in range (0,3):
            tX[i] = int (tvec[i] / scaleO[i]);
            if tX[i] > 32766:
                    tX[i] = 32766
            if tX[i] < -32766:
                    tX[i] = -32766
            
#        print (tX)

        sr = open (sr_spd_dev_path, "wb")
        os.lseek (sr.fileno(), CONFIGURATION_VECTOR_ADDRESS + 2*ii_config_target_X0, 1)
        os.write (sr.fileno(), struct.pack ("<hhh", tX[0], tX[1], tX[2]))
        sr.close ()
        
        read_back ()
            
        for i in range (0,3):
            self.etvec[i].set_text("%12.3f" %(scaleO[i]*HV1_configuration[ii_config_target_X0+i]))

    def adjust_gains (self, gains):
            #chan = ["gain_X", "gain_Y", "gain_Z"]
            ii   = [ii_config_gain_X, ii_config_gain_Y, ii_config_gain_Z]
            gain = [1, 2, 5, 10, 20]
            for ci in range(0,3):
	            for i in range(0,5):
                            if gains[ci] == gain[i]:
                                    print ("setting gain[%d]"%ci+" to %dx"%gain[i])
                                    hv1_adjust (0, i, ii[ci])
                                    self.gainselectmenuvec[ci](i)


class drift_compensation():
    def __init__(self, edv, oc):
        self.count = -1
        self.edvec = edv
        self.offset_ctrl = oc

    def update (self):
        self.tvd = [0.,0.,0.] # differential offset vector (drift rate in 1 / sec)
        for i in range (0,3):
            self.tvd[i] = float (self.edvec[i].get_text ())

        read_back ()
        self.tvi = [0.,0.,0.,0.] # initial offset vector
        for i in range (0,3):
            self.tvi[i] = scaleO[i] * HV1_configuration[ii_config_target_X0+i]
        self.tvi[3] = time.time() # initial time

        self.offset_ctrl.write_t_vector_var(self.tvi)

    def offset_adjust (self, start, count):
        if self.count >= 0:
            self.count = count
            ticks = time.time()
            t = Timer(self.interval - (ticks-start-count*self.interval), self.offset_adjust, [start, count+1])
            t.start()
            v = [0.,0.,0.]
            for i in range (0,3):
                v[i] = self.tvi[i] + (ticks - self.tvi[3]) * self.tvd[i]
            self.offset_ctrl.write_t_vector_var(v)
#            print ("Number of ticks since 12:00am, January 1, 1970:", ticks, " #", count, v)
        
    def start (self):
        self.update ()
        self.count = 0
        self.interval = 0.05 # interval in sec
        self.t = Timer(self.interval, self.offset_adjust, (time.time(), 0)) # start over at full second, for testing only
        self.t.start()
        
    def stop (self):
        self.count = -1


def toggle_configure_widgets (w):
	if w.get_active():
		for w in setup_list:
			w.show ()
	else:
		for w in setup_list:
			w.hide ()
	
def toggle_driftcompensation (w, dc):
	if w.get_active():
            dc.start ()
            print ("Drift Compensation ON")
	else:
            dc.stop ()
            print ("Drift Compensation OFF")
	
def toggle_gxsm_link (w, gl):
	global GXSM_Link_status
	if w.get_active():
		gl.start ()
		GXSM_Link_status = TRUE
		print ("GXSM Link enabled")
	else:
		gl.stop ()
		GXSM_Link_status = FALSE
		print ("GXSM Link disabled")
	
def toggle_gxsm_gain_link (w, gl):
	global GXSM_Link_status
	if w.get_active():
		gl.enable_gain_control ()
	else:
		gl.disable_gain_control ()
	
def toggle_Z0_invert (w):
	global Z0_invert_status
	if w.get_active():
		Z0_invert_status = TRUE
		print ("Z0 invert ACTIVE")
	else:
		Z0_invert_status = FALSE
		print ("Z0 normal")
	
def do_emergency(button):
	print ("Emergency Stop Action:")
	goto_presets()
#	print (" -- no action defined, doing nothing. -- ")


# create HV1 main panel
def create_hv1_app():
	global HV1_configuration
	global HV1_monitor
	name = "HV1 (SoftdB) Smart Piezo Drive Control and Monitor Panel V"+version+" * [" + sr_spd_dev_path + "]"
	if not wins.has_key(name):
		win = gobject.new(gtk.Window,
				  type=gtk.WINDOW_TOPLEVEL,
				  title=name,
				  allow_grow=False,
				  allow_shrink=False,
				  border_width=1)
		wins[name] = win
		win.connect("delete_event", delete_event)
		

		box1 = gobject.new(gtk.VBox(spacing=3))
		win.add(box1)
		box1.show()

		tr = 0

		table = gtk.Table (6, 8)
		table.set_row_spacings(5)
		table.set_col_spacings(5)
		box1.pack_start (table)
		table.show()

# Channel Headings

		spantag  = "<span size=\"18000\" font_family=\"monospace\">"
		_spantag = "</span>"
		hh = ["X", "Y", "Z"]
#		for i in range(0,3):
#			c1 = gobject.new(gtk.Label, label=spantag + "<b> " + hh[i] + " </b>("+ unit[i] +") " + _spantag)
#			c1.set_use_markup(True)
#			c1.show()
#			table.attach(c1, 1+i, 2+i, tr, tr+1)


# Monitor
#		tr = tr+1
#		separator = gobject.new(gtk.HSeparator())
#		separator.show()
#		table.attach(separator, 0, 5, tr, tr+1)

		tr = tr+1
#		lab = gobject.new(gtk.Label, label="Readings:")
#		lab.show ()
#		table.attach(lab, 0, 1, tr, tr+1)

		maxv = 200
		v = gobject.new(gtk.VBox(spacing=2))
		c1 = Instrument( gobject.new(gtk.Label), v, "Volt", "X-Axis", unit[0])
		c1.set_range(arange(0,maxv/10*11,maxv/10))
		c1.show()
		table.attach(v, 1, 2, tr, tr+1)

		v = gobject.new(gtk.VBox(spacing=2))
		c2 = Instrument( gobject.new(gtk.Label), v, "Volt", "Y-Axis", unit[1])
		c2.set_range(arange(0,maxv/10*11,maxv/10))
		c2.show()
		table.attach(v, 2, 3, tr, tr+1)

		v = gobject.new(gtk.VBox(spacing=2))
		c3 = Instrument( gobject.new(gtk.Label), v, "Volt", "Z-Axis", unit[2])
		c3.set_range(arange(0,maxv/10*11,maxv/10))
		c3.show()
		table.attach(v, 3, 4, tr, tr+1)

		tr = tr+1
#		lab = gobject.new(gtk.Label, label="Readings:")
#		lab.show ()
#		table.attach(lab, 0, 1, tr, tr+1)

#		m1 = gobject.new(Meter)

#		gobject.timeout_add (updaterate, update_HV1_monitor, c1.set_reading, c2.set_reading, c3.set_reading)
		gobject.timeout_add (updaterate, update_HV1_monitor, c1.set_reading_lohi, c2.set_reading_lohi, c3.set_reading_lohi)

		tr = tr+1
		separator = gobject.new(gtk.HSeparator())
		separator.show()
		table.attach(separator, 0, 5, tr, tr+1)

# Presets (Power Up)
		tr = tr+1
		lab = gobject.new(gtk.Label, label="Presets:")
		setup_list.append (lab)
#		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		e = []

		for i in range(0,3):
                    e.append (gtk.Entry())
                    e[i].set_text("%12.3f" %(scaleO[i]*HV1_configuration[ii_config_preset_X0+i]))
                    table.attach(e[i], 1+i, 2+i, tr, tr+1)
                    setup_list.append (e[i])
#                    e[i].show()

		button = gtk.Button(stock='gtk-apply')
		button.connect("clicked", write_p_vector, e[0], e[1], e[2])
		setup_list.append (button)
#		button.show()
		table.attach(button, 4, 5, tr, tr+1)
                
# Offset Adjusts
		tr = tr+1
		lab = gobject.new(gtk.Label, label="Offsets:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		eo = []

		for i in range(0,3):
                    eo.append (gtk.Entry())
                    eo[i].set_text("%12.3f" %(scaleO[i]*HV1_configuration[ii_config_target_X0+i]))
                    table.attach(eo[i], 1+i, 2+i, tr, tr+1)
                    eo[i].show()

		oabutton = gtk.Button(stock='gtk-apply')
		oabutton.show()
		table.attach(oabutton, 4, 5, tr, tr+1)

# Drift Compensation Setup
		tr = tr+1
		lab = gobject.new(gtk.Label, label="Drift Comp.:")
		setup_list.append (lab)
#		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		ed = []

		for i in range(0,3):
                    ed.append (gtk.Entry())
                    ed[i].set_text("%12.3f " %(scaleO[i]*HV1_driftcomp[i]) ) # + unit[i] + "/s")
                    table.attach(ed[i], 1+i, 2+i, tr, tr+1)
                    setup_list.append (ed[i])


		lab = gobject.new(gtk.Label, label= unit[0] + "/s")
		setup_list.append (lab)
		table.attach(lab, 4, 5, tr, tr+1)
#		button = gtk.Button(stock='gtk-apply')
#		setup_list.append (button)
#		table.attach(button, 4, 5, tr, tr+1)

# GAINs
		tr = tr+1
		lab = gobject.new(gtk.Label, label="Gains:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		chan = ["gain_X", "gain_Y", "gain_Z"]
		ii   = [ii_config_gain_X, ii_config_gain_Y, ii_config_gain_Z]
		gain = [" 1x", " 2x", " 5x", "10x", "20x"]
		gain_select = []
		for ci in range(0,3):
			opt = gtk.OptionMenu()
			menu = gtk.Menu()
			for i in range(0,5):
				item = make_menu_item(gain[i], make_menu_item, hv1_adjust, i, ii[ci])
				menu.append(item)
			menu.set_active(HV1_configuration[ii[ci]])
			gain_select.append (menu.set_active);
			opt.set_menu(menu)
			opt.show()
			table.attach(opt, 1+ci, 2+ci, tr, tr+1)

		ci=3
		lab = gobject.new(gtk.Label, label="Slew:")
		lab.show ()
		table.attach(lab, 1+ci, 2+ci, tr, tr+1)

# BWs
		tr = tr+1
		lab = gobject.new(gtk.Label, label="Bandwidth:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)
		chan = ["bw_X", "bw_Y", "bw_Z"]
		ii   = [ii_config_bw_X, ii_config_bw_Y, ii_config_bw_Z]
		bw   = ["50 kHz", "10 kHz", "1 kHz"]
		for ci in range(0,3):
			opt = gtk.OptionMenu()
			menu = gtk.Menu()
			for i in range(0,3):
				item = make_menu_item(bw[i], make_menu_item, hv1_adjust, i, ii[ci])
				menu.append(item)
			menu.set_active(HV1_configuration[ii[ci]])
			opt.set_menu(menu)
			opt.show()
			table.attach(opt, 1+ci, 2+ci, tr, tr+1)

# Slew rates as presets
		slew = ["0.4 V/s", "1 V/s", "4 V/s", "10 V/s", "40 V/s", "100 V/s", "400 V/s"]
#		181.81818 V / 150000/s * 32767 / N => V/s
		srsteps = [2080, 832, 208, 83, 21, 8, 2]
		ci=3
		opt = gtk.OptionMenu()
		menu = gtk.Menu()
		ist=0
		for i in range(0,7):
			item = make_menu_item(slew[i], make_menu_item, hv1_adjust, srsteps[i], ii_config_slewadjust_steps)
			menu.append(item)
			print (i, 181.81818 / 150000 * 32767 / srsteps[i])
			if HV1_configuration[ii_config_slewadjust_steps] <= srsteps[i]:
				ist = i

		menu.set_active(ist)
		opt.set_menu(menu)
		opt.show()
		table.attach(opt, 1+ci, 2+ci, tr, tr+1)

		tr = tr+1
		separator = gobject.new(gtk.HSeparator())
		separator.show()
		table.attach(separator, 0, 5, tr, tr+1)

# Link controls


		offset_control = offset_vector(eo, gain_select)
		oabutton.connect("clicked", offset_control.write_t_vector)
		dc_control = drift_compensation (ed, offset_control)
		GxsmLink = gxsm_link(offset_control)



# Closing ---

		box2 = gobject.new(gtk.HBox(spacing=10))
		box2.set_border_width(10)
		box1.pack_start(box2, expand=False)
		box2.show()
		
		button = gtk.Button(stock='gtk-quit')
#		button.set_label("Emergency STOP")
		button.connect("clicked", do_emergency)
#		button.set_flags(gtk.CAN_DEFAULT)
		Label=button.get_children()[0]
		Label=Label.get_children()[0].get_children()[1]
		Label=Label.set_label('Emergency STOP')
		button.show()
		box2.pack_start(button)

		check_button = gtk.CheckButton("Configure")
		check_button.set_active(False)
		check_button.connect('toggled', toggle_configure_widgets)
		box2.pack_start(check_button)
		check_button.show()

		dc_check_button = gtk.CheckButton("Drift Comp.")
		dc_check_button.set_active(False)
		dc_check_button.connect('toggled', toggle_driftcompensation, dc_control)
		box2.pack_start(dc_check_button)
		dc_check_button.show()

		check_button = gtk.CheckButton("GXSM Link")
		check_button.set_active(False)
		check_button.connect('toggled', toggle_gxsm_link, GxsmLink)
		box2.pack_start(check_button)
		check_button.show()
		check_button.set_sensitive (GxsmLink.status ())	

		check_button = gtk.CheckButton("GXSM Gains")
		check_button.set_active(False)
		check_button.connect('toggled', toggle_gxsm_gain_link, GxsmLink)
		box2.pack_start(check_button)
		check_button.show()
		check_button.set_sensitive (GxsmLink.status ())

		check_button = gtk.CheckButton("Z0 inv.")
		check_button.set_active(False)
		check_button.connect('toggled', toggle_Z0_invert)
		box2.pack_start(check_button)
		check_button.show()
		check_button.set_sensitive (GxsmLink.status ())	

		button = gtk.Button(stock='gtk-close')
#		button.label.set_label("Close App, HV1 keeps running")
		button.connect("clicked", delete_event)
		button.set_flags(gtk.CAN_DEFAULT)
		button.show()
		box2.pack_start(button)
		button.grab_default()

		win.show()

	wins[name].show()

def get_status():
	global HV1_monitor
	
	sr = open (sr_spd_dev_path)

	os.lseek (sr.fileno(), MONITOR_VECTOR_ADDRESS, 1)
	HV1_monitor = struct.unpack (fmt_monitor, os.read (sr.fileno(), struct.calcsize (fmt_monitor)))
	
	sr.close ()
	return 1

def do_exit(button):
        gtk.main_quit()

def destroy(*args):
        gtk.main_quit()

def locate_spm_control_dsp():
	print ("need to create a MK3 base support class.........")


##
## MAIN PROGARM INIT
##

# auto locate Mk3-Pro with HV1 firmware
for i in range(sr_spd_dev_index, 8-sr_spd_dev_index):
	sr_spd_dev_path = sr_spd_dev_base+"%d" %i

	print ("looking for MK3-Pro/HV1 at " + sr_spd_dev_path)
	try:
            with open (sr_spd_dev_path):
                # open SRanger (MK3-Pro) check for HV1, initialize
                sr = open (sr_spd_dev_path, "rw")
                    
                os.lseek (sr.fileno(), CONFIGURATION_VECTOR_ADDRESS, 1)
                HV1_configuration = struct.unpack (fmt_config, os.read (sr.fileno(), struct.calcsize (fmt_config)))
                
                os.lseek (sr.fileno(), MONITOR_VECTOR_ADDRESS, 1)
                HV1_monitor = struct.unpack (fmt_config, os.read (sr.fileno(), struct.calcsize (fmt_config)))
                
                sr.close ()
                
                print ("HV1 Magic Code ID:   %08X" %HV1_configuration[ii_config_softwareid])
                if HV1_configuration[ii_config_softwareid] == HV1_magic_softwareid:
                    print ("Identified HV1 at " + sr_spd_dev_path)
                    print ("Hardware Version :   %08X" %HV1_configuration[ii_config_hardware_version])
                    print ("Firmware Version :   %08X" %HV1_configuration[ii_config_firmware_version])
                    print ("\n Full Configuration Vector: \n")
                    print (HV1_configuration)
                    print ("\n Monitor Vector: \n")
                    print (HV1_monitor)
                    break
	except IOError:
            print ('Oh dear. No such device.')

	print ("continue seeking for HV1 ...")

if ( HV1_configuration[ii_config_softwareid] != HV1_magic_softwareid ):
		print ("\nERROR: Exiting -- sorry, no valid HV1 (Smart Piezo Drive) code Mk3-Pro identified.")
else:
	get_status()
	create_hv1_app()
	gobject.timeout_add(updaterate, get_status)	
	gtk.main()
	print ("The HV1 is still active and alive unchanged, you can reconnect for control/monitor at any time.")

print ("Byby.")

