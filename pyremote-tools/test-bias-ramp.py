import numpy as np
import time

gxsm.set ("dsp-fbs-bias","0.0") 
time.sleep(1)

for bias in np.arange(0,5.1,0.1):
	gxsm.set ("dsp-fbs-bias","{}".format(bias)) 
	time.sleep(0.01)

time.sleep(1)
gxsm.set ("dsp-fbs-bias","0.1") 
