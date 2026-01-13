#!/usr/bin/env python3

### #!/usr/bin/env python

## * Python Gxsm4 RPSPMC interprocess control libraray
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

import types
import faulthandler; faulthandler.enable()

version = "1.0.0"

import time
#import fcntl
#from threading import Timer

import struct
import array
import math
import pickle

import numpy as np

from multiprocessing import shared_memory
from multiprocessing.resource_tracker import unregister

import os
import psutil
import signal

# ACTIONS
SHM_ACTION_IDLE       = 0x0000000
SHM_ACTION_CTRL_Z_SET = 0x0000010
SHM_ACTION_CTRL_Z_FB  = 0x0000011
SHM_ACTION_GET        = 0x0000020
SHM_ACTION_SET        = 0x0000021

# SHM MEMORY LOCATION ASIGNMENTS
SHM_SIZE_ACTION_MAP        = 256

SHM_ACTION_OFFSET          = 128
SHM_ACTION_VPAR_64_START   = 130
SHM_ACTION_FRET_64_START   = 140

SHM_ACTION_FP_ID_STRING_64 = 150 # max length 64

SHM_NUM_VPAR = 9
SHM_NUM_FRET = 9

SHM_OK       = 0  ## SHM idle/ok
SHM_E_BUSY   = -1 ## SHM action busy
SHM_E_NA     = -3 ## SHM not available


class gxsm_process():
        def __init__(self, blocking=True):
                # try open shared memory created by gxsm4 to access RPSPMC's monitors and more
                self.blocking_requests = blocking
                
                # RPSPMC XYZ Monitors via GXSM4 
                self.XYZ_monitor = {
                        'monitor': [0.0,0.0,0.0], # Volts
                        'monitor_min': [0.0,0.0,0.0],
                        'monitor_max': [0.0,0.0,0.0],
                }
                self.rpspmc = {
                        'bias':    0.0,    # Volts
                        'current': 0.0, # nA
                        'gvp' :    { 'x':0.0, 'y':0.0, 'z': 0.0, 'u': 0.0, 'a': 0.0, 'b': 0.0, 'am':0.0, 'fm':0.0 },
                        'pac' :    { 'dds_freq': 0.0, 'ampl': 0.0, 'exec':0.0, 'phase': 0.0, 'freq': 0.0, 'dfreq': 0.0, 'dfreq_ctrl': 0.0 },
                        'zservo':  { 'mode': 0.0, 'setpoint': 0.0, 'cp': 0.0, 'ci': 0.0, 'cp_db': 0.0, 'ci_db': 0.0, 'upper': 0.0, 'lower': 0.0, 'setpoint_cz': 0.0, 'level': 0.0, 'in_offcomp': 0.0, 'src_mux': 0.0 }, # RPSPMC Units (i.e. Volt)
                        'scan':    { 'alpha': 0.0, 'slope' : { 'dzx': 0.0, 'dzy': 0.0, 'slew': 0.0, }, 'x': 0.0, 'y': 0.0, 'slew': 0.0, 'opts': 0.0, 'offset' : { 'x': 0.0, 'y': 0.0, 'z': 0.0 }}, # RPSPMC Units (i.e. Volt)
                        'time' :   0.0  # exact FPGA uptime. This is the FPGA time of last reading in seconds, 8ns resolution
                }

                self.gxsm4pid = 0

                for proc in psutil.process_iter(['pid', 'name']):
                        try:
                                # Check if process name matches the desired name
                                if proc.info['name'] == 'gxsm4':
                                        self.gxsm4pid = proc.info['pid']
                        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                                # Handle potential exceptions for processes that might terminate
                                # or be inaccessible during iteration
                                pass
                        
                if self.gxsm4pid > 0:
                        print ('Gxsm4 PID found: ', self.gxsm4pid)
                        self.noargs = "".encode('utf-8') # Empty PyObject
                        self.get_gxsm4pyshm_shm_block()

                self.get_gxsm4rpspmc_shm_block()

                self.read_status()
                
        # Open SHM block from Gxsm4
        def get_gxsm4rpspmc_shm_block(self):
                try:
                        self.gxsm_shm = shared_memory.SharedMemory(name='gxsm4rpspmc_monitors')
                        unregister(self.gxsm_shm._name, 'shared_memory')
                        #### do not close externally!!! gxsm4 manages: gxsm_shm.close()
                        print (self.gxsm_shm)
                        xyz=np.ndarray((9,), dtype=np.double, buffer=self.gxsm_shm.buf).reshape((3,3)).T  # X Mi Ma, Y Mi Ma, Z Mi Ma
                        print (xyz)
                except FileNotFoundError:
                        print ("SharedMemory(name='gxsm4rpspmc_monitors') is not available. Please start gxsm4 and connect RPSPMC.")

        def get_gxsm4pyshm_shm_block(self):
                try:
                        self.gxsm_pyshm = shared_memory.SharedMemory(name='gxsm4_py_shm_data_block')
                        unregister(self.gxsm_pyshm._name, 'shared_memory')
                        #### do not close externally!!! gxsm4 manages: gxsm_pyshm.close()
                        print (self.gxsm_pyshm)
                        self.init_pyshm_mappings()
                        
                except FileNotFoundError:
                        print ("SharedMemory(name='gxsm4_py_shm_data_block') is not available. Please start gxsm4.")
                        
        def init_pyshm_mappings(self):
                pyshm_status = np.frombuffer(self.gxsm_pyshm.buf, dtype=np.int64, count=1, offset=100)
                pyshm_status[0] = 0 # clear BUSY
                
                #methods = self.exec_pyshm_method('help', ())
                methods = self.exec_pyshm_method('help', ('M',))
                print (methods)
                for m in methods:
                        print (m)

                if 1: ## test
                        v = self.exec_pyshm_method('get', ('dsp-fbs-bias',))
                        print ('Return:', v)
                        
                if 0: ## tests
                        v = self.exec_pyshm_method('xxxxget', ('dsp-fbs-bias',4))
                        print ('Return:', v)
                
                        v = self.exec_pyshm_method('set', ('dsp-fbs-bias','1.234',))
                        print ('Return:', v)

                return methods
                
        def exec_pyshm_method(self, method, args):
                print ('exec_pyshm_method ', method, args)
                pyshm_status = np.frombuffer(self.gxsm_pyshm.buf, dtype=np.int64, count=1, offset=100)
                #while pyshm_status[0] == 1:
                #        time.sleep(0.01)
                #        print ('** waiting, busy. ',  pyshm_status[0])
                print ('Status: ',  pyshm_status[0])
                        
                pyshm_method = self.gxsm_pyshm.buf[0:len(method)+1]
                pyshm_object_len = np.frombuffer(self.gxsm_pyshm.buf, dtype=np.uint64, count=1, offset=128)
                method_bytes = method.encode('utf-8') + b'\x00'
                pyshm_method[:] = method_bytes # set method name

                args_data =  pickle.dumps(args)
                print ('args: ', args_data)
                print ('args.len:', len(args_data))
                pyshm_object_len[0] = len(args_data) # object size
                self.gxsm_pyshm.buf[128+8 : 128+8+len(args_data)] = args_data
                
                pyshm_status[0] = 1 # mark BUSY
                
                print ('signaling gxsm')
                os.kill(self.gxsm4pid, signal.SIGUSR1) # signal Gxsm4.PySHM...

                # wait
                while pyshm_status[0] == 1:
                        time.sleep(0.1)
                        print ('waiting')
                if pyshm_status[0] == -1:
                        print ('SHM Method Result: Error = ', pyshm_status[0])
                        return
                else:
                        print ('SHM Method Result Code = ', pyshm_status[0])

                pyshm_pickles_bytes_len = int(np.frombuffer(self.gxsm_pyshm.buf, dtype=np.uint64, count=1, offset=128))
                print ('data len:', pyshm_pickles_bytes_len)
                return pickle.loads(self.gxsm_pyshm.buf[128+8 : 128+8+pyshm_pickles_bytes_len])

        ### PySHM wrapper functions for full PyRemote Gxsm4 level embedded python compatibility

        # Auto Code Draft Gen Script (run in gxsm console), needs manual editing!!
        """
        h=gxsm.help()
        for m in h:
	        d=m.split(':')
	        args=d[-1].split('(')[-1][:-1].replace('\'','')
	        args.replace(' as string','')
	        args.replace(' string','')
	        print ("        # {}".format(d[-1]))
	        print ("        def {}(self, {}):".format(d[0][5:-1], args))
	        print ("                return self.exec_pyshm_method ('{}', ({},))".format(d[0][5:-1], args))
	        print ()
	
	"""

        def Mset (self, id, value): # id: Gxsm Entry ID, value: entry text/value as str
                return self.exec_pyshm_method('set', (id,value))
                
        def setv (self, id, value): # convenience version, value numeric
                return self.exec_pyshm_method('set', (id,str(value)))
                
        def Mget (self, id): # id: Gxsm Entry ID
                return self.exec_pyshm_method('get', (id,))
                
        ### auto gen wrapper code to edited and checked:
        
        #  print gxsm.help ()
        def help(self):
                return self.exec_pyshm_method ('help', ())

        #  gxsm.set ('refname','value as string')
        def set(self, refname, value):
                if isinstance(value, str):
                        return self.exec_pyshm_method ('set', (refname, value,))
                else:
                        return self.exec_pyshm_method ('set', (refname, str(value),))

        #  Get Gxsm entry as value, see list_refnames. gxsm.get ('refname')
        def get(self, refname):
                return self.exec_pyshm_method ('get', (refname,))

        #  Get Gxsm entry as string. gxsm.gets ('refname')
        def gets(self, refname):
                return self.exec_pyshm_method ('gets', (refname,))

        #  pointer hover over Gxsm-Entry to see tooltip with ref-name). print gxsm.list_refnames ()
        def list_refnames(self):
                return self.exec_pyshm_method ('list_refnames', ())

        #  gxsm.action('action')
        def action(self, action):
                return self.exec_pyshm_method ('action', (action,))

        #  print gxsm.list_actions ()
        def list_actions(self):
                return self.exec_pyshm_method ('list_actions', ())

        #  svec[3] = gxsm.rtquery(what as string, one of 'X|Y|Z|xy|zxy|o|f|s|i|U') 
        def rtquery(self, what):
                return self.exec_pyshm_method ('rtquery', (what,))

        #  RTQuery Current Scanline.
        def y_current(self):
                return self.exec_pyshm_method ('y_current', ())

        #  gxsm.moveto_scan_xy (x,y)
        def moveto_scan_xy(self, x,y):
                return self.exec_pyshm_method ('moveto_scan_xy', (x,y,))

        #  gxsm.createscan (ch,nx,ny,nv pixels, rx,ry in A, array.array('l', [...]), append)
        def createscan(self, ch, nx,ny,nv, rx,ry, np_array1d, append=False):
                return self.exec_pyshm_method ('createscan', (ch, nx,ny,nv, rx,ry, np_array1d, append,))

        #  gxsm.createscan (ch,nx,ny,nv pixels, rx,ry in A, array.array('f', [...]), append)
        def createscanf(self, ch, nx,ny,nv, rx,ry, array1d, append):
                return self.exec_pyshm_method ('createscanf', (ch, nx,ny,nv, rx,ry, array1d, append,))

        #  UnitId string','Label string')
        def get_scan_unit(self):
                return self.exec_pyshm_method ('get_scan_unit', ())

        #  gxsm.set_scan_unit (ch,axis='X|Y|Z|L|T','UnitId string','Label string')
        def set_scan_unit(self, ch, axis, UnitId, label):
                return self.exec_pyshm_method ('set_scan_unit', (ch, axis, UnitId, label,))

        #  gxsm.set_scan_lookup (ch,'X|Y|L',start,end)
        def set_scan_lookup(self, ch, axis, start,end):
                return self.exec_pyshm_method ('set_scan_lookup', (ch, axis, start,end,))

        #  [rx,ry,x0,y0,alpha]=gxsm.get_geometry (ch)
        def get_geometry(self, ch):
                return self.exec_pyshm_method ('get_geometry', (ch,))

        #  [dx,dy,dz,dl]=gxsm.get_differentials (ch)
        def get_differentials(self, ch):
                return self.exec_pyshm_method ('get_differentials', (ch,))

        #  [nx,ny,nv,nt]=gxsm.get_dimensions (ch)
        def get_dimensions(self, ch):
                return self.exec_pyshm_method ('get_dimensions', (ch,))

        #  [VX,VY,VZ, V2XAV,V2YAV,V2ZAV]=gxsm.get_instrument_gains_xyz ()
        def get_instrument_gains_xyz(self):
                return self.exec_pyshm_method ('get_instrument_gains_xyz', ())

        #  value=gxsm.get_data_pkt (ch, x, y, v, t)
        def get_data_pkt(self, ch, x, y, v, t):
                return self.exec_pyshm_method ('get_data_pkt', (ch, x, y, v, t,))

        #  gxsm.put_data_pkt (value, ch, x, y, v, t)
        def put_data_pkt(self, value, ch, x, y, v, t):
                return self.exec_pyshm_method ('put_data_pkt', (value, ch, x, y, v, t,))

        #  [nx,ny,array]=gxsm.get_slice (ch, v, t, yi, yn)
        def get_slice(self, ch, v, t, yi, yn):
                return self.exec_pyshm_method ('get_slice', (ch, v, t, yi, yn,))

        #  [nv,ny,array]=gxsm.get_slice_v (ch, x, t, yi, yn)
        def get_slice_v(self, ch, x, t, yi, yn):
                return self.exec_pyshm_method ('get_slice_v', (ch, x, t, yi, yn,))

        #  Get Probe Event Data from Scan in channel ch, nth: [sets, labels, units, position[x,y,ix,iy]]=gxsm.get_probe_event (ch, nth)  ** if nth>existing, returns #probe events, nth=-99: removes all events
        def get_probe_event(self, ch, nth):
                return self.exec_pyshm_method ('get_probe_event', (ch, nth,))
        
        #  gxsm.set_global_share_parameter (key, x)
        def set_global_share_parameter(self, key, x):
                return self.exec_pyshm_method ('set_global_share_parameter', (key, x,))

        #  x=gxsm.get_x_lookup (ch, i)
        def get_x_lookup(self, ch, i):
                return self.exec_pyshm_method ('get_x_lookup', (ch, i,))

        #  y=gxsm.get_y_lookup (ch, i)
        def get_y_lookup(self, ch, i):
                return self.exec_pyshm_method ('get_y_lookup', (ch, i,))

        #  v=gxsm.get_v_lookup (ch, i)
        def get_v_lookup(self, ch, i):
                return self.exec_pyshm_method ('get_v_lookup', (ch, i,))

        #  x=gxsm.get_x_lookup (ch, i, v)
        def set_x_lookup(self, ch, i, v):
                return self.exec_pyshm_method ('set_x_lookup', (ch, i, v,))

        #  y=gxsm.get_y_lookup (ch, i, v)
        def set_y_lookup(self, ch, i, v):
                return self.exec_pyshm_method ('set_y_lookup', (ch, i, v,))

        #  v=gxsm.get_v_lookup (ch, i, v)
        def set_v_lookup(self, ch, i, v):
                return self.exec_pyshm_method ('set_v_lookup', (ch, i, v,))

        #  [type, x,y,..]=gxsm.get_object (ch, n)
        def get_object(self, ch, n):
                return self.exec_pyshm_method ('get_object', (ch, n,))

        #  gxsm.add_marker_object (ch, label=str|'xy'|'Rectangle-id', mgrp=0..5|-1, x=ix,y=iy, size)
        def add_marker_object(self, ch, label, mgrp, ix,iy, size):
                return self.exec_pyshm_method ('add_marker_object', (ch, label, mgrp, ix,iy, size,))

        #  gxsm.marker_getobject_action (ch, objnameid, action='REMOVE'|'REMOVE-ALL'|'GET_COORDS'|'SET-OFFSET')
        def marker_getobject_action(self, ch, objnameid, action):
                return self.exec_pyshm_method ('marker_getobject_action', (ch, objnameid, action,))

        #  Start Scan.
        def startscan(self):
                return self.exec_pyshm_method ('startscan', ())

        #  Stop Scan.
        def stopscan(self):
                return self.exec_pyshm_method ('stopscan', ())

        #  no scan in progress, else current line index.
        def waitscan(self):
                return self.exec_pyshm_method ('waitscan', ())

        #  Scaninit.
        def scaninit(self):
                return self.exec_pyshm_method ('scaninit', ())

        #  Scanupdate.
        def scanupdate(self):
                return self.exec_pyshm_method ('scanupdate', ())

        #  Scanylookup.
        def scanylookup(self):
                return self.exec_pyshm_method ('scanylookup', ())

        #  Scan line.
        def scanline(self):
                return self.exec_pyshm_method ('scanline', ())

        #  Auto Save Scans. gxsm.autosave (). Returns current scanline y index and file name(s) if scanning.
        def autosave(self):
                return self.exec_pyshm_method ('autosave', ())

        #  Auto Update Scans. gxsm.autoupdate (). Returns current scanline y index and file name(s) if scanning.
        def autoupdate(self):
                return self.exec_pyshm_method ('autoupdate', ())

        #  gxsm.save ()
        def save(self):
                return self.exec_pyshm_method ('save', ())

        #  gxsm.saveas (ch, 'path/fname.nc')
        def saveas(self, ch, fname):
                return self.exec_pyshm_method ('saveas', (ch, fname,))

        #  gxsm.load (ch, 'path/fname.nc')
        def load(self, ch, fnamec):
                return self.exec_pyshm_method ('load', (ch, fname,))

        #  gxsm.export (ch, 'fname')
        def export(self, ch, fname):
                return self.exec_pyshm_method ('export', (ch, fname,))

        #  gxsm.import (ch, 'fname') ### NOTE: CAN NOT USE reserved name 'import' for function def
        def data_import(self, ch, fname):
                return self.exec_pyshm_method ('import', (ch, fname,))

        #  gxsm.save_drawing (ch, time, layer, 'path/fname.png|pdf|svg')
        def save_drawing(self, ch, time, layer, fname):
                return self.exec_pyshm_method ('save_drawing', (ch, time, layer, fname,))

        #  gxsm.set_view_indices (ch, time, layer)
        def set_view_indices(self, ch, time, layer):
                return self.exec_pyshm_method ('set_view_indices', (ch, time, layer,))

        #  gxsm.autodisplay ()
        def autodisplay(self):
                return self.exec_pyshm_method ('autodisplay', ())

        #  filename = gxsm.chfname (ch)
        def chfname(self, ch):
                return self.exec_pyshm_method ('chfname', (ch,))

        #  gxsm.chmodea (ch)
        def chmodea(self, ch):
                return self.exec_pyshm_method ('chmodea', (ch,))

        #  gxsm.chmodex (ch)
        def chmodex(self, ch):
                return self.exec_pyshm_method ('chmodex', (ch,))

        #  gxsm.chmodem (ch)
        def chmodem(self, ch):
                return self.exec_pyshm_method ('chmodem', (ch,))

        #  gxsm.chmoden (ch,n)
        def chmoden(self, ch,n):
                return self.exec_pyshm_method ('chmoden', (ch,n,))

        #  gxsm.chmodeno (ch)
        def chmodeno(self, ch):
                return self.exec_pyshm_method ('chmodeno', (ch,))

        #  gxsm.chmode1d (ch)
        def chview1d(self, ch):
                return self.exec_pyshm_method ('chview1d', (ch,))

        #  gxsm.chmode2d (ch)
        def chview2d(self, ch):
                return self.exec_pyshm_method ('chview2d', (ch,))

        #  gxsm.chmode3d (ch)
        def chview3d(self, ch):
                return self.exec_pyshm_method ('chview3d', (ch,))

        #  Quick.
        def quick(self):
                return self.exec_pyshm_method ('quick', ())

        #  Direct.
        def direct(self):
                return self.exec_pyshm_method ('direct', ())

        #  Log.
        def log(self):
                return self.exec_pyshm_method ('log', ())

        #  Crop (ch-src, ch-dst)
        def crop(self, src, dst):
                return self.exec_pyshm_method ('crop', (src, dst,))

        #  UnitBZ.
        def unitbz(self):
                return self.exec_pyshm_method ('unitbz', ())

        #  UnitVolt.
        def unitvolt(self):
                return self.exec_pyshm_method ('unitvolt', ())

        #  UniteV.
        def unitev(self):
                return self.exec_pyshm_method ('unitev', ())

        #  UnitS.
        def units(self):
                return self.exec_pyshm_method ('units', ())

        #  Echo string to terminal. gxsm.echo('hello gxsm to terminal') 
        def echo(self, text):
                return self.exec_pyshm_method ('echo', (text,))

        #  gxsm.logev ('hello gxsm to logfile/monitor') 
        def logev(self, text):
                return self.exec_pyshm_method ('logev', (text,))

        #  gxsm.progress ('Calculating...', fraction) 
        def progress(self):
                return self.exec_pyshm_method ('progress', ())

        #  Add Layerinformation to active scan. gxsm.add_layerinformation('Text',ch)
        def add_layerinformation(self, text,ch):
                return self.exec_pyshm_method ('add_layerinformation', (text,ch,))

        #  Da0. -- N/A for SRanger -- obsoleted
        def da0(self, ch):
                return self.exec_pyshm_method ('da0', (ch,))

        #  Generic Action Signal-Emit to GUI: Action-String. 
        def signal_emit(self, action):
                return self.exec_pyshm_method ('signal_emit', (action,))

        #  gxsm.sleep (N/10 s) -- obsoleted, use python time.sleep(sec)!
        def sleep(self, tenths):
                return self.exec_pyshm_method ('sleep', (tenths,))

        #  gxsm.set_sc_label (id [1..8],'value as string') Manage Scan Control Entry Labels
        def set_sc_label(self, id, text):
                return self.exec_pyshm_method ('set_sc_label', (id, text,))


        ########### RPSPMC ONLY
        # Read complete RPSPMC HwI Monitoring Block
        def read_status(self):
                try:

                        # XYZ MAX MIN (3x3)
                        #memcpy  (shm_ptr, spmc_signals.xyz_meter, sizeof(spmc_signals.xyz_meter));
                        #           0,1,2,  3,4,5,  6,7,8,

                        # Monitors: Bias, reg, set,   GPVU,A,B,AM,FM, MUX, Signal (Current), AD463x[2], XYZ, XYZ0, XYZS
                        #           10,    11,  12,    13, 14,15,16,17, 18, 19,               20, 21,    22,  23,   24
                        #memcpy  (shm_ptr+sizeof(spmc_signals.xyz_meter), &spmc_parameters.bias_monitor, 21*sizeof(double));

                        # PAC-PLL Monitors: dc-offset, exec_ampl_mon, dds_freq_mon, dds_dfre, volume_mon, phase_mon, control_dfreq_mon
                        #memcpy  (shm_ptr+sizeof(spmc_signals.xyz_meter)+21*sizeof(double), &pacpll_parameters.dc_offset, 6*sizeof(double));
                        #                   40,        41,            42,           43,        44,          45,        46

                        # Z-Servo Info: mode, setpoint, cp, ci, cp_db, ci_db, upper, lower, setpoint_cz, level, in_offcomp, src_mux
                        #memcpy  (shm_ptr+50*sizeof(double), &spmc_parameters.z_servo_mode, 12*sizeof(double));

                        # Scan Info: alpha, slope-dzx,dzy,slew, scanpos-x,y,slew,opts, offset-x,y,z
                        #memcpy  (shm_ptr+70*sizeof(double), &spmc_parameters.alpha, 13*sizeof(double));

                        # FPGA RPSPMC uptime in sec, 8ns resolution -- i.e. exact time of last reading
                        #memcpy  (shm_ptr+100*sizeof(double), &spmc_parameters.uptime_seconds, sizeof(double));

                        gxsm_shares=np.ndarray((101), dtype=np.double, buffer=self.gxsm_shm.buf) # flat array all shares 
                        xyz=np.ndarray((9,), dtype=np.double, buffer=self.gxsm_shm.buf).reshape((3,3)).T  # X Mi Ma, Y Mi Ma, Z Mi Ma
                        self.XYZ_monitor['monitor']=xyz[0]
                        self.XYZ_monitor['monitor_max']=xyz[1]
                        self.XYZ_monitor['monitor_min']=xyz[2]
                        print ()
                        print ('rpspmc XYZ monitors:')
                        print (self.XYZ_monitor)

                        self.rpspmc['bias']         = gxsm_shares[10]
                        self.rpspmc['current']      = gxsm_shares[19]
                        self.rpspmc['gvp']['u']     = gxsm_shares[13]
                        self.rpspmc['pac']['ampl']  = gxsm_shares[44]
                        self.rpspmc['pac']['dfreq'] = gxsm_shares[43]

                        self.rpspmc['zservo']['mode']        = gxsm_shares[50]
                        self.rpspmc['zservo']['setpoint']    = gxsm_shares[51]
                        self.rpspmc['zservo']['cp']          = gxsm_shares[52]
                        self.rpspmc['zservo']['ci']          = gxsm_shares[53]
                        self.rpspmc['zservo']['cp_db']       = gxsm_shares[54]
                        self.rpspmc['zservo']['ci_db']       = gxsm_shares[55]
                        self.rpspmc['zservo']['upper']       = gxsm_shares[56]
                        self.rpspmc['zservo']['lower']       = gxsm_shares[57]
                        self.rpspmc['zservo']['setpoint_cz'] = gxsm_shares[58]
                        self.rpspmc['zservo']['level']       = gxsm_shares[59]
                        self.rpspmc['zservo']['in_offcomp']  = gxsm_shares[60]
                        self.rpspmc['zservo']['src_mux']     = gxsm_shares[61]
                        
                        self.rpspmc['scan']['alpha']         = gxsm_shares[70]
                        self.rpspmc['scan']['slope']['dzx']  = gxsm_shares[71]
                        self.rpspmc['scan']['slope']['dzy']  = gxsm_shares[72]
                        self.rpspmc['scan']['slope']['slew'] = gxsm_shares[73]
                        self.rpspmc['scan']['x']             = gxsm_shares[74]
                        self.rpspmc['scan']['y']             = gxsm_shares[75]
                        self.rpspmc['scan']['slew']          = gxsm_shares[76]
                        self.rpspmc['scan']['opts']          = gxsm_shares[77]
                        self.rpspmc['scan']['offset']['x']   = gxsm_shares[78]
                        self.rpspmc['scan']['offset']['y']   = gxsm_shares[79]
                        self.rpspmc['scan']['offset']['z']   = gxsm_shares[80]
                        #self.rpspmc['scan']['offset']['xy_slew'] = gxsm_shares[81]
                        #self.rpspmc['scan']['offset']['z_slew']  = gxsm_shares[82]
                        
                        self.rpspmc['time']         = gxsm_shares[100]
                        print ()
                        print ('All RPSPMC Control Monitors:')
                        print (self.rpspmc)
                        print ()
                        print ("rpspmc['zservo'] Z-Servo Monitors:")
                        print (self.rpspmc['zservo'])
                        print ()
                        print ("rpspmc['scan'] Scan Geom Monitors:")
                        print (self.rpspmc['zservo'])
                        print ()
                        
                except AttributeError:
                        # just try open again
                        self.get_gxsm4rpspmc_shm_block()

        def rt_query_xyz(self):
                try:
                        gxsm_shares=np.ndarray((101), dtype=np.double, buffer=self.gxsm_shm.buf) # flat array all shares 
                        xyz=np.ndarray((9,), dtype=np.double, buffer=self.gxsm_shm.buf).reshape((3,3)).T  # X Mi Ma, Y Mi Ma, Z Mi Ma
                        self.XYZ_monitor['monitor']=xyz[0]
                        self.XYZ_monitor['monitor_max']=xyz[1]
                        self.XYZ_monitor['monitor_min']=xyz[2]

                except AttributeError:
                        # just try open again
                        self.get_gxsm4rpspmc_shm_block()

                return self.XYZ_monitor

        def rt_query_rpspmc(self):
                try:
                        gxsm_shares=np.ndarray((101), dtype=np.double, buffer=self.gxsm_shm.buf) # flat array all shares 
                        self.rpspmc['bias']         = gxsm_shares[10]
                        self.rpspmc['current']      = gxsm_shares[19]
                        self.rpspmc['gvp']['u']     = gxsm_shares[13]
                        self.rpspmc['pac']['ampl']  = gxsm_shares[44]
                        self.rpspmc['pac']['dfreq'] = gxsm_shares[43]
                        self.rpspmc['time']         = gxsm_shares[100]
                
                except AttributeError:
                        # just try open again
                        self.get_gxsm4rpspmc_shm_block()

                return self.rpspmc


        def action(self, code=-1):
                try:
                        self.rt_query_rpspmc()
                        shm_action_control = np.frombuffer(self.gxsm_shm.buf, dtype=np.uint64, count=2, offset=8*SHM_ACTION_OFFSET)
                        print ('action ** ', shm_action_control[0], code, ' ** RPSPMC time: {}s'.format(self.rpspmc['time']))
                        if shm_action_control[0] == 0:
                                if code > 0:
                                        shm_action_control[0] = code
                                        return SHM_OK
                                else:
                                        return shm_action_control[0]
                        else:
                                print ('BUSY')
                                return SHM_E_BUSY
                                
                except AttributeError:
                        # just try open again
                        self.get_gxsm4rpspmc_shm_block()
                        return SHM_E_NA
                        
        
        ## SHM GXSM4/RPSPMC ACTIONS
        def set_current_level_zcontrol (self):
                if self.blocking_requests:
                        while self.action() == SHM_E_BUSY:
                                self.rt_query_rpspmc()
                                print ('BUSY, waiting, RPSPMC time: {}s'.format(self.rpspmc['time']))
                                time.sleep(0.02)
                return self.action(SHM_ACTION_CTRL_Z_SET)
               
                    
        def set_current_level_0 (self):
                if self.blocking_requests:
                        while self.action() == SHM_E_BUSY:
                                self.rt_query_rpspmc()
                                print ('BUSY, waiting, RPSPMC time: {}s'.format(self.rpspmc['time']))
                                time.sleep(0.02)
                return self.action(SHM_ACTION_CTRL_Z_FB)

        def set_rpspmc (self, id, value):
                try:
                        if self.blocking_requests:
                                while self.action() == SHM_E_BUSY:
                                        self.rt_query_rpspmc()
                                        print ('BUSY, waiting, RPSPMC time: {}s'.format(self.rpspmc['time']))
                                        time.sleep(0.02)
                        if self.action() == SHM_ACTION_IDLE:
                                control_shm_vpar = np.frombuffer(self.gxsm_shm.buf, dtype=np.double, count=SHM_NUM_VPAR, offset=8*SHM_ACTION_VPAR_64_START)
                                control_shm_id   = self.gxsm_shm.buf[8*SHM_ACTION_FP_ID_STRING_64 : 8*SHM_ACTION_FP_ID_STRING_64+len(id)+1]
                                id_bytes = id.encode('utf-8') + b'\x00'
                                control_shm_id[:] = id_bytes # set id
                                control_shm_vpar[0] = value # value
                                ret = self.action (SHM_ACTION_SET)
                                if self.blocking_requests:
                                        while self.action() == SHM_E_BUSY:
                                                self.rt_query_rpspmc()
                                                print ('waiting, RPSPMC time: {}s'.format(self.rpspmc['time']))
                                                time.sleep(0.02)
                                print ('ACTION GXSM.SET: ', id, ' set to ', value)
                                return -1
                        else:
                                return 0
                except AttributeError:
                        # just try open again
                        self.get_gxsm4rpspmc_shm_block()

        def get_rpspmc (self, id):
                try:
                        if self.blocking_requests:
                                while self.action() == SHM_E_BUSY:
                                        self.rt_query_rpspmc()
                                        print ('BUSY, waiting, RPSPMC time: {}s'.format(self.rpspmc['time']))
                                        time.sleep(0.02)
                        if self.action() == SHM_ACTION_IDLE:
                                control_shm_vpar = np.frombuffer(self.gxsm_shm.buf, dtype=np.double, count=SHM_NUM_VPAR, offset=8*SHM_ACTION_VPAR_64_START)
                                control_shm_fret = np.frombuffer(self.gxsm_shm.buf, dtype=np.double, count=SHM_NUM_FRET, offset=8*SHM_ACTION_FRET_64_START)
                                control_shm_id   = self.gxsm_shm.buf[8*SHM_ACTION_FP_ID_STRING_64 : 8*SHM_ACTION_FP_ID_STRING_64+len(id)+1]
                                id_bytes = id.encode('utf-8') + b'\x00'
                                control_shm_id[:] = id_bytes # get id
                                control_shm_fret[0] = 0 # sanity cleanup -- not required
                                ret = self.action (SHM_ACTION_GET)
                                while self.action() == SHM_E_BUSY:
                                        self.rt_query_rpspmc()
                                        print ('waiting, RPSPMC time: {}s'.format(self.rpspmc['time']))
                                        time.sleep(0.02)
                                print ('ACTION GXSM.GET: ', id, ' value=', control_shm_fret[0])
                                return control_shm_fret[0]
                        else:
                                return -1e999
                except AttributeError:
                        # just try open again
                        self.get_gxsm4rpspmc_shm_block()
                        return -2e999
                




                
