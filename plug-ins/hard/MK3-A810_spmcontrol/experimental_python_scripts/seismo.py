#!/usr/bin/env python3

## * Python User Control for
## * Configuration and SPM Approach Control tool for the FB_spm DSP program/GXSM2 Mk3-Pro/A810 based
## * for the Signal Ranger STD/SP2 DSP board
## * 
## * Copyright (C) 2010-13 Percy Zahl
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

#### get numpy, scipy:
#### sudo apt-get install python-numpy python-scipy python-matplotlib ipython ipython-notebook python-pandas python-sympy python-nose

import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GLib

import time

import struct
import array

from numpy import *

import math
import re

import pickle
import pydot
from mk3_spmcontol_class import *

import datetime

from influxdb import InfluxDBClient
influxdb_client = InfluxDBClient(host='localhost', port=8086)

OSZISCALE = 0.66


class RecorderDeci():
        # X: 7=time, plotting Monitor Taps 20,21,0,1
    def __init__(self, parent):
        print ( "Decimating Contineous Recorder Signal 0" )
        print ( "Connectign to influxdb..." )
        ### connect to influxdb, check db
        ##influxdb_client.drop_database('seismo')
        dblist=influxdb_client.get_list_database()
        create_db=True
        for db in dblist:
            print (db)
            if db['name'] == 'seismo':
                create_db=False
                break
            #[{'name': 'telegraf'}, {'name': '_internal'}, {'name': 'pyexample'}]
        if create_db:
            influxdb_client.create_database('seismo')
        influxdb_client.switch_database('seismo')
        ###

        #[Xsignal, Xdata, OffsetX] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL1_INPUT_ID)
        #[Ysignal, Ydata, OffsetY] = parent.mk3spm.query_module_signal_input(DSP_SIGNAL_SCOPE_SIGNAL2_INPUT_ID)


        self.lastevent = parent.mk3spm.read_recorder_deci (4097, "", True)
        self.lasteventvel = parent.mk3spm.read_recorder_deci (4097, "", True) # dummy fill
        self.logfile = 'mk3_S0_py_recorder_deci256.log'
        self.logcount = 5
        self.count = 100
        
        self.maxlist = ones(256)*0.1
        self.thauto = 0.2
        self.thi = 0

        self.event_tics = 5*60

        self.t_last=datetime.datetime.utcnow()
        self.t_last_ft=datetime.datetime.utcnow()
        self.t_next_info=datetime.datetime.utcnow()

        def update_recorder():

                        m1th  = 0.2
                        m1tha = 1.5
                        dscale = 256.*32768./10.   ## decimation 256 and DAC value to volt
                                
                        rec = parent.mk3spm.read_recorder_deci (4096, self.logfile)/dscale  ## raw reading in Volts
                        if rec.size != 4096:
                            print ("rec buffer size error. size=", rec.size)
                            return self.run
                        
                        acc = rec ## in mg  ------  [1000 V/g] 1V = 1mg ** MATCH PREAMPLIFIER SETTING
                        vel = zeros(4096)  ## in mm/s
                        dt=256.0/150000.0
                        if abs(acc.max()) < 9.0:
                            vint=0.0
                            om = sum(acc, axis=0)/4096.0  # floating actual zero or center value
                            i=0
                            for a in acc:
                                om = om*0.99+0.01*a ## low pass
                                vint = vint + 9.807*(a-om)*dt
                                vel[i] = vint
                                i=i+1
                                if i>4096:
                                    break

                        else:
                              self.run ## skip, data error
                              
                        #scope.set_data (acc/m1scale_div, vel/m2scale_div)

                        if m1th >= 0.0:
                                ma = acc[-512:].max()
                                mi = acc[-512:].min()
                                peak = max(abs(ma), abs(mi))
                                
                                t=datetime.datetime.utcnow()
                                if t > self.t_next_info:
                                    print([t, "max: "+str(ma), "min: "+str(mi), "thauto: "+str(self.thauto)])
                                    self.t_next_info = t+datetime.timedelta(seconds=60)
                                
                                if peak < 9.0:
                                    self.maxlist[self.thi]=peak
                                    self.thi = (self.thi+1)%256
                                    self.thauto = median(self.maxlist)*m1tha
                                
                                    if peak >= m1th or peak >= self.thauto or self.logcount > 0:
                                        #print(self.maxlist)
                                        print(sort(self.maxlist))
                                        print ("### Auto Threashold " + str(self.thauto) + " Triggered at: " + str(t) + " max: "+str(ma) + " min: "+str(mi) + " #" + str(self.logcount)+"\n")
                                        if self.logcount > 0:
                                            n=1024
                                        else:
                                            n=4096
                                        self.log_event_data (t, ma, mi, vel.max(), acc[-n:], vel[-n:], dt, self.thauto, 2)
                                        self.log_rft_data (t, dt, acc, vel)
                                    else:
                                        self.event_tics = self.event_tics-1
                                        if self.event_tics < 1:
                                            self.event_tics = 5*60
                                            self.log_event_only (t, peak, mi, vel.max(), self.thauto)

                                if peak > m1th or peak >= self.thauto:
                                        self.logfile = 'mk3_S0_py_recorder_deci256.log'
                                        if m1th > 0.0 and self.logcount == 0:
                                                with open(self.logfile, "a") as recorder:
                                                        recorder.write("### Threashold Triggered at: " + str(t) + " max: "+str(ma) + " min: "+str(mi) + "\n")
                                                        self.count = 0
                                                        self.lastevent = acc
                                                        self.lasteventvel = vel
                                        self.logcount = 25*5
                                        self.count = self.count + 1
                                        if self.count == 24:
                                                self.lastevent = rec
                                        print ("Threashold Detected, Recording... " + str(self.count))
                                else:
                                        if self.logcount > 0:
                                                self.logcount = self.logcount - 1
                                                print ("Threashold Detected, Recording... " + str(self.logcount))
                                        else:
                                                if self.logfile != '':
                                                        self.logfile = ''
                        
        while (1):
            update_recorder ()
            time.sleep(0.2)
                        
                

    def log_event_only (self, t, max, min, velmax, thauto):
                    json_body = [
                        {
                            "measurement": "event",
                            "tags": {
                                "user": "percy",
                                "accId": "Z-10264"
                            },
                            "time": str(t),
                            "fields": {
                                "max": max,
                                "min": min,
                                "velmax": velmax,
                                "thauto": thauto
                            }
                        }
                    ]
                    influxdb_client.write_points(json_body)

    def log_event_data (self, t, max, min, velmax, acc, vel, dt, thauto, decimation=16):
                    json_body = [
                        {
                            "measurement": "event",
                            "tags": {
                                "user": "percy",
                                "accId": "Z-10264"
                            },
                            "time": str(t),
                            "fields": {
                                "max": max,
                                "min": min,
                                "velmax": velmax,
                                "thauto": thauto
                            }
                        }
                    ]
                    t = t - datetime.timedelta(seconds=dt*acc.size)
                    deci=0
                    for a,v in zip(acc, vel):
                        t = t + datetime.timedelta(seconds=dt)
                        if deci==0:
                            am = a
                            vm = abs(v)
                        deci=deci+1
                        
                        if abs(a) < 10.0:
                            am = am*0.8+0.2*a
                        if abs(v) < 10.0:
                            vm = vm*0.8+0.2*abs(v)
                            
                        if deci >= decimation and self.t_last < t:
                            deci = 0
                            json_body.append({
                                "measurement": "data",
                                "tags": {
                                    "user": "percy",
                                    "accId": "Z-10264"
                                },
                                "time": str(t),
                                "fields": {
                                    "acc": am,
                                    "vel": vm
                                }
                            })
                    influxdb_client.write_points(json_body)
                    self.t_last = t
                    #print(json_body)

    def log_rft_data (self, t, dt, acc, vel):
        if t < self.t_last_ft+datetime.timedelta(seconds=4096*dt):
            return
        self.t_last_ft=t
        HW = hanning(acc.size)
        acc_rft = abs(fft.rfft(HW*acc))/acc.size
        acc_rft = acc_rft[1:int(acc.size/2)+1]
        vel_rft = abs(fft.rfft(HW*vel))/vel.size
        vel_rft = vel_rft[1:int(vel.size/2)+1]

        N  = acc.size/2
        freqs = arange(0,N)/N/dt/2
                
        json_body = []
        i1=0
        iN=1
        NN=128
        NP=int(N/2)
        lb=1.025
        iNN=lb**NN
        for i in range(0,NN):
            i2=int(NP*(iN-1)/iNN)
            json_body.append({
                "measurement": "rft_data",
                "tags": {
                    "user": "percy",
                    "accId": "Z-10264",
                    "frq": '{:05.1f}'.format(freqs[int((i1+i2)/2)])
                },
                "time": str(t-datetime.timedelta(seconds=2048*dt)),
                "fields": {
                    "mag_acc": acc_rft[i1:i2+1].max(),
                    "mag_vel": vel_rft[i1:i2+1].max()
                }
            })
            iN=iN*lb
            i1=i2
        #print(json_body)
        influxdb_client.write_points(json_body)


    def lin_rft_data (self, t, dt, acc, vel):
        if t < self.t_last_ft+datetime.timedelta(seconds=4096*dt):
            return
        self.t_last_ft=t
        acc_rft = abs(fft.rfft(acc))/acc.size
        acc_rft = acc_rft[1:int(acc.size/2)+1]
        vel_rft = abs(fft.rfft(vel))/vel.size
        vel_rft = vel_rft[1:int(vel.size/2)+1]
        rftsize = acc.size/2
        N = 128
        dec = int(rftsize/2/N)
        acc_rft = acc_rft.reshape(-1,dec).max(1)
        dec = int(rftsize/2/N)
        vel_rft = vel_rft.reshape(-1,dec).max(1)
        freqs = arange(0,N)
        json_body = []
        for f in freqs:
            json_body.append({
                "measurement": "rft_data",
                "tags": {
                    "user": "percy",
                    "accId": "Z-10264",
                    "frq": '{:05.1f}'.format((f+1)/(N/2)/(dt*dec))
                },
                "time": str(t-datetime.timedelta(seconds=2048*dt)),
                "fields": {
                    "mag_acc": acc_rft[f],
                    "mag_vel": vel_rft[f]
                }
            })
        #print(json_body)
        influxdb_client.write_points(json_body)


    def rft_peaks (self, t, dt, acc, vel):
        if t < self.t_last_ft+datetime.timedelta(seconds=4096*dt):
            return
        self.t_last_ft=t
        np = 64
        acc_rft = abs(fft.rfft(acc[0:4096]))/4096   #acc.size
        vel_rft = abs(fft.rfft(vel[0:4096]))/4096   #vel.size
        peaks_tmp = argpartition (-acc_rft, np)
        peaks = peaks_tmp[:np]
        f_peaks = peaks/acc.size/dt
        json_body = []
        for f,peak in zip(f_peaks, peaks):
            if peak < 10 or peak > size(data)-10:
                continue
            if acc_rft[peak] < amax(data[(peak-10):(peak+10)]):
                continue
        #    print(f, acc_rft[peak])
            json_body.append({
                "measurement": "rft_peaks",
                "tags": {
                    "user": "percy",
                    "accId": "Z-10264"
                },
                "time": str(t),
                "labels": {
                    "frq": f,
                },
                "fields": {
                    "acc": acc_rft[peak]
                }
            })
        influxdb_client.write_points(json_body)


################################################################################
# CONFIGURATOR MAIN MENU/WINDOW
################################################################################

class Mk3_Configurator:
    def __init__(self):
        self.mk3spm = SPMcontrol (0)
        self.mk3spm.read_configurations ()
        self.vector_index = 0
        decirecorder = RecorderDeci (self)
 


########################################

print ( __name__ )
if __name__ == "__main__":
        mk3 = Mk3_Configurator ()

## END
