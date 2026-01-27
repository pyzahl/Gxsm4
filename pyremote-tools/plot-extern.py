#!/usr/bin/python3
import numpy as np
import matplotlib.pyplot as plt
#from scipy.signal import butter, freqz, ellip, iirnotch, tf2sos
#from scipy.interpolate import BSpline, make_interp_spline, make_smoothing_spline
import math

# connect to gxsm4 process and start wrapper class
import gxsm4process as gxsm4
gxsm = gxsm4.gxsm_process()

ch = 0
# fetch vpdata of last probe -- if exists, else error
print ('*** Getting last vpdata set from master scan in ch=',ch)
print( gxsm.get_probe_event(ch,-1) )  # ch=1, -1: get last VPdata set
columns, labels, units, xy = gxsm.get_probe_event(ch,-1)  # ch=1, get last VPdata set
# zip together for convenient data access
vpdata  = dict (zip (labels, columns[:,600:1600])) ## cut off points 0..100 (initial ramp points)
vpunits = dict (zip (labels, units))

# we got:
print ('VP Data     @XY:', xy)
print ('VP Units[Sets] :', vpunits)
print ('Set Size       :', vpdata[labels[0]].shape)
#print ('VPData         :', vpdata)

#VP Units[Sets] : {'ZS-Topo': 'Å', 'Current': 'nA', 'dFrequency': 'Hz', 'Time-Mon': 'ms'}

# setup what to print
#x='Time-Mon'
x='ZS-Topo'
y='dFrequency'

# Create VPDATA plot
plt.figure(figsize=(6, 4))
plt.title('VP-Data Plot')
plt.plot (vpdata[x], vpdata[y], '-', alpha=0.8, label=y)
plt.xlabel('{} in {}'.format(x, vpunits[x]))
plt.ylabel('{} in {}'.format(y, vpunits[y]))

#plt.ylim (-1, 1)
#plt.xlim (-0.1, 0.2)
plt.legend()
plt.grid()
plt.show()
#plt.savefig('/tmp/vpdata-plot-demo.png')
