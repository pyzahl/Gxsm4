# Gxsm Python Script file /home/percy/.gxsm4/pyaction/gxsm4-control-watchdog.py was created.
import time
import requests


class THV5():
        def __init__(self, ip):
                self.ip = ip

        def request(self, data):
                print (self.ip + data)
                return requests.get(self.ip + data)

        def THVCoarseMove(self, channel, direction, burstcount, period, voltage, start):
                #c=['X','Y','Z']
                if start:
                        self.request ('coarse?v0={}&p0={}&a0={}&c0=0'.format(int(voltage), int(1000*period), channel))
                        return self.request ('coarse?v={}&p0={}&a0={}&c0={}&bs=0'.format(int(voltage), int(1000*period), channel, burstcount*direction))
                else:
                        return self.request ('coarse?v={}&p0={}&a0={}&c0={}&bs=0'.format(0, 0, c[channel], 0))

        def TipDownApp(self, n):
                 self.THVCoarseMove('Z', -1, n, 0.5, 30,  True)
                 #time.sleep(3) ## HV Glitch wait up

""" 

### Gxsk Remote Controls for Autoapproach

# in HV Volts
def VZ_tip():
        svec=gxsm.rtquery ("z") 
        return -svec[0]

# dFreq in Hz
def dFreq():
        fvec=gxsm.rtquery ("f")
        return fvec[0]


def test_rt_param_query ():
        fvec=gxsm.rtquery ("f")
        df = fvec[0]
        print ("dF=",df)
        zvec=gxsm.rtquery ("z")
        print ('Zvec=', zvec)
        zvec=gxsm.rtquery ("z0")
        print ('Z0vec=', zvec)
        gxsm.logev("Watchdog Check dF=%gHz" %df + " zvec=%g A" %zvec[0])
        print ("Watchdog Abort")



def UserOK():
	return int(gxsm.get('script-control')) > 0

def retract():
	print('Tip Up')
	gxsm.set('dsp-fbs-mx0-current-set','0')
	while VZ_tip() > -100 and UserOK():
		print('^ ... dFreq: {:6.2f} Hz,  Z= {:6.1f} V'.format(dFreq(), VZ_tip()))
		time.sleep(0.5)

def checkRange(I=0.03, to=10):
	print('Tip Down check, timeout = {}s'.format(to))
	gxsm.set('dsp-fbs-mx0-current-set','{}'.format(I))
	tc=0.0
	while VZ_tip()  < 100 and tc < to and UserOK():
		print('v ... dFreq: {:6.2f} Hz,  Z= {:6.1f} V ... {}s'.format(dFreq(), VZ_tip(), to-tc))
		time.sleep(0.5)
		tc=tc+0.5
	return VZ_tip() < 100


def run_auto_approach(I=0.03, nsteps=1):
	count=0
	print ('Start Tip Auto Approach Procedure')
	print ('VZ = ', VZ_tip(), 'V')

	while not checkRange(I, 30) and abs(dFreq()) < 5 and UserOK():
		print('*** dFreq: {:6.2f} Hz,  Z= {:6.1f} V,  #={}'.format(dFreq(), VZ_tip(), count))
		retract()
		thv.TipDownApp(nsteps)
		print('***>>> {} step(s) down dFreq: {:6.2f} Hz,  Z= {:6.1f} V,  #={}'.format(nsteps,dFreq(), VZ_tip(), count))
		count = count + nsteps
		time.sleep(0.5)
		if abs(dFreq()) > 5:
			print (abs(dFreq()), 'Hz, waiting...')
			time.sleep(1.5)
		print (abs(dFreq()), 'Hz')
	print('*** Finished Approach or Safety Abort or Timeout *** dFreq: {:6.2f} Hz,  Z= {:6.1f} V,  #steps={}'.format(dFreq(), VZ_tip(), count))



#################

gxsm.set('script-control', '1') # ENABLE Script Control/Abort Entry

"""

thv=THV5('http://192.168.40.10/')

### TEST HV STEPPING

for d in ['Z']: #]['X', 'Y', 'Z']:
	for V in [20,40]:
		print (f'Testing {d} @ {V}V')
		thv.THVCoarseMove(d, 1, 1, 1, V, True)
		time.sleep(0.5)
		thv.THVCoarseMove(d, 1, 1000, 1, V, True)
		time.sleep(1)
		thv.THVCoarseMove(d, -1, 1000, 1, V, True)
		time.sleep(2)
	time.sleep(3)

##### GXSM Auto App Start:
"""
test_rt_param_query ()

print (abs(dFreq()), 'Hz')

run_auto_approach(0.013)

print ("Finished/Aborted.")
"""
