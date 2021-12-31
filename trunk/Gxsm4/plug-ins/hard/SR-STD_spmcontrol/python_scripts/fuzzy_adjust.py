#!/usr/bin/env python

## * Python User Control for
## * A Dolby (Digital) Pro-Logic (Thx) emulation and demonstration
## * for the Signal Ranger SP2 DSP board
## * Additional Sound Field processing options and more...
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


import pygtk
pygtk.require('2.0')

import gobject, gtk

#import GtkExtra
import struct
import array
from gtk import TRUE, FALSE

wins = {}

def destroy(*args):
        gtk.main_quit()

# open SRanger, read magic struct
def open_sranger():
	sr = open ("/dev/sranger0", "rw")
	sr.seek (0x4000, 1)
	magic_fmt = ">HHHHHHHHHHHHHHHHHHHHH"
	magic = struct.unpack (magic_fmt, sr.read (struct.calcsize (magic_fmt)))
	sr.close ()
	return magic

# update one FUZZY param at given address
def fuzzy_param_update(_adj):
	param_address = _adj.get_data("fuzzy_param_address")
	param = _adj.get_data("fuzzy_param_multiplier")*_adj.value
#	print ("Address:", param_address)
#	print ("Value:", _adj.value)
#	print ("Data:", param)
	sr = open ("/dev/sranger0", "wb")
	sr.seek (param_address, 1)
	sr.write (struct.pack (">h", param))
	sr.close ()

# update one FUZZY param at given address
def int_param_update(_adj):
	param_address = _adj.get_data("int_param_address")
	param = int (_adj.get_data("int_param_multiplier")*_adj.value)
#	print ("Address:", param_address)
#	print ("Value:", _adj.value)
#	print ("Data:", param)
	sr = open ("/dev/sranger0", "wb")
	sr.seek (param_address, 1)
	sr.write (struct.pack (">h", param))
	sr.close ()

def toggle_flag(w, mask):
	sr = open ("/dev/sranger0", "wb")
	if w.get_active():
		sr.seek (magic[5], 1)
	else:
		sr.seek (magic[5]+1, 1)
		
	sr.write (struct.pack (">h", mask))
	sr.close ()

# create FUZZY editor for FUZZY at fuzzy_address and with name
def create_fuzzy_edit(fuzzy_address, name):
	if not wins.has_key(name):
		win = gobject.new(gtk.Window,
				  type=gtk.WINDOW_TOPLEVEL,
				  title=name,
				  allow_grow=gtk.TRUE,
				  allow_shrink=gtk.TRUE,
				  border_width=10)
		wins[name] = win
		win.connect("delete_event", destroy)
		
		box1 = gobject.new(gtk.VBox())
		win.add(box1)
		box1.show()

		box2 = gobject.new(gtk.VBox(spacing=0))
		box2.set_border_width(10)
		box1.pack_start(box2)
		box2.show()

		# get current FUZZY parameters: FUZZY.level and FUZZY.gain
		sr = open ("/dev/sranger0", "rw")
		sr.seek (fuzzy_address, 1)
		fmt = ">h"
		level = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
		gain  = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
		sr.close ()

		print ("FUZZYadr:", fuzzy_address)
		print ("Level:", level)
		print ("Gain:", gain)
		
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
			hbox = gobject.new(gtk.HBox())
			box2.pack_start(hbox)
			hbox.show() #value=0, lower=0, upper=0, step_incr=0, page_incr=0, page_size=0
			# "value", "lower", "upper", "step_increment", "page_increment", "page_size"

			adjustment = gtk.Adjustment (value=level[tap]/level_fac, lower=-level_max, upper=level_max, step_incr=0.001, page_incr=0.05, page_size=0.1)
#			adjustment.set_all(value=level[tap]/level_fac, lower=0, upper=level_max, step_increment=0.001, page_increment=0.05, page_size=0.1)

			scale = gobject.new(gtk.HScale, adjustment=adjustment)
			scale.set_size_request(150, 30)
			scale.set_update_policy(gtk.UPDATE_DELAYED)
			scale.set_digits(3)
			scale.set_draw_value(gtk.TRUE)
			hbox.pack_start(scale)
			scale.show()
			adjustment.set_data("fuzzy_param_address", fuzzy_address) 
			adjustment.set_data("fuzzy_param_multiplier", level_fac) 
			adjustment.connect("value-changed", fuzzy_param_update)
			
			adjustment = gtk.Adjustment (value=gain[tap]/gain_fac, lower=-gain_max, upper=gain_max, step_incr=0.001, page_incr=0.02, page_size=0.02)
#			adjustment.set_all(value=gain[tap]/gain_fac, lower=-gain_max, upper=gain_max, step_increment=0.001, page_increment=0.02, page_size=0.02)
			scale = gobject.new(gtk.HScale, adjustment=adjustment)
			scale.set_size_request(150, 30)
			scale.set_update_policy(gtk.UPDATE_DELAYED)
			scale.set_digits(3)
			scale.set_draw_value(gtk.TRUE)
			hbox.pack_start(scale)
			scale.show()
			adjustment.set_data("fuzzy_param_address", fuzzy_address+1) 
			adjustment.set_data("fuzzy_param_multiplier", gain_fac) 
			adjustment.connect("value-changed", fuzzy_param_update)

			check_button = gtk.CheckButton("_FUZZY")
			sr = open ("/dev/sranger0", "rb")
			sr.seek (magic[5]+2, 1)
			fmt = ">h"
			flag = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
			if flag[0] & 0x0040:
				check_button.set_active(gtk.TRUE)
			sr.close ()
			check_button.connect('toggled', toggle_flag, 0x0040)
			hbox.pack_start(check_button)
			check_button.show()

			check_button = gtk.CheckButton("_OffComp")
			if flag[0] & 0x0010:
				check_button.set_active(gtk.TRUE)
			check_button.connect('toggled', toggle_flag, 0x0010)
			check_button.show()
			hbox.pack_start(check_button)

			check_button = gtk.CheckButton("_OffAdd")
			if flag[0] & 0x0080:
				check_button.set_active(gtk.TRUE)
			check_button.connect('toggled', toggle_flag, 0x0080)
			check_button.show()
			hbox.pack_start(check_button)
	wins[name].show()

magic = open_sranger()
print ("Magic:", magic)
create_fuzzy_edit(magic[18], "DFM Fuzzy Control")
gtk.main()
print ("Byby.")

