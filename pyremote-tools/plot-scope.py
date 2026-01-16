#!/usr/bin/python3
import time
import numpy as np
import matplotlib.pyplot as plt
#from scipy.signal import butter, freqz, ellip, iirnotch, tf2sos
#from scipy.interpolate import BSpline, make_interp_spline, make_smoothing_spline
import math

# connect to gxsm4 process and start wrapper class
import gxsm4process as gxsm4
gxsm = gxsm4.gxsm_process(1)
	
#for h in gxsm.list_actions ():
#	print (h)
        
print ('Scope Data:')
#gxsm.action('DSP_VP_VP_EXECUTE')

#gxsm.action('EXECUTE_ScopeSaveData')
gxsm.action('EXECUTE_FirePulse')
time.sleep(2)

gxsm.action('EXECUTE_ScopeSaveData')

scope, info = gxsm.action('GET_RPDATA_VECTOR')

print (scope, info)
Y12 = info.split(',')

# fetch vpdata of last probe -- if exists, else error
#print ('*** Getting last vpdata set from master scan in ch=0 ***')
#columns, labels, units, xy = gxsm.get_probe_event(0,-1)  # ch=0, get last VPdata set

## Grab RP Scope Data
print ('*** Getting last RP Scope data ***')
columns, labels, units, xy = (scope, ['Time-Mon', Y12[0].strip(), Y12[1].strip()],['ms', 'V','V'],[0,0,0])

# zip together for convenient data access
vpdata  = dict (zip (labels, columns[:])) ## cut off points 0..100 (initial ramp points)
vpunits = dict (zip (labels, units))

# we got:
print ('VP Data     @XY:', xy)
print ('VP Units[Sets] :', vpunits)
print ('Set Size       :', vpdata[labels[0]].shape)

#VP Units[Sets] : {'ZS-Topo': 'Å', 'Current': 'nA', 'dFrequency': 'Hz', 'Time-Mon': 'ms'}

# setup what to print
x='Time-Mon'
#y='ZS-Topo'
#y='dFrequency'
y='Current'
y='IN1'
y2='IN2'

# Create VPDATA plot
plt.figure(figsize=(6, 4))
#plt.title('VP-Data Plot')
plt.title('RP Scope Plot')
plt.plot (vpdata[x], vpdata[y], '-', alpha=0.8, label=y)
plt.plot (vpdata[x], vpdata[y2], '-', alpha=0.8, label=y2)
plt.xlabel('{} in {}'.format(x, vpunits[x]))
plt.ylabel('{} in {}'.format(y, vpunits[y]))

#plt.ylim (-1, 1)
#plt.xlim (-0.1, 0.2)
plt.legend()
plt.grid()
plt.show()
#plt.savefig('/tmp/vpdata-plot-demo.png')
