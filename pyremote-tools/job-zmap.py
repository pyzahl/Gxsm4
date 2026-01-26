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

def run_multi_sls_on_rects(ch):
	# marks in scan channel ch
	ch_ref=ch-1
	# scan first 100 objects
	i=0
	s = gxsm.rtquery('s') # check RPSPMC GVP status. 5=Reset/completed
	print ('Start on all Rects, Status: ', s)
	while i<10:
		# Cancel job when SC set to 0
		o=gxsm.get_object(ch_ref, i)
		#print (i,o)
		i=i+1
		if gxsm.get('script-control') < 1:
			print('Script Control: Stop job noted')
			break
		if o[0] == 'Rectangle':
			print (i,o)
			while s[2] != 0 and gxsm.get('script-control') > 0:
				time.sleep(1)
				s = gxsm.rtquery('s') # check RPSPMC GVP status. 5=Reset/completed
				print ('Status: ', s)
			gxsm.set('SPMC-sls-xs', '{}'.format(o[1]))
			gxsm.set('SPMC-sls-ys', '{}'.format(o[2]))
			gxsm.set('SPMC-sls-xn', '{}'.format(abs(o[3]-o[1])))
			gxsm.set('SPMC-sls-yn', '{}'.format(abs(o[4]-o[2])))
			time.sleep(1.0)
			print ('Starting Scan')
			gxsm.startscan ()
			s = gxsm.rtquery('s') # check RPSPMC GVP status. 5=Reset/completed
			print ('Status: ', s)
			print ('waiting for SLS scan to complete')
			time.sleep(10.0)
			s = gxsm.rtquery('s') # check RPSPMC GVP status. 5=Reset/completed
			print ('Status: ', s)
			while 1:
				time.sleep(2)
				s = gxsm.rtquery('s') # check RPSPMC GVP status. 5=Reset/completed
				#print (s)
				if int(s[2]) == 1:
					continue
				else: ## 5 = done
							break
			print ('Next. Status:', s)
			time.sleep(10)


def run_gvp_on_points(ch, wait_move=5):
	# marks in scan channel ch
	ch_ref=ch-1

	# set to 1 for dummy run
	# set to 2 for IV execute at points
	#gxsm.set('script-control','1')

	dims=gxsm.get_dimensions(ch_ref)
	diffs=gxsm.get_differentials(ch_ref)
	geo=gxsm.get_geometry(ch_ref)
	print (diffs)
	print (geo)
	print (dims)

	# scan first 100 objects
	i=0
	while i<99:
		# Cancel job when SC set to 0
		if gxsm.get('script-control') < 1:
			time.sleep(1)
			print('Script Control: Stop job noted')
			break
		o=gxsm.get_object(ch_ref, i)
		#print (i,o)
		i=i+1
		if o[0] == 'Point':
			print (i,o)
			print ('Action @', (o[1]-dims[0]/2)*diffs[0], ',', (o[2]-dims[1]/2)*diffs[1])
			gxsm.moveto_scan_xy ((o[1]-dims[0]/2)*diffs[0], -(o[2]-dims[1]/2)*diffs[1])
			print ('Moving, waiting...')
			time.sleep(wait_move)
			print ('Execute')
			#gxsm.action ("DSP_VP_IV_EXECUTE")
			gxsm.action ("DSP_VP_VP_EXECUTE")
			print ('waiting...')
			while 1:
				time.sleep(1)
				s = gxsm.rtquery('s') # check RPSPMC GVP status. 5=Reset/completed
				print (s)
				if int(s[2]) == 5:
					print ('done.')
					break
				print ('waiting for GVP to complete')
			time.sleep(1)



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
zlist = np.append(zlist, np.arange(1.0, 2.6, 0.1))
zlist = np.append(zlist, np.arange(2.6, 4.0, 0.2))
zlist = np.append(zlist, np.arange(4.0, 9.0, 0.5))
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
zlist = np.append(zlist, 20.0)
zlist = np.append(zlist, 30.0)
zlist = np.append(zlist, 50.0)
zlist = np.append(zlist, 1.0)
zlist = np.append(zlist, 10.0)
zlist = np.append(zlist, 200.0)
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


if 0:
	run_multi_sls_on_rects(3)

if 0:
	run_gvp_on_points(3)

if 1:
	count = 0
	for z in zlist:
		count = count + 1
		if  z>15:
			gxsm.set ("dsp-fbs-scan-speed-scan",'{}'.format(20+int(sc['ScanSpeed'])))
		elif  z>8:
			gxsm.set ("dsp-fbs-scan-speed-scan",'{}'.format(10+int(sc['ScanSpeed'])))
		elif  z>5:
			gxsm.set ("dsp-fbs-scan-speed-scan",'{}'.format(5+int(sc['ScanSpeed'])))
		else:
			GetSC()
			gxsm.set ("dsp-fbs-scan-speed-scan",'{}'.format(sc['ScanSpeed']))
		zz=z0+z
		#zz=z0-z
		sc['Z_now'] = z
		SetSC()
		print(str(datetime.now()) + " #" + str(count) + " Setting Z-Pos/Setpoint = {:8.2f} A".format( zz))
		gxsm.set ("dsp-adv-dsp-zpos-ref", "{:8.2f}".format( zz))
		time.sleep(1)
		
		# disabled
		if 1 and count > 2 and z == 0.0:
			print("Starting Fz on points in 10s")
			time.sleep(10)
			run_gvp_on_points(3)

		if 1:
			print ('Multi SLS on CH3 rects, z=',z)
			run_multi_sls_on_rects(3)
		else:
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
				time.sleep(30)
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

