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

import gi
gi.require_version("Gtk", "4.0")
from gi.repository import Gtk

import os		# use os because python IO is bugy

import cairo
import time
import fcntl
from threading import Timer

import struct
import array
import math
from numpy  import *

class Meter(Gtk.DrawingArea):
    def __init__(self, parent, widget_scale=1.0):
        self.cairo_scale=widget_scale
        self.par = parent
        super(Meter, self).__init__()
        self.vumeter = 0

        # display mode
        # Logarithmic Meter, Absolut, dB -- VU style
        if self.par.frametype == "VU":
            self.vumeter = 1
            self.set_size_request(220*self.cairo_scale, 220*self.cairo_scale)
            if os.path.isfile("vumeter-frame.png"):
                imagefile="vumeter-frame.png"
            elif os.path.isfile("/usr/share/gxsm3/pixmaps/vumeter-frame.png"):
                imagefile="/usr/share/gxsm3/pixmaps/vumeter-frame.png"
            else:
                imagefile="/usr/share/gxsm/pixmaps/vumeter-frame.png"
            self.vumetersurface = cairo.ImageSurface.create_from_png(imagefile)
            cr = cairo.Context (self.vumetersurface)
            cr.set_source_surface(self.vumetersurface, 0,0)  
            cr.paint()
            cr.set_line_width(0.8)

            cr.select_font_face("Ununtu", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
            cr.set_font_size(12)
            cr.set_source_rgb(0.95, 0.95, 0.95)
            reading = self.par.label
            (x, y, width, height, dx, dy) = cr.text_extents(reading)
            cr.move_to(108-width/2, 190)
            cr.text_path(reading)
            cr.stroke()

        # Linear Meter "Volt", etc.
        # load plain meter frame, annotate with all static elements
        if self.par.frametype == "Volt":
            self.vumeter = 2
            self.set_size_request(220*self.cairo_scale, 220*self.cairo_scale)
            if os.path.isfile("meter-frame.png"):
                imagefile="meter-frame.png"
            elif os.path.isfile("/usr/share/gxsm3/pixmaps/meter-frame.png"):
                imagefile="/usr/share/gxsm3/pixmaps/meter-frame.png"
            else:
                imagefile="/usr/share/gxsm/pixmaps/meter-frame.png"
            self.vumetersurface = cairo.ImageSurface.create_from_png(imagefile)
            cr = cairo.Context (self.vumetersurface)
            cr.set_source_surface(self.vumetersurface, 0,0)  
            cr.paint()
            cr.set_line_width(0.8)

            # scalings,.. to compute needle displacement
            vrange = self.par.mrange[len(self.par.mrange)-1]
            angle_scale = 0.8/vrange
            angle_offset = 0. 
            lowervalue = 0.
            needle_len = 103.0

            # create scale: left (-) / right (+) half, Zero Center.
            cr.set_source_rgb(0.85, 0.01, 0.4)
            phic = 3.*math.pi/2.
            bow  = 0.7853975
            cr.arc (108, 169, needle_len,  phic, phic+bow);
            cr.stroke();
            cr.set_source_rgb(0.35, 0.31, 0.24)
            cr.arc (108, 169, needle_len, phic-bow, phic);
            cr.stroke();

            cr.set_line_width(4.0)
            cr.set_source_rgb(0.85, 0.01, 0.4)
            phic = 3.*math.pi/2.
            cr.arc (108, 169, needle_len+2,  phic+bow*0.8, phic+bow);
            cr.stroke();
            cr.arc (108, 169, needle_len+2, phic-bow, phic-bow*0.8);
            cr.stroke();

            # ticks and labels
#            cr.select_font_face("Courier", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
            cr.select_font_face("Monospace", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
            cr.set_source_rgb(0.35, 0.31, 0.24)
            cr.set_line_width(0.7)
            cr.set_font_size(8)

            # ticks -10 .. 0 .. +10
            ticks = arange(-10,11,1)
            i=0
            for v in ticks:
                theta = ((v-lowervalue) * angle_scale) - angle_offset
                
                if theta < -0.7853975:
                    theta = -0.7853975
                if theta > 0.7853975:
                    theta = 0.7853975

                cr.save ()
                cr.translate (108, 169)
                cr.rotate (math.pi+theta)
                cr.move_to (0, needle_len)
                if i%2 == 0:
                    cr.line_to (0, needle_len+5.0)
                else:
                    cr.line_to (0, needle_len+3.5)
                cr.stroke()
                
                if i%2 == 0:
                    lab = "%d"%v
                    if (i == 0):
                        lab = "%d"%v
                    else:
                        lab = "%d"%(math.fabs(v))
                    (x, y, width, height, dx, dy) = cr.text_extents(lab)
                    cr.translate (width/2, needle_len+10)
                    cr.rotate (math.pi)
                    cr.move_to (0,0)
                    cr.text_path (lab)
                    cr.stroke ()
                    
                cr.restore ()
                i = i+1

            # ticks -10 .. 0 .. +10 at 2^[8...15]
            for p in [8,9,10,11,12,13,14,15]:
                v = 10.*(1<<p)/32768
                theta = ((v-lowervalue) * angle_scale) - angle_offset
                
                if theta < -0.7853975:
                    theta = -0.7853975
                if theta > 0.7853975:
                    theta = 0.7853975

                cr.save ()
                cr.translate (108, 169)
                cr.rotate (math.pi+theta)
                cr.move_to (0, needle_len-2)
                cr.line_to (0, needle_len-3.5)
                cr.stroke()
                cr.restore ()
                cr.save ()
                cr.translate (108, 169)
                cr.rotate (math.pi-theta)
                cr.move_to (0, needle_len-2)
                cr.line_to (0, needle_len-3.5)
                cr.stroke()
                cr.restore ()

            # add frame "signal name" label
            cr.select_font_face("Ununtu", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
            cr.set_font_size(12)
            cr.set_source_rgb(0.95, 0.95, 0.95)
            reading = self.par.label
            (x, y, width, height, dx, dy) = cr.text_extents(reading)
            cr.move_to(108-width/2, 190)
            cr.text_path(reading)
            cr.stroke()

        if self.vumeter == 0:
            self.set_size_request(50, -1)

        #self.connect("draw", self.draw)
        self.set_draw_func(self.draw, None)
        #self.set_child(self.drawing_area)
        

    def draw(self, widget, cr, width, height, data):
        cr.scale(self.cairo_scale, self.cairo_scale)
        if self.vumeter > 0:
            cr.set_source_surface (self.vumetersurface)
            cr.paint()
            cr.stroke()
            cr.set_line_width(0.8)

            cr.select_font_face("Courier", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
            #cr.select_font_face("Ubuntu Mono", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
            cr.set_font_size(10)
            
            vrange = self.par.mrange[len(self.par.mrange)-1]

            if self.vumeter==1 :
                maxdb = 28
                mindb = -20*log10(1<<16)
                lowervalue = mindb
                angle_scale = math.pi/2./(maxdb-mindb)   #15.7
                angle_offset = 0.7853975 
            else:
                if self.vumeter==2 :
                    angle_scale = 0.8/vrange
                    angle_offset = 0. 
                    lowervalue = 0.

            needle_len = 98.0
            needle_len2= 78.0
            needle_hub = 21.0

#            lp = lp * 0.8 + env * bias * 0.2  # low pass!

            theta = ((self.par.get_cur_value()-lowervalue) * angle_scale) - angle_offset

            flag = 3
            if theta < -0.7853975:
                theta = -0.7853975
                flag = -1 
            if theta > 0.7853975:
                theta = 0.7853975
                flag = 1 
            if math.fabs(theta) < 0.00003:
                flag = 0

            cr.save ()
            cr.translate (108, 169)
            cr.rotate (math.pi+theta)

            cr.set_source_rgb(0.35, 0.31, 0.24)
            cr.move_to(0, needle_hub)
            cr.line_to(0, needle_len)
            cr.stroke()
            
            cr.restore ()

            theta = ((self.par.get_hi_value()-lowervalue) * angle_scale) - angle_offset

            if theta < -0.7853975:
                theta = -0.7853975
            if theta > 0.7853975:
                theta = 0.7853975

            cr.save ()
            cr.translate (108, 169)
            cr.rotate (math.pi+theta)

            cr.set_source_rgb(0.85, 0.01, 0.4)
            cr.move_to(0, needle_hub)
            cr.line_to(0, needle_len2)
            cr.stroke()

            cr.restore ()

            theta = ((self.par.get_lo_value()-lowervalue) * angle_scale) - angle_offset

            if theta < -0.7853975:
                theta = -0.7853975
            if theta > 0.7853975:
                theta = 0.7853975

            cr.save ()
            cr.translate (108, 169)
            cr.rotate (math.pi+theta)

            cr.set_source_rgb(0.4, 0.01, 0.85)
            cr.move_to(0, needle_hub)
            cr.line_to(0, needle_len2)
            cr.stroke()

            cr.restore ()

            cr.set_source_rgb(0.35, 0.31, 0.24)

            # Unit, factor
            cr.set_font_size(11)
            cr.set_source_rgb(0.35, 0.31, 0.24)
            reading = self.par.unitx
            (x, y, width, height, dx, dy) = cr.text_extents(reading)
            cr.move_to(108-width/2, 110)
            cr.text_path(reading)
            cr.stroke()

            Ufactor = vrange/10.
            cr.set_font_size(10)
            reading = "x %g"%Ufactor
            (x, y, width, height, dx, dy) = cr.text_extents(reading)
            cr.move_to(108-width/2, 120)
            cr.text_path(reading)
            cr.stroke()

            # actual
            reading = self.par.format%(self.par.cur_value)
            (x, y, width, height, dx, dy) = cr.text_extents(reading)
            cr.move_to(60-width/2, 160)
            cr.text_path(reading)
            cr.stroke()

            # min/max
            cr.set_source_rgb(0.85, 0.01, 0.4)
            reading = self.par.format%(self.par.cur_value_hi)
            (x, y, width, height, dx, dy) = cr.text_extents(reading)
            cr.move_to(220-60-width/2, 160)
            cr.text_path(reading)
            cr.stroke()

            cr.set_source_rgb(0.4, 0.01, 0.85)
            reading = self.par.format%(self.par.cur_value_lo)
            (x, y, width, height, dx, dy) = cr.text_extents(reading)
            cr.move_to(220-60-width/2, 160-height*1.6)
            cr.text_path(reading)
            cr.stroke()

            cr.set_font_size(14)
            cr.set_source_rgb(1.0, 0.5, 0.5)
            reading = self.par.polarity
            (x, y, width, height, dx, dy) = cr.text_extents(reading)
            cr.move_to(108-width/2, 169)
            cr.text_path(reading)
            cr.stroke()

            if flag < 3:
                if flag == 0:
                    reading = "~0~"
                else:
                    reading = "OVR!"
                (x, y, width, height, dx, dy) = cr.text_extents(reading)
                cr.move_to(108+flag*70-width/2, 140)
                cr.text_path(reading)
                cr.stroke()

        else:
            cr.set_line_width(0.8)

            cr.select_font_face("Courier", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
            cr.set_font_size(8)

            (x, y, wminus, height, dx, dy) = cr.text_extents("--")
            pad   = 2*wminus
            width = self.allocation.width - 2*pad
            vrange = (self.par.mrange[len(self.par.mrange)-1] - self.par.mrange[0])
            fac = float(width)/vrange
            self.cur_pos = fac * self.par.get_cur_value()
            self.hi = fac * self.par.get_hi_value()
            self.lo = fac * self.par.get_lo_value()
            d = 1.+width/100.
            till = self.cur_pos
            full = width*1.0
            center = pad+width/2
            h=5

            if (self.hi != 0 and self.lo != 0 ):
                cr.set_source_rgb(0.0, 1.0, 0.00)
                cr.rectangle(center+self.lo, 0, self.hi - self.lo, h)
                cr.save()
                cr.clip()
                cr.paint()
                cr.restore()

                cr.set_source_rgb(1.0, 0.2, 0.00)
                cr.rectangle(center+self.hi-d, 0, d, h)
                cr.save()
                cr.clip()
                cr.paint()
                cr.restore()
            
                cr.set_source_rgb(1.0, 0.2, 0.00)
                cr.rectangle(center+self.lo+d, 0, -d, h)
                cr.save()
                cr.clip()
                cr.paint()
                cr.restore()

            if (self.cur_pos >= full):
            
                cr.set_source_rgb(0.0, 1.0, 0.0)
                cr.rectangle(center, 0, full, h)
                cr.save()
                cr.clip()
                cr.paint()
                cr.restore()
                
                cr.set_source_rgb(1.0, 0.0, 0.0) # red
                cr.rectangle(center+full, 0, till-full, h)
                cr.save()
                cr.clip()
                cr.paint()
                cr.restore()

            else:
                if (self.cur_pos <= -full):
            
                    cr.set_source_rgb(1.0, 1.0, 0.72)
                    cr.rectangle(center, 0, -full, h)
                    cr.save()
                    cr.clip()
                    cr.paint()
                    cr.restore()
                    
                    cr.set_source_rgb(1.0, 0.68, 0.68)
                    cr.rectangle(center-full, 0, till+full, h)
                    cr.save()
                    cr.clip()
                    cr.paint()
                    cr.restore()
                    
                else:     
                    cr.set_source_rgb(0.0, 0.0, 1.0) # blue
                    cr.rectangle(center, 0, till, h)
                    cr.save()
                    cr.clip()
                    cr.paint()
                    cr.restore()
                    
            

                cr.set_source_rgb(0.35, 0.31, 0.24)
                for v in self.par.mrange:
                    xp = fac * (v - self.par.mrange[0])
                    cr.move_to(pad+xp, 0)
                    cr.line_to(pad+xp, 5)
                    cr.stroke()
                    lab = "%d"%v
                    (x, y, width, height, dx, dy) = cr.text_extents(lab)
                    if (v < 0):
                        width = width+wminus
                        cr.move_to(pad+xp-width/2, 15)
                        cr.text_path(lab)
                        cr.stroke()
        return False
                        
class Instrument(Gtk.Label):
    def __init__(self, parent, vb, ft="VU", l="Meter", u="dB", fmt="%+5.3f ", widget_scale=1.0):
        #Initialize the Widget
        Gtk.Widget.__init__(self)
        self.set_use_markup(True)
        self.cur_value = 0
        self.cur_value_lo = 0
        self.cur_value_hi = 0
        self.polarity="+"
        self.frametype=ft
        self.label = l
        self.unit = u
        self.unitx = u
        if self.unitx == "V":
            self.unitx = "Volts"
        if self.unitx == "A":
            self.unitx = "Amps"
        if self.unitx == "Hz":
            self.unitx = "Hertz"
        self.format = fmt + self.unit
        self.mrange = arange(0, 11, 1)
        self.meter = Meter(self, widget_scale)
        vb.append(self)
        vb.append(self.meter)

    def set_range (self, r):
        self.mrange = r

    def set_label (self, str):
        self.label = str

    def set_format (self, str):
        self.format = str + " " + self.unit

    def set_unit (self, str):
        self.unit = str
        self.unitx = str
        if self.unitx == "V":
            self.unitx = "Volts"
        if self.unitx == "A":
            self.unitx = "Amps"
        if self.unitx == "Hz":
            self.unitx = "Hertz"

    def set_reading (self, value):
        spantag  = "<span size=\"24000\" font_family=\"monospace\"><b>"   # .. color=\"#ff0000\"
        self.set_markup (spantag + self.format %value + "</b></span>")
        self.cur_value = value       
        GLib.idle_add (self.meter.queue_draw())
        
    def set_reading_lohi_markup (self, value, lo, hi):
        spantag  = "<span size=\"24000\" font_family=\"monospace\"><b>"   # .. color=\"#ff0000\"
        spantag2 = "<span size=\"10000\" font_family=\"monospace\">"   # .. color=\"#ff0000\"
        self.set_markup (spantag + self.format %value + "</b></span>"
                         + "\n" + spantag2
                         + self.format %lo
                         + "  "
                         + self.format %hi
                         + "</span>"
        )
        self.cur_value = value       
        self.cur_value_lo = lo
        self.cur_value_hi = hi
        self.meter.queue_draw()

    def set_reading_lohi (self, value, lo, hi):
        self.cur_value = value       
        self.cur_value_lo = lo
        self.cur_value_hi = hi
        self.meter.queue_draw()

    def set_reading_vu (self, value, hi, pol="+", lp=0.5):
        self.cur_value = (1.-lp)*self.cur_value + lp*value
        self.cur_value_lo = 0.97*self.cur_value_lo + 0.03*value
        if value < self.cur_value_lo:
            self.cur_value_lo = value
        
        self.cur_value_hi = 0.97*self.cur_value_hi + 0.03*hi
        if hi > self.cur_value_hi:
            self.cur_value_hi = hi

        self.polarity=pol
        self.meter.queue_draw()
        
    def set_reading_auto_vu (self, value, pol="+", lp=0.5):
        self.cur_value = (1.-lp)*self.cur_value + lp*value
        self.cur_value_hi = 0.97*self.cur_value_hi + 0.03*value
        if value > self.cur_value_hi:
            self.cur_value_hi = value
        self.cur_value_lo = 0.97*self.cur_value_lo + 0.03*value
        if value < self.cur_value_lo:
            self.cur_value_lo = value
        self.polarity=pol
        self.meter.queue_draw()
        
    def get_format(self):
        return self.format
    def get_cur_value(self):
        return self.cur_value
    def get_lo_value(self):
        return self.cur_value_lo
    def get_hi_value(self):
        return self.cur_value_hi

## END
