#!/usr/bin/env python

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
import os		# use os because python IO is bugy
import time
import fcntl

#import GtkExtra
import struct
import array

import math

# Set here the SRanger dev to be used -- or first to start looking for it:
sr_dev_index = 1
sr_dev_base = "/dev/sranger_mk2_"
sr_dev_path = "/dev/sranger_mk2_1"

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
global HV1_monitor

DIGVfacM = 200./32767.
DIGVfacO = 181.81818/32767.

scaleM = [ DIGVfacM, DIGVfacM, DIGVfacM ]
scaleO = [ DIGVfacO, DIGVfacO, DIGVfacO ]
unit  = [ "V", "V", "V" ]

updaterate = 88	# update time watching the SRanger in ms

wins = {}

setup_list = []

def delete_event(win, event=None):
	win.hide()
	# don't destroy window -- just leave it hidden
	return True

def make_menu_item(name, _object, callback, value, identifier):
	item = gtk.MenuItem(name)
	item.connect("activate", callback, value, identifier)
	item.show()
	return item

def read_back():
	global HV1_configuration
	sr = open (sr_dev_path, "rw")
	
	os.lseek (sr.fileno(), CONFIGURATION_VECTOR_ADDRESS, 1)
	HV1_configuration = struct.unpack (fmt_config, os.read (sr.fileno(), struct.calcsize (fmt_config)))
	
	sr.close ()
	
def _X_write_back_():
	global HV1_configuration
        sr = open (sr_dev_path, "wb")
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

	spantag  = "<span size=\"24000\" font_family=\"monospace\">"   # .. color=\"#ff0000\"
		
	_c1set (spantag + "<b>%8.3f " %(scaleM[0]*HV1_monitor[ii_monitor_X]) + unit[0] + "</b></span>")
	_c2set (spantag + "<b>%8.3f " %(scaleM[1]*HV1_monitor[ii_monitor_Y]) + unit[1] + "</b></span>")
	_c3set (spantag + "<b>%8.3f " %(scaleM[2]*HV1_monitor[ii_monitor_Z]) + unit[2] + "</b></span>")
	
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

	sr = open (sr_dev_path, "wb")
	os.lseek (sr.fileno(), CONFIGURATION_VECTOR_ADDRESS + 2*ii_config_preset_X0, 1)
	os.write (sr.fileno(), struct.pack ("<hhh", pX0, pY0, pZ0))
	sr.close ()
	read_back ()

	epX0.set_text("%12.3f" %(scaleO[0]*HV1_configuration[ii_config_preset_X0]))
	epY0.set_text("%12.3f" %(scaleO[1]*HV1_configuration[ii_config_preset_Y0]))
	epZ0.set_text("%12.3f" %(scaleO[2]*HV1_configuration[ii_config_preset_Z0]))


def write_t_vector(button, etX0, etY0, etZ0):
	global HV1_configuration

	tX0 = int (float (etX0.get_text ()) / scaleO[0]);
	tY0 = int (float (etY0.get_text ()) / scaleO[1]);
	tZ0 = int (float (etZ0.get_text ()) / scaleO[2]);

	print tX0, tY0, tZ0

	sr = open (sr_dev_path, "wb")
	os.lseek (sr.fileno(), CONFIGURATION_VECTOR_ADDRESS + 2*ii_config_target_X0, 1)
	os.write (sr.fileno(), struct.pack ("<hhh", tX0, tY0, tZ0))
	sr.close ()

	read_back ()

	etX0.set_text("%12.3f" %(scaleO[0]*HV1_configuration[ii_config_target_X0]))
	etY0.set_text("%12.3f" %(scaleO[1]*HV1_configuration[ii_config_target_Y0]))
	etZ0.set_text("%12.3f" %(scaleO[2]*HV1_configuration[ii_config_target_Z0]))

def hv1_adjust(_object, _value, _identifier):
	value = _value
	index = _identifier
	if index >= ii_config_preset_X0 and index < ii_config_dum:
		sr = open (sr_dev_path, "wb")
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

	print "goto presets:"
	print tX0, tY0, tZ0

	sr = open (sr_dev_path, "wb")
	os.lseek (sr.fileno(), CONFIGURATION_VECTOR_ADDRESS + 2*ii_config_target_X0, 1)
	os.write (sr.fileno(), struct.pack ("<hhh", tX0, tY0, tZ0))
	sr.close ()

	read_back ()

#	etX0.set_text("%12.3f" %(scaleO[0]*HV1_configuration[ii_config_target_X0]))
#	etY0.set_text("%12.3f" %(scaleO[1]*HV1_configuration[ii_config_target_Y0]))
#	etZ0.set_text("%12.3f" %(scaleO[2]*HV1_configuration[ii_config_target_Z0]))

def toggle_configure_widgets (w):
	if w.get_active():
		for w in setup_list:
			w.show ()
	else:
		for w in setup_list:
			w.hide ()
	
def do_emergency(button):
	print "Emergency Stop Action:"
#	print " -- no action defined, doing nothing. -- "
	goto_presets()


# create ratemeter dialog
def create_hv1_app():
	global HV1_configuration
	global HV1_monitor
	name = "HV1 (SoftdB) Smart Piezo Drive Control and Monitor Panel V"+version+" * [" + sr_dev_path + "]"
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

		table = gtk.Table (5, 8)
		table.set_row_spacings(5)
		table.set_col_spacings(5)
		box1.pack_start (table)
		table.show()

# Channel Headings

		spantag  = "<span size=\"18000\" font_family=\"monospace\">"
		_spantag = "</span>"
		hh = ["X", "Y", "Z"]
		for i in range(0,3):
			c1 = gobject.new(gtk.Label, label=spantag + "<b> " + hh[i] + " </b>("+ unit[i] +") " + _spantag)
			c1.set_use_markup(True)
			c1.show()
			table.attach(c1, 1+i, 2+i, tr, tr+1)


# Monitor
		tr = tr+1
		separator = gobject.new(gtk.HSeparator())
		separator.show()
		table.attach(separator, 0, 5, tr, tr+1)

		tr = tr+1
		lab = gobject.new(gtk.Label, label="Readings:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		c1 = gobject.new(gtk.Label, label="-XX-")
		c1.set_use_markup(True)
		c1.show()
		table.attach(c1, 1, 2, tr, tr+1)

		c2 = gobject.new(gtk.Label, label="-YY-")
		c2.set_use_markup(True)
		c2.show()
		table.attach(c2, 2, 3, tr, tr+1)

		c3 = gobject.new(gtk.Label, label="-ZZ-")
		c3.set_use_markup(True)
		c3.show()
		table.attach(c3, 3, 4, tr, tr+1)

		gobject.timeout_add (updaterate, update_HV1_monitor, c1.set_markup, c2.set_markup, c3.set_markup)

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

		entry_pX0 = gtk.Entry()
		entry_pX0.set_text("%12.3f" %(scaleO[0]*HV1_configuration[ii_config_preset_X0]))
		table.attach(entry_pX0, 1, 2, tr, tr+1)
		setup_list.append (entry_pX0)
#		entry_pX0.show()
		entry_pY0 = gtk.Entry()
		entry_pY0.set_text("%12.3f" %(scaleO[1]*HV1_configuration[ii_config_preset_Y0]))
		table.attach(entry_pY0, 2, 3, tr, tr+1)
		setup_list.append (entry_pY0)
#		entry_pY0.show()
		entry_pZ0 = gtk.Entry()
		entry_pZ0.set_text("%12.3f" %(scaleO[2]*HV1_configuration[ii_config_preset_Z0]))
		table.attach(entry_pZ0, 3, 4, tr, tr+1)
		setup_list.append (entry_pZ0)
#		entry_pZ0.show()
		button = gtk.Button(stock='gtk-apply')
		button.connect("clicked", write_p_vector, entry_pX0, entry_pY0, entry_pZ0)
		setup_list.append (button)
#		button.show()
		table.attach(button, 4, 5, tr, tr+1)

# Offset Adjusts
		tr = tr+1
		lab = gobject.new(gtk.Label, label="Offsets:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		entry_tX0 = gtk.Entry()
		entry_tX0.set_text("%12.3f" %(scaleO[0]*HV1_configuration[ii_config_target_X0]))
		table.attach(entry_tX0, 1, 2, tr, tr+1)
		entry_tX0.show()
		entry_tY0 = gtk.Entry()
		entry_tY0.set_text("%12.3f" %(scaleO[1]*HV1_configuration[ii_config_target_Y0]))
		table.attach(entry_tY0, 2, 3, tr, tr+1)
		entry_tY0.show()
		entry_tZ0 = gtk.Entry()
		entry_tZ0.set_text("%12.3f" %(scaleO[2]*HV1_configuration[ii_config_target_Z0]))
		table.attach(entry_tZ0, 3, 4, tr, tr+1)
		entry_tZ0.show()
		button = gtk.Button(stock='gtk-apply')
		button.connect("clicked", write_t_vector, entry_tX0, entry_tY0, entry_tZ0)
		button.show()
		table.attach(button, 4, 5, tr, tr+1)

# GAINs
		tr = tr+1
		lab = gobject.new(gtk.Label, label="Gains:")
		lab.show ()
		table.attach(lab, 0, 1, tr, tr+1)

		chan = ["gain_X", "gain_Y", "gain_Z"]
		ii   = [ii_config_gain_X, ii_config_gain_Y, ii_config_gain_Z]
		gain = [" 1x", " 2x", " 5x", "10x", "20x"]
		for ci in range(0,3):
			opt = gtk.OptionMenu()
			menu = gtk.Menu()
			for i in range(0,5):
				item = make_menu_item(gain[i], make_menu_item, hv1_adjust, i, ii[ci])
				menu.append(item)
			menu.set_active(HV1_configuration[ii[ci]])
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
			print i, 181.81818 / 150000 * 32767 / srsteps[i]
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
		Label=Label.set_label('Emergency STOP (GOTO Presets)')
		button.show()
		box2.pack_start(button)

		check_button = gtk.CheckButton("Configure")
		check_button.set_active(False)
		check_button.connect('toggled', toggle_configure_widgets)
		box2.pack_start(check_button)
		check_button.show()

		button = gtk.Button(stock='gtk-close')
#		button.label.set_label("Close App, HV1 keeps running")
		button.connect("clicked", do_exit)
		button.set_flags(gtk.CAN_DEFAULT)
		button.show()
		box2.pack_start(button)
		button.grab_default()

		win.show()

	wins[name].show()

def get_status():
	global HV1_monitor
	
	sr = open (sr_dev_path)

	os.lseek (sr.fileno(), MONITOR_VECTOR_ADDRESS, 1)
	HV1_monitor = struct.unpack (fmt_monitor, os.read (sr.fileno(), struct.calcsize (fmt_monitor)))
	
	sr.close ()
	return 1

def do_exit(button):
        gtk.main_quit()

def destroy(*args):
        gtk.main_quit()


##
## MAIN PROGARM INIT
##

# auto locate Mk3-Pro with HV1 firmware
for i in range(sr_dev_index, 8-sr_dev_index):
	sr_dev_path = sr_dev_base+"%d" %i

	print "looking for MK3-Pro/HV1 at " + sr_dev_path
	# open SRanger (MK3-Pro) check for HV1, initialize
	sr = open (sr_dev_path, "rw")
	
	os.lseek (sr.fileno(), CONFIGURATION_VECTOR_ADDRESS, 1)
	HV1_configuration = struct.unpack (fmt_config, os.read (sr.fileno(), struct.calcsize (fmt_config)))
	
	os.lseek (sr.fileno(), MONITOR_VECTOR_ADDRESS, 1)
	HV1_monitor = struct.unpack (fmt_config, os.read (sr.fileno(), struct.calcsize (fmt_config)))
	
	sr.close ()
	
	print ("HV1 Magic Code ID:   %08X" %HV1_configuration[ii_config_softwareid])
	if HV1_configuration[ii_config_softwareid] == HV1_magic_softwareid:
		print "Identified HV1 at " + sr_dev_path
		print ("Hardware Version :   %08X" %HV1_configuration[ii_config_hardware_version])
		print ("Firmware Version :   %08X" %HV1_configuration[ii_config_firmware_version])
		print ("\n Full Configuration Vector: \n")
		print (HV1_configuration)
		print ("\n Monitor Vector: \n")
		print (HV1_monitor)
		break
	print "continue seeking for HV1 ..."

if ( HV1_configuration[ii_config_softwareid] != HV1_magic_softwareid ):
		print ("\nERROR: Exiting -- sorry, no valid HV1 (Smart Piezo Drive) code Mk3-Pro identified.")
else:
        get_status()
	create_hv1_app()
	gobject.timeout_add(updaterate, get_status)	
	gtk.main()
	print "The HV1 is still active and alive unchanged, you can reconnect for control/monitor at any time."

print ("Byby.")

