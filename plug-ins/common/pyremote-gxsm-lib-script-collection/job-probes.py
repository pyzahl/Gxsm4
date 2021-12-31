#GXSM_USE_LIBRARY <gxsm3-lib-probe>

#print np.arange(-0.1,-1.0,-0.1)
#[-0.1 -0.2 -0.3 -0.4 -0.5 -0.6 -0.7 -0.8 -0.9]

#ui = [ 
#0.1 20 
#0.5 80
#1.0 100
#2.0 200]

wait=50

probe = probe_control()

#        def run_IV_forall_points (self, ref_ch=0):

#probe.run_IV_forall_points ()

scan = scan_control()

scan.run_set (np.arange(-1.8, -2.0, -0.1))
scan.run_set (np.arange(-1.2, -1.8, -0.1))
scan.run_set (np.arange(-0.4, -2.6, -0.2))

#gxsm.set("dsp-fbs-bias","-0.65")
#gxsm.set("dsp-fbs-bias","-0.65")
#gxsm.set("dsp-fbs-mx0-current-set","0.1")
#gxsm.set("dsp-fbs-mx0-current-set","0.1")
#gxsm.set("dsp-fbs-bias","-0.2")
#gxsm.sleep(wait)
#scan.run_set([0.20])
gxsm.set("dsp-fbs-mx0-current-set","0.01")
gxsm.set("dsp-fbs-mx0-current-set","0.01")
gxsm.set("dsp-fbs-bias","0.7")
gxsm.set("dsp-fbs-bias","0.7")
gxsm.set("dsp-fbs-mx0-current-set","0.25")
gxsm.set("dsp-fbs-mx0-current-set","0.25")
gxsm.set("dsp-fbs-bias","0.7")
gxsm.sleep(wait)
scan.run_set (np.arange(0.4, 2.6,0.2))
#scan.run_set([0.7, 0.9, 1.5])
#gxsm.set("dsp-fbs-bias","1.3")
#gxsm.set("dsp-fbs-bias","1.3")
#gxsm.set("dsp-fbs-mx0-current-set","0.25")
#gxsm.set("dsp-fbs-mx0-current-set","0.25")
#gxsm.set("dsp-fbs-bias","1.3")
#gxsm.sleep(wait)
#scan.run_set (np.arange(1.3, 1.5,0.1))

#gxsm.set("dsp-fbs-bias","-0.2")
#gxsm.set("dsp-fbs-bias","-0.2")
#gxsm.set("dsp-fbs-mx0-current-set","0.2")
#gxsm.set("dsp-fbs-mx0-current-set","0.2")
#gxsm.set("dsp-fbs-bias","-0.2")
#gxsm.sleep(wait)
#scan.run_set (np.arange(-0.20, -1.0,-0.1))

#gxsm.set("dsp-fbs-bias","-1.0")
#gxsm.set("dsp-fbs-bias","-1.0")
#gxsm.set("dsp-fbs-mx0-current-set","0.250")
#gxsm.set("dsp-fbs-mx0-current-set","0.250")
#gxsm.sleep(wait)
#scan.run_set (np.arange(-1.0, -2.5,-0.1))


#gxsm.set("dsp-fbs-mx0-current-set","0.20")
#gxsm.set("dsp-fbs-mx0-current-set","0.20")
#gxsm.set("dsp-fbs-bias","0.55")
#gxsm.set("dsp-fbs-bias","0.4")
#gxsm.set("dsp-fbs-bias","0.4")
#gxsm.sleep(wait)
#scan.run_set (np.arange(0.6,1.0,0.))


#gxsm.set("dsp-fbs-bias","1.0")
#gxsm.set("dsp-fbs-bias","1.0")
#gxsm.set("dsp-fbs-mx0-current-set","0.25")
#gxsm.set("dsp-fbs-mx0-current-set","0.25")
#gxsm.sleep(wait)
#scan.run_set (np.arange(1.0,2.5, 0.3))

gxsm.set("dsp-fbs-mx0-current-set","0.03")
gxsm.set("dsp-fbs-mx0-current-set","0.03")
gxsm.set("dsp-fbs-bias","0.1")
gxsm.set("dsp-fbs-bias","0.1")
