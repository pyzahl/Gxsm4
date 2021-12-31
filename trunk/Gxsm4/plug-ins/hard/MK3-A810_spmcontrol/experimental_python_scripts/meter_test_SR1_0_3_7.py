#!/usr/bin/env python3

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
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GLib

import os                # use os because python IO is bugy

import cairo
import time
import fcntl
from threading import Timer

#import GtkExtra
import struct
import array
import math
from numpy  import *
from pylab import *
import matplotlib.pyplot as plt
import matplotlib.animation as animation

from meterwidget_3_7 import *
from scopewidget_3_7 import *


wins = {}

sr_dev_base = "/dev/sranger"
global sr_dev_path
updaterate = 200

delaylinelength = 32

global SPM_STATEMACHINE
global analog_offset         # at magic[10]+16
global AIC_gain          # at magic[8]+1+3*8
global AIC_in_buffer         # at magic[6]


def delete_event(win, event=None):
        win.hide()
        # don't destroy window -- just leave it hidden
        return True

# open SRanger, read magic struct
def open_sranger():
    global sr_dev_path

    for i in range(0,8):
        sr_dev_path = sr_dev_base+"%d" %i
        print ("looking for SRanger-STD/SP2 at " + sr_dev_path)
        try:
                print ("try open " + sr_dev_path)
                sr = open (sr_dev_path, "rb")
                print (sr)
                sr.seek (0x4000, 1)
                magic_fmt = ">HHHHHHHHHHHHHHHHHHHHH"
                magic = struct.unpack (magic_fmt, sr.read (struct.calcsize (magic_fmt)))
                print (magic)
                sr.close ()
                return magic

        except IOError:
                print ('Oh dear. No such device.')
    print ('No accessible DSP device SRanger found. Plugged in, booted, Permissions OK?')
    return None

# create iir edit windows, IIR param addresses are at magic[14...17]
def zero_buffers(_button):
        for o in range(0,8):
                zero_128 (magic[11]+128*o)

def get_status():
        global SPM_STATEMACHINE
        global analog_offset
        global AIC_gain
        global AIC_in_buffer

        sr = open (sr_dev_path)

        fmt = ">hhhhhhhhllllllll"
        os.lseek (sr.fileno(), magic[10] + 16, 0)
        analog_offset = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
        fmt = ">hhhhhhhh"
        os.lseek (sr.fileno(), magic[10] + 16 + 8+2*2*8+8+6*32+2*128, 0)
        analog_out_offset = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))

        fmt = ">hhhhlllhhh"
        os.lseek (sr.fileno(), magic[5], 0)
        SPM_STATEMACHINE = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))
        
        fmt = ">hhhhhhhhhhhhhhhh"
        os.lseek (sr.fileno(), magic[6], 0)
        AIC_in_buffer = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))

        fmt = ">hhhhhhhh"
        os.lseek (sr.fileno(), magic[8]+1+3*8, 0)
        AIC_gain = struct.unpack (fmt, os.read (sr.fileno(), struct.calcsize (fmt)))

        sr.close ()
        return 1






def set_flag(mask):
        sr = open (sr_dev_path, "wb")
        sr.seek (magic[5], 1)
        sr.write (struct.pack (">h", mask))
        sr.close ()
        get_status()

def reset_flag(mask):
        sr = open (sr_dev_path, "wb")
        sr.seek (magic[5]+1, 1)
        sr.write (struct.pack (">h", mask))
        sr.close ()
        get_status()

def toggle_flag(w, mask):
        sr = open (sr_dev_path, "wb")
        if w.get_active():
                sr.seek (magic[5], 1)
        else:
                sr.seek (magic[5]+1, 1)
        sr.write (struct.pack (">h", mask))
        sr.close ()
        get_status()
        
def check_dsp_status(_set):
        _set ("DataProcessTicks: %05d" %SPM_STATEMACHINE[8] + "  Idle: %05d" %SPM_STATEMACHINE[9] + "  DSP Load: %.1f" %(100.*SPM_STATEMACHINE[8]/(SPM_STATEMACHINE[8]+SPM_STATEMACHINE[9])) + "  Peak: %.1f" %(100.*SPM_STATEMACHINE[10]/(SPM_STATEMACHINE[10]+SPM_STATEMACHINE[11])))
        # + " S:%05d" %(SPM_STATEMACHINE[8]+SPM_STATEMACHINE[9]))
        return 1

def zero_128(adr):
        sr = open (sr_dev_path, "wb")
        sr.seek (adr, 1)
        sr.write (struct.pack (">hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh",
                               0,0,0,0, 0,0,0,0,
                               0,0,0,0, 0,0,0,0,
                               0,0,0,0, 0,0,0,0,
                               0,0,0,0, 0,0,0,0,
                                      0,0,0,0, 0,0,0,0,
                               0,0,0,0, 0,0,0,0,
                               0,0,0,0, 0,0,0,0,
                               0,0,0,0, 0,0,0,0,
                               0,0,0,0, 0,0,0,0,
                               0,0,0,0, 0,0,0,0,
                               0,0,0,0, 0,0,0,0,
                               0,0,0,0, 0,0,0,0,
                               0,0,0,0, 0,0,0,0,
                               0,0,0,0, 0,0,0,0,
                               0,0,0,0, 0,0,0,0,
                               0,0,0,0, 0,0,0,0,
                               ))
        sr.close ()

def _smacXXX(a, x, f):
        s = a + x*f*2
        if s > 1<<31:  ### what's going wrong here???
                s = 1<<31
        if s < -(1>>31):
                s = -(1<<31)
        return s

def _smac(a, x, f):
        s = a + x*f*2
        return s

# get current IIR parameters: IIR.delay and IIR.gain -- and simulate result similar to DSP calculations
def read_iir(_button):
        iir_address=magic[10]
        fmt = ">hhhhhhhhhhhhhhhhhh"
        sr = open (sr_dev_path, "rb")
        sr.seek (iir_address, 1)
        iir_delay    = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        iir_sa0_gain = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        iir_sa1_gain = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        iir_sa2_gain = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        iir_sa3_gain = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        iir_sb0_gain = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        iir_sb1_gain = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        iir_sb2_gain = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        iir_sb3_gain = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        sr.close ()
        
        print ("IIRadr:", iir_address)
        print ("Delay:", iir_delay)
        print ("Gain-b0:", iir_sb0_gain)
        print ("Gain-a0:", iir_sa0_gain)
        print ("Gain-b1:", iir_sb1_gain)
        print ("Gain-a1:", iir_sa1_gain)
        print ("Gain-b2:", iir_sb2_gain)
        print ("Gain-a2:", iir_sa2_gain)
        print ("Gain-b3:", iir_sb3_gain)
        print ("Gain-a3:", iir_sa3_gain)

        input_delayline = zeros (delaylinelength)
        stage12_delayline = zeros (delaylinelength)
        stage23_delayline = zeros (delaylinelength)
        stage34_delayline = zeros (delaylinelength)
        stage4f_delayline = zeros (delaylinelength)

        NT = 6000
        t = arange(0.0, NT, 1.0)/22.000
        step = zeros (NT)
        response = zeros (NT)
        p=0.5 *327.6
        for i in range(1024, 2000):
                step[i] = p

        for i in range(2000, NT):
                n=i-2000
                f=n/(NT-2000.)
                step[i] = p*math.cos (n*2*math.pi/22000.*(10+500.*exp(-10.*(1.-f))))

        slast=0

        G=1
        cfs=2
        cfshl=16-cfs
        
        DELAYLINE_MASK = delaylinelength-1
        for k in range(0, NT):
                input_delayline [k & DELAYLINE_MASK] = round (step[k])

#                // IIR input stage1 ----------------------------------
                sum = 0.
                sum = _smac (sum, input_delayline[(k+iir_delay[0]) & DELAYLINE_MASK], iir_sb0_gain[0])
                sum = _smac (sum, input_delayline[(k+iir_delay[1]) & DELAYLINE_MASK], iir_sb0_gain[1])
                sum = _smac (sum, input_delayline[(k+iir_delay[2]) & DELAYLINE_MASK], iir_sb0_gain[2])
                sum = _smac (sum, input_delayline[(k+iir_delay[3]) & DELAYLINE_MASK], iir_sb0_gain[3])

#                // IIR stage12 ----------------------------------
#0                sum = _smac (sum, stage12_delayline[(k+iir_delay[0]) & DELAYLINE_MASK], iir_sa0_gain[0])
                sum = _smac (sum, stage12_delayline[(k+iir_delay[1]) & DELAYLINE_MASK], iir_sa0_gain[1])
                sum = _smac (sum, stage12_delayline[(k+iir_delay[2]) & DELAYLINE_MASK], iir_sa0_gain[2])
                sum = _smac (sum, stage12_delayline[(k+iir_delay[3]) & DELAYLINE_MASK], iir_sa0_gain[3])

                signal = sum / (1<<cfshl)
                stage12_delayline [k & DELAYLINE_MASK] = signal

                sum = 0.
                sum = _smac (sum, stage12_delayline[(k+iir_delay[0]) & DELAYLINE_MASK], iir_sb1_gain[0])
                sum = _smac (sum, stage12_delayline[(k+iir_delay[1]) & DELAYLINE_MASK], iir_sb1_gain[1])
                sum = _smac (sum, stage12_delayline[(k+iir_delay[2]) & DELAYLINE_MASK], iir_sb1_gain[2])
                sum = _smac (sum, stage12_delayline[(k+iir_delay[3]) & DELAYLINE_MASK], iir_sb1_gain[3])

#                // IIR stage23 ----------------------------------
#0                sum = _smac (sum, stage23_delayline[(k+iir_delay[0]) & DELAYLINE_MASK], iir_sa1_gain[0])
                sum = _smac (sum, stage23_delayline[(k+iir_delay[1]) & DELAYLINE_MASK], iir_sa1_gain[1])
                sum = _smac (sum, stage23_delayline[(k+iir_delay[2]) & DELAYLINE_MASK], iir_sa1_gain[2])
                sum = _smac (sum, stage23_delayline[(k+iir_delay[3]) & DELAYLINE_MASK], iir_sa1_gain[3])

                signal = sum / (1<<cfshl)
                stage23_delayline [k & DELAYLINE_MASK] = signal

                sum = 0.
                sum = _smac (sum, stage23_delayline[(k+iir_delay[0]) & DELAYLINE_MASK], iir_sb2_gain[0])
                sum = _smac (sum, stage23_delayline[(k+iir_delay[1]) & DELAYLINE_MASK], iir_sb2_gain[1])
                sum = _smac (sum, stage23_delayline[(k+iir_delay[2]) & DELAYLINE_MASK], iir_sb2_gain[2])
                sum = _smac (sum, stage23_delayline[(k+iir_delay[3]) & DELAYLINE_MASK], iir_sb2_gain[3])

#                // IIR stage34 ----------------------------------
#0                sum = _smac (sum, stage34_delayline[(k+iir_delay[0]) & DELAYLINE_MASK], iir_sa2_gain[0])
                sum = _smac (sum, stage34_delayline[(k+iir_delay[1]) & DELAYLINE_MASK], iir_sa2_gain[1])
                sum = _smac (sum, stage34_delayline[(k+iir_delay[2]) & DELAYLINE_MASK], iir_sa2_gain[2])
                sum = _smac (sum, stage34_delayline[(k+iir_delay[3]) & DELAYLINE_MASK], iir_sa2_gain[3])

                signal = sum / (1<<cfshl)
                stage34_delayline [k & DELAYLINE_MASK] = signal

                sum = 0.
                sum = _smac (sum, stage34_delayline[(k+iir_delay[0]) & DELAYLINE_MASK], iir_sb3_gain[0])
                sum = _smac (sum, stage34_delayline[(k+iir_delay[1]) & DELAYLINE_MASK], iir_sb3_gain[1])
                sum = _smac (sum, stage34_delayline[(k+iir_delay[2]) & DELAYLINE_MASK], iir_sb3_gain[2])
                sum = _smac (sum, stage34_delayline[(k+iir_delay[3]) & DELAYLINE_MASK], iir_sb3_gain[3])

#                // IIR stage4f ----------------------------------
#                sum = _smac (sum, stage4f_delayline[(k+iir_delay[0]) & DELAYLINE_MASK], iir_sa3_gain[0])
                sum = _smac (sum, stage4f_delayline[(k+iir_delay[1]) & DELAYLINE_MASK], iir_sa3_gain[1])
                sum = _smac (sum, stage4f_delayline[(k+iir_delay[2]) & DELAYLINE_MASK], iir_sa3_gain[2])
                sum = _smac (sum, stage4f_delayline[(k+iir_delay[3]) & DELAYLINE_MASK], iir_sa3_gain[3])

                signal = sum / (1<<cfshl)
                stage4f_delayline [k & DELAYLINE_MASK] = signal

                # simple
#                signal = iir_in_gain[0]*step[k] + iir_s12_gain[0]*slast
#                signal = signal / (1<<15)
#                slast  = signal

                response[k] = signal
                        
        s0 = step/32767.
        s1 = response/32767.
        plot(t, s0)

        subplot(2, 1, 1)
        plot(t, s0, 'y.-')
        title('LFE Signal Processing')
        ylabel('step (1V)')

        subplot(2, 1, 2)
        plot(t, s1, 'r.-')
        xlabel('time (ms)')
        ylabel('response (1V)')
        grid(True)
#        savefig("test.png")
        show()


def read_delaylines(_button):
        # pause mode MD_AIC_RECONFIG (==8) in statemachine
        sr = open (sr_dev_path, "wb")
        sr.seek (magic[5], 1)
        sr.write (struct.pack (">h", 0x20))
        sr.close ()

        fmt = ">hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"
        sr = open (sr_dev_path, "rb")
        sr.seek (magic[11], 1)
        delayline_0  = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        sr.close ()
        sr = open (sr_dev_path, "rb")
        sr.seek (magic[12], 1)
        delayline_12 = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        sr.close ()
        sr = open (sr_dev_path, "rb")
        sr.seek (magic[13], 1)
        delayline_23 = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        sr.close ()
        sr = open (sr_dev_path, "rb")
        sr.seek (magic[14], 1)
        delayline_34 = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        sr.close ()
        sr = open (sr_dev_path, "rb")
        sr.seek (magic[15], 1)
        delayline_4f = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        sr.close ()

        sr = open (sr_dev_path, "wb")
        sr.seek (magic[5], 1)
        sr.write (struct.pack (">hh", 0, 0x20))
        sr.close ()


        print ("Input Delayline0:", delayline_0)
        print ("Output Delayline12:", delayline_4f)

        t = arange(0.0, delaylinelength, 1.0)/22.000
        s0 = array(delayline_0)/3276.7
        s1 = array(delayline_4f)/3276.7
        plot(t, s0)

        subplot(2, 1, 1)
        plot(t, s0, 'y.-')
        title('LFE Signal Processing')
        ylabel('In s0 (mV)')

        subplot(2, 1, 2)
        plot(t, s1, 'r.-')
        xlabel('time (ms)')
        ylabel('Out s12 (mV)')
        grid(True)
#        savefig("test.png")
        show()

def read_subbuffers(_button):
        NUM=128
        t  = arange(0.0, NUM, 1.0)/22.000

        def readdata():
                sr = open (sr_dev_path, "wb")
                sr.seek (magic[5], 1)
                sr.write (struct.pack (">h", 0x20))
                sr.close ()

                fmt = ">hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"
                sr = open (sr_dev_path, "rb")
                sr.seek (magic[16], 1)
                delayline_0  = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
                sr.close ()
                sr = open (sr_dev_path, "rb")
                sr.seek (magic[17], 1)
                delayline_1 = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
                sr.close ()
                
                sr = open (sr_dev_path, "wb")
                sr.seek (magic[5], 1)
                sr.write (struct.pack (">hh", 0, 0x20))
                sr.close ()

                s0 = np.array(delayline_0)/3276.7
                s1 = np.array(delayline_1)/3276.7
                return s0,s1

        s0, s1 = readdata()

        fig, ax = plt.subplots()
#        plt.subplot(2, 1, 1)
        line1, = ax.plot(t, s0, 'y-')
        plt.title('LFE Signal Processing')
        plt.ylabel('Dec Subbuf (mV)')
        plt.grid(True)
        plt.ylim(-1., 1.)

#        plt.subplot(2, 1, 2)
        line2, = ax.plot(t, s1, 'r-')
        plt.xlabel('time (ms)')
        plt.ylabel('Inc Subbuf (mV)')
        plt.grid(True)
        plt.ylim(-10., 10.)
        #        savefig("test.png")


        def update_line(i):
                y1, y2 = readdata()
                line1.set_ydata(y1) # update the data
                line2.set_ydata(y2) # update the data
                return line1, line2

        def init():
                line1.set_ydata(np.ma.array(t, mask=True))
                line2.set_ydata(np.ma.array(t, mask=True))
                return line1,

        ani = animation.FuncAnimation(fig, update_line, arange(1,1000), init_func=init, interval=200, blit=True)
        plt.show()


class SignalScope():

    def __init__(self, _button):

        label = "Oscilloscope"
        name=label
        self.ss = 0
        self.block_length = 128
        self.restart_func = self.nop
        self.trigger = 0
        self.trigger_level = 0
        win = Gtk.Window()
        wins[name] = win
        v = Gtk.VBox(spacing=0)

        scope = Oscilloscope( Gtk.Label( v, "XT", label))
        scope.scope.set_wide (True)
        scope.show()
        scope.set_chinfo(["L", "R"])
        win.add(v)
        if not self.ss:
                self.block_length = 128
                #parent.mk3spm.set_recorder (self.block_length, self.trigger, self.trigger_level)

        table = Gtk.Table(n_rows=4, n_columns=2)
        #table.set_row_spacings(5)
        #table.set_col_spacings(5)
        v.pack_start (table, True, True, 0)
        table.show()

        tr=0
        lab = Gtk.Label(label="# Samples:")
        lab.show ()
        table.attach(lab, 2, 3, tr, tr+1)
        Samples = Gtk.Entry()
        Samples.set_text("%d"%self.block_length)
        table.attach(Samples, 2, 3, tr+1, tr+2)
        Samples.show()

        #                [lab1, menu1] = parent.make_signal_selector (DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID, signal, "CH1: ", parent.global_vector_index)
        #                lab = GObject.new(Gtk.Label, label="CH1-scale:")
        #                lab1.show ()
        #                table.attach(lab1, 0, 1, tr, tr+1)
        Xscale = Gtk.Entry()
        Xscale.set_text("1.")
        table.attach(Xscale, 0, 1, tr+1, tr+2)
        Xscale.show()

        #                [signal,data,offset] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID)
        #                [lab2, menu1] = parent.make_signal_selector (DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID, signal, "CH2: ", parent.global_vector_index)
        ##                lab = GObject.new(Gtk.Label, label="CH2-scale:")
        #                lab2.show ()
        #                table.attach(lab2, 1, 2, tr, tr+1)
        Yscale = Gtk.Entry()
        Yscale.set_text("1.")
        table.attach(Yscale, 1, 2, tr+1, tr+2)
        Yscale.show()
        
        tr = tr+2
        labx0 = Gtk.Label(label="X-off:")
        labx0.show ()
        table.attach(labx0, 0, 1, tr, tr+1)
        Xoff = Gtk.Entry()
        Xoff.set_text("0")
        table.attach(Xoff, 0, 1, tr+1, tr+2)
        Xoff.show()
        laby0 = Gtk.Label(label="Y-off:")
        laby0.show ()
        table.attach(laby0, 1, 2, tr, tr+1)
        Yoff = Gtk.Entry()
        Yoff.set_text("0")
        table.attach(Yoff, 1, 2, tr+1, tr+2)
        Yoff.show()
        
        self.xdc = 0.
        self.ydc = 0.

        def readdata():
                self.block_length = 128
                sr = open (sr_dev_path, "wb")
                sr.seek (magic[5], 1)
                sr.write (struct.pack (">h", 0x20))
                sr.close ()
                
                fmt = ">hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"

                sr = open (sr_dev_path, "rb")
                sr.seek (magic[16], 1)
                delayline_0  = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
                sr.close ()
                sr = open (sr_dev_path, "rb")
                sr.seek (magic[17], 1)
                delayline_1 = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
                sr.close ()
                
                sr = open (sr_dev_path, "wb")
                sr.seek (magic[5], 1)
                sr.write (struct.pack (">hh", 0, 0x20))
                sr.close ()
                
                s0 = np.array(delayline_0)/3276.7
                s1 = np.array(delayline_1)/3276.7
                return s0,s1
        
        def update_scope(set_data, xs, ys, x0, y0, num, x0l, y0l):
                #blck = parent.mk3spm.check_recorder_ready ()
                blck = -1
                if blck == -1:
                        n = self.block_length
                        #[xd, yd] = parent.mk3spm.read_recorder (n)
                        [xd, yd] = readdata ()
                        
                        #if not self.run:
                        #                save("mk3_recorder_xdata", xd)
                        #                save("mk3_recorder_ydata", yd)
                        #                scope.set_flash ("Saved: mk3_recorder_[xy]data")
                        
                        # auto subsample if big
                        nss = n
                        nraw = n
                        if n > 4096:
                                ss = int(n/2048)
                                end =  ss * int(len(xd)/ss)
                                nss = (int)(n/ss)
                                xd = mean(xd[:end].reshape(-1, ss), 1)
                                yd = mean(yd[:end].reshape(-1, ss), 1)
                                scope.set_info(["sub sampling: %d"%n + " by %d"%ss,
                                                "T = %g ms"%(n/22.)])
                        else:
                                scope.set_info(["T = %g ms"%(n/22.)])
                                
                        # change number samples?
                        try:
                                self.block_length = int(num())
                                if self.block_length < 64:
                                        self.block_length = 64
                                if self.block_length > 999999:
                                        print ("MAX BLOCK LEN is 999999")
                                        self.block_length = 1024
                        except ValueError:
                                self.block_length = 128

                        if self.block_length != n or self.ss:
                                self.run = False
                                self.run_button.set_label("RESTART")

                        ##if not self.ss:
                        ##parent.mk3spm.set_recorder (self.block_length, self.trigger, self.trigger_level)
                        #                                v = value * signal[SIG_D2U]
                        #                                maxv = (1<<31)*math.fabs(signal[SIG_D2U])
                        try:
                                xscale_div = float(xs())
                        except ValueError:
                                xscale_div = 1
                                
                        try:
                                yscale_div = float(ys())
                        except ValueError:
                                yscale_div = 1

                        n = nss
                        try:
                                self.xoff = float(x0())
                                for i in range (0, n, 8):
                                        self.xdc = 0.9 * self.xdc + 0.1 * xd[i] * 1. #Xsignal[SIG_D2U]
                                x0l("X-DC = %g"%self.xdc)
                        except ValueError:
                                for i in range (0, n, 8):
                                        self.xoff = 0.9 * self.xoff + 0.1 * xd[i] * 1. #Xsignal[SIG_D2U]
                                x0l("X-off: %g"%self.xoff)

                        try:
                                self.yoff = float(y0())
                                for i in range (0, n, 8):
                                        self.ydc = 0.9 * self.ydc + 0.1 * yd[i] * 1. #Ysignal[SIG_D2U]
                                y0l("Y-DC = %g"%self.ydc)
                        except ValueError:
                                for i in range (0, n, 8):
                                        self.yoff = 0.9 * self.yoff + 0.1 * yd[i] * 1. #Ysignal[SIG_D2U]
                                y0l("Y-off: %g"%self.yoff)

                        if math.fabs(xscale_div) > 0:
                                xd = -(xd * 1. - self.xoff) / xscale_div
                        else:
                                xd = xd * 0. # ZERO/OFF
                                        
                        self.trigger_level = self.xoff / 1

                        if math.fabs(yscale_div) > 0:
                                yd = -(yd * 1. - self.yoff) / yscale_div
                        else:
                                yd = yd * 0. # ZERO/OFF
                                
                        scope.set_scale ( { 
                                "CH1:": "%g"%xscale_div + " V",
                                "CH2:": "%g"%yscale_div + " V",
                                "Timebase:": "%g ms"%(nraw/22./20.) 
                        })
                                
                        scope.set_data (xd, yd)

                if self.mode > 1:
                        self.run_button.set_label("SINGLE")
                else:
                        scope.set_info(["waiting for trigger or data [%d]"%blck])
                        scope.queue_draw()

                return self.run

                def stop_update_scope (win, event=None):
                        print ("STOP, hide.")
                        win.hide()
                        self.run = False
                        return True

                def toggle_run_recorder (b):
                        if self.run:
                                self.run = False
                                self.run_button.set_label("RUN")
                        else:
                                self.restart_func ()
                                
                                #[Xsignal, Xdata, Offset] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID)
                                #[Ysignal, Ydata, Offset] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID)
                                #scope.set_chinfo([Xsignal[SIG_NAME], Ysignal[SIG_NAME]])

                                period = int(2.*self.block_length/22.)
                                if period < 200:
                                        period = 200

                                if self.mode < 2: 
                                        self.run_button.set_label("STOP")
                                        self.run = True
                                else:
                                        self.run_button.set_label("ARMED")
                                        #parent.mk3spm.set_recorder (self.block_length, self.trigger, self.trigger_level)
                                        self.run = False

                                GLib.timeout_add (period, update_scope, scope, Xscale.get_text, Yscale.get_text, Xoff.get_text, Yoff.get_text, Samples.get_text, labx0.set_text, laby0.set_text)

                def toggle_trigger (b):
                        if self.trigger == 0:
                                self.trigger = 1
                                self.trigger_button.set_label("TRIGGER POS")
                        else:
                                if self.trigger > 0:
                                        self.trigger = -1
                                        self.trigger_button.set_label("TRIGGER NEG")
                                else:
                                        self.trigger = 0
                                        self.trigger_button.set_label("TRIGGER OFF")
                        print (self.trigger, self.trigger_level)

                def toggle_mode (b):
                        if self.mode == 0:
                                self.mode = 1
                                self.ss = 0
                                self.mode_button.set_label("T-Auto")
                        else:
                                if self.mode == 1:
                                        self.mode = 2
                                        self.ss = 0
                                        self.mode_button.set_label("T-Normal")
                                else:
                                        if self.mode == 2:
                                                self.mode = 3
                                                self.ss = 1
                                                self.mode_button.set_label("T-Single")
                                        else:
                                                self.mode = 0
                                                self.ss = 0
                                                self.mode_button.set_label("T-Free")

                self.run_button = Gtk.Button("STOP")
#                self.run_button.set_use_stock(True)
                self.run_button.connect("clicked", toggle_run_recorder)
                self.hb = Gtk.HBox()
                self.hb.pack_start (self.run_button, True, True, 0)
                self.mode_button = Gtk.Button("M: Free")
                self.mode=0 # 0 free, 1 auto, 2 nommal, 3 single
                self.mode_button.connect("clicked", toggle_mode)
                self.hb.pack_start (self.mode_button, True, True, 0)
                self.mode_button.show ()
                table.attach(self.hb, 2, 3, tr, tr+1)
                tr = tr+1

                self.trigger_button = Gtk.Button("TRIGGER-OFF")
                self.trigger_button.connect("clicked", toggle_trigger)
                self.trigger_button.show ()
                table.attach(self.trigger_button, 2, 3, tr, tr+1)

                self.run = False
                win.connect("delete_event", stop_update_scope)
                toggle_run_recorder (self.run_button)
                
        wins[name].show_all()

    def set_restart_callback (self, func):
        self.restart_func = func
                        
    def nop (self):
        return 0
                        



        
# set tap delays to 0, 8, ..40,[] 3x, zero all gains
def set_iir_zero(adr):
        n=1
        dl=delaylinelength
        sr = open (sr_dev_path, "wb")
        sr.seek (magic[10], 1)
        sr.write (struct.pack (">hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh",
                               dl-0,dl-1*n,dl-2*n,dl-3*n,dl-4*n,dl-5*n,
                               dl-0,dl-1*n,dl-2*n,dl-3*n,dl-4*n,dl-5*n,
                               dl-0,dl-1*n,dl-2*n,dl-3*n,dl-4*n,dl-5*n,
                               0,0,0,0,0,0,
                               0,0,0,0,0,0,
                               0,0,0,0,0,0,
                               0,0,0,0,0,0,
                               0,0,0,0,0,0,
                               0,0,0,0,0,0
                               ))
        sr.close ()
        
def set_iir_zero_mk_delay(n):
        dl=delaylinelength
        sr = open (sr_dev_path, "wb")
        sr.seek (magic[10], 1)
        sr.write (struct.pack (">hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh",
                               dl-0,dl-1*n,dl-2*n,dl-3*n,dl-4*n,dl-5*n,
                               dl-0,dl-1*n,dl-2*n,dl-3*n,dl-4*n,dl-5*n,
                               dl-0,dl-1*n,dl-2*n,dl-3*n,dl-4*n,dl-5*n,
                               0,0,0,0,0,0,
                               0,0,0,0,0,0,
                               0,0,0,0,0,0,
                               0,0,0,0,0,0,
                               0,0,0,0,0,0,
                               0,0,0,0,0,0
                               ))
        sr.close ()
        
# -- n is decimation factor, is 1 (off) or 8 (on
def calc_iir_stop():
        set_iir_zero_mk_delay(1)
        ab = calc_iir_coef_stop ()
        return ab

def calc_iir_pass():
        set_iir_zero_mk_delay(1)
        ab = calc_iir_coef_pass ()
        return ab

def calc_iir(fg,n):
        set_iir_zero_mk_delay(n)
        ab = calc_iir_coef_1simple (fg/22000.*n)
        return ab

def calc_iir_biquad(fg,n):
#        set_iir_zero_mk_delay(1)
        ab = calc_iir_coef_biquad_single (fg/22000.*n)
        return ab


##         Cookbook formulae for audio EQ biquad filter coefficients
##----------------------------------------------------------------------------
##           by Robert Bristow-Johnson  <rbj@audioimagination.com>


##All filter transfer functions were derived from analog prototypes (that
##are shown below for each EQ filter type) and had been digitized using the
##Bilinear Transform.  BLT frequency warping has been taken into account for
##both significant frequency relocation (this is the normal "prewarping" that
##is necessary when using the BLT) and for bandwidth readjustment (since the
##bandwidth is compressed when mapped from analog to digital using the BLT).

##First, given a biquad transfer function defined as:

##            b0 + b1*z^-1 + b2*z^-2
##    H(z) = ------------------------                                  (Eq 1)
##            a0 + a1*z^-1 + a2*z^-2

##This shows 6 coefficients instead of 5 so, depending on your architechture,
##you will likely normalize a0 to be 1 and perhaps also b0 to 1 (and collect
##that into an overall gain coefficient).  Then your transfer function would
##look like:

#### THIS IT IS HERE #####

##            (b0/a0) + (b1/a0)*z^-1 + (b2/a0)*z^-2
##    H(z) = ---------------------------------------                   (Eq 2)
##               1 + (a1/a0)*z^-1 + (a2/a0)*z^-2

##or

##                      1 + (b1/b0)*z^-1 + (b2/b0)*z^-2
##    H(z) = (b0/a0) * ---------------------------------               (Eq 3)
##                      1 + (a1/a0)*z^-1 + (a2/a0)*z^-2


##The most straight forward implementation would be the "Direct Form 1"
##(Eq 2):

##    y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2]
##                        - (a1/a0)*y[n-1] - (a2/a0)*y[n-2]            (Eq 4)

##This is probably both the best and the easiest method to implement in the
##56K and other fixed-point or floating-point architechtures with a double
##wide accumulator.



##Begin with these user defined parameters:

##    Fs (the sampling frequency)

##    f0 ("wherever it's happenin', man."  Center Frequency or
##        Corner Frequency, or shelf midpoint frequency, depending
##        on which filter type.  The "significant frequency".)

##    dBgain (used only for peaking and shelving filters)

##    Q (the EE kind of definition, except for peakingEQ in which A*Q is
##        the classic EE Q.  That adjustment in definition was made so that
##        a boost of N dB followed by a cut of N dB for identical Q and
##        f0/Fs results in a precisely flat unity gain filter or "wire".)

##     _or_ BW, the bandwidth in octaves (between -3 dB frequencies for BPF
##        and notch or between midpoint (dBgain/2) gain frequencies for
##        peaking EQ)

##     _or_ S, a "shelf slope" parameter (for shelving EQ only).  When S = 1,
##        the shelf slope is as steep as it can be and remain monotonically
##        increasing or decreasing gain with frequency.  The shelf slope, in
##        dB/octave, remains proportional to S for all other values for a
##        fixed f0/Fs and dBgain.



##Then compute a few intermediate variables:
        
##    A  = sqrt( 10^(dBgain/20) )
##       =       10^(dBgain/40)     (for peaking and shelving EQ filters only)

##    w0 = 2*pi*f0/Fs

##    cos(w0)
##    sin(w0)

##    alpha = sin(w0)/(2*Q)                                       (case: Q)
##          = sin(w0)*sinh( ln(2)/2 * BW * w0/sin(w0) )           (case: BW)
##          = sin(w0)/2 * sqrt( (A + 1/A)*(1/S - 1) + 2 )         (case: S)

##        FYI: The relationship between bandwidth and Q is
##             1/Q = 2*sinh(ln(2)/2*BW*w0/sin(w0))     (digital filter w BLT)
##        or   1/Q = 2*sinh(ln(2)/2*BW)             (analog filter prototype)

##        The relationship between shelf slope and Q is
##             1/Q = sqrt((A + 1/A)*(1/S - 1) + 2)

##    2*sqrt(A)*alpha  =  sin(w0) * sqrt( (A^2 + 1)*(1/S - 1) + 2*A )
##        is a handy intermediate variable for shelving EQ filters.


##Finally, compute the coefficients for whichever filter type you want:
##   (The analog prototypes, H(s), are shown for each filter
##        type for normalized frequency.)


##LPF:        H(s) = 1 / (s^2 + s/Q + 1)

##            b0 =  (1 - cos(w0))/2
##            b1 =   1 - cos(w0)
##            b2 =  (1 - cos(w0))/2
##            a0 =   1 + alpha
##            a1 =  -2*cos(w0)
##            a2 =   1 - alpha



##HPF:        H(s) = s^2 / (s^2 + s/Q + 1)

##            b0 =  (1 + cos(w0))/2
##            b1 = -(1 + cos(w0))
##            b2 =  (1 + cos(w0))/2
##            a0 =   1 + alpha
##            a1 =  -2*cos(w0)
##            a2 =   1 - alpha



##BPF:        H(s) = s / (s^2 + s/Q + 1)  (constant skirt gain, peak gain = Q)

##            b0 =   sin(w0)/2  =   Q*alpha
##            b1 =   0
##            b2 =  -sin(w0)/2  =  -Q*alpha
##            a0 =   1 + alpha
##            a1 =  -2*cos(w0)
##            a2 =   1 - alpha


##BPF:        H(s) = (s/Q) / (s^2 + s/Q + 1)      (constant 0 dB peak gain)

##            b0 =   alpha
##            b1 =   0
##            b2 =  -alpha
##            a0 =   1 + alpha
##            a1 =  -2*cos(w0)
##            a2 =   1 - alpha



##notch:      H(s) = (s^2 + 1) / (s^2 + s/Q + 1)

##            b0 =   1
##            b1 =  -2*cos(w0)
##            b2 =   1
##            a0 =   1 + alpha
##            a1 =  -2*cos(w0)
##            a2 =   1 - alpha



##APF:        H(s) = (s^2 - s/Q + 1) / (s^2 + s/Q + 1)

##            b0 =   1 - alpha
##            b1 =  -2*cos(w0)
##            b2 =   1 + alpha
##            a0 =   1 + alpha
##            a1 =  -2*cos(w0)
##            a2 =   1 - alpha



##peakingEQ:  H(s) = (s^2 + s*(A/Q) + 1) / (s^2 + s/(A*Q) + 1)

##            b0 =   1 + alpha*A
##            b1 =  -2*cos(w0)
##            b2 =   1 - alpha*A
##            a0 =   1 + alpha/A
##            a1 =  -2*cos(w0)
##            a2 =   1 - alpha/A



##lowShelf: H(s) = A * (s^2 + (sqrt(A)/Q)*s + A)/(A*s^2 + (sqrt(A)/Q)*s + 1)

##            b0 =    A*( (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha )
##            b1 =  2*A*( (A-1) - (A+1)*cos(w0)                   )
##            b2 =    A*( (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha )
##            a0 =        (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha
##            a1 =   -2*( (A-1) + (A+1)*cos(w0)                   )
##            a2 =        (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha



##highShelf: H(s) = A * (A*s^2 + (sqrt(A)/Q)*s + 1)/(s^2 + (sqrt(A)/Q)*s + A)

##            b0 =    A*( (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha )
##            b1 = -2*A*( (A-1) + (A+1)*cos(w0)                   )
##            b2 =    A*( (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha )
##            a0 =        (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha
##            a1 =    2*( (A-1) - (A+1)*cos(w0)                   )
##            a2 =        (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha

def calc_iir_coef_biquad_single(r):
        w = 2.*math.pi*r
        c = math.cos (w)
        s = math.sin (w)
        BW = 1.
        Q = 1./(2*math.sinh (math.log (2)/2.*BW*w/s))
        alpha = s/(2.*Q)
        a0 = 1 + alpha
        
        a=zeros( (3*4,4) )
        b=zeros( (3*4,4) )

        b[1][0] = (1-c)/2.   # b0
        b[1][1] =  1-c       # b1
        b[1][2] = (1-c)/2.   # b2

        # a0 DSP := 1 normed #  a0 = 1+alpha
        a[0][1] = -(-2*c)       # -a1
        a[0][2] = -(1-alpha)    # -a2

        a = a/a0
        b = b/a0

        b[0][0] = 1.         # 1
        b[3][0] = 1.         # 1
        b[6][0] = 1.
        b[9][0] = 1.

        print ("BiQuad LP calculated:")
        return [a,b]        


# H(s) = poly(An,s) / poly(Bn,s)
#
#                                  (10^(DSa_pass)) Prod_m(cB2(m,n))  
# HChebyshev,n (n even) (s) = ---------------------------------------------
#                                Prod_m(s^2 + cB1(m,n) + cB2(m,n)))
#
#                                  sinh(cD(n)) Prod_m(cB2(m,n))  
# HChebyshev,n (n odd) (s) = ---------------------------------------------
#                            (s+sinh(D)) Prod_m(s^2 + cB1(m,n) + cB2(m,n)))

## http://www.eng.iastate.edu/ee424/labs/C54Docs/spra079.pdf
## higher order: factorize into 2nd order filter products: H4 = H2_1 x H2_2 x ...
# use scilab filter design tools to calculate coefficients:
# Hlp=iir(4,'lp','cheb1',[75/22000/2,0],[0.001,0])
# nf=factors(numer(Hlp))  ==> poly (b[3k][0,1,2])
# df=factors(denom(Hlp))  ==> poly (a[3k][0,2,2])

def calc_iir_coef_stop():
        a=zeros( (3*4,4) )
        b=zeros( (3*4,4) )
        print ("stop IIR calculated:")
        return [a,b]

def calc_iir_coef_pass():
        C = 0
        a=zeros( (3*4,4) )
        b=zeros( (3*4,4) )
        a[0][1] = C
        a[3][1] = 0.

        b[0][0] = 1.-C
        b[0][1] = 0.

        b[3][0] = 1.
        b[6][0] = 1.
        b[9][0] = 1.
        print ("pass IIR calculated:")
        return [a,b]

def calc_iir_coef_1simple(r):
        C=math.exp(-r*2.*math.pi) # near 1
        print ("C=%g"%C)
        a=zeros( (3*4,4) )
        b=zeros( (3*4,4) )
        a[0][1] = C
        a[3][1] = 0.

        b[0][0] = 1.-C
        b[0][1] = 0.

        b[3][0] = 1.
        b[6][0] = 1.
        b[9][0] = 1.
        print ("simple LP calculated:")
        return [a,b]

def norm_iir_coef(a,b):
        # pre-scale
        # norm/limit to elements 0..1, split as needed
        print ("original")
        print ("a=")
        print (a)
        print ("b=")
        print (b)
        for ki in range(0,3):
                sa=1.
                sb=1.
                for i in range(0,4):
                        k=ki*3
                        sa = sa + math.fabs(a[k][i])
                        sb = sb + math.fabs(b[k][i])

                print ("Sum a,b[%d] = :"%ki + "(%f , " %sa + "%f)" %sb)

        a=a/4
        b=b/4

        print ("Scaled, Limited/Normed, Q15")
        # Q15
        q=32767.
        a=a*q
        b=b*q
        print ("a=")
        print (a)
        print ("b=")
        print (b)
        return [a,b]

def norm_iir_coef_XXX(a,b):
        # pre-scale
        a=a/2
        b=b/2
        # norm/limit to elements 0..1, split as needed
        for i in range(0,4):
                for ki in range(0,3):
                        k=ki*3
                        for q in range(0,2):
                                if a[k+q][i] > 1.:
                                        a[k+q+1][i]=a[k+q][i]-1.
                                        a[k+q][i]=1.
                                if a[k+q][i] < -1.:
                                        a[k+q+1][i]=a[k+q][i]+1.
                                        a[k+q][i]=-1.
                                if b[k+q][i] > 1.:
                                        b[k+q+1][i]=b[k+q][i]-1.
                                        b[k+q][i]=1.
                                if b[k+q][i] < -1.:
                                        b[k+q+1][i]=b[k+q][i]+1.
                                        b[k+q][i]=-1.
        print ("Scaled, Limited/Normed, Q15")
        # Q15
        q=32767.
        a=a*q
        b=b*q
        print ("a=")
        print (a)
        print ("b=")
        print (b)
        return [a,b]

#set gains for simple lowpass
def set_iir_stop(adr):
        set_iir_gains (calc_iir_stop())
def set_iir_pass(adr):
        set_iir_gains (calc_iir_pass())
def set_iir_low50(adr):
        set_iir_gains (calc_iir(50.,1))
def set_iir_low60(adr):
        set_iir_gains (calc_iir(60.,1))
def set_iir_low75(adr):
        set_iir_gains (calc_iir(75.,1))
def set_iir_low100(adr):
        set_iir_gains (calc_iir(100.,1))

#set gains for cheb low 60, 75, ..
def set_iir_lowbq60(adr):
        set_iir_gains (calc_iir_biquad(60.,8))
def set_iir_lowbq75(adr):
        set_iir_gains (calc_iir_biquad(75.,8))
def set_iir_lowbq100(adr):
        set_iir_gains (calc_iir_biquad(100.,8))

def set_iir_gains(ab):
        [a,b] = [ab[0], ab[1]]
        
        h = array ([ b[1][2],         b[1][1], b[1][0], a[0][2], a[0][1]/2, 0., 0., 1., 0., 0. ])

        print ("H 2xBiQuad=")
        print (h)

        h = h*32767.*32767.*2.

        print ("H32 2xBiQuad=")
        print (h)


        [a,b] = norm_iir_coef(ab[0], ab[1])
        sr = open (sr_dev_path, "wb")
        sr.seek (magic[10]+18, 1)
        sr.write (struct.pack (">hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh",
                               round(a[0+0,0]),round(a[0+0,1]),round(a[0+0,2]),round(a[0+0,3]),0,0,
                               round(a[0+1,0]),round(a[0+1,1]),round(a[0+1,2]),round(a[0+1,3]),0,0,
                               round(a[0+2,0]),round(a[0+2,1]),round(a[0+2,2]),round(a[0+2,3]),0,0,

                               round(a[3+0,0]),round(a[3+0,1]),round(a[3+0,2]),round(a[3+0,3]),0,0,
                               round(a[3+1,0]),round(a[3+1,1]),round(a[3+1,2]),round(a[3+1,3]),0,0,
                               round(a[3+2,0]),round(a[3+2,1]),round(a[3+2,2]),round(a[3+2,3]),0,0,

                               round(a[6+0,0]),round(a[6+0,1]),round(a[6+0,2]),round(a[6+0,3]),0,0,
                               round(a[6+1,0]),round(a[6+1,1]),round(a[6+1,2]),round(a[6+1,3]),0,0,
                               round(a[6+2,0]),round(a[6+2,1]),round(a[6+2,2]),round(a[6+2,3]),0,0,

                               round(a[9+0,0]),round(a[9+0,1]),round(a[9+0,2]),round(a[9+0,3]),0,0,
                               round(a[9+1,0]),round(a[9+1,1]),round(a[9+1,2]),round(a[9+1,3]),0,0,
                               round(a[9+2,0]),round(a[9+2,1]),round(a[9+2,2]),round(a[9+2,3]),0,0,

                               round(b[0+0,0]),round(b[0+0,1]),round(b[0+0,2]),round(b[0+0,3]),0,0,
                               round(b[0+1,0]),round(b[0+1,1]),round(b[0+1,2]),round(b[0+1,3]),0,0,
                               round(b[0+2,0]),round(b[0+2,1]),round(b[0+2,2]),round(b[0+2,3]),0,0,

                               round(b[3+0,0]),round(b[3+0,1]),round(b[3+0,2]),round(b[3+0,3]),0,0,
                               round(b[3+1,0]),round(b[3+1,1]),round(b[3+1,2]),round(b[3+1,3]),0,0,
                               round(b[3+2,0]),round(b[3+2,1]),round(b[3+2,2]),round(b[3+2,3]),0,0,

                               round(b[6+0,0]),round(b[6+0,1]),round(b[6+0,2]),round(b[6+0,3]),0,0,
                               round(b[6+1,0]),round(b[6+1,1]),round(b[6+1,2]),round(b[6+1,3]),0,0,
                               round(b[6+2,0]),round(b[6+2,1]),round(b[6+2,2]),round(b[6+2,3]),0,0,

                                      round(b[9+0,0]),round(b[9+0,1]),round(b[9+0,2]),round(b[9+0,3]),0,0,
                               round(b[9+1,0]),round(b[9+1,1]),round(b[9+1,2]),round(b[9+1,3]),0,0,
                               round(b[9+2,0]),round(b[9+2,1]),round(b[9+2,2]),round(b[9+2,3]),0,0
                               ))
        sr.close ()

        sr = open (sr_dev_path, "wb")
        sr.seek (magic[19], 1)
        sr.write (struct.pack (">llllllllll",
                               round(h[0]), round(h[1]), round(h[2]), round(h[3]), round(h[4]), 
                               round(h[5]), round(h[6]), round(h[7]), round(h[8]), round(h[9])
                               ))
        sr.close ()

def tubetransfer_test_run(value):
        fmt = ">hhhhh"
        sr = open (sr_dev_path, "wb")
        sr.seek (magic[18]+10, 1)
        sr.write (struct.pack (fmt, round(value), 0, 0, 0, 0))
        sr.close ()

        sr = open (sr_dev_path, "rb")
        sr.seek (magic[18]+10, 1)
        tubetransfer = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        sr.close ()
        print (value, " => ", tubetransfer)
#        print (tubetransfer[0],tubetransfer[1])


def tubetransfer_test(_button):
#        for v in range (-5000, 5000):
#                tubetransfer_test_run (v)    

        for v in range (-5, 5):
                for vv in range (-5, 5):
                        tubetransfer_test_run (vv+v*1024)    

#        for v in range (-10, 10):
#                tubetransfer_test_run (v)    
#        for v in range (-10, 10):
#                tubetransfer_test_run (1024+v)    
#        for v in range (-10, 10):
#                tubetransfer_test_run (-1024+v)    
#        for v in range (-10, 10):
#                tubetransfer_test_run (256+v*16)    

def read_tubetransfer(_button):
        sr = open (sr_dev_path, "rb")
        sr.seek (magic[20], 1)
        fmt = ">hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"
        tubetransfer = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
        sr.close ()
        print (tubetransfer)

def write_tubetransfer(_button, scale):
        sr = open (sr_dev_path, "wb")
        sr.seek (magic[20], 1)

        t = zeros (65);
        # for(i=0, x=-32768; x <= 32768; x+=32768/32) { printf( ("%8d, ", round (32768*atan(10*x/32768)/atan(10))); if (++i==8) {printf ("\n"); i=0;}})
        x = -32768;
        for i in range(0,65):
                t[i] = round (32767.*math.atan(scale*x/32768.)/math.atan(scale))
                x = x+32768/32

        fmt = ">hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"
        sr.write (struct.pack (fmt, 
                               round(t[0]), round(t[1]), round(t[2]), round(t[3]), round(t[4]), round(t[5]), round(t[6]), round(t[7]), 
                               round(t[8]), round(t[9]), round(t[10]), round(t[11]), round(t[12]), round(t[13]), round(t[14]), round(t[15]), 
                               round(t[16]), round(t[17]), round(t[18]), round(t[19]), round(t[20]), round(t[21]), round(t[22]), round(t[23]), 
                               round(t[24]), round(t[25]), round(t[26]), round(t[27]), round(t[28]), round(t[29]), round(t[30]), round(t[31]), 
                               round(t[32]), round(t[33]), round(t[34]), round(t[35]), round(t[36]), round(t[37]), round(t[38]), round(t[39]), 
                               round(t[40]), round(t[41]), round(t[42]), round(t[43]), round(t[44]), round(t[45]), round(t[46]), round(t[47]), 
                               round(t[48]), round(t[49]), round(t[50]), round(t[51]), round(t[52]), round(t[53]), round(t[54]), round(t[55]), 
                               round(t[56]), round(t[57]), round(t[58]), round(t[59]), round(t[60]), round(t[61]), round(t[62]), round(t[63]), 
                               round(t[64])
                       ))
        sr.close ()
        print (t)

def write_tubetransfer20(_button):
        write_tubetransfer(_button, 20.)

def write_tubetransfer10(_button):
        write_tubetransfer(_button, 10.)

def write_tubetransfer5(_button):
        write_tubetransfer(_button, 5.)

def write_tubetransfer1(_button):
        write_tubetransfer(_button, 1.)

def create_iir_edit(_button):
        create_iir_delay_gain_edit(_button, magic[10], "IIR Tap Delays")

def create_gain_edit_out(_button):
        create_gain_edit(_button, magic[8]+1+3*8, "AIC In/Out Gain")

def delay_inv(x):
        return delaylinelength-x

# update one IIR param at given address
def iir_delay_param_update(_adj):
        param_address = _adj.get_data("iir_param_address")
        param = delay_inv(_adj.get_data("iir_param_multiplier")*_adj.value)
#        print ("Address:", param_address)
#        print ("Value:", _adj.value)
#        print ("Data:", param)
        sr = open (sr_dev_path, "wb")
        sr.seek (param_address, 1)
        sr.write (struct.pack (">h", param))
        sr.close ()

# update one IIR param at given address
def iir_gain_param_update(_adj):
        param_address = _adj.get_data("iir_param_address")
        param = int (_adj.get_data("iir_param_multiplier")*_adj.value)
#        print ("Address:", param_address)
#        print ("Value:", _adj.value)
#        print ("Data:", param)
        sr = open (sr_dev_path, "wb")
        sr.seek (param_address, 1)
        sr.write (struct.pack (">h", param))
        sr.close ()

# create IIR editor 
def create_iir_delay_gain_edit(_button, iir_address, name):
        if name not in wins:
                win = Gtk.Window ()
                wins[name] = win
                win.connect("delete_event", delete_event)
                
                box1 = Gtk.VBox(spacing=10)
                win.add(box1)
                box1.show()

                box2 = Gtk.VBox(spacing=10)
                box2.set_border_width(10)
                box1.pack_start(box2, True, True, 0)
                box2.show()

                # get current IIR parameters: IIR_delay and IIR_gain
                fmt = ">hhhhhhhhhhhhhhhhhh"
                sr = open (sr_dev_path, "rb")
                sr.seek (iir_address, 1)
                delay   = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
                gain0   = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
                gain12  = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
                sr.close ()

                delay = map (delay_inv, delay)

                print ("IIRadr:", iir_address)
                print ("Delay:", delay)
                print ("Gain-In:", gain0)
                print ("Gain-12:", gain12)
                
                hbox = Gtk.HBox(spacing=10)
                box2.pack_start(hbox, True, True, 0)
                hbox.show()

                lab = Gtk.Label( label="Tap Delay [ms]")
                hbox.pack_start(lab, True, True, 0)
                lab.show()
                lab = Gtk.Label( label="TapIn Gain")
                hbox.pack_start(lab, True, True, 0)
                lab.show()
                lab = Gtk.Label( label="Tap12 Gain")
                hbox.pack_start(lab, True, True, 0)
                lab.show()

                delay_max = delaylinelength/22.000 # ms (0...delaylinelength) / 22.000kHz
                delay_fac = 22.000
                gain_max  = 1.02 # 0...32767 / Q15
                gain_fac  = 32767.0

                # create scales for delay and gain for all taps...
                for tap in range(0,18):
                        hbox = Gtk.HBox(spacing=10)
                        box2.pack_start(hbox, True, True, 0)
                        hbox.show()
                        # "value", "lower", "upper", "step_increment", "page_increment", "page_size"
                        adjustment = Gtk.Adjustment(value=delay[tap]/delay_fac, lower=0, upper=delay_max, step_increment=0.1, page_increment=1, page_size=1)
                        scale = Gtk.HScale( adjustment=adjustment)
                        scale.set_size_request(150, 40)
                        scale.set_update_policy(Gtk.UPDATE_DELAYED)
                        scale.set_digits(1)
                        scale.set_draw_value(True)
                        hbox.pack_start(scale, True, True, 0)
                        scale.show()
                        adjustment.set_data("iir_param_address", iir_address+tap) 
                        adjustment.set_data("iir_param_multiplier", delay_fac) 
                        adjustment.connect("value-changed", iir_delay_param_update)
                        
                        adjustment = Gtk.Adjustment(value=gain0[tap]/gain_fac, lower=-gain_max, upper=gain_max, step_increment=0.0001, page_increment=0.02, page_size=0.02)
                        scale = Gtk.HScale( adjustment=adjustment)
                        scale.set_size_request(250, 40)
                        scale.set_update_policy(Gtk.UPDATE_DELAYED)
                        scale.set_digits(5)
                        scale.set_draw_value(True)
                        hbox.pack_start(scale, True, True, 0)
                        scale.show()
                        adjustment.set_data("iir_param_address", iir_address+tap+18) 
                        adjustment.set_data("iir_param_multiplier", gain_fac) 
                        adjustment.connect("value-changed", iir_gain_param_update)

                        adjustment = Gtk.Adjustment(value=gain12[tap]/gain_fac, lower=-gain_max, upper=gain_max, step_increment=0.0001, page_increment=0.02, page_size=0.02)
                        scale = Gtk.HScale( adjustment=adjustment)
                        scale.set_size_request(250, 40)
                        scale.set_update_policy(Gtk.UPDATE_DELAYED)
                        scale.set_digits(5)
                        scale.set_draw_value(True)
                        hbox.pack_start(scale, True, True, 0)
                        scale.show()
                        adjustment.set_data("iir_param_address", iir_address+tap+18+18) 
                        adjustment.set_data("iir_param_multiplier", gain_fac) 
                        adjustment.connect("value-changed", iir_gain_param_update)

                separator = Gtk.HSeparator()
                box1.pack_start(separator, False, True, 0)
                separator.show()

                box2 = Gtk.VBox(spacing=10)
                box2.set_border_width(10)
                box1.pack_start(box2, False, True, 0)
                box2.show()
                
                button = Gtk.Button( label="close")
                button.connect("clicked", lambda w: win.hide())
                box2.pack_start(button, True, True, 0)
                #button.set_flags(Gtk.CAN_DEFAULT)
                #button.grab_default()
                button.show()
        wins[name].show()

def aic_dbgain_set(aic, gin, gout):
        gainreg = { -42: 15,
                    -36: 1, -30: 2, -24: 3, -18: 4,
                    -12: 5,  -9: 6,  -6: 7,  -3: 8,
                    0: 0,
                    3: 9,   6: 10,  9: 11,  12: 12,
                    18: 13, 24: 14,
        }
        param_address = magic[8]+1+3*8 + aic
#        print ("Address:", param_address)
#        print ("Value:", _adj.value)
        sr = open (sr_dev_path, "wb")
        sr.seek (param_address, 1)
        db_in  = int(gin)
        db_out = int(gout)
        print ("dB in:  ", db_in)
        print ("dB out: ", db_out)

        # adjust
        if (db_in < -12 or db_in > 12):
                db_in = int(db_in/6)*6
        else:
                db_in = int(db_in/3)*3

        if (db_out < -12 or db_out > 12):
                db_out = int(db_out/6)*6
        else:
                db_out = int(db_out/3)*3

##        _adj.get_data("aic_dbgain_in_adj").set_value (db_in)
##        _adj.get_data("aic_dbgain_out_adj").set_value (db_out)
        print ("dB in:  ", db_in)
        print ("dB out: ", db_out)
        print ("dB in:  ", db_in, gainreg[db_in]<<4)
        print ("dB out: ", db_out, gainreg[db_out])
        print ("Reg: ", gainreg[db_in]<<4 | gainreg[db_out])

        sr.write (struct.pack (">h", gainreg[db_in]<<4 | gainreg[db_out]))
        sr.close ()

        # set mode MD_AIC_RECONFIG (==8) in statemachine
        sr = open (sr_dev_path, "wb")
        sr.seek (magic[5], 1)
        sr.write (struct.pack (">h", 8))
        sr.close ()


# update one IIR param at given address
def aic_dbgain_update(_adj, param_address, dbin, dbout):
        gainreg = { -42: 15,
                    -36: 1, -30: 2, -24: 3, -18: 4,
                    -12: 5,  -9: 6,  -6: 7,  -3: 8,
                    0: 0,
                    3: 9,   6: 10,  9: 11,  12: 12,
                    18: 13, 24: 14,
        }
        #param_address = _adj.get_data("aic_dbgain_address")
#        print ("Address:", param_address)
#        print ("Value:", _adj.value)
        sr = open (sr_dev_path, "wb")
        sr.seek (param_address, 1)
        db_in  = int(dbin.get_value())
        db_out = int(dbout.get_value())
        print ("dB in:  ", db_in)
        print ("dB out: ", db_out)

        # adjust
        if (db_in < -12 or db_in > 12):
                db_in = int(db_in/6)*6
        else:
                db_in = int(db_in/3)*3

        if (db_out < -12 or db_out > 12):
                db_out = int(db_out/6)*6
        else:
                db_out = int(db_out/3)*3

##        _adj.get_data("aic_dbgain_in_adj").set_value (db_in)
##        _adj.get_data("aic_dbgain_out_adj").set_value (db_out)
#        print ("dB in:  ", db_in, gainreg[int(db_in)]<<4)
#        print ("dB out: ", db_out, gainreg[db_out])
#        print ("Reg: ", gainreg[db_in]<<4 | gainreg[db_out])

        sr.write (struct.pack (">h", gainreg[db_in]<<4 | gainreg[db_out]))
        sr.close ()

        # set mode MD_AIC_RECONFIG (==8) in statemachine
        sr = open (sr_dev_path, "wb")
        sr.seek (magic[5], 1)
        sr.write (struct.pack (">h", 8))
        sr.close ()


# create Gain edjustments
def create_gain_edit(_button, dbgain_address, name):
        if name not in wins:
                win = Gtk.Window()
                                  
                wins[name] = win
                win.connect("delete_event", delete_event)
                
                box1 = Gtk.VBox()
                win.add(box1)
                box1.show()

                box2 = Gtk.VBox()
                box2.set_border_width(10)
                box1.pack_start(box2, True, True, 0)
                box2.show()

                # get current Gain parameters
                #                sr = open (sr_dev_path, "rb")
                #                sr.seek (dbgain_address, 1)
                #                fmt = ">hhhhhhhh"
                #                dbgain  = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
                #                sr.close ()

                hbox = Gtk.HBox()
                box2.pack_start(hbox, True, True, 0)
                hbox.show()

                lab = Gtk.Label(label="In Gain [dB]")
                hbox.pack_start(lab, True, True, 0)
                lab.show()

                lab = Gtk.Label(label="Out Gain [dB]")
                hbox.pack_start(lab, True, True, 0)
                lab.show()

                aic_in_name  = ["LFE", "L", "R", "-",  "-", "-", "-", "-"  ]
                aic_in_gain  = [ +6,   1,   1,   0,    0,   0,   0,   0   ]
                aic_out_name = ["SUB", "PAS", "DEC", "-",  "Tube-L", "Tube-R", "pass-L", "pass-R" ]
                aic_out_gain = [  +2, -12, -12,  -12,   -12,   -12, -12,  -12  ]

                # create scales for delay and gain for all taps...
                for aic in range(0,8):
                        hbox = Gtk.HBox()
                        box2.pack_start(hbox, True, True, 0)
                        hbox.show()
                        
                        lab = Gtk.Label(label="AIC"+str(aic)+": "+aic_in_name[aic])
                        lab.set_size_request(70, -1)
                        hbox.pack_start(lab, True, True, 0)
                        lab.show()

                        adjustment1 = Gtk.Adjustment(value=aic_in_gain[aic], lower=-40, upper=24, step_increment=1, page_increment=3, page_size=0)
                        scale = Gtk.HScale( adjustment=adjustment1)
                        scale.set_size_request(150, 40)
                        #scale.set_update_policy(Gtk.UPDATE_DELAYED)
                        scale.set_digits(0)
                        scale.set_draw_value(True)
                        hbox.pack_start(scale, True, True, 0)
                        scale.show()
                        #adjustment1.set_data("aic_dbgain_address", dbgain_address+aic) 

                        adjustment2 = Gtk.Adjustment(value=aic_out_gain[aic], lower=-40, upper=24, step_increment=1, page_increment=3, page_size=0)
                        scale = Gtk.HScale( adjustment=adjustment2)
                        scale.set_size_request(150, 40)
                        #scale.set_update_policy(Gtk.UPDATE_DELAYED)
                        scale.set_digits(0)
                        scale.set_draw_value(True)
                        hbox.pack_start(scale, True, True, 0)
                        scale.show()
                        #adjustment2.set_data("aic_dbgain_address", dbgain_address+aic) 
                        adjustment2.connect("value-changed", aic_dbgain_update, dbgain_address+aic, adjustment1, adjustment2) 
                        adjustment1.connect("value-changed", aic_dbgain_update, dbgain_address+aic, adjustment1, adjustment2) 

                        ##adjustment1.set_data("aic_dbgain_in_adj", adjustment1) 
                        ##adjustment1.set_data("aic_dbgain_out_adj", adjustment2) 
                        ##adjustment2.set_data("aic_dbgain_in_adj", adjustment1) 
                        ##adjustment2.set_data("aic_dbgain_out_adj", adjustment2) 

                        lab = Gtk.Label( label=aic_out_name[aic])
                        lab.set_size_request(70, -1)
                        hbox.pack_start(lab, True, True, 0)
                        lab.show()


                separator = Gtk.HSeparator()
                box1.pack_start(separator, False, True, 0)
                separator.show()

                box2 = Gtk.VBox(spacing=10)
                box2.set_border_width(10)
                box1.pack_start(box2, False, True, 0)
                box2.show()
                
                button = Gtk.Button( label="close")
                button.connect("clicked", lambda w: win.hide())
                box2.pack_start(button, True, True, 0)
                #button.set_flags(Gtk.CAN_DEFAULT)
                #button.grab_default()
                button.show()
        wins[name].show()

def create_vu_meter_for(_button, data_address, name):
        if name not in wins:
                win = Gtk.Window(title=name)
                wins[name] = win
                win.connect("delete_event", delete_event)
                
                box1 = Gtk.VBox()
                win.add(box1)
                box1.show()
                
                v1 = Gtk.VBox()
                v1.show()
                c1 = Instrument( Gtk.Label(label='In'), v1, "VU", "LFE In", widget_scale=0.75)
              
                v2 = Gtk.VBox()
                v2.show()
                c2 = Instrument( Gtk.Label(label='Out'), v2, "VU", "LFE out", widget_scale=0.75)
                box1.pack_start (v1, True, True, 0)
                box1.pack_start (v2, True, True, 0)

                
                def update_meter(vu1set, vu2set, data_address=data_address, count=[0]):
                        # get current Gain parameters
                        sr = open (sr_dev_path, "rb")
                        sr.seek (data_address, 1)
                        fmt = ">hhhh"
                        data = struct.unpack (fmt, sr.read (struct.calcsize (fmt)))
                        sr.close ()
                        count[0] = count[0] + 1
#                        print (data)
                        db = 2*array(data)+1
                        for i in range(0,4):
                            db[i] = -96.33+20.*math.log10( math.fabs (db[i]))
#                        print (data)
#                        print (db)
                        vu1set (db[0], db[1])
                        vu2set (db[2], db[3])
                        return True

                def reset_meter(_button, data_address=data_address):
                        sr = open (sr_dev_path, "wb")
                        sr.seek (data_address, 1)
                        sr.write (struct.pack (">hhhh", 0,0, 0,0))
                        sr.close ()
                        return True

                GLib.timeout_add (77, update_meter, c1.set_reading_vu, c2.set_reading_vu)
                GLib.timeout_add (1000, reset_meter, 0)

                box2 = Gtk.VBox()
                box2.set_border_width(10)
                box1.pack_start(box2, False, True, 0)
                box2.show()
                
                button = Gtk.Button(label="reset")
                button.connect("clicked", reset_meter)
                box2.pack_start(button, True, True, 0)
                button.show()
                
        wins[name].show()
        
def create_vu_meter(_button):
        create_vu_meter_for(_button, magic[18], "VU in-X-peak out-X-peak")

def create_vu_meter_ltr(_button):
        create_vu_meter_for(_button, magic[18]+4, "VU in-L / tube-tr-L peak")

def AIC_in_offset_update(_adj, param_address):
        #param_address = _adj.get_data("AIC_offset_address")
        param = int (_adj.value*3.2767)
        sr = open (sr_dev_path, "wb")
        sr.seek (param_address, 1)
        sr.write (struct.pack (">h", param))
        sr.close ()

def AIC_out_offset_update(_adj, param_address):
        #param_address = _adj.get_data("AIC_offset_address")
        param = int (_adj.value*3.2767*5)
        sr = open (sr_dev_path, "wb")
        sr.seek (param_address, 1)
        sr.write (struct.pack (">h", param))
        sr.close ()
        
def force_offset_recalibration (w, mask):
        toggle_flag(w, 0x100)
        
# updates the right side of the offset dialog
def offset_update(_labin,_labout,_adj,_tap):
        global analog_offset
        global AIC_in_buffer
        global forceupdate
        gainfac0db  = 1.0151 # -0.1038V Offset
        gainfac6db  = 2.0217 # -0.1970
        gainfac12db = 4.0424 # -0.3845
        gainfac18db = 8.0910 # -0.7620
        level_fac = 0.305185095 # mV/Bit
        diff=(AIC_in_buffer[_tap])*level_fac
        outmon=(AIC_in_buffer[8+_tap])

        _labin("%.1f mV" %diff)
        _labout("%.1f mV" %outmon)
# 4.988
#        _labin("%.1f mV %10.3f mV %10.3f mV %10.3f um" %(diff,slowdiff/gainfac0db,AIC_slow[_tap], (AIC_slow[_tap]/LVDT_V2umX)))
                
        if (forceupdate >> _tap) & 1:
                _adj(analog_offset[_tap]*level_fac)
                forceupdate = forceupdate & (255 - (1 << _tap))
        return 1

# build offset editor dialog
def create_offset_edit(_button):
        global analog_offset
        global forceupdate
        name="AIC Offset Control" + "[" + sr_dev_path + "]"
        address=magic[10]+16
        if name not in wins:
                win = Gtk.Window()
                wins[name] = win
                win.connect("delete_event", delete_event)
                
                box1 = Gtk.VBox()
                win.add(box1)

                box2 = Gtk.VBox(spacing=0)
                box2.set_border_width(10)
                box1.pack_start(box2, True, True, 0)

                table = Gtk.Table (n_rows=6, n_columns=17)
                box2.pack_start(table, False, False, 0)
     
                lab = Gtk.Label( label="AIC-in Offset Level [mV]")
                table.attach (lab, 1, 2, 0, 1)

                lab = Gtk.Label( label="Reading")
                lab.set_alignment(1.0, 0.5)
                table.attach (lab, 2, 3, 0, 1) #, Gtk.FILL | Gtk.EXPAND )

                lab = Gtk.Label( label="AIC-out Offset Level [mV]")
                table.attach (lab, 5, 6, 0, 1)

                lab = Gtk.Label( label="Reading")
                lab.set_alignment(1.0, 0.5)
                table.attach (lab, 6, 7, 0, 1) #, Gtk.FILL | Gtk.EXPAND )

                level_max = 10000. # mV
                level_fac = 0.305185095 # mV/Bit
                forceupdate = 255

                # create scales for level and gain for all taps...
                for tap in range(0,8):
                        adjustment = Gtk.Adjustment (value=analog_offset[tap]*level_fac, lower=-level_max, upper=level_max, step_incr=0.1, page_incr=2, page_size=50)
                        #adjustment.set_data("AIC_offset_address", address+tap) 
                        adjustment.connect("value-changed", AIC_in_offset_update, address+tap) 

                        scale = Gtk.HScale( adjustment=adjustment)
                        scale.set_size_request(250, 30)
                        #scale.set_update_policy(Gtk.UPDATE_DELAYED)
                        scale.set_digits(1)
                        scale.set_draw_value(True)

                        lab = Gtk.Label( label="AIC_"+str(tap))
                        table.attach (lab, 0, 1, tap+1, tap+2)
                        table.attach (scale, 1, 2, tap+1, tap+2)
                        
                        labin = Gtk.Label( label="+####.# mV")
                        labin.set_alignment(1.0, 0.5)
                        table.attach (labin, 2, 3, tap+1, tap+2) #, Gtk.FILL | Gtk.EXPAND )

                        labout = Gtk.Label( label="+####.# mV")
                        labout.set_alignment(1.0, 0.5)
                        table.attach (labout, 6, 7, tap+1, tap+2) #, Gtk.FILL | Gtk.EXPAND )

                        GLib.timeout_add(updaterate, offset_update, labin.set_text, labout.set_text, adjustment.set_value, tap)
                                                

                separator = Gtk.HSeparator()
                box1.pack_start(separator, False, True, 0)
                separator.show()

                check_button = Gtk.CheckButton("Offset Compensation (automatic offset compensation \nneeds Zero volt or open inputs at DSP startup!)")
                if SPM_STATEMACHINE[2] & 0x0010:
                        check_button.set_active(True)
                check_button.connect('toggled', toggle_flag, 0x0010)
                GLib.timeout_add (updaterate, check_update, check_button.set_active, 0x0010)
                box1.pack_start(check_button, True, True, 0)

                box2 = Gtk.HBox()
                box2.set_border_width(10)
                box1.pack_start(box2, False, True, 0)

                button = Gtk.Button(label="AIC-in Auto Offset")
                button.connect("clicked", force_offset_recalibration, 0)
                box2.pack_start(button, True, True, 0)
                
                separator = Gtk.HSeparator()
                box1.pack_start(separator, False, True, 0)

                box2 = Gtk.VBox()
                box2.set_border_width(10)
                box1.pack_start(box2, False, True, 0)
                
                button = Gtk.Button.new_with_mnemonic('close')
                button.connect("clicked", lambda w: win.hide())
                box2.pack_start(button, True, True, 0)
                #button.set_flags(Gtk.CAN_DEFAULT)
                #button.grab_default()

        wins[name].show_all()

def check_update(_set, _mask):
        if SPM_STATEMACHINE[2] & _mask:
                _set (True)
        else:
                _set (False)
        return 1

def iir_control(_object, _value, _identifier):
        value = 0
        if _identifier=="iir_biquad1":
                value = _object.get_value()
                set_iir_gains (calc_iir_biquad(value,8))
        return 1

# create SR DSP settings dialog
def create_settings_edit(_button):
        name = "SR DSP LFE processing Settings"
        if name not in wins:
                win = Gtk.Window(title=name)
                wins[name] = win
                win.connect("delete_event", delete_event)
                

                box1 = Gtk.VBox(spacing=5)
                win.add(box1)
                box1.show()

                box2 = Gtk.VBox(spacing=5)
                box2.set_border_width(10)
                box1.pack_start(box2, True, True, 0)
                box2.show()

                hbox = Gtk.HBox()
                box2.pack_start(hbox, True, True, 0)
                hbox.show()

                lab = Gtk.Label(label="* * * State Flags [state.mode] * * *\n -->  watching every %d ms" %updaterate)
                hbox.pack_start(lab, True, True, 0)
                lab.show()

                separator = Gtk.HSeparator()
                box2.pack_start(separator, False, True, 0)
                separator.show()

                hbox = Gtk.VBox()
                box2.pack_start(hbox, True, True, 0)
                hbox.show()

                box2 = Gtk.VBox(spacing=10)
                box2.set_border_width(10)
                box1.pack_start(box2, False, True, 0)
                box2.show()
                
                box3 = Gtk.HBox()
                hbox.pack_start(box3, True, True, 0)
                box3.show()                

                adj = Gtk.Adjustment(75, 10, 500., 1., 10., 0)
                fspinner = Gtk.SpinButton()
                fspinner.set_adjustment(adj)
                fspinner.set_digits(1)
                fspinner.set_snap_to_ticks(True)
                fspinner.set_wrap(False)
                fspinner.set_numeric(True)
                #fspinner.set_update_policy(Gtk.UPDATE_IF_VALID)
                fspinner.connect('value-changed', iir_control, 0, "iir_biquad1")
                fspinner.show()

                ulabel = Gtk.Label(label="Hz")
                ulabel.show()

                check_button = Gtk.CheckButton("BiQuad 1 [0x1000] LP")
                check_button.set_active(False)
                GLib.timeout_add (updaterate, check_update, check_button.set_active, 0x1000)
                check_button.connect('toggled', toggle_flag, 0x1000)
                check_button.show()
                box3.pack_start(check_button, True, True, 0)
                box3.pack_start(fspinner, True, True, 0)
                box3.pack_start(ulabel, True, True, 0)

                check_button = Gtk.CheckButton(label="IIR 32bit processing [0x0080]")
                check_button.set_active(False)
                GLib.timeout_add (updaterate, check_update, check_button.set_active, 0x0080)
                check_button.connect('toggled', toggle_flag, 0x0080)
                check_button.show()
                hbox.pack_start(check_button, True, True, 0)

                check_button = Gtk.CheckButton(label="Mix L/R with LFE [0x0200]")
                check_button.set_active(False)
                GLib.timeout_add (updaterate, check_update, check_button.set_active, 0x0200)
                check_button.connect('toggled', toggle_flag, 0x0200)
                check_button.show()
                hbox.pack_start(check_button, True, True, 0)

                check_button = Gtk.CheckButton(label="Auto LFE Offsetcomp  [0x0400]")
                check_button.set_active(False)
                GLib.timeout_add (updaterate, check_update, check_button.set_active, 0x0400)
                check_button.connect('toggled', toggle_flag, 0x0400)
                check_button.show()
                hbox.pack_start(check_button, True, True, 0)

                check_button = Gtk.CheckButton(label="L/R tube transfer -> 5,6 [0x0800]")
                check_button.set_active(False)
                GLib.timeout_add (updaterate, check_update, check_button.set_active, 0x0800)
                check_button.connect('toggled', toggle_flag, 0x0800)
                check_button.show()
                hbox.pack_start(check_button, True, True, 0)

                check_button = Gtk.CheckButton(label="Pause Processing [0x0020]")
                check_button.set_active(False)
                GLib.timeout_add (updaterate, check_update, check_button.set_active, 0x0020)
                check_button.connect('toggled', toggle_flag, 0x0020)
                check_button.show()
                hbox.pack_start(check_button, True, True, 0)

                check_button = Gtk.CheckButton(label="Recalib Offsets [0x0100] (must be in pause)")
                check_button.set_active(False)
                GLib.timeout_add (updaterate, check_update, check_button.set_active, 0x0100)
                check_button.connect('toggled', toggle_flag, 0x0100)
                check_button.show()
                hbox.pack_start(check_button, True, True, 0)

                check_button = Gtk.CheckButton(label="Decimate In0 [0x0040])")
                check_button.set_active(False)
                GLib.timeout_add (updaterate, check_update, check_button.set_active, 0x040)
                check_button.connect('toggled', toggle_flag, 0x0040)
                check_button.show()
                hbox.pack_start(check_button, True, True, 0)

                check_button = Gtk.CheckButton(label="Offset Compensation [0x0010]\n (automatic offset compensation, needs\n Zero volt or open inputs at DSP startup!)")
                if SPM_STATEMACHINE[2] & 0x0010:
                        check_button.set_active(True)
                check_button.connect('toggled', toggle_flag, 0x0010)
                GLib.timeout_add (updaterate, check_update, check_button.set_active, 0x0010)
                check_button.show()
                hbox.pack_start(check_button, True, True, 0)

                box2 = Gtk.VBox(spacing=10)
                box2.set_border_width(10)
                box1.pack_start(box2, False, True, 0)
                box2.show()
                
                button = Gtk.Button.new_with_mnemonic('Close')
                button.connect("clicked", lambda w: win.hide())
                box2.pack_start(button, True, True, 0)
                button.show()
        wins[name].show()


def do_exit(button):
        Gtk.main_quit()

def destroy(*args):
        Gtk.main_quit()

        
def create_main_window():
        buttons = {
                " IIR Settings": create_settings_edit,
                "AIC Gains": create_gain_edit_out,
                "Zero Buffers": zero_buffers,
                "IIR stop": set_iir_stop,
                "IIR pass": set_iir_pass,
                "IIR LP1 50Hz": set_iir_low50,
                "IIR LP1 60Hz": set_iir_low60,
                "IIR LP1 75Hz": set_iir_low75,
                "IIR LP-BiQ 75Hz": set_iir_lowbq75,
                "VU Meter": create_vu_meter,
                "VU Meter LTr": create_vu_meter_ltr,
                "Read IIR parameters+Test": read_iir,
                "Read delaylines": read_delaylines,
                "Read subbbuffers": read_subbuffers,
                "Read subbbuffers 2": SignalScope,
                "AIC Offset Edit": create_offset_edit,
                "Tube Transfer Read": read_tubetransfer,
                "Tube Transfer a x20 Write": write_tubetransfer20,  
                "Tube Transfer b x10 Write": write_tubetransfer10,  
                "Tube Transfer c x5 Write": write_tubetransfer5,
                "Tube Transfer d x1 Write": write_tubetransfer1,
                "Tube Transfer Test": tubetransfer_test,
         }
        win = Gtk.Window()
        win.set_name("main window")
        win.set_size_request(250, 520)
        win.connect("destroy", destroy)
        win.connect("delete_event", destroy)

        box1 = Gtk.VBox()
        win.add(box1)
        box1.show()
        scrolled_window = Gtk.ScrolledWindow()
        scrolled_window.set_border_width(10)
        box1.pack_start(scrolled_window, True, True, 0)
        scrolled_window.show()
        box2 = Gtk.VBox()
        box2.set_border_width(5)
        scrolled_window.add(box2)
        box2.show()
        k = buttons.keys()
        #k.sort()
        for i in k:
                button = Gtk.Button(label=i)

                if buttons[i]:
                        button.connect("clicked", buttons[i])
                else:
                        button.set_sensitive(False)
                box2.pack_start(button, True, True, 0)
                button.show()

        separator = Gtk.HSeparator()
        box1.pack_start(separator, False, True, 0)
        separator.show()
        box2 = Gtk.VBox(spacing=10)
        box2.set_border_width(5)
        box1.pack_start(box2, False, True, 0)
        box2.show()
        button = Gtk.Button(label="close")
        button.connect("clicked", do_exit)
        #button.set_flags(Gtk.CAN_DEFAULT)
        box2.pack_start(button, True, True, 0)
        #button.grab_default()
        button.show()
        win.show()

def configure_1():
#        aic_dbgain_set(0, 10, -24)
#        aic_dbgain_set(1, 9, -12)
#        aic_dbgain_set(2, 9, -12)
        aic_dbgain_set(0, 6, 2)
        aic_dbgain_set(1, 1, -12)
        aic_dbgain_set(2, 1, -12)
        set_flag (0x0200)
        set_flag (0x0040)
        set_flag (0x0100)
        set_flag (0x0020)
        return 0

def configure_2():
        set_flag (0x0010)
        set_flag (0x0040)
        set_flag (0x0200)
        set_iir_low60 (0)
        aic_dbgain_set(0, 10, 8)
        return 0
        
magic = open_sranger()
if magic:
        print ("Magic:", magic)
        get_status()
        create_main_window()
        GLib.timeout_add(2000, configure_1)
        GLib.timeout_add(4000, configure_2)

        GLib.timeout_add(updaterate, get_status)
        Gtk.main()
        print ("Byby.")



