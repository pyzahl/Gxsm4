#!/usr/bin/env python3

### #!/usr/bin/env python

## * Python User Control for
## * Createc HV5
## * 
## * Copyright (C) 2013 Percy Zahl
## *
## * Author: Percy Zahl 
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

import gi
gi.require_version('Gtk', '4.0')

from gi.repository import Gtk, Gdk, GLib

import cairo
import os                # use os because python IO is bugy
import serial

import time
import fcntl
from threading import Timer

#import GtkExtra
import struct
import array

import math

import requests
import numpy as np

from meterwidget import *

import gxsm4process as gxsm4

METER_SCALE = 0.66

FALSE = 0
TRUE  = -1


gxsm = gxsm4.gxsm_process()



##########################
### ANDON CONTROL UTIL 
##########################

#stty -F /dev/tty 9600 cs8 -cstopb -parenb
#echo -e '\xA0\x01\x00\xA1' >/dev/ttyUSB0
#A0 01 01 A2 	Red light on
#A0 01 01 B3 	Red light flashing
#A0 01 00 A1 	Red light off
#A0 03 01 A4 	Yellow light on
#A0 03 01 B5 	Yellow light flashing
#A0 03 00 A3 	Yellow light off
#A0 02 01 A3 	Green light on
#A0 02 01 B4 	Green light flashing
#A0 02 00 A2 	Green light off
#A0 04 01 A5 	Turn the beep louder
#A0 04 01 B6 	The buzzer sounds loud and intermittently
#A0 04 01 C7 	Turn on the buzzer sound
#A0 04 01 D8 	The beeping sound is on and off
#A0 04 00 A4 	Beep sound off

class Andon():
        def __init__(self, ip):
                self.ready = False
                self.andon = {
                    'R1' : [bytes([ 0xA0, 0x01, 0x01, 0xA2 ]), 'Red light on' ],
                    'RF' : [bytes([ 0xA0, 0x01, 0x01, 0xB3 ]), 'Red light flashing' ],
                    'R0' : [bytes([ 0xA0, 0x01, 0x00, 0xA1 ]), 'Red light off' ],
                    'Y1' : [bytes([ 0xA0, 0x03, 0x01, 0xA4 ]), 'Yellow light on' ],
                    'YF' : [bytes([ 0xA0, 0x03, 0x01, 0xB5 ]), 'Yellow light flashing' ],
                    'Y0' : [bytes([ 0xA0, 0x03, 0x00, 0xA3 ]), 'Yellow light off' ],
                    'G1' : [bytes([ 0xA0, 0x02, 0x01, 0xA3 ]), 'Green light on' ],
                    'GF' : [bytes([ 0xA0, 0x02, 0x01, 0xB4 ]), 'Green light flashing' ],
                    'G0' : [bytes([ 0xA0, 0x02, 0x00, 0xA2 ]), 'Green light off' ],
                    'B+' : [bytes([ 0xA0, 0x04, 0x01, 0xA5 ]), 'Turn the beep louder' ],
                    'BB' : [bytes([ 0xA0, 0x04, 0x01, 0xB6 ]), 'The buzzer sounds loud and intermittently' ],
                    'B1' : [bytes([ 0xA0, 0x04, 0x01, 0xC7 ]), 'Turn on the buzzer sound' ],
                    'BF' : [bytes([ 0xA0, 0x04, 0x01, 0xD8 ]), 'The beeping sound is on and off' ],
                    'B0' : [bytes([ 0xA0, 0x04, 0x00, 0xA4 ]), 'Beep sound off' ],
                }

        def set_andon(cmd):
            try:
                ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)
                print ('Sending: ', self.andon[cmd][1])
                ser.write(self.andon[cmd][0])
                time.sleep(0.5)
                ser.close ()
            except:
                print ('USB/TTY Error')


        def save():
            set_andon ('BF')
            time.sleep(1)
            set_andon ('Y1')
            time.sleep(1)
            set_andon ('RF')
            time.sleep(1)
            set_andon ('R0')
            time.sleep(1)
            set_andon ('G1')
            time.sleep(1)
            set_andon ('R0')
            time.sleep(1)
            set_andon ('Y0')
            time.sleep(1)
            set_andon ('B0')


        def engage():
            set_andon ('BF')
            time.sleep(1)
            set_andon ('GF')
            time.sleep(1)
            set_andon ('Y1')
            set_andon ('G0')
            time.sleep(1)
            set_andon ('R1')
            time.sleep(1)
            set_andon ('Y0')
            time.sleep(1)
            set_andon ('G0')
            time.sleep(1)
            set_andon ('B0')


        def undefined():
            set_andon ('BF')
            time.sleep(1)
            set_andon ('Y1')
            time.sleep(1)
            set_andon ('RF')
            time.sleep(1)
            set_andon ('R0')
            time.sleep(1)
            set_andon ('GF')
            time.sleep(1)
            set_andon ('R0')
            time.sleep(1)
            set_andon ('YF')
            time.sleep(1)
            set_andon ('B0')

        def check_SPM_status():
                xyz = gxsm.rt_query_xyz()
                
                status=0
                while ready:
                    time.sleep(5)
                    print (xyz)

                    # -5 all up
                    if xyz['monitor'][2] > -4 and status != 1:
                        status = 1
                        print ("STM tip in range")
                        engage ()

                    if xyz['monitor'][2] < -4 and status != 2:
                        status = 2
                        print ("STM tip out of range")
                        save ()        

        
#####################################






# Createc HV Control

SWAP_CXY=1

global rpspmc

rpspmc = {
        'bias': 0.0,
        'current': 0.0,
        'gvp' : { 'x':0.0, 'y':0.0, 'z': 0.0, 'u': 0.0, 'a': 0.0, 'b': 0.0, 'am':0.0, 'fm':0.0 },
        'pac' : { 'dds_freq': 0.0, 'ampl': 0.0, 'exec':0.0, 'phase': 0.0, 'freq': 0.0, 'dfreq': 0.0, 'dfreq_ctrl': 0.0 },
        'time' : 0.0
        }

global CHV5_configuration
global CHV5_driftcomp
global CHV5_monitor
global CHV5_coarse
global CHV5_gain_list
global CHV5_gains

CHV5_configuration = {
        'gain':  [2,2,3],
        'filter': [0,0,0],
        'bw': [0,0,0],
        'target': [0.,0.,0.]
}

CHV5_monitor = {
        'monitor': [0.0,0.0,0.0],
        'monitor_min': [0.0,0.0,0.0],
        'monitor_max': [0.0,0.0,0.0],
        }

CHV5_gain_list = [3,6,12,24]
CHV5_gains = [12., 12., 24.]


CHV5_coarse = {
        'steps': [10,10,5],
        'volts': [100.0,100.0,75.0],
        'period': [500,500,500]
        }

CHV5_driftcomp = [ 0., 0., 0. ]

DIGVfacM = 1. #200./32767.
DIGVfacO = 1. #181.81818/32767.

scaleM = [ DIGVfacM, DIGVfacM, DIGVfacM ]
scaleO = [ DIGVfacO, DIGVfacO, DIGVfacO ]
unit  = [ "V", "V", "V", "dB" ]

updaterate = 88        # update time watching the SRanger in ms

GXSM_Link_status = FALSE

setup_list = []
control_list = []

               
#    // Sets HV gain parameters and filter settings
#    // @param gainx,gainy,gainz - Gain values for each axis (1-255)
#    // @param filter - Enable/disable primary filter XY
#    // @param filter2nd - Enable/disable secondary filter Z
#    // @param coarseboard - Board number for coarse voltage control
#    // @param coarsevoltage - Voltage setting for coarse control (0-255)
#    // @return 0 if successful
#    function SetHVGain(gainx, gainy, gainz: Integer; filter, filter2nd: Boolean;
#      coarseboard, coarsevoltage: Integer): Integer;
#
#    // Controls coarse movement for specified channel
#    // @param channel - Channel number (1-3 for X,Y,Z)
#    // @param direction - Movement direction (-1 or 1)
#    // @param burstcount - Number of steps to move
#    // @param period - Time period between steps
#    // @param start - Start flag (1 to start movement)
#    // @return 0 if successful
#    function CoarseMove(channel, direction, burstcount, period, start: Integer): Integer;

class THV5():
        def __init__(self, ip):
                self.besocke = False
                self.ip = ip

        def request_stmafm(self, data):
                #idhttp1.Get(ipstring + 'stmafm?' + command, ResponseStream);
                print (self.ip + 'stmafm?' + data)
                #return requests.get(self.ip + 'stmafm?' + data)

        def request(self, data):
                #idhttp1.Get(ipstring + 'stmafm?' + command, ResponseStream);
                print (self.ip + data)
                return requests.get(self.ip + data)

        def myip(self):
                return self.ip

        def request_coarse(self, data):
                #idhttp1.Get(ipstring + 'stmafm?' + command, ResponseStream);
                print (self.ip + 'coarse?' + data)
                #return requests.get(self.ip + 'stmafm?' + data)

                #http://192.168.40.10/coarse?v0=150&p0=500&a0=Z&c0=33&bs=0

        #http://192.168.40.10/gain?g0=12&l0=ON&e0=OFF&g1=12&l1=ON&e1=OFF&g2=12&l2=ON&e2=ON&g3=3&l3=OFF&e3=OFF
        # X 12x, lowpass on, endfilter off
        # Y 12x, lowpass on, endfilter off
        # Z 12x, lowpass on, endfilter on
        # AUX 3 off off
                
        # gain: 3,6,12,24  filters: 'ON','OFF' 
        def SetTHVGain(self, gainx, gainy, gainz):
                return self.request ('gain?gain?g0={}&l0=ON&e0=OFF&g1={}&l1=ON&e1=OFF&g2={}&l2=ON&e2=ON&g3=3&l3=OFF&e3=OFF'.format(gainx, gainy, gainz))
                #return self.request ('gain?f1={}&f2={}&g0={}&g1=&g2={}&v{}={}'.format(filter, filter2nd, gainx, gainy, gainz, coarseboard, coarsevoltage))


        # http://192.168.40.10/coarse?v0=150&p0=500&a0=Z&c0=10000
        # http://192.168.40.10/coarse?v0=150&p0=500&a0=Z&c0=33&bs=0     150V 500 us Z 33 steps   START
        # http://192.168.40.10/coarse?v0=150&p0=500&a0=X&c0=33&bs=0
        # http://192.168.40.10/coarse?v0=150&p0=500&a0=Y&c0=33&bs=0
        #
        # http://192.168.40.10/coarse?v0=150&p0=500&a0=Y&c0=33          SAVE
        #
        # 
        
        def SetTHVCoarseMove(self, channel, direction, burstcount, period, voltage, start):
                c=['X','Y','Z']
                ## http://192.168.40.10/coarse?v0=15&p0=500&a0=Z&c0=1&bs=0
                if start:
                        return self.request ('coarse?v={}&p0={}&a0={}&c0={}&bs=0'.format(int(voltage), int(period), c[channel], burstcount*direction))
                else:
                        return self.request ('coarse?v={}&p0={}&a0={}&c0={}&bs=0'.format(0, 0, c[channel], 0))

        #return self.request ('k0={}&c0={}&p0={}&bb=0'.format(channel, burstcount*direction, period))
        #return self.request ('k0={}&c0={}&p0={}'.format(channel, burstcount*direction, period))

        # Prizm
        #if start:
        #        return self.request ('a0={}&c0={}&p0={}&bs=0'.format(channel, burstcount*direction, period))
        #else:
        #        return self.request ('a0={}&c0={}&p0={}'.format(channel, burstcount*direction, period))

        def THVReset (self):
                return self.request ('fd=1')


        def THVStatus (self):
                settings = self.request ('')
                print (settings)
                return settings.split('&')
        
                        
class gxsm_link():
        def __init__(self, oc):
                print ("Gxsm Link Init.")

        def offset_adjust (self, start, count):
                global Z0_invert_status
                ticks = time.time()
                if cound >= 0:
                        t = Timer(self.interval - (ticks-start-count*self.interval), self.offset_adjust, [start, count+1])
                        t.start()
                v = [0.,0.,0.]
                print ("Number of ticks since 12:00am, January 1, 1970:", ticks, " #", count, v)
            
        def start (self):
                self.count = 0
                self.interval = 0.100 # interval in sec
                self.t = Timer(self.interval, self.offset_adjust, (time.time(), 0)) # start over at full second, for testing only
                self.t.start()
        
        def stop (self):
                self.count = -1

        def enable_gain_control (self):
                self.gxsm_gain_control_link = True;

        def disable_gain_control (self):
                self.gxsm_gain_control_link = False;

                
        def status (self):
                return 0



def delete_event(win, event=None):
        global GXSM_Link_status
        print ("Link: ", GXSM_Link_status)
        if GXSM_Link_status:
                print ("GXSM Link is active, please disable Link to quit")
                idlg = Gtk.MessageDialog(
                        flags=0,
                        message_type=Gtk.MessageType.INFO,
                        buttons=Gtk.ButtonsType.OK,
                        text="Warning: GXSM Link is active!\nLink will go done on SPD Control termination.")
                idlg.run()
                idlg.destroy()
                return False
        else:
                win.hide()
                Gtk.main_quit()
        return True

def make_menu_item(name, _object, callback, value, identifier):
        item = Gtk.MenuItem(name)
        item.connect("activate", callback, value, identifier)
        item.show()
        return item

def read_back():
        global CHV5_configuration
        
def _X_write_back_():
        global CHV5_configuration

## show only current
global QPAmpl
QPAmpl = [0.,0.,0.]
def update_CHV5_monitor(_c1set, _c2set, _c3set, _cqp):
        global CHV5_configuration
        global CHV5_monitor
        global rpspmc
        global QPAmpl

        _c1set (scaleM[0] * CHV5_monitor['monitor'][0], scaleM[0] * CHV5_monitor['monitor_min'][0], scaleM[0] * CHV5_monitor['monitor_max'][0])
        _c2set (scaleM[1] * CHV5_monitor['monitor'][1], scaleM[1] * CHV5_monitor['monitor_min'][1], scaleM[1] * CHV5_monitor['monitor_max'][1])
        _c3set (scaleM[2] * CHV5_monitor['monitor'][2], scaleM[2] * CHV5_monitor['monitor_min'][2], scaleM[2] * CHV5_monitor['monitor_max'][2])

        ## print (rpspmc)
        QPAmpl[0] = -100+10*rpspmc['pac']['ampl']
        if QPAmpl[0] > 0.0:
                QPAmpl[0] = 20*(math.log10(rpspmc['pac']['ampl'])-1)
        #rpspmc['pac']['dfreq']rpspmc['pac']['dfreq']
        _cqp (QPAmpl[0], 5*(rpspmc['pac']['dfreq']-1))
        #_cqp (0, -10, -100)  # 0 -> 1dB   10 -> +20dB    -10 -> 0dB   -100 -> -20dB
        #mu = 0.02
        #QPAmpl[2] = QPAmpl[0] if QPAmpl[2] < QPAmpl[0] else (1-mu)*QPAmpl[2]+mu*QPAmpl[0]
        #QPAmpl[1] = QPAmpl[0] if QPAmpl[1] > QPAmpl[0] else (1-mu)*QPAmpl[2]+mu*QPAmpl[0]
        
        return 1

## show min/max at 1Hz
def update_CHV5_monitor_full(_c1set, _c2set, _c3set):
        global CHV5_configuration
        global CHV5_monitor

        spantag  = "<span size=\"24000\" font_family=\"monospace\">"   # .. color=\"#ff0000\"
        spantag2 = "<span size=\"10000\" font_family=\"monospace\">"   # .. color=\"#ff0000\"
                
        _c1set (spantag + "<b>%8.3f " %(scaleM[0]*CHV5_monitor[ii_monitor_X]) + unit[0] + "</b></span>\n"
                + spantag2
                + "%8.3f " %(scaleM[0]*CHV5_monitor[ii_monitor_Xmin]) + unit[0]
                + "%8.3f " %(scaleM[0]*CHV5_monitor[ii_monitor_Xmax]) + unit[0]
                +" </span>")
        _c2set (spantag + "<b>%8.3f " %(scaleM[1]*CHV5_monitor[ii_monitor_Y]) + unit[1] + "</b></span>\n"
                + spantag2
                + "%8.3f " %(scaleM[0]*CHV5_monitor[ii_monitor_Ymin]) + unit[0]
                + "%8.3f " %(scaleM[0]*CHV5_monitor[ii_monitor_Ymax]) + unit[0]
                +" </span>")
        _c3set (spantag + "<b>%8.3f " %(scaleM[2]*CHV5_monitor[ii_monitor_Z]) + unit[2] + "</b></span>\n"
                + spantag2
                + "%8.3f " %(scaleM[0]*CHV5_monitor[ii_monitor_Zmin]) + unit[0]
                + "%8.3f " %(scaleM[0]*CHV5_monitor[ii_monitor_Zmax]) + unit[0]
                +" </span>")
        
        return 1


def hv1_adjust(_object, _value, _identifier):
        global CHV5_gain_list
        global CHV5_gains
        value = _value
        index = _identifier
        #if index >= ii_config_preset_X0 and index < ii_config_dum:

        CHV5_gains[index] = CHV5_gain_list[value]

        print ('HV-adjust: ', index, value, ', CHV5 Gains: ', CHV5_gains, ' ... may toggle to enforce up-to-now.')

        read_back ()
        return 1


        
class offset_vector():
    def __init__(self, etv, gsv):
        self.etvec = etv
        self.gainselectmenuvec = gsv

    def update_display(self):
        for i in range (0,3):
            self.etvec[i].set_text("%12.3f" %(scaleO[i]*CHV5_configuration['target'][i]))
        return False

    def write_t_vector(self, button):
        global CHV5_configuration
        tX = [0,0,0]
        for i in range (0,3):
            tX[i] = int (float (self.etvec[i].get_text ()) / scaleO[i]);
            if tX[i] > 32766:
                    tX[i] = 32766
            if tX[i] < -32766:
                    tX[i] = -32766

#        print (tX)

        ##os.write (sr.fileno(), struct.pack ("<hhh", tX[0], tX[1], tX[2]))
        
        read_back ()
        GLib.idle_add (self.update_display)

    def write_t_vector_var(self, tvec):
        global CHV5_configuration
        tX = [0,0,0]
        for i in range (0,3):
            tX[i] = int (tvec[i] / scaleO[i]);
            if tX[i] > 32766:
                    tX[i] = 32766
            if tX[i] < -32766:
                    tX[i] = -32766
            
#        print (tX)

        ##os.write (sr.fileno(), struct.pack ("<hhh", tX[0], tX[1], tX[2]))
        
        read_back ()
        GLib.idle_add (self.update_display)

class drift_compensation():
    def __init__(self, edv, oc):
        self.count = -1
        self.edvec = edv
        self.offset_ctrl = oc

    def update (self):
        self.tvd = [0.,0.,0.] # differential offset vector (drift rate in 1 / sec)
        for i in range (0,3):
            self.tvd[i] = float (self.edvec[i].get_text ())

        read_back ()
        self.tvi = [0.,0.,0.,0.] # initial offset vector
        for i in range (0,3):
            self.tvi[i] = scaleO[i] * CHV5_configuration['target'][i]
        self.tvi[3] = time.time() # initial time

        self.offset_ctrl.write_t_vector_var(self.tvi)

    def offset_adjust (self, start, count):
        if self.count >= 0:
            self.count = count
            ticks = time.time()
            t = Timer(self.interval - (ticks-start-count*self.interval), self.offset_adjust, [start, count+1])
            t.start()
            v = [0.,0.,0.]
            for i in range (0,3):
                v[i] = self.tvi[i] + (ticks - self.tvi[3]) * self.tvd[i]
            self.offset_ctrl.write_t_vector_var(v)
#            print ("Number of ticks since 12:00am, January 1, 1970:", ticks, " #", count, v)
        
    def start (self):
        self.update ()
        self.count = 0
        self.interval = 0.05 # interval in sec
        self.t = Timer(self.interval, self.offset_adjust, (time.time(), 0)) # start over at full second, for testing only
        self.t.start()
        
    def stop (self):
        self.count = -1


def toggle_configure_widgets (w):
        print ("CONFIG STATES:")
        if w.get_active():
                print ("ACTIVE")
        if w.get_inconsistent ():
                print ("INCONSISTENT")
        if w.get_active():
                if w.get_inconsistent ():
                        w.set_inconsistent (False)
                else:
                        w.set_inconsistent (True)
        if w.get_active():
                if w.get_inconsistent ():
                        for w in control_list:
                                w.hide ()
                else:
                        for w in control_list:
                                w.show ()
                for w in setup_list:
                        w.show ()
        else:
                for w in setup_list:
                        w.hide ()
                for w in control_list:
                        w.hide ()
        #win.resize(1, 1)

                        
def toggle_driftcompensation (w, dc):
        if w.get_active():
            dc.start ()
            print ("Drift Compensation ON")
        else:
            dc.stop ()
            print ("Drift Compensation OFF")
        
def toggle_gxsm_link (w, gl):
        global GXSM_Link_status
        if w.get_active():
                gl.start ()
                GXSM_Link_status = TRUE
                print ("GXSM Link enabled")
        else:
                gl.stop ()
                GXSM_Link_status = FALSE
                print ("GXSM Link disabled")
        
def toggle_gxsm_gain_link (w, gl):
        global GXSM_Link_status
        if w.get_active():
                gl.enable_gain_control ()
        else:
                gl.disable_gain_control ()
        
def toggle_Z0_invert (w):
        global Z0_invert_status
        if w.get_active():
                Z0_invert_status = TRUE
                print ("Z0 invert ACTIVE")
        else:
                Z0_invert_status = FALSE
                print ("Z0 normal")



        
def do_emergency(button,w,gl):
        print ("Emergency Stop Action:")
        global GXSM_Link_status
        if w.get_active():
                gl.stop ()
                GXSM_Link_status = FALSE
                print ("GXSM Link enabled")
                w.set_active(False)
        #goto_presets()

def on_gain_changed(combo, ii, self):
        tree_iter = combo.get_active_iter()
        if tree_iter is not None:
                model = combo.get_model()
                g = model[tree_iter][0]
                position = combo.get_active()
                print("Gain {}: {} [{}]".format(ii,g,position))
                hv1_adjust (None, position, ii)
        self.set_gains()

# besides print label, same code:                
def on_bw_changed(combo, ii):
        tree_iter = combo.get_active_iter()
        if tree_iter is not None:
                model = combo.get_model()
                g = model[tree_iter][0]
                position = combo.get_active()
                print("BW {}: {} [{}]".format(ii,g,position))
                #hv1_adjust (None, position, ii)


def get_status():
        global CHV5_monitor
        global CHV5_gains

        global rpspmc

        XYZ_Vout = gxsm.rt_query_xyz()
        CHV5_monitor['monitor']     = XYZ_Vout['monitor']     * CHV5_gains
        CHV5_monitor['monitor_max'] = XYZ_Vout['monitor_max'] * CHV5_gains
        CHV5_monitor['monitor_min'] = XYZ_Vout['monitor_min'] * CHV5_gains
        rpspmc      = gxsm.rt_query_rpspmc()

        return 1

def do_exit(button):
        Gtk.main_quit()

def destroy(*args):
        Gtk.main_quit()

def locate_spm_control_dsp():
        print ("need to create a MK3 base support class.........")


class HV5App(Gtk.Application):
        def __init__(self, *args, **kwargs):
                print ("Init THV5 App...")
                super().__init__(*args, application_id="com.createc.hv5app", **kwargs)

                print ("THV5 ...")
                #self.thv = THV5('http://192.168.0.195/')
                #self.thv = THV5('http://130.199.243.191/') # self test
                self.thv = THV5('http://192.168.40.10/') ## Createc HV5 default in lab local network

                print ("Get Status ...")
                get_status()
                
                print ("Done.")
                self.window = None


        def start_coarse(self, button, a,b,c, dir, es, ev, ep):
                axis = 0
                steps = 0
                volts = 0
                period = 500
                d=1
                if abs(dir) > 0:
                        axis = abs(dir)-1
                        if dir < 0:
                                d=-1
                        steps = int(es[axis].get_text())
                        volts = float(ev[axis].get_text())
                        period = float(ep[axis].get_text())
                print ('do coarse START', dir, steps, volts, period)
                self.thv.SetTHVCoarseMove(axis, d, steps, period, volts, True)

                
        def stop_coarse(self, button, a, dir, es, ev, ep):
                print ('do coarse STOP', dir)

                axis = 0
                if abs(dir) > 0:
                        axis = abs(dir)-1
                self.thv.SetTHVCoarseMove(axis, 1, 0, 0, 0, False)


        def set_gains(self):
                global CHV5_gains
                self.thv.SetTHVGain(CHV5_gains[0], CHV5_gains[1], CHV5_gains[2])

        ## GXSM4 / RPSPM communication set/get + control demos
        def on_button_set_bias(self, w, bias):
                gxsm.set ('dsp-fbs-bias', float( bias.get_text () ))
                
        def on_button_get_bias(self, w, bias):
                bias.set_text('{}'.format(gxsm.get ('dsp-fbs-bias')))

        def on_button_zcontrol(self, w):
                gxsm.set_current_level_zcontrol ()
                
        def on_button_znormal(self, w):
                gxsm.set_current_level_0 () ## normal Z control
                
        def on_button_set(self, w, eid, val):
                gxsm.set (eid.get_text(), float( val.get_text () ))
                
        def on_button_get(self, w, eid, val):
                print (eid.get_text(), gxsm.get (eid.get_text()))
                val.set_text('{}'.format(gxsm.get (eid.get_text())))
    
        def do_activate(self):
                global CHV5_configuration
                global CHV5_monitor
                global CHV5_gains
                
                if not self.window:
                        print ("Build GUI...")
                        self.window = Gtk.ApplicationWindow(application=self, title="Createc THV5 @ "+self.thv.myip())
                        name = "Createc HV Control"

                        print ('do_activate: CHV5 Gains: ', CHV5_gains, ' ... may toggle to enforce up-to-now.')
                        
                        hb = Gtk.HeaderBar() 
                        #hb.set_show_close_button(True) 
                        #hb.props.title = "CHV5"
                        self.window.set_titlebar(hb) 
                        #hb.set_subtitle("Createc Piezo Drive") 
                        
                        grid = Gtk.Grid()
                        self.window.set_child (grid)

                        tr=1

                        maxvqp = 10
                        v = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
                        cqp = Instrument( Gtk.Label(), v, "VU", "QP", unit[3], widget_scale=METER_SCALE)
                        cqp.set_range(arange(0,maxvqp/10*11,maxvqp/10))
                        grid.attach(v, 0,tr, 1,1)

                        
                        maxv = 200
                        v = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
                        c1 = Instrument( Gtk.Label(), v, "Volt", "X-Axis", unit[0], widget_scale=METER_SCALE)
                        c1.set_range(arange(0,maxv/10*11,maxv/10))
                        grid.attach(v, 1,tr, 1,1)
                        
                        v = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
                        c2 = Instrument( Gtk.Label(), v, "Volt", "Y-Axis", unit[1], widget_scale=METER_SCALE)
                        c2.set_range(arange(0,maxv/10*11,maxv/10))
                        grid.attach(v, 2,tr, 1,1)
                        
                        v = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
                        c3 = Instrument( Gtk.Label(), v, "Volt", "Z-Axis", unit[2], widget_scale=METER_SCALE)
                        c3.set_range(arange(0,maxv/10*11,maxv/10))
                        grid.attach(v, 3,tr, 1,1)
                        tr=tr+1
                        
                        GLib.timeout_add (updaterate, update_CHV5_monitor, c1.set_reading_lohi, c2.set_reading_lohi, c3.set_reading_lohi, cqp.set_reading_auto_vu_extra)
                        separator = Gtk.Separator(orientation=Gtk.Orientation.HORIZONTAL)
                        grid.attach(separator, 0, tr, 5, 1)
                        tr=tr+1
                        
                        
                        # Offset Adjusts
                        tr = tr+1
                        lab = Gtk.Label(label="Offsets:")
                        control_list.append (lab)
                        grid.attach(lab, 0, tr, 1, 1)
                        
                        eo = []
                        
                        for i in range(0,3):
                                eo.append (Gtk.Entry())
                                eo[i].set_text("%12.3f" %(scaleO[i]*CHV5_configuration['target'][i]))
                                eo[i].set_width_chars(8)
                                control_list.append (eo[i])
                                grid.attach(eo[i], 1+i, tr, 1, 1)

                        oabutton = Gtk.Button() #stock='gtk-apply')
                        control_list.append (oabutton)
                        grid.attach(oabutton, 4, tr, 1, 1)
                        
                        # Drift Compensation Setup
                        tr = tr+1
                        lab = Gtk.Label(label="Drift Comp.:")
                        setup_list.append (lab)
                        grid.attach(lab, 0, tr, 1, 1)
                        
                        ed = []
                    
                        for i in range(0,3):
                                ed.append (Gtk.Entry())
                                ed[i].set_text("%12.3f " %(scaleO[i]*CHV5_driftcomp[i]) ) # + unit[i] + "/s")
                                ed[i].set_width_chars(8)
                                grid.attach(ed[i], 1+i, tr, 1, 1)
                                setup_list.append (ed[i])

                        lab = Gtk.Label(label=unit[0] + "/s")
                        setup_list.append (lab)
                        grid.attach(lab, 4, tr, 1, 1)

                        # GAINs
                        tr = tr+1
                        lab = Gtk.Label(label="Gains:")
                        control_list.append (lab)
                        grid.attach(lab, 0, tr, 1, 1)
                        
                        gain_store = Gtk.ListStore(str)
                        chan = ["gain_X", "gain_Y", "gain_Z"]
                        gain = [" 3x", " 6x", " 12x", " 24x"]
                        for g in gain:
                                gain_store.append([g])
                                gain_select = []
                        for ci in range(0,3):
                                opt = Gtk.ComboBox.new_with_model(gain_store)
                                opt.connect("changed", on_gain_changed, ci, self)
                                renderer_text = Gtk.CellRendererText()
                                opt.pack_start(renderer_text, True)
                                opt.add_attribute(renderer_text, "text", 0)
                                opt.set_active(CHV5_configuration['gain'][ci])
                                gain_select.append(opt.set_active)
                                #control_list.append (opt)
                                grid.attach(opt, 1+ci, tr, 1, 1)

                        ci=3

                        # BWs
                        tr = tr+1
                        lab = Gtk.Label(label="Bandwidth:")
                        setup_list.append (lab)
                        grid.attach(lab, 0, tr, 1, 1)
                        chan = ["bw_X", "bw_Y", "bw_Z"]
                        bw_store = Gtk.ListStore(str)
                        bw   = ["50 kHz", "10 kHz", "1 kHz"]
                        for b in bw:
                                bw_store.append([b])
                                
                        for ci in range(0,3):
                                opt = Gtk.ComboBox.new_with_model(bw_store)
                                opt.connect("changed", on_bw_changed, ci)
                                renderer_text = Gtk.CellRendererText()
                                opt.pack_start(renderer_text, True)
                                opt.add_attribute(renderer_text, "text", 0)
                                opt.set_active(CHV5_configuration['bw'][ci])
                                setup_list.append (opt)
                                grid.attach(opt, 1+ci, tr, 1, 1)

                        tr = tr+1
                        separator = Gtk.Separator(orientation=Gtk.Orientation.HORIZONTAL)
                        grid.attach(separator, 0, tr, 5, 1)
                        tr = tr+1
                        
                        # Link controls
                        
                        offset_control = offset_vector(eo, gain_select)
                        oabutton.connect("clicked", offset_control.write_t_vector)
                        dc_control = drift_compensation (ed, offset_control)
                        GxsmLink = gxsm_link(offset_control)
                        
                        # Closing ---
                        
                        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
                        #grid.attach(hbox, 0, tr, 5, 1)
                        hb.pack_end(hbox)
                        
                        cbl = check_button = Gtk.CheckButton(label="GXSM Link")
                        button = Gtk.Button.new_with_mnemonic(label='Stop')
                        button.connect("clicked", do_emergency, cbl, GxsmLink)
                        #Label=button.get_children()[0]
                        #Label=Label.get_children()[0].get_children()[1]
                        #Label=Label.set_label('STOP')
                        control_list.append (button)
                        grid.attach(button, 4, 1, 1, 1)
                        
                        cbc = check_button = Gtk.CheckButton(label="Config")
                        check_button.set_active(False)
                        check_button.connect('toggled', toggle_configure_widgets)
                        hbox.append(check_button)
                        
                        dc_check_button = Gtk.CheckButton(label="Drift Comp.")
                        dc_check_button.set_active(False)
                        dc_check_button.connect('toggled', toggle_driftcompensation, dc_control)
                        setup_list.append (dc_check_button)
                        hbox.append(dc_check_button)
                        
                        check_button=cbl
                        check_button.set_active(False)
                        check_button.connect('toggled', toggle_gxsm_link, GxsmLink)
                        hbox.append(check_button)
                        #setup_list.append (check_button)
                        check_button.set_sensitive (GxsmLink.status ())        
                        
                        check_button = Gtk.CheckButton(label="GXSM Gains")
                        check_button.set_active(False)
                        check_button.connect('toggled', toggle_gxsm_gain_link, GxsmLink)
                        hbox.append(check_button)
                        #setup_list.append (check_button)
                        check_button.set_sensitive (GxsmLink.status ())
                        
                        check_button = Gtk.CheckButton(label="Z0 inv.")
                        check_button.set_active(True)
                        check_button.connect('toggled', toggle_Z0_invert)
                        setup_list.append (check_button)
                        hbox.append(check_button)
                        check_button.set_sensitive (GxsmLink.status ())        
                        
                        toggle_configure_widgets(cbc)
                        
                        GLib.timeout_add(updaterate, get_status)        

                        #### Coarse Controller

                        grid_chv5c = Gtk.Grid()
                        grid.attach(grid_chv5c, 10, 0, 10, 5)

                        if SWAP_CXY:
                                DX=-2
                                DY=-1
                                axis = [1,0,2]
                        else:
                                DX=1
                                DY=2
                                axis = [0,1,2]

                        lxyz = ['X','Y','Z']
                                
                        es = [Gtk.Entry(), Gtk.Entry(), Gtk.Entry()] ## Entry Steps
                        ev = [Gtk.Entry(), Gtk.Entry(), Gtk.Entry()] ## Entry Volts
                        ep = [Gtk.Entry(), Gtk.Entry(), Gtk.Entry()] ## Entro Periods
                        grid_chv5c.attach(Gtk.Label(label='Ax'), 16, 3, 1, 1)
                        grid_chv5c.attach(Gtk.Label(label='# Steps'), 17, 3, 1, 1)
                        grid_chv5c.attach(Gtk.Label(label='Amplitude in V'), 18, 3, 1, 1)
                        grid_chv5c.attach(Gtk.Label(label='Period in µs'), 19, 3, 1, 1)
                        for r in range(0,3):
                                i = axis[r]
                                grid_chv5c.attach(Gtk.Label(label=lxyz[i]), 16, 5+r, 1, 1)
                                es[i].set_text("{}".format(CHV5_coarse['steps'][i]))
                                es[i].set_width_chars(8)
                                #control_list.append (eo[i])
                                grid_chv5c.attach(es[i], 17, 5+r, 1, 1)
                                ev[i].set_text("{}".format(CHV5_coarse['volts'][i]))
                                ev[i].set_width_chars(8)
                                #control_list.append (evo[i])
                                grid_chv5c.attach(ev[i], 18, 5+r, 1, 1)
                                ep[i].set_text("{}".format(CHV5_coarse['period'][i]))
                                ep[i].set_width_chars(8)
                                #control_list.append (ep[i])
                                grid_chv5c.attach(ep[i], 19, 5+r, 1, 1)

                        button = Gtk.Button.new_from_icon_name("media-playback-stop")
                        click_gesture = Gtk.GestureClick()
                        click_gesture.connect("pressed", self.start_coarse, 0, es, ev, ep)
                        click_gesture.connect("end",  self.stop_coarse, 0, es, ev, ep)
                        button.get_child().add_controller(click_gesture)
                        #control_list.append (button)
                        grid_chv5c.attach(button, 11, 6, 1, 1)

                        button = Gtk.Button.new_from_icon_name("arrow-left")
                        click_gesture = Gtk.GestureClick()
                                
                        click_gesture.connect("pressed",  self.start_coarse, -DX, es, ev, ep)
                        click_gesture.connect("end",  self.stop_coarse, -DX, es, ev, ep)
                        button.get_child().add_controller(click_gesture)
                        #control_list.append (button)
                        grid_chv5c.attach(button, 10, 6, 1, 1)

                        button = Gtk.Button.new_from_icon_name("arrow-right")
                        click_gesture = Gtk.GestureClick()
                        click_gesture.connect("pressed",  self.start_coarse, DX, es, ev, ep)
                        click_gesture.connect("end",  self.stop_coarse, DX, es, ev, ep)
                        button.get_child().add_controller(click_gesture)
                        #control_list.append (button)
                        grid_chv5c.attach(button, 12, 6, 1, 1)

                        button = Gtk.Button.new_from_icon_name("arrow-up")
                        click_gesture = Gtk.GestureClick()
                        click_gesture.connect("pressed",  self.start_coarse, DY, es, ev, ep)
                        click_gesture.connect("end",  self.stop_coarse, DY, es, ev, ep)
                        button.get_child().add_controller(click_gesture)
                        #control_list.append (button)
                        grid_chv5c.attach(button, 11, 5, 1, 1)

                        button = Gtk.Button.new_from_icon_name("arrow-down")
                        click_gesture = Gtk.GestureClick()
                        click_gesture.connect("pressed",  self.start_coarse, -DY, es, ev, ep)
                        click_gesture.connect("end",  self.stop_coarse, -DY, es, ev, ep)
                        button.get_child().add_controller(click_gesture)
                        #control_list.append (button)
                        grid_chv5c.attach(button, 11, 7, 1, 1)
                        

                        button = Gtk.Button.new_from_icon_name("arrow-up")
                        click_gesture = Gtk.GestureClick()
                        click_gesture.connect("pressed",  self.start_coarse, 3, es, ev, ep)
                        click_gesture.connect("end",  self.stop_coarse, 3, es, ev, ep)
                        button.get_child().add_controller(click_gesture)
                        #control_list.append (button)
                        grid_chv5c.attach(button, 15, 5, 1, 1)

                        button = Gtk.Button.new_from_icon_name("arrow-down")
                        click_gesture = Gtk.GestureClick()
                        click_gesture.connect("pressed",  self.start_coarse, -3, es, ev, ep)
                        click_gesture.connect("end",  self.stop_coarse, -3, es, ev, ep)
                        button.get_child().add_controller(click_gesture)
                        #control_list.append (button)
                        grid_chv5c.attach(button, 15, 7, 1, 1)

                        ###########
                       
                        ## ACTION DEMOS ##
                        
                        bias = Gtk.Entry ()
                        grid_chv5c.attach(bias, 19, 8, 1, 1)
                        bias.set_text('---')
                        
                        button = Gtk.Button(label="Set Bias")
                        button.connect("clicked", self.on_button_set_bias, bias)
                        grid_chv5c.attach(button, 17, 8, 1, 1)

                        button = Gtk.Button(label="Get Bias")
                        button.connect("clicked", self.on_button_get_bias, bias)
                        grid_chv5c.attach(button, 18, 8, 1, 1)

                        button = Gtk.Button(label="Z-Control")
                        button.connect("clicked", self.on_button_zcontrol)
                        grid_chv5c.attach(button, 17, 9, 1, 1)

                        button = Gtk.Button(label="Z Normal")
                        button.connect("clicked", self.on_button_znormal)
                        grid_chv5c.attach(button, 18, 9, 1, 1)

                        #gxsm_eid = Gtk.Entry ()
                        #grid_chv5c.attach(gxsm_eid, 13, 10, 4, 1)
                        #gxsm_eid.set_text('dsp-adv-dsp-zpos-ref')
                        #val = Gtk.Entry ()
                        #grid_chv5c.attach(val, 19, 10, 1, 1)

                        #button = Gtk.Button(label="Set")
                        #button.connect("clicked", self.on_button_set, gxsm_eid, val)
                        #grid_chv5c.attach(button, 17, 10, 1, 1)
                                                
                        #button = Gtk.Button(label="Get")
                        #button.connect("clicked", self.on_button_get, gxsm_eid, val)
                        #grid_chv5c.attach(button, 18, 10, 1, 1)
                        
                        #print (gxsm_eid.get_text())


                        print ("*** GUI complete ***")

                        self.window.present()
                        print ("*** Show Window, Run ***")
                        
                        
if __name__ == "__main__":
        print ("*** MAIN ***")
        app = HV5App()
        print ("*** READY ***")
        app.run()
        print ("*** EXITING ***")


