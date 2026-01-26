from datetime import datetime
import time
import numpy as np

sc = dict(MoveWait=3.0, FinishWait=2.0, X=0, Y=0, P=0, Run=1)


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


def run_scan_at_offset(x,y):
	GetSC()
	s = gxsm.rtquery('s') # check RPSPMC GVP status. 5=Reset/completed
	gxsm.waitscan (True)
	time.sleep(sc['FinishWait']) ## tweak as required -- give post scan time to save all data

	# Set Scan Offset
	###gxsm.moveto_scan_xy (x,y) ### Note -- need to check this dedicated one for RPSPMC
	gxsm.set('OffsetX','{}'.format(x))
	gxsm.set('OffsetY','{}'.format(y))
	time.sleep(sc['MoveWait']) ## tweak as require
        
	print ('Starting Scan')
	gxsm.startscan ()

	gxsm.waitscan (True) # if you want to wait at end...
	#time.sleep(5) ## tweak as require for saving data, ....


# init for go
gxsm.set('script-control','1')

def test():
	s = gxsm.rtquery('s')  # check RPSPMC GVP status. 5=Reset/completed
	sf = gxsm.rtquery('W') # check ScanningFlag
	l = gxsm.waitscan(False)

	print ('GVP Status Vector:', s, ' At ScanLine=',l, ' ScanningFlg=', sf[0])

	print ('Waiting for Scan End, Blocking')
	gxsm.waitscan(True)
	print ('Scan Ended/Not Scanning.')

	s = gxsm.rtquery('s') # check RPSPMC GVP status. 5=Reset/completed
	l = gxsm.waitscan(False)
	print ('GVP Status Vector:', s, ' Line=',l)

#test ()

gxsm.moveto_scan_xy (0,0)

count=0	
for x in np.arange (-1000, 1500, 500):
	for y in np.arange (-1000, 1500, 500):
		count=count+1
		print (x,y)
		GetSC()
		sc['X']=x
		sc['Y']=y
		sc['P']=100*count/9
		SetSC()
		if gxsm.get('script-control') > 0 and sc['Run']>0:
			run_scan_at_offset(x,y)
		else:
			break