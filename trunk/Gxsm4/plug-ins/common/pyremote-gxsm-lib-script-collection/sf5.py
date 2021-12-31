# Example python action script file /home/percy/.gxsm3/pyaction/sf5.py was created.
# - see the Gxsm3 manual for more information
#gxsm.set ("dsp-fbs-bias","0.1") # set Bias to 0.1V
#gxsm.set ("dsp-fbs-mx0-current-set","0.01") # Set Current Setpoint to 0.01nA
#gxsm.set ("dsp-LCK-AC-Bias-Amp","0.0") # OFF -- 0mV AC modulation on bias

# wait for VP finished and check/return Motor value/abort cond.
def wait_for_vp ():
        vpaction = 1
        M = 0.
        while vpaction > 0 and M > -2.:
	        gxsm.sleep (10) # sleep 10/10 sec
 		M=gxsm.get ("dsp-fbs-motor")
 	      	svec=gxsm.rtquery ("s")
		s = int(svec[0])
		vpaction = s&8
	        if os.path.exists("remote.py-stop"):
	        	M = -10.
			break
	print "FB: ", s&1, " Scan: ", s&(2+4), "  VP: ", s&8, "   Mov: ", s&16, " PLL: ", s&32, " **Motor=", M
	gxsm.sleep(20)
	return M


dz=20
x=gxsm.get("OffsetX")
#gxsm.set("OffsetX","%g"%(x+1500))
gxsm.set("OffsetX","%g"%(-1500))
gxsm.sleep(20)
gxsm.set ("dsp-gvp-dz02","%g"%dz) # set Bias to 0.1V
gxsm.set ("dsp-gvp-dz04","%g"%(-dz)) # set Bias to 0.1V
gxsm.action("DSP_VP_GVP_EXECUTE")
wait_for_vp ()
gxsm.sleep (20)  # sleep for 2/10s
gxsm.set("OffsetX","%g"%x)