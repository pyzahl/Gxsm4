#!/usr/bin/python3
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib import style
from scipy.special import i0

#from scipy.signal import butter, freqz, ellip, iirnotch, tf2sos
#from scipy.interpolate import BSpline, make_interp_spline, make_smoothing_spline
import math

import time

# connect to gxsm4 process and start wrapper class
import gxsm4process as gxsm4
gxsm = gxsm4.gxsm_process()

bias=gxsm.get ("dsp-fbs-bias") # save Bias
ci=gxsm.get ("dsp-fbs-ci")
cp=gxsm.get ("dsp-fbs-cp")
#gxsm.set ("dsp-fbs-ci","{}".format(ci*1.2)) # slow
#gxsm.set ("dsp-fbs-cp","{}".format(cp*1.2)) # slow
#gxsm.set ("dsp-fbs-bias","-3") # Pulse
time.sleep(0.05)
#gxsm.set ("dsp-fbs-bias","{}".format(bias)) # restore Bias
#gxsm.set ("dsp-fbs-ci","{}".format(ci)) # restore Bias
#gxsm.set ("dsp-fbs-cp","{}".format(cp)) # restore Bias

ZAV=1.

wait=4000

N=40
MM=5
M=2

def dz_func(ampl, k=1.0):
        return 1.0/(2*k)*np.log(i0(2*k*ampl)) # in Ang


def measure_z (M):
        z=0.0
        for q in range(0,M):
                z=z+gxsm.get('dsp-adv-dsp-zpos-mon')
        return z/M

def measure_dz (mv):
        gxsm.set ("rp-pacpll-AMPLITUDE-FB-SETPOINT","{}".format(0.0))
        time.sleep(wait)
        z0=measure_z (M)

        gxsm.set ("rp-pacpll-AMPLITUDE-FB-SETPOINT","{}".format(mv))
        time.sleep(wait)

        z=measure_z (M)
        dz=100*(z-z0)
        return dz,z,z0

def measure_dzpmv (zprev, mvnew):
        z=measure_z (M)
        dz=100*(z-zprev)
        gxsm.set ("rp-pacpll-AMPLITUDE-FB-SETPOINT","{}".format(mvnew))
        return dz,z


z = np.zeros(N*MM)
dz = np.zeros(N*MM)
x = np.zeros(N*MM)

print ('Setup:')
for k in range(0, N):
        for m in range (0,MM):
                i=k*MM + m
                mv = k*0.25
                if i%2 == 0:
                        x[i]=0.0
                else:
                        x[i]=mv
                #dz[i], z[i], z0 = measure_dz (mv)
                #print ('#{} of {}:  {} mV, dz: {} pm @ z={}, z0={} Ang'.format(i, N*MM, mv, dz[i], z[i], z0))
                print ('#{} of {}:  {} mV'.format(i, N*MM, mv))

np.save('dz-of-ampl.npy', [x, z, dz])


gxsm.set ("rp-pacpll-AMPLITUDE-FB-SETPOINT","{}".format(0.0))
time.sleep(2)
z[0]=measure_z (M)

# Create VPDATA plot

fig=plt.figure()
#plt.figure(figsize=(6, 4))
ax1 = fig.add_subplot(1, 1, 1)


def animate(i):
        if (i < N*MM):
                #dz[i], z[i], z0 = measure_dz (x[i])
                if i < N*MM-1 and i > 1:
                        dz[i], z[i] = measure_dzpmv (z[i-1], x[i+1])
                else:
                        dz[i], z[i] = measure_dzpmv (z[0], 0.0)
                if i%2 == 0 and i > 1:
                        dz[i] = -dz[i]
                        x[i]  = x[i-1]
                print ('#{} of {}:  {} mV, dz: {} pm @ z={} Ang'.format(i, N*MM, x[i], dz[i], z[i]))


                np.save('dz-of-ampl.npy', [x, dz, z])
        
                ax1.clear()

                k=1.0
                if x[i] > 0:
                        ff1 =      dz[i]/(100*dz_func(x[i],k))
                        ff2 = ff1 *dz[i]/(100*dz_func(x[i]*ff1,k))
                        ff  = ff2 *dz[i]/(100*dz_func(x[i]*ff2,k))
                        ax1.plot (x[0:i], 100*dz_func (x[0:i]*ff,k))

                ax1.plot (x[0:i], dz[0:i], 'x') #, label='{.2}'.format(dz))
                ax1.set_title('Amplitude Calibration Plot dz (mV)', k={}, ff={}'.format(k,ff)))
                ax1.set_xlabel('{} in {}'.format("Exec", "mV"))
                ax1.set_ylabel('{} in {}'.format("dz", "pm"))


ani = animation.FuncAnimation(fig, animate, interval=wait) # interval in milliseconds (1000ms = 1 second)

#plt.ylim (-1, 1)
#plt.xlim (-0.1, 0.2)
#plt.legend()
plt.grid()
plt.show()


