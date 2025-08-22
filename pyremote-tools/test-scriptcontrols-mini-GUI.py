import numpy as np
import time

# test "manually"
print('main SC:', gxsm.get('script-control'))
gxsm.set('script-control', '3')

print('py-sc01: ', gxsm.get('py-sc01'))
gxsm.set('py-sc01', '1.01')

print('py-sc02: ', gxsm.get('py-sc02'))
gxsm.set('py-sc02', '2.02')

gxsm.set_sc_label('py-sc02', 'SuperSC')

gxsm.set_sc_label('py-sc04', 'Molecule#')

# Test via little helper dict
# Up to 10
#sc = dict(Points=330,  Molecule=123,  Iref=40, Zdown=-0.6, Zstart=-0.1)
sc = dict(STM_Range=70, AFM_Range=45,  Molecule=1,  I_ref=40, 
		     CZ_Level=0.1,  Z_down=-0.6, Z_start=-0.1, Wait=1)

print(sc)

# Setup SCs
def SetupSC():
	for i, e in enumerate(sc.items()):
		id='py-sc{:02d}'.format(i+1)
		print (id, e[0], e[1])
		gxsm.set_sc_label(id, e[0])
		gxsm.set(id, '{:.4f}'.format(e[1]))

# Reset and update SCs to defaults -- demo
def SetSC():
	for i, e in enumerate(sc.items()):
		id='py-sc{:02d}'.format(i+1)
		gxsm.set(id, '{:.4f}'.format(e[1]))

SetupSC()

# Read / Update dict
def GetSC():
	for i, e in enumerate(sc.items()):
		id='py-sc{:02d}'.format(i+1)
		print (id, ' => ', e[0], e[1])
		sc[e[0]] = float(gxsm.get(id))
		print (id, '<=', sc[e[0]])

GetSC()
print (sc)

print('Waiting as long as Wait > 0...')

while sc['Wait']>0:
	time.sleep(0.5)
	GetSC()

sc['Points']=400




sc = dict(SC1=0, SC2=0,SC3=0, SC4=0,SC5=0, SC6=0,SC7=0, SC8=0)
SetupSC()


sc = dict(dFreq=0.0, It=0.0,MIX1=0.0, MIX2=0.0,MIX3=0.0, Wait=1)
SetupSC()

while sc['Wait']>0:
	dfi=gxsm.rtquery ("f") # dFreq, I-avg, I-RMS!
	mixer123=gxsm.rtquery ("M") # in Volts!!!
	M=np.array(mixer123)
	sc['dFreq']=dfi[0]
	sc['It']=dfi[1]
	sc['MIX1']=M[0]
	sc['MIX2']=M[1]
	sc['MIX3']=M[2]
	SetSC()
	time.sleep(0.5)
	GetSC()





#mixer123=gxsm.rtquery ("m99") # update signal mapping table
#for i in range(0,22):
#	# "mNN" :              MonitorSignal[NN] -- Monitor Singals at index SigMon[NN]: [Scaled Signal in Unit, raw signal, scale, ret=unit-string] (NN = 00 ... 22), 99: reload signal mapping
#	mon=gxsm.rtquery ("m{:02d}".format(i)) # as mapped via signal table
#	print(i, mon)





SetSC()
sc = dict(SC1=0, SC2=0,SC3=0, SC4=0,SC5=0, SC6=0,SC7=0, SC8=0)
SetupSC()


