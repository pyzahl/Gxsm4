#!/usr/bin/env python3

### #!/usr/bin/env python

## * Python Gxsm4 RPSPMC interprocess control libraray
## * provides:
## * PySHM + RPSPMC Monitors
## * External Gxsm4 pyremote extension and compatibility wrapper.
## * 
## * Copyright (C) 2026 Percy Zahl
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

METH_ERR_CODE = ['OK', 'E:RESULT EXCEEDS SHM SIZE', 'E:PySHM Init Error', 'E:PySHM unsupported method flags', 'E:PySHM invalid method name', 'E:PySHM Unknow Error Code']

PYSHM_GXSM_PROCESS_VERSION = '1.0.1'

class gxsm_process():
        # verbose = 0: no messages but startup
        # verbose = 1: basic messages, timing perf info
        # verbose = 2: all messages
        def __init__(self, verbose=0):

                self.verbose = verbose
                
                print ('Gxsm4 Process Class Init. PySHM + RPSPMC Monitors. V {}'.format(self.version()))
                print ('External Gxsm4 pyremote extension and compatibility wrapper')
                print ('Verbose Level: {}'.format(self.verbose))
                print ('***********************************************************')
                
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

                # look for gxsm4 process and open PySHM -- external pyremote interface
                self.set_gxsm4_PID()

                # try open shared memory created by gxsm4 to access RPSPMC's monitors
                # look for gxsm rpspmc SHM monitoring block
                self.get_gxsm4rpspmc_shm_block()

                self.read_status()

        def version(self):
                return PYSHM_GXSM_PROCESS_VERSION

        def debug_print (self, vlevel, *message):
                if self.verbose >= vlevel:
                        print (*message)
        
        # find and set gxsm4 process id (PID) used for signaling or if given use specific PID
        # all PySHM requires one gxsm4 process -- warning: currently undefined behavior in case of multiple gxsm4 instances!
        def set_gxsm4_PID(self, pid=0):
                self.gxsm4pid = pid

                if self.gxsm4pid == 0:
                        # search process list for gxsm4
                        for proc in psutil.process_iter(['pid', 'name']):
                                try:
                                        # Check if process name matches 'gxsm4'
                                        if proc.info['name'] == 'gxsm4':
                                                self.gxsm4pid = proc.info['pid']
                                except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                                        pass

                if self.gxsm4pid > 0:
                        self.debug_print (1, 'gxsm4 PID found: ', self.gxsm4pid)
                        self.noargs = "".encode('utf-8') # Empty PyObject
                        self.get_gxsm4pyshm_shm_block()
                else:
                        self.debug_print (1, 'No gxsm4 PID found. PySHM not available.')
                
        # Open SHM block from Gxsm4
        def get_gxsm4rpspmc_shm_block(self):
                try:
                        self.gxsm_shm = shared_memory.SharedMemory(name='gxsm4rpspmc_monitors')
                        unregister(self.gxsm_shm._name, 'shared_memory')
                        #### do not close externally!!! gxsm4 manages: gxsm_shm.close()
                        self.debug_print (1, 'RPSPMC Monitor SHM: ', self.gxsm_shm)
                except FileNotFoundError:
                        self.debug_print (1, "SharedMemory(name='gxsm4rpspmc_monitors') is not available. Please start gxsm4 and connect RPSPMC.")

        def get_gxsm4pyshm_shm_block(self):
                try:
                        self.gxsm_pyshm = shared_memory.SharedMemory(name='gxsm4_py_shm_data_block')
                        unregister(self.gxsm_pyshm._name, 'shared_memory')
                        #### do not close externally!!! gxsm4 manages: gxsm_pyshm.close()
                        self.debug_print (1, 'PySHM: ', self.gxsm_pyshm)
                        self.init_pyshm_mappings()
                        
                except FileNotFoundError:
                        self.debug_print (1, "SharedMemory(name='gxsm4_py_shm_data_block') is not available. Please start gxsm4.")
                        
        def init_pyshm_mappings(self):
                pyshm_status = np.frombuffer(self.gxsm_pyshm.buf, dtype=np.int64, count=1, offset=100)
                pyshm_status[0] = 0 # clear BUSY
                methods = self.exec_pyshm_method('help', ())
                self.debug_print (1, 'Reading Gxsm4 PySHM methods:')
                for m in methods:
                        self.debug_print (1, m)

                if self.verbose >= 2:
                        self.debug_print (1, 'Test Read of HwI Bias Entry:')
                        v = self.exec_pyshm_method('get', ('dsp-fbs-bias',))
                        self.debug_print (1, 'Return:', v)
                        
                return methods
                
        def exec_pyshm_method(self, method, args):
                self.debug_print (1, 'exec_pyshm_method: {} ({}) '.format(method, args))
                pyshm_status = np.frombuffer(self.gxsm_pyshm.buf, dtype=np.int64, count=1, offset=100)
                #while pyshm_status[0] == 1:
                #        time.sleep(0.01)
                #        self.debug_print (1, '** waiting, busy. ',  pyshm_status[0])
                self.debug_print (2, 'Status: {}'.format(pyshm_status[0]))

                # setup method name and pickled function arguments
                pyshm_method = self.gxsm_pyshm.buf[0:len(method)+1]
                pyshm_object_len = np.frombuffer(self.gxsm_pyshm.buf, dtype=np.uint64, count=1, offset=128)
                method_bytes = method.encode('utf-8') + b'\x00'
                pyshm_method[:] = method_bytes # set method name

                # pickle arguments and copy to SHM
                args_data =  pickle.dumps(args)
                self.debug_print (2, 'args.len: {}'.format(len(args_data)))
                pyshm_object_len[0] = len(args_data) # object size
                self.gxsm_pyshm.buf[128+8 : 128+8+len(args_data)] = args_data

                # mark status as BUSY: this is reset to =0 on the gxsm4 process side to acknlowedge task completion and valid return data in SHM
                pyshm_status[0] = 1

                # signal Gxsm4.PySHM using async kernel signal SIGUSR1
                self.debug_print (2, 'signaling gxsm')
                start_time = time.perf_counter()
                os.kill(self.gxsm4pid, signal.SIGUSR1)

                # wait for gxsm4 acknoleged completion and valid return pickle data in SHM
                wms=0.0
                n=100
                while pyshm_status[0] == 1:
                        if wms > 100:
                                n+=1
                                if n > 100:
                                        n=0
                                        self.debug_print (1, 'waiting {} ** {}'.format(wms, pyshm_status[0]))
                        if pyshm_status[0] == 1:
                                time.sleep (0.0005)
                                wms+=0.5

                end_time = time.perf_counter()
                elapsed_time = 1e3*(end_time - start_time)
                self.debug_print (1, 'Method >gxsm.{}< completed in ~{} ms. Elapsed time from signal: {:.1f} ms'.format(method, wms, elapsed_time))
                                
                if pyshm_status[0] < 0: # -1 ... -4: valid Error Codes
                        self.debug_print (0, 'SHM Method Result Error Code: {} => {}'.format(pyshm_status[0], METH_ERR_CODE[int(min(5,-pyshm_status[0]))]))
                        return
                else:
                        if pyshm_status[0] == 0: # OK
                                self.debug_print (2, 'SHM Method Result is Ready.')
                        else: # ???
                                self.debug_print (0, 'SHM Method Result: Unexpected Completion Code: {}'.format(pyshm_status[0]))

                pyshm_pickles_bytes_len = int(np.frombuffer(self.gxsm_pyshm.buf, dtype=np.uint64, count=1, offset=128))
                self.debug_print (1, 'return data len: {}'.format(pyshm_pickles_bytes_len))
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
	        self.debug_print (1, "        # {}".format(d[-1]))
	        self.debug_print (1, "        def {}(self, {}):".format(d[0][5:-1], args))
	        self.debug_print (1, "                return self.exec_pyshm_method ('{}', ({},))".format(d[0][5:-1], args))
	        self.debug_print (1, )
	
	"""

        ### auto gen wrapper code to be edited and verified.
        ### edited to be python syntax compliant. help, get, set, get_probe tested OK.
        
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

        #  [VX,VY,VZ, V2XAV,V2YAV,V2ZAV]=gxsm.get_instrument_gains ()
        def get_instrument_gains(self):
                return self.exec_pyshm_method ('get_instrument_gains', ())

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


        #################################################
        
        # convenience functions
        def set_current_level_zcontrol (self):
                return self.set('dsp-fbs-mx0-current-level', self.get('dsp-fbs-mx0-current-set'))
                    
        def set_current_level_0 (self):
                return self.set('dsp-fbs-mx0-current-level', 0)

        

        ########### RPSPMC ONLY
        # Read complete RPSPMC HwI Monitoring Block
        def read_status(self):
                try:

                        ### NEW FAST SHM XYZ.. postions:
                        #define VEC___T  0 // Sec FPGA
                        #define VEC___X  1 // 1,2: MAX, MIN peal readings, slow
                        #define VEC___Y  4 // 5,6: MAX, MIN
                        #define VEC___Z  7 // 8,9: MAX, MIN
                        #define VEC___U  10// Bias
                        #define VEC_IN1  11
                        #define VEC_IN2  12
                        #define VEC_IN3  13
                        #define VEC_IN4  14
                        #define VEC__AMPL 15
                        #define VEC__EXEC 16
                        #define VEC_DFREQ 17
                        #define VEC_PHASE 18
                        #define VEC______ 19
        
                        
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
                        xyz=np.ndarray((9,), dtype=np.double, buffer=self.gxsm_shm.buf[8:],).reshape((3,3)).T  # X Mi Ma, Y Mi Ma, Z Mi Ma
                        self.XYZ_monitor['monitor']=xyz[0]
                        self.XYZ_monitor['monitor_max']=xyz[1]
                        self.XYZ_monitor['monitor_min']=xyz[2]
                        self.debug_print (1, )
                        self.debug_print (1, 'rpspmc XYZ monitors:')
                        self.debug_print (1, self.XYZ_monitor)

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
                        self.debug_print (1, )
                        self.debug_print (1, 'All RPSPMC Control Monitors:')
                        self.debug_print (1, self.rpspmc)
                        self.debug_print (1, )
                        self.debug_print (1, "rpspmc['zservo'] Z-Servo Monitors:")
                        self.debug_print (1, self.rpspmc['zservo'])
                        self.debug_print (1, )
                        self.debug_print (1, "rpspmc['scan'] Scan Geom Monitors:")
                        self.debug_print (1, self.rpspmc['zservo'])
                        self.debug_print (1, )
                        
                except AttributeError:
                        # just try open again
                        self.get_gxsm4rpspmc_shm_block()

        def rt_query_xyz(self):
                try:
                        gxsm_shares=np.ndarray((101), dtype=np.double, buffer=self.gxsm_shm.buf) # flat array all shares 
                        xyz=np.ndarray((9,), dtype=np.double, buffer=self.gxsm_shm.buf[8:],).reshape((3,3)).T  # X Mi Ma, Y Mi Ma, Z Mi Ma
                        #xyz=np.ndarray((9,), dtype=np.double, buffer=self.gxsm_shm.buf).reshape((3,3)).T  # X Mi Ma, Y Mi Ma, Z Mi Ma
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







                
