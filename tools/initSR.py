#!/usr/bin/python3 -Es

## * initialize your Signal Ranger board
## * and load DSP Code
## *
## * code is based on spm_autoconf.py from Bastian Weyers
## * 
## * Copyright (C) 2009 Thorsten Wagner / ventiotec (R) Dolega und Wagner GbR
## * Copyright (C) 2020 Thorsten Wagner / ventiotec (R) Wagner und Muenchow GbR
## *
## * Author: Thorsten Wagner <linux@ventiotec.com>
## * WWW Home: http://sranger.sf.net / http://www.ventiotec.com
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


###############################################################################
#
#      Please adapt the following settings to your own needs
#
###############################################################################
import os		# use os because python IO is bugy
## settings for your SR-STD or SR-SP2
sranger_file_location = "/dev/sranger0"
loadusb_folder_location = os.environ['HOME']+"/SRanger/loadusb64"
dspcode_file_location = os.environ['HOME']+"/SRanger/TiCC-project-files/FB_spmcontrol/FB_spmcontrol.out"

## settings for your SR-MK2 with A810
sranger_mk2_file_location = "/dev/sranger_mk2_0"
loadusb_mk2_folder_location = os.environ['HOME']+"/SRanger/loadusb-mk2"
dspcode_mk2_file_location = os.environ['HOME']+"/SRanger/TiCC-project-files/MK2-A810_FB_spmcontrol/FB_spmcontrol.out"


###############################################################################
# declarations and imports
###############################################################################


version = "2.0 (31.10.2020)"

import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GLib 

import time
import struct
import array

wins = {}

def delete_event(win, event=None):
	win.hide()
	# don't destroy window -- just leave it hidden
	return True


# open SRanger STD/SP2, read magic offsets (look at FB_spm_dataexchange.h -> SPM_MAGIC_DATA_LOCATIONS)
def open_sranger():
	sr = open (sranger_file_location, "rw")
	sr.seek (0x4000, 1)
	magic_fmt = ">HHHHHHHHHHHHHHHHHHHHHH"
	magic = struct.unpack (magic_fmt, sr.read (struct.calcsize (magic_fmt)))
	sr.close ()
	return magic

# open SRanger MK2, read magic offsets (look at FB_spm_dataexchange.h -> SPM_MAGIC_DATA_LOCATIONS)
def open_mk2_sranger():
	sr = open (sranger_mk2_file_location, "rw")
	sr.seek (0x5000, 1)
	magic_fmt = "<HHHHHHHHHHHHHHHHHHHHHHHHHH"
	magic = struct.unpack (magic_fmt, sr.read (struct.calcsize (magic_fmt)))
	sr.close ()
	return magic

def init():
# for ST-STD
	os.system("cd")
	os.system("sudo mount --type debugfs none /sys/kernel/debug/")
	time.sleep(1)
	os.system("sudo fxload-sr-i386 -I SRangerFx -D /dev/bus/usb/`cat /sys/kernel/debug/usb/devices | grep \"T:\|P:\" | grep -B1 0a59 | tac | tail -n1 | awk -F \"=\" '{print $2 $7}' | awk '{ printf \"%03d/%03d\",$1,$3 }'`")
	time.sleep(3)
# changing attributes for the devices. Should be obsolent if the rights are already set in the defaults of the udev file system
# for SR-STD/SP2
	os.system("sudo chmod 666 " + sranger_file_location)
# for SR-MK2
	os.system("sudo chmod 666 " + sranger_mk2_file_location)

def loadusb():
	global magic
	add_event("Testing availible devices.\n")
	# Check if SR-STD/SP2 device exists
	if (os.access(sranger_file_location, 0) == 0):
		add_event("No device " + sranger_file_location + " found.\n")
	else: 
		add_event("Device SR-STD/SP2 found!\n")
		# Check if path to loadusb and DSP code (for SR-STD/SP2) exist		
		if (os.access(loadusb_folder_location + "/loadusb", 0) == 0):
			add_event("Error!\nCould not start ./loadusb from:\n" + loadusb_folder_location)
			add_event("\nPlease enter the correct path in the\ninitSR.py script for:\n")
			add_event("loadusb_folder_location = ")
			return False
		if (os.access(dspcode_file_location, 0) == 0):
			add_event("Error!\nCould not open DSP Code:\n" + dspcode_file_location)
			add_event("\nPlease enter the correct path in the\ninitSR.py script for:\n")
			add_event("dspcode_file_location = ")
			return False
		# Load DSP-code to SR-STD/SP2
		os.system("cd " + loadusb_folder_location + "; ./loadusb " + dspcode_file_location)
		time.sleep(2)
		magic = open_sranger()
		add_event("Magic ID: %04X" %magic[0])
		add_event("SPM Version: %04X" %magic[1])
		add_event("Software ID: %04X" %magic[4])
		add_event("DSP Code Released: " + "%02X." %(magic[3] & 0xFF) + "%02X." %((magic[3] >> 8) & 0xFF) +"%X \n" %magic[2])

	# Check if SR-MK2 device exists	
	if (os.access(sranger_mk2_file_location, 0) == 0):
		add_event("No device " + sranger_mk2_file_location + " found.\n")
	else: 
		add_event("Device SR-MK2 found!\n")		
		# Check if path to loadusb and DSP code (for SR-MK2) exist
		if (os.access(loadusb_mk2_folder_location + "/loadusb", 0) == 0):
			add_event("Error!\nCould not start ./loadusb from:\n" + loadusb_mk2_folder_location)
			add_event("\nPlease enter the correct path in the\ninitSR.py script for:\n")
			add_event("loadusb_mk2_folder_location = ")
			return False
		if (os.access(dspcode_mk2_file_location, 0) == 0):
			add_event("Error!\nCould not open DSP Code for MK2:\n" + dspcode_mk2_file_location)
			add_event("\nPlease enter the correct path in the\ninitSR.py script for:\n")
			add_event("dspcode_mk2_file_location = ")
			return False
		# Load DSP-Code to SR-MK2		
		# os.system("cd " + loadusb_mk2_folder_location + "; ./loadusb " + dspcode_mk2_file_location)
		# time.sleep(.5)
		magic = open_mk2_sranger()
		add_event("Magic ID: %04X" %magic[0])
		add_event("SPM Version: %04X" %magic[1])
		add_event("Software ID: %04X" %magic[4])
		add_event("DSP Code Released: " + "%02X." %(magic[3] & 0xFF) + "%02X." %((magic[3] >> 8) & 0xFF) +"%X \n" %magic[2])
		add_event("Upload of DSP code to \nthe SR-MK2 not possible yet.\n")
		add_event("Please use the MiniDebugger\nunder windows shipped with\nyour board.")
		# For now - 14.06.2009 - the loadusb for the mk2 is still buggy under windows. So please use the windows tool. tw	
def go_button(button):
	event_list.set_label("")
	init()
	loadusb()
	button.disconnect(go_button_handler)
	button.set_label("_Quit")
	button.connect("clicked", destroy)
	
def destroy(*args):
        Gtk.main_quit()


# adds a new entry to the event_list
def add_event(text):
	event_list.set_label(event_list.get_label() + text + "\n")
	return


def create_main_window():
	global event_list
	global go_button_handler
	win = Gtk.Window(title="initSR")
	win.set_size_request(300, 320)
	win.connect("destroy", destroy)
	win.connect("delete_event", destroy)
	
	box1 = Gtk.VBox()
	win.add(box1)
	box1.show()
	scrolled_window = Gtk.ScrolledWindow()
	scrolled_window.set_policy(Gtk.PolicyType.AUTOMATIC,Gtk.PolicyType.AUTOMATIC)
	box1.pack_start(scrolled_window,True, True, 0)
	scrolled_window.show()
	
	box2 = Gtk.VBox()
	box2.set_border_width(5)
	scrolled_window.add(box2)
	box2.show()
	
	event_list = Gtk.Label(label="Welcome to the SRanger\ninitSR script v. " + version + "\n\n")
	add_event("This script will:")
	add_event(" 1.\tTry to generate the usb \n\tdevices related to your\n\tSRanger board")
	add_event(" 2.\tUpload the right DSP code")
	box2.pack_start(event_list,True, True,0)
	event_list.show()

	box2 = Gtk.VBox()
	box2.set_border_width(5)
	box1.pack_start(box2, True, True, 0)
	box2.show()
	button = Gtk.Button.new_with_mnemonic("_Next")
	go_button_handler = button.connect("clicked", go_button)
	box2.pack_start(button, True, True, 0)
	button.set_can_default(True) 
	button.grab_default()
	button.show()
	win.show()

create_main_window()

Gtk.main()
print ("Bye bye!")

