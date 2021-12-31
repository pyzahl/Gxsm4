#!/usr/bin/env python
#
# Script for reading values from MK2
# ============================================
#   dalibor.sulc@sventek.net october 2013
# 
version = "0.2"

import pygtk
pygtk.require('2.0')

import gobject, gtk
import os		# use os because python IO is bugy
import time
import sys
import telnetlib
import string
import threading
import numpy
import struct
import re

# **** SETTINGS ****
signals = {
           0: "delta F",
           1: "fiber signal",
           3: "dissipation",
           4: "RMS",
           }
units = {
    0: "Hz",
    1: "mV",
    3: "V",
    4: "V",
    }

conversion = {
    0: 0.015,
    1: 1,
    3: 0.001,
    4: 0.001,
}

digits = {
    0: 1,
    1: 1,
    3: 3,
    4: 3,
}

updaterate = 80
sr_dev_path = "/dev/sranger_mk2_0"
magic_address = 0x5000

i_magic = 0
magic_fmt = "<HHHHHHHHHHHHHHHHHHHHHHHHHH"
i_AIC_in = 6
fmt_AIC_in = "<hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"
i_AIC_out = 7
level_fac = 0.305185095 # mV/Bit
averaging = 20 # values will take for averaging 

# global variables
labels = {}
vals = 0
avg_vals = {}
# **** SIGNALS ****
def destroy(*args):
    gtk.main_quit()

# **** END SIGNALS ****

# **** GUI **** 
def create_main_window():
    win = gobject.new(gtk.Window,
                      type = gtk.WINDOW_TOPLEVEL,
                      title = 'Value Reader ' + version,
                      allow_grow = True,
                      allow_shrink = True,
                      border_width = 0)
    win.set_name("main window")
    win.set_size_request(200,50*len(signals))
    win.connect("destroy", destroy)
    win.connect("delete_event", destroy)
    main_table = gtk.Table(len(signals),2,True)
    win.add(main_table)
    main_table.show()

    # labels with values
    k = signals.keys()
    
    j = 0
    for i in k:
        
        label = gtk.Label()
        label.set_markup("<small>" + signals[i] + "</small>")
        label.set_tooltip_markup("channel " + str(i))
        label.set_alignment(0.2,0.5)
        label.show()
        main_table.attach(label, 0, 1, j, j+1)
        
        labels[i] = gtk.Label()
        labels[i].set_markup("<big>" + str(12345)+" mV</big>")
        labels[i].set_tooltip_markup("channel " + str(i))
        labels[i].set_alignment(0.7,0.5)
        labels[i].show()
        main_table.attach(labels[i], 1, 2, j, j+1)
        j = j+1
    win.show()

# **** END GUI ****

def open_sranger():
	sr = open (sr_dev_path, "rw")
	sr.seek (magic_address, 1)
	magic = struct.unpack (magic_fmt, sr.read (struct.calcsize (magic_fmt)))
	sr.close ()
	return magic

def getValues():
    global avg_vals, vals
    k = signals.keys()
    values = getSignals()
    
    for i in k:
        unit = units[i]
        value = str( round(values[i]*level_fac*conversion[i] * pow(10,digits[i])) /  pow(10,digits[i]) ) + " " + unit
        if (round(values[i]*level_fac) < 9000):
            labels[i].set_markup("<big>" + value + "</big>")
        else:          
            labels[i].set_markup("<span foreground='red' size='large'>" + value + "</span>")

        if (vals == 0):
            avg_vals[i] = (values[i] * level_fac)
        elif (vals < averaging):
            avg_vals[i] =   avg_vals[i] + (values[i] * level_fac)
        else:
            labels[i].set_tooltip_markup(str(round(avg_vals[i]/averaging)))
            avg_vals[i] = 0

    if (vals <averaging):
        vals = vals+1
    else:
        vals = 0
    return 1

def getSignals():
    sr = open(sr_dev_path, "rb")
    os.lseek(sr.fileno(),magic[i_AIC_in], 0)
    AIC_in_buffer = struct.unpack (fmt_AIC_in, os.read (sr.fileno(), struct.calcsize (fmt_AIC_in)))
    sr.close()
    return AIC_in_buffer

magic = open_sranger()
create_main_window()
getValues()
gobject.timeout_add(updaterate, getValues)
gtk.main()
