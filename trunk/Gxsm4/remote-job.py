#GXSM_USE_LIBRARY <gxsm3-lib-scan>

scan = scan_control()
tc = tip_control()

# scan a mosaic set of images
gxsm.set("RangeX","700")
gxsm.set("PointsX","500")
c= tc.make_coords(660, 4)
scan.run_mosaic(c, [-1400, -1400])

#c= tc.make_coords(1600, 3)
#scan.run_mosaic(c, [-1600, -1600])


#print np.arange(-0.1,-1.0,-0.1)
#[-0.1 -0.2 -0.3 -0.4 -0.5 -0.6 -0.7 -0.8 -0.9]

# run series of bias values
scan.run_set([-1.25, -0.97, -0.7, -0.4, -0.2, 0.2, 0.5, 0.92,  0.28, 0.43, 0.57, 0.85, 1.20 ])

#scan.run_set(np.arange(-1.5,-0.1,0.05))
#scan.run_set(np.arange(0.05,1.5,0.05))

#while gxsm.waitscan() >= 0:
#	gxsm.sleep (50)
#	print (gxsm.waitscan())
#scan.run_set([-1.2])
#while gxsm.waitscan() >= 0:
#	gxsm.sleep (50)
#	print (gxsm.waitscan())

#gxsm.set("dsp-fbs-mx0-current-set","0.02")
#scan.run_set (np.arange(0.05, 0.2,0.05))

#gxsm.set("dsp-fbs-mx0-current-set","0.035")
#scan.run_set (np.arange(0.2, 1.6,0.2))

#scan.run_set (np.arange(0.1, 0.8,0.1))



#gxsm.set("dsp-fbs-bias","-0.65")
#gxsm.set("dsp-fbs-bias","-0.65")
#gxsm.set("dsp-fbs-mx0-current-set","0.1")
#gxsm.set("dsp-fbs-mx0-current-set","0.1")
#gxsm.set("dsp-fbs-bias","-0.2")
#gxsm.sleep(wait)
#scan.run_set([0.20])
#gxsm.set("dsp-fbs-bias","0.7")
#gxsm.set("dsp-fbs-bias","0.7")
#gxsm.set("dsp-fbs-mx0-current-set","0.25")
#gxsm.set("dsp-fbs-mx0-current-set","0.25")
#gxsm.set("dsp-fbs-bias","0.7")
#gxsm.sleep(wait)
#scan.run_set (np.arange(0.2, 2.6,0.2))
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

#gxsm.set("dsp-fbs-mx0-current-set","0.03")
#gxsm.set("dsp-fbs-mx0-current-set","0.03")
#gxsm.set("dsp-fbs-bias","0.1")
#gxsm.set("dsp-fbs-bias","0.1")
