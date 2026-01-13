import numpy as np
import time

if 0:
	for h in gxsm.list_actions ():
		print (h)
	print ('*')

n=np.zeros(16)
dt=np.zeros(16)


gxsm.action("UNCHECK-SCAN-REPEAT")
gxsm.action("UNCHECK-MainAutoSave")

for i in range(0,10):
	n[i]=gxsm.get("dsp-gvp-n{:02d}".format(i))
	dt[i]=gxsm.get("dsp-gvp-dt{:02d}".format(i))
	gxsm.set("dsp-gvp-n{:02d}".format(i), '{}'.format(100+i))
	gxsm.set("dsp-gvp-dt{:02d}".format(i), '{}'.format(2))
	gxsm.action("CHECK-dsp-gvp-FB{:02d}".format(i))
	gxsm.action("CHECK-dsp-gvp-VS{:02d}".format(i))

gxsm.sleep(10)

for i in range(9,-1,-1):
	gxsm.set("dsp-gvp-n{:02d}".format(i), '{}'.format(n[i]))
	gxsm.set("dsp-gvp-dt{:02d}".format(i), '{}'.format(dt[i]))
	gxsm.action("UNCHECK-dsp-gvp-FB{:02d}".format(i))
	gxsm.action("UNCHECK-dsp-gvp-VS{:02d}".format(i))

gxsm.action("CHECK-MainAutoSave")
gxsm.action("CHECK-SCAN-REPEAT")
