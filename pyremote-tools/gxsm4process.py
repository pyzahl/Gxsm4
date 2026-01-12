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
                        'bias': 0.0,    # Volts
                        'current': 0.0, # nA
                        'gvp' : { 'x':0.0, 'y':0.0, 'z': 0.0, 'u': 0.0, 'a': 0.0, 'b': 0.0, 'am':0.0, 'fm':0.0 },
                        'pac' : { 'dds_freq': 0.0, 'ampl': 0.0, 'exec':0.0, 'phase': 0.0, 'freq': 0.0, 'dfreq': 0.0, 'dfreq_ctrl': 0.0 },
                        'time' : 0.0  # exact FPGA uptime. This is the FPGA time of last reading in seconds, 8ns resolution
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
                        
                #if self.gxsm4pid > 0:
                if 0:
                        print ('Gxsm4 PID found: ', self.gxsm4pid)
                        self.noargs = "".encode('utf-8') # Empty PyObject
                        self.get_gxsm4pyshm_shm_block()

                self.get_gxsm4rpspmc_shm_block()
                        
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

                if 0: ## tests
                        v = self.exec_pyshm_method('get', ('dsp-fbs-bias',))
                        print ('Return:', v)
                        
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
                        
        # Read complete Monitoring Block
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

                        # FPGA RPSPMC uptime in sec, 8ns resolution -- i.e. exact time of last reading
                        #memcpy  (shm_ptr+100*sizeof(double), &spmc_parameters.uptime_seconds, sizeof(double));

                        gxsm_shares=np.ndarray((101), dtype=np.double, buffer=self.gxsm_shm.buf) # flat array all shares 
                        xyz=np.ndarray((9,), dtype=np.double, buffer=self.gxsm_shm.buf).reshape((3,3)).T  # X Mi Ma, Y Mi Ma, Z Mi Ma
                        self.XYZ_monitor['monitor']=xyz[0]
                        self.XYZ_monitor['monitor_max']=xyz[1]
                        self.XYZ_monitor['monitor_min']=xyz[2]
                        print (self.XYZ_monitor)

                        self.rpspmc['bias']         = gxsm_shares[10]
                        self.rpspmc['current']      = gxsm_shares[19]
                        self.rpspmc['gvp']['u']     = gxsm_shares[13]
                        self.rpspmc['pac']['ampl']  = gxsm_shares[44]
                        self.rpspmc['pac']['dfreq'] = gxsm_shares[43]
                        self.rpspmc['time']         = gxsm_shares[100]
                        print (rpspmc)
                
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

        def set (self, id, value):
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

        def get (self, id):
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
                




                
