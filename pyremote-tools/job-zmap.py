import numpy as np
import time
from datetime import datetime

sc = dict(Z_down=1.0, Z_now=0.0, ASC=0.0, ScanSpeed=12.0)

# Setup SCs
def SetupSC():
	for i, e in enumerate(sc.items()):
		id='py-sc{:02d}'.format(i+1)
		#print (id, e[0], e[1])
		gxsm.set_sc_label(id, e[0])
		gxsm.set(id, '{:.4f}'.format(e[1]))

SetupSC()

# Read / Update dict
def GetSC():
	for i, e in enumerate(sc.items()):
		id='py-sc{:02d}'.format(i+1)
		#print (id, ' => ', e[0], e[1])
		sc[e[0]] = float(gxsm.get(id))
		#print (id, '<=', sc[e[0]])

# Update SCs
def SetSC():
	for i, e in enumerate(sc.items()):
		id='py-sc{:02d}'.format(i+1)
		gxsm.set(id, '{:.4f}'.format(e[1]))





def exit_force_map(bias=0.2, current=0.02):
	gxsm.set ("dsp-adv-scan-fast-return","1")
	gxsm.set ("dsp-fbs-mx0-current-set","%f"%current)
	gxsm.set ("dsp-fbs-mx0-current-level","0.00")
	#gxsm.set ("dsp-fbs-ci","35")
	#gxsm.set ("dsp-fbs-cp","40")
	gxsm.set ("dsp-fbs-scan-speed-scan","250")
	gxsm.set ("dsp-fbs-bias","%f" %bias)

def auto_afm_scanspeed(y):
	ms = gxsm.get_slice(2, 0,0, y,1) # ch, v, t, yi, yn   ## AFM dFreq in CH3
	med = np.median(ms)
	dFspan = np.max(ms) - np.min(ms)
	if dFspan > 1.2:
		gxsm.set ("dsp-adv-scan-fast-return","4")
		time.sleep(1)
		gxsm.set ("dsp-fbs-scan-speed-scan","5")
	elif dFspan > 1.0:
		gxsm.set ("dsp-adv-scan-fast-return","3")
		time.sleep(1)
		gxsm.set ("dsp-fbs-scan-speed-scan","8")
	elif dFspan > 0.7:
		gxsm.set ("dsp-adv-scan-fast-return","3")
		time.sleep(1)
		gxsm.set ("dsp-fbs-scan-speed-scan","12")
	elif dFspan > 0.5:
		gxsm.set ("dsp-adv-scan-fast-return","3")
		time.sleep(1)
		gxsm.set ("dsp-fbs-scan-speed-scan","15")
	elif dFspan > 0.4:
		gxsm.set ("dsp-adv-scan-fast-return","2")
		time.sleep(1)
		gxsm.set ("dsp-fbs-scan-speed-scan","20")
	else:
		gxsm.set ("dsp-adv-scan-fast-return","1")
		time.sleep(1)
		gxsm.set ("dsp-fbs-scan-speed-scan","20")
	#print('Median: ', np.median(ms))
	#print('Min: ', np.min(ms))
	#print('Max: ', np.max(ms))
	#print('Range: ', np.max(ms) - np.min(ms))



#GetSC()
#SetSC()

gxsm.set('script-control','2')
sc0=gxsm.get('script-control')

#gxsm.action('UNCHECK-SCAN-REPEAT')

z0=gxsm.get ("dsp-adv-dsp-zpos-ref")
####z0=7.6

zlist = np.arange(0.0, 1.0, 0.1)
zlist = np.append(zlist, np.arange(1.0, 2.0, 0.2))
zlist = np.append(zlist, np.arange(2.0, 4.0, 0.25))
zlist = np.append(zlist, np.arange(4.0, 8.0, 0.5))
zlist = np.append(zlist, 9.0)
zlist = np.append(zlist, 10.0)
zlist = np.append(zlist, 11.0)
zlist = np.append(zlist, 12.5)
zlist = np.append(zlist, 15.0)
zlist = np.append(zlist, 20.0)
zlist = np.append(zlist, 25.0)
zlist = np.append(zlist, 30.0)
zlist = np.append(zlist, 2.0)
zlist = np.append(zlist, 1.0)
zlist = np.append(zlist, 0.5)
zlist = np.append(zlist, 0.2)
zlist = np.append(zlist, 0.0)
zlist = np.append(zlist, 10.0)


if (0):
	print("Waiting")
	lp=1
	l=gxsm.waitscan(False)
	while l >= 0 and sc0 > 1: 
		#print ('Line=',l)
		sc0=gxsm.get('script-control')
		l=gxsm.waitscan(False)
		time.sleep(1)
		if l > lp:
			lp=l
			GetSC()
			if sc['ASC'] > 0:
				auto_afm_scanspeed(l)
	z0=z0+0.1
	print("New z0 +0.1 = ", z0)


print (zlist)
print('Z absolut:')
print (zlist+z0)

user_speed=float(gxsm.get ("dsp-fbs-scan-speed-scan"))
sc['ScanSpeed'] = user_speed
SetSC()


for z in zlist:
	if  z>15:
		gxsm.set ("dsp-fbs-scan-speed-scan","30")
	elif  z>8:
		gxsm.set ("dsp-fbs-scan-speed-scan","25")
	elif  z>5:
		gxsm.set ("dsp-fbs-scan-speed-scan","15")
	else:
		GetSC()
		gxsm.set ("dsp-fbs-scan-speed-scan",'{}'.format(sc['ScanSpeed']))
	zz=z0+z
	#zz=z0-z
	sc['Z_now'] = z
	SetSC()
	print(str(datetime.now()) + "Setting Z-Pos/Setpoint = {:8.2f} A".format( zz))
	gxsm.set ("dsp-adv-dsp-zpos-ref", "{:8.2f}".format( zz))
	time.sleep(1)
	print("Start")
	gxsm.startscan()
	time.sleep(15)
	sc0=gxsm.get('script-control')
	l =gxsm.waitscan(False)
	#print ('Line=',l, ', z0=', z0, ', z=',z, ', zz=', zz)
	print("Waiting")
	time.sleep(15)
	lp=1
	while l >= 0 and sc0 > 1 : 
		sc0=gxsm.get('script-control')
		l =gxsm.waitscan(False)
		#print ('Line=',l,lp)
		if l == lp or l == 0: # stopped/completed
			break
		time.sleep(7)
		#print (l)
		if l > lp:
			lp=l
			GetSC()
			if sc['ASC'] > 0:
				auto_afm_scanspeed(l)
	if sc0 < 2:
		break		
	print("Z next in 5s")
	time.sleep(5)
	print("Z next")

#if sc0 <1: 
#	exit_force_map()

#if sc0 > 2: 
#	exit_force_map()


#exit_force_map()

