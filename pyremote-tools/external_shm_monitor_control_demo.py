#!/usr/bin/env python3

### #!/usr/bin/env python

## * Python XYZ View and Coarse Template GUI
## * with demo Gxsm4+RPSPMC external actions
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


# Template HV Control

SWAP_CXY=1


global rpspmc

rpspmc = {
        'bias': 0.0,
        'current': 0.0,
        'gvp' : { 'x':0.0, 'y':0.0, 'z': 0.0, 'u': 0.0, 'a': 0.0, 'b': 0.0, 'am':0.0, 'fm':0.0 },
        'pac' : { 'dds_freq': 0.0, 'ampl': 0.0, 'exec':0.0, 'phase': 0.0, 'freq': 0.0, 'dfreq': 0.0, 'dfreq_ctrl': 0.0 },
        'time' : 0.0
        }

global PZHV_QXYZ_configuration
global PZHV_QXYZ_driftcomp
global PZHV_QXYZ_monitor
global PZHV_QXYZ_coarse
global PZHV_QXYZ_gain_list
global PZHV_QXYZ_gains

PZHV_QXYZ_configuration = {
        'gain':  [2,2,3],
        'filter': [0,0,0],
        'bw': [0,0,0],
        'target': [0.,0.,0.]
}

PZHV_QXYZ_monitor = {
        'monitor': [0.0,0.0,0.0],
        'monitor_min': [0.0,0.0,0.0],
        'monitor_max': [0.0,0.0,0.0],
        }

PZHV_QXYZ_gain_list = [3,6,12,24]
PZHV_QXYZ_gains = [12., 12., 24.]


PZHV_QXYZ_coarse = {
        'steps': [10,10,5],
        'volts': [100.0,100.0,75.0],
        'period': [500,500,500]
        }

PZHV_QXYZ_driftcomp = [ 0., 0., 0. ]

DIGVfacM = 1. #200./32767.
DIGVfacO = 1. #181.81818/32767.

scaleM = [ DIGVfacM, DIGVfacM, DIGVfacM ]
scaleO = [ DIGVfacO, DIGVfacO, DIGVfacO ]
unit  = [ "V", "V", "V", "dB" ]

updaterate = 88        # update time watching the SRanger in ms

GXSM_Link_status = FALSE

setup_list = []
control_list = []

gxsm = gxsm4.gxsm_process()

class PZXX():
        def __init__(self, ip):
                self.besocke = False
                self.ip = ip
                print ('PZXX @ test ip ', ip)

        def request_stmafm(self, data):
                print (self.ip + 'stmafm?' + data)
                #return requests.get(self.ip + 'stmafm?' + data)

        def request(self, data):
                print (self.ip + data)
                #return requests.get(self.ip + data)

        def myip(self):
                return self.ip

        def request_coarse(self, data):
                print (self.ip + 'coarse?' + data)
                return
                return requests.get(self.ip + 'stmafm?' + data)

                
        # gain: 3,6,12,24  filters: 'ON','OFF' 
        def SetTHVGain(self, gainx, gainy, gainz):
                return self.request ('gain?gain?g0={}&l0=ON&e0=OFF&g1={}&l1=ON&e1=OFF&g2={}&l2=ON&e2=ON&g3=3&l3=OFF&e3=OFF'.format(gainx, gainy, gainz))

        def SetTHVCoarseMove(self, channel, direction, burstcount, period, voltage, start):
                c=['X','Y','Z']
                return
                if start:
                        return self.request ('coarse?v={}&p0={}&a0={}&c0={}&bs=0'.format(int(voltage), int(period), c[channel], burstcount*direction))
                else:
                        return self.request ('coarse?v={}&p0={}&a0={}&c0={}&bs=0'.format(0, 0, c[channel], 0))

        def THVReset (self):
                return
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
        global PZHV_QXYZ_configuration
        
def _X_write_back_():
        global PZHV_QXYZ_configuration

## show only current
global QPAmpl
QPAmpl = [0.,0.,0.]
def update_PZHV_QXYZ_monitor(_c1set, _c2set, _c3set, _cqp):
        global PZHV_QXYZ_configuration
        global PZHV_QXYZ_monitor
        global rpspmc
        global QPAmpl

        _c1set (scaleM[0] * PZHV_QXYZ_monitor['monitor'][0], scaleM[0] * PZHV_QXYZ_monitor['monitor_min'][0], scaleM[0] * PZHV_QXYZ_monitor['monitor_max'][0])
        _c2set (scaleM[1] * PZHV_QXYZ_monitor['monitor'][1], scaleM[1] * PZHV_QXYZ_monitor['monitor_min'][1], scaleM[1] * PZHV_QXYZ_monitor['monitor_max'][1])
        _c3set (scaleM[2] * PZHV_QXYZ_monitor['monitor'][2], scaleM[2] * PZHV_QXYZ_monitor['monitor_min'][2], scaleM[2] * PZHV_QXYZ_monitor['monitor_max'][2])

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
def update_PZHV_QXYZ_monitor_full(_c1set, _c2set, _c3set):
        global PZHV_QXYZ_configuration
        global PZHV_QXYZ_monitor

        spantag  = "<span size=\"24000\" font_family=\"monospace\">"   # .. color=\"#ff0000\"
        spantag2 = "<span size=\"10000\" font_family=\"monospace\">"   # .. color=\"#ff0000\"
                
        _c1set (spantag + "<b>%8.3f " %(scaleM[0]*PZHV_QXYZ_monitor[ii_monitor_X]) + unit[0] + "</b></span>\n"
                + spantag2
                + "%8.3f " %(scaleM[0]*PZHV_QXYZ_monitor[ii_monitor_Xmin]) + unit[0]
                + "%8.3f " %(scaleM[0]*PZHV_QXYZ_monitor[ii_monitor_Xmax]) + unit[0]
                +" </span>")
        _c2set (spantag + "<b>%8.3f " %(scaleM[1]*PZHV_QXYZ_monitor[ii_monitor_Y]) + unit[1] + "</b></span>\n"
                + spantag2
                + "%8.3f " %(scaleM[0]*PZHV_QXYZ_monitor[ii_monitor_Ymin]) + unit[0]
                + "%8.3f " %(scaleM[0]*PZHV_QXYZ_monitor[ii_monitor_Ymax]) + unit[0]
                +" </span>")
        _c3set (spantag + "<b>%8.3f " %(scaleM[2]*PZHV_QXYZ_monitor[ii_monitor_Z]) + unit[2] + "</b></span>\n"
                + spantag2
                + "%8.3f " %(scaleM[0]*PZHV_QXYZ_monitor[ii_monitor_Zmin]) + unit[0]
                + "%8.3f " %(scaleM[0]*PZHV_QXYZ_monitor[ii_monitor_Zmax]) + unit[0]
                +" </span>")
        
        return 1


def hv1_adjust(_object, _value, _identifier):
        global PZHV_QXYZ_gain_list
        global PZHV_QXYZ_gains
        value = _value
        index = _identifier
        #if index >= ii_config_preset_X0 and index < ii_config_dum:

        PZHV_QXYZ_gains[index] = PZHV_QXYZ_gain_list[value]

        print ('HV-adjust: ', index, value, ', PZHVXX Gains: ', PZHV_QXYZ_gains, ' ... may toggle to enforce up-to-now.')

        read_back ()
        return 1


        
class offset_vector():
    def __init__(self, etv, gsv):
        self.etvec = etv
        self.gainselectmenuvec = gsv

    def update_display(self):
        for i in range (0,3):
            self.etvec[i].set_text("%12.3f" %(scaleO[i]*PZHV_QXYZ_configuration['target'][i]))
        return False

    def write_t_vector(self, button):
        global PZHV_QXYZ_configuration
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
        global PZHV_QXYZ_configuration
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
            self.tvi[i] = scaleO[i] * PZHV_QXYZ_configuration['target'][i]
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
        global PZHV_QXYZ_monitor
        global PZHV_QXYZ_gains

        global rpspmc
        
        PZHV_QXYZ_monitor = gxsm.rt_query_xyz()
        rpspmc            = gxsm.rt_query_rpspmc()
                
        return 1

def do_exit(button):
        Gtk.main_quit()

def destroy(*args):
        Gtk.main_quit()

def locate_spm_control_dsp():
        print ("need to create a MK3 base support class.........")


class PZHVXYZ_App(Gtk.Application):
        def __init__(self, *args, **kwargs):
                print ("Init PZXYZ App...")
                super().__init__(*args, application_id="com.createc.hv5app", **kwargs)

                print ("PZXX ...")
                self.thv = PZXX('http://192.168.0.195/')  # any existing "dummy" IP to send http requests to even been ignored
                #self.thv = PZXX('http://130.199.243.191/') # self test
                #self.thv = PZXX('http://192.168.40.10/')

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
                global PZHV_QXYZ_gains
                self.thv.SetTHVGain(PZHV_QXYZ_gains[0], PZHV_QXYZ_gains[1], PZHV_QXYZ_gains[2])


        ## GXSM4 / RPSPM communication set/get + control demos
        def on_button_set_bias(self, w, bias):
                #for i in range (0,100):
                #        gxsm.set ('dsp-fbs-bias',i/100)
                gxsm.set ('dsp-fbs-bias', float( bias.get_text () ))
                
        def on_button_get_bias(self, w, bias):
                bias.set_text('{}'.format(gxsm.get ('dsp-fbs-bias')))
                #for i in range (0,500):
                #        bias.set_text('{}'.format(gxsm.get ('dsp-fbs-bias')))

        def on_button_zcontrol(self, w):
                gxsm.set_current_level_zcontrol ()
                
        def on_button_znormal(self, w):
                gxsm.set_current_level_0 () ## normal Z control
                
        def on_test_button(self, w):
                x = gxsm.list_refnames()
                print (x)
                for eid in x:
                        print ('ENTRY {} = {}'.format(eid, gxsm.get(eid)))
                print(gxsm.get_instrument_gains())
                               
                        
        def on_button_set(self, w, eid, val):
                gxsm.set (eid.get_text(), float( val.get_text () ))
                
        def on_button_get(self, w, eid, val):
                print (eid.get_text(), gxsm.get (eid.get_text()))
                val.set_text('{}'.format(gxsm.get (eid.get_text())))

                id = eid.get_text()
                vr = gxsm.get (id)
                print ('FLOOD TEST', id, vr)
                for i in range (0, 10000):
                        v = gxsm.get (id)
                        if v != vr:
                                print ('EE:',i,v,vr)
                print ('FLOOD TEST completed')
                
                
        def do_activate(self):
                global PZHV_QXYZ_configuration
                global PZHV_QXYZ_monitor
                global PZHV_QXYZ_gains
                
                if not self.window:
                        print ("Build GUI...")
                        self.window = Gtk.ApplicationWindow(application=self, title="Createc PZXX @ "+self.thv.myip())
                        name = "Createc HV Control"

                        print ('do_activate: PZHVXX Gains: ', PZHV_QXYZ_gains, ' ... may toggle to enforce up-to-now.')
                        
                        hb = Gtk.HeaderBar() 
                        #hb.set_show_close_button(True) 
                        #hb.props.title = "PZHVXX"
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
                        
                        GLib.timeout_add (updaterate, update_PZHV_QXYZ_monitor, c1.set_reading_lohi, c2.set_reading_lohi, c3.set_reading_lohi, cqp.set_reading_auto_vu_extra)
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
                                eo[i].set_text("%12.3f" %(scaleO[i]*PZHV_QXYZ_configuration['target'][i]))
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
                                ed[i].set_text("%12.3f " %(scaleO[i]*PZHV_QXYZ_driftcomp[i]) ) # + unit[i] + "/s")
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
                                opt.set_active(PZHV_QXYZ_configuration['gain'][ci])
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
                                opt.set_active(PZHV_QXYZ_configuration['bw'][ci])
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
                                es[i].set_text("{}".format(PZHV_QXYZ_coarse['steps'][i]))
                                es[i].set_width_chars(8)
                                #control_list.append (eo[i])
                                grid_chv5c.attach(es[i], 17, 5+r, 1, 1)
                                ev[i].set_text("{}".format(PZHV_QXYZ_coarse['volts'][i]))
                                ev[i].set_width_chars(8)
                                #control_list.append (evo[i])
                                grid_chv5c.attach(ev[i], 18, 5+r, 1, 1)
                                ep[i].set_text("{}".format(PZHV_QXYZ_coarse['period'][i]))
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

                        button = Gtk.Button(label="Test Button")
                        button.connect("clicked", self.on_test_button)
                        grid_chv5c.attach(button, 19, 9, 1, 1)

                        gxsm_eid = Gtk.Entry ()
                        grid_chv5c.attach(gxsm_eid, 13, 10, 4, 1)
                        gxsm_eid.set_text('dsp-adv-dsp-zpos-ref')
                        val = Gtk.Entry ()
                        grid_chv5c.attach(val, 19, 10, 1, 1)

                        button = Gtk.Button(label="Set")
                        button.connect("clicked", self.on_button_set, gxsm_eid, val)
                        grid_chv5c.attach(button, 17, 10, 1, 1)
                                                
                        button = Gtk.Button(label="Get")
                        button.connect("clicked", self.on_button_get, gxsm_eid, val)
                        grid_chv5c.attach(button, 18, 10, 1, 1)
                        
                        print (gxsm_eid.get_text())
                        
                        print ("*** GUI complete ***")

                        self.window.present()
                        print ("*** Show Window, Run ***")
                        
                        
if __name__ == "__main__":
        print ("*** MAIN ***")
        app = PZHVXYZ_App()
        print ("*** READY ***")
        app.run()
        print ("*** EXITING ***")


#gxsm_shm.close()

