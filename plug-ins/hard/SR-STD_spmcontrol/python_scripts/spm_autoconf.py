#!/usr/bin/env python

## * Autoconfiguration file
### * for the Signal Ranger STD/SP2 DSP board
## * 
## * Copyright (C) 2006 Bastian Weyers
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





###############################################################################
#
#      Please adapt the following settings to your own needs
#
###############################################################################

sranger_file_location = "/dev/sranger0"
offset_file_location = "~/offset"
loadusb_folder_location = "~/SRanger/loadusb64"
dspcode_file_location = "~/SRanger/TiCC-project-files/FB_spmcontrol/FB_spmcontrol.out"

# Gain for the AICs, 3dB means a factor of sqrt(2)
# available gains are -42 -36 -30 -24 -18 -12 -9 -6 -3 0 3 6 9 12 18 24
# The input gain of 3 is default for all channels, if you use an additional resistor of 5kOhm
# in series with the input. If you do not have, use gain 0 for each input channel.
# You will have to use gain 3 and the additional 5kOhm resistor, if you want to have the
# full input range of +/-10V. There is already an voltage devider with 2kOhm and 8kOhm. This 
# should make from +/-10V +/-2V. Unfortunatelly this is too close to the internal voltages of 
# the OPs - there are not working rail-to-rail. Therefore just add an (variable) 5kOhm in series 
# with the 8kOhm on the board and increase the gain to 3 dB. Of course, you have to calibrate 
# the input in this case. (stm@sf.net [Thorsten Wagner] 26.06.08) 
ADC_gain = [3,3,3,3,3,3,3,3]

# The gain of 6dB for channell 5 (usually the Z-signal) and 7 ist due to the fact that some
# filters are not enabled for theses channels. The filters are deactivated for these channels
# because the cause an additional delay. If the filters are turned off and the gain is 0dB the
# output is limited to +/-1V. Unless you change the settings for the filters keep it 
# 0,0,0,0,0,6,0,6. (stm@sf.net [Thorsten Wagner] 26.06.08)
DAC_gain = [0,0,0,0,0,6,0,6]


#external fb control: Feedback OFF if <ext_channel> is above <ext_treshold>
Ext_Control = False
ext_channel = 6
ext_treshold = 1.2

#enable for STM, usually handled by GXSM!
Log_transformation = True

#offset compensation for analog signals
#DAC offset in mV
ADC_Offset_Compensation = True
DAC6_offset = -4.9

#AIC[6] * (-1) -> AIC[5]
Z_Transformation = False

#linear mixing for FB srcs AIC[1] & AIC[5]
Fuzzy = False
fuzzy_level = 2.1
fuzzy_tap_gain = -.0344

#enable offset adding to scan signals: AIC[0/1/2] not used
Offset_Adding = True

#enable computation of additional slow AIC6/7 statistical data
AIC_Slow = False

#Pass signal to DAC7, ext_dump might be one of the following:
#Feedback = 0   X_Offset = 1   log[AIC5] = 2   FB delta = 3
Ext_pass = False
ext_dump = 0
ext_on   = 0.0
ext_off  = 2.0

###############################################################################
#
###############################################################################


version = "1.2"

import pygtk
pygtk.require('2.0')

import gobject, gtk
import os		# use os because python IO is bugy
import time
import struct
import array



wins = {}

def delete_event(win, event=None):
	win.hide()
	# don't destroy window -- just leave it hidden
	return True


# open SRanger, read magic offsets (look at FB_spm_dataexchange.h -> SPM_MAGIC_DATA_LOCATIONS)
def open_sranger():
	sr = open (sranger_file_location, "rw")
	sr.seek (0x4000, 1)
	magic_fmt = ">HHHHHHHHHHHHHHHHHHHHHH"
	magic = struct.unpack (magic_fmt, sr.read (struct.calcsize (magic_fmt)))
	sr.close ()
	return magic


def set_aic_gain():
	add_event("Upload gain settings / dB ...")
	add_event("Please check if the gains are sufficient \nfor your instrument.\nIf not change by editing this script.")
	gainreg = { -42: 15, -36: 1, -30: 2, -24: 3, -18: 4,  -12: 5,  -9: 6,  -6: 7,  -3: 8,  0: 0,  3: 9,   6: 10,  9: 11,  12: 12,  18: 13, 24: 14,}
	add_event("Channel\tADC-gain\tDAC-gain")
	sr = open (sranger_file_location, "wb")
	sr.seek (magic[8]+1+3*8, 1)	
	for channel in range(0,8):
		sr.write (struct.pack (">h", gainreg[ADC_gain[channel]]<<4 | gainreg[DAC_gain[channel]]))
		add_event("\t" + str(channel) + "\t\t" + str(ADC_gain[channel]) + "\t\t" + str(DAC_gain[channel]))
	sr.close ()
	# set mode MD_AIC_RECONFIG (==8) in statemachine	
	sr = open (sranger_file_location, "wb")
	sr.seek (magic[5], 1)
	sr.write (struct.pack (">h", 0x4000))
	sr.close ()
	add_event("")

def feature_settings():
	add_event("Upload ext_FB_control settings...")
	sr = open (sranger_file_location, "wb")
	sr.seek (magic[20], 1)
	sr.write (struct.pack (">h", ext_channel))
	sr.write (struct.pack (">h", int(ext_treshold*3276.7)))
	sr.write (struct.pack (">h", ext_dump))
	sr.write (struct.pack (">h", int(ext_on*16383.)))
	sr.write (struct.pack (">h", int(ext_off*16383.)))
	sr.close ()
	
	add_event("Upload fuzzy settings...")
	sr = open (sranger_file_location, "wb")
	sr.seek (magic[18], 1)
	sr.write (struct.pack (">h", int(fuzzy_level*3276.7)))
	sr.write (struct.pack (">h", int(fuzzy_tap_gain*32767.0)))
	sr.close ()
	
	add_event("Configure DSP settings...\n")
	set_option(Ext_Control, 0x0100)
	set_option(Log_transformation, 0x0008)
	set_option(ADC_Offset_Compensation, 0x0010)
	set_option(Z_Transformation, 0x0020)
	set_option(Fuzzy, 0x0040)
	set_option(Offset_Adding, 0x0080)
	set_option(AIC_Slow, 0x0200)
	set_option(Ext_pass, 0x0400)
	return 1


def set_option(toggle, mask):
	sr = open (sranger_file_location, "wb")
	if toggle:
		sr.seek (magic[5], 1)
	else:
		sr.seek (magic[5]+1, 1)
	sr.write (struct.pack (">h", mask))
	sr.close ()
	time.sleep(.2)


def ADC_offset_compensation():
	add_event("Upload offset settings / mV ...")
	array_fmt = ">hhhhhhhh"
	file = open (offset_file_location, "rb")
	array = file.read (struct.calcsize (array_fmt))
	file.close ()
	sr = open (sranger_file_location, "wb")
	sr.seek (magic[10]+16, 1)
	sr.write (array)
	sr.close ()
	sr = open (sranger_file_location, "wb")
	sr.seek (magic[10]+16+8+2*2*8+8+6*32+2*128+6, 1)
	sr.write (struct.pack (">h", int(DAC6_offset*3.2767*5)))
	sr.close ()
	add_event("Channel\tADC \tDAC")
	for channel in range(0,8):
		if channel == 6:
			add_event("     %d\t\t" %channel + "%0.1f\t" %(struct.unpack (array_fmt, array)[channel]*0.305) + "%0.1f" %DAC6_offset)
		else:
			add_event("     %d\t\t" %channel + "%0.1f\t" %(struct.unpack (array_fmt, array)[channel]*0.305) + "---")
	add_event("");


def loadusb():
	global magic
	add_event("Uploading DSP Code...")
	os.system("cd " + loadusb_folder_location + "; ./loadusb " + dspcode_file_location)
	time.sleep(.5)
	magic = open_sranger()
	add_event("Magic ID: %04X" %magic[0])
	add_event("SPM Version: %04X" %magic[1])
	add_event("Software ID: %04X" %magic[4])
	add_event("DSP Code Released: " + "%02X." %(magic[3] & 0xFF) + "%02X." %((magic[3] >> 8) & 0xFF) +"%X \n" %magic[2])


def check_files():
	if (os.access(sranger_file_location, 2) == 0):
		add_event("Error!\nCould not open Signal Ranger\ndevice: /dev/sranger0")
		add_event("\nPlease check if:\n 1. all cables are pluged in\n 2. sranger is powered on")
		add_event(" 3. permissions are set to r+w")
		return False
	if (os.access(loadusb_folder_location + "/loadusb", 0) == 0):
		add_event("Error!\nCould not start ./loadusb from:\n" + loadusb_folder_location)
		add_event("\nPlease enter the correct path in the\nspm_autoconf.py script for:\n")
		add_event("loadusb_folder_location = ")
		return False
	if (os.access(dspcode_file_location, 0) == 0):
		add_event("Error!\nCould not open DSP Code:\n" + dspcode_file_location)
		add_event("\nPlease enter the correct path in the\nspm_autoconf.py script for:\n")
		add_event("dspcode_file_location = ")
		return False
	if (os.access(offset_file_location, 0) == 0):
		add_event("Error!\nCould not open offset file:\n" + offset_file_location)
		add_event("\nPlease enter the correct path in the\nspm_autoconf.py script for:\n")
		add_event("offset_file_location = ")
		return False
	return True


def go_button(button):
	event_list.set_label("")
        if check_files():
		loadusb()
		set_aic_gain()
		ADC_offset_compensation()
		feature_settings()
	button.disconnect(go_button_handler)
	button.set_label("gtk-quit")
	button.connect("clicked", destroy)
	
def destroy(*args):
        gtk.main_quit()


# adds a new entry to the event_list
def add_event(text):
	event_list.set_label(event_list.get_label() + text + "\n")
	return


def create_main_window():
	global event_list
	global go_button_handler
	win = gobject.new(gtk.Window,
	             type=gtk.WINDOW_TOPLEVEL,
                     title='SRanger AutoConfigurator '+version,
                     allow_grow=True,
                     allow_shrink=True,
                     border_width=5)
	win.set_name("main window")
	win.set_size_request(300, 320)
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
	
	event_list = gobject.new(gtk.Label, label="Welcome to the SRanger\nAutoConfigurator v. " + version + "\n\n")
	add_event("This script will:")
	add_event(" 1. Check for the SRanger board")
	add_event(" 2. Upload the DSP code")
	add_event(" 3. Configure the DSP settings")
	add_event("\n\nBeware: All Settings can easily be changed\nby editing the spm_autoconf.py.")
	box2.pack_start(event_list)
	event_list.show()

	separator = gobject.new(gtk.HSeparator())
	box1.pack_start(separator, expand=False)
	separator.show()
	box2 = gobject.new(gtk.VBox(spacing=10))
	box2.set_border_width(5)
	box1.pack_start(box2, expand=False)
	box2.show()
	button = gtk.Button(stock="gtk-go-forward")
	go_button_handler = button.connect("clicked", go_button)
	button.set_flags(gtk.CAN_DEFAULT)
	box2.pack_start(button)
	button.grab_default()
	button.show()
	win.show()

create_main_window()

gtk.main()
print ("Byby.")

