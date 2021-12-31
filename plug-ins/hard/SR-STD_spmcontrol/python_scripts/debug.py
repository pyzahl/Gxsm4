#!/usr/bin/env python

## * Python Debug Tool
## * Debuging and low level configuration tool for the FB_spm DSP program/GXSM2
## * for the Signal Ranger STD/SP2 DSP board
## * 
## * Copyright (C) 2004 Bastian Weyers
## *
## * Author: Bastian Weyers <weyers@users.sf.net>
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

import pygtk
pygtk.require('2.0')

import gobject, gtk
import os
import struct
import array

global updaterate
global flag		# 1 = add value to LOG
			# 2 = 
			# 4 = 
global MAGIC
global AREA_SCAN	# at MAGIC[11]
global PROBE            # at MAGIC[13]
global DATAFIFO		# at MAGIC[15]
global PROBEDATAFIFO	# at MAGIC[16]


wins = {}

# open SRanger, read MAGIC offsets (look at FB_spm_dataexchange.h -> SPM_MAGIC_DATA_LOCATIONS)

# settings dialog
def create_setting(_button):
	global MAGIC
	global PROBE	
	global AREA_SCAN
	name = "Settings"
	win = gobject.new(gtk.Window,
			  type=gtk.WINDOW_TOPLEVEL,
			  title=name,
			  allow_grow=gtk.TRUE,
			  allow_shrink=gtk.TRUE,
			  border_width=5)
	wins[name] = win
	win.connect("destroy", lambda w: win.hide())
	win.set_size_request(500, 500)

	box1 = gobject.new(gtk.VBox(gtk.FALSE,0))
	win.add(box1)
		
	hbox = gobject.new(gtk.HBox(gtk.FALSE, 0))
	hbox.set_border_width(5)
	hbox.set_spacing(15)
	box1.pack_start(hbox)

	vbox = gobject.new(gtk.VBox(gtk.FALSE, 0))
	hbox.pack_start(vbox)
	lab = gobject.new(gtk.Label, label="Probe settings")
	vbox.pack_start(lab)

	scrolled_window = gobject.new(gtk.ScrolledWindow())
	scrolled_window.set_size_request(250,430)
	scrolled_window.set_border_width(2)
	scrolled_window.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
	vbox.pack_start(scrolled_window)
	table_box = gobject.new(gtk.VBox())
	table_box.set_border_width(0)
	scrolled_window.add_with_viewport(table_box)
	table_box.show()

	table = gtk.Table (3, 31)
	table.set_row_spacings(5)
	table.set_col_spacings(4)
	table_box.pack_start(table)
     
	descriptor = ["start", "stop", "srcs", "Xchan", "nx_init[1]", "nx_init[2]", "nx_init[3]", "nx_finish[1]", "nx_finish[2]", "nx_finish[3]", "f_dx_init[1]", "f_dx_init[2]", "f_dx_init[3]", "f_dx_finish[1]", "f_dx_finish[2]", "f_dx_finish[3]", "nx", "dnx", "f_dx", "AC_amp", "AC_frq", "AC_phase1st", "AC_phase2nd", "AC_nAve", "dummy_alignment", "Xpos", "LockIn_0", "LockIn_1st", "LockIn_2nd", "ix", "iix", "state", "pflg"]
	for tap in range(33):
		lab = gobject.new(gtk.Label, label=descriptor[tap])
		table.attach (lab, 0, 1, tap, tap+1)
		value = gobject.new(gtk.Label, label="wait...")
		gobject.timeout_add(updaterate, refresh, value.set_text, "PROBE", tap)
		table.attach (value, 1, 2, tap, tap+1)
		button = gtk.Button("...")
		button.connect("clicked", change_value, win, descriptor[tap], "PROBE", tap)
		table.attach (button, 2, 3, tap, tap+1)


	vbox = gobject.new(gtk.VBox(gtk.FALSE, 0))
	hbox.pack_start(vbox)
	lab = gobject.new(gtk.Label, label="Scan settings")
	vbox.pack_start(lab)

	scrolled_window = gobject.new(gtk.ScrolledWindow())
	scrolled_window.set_size_request(250,430)
	scrolled_window.set_border_width(2)
	scrolled_window.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
	vbox.pack_start(scrolled_window)
	table_box = gobject.new(gtk.VBox())
	table_box.set_border_width(0)
	scrolled_window.add_with_viewport(table_box)
	table_box.show()

	table = gtk.Table (3, 32)
	table.set_row_spacings(5)
	table.set_col_spacings(4)
	table_box.pack_start(table)
     
	descriptor = ["start", "stop", "rotxx", "rotxy", "rotyx", "rotyy", "nx_pre", "dnx_probe", "srcs_xp", "srcs_xm", "srcs_2nd_xp", "srcs_2nd_xm", "nx", "ny", "fs_dx", "fs_dy", "num_steps_move_xy", "fm_dx", "fm_dy", "dnx", "dny", "Zoff_2nd_xp", "Zoff_2nd_xm", "Xpos", "Ypos", "cfs_dx", "cfs_dy", "iix", "iiy", "ix", "iy", "sstate", "pflg"]
	for tap in range(33):
		lab = gobject.new(gtk.Label, label=descriptor[tap])
		table.attach (lab, 0, 1, tap, tap+1)
		value = gobject.new(gtk.Label, label="%i" %AREA_SCAN[tap])
		gobject.timeout_add(updaterate, refresh, value.set_text, "AREA_SCAN", tap)
		table.attach (value, 1, 2, tap, tap+1)
		button = gtk.Button("...")
		button.connect("clicked", change_value, win, descriptor[tap], "AREA_SCAN", tap)
		table.attach (button, 2, 3, tap, tap+1)

	separator = gobject.new(gtk.HSeparator())
	box1.pack_start(separator, expand=gtk.FALSE)
	separator.show()

	hbox = gobject.new(gtk.HBox(gtk.FALSE, 0))
	hbox.set_border_width(5)
	hbox.set_spacing(15)
	box1.pack_start(hbox)
	
	button = gtk.Button(stock='gtk-close')
	button.set_border_width(3)
	button.connect("clicked", lambda w: win.hide())
	hbox.pack_start(button)
	
	button = gtk.Button("Reset values")
	button.set_border_width(3)
	button.connect("clicked", lambda w: win.hide())
	hbox.pack_start(button)

	wins[name].show_all()

# create probe info
def create_probe_info(_button):
	global PROBE	
	global AREA_SCAN
	name = "Probe info"
	win = gobject.new(gtk.Window,
			  type=gtk.WINDOW_TOPLEVEL,
			  title=name,
			  allow_grow=gtk.TRUE,
			  allow_shrink=gtk.TRUE,
			  border_width=5)
	wins[name] = win
	win.connect("destroy", lambda w: win.hide())

	box1 = gobject.new(gtk.VBox(gtk.FALSE,0))
	win.add(box1)
	
	lab = gobject.new(gtk.Label, label="Probe info...")
	probe_info(lab)
	gobject.timeout_add(updaterate, probe_info, lab)
	box1.pack_start(lab)

	wins[name].show_all()

	
def probe_info(label):
	global PROBE	
	global AREA_SCAN
	start_volt = PROBE[25]/1073741824
	pre_time   = (PROBE[4]+PROBE[5]+PROBE[6])/22.1
	pre_volt   = (PROBE[4]*PROBE[10]+PROBE[5]*PROBE[11]+PROBE[6]*PROBE[12])/1073741824 + start_volt
	probe_time = ((PROBE[16]+1)*PROBE[17])/22.1
	probe_volt = ((PROBE[17]+1)*PROBE[17])*PROBE[18]/1073741824 + pre_volt
	post_time  = (PROBE[7]+PROBE[8]+PROBE[9])/22.1
	post_volt  = (PROBE[7]*PROBE[13]+PROBE[8]*PROBE[14]+PROBE[9]*PROBE[15])/1073741824 + probe_volt
	total_time = pre_time+probe_time+post_time
	label.set_text(
		"Probing starts at %.3f V \n" %start_volt +
		"runs in %d ms " %pre_time + "to %.3f V\n" %pre_volt +
		"probes in %d ms " %probe_time + "to %.3f V\n" %probe_volt +
		"runs back in %d ms " %post_time + "to %.3f V\n" %post_volt +
		"total time is %d ms" %total_time)
	return 1

def create_log(_button):
	global AREA_SCAN
	name = "Data logger"
	win = gobject.new(gtk.Window,
			  type=gtk.WINDOW_TOPLEVEL,
			  title=name,
			  allow_grow=gtk.TRUE,
			  allow_shrink=gtk.TRUE,
			  border_width=5)
	wins[name] = win
	win.connect("destroy", lambda w: win.hide())

	box1 = gobject.new(gtk.VBox(gtk.FALSE,0))
	win.add(box1)

	scrolled_window = gtk.ScrolledWindow()
	scrolled_window.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
	scrolled_window.set_size_request(600, 400)
	liste = gtk.ListStore(gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT, gobject.TYPE_INT)
	tree_view = gtk.TreeView(liste)
	scrolled_window.add(tree_view)
	descriptor = ["start", "stop", "srcs", "Xchan", "nx_init[1]", "nx_init[2]", "nx_init[3]", "nx_finish[1]", "nx_finish[2]", "nx_finish[3]", "f_dx_init[1]", "f_dx_init[2]", "f_dx_init[3]", "f_dx_finish[1]", "f_dx_finish[2]", "f_dx_finish[3]", "nx", "dnx", "f_dx", "AC_amp", "AC_frq", "AC_phase1st", "AC_phase2nd", "AC_nAve", "dummy_alignment", "Xpos", "LockIn_0", "LockIn_1st", "LockIn_2nd", "ix", "iix", "state", "pflg"]
	cell = gtk.CellRendererText()
	for tap in range(33):
		column = gtk.TreeViewColumn(descriptor[tap], cell, text=tap)
		tree_view.append_column(column)
	
	gobject.timeout_add(updaterate, new_log_entry, liste, tree_view)
	box1.pack_start(scrolled_window)

	box2 = gobject.new(gtk.HBox(gtk.FALSE,0))
	box1.pack_start(box2)

	start = gtk.Button("Start")
	start.set_size_request(80, 10)
	start.connect("clicked", change_flag, "LOG_on")
	box2.pack_start(start)

	stop = gtk.Button("Stop")
	stop.set_size_request(80, 20)
	stop.connect("clicked", change_flag, "LOG_off")
	box2.pack_start(stop)

	clear = gtk.Button("Clear")
	clear.set_size_request(80, 20)
	clear.connect("clicked", lambda w: liste.clear())
	box2.pack_start(clear)


	wins[name].show_all()

def new_log_entry(liste, tree_view):
	global flag
	global PROBE
	if (flag & 0x0001):
		iter = liste.append()
		for tap in range(33):
			liste.set(iter, tap, PROBE[tap])
	return 1

def change_flag(_button, todo):
	global flag
	if(todo=="LOG_on"):
		flag = (flag | 0x0001)
	if(todo=="LOG_off"):
		flag = (flag & 0xFFFE)
		


# create info dialog
def create_info(_button):
	global MAGIC
	name = "Area scan test utility v. 0.1"
	if not wins.has_key(name):
		win = gobject.new(gtk.Window,
				  type=gtk.WINDOW_TOPLEVEL,
				  title=name,
				  allow_grow=gtk.TRUE,
				  allow_shrink=gtk.TRUE,
				  border_width=10)
		wins[name] = win
		win.set_size_request(300, 220)
		win.connect("delete_event", lambda w: win.hide())

		box1 = gobject.new(gtk.VBox())
		win.add(box1)
		box1.show()

		box2 = gobject.new(gtk.VBox(spacing=0))
		box2.set_border_width(10)
		box1.pack_start(box2)
		box2.show()

		lab = gobject.new(gtk.Label, label="General SRanger DSP code info\n")
		box2.pack_start(lab)
		lab.show()

		lab = gobject.new(gtk.Label, label="MAGIC ID: %04X" %MAGIC[0])
		box2.pack_start(lab)
		lab.show()

		lab = gobject.new(gtk.Label, label="SPM Version: %04X" %MAGIC[1])
		box2.pack_start(lab)
		lab.show()

		lab = gobject.new(gtk.Label, label="Software ID: %04X" %MAGIC[4])
		box2.pack_start(lab)
		lab.show()

		lab = gobject.new(gtk.Label, label="DSP Code Released: " + "%02X." %(MAGIC[3] & 0xFF) + "%02X." %((MAGIC[3] >> 8) & 0xFF) +"%X " %MAGIC[2])
		box2.pack_start(lab)
		lab.show()

		separator = gobject.new(gtk.HSeparator())
		box1.pack_start(separator, expand=gtk.FALSE)
		separator.show()

		lab = gobject.new(gtk.Label, label="MAGICs:\n" + str(MAGIC))
		box2.pack_start(lab)
		lab.show()

		separator = gobject.new(gtk.HSeparator())
		box1.pack_start(separator, expand=gtk.FALSE)
		separator.show()

		box2 = gobject.new(gtk.VBox(spacing=10))
		box2.set_border_width(10)
		box1.pack_start(box2, expand=gtk.FALSE)
		box2.show()
		
		button = gtk.Button(stock='gtk-close')
		button.connect("clicked", lambda w: win.hide())
		box2.pack_start(button)
		button.set_flags(gtk.CAN_DEFAULT)
		button.grab_default()
		button.show()
	wins[name].show()

def change_value(_button, parent_window, identifier, structure, tap):
	global PROBE
	global AREA_SCAN
	name = identifier
	win = gobject.new(gtk.Window,
			  type=gtk.WINDOW_TOPLEVEL,
			  title=name,
			  allow_grow=gtk.TRUE,
			  allow_shrink=gtk.TRUE,
			  border_width=10)
	wins[name] = win
	win.set_modal(gtk.TRUE)
	win.set_transient_for(parent_window)
	win.set_size_request(160, 100)
	box1 = gobject.new(gtk.VBox())
	win.add(box1)
	box1.show()
	box2 = gobject.new(gtk.HBox(spacing=0))
	box2.set_border_width(10)
	box1.pack_start(box2)
	box2.show()
	
	if (structure == "PROBE"):
		adj = gtk.Adjustment(PROBE[tap], -2147483648, 2147483648, 1, 1, 0)
	if (structure == "AREA_SCAN"):
		adj = gtk.Adjustment(AREA_SCAN[tap], -2147483648, 2147483648, 1, 1, 0)

	spinner = gtk.SpinButton(adj, 0 , 0)
	spinner.set_wrap(gtk.FALSE)
	#spinner.set_update_policy(gtk.UPDATE_ALWAYS)
	spinner.set_numeric(gtk.TRUE)
	spinner.connect("value-changed", store_value, win, spinner, structure, tap)
	box2.pack_start(spinner)

	box3 = gobject.new(gtk.HBox())
	box1.pack_start(box3)

	ok = gtk.Button(stock='gtk-ok')
	ok.set_size_request(80, 10)
	ok.connect("clicked", store_value, win, spinner, structure, tap)
	box3.pack_start(ok)

	cancel = gtk.Button(stock='gtk-cancel')
	cancel.set_size_request(80, 20)
	cancel.connect("clicked", lambda w: win.hide())
	box3.pack_start(cancel)

	wins[name].show_all()

def store_value(_button, parent_window, _object, structure, tap):
	global PROBE
	global AREA_SCAN
	sr = open ("/dev/sranger0", "r+b")
	if (structure == "AREA_SCAN"):
		print("+++++++aktualisiere AREA_SCAN+++++++++++")
		TEMP = AREA_SCAN[:tap] + (int(_object.get_value()),) + AREA_SCAN[(tap+1):]
		os.lseek (sr.fileno(), MAGIC[11], 0)
		sr.write (apply(struct.pack, [">hhhhhhhhhhhhhhlllllhhhhllllhhhhhh"]  + list(TEMP)))
		print(TEMP)
	if (structure == "PROBE"):
		print("+++++++aktualisiere PROBE+++++++++++")
		TEMP = PROBE[:tap] + (int(_object.get_value()),) + PROBE[(tap+1):]
		os.lseek (sr.fileno(), MAGIC[13], 0)
		sr.write (apply(struct.pack, [">hhhhhhhhhhllllllhhlhhhhhhllllhhhh"]  + list(TEMP)))
		print(TEMP)
	sr.close ()
	get_status()
	parent_window.hide()

def start_scan(_button):
	global MAGIC
	global AREA_SCAN
	global PROBE
	global DATAFIFO
	global PROBEDATAFIFO
	
	sr = open ("/dev/sranger0", "r+b")
	os.lseek (sr.fileno(), MAGIC[16]+1, 0)
	sr.write (struct.pack (">h", 0))
	sr.close ()
	sr = open ("/dev/sranger0", "r+b")
	os.lseek (sr.fileno(), MAGIC[13], 0)
	sr.write (struct.pack (">h", 1))
	sr.close ()
	return 1


def create_main_window():
	buttons = {
		   "Settings": create_setting,
		   "Start Scan": start_scan,
		   "Probe info": create_probe_info,
		   "Data logger": create_log,
	   	 }
	win = gobject.new(gtk.Window,
	             type=gtk.WINDOW_TOPLEVEL,
                     title='Debug Tool v0.1',
                     allow_grow=gtk.TRUE,
                     allow_shrink=gtk.TRUE,
                     border_width=5)
	win.set_name("main window")
	win.set_size_request(220, 220)
	win.connect("destroy", lambda w: gtk.main_quit())

	box1 = gobject.new(gtk.VBox(gtk.FALSE, 0))
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
	k = buttons.keys()
	k.sort()
	for i in k:
		button = gobject.new(gtk.Button, label=i)

		if buttons[i]:
			button.connect("clicked", buttons[i])
		else:
			button.set_sensitive(gtk.FALSE)
		box2.pack_start(button)
		button.show()

	separator = gobject.new(gtk.HSeparator())
	box1.pack_start(separator, expand=gtk.FALSE)
	separator.show()
	box2 = gobject.new(gtk.VBox(spacing=10))
	box2.set_border_width(5)
	box1.pack_start(box2, expand=gtk.FALSE)
	box2.show()
	button = gtk.Button(stock='gtk-close')
	button.connect("clicked", lambda w: gtk.main_quit())
	button.set_flags(gtk.CAN_DEFAULT)
	box2.pack_start(button)
	button.grab_default()
	button.show()
	create_startup_info(win)
	win.show()

def refresh(object, structure, tap):
	global AREA_SCAN
	global PROBE
	if (structure == "PROBE"):
		object("%i" %PROBE[tap])
	if (structure == "AREA_SCAN"):
		object("%i" %AREA_SCAN[tap])
	return 1
	
def get_status():
	global AREA_SCAN
	global PROBE
	global DATAFIFO
	global PROBEDATAFIFO

	sr = open ("/dev/sranger0", "r+b")

	fmt = ">hhhhhhhhhhhhhhlllllhhhhllllhhhhhh"
	os.lseek (sr.fileno(), MAGIC[11], 0)
	AREA_SCAN = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))

	fmt = ">hhhhhhhhhhllllllhhlhhhhhhllllhhhh"
	os.lseek (sr.fileno(), MAGIC[13], 0)
	PROBE = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
	
	fmt = ">hhhhhh"
	os.lseek (sr.fileno(), MAGIC[15], 0)
	DATAFIFO = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))

	fmt = ">hhhhhh"
	os.lseek (sr.fileno(), MAGIC[16], 0)
	PROBEDATAFIFO = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))

	sr.close ()
	return 1

def create_startup_info(parent):
	name = "Warning!"
	win = gobject.new(gtk.Window,
			  type=gtk.WINDOW_TOPLEVEL,
			  title=name,
			  allow_grow=gtk.TRUE,
			  allow_shrink=gtk.TRUE,
			  border_width=10)
	wins[name] = win
	win.set_modal(gtk.TRUE)
	win.set_size_request(240, 200)
	win.set_transient_for(parent)

	box1 = gobject.new(gtk.VBox())
	win.add(box1)

	lab = gobject.new(gtk.Label, label="Warning:\nThis is a debuging and low\nlevel configuration tool.\nMissusage can lead to tip\ncrash or other problems!")
	box1.pack_start(lab)


	separator = gobject.new(gtk.HSeparator())
	box1.pack_start(separator, expand=gtk.TRUE)

	button = gtk.Button(stock='gtk-close')
	button.connect("clicked", lambda w: win.hide())
	box1.pack_start(button)

	wins[name].show_all()


updaterate = 500
flag = 0x0000

print ("read magic structure...")
sr = open ("/dev/sranger0", "r+b")
fmt = ">HHHHHHHHHHHHHHHHHHHHHH"
os.lseek (sr.fileno(), 0x4000, 0)
MAGIC = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
sr.close ()


get_status()
gobject.timeout_add(updaterate, get_status)	


create_main_window()
gtk.main()

print ("bye, bye")
