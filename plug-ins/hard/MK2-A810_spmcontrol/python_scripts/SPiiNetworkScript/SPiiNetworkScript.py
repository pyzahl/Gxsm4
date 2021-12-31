#!/usr/bin/env python
#
# Script for autoapproaching with SPii and MK2
# ============================================
#   dalibor.sulc@sventek.net september 2013
# 
version = "0.0"

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

try: 
	import gtk.glade
except:
	sys.exit(1)

# global variables
AutoAppOn = False
updaterate = 100 # ms (for reading possition)
staterate = 80   # ms (for state machine for autoapproach) 
switch = 0       # switch for ploting possition into gui
app_ci = 0       # original feedback constant 
retract_ci = 0   # feedback constant for retraction
Zspan = 1000     # tolerance for full retract/elongate
statetimer = 0   # init of state machine timer
max_delay = 40   # max cycles for retracting (for turning off approaching)
X_enable = False # enable X axis
Z_enable = False # enable Z axis
level_fac = 0.305185095 # mV/Bit

red_color = "#FFBBBB"
green_color = "#BBFFBB"

class SPiiNetwork: # class for communication with SPii controler via LAN network
	# class variables
	IP = "130.199.242.45"
	port = 701
	tn = 0
	# contructor
	def __init__(self):
		self.tn = telnetlib.Telnet(self.IP, self.port);

	# destructor
	def __del__(self):
		self.tn.close()

	# sender
	def send(self,message):
		self.tn.write(message+"\r")
		#print (message+"\r")
		return True

	def sendRead(self,message):
		self.send(message)
		received = self.tn.read_until(":")
		#print received
		if (len(received) > 2):
			return float(received.replace(" ","").replace(":",""))
		else:
			return float("nan")
	# getters
	def getPos(self,axis):
		return self.sendRead("?FPOS("+str(axis)+")")

	def getStatus (self, axis, parameter):
		return self.sendRead("?MST("+str(axis)+").#"+parameter)
	
       	# setters
	def enable(self,axis):
		self.sendRead("enable " + str(axis))

	def disable(self,axis):
		self.sendRead("disable " + str(axis))
		     
	def makeStep(self,axis,velocity,step):
		self.sendRead("ptp/rv " + str(axis) + ", " + str(step) + ", " + str(velocity))
		time.sleep(0.1)

##### END of SPiiNetwork #####	

class DSP: # class for communication with DSP MK2-A810
	# class variables
	sr_dev_path = "/dev/sranger_mk2_0"
	magic_address = 0x5000
	magic_fmt = "<HHHHHHHHHHHHHHHHHHHHHHHHHH"
	magic = 0
	i_AIC_in = 6
	fmt_AIC_in = "<hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"
	i_feedback = 9
	i_AIC_out = 7
	
	def __init__(self):
		self.magic = self.open_sranger()
		#print self.magic
		
	def __del__(self):
		return True

	def open_sranger(self):
		sr = open(self.sr_dev_path,"rw")
		sr.seek(self.magic_address,1)
		magic = struct.unpack(self.magic_fmt, sr.read (struct.calcsize(self.magic_fmt)))
		sr.close()
		return magic

 	def getZ(self): # get position of Z
		sr = open(self.sr_dev_path,"rb")
		sr.seek(self.magic[self.i_AIC_out]+5,1)
		fmt = "<h"
		data = struct.unpack(fmt,sr.read(struct.calcsize(fmt)))
		sr.close()
		return data[0]

	def getCi(self): # get value of Ci (feedback)
		sr = open(self.sr_dev_path,"rb")
		sr.seek(self.magic[self.i_feedback]+1,1)
		fmt = "<h"
		data = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
		sr.close()
		return data[0]
	
	def setCi(self,value):
		sr = open(self.sr_dev_path, "wb")
		sr.seek(self.magic[self.i_feedback]+1, 1)
		sr.write(struct.pack("<h", value))
		sr.close()
		return True

	def getSignal(self):
		sr = open(self.sr_dev_path, "rb")
		os.lseek(sr.fileno(),self.magic[self.i_AIC_in], 0)
		AIC_in_buffer = struct.unpack (self.fmt_AIC_in, os.read (sr.fileno(), struct.calcsize (self.fmt_AIC_in)))
		sr.close()
		return AIC_in_buffer

	def getDissipation(self):
		global avg_vals, vals, level_fac
		values = self.getSignal()
		return values[3]*level_fac
			
# create global instance of telnet
t = SPiiNetwork()

# create instance of DSP
dsp = DSP()

# buttons
def onClickXGO(button):
	global t, builder, X_enable
	if (X_enable):
		builder.get_object("infoPanel").set_text("")
		vel = float(builder.get_object("XVel_entry").get_text())
		step = float(builder.get_object("XStep_entry").get_text())
		t.makeStep(0,vel,step)
	else:
		builder.get_object("infoPanel").set_text("Axis is not enabled!")	
	     

def onClickZGO(button):
	global t, builder, Z_enable
	if (Z_enable):
		builder.get_object("infoPanel").set_text("")
		vel = float(builder.get_object("ZVel_entry").get_text())
		step = float(builder.get_object("ZStep_entry").get_text())
		t.makeStep(1,vel,step)
	else:
		builder.get_object("infoPanel").set_text("Axis is not enabled!")

def onClickAutoApp(button):
	global AutoAppOn, updaterate, dsp, app_ci, retract_ci, state, Z_enable
	builder.get_object("infoPanel").set_text("")
	step = float(builder.get_object("ZStep_entry").get_text())

	if (not(Z_enable)):
		builder.get_object("infoPanel").set_text("Axis is not enabled!")
	elif (step < -5):
		builder.get_object("infoPanel").set_text("Step is too large!")
	elif (step > 0):
		builder.get_object("infoPanel").set_text("Step is wrong way (should be negative)!")
	else:
		if (AutoAppOn == False): # start approaching
			button.set_label("Stop Approach")
			builder.get_object("infoPanel").set_text("Approaching...")
			app_ci = dsp.getCi()
			# in case of negative Ci
			if (app_ci <= 0):
				app_ci = 20
			retract_ci = app_ci*-10
			state = 1 # turn on state machine
			statetimer = gobject.timeout_add(staterate, stateMachine)
			AutoAppOn = True
		else: # stop approaching
			button.set_label("AutoApproach")
			state = 0
			builder.get_object("infoPanel").set_text("Approach was stopped by user.")
			AutoAppOn = False	


# enable/disable axis
def xToggle(button):
	global X_enable, t, red_color, green_color	
	cmap = button.get_colormap()
	red = cmap.alloc_color(red_color)
	green = cmap.alloc_color(green_color)
	style = button.get_style().copy()

	if (button.get_active()):
		X_enable = True
		t.enable(0)
		style.bg[gtk.STATE_PRELIGHT] = green
	else:
		X_enable = False
		t.disable(0)
		style.bg[gtk.STATE_PRELIGHT] = red

	button.set_style(style)

def zToggle(button):
	global Z_enable, t, red_color, green_color	
	cmap = button.get_colormap()
	red = cmap.alloc_color(red_color)
	green = cmap.alloc_color(green_color)
	style = button.get_style().copy()

	if (button.get_active()):
		Z_enable = True
		t.enable(1)
		style.bg[gtk.STATE_PRELIGHT] = green
	else:
		Z_enable = False
		t.disable(1)
		style.bg[gtk.STATE_PRELIGHT] = red

	button.set_style(style)


# fill up possition entries
def fillPos():
	global t, builder, switch
	#builder.get_object("infoPanel").set_text(t.getStatus(0))
	if (switch == 0): # get position in X
		pos = t.getPos(0)
		if (not(numpy.isnan(pos))):
			entry = builder.get_object("XPos_entry")
			entry.set_text(str(pos))
			switch=switch+1
	elif (switch == 1): # get position in Y
		pos = t.getPos(1)
		if (not(numpy.isnan(pos))):
			entry = builder.get_object("ZPos_entry")
			entry.set_text(str(pos))
		switch = switch+1
	elif (switch == 2): # get enabled status of X
		status = t.getStatus(0,"ENABLED")
		toggle = builder.get_object("togglebutton2")
		
		if (status == 0):
			toggle.set_active(False)
		else:
			toggle.set_active(True)
		switch = switch+1
	elif (switch == 3): # get enabled status of Y
		status = t.getStatus(1,"ENABLED")
		toggle = builder.get_object("togglebutton1")
		if (status == 0):
			toggle.set_active(False)
		else:
			toggle.set_active(True)
		switch = switch+1
	elif (switch == 4): # get moving status of X
		status = t.getStatus(0,"MOVE")
		Xbtn = builder.get_object("XGO_button")
		cmap = Xbtn.get_colormap()
		red = cmap.alloc_color(red_color)
		green = cmap.alloc_color(green_color)
		style = Xbtn.get_style().copy()

		if (status == 0):
			style.bg[gtk.STATE_NORMAL] = green
			style.bg[gtk.STATE_ACTIVE] = green
			style.bg[gtk.STATE_PRELIGHT] = green
		else:
			style.bg[gtk.STATE_NORMAL] = red
			style.bg[gtk.STATE_ACTIVE] = red
			style.bg[gtk.STATE_PRELIGHT] = red	
		Xbtn.set_style(style)
		switch = switch+1
	elif (switch == 5): # get moving status of Y
		status = t.getStatus(1,"MOVE")
		Xbtn = builder.get_object("ZGO_button")
		cmap = Xbtn.get_colormap()
		red = cmap.alloc_color(red_color)
		green = cmap.alloc_color(green_color)
		style = Xbtn.get_style().copy()
		if (status == 0):
			style.bg[gtk.STATE_NORMAL] = green
			style.bg[gtk.STATE_ACTIVE] = green
			style.bg[gtk.STATE_PRELIGHT] = green
		else:
			style.bg[gtk.STATE_NORMAL] = red
			style.bg[gtk.STATE_ACTIVE] = red
			style.bg[gtk.STATE_PRELIGHT] = red	
		Xbtn.set_style(style)

		switch = 0
	global dsp
	return True
state = 0
numCycles = 0
# state machine for autoapproach
def stateMachine():
	global state, dsp, app_ci, max_delay, numCycles, t, builder, AutoAppOn
	
	# check for dissipation
	dissip = dsp.getDissipation()
	if (dissip > 8000):
		builder.get_object("infoPanel").set_text("!!! Dissipation alert !!!")
		builder.get_object("AutoApp_button").set_label("AutoApproach!")
		AutoAppOn = False
		state = 0
		
	# approach tip
	if (state == 1):
		builder.get_object("infoPanel").set_text("Approaching... elongating tip")
		dsp.setCi(app_ci)
		state = 2
		numCycles = 0
		return True

	# wait for full alongate 
	if (state == 2):
		zpos = dsp.getZ()
		if (numCycles > max_delay):
			builder.get_object("infoPanel").set_text("Approached!")
			builder.get_object("AutoApp_button").set_label("AutoApproach!")
			AutoAppOn = False
			state = 0
			return False
		if (zpos > ((2**15) - Zspan)):
			state = 3
		numCycles += 1
		return True

	# retract tip
	if (state == 3):
		builder.get_object("infoPanel").set_text("Approaching... retracting tip")
		dsp.setCi(retract_ci)
		state = 4
		return True

	# wait for full retract
	if (state == 4):
		zpos = dsp.getZ()
		if (zpos < (-(2**15) + Zspan)):
			state = 5
			builder.get_object("infoPanel").set_text("Approaching... macro approaching")
		return True

	# do macro approach
	if (state == 5):
		vel = float(builder.get_object("ZVel_entry").get_text())
		step = float(builder.get_object("ZStep_entry").get_text())
		time.sleep(abs(step/vel))
		t.makeStep(1,vel,step)
		state = 1 
		return True

# timer for filling up position input fields
maintimer = gobject.timeout_add(updaterate,fillPos)	

# system
def onDeleteWindow(*args):
	global t
        gtk.main_quit()

builder = gtk.Builder()
builder.add_from_file("gui_gtkBuilder.glade")
window = builder.get_object("main_window")

handlers = {
	"onDeleteWindow" : onDeleteWindow,
	# buttons
	"XGO_button_clicked" : onClickXGO,
	"ZGO_button_clicked" : onClickZGO,
	"X_enableToggle" : xToggle,
	"Z_enableToggle" : zToggle,
	"AutoApp_button_clicked" : onClickAutoApp,
}

builder.connect_signals(handlers)

# default values
builder.get_object("XVel_entry").set_text("100")
builder.get_object("XStep_entry").set_text("20")
builder.get_object("ZVel_entry").set_text("1")
builder.get_object("ZStep_entry").set_text("1")

# button colors - enabled toggle buttons
Xbtn = builder.get_object("togglebutton2")
cmap = Xbtn.get_colormap()
red = cmap.alloc_color(red_color)
green = cmap.alloc_color(green_color)
style = Xbtn.get_style().copy()
style.bg[gtk.STATE_NORMAL] = red
style.bg[gtk.STATE_ACTIVE] = green
style.bg[gtk.STATE_PRELIGHT] = red
Xbtn.set_style(style)


Xbtn = builder.get_object("togglebutton1")
cmap = Xbtn.get_colormap()
red = cmap.alloc_color(red_color)
green = cmap.alloc_color(green_color)
style = Xbtn.get_style().copy()
style.bg[gtk.STATE_NORMAL] = red
style.bg[gtk.STATE_ACTIVE] = green
style.bg[gtk.STATE_PRELIGHT] = red
Xbtn.set_style(style)

# GO buttons color
Xbtn = builder.get_object("XGO_button")
cmap = Xbtn.get_colormap()
red = cmap.alloc_color(red_color)
green = cmap.alloc_color(green_color)
style = Xbtn.get_style().copy()
style.bg[gtk.STATE_NORMAL] = green
style.bg[gtk.STATE_ACTIVE] = green
style.bg[gtk.STATE_PRELIGHT] = green
Xbtn.set_style(style)

Xbtn = builder.get_object("ZGO_button")
cmap = Xbtn.get_colormap()
red = cmap.alloc_color(red_color)
green = cmap.alloc_color(green_color)
style = Xbtn.get_style().copy()
style.bg[gtk.STATE_NORMAL] = green
style.bg[gtk.STATE_ACTIVE] = green
style.bg[gtk.STATE_PRELIGHT] = green
Xbtn.set_style(style)

window.show_all()

gtk.main()







