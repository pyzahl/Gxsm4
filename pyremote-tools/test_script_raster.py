import time
import numpy as np
from datetime import datetime

r=400  # range
n=51    # num positions
reps = 15 # repetitions

m=0
# print positions

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

starttime = datetime.now()
for y in np.arange(-r/2, r/2+1, r/(n-1)):
        if gxsm.get('script-control') !=1: # User Cancel via Script-Control
                break
        if int(gxsm.rtquery ("s")[0])&(8+16+2+4) and gxsm.get('script-control'):
                print ('pre check: ready failed')
                break
        for x in np.arange(-r/2, r/2+1, r/(n-1)):
                now = datetime.now()
                if m>1:
                        print ('Elapsed: ', (now-starttime) )
                        print ('per run: ', (now-starttime)/m )
                        print ('ETA: ', now+(now-starttime)/m*(n*n*reps-m) )
                print('Tip to: [', x,y,']  @', now.timestamp())
                if gxsm.get('script-control') !=1:
                        break
                        
                if int(gxsm.rtquery ("s")[0])&(8+16+2+4) and gxsm.get('script-control'):
                        print ('pre check: XY ready failed.')
                        break
               		         
                gxsm.moveto_scan_xy(x,y) # this always completes the move before returning, no need to wait
                gxsm.add_marker_object (0,'xy#'+str(m),0,0,0,0.2) # add current and actual DSP tip pos marker at scale 0.2 to scan channel 1 (index=0)

                if int(gxsm.rtquery ("s")[0])&(8+16+2+4) and gxsm.get('script-control'):
                        print ('XY move failed.')
                        break
                        
                if exec_probe:
                        for i in range(1,reps):
                                    if gxsm.get('script-control') !=1:
                                            break
                                    m=m+1
                                    print("VP[",i,m,"]")
                                    #gxsm.action("DSP_VP_RCL_VPD")
                                    while int(gxsm.rtquery ("s")[0])&(8+16+2+4) and gxsm.get('script-control'):
                                            print ('pre check VP ready...')
                                            time.sleep(0.5)
                                    gxsm.action("DSP_VP_GVP_EXECUTE")
                                    print("Zzzz1")
                                    time.sleep(2.0)  # give time to actually start GVP!
                                    while int(gxsm.rtquery ("s")[0])&(8+16+2+4) and gxsm.get('script-control'):
                                            print ('waiting for VP...')
                                            time.sleep(0.5)
                                    print("Zzzz2")
                                    time.sleep(2.0)  # give time to store  GVP!