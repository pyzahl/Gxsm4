#!/usr/bin/env python3
import threading
import gxsm4process as gxsm4

gxsm = gxsm4.gxsm_process(use_asyncio=True)


def flood(name):
        id2 = 'dsp-fbs-bias'
        id = 'dsp-adv-dsp-zpos-ref'

        gxsm.set (id2, 1.0)

        vr = gxsm.get (id)

        print (name, ' FLOOD TEST ',id, vr)

        for i in range (0, 10000):
                v = gxsm.get (id)
                if i % 1000 == 500:
                        gxsm.set (id2, 1e-3*(i-5000))
                if v != vr:
                        print (name, ' EE:',i,v,vr)
        print (name, ' FLOOD TEST completed')

gxsm.rt_query_rpspmc()
print ('Setpoint: ', gxsm.rpspmc['zservo']['setpoint'])
print ('RPSPMC:', gxsm.rpspmc)
        
tA = threading.Thread(target=flood, args=('A',))
tA.start()

tB = threading.Thread(target=flood, args=('B',))
tB.start()

print ('TEST COMPLETED')                
