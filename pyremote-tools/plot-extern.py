#!/usr/bin/python3
import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import butter, freqz, ellip, iirnotch, tf2sos
from scipy.interpolate import BSpline, make_interp_spline, make_smoothing_spline
import math
import time
import matplotlib as mpl

import gxsm4process as gxsm4

gxsm = gxsm4.gxsm_process()

print ('*** Getting last vpdata set from master scan in ch=0 ***')
columns, labels, units, xy = gxsm.get_probe_event(1,-1)  # ch=2, get last VPdata set

vpdata  = dict (zip (labels, columns[:,100:])) ## cut off points 0..100 (initial ramp points)
vpunits = dict (zip (labels, units))

print ('VP Data     @XY:', xy)
print ('VP Units[Sets] :', vpunits)
print ('Set Size       :', vpdata[labels[0]].shape)

# Demo VPDATA plot
plt.figure(figsize=(6, 4))
plt.title('VP-Data Plot Demo')
plt.plot (vpdata['ZS-Topo'], vpdata['dFrequency'], '-', alpha=0.8, label='LockIn-S-DC')
plt.xlabel('Z in ' + vpunits['ZS-Topo'])
plt.ylabel('dF in ' + vpunits['dFrequency'])

#plt.ylim (-1, 1)
#plt.xlim (-0.1, 0.2)
plt.legend()
plt.grid()
plt.show()
#plt.savefig('/tmp/vpdata-plot-demo.png')
