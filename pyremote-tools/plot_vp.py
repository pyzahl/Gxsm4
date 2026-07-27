#!/usr/bin/python3

import numpy as np
import matplotlib.pyplot as plt
import scipy.signal as sig
import glob2
import os
import re
from scipy.signal import savgol_filter
folder="/home/percy/BNLBox2T/Percy-P0/2026-0724-Ag111-ESR-CalPrep"
base="Ag111"

### 10mV AC LockIn on Bias
Zfiles = glob2.glob(folder+"/Ag111-007-VP00[3]-VP.vpdata")               #  0 dBm OFF
Zfiles = Zfiles + glob2.glob(folder+"/Ag111-010-VP00[2345]-VP.vpdata")    # +1 dBm
Zfiles = Zfiles + glob2.glob(folder+"/Ag111-011-VP00[234678]-VP.vpdata")      # +2 dBm  8: at very slow 0.002V/s
Zfiles = Zfiles + glob2.glob(folder+"/Ag111-012-VP00[12]-VP.vpdata")      # +5 dBm
## 0dBm

Info = ['0dBm',#'0dBm',
        '+1dBm','+1dBm','+1dBm','+1dBm',
        '+2dBm','+2dBm','+2dBm','+2dBm','+2dBm','+2dBm',
        '+5dBm','+5dBm',
#        '0dBm',
        ]

#Info = ['RF OFF','RF OFF',
#        'RF 1.97GHz @ +1dBm',
#        'RF 1.97GHz @ +1dBm',
#        ]

print ('File List')
print (Zfiles)
                               
#fig, (ax0, ax1) = plt.subplots(nrows=2, sharex=True)
fig, axis = plt.subplots(nrows=3)

print ('------------------')

once=True
for f,info in zip(Zfiles, Info):
        u=['1']
        l=['i']

        print('processing:')
        basename = os.path.basename(f)
        
        print (basename)
        with open(f, 'r') as file:
                for line in file:
                        if line.startswith('#C Index'):
                                #print(f"Headers: {line.strip()}")
                                headers = re.findall(r'"([^"]*)"', line)
                                #print(headers)

        for h in headers:
                t = h.split()
                l.append(t[0])
                u.append(t[1])

                                
        columns = np.transpose(np.loadtxt (f))

        #print (columns)
        print (l)
        print (u)

        vpdata = dict(zip(l, columns[:, 200:-200]))
        vpunits = dict(zip(l, u))


        xd = 'Bias'
        yd = ['Current', '08-LockIn-Mag', '10-LockIn-Y']
        #yd = ['Current', '08-LockIn-Mag', '12-LockIn-X', '10-LockIn-Y']
        
        for ax, y in zip(axis[0:3], yd):
                ax.legend()
                ax.set_title (y)
                denoised = savgol_filter (vpdata[y], window_length=150, polyorder=3)

                X = sig.decimate(vpdata[xd], 8)
                Y = sig.decimate(denoised, 8)

                #ax.plot (vpdata[xd], vpdata[y], "-", label=f)
                #ax.plot (vpdata[xd], denoised, ".", alpha=0.5,  label=info+basename)
                ax.plot (X, Y, ".", markersize=3, alpha=0.95,  label=info) # label=info+' '+basename)
                ax.set_xlabel (xd + ' in ' + vpunits[xd])
                ax.set_ylabel (y + ' in ' + vpunits[y])
                ax.grid (True)
                ax.legend ()

        if 0: #once:
                denoised = savgol_filter (vpdata['10-LockIn-Y'], window_length=1500, polyorder=3)
                Y = sig.decimate(denoised, 128)

                once=False
                ax = axis[3]
                V   = sig.decimate(vpdata[xd], 16)
                Vo  = -0.075
                Vrf = 0.01
                p   = (V-Vo)/Vrf
                w   = 1.0/(np.pi*Vrf*np.sqrt(1-p*p))
                ax.legend()
                ax.set_title ('Model')
                #ax.plot (V, w, ".", label='w')
                ax.plot (V, Y, ".", label='TestData')
                ax.plot (V, np.convolve(Y,w,'same'), "-", label='Conv.w.Model')
                ax.set_xlabel (xd + ' in ' + vpunits[xd])
                ax.set_ylabel (' Model ')
                ax.grid (True)
                ax.legend ()
                break

                
        if 0:
                #axis[2].plot (vpdata[xd], vpdata[yd[2]], "-", label=yd[2])
                axis[2].plot (vpdata[xd], vpdata[yd[3]], "-", label=yd[3])
                axis[2].legend ()

                data = np.atan2( vpdata[yd[2]], vpdata[yd[3]])
                denoised = savgol_filter (data, window_length=150, polyorder=3)
                axis[3].plot (vpdata[xd], denoised, "-", label='ph')
                axis[3].legend ()

                data = np.sqrt( vpdata[yd[2]] * vpdata[yd[2]] +  vpdata[yd[3]] * vpdata[yd[3]])
                denoised = savgol_filter (data, window_length=150, polyorder=3)      
                axis[1].plot (vpdata[xd], 10*denoised, "-", label='R')

                denoised = savgol_filter (vpdata[yd[1]], window_length=150, polyorder=3)      
                axis[1].plot (vpdata[xd], denoised, "-", label='Rd')
                axis[1].legend ()

        
plt.show ()

