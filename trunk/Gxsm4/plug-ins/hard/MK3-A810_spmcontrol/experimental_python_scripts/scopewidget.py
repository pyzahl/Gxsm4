#!/usr/bin/env python

## * Python Module VU or general Meter/Instrument gtk widget
## *
## * Copyright (C) 2014 Percy Zahl
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

###########################################################################
## The original VU meter is a passive electromechanical device, namely
## a 200 uA DC d'Arsonval movement ammeter fed from a full wave
## copper-oxide rectifier mounted within the meter case. The mass of the
## needle causes a relatively slow response, which in effect integrates
## the signal, with a rise time of 300 ms. 0 VU is equal to +4 [dBu], or
## 1.228 volts RMS across a 600 ohm load. 0 VU is often referred to as "0
## dB".[1] The meter was designed not to measure the signal, but to let
## users aim the signal level to a target level of 0 VU (sometimes
## labelled 100%), so it is not important that the device is non-linear
## and imprecise for low levels. In effect, the scale ranges from -20 VU
## to +3 VU, with -3 VU right in the middle. Purely electronic devices
## may emulate the response of the needle; they are VU-meters inasmuch as
## they respect the standard.

import pygtk
pygtk.require('2.0')
import os		# use os because python IO is bugy

import gobject, gtk
import cairo
import time
import fcntl
from threading import Timer

#import GtkExtra
import struct
import array
import math
from gtk import TRUE, FALSE
from numpy  import *

wins = {}

sr_dev_base = "/dev/sranger"
global sr_dev_path
updaterate = 200

delaylinelength = 32

global SPM_STATEMACHINE
global analog_offset	 # at magic[10]+16
global AIC_gain          # at magic[8]+1+3*8
global AIC_in_buffer	 # at magic[6]

unit  = [ "dB", "dB" ]


class Scope(gtk.DrawingArea):

    def __init__(self, parent):
      
        self.par = parent
        super(Scope, self).__init__()
        self.set_lambda_frqmap ()
        self.set_dBX (False)
        self.set_dBY (False)
        self.set_dBZ (False)
        self.set_dBU (False)
        self.set_dBV (False)
        self.set_ftX (0)
        self.set_ftY (0)
        self.set_wide (False)
        self.set_xy (False)
        self.set_fade (0.0)
        self.set_points (False)
        self.set_markers (10,10)
        self.connect("expose-event", self.expose)
        self.set_events(gtk.gdk.BUTTON_PRESS_MASK)
        self.connect('button-press-event', self.on_drawing_area_button_press)
        self.display_info = 300
        self.set_flash ("Starting...")
        self.set_subsample_factor()
        self.Xdata_rft = zeros(2)
        self.Ydata_rft = zeros(2)

        
    def set_subsample_factor(self, sf=1):
        self.ss_fac=sf
        
    def set_flash (self, message):
        self.flash_message = message
        self.display_info = 300
	gobject.timeout_add (20, self.info_fade)
        
    def info_fade(self):
        if self.display_info > 0:
            self.display_info = self.display_info-1
	    gobject.timeout_add (20, self.info_fade)
            
    def set_markers (self, mx=0, my=0):
        self.Xmarkers = mx
        self.Ymarkers = my
        
    def on_drawing_area_button_press(self, widget, event):
        # print event.x, ' ', event.y
        # Mode toggle buttons
        if event.x > 50 and event.x < 100:
            if event.y > 550 and event.y < 570: 
                self.set_wide (not self.wide)
            if event.y > 570 and event.y < 590: 
                self.set_xy (not self.xy)
        elif event.x > 100 and event.x < 150:
            if event.y > 550 and event.y < 570: 
                self.set_fade (self.fade+0.1)
            if event.y > 570 and event.y < 590: 
                self.set_points (not self.points)
        elif event.x > 150 and event.x < 185:
            if event.y > 550 and event.y < 570: 
                self.set_ftX (self.ftX+1)
            if event.y > 570 and event.y < 590: 
                self.set_ftY (self.ftY+1)
        elif event.x > 185 and event.x < 220:
            if event.y > 550 and event.y < 570: 
                self.set_dBX (not self.dBX)
            if event.y > 570 and event.y < 590: 
                self.set_dBY (not self.dBY)
            if event.y > 530 and event.y < 550: 
                self.set_dBZ (not self.dBZ)
            if event.y > 510 and event.y < 530: 
                self.set_dBU (not self.dBU)
            if event.y > 490 and event.y < 510: 
                self.set_dBV (not self.dBV)
        elif event.x > 220 and event.x < 250:
            if event.y > 550 and event.y < 570: 
                self.set_lambda_frqmap (True)
            if event.y > 570 and event.y < 590: 
                self.set_lambda_frqmap (False)

        if self.display_info > 0:
            self.display_info = 300
        else:
            self.display_info = 300
	    gobject.timeout_add (20, self.info_fade)
            
    def set_lambda_frqmap (self, fmaplog=False):
        if fmaplog:
            lwf = 2.*self.xw / log (1+2.*self.xw)
            self.lambda_frqmap =  lambda x: lwf*log(1+self.xw+x)-self.xw
        else:
            self.lambda_frqmap =  lambda x: x
            
    def set_dBX(self, db):
        self.dBX = db
    def set_dBY(self, db):
        self.dBY = db
    def set_dBZ(self, db):
        self.dBZ = db
    def set_dBU(self, db):
        self.dBU = db
    def set_dBV(self, db):
        self.dBV = db

    def set_ftX(self, ft):
        self.ftX = ft%3
    def set_ftY(self, ft):
        self.ftY = ft%3

    def set_xy(self, xy):
        self.xy = xy

    def set_fade(self, f):
        if f > 1.:
            self.fade = 0.0
        else:
            self.fade = f

    def set_points(self, p):
        self.points = p

    def set_wide(self, w):
        self.wide = w
        if (self.wide):
            self.xcenter = 525
            self.xw=20.0
            self.set_size_request(1050, 600)
	    if os.path.isfile("scope-frame-wide.png"):
		imagefile="scope-frame-wide.png"
	    elif os.path.isfile("/usr/share/gxsm3/pixmaps/scope-frame-wide.png"):
		imagefile="/usr/share/gxsm3/pixmaps/scope-frame-wide.png"
            else:
		imagefile="/usr/share/gxsm/pixmaps/scope-frame-wide.png"
        else:
            self.xcenter = 275
            self.xw=10.0
            self.set_size_request(550, 600)
	    if os.path.isfile("scope-frame.png"):
		imagefile="scope-frame.png"
	    elif os.path.isfile("/usr/share/gxsm3/pixmaps/scope-frame.png"):
		imagefile="/usr/share/gxsm3/pixmaps/scope-frame.png"
            else:
		imagefile="/usr/share/gxsm/pixmaps/scope-frame.png"
        self.vuscopesurface = cairo.ImageSurface.create_from_png(imagefile)
        cr = cairo.Context (self.vuscopesurface)
        cr.set_source_surface(self.vuscopesurface, 0,0)  
        cr.paint()

        cr.save ()

        # center of screen is 0,0
        # full scale is +/-10
        cr.translate (self.xcenter, 275)
        cr.scale (25., 25.)

        # ticks -10 .. 0 .. +10
        cr.set_source_rgba(0.27, 0.48, 0.69, 0.25) # TICKS
        cr.set_line_width(0.03)

        ticks = arange(-self.xw,self.xw+1,1)
        for tx in ticks:
            cr.move_to (tx, -10.)
            cr.line_to (tx, 10.)
            cr.stroke()
        ticks = arange(-10,11,1)
        for ty in ticks:
            cr.move_to (-self.xw, ty)
            cr.line_to (self.xw, ty)
            cr.stroke()

        cr.set_source_rgba(0.37, 0.58, 0.79, 0.35) # ZERO AXIS
        cr.set_line_width(0.05)
        cr.move_to (0., -10.)
        cr.line_to (0., 10.)
        cr.stroke()
        cr.move_to (-self.xw, 0.)
        cr.line_to (self.xw, 0.)
        cr.stroke()
        
        cr.restore ()

    def get_wide(self):
        return self.wide

    def plot_xt(self, cr, ydata, lwp, r,g,b,a, xmap = lambda x: x):
            cr.set_source_rgba (r,g,b,a)
            dx = (2*self.xw) / size(ydata)
            x = -self.xw
            if self.points:
                for y in ydata:
                    cr.move_to(xmap(x), y)
                    cr.line_to(xmap(x)+lwp, y)
                    cr.stroke()
                    x = x+dx
            else:
                yp = ydata[0]
                for y in ydata:
                    cr.move_to(xmap(x), yp)
                    x = x+dx
                    yp = y
                    cr.line_to(xmap(x), y)
                    cr.stroke()

    def plot_xy(self, cr, xydata, lwp, r,g,b,a):
            alpha=a
            if self.points:
                for xy in xydata:
                    cr.set_source_rgba (r,g,b,alpha)
                    if alpha > 0.01:
                        alpha = alpha - self.fade/200
                    cr.move_to (xy[0], xy[1])
                    cr.line_to (xy[0]+lwp, xy[1])
                    cr.stroke ()
            else:
                xyp = xydata[0]
                for xy in xydata:
                    cr.set_source_rgba (r,g,b,alpha)
                    if alpha > 0.01:
                        alpha = alpha - self.fade/200
                    cr.move_to (xyp[0], xyp[1])
                    cr.line_to (xy[0], xy[1])
                    cr.stroke ()
                    xyp=xy

    def plot_markers(self, cr, data, markers, r, g, b, a, ylog=False, xmap = lambda x: x):
        if markers > 0:
            cr.select_font_face("Droid Sans") #, cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
            cr.set_font_size(0.5)
            np=50
            #            if data.size > 512:
            #                q=int(data.size/256)
            #                peaks = q*sort(argpartition (-decimate(data, q,'fir'), np))
            #            else:
            peaks_tmp = argpartition (-data, np)
            peaks = peaks_tmp[:np]
            dx = (2*self.xw) / size(data)
            x_peaks = -self.xw+dx*(1+peaks)
            f_peaks = 75000.*peaks/size(data)/self.ss_fac
            pcount=0
            for x, f, peak in zip(x_peaks, f_peaks, peaks):
                if peak < 10 or peak > size(data)-10:
                    continue
                if data[peak] < amax(data[(peak-10):(peak+10)]):
                    continue
                pcount=pcount+1
                if pcount > markers:
                    break
                cr.set_source_rgba (r,g,b,a)
                if ylog:
                    y = -2*log(data[peak])
                else:
                    y = -data[peak]

                xx = xmap(x)
                cr.move_to (xx, y)
                s=0.1
                cr.line_to (xx-s, y-s)
                cr.line_to (xx+s, y-s)
                cr.line_to (xx, y)
                cr.stroke ()
                cr.set_source_rgba(r,g,b,a)
                reading = "%g Hz"%f
                (rx, ry, width, height, dx, dy) = cr.text_extents(reading)
                cr.move_to(xx-width/2, y-1.2*s-2.5*height)
                cr.text_path(reading)
                cr.stroke()
                reading = "%g"%data[peak]
                (rx, ry, width, height, dx, dy) = cr.text_extents(reading)
                cr.move_to(xx-width/2, y-1.2*s-height)
                cr.text_path(reading)
                cr.stroke()
                    
    def draw_buttons(self, cr):
        if self.display_info > 0:

            cr.set_source_rgba(0.9, 0.9, 0.9, self.display_info/100.) # LIGHT GREY

            yb=550
        
            if self.xy:
                reading = "X-Y"
            else:
                reading = "X-t"
            cr.move_to(60, yb-20)
            cr.text_path(reading)
            cr.stroke()

            if self.fade>0:
                reading = "F:%g"%self.fade
            else:
                reading = "---"
            cr.move_to(105, yb-40)
            cr.text_path(reading)
            cr.stroke()

            if self.points:
                reading = "Points"
            else:
                reading = "Vector"

            cr.move_to(105, yb-20)
            cr.text_path(reading)
            cr.stroke()

            if self.ftX:
                reading = "FT(X)"
            else:
                reading = "X"
            cr.move_to(152, yb-40)
            cr.text_path(reading)
            cr.stroke()

            if self.ftY:
                reading = "FT(Y)"
            else:
                reading = "Y"
            cr.move_to(152, yb-20)
            cr.text_path(reading)
            cr.stroke()

            if self.dBX:
                reading = "dB X"
            else:
                reading = "Lin X"
            cr.move_to(188, yb-40)
            cr.text_path(reading)
            cr.stroke()

            if self.dBY:
                reading = "dB Y"
            else:
                reading = "Lin Y"
            cr.move_to(188, yb-20)
            cr.text_path(reading)
            cr.stroke()

            if self.dBZ:
                reading = "dB Z"
                cr.move_to(188, yb-60)
                cr.text_path(reading)
                cr.stroke()

            if self.dBU:
                reading = "dB U"
                cr.move_to(188, yb-80)
                cr.text_path(reading)
                cr.stroke()

            if self.dBV:
                reading = "dB V"
                cr.move_to(188, yb-100)
                cr.text_path(reading)
                cr.stroke()

            if len (self.flash_message) > 0:
                cr.move_to(25, 25)
                cr.text_path(self.flash_message)
                cr.stroke()
        else:
            self.flash_message = ""
                
    def expose(self, widget, event):
        cr = widget.window.cairo_create()
        cr.set_source_surface (self.vuscopesurface)
        cr.paint()
        cr.stroke()

        cr.save ()

        self.par.count = self.par.count + 1
         
        # center of screen is 0,0
        # full scale is +/-10
        cr.translate (self.xcenter, 275)
        cr.scale (25., 25.)
        lwp = 0.1
        if self.points:
            cr.set_line_width(lwp)
        else:
            cr.set_line_width(0.04)
        
        alpha = 0.85

        nx=size(self.par.Xdata)/2
        ny=size(self.par.Xdata)/2
        if self.xy and nx > 2 and nx == ny:
            if self.dBX and self.dBY: # 10 dB / div
                xydata = column_stack(( -2.*log(abs(self.par.Xdata)), -2.*log(abs(self.par.Ydata))))
            elif self.dBX:
                xydata = column_stack(( -2.*log(abs(self.par.Xdata)), self.par.Ydata))
            elif self.dBY:
                xydata = column_stack((self.par.Xdata, -2.*log(abs(self.par.Ydata))))
            else:
                xydata = column_stack((self.par.Xdata, self.par.Ydata))
            self.plot_xy (cr, xydata, lwp, 1., 0.925, 0., alpha) # YELLOW
        else:
            if self.ftX < 2:
                if self.dBX: # 10 dB / div
                    self.plot_xt (cr, -2.*log(abs(self.par.Xdata)), lwp, 1., 0.925, 0., alpha) # YELLOW
                else:
                    self.plot_xt (cr, self.par.Xdata, lwp, 1., 0.925, 0., alpha) # YELLOW
            if self.ftX:
                Xdata_rft_new = abs(fft.rfft(self.par.Xdata))/nx
                if self.Xdata_rft.size != Xdata_rft_new.size:
                    self.Xdata_rft = Xdata_rft_new
                else:
                    self.Xdata_rft = self.Xdata_rft*0.02 + Xdata_rft_new*0.98
                if self.dBX:
                    self.plot_xt (cr, -2.*log(self.Xdata_rft), lwp, 1., 0.925, 0., alpha-0.2, self.lambda_frqmap) # YELLOW
                    self.plot_markers (cr, self.Xdata_rft, self.Xmarkers, 1., 0.925, 0., alpha-0.2, True,  self.lambda_frqmap)
                else:
                    self.plot_xt (cr, -self.Xdata_rft, lwp, 1., 0.925, 0., alpha-0.2) # YELLOW
                    self.plot_markers (cr, self.Xdata_rft, self.Xmarkers, 1., 0.925, 0., alpha-0.2)

            if size(self.par.Ydata) > 1:
                if self.ftY < 2:
                    if self.dBY: # 10 dB / div
                        self.plot_xt (cr, -2.*log(abs(self.par.Ydata)), lwp, 1., 0.075, 0., alpha-0.2) # RED
                    else:
                        self.plot_xt (cr, self.par.Ydata, lwp, 1., 0.075, 0., alpha) # RED
                if self.ftY:
                    Ydata_rft_new = abs(fft.rfft(self.par.Ydata))/ny
                    if self.Ydata_rft.size != Ydata_rft_new.size:
                        self.Ydata_rft = Ydata_rft_new
                    else:
                        self.Ydata_rft = self.Ydata_rft*0.02 + Ydata_rft_new*0.98
                    if self.dBY:
                        self.plot_xt (cr, -2.*log(self.Ydata_rft), lwp, 1., 0.075, 0., alpha-0.2, self.lambda_frqmap) # RED
                        self.plot_markers (cr, self.Ydata_rft, self.Ymarkers, 1., 0.075, 0., alpha-0.2, True, self.lambda_frqmap)
                    else:
                        self.plot_xt (cr, -self.Ydata_rft, lwp, 1., 0.075, 0., alpha-0.2) # RED
                        self.plot_markers (cr, self.Ydata_rft, self.Ymarkers, 1., 0.075, 0., alpha-0.2)
                    
        if size(self.par.Zdata) > 1:
            if self.dBZ: # 10 dB / div
                self.plot_xt (cr, -2.*log(abs(self.par.Zdata)), lwp, 0., 1., 0., alpha) # GREEN
            else:
                self.plot_xt (cr, self.par.Zdata, lwp, 0., 1., 0., alpha) # GREEN

        if size(self.par.Udata) > 1:
            if self.dBU: # 10 dB / div
                self.plot_xt (cr, -2.*log(abs(self.par.Udata)), lwp, 0., 1., 1., alpha) # Cyan
            else:
                self.plot_xt (cr, self.par.Udata, lwp, 0., 1., 1., alpha) # Cyan

        if size(self.par.Vdata) > 1:
            if self.dBV: # 10 dB / div
                self.plot_xt (cr, -2.*log(abs(self.par.Vdata)), lwp, 0.4, 0.4, 1., alpha) # light Blue
            else:
                self.plot_xt (cr, self.par.Vdata, lwp, 0.4, 0.4, 1., alpha) # light Blue

        if size (self.par.XYdata[0]) > 1:
            self.plot_xy (cr, self.par.XYdata, lwp, 0., 1., 0., alpha) # GREEN

        cr.restore ()

        if (self.wide):
            x0=25
            x1=260
            x2=350
            x3=750
            x4=850
            x5=1025
        else:
            x0=25
            x1=260
            x2=320
            x3=350
            x4=450
            x5=525
            
        ylineOS0 = 45
        yline1 = 560
        yline2 = 575
        yline3 = 590

        cr.select_font_face("Droid Sans") #, cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
        cr.set_font_size(12)
        cr.set_line_width(0.8)

        cr.set_source_rgba(0., 1., 0., 1.) # GREEN

        if self.par.count == 1:
            reading = "Enable/Disable PAC/PLL processing to run/hold scope!"
            cr.move_to(x1, ylineOS0)
            cr.text_path(reading)
            cr.stroke()

        y=0
        for txt in self.par.info:
            if txt == "":
                y=y+2
                continue
            cr.move_to(x0, ylineOS0+y*25)
            cr.text_path(txt)
            cr.stroke()
            y=y+1

        y=0
        for txt in self.par.chinfo:
            if y==0:
                cr.set_source_rgba(1., 0.925, 0., alpha) # YELLOW
            else:
                if y == 1:
                    cr.set_source_rgba(1., 0.075, 0., alpha) # RED
                else:
                    if y == 2:
                        cr.set_source_rgba(0., 1., 0., alpha) # GREEN
                    else:
                        if y == 3:
                            cr.set_source_rgba(0., 1., 1., alpha) # CYAN
                        else:
                            cr.set_source_rgba(0.4, 0.4, 1., alpha) # light BLUE
            reading = txt + ": CH%d"%(y+1)
            (rx, ry, width, height, dx, dy) = cr.text_extents(reading)
            cr.move_to(x5-width, ylineOS0+y*25)
            cr.text_path(reading)
            cr.stroke()
            y=y+1
            
        self.draw_buttons (cr)
        
        cr.set_source_rgba(0.03, 0., 0.97, 1.) # BLUE
            
        # record count
        reading = "#: %d"%self.par.count
        cr.move_to(x1, yline1)
        cr.text_path(reading)
        cr.stroke()

        # CH1: (X-sig) scale/div
        y=yline1
        ch=1
        for s in self.par.scale.keys():
            # (x, y, width, height, dx, dy) = cr.text_extents(reading)
            cr.move_to (x3, y)
            cr.text_path (s)
            cr.stroke ()
            reading = self.par.scale[s]
            if (self.dBX and ch==1) or (self.dBY and ch==2):
                reading = reading + " = 0dB @ 10 dB/div"
            else:
                reading = reading + "/div"
            (rx, ry, width, height, dx, dy) = cr.text_extents(reading)
            cr.move_to(x5-width, y)
            cr.text_path(reading)
            cr.stroke()
            y=y+15
            ch = ch+1

        
class Oscilloscope(gtk.Label):
    def __init__(self, parent, vb, ft="XT", l="Scope"):
        
        #Initialize the Widget
        gtk.Widget.__init__(self)
        self.set_use_markup(True)
        self.frametype=ft
        self.label = l
        self.info = []
        self.chinfo = []
        self.scale = { 
            "CH1-scale": "1 V/div",
            "CH2-scale": "1 V/div",
            "Timebase":  "20 ms/div" 
            }

        self.Xdata = zeros(2)
        self.Ydata = zeros(0)
        self.Zdata = zeros(0)
        self.Udata = zeros(0)
        self.Vdata = zeros(0)
        self.XYdata = array([[],[]])
        self.scope = Scope(self)
        self.count = 0
        vb.pack_start(self, False, False, 0)
        vb.pack_start(self.scope, False, False, 0)
        vb.show_all()
        self.show_all()

    def get_wide (self):
        return self.scope.get_wide()
        
    def set_data (self, Xd, Yd, Zd=zeros(0), XYd=[zeros(0), zeros(0)]):
        self.Xdata = Xd
        self.Ydata = Yd
        self.Zdata = Zd
        self.XYdata = XYd
        self.scope.queue_draw()

    def avg_data (self, Xd, Yd, Zd=zeros(0), XYd=[zeros(0), zeros(0)]):
        if self.Xdata.size != Xd:
            self.Xdata = Xd
            self.Ydata = Yd
            self.Zdata = Zd
            self.XYdata = XYd
        else:
            self.Xdata = self.Xdata*0.9 + Xd*0.1
            self.Ydata = self.Ydata*0.9 + Yd*0.1
            self.Zdata = Zd
            self.XYdata = XYd
            
        self.scope.queue_draw()

    def set_data_with_uv (self, Xd, Yd, Zd=zeros(0), Ud=zeros(0), Vd=zeros(0), XYd=[zeros(0), zeros(0)]):
        self.Xdata = Xd
        self.Ydata = Yd
        self.Zdata = Zd
        self.Udata = Ud
        self.Vdata = Vd
        self.XYdata = XYd
        self.scope.queue_draw()

    def set_info (self, infolist):
        self.info = infolist
        
    def set_flash (self, message):
        self.scope.set_flash (message)
        
    def set_chinfo (self, infolist):
        self.chinfo = infolist

    def set_markers (self, nmarkersx, nmarkersy):
        self.scope.set_markers (nmarkesx, nmarkersy)
        
    def set_scale (self, s):
        self.scale = s
        
    def set_label (self, str):
        self.label = str

    def set_subsample_factor(self, sf):
        self.scope.set_subsample_factor(sf)

    def queue_draw (self):
        self.scope.queue_draw()
        
        
## END
