import time
import numpy as np
from datetime import datetime
from datetime import timedelta

r=100  # range in Ang (square)
n=50    # num positions in X and Y (square)
reps = 2# repetitions at each point

m=0 # counter of completed VPs

nskip=0 #n*1+2 # noramlly 0

## VMsize monitoring
##$$  grep VmSize Ev_Mar2022.log | awk '{print $14/1024/1024}' > gxsm-vm
##$$  grep VmSize Ev_Mar2022.log | grep "Mar 28" | awk '{ vms=$14/1024./1024.; ts="\""$2" "$3" "$4"\""; ("date +%s -d "ts)| getline epochsec; print epochsec, vms}' > gxsm-vm; xmgrace gxsm-vm
##$$  grep VmSize Ev_Mar2022.log | grep "Mar 28" | awk '!(NR%120)' | tail -n 2000 | awk '{ vms=$14/1024./1024.; ts="\""$2" "$3" "$4"\""; ("date +%s -d "ts)| getline epochsec; print (epochsec-1648479015)/3600, vms}' > gxsm-vm

## Self tests
## assuming column 5,6 contain X and Y pos for verification
##$$  grep -v \# testrmp033-VP???-VP.vpdata | awk '{print $5, $6}' | awk '!(NR%100)'  >xy33
##$$  grep -v \# /tmp/TESTRASTER/TestXX025-VP???-VP.vpdata | awk '{print $5, $6}' | awk '!(NR%100)'  >/tmp/TESTRASTER/xy; xmgrace /tmp/TESTRASTER/xy&


positions = np.arange(-r/2, r/2+1, r/(n-1)) # raster position list used for X and Y
print ('Positions:')
print (positions)


positionsy = np.arange(25., -25, -5.) # raster position list used for X and Y


now = datetime.now()

print("Current Time, Start =", now,  ' [', now.timestamp(),'s]')
print ('Raster probe at X and Y as listed below, N:', n, ',  repetistion:', reps, ',  total count:', n*n*reps)
print (np.arange(-r/2, r/2+1, r/(n-1)))

exec_probe =  True

gxsm.moveto_scan_xy(0,0)

# dsp_status = int(gxsm.rtquery ("s")[0]) & (8+16+2+4)
#  bits masks for 
# 2+4 :  scan or paused scan
#  +8:  move
# +16: VP 
# should be all clear for next action.
# 1: FB shoud be set except when VP disables it

gxsm.chmodea(0)  # activate
gxsm.set('script-control','1')

gxsm.set('OffsetX', '0')
gxsm.set('OffsetY', '0')


def watch_mixin():
        mixer_vec = gxsm.rtquery ("M")
        print ('Mixer IN = [{} V, {} V, {} V] '.format(mixer_vec[0], mixer_vec[1], mixer_vec[2] ))
        print('INIT')
        #mixer_vec = gxsm.rtquery ("m99") # force signal monitor table reread in case of manua loveride after gxsm start
        while gxsm.get('script-control'):
                mixer_fi = gxsm.rtquery ("f")
                mixer_vec = gxsm.rtquery ("M")
                print ('Mixer IN  [I={:7.5f} nA, dF={:7.5f} Hz, M1={:7.5f} V, M2={:7.5f} V, M3={:7.5f} V] '.format(mixer_fi[1], mixer_fi[0], mixer_vec[0], mixer_vec[1], mixer_vec[2] ))
                #mixer_vec = gxsm.rtquery ("m19")
                #print ('SMON19 = {}  {} '.format(mixer_vec[0], mixer_vec[3]))
                #mixer_vec = gxsm.rtquery ("m20")
                #print ('SMON20 ={}  {} '.format(mixer_vec[0], mixer_vec[3]))
                #mixer_vec = gxsm.rtquery ("m21")
                #print ('SMON21= {}  {} '.format(mixer_vec[0], mixer_vec[3]))
                #mixer_vec = gxsm.rtquery ("m07")
                #print ('SMON07 ={}  {} '.format(mixer_vec[0], mixer_vec[3]))
                time.sleep(0.5)

watch_mixin()


def reset_scan(x=0,y=0):
        print ('issueing 3x scan stop+reset')
        gxsm.stopscan()
        time.sleep(0.2)
        gxsm.stopscan()
        time.sleep(0.2)
        gxsm.stopscan()
        time.sleep(0.2)
        gxsm.moveto_scan_xy(x,y)
        time.sleep(0.2)
        gxsm.stopscan()
        time.sleep(0.2)
        gxsm.stopscan()
        time.sleep(0.2)
        gxsm.stopscan()
        time.sleep(0.2)
        gxsm.moveto_scan_xy(x,y)
        time.sleep(0.2)
        gxsm.set('OffsetX', '1')
        gxsm.set('OffsetX', '0')
        gxsm.set('OffsetY', '0')


# Status check and user abort support utility
def ready_check(msg, x=0,y=0, delay=1.0, tout=60):
        if gxsm.get('script-control') !=1: # User Cancel via Script-Control
                print ('User abort requested')
                return False # abort by user
       
        timeout=datetime.now()+timedelta(seconds = tout)
        dt=timeout-datetime.now()
        s=int(gxsm.rtquery ("s")[0])
        while int(s&(8+16+2+4)) and gxsm.get('script-control') and dt > timedelta(seconds = 0):
                print ('{} * status=0x{:04x} * --> timeout count {} '.format(msg, s, dt))
                if s == 0x45 or s == 0x47:
                        reset_scan(x,y)
                time.sleep(delay)
                dt=timeout-datetime.now()
                s=int(gxsm.rtquery ("s")[0])
        if dt <= timedelta(seconds = 0):
                return False # ready check failed
        else:
                return True # OK
        
# Note start time
starttime = datetime.now()
x=0
for y in positionsy:
        if not ready_check('start line pre check failed',x,y):
                break
        
        for x in positions:
        
                if nskip > 0:
                        nskip=nskip-1
                        continue
                now = datetime.now()  # some time reporting and estimates as very long
                if m>1:
                        print ('Elapsed: ', (now-starttime) )
                        print ('per run: ', (now-starttime)/m )
                        print ('ETA: ', now+(now-starttime)/m*(n*n*reps-m) )
                        
                if not ready_check('pre check: XY move ready failed',x,y):
                        break
               		         
                print('Tip to: [', x,y,']  @', now.timestamp())
                gxsm.moveto_scan_xy(x,y) # this always completes the move before returning, no need to wait
                time.sleep(0.2)
                gxsm.add_marker_object (0,'xy#'+str(m),0,0,0,0.2) # add current and actual DSP tip pos marker at scale 0.2 to scan channel 1 (index=0)

                if not ready_check('XY move failed',x,y):
                        break
                        
                if exec_probe:
                        for i in range(1,reps):
                                    if not ready_check('pre check ready for VP failed',x,y):
                                           break
                                    m=m+1
                                    print("VP[",i,m,"]")

                                    #gxsm.action("DSP_VP_RCL_VPD")
                                    gxsm.action("DSP_VP_GVP_EXECUTE")

                                    print("Zzzz1")
                                    time.sleep(2.0)  # give time to actually start GVP!

                                    if not ready_check('waiting for VP', x,y, 0.5, 120.0):
                                            break

                                    print("Zzzz2")
                                    time.sleep(2.0)  # give time to store  GVP!
                                    reset_scan(x,y)
