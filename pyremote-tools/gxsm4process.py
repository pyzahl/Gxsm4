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

import faulthandler; faulthandler.enable()

version = "1.0.0"

#import time
#import fcntl
#from threading import Timer

import struct
import array
import math

import numpy as np

from multiprocessing import shared_memory
from multiprocessing.resource_tracker import unregister


class gxsm_process():
        def __init__(self, ip):
                # try open shared memory created by gxsm4 to access RPSPMC's monitors and more

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
                        'pac' : { 'dds_freq': 0.0, 'ampl': 0.0, 'exec':0.0, 'phase': 0.0, 'freq': 0.0, 'dfreq': 0.0, 'dfreq_ctrl': 0.0 }
                }

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
                        print ("SharedMemory(name='gxsm4rpspmc_monitors') not available. Please start gxsm4 and connect RPSPMC.")

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

                        gxsm_shares=np.ndarray((50), dtype=np.double, buffer=self.gxsm_shm.buf) # flat array all shares 
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
                        print (rpspmc)
                
                except NameError:
                        # just try open again
                        self.get_gxsm4rpspmc_shm_block()

        def rt_query_xyz(self):
                try:
                        gxsm_shares=np.ndarray((50), dtype=np.double, buffer=self.gxsm_shm.buf) # flat array all shares 
                        xyz=np.ndarray((9,), dtype=np.double, buffer=self.gxsm_shm.buf).reshape((3,3)).T  # X Mi Ma, Y Mi Ma, Z Mi Ma
                        self.XYZ_monitor['monitor']=xyz[0]
                        self.XYZ_monitor['monitor_max']=xyz[1]
                        self.XYZ_monitor['monitor_min']=xyz[2]

                except NameError:
                        # just try open again
                        self.get_gxsm4rpspmc_shm_block()

                return self.XYZ_monitor

        def rt_query_rpspmc(self):
                try:
                        gxsm_shares=np.ndarray((50), dtype=np.double, buffer=self.gxsm_shm.buf) # flat array all shares 
                        self.rpspmc['bias']         = gxsm_shares[10]
                        self.rpspmc['current']      = gxsm_shares[19]
                        self.rpspmc['gvp']['u']     = gxsm_shares[13]
                        self.rpspmc['pac']['ampl']  = gxsm_shares[44]
                        self.rpspmc['pac']['dfreq'] = gxsm_shares[43]
                
                except NameError:
                        # just try open again
                        self.get_gxsm4rpspmc_shm_block()

                return self.rpspmc


        #### TEST SHM GXSM4/RPSPMC ACTIONS
        def set_current_level_zcontrol (self):
                try:
                        control_shm = np.ndarray((129,), dtype=np.double, buffer=self.gxsm_shm.buf)
                        if control_shm[128] == 0:
                                print ('TEST COPY CURRENT to LEVEL: SHM[128]=', control_shm[128])
                                control_shm[128] = 2
                        else:
                                print ('BUSY')
                except NameError:
                        # just try open again
                        self.get_gxsm4rpspmc_shm_block()
                    
        def set_current_level_0 (self):
                try:
                        control_shm = np.ndarray((129,), dtype=np.double, buffer=self.gxsm_shm.buf)
                        if control_shm[128] == 0:
                                control_shm = np.ndarray((129,), dtype=np.double, buffer=self.gxsm_shm.buf)
                                print ('TEST SET LEVEL to 0: SHM[128]=', control_shm[128])
                                control_shm[128] = 1
                        else:
                                print ('BUSY')
                except NameError:
                        # just try open again
                        self.get_gxsm4rpspmc_shm_block()

        def gxsm_set (self, id, value):
                try:
                        control_shm = np.ndarray((132,), dtype=np.double, buffer=self.gxsm_shm.buf)
                        if control_shm[129] == 0: # check process ready
                                #id = 'dsp-fbs-bias' ### LENGTH MUST BE < 64
                                control_shm_id = gxsm_shm.buf[8*132:8*132+len(id)+1]
                                id_bytes = id.encode('utf-8') + b'\x00'
                                control_shm_id[:] = id_bytes # set id
                                control_shm[131] = value # value
                                control_shm[129] = 1 # request action set
                                print ('TEST GXSM.SET: SHM[129]=', control_shm[129], id, ' set to ', value)
                except NameError:
                        # just try open again
                        self.get_gxsm4rpspmc_shm_block()

        def gxsm_get (self, id):
                try:
                        control_shm = np.ndarray((132,), dtype=np.double, buffer=self.gxsm_shm.buf)
                        if control_shm[130] == 0: # check process ready
                                #id = 'dsp-fbs-bias' ### LENGTH MUST BE < 64
                                control_shm_id = gxsm_shm.buf[8*132:8*132+len(id)+1]
                                id_bytes = id.encode('utf-8') + b'\x00'
                                control_shm_id[:] = id_bytes # get id
                                control_shm[131] = 0 # sanity cleanup -- not required
                                control_shm[130] = 1 # request action get
                                while control_shm[130] == 1:
                                        print ('waiting')
                                        time.sleep(0.1)
                                print ('TEST GXSM.GET: SHM[130]=', control_shm[130], id, ' value=', control_shm[131])
                                return control_shm[131]
                        else:
                                return -1e999
                except NameError:
                        # just try open again
                        self.get_gxsm4rpspmc_shm_block()
                        return -2e999
                


