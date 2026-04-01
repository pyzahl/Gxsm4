import time
import numpy as np
# Example python action script file /home/percy/.gxsm3/pyaction/sf2.py was created.
# - see the Gxsm3 manual for more information
#gxsm.set ("dsp-fbs-mx0-current-set","0.0025") # Set Current Setpoint to 0.01nA
#gxsm.set ("dsp-fbs-bias","0.60") # set Bias to 0.1V
# gxsm.sleep (2)  # sleep for 2/10s
#gxsm.set ("dsp-fbs-scan-speed-scan","400") # Slow for poickup

# PULSE
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

N=10
z = np.zeros(N)

M=100
for i in range(0, N):
	#print ('measuring z-ref #{}'.format(i))
	gxsm.set ("rp-pacpll-AMPLITUDE-FB-SETPOINT","{}".format(0.0))
	time.sleep(4.0)
	z0=0.0
	for q in range(0,M):
		z0=z0+gxsm.get('dsp-adv-dsp-zpos-mon')
	z0=z0/M

	mv = i*1.0
	gxsm.set ("rp-pacpll-AMPLITUDE-FB-SETPOINT","{}".format(mv))
	time.sleep(4.0)

	#print ('measuring z@ {} mV'.format(mv))
	z[i]=0.0
	for q in range(0,M):
		z[i]=z[i]+gxsm.get('dsp-adv-dsp-zpos-mon')
	z[i]=z[i]/M
	print ('{} mV, dz: {} pm'.format(mv, 100*(z[i]-z0)))
